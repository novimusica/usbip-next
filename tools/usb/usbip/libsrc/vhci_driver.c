/*
 * Copyright (C) 2015 Nobuo Iwata
 *               2005-2007 Takahiro Hirofuchi
 */

#include "usbip_common.h"
#include "vhci_driver.h"
#include <limits.h>
#include <netdb.h>
#include <libudev.h>
#include <fcntl.h>
#include <errno.h>
#include "sysfs_utils.h"

#undef  PROGNAME
#define PROGNAME "libusbip"

static struct udev_device *vhci_hc_device;
static struct udev *udev_context;
static int vhci_nports;

struct usbip_vhci_device {
	int port;
	uint32_t status;

	uint32_t devid;

	uint8_t busnum;
	uint8_t devnum;

	/* usbip_class_device list */
	struct usbip_usb_device udev;
};

static int imported_device_init(struct usbip_vhci_device *vdev, char *busid)
{
	struct udev_device *sudev;

	sudev = udev_device_new_from_subsystem_sysname(udev_context,
						       "usb", busid);
	if (!sudev) {
		dbg("udev_device_new_from_subsystem_sysname failed: %s", busid);
		return -1;
	}
	read_usb_device(sudev, &vdev->udev);
	udev_device_unref(sudev);

	return 0;
}

struct status_context {
	int controller;
	const char *c;
};

#define OPEN_MODE_FIRST      0
#define OPEN_MODE_REOPEN     1

static int open_hc_device(int mode);

#define MAX_STATUS_NAME 16

static int open_status(struct status_context *ctx, int mode)
{
	char name[MAX_STATUS_NAME+1];

	if (mode == OPEN_MODE_FIRST)
		ctx->controller = 0;
	else
		(ctx->controller)++;

	if (open_hc_device(OPEN_MODE_REOPEN))
		return -1;

	if (ctx->controller == 0)
		strcpy(name, "status");
	else
		snprintf(name, MAX_STATUS_NAME + 1,
				"status.%d", ctx->controller);
	ctx->c = udev_device_get_sysattr_value(vhci_hc_device, name);
	if (ctx->c == NULL)
		return -1;

	return 0;
}

static void close_status(struct status_context *ctx)
{
	ctx->c = NULL;
}

static int next_status_line(struct status_context *ctx)
{
	const char *c = ctx->c;

	while ((c = strchr(c, '\n')) == NULL) {
		if (open_status(ctx, OPEN_MODE_REOPEN))
			return -1;
		c = ctx->c;
	}
	ctx->c = c + 1;
	return 0;
}

static int parse_status_line(struct status_context *ctx,
			     struct usbip_vhci_device *vdev)
{
	int port, status, speed, devid;
	unsigned long socket;
	char lbusid[SYSFS_BUS_ID_SIZE];
	int ret;

	if (next_status_line(ctx))
		return -1;

	memset(vdev, 0, sizeof(struct usbip_vhci_device));

	if (ctx->c == NULL || *(ctx->c) == '\0') {
		dbg("no more data to scan");
		return -1;
	}

	ret = sscanf(ctx->c, "%d %d %d %x %lx %31s\n",
				&port, &status, &speed,
				&devid, &socket, lbusid);
	if (ret < 6) {
		dbg("sscanf failed: %d", ret);
		return -1;
	}

	dbg("port %d status %d speed %d devid %x", port, status, speed, devid);
	dbg("socket %lx lbusid %s", socket, lbusid);

	vdev->port	= port;
	vdev->status	= status;
	vdev->devid	= devid;
	vdev->busnum	= (devid >> 16);
	vdev->devnum	= (devid & 0x0000ffff);

	if (vdev->status != VDEV_ST_NULL &&
	    vdev->status != VDEV_ST_NOTASSIGNED) {
		if (imported_device_init(vdev, lbusid)) {
			dbg("imported_device_init failed");
			return -1;
		}
	}

	return 0;
}

static int get_nports(void)
{
	const char *attr_nports;

	attr_nports = udev_device_get_sysattr_value(vhci_hc_device, "nports");
	if (!attr_nports) {
		err("udev_device_get_sysattr_value failed");
		return -1;
	}

	vhci_nports = strtol(attr_nports, NULL, 10);
	if (vhci_nports <= 0) {
		err("invalid nports value");
		return -1;
	}

	return 0;
}

/*
 * Read the given port's record.
 *
 * To avoid buffer overflow we will read the entire line and
 * validate each part's size. The initial buffer is padded by 4 to
 * accommodate the 2 spaces, 1 newline and an additional character
 * which is needed to properly validate the 3rd part without it being
 * truncated to an acceptable length.
 */
