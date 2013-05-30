#include "switch.h"

enum {
	DATA_MODE,
	DATA_MSG,
	DATA_INTERFACE,
	__DATA_MAX
};

static void handle_generic(struct usbdev_data *data, struct blob_attr **tb)
{
	fprintf(stderr, "Do generic switch!\n");
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
	/* TODO */
}

static void handle_kobil(struct usbdev_data *data, struct blob_attr **tb)
{
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
	/* TODO */
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
	};
	struct blob_attr *tb[__DATA_MAX];
	int mode = MODE_GENERIC;

	blobmsg_parse(data_policy, __DATA_MAX, tb, blobmsg_data(data->info), blobmsg_data_len(data->info));

	if (tb[DATA_INTERFACE])
		data->interface = blobmsg_get_u32(tb[DATA_INTERFACE]);

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
}
