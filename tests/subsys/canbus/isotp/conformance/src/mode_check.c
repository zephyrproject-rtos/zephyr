/*
 * Copyright (c) 2023 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This test suite checks that correct errors are returned when trying to use the ISO-TP
 * protocol with CAN-FD mode even though the controller does not support CAN-FD.
 */

#include <zephyr/canbus/isotp.h>
#include <zephyr/ztest.h>

static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
static struct isotp_recv_ctx recv_ctx;
static struct isotp_send_ctx send_ctx;
static bool canfd_capable;

static const struct isotp_fc_opts fc_opts = {
	.bs = 0,
	.stmin = 0
};

static const struct isotp_msg_id rx_addr = {
	.std_id = 0x20,
#ifdef CONFIG_TEST_USE_CAN_FD_MODE
	.dl = CONFIG_TEST_ISOTP_TX_DL,
	.flags = ISOTP_MSG_FDF | ISOTP_MSG_BRS,
#endif
};

static const struct isotp_msg_id tx_addr = {
	.std_id = 0x21,
#ifdef CONFIG_TEST_USE_CAN_FD_MODE
	.dl = CONFIG_TEST_ISOTP_TX_DL,
	.flags = ISOTP_MSG_FDF | ISOTP_MSG_BRS,
#endif
};

ZTEST(isotp_conformance_mode_check, test_bind)
{
	int err;

	err = isotp_bind(&recv_ctx, can_dev, &rx_addr, &tx_addr, &fc_opts, K_NO_WAIT);
	if (IS_ENABLED(CONFIG_TEST_USE_CAN_FD_MODE) && !canfd_capable) {
		zassert_equal(err, ISOTP_N_ERROR);
	} else {
		zassert_equal(err, ISOTP_N_OK);
	}

	isotp_unbind(&recv_ctx);
}

ZTEST(isotp_conformance_mode_check, test_send)
{
	uint8_t buf[] = { 1, 2, 3 };
	int err;

	err = isotp_send(&send_ctx, can_dev, buf, sizeof(buf), &rx_addr, &tx_addr, NULL, NULL);
	if (IS_ENABLED(CONFIG_TEST_USE_CAN_FD_MODE) && !canfd_capable) {
		zassert_equal(err, ISOTP_N_ERROR);
	} else {
		zassert_equal(err, ISOTP_N_OK);
	}
}

static void *isotp_conformance_mode_check_setup(void)
{
	can_mode_t cap;
	int err;

	zassert_true(device_is_ready(can_dev), "CAN device not ready");

	err = can_get_capabilities(can_dev, &cap);
	zassert_equal(err, 0, "failed to get CAN controller capabilities (err %d)", err);

	canfd_capable = (cap & CAN_MODE_FD) != 0;

	(void)can_stop(can_dev);

	err = can_set_mode(can_dev, CAN_MODE_LOOPBACK | (canfd_capable ? CAN_MODE_FD : 0));
	zassert_equal(err, 0, "Failed to set mode [%d]", err);

	err = can_start(can_dev);
	zassert_equal(err, 0, "Failed to start CAN controller [%d]", err);

	return NULL;
}

ZTEST_SUITE(isotp_conformance_mode_check, NULL, isotp_conformance_mode_check_setup, NULL, NULL,
	    NULL);
