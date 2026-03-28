/*
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/canbus/isotp.h>
#include <zephyr/drivers/can.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>
#include <strings.h>
#include "random_data.h"

#if !defined(CONFIG_TEST_USE_CAN_FD_MODE) || CONFIG_TEST_ISOTP_TX_DL == 8
#define DATA_SIZE_SF      7
#define DATA_SIZE_CF      7
#define DATA_SIZE_SF_EXT  6
#define DATA_SIZE_FF      6
#define TX_DL             8
#define DATA_SEND_LENGTH  272
#define SF_PCI_BYTE_1     ((SF_PCI_TYPE << PCI_TYPE_POS) | DATA_SIZE_SF)
#define SF_PCI_BYTE_2_EXT ((SF_PCI_TYPE << PCI_TYPE_POS) | DATA_SIZE_SF_EXT)
#define SF_PCI_BYTE_LEN_8 ((SF_PCI_TYPE << PCI_TYPE_POS) | (DATA_SIZE_SF + 1))
#else
#define DATA_SIZE_SF      (TX_DL - 2)
#define DATA_SIZE_CF      (TX_DL - 1)
#define DATA_SIZE_SF_EXT  (TX_DL - 3)
#define DATA_SIZE_FF      (TX_DL - 2)
#define TX_DL             CONFIG_TEST_ISOTP_TX_DL
/* Send length must be larger than FF + (8 * CF).
 * But not so big that the remainder cannot fit into the buffers.
 */
#define DATA_SEND_LENGTH  (100 + DATA_SIZE_FF + (8 * DATA_SIZE_CF))
#define SF_PCI_BYTE_1     (SF_PCI_TYPE << PCI_TYPE_POS)
#define SF_PCI_BYTE_2     DATA_SIZE_SF
#define SF_PCI_BYTE_2_EXT (SF_PCI_TYPE << PCI_TYPE_POS)
#define SF_PCI_BYTE_3_EXT DATA_SIZE_SF_EXT
#endif

#define DATA_SIZE_FC      3
#define PCI_TYPE_POS      4
#define SF_PCI_TYPE       0
#define EXT_ADDR           5
#define FF_PCI_TYPE        1
#define FF_PCI_BYTE_1(dl)      ((FF_PCI_TYPE << PCI_TYPE_POS) | ((dl) >> 8))
#define FF_PCI_BYTE_2(dl)      ((dl) & 0xFF)
#define FC_PCI_TYPE        3
#define FC_PCI_CTS         0
#define FC_PCI_WAIT        1
#define FC_PCI_OVFLW       2
#define FC_PCI_BYTE_1(FS)  ((FC_PCI_TYPE << PCI_TYPE_POS) | (FS))
#define FC_PCI_BYTE_2(BS)  (BS)
#define FC_PCI_BYTE_3(ST_MIN) (ST_MIN)
#define CF_PCI_TYPE        2
#define CF_PCI_BYTE_1      (CF_PCI_TYPE << PCI_TYPE_POS)
#define STMIN_VAL_1        5
#define STMIN_VAL_2        50
#define STMIN_UPPER_TOLERANCE 5

#define BS_TIMEOUT_UPPER_MS   1100
#define BS_TIMEOUT_LOWER_MS   1000

/*
 * @addtogroup t_can
 * @{
 * @defgroup t_can_isotp test_can_isotp
 * @brief TestPurpose: verify correctness of the iso tp implementation
 * @details
 * - Test Steps
 *   -#
 * - Expected Results
 *   -#
 * @}
 */

struct frame_desired {
	uint8_t data[CAN_MAX_DLEN];
	uint8_t length;
};

struct frame_desired des_frames[DIV_ROUND_UP((DATA_SEND_LENGTH - DATA_SIZE_FF),
					     DATA_SIZE_CF)];


const struct isotp_fc_opts fc_opts = {
	.bs = 8,
	.stmin = 0
};
const struct isotp_fc_opts fc_opts_single = {
	.bs = 0,
	.stmin = 0
};

const struct isotp_msg_id rx_addr = {
	.std_id = 0x10,
#ifdef CONFIG_TEST_USE_CAN_FD_MODE
	.dl = CONFIG_TEST_ISOTP_TX_DL,
	.flags = ISOTP_MSG_FDF | ISOTP_MSG_BRS,
#endif
};
const struct isotp_msg_id tx_addr = {
	.std_id = 0x11,
#ifdef CONFIG_TEST_USE_CAN_FD_MODE
	.dl = CONFIG_TEST_ISOTP_TX_DL,
	.flags = ISOTP_MSG_FDF | ISOTP_MSG_BRS,
#endif
};

const struct isotp_msg_id rx_addr_ext = {
	.std_id = 0x10,
	.ext_addr = EXT_ADDR,
#ifdef CONFIG_TEST_USE_CAN_FD_MODE
	.dl = CONFIG_TEST_ISOTP_TX_DL,
	.flags = ISOTP_MSG_EXT_ADDR | ISOTP_MSG_FDF | ISOTP_MSG_BRS,
#else
	.flags = ISOTP_MSG_EXT_ADDR,
#endif
};

const struct isotp_msg_id tx_addr_ext = {
	.std_id = 0x11,
	.ext_addr = EXT_ADDR,
#ifdef CONFIG_TEST_USE_CAN_FD_MODE
	.dl = CONFIG_TEST_ISOTP_TX_DL,
	.flags = ISOTP_MSG_EXT_ADDR | ISOTP_MSG_FDF | ISOTP_MSG_BRS,
#else
	.flags = ISOTP_MSG_EXT_ADDR,
#endif
};

const struct isotp_msg_id rx_addr_fixed = {
	.ext_id = 0x18DA0201,
#ifdef CONFIG_TEST_USE_CAN_FD_MODE
	.dl = CONFIG_TEST_ISOTP_TX_DL,
	.flags = ISOTP_MSG_FIXED_ADDR | ISOTP_MSG_IDE | ISOTP_MSG_FDF | ISOTP_MSG_BRS,
#else
	.flags = ISOTP_MSG_FIXED_ADDR | ISOTP_MSG_IDE,
#endif
};

