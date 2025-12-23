/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/drivers/i2c/rtio.h>

#include <string.h>

#define I2C_CONTROLLER_NODE              DT_ALIAS(i2c_controller)
#define I2C_CONTROLLER_TARGET_NODE       DT_ALIAS(i2c_controller_target)
#define I2C_CONTROLLER_DEVICE_GET        DEVICE_DT_GET(I2C_CONTROLLER_NODE)
#define I2C_CONTROLLER_TARGET_DEVICE_GET DEVICE_DT_GET(I2C_CONTROLLER_TARGET_NODE)
#define I2C_TARGET_ADDR                  0x0A
#define SAMPLE_TIMEOUT                   K_SECONDS(1)

static const struct device *sample_i2c_controller = I2C_CONTROLLER_DEVICE_GET;
static const struct device *sample_i2c_controller_target = I2C_CONTROLLER_TARGET_DEVICE_GET;

/* Data to write and buffer to store write in */
static uint8_t sample_write_data[CONFIG_I2C_RTIO_LOOPBACK_DATA_WRITE_MAX_SIZE];
static uint8_t sample_write_buf[sizeof(sample_write_data)];
static uint32_t sample_write_buf_pos;

/* Data to read and buffer to store read in */
static uint8_t sample_read_data[CONFIG_I2C_RTIO_LOOPBACK_DATA_READ_MAX_SIZE];
static uint32_t sample_read_data_pos;
static uint8_t sample_read_buf[sizeof(sample_read_data)];

/*
 * The user defines an RTIO context to which actions like writes and reads will be
 * submitted, and the results of said actions will be retrieved.
 *
 * We will be using 3 submission queue events (SQEs); i2c write, i2c read,
 * done callback, and 2 completion queue events (CQEs); i2c write result,
 * i2c read result.
 */
RTIO_DEFINE(sample_rtio, 3, 2);

/*
 * The user defines an RTIO IODEV which wraps the device which will perform the
 * actions submitted to the RTIO context. In this sample, we are using an I2C
 * controller device, so we use the I2C specific helper to define the RTIO IODEV.
 */
I2C_IODEV_DEFINE(sample_rtio_iodev, I2C_CONTROLLER_NODE, I2C_TARGET_ADDR);

/*
 * For async write read operation we will be waiting for a callback from RTIO.
 * We will wait on this sem which we will give from the callback.
 */
static K_SEM_DEFINE(sample_write_read_sem, 0, 1);

/*
 * We register a simple I2C target which we will be targeting using RTIO. We
 * store written data, and return sample_read_data when read.
 */
static int sample_target_write_requested(struct i2c_target_config *target_config)
{
	sample_write_buf_pos = 0;
	return 0;
}

static int sample_target_read_requested(struct i2c_target_config *target_config, uint8_t *val)
{
	sample_read_data_pos = 0;
	*val = sample_read_data[sample_read_data_pos];
	return 0;
}

static int sample_target_write_received(struct i2c_target_config *target_config, uint8_t val)
{
	if (sample_write_buf_pos == sizeof(sample_write_buf)) {
		return -ENOMEM;
	}

	sample_write_buf[sample_write_buf_pos] = val;
	sample_write_buf_pos++;
	return 0;
}

static int sample_target_read_processed(struct i2c_target_config *target_config, uint8_t *val)
{
	sample_read_data_pos++;

	if (sample_read_data_pos == sizeof(sample_read_data)) {
		return -ENOMEM;
	}

	*val = sample_read_data[sample_read_data_pos];
	return 0;
}

#ifdef CONFIG_I2C_TARGET_BUFFER_MODE
static void sample_target_buf_write_received(struct i2c_target_config *target_config,
					     uint8_t *data,
					     uint32_t size)
{
	sample_write_buf_pos = MIN(size, ARRAY_SIZE(sample_write_buf));
	memcpy(sample_write_buf, data, sample_write_buf_pos);
}

static int sample_target_buf_read_requested(struct i2c_target_config *target_config,
					    uint8_t **data,
					    uint32_t *size)
{
	*data = sample_read_data;
	*size = sizeof(sample_read_data);
	return 0;
}
#endif /* CONFIG_I2C_TARGET_BUFFER_MODE */

static int sample_target_stop(struct i2c_target_config *config)
{
	ARG_UNUSED(config);
	return 0;
}

static const struct i2c_target_callbacks sample_target_callbacks = {
	.write_requested = sample_target_write_requested,
	.read_requested = sample_target_read_requested,
	.write_received = sample_target_write_received,
	.read_processed = sample_target_read_processed,
#ifdef CONFIG_I2C_TARGET_BUFFER_MODE
	.buf_write_received = sample_target_buf_write_received,
	.buf_read_requested = sample_target_buf_read_requested,
#endif
	.stop = sample_target_stop,
};

