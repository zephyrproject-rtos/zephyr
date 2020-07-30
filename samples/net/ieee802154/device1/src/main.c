/*
 * Copyright (c) 2020 Nordic Semmiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define LOG_LEVEL 4
#include <logging/log.h>
LOG_MODULE_REGISTER(ieee802154_test_dev1);

#include <stdio.h>
#include <zephyr.h>
#include <net/buf.h>
#include <stdlib.h>
#include <net/ieee802154_radio.h>
#include <ieee802154/ieee802154_frame.h>
#include <net_private.h>
#include "config_parser.h"
#include "posix_board_if.h"

#define CFG_COLS_OFFSET 4
#define TEST_CONFIG_FILENAME "../../../test_data"

struct ieee802154_test_config {
	uint8_t ieee_addr[8];
	uint8_t short_addr[2];
	uint8_t pan_id[2];
	bool is_pan_coord;
};

struct ieee802154_test {
	struct ieee802154_test_config *configs;
	int ctr;
	int ntests;
};

static struct ieee802154_radio_api *radio_api;
static struct device *ieee802154_dev;
static struct ieee802154_test test_data = { .ctr = 0,
					    .ntests = 0,
					    .configs = NULL };

static void radio_configure(struct ieee802154_test_config *config);

static int load_test_data(const char *config_file,
			  struct ieee802154_test *test_data)
{
	int configs_ctr;
	int tests_num;
	struct config_parser tests_parser = {
		.ncols = 10,
		.datatypes = { CSV_DATATYPE_BYTESTREAM, CSV_DATATYPE_BYTESTREAM,
			       CSV_DATATYPE_BYTESTREAM, CSV_DATATYPE_BOOLEAN,
			       CSV_DATATYPE_BYTESTREAM, CSV_DATATYPE_BYTESTREAM,
			       CSV_DATATYPE_BYTESTREAM, CSV_DATATYPE_BOOLEAN,
			       CSV_DATATYPE_BYTESTREAM,
			       CSV_DATATYPE_BYTESTREAM }
	};

	if (!config_openfile(config_file)) {
		return -EIO;
	}

	tests_num = get_config_lines_number();

	test_data->configs =
		malloc(sizeof(struct ieee802154_test_config) * tests_num);
	if (!test_data->configs) {
		config_closefile();
		return -ENOMEM;
	}

	test_data->ctr = 0;

	configs_ctr = 0;
	for (;; configs_ctr++) {
		if (!config_parse_line(&tests_parser)) {
			config_parser_clean(&tests_parser);
			break;
		}

		memcpy(test_data->configs[configs_ctr].ieee_addr,
		       tests_parser.dest[CFG_COLS_OFFSET + 0].bytestream + 1,
		       8);
		memcpy(test_data->configs[configs_ctr].short_addr,
		       tests_parser.dest[CFG_COLS_OFFSET + 1].bytestream + 1,
		       2);
		memcpy(test_data->configs[configs_ctr].pan_id,
		       tests_parser.dest[CFG_COLS_OFFSET + 2].bytestream + 1,
		       2);

		test_data->configs[configs_ctr].is_pan_coord =
			*tests_parser.dest[3].boolean;

		config_parser_clean(&tests_parser);
	}

	test_data->ntests = configs_ctr;
	config_closefile();

	return configs_ctr;
}

static void example_cleanup(void)
{
	if (test_data.configs) {
		free(test_data.configs);
	}
}

int net_recv_data(struct net_if *iface, struct net_pkt *pkt)
{
	size_t len = net_pkt_get_len(pkt);
	uint8_t rx_buf[127];
	uint8_t *p = rx_buf;
	int ret;

	LOG_INF("Got data, pkt %p, len %d", pkt, len);

	net_pkt_hexdump(pkt, "<");

	if (len > (127 - 2)) {
		LOG_ERR("Too large packet");
		ret = -ENOMEM;
		goto out;
	}

	/* Add length 1 byte */
	*p++ = (uint8_t)len;

	/* This is needed to work with pkt */
	net_pkt_cursor_init(pkt);

	ret = net_pkt_read(pkt, p, len);
	if (ret < 0) {
		LOG_ERR("Cannot read pkt");
		goto out;
	}

	p += len;

	/* Add LQI at the end of the packet */
	*p = net_pkt_ieee802154_lqi(pkt);

