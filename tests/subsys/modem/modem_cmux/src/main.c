/*
 * Copyright (c) 2022 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*************************************************************************************************/
/*                                        Dependencies                                           */
/*************************************************************************************************/
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <string.h>

#include <zephyr/modem/cmux.h>
#include <modem_backend_mock.h>

/*************************************************************************************************/
/*                                         Definitions                                           */
/*************************************************************************************************/
#define EVENT_CMUX_CONNECTED		BIT(0)
#define EVENT_CMUX_DLCI1_OPEN		BIT(1)
#define EVENT_CMUX_DLCI2_OPEN		BIT(2)
#define EVENT_CMUX_DLCI1_RECEIVE_READY	BIT(3)
#define EVENT_CMUX_DLCI1_TRANSMIT_IDLE	BIT(4)
#define EVENT_CMUX_DLCI2_RECEIVE_READY	BIT(5)
#define EVENT_CMUX_DLCI2_TRANSMIT_IDLE	BIT(6)
#define EVENT_CMUX_DLCI1_CLOSED		BIT(7)
#define EVENT_CMUX_DLCI2_CLOSED		BIT(8)
#define EVENT_CMUX_DISCONNECTED		BIT(9)
#define CMUX_BASIC_HRD_SMALL_SIZE	6
#define CMUX_BASIC_HRD_LARGE_SIZE	7

/*************************************************************************************************/
/*                                          Instances                                            */
/*************************************************************************************************/
static struct modem_cmux cmux;
static uint8_t cmux_receive_buf[127];
static uint8_t cmux_transmit_buf[149];
static struct modem_cmux_dlci dlci1;
static struct modem_cmux_dlci dlci2;
static struct modem_pipe *dlci1_pipe;
static struct modem_pipe *dlci2_pipe;

static struct k_event cmux_event;

static struct modem_backend_mock bus_mock;
static uint8_t bus_mock_rx_buf[4096];
static uint8_t bus_mock_tx_buf[4096];
static struct modem_pipe *bus_mock_pipe;

static uint8_t dlci1_receive_buf[127];
static uint8_t dlci2_receive_buf[127];

static uint8_t buffer1[4096];
static uint8_t buffer2[4096];

/*************************************************************************************************/
/*                                          Callbacks                                            */
/*************************************************************************************************/
static void test_modem_dlci1_pipe_callback(struct modem_pipe *pipe, enum modem_pipe_event event,
					   void *user_data)
{
	switch (event) {
	case MODEM_PIPE_EVENT_OPENED:
		k_event_post(&cmux_event, EVENT_CMUX_DLCI1_OPEN);
		break;

	case MODEM_PIPE_EVENT_RECEIVE_READY:
		k_event_post(&cmux_event, EVENT_CMUX_DLCI1_RECEIVE_READY);
		break;

	case MODEM_PIPE_EVENT_TRANSMIT_IDLE:
		k_event_post(&cmux_event, EVENT_CMUX_DLCI1_TRANSMIT_IDLE);
		break;

	case MODEM_PIPE_EVENT_CLOSED:
		k_event_post(&cmux_event, EVENT_CMUX_DLCI1_CLOSED);
		break;

	default:
		break;
	}
}

static void test_modem_dlci2_pipe_callback(struct modem_pipe *pipe, enum modem_pipe_event event,
					   void *user_data)
{
	switch (event) {
	case MODEM_PIPE_EVENT_OPENED:
		k_event_post(&cmux_event, EVENT_CMUX_DLCI2_OPEN);
		break;

	case MODEM_PIPE_EVENT_RECEIVE_READY:
		k_event_post(&cmux_event, EVENT_CMUX_DLCI2_RECEIVE_READY);
		break;

	case MODEM_PIPE_EVENT_TRANSMIT_IDLE:
		k_event_post(&cmux_event, EVENT_CMUX_DLCI2_TRANSMIT_IDLE);
		break;

	case MODEM_PIPE_EVENT_CLOSED:
		k_event_post(&cmux_event, EVENT_CMUX_DLCI2_CLOSED);
		break;

	default:
		break;
	}
}

/*************************************************************************************************/
/*                                         CMUX frames                                           */
/*************************************************************************************************/
static uint8_t cmux_frame_control_sabm_cmd[] = {0xF9, 0x03, 0x3F, 0x01, 0x1C, 0xF9};
static uint8_t cmux_frame_control_sabm_ack[] = {0xF9, 0x03, 0x73, 0x01, 0xD7, 0xF9};
static uint8_t cmux_frame_control_cld_cmd[] = {0xF9, 0x03, 0xEF, 0x05, 0xC3, 0x01, 0xF2, 0xF9};
static uint8_t cmux_frame_control_cld_ack[] = {0xF9, 0x03, 0xEF, 0x05, 0xC1, 0x01, 0xF2, 0xF9};
static uint8_t cmux_frame_dlci1_sabm_cmd[] = {0xF9, 0x07, 0x3F, 0x01, 0xDE, 0xF9};
static uint8_t cmux_frame_dlci1_sabm_ack[] = {0xF9, 0x07, 0x73, 0x01, 0x15, 0xF9};
static uint8_t cmux_frame_dlci1_disc_cmd[] = {0xF9, 0x07, 0x53, 0x01, 0x3F, 0xF9};
static uint8_t cmux_frame_dlci1_ua_ack[] = {0xF9, 0x07, 0x73, 0x01, 0x15, 0xF9};
static uint8_t cmux_frame_dlci2_sabm_cmd[] = {0xF9, 0x0B, 0x3F, 0x01, 0x59, 0xF9};
static uint8_t cmux_frame_dlci2_sabm_ack[] = {0xF9, 0x0B, 0x73, 0x01, 0x92, 0xF9};
static uint8_t cmux_frame_dlci2_disc_cmd[] = {0xF9, 0x0B, 0x53, 0x01, 0xB8, 0xF9};
static uint8_t cmux_frame_dlci2_ua_ack[] = {0xF9, 0x0B, 0x73, 0x01, 0x92, 0xF9};
static uint8_t cmux_frame_control_msc_cmd[] = {0xF9, 0x01, 0xFF, 0x0B, 0xE3,
					       0x07, 0x0B, 0x09, 0x01, 0x6C, 0xF9};

