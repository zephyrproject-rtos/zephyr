/*
 * SPDX-FileCopyrightText: Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/linker/devicetree_regions.h>

#define SAMPLE_I2S_TXRX_NODE DT_ALIAS(sample_i2s_txrx)
#define SAMPLE_I2S_RX_NODE DT_ALIAS(sample_i2s_rx)
#define SAMPLE_I2S_TX_NODE DT_ALIAS(sample_i2s_tx)

#define SAMPLE_TRANSCEIVE DT_NODE_EXISTS(SAMPLE_I2S_TXRX_NODE)

#define SAMPLE_BUFFER_COUNT CONFIG_SAMPLE_BUFFER_COUNT
#define SAMPLE_SQE_COUNT SAMPLE_BUFFER_COUNT
#define SAMPLE_CQE_COUNT SAMPLE_BUFFER_COUNT

#if CONFIG_SAMPLE_WORD_SIZE_8
#define SAMPLE_WORD_SIZE 8
#define SAMPLE_WORD_BYTE_SIZE 1
#define SAMPLE_WORD_TYPE uint8_t
#elif CONFIG_SAMPLE_WORD_SIZE_16
#define SAMPLE_WORD_SIZE 16
#define SAMPLE_WORD_BYTE_SIZE 2
#define SAMPLE_WORD_TYPE uint16_t
#endif

#define SAMPLE_CHANNEL_COUNT 2
#define SAMPLE_FRAME_SIZE (SAMPLE_WORD_BYTE_SIZE * SAMPLE_CHANNEL_COUNT)
#define SAMPLE_FRAME_CLK_FREQ CONFIG_SAMPLE_FRAME_CLK_FREQ
#define SAMPLE_FRAME_COUNT CONFIG_SAMPLE_FRAME_COUNT
#define SAMPLE_STREAM_COUNT CONFIG_SAMPLE_STREAM_COUNT
#define SAMPLE_STREAM_BUFFER_COUNT CONFIG_SAMPLE_STREAM_BUFFER_COUNT
#define SAMPLE_SIGNAL_FREQ CONFIG_SAMPLE_SIGNAL_FREQ
#define SAMPLE_SIGNAL_ATTENUATION CONFIG_SAMPLE_SIGNAL_ATTENUATION
#define SAMPLE_STREAM_DELAY CONFIG_SAMPLE_STREAM_DELAY
#define SAMPLE_SIGNAL_AMPLITUDE_LSB (GENMASK(SAMPLE_WORD_SIZE - 1, 0) / SAMPLE_SIGNAL_ATTENUATION)

#define SAMPLE_SAMPLE_DIFF \
	(SAMPLE_SIGNAL_AMPLITUDE_LSB / (SAMPLE_FRAME_CLK_FREQ / SAMPLE_SIGNAL_FREQ))

#define SAMPLE_TIMEOUT_MS 1000
#define SAMPLE_TIMEOUT K_MSEC(SAMPLE_TIMEOUT_MS)

#define SAMPLE_BUFFER_SIZE (SAMPLE_FRAME_SIZE * SAMPLE_FRAME_COUNT)

/* Apply memory region and attributes from devicetree if defined */
#define SAMPLE_MEMORY_ATTR(node_id)								\
	IF_ENABLED(										\
		DT_NODE_HAS_PROP(node_id, memory_regions),					\
		(										\
			LINKER_DT_NODE_REGION_NAME(DT_PHANDLE(node_id, memory_regions))		\
		)										\
	)

/* If there are separate RX and TX I2S devices, assert both are present and unique */
#if SAMPLE_TRANSCEIVE == 0
BUILD_ASSERT(DT_NODE_EXISTS(SAMPLE_I2S_RX_NODE));
BUILD_ASSERT(DT_NODE_EXISTS(SAMPLE_I2S_TX_NODE));
BUILD_ASSERT(DT_SAME_NODE(SAMPLE_I2S_RX_NODE, SAMPLE_I2S_TX_NODE) == 0);
#endif

BUILD_ASSERT(
	SAMPLE_STREAM_BUFFER_COUNT <= SAMPLE_BUFFER_COUNT,
	"stream buffer count exceeds buffer count"
);

