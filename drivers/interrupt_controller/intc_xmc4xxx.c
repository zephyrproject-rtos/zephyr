/*
 * Copyright (c) 2022 Schlumberger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_xmc4xxx_intc

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/interrupt-controller/infineon-xmc4xxx-intc.h>
#include <zephyr/irq.h>

#include <xmc_eru.h>

/* In Infineon XMC4XXX SoCs, gpio interrupts are triggered via an Event Request Unit (ERU) */
/* module. A subset of the GPIOs are connected to the ERU. The ERU monitors edge triggers */
/* and creates a SR. */

/* This driver configures the ERU for a target port/pin combination for rising/falling */
/* edge events. Note that the ERU module does not generate SR based on the gpio level. */
/* Internally the ERU tracks the *status* of an event. The status is set on a positive edge and */
/* unset on a negative edge (or vice-versa depending on the configuration). The value of */
/* the status is used to implement a level triggered interrupt; The ISR checks the status */
/* flag and calls the callback function if the status is set. */

/* The ERU configurations for supported port/pin combinations are stored in a devicetree file */
/* dts/arm/infineon/xmc4xxx_x_x-intc.dtsi. The configurations are stored in the opaque array */
/* uint16 port_line_mapping[]. The bitfields for the opaque entries are defined in */
/* dt-bindings/interrupt-controller/infineon-xmc4xxx-intc.h. */

struct isr_cb {
	/* if fn is NULL it implies the interrupt line has not been allocated */
	void (*fn)(const struct device *dev, int pin);
	void *data;
	enum gpio_int_mode mode;
	uint8_t port_id;
	uint8_t pin;
};

#define MAX_ISR_NUM 8
struct intc_xmc4xxx_data {
	struct isr_cb cb[MAX_ISR_NUM];
};

#define NUM_ERUS 2
struct intc_xmc4xxx_config {
	XMC_ERU_t *eru_regs[NUM_ERUS];
};

static const uint16_t port_line_mapping[DT_INST_PROP_LEN(0, port_line_mapping)] =
				DT_INST_PROP(0, port_line_mapping);

int intc_xmc4xxx_gpio_enable_interrupt(int port_id, int pin, enum gpio_int_mode mode,
				       enum gpio_int_trig trig,
				       void (*fn)(const struct device *, int), void *user_data)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	struct intc_xmc4xxx_data *data = dev->data;
	const struct intc_xmc4xxx_config *config = dev->config;
	int ret = -ENOTSUP;

	for (int i = 0; i < ARRAY_SIZE(port_line_mapping); i++) {
		XMC_ERU_ETL_CONFIG_t etl_config = {0};
		XMC_ERU_OGU_CONFIG_t isr_config = {0};
		XMC_ERU_ETL_EDGE_DETECTION_t trig_xmc;
		XMC_ERU_t *eru;
		int port_map, pin_map, line, eru_src, eru_ch;
		struct isr_cb *cb;

		port_map = XMC4XXX_INTC_GET_PORT(port_line_mapping[i]);
		pin_map  = XMC4XXX_INTC_GET_PIN(port_line_mapping[i]);

		if (port_map != port_id || pin_map != pin) {
			continue;
		}

		line = XMC4XXX_INTC_GET_LINE(port_line_mapping[i]);
		cb = &data->cb[line];
		if (cb->fn) {
			/* It's already used. Continue search for available line */
			/* with same port/pin */
			ret = -EBUSY;
			continue;
		}

		eru_src = XMC4XXX_INTC_GET_ERU_SRC(port_line_mapping[i]);
		eru_ch  = line & 0x3;

		if (trig == GPIO_INT_TRIG_HIGH) {
			trig_xmc = XMC_ERU_ETL_EDGE_DETECTION_RISING;
		} else if (trig == GPIO_INT_TRIG_LOW) {
			trig_xmc = XMC_ERU_ETL_EDGE_DETECTION_FALLING;
		} else if (trig == GPIO_INT_TRIG_BOTH) {
			trig_xmc = XMC_ERU_ETL_EDGE_DETECTION_BOTH;
		} else {
			return -EINVAL;
		}

		cb->port_id = port_id;
		cb->pin = pin;
		cb->mode = mode;
		cb->fn = fn;
		cb->data = user_data;

		/* setup the eru */
		etl_config.edge_detection = trig_xmc;
		etl_config.input_a = eru_src;
		etl_config.input_b = eru_src;
		etl_config.source = eru_src >> 2;
		etl_config.status_flag_mode = XMC_ERU_ETL_STATUS_FLAG_MODE_HWCTRL;
		etl_config.enable_output_trigger = 1;
		etl_config.output_trigger_channel = eru_ch;

		eru = config->eru_regs[line >> 2];

		XMC_ERU_ETL_Init(eru, eru_ch, &etl_config);

		isr_config.service_request = XMC_ERU_OGU_SERVICE_REQUEST_ON_TRIGGER;
		XMC_ERU_OGU_Init(eru, eru_ch, &isr_config);

		/* if the gpio level is already set then we must manually set the interrupt to */
		/* pending */
		if (mode == GPIO_INT_MODE_LEVEL) {
			ret = gpio_pin_get_raw(user_data, pin);
			if (ret < 0) {
				return ret;
			}
#define NVIC_ISPR_BASE 0xe000e200u
			if ((ret == 0 && trig == GPIO_INT_TRIG_LOW) ||
			    (ret == 1 && trig == GPIO_INT_TRIG_HIGH)) {
				eru->EXICON_b[eru_ch].FL = 1;
				/* put interrupt into pending state */
				*(uint32_t *)(NVIC_ISPR_BASE) |= BIT(line + 1);
			}
		}

		return 0;
	}
	return ret;
}

