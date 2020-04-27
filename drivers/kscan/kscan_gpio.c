/*
 * Copyright (c) 2020 Peter Johanson <peter@peterjohanson.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gpio_kscan

#include <device.h>
#include <drivers/kscan.h>
#include <drivers/gpio.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(kscan_gpio, CONFIG_KSCAN_LOG_LEVEL);

#define MATRIX_NODE_ID DT_DRV_INST(0)
#define MATRIX_ROWS DT_PROP_LEN(MATRIX_NODE_ID, row_gpios)
#define MATRIX_COLS DT_PROP_LEN(MATRIX_NODE_ID, col_gpios)

#if DT_ENUM_IDX(MATRIX_NODE_ID, diode_direction) == 0
#define SCAN_ROW_TO_COLUMN
#else
#define SCAN_COL_TO_ROW
#endif

struct kscan_gpio_item_config {
	char *label;
	gpio_pin_t pin;
	gpio_flags_t flags;
};

struct kscan_gpio_config {
	u8_t debounce_period;
	struct kscan_gpio_item_config rows[MATRIX_ROWS];
	struct kscan_gpio_item_config cols[MATRIX_COLS];
};

struct kscan_gpio_data {
	kscan_callback_t callback;
#if DT_PROP(MATRIX_NODE_ID, debounce_id) == 0
	struct k_work work;
#else
	struct k_delayed_work work;
#endif
	bool matrix_state[MATRIX_ROWS][MATRIX_COLS];
	struct device *rows[MATRIX_ROWS];
	struct device *cols[MATRIX_COLS];
	struct device *dev;
};

struct kscan_gpio_irq_callback {
#if DT_PROP(MATRIX_NODE_ID, debounce_id) == 0
	struct k_work *work;
#else
	struct k_delayed_work *work;
#endif
	struct gpio_callback callback;
};

static int kscan_gpio_config_interrupts(struct device **devices,
		const struct kscan_gpio_item_config *configs,
		size_t len, gpio_flags_t flags)
{
	for (int i = 0; i < len; i++) {
		struct device *dev = devices[i];
		const struct kscan_gpio_item_config *cfg = &configs[i];

		int err = gpio_pin_interrupt_configure(dev, cfg->pin, flags);

		if (err) {
			LOG_ERR("Unable to enable matrix GPIO interrupt");
			return err;
		}
	}

	return 0;
}


#ifdef SCAN_ROW_TO_COLUMN

static struct kscan_gpio_irq_callback irq_callbacks[MATRIX_COLS];

static int kscan_gpio_enable_interrupts(struct device *dev)
{
	struct kscan_gpio_data *data = dev->driver_data;
	const struct kscan_gpio_config *cfg = dev->config_info;

	return kscan_gpio_config_interrupts(
			data->cols, cfg->cols, MATRIX_COLS,
			GPIO_INT_DEBOUNCE | GPIO_INT_EDGE_BOTH);

}

static int kscan_gpio_disable_interrupts(struct device *dev)
{
	struct kscan_gpio_data *data = dev->driver_data;
	const struct kscan_gpio_config *cfg = dev->config_info;

	return kscan_gpio_config_interrupts(
			data->cols, cfg->cols, MATRIX_COLS,
			GPIO_INT_DISABLE);
}

static void kscan_gpio_set_row_state(struct device *dev, int value)
{
	struct kscan_gpio_data *data = dev->driver_data;
	const struct kscan_gpio_config *cfg = dev->config_info;

	for (int r = 0; r < MATRIX_ROWS; r++) {
		struct device *row = data->rows[r];
		const struct kscan_gpio_item_config *row_cfg = &cfg->rows[r];

		gpio_pin_set(row, row_cfg->pin, value);
	}
}


static int kscan_gpio_read(struct device *dev)
{
	struct kscan_gpio_data *data = dev->driver_data;
	const struct kscan_gpio_config *cfg = dev->config_info;

	static bool read_state[MATRIX_ROWS][MATRIX_COLS];

	LOG_DBG("Scanning the matrix for updated state");

	/* Disable our interrupts temporarily while we scan, to avoid       */
	/* re-entry while we iterate columns and set them active one by one */
	/* to get pressed state for each matrix cell.                       */
	kscan_gpio_disable_interrupts(dev);
	kscan_gpio_set_row_state(dev, 0);

	for (int r = 0; r < MATRIX_ROWS; r++) {
		struct device *row = data->rows[r];
		const struct kscan_gpio_item_config *row_cfg = &cfg->rows[r];

		gpio_pin_set(row, row_cfg->pin, 1);
		for (int c = 0; c < MATRIX_COLS; c++) {
			struct device *col = data->cols[c];
			const struct kscan_gpio_item_config *col_cfg
				= &cfg->cols[c];

			read_state[r][c] =
				gpio_pin_get(col, col_cfg->pin) > 0;
		}

		gpio_pin_set(row, row_cfg->pin, 0);
	}

	/* Set all our outputs as active again, then re-enable interrupts, */
	/* so we can trigger interrupts again for future press/release     */
	kscan_gpio_set_row_state(dev, 1);
	kscan_gpio_enable_interrupts(dev);

	for (int r = 0; r < MATRIX_ROWS; r++) {
		for (int c = 0; c < MATRIX_COLS; c++) {
			bool pressed = read_state[r][c];

			if (pressed != data->matrix_state[r][c]) {
				LOG_DBG("Sending event at %d,%d state %s",
					r, c, (pressed ? "on" : "off"));
				data->matrix_state[r][c] = pressed;
				data->callback(dev, r, c, pressed);
			}
		}
	}

	return 0;
}

