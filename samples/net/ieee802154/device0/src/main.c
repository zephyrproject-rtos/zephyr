/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL 4
#include <logging/log.h>
LOG_MODULE_REGISTER(ieee802154_test_dev0);

#include <stdio.h>
#include <zephyr.h>
#include <net/buf.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <net/ieee802154_radio.h>
#include <ieee802154/ieee802154_frame.h>
#include <net_private.h>
#include "config_parser.h"

#include "posix_board_if.h"

#define TEST_WAIT_FOR_RX_TIMEOUT_MS (1000)
#define TEST_DATA_FILE "../../../test_data"
#define CFG_COLS_OFFSET (0)

struct ieee802154_test_config {
	uint8_t ieee_addr[8];
	uint8_t short_addr[2];
	uint8_t pan_id[2];
	bool is_pan_coord;
};

struct ieee802154_test {
	/* Data to send */
	uint8_t **tx;

	/* Expected response, NULL if response is not expected */
	uint8_t **expected_rx;

	/**
	 * Actual received response.
	 * Can be read only if rx_wait sem was successfully taken.
	 */
	uint8_t *actual_rx;

	/* Executed tests counter */
	int ctr;

	/* Total number of tests */
	int ntests;

	/**
	 * Semaphore for transmissions and responses synchronisation.
	 * Given by net_recv_data on any reception.
	 */
	struct k_sem rx_wait;

	/* Results of tests*/
	bool *results;

	/* Device config for each test */
	struct ieee802154_test_config *configs;
};

static struct ieee802154_radio_api *radio_api;
static struct device *ieee802154_dev;
static struct ieee802154_test test_data = { .tx = NULL,
					    .expected_rx = NULL,
					    .results = NULL,
					    .ctr = 0,
					    .ntests = 0 };

static int tx(struct net_pkt *pkt)
{
	struct net_buf *buf = net_buf_frag_last(pkt->buffer);
	int ret;

	ret = radio_api->tx(ieee802154_dev, IEEE802154_TX_MODE_DIRECT, pkt,
			    buf);
	if (ret) {
		printk("Error sending data\n");
	}

	return ret;
}

/** Fill the packet with the data
 *
 * Arguments:
 * pkt		-	The pointer that pkt will be allocated on
 * psdu		-	The data that pkt will be filled with. psdu[0] must
 * 				be the data length
 *
 * Returns:
 * 0		-	Success
 * Negative - 	Failure
 */
static int net_pkt_fill(struct net_pkt **pkt, uint8_t *psdu)
{
	uint8_t packet_len = psdu[0];

	*pkt = net_pkt_alloc_with_buffer(NULL, packet_len, AF_UNSPEC, 0,
					 K_FOREVER);
	if (!(*pkt)) {
		return -ENOMEM;
	}

	net_pkt_cursor_init(*pkt);
	net_pkt_write(*pkt, psdu + 1, packet_len);
	printk("pkt %p len %u\n", *pkt, packet_len);
	return 0;
}

/* Function called by the lower layer of the network stack */
int net_recv_data(struct net_if *iface, struct net_pkt *pkt)
{
	size_t len = net_pkt_get_len(pkt);
	uint8_t rx_buf[127];
	uint8_t *p = rx_buf;
	int ret;
	struct net_buf *buf;

	LOG_INF("Got data, pkt %p, len %d", pkt, len);

	net_pkt_hexdump(pkt, "<");

	if (len > (127 - 2)) {
		LOG_ERR("Too large packet");
		ret = -ENOMEM;
		goto out;
	}

	buf = net_buf_frag_last(pkt->buffer);
	test_data.actual_rx = malloc(buf->len + 1);
	memcpy(test_data.actual_rx + 1, buf->data, buf->len);
	test_data.actual_rx[0] = buf->len;
	k_sem_give(&test_data.rx_wait);

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

static void radio_init(void)
{
	enum ieee802154_config_type config_type =
		IEEE802154_CONFIG_AUTO_ACK_FPB;
	const struct ieee802154_config config = {
		.auto_ack_fpb.enabled = true,
		.auto_ack_fpb.mode = IEEE802154_FPB_ADDR_MATCH_ZIGBEE
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
	radio_api->configure(ieee802154_dev, config_type, &config);

	if (radio_api->start(ieee802154_dev)) {
		printf("Radio couldn't be started - "
		       "make sure that program was started in babble sim mode (-bsim)");
		posix_exit(-EFAULT);
	}
}

/**
 * Loads tests config from config_file and fills test_data
 *
 * Returns:
 * entries number   -       1 or more entries in test_data was filled
 * negative         -       Parsing failure
 */
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
			       CSV_DATATYPE_BYTESTREAM },
	};

	if (!config_openfile(config_file)) {
		return -EIO;
	}

	tests_num = get_config_lines_number();

	/** TODO: Handle malloc possible failures */
	test_data->tx = malloc(sizeof(uint8_t *) * tests_num);
	test_data->expected_rx = malloc(sizeof(uint8_t *) * tests_num);
	test_data->results = malloc(sizeof(bool) * tests_num);
	test_data->configs =
		malloc(sizeof(struct ieee802154_test_config) * tests_num);
	k_sem_init(&test_data->rx_wait, 0, 1);
	test_data->ctr = 0;

	configs_ctr = 0;
	for (;; configs_ctr++) {
		if (!config_parse_line(&tests_parser)) {
			config_parser_clean(&tests_parser);
			break;
		}

		/** tx data is in 8th column in the config file */
		int tx_size = tests_parser.dest[8].bytestream[0];
		/** expected rx is in the 9th column in the config file */
		int rx_size = tests_parser.dest[9].bytestream[0];

		test_data->tx[configs_ctr] = malloc(tx_size + 1);
		test_data->expected_rx[configs_ctr] =
			rx_size > 0 ? malloc(rx_size) : NULL;

		memcpy(test_data->tx[configs_ctr],
		       tests_parser.dest[8].bytestream, tx_size + 1);

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

		if (test_data->expected_rx[configs_ctr]) {
			memcpy(test_data->expected_rx[configs_ctr],
			       tests_parser.dest[9].bytestream, rx_size + 1);
		}

		config_parser_clean(&tests_parser);
	}

	test_data->ntests = configs_ctr;
	config_closefile();

	return configs_ctr;
}