int intc_xmc4xxx_gpio_disable_interrupt(int port_id, int pin)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	const struct intc_xmc4xxx_config *config = dev->config;
	struct intc_xmc4xxx_data *data = dev->data;
	int eru_ch;

	for (int line = 0; line < ARRAY_SIZE(data->cb); line++) {
		struct isr_cb *cb;

		cb = &data->cb[line];
		eru_ch = line & 0x3;
		if (cb->fn && cb->port_id == port_id && cb->pin == pin) {
			XMC_ERU_t *eru = config->eru_regs[line >> 2];

			cb->fn = NULL;
			/* disable the SR */
			eru->EXICON_b[eru_ch].PE = 0;
			/* unset the status flag */
			eru->EXICON_b[eru_ch].FL = 0;
			/* no need to clear other variables in cb*/
			return 0;
		}
	}
	return -EINVAL;
}

static void intc_xmc4xxx_isr(void *arg)
{
	int line = (int)arg;
	const struct device *dev = DEVICE_DT_INST_GET(0);
	struct intc_xmc4xxx_data *data = dev->data;
	const struct intc_xmc4xxx_config *config = dev->config;
	struct isr_cb *cb = &data->cb[line];
	XMC_ERU_t *eru = config->eru_regs[line >> 2];
	int eru_ch = line & 0x3;

	/* The callback function may actually disable the interrupt and set cb->fn = NULL */
	/* as is done in tests/drivers/gpio/gpio_api_1pin. Assume that the callback function */
	/* will NOT disable the interrupt and then enable another port/pin */
	/* in the same callback which could potentially set cb->fn again. */
	while (cb->fn) {
		cb->fn(cb->data, cb->pin);
		/* for level triggered interrupts we have to manually check the status. */
		if (cb->mode == GPIO_INT_MODE_LEVEL && eru->EXICON_b[eru_ch].FL == 1) {
			continue;
		}
		/* break for edge triggered interrupts */
		break;
	}
}

#define INTC_IRQ_CONNECT_ENABLE(name, line_number)                                                \
	COND_CODE_1(DT_INST_IRQ_HAS_NAME(0, name),                                                \
	(IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, name, irq),                                           \
		DT_INST_IRQ_BY_NAME(0, name, priority), intc_xmc4xxx_isr, (void *)line_number, 0); \
		irq_enable(DT_INST_IRQ_BY_NAME(0, name, irq));), ())

static int intc_xmc4xxx_init(const struct device *dev)
{
	/* connect irqs only if they defined by name in the dts */
	INTC_IRQ_CONNECT_ENABLE(eru0sr0, 0);
	INTC_IRQ_CONNECT_ENABLE(eru0sr1, 1);
	INTC_IRQ_CONNECT_ENABLE(eru0sr2, 2);
	INTC_IRQ_CONNECT_ENABLE(eru0sr3, 3);
	INTC_IRQ_CONNECT_ENABLE(eru1sr0, 4);
	INTC_IRQ_CONNECT_ENABLE(eru1sr1, 5);
	INTC_IRQ_CONNECT_ENABLE(eru1sr2, 6);
	INTC_IRQ_CONNECT_ENABLE(eru1sr3, 7);
	return 0;
}

struct intc_xmc4xxx_data intc_xmc4xxx_data0;

struct intc_xmc4xxx_config intc_xmc4xxx_config0 = {
	.eru_regs = {
		(XMC_ERU_t *)DT_INST_REG_ADDR_BY_NAME(0, eru0),
		(XMC_ERU_t *)DT_INST_REG_ADDR_BY_NAME(0, eru1),
	},
};

DEVICE_DT_INST_DEFINE(0, intc_xmc4xxx_init, NULL,
		&intc_xmc4xxx_data0, &intc_xmc4xxx_config0, PRE_KERNEL_1,
		CONFIG_INTC_INIT_PRIORITY, NULL);