const struct isotp_msg_id tx_addr_fixed = {
	.ext_id = 0x18DA0102,
#ifdef CONFIG_TEST_USE_CAN_FD_MODE
	.dl = CONFIG_TEST_ISOTP_TX_DL,
	.flags = ISOTP_MSG_FIXED_ADDR | ISOTP_MSG_IDE | ISOTP_MSG_FDF | ISOTP_MSG_BRS,
#else
	.flags = ISOTP_MSG_FIXED_ADDR | ISOTP_MSG_IDE,
#endif
};

static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
static struct isotp_recv_ctx recv_ctx;
static struct isotp_send_ctx send_ctx;
static uint8_t data_buf[128];
CAN_MSGQ_DEFINE(frame_msgq, 10);
static struct k_sem send_compl_sem;

void send_complete_cb(int error_nr, void *arg)
{
	int expected_err_nr = POINTER_TO_INT(arg);

	zassert_equal(error_nr, expected_err_nr,
		      "Unexpected error nr. expect: %d, got %d",
		      expected_err_nr, error_nr);
	k_sem_give(&send_compl_sem);
}

static void print_hex(const uint8_t *ptr, size_t len)
{
	while (len--) {
		printk("%02x ", *ptr++);
	}
}


static int check_data(const uint8_t *frame, const uint8_t *desired, size_t length)
{
	int ret;

	ret = memcmp(frame, desired, length);
	if (ret) {
		printk("desired bytes:\n");
		print_hex(desired, length);
		printk("\nreceived (%zu bytes):\n", length);
		print_hex(frame, length);
		printk("\n");
	}

	return ret;
}

static void send_sf(void)
{
	int ret;

	ret = isotp_send(&send_ctx, can_dev, random_data, DATA_SIZE_SF,
			 &rx_addr, &tx_addr, send_complete_cb, INT_TO_POINTER(ISOTP_N_OK));
	zassert_equal(ret, 0, "Send returned %d", ret);
}


static void get_sf(size_t data_size)
{
	int ret;

	memset(data_buf, 0, sizeof(data_buf));
	ret = isotp_recv(&recv_ctx, data_buf, sizeof(data_buf), K_MSEC(1000));
	zassert_equal(ret, data_size, "recv returned %d", ret);

	ret = check_data(data_buf, random_data, data_size);
	zassert_equal(ret, 0, "Data differ");
}

static void get_sf_ignore(void)
{
	int ret;

	ret = isotp_recv(&recv_ctx, data_buf, sizeof(data_buf), K_MSEC(200));
	zassert_equal(ret, ISOTP_RECV_TIMEOUT, "recv returned %d", ret);
}

static void send_test_data(const uint8_t *data, size_t len)
{
	int ret;

	ret = isotp_send(&send_ctx, can_dev, data, len, &rx_addr, &tx_addr,
			 send_complete_cb, INT_TO_POINTER(ISOTP_N_OK));
	zassert_equal(ret, 0, "Send returned %d", ret);
}

static void receive_test_data(const uint8_t *data, size_t len, int32_t delay)
{
	size_t remaining_len = len;
	int ret, recv_len;
	const uint8_t *data_ptr = data;

	do {
		memset(data_buf, 0, sizeof(data_buf));
		recv_len = isotp_recv(&recv_ctx, data_buf, sizeof(data_buf),
				      K_MSEC(1000));
		zassert_true(recv_len >= 0, "recv error: %d", recv_len);

		zassert_true(remaining_len >= recv_len,
			     "More data then expected");
		ret = check_data(data_buf, data_ptr, recv_len);
		zassert_equal(ret, 0, "Data differ");
		data_ptr += recv_len;
		remaining_len -= recv_len;

		if (delay) {
			k_msleep(delay);
		}
	} while (remaining_len);

	ret = isotp_recv(&recv_ctx, data_buf, sizeof(data_buf), K_MSEC(50));
	zassert_equal(ret, ISOTP_RECV_TIMEOUT,
		      "Expected timeout but got %d", ret);
}

static void send_frame_series(struct frame_desired *frames, size_t length,
			      uint32_t id)
{
	int i, ret;
	struct can_frame frame = {
		.flags = ((id > 0x7FF) ? CAN_FRAME_IDE : 0) |
			 (IS_ENABLED(CONFIG_TEST_USE_CAN_FD_MODE) ?
			  CAN_FRAME_FDF | CAN_FRAME_BRS : 0),
		.id = id
	};
	struct frame_desired *desired = frames;

	for (i = 0; i < length; i++) {
		frame.dlc = can_bytes_to_dlc(desired->length);
		memcpy(frame.data, desired->data, desired->length);
		ret = can_send(can_dev, &frame, K_MSEC(500), NULL, NULL);
		zassert_equal(ret, 0, "Sending msg %d failed.", i);
		desired++;
	}
}

static void check_frame_series(struct frame_desired *frames, size_t length,
			       struct k_msgq *msgq)
{
	int i, ret;
	struct can_frame frame;
	struct frame_desired *desired = frames;

	for (i = 0; i < length; i++) {
		ret = k_msgq_get(msgq, &frame, K_MSEC(500));
		zassert_equal(ret, 0, "Timeout waiting for msg nr %d. ret: %d",
			      i, ret);

		zassert_equal(frame.dlc, can_bytes_to_dlc(desired->length),
			      "DLC of frame nr %d differ. Desired: %d, Got: %d",
			      i, can_bytes_to_dlc(desired->length), frame.dlc);

		ret = check_data(frame.data, desired->data, desired->length);
		zassert_equal(ret, 0, "Data differ");

		desired++;
	}
	ret = k_msgq_get(msgq, &frame, K_MSEC(200));
	zassert_equal(ret, -EAGAIN, "Expected timeout, but received %d", ret);
}

static int add_rx_msgq(uint32_t id, uint32_t mask)
{
	int filter_id;
	struct can_filter filter = {
		.flags = (id > 0x7FF) ? CAN_FILTER_IDE : 0,
		.id = id,
		.mask = mask
	};

	filter_id = can_add_rx_filter_msgq(can_dev, &frame_msgq, &filter);
	zassert_not_equal(filter_id, -ENOSPC, "Filter full");
	zassert_true((filter_id >= 0), "Negative filter number [%d]",
		     filter_id);

	return filter_id;
}