#else

static struct kscan_gpio_irq_callback irq_callbacks[MATRIX_ROWS];

static int kscan_gpio_enable_interrupts(struct device *dev)
{
	struct kscan_gpio_data *data = dev->driver_data;
	const struct kscan_gpio_config *cfg = dev->config_info;

	return kscan_gpio_config_interrupts(data->rows, cfg->rows, MATRIX_ROWS,
			GPIO_INT_DEBOUNCE | GPIO_INT_EDGE_BOTH);
}

static int kscan_gpio_disable_interrupts(struct device *dev)
{
	struct kscan_gpio_data *data = dev->driver_data;
	const struct kscan_gpio_config *cfg = dev->config_info;

	return kscan_gpio_config_interrupts(data->rows, cfg->rows, MATRIX_ROWS,
			GPIO_INT_DISABLE);
}

static void kscan_gpio_set_column_state(struct device *dev, int value)
{
	struct kscan_gpio_data *data = dev->driver_data;
	const struct kscan_gpio_config *cfg = dev->config_info;

	for (int c = 0; c < MATRIX_COLS; c++) {
		struct device *col = data->cols[c];
		const struct kscan_gpio_item_config *col_cfg = &cfg->cols[c];

		gpio_pin_set(col, col_cfg->pin, value);
	}
}

static int kscan_gpio_read(struct device *dev)
{
	struct kscan_gpio_data *data = dev->driver_data;
	const struct kscan_gpio_config *cfg = dev->config_info;

	static bool read_state[MATRIX_ROWS][MATRIX_COLS];

	LOG_DBG("Scanning the matrix for updated state");

	/* Disable our interrupts temporarily while we scan, to avoid       */
	/* re-entry while we iterate columns and set them active one by one */
	/* to get pressed state for each matrix cell.                       */
	kscan_gpio_disable_interrupts(dev);
	kscan_gpio_set_column_state(dev, 0);

	for (int c = 0; c < MATRIX_COLS; c++) {
		struct device *col = data->cols[c];
		const struct kscan_gpio_item_config *col_cfg = &cfg->cols[c];

		gpio_pin_set(col, col_cfg->pin, 1);

		for (int r = 0; r < MATRIX_ROWS; r++) {
			struct device *row = data->rows[r];
			const struct kscan_gpio_item_config *row_cfg
				= &cfg->rows[r];

			read_state[r][c] =
				gpio_pin_get(row, row_cfg->pin) > 0;
		}

		gpio_pin_set(col, col_cfg->pin, 0);
	}

	/* Set all our outputs as active again, then re-enable interrupts, */
	/* so we can trigger interrupts again for future press/release     */
	kscan_gpio_set_column_state(dev, 1);
	kscan_gpio_enable_interrupts(dev);

	for (int r = 0; r < MATRIX_ROWS; r++) {
		for (int c = 0; c < MATRIX_COLS; c++) {
			bool pressed = read_state[r][c];

			if (pressed != data->matrix_state[r][c]) {
				LOG_DBG("Sending event at %d,%d state %s",
					r, c, (pressed ? "on" : "off"));
				data->matrix_state[r][c] = pressed;
				data->callback(dev, r, c, pressed);
			}
		}
	}

	return 0;
}

