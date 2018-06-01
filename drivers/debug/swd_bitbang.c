/*
 * Copyright (c) 2018-2019 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file is based on SW_DP.c from DAPLink Interface Firmware.
 * Copyright (c) 2009-2016, ARM Limited, All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */


/* Serial Wire Interface bit-bang driver */

#define DT_DRV_COMPAT dap_sw_gpio

#include <zephyr.h>
#include <drivers/gpio.h>
#include <drivers/debug/swd.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(swd, CONFIG_SWD_DRIVER_LOG_LEVEL);

#define DP0_CLK_GPIOS_PORT	DT_INST_GPIO_LABEL(0, clk_gpios)
#define DP0_CLK_GPIOS_PIN	DT_INST_GPIO_PIN(0, clk_gpios)
#define DP0_DOUT_GPIOS_PORT	DT_INST_GPIO_LABEL(0, dout_gpios)
#define DP0_DOUT_GPIOS_PIN	DT_INST_GPIO_PIN(0, dout_gpios)
#define DP0_DIN_GPIOS_PORT	DT_INST_GPIO_LABEL(0, din_gpios)
#define DP0_DIN_GPIOS_PIN	DT_INST_GPIO_PIN(0, din_gpios)
#define DP0_DNOE_GPIOS_PORT	DT_INST_GPIO_LABEL(0, dnoe_gpios)
#define DP0_DNOE_GPIOS_PIN	DT_INST_GPIO_PIN(0, dnoe_gpios)
#define DP0_NOE_GPIOS_PORT	DT_INST_GPIO_LABEL(0, noe_gpios)
#define DP0_NOE_GPIOS_PIN	DT_INST_GPIO_PIN(0, noe_gpios)
#define DP0_RESET_GPIOS_PORT	DT_INST_GPIO_LABEL(0, reset_gpios)
#define DP0_RESET_GPIOS_PIN	DT_INST_GPIO_PIN(0, reset_gpios)

/* Number of processor cycles for GPIO port write operations. */
#define GPIO_PORT_WRITE_CYCLES	DT_INST_PROP(0, port_write_cycles)

#if defined(CONFIG_SOC_SERIES_NRF52X)
#define CPU_CLOCK		64000000U
#else
#define CPU_CLOCK		CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
#endif

#define CLOCK_DELAY(swclk_freq) \
	((CPU_CLOCK / 2 / swclk_freq) - GPIO_PORT_WRITE_CYCLES)

/*
 * Default SWCLK frequency in Hz.
 * sw_clock can be used to overwrite this default value.
 */
#define SWD_DEFAULT_SWCLK_FREQUENCY	1000000U

#define DELAY_SLOW_CYCLES		3U

struct sw_config {
	const struct device *swclk;
	const struct device *swdio_out;
	const struct device *swdio_in;
	const struct device *swdio_noe;
	const struct device *swd_noe;
	const struct device *reset;

	uint32_t clock_delay;
	uint8_t turnaround;
	bool data_phase;
	bool fast_clock;
};

static struct sw_config sw_cfg;

static uint8_t sw_request_lut[16] = {0U};

static void mk_sw_request_lut(void)
{
	uint32_t parity = 0U;

	for (int request = 0; request < sizeof(sw_request_lut); request++) {
		parity = request;
		parity ^= parity >> 2;
		parity ^= parity >> 1;

		/*
		 * Move A[3:3], RnW, APnDP bits to their position,
		 * add start bit, stop bit(6), and park bit.
		 */
		sw_request_lut[request] =  BIT(7) | (request << 1) | BIT(0);
		/* Add parity bit */
		if (parity & 0x01U) {
			sw_request_lut[request] |= BIT(5);
		}
	}

	LOG_HEXDUMP_DBG(sw_request_lut, sizeof(sw_request_lut), "request lut");
}

static uint32_t ALWAYS_INLINE sw_get32bit_parity(uint32_t data)
{
	data ^= data >> 16;
	data ^= data >> 8;
	data ^= data >> 4;
	data ^= data >> 2;
	data ^= data >> 1;
	return data & 1U;
}