static void prepare_fc_frame(struct frame_desired *frame, uint8_t st,
			     const struct isotp_fc_opts *opts, bool tx)
{
	frame->data[0] = FC_PCI_BYTE_1(st);
	frame->data[1] = FC_PCI_BYTE_2(opts->bs);
	frame->data[2] = FC_PCI_BYTE_3(opts->stmin);
	if ((IS_ENABLED(CONFIG_ISOTP_ENABLE_TX_PADDING) && tx) ||
	    (IS_ENABLED(CONFIG_ISOTP_REQUIRE_RX_PADDING) && !tx)) {
		memset(&frame->data[DATA_SIZE_FC], 0xCC, 8 - DATA_SIZE_FC);
		frame->length = 8;
	} else {
		frame->length = DATA_SIZE_FC;
	}
}

static void prepare_sf_frame(struct frame_desired *frame, const uint8_t *data)
{
	frame->data[0] = SF_PCI_BYTE_1;
#ifdef SF_PCI_BYTE_2
	frame->data[1] = SF_PCI_BYTE_2;
	memcpy(&frame->data[2], data, DATA_SIZE_SF);
	frame->length = DATA_SIZE_SF + 2;
#else
	memcpy(&frame->data[1], data, DATA_SIZE_SF);
	frame->length = DATA_SIZE_SF + 1;
#endif
}

static void prepare_sf_ext_frame(struct frame_desired *frame, const uint8_t *data)
{
	frame->data[0] = rx_addr_ext.ext_addr;
	frame->data[1] = SF_PCI_BYTE_2_EXT;
#ifdef SF_PCI_BYTE_3_EXT
	frame->data[2] = SF_PCI_BYTE_3_EXT;
	memcpy(&frame->data[3], data, DATA_SIZE_SF_EXT);
	frame->length = DATA_SIZE_SF_EXT + 3;
#else
	memcpy(&frame->data[2], data, DATA_SIZE_SF_EXT);
	frame->length = DATA_SIZE_SF_EXT + 2;
#endif
}

static void prepare_cf_frames(struct frame_desired *frames, size_t frames_cnt,
			      const uint8_t *data, size_t data_len, bool tx)
{
	int i;
	const uint8_t *data_ptr = data;
	size_t remaining_length = data_len;

	for (i = 0; i < frames_cnt && remaining_length; i++) {
		frames[i].data[0] = CF_PCI_BYTE_1 | ((i+1) & 0x0F);
		frames[i].length = TX_DL;
		memcpy(&frames[i].data[1], data_ptr, DATA_SIZE_CF);

		if (remaining_length < DATA_SIZE_CF) {
			if ((IS_ENABLED(CONFIG_ISOTP_ENABLE_TX_PADDING) && tx) ||
			    (IS_ENABLED(CONFIG_ISOTP_REQUIRE_RX_PADDING) && !tx)) {
				uint8_t padded_dlc = can_bytes_to_dlc(MAX(8, remaining_length + 1));
				uint8_t padded_len = can_dlc_to_bytes(padded_dlc);

				memset(&frames[i].data[remaining_length + 1], 0xCC,
				       padded_len - remaining_length - 1);
				frames[i].length = padded_len;
			} else {
				frames[i].length = remaining_length + 1;
			}
			remaining_length = 0;
		}

		remaining_length -= DATA_SIZE_CF;
		data_ptr += DATA_SIZE_CF;
	}
}

ZTEST(isotp_conformance, test_send_sf)
{
	int filter_id;
	struct frame_desired des_frame;

	prepare_sf_frame(&des_frame, random_data);

	filter_id = add_rx_msgq(rx_addr.std_id, CAN_STD_ID_MASK);
	zassert_true((filter_id >= 0), "Negative filter number [%d]",
		     filter_id);

	send_sf();

	check_frame_series(&des_frame, 1, &frame_msgq);

	can_remove_rx_filter(can_dev, filter_id);
}

ZTEST(isotp_conformance, test_receive_sf)
{
	int ret;
	struct frame_desired single_frame;

	prepare_sf_frame(&single_frame, random_data);

	ret = isotp_bind(&recv_ctx, can_dev, &rx_addr, &tx_addr,
			 &fc_opts_single, K_NO_WAIT);
	zassert_equal(ret, ISOTP_N_OK, "Binding failed [%d]", ret);

	send_frame_series(&single_frame, 1, rx_addr.std_id);

	get_sf(DATA_SIZE_SF);

	/* Frame size too big should be ignored/dropped */
	#ifdef SF_PCI_BYTE_2
	single_frame.data[1]++;
	#else
	single_frame.data[0] = SF_PCI_BYTE_LEN_8;
	#endif
	send_frame_series(&single_frame, 1, rx_addr.std_id);
	get_sf_ignore();

#ifdef CONFIG_ISOTP_REQUIRE_RX_PADDING
	single_frame.data[0] = SF_PCI_BYTE_1;
	single_frame.length = 7;
	send_frame_series(&single_frame, 1, rx_addr.std_id);

	get_sf_ignore();
#endif

	isotp_unbind(&recv_ctx);
}

ZTEST(isotp_conformance, test_send_sf_ext)
{
	int filter_id, ret;
	struct frame_desired des_frame;

	prepare_sf_ext_frame(&des_frame, random_data);

	filter_id = add_rx_msgq(rx_addr_ext.std_id, CAN_STD_ID_MASK);
	zassert_true((filter_id >= 0), "Negative filter number [%d]",
		     filter_id);

	ret = isotp_send(&send_ctx, can_dev, random_data, DATA_SIZE_SF_EXT,
			 &rx_addr_ext, &tx_addr_ext, send_complete_cb,
			 INT_TO_POINTER(ISOTP_N_OK));
	zassert_equal(ret, 0, "Send returned %d", ret);

	check_frame_series(&des_frame, 1, &frame_msgq);

	can_remove_rx_filter(can_dev, filter_id);
}