/*
 * The user defines an RTIO context for which actions like writes, reads, and write/reads will be
 * submitted, and the results of said actions will be retrieved. The RTIO context is device
 * agnostic.
 *
 * We will be using 1 submission queue event (SQE) and 1 completion queue event (CQE) for each
 * buffer, or set of buffers in case I2S_DIR is BOTH (write/read).
 *
 * An SQE specifies an entire I2S operation, including buffers and I2S interface configuration
 * options. The RTIO_OP of the SQE specifies which buffers are provided, it can be mapped loosely
 * to the I2S API as follows:
 *
 *   RTIO_OP_RX: i2s_configure() + i2s_buf_read() + i2s_trigger(START)
 *   RTIO_OP_TX: i2s_configure() + i2s_buf_write() + i2s_trigger(START)
 *   RTIO_OP_TXRX: i2s_configure() + i2s_buf_write() + i2s_buf_read() + i2s_trigger(START)
 *
 * The I2S interface configuration options applied with i2s_configure() is part of the RTIO iodev
 * (described later) associated with the SQE.
 *
 * We will define one RTIO context for each I2S device.
 *
 * An extra SQE is allocated to sample_tx_rtio if defined to store a delay submission (explained
 * later).
 */
#if SAMPLE_TRANSCEIVE
RTIO_DEFINE(sample_txrx_rtio, SAMPLE_SQE_COUNT, SAMPLE_CQE_COUNT);
#else
RTIO_DEFINE(sample_rx_rtio, SAMPLE_SQE_COUNT, SAMPLE_CQE_COUNT);
RTIO_DEFINE(sample_tx_rtio, SAMPLE_SQE_COUNT + 1, SAMPLE_CQE_COUNT);
#endif

/*
 * We define an I2S iodev which binds the I2S device and I2S interface configuration options
 * to a device agnostic RTIO context. When we submit an SQE which uses this iodev to an RTIO
 * context, the I2S device and I2S interface configuration options defined below will be used
 * for the RX, TX or TXRX operation.
 *
 * If we are transceiving, we only need to define one device which acts as controller, otherwise
 * we need to define one iodev for the controller, which will transmit only, and one iodev for the
 * target which will receive only.
 */
#if SAMPLE_TRANSCEIVE
I2S_DT_IODEV_DEFINE(
	sample_txrx_iodev,
	SAMPLE_I2S_TXRX_NODE,
	SAMPLE_WORD_SIZE,
	SAMPLE_CHANNEL_COUNT,
	I2S_FMT_DATA_FORMAT_I2S,
	I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_CONTROLLER,
	SAMPLE_FRAME_CLK_FREQ,
	SAMPLE_TIMEOUT_MS
);
#else
I2S_DT_IODEV_DEFINE(
	sample_rx_iodev,
	SAMPLE_I2S_RX_NODE,
	SAMPLE_WORD_SIZE,
	SAMPLE_CHANNEL_COUNT,
	I2S_FMT_DATA_FORMAT_I2S,
	I2S_OPT_FRAME_CLK_TARGET | I2S_OPT_BIT_CLK_TARGET,
	SAMPLE_FRAME_CLK_FREQ,
	SAMPLE_TIMEOUT_MS
);
I2S_DT_IODEV_DEFINE(
	sample_tx_iodev,
	SAMPLE_I2S_TX_NODE,
	SAMPLE_WORD_SIZE,
	SAMPLE_CHANNEL_COUNT,
	I2S_FMT_DATA_FORMAT_I2S,
	I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_CONTROLLER,
	SAMPLE_FRAME_CLK_FREQ,
	SAMPLE_TIMEOUT_MS
);
#endif

#if SAMPLE_TRANSCEIVE
SAMPLE_MEMORY_ATTR(SAMPLE_I2S_TXRX_NODE) static uint8_t
	sample_tx_buffers[SAMPLE_BUFFER_COUNT][SAMPLE_BUFFER_SIZE];

SAMPLE_MEMORY_ATTR(SAMPLE_I2S_TXRX_NODE) static uint8_t
	sample_rx_buffers[SAMPLE_BUFFER_COUNT][SAMPLE_BUFFER_SIZE];
#else
SAMPLE_MEMORY_ATTR(SAMPLE_I2S_RX_NODE) static uint8_t
	sample_tx_buffers[SAMPLE_BUFFER_COUNT][SAMPLE_BUFFER_SIZE];

SAMPLE_MEMORY_ATTR(SAMPLE_I2S_TX_NODE) static uint8_t
	sample_rx_buffers[SAMPLE_BUFFER_COUNT][SAMPLE_BUFFER_SIZE];
#endif