static uint8_t cmux_frame_control_msc_ack[] = {0xF9, 0x01, 0xFF, 0x0B, 0xE1,
					       0x07, 0x0B, 0x09, 0x01, 0x6C, 0xF9};

static uint8_t cmux_frame_control_fcon_cmd[] = {0xF9, 0x01, 0xFF, 0x05, 0xA3, 0x01, 0x86, 0xF9};
static uint8_t cmux_frame_control_fcon_ack[] = {0xF9, 0x01, 0xFF, 0x05, 0xA1, 0x01, 0x86, 0xF9};
static uint8_t cmux_frame_control_fcoff_cmd[] = {0xF9, 0x01, 0xFF, 0x05, 0x63, 0x01, 0x86, 0xF9};
static uint8_t cmux_frame_control_fcoff_ack[] = {0xF9, 0x01, 0xFF, 0x05, 0x61, 0x01, 0x86, 0xF9};

/*************************************************************************************************/
/*                                     DLCI2 AT CMUX frames                                      */
/*************************************************************************************************/
static uint8_t cmux_frame_dlci2_at_cgdcont[] = {
	0xF9, 0x0B, 0xEF, 0x43, 0x41, 0x54, 0x2B, 0x43, 0x47, 0x44, 0x43, 0x4F, 0x4E,
	0x54, 0x3D, 0x31, 0x2C, 0x22, 0x49, 0x50, 0x22, 0x2C, 0x22, 0x74, 0x72, 0x61,
	0x63, 0x6B, 0x75, 0x6E, 0x69, 0x74, 0x2E, 0x6D, 0x32, 0x6D, 0x22, 0x23, 0xF9};

static uint8_t cmux_frame_data_dlci2_at_cgdcont[] = {
	0x41, 0x54, 0x2B, 0x43, 0x47, 0x44, 0x43, 0x4F, 0x4E, 0x54, 0x3D,
	0x31, 0x2C, 0x22, 0x49, 0x50, 0x22, 0x2C, 0x22, 0x74, 0x72, 0x61,
	0x63, 0x6B, 0x75, 0x6E, 0x69, 0x74, 0x2E, 0x6D, 0x32, 0x6D, 0x22};

static uint8_t cmux_frame_dlci2_at_newline[] = {0xF9, 0x0B, 0xEF, 0x05, 0x0D, 0x0A, 0xB7, 0xF9};

static uint8_t cmux_frame_data_dlci2_at_newline[] = {0x0D, 0x0A};

/*************************************************************************************************/
/*                                     DLCI2 AT CMUX error frames				 */
/*************************************************************************************************/
static uint8_t cmux_frame_dlci2_at_cgdcont_invalid_length[] = {
	0xF9, 0x0B, 0xEF, 0xFE, 0x41, 0x54, 0x2B, 0x43, 0x47, 0x44, 0x43, 0x4F, 0x4E,
	0x54, 0x3D, 0x31, 0x2C, 0x22, 0x49, 0x50, 0x22, 0x2C, 0x22, 0x74, 0x72, 0x61,
	0x63, 0x6B, 0x75, 0x6E, 0x69, 0x74, 0x2E, 0x6D, 0x32, 0x6D, 0x22, 0x23, 0xF9};

/*************************************************************************************************/
/*                                    DLCI1 AT CMUX frames                                       */
/*************************************************************************************************/
static uint8_t cmux_frame_dlci1_at_at[] = {0xF9, 0x07, 0xEF, 0x05, 0x41, 0x54, 0x30, 0xF9};

static uint8_t cmux_frame_data_dlci1_at_at[] = {0x41, 0x54};

static uint8_t cmux_frame_dlci1_at_newline[] = {0xF9, 0x07, 0xEF, 0x05, 0x0D, 0x0A, 0x30, 0xF9};

static uint8_t cmux_frame_data_dlci1_at_newline[] = {0x0D, 0x0A};

/*************************************************************************************************/
/*                                DLCI1 AT CMUX Desync frames                                    */
/*************************************************************************************************/
static uint8_t cmux_frame_dlci1_at_at_desync[] = {0x41, 0x54, 0x30, 0xF9};

/*************************************************************************************************/
/*                                   DLCI2 PPP CMUX frames                                       */
/*************************************************************************************************/
static uint8_t cmux_frame_dlci2_ppp_52[] = {
	0xF9, 0x0B, 0xEF, 0x69, 0x7E, 0xFF, 0x7D, 0x23, 0xC0, 0x21, 0x7D, 0x21, 0x7D, 0x20, 0x7D,
	0x20, 0x7D, 0x38, 0x7D, 0x22, 0x7D, 0x26, 0x7D, 0x20, 0x7D, 0x20, 0x7D, 0x20, 0x7D, 0x20,
	0x7D, 0x23, 0x7D, 0x24, 0xC0, 0x23, 0x7D, 0x25, 0x7D, 0x26, 0x53, 0x96, 0x7D, 0x38, 0xAA,
	0x7D, 0x27, 0x7D, 0x22, 0x7D, 0x28, 0x7D, 0x22, 0xD5, 0xA8, 0x7E, 0xF6, 0xF9};

static uint8_t cmux_frame_data_dlci2_ppp_52[] = {
	0x7E, 0xFF, 0x7D, 0x23, 0xC0, 0x21, 0x7D, 0x21, 0x7D, 0x20, 0x7D, 0x20, 0x7D,
	0x38, 0x7D, 0x22, 0x7D, 0x26, 0x7D, 0x20, 0x7D, 0x20, 0x7D, 0x20, 0x7D, 0x20,
	0x7D, 0x23, 0x7D, 0x24, 0xC0, 0x23, 0x7D, 0x25, 0x7D, 0x26, 0x53, 0x96, 0x7D,
	0x38, 0xAA, 0x7D, 0x27, 0x7D, 0x22, 0x7D, 0x28, 0x7D, 0x22, 0xD5, 0xA8, 0x7E};