ZTEST(isotp_conformance, test_receive_sf_ext)
{
	int ret;
	struct frame_desired single_frame;

	prepare_sf_ext_frame(&single_frame, random_data);

	ret = isotp_bind(&recv_ctx, can_dev, &rx_addr_ext, &tx_addr,
			 &fc_opts_single, K_NO_WAIT);
	zassert_equal(ret, ISOTP_N_OK, "Binding failed [%d]", ret);

	send_frame_series(&single_frame, 1, rx_addr.std_id);

	get_sf(DATA_SIZE_SF_EXT);

	/* Frame size too big should be ignored/dropped */
	#ifdef SF_PCI_BYTE_2
	single_frame.data[2]++;
	#else
	single_frame.data[1] = SF_PCI_BYTE_1;
	#endif
	send_frame_series(&single_frame, 1, rx_addr.std_id);
	get_sf_ignore();

#ifdef CONFIG_ISOTP_REQUIRE_RX_PADDING
	single_frame.data[1] = SF_PCI_BYTE_2_EXT;
	single_frame.length = 7;
	send_frame_series(&single_frame, 1, rx_addr.std_id);

	get_sf_ignore();
#endif

	isotp_unbind(&recv_ctx);
}

ZTEST(isotp_conformance, test_send_sf_fixed)
{
	int filter_id, ret;
	struct frame_desired des_frame;

	prepare_sf_frame(&des_frame, random_data);

	/* mask to allow any priority and source address (SA) */
	filter_id = add_rx_msgq(rx_addr_fixed.ext_id, 0x03FFFF00);
	zassert_true((filter_id >= 0), "Negative filter number [%d]",
		     filter_id);

	ret = isotp_send(&send_ctx, can_dev, random_data, DATA_SIZE_SF,
			 &rx_addr_fixed, &tx_addr_fixed, send_complete_cb,
			 INT_TO_POINTER(ISOTP_N_OK));
	zassert_equal(ret, 0, "Send returned %d", ret);

	check_frame_series(&des_frame, 1, &frame_msgq);

	can_remove_rx_filter(can_dev, filter_id);
}

ZTEST(isotp_conformance, test_receive_sf_fixed)
{
	int ret;
	struct frame_desired single_frame;

	prepare_sf_frame(&single_frame, random_data);

	ret = isotp_bind(&recv_ctx, can_dev, &rx_addr_fixed, &tx_addr_fixed,
			 &fc_opts_single, K_NO_WAIT);
	zassert_equal(ret, ISOTP_N_OK, "Binding failed [%d]", ret);

	/* default source address */
	send_frame_series(&single_frame, 1, rx_addr_fixed.ext_id);
	get_sf(DATA_SIZE_SF);

	/* different source address */
	send_frame_series(&single_frame, 1, rx_addr_fixed.ext_id | 0xFF);
	get_sf(DATA_SIZE_SF);

	/* different priority */
	send_frame_series(&single_frame, 1, rx_addr_fixed.ext_id | (7U << 26));
	get_sf(DATA_SIZE_SF);

	/* different target address (should fail) */
	send_frame_series(&single_frame, 1, rx_addr_fixed.ext_id | 0xFF00);
	get_sf_ignore();

	isotp_unbind(&recv_ctx);
}

ZTEST(isotp_conformance, test_send_data)
{
	struct frame_desired fc_frame, ff_frame;
	const uint8_t *data_ptr = random_data;
	size_t remaining_length = DATA_SEND_LENGTH;
	int filter_id;

	ff_frame.data[0] = FF_PCI_BYTE_1(DATA_SEND_LENGTH);
	ff_frame.data[1] = FF_PCI_BYTE_2(DATA_SEND_LENGTH);
	memcpy(&ff_frame.data[2], data_ptr, DATA_SIZE_FF);
	ff_frame.length = TX_DL;
	data_ptr += DATA_SIZE_FF;
	remaining_length -= DATA_SIZE_FF;

	prepare_fc_frame(&fc_frame, FC_PCI_CTS, &fc_opts_single, false);

	prepare_cf_frames(des_frames, ARRAY_SIZE(des_frames), data_ptr,
			  remaining_length, true);

	filter_id = add_rx_msgq(rx_addr.std_id, CAN_STD_ID_MASK);
	zassert_true((filter_id >= 0), "Negative filter number [%d]",
		     filter_id);

	send_test_data(random_data, DATA_SEND_LENGTH);

	check_frame_series(&ff_frame, 1, &frame_msgq);

	send_frame_series(&fc_frame, 1, tx_addr.std_id);

	check_frame_series(des_frames, ARRAY_SIZE(des_frames), &frame_msgq);

	can_remove_rx_filter(can_dev, filter_id);
}

ZTEST(isotp_conformance, test_send_data_blocks)
{
	const uint8_t *data_ptr = random_data;
	size_t remaining_length = DATA_SEND_LENGTH;
	struct frame_desired *data_frame_ptr = des_frames;
	int filter_id, ret;
	struct can_frame dummy_frame;
	struct frame_desired fc_frame, ff_frame;

	ff_frame.data[0] = FF_PCI_BYTE_1(DATA_SEND_LENGTH);
	ff_frame.data[1] = FF_PCI_BYTE_2(DATA_SEND_LENGTH);
	memcpy(&ff_frame.data[2], data_ptr, DATA_SIZE_FF);
	ff_frame.length = DATA_SIZE_FF + 2;
	data_ptr += DATA_SIZE_FF;
	remaining_length -= DATA_SIZE_FF;

	prepare_fc_frame(&fc_frame, FC_PCI_CTS, &fc_opts, false);

	prepare_cf_frames(des_frames, ARRAY_SIZE(des_frames), data_ptr,
			  remaining_length, true);

	remaining_length = DATA_SEND_LENGTH;

	filter_id = add_rx_msgq(rx_addr.std_id, CAN_STD_ID_MASK);
	zassert_true((filter_id >= 0), "Negative filter number [%d]",
		     filter_id);

	send_test_data(random_data, DATA_SEND_LENGTH);

	check_frame_series(&ff_frame, 1, &frame_msgq);
	remaining_length -= DATA_SIZE_FF;

	send_frame_series(&fc_frame, 1, tx_addr.std_id);

	check_frame_series(data_frame_ptr, fc_opts.bs, &frame_msgq);
	data_frame_ptr += fc_opts.bs;
	remaining_length -= fc_opts.bs * DATA_SIZE_CF;
	ret = k_msgq_get(&frame_msgq, &dummy_frame, K_MSEC(50));
	zassert_equal(ret, -EAGAIN, "Expected timeout but got %d", ret);

	fc_frame.data[1] = FC_PCI_BYTE_2(2);
	send_frame_series(&fc_frame, 1, tx_addr.std_id);

	/* dynamic bs */
	check_frame_series(data_frame_ptr, 2, &frame_msgq);
	data_frame_ptr += 2;
	remaining_length -= 2  * DATA_SIZE_CF;
	ret = k_msgq_get(&frame_msgq, &dummy_frame, K_MSEC(50));
	zassert_equal(ret, -EAGAIN, "Expected timeout but got %d", ret);

	/* get the rest */
	fc_frame.data[1] = FC_PCI_BYTE_2(0);
	send_frame_series(&fc_frame, 1, tx_addr.std_id);

	check_frame_series(data_frame_ptr,
			   DIV_ROUND_UP(remaining_length, DATA_SIZE_CF),
			   &frame_msgq);
	ret = k_msgq_get(&frame_msgq, &dummy_frame, K_MSEC(50));
	zassert_equal(ret, -EAGAIN, "Expected timeout but got %d", ret);

	can_remove_rx_filter(can_dev, filter_id);
}