out:
	net_pkt_unref(pkt);
	radio_configure(&test_data.configs[test_data.ctr++]);

	return ret;
}

/* Configure addresses of this device */
static void device_addr_set(const uint8_t *addr, bool extended, bool pan)
{
	struct ieee802154_filter filter;

	if (pan) {
		filter.pan_id = *(uint16_t *)addr;
		radio_api->filter(ieee802154_dev, true,
				  IEEE802154_FILTER_TYPE_PAN_ID, &filter);
	} else if (extended) {
		filter.ieee_addr = (uint8_t *)addr;
		radio_api->filter(ieee802154_dev, true,
				  IEEE802154_FILTER_TYPE_IEEE_ADDR, &filter);
	} else {
		filter.short_addr = *(uint16_t *)addr;
		radio_api->filter(ieee802154_dev, true,
				  IEEE802154_FILTER_TYPE_SHORT_ADDR, &filter);
	}
}

/* Set coordinator role of the device */
static void set_coord_role(bool enabled)
{
	const struct ieee802154_config config = { .pan_coordinator = enabled };

	radio_api->configure(ieee802154_dev, IEEE802154_CONFIG_PAN_COORDINATOR,
			     &config);
}

static void radio_configure(struct ieee802154_test_config *config)
{
	set_coord_role(config->is_pan_coord);
	device_addr_set(config->ieee_addr, true, false);
	device_addr_set(config->pan_id, false, true);
	device_addr_set(config->short_addr, false, false);
}

static void example_init(void)
{
	enum ieee802154_config_type config_types[] = {
		IEEE802154_CONFIG_AUTO_ACK_FPB, IEEE802154_CONFIG_PROMISCUOUS
	};
	const struct ieee802154_config configs[] = {
		{ .auto_ack_fpb.enabled = true,
		  .auto_ack_fpb.mode = IEEE802154_FPB_ADDR_MATCH_ZIGBEE },
		{ .promiscuous = true }
	};

	uint8_t channel = 20;
	int8_t power = -12;

	net_pkt_init();
	ieee802154_dev =
		device_get_binding(CONFIG_IEEE802154_NATIVE_POSIX_DRV_NAME);

	if (ieee802154_dev) {
		radio_api = (struct ieee802154_radio_api *)ieee802154_dev->api;
	} else {
		printk("Couldn't bind %s\n",
		       CONFIG_IEEE802154_NATIVE_POSIX_DRV_NAME);
		posix_exit(-EFAULT);
	}

	printf("%10s\n", "Radio configuration:");
	printf("%-10s %u\n", "Channel:", channel);
	printf("%-10s %d dBm\n", "Power:", power);

	radio_api->set_channel(ieee802154_dev, channel);
	radio_api->set_txpower(ieee802154_dev, power);

	for (int i = 0; i < ARRAY_SIZE(config_types); i++) {
		radio_api->configure(ieee802154_dev, config_types[i],
				     &configs[i]);
	}

	radio_configure(&test_data.configs[test_data.ctr++]);
	int status = radio_api->start(ieee802154_dev);

	if (status) {
		printf("Radio couldn't be started - make sure "
		       "that program was started in babble sim mode (-bsim)");
		posix_exit(-EFAULT);
	}
}

void main(void)
{
	/* Read the test config file */
	load_test_data(TEST_CONFIG_FILENAME, &test_data);

	/* Do the device binding, start the radio and initialize net_pkt */
	example_init();

	k_sleep(K_FOREVER);

	example_cleanup();

	/* Terminate */
	posix_exit(0);
}
