/*
 * SPDX-FileCopyrightText: Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/rtio/rtio.h>

#include <string.h>

#define SAMPLE_SPI_CONTROLLER_NODE DT_NODELABEL(spi_controller)
#define SAMPLE_SPI_CONTROLLER_BUS DEVICE_DT_GET(DT_BUS(SAMPLE_SPI_CONTROLLER_NODE))
#define SAMPLE_SPI_TARGET_NODE DT_NODELABEL(spi_target)
#define SAMPLE_SPI_TARGET_BUS DEVICE_DT_GET(DT_BUS(SAMPLE_SPI_TARGET_NODE))

#if CONFIG_SAMPLE_SPI_MODE_0
#define SAMPLE_CONFIG_MODE (0)
#elif CONFIG_SAMPLE_SPI_MODE_1
#define SAMPLE_CONFIG_MODE (SPI_MODE_CPHA)
#elif CONFIG_SAMPLE_SPI_MODE_2
#define SAMPLE_CONFIG_MODE (SPI_MODE_CPOL)
#elif CONFIG_SAMPLE_SPI_MODE_3
#define SAMPLE_CONFIG_MODE (SPI_MODE_CPOL | SPI_MODE_CPHA)
#endif

#if CONFIG_SAMPLE_SPI_BIT_ORDER_MSB
#define SAMPLE_CONFIG_BIT_ORDER (SPI_TRANSFER_MSB)
#elif CONFIG_SAMPLE_SPI_BIT_ORDER_LSB
#define SAMPLE_CONFIG_BIT_ORDER (SPI_TRANSFER_LSB)
#endif

#define SAMPLE_SPI_CONTROLLER_CONFIG \
	(SPI_WORD_SET(8) | SAMPLE_CONFIG_MODE | SAMPLE_CONFIG_BIT_ORDER | SPI_OP_MODE_MASTER)

#define SAMPLE_SPI_TARGET_CONFIG \
	(SPI_WORD_SET(8) | SAMPLE_CONFIG_MODE | SAMPLE_CONFIG_BIT_ORDER | SPI_OP_MODE_SLAVE)

#define SAMPLE_TIMEOUT K_SECONDS(1)

#define SAMPLE_WRITE_SIZE CONFIG_SAMPLE_DATA_WRITE_SIZE
#define SAMPLE_READ_SIZE CONFIG_SAMPLE_DATA_READ_SIZE
#define SAMPLE_BUFFER_SIZE (SAMPLE_WRITE_SIZE + SAMPLE_READ_SIZE)

/*
 * The user defines RTIO contexts for which actions like writes and reads will be
 * submitted, and the results of said actions will be retrieved. They are device
 * agnostic.
 *
 * We have one SPI controller device, and one SPI target device. One RTIO context
 * is defined for each of them. This allows us to easily manage their separate
 * transactions as each is tied to a specific RTIO context.
 *
 *
 * We will be using 1 submission queue event (SQEs) for SPI transceive
 * and 1 completion queue event (CQE) for the transaction result.
 */
RTIO_DEFINE(sample_controller_rtio, 1, 1);
RTIO_DEFINE(sample_target_rtio, 1, 1);

/*
 * The user defines RTIO IODEVs which bind the RTIO contexts to the specific
 * underlying devices, in this case SPI devices. The helper macro
 * SPI_DT_IODEV_DEFINE defines an RTIO IODEV specific to a SPI device defined
 * in the devicetree.
 */
SPI_DT_IODEV_DEFINE(sample_controller_rtio_iodev,
		    SAMPLE_SPI_CONTROLLER_NODE,
		    SAMPLE_SPI_CONTROLLER_CONFIG);
SPI_DT_IODEV_DEFINE(sample_target_rtio_iodev,
		    SAMPLE_SPI_TARGET_NODE,
		    SAMPLE_SPI_TARGET_CONFIG);

/* Source of truth transaction data */
static uint8_t sample_write_data[SAMPLE_BUFFER_SIZE];
static uint8_t sample_read_data[SAMPLE_BUFFER_SIZE];

/*
 * The controller will write data from sample_controller_write_data then read
 * data into sample_controller_read_buf.
 */
static uint8_t sample_controller_write_data[SAMPLE_BUFFER_SIZE];
static uint8_t sample_controller_read_buf[SAMPLE_BUFFER_SIZE];

/*
 * The target will read data into sample_target_read_buf, then write data from
 * sample_target_write_data.
 */
static uint8_t sample_target_read_buf[SAMPLE_BUFFER_SIZE];
static uint8_t sample_target_write_data[SAMPLE_BUFFER_SIZE];

static void sample_init_data(void)
{
	memset(sample_write_data, 0xFF, SAMPLE_BUFFER_SIZE);
	for (size_t i = 0; i < SAMPLE_WRITE_SIZE; i++) {
		sample_write_data[i] = (uint8_t)i;
	}

	memset(sample_read_data, 0xFF, SAMPLE_BUFFER_SIZE);
	for (size_t i = 0; i < SAMPLE_READ_SIZE; i++) {
		sample_read_data[i + SAMPLE_WRITE_SIZE] = 255 - (uint8_t)i;
	}

	memcpy(sample_controller_write_data, sample_write_data, SAMPLE_BUFFER_SIZE);
	memset(sample_controller_read_buf, 0x80, SAMPLE_BUFFER_SIZE);
	memset(sample_target_read_buf, 0x80, SAMPLE_BUFFER_SIZE);
	memcpy(sample_target_write_data, sample_read_data, SAMPLE_BUFFER_SIZE);
}

