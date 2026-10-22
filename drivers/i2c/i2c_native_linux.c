/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_native_linux_i2c

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <cmdline.h>
#include <nsi_errno.h>
#include <nsi_tracing.h>
#include <posix_native_task.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/dt-bindings/i2c/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include "i2c-priv.h"
#include "i2c_native_linux_bottom.h"

#define I2C_NATIVE_LINUX_IRQ_PRIORITY 2

/* Read data to copy out of the bounce buffer */
struct i2c_native_linux_copy {
	uint8_t *dst;
	const uint8_t *src;
	uint32_t len;
};

struct i2c_native_linux_config {
	const char *path;
	uint32_t bitrate;
};

struct i2c_native_linux_data {
	/* --<node name>_path */
	char *cmdline_path;
	/* Host side of the adapter, NULL when it could not be opened */
	void *host;
	uint32_t dev_config;
	struct k_mutex lock;
	/* Given by the ISR when the host thread is done with a transfer */
	struct k_sem done;
	int result;
	struct i2c_native_linux_msg msgs[I2C_NATIVE_LINUX_MAX_MSGS];
	/* msgs[i] points into the bounce buffer */
	bool bounced[I2C_NATIVE_LINUX_MAX_MSGS];
	/* One per read message, i2c_transfer() takes at most UINT8_MAX messages */
	struct i2c_native_linux_copy copies[UINT8_MAX];
	uint8_t bounce[CONFIG_I2C_NATIVE_LINUX_BOUNCE_SIZE];
};

/* A NACK and other bus errors become -EIO */
static int i2c_native_linux_error(int ret)
{
	int err;

	err = nsi_errno_from_mid(-ret);

	switch (err) {
	case EOPNOTSUPP:
		return -ENOTSUP;
	case ETIMEDOUT:
	case EAGAIN:
		return -err;
	default:
		return -EIO;
	}
}

static int i2c_native_linux_configure(const struct device *dev, uint32_t dev_config)
{
	struct i2c_native_linux_data *data = dev->data;

	/* i2c-dev has no target mode */
	if ((dev_config & I2C_MODE_CONTROLLER) == 0U) {
		return -ENOTSUP;
	}

	/* The host adapter sets the bus speed, the configuration is only kept for get_config() */
	data->dev_config = dev_config;

	return 0;
}

static int i2c_native_linux_get_config(const struct device *dev, uint32_t *dev_config)
{
	const struct i2c_native_linux_data *data = dev->data;

	*dev_config = data->dev_config;

	return 0;
}

/* Copy write data to <start> now, or record where read data goes after the transfer */
static void i2c_native_linux_bounce(struct i2c_native_linux_data *data, int *ncopies,
				    uint8_t *start, uint8_t *buf, uint32_t len, bool read)
{
	struct i2c_native_linux_copy *copy;

	if (read) {
		copy = &data->copies[*ncopies];
		copy->dst = buf;
		copy->src = start;
		copy->len = len;
		(*ncopies)++;
	} else if (len > 0U) {
		memcpy(start, buf, len);
	}
}

/*
 * Join a message without RESTART to the previous one of the same direction.
 * Few adapters support I2C_M_NOSTART, so both are merged in the bounce buffer.
 */
static int i2c_native_linux_append(struct i2c_native_linux_data *data, int n, size_t *used,
				   int *ncopies, const struct i2c_msg *msg)
{
	struct i2c_native_linux_msg *prev;
	bool prev_bounced;
	uint8_t *start;
	size_t need;
	bool read;

	prev = &data->msgs[n - 1];
	prev_bounced = data->bounced[n - 1];
	read = (prev->flags & I2C_NATIVE_LINUX_M_RD) != 0U;

	need = msg->len;
	if (!prev_bounced) {
		need += prev->len;
	}

	if ((prev->len + msg->len) > UINT16_MAX || (*used + need) > sizeof(data->bounce)) {
		nsi_print_warning("i2c-native-linux: transfer does not fit the %u byte bounce "
				  "buffer, see CONFIG_I2C_NATIVE_LINUX_BOUNCE_SIZE\n",
				  (unsigned int)sizeof(data->bounce));
		return -ENOMEM;
	}

	/* Move the previous message into the bounce buffer once */
	if (!prev_bounced) {
		start = &data->bounce[*used];
		i2c_native_linux_bounce(data, ncopies, start, prev->buf, prev->len, read);

		prev->buf = start;
		*used += prev->len;
		data->bounced[n - 1] = true;
	}

	/* It is last in the buffer, so it grows in place */
	start = &data->bounce[*used];
	i2c_native_linux_bounce(data, ncopies, start, msg->buf, msg->len, read);

	*used += msg->len;
	prev->len += msg->len;

	return 0;
}

