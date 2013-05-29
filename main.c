#include <stdio.h>
#include <getopt.h>
#include <stdbool.h>

#include <libubox/blobmsg_json.h>
#include <libubox/avl.h>
#include <libubox/avl-cmp.h>
#include <libusb.h>

#define DEFAULT_CONFIG "/etc/usb-mode.json"

struct device {
	struct avl_node avl;
	struct blob_attr *data;
};

struct usbdev_data {
	struct libusb_device_descriptor desc;
	libusb_device_handle *devh;
	struct blob_attr *info;

	char idstr[10];
	char mfg[128], prod[128], serial[128];
};

static int verbose = 0;
static const char *config_file = DEFAULT_CONFIG;
static struct blob_buf conf;

static struct blob_attr **messages;
static int n_messages;

static struct avl_tree devices;

static struct libusb_context *usb;
static struct libusb_device **usbdevs;
static int n_usbdevs;

static int parse_config(void)
{
	enum {
		CONF_MESSAGES,
		CONF_DEVICES,
		__CONF_MAX
	};
	static const struct blobmsg_policy policy[__CONF_MAX] = {
		[CONF_MESSAGES] = { .name = "messages", .type = BLOBMSG_TYPE_ARRAY },
		[CONF_DEVICES] = { .name = "devices", .type = BLOBMSG_TYPE_TABLE },
	};
	struct blob_attr *tb[__CONF_MAX];
	struct blob_attr *cur;
	struct device *dev;
	int rem;

	blobmsg_parse(policy, __CONF_MAX, tb, blob_data(conf.head), blob_len(conf.head));
	if (!tb[CONF_MESSAGES] || !tb[CONF_DEVICES]) {
		fprintf(stderr, "Configuration incomplete\n");
		return -1;
	}

	blobmsg_for_each_attr(cur, tb[CONF_MESSAGES], rem)
		n_messages++;

	messages = calloc(n_messages, sizeof(*messages));
	n_messages = 0;
	blobmsg_for_each_attr(cur, tb[CONF_MESSAGES], rem)
		messages[n_messages++] = cur;

	blobmsg_for_each_attr(cur, tb[CONF_DEVICES], rem) {
	    dev = calloc(1, sizeof(*dev));
	    dev->avl.key = blobmsg_name(cur);
	    dev->data = cur;
	    avl_insert(&devices, &dev->avl);
	}

	return 0;
}

static int usage(const char *prog)
{
	fprintf(stderr, "Usage: %s <command> <options>\n"
		"Commands:\n"
		"	-l		List matching devices\n"
		"	-s		Modeswitch matching devices\n"
		"\n"
		"Options:\n"
		"	-v		Verbose output\n"
		"	-c <file>	Set configuration file to <file> (default: %s)\n"
		"\n", prog, DEFAULT_CONFIG);
	return 1;
}

typedef void (*cmd_cb_t)(struct usbdev_data *data);

static struct blob_attr *
find_dev_data(struct usbdev_data *data, struct device *dev)
{
	struct blob_attr *cur;
	int rem;

	blobmsg_for_each_attr(cur, dev->data, rem) {
		const char *name = blobmsg_name(cur);
		const char *next;
		char *val;

		if (!strcmp(blobmsg_name(cur), "*"))
			return cur;

		next = strchr(name, '=');
		if (!next)
			continue;

		next++;
		if (!strncmp(name, "uMa", 3)) {
			val = data->mfg;
		} else if (!strncmp(name, "uPr", 3)) {
			val = data->prod;
		} else if (!strncmp(name, "uSe", 3)) {
			val = data->serial;
		} else {
			/* ignore unsupported scsi attributes */
			return cur;
		}

		if (!strcmp(val, next))
			return cur;
	}

	return NULL;
}

static void iterate_devs(cmd_cb_t cb)
{
	struct usbdev_data data;
	struct device *dev;
	int i;

	if (!cb)
		return;

	for (i = 0; i < n_usbdevs; i++) {
		memset(&data, 0, sizeof(data));

		if (libusb_get_device_descriptor(usbdevs[i], &data.desc))
			continue;

		sprintf(data.idstr, "%04x:%04x", data.desc.idVendor, data.desc.idProduct);

		dev = avl_find_element(&devices, data.idstr, dev, avl);
		if (!dev)
			continue;

		if (libusb_open(usbdevs[i], &data.devh))
			continue;

		libusb_get_string_descriptor_ascii(
			data.devh, data.desc.iManufacturer,
			(void *) data.mfg, sizeof(data.mfg));
		libusb_get_string_descriptor_ascii(
			data.devh, data.desc.iProduct,
			(void *) data.prod, sizeof(data.prod));
		libusb_get_string_descriptor_ascii(
			data.devh, data.desc.iSerialNumber,
			(void *) data.serial, sizeof(data.serial));

		data.info = find_dev_data(&data, dev);
		if (data.info)
			cb(&data);
		libusb_close(data.devh);
	}
}

static void handle_list(struct usbdev_data *data)
{
	fprintf(stderr, "Found device: %s (Manufacturer: \"%s\", Product: \"%s\", Serial: \"%s\")\n",
		data->idstr, data->mfg, data->prod, data->serial);
}

static void handle_switch(struct usbdev_data *data)
{
}

int main(int argc, char **argv)
{
	cmd_cb_t cb = NULL;
	int ret;
	int ch;

	avl_init(&devices, avl_strcmp, false, NULL);

	while ((ch = getopt(argc, argv, "lsc:v")) != -1) {
		switch (ch) {
		case 'l':
			cb = handle_list;
			break;
		case 's':
			cb = handle_switch;
			break;
		case 'c':
			config_file = optarg;
			break;
		case 'v':
			verbose++;
			break;
		default:
			return usage(argv[0]);
		}
	}

	blob_buf_init(&conf, 0);
	if (!blobmsg_add_json_from_file(&conf, config_file) ||
	    parse_config()) {
		fprintf(stderr, "Failed to load config file\n");
		return 1;
	}

	ret = libusb_init(&usb);
	if (ret) {
		fprintf(stderr, "Failed to initialize libusb: %s\n", libusb_error_name(ret));
		return 1;
	}

	n_usbdevs = libusb_get_device_list(usb, &usbdevs);
	iterate_devs(cb);
	libusb_free_device_list(usbdevs, 1);
	libusb_exit(usb);

	return 0;
}
