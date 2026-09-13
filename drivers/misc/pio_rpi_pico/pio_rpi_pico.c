/*
 * Copyright (c) 2023 Tokita, Hiroshi <tokita.hiroshi@fujitsu.com>
 * Copyright (c) 2023 Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/misc/pio_rpi_pico/pio_rpi_pico.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/__assert.h>

#define DT_DRV_COMPAT raspberrypi_pico_pio

struct pio_rpi_pico_config {
	/* Must stay first, pio_rpi_pico_get_pio() casts the config to a PIO */
	PIO pio;
	const struct device *clk_dev;
	clock_control_subsys_t clk_id;
	const struct reset_dt_spec reset;
#ifdef CONFIG_PIO_RPI_PICO_IRQ
	void (*irq_config_func)(void);
#endif
};

int pio_rpi_pico_allocate_sm(const struct device *dev, size_t *sm)
{
	const struct pio_rpi_pico_config *config = dev->config;
	int retval;

	retval = pio_claim_unused_sm(config->pio, false);
	if (retval < 0) {
		return -EBUSY;
	}

	*sm = (size_t)retval;
	return 0;
}

#ifdef CONFIG_PIO_RPI_PICO_IRQ

BUILD_ASSERT(NUM_PIO_IRQS == 2, "The driver expects a PIO block to have two interrupt lines");

struct pio_rpi_pico_irq_handler {
	pio_rpi_pico_irq_cb_t cb;
	void *user_data;
	uint32_t source_mask;
};

struct pio_rpi_pico_data {
	struct pio_rpi_pico_irq_handler handlers[NUM_PIO_IRQS]
					       [CONFIG_PIO_RPI_PICO_MAX_IRQ_HANDLERS];
	struct k_spinlock lock;
};

int pio_rpi_pico_register_irq(const struct device *dev, uint8_t irq_index, uint32_t source_mask,
			      pio_rpi_pico_irq_cb_t cb, void *user_data)
{
	struct pio_rpi_pico_data *data = dev->data;
	k_spinlock_key_t key;
	int retval = -ENOMEM;

	if (irq_index >= NUM_PIO_IRQS || source_mask == 0 || cb == NULL) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	for (size_t i = 0; i < ARRAY_SIZE(data->handlers[irq_index]); i++) {
		struct pio_rpi_pico_irq_handler *handler = &data->handlers[irq_index][i];

		if (handler->cb != NULL) {
			continue;
		}

		handler->user_data = user_data;
		handler->source_mask = source_mask;
		/*
		 * Published last, the ISR takes a non-NULL callback as the
		 * marker that the rest of the entry is valid.
		 */
		compiler_barrier();
		handler->cb = cb;
		retval = 0;
		break;
	}

	k_spin_unlock(&data->lock, key);

	return retval;
}

void pio_rpi_pico_irq_sources_set(const struct device *dev, uint8_t irq_index,
				  uint32_t source_mask, bool enable)
{
	const struct pio_rpi_pico_config *config = dev->config;

	__ASSERT(irq_index < NUM_PIO_IRQS, "irq_index %u out of range", irq_index);
	if (irq_index >= NUM_PIO_IRQS) {
		return;
	}

	/*
	 * Writes through the atomic set/clear register aliases, so children
	 * sharing the block cannot clobber each other's sources.
	 */
	pio_set_irqn_source_mask_enabled(config->pio, irq_index, source_mask, enable);
}

static void pio_rpi_pico_isr(const struct device *dev, uint8_t irq_index)
{
	const struct pio_rpi_pico_config *config = dev->config;
	struct pio_rpi_pico_data *data = dev->data;
	uint32_t ints = config->pio->irq_ctrl[irq_index].ints;

	for (size_t i = 0; i < ARRAY_SIZE(data->handlers[irq_index]); i++) {
		struct pio_rpi_pico_irq_handler *handler = &data->handlers[irq_index][i];
		pio_rpi_pico_irq_cb_t cb = handler->cb;

		if (cb != NULL && (handler->source_mask & ints) != 0) {
			cb(dev, ints, handler->user_data);
		}
	}
}