static uint8_t cmux_frame_dlci2_ppp_18[] = {0xF9, 0x0B, 0xEF, 0x25, 0x7E, 0xFF, 0x7D, 0x23,
					    0xC0, 0x21, 0x7D, 0x22, 0x7D, 0x21, 0x7D, 0x20,
					    0x7D, 0x24, 0x7D, 0x3C, 0x90, 0x7E, 0x8F, 0xF9};

static uint8_t cmux_frame_data_dlci2_ppp_18[] = {0x7E, 0xFF, 0x7D, 0x23, 0xC0, 0x21,
						 0x7D, 0x22, 0x7D, 0x21, 0x7D, 0x20,
						 0x7D, 0x24, 0x7D, 0x3C, 0x90, 0x7E};

static uint8_t cmux_frame_data_large[127] = { [0 ... 126] = 0xAA };

const static struct modem_backend_mock_transaction transaction_control_cld = {
	.get = cmux_frame_control_cld_cmd,
	.get_size = sizeof(cmux_frame_control_cld_cmd),
	.put = cmux_frame_control_cld_ack,
	.put_size = sizeof(cmux_frame_control_cld_ack)
};

const static struct modem_backend_mock_transaction transaction_control_sabm = {
	.get = cmux_frame_control_sabm_cmd,
	.get_size = sizeof(cmux_frame_control_sabm_cmd),
	.put = cmux_frame_control_sabm_ack,
	.put_size = sizeof(cmux_frame_control_sabm_ack)
};

const static struct modem_backend_mock_transaction transaction_dlci1_disc = {
	.get = cmux_frame_dlci1_disc_cmd,
	.get_size = sizeof(cmux_frame_dlci1_disc_cmd),
	.put = cmux_frame_dlci1_ua_ack,
	.put_size = sizeof(cmux_frame_dlci1_ua_ack)
};

const static struct modem_backend_mock_transaction transaction_dlci2_disc = {
	.get = cmux_frame_dlci2_disc_cmd,
	.get_size = sizeof(cmux_frame_dlci2_disc_cmd),
	.put = cmux_frame_dlci2_ua_ack,
	.put_size = sizeof(cmux_frame_dlci2_ua_ack)
};

const static struct modem_backend_mock_transaction transaction_dlci1_sabm = {
	.get = cmux_frame_dlci1_sabm_cmd,
	.get_size = sizeof(cmux_frame_dlci1_sabm_cmd),
	.put = cmux_frame_dlci1_ua_ack,
	.put_size = sizeof(cmux_frame_dlci1_ua_ack)
};

const static struct modem_backend_mock_transaction transaction_dlci2_sabm = {
	.get = cmux_frame_dlci2_sabm_cmd,
	.get_size = sizeof(cmux_frame_dlci2_sabm_cmd),
	.put = cmux_frame_dlci2_ua_ack,
	.put_size = sizeof(cmux_frame_dlci2_ua_ack)
};

static void test_modem_cmux_callback(struct modem_cmux *cmux, enum modem_cmux_event event,
				     void *user_data)
{
	if (event == MODEM_CMUX_EVENT_CONNECTED) {
		k_event_post(&cmux_event, EVENT_CMUX_CONNECTED);
		return;
	}

	if (event == MODEM_CMUX_EVENT_DISCONNECTED) {
		k_event_post(&cmux_event, EVENT_CMUX_DISCONNECTED);
		return;
	}
}

static void *test_modem_cmux_setup(void)
{
	uint32_t events;

	struct modem_cmux_dlci_config dlci1_config = {
		.dlci_address = 1,
		.receive_buf = dlci1_receive_buf,
		.receive_buf_size = sizeof(dlci1_receive_buf),
	};

	struct modem_cmux_dlci_config dlci2_config = {
		.dlci_address = 2,
		.receive_buf = dlci2_receive_buf,
		.receive_buf_size = sizeof(dlci2_receive_buf),
	};

	k_event_init(&cmux_event);

	struct modem_cmux_config cmux_config = {
		.callback = test_modem_cmux_callback,
		.user_data = NULL,
		.receive_buf = cmux_receive_buf,
		.receive_buf_size = sizeof(cmux_receive_buf),
		.transmit_buf = cmux_transmit_buf,
		.transmit_buf_size = ARRAY_SIZE(cmux_transmit_buf),
	};

	modem_cmux_init(&cmux, &cmux_config);
	dlci1_pipe = modem_cmux_dlci_init(&cmux, &dlci1, &dlci1_config);
	dlci2_pipe = modem_cmux_dlci_init(&cmux, &dlci2, &dlci2_config);

	const struct modem_backend_mock_config bus_mock_config = {
		.rx_buf = bus_mock_rx_buf,
		.rx_buf_size = sizeof(bus_mock_rx_buf),
		.tx_buf = bus_mock_tx_buf,
		.tx_buf_size = sizeof(bus_mock_tx_buf),
		.limit = 32,
	};

	bus_mock_pipe = modem_backend_mock_init(&bus_mock, &bus_mock_config);
	__ASSERT_NO_MSG(modem_pipe_open(bus_mock_pipe, K_SECONDS(10)) == 0);

	/* Connect CMUX */
	__ASSERT_NO_MSG(modem_cmux_attach(&cmux, bus_mock_pipe) == 0);
	modem_backend_mock_prime(&bus_mock, &transaction_control_sabm);
	__ASSERT_NO_MSG(modem_cmux_connect_async(&cmux) == 0);
	events = k_event_wait(&cmux_event, EVENT_CMUX_CONNECTED, false, K_MSEC(100));
	__ASSERT_NO_MSG(events == EVENT_CMUX_CONNECTED);

	/* Open DLCI channels */
	modem_pipe_attach(dlci1_pipe, test_modem_dlci1_pipe_callback, NULL);
	modem_backend_mock_prime(&bus_mock, &transaction_dlci1_sabm);
	__ASSERT_NO_MSG(modem_pipe_open_async(dlci1_pipe) == 0);
	events = k_event_wait(&cmux_event, EVENT_CMUX_DLCI1_OPEN, false, K_MSEC(100));
	__ASSERT_NO_MSG((events & EVENT_CMUX_DLCI1_OPEN));

	modem_pipe_attach(dlci2_pipe, test_modem_dlci2_pipe_callback, NULL);
	modem_backend_mock_prime(&bus_mock, &transaction_dlci2_sabm);
	__ASSERT_NO_MSG(modem_pipe_open_async(dlci2_pipe) == 0);
	events = k_event_wait(&cmux_event, EVENT_CMUX_DLCI2_OPEN, false, K_MSEC(100));
	__ASSERT_NO_MSG((events & EVENT_CMUX_DLCI2_OPEN));

	return NULL;
}