/*
 * We will generate two (left and right) sawtooth signals (y-axis is amplitude, x-axis is period
 * defined by 1/SAMPLE_SIGNAL_FREQ):
 *
 *         Left channel
 *      y
 *
 *    1 |    /|    /|
 *      |   / |   / |
 *    0 +--------------- x
 *      | /   | /   | /
 *   -1 |/    |/    |/
 *      0     1     2
 *
 *         Right channel
 *      y
 *
 *    1 | /|    /|    /|
 *      |/ |   / |   / |
 *    0 +--------------- x
 *      |  | /   | /   |
 *   -1 |  |/    |/    |
 *      0     1     2
 *
 * synchronized to the submissions. Both have the same frequency and amplitude, the right channel
 * has a phase shift of 180 degrees to differentiate it from the left channel.
 *
 * The sawtooth signal is simply a constant addition (of SAMPLE_SAMPLE_DIFF) to the previous
 * sample, overflowing every "tooth". We use the sequence or consumption index to generate the
 * first frame of the buffer, then we simply add SAMPLE_SAMPLE_DIFF to every following sample.
 */
static void sample_generate_first_frame(size_t submit_index, SAMPLE_WORD_TYPE *buffer)
{
	buffer[0] = (SAMPLE_WORD_TYPE)(submit_index * SAMPLE_FRAME_COUNT * SAMPLE_SAMPLE_DIFF);
	buffer += 1;
	buffer[0] = buffer[-1];
	buffer[0] += (SAMPLE_WORD_TYPE)((SAMPLE_FRAME_COUNT / 2) * SAMPLE_SAMPLE_DIFF);
	buffer += 1;
}

static void sample_generate_following_frame(SAMPLE_WORD_TYPE *buffer_it)
{
	buffer_it[0] = buffer_it[-2] + SAMPLE_SAMPLE_DIFF;
	buffer_it += 1;
	buffer_it[0] = buffer_it[-2] + SAMPLE_SAMPLE_DIFF;
}

static void sample_generate_signal(size_t submit_index)
{
	size_t buffer_index;
	SAMPLE_WORD_TYPE *tx_buffer_it;

	buffer_index = submit_index % SAMPLE_BUFFER_COUNT;
	tx_buffer_it = (SAMPLE_WORD_TYPE *)sample_tx_buffers[buffer_index];

	sample_generate_first_frame(submit_index, tx_buffer_it);
	tx_buffer_it += SAMPLE_CHANNEL_COUNT;

	for (size_t i = 1; i < SAMPLE_FRAME_COUNT; i++) {
		sample_generate_following_frame(tx_buffer_it);
		tx_buffer_it += SAMPLE_CHANNEL_COUNT;
	}
}

static void sample_delay_frame(SAMPLE_WORD_TYPE *buffer_it)
{
	buffer_it[0] -= (SAMPLE_WORD_TYPE)(SAMPLE_STREAM_DELAY * SAMPLE_SAMPLE_DIFF);
	buffer_it += 1;
	buffer_it[0] -= (SAMPLE_WORD_TYPE)(SAMPLE_STREAM_DELAY * SAMPLE_SAMPLE_DIFF);
}

static int sample_validate_following_frame(SAMPLE_WORD_TYPE *buffer_it)
{
	if (buffer_it[0] != (SAMPLE_WORD_TYPE)(buffer_it[-2] + SAMPLE_SAMPLE_DIFF)) {
		return -EINVAL;
	}

	if (buffer_it[1] != (SAMPLE_WORD_TYPE)(buffer_it[-1] + SAMPLE_SAMPLE_DIFF)) {
		return -EINVAL;
	}

	return 0;
}

static int sample_validate_signal(size_t consume_index)
{
	size_t buffer_index;
	SAMPLE_WORD_TYPE *rx_buffer_it;
	SAMPLE_WORD_TYPE *rx_buffer_end;
	size_t frame_index;
	SAMPLE_WORD_TYPE first_frame[SAMPLE_CHANNEL_COUNT];

	buffer_index = consume_index % SAMPLE_BUFFER_COUNT;
	rx_buffer_it = (SAMPLE_WORD_TYPE *)sample_rx_buffers[buffer_index];
	rx_buffer_end = rx_buffer_it + SAMPLE_FRAME_COUNT * SAMPLE_CHANNEL_COUNT;
	frame_index = consume_index * SAMPLE_FRAME_COUNT;

	sample_generate_first_frame(consume_index, first_frame);

	if (frame_index < SAMPLE_STREAM_DELAY) {
		while (rx_buffer_it < rx_buffer_end && frame_index < SAMPLE_STREAM_DELAY) {
			rx_buffer_it += SAMPLE_CHANNEL_COUNT;
			frame_index++;
		}

		if (rx_buffer_it == rx_buffer_end) {
			return 0;
		}
	} else {
		sample_delay_frame(first_frame);
	}

	if (memcmp(rx_buffer_it, first_frame, SAMPLE_FRAME_SIZE)) {
		printk("rx buffer %u is invalid\n", consume_index);
		return -EINVAL;
	}

	rx_buffer_it += SAMPLE_CHANNEL_COUNT;

	while (rx_buffer_it < rx_buffer_end) {
		if (sample_validate_following_frame(rx_buffer_it)) {
			printk("rx buffer %u is invalid\n", consume_index);
			return -EINVAL;
		}

		rx_buffer_it += SAMPLE_CHANNEL_COUNT;
	}

	return 0;
}