ZTEST(isotp_conformance, test_receive_data)
{
	const uint8_t *data_ptr = random_data;
	size_t remaining_length = DATA_SEND_LENGTH;
	int filter_id, ret;
	struct frame_desired fc_frame, ff_frame;

	ff_frame.data[0] = FF_PCI_BYTE_1(DATA_SEND_LENGTH);
	ff_frame.data[1] = FF_PCI_BYTE_2(DATA_SEND_LENGTH);
	memcpy(&ff_frame.data[2], data_ptr, DATA_SIZE_FF);
	ff_frame.length = TX_DL;
	data_ptr += DATA_SIZE_FF;
	remaining_length -= DATA_SIZE_FF;

	prepare_fc_frame(&fc_frame, FC_PCI_CTS, &fc_opts_single, true);

	prepare_cf_frames(des_frames, ARRAY_SIZE(des_frames), data_ptr,
			  remaining_length, false);

	filter_id = add_rx_msgq(tx_addr.std_id, CAN_STD_ID_MASK);

	ret = isotp_bind(&recv_ctx, can_dev, &rx_addr, &tx_addr,
			 &fc_opts_single, K_NO_WAIT);
	zassert_equal(ret, ISOTP_N_OK, "Binding failed [%d]", ret);

	send_frame_series(&ff_frame, 1, rx_addr.std_id);

	check_frame_series(&fc_frame, 1, &frame_msgq);

	send_frame_series(des_frames, ARRAY_SIZE(des_frames), rx_addr.std_id);

	receive_test_data(random_data, DATA_SEND_LENGTH, 0);

	can_remove_rx_filter(can_dev, filter_id);
	isotp_unbind(&recv_ctx);
}

ZTEST(isotp_conformance, test_receive_data_blocks)
{
	const uint8_t *data_ptr = random_data;
	size_t remaining_length = DATA_SEND_LENGTH;
	struct frame_desired *data_frame_ptr = des_frames;
	int filter_id, ret;
	size_t remaining_frames;
	struct frame_desired fc_frame, ff_frame;

	struct can_frame dummy_frame;

	ff_frame.data[0] = FF_PCI_BYTE_1(DATA_SEND_LENGTH);
	ff_frame.data[1] = FF_PCI_BYTE_2(DATA_SEND_LENGTH);
	memcpy(&ff_frame.data[2], data_ptr, DATA_SIZE_FF);
	ff_frame.length = DATA_SIZE_FF + 2;
	data_ptr += DATA_SIZE_FF;
	remaining_length -= DATA_SIZE_FF;

	prepare_fc_frame(&fc_frame, FC_PCI_CTS, &fc_opts, true);

	prepare_cf_frames(des_frames, ARRAY_SIZE(des_frames), data_ptr,
			  remaining_length, false);

	remaining_frames = DIV_ROUND_UP(remaining_length, DATA_SIZE_CF);

	filter_id = add_rx_msgq(tx_addr.std_id, CAN_STD_ID_MASK);
	zassert_true((filter_id >= 0), "Negative filter number [%d]",
		     filter_id);

	ret = isotp_bind(&recv_ctx, can_dev, &rx_addr, &tx_addr,
			 &fc_opts, K_NO_WAIT);
	zassert_equal(ret, ISOTP_N_OK, "Binding failed [%d]", ret);

	send_frame_series(&ff_frame, 1, rx_addr.std_id);

	while (remaining_frames) {
		check_frame_series(&fc_frame, 1, &frame_msgq);

		if (remaining_frames >= fc_opts.bs) {
			send_frame_series(data_frame_ptr, fc_opts.bs,
					  rx_addr.std_id);
			data_frame_ptr += fc_opts.bs;
			remaining_frames -= fc_opts.bs;
		} else {
			send_frame_series(data_frame_ptr, remaining_frames,
					  rx_addr.std_id);
			data_frame_ptr += remaining_frames;
			remaining_frames = 0;
		}
	}

	ret = k_msgq_get(&frame_msgq, &dummy_frame, K_MSEC(50));
	zassert_equal(ret, -EAGAIN, "Expected timeout but got %d", ret);

	receive_test_data(random_data, DATA_SEND_LENGTH, 0);

	can_remove_rx_filter(can_dev, filter_id);
	isotp_unbind(&recv_ctx);
}