#endif /* CONFIG_PIO_RPI_PICO_IRQ */

static int pio_rpi_pico_init(const struct device *dev)
{
	const struct pio_rpi_pico_config *config = dev->config;
	int ret;

	ret = clock_control_on(config->clk_dev, config->clk_id);
	if (ret < 0) {
		return ret;
	}

	ret = reset_line_toggle_dt(&config->reset);
	if (ret < 0) {
		return ret;
	}

#ifdef CONFIG_PIO_RPI_PICO_IRQ
	/*
	 * The reset above masks every source out of INTE, so the lines can be
	 * enabled here and stay quiet until a child unmasks a source.
	 */
	config->irq_config_func();
#endif

	return 0;
}

#ifdef CONFIG_PIO_RPI_PICO_IRQ

#define PIO_RPI_PICO_ISR_DEFINE(idx, irq_index)                                                    \
	static void pio_rpi_pico_isr_##idx##_##irq_index(const struct device *dev)                 \
	{                                                                                          \
		pio_rpi_pico_isr(dev, irq_index);                                                  \
	}

#define PIO_RPI_PICO_IRQ_CONNECT(idx, name, irq_index)                                             \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(idx, name, irq),                                           \
		    DT_INST_IRQ_BY_NAME(idx, name, priority),                                      \
		    pio_rpi_pico_isr_##idx##_##irq_index,                                          \
		    DEVICE_DT_INST_GET(idx), 0);                                                   \
	irq_enable(DT_INST_IRQ_BY_NAME(idx, name, irq));

#define PIO_RPI_PICO_IRQ_DEFINE(idx)                                                               \
	PIO_RPI_PICO_ISR_DEFINE(idx, 0)                                                            \
	PIO_RPI_PICO_ISR_DEFINE(idx, 1)                                                            \
                                                                                                   \
	static void pio_rpi_pico_irq_config_##idx(void)                                            \
	{                                                                                          \
		PIO_RPI_PICO_IRQ_CONNECT(idx, irq0, 0)                                             \
		PIO_RPI_PICO_IRQ_CONNECT(idx, irq1, 1)                                             \
	}                                                                                          \
                                                                                                   \
	static struct pio_rpi_pico_data pio_rpi_pico_data_##idx;

#define PIO_RPI_PICO_IRQ_CONFIG_INIT(idx) .irq_config_func = pio_rpi_pico_irq_config_##idx,
#define PIO_RPI_PICO_DATA_GET(idx)        &pio_rpi_pico_data_##idx

#else /* CONFIG_PIO_RPI_PICO_IRQ */

#define PIO_RPI_PICO_IRQ_DEFINE(idx)
#define PIO_RPI_PICO_IRQ_CONFIG_INIT(idx)
#define PIO_RPI_PICO_DATA_GET(idx) NULL

#endif /* CONFIG_PIO_RPI_PICO_IRQ */

#define RPI_PICO_PIO_INIT(idx)                                                                     \
	PIO_RPI_PICO_IRQ_DEFINE(idx)                                                               \
                                                                                                   \
	static const struct pio_rpi_pico_config pio_rpi_pico_config_##idx = {                      \
		.pio = (PIO)DT_INST_REG_ADDR(idx),                                                 \
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),                                \
		.clk_id = (clock_control_subsys_t)DT_INST_PHA_BY_IDX(0, clocks, 0, clk_id),        \
		.reset = RESET_DT_SPEC_INST_GET(idx),                                              \
		PIO_RPI_PICO_IRQ_CONFIG_INIT(idx)                                                  \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, &pio_rpi_pico_init, NULL, PIO_RPI_PICO_DATA_GET(idx),           \
			      &pio_rpi_pico_config_##idx, PRE_KERNEL_2,                            \
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

DT_INST_FOREACH_STATUS_OKAY(RPI_PICO_PIO_INIT)