static int sample_validate_transaction(void)
{
	int ret;

	ret = memcmp(sample_read_data, sample_controller_read_buf, SAMPLE_BUFFER_SIZE);
	if (ret) {
		return ret;
	}

	ret = memcmp(sample_write_data, sample_target_read_buf, SAMPLE_BUFFER_SIZE);
	if (ret) {
		return ret;
	}

	return 0;
}

static int sample_submit_target_read_write(void)
{
	struct rtio_sqe *sqe;
	int ret;

	sqe = rtio_sqe_acquire(&sample_target_rtio);
	rtio_sqe_prep_transceive(sqe,
				 &sample_target_rtio_iodev,
				 RTIO_PRIO_NORM,
				 sample_target_write_data,
				 sample_target_read_buf,
				 SAMPLE_BUFFER_SIZE,
				 NULL);

	/*
	 * We will now submit the transaction, allowing the RTIO context to start
	 * performing the SQEs. The RTIO context will start the read SQE, which
	 * will be waiting until we start the write from the SPI controller device.
	 */
	ret = rtio_submit(&sample_target_rtio, 0);
	if (ret) {
		return ret;
	}

	return 0;
}

static int sample_submit_controller_write_read(void)
{
	struct rtio_sqe *sqe;
	int ret;

	sqe = rtio_sqe_acquire(&sample_controller_rtio);
	rtio_sqe_prep_transceive(sqe,
				 &sample_controller_rtio_iodev,
				 RTIO_PRIO_NORM,
				 sample_controller_write_data,
				 sample_controller_read_buf,
				 SAMPLE_BUFFER_SIZE,
				 NULL);

	/*
	 * The RTIO context will start the write SQE, which the waiting target
	 * device will receive, allowing both transactions to complete in the
	 * background.
	 */
	ret = rtio_submit(&sample_controller_rtio, 0);
	if (ret) {
		return ret;
	}

	return 0;
}

static int sample_await_target_read_write(void)
{
	struct rtio_cqe *cqe;
	int ret;

	/* Wait for the CQE generated by the target device transaction completing */
	cqe = rtio_cqe_consume_block(&sample_target_rtio);

	/*
	 * Copy the result of the transaction. If any SQE of the transaction fails,
	 * CQEs will still be generated for successive SQEs if RTIO_SQE_NO_RESPONSE
	 * is not set for them.
	 */
	ret = cqe->result;

	/* Remember to release the CQE once done */
	rtio_cqe_release(&sample_target_rtio, cqe);

	return ret;
}

static int sample_await_controller_write_read(void)
{
	struct rtio_cqe *cqe;
	int ret;

	cqe = rtio_cqe_consume_block(&sample_controller_rtio);

	ret = cqe->result;

	rtio_cqe_release(&sample_controller_rtio, cqe);

	return ret;
}

int main(void)
{
	int ret;

	if (!spi_is_ready_iodev(&sample_controller_rtio_iodev)) {
		printk("%s iodev not ready\n", "controller");
		return 0;
	}

	if (!spi_is_ready_iodev(&sample_controller_rtio_iodev)) {
		printk("%s iodev not ready\n", "target");
		return 0;
	}

	sample_init_data();

	printk("%s %s\n", "submit_target_read_write", "running");
	ret = sample_submit_target_read_write();
	if (ret) {
		printk("%s %s (%d)\n", "submit_target_read_write", "failed", ret);
		return 0;
	}

	/*
	 * Since RTIO, and the hardware itself, is asynchronous, we can't know exactly when
	 * the SPI target device is ready to transceive. We wait "a while" post submitting
	 * the transceive SQE to increase the odds the SPI target is ready and waiting for
	 * the SPI controller to start the transaction.
	 */
	printk("waiting for target ready\n");
	k_sleep(SAMPLE_TIMEOUT);

	printk("%s %s\n", "submit_controller_write_read", "running");
	ret = sample_submit_controller_write_read();
	if (ret) {
		printk("%s %s (%d)\n", "submit_controller_write_read", "failed", ret);
		return 0;
	}

	printk("%s %s\n", "await_target_read_write", "running");
	ret = sample_await_target_read_write();
	if (ret < 0) {
		printk("%s %s (%d)\n", "await_target_read_write", "failed", ret);
		return 0;
	}

	printk("%s %s\n", "await_controller_write_read", "running");
	ret = sample_await_controller_write_read();
	if (ret) {
		printk("%s %s (%d)\n", "await_controller_write_read", "failed", ret);
		return 0;
	}

	printk("%s %s\n", "sample_validate_transaction", "running");
	ret = sample_validate_transaction();
	if (ret) {
		printk("%s %s (%d)\n", "sample_validate_transaction", "failed", ret);
		return 0;
	}

	printk("sample complete\n");
	return 0;
}