#if SAMPLE_TRANSCEIVE == 0
static int sample_submit_controller_delay(void)
{
	struct rtio_sqe *sqe;

	sqe = rtio_sqe_acquire(&sample_tx_rtio);
	if (sqe == NULL) {
		return -ENOMEM;
	}

	rtio_sqe_prep_delay(sqe, SAMPLE_TIMEOUT, NULL);
	rtio_submit(&sample_tx_rtio, 0);
	return 0;
}
#endif

static int sample_prep_submission(size_t *submit_index)
{
	size_t buffer_index;
	uint8_t *tx_buffer;
	uint8_t *rx_buffer;
	struct rtio_sqe *sqe;

	/* Fill the buffer with the generated sawtooth signal */
	sample_generate_signal(*submit_index);

	buffer_index = *submit_index % SAMPLE_BUFFER_COUNT;
	tx_buffer = sample_tx_buffers[buffer_index];
	rx_buffer = sample_rx_buffers[buffer_index];

#if SAMPLE_TRANSCEIVE
	sqe = rtio_sqe_acquire(&sample_txrx_rtio);
	if (sqe == NULL) {
		return -ENOMEM;
	}

	rtio_sqe_prep_transceive(sqe,
				 &sample_txrx_iodev,
				 RTIO_PRIO_NORM,
				 tx_buffer,
				 rx_buffer,
				 SAMPLE_BUFFER_SIZE,
				 NULL);

	rtio_submit(&sample_txrx_rtio, 0);
#else
	sqe = rtio_sqe_acquire(&sample_rx_rtio);
	if (sqe == NULL) {
		return -ENOMEM;
	}

	rtio_sqe_prep_read(sqe,
			   &sample_rx_iodev,
			   RTIO_PRIO_NORM,
			   rx_buffer,
			   SAMPLE_BUFFER_SIZE,
			   NULL);

	sqe = rtio_sqe_acquire(&sample_tx_rtio);
	if (sqe == NULL) {
		return -ENOMEM;
	}

	rtio_sqe_prep_read(sqe,
			   &sample_tx_iodev,
			   RTIO_PRIO_NORM,
			   tx_buffer,
			   SAMPLE_BUFFER_SIZE,
			   NULL);
#endif

	*submit_index = *submit_index + 1;
	return 0;
}

static void sample_submit_submissions(void)
{
#if SAMPLE_TRANSCEIVE
	rtio_submit(&sample_txrx_rtio, 0);
#else
	rtio_submit(&sample_rx_rtio, 0);
	rtio_submit(&sample_tx_rtio, 0);
#endif
}

static int sample_prep_and_submit_submission(size_t *submit_index)
{
	int ret;

	ret = sample_prep_submission(submit_index);
	if (ret) {
		return ret;
	}

	sample_submit_submissions();
	return 0;
}

static int sample_consume(size_t *consume_index)
{
	struct rtio_cqe *cqe;
	int ret;

#if SAMPLE_TRANSCEIVE
	/* A CQE will be generated by the I2S device once a submission is complete */
	cqe = rtio_cqe_consume_block(&sample_txrx_rtio);
	ret = cqe->result;
	rtio_cqe_release(&sample_txrx_rtio, cqe);
#else
	/*
	 * A CQE will be generated by the TX and RX I2S devices respectively. The order is not
	 * guaranteed, and does not matter. RTIO queues both SQEs and CQEs, and both devices will
	 * complete their end of the transaction at nearly the same time since they are synced and
	 * have the same size buffers. We will wait for the TX device CQE first, then the RX CQE in
	 * this case.
	 */
	cqe = rtio_cqe_consume_block(&sample_tx_rtio);
	ret = cqe->result;
	rtio_cqe_release(&sample_tx_rtio, cqe);
	if (ret) {
		return ret;
	}

	cqe = rtio_cqe_consume_block(&sample_rx_rtio);
	ret = cqe->result;
	rtio_cqe_release(&sample_rx_rtio, cqe);
	if (ret) {
		return ret;
	}
#endif

	ret = sample_validate_signal(*consume_index);
	if (ret) {
		return ret;
	}

	*consume_index = *consume_index + 1;
	return 0;
}