static struct i2c_target_config sample_target_config = {
	.address = I2C_TARGET_ADDR,
	.callbacks = &sample_target_callbacks,
};

static int sample_init_i2c_target(void)
{
	return i2c_target_register(sample_i2c_controller_target, &sample_target_config);
}

static void sample_reset_buffers(void)
{
	memset(sample_write_buf, 0, sizeof(sample_write_buf));
	memset(sample_read_buf, 0, sizeof(sample_read_buf));
}

static int sample_standard_write_read(void)
{
	int ret;
	struct i2c_msg msgs[2];

	msgs[0].buf = sample_write_data;
	msgs[0].len = sizeof(sample_write_data);
	msgs[0].flags = I2C_MSG_WRITE;

	msgs[1].buf = sample_read_buf;
	msgs[1].len = sizeof(sample_read_buf);
	msgs[1].flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP;

	ret = i2c_transfer(sample_i2c_controller,
			   msgs,
			   ARRAY_SIZE(msgs),
			   I2C_TARGET_ADDR);
	if (ret) {
		return -EIO;
	}

	return 0;
}

static int sample_validate_write_read(void)
{
	int ret;

	if (sample_write_buf_pos != sizeof(sample_write_data)) {
		printk("Posittion error: %zu != %zu\n",
		       sample_write_buf_pos, sizeof(sample_write_data));
		return -EIO;
	}

	ret = memcmp(sample_write_buf, sample_write_data, sizeof(sample_write_data));
	if (ret) {
		for (int n = 0; n < sizeof(sample_write_data); n++) {
			if (sample_write_buf[n] != sample_write_data[n]) {
				printk("Write at offset %u: %02x != %02x\n",
				       n, sample_write_buf[n], sample_write_data[n]);
			}
		}
		return -EIO;
	}

	ret = memcmp(sample_read_buf, sample_read_data, sizeof(sample_read_data));
	if (ret) {
		for (int n = 0; n < sizeof(sample_read_data); n++) {
			if (sample_read_buf[n] != sample_read_data[n]) {
				printk("Read at offset %u: %02x != %02x\n",
				       n, sample_read_buf[n], sample_read_data[n]);
			}
		}
		return -EIO;
	}

	return 0;
}

/* This is functionally identical to sample_standard_write_read() but uses RTIO */
static int sample_rtio_write_read(void)
{
	struct rtio_sqe *wr_sqe, *rd_sqe;
	struct rtio_cqe *wr_rd_cqe;
	int ret;

	/*
	 * We allocate one of the 3 submission queue events (SQEs) as defined by
	 * RTIO_DEFINE() and configure it to write sample_write_data to
	 * sample_rtio_iodev.
	 */
	wr_sqe = rtio_sqe_acquire(&sample_rtio);
	rtio_sqe_prep_write(wr_sqe,
			    &sample_rtio_iodev,
			    0,
			    sample_write_data,
			    sizeof(sample_write_data),
			    NULL);

	/*
	 * This write SQE is followed by a read SQE, which is part of a single
	 * transaction. We configure this by setting the RTIO_SQE_TRANSACTION.
	 */
	wr_sqe->flags |= RTIO_SQE_TRANSACTION;

	/*
	 * We then allocate an SQE and configure it to read
	 * sizeof(sample_read_buf) into sample_read_buf.
	 */
	rd_sqe = rtio_sqe_acquire(&sample_rtio);
	rtio_sqe_prep_read(rd_sqe,
			    &sample_rtio_iodev,
			    0,
			    sample_read_buf,
			    sizeof(sample_read_buf), NULL);

	/*
	 * Since we are working with I2C messages, we need to specify the I2C
	 * message options. The I2C_READ and I2C_WRITE are implicit since we
	 * are preparing read and write SQEs, I2C_STOP and I2C_RESTART are not,
	 * so we add them to the read SQE.
	 */
	rd_sqe->iodev_flags = RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;

	/*
	 * With the two SQEs of the sample_rtio context prepared, we call
	 * rtio_submit() to have them executed. This call will execute all
	 * prepared SQEs.
	 *
	 * In this case, we wait for the two SQEs to be completed before
	 * continuing, similar to calling i2c_transfer().
	 */
	ret = rtio_submit(&sample_rtio, 2);
	if (ret) {
		return -EIO;
	}

	/*
	 * Since the RTIO SQEs are executed in the background, we need to
	 * get the CQE after and check its result. Since we configured the
	 * write and read SQEs as a single transaction, only one CQE is
	 * generated which includes both of them. If we had chained them
	 * instead, one CQE would be created for each of them.
	 */
	wr_rd_cqe = rtio_cqe_consume(&sample_rtio);
	if (wr_rd_cqe->result) {
		return -EIO;
	}

	/* Release the CQE after having checked its result. */
	rtio_cqe_release(&sample_rtio, wr_rd_cqe);
	return 0;
}