static ALWAYS_INLINE void pin_delay_asm(uint32_t delay)
{
	__asm volatile ("movs r3, %[p]\n"
			".start_%=:\n"
			"subs r3, #1\n"
			"bne .start_%=\n"
			:
			: [p] "r" (delay)
			: "r3", "cc"
			);
}

#if defined(CONFIG_SOC_SERIES_NRF52X)

/*
 * Set SWCLK DAP hardware output pin to high level.
 */
static ALWAYS_INLINE void pin_swclk_set(void)
{
#if DT_REG_ADDR(DT_PHANDLE(DT_DRV_INST(0), clk_gpios)) == NRF_P0_BASE
	NRF_P0->OUTSET = BIT(DP0_CLK_GPIOS_PIN);
#elif DT_REG_ADDR(DT_PHANDLE(DT_DRV_INST(0), clk_gpios)) == NRF_P1_BASE
	NRF_P1->OUTSET = BIT(DP0_CLK_GPIOS_PIN);
#else
#error "Not defined for this SoC family"
#endif
}

/*
 * Set SWCLK DAP hardware output pin to low level.
 */
static ALWAYS_INLINE void pin_swclk_clr(void)
{
#if DT_REG_ADDR(DT_PHANDLE(DT_DRV_INST(0), clk_gpios)) == NRF_P0_BASE
	NRF_P0->OUTCLR = BIT(DP0_CLK_GPIOS_PIN);
#elif DT_REG_ADDR(DT_PHANDLE(DT_DRV_INST(0), clk_gpios)) == NRF_P1_BASE
	NRF_P1->OUTCLR = BIT(DP0_CLK_GPIOS_PIN);
#else
#error "Not defined for this SoC family"
#endif
}

/*
 * Set the SWDIO DAP hardware output pin to high level.
 */
static ALWAYS_INLINE void pin_swdio_set(void)
{
#if DT_REG_ADDR(DT_PHANDLE(DT_DRV_INST(0), dout_gpios)) == NRF_P0_BASE
	NRF_P0->OUTSET = BIT(DP0_DOUT_GPIOS_PIN);
#elif DT_REG_ADDR(DT_PHANDLE(DT_DRV_INST(0), dout_gpios)) == NRF_P1_BASE
	NRF_P1->OUTSET = BIT(DP0_DOUT_GPIOS_PIN);
#else
#error "Not defined for this SoC family"
#endif
}

/*
 * Set the SWDIO DAP hardware output pin to low level.
 */
static ALWAYS_INLINE void pin_swdio_clr(void)
{
#if DT_REG_ADDR(DT_PHANDLE(DT_DRV_INST(0), dout_gpios)) == NRF_P0_BASE
	NRF_P0->OUTCLR = BIT(DP0_DOUT_GPIOS_PIN);
#elif DT_REG_ADDR(DT_PHANDLE(DT_DRV_INST(0), dout_gpios)) == NRF_P1_BASE
	NRF_P1->OUTCLR = BIT(DP0_DOUT_GPIOS_PIN);
#else
#error "Not defined for this SoC family"
#endif
}

/*
 * Set the SWDIO DAP hardware output pin to bit level.
 */
static ALWAYS_INLINE void pin_swdio_out(uint32_t bit)
{
	if (bit & 1U) {
		NRF_P1->OUTSET = BIT(DP0_DOUT_GPIOS_PIN);
	} else {
		NRF_P1->OUTCLR = BIT(DP0_DOUT_GPIOS_PIN);
	}
}

/*
 * Return current level of the SWDIO DAP hardware input pin.
 */
static ALWAYS_INLINE uint32_t pin_swdio_in(void)
{
#if DT_REG_ADDR(DT_PHANDLE(DT_DRV_INST(0), din_gpios)) == NRF_P0_BASE
	return ((NRF_P0->IN >> DP0_DIN_GPIOS_PIN) & 1);
#elif DT_REG_ADDR(DT_PHANDLE(DT_DRV_INST(0), din_gpios)) == NRF_P1_BASE
	return ((NRF_P1->IN >> DP0_DIN_GPIOS_PIN) & 1);
#else
#error "Not defined for this SoC family"
#endif
}

