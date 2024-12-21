/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <string.h>

#include <zephyr/modem/cmux.h>
#include <modem_backend_mock.h>

/* CMUX state flags */
#define EVENT_CMUX_CONNECTED     BIT(0)
#define EVENT_CMUX_DLCI1_OPEN    BIT(1)
#define EVENT_CMUX_DLCI2_OPEN    BIT(2)
#define EVENT_CMUX_DLCI1_CLOSED  BIT(3)
#define EVENT_CMUX_DLCI2_CLOSED  BIT(4)
#define EVENT_CMUX_DISCONNECTED  BIT(5)
#define EVENT_CMUX_DLCI1_RX_DATA BIT(6)
#define EVENT_CMUX_DLCI2_RX_DATA BIT(7)

/* CMUX DTE variables */
static struct modem_cmux cmux_dte;
static uint8_t cmux_receive_buf[127];
static uint8_t cmux_transmit_buf[149];
static struct modem_cmux_dlci dlci1;
static struct modem_cmux_dlci dlci2;
static struct modem_pipe *dlci1_pipe;
static struct modem_pipe *dlci2_pipe;

/* CMUX DCE variables */
static struct modem_cmux cmux_dce;
static uint8_t cmux_receive_buf_dce[127];
static uint8_t cmux_transmit_buf_dce[149];
static struct modem_cmux_dlci dlci1_dce;
static struct modem_cmux_dlci dlci2_dce;
static struct modem_pipe *dlci1_pipe_dce;
static struct modem_pipe *dlci2_pipe_dce;

/* DTE & DCE Event */
static struct k_event cmux_event_dte;
static struct k_event cmux_event_dce;

/* Backend MOCK */
static struct modem_backend_mock bus_mock_dte;
static struct modem_backend_mock bus_mock_dce;
static uint8_t bus_mock_rx_buf[2048];
static uint8_t bus_mock_tx_buf[2048];
static uint8_t bus_mock_rx_buf_dce[2048];
static uint8_t bus_mock_tx_buf_dce[2048];
static struct modem_pipe *bus_mock_pipe;
static struct modem_pipe *bus_mock_pipe_dce;

static uint8_t dlci1_receive_buf[127];
static uint8_t dlci2_receive_buf[127];
static uint8_t dlci1_receive_buf_dce[127];
static uint8_t dlci2_receive_buf_dce[127];

static uint8_t buffer1[2048];
static uint8_t buffer2[2048];

static void test_dlci1_pipe_cb(struct modem_pipe *pipe, enum modem_pipe_event event,
			       void *user_data)
{
	switch (event) {
	case MODEM_PIPE_EVENT_OPENED:
		k_event_post(&cmux_event_dte, EVENT_CMUX_DLCI1_OPEN);
		break;

	case MODEM_PIPE_EVENT_CLOSED:
		k_event_post(&cmux_event_dte, EVENT_CMUX_DLCI1_CLOSED);
		break;

	case MODEM_PIPE_EVENT_RECEIVE_READY:
		k_event_post(&cmux_event_dte, EVENT_CMUX_DLCI1_RX_DATA);
		break;

	default:
		break;
	}
}

static void test_dlci2_pipe_cb(struct modem_pipe *pipe, enum modem_pipe_event event,
			       void *user_data)
{
	switch (event) {
	case MODEM_PIPE_EVENT_OPENED:
		k_event_post(&cmux_event_dte, EVENT_CMUX_DLCI2_OPEN);
		break;

	case MODEM_PIPE_EVENT_CLOSED:
		k_event_post(&cmux_event_dte, EVENT_CMUX_DLCI2_CLOSED);
		break;
	case MODEM_PIPE_EVENT_RECEIVE_READY:
		k_event_post(&cmux_event_dte, EVENT_CMUX_DLCI2_RX_DATA);
		break;

	default:
		break;
	}
}

static void test_dlci1_pipe_cb_dce(struct modem_pipe *pipe, enum modem_pipe_event event,
				   void *user_data)
{
	switch (event) {
	case MODEM_PIPE_EVENT_OPENED:
		k_event_post(&cmux_event_dce, EVENT_CMUX_DLCI1_OPEN);
		break;

	case MODEM_PIPE_EVENT_CLOSED:
		k_event_post(&cmux_event_dce, EVENT_CMUX_DLCI1_CLOSED);
		break;

	case MODEM_PIPE_EVENT_RECEIVE_READY:
		k_event_post(&cmux_event_dce, EVENT_CMUX_DLCI1_RX_DATA);
		break;

	default:
		break;
	}
}