#endif

static void kscan_gpio_irq_callback_handler(struct device *dev,
		struct gpio_callback *cb, gpio_port_pins_t pin)
{
	struct kscan_gpio_irq_callback *data =
		CONTAINER_OF(cb, struct kscan_gpio_irq_callback, callback);

#if DT_PROP(MATRIX_NODE_ID, debounce_id) == 0
	k_work_submit(data->work);
#else
	k_delayed_work_cancel(data->work);
	k_delayed_work_submit(data->work,
			K_MSEC(DT_PROP(MATRIX_NODE_ID, debounce_id)));
#endif
}

static void kscan_gpio_work_handler(struct k_work *work)
{
	struct kscan_gpio_data *data =
		CONTAINER_OF(work, struct kscan_gpio_data, work);

	kscan_gpio_read(data->dev);
}

static int kscan_gpio_configure(struct device *dev, kscan_callback_t callback)
{
	struct kscan_gpio_data *data = dev->driver_data;

	if (!callback) {
		return -EINVAL;
	}

	data->callback = callback;

	return 0;
}

static int kscan_gpio_init(struct device *dev)
{
	struct kscan_gpio_data *data = dev->driver_data;
	const struct kscan_gpio_config *cfg = dev->config_info;
	int err;

#ifdef SCAN_ROW_TO_COLUMN
	gpio_flags_t col_gpio_dir = GPIO_INPUT;
	gpio_flags_t row_gpio_dir = GPIO_OUTPUT_ACTIVE;
#else
	gpio_flags_t col_gpio_dir = GPIO_OUTPUT_ACTIVE;
	gpio_flags_t row_gpio_dir = GPIO_INPUT;
#endif

	for (int c = 0; c < MATRIX_COLS; c++) {
		const struct kscan_gpio_item_config *item_cfg = &cfg->cols[c];

		data->cols[c] = device_get_binding(item_cfg->label);
		if (data->cols[c] == NULL) {
			LOG_ERR("Unable to find column GPIO device\n");
			return -EINVAL;
		}

		err = gpio_pin_configure(data->cols[c],
				item_cfg->pin,
				col_gpio_dir | item_cfg->flags);
		if (err < 0) {
			LOG_ERR("Unable to configure column GPIO pin");
			return -EINVAL;
		}

#ifdef SCAN_ROW_TO_COLUMN
		irq_callbacks[c].work = &data->work;
		gpio_init_callback(&irq_callbacks[c].callback,
				kscan_gpio_irq_callback_handler,
				BIT(item_cfg->pin));
		gpio_add_callback(data->cols[c], &irq_callbacks[c].callback);
#endif
	}

	for (int r = 0; r < MATRIX_ROWS; r++) {
		const struct kscan_gpio_item_config *item_cfg = &cfg->rows[r];

		data->rows[r] = device_get_binding(item_cfg->label);
		if (data->rows[r] == NULL) {
			LOG_ERR("Unable to find row GPIO device\n");
			return -EINVAL;
		}

		int err = gpio_pin_configure(data->rows[r],
				item_cfg->pin,
				row_gpio_dir | item_cfg->flags);
		if (err) {
			LOG_ERR("Unable to configure row GPIO pin");
			return -EINVAL;
		}

#ifdef SCAN_COL_TO_ROW
		irq_callbacks[r].work = &data->work;
		gpio_init_callback(&irq_callbacks[r].callback,
				kscan_gpio_irq_callback_handler,
				BIT(item_cfg->pin));
		gpio_add_callback(data->rows[r], &irq_callbacks[r].callback);
#endif
	}

	data->dev = dev;

#if DT_PROP(MATRIX_NODE_ID, debounce_id) == 0
	k_work_init(&data->work, kscan_gpio_work_handler);
#else
	k_delayed_work_init(&data->work, kscan_gpio_work_handler);
#endif

	return 0;
}