static void test_modem_cmux_before(void *f)
{
	/* Reset events */
	k_event_clear(&cmux_event, UINT32_MAX);

	/* Reset mock pipes */
	modem_backend_mock_reset(&bus_mock);
}

ZTEST(modem_cmux, test_modem_cmux_receive_dlci2_at)
{
	int ret;
	uint32_t events;

	modem_backend_mock_put(&bus_mock, cmux_frame_dlci2_at_cgdcont,
			       sizeof(cmux_frame_dlci2_at_cgdcont));

	modem_backend_mock_put(&bus_mock, cmux_frame_dlci2_at_newline,
			       sizeof(cmux_frame_dlci2_at_newline));

	k_msleep(100);

	events = k_event_test(&cmux_event, EVENT_CMUX_DLCI2_RECEIVE_READY);
	zassert_equal(events, EVENT_CMUX_DLCI2_RECEIVE_READY,
		      "Receive ready event not received for DLCI2 pipe");

	ret = modem_pipe_receive(dlci2_pipe, buffer2, sizeof(buffer2));
	zassert_true(ret == (sizeof(cmux_frame_data_dlci2_at_cgdcont) +
			     sizeof(cmux_frame_data_dlci2_at_newline)),
		     "Incorrect number of bytes received");

	zassert_true(memcmp(buffer2, cmux_frame_data_dlci2_at_cgdcont,
			    sizeof(cmux_frame_data_dlci2_at_cgdcont)) == 0,
		     "Incorrect data received");

	zassert_true(memcmp(&buffer2[sizeof(cmux_frame_data_dlci2_at_cgdcont)],
			    cmux_frame_data_dlci2_at_newline,
			    sizeof(cmux_frame_data_dlci2_at_newline)) == 0,
		     "Incorrect data received");
}

ZTEST(modem_cmux, test_modem_cmux_receive_dlci1_at)
{
	int ret;
	uint32_t events;

	modem_backend_mock_put(&bus_mock, cmux_frame_dlci1_at_at, sizeof(cmux_frame_dlci1_at_at));
	modem_backend_mock_put(&bus_mock, cmux_frame_dlci1_at_newline,
			       sizeof(cmux_frame_dlci1_at_newline));

	k_msleep(100);

	events = k_event_test(&cmux_event, EVENT_CMUX_DLCI1_RECEIVE_READY);
	zassert_equal(events, EVENT_CMUX_DLCI1_RECEIVE_READY,
		      "Receive ready event not received for DLCI1 pipe");

	ret = modem_pipe_receive(dlci1_pipe, buffer1, sizeof(buffer1));
	zassert_true(ret == (sizeof(cmux_frame_data_dlci1_at_at) +
			     sizeof(cmux_frame_data_dlci1_at_newline)),
		     "Incorrect number of bytes received");

	zassert_true(memcmp(buffer1, cmux_frame_data_dlci1_at_at,
			    sizeof(cmux_frame_data_dlci1_at_at)) == 0,
		     "Incorrect data received");

	zassert_true(memcmp(&buffer1[sizeof(cmux_frame_data_dlci1_at_at)],
			    cmux_frame_data_dlci1_at_newline,
			    sizeof(cmux_frame_data_dlci1_at_newline)) == 0,
		     "Incorrect data received");
}

ZTEST(modem_cmux, test_modem_cmux_receive_dlci2_ppp)
{
	int ret;
	uint32_t events;

	modem_backend_mock_put(&bus_mock, cmux_frame_dlci2_ppp_52, sizeof(cmux_frame_dlci2_ppp_52));
	modem_backend_mock_put(&bus_mock, cmux_frame_dlci2_ppp_18, sizeof(cmux_frame_dlci2_ppp_18));

	k_msleep(100);

	events = k_event_test(&cmux_event, EVENT_CMUX_DLCI2_RECEIVE_READY);
	zassert_equal(events, EVENT_CMUX_DLCI2_RECEIVE_READY,
		      "Receive ready event not received for DLCI2 pipe");

	ret = modem_pipe_receive(dlci2_pipe, buffer2, sizeof(buffer2));
	zassert_true(ret == (sizeof(cmux_frame_data_dlci2_ppp_52) +
			     sizeof(cmux_frame_data_dlci2_ppp_18)),
		     "Incorrect number of bytes received");

	zassert_true(memcmp(buffer2, cmux_frame_data_dlci2_ppp_52,
			    sizeof(cmux_frame_data_dlci2_ppp_52)) == 0,
		     "Incorrect data received");

	zassert_true(memcmp(&buffer2[sizeof(cmux_frame_data_dlci2_ppp_52)],
			    cmux_frame_data_dlci2_ppp_18,
			    sizeof(cmux_frame_data_dlci2_ppp_18)) == 0,
		     "Incorrect data received");
}

