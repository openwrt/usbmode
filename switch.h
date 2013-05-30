#ifndef __USBMODE_SWITCH_H
#define __USBMODE_SWITCH_H

#include <libubox/blobmsg.h>
#include <libusb.h>

struct usbdev_data {
	struct libusb_device_descriptor desc;
	libusb_device_handle *devh;
	struct blob_attr *info;

	char idstr[10];
	char mfg[128], prod[128], serial[128];
};

enum {
	DATA_MODE,
	DATA_MSG,
	DATA_MSG2,
	DATA_MSG3,
	__DATA_MAX
};

void handle_switch(struct usbdev_data *data);

#endif