static int i2c_native_linux_transfer_msgs(const struct device *dev, struct i2c_msg *msgs,
					  uint8_t num_msgs, uint16_t addr)
{
	struct i2c_native_linux_data *data = dev->data;
	struct i2c_native_linux_copy *copy;
	struct i2c_native_linux_msg *hmsg;
	const struct i2c_msg *msg;
	size_t used = 0U;
	int ncopies = 0;
	int n = 0;
	int ret = 0;
	bool prev_read;
	bool joined;
	bool read;
	bool last;
	uint8_t i;
	int c;

	if (data->host == NULL) {
		return -ENODEV;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	for (i = 0U; i < num_msgs; i++) {
		msg = &msgs[i];
		read = i2c_is_read_op(msg);
		/* i2c-dev ends every transfer with a stop condition */
		last = i2c_is_stop_op(msg) || i == (num_msgs - 1U);

		joined = false;
		if (n > 0) {
			prev_read = (data->msgs[n - 1].flags & I2C_NATIVE_LINUX_M_RD) != 0U;
			joined = !i2c_is_restart_op(msg) && read == prev_read;
		}

		if (joined) {
			ret = i2c_native_linux_append(data, n, &used, &ncopies, msg);
		} else if (n == I2C_NATIVE_LINUX_MAX_MSGS || msg->len > UINT16_MAX) {
			ret = -EINVAL;
		} else {
			hmsg = &data->msgs[n];
			hmsg->buf = msg->buf;
			hmsg->addr = addr;
			hmsg->len = (uint16_t)msg->len;
			hmsg->flags = 0U;

			if (read) {
				hmsg->flags |= I2C_NATIVE_LINUX_M_RD;
			}

			if ((msg->flags & I2C_MSG_ADDR_10_BITS) != 0U) {
				hmsg->flags |= I2C_NATIVE_LINUX_M_TEN;
			}

			data->bounced[n] = false;
			n++;
		}

		if (ret != 0) {
			break;
		}

		if (!last) {
			continue;
		}

		/* The host thread runs the transfer, simulated time goes on meanwhile */
		i2c_native_linux_transfer_start(data->host, data->msgs, n);
		k_sem_take(&data->done, K_FOREVER);

		ret = data->result;
		if (ret != 0) {
			ret = i2c_native_linux_error(ret);
			break;
		}

		for (c = 0; c < ncopies; c++) {
			copy = &data->copies[c];
			memcpy(copy->dst, copy->src, copy->len);
		}

		used = 0U;
		ncopies = 0;
		n = 0;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static DEVICE_API(i2c, i2c_native_linux_api) = {
	.configure = i2c_native_linux_configure,
	.get_config = i2c_native_linux_get_config,
	.transfer = i2c_native_linux_transfer_msgs,
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

static void i2c_native_linux_isr(const void *arg);

static int i2c_native_linux_init(const struct device *dev)
{
	const struct i2c_native_linux_config *config = dev->config;
	struct i2c_native_linux_data *data = dev->data;
	const char *path;
	int ret;

	k_mutex_init(&data->lock);
	k_sem_init(&data->done, 0, 1);
	data->dev_config = I2C_MODE_CONTROLLER | i2c_map_dt_bitrate(config->bitrate);

	path = config->path;
	if (data->cmdline_path != NULL) {
		path = data->cmdline_path;
	}

	data->host = i2c_native_linux_open(path, CONFIG_I2C_NATIVE_LINUX_IRQ,
					   CONFIG_I2C_NATIVE_LINUX_POLL_PERIOD, &ret);
	if (data->host == NULL) {
		nsi_print_warning("%s: no I2C adapter at %s\n", dev->name, path);
		return -ENODEV;
	}

	/* Shared by all instances */
	IRQ_CONNECT(CONFIG_I2C_NATIVE_LINUX_IRQ, I2C_NATIVE_LINUX_IRQ_PRIORITY,
		    i2c_native_linux_isr, NULL, 0);
	irq_enable(CONFIG_I2C_NATIVE_LINUX_IRQ);

	return 0;
}

#define I2C_NATIVE_LINUX_INIT(inst)                                                                \
	static const struct i2c_native_linux_config i2c_native_linux_config_##inst = {             \
		.path = DT_INST_PROP(inst, path),                                                  \
		.bitrate = DT_INST_PROP_OR(inst, clock_frequency, I2C_BITRATE_STANDARD),           \
	};                                                                                         \
                                                                                                   \
	static struct i2c_native_linux_data i2c_native_linux_data_##inst;                          \
                                                                                                   \
	I2C_DEVICE_DT_INST_DEFINE(inst, i2c_native_linux_init, NULL,                               \
				  &i2c_native_linux_data_##inst, &i2c_native_linux_config_##inst,  \
				  POST_KERNEL, CONFIG_I2C_INIT_PRIORITY, &i2c_native_linux_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_NATIVE_LINUX_INIT)

static ALWAYS_INLINE void i2c_native_linux_done(struct i2c_native_linux_data *data)
{
	if (data->host != NULL && i2c_native_linux_transfer_done(data->host, &data->result)) {
		k_sem_give(&data->done);
	}
}

#define I2C_NATIVE_LINUX_DONE(inst) i2c_native_linux_done(&i2c_native_linux_data_##inst);

static void i2c_native_linux_isr(const void *arg)
{
	ARG_UNUSED(arg);

	DT_INST_FOREACH_STATUS_OKAY(I2C_NATIVE_LINUX_DONE)
}

#define I2C_NATIVE_LINUX_OPTION(inst)                                                              \
	{                                                                                          \
		.option = DEVICE_DT_NAME(DT_DRV_INST(inst)) "_path",                               \
		.name = "path",                                                                    \
		.type = 's',                                                                       \
		.dest = (void *)&i2c_native_linux_data_##inst.cmdline_path,                        \
		.descript = "i2c-dev node of the " DEVICE_DT_NAME(DT_DRV_INST(inst))               \
			    " I2C controller, overrides its devicetree path",                      \
	},

static void i2c_native_linux_options(void)
{
	static struct args_struct_t options[] = {
		DT_INST_FOREACH_STATUS_OKAY(I2C_NATIVE_LINUX_OPTION) ARG_TABLE_ENDMARKER};

	native_add_command_line_opts(options);
}

NATIVE_TASK(i2c_native_linux_options, PRE_BOOT_1, 10);
NATIVE_TASK(i2c_native_linux_close_all, ON_EXIT, 99);