ZTEST(modem_cmux, test_modem_cmux_transmit_dlci2_ppp)
{
	int ret;
	uint32_t events;

	ret = modem_pipe_transmit(dlci2_pipe, cmux_frame_data_dlci2_ppp_52,
				  sizeof(cmux_frame_data_dlci2_ppp_52));
	zassert_true(ret == sizeof(cmux_frame_data_dlci2_ppp_52), "Failed to send DLCI2 PPP 52");

	events = k_event_wait(&cmux_event, EVENT_CMUX_DLCI2_TRANSMIT_IDLE, false, K_MSEC(200));
	zassert_equal(events, EVENT_CMUX_DLCI2_TRANSMIT_IDLE,
		      "Transmit idle event not received for DLCI2 pipe");

	k_event_clear(&cmux_event, EVENT_CMUX_DLCI2_TRANSMIT_IDLE);

	ret = modem_pipe_transmit(dlci2_pipe, cmux_frame_data_dlci2_ppp_18,
				  sizeof(cmux_frame_data_dlci2_ppp_18));
	zassert_true(ret == sizeof(cmux_frame_data_dlci2_ppp_18), "Failed to send DLCI2 PPP 18");

	events = k_event_wait(&cmux_event, EVENT_CMUX_DLCI2_TRANSMIT_IDLE, false, K_MSEC(200));
	zassert_equal(events, EVENT_CMUX_DLCI2_TRANSMIT_IDLE,
		      "Transmit idle event not received for DLCI2 pipe");

	ret = modem_backend_mock_get(&bus_mock, buffer2, sizeof(buffer2));
	zassert_true(ret == (sizeof(cmux_frame_dlci2_ppp_52) + sizeof(cmux_frame_dlci2_ppp_18)),
		     "Incorrect number of bytes transmitted");
}

ZTEST(modem_cmux, test_modem_cmux_resync)
{
	int ret;

	modem_backend_mock_put(&bus_mock, cmux_frame_dlci1_at_at_desync,
			       sizeof(cmux_frame_dlci1_at_at_desync));
	modem_backend_mock_put(&bus_mock, cmux_frame_dlci1_at_at, sizeof(cmux_frame_dlci1_at_at));
	modem_backend_mock_put(&bus_mock, cmux_frame_dlci1_at_newline,
			       sizeof(cmux_frame_dlci1_at_newline));

	k_msleep(100);

	ret = modem_pipe_receive(dlci1_pipe, buffer1, sizeof(buffer1));

	zassert_true(ret == (sizeof(cmux_frame_data_dlci1_at_at) +
			     sizeof(cmux_frame_data_dlci1_at_newline)),
		     "Incorrect number of bytes received");

	zassert_true(memcmp(buffer1, cmux_frame_data_dlci1_at_at,
			    sizeof(cmux_frame_data_dlci1_at_at)) == 0,
		     "Incorrect data received");

	zassert_true(memcmp(&buffer1[sizeof(cmux_frame_data_dlci1_at_at)],
			    cmux_frame_data_dlci1_at_newline,
			    sizeof(cmux_frame_data_dlci1_at_newline)) == 0,
		     "Incorrect data received");
}

ZTEST(modem_cmux, test_modem_cmux_flow_control_dlci2)
{
	int ret;

	modem_backend_mock_put(&bus_mock, cmux_frame_control_fcoff_cmd,
			       sizeof(cmux_frame_control_fcoff_cmd));

	k_msleep(100);

	ret = modem_backend_mock_get(&bus_mock, buffer1, sizeof(buffer1));
	zassert_true(ret == sizeof(cmux_frame_control_fcoff_ack),
		     "Incorrect number of bytes received");

	zassert_true(memcmp(buffer1, cmux_frame_control_fcoff_ack,
			    sizeof(cmux_frame_control_fcoff_ack)) == 0,
		     "Incorrect data received");

	ret = modem_pipe_transmit(dlci2_pipe, cmux_frame_data_dlci2_ppp_52,
				  sizeof(cmux_frame_data_dlci2_ppp_52));

	zassert_true(ret == 0, "Failed to block transmit while flow control is off");

	ret = modem_backend_mock_get(&bus_mock, buffer1, sizeof(buffer1));
	zassert_true(ret == 0, "FCOFF failed to prevent transmission of data");
	modem_backend_mock_put(&bus_mock, cmux_frame_control_fcon_cmd,
			       sizeof(cmux_frame_control_fcon_cmd));

	k_msleep(100);

	ret = modem_backend_mock_get(&bus_mock, buffer1, sizeof(buffer1));
	zassert_true(ret == sizeof(cmux_frame_control_fcon_ack),
		     "Incorrect number of bytes received");

	zassert_true(memcmp(buffer1, cmux_frame_control_fcon_ack,
			    sizeof(cmux_frame_control_fcon_ack)) == 0,
		     "Incorrect data received");

	ret = modem_pipe_transmit(dlci2_pipe, cmux_frame_data_dlci2_ppp_52,
				  sizeof(cmux_frame_data_dlci2_ppp_52));

	zassert_true(ret == sizeof(cmux_frame_data_dlci2_ppp_52),
		     "Transmit failed after flow control is enabled");

	k_msleep(100);

	ret = modem_backend_mock_get(&bus_mock, buffer1, sizeof(buffer1));
	zassert_true(ret == sizeof(cmux_frame_dlci2_ppp_52),
		     "Transmit failed after flow control is enabled");

	zassert_true(memcmp(buffer1, cmux_frame_dlci2_ppp_52,
			    sizeof(cmux_frame_dlci2_ppp_52)) == 0,
		     "Incorrect data received");
}