/*
 * Configure the SWDIO DAP hardware to output mode.
 * This is default configuration for every transfer.
 */
static ALWAYS_INLINE void pin_swdio_out_enable(void)
{
#if DT_REG_ADDR(DT_PHANDLE(DT_DRV_INST(0), dnoe_gpios)) == NRF_P0_BASE
	NRF_P0->OUTSET = BIT(DP0_DNOE_GPIOS_PIN);
#elif DT_REG_ADDR(DT_PHANDLE(DT_DRV_INST(0), dnoe_gpios)) == NRF_P1_BASE
	NRF_P1->OUTSET = BIT(DP0_DNOE_GPIOS_PIN);
#else
#error "Not defined for this SoC family"
#endif
}

/*
 * Configure the SWDIO DAP hardware to input mode.
 */
static ALWAYS_INLINE void pin_swdio_out_disable(void)
{
#if DT_REG_ADDR(DT_PHANDLE(DT_DRV_INST(0), dnoe_gpios)) == NRF_P0_BASE
	NRF_P0->OUTCLR = BIT(DP0_DNOE_GPIOS_PIN);
#elif DT_REG_ADDR(DT_PHANDLE(DT_DRV_INST(0), dnoe_gpios)) == NRF_P1_BASE
	NRF_P1->OUTCLR = BIT(DP0_DNOE_GPIOS_PIN);
#else
#error "Not defined for this SoC family"
#endif
}

#else
#error "Not implemented for this SoC family"
#endif

#define SW_CLOCK_CYCLE()				\
	do {						\
		pin_swclk_clr();			\
		pin_delay_asm(sw_cfg.clock_delay);	\
		pin_swclk_set();			\
		pin_delay_asm(sw_cfg.clock_delay);	\
	} while (0)

#define SW_WRITE_BIT(bit)				\
	do {						\
		pin_swdio_out(bit);			\
		pin_swclk_clr();			\
		pin_delay_asm(sw_cfg.clock_delay);	\
		pin_swclk_set();			\
		pin_delay_asm(sw_cfg.clock_delay);	\
	} while (0)

#define SW_READ_BIT(bit)				\
	do {						\
		pin_swclk_clr();			\
		pin_delay_asm(sw_cfg.clock_delay);	\
		bit = pin_swdio_in();			\
		pin_swclk_set();			\
		pin_delay_asm(sw_cfg.clock_delay);	\
	} while (0)

static int sw_sequence(const struct device *dev, uint32_t count,
		       const uint8_t *data)
{
	uint32_t val = 0;
	uint32_t n = 0;
	unsigned int key;

	LOG_DBG("count %u", count);
	LOG_HEXDUMP_DBG(data, count, "sequence bit data");
	key = irq_lock();

	while (count--) {
		if (n == 0U) {
			val = *data++;
			n = 8U;
		}
		if (val & 1U) {
			pin_swdio_set();
		} else {
			pin_swdio_clr();
		}
		SW_CLOCK_CYCLE();
		val >>= 1;
		n--;
	}

	irq_unlock(key);

	return 0;
}

static ALWAYS_INLINE void _clock_cycle_turnaround(void)
{
	uint32_t n;

	for (n = sw_cfg.turnaround; n; n--) {
		SW_CLOCK_CYCLE();
	}
}