static void test_dlci2_pipe_cb_dce(struct modem_pipe *pipe, enum modem_pipe_event event,
				   void *user_data)
{
	switch (event) {
	case MODEM_PIPE_EVENT_OPENED:
		k_event_post(&cmux_event_dce, EVENT_CMUX_DLCI2_OPEN);
		break;

	case MODEM_PIPE_EVENT_CLOSED:
		k_event_post(&cmux_event_dce, EVENT_CMUX_DLCI2_CLOSED);
		break;
	case MODEM_PIPE_EVENT_RECEIVE_READY:
		k_event_post(&cmux_event_dce, EVENT_CMUX_DLCI2_RX_DATA);
		break;

	default:
		break;
	}
}

/*************************************************************************************************/
/*                                     DLCI2 AT CMUX frames                                      */
/*************************************************************************************************/
static uint8_t cmux_frame_data_dlci2_at_cgdcont[] = {
	0x41, 0x54, 0x2B, 0x43, 0x47, 0x44, 0x43, 0x4F, 0x4E, 0x54, 0x3D,
	0x31, 0x2C, 0x22, 0x49, 0x50, 0x22, 0x2C, 0x22, 0x74, 0x72, 0x61,
	0x63, 0x6B, 0x75, 0x6E, 0x69, 0x74, 0x2E, 0x6D, 0x32, 0x6D, 0x22};

static uint8_t cmux_frame_data_dlci2_at_newline[] = {0x0D, 0x0A};

/*************************************************************************************************/
/*                                    DLCI1 AT CMUX frames                                       */
/*************************************************************************************************/
static uint8_t cmux_frame_data_dlci1_at_at[] = {0x41, 0x54};

static uint8_t cmux_frame_data_dlci1_at_newline[] = {0x0D, 0x0A};

/*************************************************************************************************/
/*                                   DLCI2 PPP CMUX frames                                       */
/*************************************************************************************************/
static uint8_t cmux_frame_data_dlci2_ppp_52[] = {
	0x7E, 0xFF, 0x7D, 0x23, 0xC0, 0x21, 0x7D, 0x21, 0x7D, 0x20, 0x7D, 0x20, 0x7D,
	0x38, 0x7D, 0x22, 0x7D, 0x26, 0x7D, 0x20, 0x7D, 0x20, 0x7D, 0x20, 0x7D, 0x20,
	0x7D, 0x23, 0x7D, 0x24, 0xC0, 0x23, 0x7D, 0x25, 0x7D, 0x26, 0x53, 0x96, 0x7D,
	0x38, 0xAA, 0x7D, 0x27, 0x7D, 0x22, 0x7D, 0x28, 0x7D, 0x22, 0xD5, 0xA8, 0x7E};

static uint8_t cmux_frame_data_dlci2_ppp_18[] = {0x7E, 0xFF, 0x7D, 0x23, 0xC0, 0x21,
						 0x7D, 0x22, 0x7D, 0x21, 0x7D, 0x20,
						 0x7D, 0x24, 0x7D, 0x3C, 0x90, 0x7E};

static void test_cmux_ctrl_cb(struct modem_cmux *cmux, enum modem_cmux_event event, void *user_data)
{
	if (event == MODEM_CMUX_EVENT_CONNECTED) {
		k_event_post(&cmux_event_dte, EVENT_CMUX_CONNECTED);
		return;
	}

	if (event == MODEM_CMUX_EVENT_DISCONNECTED) {
		k_event_post(&cmux_event_dte, EVENT_CMUX_DISCONNECTED);
		return;
	}
}

static void test_cmux_ctrl_cb_dce(struct modem_cmux *cmux, enum modem_cmux_event event,
				  void *user_data)
{
	if (event == MODEM_CMUX_EVENT_CONNECTED) {
		k_event_post(&cmux_event_dce, EVENT_CMUX_CONNECTED);
		return;
	}

	if (event == MODEM_CMUX_EVENT_DISCONNECTED) {
		k_event_post(&cmux_event_dce, EVENT_CMUX_DISCONNECTED);
		return;
	}
}