ZTEST(modem_cmux, test_modem_cmux_msc_cmd_ack)
{
	int ret;

	modem_backend_mock_put(&bus_mock, cmux_frame_control_msc_cmd,
			       sizeof(cmux_frame_control_msc_cmd));

	k_msleep(100);

	ret = modem_backend_mock_get(&bus_mock, buffer1, sizeof(buffer1));
	zassert_true(ret == sizeof(cmux_frame_control_msc_ack),
		     "Incorrect number of bytes received");

	zassert_true(memcmp(buffer1, cmux_frame_control_msc_ack,
			    sizeof(cmux_frame_control_msc_ack)) == 0,
		     "Incorrect MSC ACK received");
}

ZTEST(modem_cmux, test_modem_cmux_dlci1_close_open)
{
	int ret;
	uint32_t events;

	/* Close DLCI1 */
	zassert_true(modem_pipe_close_async(dlci1_pipe) == 0, "Failed to close DLCI1 pipe");

	k_msleep(100);

	ret = modem_backend_mock_get(&bus_mock, buffer1, sizeof(buffer1));
	zassert_true(ret == sizeof(cmux_frame_dlci1_disc_cmd),
		     "Incorrect number of bytes received for DLCI1 close cmd");

	zassert_true(memcmp(buffer1, cmux_frame_dlci1_disc_cmd,
			    sizeof(cmux_frame_dlci1_disc_cmd)) == 0,
		     "Incorrect DLCI1 close cmd received");

	modem_backend_mock_put(&bus_mock, cmux_frame_dlci1_ua_ack,
			       sizeof(cmux_frame_dlci1_ua_ack));

	events = k_event_wait_all(&cmux_event, (EVENT_CMUX_DLCI1_CLOSED),
				  false, K_MSEC(100));

	zassert_true((events & EVENT_CMUX_DLCI1_CLOSED),
		     "DLCI1 not closed as expected");

	/* Wait for potential T1 timeout */
	k_msleep(500);

	ret = modem_backend_mock_get(&bus_mock, buffer1, sizeof(buffer1));
	zassert_true(ret == 0, "Received unexpected data");

	/* Open DLCI1 */
	zassert_true(modem_pipe_open_async(dlci1_pipe) == 0, "Failed to open DLCI1 pipe");

	k_msleep(100);

	ret = modem_backend_mock_get(&bus_mock, buffer1, sizeof(buffer1));
	zassert_true(ret == sizeof(cmux_frame_dlci1_sabm_cmd),
		     "Incorrect number of bytes received for DLCI1 open cmd");

	zassert_true(memcmp(buffer1, cmux_frame_dlci1_sabm_cmd,
			    sizeof(cmux_frame_dlci1_sabm_cmd)) == 0,
		     "Incorrect DLCI1 open cmd received");

	modem_backend_mock_put(&bus_mock, cmux_frame_dlci1_sabm_ack,
			       sizeof(cmux_frame_dlci1_sabm_ack));

	events = k_event_wait_all(&cmux_event, (EVENT_CMUX_DLCI1_OPEN),
				  false, K_MSEC(100));

	zassert_true((events & EVENT_CMUX_DLCI1_OPEN),
		     "DLCI1 not opened as expected");

	/* Wait for potential T1 timeout */
	k_msleep(500);

	ret = modem_backend_mock_get(&bus_mock, buffer1, sizeof(buffer1));
	zassert_true(ret == 0, "Received unexpected data");
}