static int sw_transfer(const struct device *dev,
		       uint8_t request, uint32_t *data,
		       uint8_t idle_cycles, uint8_t *response)
{
	unsigned int key;
	uint32_t ack;
	uint32_t bit;
	uint32_t val;
	uint32_t parity = 0;
	uint32_t n;

	LOG_DBG("request 0x%02x idle %u", request, idle_cycles);
	if (!(request & SWD_REQUEST_RnW)) {
		LOG_DBG("write data 0x%08x", *data);
		parity = sw_get32bit_parity(*data);
	}

	key = irq_lock();

	val = sw_request_lut[request & 0xFU];
	for (n = 8U; n; n--) {
		SW_WRITE_BIT(val);
		val >>= 1;
	}

	pin_swdio_out_disable();
	_clock_cycle_turnaround();

	/* Acknowledge response */
	SW_READ_BIT(bit);
	ack = bit << 0;
	SW_READ_BIT(bit);
	ack |= bit << 1;
	SW_READ_BIT(bit);
	ack |= bit << 2;

	if (ack == SWD_ACK_OK) {
		/* Data transfer */
		if (request & SWD_REQUEST_RnW) {
			/* Read data */
			val = 0U;
			for (n = 32U; n; n--) {
				/* Read RDATA[0:31] */
				SW_READ_BIT(bit);
				val >>= 1;
				val |= bit << 31;
			}

			/* Read parity bit */
			SW_READ_BIT(bit);
			_clock_cycle_turnaround();
			pin_swdio_out_enable();

			if ((sw_get32bit_parity(val) ^ bit) & 1U) {
				ack = SWD_TRANSFER_ERROR;
			}

			if (data) {
				*data = val;
			}

		} else {
			_clock_cycle_turnaround();

			pin_swdio_out_enable();
			/* Write data */
			val = *data;
			for (n = 32U; n; n--) {
				SW_WRITE_BIT(val);
				val >>= 1;
			}

			/* Write parity bit */
			SW_WRITE_BIT(parity);
		}
		/* Idle cycles */
		n = idle_cycles;
		if (n) {
			pin_swdio_out(0U);
			for (; n; n--) {
				SW_CLOCK_CYCLE();
			}
		}

		pin_swdio_out(1U);
		irq_unlock(key);
		if (request & SWD_REQUEST_RnW) {
			LOG_DBG("read data 0x%08x", *data);
		}

		if (response) {
			*response = (uint8_t)ack;
		}

		return 0;
	}

	if ((ack == SWD_ACK_WAIT) || (ack == SWD_ACK_FAULT)) {
		/* WAIT OR fault response */
		if (sw_cfg.data_phase) {
			for (n = 32U + 1U + sw_cfg.turnaround; n; n--) {
				/* Dummy Read RDATA[0:31] + Parity */
				SW_CLOCK_CYCLE();
			}
		} else {
			_clock_cycle_turnaround();
		}

		pin_swdio_out_enable();
		pin_swdio_out(1U);
		irq_unlock(key);
		LOG_DBG("Transfer wait or fault");
		if (response) {
			*response = (uint8_t)ack;
		}

		return 0;
	}

	/* Protocol error */
	for (n = sw_cfg.turnaround + 32U + 1U; n; n--) {
		/* Back off data phase */
		SW_CLOCK_CYCLE();
	}

	pin_swdio_out_enable();
	pin_swdio_out(1U);
	irq_unlock(key);
	LOG_INF("Protocol error");
	if (response) {
		*response = (uint8_t)ack;
	}

	return 0;
}

static int sw_set_pins(const struct device *dev, uint8_t pins, uint8_t value)
{
	LOG_DBG("pins 0x%02x value 0x%02x", pins, value);

	if (pins & BIT(DAP_SW_SWCLK_PIN)) {
		if (value & BIT(DAP_SW_SWCLK_PIN)) {
			gpio_pin_set(sw_cfg.swclk, DP0_CLK_GPIOS_PIN, 1);
		} else {
			gpio_pin_set(sw_cfg.swclk, DP0_CLK_GPIOS_PIN, 0);
		}
	}

	if (pins & BIT(DAP_SW_SWDIO_PIN)) {
		if (value & BIT(DAP_SW_SWDIO_PIN)) {
			gpio_pin_set(sw_cfg.swdio_out, DP0_DOUT_GPIOS_PIN, 1);
		} else {
			gpio_pin_set(sw_cfg.swdio_out, DP0_DOUT_GPIOS_PIN, 0);
		}
	}

	if (pins & BIT(DAP_SW_nRESET_PIN)) {
		if (value & BIT(DAP_SW_nRESET_PIN)) {
			gpio_pin_set(sw_cfg.reset, DP0_RESET_GPIOS_PIN, 1);
		} else {
			gpio_pin_set(sw_cfg.reset, DP0_RESET_GPIOS_PIN, 0);
		}
	}

	return 0;
}