static void test_data_release(void)
{
	for (int i = 0; i < test_data.ntests; i++) {
		free(test_data.tx[i]);
		free(test_data.expected_rx[i]);
	}

	free(test_data.tx);
	free(test_data.expected_rx);
	free(test_data.results);
}

/**
 * Send bytes using radio_api.
 *
 * Arguments:
 * bytes	-	data to send, bytes[0] is the size of following data
 *
 * Returns:
 * 0		-	Tx suceeded
 * Negative     -	Tx failed
 */
static int send_bytes(uint8_t *bytes)
{
	struct net_pkt *netpkt;
	int tx_status;

	net_pkt_fill(&netpkt, bytes);
	tx_status = tx(netpkt);
	net_pkt_unref(netpkt);
	printf("Frame transmission %s\n", !tx_status ? "succeded" : "failed");
	return tx_status;
}

static void execute_tests(void)
{
	for (test_data.ctr = 0; test_data.ctr < test_data.ntests;
	     test_data.ctr++) {
		radio_configure(&test_data.configs[test_data.ctr]);
		k_sem_reset(&test_data.rx_wait);

		int tx_result = send_bytes(test_data.tx[test_data.ctr]);
		int sem_result;

		if (tx_result) {
			test_data.results[test_data.ctr] = false;
			continue;
		}

		sem_result = k_sem_take(&test_data.rx_wait,
					K_MSEC(TEST_WAIT_FOR_RX_TIMEOUT_MS));

		/* Sem taken or timed out, check if rx was required */
		if (test_data.expected_rx[test_data.ctr]) {
			if (sem_result == 0) {
				/* Rx required and received */
				test_data.results[test_data.ctr] = true;
				if (test_data.actual_rx[0] ==
				    test_data.expected_rx[test_data.ctr][0]) {
					if (0 ==
					    memcmp(&test_data.actual_rx[1],
						   &test_data.expected_rx
							    [test_data.ctr][1],
						   test_data.actual_rx[0])) {
						test_data.results[test_data.ctr] =
							true;
					} else {
						test_data.results[test_data.ctr] =
							false;
					}
				}

				free(test_data.actual_rx);
				test_data.actual_rx = NULL;
			} else {
				/* Rx required and not received */
				test_data.results[test_data.ctr] = false;
			}
		} else {
			if (0 == sem_result) {
				/* Rx not required and received */
				test_data.results[test_data.ctr] = false;
				free(test_data.actual_rx);
				test_data.actual_rx = NULL;
			} else {
				/* Rx not required and not received */
				test_data.results[test_data.ctr] = true;
			}
		}
	}

	printf("\n***\tTest Results\t***\n");
	for (int i = 0; i < test_data.ntests; i++) {
		printf("[%d] %10s\n", i,
		       test_data.results[i] ? "Passed" : "Failed");
	}
}

void main(void)
{
	/* Do the device binding, start the radio and initialize net_pkt */
	radio_init();

	/* Load test data from file and init test_data structure */
	load_test_data(TEST_DATA_FILE, &test_data);

	/* Give some time receivers to start receiving */
	k_msleep(100);

	/* Start tests execution */
	execute_tests();

	/* Cleanup */
	test_data_release();

	/* Terminate */
	posix_exit(0);
}