static void rtio_write_read_done_callback(struct rtio *r, const struct rtio_sqe *sqe,
					  int result, void *arg0)
{
	struct k_sem *sem = arg0;
	struct rtio_cqe *wr_rd_cqe;

	/* See sample_rtio_write_read() */
	wr_rd_cqe = rtio_cqe_consume(&sample_rtio);
	if (wr_rd_cqe->result) {
		/* Signal write and read SQEs completed with error */
		k_sem_reset(sem);
	}

	/* See sample_rtio_write_read() */
	rtio_cqe_release(&sample_rtio, wr_rd_cqe);

	/* Signal write and read SQEs completed with success */
	k_sem_give(sem);
}

/*
 * Aside from the blocking wait for the sample_write_read_sem, async RTIO
 * can be performed entirely from within ISRs.
 */
static int sample_rtio_write_read_async(void)
{
	struct rtio_sqe *wr_sqe, *rd_sqe, *cb_sqe;
	int ret;

	/* See sample_rtio_write_read() */
	wr_sqe = rtio_sqe_acquire(&sample_rtio);
	rtio_sqe_prep_write(wr_sqe,
			    &sample_rtio_iodev,
			    0,
			    sample_write_data,
			    sizeof(sample_write_data), NULL);
	wr_sqe->flags |= RTIO_SQE_TRANSACTION;

	/* See sample_rtio_write_read() */
	rd_sqe = rtio_sqe_acquire(&sample_rtio);
	rtio_sqe_prep_read(rd_sqe,
			    &sample_rtio_iodev,
			    0,
			    sample_read_buf,
			    sizeof(sample_read_buf), NULL);
	rd_sqe->iodev_flags = RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;

	/*
	 * The next SQE is a callback which we will use to signal that the
	 * write and read SQEs have completed. It has to be executed only
	 * after the write and read SQEs, which we configure by setting the
	 * RTIO_SQE_CHAINED flag.
	 */
	rd_sqe->flags |= RTIO_SQE_CHAINED;

	/*
	 * Prepare the callback SQE. The SQE allows us to pass an optional
	 * argument to the handler, which we will use to store a pointer to
	 * the binary semaphore we will be waiting on.
	 */
	cb_sqe = rtio_sqe_acquire(&sample_rtio);
	rtio_sqe_prep_callback_no_cqe(cb_sqe,
				      rtio_write_read_done_callback,
				      &sample_write_read_sem,
				      NULL);

	/*
	 * Submit the SQEs for execution, without waiting for any of them
	 * to be completed. We use the callback to signal completion of all
	 * of them.
	 */
	ret = rtio_submit(&sample_rtio, 0);
	if (ret) {
		return -EIO;
	}

	/*
	 * We wait for the callback which signals RTIO transfer has completed.
	 *
	 * We will be checking the CQE result from within the callback, which
	 * is entirely safe given RTIO is designed to work from ISR context.
	 * If the result is ok, we give the sem, if its not ok, we reset the
	 * sem (which makes k_sem_take() return an error).
	 */
	ret = k_sem_take(&sample_write_read_sem, SAMPLE_TIMEOUT);
	if (ret) {
		return -EIO;
	}

	return 0;
}

int main(void)
{
	int ret, n;

	for (n = 0; n < sizeof(sample_write_data); n++) {
		sample_write_data[n] = (0xFF - n) % 0xFF;
	}

	for (n = 0; n < sizeof(sample_read_data); n++) {
		sample_read_data[n] = n % 0xFF;
	}

	printk("%s %s\n", "init_i2c_target", "running");
	ret = sample_init_i2c_target();
	if (ret) {
		printk("%s %s\n", "init_i2c_target", "failed");
		return 0;
	}

	sample_reset_buffers();

	printk("%s %s\n", "standard_write_read", "running");
	ret = sample_standard_write_read();
	if (ret) {
		printk("%s %s\n", "standard_write_read", "failed");
		return 0;
	}

	ret = sample_validate_write_read();
	if (ret) {
		printk("%s %s\n", "standard_write_read", "corrupted");
		return 0;
	}

	sample_reset_buffers();

	printk("%s %s\n", "rtio_write_read", "running");
	ret = sample_rtio_write_read();
	if (ret) {
		printk("%s %s\n", "rtio_write_read", "failed");
		return 0;
	}

	ret = sample_validate_write_read();
	if (ret) {
		printk("%s %s\n", "rtio_write_read", "corrupted");
		return 0;
	}

	sample_reset_buffers();

	printk("%s %s\n", "rtio_write_read_async", "running");
	ret = sample_rtio_write_read_async();
	if (ret) {
		printk("%s %s\n", "rtio_write_read_async", "failed");
		return 0;
	}

	ret = sample_validate_write_read();
	if (ret) {
		printk("%s %s\n", "rtio_write_read_async", "corrupted");
		return 0;
	}

	printk("sample complete\n");
	return 0;
}