static int sw_get_pins(const struct device *dev, uint8_t *state)
{
	uint32_t val;

	val = gpio_pin_get(sw_cfg.reset, DP0_RESET_GPIOS_PIN);
	*state = val ? BIT(DAP_SW_nRESET_PIN) : 0;

	val = gpio_pin_get(sw_cfg.swdio_in, DP0_DIN_GPIOS_PIN);
	*state |= val ? BIT(DAP_SW_SWDIO_PIN) : 0;

	val = gpio_pin_get(sw_cfg.swclk, DP0_CLK_GPIOS_PIN);
	*state |= val ? BIT(DAP_SW_SWCLK_PIN) : 0;

	LOG_DBG("pins state 0x%02x", *state);

	return 0;
}

static int sw_set_clock(const struct device *dev, uint32_t clock)
{
	uint32_t delay;

	sw_cfg.fast_clock = false;
	delay = ((CPU_CLOCK / 2U) + (clock - 1U)) / clock;

	if (delay > GPIO_PORT_WRITE_CYCLES) {
		delay -= GPIO_PORT_WRITE_CYCLES;
		delay = (delay + (DELAY_SLOW_CYCLES - 1U)) / DELAY_SLOW_CYCLES;
	} else {
		delay = 1U;
	}

	sw_cfg.clock_delay = delay;

	LOG_WRN("cpu_clock %d, delay %d", CPU_CLOCK, sw_cfg.clock_delay);

	return 0;
}

static int sw_configure(const struct device *dev,
			uint8_t turnaround, bool data_phase)
{
	sw_cfg.turnaround = turnaround;
	sw_cfg.data_phase = data_phase;

	LOG_INF("turnaround %d, data_phase %d",
		sw_cfg.turnaround, sw_cfg.data_phase);

	return 0;
}

static int sw_port_on(const struct device *dev)
{
	LOG_DBG("");
	gpio_pin_set(sw_cfg.swclk, DP0_CLK_GPIOS_PIN, 1);
	gpio_pin_set(sw_cfg.swdio_out, DP0_DOUT_GPIOS_PIN, 1);
	gpio_pin_set(sw_cfg.swdio_noe, DP0_DNOE_GPIOS_PIN, 1);
	gpio_pin_set(sw_cfg.swd_noe, DP0_NOE_GPIOS_PIN, 1);
	gpio_pin_set(sw_cfg.reset, DP0_RESET_GPIOS_PIN, 1);

	return 0;
}

static int sw_port_off(const struct device *dev)
{
	gpio_pin_set(sw_cfg.swdio_noe, DP0_DNOE_GPIOS_PIN, 0);
	gpio_pin_set(sw_cfg.swd_noe, DP0_NOE_GPIOS_PIN, 0);
	gpio_pin_set(sw_cfg.reset, DP0_RESET_GPIOS_PIN, 1);

	return 0;
}