static void cmux_dte_init(void)
{
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

	struct modem_cmux_config cmux_config = {
		.callback = test_cmux_ctrl_cb,
		.user_data = NULL,
		.receive_buf = cmux_receive_buf,
		.receive_buf_size = sizeof(cmux_receive_buf),
		.transmit_buf = cmux_transmit_buf,
		.transmit_buf_size = ARRAY_SIZE(cmux_transmit_buf),
	};

	const struct modem_backend_mock_config bus_mock_config = {
		.rx_buf = bus_mock_rx_buf,
		.rx_buf_size = sizeof(bus_mock_rx_buf),
		.tx_buf = bus_mock_tx_buf,
		.tx_buf_size = sizeof(bus_mock_tx_buf),
		.limit = 32,
	};

	modem_cmux_init(&cmux_dte, &cmux_config);
	dlci1_pipe = modem_cmux_dlci_init(&cmux_dte, &dlci1, &dlci1_config);
	dlci2_pipe = modem_cmux_dlci_init(&cmux_dte, &dlci2, &dlci2_config);
	/* Init Backend DTE */
	bus_mock_pipe = modem_backend_mock_init(&bus_mock_dte, &bus_mock_config);
	__ASSERT_NO_MSG(modem_pipe_open(bus_mock_pipe, K_SECONDS(10)) == 0);
	__ASSERT_NO_MSG(modem_cmux_attach(&cmux_dte, bus_mock_pipe) == 0);
	modem_pipe_attach(dlci1_pipe, test_dlci1_pipe_cb, NULL);
	modem_pipe_attach(dlci2_pipe, test_dlci2_pipe_cb, NULL);
}

static void cmux_dce_init(void)
{
	struct modem_cmux_dlci_config dlci1_config = {
		.dlci_address = 1,
		.receive_buf = dlci1_receive_buf_dce,
		.receive_buf_size = sizeof(dlci1_receive_buf_dce),
	};

	struct modem_cmux_dlci_config dlci2_config = {
		.dlci_address = 2,
		.receive_buf = dlci2_receive_buf_dce,
		.receive_buf_size = sizeof(dlci2_receive_buf_dce),
	};

	struct modem_cmux_config cmux_config_dce = {
		.callback = test_cmux_ctrl_cb_dce,
		.user_data = NULL,
		.receive_buf = cmux_receive_buf_dce,
		.receive_buf_size = sizeof(cmux_receive_buf_dce),
		.transmit_buf = cmux_transmit_buf_dce,
		.transmit_buf_size = ARRAY_SIZE(cmux_transmit_buf_dce),
	};

	const struct modem_backend_mock_config bus_mock_config = {
		.rx_buf = bus_mock_rx_buf_dce,
		.rx_buf_size = sizeof(bus_mock_rx_buf_dce),
		.tx_buf = bus_mock_tx_buf_dce,
		.tx_buf_size = sizeof(bus_mock_tx_buf_dce),
		.limit = 32,
	};

	modem_cmux_init(&cmux_dce, &cmux_config_dce);
	dlci1_pipe_dce = modem_cmux_dlci_init(&cmux_dce, &dlci1_dce, &dlci1_config);
	dlci2_pipe_dce = modem_cmux_dlci_init(&cmux_dce, &dlci2_dce, &dlci2_config);
	/* Init Backend DCE */
	bus_mock_pipe_dce = modem_backend_mock_init(&bus_mock_dce, &bus_mock_config);
	__ASSERT_NO_MSG(modem_pipe_open(bus_mock_pipe_dce, K_SECONDS(10)) == 0);
	__ASSERT_NO_MSG(modem_cmux_attach(&cmux_dce, bus_mock_pipe_dce) == 0);
	modem_pipe_attach(dlci1_pipe_dce, test_dlci1_pipe_cb_dce, NULL);
	modem_pipe_attach(dlci2_pipe_dce, test_dlci2_pipe_cb_dce, NULL);
}