ZTEST(isotp_conformance, test_send_timeouts)
{
	int ret;
	uint32_t start_time, time_diff;
	struct frame_desired fc_cts_frame;

	prepare_fc_frame(&fc_cts_frame, FC_PCI_CTS, &fc_opts, false);

	/* Test timeout for first FC*/
	start_time = k_uptime_get_32();
	ret = isotp_send(&send_ctx, can_dev, random_data, sizeof(random_data),
			 &tx_addr, &rx_addr, NULL, NULL);
	time_diff = k_uptime_get_32() - start_time;
	zassert_equal(ret, ISOTP_N_TIMEOUT_BS, "Expected timeout but got %d",
		      ret);
	zassert_true(time_diff <= BS_TIMEOUT_UPPER_MS,
		     "Timeout too late (%dms)",  time_diff);
	zassert_true(time_diff >= BS_TIMEOUT_LOWER_MS,
		     "Timeout too early (%dms)", time_diff);

	/* Test timeout for consecutive FC frames */
	k_sem_reset(&send_compl_sem);
	ret = isotp_send(&send_ctx, can_dev, random_data, sizeof(random_data),
			 &tx_addr, &rx_addr, send_complete_cb,
			 INT_TO_POINTER(ISOTP_N_TIMEOUT_BS));
	zassert_equal(ret, ISOTP_N_OK, "Send returned %d", ret);

	send_frame_series(&fc_cts_frame, 1, rx_addr.std_id);

	start_time = k_uptime_get_32();
	ret = k_sem_take(&send_compl_sem, K_MSEC(BS_TIMEOUT_UPPER_MS));
	zassert_equal(ret, 0, "Timeout too late");

	time_diff = k_uptime_get_32() - start_time;
	zassert_true(time_diff >= BS_TIMEOUT_LOWER_MS,
		     "Timeout too early (%dms)", time_diff);

	/* Test timeout reset with WAIT frame */
	k_sem_reset(&send_compl_sem);
	ret = isotp_send(&send_ctx, can_dev, random_data, sizeof(random_data),
			 &tx_addr, &rx_addr, send_complete_cb,
			 INT_TO_POINTER(ISOTP_N_TIMEOUT_BS));
	zassert_equal(ret, ISOTP_N_OK, "Send returned %d", ret);

	ret = k_sem_take(&send_compl_sem, K_MSEC(800));
	zassert_equal(ret, -EAGAIN, "Timeout too early");

	fc_cts_frame.data[0] = FC_PCI_BYTE_1(FC_PCI_CTS);
	send_frame_series(&fc_cts_frame, 1, rx_addr.std_id);

	start_time = k_uptime_get_32();
	ret = k_sem_take(&send_compl_sem, K_MSEC(BS_TIMEOUT_UPPER_MS));
	zassert_equal(ret, 0, "Timeout too late");
	time_diff = k_uptime_get_32() - start_time;
	zassert_true(time_diff >= BS_TIMEOUT_LOWER_MS,
		     "Timeout too early (%dms)", time_diff);
}

ZTEST(isotp_conformance, test_receive_timeouts)
{
	int ret;
	uint32_t start_time, time_diff;
	struct frame_desired ff_frame;

	ff_frame.data[0] = FF_PCI_BYTE_1(DATA_SEND_LENGTH);
	ff_frame.data[1] = FF_PCI_BYTE_2(DATA_SEND_LENGTH);
	memcpy(&ff_frame.data[2], random_data, DATA_SIZE_FF);
	ff_frame.length = DATA_SIZE_FF + 2;

	ret = isotp_bind(&recv_ctx, can_dev, &rx_addr, &tx_addr,
			 &fc_opts, K_NO_WAIT);
	zassert_equal(ret, ISOTP_N_OK, "Binding failed [%d]", ret);

	send_frame_series(&ff_frame, 1, rx_addr.std_id);
	start_time = k_uptime_get_32();

	ret = isotp_recv(&recv_ctx, data_buf, sizeof(data_buf), K_FOREVER);
	zassert_equal(ret, DATA_SIZE_FF,
		      "Expected FF data length but got %d", ret);
	ret = isotp_recv(&recv_ctx, data_buf, sizeof(data_buf), K_FOREVER);
	zassert_equal(ret, ISOTP_N_TIMEOUT_CR,
		      "Expected timeout but got %d", ret);

	time_diff = k_uptime_get_32() - start_time;
	zassert_true(time_diff >= BS_TIMEOUT_LOWER_MS,
		     "Timeout too early (%dms)", time_diff);
	zassert_true(time_diff <= BS_TIMEOUT_UPPER_MS,
		     "Timeout too slow (%dms)", time_diff);

	isotp_unbind(&recv_ctx);
}

ZTEST(isotp_conformance, test_stmin)
{
	int filter_id, ret;
	struct frame_desired fc_frame, ff_frame;
	struct can_frame raw_frame;
	uint32_t start_time, time_diff;
	struct isotp_fc_opts fc_opts_stmin = {
		.bs = 2, .stmin = STMIN_VAL_1
	};

	if (CONFIG_SYS_CLOCK_TICKS_PER_SEC < 1000) {
		/* This test requires millisecond tick resolution */
		ztest_test_skip();
	}

	ff_frame.data[0] = FF_PCI_BYTE_1(DATA_SIZE_FF + DATA_SIZE_CF * 4);
	ff_frame.data[1] = FF_PCI_BYTE_2(DATA_SIZE_FF + DATA_SIZE_CF * 4);
	memcpy(&ff_frame.data[2], random_data, DATA_SIZE_FF);
	ff_frame.length = DATA_SIZE_FF + 2;

	prepare_fc_frame(&fc_frame, FC_PCI_CTS, &fc_opts_stmin, false);

	filter_id = add_rx_msgq(rx_addr.std_id, CAN_STD_ID_MASK);
	zassert_true((filter_id >= 0), "Negative filter number [%d]",
		     filter_id);

	send_test_data(random_data, DATA_SIZE_FF + DATA_SIZE_CF * 4);

	check_frame_series(&ff_frame, 1, &frame_msgq);

	send_frame_series(&fc_frame, 1, tx_addr.std_id);

	ret = k_msgq_get(&frame_msgq, &raw_frame, K_MSEC(100));
	zassert_equal(ret, 0, "Expected to get a message. [%d]", ret);

	start_time = k_uptime_get_32();
	ret = k_msgq_get(&frame_msgq, &raw_frame,
			 K_MSEC(STMIN_VAL_1 + STMIN_UPPER_TOLERANCE));
	time_diff = k_uptime_get_32() - start_time;
	zassert_equal(ret, 0, "Expected to get a message within %dms. [%d]",
		      STMIN_VAL_1 + STMIN_UPPER_TOLERANCE, ret);
	zassert_true(time_diff >= STMIN_VAL_1, "STmin too short (%dms)",
		     time_diff);

	fc_frame.data[2] = FC_PCI_BYTE_3(STMIN_VAL_2);
	send_frame_series(&fc_frame, 1, tx_addr.std_id);

	ret = k_msgq_get(&frame_msgq, &raw_frame, K_MSEC(100));
	zassert_equal(ret, 0, "Expected to get a message. [%d]", ret);

	start_time = k_uptime_get_32();
	ret = k_msgq_get(&frame_msgq, &raw_frame,
			 K_MSEC(STMIN_VAL_2 + STMIN_UPPER_TOLERANCE));
	time_diff = k_uptime_get_32() - start_time;
	zassert_equal(ret, 0, "Expected to get a message within %dms. [%d]",
		      STMIN_VAL_2 + STMIN_UPPER_TOLERANCE, ret);
	zassert_true(time_diff >= STMIN_VAL_2, "STmin too short (%dms)",
		     time_diff);

	can_remove_rx_filter(can_dev, filter_id);
}