static int sw_gpio_init(const struct device *dev)
{
	LOG_DBG("");

	/* Configure GPIO pin SWCLK */
	sw_cfg.swclk = device_get_binding(DP0_CLK_GPIOS_PORT);

	if (sw_cfg.swclk == NULL) {
		LOG_ERR("Failed to get pointer to %s device!",
			DP0_CLK_GPIOS_PORT);
		return -EINVAL;
	}

	LOG_DBG("GPIO port label %s, reg0 %x",
		DT_PROP(DT_PHANDLE(DT_DRV_INST(0), clk_gpios), label),
		DT_REG_ADDR(DT_PHANDLE(DT_DRV_INST(0), clk_gpios)));

	gpio_pin_configure(sw_cfg.swclk, DP0_CLK_GPIOS_PIN,
			   GPIO_OUTPUT_ACTIVE | GPIO_ACTIVE_HIGH |
			   GPIO_DS_DFLT_HIGH);

	/* Configure GPIO pin SWDIO_OUT */
	sw_cfg.swdio_out = device_get_binding(DP0_DOUT_GPIOS_PORT);
	if (sw_cfg.swdio_out == NULL) {
		LOG_ERR("Failed to get pointer to %s device!",
			DP0_DOUT_GPIOS_PORT);
		return -EINVAL;
	}

	gpio_pin_configure(sw_cfg.swdio_out, DP0_DOUT_GPIOS_PIN,
			   GPIO_OUTPUT_ACTIVE | GPIO_ACTIVE_HIGH |
			   GPIO_DS_DFLT_HIGH);

	/* Configure GPIO pin SWDIO_IN */
	sw_cfg.swdio_in = device_get_binding(DP0_DIN_GPIOS_PORT);
	if (sw_cfg.swdio_in == NULL) {
		LOG_ERR("Failed to get pointer to %s device!",
			DP0_DIN_GPIOS_PORT);
		return -EINVAL;
	}

	gpio_pin_configure(sw_cfg.swdio_in, DP0_DIN_GPIOS_PIN,
			   GPIO_INPUT | GPIO_PULL_UP);

	/* Configure GPIO pin SWDIO_NOE */
	sw_cfg.swdio_noe = device_get_binding(DP0_DNOE_GPIOS_PORT);
	if (sw_cfg.swdio_noe == NULL) {
		LOG_ERR("Failed to get pointer to %s device!",
			DP0_DNOE_GPIOS_PORT);
		return -EINVAL;
	}

	gpio_pin_configure(sw_cfg.swdio_noe, DP0_DNOE_GPIOS_PIN,
			   GPIO_OUTPUT_INACTIVE | GPIO_ACTIVE_HIGH |
			   GPIO_DS_DFLT_HIGH);

	/* Configure GPIO pin SWD_NOE */
	sw_cfg.swd_noe = device_get_binding(DP0_NOE_GPIOS_PORT);
	if (sw_cfg.swd_noe == NULL) {
		LOG_ERR("Failed to get pointer to %s device!",
			DP0_NOE_GPIOS_PORT);
		return -EINVAL;
	}

	gpio_pin_configure(sw_cfg.swd_noe, DP0_NOE_GPIOS_PIN,
			   GPIO_OUTPUT_INACTIVE | GPIO_ACTIVE_HIGH |
			   GPIO_DS_DFLT_HIGH);

	/* Configure GPIO pin RESET */
	sw_cfg.reset = device_get_binding(DP0_RESET_GPIOS_PORT);
	if (sw_cfg.reset == NULL) {
		LOG_ERR("Failed to get pointer to %s device!",
			DP0_RESET_GPIOS_PORT);
		return -EINVAL;
	}

	gpio_pin_configure(sw_cfg.reset, DP0_RESET_GPIOS_PIN,
			   GPIO_OUTPUT_ACTIVE | GPIO_ACTIVE_HIGH |
			   GPIO_PULL_UP);

	sw_cfg.turnaround = 1U;
	sw_cfg.data_phase = false;
	sw_cfg.fast_clock = false;
	sw_cfg.clock_delay = CLOCK_DELAY(SWD_DEFAULT_SWCLK_FREQUENCY);
	mk_sw_request_lut();

	return 0;
}

struct swd_api swd_bitbang_api = {
	.sw_sequence	= sw_sequence,
	.sw_transfer	= sw_transfer,
	.sw_set_pins	= sw_set_pins,
	.sw_get_pins	= sw_get_pins,
	.sw_set_clock	= sw_set_clock,
	.sw_configure	= sw_configure,
	.sw_port_on	= sw_port_on,
	.sw_port_off	= sw_port_off,
};


DEVICE_AND_API_INIT(sw_dp_gpio, DT_INST_LABEL(0),
		    sw_gpio_init, &sw_cfg, NULL,
		    POST_KERNEL, CONFIG_SWD_DRIVER_INIT_PRIO,
		    &swd_bitbang_api);