static void *test_setup(void)
{
	uint32_t events;

	/* Init Event mask's */
	k_event_init(&cmux_event_dte);
	k_event_init(&cmux_event_dce);

	/* Init CMUX, Pipe and Backend instances */
	cmux_dte_init();
	cmux_dce_init();

	/* Create MOCK bridge */
	modem_backend_mock_bridge(&bus_mock_dte, &bus_mock_dce);

	/* Connect CMUX by DTE */
	__ASSERT_NO_MSG(modem_cmux_connect_async(&cmux_dte) == 0);
	events = k_event_wait(&cmux_event_dte, EVENT_CMUX_CONNECTED, false, K_MSEC(100));
	__ASSERT_NO_MSG((events & EVENT_CMUX_CONNECTED));
	events = k_event_wait(&cmux_event_dce, EVENT_CMUX_CONNECTED, false, K_MSEC(100));
	__ASSERT_NO_MSG((events & EVENT_CMUX_CONNECTED));

	/* Open DLCI channels init by DTE */
	__ASSERT_NO_MSG(modem_pipe_open_async(dlci1_pipe) == 0);
	events = k_event_wait(&cmux_event_dte, EVENT_CMUX_DLCI1_OPEN, false, K_MSEC(100));
	__ASSERT_NO_MSG((events & EVENT_CMUX_DLCI1_OPEN));
	events = k_event_wait(&cmux_event_dce, EVENT_CMUX_DLCI1_OPEN, false, K_MSEC(100));
	__ASSERT_NO_MSG((events & EVENT_CMUX_DLCI1_OPEN));

	__ASSERT_NO_MSG(modem_pipe_open_async(dlci2_pipe) == 0);
	events = k_event_wait(&cmux_event_dte, EVENT_CMUX_DLCI2_OPEN, false, K_MSEC(100));
	__ASSERT_NO_MSG((events & EVENT_CMUX_DLCI2_OPEN));
	events = k_event_wait(&cmux_event_dce, EVENT_CMUX_DLCI2_OPEN, false, K_MSEC(100));
	__ASSERT_NO_MSG((events & EVENT_CMUX_DLCI2_OPEN));

	return NULL;
}

static void test_before(void *f)
{
	/* Reset events */
	k_event_clear(&cmux_event_dte, UINT32_MAX);
	k_event_clear(&cmux_event_dce, UINT32_MAX);

	/* Reset mock pipes */
	modem_backend_mock_reset(&bus_mock_dte);
	modem_backend_mock_reset(&bus_mock_dce);
}