ZTEST(isotp_conformance, test_receiver_fc_errors)
{
	int ret, filter_id;
	struct frame_desired ff_frame, fc_frame;

	ff_frame.data[0] = FF_PCI_BYTE_1(DATA_SEND_LENGTH);
	ff_frame.data[1] = FF_PCI_BYTE_2(DATA_SEND_LENGTH);
	memcpy(&ff_frame.data[2], random_data, DATA_SIZE_FF);
	ff_frame.length = DATA_SIZE_FF + 2;

	prepare_fc_frame(&fc_frame, FC_PCI_CTS, &fc_opts, true);

	filter_id = add_rx_msgq(tx_addr.std_id, CAN_STD_ID_MASK);
	zassert_true((filter_id >= 0), "Negative filter number [%d]",
		     filter_id);

	ret = isotp_bind(&recv_ctx, can_dev, &rx_addr, &tx_addr,
			 &fc_opts, K_NO_WAIT);
	zassert_equal(ret, ISOTP_N_OK, "Binding failed [%d]", ret);

	/* wrong sequence number */
	send_frame_series(&ff_frame, 1, rx_addr.std_id);
	check_frame_series(&fc_frame, 1, &frame_msgq);
	prepare_cf_frames(des_frames, ARRAY_SIZE(des_frames),
			  random_data + DATA_SIZE_FF,
			  sizeof(random_data) - DATA_SIZE_FF, false);
	/* SN should be 2 but is set to 3 for this test */
	des_frames[1].data[0] = CF_PCI_BYTE_1 | (3 & 0x0F);
	send_frame_series(des_frames, fc_opts.bs, rx_addr.std_id);

	ret = isotp_recv(&recv_ctx, data_buf, sizeof(data_buf), K_MSEC(200));
	zassert_equal(ret, DATA_SIZE_FF,
		      "Expected FF data length but got %d", ret);
	ret = isotp_recv(&recv_ctx, data_buf, sizeof(data_buf), K_MSEC(200));
	zassert_equal(ret, ISOTP_N_WRONG_SN,
		      "Expected wrong SN but got %d", ret);

	/* buffer overflow */
	ff_frame.data[0] = FF_PCI_BYTE_1(0xFFF);
	ff_frame.data[1] = FF_PCI_BYTE_2(0xFFF);

	fc_frame.data[0] = FC_PCI_BYTE_1(FC_PCI_OVFLW);
	fc_frame.data[1] = FC_PCI_BYTE_2(0);
	fc_frame.data[2] = FC_PCI_BYTE_3(0);

	isotp_unbind(&recv_ctx);
	ret = isotp_bind(&recv_ctx, can_dev, &rx_addr, &tx_addr,
			 &fc_opts_single, K_NO_WAIT);
	zassert_equal(ret, ISOTP_N_OK, "Binding failed [%d]", ret);

	send_frame_series(&ff_frame, 1, rx_addr.std_id);
	check_frame_series(&fc_frame, 1, &frame_msgq);

	can_remove_rx_filter(can_dev, filter_id);
	k_msgq_cleanup(&frame_msgq);
	isotp_unbind(&recv_ctx);
}

ZTEST(isotp_conformance, test_sender_fc_errors)
{
	int ret, filter_id, i;
	struct frame_desired ff_frame, fc_frame;

	ff_frame.data[0] = FF_PCI_BYTE_1(DATA_SEND_LENGTH);
	ff_frame.data[1] = FF_PCI_BYTE_2(DATA_SEND_LENGTH);
	memcpy(&ff_frame.data[2], random_data, DATA_SIZE_FF);
	ff_frame.length = DATA_SIZE_FF + 2;

	filter_id = add_rx_msgq(tx_addr.std_id, CAN_STD_ID_MASK);

	/* invalid flow status */
	prepare_fc_frame(&fc_frame, 3, &fc_opts, false);

	k_sem_reset(&send_compl_sem);
	ret = isotp_send(&send_ctx, can_dev, random_data, DATA_SEND_LENGTH,
			 &tx_addr, &rx_addr, send_complete_cb,
			 INT_TO_POINTER(ISOTP_N_INVALID_FS));
	zassert_equal(ret, ISOTP_N_OK, "Send returned %d", ret);

	check_frame_series(&ff_frame, 1, &frame_msgq);
	send_frame_series(&fc_frame, 1, rx_addr.std_id);
	ret = k_sem_take(&send_compl_sem, K_MSEC(200));
	zassert_equal(ret, 0, "Send complete callback not called");

	/* buffer overflow */
	k_sem_reset(&send_compl_sem);
	ret = isotp_send(&send_ctx, can_dev, random_data, DATA_SEND_LENGTH,
			 &tx_addr, &rx_addr, send_complete_cb,
			 INT_TO_POINTER(ISOTP_N_BUFFER_OVERFLW));

	check_frame_series(&ff_frame, 1, &frame_msgq);
	fc_frame.data[0] = FC_PCI_BYTE_1(FC_PCI_OVFLW);
	send_frame_series(&fc_frame, 1, rx_addr.std_id);
	ret = k_sem_take(&send_compl_sem, K_MSEC(200));
	zassert_equal(ret, 0, "Send complete callback not called");

	/* wft overrun */
	k_sem_reset(&send_compl_sem);
	ret = isotp_send(&send_ctx, can_dev, random_data, DATA_SEND_LENGTH,
			 &tx_addr, &rx_addr, send_complete_cb,
			 INT_TO_POINTER(ISOTP_N_WFT_OVRN));

	check_frame_series(&ff_frame, 1, &frame_msgq);
	fc_frame.data[0] = FC_PCI_BYTE_1(FC_PCI_WAIT);
	for (i = 0; i < CONFIG_ISOTP_WFTMAX + 1; i++) {
		send_frame_series(&fc_frame, 1, rx_addr.std_id);
	}

	ret = k_sem_take(&send_compl_sem, K_MSEC(200));
	zassert_equal(ret, 0, "Send complete callback not called");
	k_msgq_cleanup(&frame_msgq);
	can_remove_rx_filter(can_dev, filter_id);
}