static const struct kscan_driver_api gpio_driver_api = {
	.config = kscan_gpio_configure,
	.enable_callback = kscan_gpio_enable_interrupts,
	.disable_callback = kscan_gpio_disable_interrupts,
};

#define _KSCAN_GPIO_ITEM_CFG_INIT(prop, idx) \
	{ \
	  .label = DT_GPIO_LABEL_BY_IDX(MATRIX_NODE_ID, prop, idx), \
	  .pin = DT_GPIO_PIN_BY_IDX(MATRIX_NODE_ID, prop, idx), \
	  .flags = DT_GPIO_FLAGS_BY_IDX(MATRIX_NODE_ID, prop, idx), \
	}

static const struct kscan_gpio_config kscan_gpio_config = {
	.rows = {
#if DT_PROP_HAS_IDX(MATRIX_NODE_ID, row_gpios, 0)
		_KSCAN_GPIO_ITEM_CFG_INIT(row_gpios, 0),
#endif
#if DT_PROP_HAS_IDX(MATRIX_NODE_ID, row_gpios, 1)
		_KSCAN_GPIO_ITEM_CFG_INIT(row_gpios, 1),
#endif
#if DT_PROP_HAS_IDX(MATRIX_NODE_ID, row_gpios, 2)
		_KSCAN_GPIO_ITEM_CFG_INIT(row_gpios, 2),
#endif
#if DT_PROP_HAS_IDX(MATRIX_NODE_ID, row_gpios, 3)
		_KSCAN_GPIO_ITEM_CFG_INIT(row_gpios, 3),
#endif
#if DT_PROP_HAS_IDX(MATRIX_NODE_ID, row_gpios, 4)
		_KSCAN_GPIO_ITEM_CFG_INIT(row_gpios, 4),
#endif
#if DT_PROP_HAS_IDX(MATRIX_NODE_ID, row_gpios, 5)
		_KSCAN_GPIO_ITEM_CFG_INIT(row_gpios, 5),
#endif
#if DT_PROP_HAS_IDX(MATRIX_NODE_ID, row_gpios, 6)
		_KSCAN_GPIO_ITEM_CFG_INIT(row_gpios, 6),
#endif
#if DT_PROP_HAS_IDX(MATRIX_NODE_ID, row_gpios, 7)
		_KSCAN_GPIO_ITEM_CFG_INIT(row_gpios, 7),
#endif
#if DT_PROP_HAS_IDX(MATRIX_NODE_ID, row_gpios, 8)
		_KSCAN_GPIO_ITEM_CFG_INIT(row_gpios, 8),
#endif
#if DT_PROP_HAS_IDX(MATRIX_NODE_ID, row_gpios, 9)
		_KSCAN_GPIO_ITEM_CFG_INIT(row_gpios, 9),
#endif
#if DT_PROP_HAS_IDX(MATRIX_NODE_ID, row_gpios, 10)
		_KSCAN_GPIO_ITEM_CFG_INIT(row_gpios, 10),
#endif
	},
	.cols = {
#if DT_PROP_HAS_IDX(MATRIX_NODE_ID, col_gpios, 0)
		_KSCAN_GPIO_ITEM_CFG_INIT(col_gpios, 0),
#endif
#if DT_PROP_HAS_IDX(MATRIX_NODE_ID, col_gpios, 1)
		_KSCAN_GPIO_ITEM_CFG_INIT(col_gpios, 1),
#endif
#if DT_PROP_HAS_IDX(MATRIX_NODE_ID, col_gpios, 2)
		_KSCAN_GPIO_ITEM_CFG_INIT(col_gpios, 2),
#endif
#if DT_PROP_HAS_IDX(MATRIX_NODE_ID, col_gpios, 3)
		_KSCAN_GPIO_ITEM_CFG_INIT(col_gpios, 3),
#endif
#if DT_PROP_HAS_IDX(MATRIX_NODE_ID, col_gpios, 4)
		_KSCAN_GPIO_ITEM_CFG_INIT(col_gpios, 4),
#endif
#if DT_PROP_HAS_IDX(MATRIX_NODE_ID, col_gpios, 5)
		_KSCAN_GPIO_ITEM_CFG_INIT(col_gpios, 5),
#endif
#if DT_PROP_HAS_IDX(MATRIX_NODE_ID, col_gpios, 6)
		_KSCAN_GPIO_ITEM_CFG_INIT(col_gpios, 6),
#endif
#if DT_PROP_HAS_IDX(MATRIX_NODE_ID, col_gpios, 7)
		_KSCAN_GPIO_ITEM_CFG_INIT(col_gpios, 7),
#endif
#if DT_PROP_HAS_IDX(MATRIX_NODE_ID, col_gpios, 8)
		_KSCAN_GPIO_ITEM_CFG_INIT(col_gpios, 8),
#endif
#if DT_PROP_HAS_IDX(MATRIX_NODE_ID, col_gpios, 9)
		_KSCAN_GPIO_ITEM_CFG_INIT(col_gpios, 9),
#endif
#if DT_PROP_HAS_IDX(MATRIX_NODE_ID, col_gpios, 10)
		_KSCAN_GPIO_ITEM_CFG_INIT(col_gpios, 10),
#endif
#if DT_PROP_HAS_IDX(MATRIX_NODE_ID, col_gpios, 11)
		_KSCAN_GPIO_ITEM_CFG_INIT(col_gpios, 11),
#endif
#if DT_PROP_HAS_IDX(MATRIX_NODE_ID, col_gpios, 12)
		_KSCAN_GPIO_ITEM_CFG_INIT(col_gpios, 12),
#endif
#if DT_PROP_HAS_IDX(MATRIX_NODE_ID, col_gpios, 13)
		_KSCAN_GPIO_ITEM_CFG_INIT(col_gpios, 13),
#endif
#if DT_PROP_HAS_IDX(MATRIX_NODE_ID, col_gpios, 14)
		_KSCAN_GPIO_ITEM_CFG_INIT(col_gpios, 14),
#endif
#if DT_PROP_HAS_IDX(MATRIX_NODE_ID, col_gpios, 15)
		_KSCAN_GPIO_ITEM_CFG_INIT(col_gpios, 15),
#endif
#if DT_PROP_HAS_IDX(MATRIX_NODE_ID, col_gpios, 16)
		_KSCAN_GPIO_ITEM_CFG_INIT(col_gpios, 16),
#endif
#if DT_PROP_HAS_IDX(MATRIX_NODE_ID, col_gpios, 17)
		_KSCAN_GPIO_ITEM_CFG_INIT(col_gpios, 17),
#endif
#if DT_PROP_HAS_IDX(MATRIX_NODE_ID, col_gpios, 18)
		_KSCAN_GPIO_ITEM_CFG_INIT(col_gpios, 18),
#endif
#if DT_PROP_HAS_IDX(MATRIX_NODE_ID, col_gpios, 19)
		_KSCAN_GPIO_ITEM_CFG_INIT(col_gpios, 19),
#endif
	}
};

static struct kscan_gpio_data kscan_gpio_data;

DEVICE_AND_API_INIT(kscan_gpio, DT_INST_LABEL(0), kscan_gpio_init,
		    &kscan_gpio_data, &kscan_gpio_config,
		    POST_KERNEL, CONFIG_KSCAN_INIT_PRIORITY,
		    &gpio_driver_api);