static int read_record(int port, char *host, unsigned long host_len,
		char *port_s, unsigned long port_slen, char *busid)
{
	int part;
	FILE *file;
	char path[PATH_MAX+1];
	char *buffer, *start, *end;
	char delim[] = {' ', ' ', '\n'};
	int max_len[] = {(int)host_len, (int)port_slen, SYSFS_BUS_ID_SIZE};
	size_t buffer_len = host_len + port_slen + SYSFS_BUS_ID_SIZE + 4;

	buffer = (char *)malloc(buffer_len);
	if (!buffer)
		return -1;

	snprintf(path, PATH_MAX, VHCI_STATE_PATH"/port%d", port);

	file = fopen(path, "r");
	if (!file) {
		err("fopen %s", path);
		free(buffer);
		return -1;
	}

	if (fgets(buffer, buffer_len, file) == NULL) {
		err("fgets %s", path);
		free(buffer);
		fclose(file);
		return -1;
	}
	fclose(file);

	/* validate the length of each of the 3 parts */
	start = buffer;
	for (part = 0; part < 3; part++) {
		end = strchr(start, delim[part]);
		if (end == NULL || (end - start) > max_len[part]) {
			free(buffer);
			return -1;
		}
		start = end + 1;
	}

	if (sscanf(buffer, "%s %s %s\n", host, port_s, busid) != 3) {
		err("sscanf");
		free(buffer);
		return -1;
	}

	free(buffer);

	return 0;
}

static int open_hc_device(int mode)
{
	if (vhci_hc_device && mode == OPEN_MODE_REOPEN)
		udev_device_unref(vhci_hc_device);

	vhci_hc_device =
		udev_device_new_from_subsystem_sysname(udev_context,
						       USBIP_VHCI_BUS_TYPE,
						       USBIP_VHCI_DRV_NAME);
	if (!vhci_hc_device) {
		err("udev_device_new_from_subsystem_sysname failed");
		return -1;
	}
	return 0;
}

static void close_hc_device(void)
{
	udev_device_unref(vhci_hc_device);
	vhci_hc_device = NULL;
}

/* ---------------------------------------------------------------------- */

int usbip_vhci_driver_open(void)
{
	udev_context = udev_new();
	if (!udev_context) {
		err("udev_new failed");
		goto err_out;
	}

	if (open_hc_device(OPEN_MODE_FIRST))
		goto err_unref_udev;

	if (get_nports() || vhci_nports <= 0) {
		err("failed to get nports");
		goto err_close_hc;
	}
	dbg("available ports: %d", vhci_nports);

	return 0;

err_close_hc:
	close_hc_device();
err_unref_udev:
	udev_unref(udev_context);
	udev_context = NULL;
err_out:
	return -1;
}

void usbip_vhci_driver_close(void)
{
	close_hc_device();

	if (udev_context)
		udev_unref(udev_context);
}

int usbip_vhci_get_free_port(void)
{
	struct status_context context;
	struct usbip_vhci_device vdev;

	if (open_status(&context, OPEN_MODE_FIRST))
		return -1;

	while (!parse_status_line(&context, &vdev)) {
		if (vdev.status == VDEV_ST_NULL) {
			dbg("found free port %d", vdev.port);
			close_status(&context);
			return vdev.port;
		}
	}
	close_status(&context);
	return -1;
}

int usbip_vhci_find_device(const char *host, const char *busid)
{
	int ret;
	char rhost[NI_MAXHOST] = "unknown host";
	char rserv[NI_MAXSERV] = "unknown port";
	char rbusid[SYSFS_BUS_ID_SIZE];
	struct status_context context;
	struct usbip_vhci_device vdev;

	if (open_status(&context, OPEN_MODE_FIRST))
		return -1;

	while (!parse_status_line(&context, &vdev)) {
		if (vdev.status != VDEV_ST_USED)
			continue;

		ret = read_record(vdev.port, rhost, NI_MAXHOST,
				  rserv, NI_MAXSERV, rbusid);
		if (!ret &&
			!strncmp(host, rhost, NI_MAXHOST) &&
			!strncmp(busid, rbusid, SYSFS_BUS_ID_SIZE)) {
			close_status(&context);
			return vdev.port;
		}
	}
	close_status(&context);
	return -1;
}

static int usbip_vhci_attach_device2(int port, int sockfd, uint32_t devid,
		uint32_t speed)
{
	char buff[200]; /* what size should be ? */
	char attach_attr_path[SYSFS_PATH_MAX];
	char attr_attach[] = "attach";
	const char *path;
	int ret;

	snprintf(buff, sizeof(buff), "%u %d %u %u",
			port, sockfd, devid, speed);
	dbg("writing: %s", buff);

	path = udev_device_get_syspath(vhci_hc_device);
	snprintf(attach_attr_path, sizeof(attach_attr_path), "%s/%s",
		 path, attr_attach);
	dbg("attach attribute path: %s", attach_attr_path);

	ret = write_sysfs_attribute(attach_attr_path, buff, strlen(buff));
	if (ret < 0) {
		dbg("write_sysfs_attribute failed");
		return -1;
	}

	dbg("attached port: %d", port);

	return 0;
}