ZTEST(isotp_conformance, test_canfd_mandatory_padding)
{
	/* Mandatory padding of CAN FD frames (TX_DL > 8).
	 * Must be padded with 0xCC up to the nearest DLC.
	 */
#if TX_DL < 12
	ztest_test_skip();
#else
	/* Input a single frame packet of 10 bytes */
	uint8_t data_size_sf = 10 - 2;
	int filter_id, ret;
	struct can_frame frame = {};
	const uint8_t expected_padding[] = { 0xCC, 0xCC };

	filter_id = add_rx_msgq(rx_addr.std_id, CAN_STD_ID_MASK);

	ret = isotp_send(&send_ctx, can_dev, random_data, data_size_sf,
			 &rx_addr, &tx_addr, send_complete_cb, INT_TO_POINTER(ISOTP_N_OK));
	zassert_equal(ret, 0, "Send returned %d", ret);

	ret = k_msgq_get(&frame_msgq, &frame, K_MSEC(500));
	zassert_equal(ret, 0, "Timeout waiting for msg. ret: %d", ret);

	/* The output frame should be 12 bytes, with the last two bytes being 0xCC */
	zassert_equal(can_dlc_to_bytes(frame.dlc), 12, "Incorrect DLC");
	zassert_mem_equal(&frame.data[10], expected_padding, sizeof(expected_padding));

	can_remove_rx_filter(can_dev, filter_id);
#endif
}

ZTEST(isotp_conformance, test_canfd_rx_dl_validation)
{
	/* First frame defines the RX data length, consecutive frames
	 * must have the same length (except the last frame)
	 */
#if TX_DL < 16
	ztest_test_skip();
#else

	uint8_t data_size_ff = 16 - 2;
	uint8_t data_size_cf = 12 - 1;
	uint8_t data_send_length = data_size_ff + 2 * data_size_cf;
	const uint8_t *data_ptr = random_data;
	int filter_id, ret;
	struct frame_desired fc_frame, ff_frame;

	/* FF uses a TX_DL of 16 */
	ff_frame.data[0] = FF_PCI_BYTE_1(data_send_length);
	ff_frame.data[1] = FF_PCI_BYTE_2(data_send_length);
	memcpy(&ff_frame.data[2], data_ptr, data_size_ff);
	ff_frame.length = data_size_ff + 2;
	data_ptr += data_size_ff;

	prepare_fc_frame(&fc_frame, FC_PCI_CTS, &fc_opts_single, true);

	/* Two CF frames using a TX_DL of 12 */
	des_frames[0].data[0] = CF_PCI_BYTE_1 | (1 & 0x0F);
	des_frames[0].length = data_size_cf + 1;
	memcpy(&des_frames[0].data[1], data_ptr, data_size_cf);
	data_ptr += data_size_cf;

	des_frames[1].data[0] = CF_PCI_BYTE_1 | (2 & 0x0F);
	des_frames[1].length = data_size_cf + 1;
	memcpy(&des_frames[1].data[1], data_ptr, data_size_cf);
	data_ptr += data_size_cf;

	filter_id = add_rx_msgq(tx_addr.std_id, CAN_STD_ID_MASK);

	ret = isotp_bind(&recv_ctx, can_dev, &rx_addr, &tx_addr,
			 &fc_opts_single, K_NO_WAIT);
	zassert_equal(ret, ISOTP_N_OK, "Binding failed [%d]", ret);

	send_frame_series(&ff_frame, 1, rx_addr.std_id);

	check_frame_series(&fc_frame, 1, &frame_msgq);

	send_frame_series(des_frames, 2, rx_addr.std_id);

	/* Assert that the packet was dropped and an error returned */
	ret = isotp_recv(&recv_ctx, data_buf, sizeof(data_buf), K_MSEC(200));
	zassert_equal(ret, ISOTP_N_ERROR, "recv returned %d", ret);

	can_remove_rx_filter(can_dev, filter_id);
	isotp_unbind(&recv_ctx);
#endif
}

static bool canfd_predicate(const void *state)
{
	ARG_UNUSED(state);

#ifdef CONFIG_TEST_USE_CAN_FD_MODE
	can_mode_t cap;
	int err;

	err = can_get_capabilities(can_dev, &cap);
	zassert_equal(err, 0, "failed to get CAN controller capabilities (err %d)", err);

	if ((cap & CAN_MODE_FD) == 0) {
		return false;
	}
#endif

	return true;
}

static void *isotp_conformance_setup(void)
{
	int ret;

	zassert_true(sizeof(random_data) >= sizeof(data_buf) * 2 + 10,
		     "Test data size to small");

	zassert_true(device_is_ready(can_dev), "CAN device not ready");

	(void)can_stop(can_dev);

	ret = can_set_mode(can_dev, CAN_MODE_LOOPBACK |
			   (IS_ENABLED(CONFIG_TEST_USE_CAN_FD_MODE) ? CAN_MODE_FD : 0));
	zassert_equal(ret, 0, "Failed to set mode [%d]", ret);

	ret = can_start(can_dev);
	zassert_equal(ret, 0, "Failed to start CAN controller [%d]", ret);

	k_sem_init(&send_compl_sem, 0, 1);

	return NULL;
}

ZTEST_SUITE(isotp_conformance, canfd_predicate, isotp_conformance_setup, NULL, NULL, NULL);