ZTEST(modem_cmux, test_modem_cmux_disconnect_connect)
{
	uint32_t events;
	int ret;

	/* Disconnect CMUX */
	zassert_true(modem_pipe_close_async(dlci1_pipe) == 0, "Failed to close DLCI1");
	zassert_true(modem_pipe_close_async(dlci2_pipe) == 0, "Failed to close DLCI2");

	modem_backend_mock_put(&bus_mock, cmux_frame_dlci1_ua_ack,
			       sizeof(cmux_frame_dlci1_ua_ack));

	modem_backend_mock_put(&bus_mock, cmux_frame_dlci2_ua_ack,
			       sizeof(cmux_frame_dlci2_ua_ack));

	events = k_event_wait_all(&cmux_event, (EVENT_CMUX_DLCI1_CLOSED | EVENT_CMUX_DLCI2_CLOSED),
				  false, K_MSEC(100));

	zassert_true((events & EVENT_CMUX_DLCI1_CLOSED), "Failed to close DLCI1");
	zassert_true((events & EVENT_CMUX_DLCI2_CLOSED), "Failed to close DLCI2");

	/* Discard CMUX DLCI DISC commands */
	modem_backend_mock_reset(&bus_mock);
	zassert_true(modem_cmux_disconnect_async(&cmux) == 0, "Failed to disconnect CMUX");

	k_msleep(100);

	ret = modem_backend_mock_get(&bus_mock, buffer1, sizeof(buffer1));

	zassert_true(ret == sizeof(cmux_frame_control_cld_cmd),
		     "Incorrect number of bytes received for CLD cmd");

	zassert_true(memcmp(buffer1, cmux_frame_control_cld_cmd,
			    sizeof(cmux_frame_control_cld_cmd)) == 0,
		     "Incorrect DLC cmd received");

	modem_backend_mock_put(&bus_mock, cmux_frame_control_cld_ack,
			       sizeof(cmux_frame_control_cld_ack));

	events = k_event_wait_all(&cmux_event, (EVENT_CMUX_DISCONNECTED), false, K_MSEC(100));
	zassert_true((events & EVENT_CMUX_DISCONNECTED), "Failed to disconnect CMUX");

	/* Wait for potential T1 timeout */
	k_msleep(500);

	ret = modem_backend_mock_get(&bus_mock, buffer1, sizeof(buffer1));
	zassert_true(ret == 0, "Received unexpected data");

	/* Reconnect CMUX */
	zassert_true(modem_cmux_connect_async(&cmux) == 0, "Failed to connect CMUX");

	k_msleep(100);

	ret = modem_backend_mock_get(&bus_mock, buffer1, sizeof(buffer1));
	zassert_true(ret == sizeof(cmux_frame_control_sabm_cmd),
		     "Incorrect number of bytes received for SABM cmd");

	zassert_true(memcmp(buffer1, cmux_frame_control_sabm_cmd,
			    sizeof(cmux_frame_control_sabm_cmd)) == 0,
		     "Incorrect SABM cmd received");

	modem_backend_mock_put(&bus_mock, cmux_frame_control_sabm_ack,
			       sizeof(cmux_frame_control_sabm_ack));

	events = k_event_wait_all(&cmux_event, (EVENT_CMUX_CONNECTED), false, K_MSEC(100));
	zassert_true((events & EVENT_CMUX_CONNECTED), "Failed to connect CMUX");

	/* Wait for potential T1 timeout */
	k_msleep(500);

	ret = modem_backend_mock_get(&bus_mock, buffer1, sizeof(buffer1));
	zassert_true(ret == 0, "Received unexpected data");

	/* Open DLCI1 */
	zassert_true(modem_pipe_open_async(dlci1_pipe) == 0, "Failed to open DLCI1 pipe");

	k_msleep(100);

	ret = modem_backend_mock_get(&bus_mock, buffer1, sizeof(buffer1));
	zassert_true(ret == sizeof(cmux_frame_dlci1_sabm_cmd),
		     "Incorrect number of bytes received for DLCI1 open cmd");

	zassert_true(memcmp(buffer1, cmux_frame_dlci1_sabm_cmd,
			    sizeof(cmux_frame_dlci1_sabm_cmd)) == 0,
		     "Incorrect DLCI1 open cmd received");

	modem_backend_mock_put(&bus_mock, cmux_frame_dlci1_sabm_ack,
			       sizeof(cmux_frame_dlci1_sabm_ack));

	events = k_event_wait_all(&cmux_event, (EVENT_CMUX_DLCI1_OPEN),
				  false, K_MSEC(100));

	zassert_true((events & EVENT_CMUX_DLCI1_OPEN),
		     "DLCI1 not opened as expected");

	/* Wait for potential T1 timeout */
	k_msleep(500);

	ret = modem_backend_mock_get(&bus_mock, buffer1, sizeof(buffer1));
	zassert_true(ret == 0, "Received unexpected data");

	/* Open DLCI2 */
	zassert_true(modem_pipe_open_async(dlci2_pipe) == 0, "Failed to open DLCI2 pipe");

	k_msleep(100);

	ret = modem_backend_mock_get(&bus_mock, buffer1, sizeof(buffer1));
	zassert_true(ret == sizeof(cmux_frame_dlci2_sabm_cmd),
		     "Incorrect number of bytes received for DLCI1 open cmd");

	zassert_true(memcmp(buffer1, cmux_frame_dlci2_sabm_cmd,
			    sizeof(cmux_frame_dlci2_sabm_cmd)) == 0,
		     "Incorrect DLCI1 open cmd received");

	modem_backend_mock_put(&bus_mock, cmux_frame_dlci2_sabm_ack,
			       sizeof(cmux_frame_dlci2_sabm_ack));

	events = k_event_wait_all(&cmux_event, (EVENT_CMUX_DLCI2_OPEN),
				  false, K_MSEC(100));

	zassert_true((events & EVENT_CMUX_DLCI2_OPEN),
		     "DLCI1 not opened as expected");

	/* Wait for potential T1 timeout */
	k_msleep(500);

	ret = modem_backend_mock_get(&bus_mock, buffer1, sizeof(buffer1));
	zassert_true(ret == 0, "Received unexpected data");
}

ZTEST(modem_cmux, test_modem_cmux_disconnect_connect_sync)
{
	modem_backend_mock_prime(&bus_mock, &transaction_dlci1_disc);
	zassert_true(modem_pipe_close(dlci1_pipe, K_SECONDS(10)) == 0, "Failed to close DLCI1");
	modem_backend_mock_prime(&bus_mock, &transaction_dlci2_disc);
	zassert_true(modem_pipe_close(dlci2_pipe, K_SECONDS(10)) == 0, "Failed to close DLCI2");
	modem_backend_mock_prime(&bus_mock, &transaction_control_cld);
	zassert_true(modem_cmux_disconnect(&cmux) == 0, "Failed to disconnect CMUX");
	zassert_true(modem_cmux_disconnect(&cmux) == -EALREADY,
		     "Should already be disconnected");

	modem_backend_mock_prime(&bus_mock, &transaction_control_sabm);
	zassert_true(modem_cmux_connect(&cmux) == 0, "Failed to connect CMUX");
	zassert_true(modem_cmux_connect(&cmux) == -EALREADY,
		     "Should already be connected");

	modem_backend_mock_prime(&bus_mock, &transaction_dlci1_sabm);
	zassert_true(modem_pipe_open(dlci1_pipe, K_SECONDS(10)) == 0,
		     "Failed to open DLCI1 pipe");
	modem_backend_mock_prime(&bus_mock, &transaction_dlci2_sabm);
	zassert_true(modem_pipe_open(dlci2_pipe, K_SECONDS(10)) == 0,
		     "Failed to open DLCI2 pipe");
}

ZTEST(modem_cmux, test_modem_cmux_dlci_close_open_sync)
{
	modem_backend_mock_prime(&bus_mock, &transaction_dlci1_disc);
	zassert_true(modem_pipe_close(dlci1_pipe, K_SECONDS(10)) == 0, "Failed to close DLCI1");
	modem_backend_mock_prime(&bus_mock, &transaction_dlci2_disc);
	zassert_true(modem_pipe_close(dlci2_pipe, K_SECONDS(10)) == 0, "Failed to close DLCI2");
	modem_backend_mock_prime(&bus_mock, &transaction_dlci1_sabm);
	zassert_true(modem_pipe_open(dlci1_pipe, K_SECONDS(10)) == 0,
		     "Failed to open DLCI1 pipe");
	modem_backend_mock_prime(&bus_mock, &transaction_dlci2_sabm);
	zassert_true(modem_pipe_open(dlci2_pipe, K_SECONDS(10)) == 0,
		     "Failed to open DLCI2 pipe");
}