static unsigned long get_devid(uint8_t busnum, uint8_t devnum)
{
	return (busnum << 16) | devnum;
}

/* will be removed */
int usbip_vhci_attach_device(int port, int sockfd, uint8_t busnum,
		uint8_t devnum, uint32_t speed)
{
	int devid = get_devid(busnum, devnum);

	return usbip_vhci_attach_device2(port, sockfd, devid, speed);
}

int usbip_vhci_detach_device(int port)
{
	char detach_attr_path[SYSFS_PATH_MAX];
	char attr_detach[] = "detach";
	char buff[200]; /* what size should be ? */
	const char *path;
	int ret;

	snprintf(buff, sizeof(buff), "%u", port);
	dbg("writing: %s", buff);

	path = udev_device_get_syspath(vhci_hc_device);
	snprintf(detach_attr_path, sizeof(detach_attr_path), "%s/%s",
		 path, attr_detach);
	dbg("detach attribute path: %s", detach_attr_path);

	ret = write_sysfs_attribute(detach_attr_path, buff, strlen(buff));
	if (ret < 0) {
		dbg("write_sysfs_attribute failed");
		return -1;
	}

	dbg("detached port: %d", port);

	return 0;
}

static int usbip_vhci_imported_device_dump(struct usbip_vhci_device *vdev)
{
	char product_name[100];
	char host[NI_MAXHOST] = "unknown host";
	char serv[NI_MAXSERV] = "unknown port";
	char remote_busid[SYSFS_BUS_ID_SIZE];
	int ret;
	int read_record_error = 0;

	if (vdev->status == VDEV_ST_NULL || vdev->status == VDEV_ST_NOTASSIGNED)
		return 0;

	ret = read_record(vdev->port, host, sizeof(host), serv, sizeof(serv),
			  remote_busid);
	if (ret) {
		err("read_record");
		read_record_error = 1;
	}

	printf("Port %02d: <%s> at %s\n", vdev->port,
	       usbip_status_string(vdev->status),
	       usbip_speed_string(vdev->udev.speed));

	usbip_names_get_product(product_name, sizeof(product_name),
				vdev->udev.idVendor, vdev->udev.idProduct);

	printf("       %s\n",  product_name);

	if (!read_record_error) {
		printf("%10s -> usbip://%s:%s/%s\n", vdev->udev.busid,
		       host, serv, remote_busid);
		printf("%10s -> remote bus/dev %03d/%03d\n", " ",
		       vdev->busnum, vdev->devnum);
	} else {
		printf("%10s -> unknown host, remote port and remote busid\n",
		       vdev->udev.busid);
		printf("%10s -> remote bus/dev %03d/%03d\n", " ",
		       vdev->busnum, vdev->devnum);
	}

	return 0;
}

int usbip_vhci_imported_devices_dump(void)
{
	struct status_context context;
	struct usbip_vhci_device vdev;

	if (open_status(&context, OPEN_MODE_FIRST))
		goto err_out;

	while (!parse_status_line(&context, &vdev)) {
		if (vdev.status == VDEV_ST_NULL ||
		    vdev.status == VDEV_ST_NOTASSIGNED)
			continue;

		if (usbip_vhci_imported_device_dump(&vdev))
			goto err_close;
	}
	close_status(&context);

	return 0;
err_close:
	close_status(&context);
err_out:
	return -1;
}

#define MAX_BUFF 100
int usbip_vhci_create_record(const char *host, const char *port_s,
			     const char *busid, int port)
{
	int fd;
	char path[PATH_MAX+1];
	char buff[MAX_BUFF+1];
	int ret;

	ret = mkdir(VHCI_STATE_PATH, 0700);
	if (ret < 0) {
		/* if VHCI_STATE_PATH exists, then it better be a directory */
		if (errno == EEXIST) {
			struct stat s;

			ret = stat(VHCI_STATE_PATH, &s);
			if (ret < 0)
				return -1;
			if (!(s.st_mode & S_IFDIR))
				return -1;
		} else
			return -1;
	}

	snprintf(path, PATH_MAX, VHCI_STATE_PATH"/port%d", port);

	fd = open(path, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
	if (fd < 0)
		return -1;

	snprintf(buff, MAX_BUFF, "%s %s %s\n",
			host, port_s, busid);

	ret = write(fd, buff, strlen(buff));
	if (ret != (ssize_t) strlen(buff)) {
		close(fd);
		return -1;
	}

	close(fd);

	return 0;
}

int usbip_vhci_delete_record(int port)
{
	char path[PATH_MAX+1];

	snprintf(path, PATH_MAX, VHCI_STATE_PATH"/port%d", port);

	remove(path);
	rmdir(VHCI_STATE_PATH);

	return 0;
}