ZTEST(modem_cmux_pair, test_modem_cmux_dce_receive_dlci2_at)
{
	int ret;
	uint32_t events;

	modem_pipe_transmit(dlci2_pipe, cmux_frame_data_dlci2_at_cgdcont,
			    sizeof(cmux_frame_data_dlci2_at_cgdcont));

	modem_pipe_transmit(dlci2_pipe, cmux_frame_data_dlci2_at_newline,
			    sizeof(cmux_frame_data_dlci2_at_newline));

	k_msleep(100);

	events = k_event_wait(&cmux_event_dce, EVENT_CMUX_DLCI2_RX_DATA, false, K_MSEC(100));
	zassert_true((events & EVENT_CMUX_DLCI2_RX_DATA), "DLCI2 dce not rx data");

	ret = modem_pipe_receive(dlci2_pipe_dce, buffer2, sizeof(buffer2));
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

ZTEST(modem_cmux_pair, test_modem_cmux_dce_receive_dlci1_at)
{
	int ret;
	uint32_t events;

	modem_pipe_transmit(dlci1_pipe, cmux_frame_data_dlci1_at_at,
			    sizeof(cmux_frame_data_dlci1_at_at));
	modem_pipe_transmit(dlci1_pipe, cmux_frame_data_dlci1_at_newline,
			    sizeof(cmux_frame_data_dlci1_at_newline));

	k_msleep(100);

	events = k_event_wait(&cmux_event_dce, EVENT_CMUX_DLCI1_RX_DATA, false, K_MSEC(100));
	zassert_true((events & EVENT_CMUX_DLCI1_RX_DATA), "DLCI1 dce not rx data");

	ret = modem_pipe_receive(dlci1_pipe_dce, buffer1, sizeof(buffer1));
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

ZTEST(modem_cmux_pair, test_modem_cmux_dce_receive_dlci2_ppp)
{
	int ret;

	uint32_t events;

	modem_pipe_transmit(dlci2_pipe, cmux_frame_data_dlci2_ppp_52,
			    sizeof(cmux_frame_data_dlci2_ppp_52));
	modem_pipe_transmit(dlci2_pipe, cmux_frame_data_dlci2_ppp_18,
			    sizeof(cmux_frame_data_dlci2_ppp_18));

	k_msleep(100);

	events = k_event_wait(&cmux_event_dce, EVENT_CMUX_DLCI2_RX_DATA, false, K_MSEC(100));
	zassert_true((events & EVENT_CMUX_DLCI2_RX_DATA), "DLCI2 dce not rx data");

	ret = modem_pipe_receive(dlci2_pipe_dce, buffer2, sizeof(buffer2));
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

ZTEST(modem_cmux_pair, test_modem_cmux_dce_transmit_dlci2_ppp)
{
	int ret;
	uint32_t events;

	ret = modem_pipe_transmit(dlci2_pipe_dce, cmux_frame_data_dlci2_ppp_52,
				  sizeof(cmux_frame_data_dlci2_ppp_52));

	zassert_true(ret == sizeof(cmux_frame_data_dlci2_ppp_52), "Failed to send DLCI2 PPP 52 %d",
		     ret);
	ret = modem_pipe_transmit(dlci2_pipe_dce, cmux_frame_data_dlci2_ppp_18,
				  sizeof(cmux_frame_data_dlci2_ppp_18));

	zassert_true(ret == sizeof(cmux_frame_data_dlci2_ppp_18), "Failed to send DLCI2 PPP 18");

	k_msleep(100);

	events = k_event_wait(&cmux_event_dte, EVENT_CMUX_DLCI2_RX_DATA, false, K_MSEC(100));
	zassert_true((events & EVENT_CMUX_DLCI2_RX_DATA), "DLCI2 dce not rx data");

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

ZTEST(modem_cmux_pair, test_modem_cmux_dlci1_close_open)
{
	uint32_t events;

	/* Close DLCI1 */
	zassert_true(modem_pipe_close_async(dlci1_pipe) == 0, "Failed to close DLCI1 pipe");
	k_msleep(100);
	events = k_event_wait_all(&cmux_event_dte, (EVENT_CMUX_DLCI1_CLOSED), false, K_MSEC(100));

	zassert_true((events & EVENT_CMUX_DLCI1_CLOSED), "DLCI1 not closed as expected");
	events = k_event_wait_all(&cmux_event_dce, (EVENT_CMUX_DLCI1_CLOSED), false, K_MSEC(100));

	zassert_true((events & EVENT_CMUX_DLCI1_CLOSED), "DLCI1 not closed as expected");

	/* Open DLCI1 */
	zassert_true(modem_pipe_open_async(dlci1_pipe) == 0, "Failed to open DLCI1 pipe");

	k_msleep(100);

	events = k_event_wait_all(&cmux_event_dte, (EVENT_CMUX_DLCI1_OPEN), false, K_MSEC(100));

	zassert_true((events & EVENT_CMUX_DLCI1_OPEN), "DLCI1 not opened as expected");
	events = k_event_wait_all(&cmux_event_dce, (EVENT_CMUX_DLCI1_OPEN), false, K_MSEC(100));

	zassert_true((events & EVENT_CMUX_DLCI1_OPEN), "DLCI1 not opened as expected");
}

ZTEST(modem_cmux_pair, test_modem_cmux_disconnect_connect)
{
	uint32_t events;

	/* Disconnect CMUX */
	zassert_true(modem_pipe_close_async(dlci1_pipe) == 0, "Failed to close DLCI1");
	zassert_true(modem_pipe_close_async(dlci2_pipe) == 0, "Failed to close DLCI2");
	k_msleep(100);

	events = k_event_wait_all(&cmux_event_dte,
				  (EVENT_CMUX_DLCI1_CLOSED | EVENT_CMUX_DLCI2_CLOSED), false,
				  K_MSEC(100));

	zassert_true((events & EVENT_CMUX_DLCI1_CLOSED), "Failed to close DLCI1");
	zassert_true((events & EVENT_CMUX_DLCI2_CLOSED), "Failed to close DLCI2");

	/* Discard CMUX DLCI DISC commands */
	modem_backend_mock_reset(&bus_mock_dte);
	zassert_true(modem_cmux_disconnect_async(&cmux_dte) == 0, "Failed to disconnect CMUX");

	k_msleep(100);

	events = k_event_wait_all(&cmux_event_dte, (EVENT_CMUX_DISCONNECTED), false, K_MSEC(100));
	zassert_true((events & EVENT_CMUX_DISCONNECTED), "Failed to disconnect CMUX");

	/* Reconnect CMUX */
	zassert_true(modem_cmux_connect_async(&cmux_dte) == 0, "Failed to connect CMUX");

	k_msleep(100);

	events = k_event_wait_all(&cmux_event_dte, (EVENT_CMUX_CONNECTED), false, K_MSEC(100));
	zassert_true((events & EVENT_CMUX_CONNECTED), "Failed to connect CMUX");

	/* Open DLCI1 */
	zassert_true(modem_pipe_open_async(dlci1_pipe) == 0, "Failed to open DLCI1 pipe");

	k_msleep(100);

	events = k_event_wait_all(&cmux_event_dte, (EVENT_CMUX_DLCI1_OPEN), false, K_MSEC(100));

	zassert_true((events & EVENT_CMUX_DLCI1_OPEN), "DLCI1 not opened as expected");

	/* Open DLCI2 */
	zassert_true(modem_pipe_open_async(dlci2_pipe) == 0, "Failed to open DLCI2 pipe");

	k_msleep(100);

	events = k_event_wait_all(&cmux_event_dte, (EVENT_CMUX_DLCI2_OPEN), false, K_MSEC(100));

	zassert_true((events & EVENT_CMUX_DLCI2_OPEN), "DLCI1 not opened as expected");
}

ZTEST(modem_cmux_pair, test_modem_cmux_disconnect_connect_sync)
{
	uint32_t events;

	zassert_true(modem_pipe_close(dlci1_pipe, K_SECONDS(10)) == 0, "Failed to close DLCI1");
	zassert_true(modem_pipe_close(dlci2_pipe, K_SECONDS(10)) == 0, "Failed to close DLCI2");
	events = k_event_wait_all(&cmux_event_dce,
				  (EVENT_CMUX_DLCI1_CLOSED | EVENT_CMUX_DLCI2_CLOSED), false,
				  K_MSEC(100));
	zassert_true((events & EVENT_CMUX_DLCI1_CLOSED), "DCE DLCI1 not closed as expected");
	zassert_true((events & EVENT_CMUX_DLCI2_CLOSED), "DCE DLCI2 not closed as expected");

	zassert_true(modem_cmux_disconnect(&cmux_dte) == 0, "Failed to disconnect CMUX");
	zassert_true(modem_cmux_disconnect(&cmux_dte) == -EALREADY,
		     "Should already be disconnected");
	zassert_true(modem_cmux_disconnect(&cmux_dce) == -EALREADY,
		     "Should already be disconnected");

	k_msleep(100);

	zassert_true(modem_cmux_connect(&cmux_dte) == 0, "Failed to connect CMUX");
	zassert_true(modem_cmux_connect(&cmux_dte) == -EALREADY, "Should already be connected");
	zassert_true(modem_cmux_connect(&cmux_dce) == -EALREADY, "Should already be connected");

	zassert_true(modem_pipe_open(dlci1_pipe, K_SECONDS(10)) == 0, "Failed to open DLCI1 pipe");
	zassert_true(modem_pipe_open(dlci2_pipe, K_SECONDS(10)) == 0, "Failed to open DLCI2 pipe");
	events = k_event_wait_all(&cmux_event_dce, (EVENT_CMUX_DLCI1_OPEN | EVENT_CMUX_DLCI2_OPEN),
				  false, K_MSEC(100));
	zassert_true((events & EVENT_CMUX_DLCI1_OPEN), "DCE DLCI1 not open as expected");
	zassert_true((events & EVENT_CMUX_DLCI2_OPEN), "DCE DLCI2 not open as expected");
}

ZTEST(modem_cmux_pair, test_modem_cmux_dlci_close_open_sync)
{
	uint32_t events;

	zassert_true(modem_pipe_close(dlci1_pipe, K_SECONDS(10)) == 0, "Failed to close DLCI1");
	zassert_true(modem_pipe_close(dlci2_pipe, K_SECONDS(10)) == 0, "Failed to close DLCI2");

	events = k_event_wait_all(&cmux_event_dce,
				  (EVENT_CMUX_DLCI1_CLOSED | EVENT_CMUX_DLCI2_CLOSED), false,
				  K_MSEC(100));
	zassert_true((events & EVENT_CMUX_DLCI1_CLOSED), "DCE DLCI1 not closed as expected");
	zassert_true((events & EVENT_CMUX_DLCI2_CLOSED), "DCE DLCI2 not closed as expected");

	zassert_true(modem_pipe_open(dlci1_pipe, K_SECONDS(10)) == 0, "Failed to open DLCI1 pipe");
	zassert_true(modem_pipe_open(dlci2_pipe, K_SECONDS(10)) == 0, "Failed to open DLCI2 pipe");
	/* Verify DCE side channels are open also */
	events = k_event_wait_all(&cmux_event_dce, (EVENT_CMUX_DLCI1_OPEN | EVENT_CMUX_DLCI2_OPEN),
				  false, K_MSEC(100));
	zassert_true((events & EVENT_CMUX_DLCI1_OPEN), "DCE DLCI1 not open as expected");
	zassert_true((events & EVENT_CMUX_DLCI2_OPEN), "DCE DLCI2 not open as expected");
}

ZTEST_SUITE(modem_cmux_pair, NULL, test_setup, test_before, NULL, NULL);