int main(void)
{
	size_t submit_index;
	size_t consume_index;
	int ret;

#if SAMPLE_TRANSCEIVE
	/* Ensure I2S device is ready */
	if (!i2s_is_ready_iodev(&sample_txrx_iodev)) {
		printk("%s iodev is not ready\n", "txrx");
		return 0;
	}
#else
	/* Ensure I2S devices are ready */
	if (!i2s_is_ready_iodev(&sample_rx_iodev)) {
		printk("%s iodev is not ready\n", "rx");
		return 0;
	}

	if (!i2s_is_ready_iodev(&sample_tx_iodev)) {
		printk("%s iodev is not ready\n", "tx");
		return 0;
	}
#endif

	/*
	 * We will stream data in both directions until we reach SAMPLE_STREAM_COUNT. We track
	 * the stream progression using these two indexes.
	 */
	submit_index = 0;
	consume_index = 0;

#if SAMPLE_TRANSCEIVE == 0
	/*
	 * To ensure the I2S target device is ready once the I2S controller device starts the
	 * clock, which happens once it handles the first transmit SQE, we submit a delay SQE to
	 * the I2S controller device as the first submission.
	 *
	 * RTIO is asynchronous, we can not know exactly when an iodev starts handling submissions.
	 * We do know that submissions will be handled "as soon as possible", and we know in which
	 * order they will be handled.
	 */
	ret = sample_submit_controller_delay();
	if (ret) {
		printk("failed to submit controller delay (%d)\n", ret);
		return 0;
	}

	printk("submitted controller delay (%ums)\n", SAMPLE_TIMEOUT_MS);
#endif

	/*
	 * The I2S stream is buffered/preloaded. We prepare the first submissions and submit them
	 * all at once before we enter the "streaming" stage where we submit new buffers to the
	 * RTIO queue as they become ready, and read the received buffers as the RTIO submissions
	 * are completed.
	 */
	printk("preparing %u initial submissions\n", SAMPLE_STREAM_BUFFER_COUNT);
	for (size_t i = 0; i < SAMPLE_STREAM_BUFFER_COUNT; i++) {
		printk("%s %u\n", "preparing submission ", submit_index);
		ret = sample_prep_submission(&submit_index);
		if (ret) {
			printk("failed to %s %u (%d)\n", "prepare", submit_index, ret);
			return 0;
		}
	}

	/* Submit all the prepared submissions at once, starting the buffered/preloaded stream */
	printk("submitting %u initial submissions\n", SAMPLE_STREAM_BUFFER_COUNT);
	sample_submit_submissions();

	/*
	 * The I2S stream is now queued up. The stream will start once the I2S controller device
	 * receives the submitted SQEs asyncronously. Once the SQE has completed, a CQE will be
	 * generated for it. Since we are using a single RTIO instance for each I2S device, and all
	 * SQEs are given the same priority, the SQEs will be handled, and the CQEs generated, in
	 * the order they where submitted, for each I2S device respectively.
	 */

	/* We will consume every completion until we reach SAMPLE_STREAM_COUNT */
	while (consume_index < SAMPLE_STREAM_COUNT) {
		printk("%s %u\n", "consuming", consume_index);
		ret = sample_consume(&consume_index);
		if (ret) {
			printk("failed to %s %u (%d)\n", "consume", consume_index, ret);
			return 0;
		}

		/*
		 * We will submit a new submission every time a completion is consumed, until we
		 * reach SAMPLE_STREAM_COUNT submissions.
		 */
		if (submit_index < SAMPLE_STREAM_COUNT) {
			printk("%s %u\n", "submitting", submit_index);
			ret = sample_prep_and_submit_submission(&submit_index);
			if (ret) {
				printk("failed to %s %u (%d)\n", "submit", submit_index, ret);
				return 0;
			}
		}
	}

	printk("sample complete\n");
	return 0;
}