ZTEST(modem_cmux, test_modem_cmux_prevent_work_while_released)
{
	const uint8_t transmit[2];
	uint8_t receive[2];

	/* Disconnect CMUX */
	modem_backend_mock_prime(&bus_mock, &transaction_control_cld);
	zassert_ok(modem_cmux_disconnect(&cmux));

	/* Start work to connect CMUX and open DLCI channels */
	zassert_ok(modem_cmux_connect_async(&cmux));
	zassert_ok(modem_pipe_open_async(dlci1_pipe));
	zassert_ok(modem_pipe_open_async(dlci2_pipe));

	/* Wait for and validate CMUX is sending requests */
	k_msleep(500);
	zassert_true(modem_backend_mock_get(&bus_mock, buffer1, sizeof(buffer1)) > 0);

	/* Release CMUX and validate no more requests are sent */
	modem_cmux_release(&cmux);
	modem_backend_mock_get(&bus_mock, buffer1, sizeof(buffer1));
	k_msleep(500);
	zassert_true(modem_backend_mock_get(&bus_mock, buffer1, sizeof(buffer1)) == 0);

	/* Validate no new requests can be submitted */
	modem_cmux_connect(&cmux);
	modem_cmux_disconnect(&cmux);
	modem_pipe_open(dlci1_pipe, K_SECONDS(10));
	modem_pipe_open(dlci2_pipe, K_SECONDS(10));
	modem_pipe_transmit(dlci1_pipe, transmit, sizeof(transmit));
	modem_pipe_transmit(dlci2_pipe, transmit, sizeof(transmit));
	modem_pipe_receive(dlci1_pipe, receive, sizeof(receive));
	modem_pipe_receive(dlci2_pipe, receive, sizeof(receive));
	modem_pipe_close(dlci1_pipe, K_SECONDS(10));
	modem_pipe_close(dlci2_pipe, K_SECONDS(10));
	k_msleep(500);
	zassert_true(modem_backend_mock_get(&bus_mock, buffer1, sizeof(buffer1)) == 0);

	/* Restore CMUX */
	zassert_ok(modem_cmux_attach(&cmux, bus_mock_pipe));
	modem_backend_mock_prime(&bus_mock, &transaction_control_sabm);
	zassert_ok(modem_cmux_connect(&cmux));
	modem_backend_mock_prime(&bus_mock, &transaction_dlci1_sabm);
	zassert_ok(modem_pipe_open(dlci1_pipe, K_SECONDS(10)));
	modem_backend_mock_prime(&bus_mock, &transaction_dlci2_sabm);
	zassert_ok(modem_pipe_open(dlci2_pipe, K_SECONDS(10)));
}

ZTEST(modem_cmux, test_modem_drop_frames_with_invalid_length)
{
	int ret;
	uint32_t events;

	modem_backend_mock_put(&bus_mock, cmux_frame_dlci2_at_cgdcont_invalid_length,
			       sizeof(cmux_frame_dlci2_at_cgdcont_invalid_length));

	k_msleep(100);

	events = k_event_test(&cmux_event, EVENT_CMUX_DLCI2_RECEIVE_READY);

	zassert_false(events & EVENT_CMUX_DLCI2_RECEIVE_READY,
		      "Receive event should not have been received for DLCI2 pipe");

	modem_backend_mock_put(&bus_mock, cmux_frame_dlci2_at_cgdcont,
			       sizeof(cmux_frame_dlci2_at_cgdcont));

	modem_backend_mock_put(&bus_mock, cmux_frame_dlci2_at_newline,
			       sizeof(cmux_frame_dlci2_at_newline));

	k_msleep(100);

	events = k_event_test(&cmux_event, EVENT_CMUX_DLCI2_RECEIVE_READY);
	zassert_equal(events, EVENT_CMUX_DLCI2_RECEIVE_READY,
		      "Receive ready event not received for DLCI2 pipe");

	ret = modem_pipe_receive(dlci2_pipe, buffer2, sizeof(buffer2));
	zassert_true(ret == (sizeof(cmux_frame_data_dlci2_at_cgdcont) +
			     sizeof(cmux_frame_data_dlci2_at_newline)),
		     "Incorrect number of bytes received");

	zassert_true(memcmp(buffer2, cmux_frame_data_dlci2_at_cgdcont,
			    sizeof(cmux_frame_data_dlci2_at_cgdcont)) == 0,
		     "Incorrect data received");

	zassert_true(memcmp(&buffer2[sizeof(cmux_frame_data_dlci2_at_cgdcont)],
			    cmux_frame_data_dlci2_at_newline,
			    sizeof(cmux_frame_data_dlci2_at_newline)) == 0,
		     "Incorrect data received");
}

ZTEST(modem_cmux, test_modem_cmux_split_large_data)
{
	int ret;
	uint32_t events;

	ret = modem_pipe_transmit(dlci2_pipe, cmux_frame_data_large,
				  sizeof(cmux_frame_data_large));
	zassert_true(ret == CONFIG_MODEM_CMUX_MTU, "Failed to split large data %d", ret);

	events = k_event_wait(&cmux_event, EVENT_CMUX_DLCI2_TRANSMIT_IDLE, false, K_MSEC(200));
	zassert_equal(events, EVENT_CMUX_DLCI2_TRANSMIT_IDLE,
		      "Transmit idle event not received for DLCI2 pipe");

	ret = modem_backend_mock_get(&bus_mock, buffer2, sizeof(buffer2));
	zassert_true(ret == CONFIG_MODEM_CMUX_MTU + CMUX_BASIC_HRD_SMALL_SIZE,
		     "Incorrect number of bytes transmitted %d", ret);
}

ZTEST_SUITE(modem_cmux, NULL, test_modem_cmux_setup, test_modem_cmux_before, NULL, NULL);
