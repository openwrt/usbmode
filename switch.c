#include <unistd.h>
#include "switch.h"

enum {
	DATA_MODE,
	DATA_MSG,
	DATA_INTERFACE,
	DATA_MSG_EP,
	DATA_RES_EP,
	DATA_RESPONSE,
	DATA_RELEASE_DELAY,
	DATA_CONFIG,
	DATA_ALT,
	__DATA_MAX
};

static void detach_driver(struct usbdev_data *data)
{
	libusb_detach_kernel_driver(data->devh, data->interface);
}

static int send_msg(struct usbdev_data *data, int msg)
{
	int transferred;

	return libusb_bulk_transfer(data->devh, data->msg_endpoint,
				    (void *) messages[msg], message_len[msg],
				    &transferred, 3000);
}

static int read_response(struct usbdev_data *data, int len)
{
	unsigned char *buf;
	int ret, transferred;

	if (len < 13)
		len = 13;
	buf = alloca(len);
	ret = libusb_bulk_transfer(data->devh, data->response_endpoint,
				   buf, len, &transferred, 3000);
	libusb_bulk_transfer(data->devh, data->response_endpoint,
			     buf, 13, &transferred, 100);
	return ret;
}

static void send_messages(struct usbdev_data *data, struct blob_attr *attr)
{
	struct blob_attr *cur;
	int rem;

	libusb_claim_interface(data->devh, data->interface);
	libusb_clear_halt(data->devh, data->msg_endpoint);

	blobmsg_for_each_attr(cur, attr, rem) {
		int msg, len;

		if (blobmsg_type(cur) != BLOBMSG_TYPE_INT32) {
			fprintf(stderr, "Invalid data in message list\n");
			return;
		}

		msg = blobmsg_get_u32(cur);
		if (msg >= n_messages) {
			fprintf(stderr, "Message index out of range!\n");
			return;
		}

		if (send_msg(data, msg)) {
			fprintf(stderr, "Failed to send switch message\n");
			continue;
		}

		if (!data->need_response)
			continue;

		if (!memcmp(messages[msg], "\x55\x53\x42\x43", 4))
			len = 13;
		else
			len = message_len[msg];

		if (read_response(data, len))
			return;
	}

	libusb_clear_halt(data->devh, data->msg_endpoint);
	libusb_clear_halt(data->devh, data->response_endpoint);

	usleep(200000);

	if (data->release_delay)
		usleep(data->release_delay * 1000);

	libusb_release_interface(data->devh, data->interface);
	return;
}

static void handle_generic(struct usbdev_data *data, struct blob_attr **tb)
{
	detach_driver(data);
	send_messages(data, tb[DATA_MSG]);
}

static void handle_huawei(struct usbdev_data *data, struct blob_attr **tb)
{
	/* TODO */
}

static void handle_sierra(struct usbdev_data *data, struct blob_attr **tb)
{
	/* TODO */
}

static void handle_sony(struct usbdev_data *data, struct blob_attr **tb)
{
	/* TODO */
}

static void handle_qisda(struct usbdev_data *data, struct blob_attr **tb)
{
	/* TODO */
}

static void handle_gct(struct usbdev_data *data, struct blob_attr **tb)
{
	detach_driver(data);
	/* TODO */
}

static void handle_kobil(struct usbdev_data *data, struct blob_attr **tb)
{
	detach_driver(data);
	/* TODO */
}

static void handle_sequans(struct usbdev_data *data, struct blob_attr **tb)
{
	/* TODO */
}

static void handle_mobile_action(struct usbdev_data *data, struct blob_attr **tb)
{
	/* TODO */
}

static void handle_cisco(struct usbdev_data *data, struct blob_attr **tb)
{
	detach_driver(data);
	/* TODO */
}

static void set_alt_setting(struct usbdev_data *data, int setting)
{
	if (libusb_claim_interface(data->devh, data->interface))
		return;

	libusb_set_interface_alt_setting(data->devh, data->interface, setting);
	libusb_release_interface(data->devh, data->interface);
}

enum {
	MODE_GENERIC,
	MODE_HUAWEI,
	MODE_SIERRA,
	MODE_SONY,
	MODE_QISDA,
	MODE_GCT,
	MODE_KOBIL,
	MODE_SEQUANS,
	MODE_MOBILE_ACTION,
	MODE_CISCO,
	__MODE_MAX
};

static const struct {
	const char *name;
	void (*cb)(struct usbdev_data *data, struct blob_attr **tb);
} modeswitch_cb[__MODE_MAX] = {
	[MODE_GENERIC] = { "Generic", handle_generic },
	[MODE_HUAWEI] = { "Huawei", handle_huawei },
	[MODE_SIERRA] = { "Sierra", handle_sierra },
	[MODE_SONY] = { "Sony", handle_sony },
	[MODE_QISDA] = { "Qisda", handle_qisda },
	[MODE_GCT] = { "GCT", handle_gct },
	[MODE_KOBIL] = { "Kobil", handle_kobil },
	[MODE_SEQUANS] = { "Sequans", handle_sequans },
	[MODE_MOBILE_ACTION] = { "MobileAction", handle_mobile_action },
	[MODE_CISCO] = { "Cisco", handle_cisco },
};

void handle_switch(struct usbdev_data *data)
{
	static const struct blobmsg_policy data_policy[__DATA_MAX] = {
		[DATA_MODE] = { .name = "mode", .type = BLOBMSG_TYPE_STRING },
		[DATA_MSG] = { .name = "msg", .type = BLOBMSG_TYPE_ARRAY },
		[DATA_INTERFACE] = { .name = "interface", .type = BLOBMSG_TYPE_INT32 },
		[DATA_MSG_EP] = { .name = "msg_endpoint", .type = BLOBMSG_TYPE_INT32 },
		[DATA_RES_EP] = { .name = "response_endpoint", .type = BLOBMSG_TYPE_INT32 },
		[DATA_RESPONSE] = { .name = "response", .type = BLOBMSG_TYPE_INT32 },
		[DATA_CONFIG] = { .name = "config", .type = BLOBMSG_TYPE_INT32 },
		[DATA_ALT] = { .name = "alt", .type = BLOBMSG_TYPE_INT32 },
	};
	struct blob_attr *tb[__DATA_MAX];
	int mode = MODE_GENERIC;

	blobmsg_parse(data_policy, __DATA_MAX, tb, blobmsg_data(data->info), blobmsg_data_len(data->info));

	if (tb[DATA_INTERFACE])
		data->interface = blobmsg_get_u32(tb[DATA_INTERFACE]);

	if (tb[DATA_MSG_EP])
		data->msg_endpoint = blobmsg_get_u32(tb[DATA_MSG_EP]);

	if (tb[DATA_RES_EP])
		data->response_endpoint = blobmsg_get_u32(tb[DATA_RES_EP]);

	if (tb[DATA_RELEASE_DELAY])
		data->release_delay = blobmsg_get_u32(tb[DATA_RELEASE_DELAY]);

	if (tb[DATA_RESPONSE])
		data->need_response = blobmsg_get_bool(tb[DATA_RESPONSE]);

	if (tb[DATA_MODE]) {
		const char *modestr;
		int i;

		modestr = blobmsg_data(tb[DATA_MODE]);
		for (i = 0; i < __MODE_MAX; i++) {
			if (strcmp(modeswitch_cb[i].name, modestr) != 0)
				continue;

			mode = i;
			break;
		}
	}

	modeswitch_cb[mode].cb(data, tb);

	if (tb[DATA_CONFIG]) {
		int config, config_new;

		config_new = blobmsg_get_u32(tb[DATA_CONFIG]);
		if (libusb_get_configuration(data->devh, &config) ||
		    config != config_new)
			libusb_set_configuration(data->devh, config_new);
	}

	if (tb[DATA_ALT]) {
		int new = blobmsg_get_u32(tb[DATA_ALT]);
		set_alt_setting(data, new);
	}
}
