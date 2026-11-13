/*
 * Copyright (c) 2026 Embeint Inc
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/lora/sx126x_emul.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "sx126x_hal.h"
#include "sx126x_regs.h"

LOG_MODULE_REGISTER(sx126x_emul, CONFIG_LORA_LOG_LEVEL);

#define EMUL_NUM_REGS 0x1000U
/* ReadRegister header (4 bytes) plus a full buffer */
#define EMUL_XFER_MAX (4U + SX126X_BUFFER_SIZE)

#define EMUL_SPI_MAX_FREQ        16000000U
#define EMUL_RX_DELAY            K_MSEC(1)
#define EMUL_TX_DURATION_DEFAULT K_MSEC(1)

/* Status byte: chip mode in bits 6:4, command status in bits 3:1 */
#define EMUL_STATUS_MODE_SHIFT 4
#define EMUL_STATUS_CMD_SHIFT  1

#define EMUL_FREQ_MIN_HZ   150000000U
#define EMUL_FREQ_MAX_HZ   960000000U
/* CalibrateImage takes its band edges in 4 MHz steps */
#define EMUL_IMAGE_STEP_HZ 4000000U
/* Image calibration the radio runs at power on: 902 to 928 MHz */
#define EMUL_IMAGE_CAL_POR_LOW  0xE1
#define EMUL_IMAGE_CAL_POR_HIGH 0xE9

/* Symbol time above which low data rate optimization is mandated */
#define EMUL_LDRO_SYMBOL_US 16380U
#define EMUL_SF56_MIN_PREAMBLE 12U

#define EMUL_LORA_MOD_PARAMS 4U
#define EMUL_LORA_PKT_PARAMS 6U
#define EMUL_GFSK_MOD_PARAMS 8U
#define EMUL_GFSK_PKT_PARAMS 9U

/* TX modulation and IQ polarity workaround bits (datasheet 15.1, 15.4) */
#define EMUL_TX_MODULATION_BIT BIT(2)
#define EMUL_IQ_POLARITY_BIT   BIT(2)

#define EMUL_FALLBACK_FS        0x40
#define EMUL_FALLBACK_STDBY_XOSC 0x30
#define EMUL_FALLBACK_STDBY_RC  0x20

#define EMUL_REG_XTA_TRIM 0x0911
#define EMUL_REG_XTB_TRIM 0x0912
/* Trim the state machine sets when the crystal first starts, or for a TCXO (datasheet 4.1.3) */
#define EMUL_XTAL_TRIM_XOSC  0x12
#define EMUL_XTA_TRIM_TCXO   0x2F
/* OCP after power on, and as SetPaConfig sets it for each PA (datasheet Table 5-2) */
#define EMUL_OCP_POR         0x18
#define EMUL_OCP_LOW_PA      0x18
#define EMUL_OCP_HP_PA       0x38
#define EMUL_RETENTION_ADDRS (SX126X_REG_RETENTION_LIST + 1)

/* GetDeviceErrors XOSC_START_ERR (datasheet Table 13-86) */
#define EMUL_OP_ERR_XOSC_START           BIT(5)
/* GFSK RxStatus from GetPacketStatus (datasheet Table 13-80) */
#define EMUL_GFSK_RX_STATUS_PKT_RECEIVED BIT(1)
#define EMUL_GFSK_RX_STATUS_CRC_ERR      BIT(4)
#define EMUL_GFSK_RX_STATUS_LENGTH_ERR   BIT(3)
/* GFSK CRCType for no CRC (datasheet Table 13-61) */
#define EMUL_GFSK_CRC_OFF                0x01

/* LoRa coding rates with the long interleaver (datasheet Table 13-49) */
#define EMUL_LORA_CR_4_5_LI 0x05
#define EMUL_LORA_CR_4_6_LI 0x06
#define EMUL_LORA_CR_4_8_LI 0x07

/* TxClampConfig reset value, before the PA clamping workaround (datasheet 15.2) */
#define EMUL_TX_CLAMP_CFG_RESET 0xC8
/* Writing 0 stops the RTC behind the RX timeout (datasheet 15.3) */
#define EMUL_REG_RTC_CTRL       0x0902

/* Low power PA duty cycle limits above and below 400 MHz (datasheet 13.1.14) */
#define EMUL_LP_PA_LOW_FREQ_HZ       400000000U
#define EMUL_LP_PA_DUTY_MAX          0x07
#define EMUL_LP_PA_DUTY_MAX_LOW_FREQ 0x04

/*
 * BUSY high times. Startup, calibration and mode switches follow the
 * datasheet (Table 8-2, 13.1.12); the time a write command without a mode
 * change keeps BUSY high is not specified, so a short figure stands in.
 */
#define EMUL_BUSY_CMD_US        10U
#define EMUL_BUSY_COLD_START_US 3500U
#define EMUL_BUSY_WARM_START_US 340U
#define EMUL_BUSY_CALIBRATE_US  3500U
/* The radio must not see NSS fall within this time of SetSleep (datasheet 13.1.1) */
#define EMUL_SLEEP_SETTLE_US    500U

#define EMUL_ERROR(target, fmt, ...)                                                               \
	do {                                                                                       \
		((struct sx126x_emul_data *)(target)->data)->errors++;                             \
		LOG_ERR("%s: " fmt, (target)->dev->name, ##__VA_ARGS__);                           \
	} while (false)

struct sx126x_emul_cfg {
	struct gpio_dt_spec reset;
	struct gpio_dt_spec busy;
	struct gpio_dt_spec dio1;
	enum sx126x_variants variant;
	/* The board clocks the radio from a TCXO rather than a crystal */
	bool tcxo_fitted;
};

struct sx126x_emul_data {
	const struct emul *target;
	struct k_mutex lock;
	struct gpio_callback reset_cb;
	struct gpio_callback busy_cb;
	struct gpio_callback dio1_cb;
	struct k_work_delayable tx_work;
	struct k_work_delayable rx_work;
	struct k_work_delayable timeout_work;
	struct k_work_delayable dc_work;
	/* RX timeout left running past RxDone in implicit header mode (datasheet 15.3) */
	struct k_work_delayable rtc_work;
	struct k_timer busy_timer;

	enum sx126x_emul_mode mode;
	uint8_t cmd_status;
	uint8_t fallback;
	uint16_t op_errors;
	/*
	 * BUSY held high for busy_cyc cycles from busy_start, besides reset and
	 * sleep, and NSS not to fall for settle_cyc cycles from sleep_start. The
	 * busy timer zeroes each once it has elapsed, before the cycle counter
	 * can wrap.
	 */
	uint32_t busy_start;
	uint32_t busy_cyc;
	uint32_t sleep_start;
	uint32_t settle_cyc;
	bool sleep_warm;
	/* The oscillator has run since power on */
	bool xosc_started;

	/* Configuration written by commands */
	uint8_t packet_type;
	bool packet_type_set;
	bool mod_params_set;
	bool pkt_params_set;
	uint8_t mod_params[EMUL_GFSK_MOD_PARAMS];
	uint8_t pkt_params[EMUL_GFSK_PKT_PARAMS];
	uint32_t freq_reg;
	bool freq_set;
	int8_t tx_power;
	uint8_t ramp_time;
	bool tx_params_set;
	uint8_t pa_config[4];
	bool pa_config_set;
	uint8_t image_cal[2];
	/* The image calibration ran with a working 32 MHz clock */
	bool image_cal_valid;
	bool dcdc;
	bool tcxo;
	uint8_t tcxo_voltage;
	uint32_t tcxo_delay;
	bool dio2_rf_switch;
	bool stop_on_preamble;
	uint16_t irq_mask;
	uint16_t dio1_mask;
	uint16_t dio2_mask;
	uint16_t dio3_mask;
	uint16_t irq_status;
	uint8_t tx_base;
	uint8_t rx_base;
	uint8_t buffer[SX126X_BUFFER_SIZE];
	uint8_t regs[EMUL_NUM_REGS];

	/* Reception */
	bool rx_continuous;
	/* A preamble has been detected, the sync word or header is still to come */
	bool rx_detecting;
	uint32_t dc_rx_period;
	uint32_t dc_sleep_period;
	bool dc_sleeping;
	uint8_t rx_len;
	uint8_t rx_offset;
	/* Where the next received byte goes; continues across packets in continuous RX */
	uint8_t rx_ptr;
	uint8_t pkt_status[3];
	int16_t rssi_inst;
	struct sx126x_emul_rx_packet rx_pending;
	bool rx_pending_valid;

	/* Transmission */
	k_timeout_t tx_duration;
	sx126x_emul_tx_cb_t tx_cb;
	void *tx_cb_user_data;

	sx126x_emul_cmd_cb_t cmd_cb;
	void *cmd_cb_user_data;

	struct sx126x_emul_tx_packet last_tx;
	bool last_tx_valid;
	struct sx126x_emul_radio last_rx;
	bool last_rx_valid;
	struct sx126x_emul_stats stats;
	uint32_t errors;
};

struct emul_reg_default {
	uint16_t addr;
	uint8_t val;
};

/*
 * Documented reset values (datasheet Table 12-1, 4.1.3 and 5.1). Registers the
 * datasheet leaves undefined, such as the GFSK sync word, read as zero.
 */
static const struct emul_reg_default emul_reg_defaults[] = {
	{SX126X_REG_GFSK_WHITENING_MSB, 0x01},
	{SX126X_REG_GFSK_WHITENING_MSB + 1, 0x00},
	{SX126X_REG_GFSK_CRC_INIT_MSB, 0x1D},
	{SX126X_REG_GFSK_CRC_INIT_MSB + 1, 0x0F},
	{SX126X_REG_GFSK_CRC_POLY_MSB, 0x10},
	{SX126X_REG_GFSK_CRC_POLY_MSB + 1, 0x21},
	{SX126X_REG_IQ_POLARITY, 0x0D},
	{SX126X_REG_LORA_SYNC_WORD_MSB, 0x14},
	{SX126X_REG_LORA_SYNC_WORD_LSB, 0x24},
	{SX126X_REG_TX_MODULATION, 0x01},
	{SX126X_REG_RX_GAIN, SX126X_RX_GAIN_POWER_SAVING},
	{SX126X_REG_TX_CLAMP_CFG, EMUL_TX_CLAMP_CFG_RESET},
	{SX126X_REG_OCP, EMUL_OCP_POR},
	{EMUL_REG_XTA_TRIM, 0x05},
	{EMUL_REG_XTB_TRIM, 0x05},
};

/*
 * Registers that a warm-start sleep loses unless they are on the retention
 * list (datasheet 9.6).
 */
static const uint16_t emul_volatile_regs[] = {
	SX126X_REG_RX_GAIN,
};

static const struct {
	uint8_t reg;
	uint32_t hz;
} emul_lora_bw[] = {
	{SX126X_LORA_BW_7_8, 7810},     {SX126X_LORA_BW_10_4, 10420},
	{SX126X_LORA_BW_15_6, 15630},   {SX126X_LORA_BW_20_8, 20830},
	{SX126X_LORA_BW_31_25, 31250},  {SX126X_LORA_BW_41_7, 41670},
	{SX126X_LORA_BW_62_5, 62500},   {SX126X_LORA_BW_125, 125000},
	{SX126X_LORA_BW_250, 250000},   {SX126X_LORA_BW_500, 500000},
};

static const struct {
	uint8_t reg;
	uint32_t hz;
} emul_gfsk_bw[] = {
	{SX126X_GFSK_BW_4800, 4800},     {SX126X_GFSK_BW_5800, 5800},
	{SX126X_GFSK_BW_7300, 7300},     {SX126X_GFSK_BW_9700, 9700},
	{SX126X_GFSK_BW_11700, 11700},   {SX126X_GFSK_BW_14600, 14600},
	{SX126X_GFSK_BW_19500, 19500},   {SX126X_GFSK_BW_23400, 23400},
	{SX126X_GFSK_BW_29300, 29300},   {SX126X_GFSK_BW_39000, 39000},
	{SX126X_GFSK_BW_46900, 46900},   {SX126X_GFSK_BW_58600, 58600},
	{SX126X_GFSK_BW_78200, 78200},   {SX126X_GFSK_BW_93800, 93800},
	{SX126X_GFSK_BW_117300, 117300}, {SX126X_GFSK_BW_156200, 156200},
	{SX126X_GFSK_BW_187200, 187200}, {SX126X_GFSK_BW_234300, 234300},
	{SX126X_GFSK_BW_312000, 312000}, {SX126X_GFSK_BW_373600, 373600},
	{SX126X_GFSK_BW_467000, 467000},
};

static uint32_t emul_lora_bw_hz(uint8_t reg)
{
	for (size_t i = 0; i < ARRAY_SIZE(emul_lora_bw); i++) {
		if (emul_lora_bw[i].reg == reg) {
			return emul_lora_bw[i].hz;
		}
	}

	return 0;
}

static uint32_t emul_gfsk_bw_hz(uint8_t reg)
{
	for (size_t i = 0; i < ARRAY_SIZE(emul_gfsk_bw); i++) {
		if (emul_gfsk_bw[i].reg == reg) {
			return emul_gfsk_bw[i].hz;
		}
	}

	return 0;
}

/* Frequency step of the synthesizer is XTAL / 2^25, rounded to the nearest Hz */
static uint32_t emul_freq_hz(uint32_t reg)
{
	return (uint32_t)((((uint64_t)reg * SX126X_XTAL_FREQ) + BIT64(24)) >> 25);
}

static uint8_t emul_reg_default(uint16_t addr)
{
	for (size_t i = 0; i < ARRAY_SIZE(emul_reg_defaults); i++) {
		if (emul_reg_defaults[i].addr == addr) {
			return emul_reg_defaults[i].val;
		}
	}

	return 0;
}

static bool emul_is_standby(const struct sx126x_emul_data *data)
{
	return data->mode == SX126X_EMUL_MODE_STDBY_RC || data->mode == SX126X_EMUL_MODE_STDBY_XOSC;
}

/* Asleep, as opposed to busy processing: only an NSS falling edge ends it */
static bool emul_is_asleep(const struct sx126x_emul_data *data)
{
	return data->mode == SX126X_EMUL_MODE_SLEEP ||
	       (data->mode == SX126X_EMUL_MODE_RX_DUTY_CYCLE && data->dc_sleeping);
}

/* Still processing a command, starting up or switching modes */
static bool emul_busy_pending(const struct sx126x_emul_data *data)
{
	return data->busy_cyc != 0 && (k_cycle_get_32() - data->busy_start) < data->busy_cyc;
}

/* Still entering sleep after SetSleep */
static bool emul_sleep_settling(const struct sx126x_emul_data *data)
{
	return data->settle_cyc != 0 && (k_cycle_get_32() - data->sleep_start) < data->settle_cyc;
}

/* BUSY stays high while the radio cannot take a command over SPI */
static bool emul_is_busy(const struct sx126x_emul_data *data)
{
	return data->mode == SX126X_EMUL_MODE_RESET || emul_is_asleep(data) ||
	       emul_busy_pending(data);
}

/* Receiving, as opposed to the sleep phase of an RX duty cycle */
static bool emul_is_receiving(const struct sx126x_emul_data *data)
{
	return data->mode == SX126X_EMUL_MODE_RX ||
	       (data->mode == SX126X_EMUL_MODE_RX_DUTY_CYCLE && !data->dc_sleeping);
}

/* SetRx, SetTx and SetRxDutyCycle timeouts and periods count 15.625 us steps */
static k_timeout_t emul_steps_to_timeout(uint32_t steps)
{
	return K_USEC(((uint64_t)steps * 15625U) / 1000U);
}

static void emul_line_set(const struct gpio_dt_spec *spec, bool level)
{
	gpio_flags_t flags;

	/* The host has not configured the pin as an input yet */
	if (gpio_emul_flags_get(spec->port, spec->pin, &flags) < 0 || (flags & GPIO_INPUT) == 0) {
		return;
	}

	/* The radio drives the physical level, whatever the devicetree flags say */
	(void)gpio_emul_input_set(spec->port, spec->pin, level);
}

static void emul_lines_sync(const struct emul *target)
{
	const struct sx126x_emul_cfg *cfg = target->cfg;
	struct sx126x_emul_data *data = target->data;

	emul_line_set(&cfg->busy, emul_is_busy(data));
	emul_line_set(&cfg->dio1, (data->irq_status & data->dio1_mask) != 0);
}

static void emul_busy_start(struct sx126x_emul_data *data, uint32_t us)
{
	/* Keep the expiry handler from pairing the new start with the old duration */
	data->busy_cyc = 0;
	compiler_barrier();
	data->busy_start = k_cycle_get_32();
	compiler_barrier();
	data->busy_cyc = k_us_to_cyc_ceil32(us);
	k_timer_start(&data->busy_timer, K_USEC(us), K_NO_WAIT);
}

/*
 * Drops BUSY once the radio is done and retires elapsed busy and sleep
 * settling times. This runs in interrupt context, so it cannot take the
 * lock: the host may be polling BUSY from the system workqueue, which the
 * delayed work items also need. A thread that changes the state
 * concurrently re-syncs the lines once it is done.
 */
static void emul_busy_expiry(struct k_timer *timer)
{
	struct sx126x_emul_data *data = CONTAINER_OF(timer, struct sx126x_emul_data, busy_timer);
	const struct sx126x_emul_cfg *cfg = data->target->cfg;

	if (!emul_busy_pending(data)) {
		data->busy_cyc = 0;
	}
	if (!emul_sleep_settling(data)) {
		data->settle_cyc = 0;
	}

	emul_line_set(&cfg->busy, emul_is_busy(data));
}

/* Delay counted from the moment BUSY falls after a command taking busy_us */
static k_timeout_t emul_after_busy(uint32_t busy_us, k_timeout_t delay)
{
	return K_TICKS(k_us_to_ticks_ceil64(busy_us) + delay.ticks);
}

static void emul_raise_irq(struct sx126x_emul_data *data, uint16_t irq)
{
	/* Only IRQs enabled in the IRQ mask latch in the status */
	data->irq_status |= irq & data->irq_mask;
}

static void emul_works_cancel(struct sx126x_emul_data *data)
{
	(void)k_work_cancel_delayable(&data->tx_work);
	(void)k_work_cancel_delayable(&data->rx_work);
	(void)k_work_cancel_delayable(&data->timeout_work);
	(void)k_work_cancel_delayable(&data->dc_work);
	data->rx_detecting = false;
}

/* Calibration needs the 32 MHz clock, which a TCXO only gives once configured */
static bool emul_clock_ok(const struct emul *target)
{
	const struct sx126x_emul_cfg *cfg = target->cfg;
	const struct sx126x_emul_data *data = target->data;

	return !cfg->tcxo_fitted || data->tcxo;
}

static void emul_power_on(const struct emul *target)
{
	const struct sx126x_emul_cfg *cfg = target->cfg;
	struct sx126x_emul_data *data = target->data;

	emul_works_cancel(data);
	(void)k_work_cancel_delayable(&data->rtc_work);

	memset(data->regs, 0, sizeof(data->regs));
	for (size_t i = 0; i < ARRAY_SIZE(emul_reg_defaults); i++) {
		data->regs[emul_reg_defaults[i].addr] = emul_reg_defaults[i].val;
	}
	memset(data->buffer, 0, sizeof(data->buffer));

	data->mode = SX126X_EMUL_MODE_STDBY_RC;
	data->cmd_status = SX126X_CMD_STATUS_RESERVED;
	data->fallback = EMUL_FALLBACK_STDBY_RC;
	/*
	 * The power-on calibration needs the 32 MHz clock, which an unpowered
	 * TCXO does not provide (datasheet 13.3.6).
	 */
	data->op_errors = cfg->tcxo_fitted ? EMUL_OP_ERR_XOSC_START : 0;
	data->busy_cyc = 0;
	data->settle_cyc = 0;
	data->xosc_started = false;
	data->packet_type = SX126X_PACKET_TYPE_GFSK;
	data->packet_type_set = false;
	data->mod_params_set = false;
	data->pkt_params_set = false;
	memset(data->mod_params, 0, sizeof(data->mod_params));
	memset(data->pkt_params, 0, sizeof(data->pkt_params));
	data->freq_reg = 0;
	data->freq_set = false;
	data->tx_power = 0;
	data->ramp_time = 0;
	data->tx_params_set = false;
	memset(data->pa_config, 0, sizeof(data->pa_config));
	data->pa_config_set = false;
	data->image_cal[0] = EMUL_IMAGE_CAL_POR_LOW;
	data->image_cal[1] = EMUL_IMAGE_CAL_POR_HIGH;
	/* Datasheet 9.2.1: the power-on image calibration fails on a TCXO */
	data->image_cal_valid = !cfg->tcxo_fitted;
	data->dcdc = false;
	data->tcxo = false;
	data->tcxo_voltage = 0;
	data->tcxo_delay = 0;
	data->dio2_rf_switch = false;
	data->stop_on_preamble = false;
	data->irq_mask = 0;
	data->dio1_mask = 0;
	data->dio2_mask = 0;
	data->dio3_mask = 0;
	data->irq_status = 0;
	data->tx_base = 0;
	data->rx_base = 0;
	data->rx_continuous = false;
	data->rx_len = 0;
	data->rx_offset = 0;
	data->rx_ptr = 0;
	memset(data->pkt_status, 0, sizeof(data->pkt_status));
}

static void emul_sleep(const struct emul *target, uint8_t sleep_cfg)
{
	struct sx126x_emul_data *data = target->data;

	emul_works_cancel(data);

	if ((sleep_cfg & SX126X_SLEEP_WARM_START) == 0) {
		/* Cold start: the configuration is lost */
		emul_power_on(target);
	} else {
		uint8_t count = MIN(data->regs[SX126X_REG_RETENTION_LIST],
				    SX126X_MAX_RETENTION_REGS);

		for (size_t i = 0; i < ARRAY_SIZE(emul_volatile_regs); i++) {
			uint16_t addr = emul_volatile_regs[i];
			bool retained = false;

			for (uint8_t j = 0; j < count; j++) {
				uint16_t reg = EMUL_RETENTION_ADDRS + 2 * j;

				if (sys_get_be16(&data->regs[reg]) == addr) {
					retained = true;
				}
			}

			if (!retained) {
				data->regs[addr] = emul_reg_default(addr);
			}
		}

		/* Datasheet 7.4: the data buffer does not survive sleep */
		memset(data->buffer, 0, sizeof(data->buffer));
	}

	data->mode = SX126X_EMUL_MODE_SLEEP;
	data->sleep_warm = (sleep_cfg & SX126X_SLEEP_WARM_START) != 0;
	data->settle_cyc = 0;
	compiler_barrier();
	data->sleep_start = k_cycle_get_32();
	compiler_barrier();
	data->settle_cyc = k_us_to_cyc_ceil32(EMUL_SLEEP_SETTLE_US);
	/* BUSY stays high, but the timer retires the settling time */
	k_timer_start(&data->busy_timer, K_USEC(EMUL_SLEEP_SETTLE_US), K_NO_WAIT);
	data->stats.sleeps++;
}

static void emul_fallback(struct sx126x_emul_data *data)
{
	switch (data->fallback) {
	case EMUL_FALLBACK_FS:
		data->mode = SX126X_EMUL_MODE_FS;
		break;
	case EMUL_FALLBACK_STDBY_XOSC:
		data->mode = SX126X_EMUL_MODE_STDBY_XOSC;
		break;
	default:
		data->mode = SX126X_EMUL_MODE_STDBY_RC;
		break;
	}
}

/* TCXO start-up delay from SetDIO3AsTcxoCtrl, 0 without a TCXO */
static uint32_t emul_tcxo_delay_us(const struct sx126x_emul_data *data)
{
	return data->tcxo ? (uint32_t)(((uint64_t)data->tcxo_delay * 15625U) / 1000U) : 0;
}

/* Mode switching times (datasheet Table 8-2) */
static const struct {
	enum sx126x_emul_mode from;
	enum sx126x_emul_mode to;
	uint16_t us;
} emul_switch_times[] = {
	{SX126X_EMUL_MODE_STDBY_RC, SX126X_EMUL_MODE_STDBY_XOSC, 31},
	{SX126X_EMUL_MODE_STDBY_RC, SX126X_EMUL_MODE_FS, 50},
	{SX126X_EMUL_MODE_STDBY_RC, SX126X_EMUL_MODE_RX, 83},
	{SX126X_EMUL_MODE_STDBY_RC, SX126X_EMUL_MODE_TX, 126},
	{SX126X_EMUL_MODE_STDBY_XOSC, SX126X_EMUL_MODE_FS, 40},
	{SX126X_EMUL_MODE_STDBY_XOSC, SX126X_EMUL_MODE_TX, 105},
	{SX126X_EMUL_MODE_STDBY_XOSC, SX126X_EMUL_MODE_RX, 62},
	{SX126X_EMUL_MODE_FS, SX126X_EMUL_MODE_RX, 41},
	{SX126X_EMUL_MODE_FS, SX126X_EMUL_MODE_TX, 76},
	{SX126X_EMUL_MODE_RX, SX126X_EMUL_MODE_FS, 15},
	{SX126X_EMUL_MODE_RX, SX126X_EMUL_MODE_TX, 92},
};

static enum sx126x_emul_mode emul_switch_class(enum sx126x_emul_mode mode)
{
	return mode == SX126X_EMUL_MODE_RX_DUTY_CYCLE ? SX126X_EMUL_MODE_RX : mode;
}

/*
 * Switch to an active mode and return how long BUSY stays high doing so.
 * Leaving STDBY_RC with a TCXO also waits for its start-up delay
 * (datasheet 13.3.6).
 */
static uint32_t emul_enter_mode(const struct emul *target, enum sx126x_emul_mode mode)
{
	const struct sx126x_emul_cfg *cfg = target->cfg;
	struct sx126x_emul_data *data = target->data;
	enum sx126x_emul_mode from = emul_switch_class(data->mode);
	enum sx126x_emul_mode to = emul_switch_class(mode);
	uint32_t us = EMUL_BUSY_CMD_US;

	for (size_t i = 0; i < ARRAY_SIZE(emul_switch_times); i++) {
		if (emul_switch_times[i].from == from && emul_switch_times[i].to == to) {
			us = emul_switch_times[i].us;
		}
	}

	if (mode != SX126X_EMUL_MODE_STDBY_RC) {
		/* Datasheet 13.3.6: an unpowered TCXO gives no 32 MHz clock */
		if (cfg->tcxo_fitted && !data->tcxo) {
			EMUL_ERROR(target, "Mode %d needs the TCXO, which DIO3 does not drive",
				   mode);
			data->op_errors |= EMUL_OP_ERR_XOSC_START;
		}
		if (from == SX126X_EMUL_MODE_STDBY_RC) {
			us += emul_tcxo_delay_us(data);
		}
		/* Datasheet 4.1.3: the crystal trim is set as the oscillator first starts */
		if (!data->xosc_started && !data->tcxo) {
			data->regs[EMUL_REG_XTA_TRIM] = EMUL_XTAL_TRIM_XOSC;
			data->regs[EMUL_REG_XTB_TRIM] = EMUL_XTAL_TRIM_XOSC;
		}
		data->xosc_started = true;
	}

	data->mode = mode;

	return us;
}

/* Coding rate denominator (4/x), 0 for an invalid register value */
static uint8_t emul_lora_cr_denominator(uint8_t cr)
{
	switch (cr) {
	case SX126X_LORA_CR_4_5:
	case SX126X_LORA_CR_4_6:
	case SX126X_LORA_CR_4_7:
	case SX126X_LORA_CR_4_8:
		return cr + 4;
	case EMUL_LORA_CR_4_5_LI:
		return 5;
	case EMUL_LORA_CR_4_6_LI:
		return 6;
	case EMUL_LORA_CR_4_8_LI:
		return 8;
	default:
		return 0;
	}
}

static void emul_radio_fill(const struct sx126x_emul_data *data, struct sx126x_emul_radio *radio)
{
	const uint8_t *mod = data->mod_params;
	const uint8_t *pkt = data->pkt_params;

	memset(radio, 0, sizeof(*radio));

	radio->packet_type = data->packet_type == SX126X_PACKET_TYPE_LORA
				     ? SX126X_EMUL_PACKET_TYPE_LORA
				     : SX126X_EMUL_PACKET_TYPE_GFSK;
	radio->frequency = emul_freq_hz(data->freq_reg);
	radio->tx_power = data->tx_power;
	radio->ramp_time = data->ramp_time;
	radio->pa.duty_cycle = data->pa_config[0];
	radio->pa.hp_max = data->pa_config[1];
	radio->pa.device_sel = data->pa_config[2];
	radio->pa.pa_lut = data->pa_config[3];
	radio->rx_gain = data->regs[SX126X_REG_RX_GAIN];
	radio->iq_polarity = data->regs[SX126X_REG_IQ_POLARITY];
	radio->tx_modulation = data->regs[SX126X_REG_TX_MODULATION];
	radio->dcdc = data->dcdc;
	radio->tcxo = data->tcxo;
	radio->tcxo_voltage = data->tcxo_voltage;
	radio->dio2_rf_switch = data->dio2_rf_switch;
	radio->irq_mask = data->irq_mask;
	radio->dio1_mask = data->dio1_mask;

	if (data->packet_type == SX126X_PACKET_TYPE_LORA) {
		struct sx126x_emul_lora_params *lora = &radio->lora;

		lora->sf = mod[0];
		lora->bandwidth = emul_lora_bw_hz(mod[1]);
		lora->cr = emul_lora_cr_denominator(mod[2]);
		lora->long_interleave =
			mod[2] >= EMUL_LORA_CR_4_5_LI && mod[2] <= EMUL_LORA_CR_4_8_LI;
		lora->ldro = mod[3] != 0;
		lora->preamble_len = sys_get_be16(&pkt[0]);
		lora->implicit_header = pkt[2] == SX126X_LORA_HEADER_IMPLICIT;
		lora->payload_len = pkt[3];
		lora->crc = pkt[4] == SX126X_LORA_CRC_ON;
		lora->iq_inverted = pkt[5] == SX126X_LORA_IQ_INVERTED;
		lora->sync_word = sys_get_be16(&data->regs[SX126X_REG_LORA_SYNC_WORD_MSB]);
	} else {
		struct sx126x_emul_gfsk_params *gfsk = &radio->gfsk;
		uint32_t br_reg = sys_get_be24(&mod[0]);

		gfsk->bitrate = br_reg != 0 ? (uint32_t)((32ULL * SX126X_XTAL_FREQ) / br_reg) : 0;
		gfsk->pulse_shape = mod[3];
		gfsk->bandwidth = emul_gfsk_bw_hz(mod[4]);
		gfsk->freq_deviation = emul_freq_hz(sys_get_be24(&mod[5]));
		gfsk->preamble_bits = sys_get_be16(&pkt[0]);
		gfsk->preamble_detect = pkt[2];
		gfsk->sync_word_bits = pkt[3];
		gfsk->addr_comp = pkt[4];
		gfsk->variable_len = pkt[5] == SX126X_GFSK_PKT_VAR_LEN;
		gfsk->payload_len = pkt[6];
		gfsk->crc_type = pkt[7];
		gfsk->whitening = pkt[8] == SX126X_GFSK_WHITENING_ON;
		memcpy(gfsk->sync_word, &data->regs[SX126X_REG_GFSK_SYNC_WORD],
		       sizeof(gfsk->sync_word));
		gfsk->crc_init = sys_get_be16(&data->regs[SX126X_REG_GFSK_CRC_INIT_MSB]);
		gfsk->crc_poly = sys_get_be16(&data->regs[SX126X_REG_GFSK_CRC_POLY_MSB]);
		gfsk->whitening_init =
			sys_get_be16(&data->regs[SX126X_REG_GFSK_WHITENING_MSB]) & 0x01FF;
	}
}

static void emul_check_lora(const struct emul *target)
{
	const struct sx126x_emul_cfg *cfg = target->cfg;
	struct sx126x_emul_data *data = target->data;
	uint8_t sf = data->mod_params[0];
	uint8_t cr = data->mod_params[2];
	bool ldro = data->mod_params[3] != 0;
	uint16_t preamble_len = sys_get_be16(&data->pkt_params[0]);
	bool iq_inverted = data->pkt_params[5] == SX126X_LORA_IQ_INVERTED;
	uint32_t bw_hz = emul_lora_bw_hz(data->mod_params[1]);
	bool iq_bit = (data->regs[SX126X_REG_IQ_POLARITY] & EMUL_IQ_POLARITY_BIT) != 0;

	if (sf < 5 || sf > 12) {
		EMUL_ERROR(target, "Invalid spreading factor %u", sf);
		return;
	}

	if (bw_hz == 0) {
		EMUL_ERROR(target, "Invalid LoRa bandwidth 0x%02x", data->mod_params[1]);
		return;
	}

	if (emul_lora_cr_denominator(cr) == 0) {
		EMUL_ERROR(target, "Invalid coding rate 0x%02x", cr);
	}

	/*
	 * Datasheet 6.1.1.4 recommends LDRO from a 16.38 ms symbol time; it is
	 * treated as required.
	 */
	if (((BIT64(sf) * USEC_PER_SEC) / bw_hz) > EMUL_LDRO_SYMBOL_US && !ldro) {
		EMUL_ERROR(target, "SF%u at %u Hz requires LDRO", sf, bw_hz);
	}

	/* Datasheet 6.1.1.1 recommends 12 preamble symbols; treated as required */
	if (sf <= 6 && preamble_len < EMUL_SF56_MIN_PREAMBLE) {
		EMUL_ERROR(target, "SF%u requires a preamble of at least %u symbols, got %u", sf,
			   EMUL_SF56_MIN_PREAMBLE, preamble_len);
	}

	if (cfg->variant == SX126X_LLCC68) {
		uint8_t max_sf = bw_hz == 125000   ? 9
				 : bw_hz == 250000 ? 10
				 : bw_hz == 500000 ? 11
						   : 0;

		if (sf > max_sf) {
			EMUL_ERROR(target, "LLCC68 does not support SF%u at %u Hz", sf, bw_hz);
		}
	}

	/* Datasheet 15.4: bit cleared for inverted IQ, set otherwise */
	if (iq_bit == iq_inverted) {
		EMUL_ERROR(target, "IQ polarity workaround not applied (reg 0x%02x, IQ %s)",
			   data->regs[SX126X_REG_IQ_POLARITY],
			   iq_inverted ? "inverted" : "standard");
	}

}

static void emul_check_pa(const struct emul *target)
{
	struct sx126x_emul_data *data = target->data;
	uint32_t freq = emul_freq_hz(data->freq_reg);
	uint8_t duty_cycle = data->pa_config[0];

	if (!data->pa_config_set) {
		return;
	}

	if (data->pa_config[2] == SX126X_DEVICE_SEL_SX1261) {
		/* Datasheet 13.1.14: higher values overstress the PA */
		if (freq < EMUL_LP_PA_LOW_FREQ_HZ && duty_cycle > EMUL_LP_PA_DUTY_MAX_LOW_FREQ) {
			EMUL_ERROR(target, "paDutyCycle 0x%02x above 0x%02x below 400 MHz",
				   duty_cycle, EMUL_LP_PA_DUTY_MAX_LOW_FREQ);
		}
	} else if ((data->regs[SX126X_REG_TX_CLAMP_CFG] & SX126X_TX_CLAMP_CFG_MASK) !=
		   SX126X_TX_CLAMP_CFG_MASK) {
		/* Datasheet 15.2 */
		EMUL_ERROR(target, "PA clamping workaround not applied (reg 0x%02x)",
			   data->regs[SX126X_REG_TX_CLAMP_CFG]);
	}
}

static void emul_check_radio(const struct emul *target, bool tx)
{
	struct sx126x_emul_data *data = target->data;
	uint32_t freq = emul_freq_hz(data->freq_reg);

	if (!data->packet_type_set) {
		EMUL_ERROR(target, "No SetPacketType since reset");
	}
	if (!data->mod_params_set) {
		EMUL_ERROR(target, "No SetModulationParams since SetPacketType");
	}
	if (!data->pkt_params_set) {
		EMUL_ERROR(target, "No SetPacketParams since SetPacketType");
	}
	if (!data->freq_set) {
		EMUL_ERROR(target, "No SetRfFrequency since reset");
	}

	if (tx) {
		bool bw500 = data->packet_type == SX126X_PACKET_TYPE_LORA &&
			     data->mod_params[1] == SX126X_LORA_BW_500;
		bool tx_mod_bit =
			(data->regs[SX126X_REG_TX_MODULATION] & EMUL_TX_MODULATION_BIT) != 0;

		if (!data->pa_config_set) {
			EMUL_ERROR(target, "No SetPaConfig since reset");
		}
		if (!data->tx_params_set) {
			EMUL_ERROR(target, "No SetTxParams since reset");
		}
		/* Datasheet 15.1: bit cleared for 500 kHz LoRa, set otherwise */
		if (tx_mod_bit == bw500) {
			EMUL_ERROR(target, "TX modulation workaround not applied (reg 0x%02x)",
				   data->regs[SX126X_REG_TX_MODULATION]);
		}
		emul_check_pa(target);
	} else if (!data->image_cal_valid) {
		/* Datasheet 9.2.1 */
		EMUL_ERROR(target, "Receiving without an image calibration run on the TCXO");
	} else if (freq < data->image_cal[0] * EMUL_IMAGE_STEP_HZ ||
		   freq > data->image_cal[1] * EMUL_IMAGE_STEP_HZ) {
		EMUL_ERROR(target, "Receiving at %u Hz outside the calibrated image band %u-%u MHz",
			   freq, data->image_cal[0] * 4, data->image_cal[1] * 4);
	}

	if (data->packet_type == SX126X_PACKET_TYPE_LORA) {
		emul_check_lora(target);
	} else if (sys_get_be24(&data->mod_params[0]) == 0) {
		EMUL_ERROR(target, "GFSK bit rate not set");
	}
}

static uint8_t emul_payload_len(const struct sx126x_emul_data *data)
{
	return data->packet_type == SX126X_PACKET_TYPE_LORA ? data->pkt_params[3]
							    : data->pkt_params[6];
}

static uint32_t emul_start_tx(const struct emul *target, uint32_t timeout)
{
	struct sx126x_emul_data *data = target->data;
	struct sx126x_emul_tx_packet *pkt = &data->last_tx;
	uint64_t tx_us = k_ticks_to_us_ceil64(data->tx_duration.ticks);
	uint64_t timeout_us = k_ticks_to_us_ceil64(emul_steps_to_timeout(timeout).ticks);
	uint32_t busy_us;

	/* Datasheet Figure 9-1 */
	if (!emul_is_standby(data) && data->mode != SX126X_EMUL_MODE_FS &&
	    !emul_is_receiving(data)) {
		EMUL_ERROR(target, "SetTx outside standby, FS or RX (mode %d)", data->mode);
	}

	emul_check_radio(target, true);
	emul_works_cancel(data);
	if (timeout != SX126X_TX_TIMEOUT_NONE) {
		/* The RTC is reprogrammed for the new timeout */
		(void)k_work_cancel_delayable(&data->rtc_work);
	}

	emul_radio_fill(data, &pkt->radio);
	pkt->timeout = timeout;
	pkt->len = emul_payload_len(data);
	for (size_t i = 0; i < pkt->len; i++) {
		pkt->data[i] = data->buffer[(uint8_t)(data->tx_base + i)];
	}
	data->last_tx_valid = true;

	busy_us = emul_enter_mode(target, SX126X_EMUL_MODE_TX);
	data->stats.tx_starts++;

	/* Transmission starts as BUSY falls (datasheet 8.3.1) */
	if (timeout != SX126X_TX_TIMEOUT_NONE && tx_us > timeout_us) {
		(void)k_work_schedule(&data->timeout_work,
				      emul_after_busy(busy_us, emul_steps_to_timeout(timeout)));
	} else {
		(void)k_work_schedule(&data->tx_work, emul_after_busy(busy_us, data->tx_duration));
	}

	return busy_us;
}

static uint32_t emul_start_rx(const struct emul *target, uint32_t timeout, bool duty_cycle,
			      uint32_t rx_period, uint32_t sleep_period)
{
	struct sx126x_emul_data *data = target->data;
	uint32_t busy_us;

	if (duty_cycle) {
		/* Datasheet 13.1.7 */
		if (data->mode != SX126X_EMUL_MODE_STDBY_RC) {
			EMUL_ERROR(target, "SetRxDutyCycle outside STDBY_RC (mode %d)", data->mode);
		}
	} else if (!emul_is_standby(data) && data->mode != SX126X_EMUL_MODE_FS &&
		   data->mode != SX126X_EMUL_MODE_TX && !emul_is_receiving(data)) {
		/* Datasheet Figure 9-1 */
		EMUL_ERROR(target, "SetRx outside standby, FS, TX or RX (mode %d)", data->mode);
	}

	emul_check_radio(target, false);
	emul_works_cancel(data);
	if (duty_cycle ||
	    (timeout != SX126X_RX_TIMEOUT_SINGLE && timeout != SX126X_RX_TIMEOUT_CONTINUOUS)) {
		/* The RTC is reprogrammed for the new timeout */
		(void)k_work_cancel_delayable(&data->rtc_work);
	}

	emul_radio_fill(data, &data->last_rx);
	data->last_rx_valid = true;
	data->stats.rx_starts++;
	/* Datasheet 7.2: each entry into RX starts writing at the base address */
	data->rx_ptr = data->rx_base;

	/* Reception starts as BUSY falls (datasheet 8.3.1) */
	if (duty_cycle) {
		/* The cycle opens with its receive window */
		busy_us = emul_enter_mode(target, SX126X_EMUL_MODE_RX_DUTY_CYCLE);
		data->rx_continuous = false;
		data->dc_rx_period = rx_period;
		data->dc_sleep_period = sleep_period;
		data->dc_sleeping = false;
		(void)k_work_schedule(&data->dc_work,
				      emul_after_busy(busy_us, emul_steps_to_timeout(rx_period)));
	} else {
		busy_us = emul_enter_mode(target, SX126X_EMUL_MODE_RX);
		data->rx_continuous = timeout == SX126X_RX_TIMEOUT_CONTINUOUS;
		if (timeout != SX126X_RX_TIMEOUT_SINGLE && !data->rx_continuous) {
			(void)k_work_schedule(
				&data->timeout_work,
				emul_after_busy(busy_us, emul_steps_to_timeout(timeout)));
		}
	}

	if (data->rx_pending_valid) {
		/* Arrive inside the receive window, however short the tick makes it */
		(void)k_work_schedule(
			&data->rx_work,
			emul_after_busy(busy_us, duty_cycle ? K_NO_WAIT : EMUL_RX_DELAY));
	}

	return busy_us;
}

/* Time from the start of the preamble to the end of the sync word (GFSK) or header (LoRa) */
static uint32_t emul_sync_us(const struct sx126x_emul_data *data)
{
	const uint8_t *mod = data->mod_params;
	const uint8_t *pkt = data->pkt_params;

	if (data->packet_type == SX126X_PACKET_TYPE_LORA) {
		uint32_t bw_hz = emul_lora_bw_hz(mod[1]);
		bool explicit_hdr = pkt[2] == SX126X_LORA_HEADER_EXPLICIT;
		/* In quarter symbols: preamble, 4.25 sync symbols, 8 header symbols */
		uint64_t quarters = 4ULL * sys_get_be16(&pkt[0]) + 17U + (explicit_hdr ? 32U : 0U);

		if (bw_hz == 0 || mod[0] > 12) {
			return 0;
		}

		return (uint32_t)((quarters * BIT64(mod[0]) * USEC_PER_SEC) / (4ULL * bw_hz));
	}

	/* The bit rate register holds 32 * FXOSC / bit rate */
	return (uint32_t)(((uint64_t)(sys_get_be16(&pkt[0]) + pkt[3]) * sys_get_be24(&mod[0]) *
			   USEC_PER_SEC) /
			  (32ULL * SX126X_XTAL_FREQ));
}

/* The queued packet's preamble is detected */
static void emul_rx_detect(const struct emul *target)
{
	struct sx126x_emul_data *data = target->data;

	data->rx_detecting = true;
	emul_raise_irq(data, SX126X_IRQ_PREAMBLE_DETECTED);

	/* Datasheet 13.1.6 */
	if (data->stop_on_preamble) {
		(void)k_work_cancel_delayable(&data->timeout_work);
	}

	/* Datasheet 13.1.7: the window is restarted for 2 * rxPeriod + sleepPeriod */
	if (data->mode == SX126X_EMUL_MODE_RX_DUTY_CYCLE) {
		(void)k_work_reschedule(
			&data->dc_work,
			emul_steps_to_timeout(2 * data->dc_rx_period + data->dc_sleep_period));
	}

	(void)k_work_schedule(&data->rx_work, K_USEC(emul_sync_us(data)));
}

/* Single reception ends; continuous reception carries on listening */
static void emul_rx_end(struct sx126x_emul_data *data)
{
	if (!data->rx_continuous) {
		(void)k_work_cancel_delayable(&data->dc_work);
		emul_fallback(data);
	}
}

/* The sync word or header of the queued packet arrives, and the packet with it */
static void emul_rx_deliver(const struct emul *target)
{
	struct sx126x_emul_data *data = target->data;
	const struct sx126x_emul_rx_packet *pkt = &data->rx_pending;
	bool lora = data->packet_type == SX126X_PACKET_TYPE_LORA;
	bool implicit = lora && data->pkt_params[2] == SX126X_LORA_HEADER_IMPLICIT;
	bool crc_on = lora ? data->pkt_params[4] == SX126X_LORA_CRC_ON
			   : data->pkt_params[7] != EMUL_GFSK_CRC_OFF;
	/* Corruption goes unnoticed without a CRC */
	bool crc_error = pkt->crc_error && crc_on;
	uint8_t rssi_reg = (uint8_t)CLAMP(-2 * (int32_t)pkt->rssi, 0, UINT8_MAX);
	uint16_t irq = SX126X_IRQ_RX_DONE;
	uint8_t len = pkt->len;

	data->rx_detecting = false;
	data->rx_pending_valid = false;

	if (!lora && data->pkt_params[5] == SX126X_GFSK_PKT_VAR_LEN &&
	    pkt->len > data->pkt_params[6]) {
		/*
		 * Longer than the receiver accepts: the packet is dropped with
		 * a length error and no RxDone, single reception ending as it
		 * does when the preamble detector rejects a packet (6.2.2.1).
		 */
		emul_raise_irq(data, SX126X_IRQ_SYNC_WORD_VALID);
		data->pkt_status[0] = EMUL_GFSK_RX_STATUS_LENGTH_ERR;
		(void)k_work_cancel_delayable(&data->timeout_work);
		LOG_DBG("%s: dropped %u bytes, limit %u", target->dev->name, pkt->len,
			data->pkt_params[6]);
		emul_rx_end(data);
		return;
	}

	/* A fixed length packet carries no length on air: the radio takes the configured one */
	if (lora && data->pkt_params[2] == SX126X_LORA_HEADER_IMPLICIT) {
		len = data->pkt_params[3];
	} else if (!lora && data->pkt_params[5] == SX126X_GFSK_PKT_FIX_LEN) {
		len = data->pkt_params[6];
	}

	for (size_t i = 0; i < len; i++) {
		data->buffer[(uint8_t)(data->rx_ptr + i)] = i < pkt->len ? pkt->data[i] : 0;
	}
	data->rx_len = len;
	data->rx_offset = data->rx_ptr;
	/* Datasheet 7.2: continuous RX carries on from the end of the last packet */
	if (data->rx_continuous) {
		data->rx_ptr += len;
	}

	if (lora) {
		data->pkt_status[0] = rssi_reg;
		data->pkt_status[1] = (uint8_t)(int8_t)(pkt->snr * 4);
		data->pkt_status[2] = rssi_reg;
		if (data->pkt_params[2] == SX126X_LORA_HEADER_EXPLICIT) {
			irq |= SX126X_IRQ_HEADER_VALID;
		}
	} else {
		data->pkt_status[0] = EMUL_GFSK_RX_STATUS_PKT_RECEIVED |
				      (crc_error ? EMUL_GFSK_RX_STATUS_CRC_ERR : 0);
		data->pkt_status[1] = rssi_reg;
		data->pkt_status[2] = rssi_reg;
		irq |= SX126X_IRQ_SYNC_WORD_VALID;
	}

	if (crc_error) {
		irq |= SX126X_IRQ_CRC_ERR;
	} else {
		data->cmd_status = SX126X_CMD_STATUS_DATA_AVAILABLE;
	}

	emul_raise_irq(data, irq);
	LOG_DBG("%s: received %u bytes, IRQ status 0x%04x", target->dev->name, len,
		data->irq_status);
	data->stats.rx_done++;

	if (implicit && k_work_delayable_is_pending(&data->timeout_work)) {
		/*
		 * Datasheet 15.3: with no header to stop it, the RX timeout
		 * outlives the reception unless the host stops the RTC.
		 */
		k_ticks_t left = k_work_delayable_remaining_get(&data->timeout_work);

		(void)k_work_cancel_delayable(&data->timeout_work);
		(void)k_work_schedule(&data->rtc_work, K_TICKS(left));
	} else {
		(void)k_work_cancel_delayable(&data->timeout_work);
	}

	emul_rx_end(data);
}

static void emul_tx_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct sx126x_emul_data *data = CONTAINER_OF(dwork, struct sx126x_emul_data, tx_work);
	const struct emul *target = data->target;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->mode == SX126X_EMUL_MODE_TX) {
		emul_fallback(data);
		emul_raise_irq(data, SX126X_IRQ_TX_DONE);
		data->cmd_status = SX126X_CMD_STATUS_CMD_TX_DONE;
		data->stats.tx_done++;
		emul_lines_sync(target);

		if (data->tx_cb != NULL) {
			data->tx_cb(target, &data->last_tx, data->tx_cb_user_data);
		}
	}

	k_mutex_unlock(&data->lock);
}

static void emul_rx_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct sx126x_emul_data *data = CONTAINER_OF(dwork, struct sx126x_emul_data, rx_work);

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->rx_pending_valid && emul_is_receiving(data)) {
		if (data->rx_detecting) {
			emul_rx_deliver(data->target);
		} else {
			emul_rx_detect(data->target);
		}
		emul_lines_sync(data->target);
	}

	k_mutex_unlock(&data->lock);
}

static void emul_timeout_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct sx126x_emul_data *data = CONTAINER_OF(dwork, struct sx126x_emul_data, timeout_work);

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->mode == SX126X_EMUL_MODE_TX) {
		data->stats.tx_timeouts++;
	} else if (data->mode == SX126X_EMUL_MODE_RX && !data->rx_continuous) {
		data->stats.rx_timeouts++;
	} else {
		k_mutex_unlock(&data->lock);
		return;
	}

	/* A packet whose sync word or header has not arrived is lost */
	data->rx_detecting = false;
	(void)k_work_cancel_delayable(&data->rx_work);
	emul_fallback(data);
	emul_raise_irq(data, SX126X_IRQ_RX_TX_TIMEOUT);
	emul_lines_sync(data->target);

	k_mutex_unlock(&data->lock);
}

static void emul_rtc_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct sx126x_emul_data *data = CONTAINER_OF(dwork, struct sx126x_emul_data, rtc_work);

	k_mutex_lock(&data->lock, K_FOREVER);
	EMUL_ERROR(data->target, "Stale RX timeout after an implicit header reception, "
				 "RTC not stopped (datasheet 15.3)");
	emul_raise_irq(data, SX126X_IRQ_RX_TX_TIMEOUT);
	emul_lines_sync(data->target);
	k_mutex_unlock(&data->lock);
}

static void emul_dc_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct sx126x_emul_data *data = CONTAINER_OF(dwork, struct sx126x_emul_data, dc_work);

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->mode == SX126X_EMUL_MODE_RX_DUTY_CYCLE) {
		if (data->rx_detecting) {
			/* The restarted window ran out before the header: the packet is lost */
			(void)k_work_cancel_delayable(&data->rx_work);
			data->rx_detecting = false;
			data->rx_pending_valid = false;
		}
		data->dc_sleeping = !data->dc_sleeping;
		LOG_DBG("%s: duty cycle %s", data->target->dev->name,
			data->dc_sleeping ? "sleeping" : "receiving");
		if (data->dc_sleeping) {
			/*
			 * Waking for the next window takes a warm start, and the
			 * TCXO start-up delay when DIO3 drives one (13.1.7).
			 */
			uint32_t wake_us = EMUL_BUSY_WARM_START_US + emul_tcxo_delay_us(data);
			k_timeout_t sleep = emul_steps_to_timeout(data->dc_sleep_period);

			(void)k_work_schedule(&data->dc_work, emul_after_busy(wake_us, sleep));
		} else {
			(void)k_work_schedule(&data->dc_work,
					      emul_steps_to_timeout(data->dc_rx_period));
		}
		/* A packet sent during the sleep phase is caught by the next window */
		if (!data->dc_sleeping && data->rx_pending_valid) {
			emul_rx_detect(data->target);
		}
		emul_lines_sync(data->target);
	}

	k_mutex_unlock(&data->lock);
}

static bool emul_params(const struct emul *target, uint8_t opcode, size_t got, size_t want)
{
	struct sx126x_emul_data *data = target->data;

	if (got != want) {
		EMUL_ERROR(target, "Opcode 0x%02x takes %zu parameter bytes, got %zu", opcode,
			   want, got);
		data->cmd_status = SX126X_CMD_STATUS_CMD_PROC_ERROR;
		return false;
	}

	return true;
}

static void emul_respond(uint8_t *miso, size_t len, size_t offset, const uint8_t *src, size_t n)
{
	for (size_t i = 0; i < n && offset + i < len; i++) {
		miso[offset + i] = src[i];
	}
}

static void emul_set_pa_config(const struct emul *target, const uint8_t *p)
{
	const struct sx126x_emul_cfg *cfg = target->cfg;
	struct sx126x_emul_data *data = target->data;
	uint8_t duty_cycle = p[0];
	uint8_t hp_max = p[1];
	uint8_t device_sel = p[2];

	if (p[3] != SX126X_PA_LUT) {
		EMUL_ERROR(target, "paLut must be 0x01, got 0x%02x", p[3]);
	}

	if (cfg->variant == SX126X_SX1261) {
		if (device_sel != SX126X_DEVICE_SEL_SX1261) {
			EMUL_ERROR(target, "SX1261 needs deviceSel 1, got %u", device_sel);
		}
		if (duty_cycle > 0x07) {
			EMUL_ERROR(target, "SX1261 paDutyCycle above 0x07: 0x%02x", duty_cycle);
		}
	} else {
		if (device_sel != SX126X_DEVICE_SEL_SX1262) {
			EMUL_ERROR(target, "High power PA needs deviceSel 0, got %u", device_sel);
		}
		/* Datasheet 13.1.14: higher values can damage the PA */
		if (duty_cycle > 0x04) {
			EMUL_ERROR(target, "paDutyCycle above 0x04: 0x%02x", duty_cycle);
		}
		if (hp_max > 0x07) {
			EMUL_ERROR(target, "hpMax above 0x07: 0x%02x", hp_max);
		}
	}

	memcpy(data->pa_config, p, sizeof(data->pa_config));
	data->pa_config_set = true;
	/* Datasheet 5.1: every SetPaConfig reloads the OCP default for the PA */
	data->regs[SX126X_REG_OCP] =
		device_sel == SX126X_DEVICE_SEL_SX1261 ? EMUL_OCP_LOW_PA : EMUL_OCP_HP_PA;
}

static void emul_set_tx_params(const struct emul *target, const uint8_t *p)
{
	const struct sx126x_emul_cfg *cfg = target->cfg;
	struct sx126x_emul_data *data = target->data;
	int8_t power = (int8_t)p[0];
	int8_t min = cfg->variant == SX126X_SX1261 ? SX1261_MIN_POWER : SX1262_MIN_POWER;
	int8_t max = cfg->variant == SX126X_SX1261 ? SX1261_MAX_POWER_TX_PARAM : SX1262_MAX_POWER;

	if (power < min || power > max) {
		EMUL_ERROR(target, "TX power %d dBm outside %d to %d", power, min, max);
	}

	if (p[1] > SX126X_RAMP_3400_US) {
		EMUL_ERROR(target, "Invalid ramp time 0x%02x", p[1]);
	}

	data->tx_power = power;
	data->ramp_time = p[1];
	data->tx_params_set = true;
}

static void emul_set_packet_type(const struct emul *target, uint8_t type)
{
	struct sx126x_emul_data *data = target->data;

	/* Datasheet 13.4.2 */
	if (data->mode != SX126X_EMUL_MODE_STDBY_RC) {
		EMUL_ERROR(target, "SetPacketType outside STDBY_RC (mode %d)", data->mode);
	}

	if (type != SX126X_PACKET_TYPE_GFSK && type != SX126X_PACKET_TYPE_LORA) {
		EMUL_ERROR(target, "Unsupported packet type 0x%02x", type);
		return;
	}

	/* Modulation and packet parameters follow the packet type (datasheet 13.4.2) */
	data->packet_type = type;
	data->packet_type_set = true;
	data->mod_params_set = false;
	data->pkt_params_set = false;
}

static void emul_set_params(const struct emul *target, uint8_t opcode, const uint8_t *p, size_t n)
{
	struct sx126x_emul_data *data = target->data;
	bool lora = data->packet_type == SX126X_PACKET_TYPE_LORA;
	bool mod = opcode == SX126X_CMD_SET_MODULATION_PARAMS;
	uint8_t *dst = mod ? data->mod_params : data->pkt_params;
	size_t size = mod ? sizeof(data->mod_params) : sizeof(data->pkt_params);
	size_t want = mod ? (lora ? EMUL_LORA_MOD_PARAMS : EMUL_GFSK_MOD_PARAMS)
			  : (lora ? EMUL_LORA_PKT_PARAMS : EMUL_GFSK_PKT_PARAMS);

	if (!data->packet_type_set) {
		EMUL_ERROR(target, "Opcode 0x%02x before SetPacketType", opcode);
	} else if (!mod && !data->mod_params_set) {
		/* Datasheet 14.5 */
		EMUL_ERROR(target, "SetPacketParams before SetModulationParams");
	}

	/* Trailing parameter bytes beyond the modem's are ignored by the radio */
	if (n < want || n > size) {
		EMUL_ERROR(target, "Opcode 0x%02x takes %zu parameter bytes for %s, got %zu",
			   opcode, want, lora ? "LoRa" : "GFSK", n);
		data->cmd_status = SX126X_CMD_STATUS_CMD_PROC_ERROR;
		return;
	}

	memset(dst, 0, size);
	memcpy(dst, p, want);
	if (mod) {
		data->mod_params_set = true;
	} else {
		data->pkt_params_set = true;
	}
}

/* Datasheet 13.3.2.1: an IRQ is not generated on a DIO that has another role */
static void emul_check_dio_masks(const struct emul *target)
{
	struct sx126x_emul_data *data = target->data;

	if (data->dio2_rf_switch && data->dio2_mask != 0) {
		EMUL_ERROR(target, "IRQs 0x%04x mapped to DIO2, which drives the RF switch",
			   data->dio2_mask);
	}
	if (data->tcxo && data->dio3_mask != 0) {
		EMUL_ERROR(target, "IRQs 0x%04x mapped to DIO3, which drives the TCXO",
			   data->dio3_mask);
	}
}

/*
 * Execute a command and return how long BUSY stays high for it. Reads are
 * answered without the internal state machine and leave BUSY low
 * (datasheet 8.3.1).
 */
static uint32_t emul_command(const struct emul *target, const uint8_t *mosi, uint8_t *miso,
			     size_t len)
{
	struct sx126x_emul_data *data = target->data;
	uint8_t opcode = mosi[0];
	const uint8_t *p = &mosi[1];
	size_t n = len - 1;
	uint8_t resp[6] = {0};
	uint32_t busy_us = EMUL_BUSY_CMD_US;
	uint16_t addr;

	data->stats.commands++;

	switch (opcode) {
	case SX126X_CMD_SET_SLEEP:
		if (emul_params(target, opcode, n, 1)) {
			/* Datasheet 13.1.1 */
			if (!emul_is_standby(data)) {
				EMUL_ERROR(target, "SetSleep outside standby (mode %d)",
					   data->mode);
			}
			emul_sleep(target, p[0]);
		}
		/* BUSY stays high for as long as the radio sleeps */
		busy_us = 0;
		break;
	case SX126X_CMD_SET_STANDBY:
		if (emul_params(target, opcode, n, 1)) {
			if (p[0] > SX126X_STANDBY_XOSC) {
				EMUL_ERROR(target, "Invalid standby mode %u", p[0]);
				break;
			}
			emul_works_cancel(data);
			busy_us = emul_enter_mode(target, p[0] == SX126X_STANDBY_XOSC
								  ? SX126X_EMUL_MODE_STDBY_XOSC
								  : SX126X_EMUL_MODE_STDBY_RC);
		}
		break;
	case SX126X_CMD_SET_FS:
		if (emul_params(target, opcode, n, 0)) {
			emul_works_cancel(data);
			busy_us = emul_enter_mode(target, SX126X_EMUL_MODE_FS);
		}
		break;
	case SX126X_CMD_SET_TX:
		if (emul_params(target, opcode, n, 3)) {
			busy_us = emul_start_tx(target, sys_get_be24(p));
		}
		break;
	case SX126X_CMD_SET_RX:
		if (emul_params(target, opcode, n, 3)) {
			busy_us = emul_start_rx(target, sys_get_be24(p), false, 0, 0);
		}
		break;
	case SX126X_CMD_SET_RX_DUTY_CYCLE:
		if (emul_params(target, opcode, n, 6)) {
			if (sys_get_be24(&p[0]) == 0 || sys_get_be24(&p[3]) == 0) {
				/* Not started: the cycle would spin the workqueue */
				EMUL_ERROR(target, "RX duty cycle with a zero period");
				break;
			}
			busy_us = emul_start_rx(target, 0, true, sys_get_be24(&p[0]),
						sys_get_be24(&p[3]));
		}
		break;
	case SX126X_CMD_STOP_TIMER_ON_PREAMBLE:
		if (emul_params(target, opcode, n, 1)) {
			data->stop_on_preamble = p[0] != 0;
		}
		break;
	case SX126X_CMD_SET_TX_CONTINUOUS_WAVE:
	case SX126X_CMD_SET_TX_INFINITE_PREAMBLE:
		if (emul_params(target, opcode, n, 0)) {
			if (!data->freq_set || !data->tx_params_set || !data->pa_config_set) {
				EMUL_ERROR(target, "Opcode 0x%02x without frequency and PA setup",
					   opcode);
			}
			emul_check_pa(target);
			emul_works_cancel(data);
			busy_us = emul_enter_mode(target, SX126X_EMUL_MODE_TX);
		}
		break;
	case SX126X_CMD_SET_REGULATOR_MODE:
		if (emul_params(target, opcode, n, 1)) {
			/* Datasheet 5.1 */
			if (data->mode != SX126X_EMUL_MODE_STDBY_RC) {
				EMUL_ERROR(target, "SetRegulatorMode outside STDBY_RC (mode %d)",
					   data->mode);
			}
			data->dcdc = p[0] == SX126X_REGULATOR_DCDC;
		}
		break;
	case SX126X_CMD_CALIBRATE:
		if (emul_params(target, opcode, n, 1)) {
			if (data->mode != SX126X_EMUL_MODE_STDBY_RC) {
				EMUL_ERROR(target, "Calibrate outside STDBY_RC (mode %d)",
					   data->mode);
			}
			if ((p[0] & SX126X_CALIBRATE_IMAGE) != 0) {
				data->image_cal_valid = emul_clock_ok(target);
			}
		}
		busy_us = EMUL_BUSY_CALIBRATE_US;
		break;
	case SX126X_CMD_CALIBRATE_IMAGE:
		if (emul_params(target, opcode, n, 2)) {
			/* Calibration runs from STDBY_RC (datasheet 13.1.12) */
			if (data->mode != SX126X_EMUL_MODE_STDBY_RC) {
				EMUL_ERROR(target, "CalibrateImage outside STDBY_RC (mode %d)",
					   data->mode);
			}
			if (p[0] > p[1]) {
				EMUL_ERROR(target, "Image band 0x%02x-0x%02x reversed", p[0], p[1]);
			}
			data->image_cal[0] = p[0];
			data->image_cal[1] = p[1];
			data->image_cal_valid = emul_clock_ok(target);
		}
		break;
	case SX126X_CMD_SET_PA_CONFIG:
		if (emul_params(target, opcode, n, 4)) {
			emul_set_pa_config(target, p);
		}
		break;
	case SX126X_CMD_SET_RX_TX_FALLBACK_MODE:
		if (emul_params(target, opcode, n, 1)) {
			if (p[0] != EMUL_FALLBACK_FS && p[0] != EMUL_FALLBACK_STDBY_XOSC &&
			    p[0] != EMUL_FALLBACK_STDBY_RC) {
				EMUL_ERROR(target, "Invalid fallback mode 0x%02x", p[0]);
				break;
			}
			data->fallback = p[0];
		}
		break;
	case SX126X_CMD_WRITE_REGISTER:
		if (n < 2) {
			EMUL_ERROR(target, "WriteRegister without an address");
			break;
		}
		addr = sys_get_be16(p);
		if ((size_t)addr + (n - 2) > EMUL_NUM_REGS) {
			EMUL_ERROR(target, "WriteRegister 0x%04x+%zu out of range", addr, n - 2);
			break;
		}
		/* Table 12-1: the trim only takes effect once the crystal runs */
		if (addr <= EMUL_REG_XTB_TRIM && (size_t)addr + (n - 2) > EMUL_REG_XTA_TRIM &&
		    data->mode != SX126X_EMUL_MODE_STDBY_XOSC) {
			EMUL_ERROR(target, "Crystal trim written outside STDBY_XOSC (mode %d)",
				   data->mode);
		}
		/* Datasheet 15.3: stopping the RTC disarms a stale RX timeout */
		if (addr <= EMUL_REG_RTC_CTRL && (size_t)addr + (n - 2) > EMUL_REG_RTC_CTRL &&
		    p[2 + EMUL_REG_RTC_CTRL - addr] == 0) {
			(void)k_work_cancel_delayable(&data->rtc_work);
		}
		memcpy(&data->regs[addr], &p[2], n - 2);
		break;
	case SX126X_CMD_READ_REGISTER:
		if (n < 3) {
			EMUL_ERROR(target, "ReadRegister without an address");
			break;
		}
		addr = sys_get_be16(p);
		if ((size_t)addr + (len - 4) > EMUL_NUM_REGS) {
			EMUL_ERROR(target, "ReadRegister 0x%04x+%zu out of range", addr, len - 4);
			break;
		}
		emul_respond(miso, len, 4, &data->regs[addr], len - 4);
		busy_us = 0;
		break;
	case SX126X_CMD_WRITE_BUFFER:
		if (n < 1) {
			EMUL_ERROR(target, "WriteBuffer without an offset");
			break;
		}
		/* The buffer address wraps around */
		for (size_t i = 0; i < n - 1; i++) {
			data->buffer[(uint8_t)(p[0] + i)] = p[1 + i];
		}
		break;
	case SX126X_CMD_READ_BUFFER:
		if (n < 2) {
			EMUL_ERROR(target, "ReadBuffer without an offset");
			break;
		}
		for (size_t i = 3; i < len; i++) {
			miso[i] = data->buffer[(uint8_t)(p[0] + i - 3)];
		}
		busy_us = 0;
		break;
	case SX126X_CMD_SET_DIO_IRQ_PARAMS:
		if (emul_params(target, opcode, n, 8)) {
			data->irq_mask = sys_get_be16(&p[0]);
			data->dio1_mask = sys_get_be16(&p[2]);
			data->dio2_mask = sys_get_be16(&p[4]);
			data->dio3_mask = sys_get_be16(&p[6]);
			emul_check_dio_masks(target);
		}
		break;
	case SX126X_CMD_GET_IRQ_STATUS:
		sys_put_be16(data->irq_status, resp);
		emul_respond(miso, len, 2, resp, 2);
		busy_us = 0;
		break;
	case SX126X_CMD_CLR_IRQ_STATUS:
		if (emul_params(target, opcode, n, 2)) {
			data->irq_status &= ~sys_get_be16(p);
		}
		break;
	case SX126X_CMD_SET_DIO2_AS_RF_SWITCH:
		if (emul_params(target, opcode, n, 1)) {
			data->dio2_rf_switch = p[0] != 0;
			emul_check_dio_masks(target);
		}
		break;
	case SX126X_CMD_SET_DIO3_AS_TCXO_CTRL:
		if (emul_params(target, opcode, n, 4)) {
			if (p[0] > 0x07) {
				EMUL_ERROR(target, "Invalid TCXO voltage 0x%02x", p[0]);
			}
			data->tcxo = true;
			data->tcxo_voltage = p[0];
			data->tcxo_delay = sys_get_be24(&p[1]);
			/* Datasheet 4.1.3 */
			data->regs[EMUL_REG_XTA_TRIM] = EMUL_XTA_TRIM_TCXO;
			emul_check_dio_masks(target);
		}
		break;
	case SX126X_CMD_SET_RF_FREQUENCY:
		if (emul_params(target, opcode, n, 4)) {
			uint32_t freq;

			data->freq_reg = sys_get_be32(p);
			data->freq_set = true;
			freq = emul_freq_hz(data->freq_reg);
			if (freq < EMUL_FREQ_MIN_HZ || freq > EMUL_FREQ_MAX_HZ) {
				EMUL_ERROR(target, "Frequency %u Hz out of range", freq);
			}
		}
		break;
	case SX126X_CMD_SET_PACKET_TYPE:
		if (emul_params(target, opcode, n, 1)) {
			emul_set_packet_type(target, p[0]);
		}
		break;
	case SX126X_CMD_GET_PACKET_TYPE:
		emul_respond(miso, len, 2, &data->packet_type, 1);
		busy_us = 0;
		break;
	case SX126X_CMD_SET_TX_PARAMS:
		if (emul_params(target, opcode, n, 2)) {
			emul_set_tx_params(target, p);
		}
		break;
	case SX126X_CMD_SET_MODULATION_PARAMS:
	case SX126X_CMD_SET_PACKET_PARAMS:
		emul_set_params(target, opcode, p, n);
		break;
	case SX126X_CMD_SET_BUFFER_BASE_ADDRESS:
		if (emul_params(target, opcode, n, 2)) {
			data->tx_base = p[0];
			data->rx_base = p[1];
		}
		break;
	case SX126X_CMD_GET_STATUS:
		busy_us = 0;
		break;
	case SX126X_CMD_GET_RSSI_INST:
		/* Datasheet 13.5.4: only meaningful while receiving */
		if (!emul_is_receiving(data)) {
			EMUL_ERROR(target, "GetRssiInst outside RX (mode %d)", data->mode);
		}
		resp[0] = (uint8_t)CLAMP(-2 * (int32_t)data->rssi_inst, 0, UINT8_MAX);
		emul_respond(miso, len, 2, resp, 1);
		busy_us = 0;
		break;
	case SX126X_CMD_GET_RX_BUFFER_STATUS:
		resp[0] = data->rx_len;
		resp[1] = data->rx_offset;
		emul_respond(miso, len, 2, resp, 2);
		busy_us = 0;
		break;
	case SX126X_CMD_GET_PACKET_STATUS:
		emul_respond(miso, len, 2, data->pkt_status, sizeof(data->pkt_status));
		busy_us = 0;
		break;
	case SX126X_CMD_GET_DEVICE_ERRORS:
		sys_put_be16(data->op_errors, resp);
		emul_respond(miso, len, 2, resp, 2);
		busy_us = 0;
		break;
	case SX126X_CMD_GET_STATS:
		emul_respond(miso, len, 2, resp, sizeof(resp));
		busy_us = 0;
		break;
	case SX126X_CMD_CLR_DEVICE_ERRORS:
		data->op_errors = 0;
		break;
	case SX126X_CMD_RESET_STATS:
		break;
	default:
		EMUL_ERROR(target, "Unsupported opcode 0x%02x", opcode);
		data->cmd_status = SX126X_CMD_STATUS_CMD_PROC_ERROR;
		busy_us = 0;
		break;
	}

	return busy_us;
}

static uint8_t emul_status_byte(const struct sx126x_emul_data *data)
{
	uint8_t mode;

	switch (data->mode) {
	case SX126X_EMUL_MODE_STDBY_XOSC:
		mode = SX126X_CHIP_MODE_STBY_XOSC;
		break;
	case SX126X_EMUL_MODE_FS:
		mode = SX126X_CHIP_MODE_FS;
		break;
	case SX126X_EMUL_MODE_TX:
		mode = SX126X_CHIP_MODE_TX;
		break;
	case SX126X_EMUL_MODE_RX:
	case SX126X_EMUL_MODE_RX_DUTY_CYCLE:
		mode = SX126X_CHIP_MODE_RX;
		break;
	default:
		mode = SX126X_CHIP_MODE_STBY_RC;
		break;
	}

	return (mode << EMUL_STATUS_MODE_SHIFT) | (data->cmd_status << EMUL_STATUS_CMD_SHIFT);
}

static void emul_transaction(const struct emul *target, const uint8_t *mosi, uint8_t *miso,
			     size_t len)
{
	struct sx126x_emul_data *data = target->data;

	LOG_DBG("%s: opcode 0x%02x, %zu bytes, mode %d", target->dev->name, mosi[0], len,
		data->mode);

	uint32_t busy_us;

	if (data->mode == SX126X_EMUL_MODE_RESET) {
		memset(miso, 0, len);
		EMUL_ERROR(target, "SPI access while held in reset");
		return;
	}

	if (emul_is_asleep(data)) {
		bool warm = data->mode != SX126X_EMUL_MODE_SLEEP || data->sleep_warm;

		if (data->mode == SX126X_EMUL_MODE_SLEEP && emul_sleep_settling(data)) {
			EMUL_ERROR(target, "NSS fell within %u us of SetSleep",
				   EMUL_SLEEP_SETTLE_US);
		}

		/*
		 * The NSS falling edge wakes the radio into STDBY_RC. The
		 * command itself is not executed, so anything other than
		 * the GetStatus a host sends to wake the radio is lost.
		 */
		memset(miso, 0, len);
		emul_works_cancel(data);
		data->mode = SX126X_EMUL_MODE_STDBY_RC;
		data->stats.wakeups++;
		/* Datasheet Table 8-2 */
		emul_busy_start(data, warm ? EMUL_BUSY_WARM_START_US : EMUL_BUSY_COLD_START_US);
		if (mosi[0] != SX126X_CMD_GET_STATUS) {
			EMUL_ERROR(target, "Opcode 0x%02x sent while BUSY, lost waking the radio",
				   mosi[0]);
		}
		return;
	}

	/* Counted rather than rejected: what the radio would do is undefined */
	if (emul_busy_pending(data)) {
		EMUL_ERROR(target, "Opcode 0x%02x sent while BUSY", mosi[0]);
	}

	/* The status the radio reports clears once it has been clocked out */
	memset(miso, emul_status_byte(data), len);
	data->cmd_status = SX126X_CMD_STATUS_RESERVED;
	busy_us = emul_command(target, mosi, miso, len);
	if (busy_us > 0) {
		emul_busy_start(data, busy_us);
	}

	if (data->cmd_cb != NULL) {
		emul_raise_irq(data, data->cmd_cb(target, mosi[0], &mosi[1], len - 1,
						  data->cmd_cb_user_data));
	}
}

static void emul_spi_config_check(const struct emul *target, const struct spi_config *config)
{
	if (config == NULL) {
		return;
	}

	if ((SPI_MODE_GET(config->operation) & (SPI_MODE_CPOL | SPI_MODE_CPHA)) != 0) {
		EMUL_ERROR(target, "SPI must use CPOL=0, CPHA=0");
	}

	if (SPI_WORD_SIZE_GET(config->operation) != 8) {
		EMUL_ERROR(target, "SPI word size must be 8 bits");
	}

	if ((config->operation & SPI_TRANSFER_LSB) != 0) {
		EMUL_ERROR(target, "SPI must transfer MSB first");
	}

	if (config->frequency > EMUL_SPI_MAX_FREQ) {
		EMUL_ERROR(target, "SPI clock %u Hz above %u Hz", config->frequency,
			   EMUL_SPI_MAX_FREQ);
	}
}

static int sx126x_emul_io(const struct emul *target, const struct spi_config *config,
			  const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs)
{
	struct sx126x_emul_data *data = target->data;
	uint8_t mosi[EMUL_XFER_MAX] = {0};
	uint8_t miso[EMUL_XFER_MAX];
	size_t tx_len = 0;
	size_t rx_len = 0;
	size_t len;
	size_t pos;

	if (tx_bufs != NULL) {
		for (size_t i = 0; i < tx_bufs->count; i++) {
			const struct spi_buf *buf = &tx_bufs->buffers[i];

			if (tx_len + buf->len > sizeof(mosi)) {
				LOG_ERR("Transfer too long");
				return -EIO;
			}
			/* A missing buffer clocks out NOPs */
			if (buf->buf != NULL) {
				memcpy(&mosi[tx_len], buf->buf, buf->len);
			}
			tx_len += buf->len;
		}
	}

	if (rx_bufs != NULL) {
		for (size_t i = 0; i < rx_bufs->count; i++) {
			rx_len += rx_bufs->buffers[i].len;
		}
	}

	len = MAX(tx_len, rx_len);
	if (len == 0) {
		return 0;
	}
	if (len > sizeof(miso)) {
		LOG_ERR("Transfer too long");
		return -EIO;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	emul_spi_config_check(target, config);
	emul_transaction(target, mosi, miso, len);
	emul_lines_sync(target);
	k_mutex_unlock(&data->lock);

	if (rx_bufs != NULL) {
		pos = 0;
		for (size_t i = 0; i < rx_bufs->count; i++) {
			const struct spi_buf *buf = &rx_bufs->buffers[i];

			if (buf->buf != NULL) {
				memcpy(buf->buf, &miso[pos], buf->len);
			}
			pos += buf->len;
		}
	}

	return 0;
}

static void emul_reset_line_handler(const struct device *port, struct gpio_callback *cb,
				    gpio_port_pins_t pins)
{
	struct sx126x_emul_data *data = CONTAINER_OF(cb, struct sx126x_emul_data, reset_cb);
	const struct emul *target = data->target;
	const struct sx126x_emul_cfg *cfg = target->cfg;
	gpio_flags_t flags;
	bool asserted;

	ARG_UNUSED(pins);

	/* NRESET has an internal pull-up: only a driven low level resets the radio */
	asserted = gpio_emul_flags_get(port, cfg->reset.pin, &flags) == 0 &&
		   (flags & GPIO_OUTPUT) != 0 && gpio_emul_output_get(port, cfg->reset.pin) == 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (asserted && data->mode != SX126X_EMUL_MODE_RESET) {
		emul_works_cancel(data);
		(void)k_work_cancel_delayable(&data->rtc_work);
		data->mode = SX126X_EMUL_MODE_RESET;
		data->stats.resets++;
	} else if (!asserted && data->mode == SX126X_EMUL_MODE_RESET) {
		emul_power_on(target);
		/* Startup and calibration (datasheet 9.1, Table 8-2) */
		emul_busy_start(data, EMUL_BUSY_COLD_START_US);
	}

	emul_lines_sync(target);
	k_mutex_unlock(&data->lock);
}

/*
 * gpio_emul fires these when the host reconfigures BUSY or DIO1 as an input,
 * which is when the level the radio holds can first be driven onto them.
 */
static void emul_busy_line_handler(const struct device *port, struct gpio_callback *cb,
				   gpio_port_pins_t pins)
{
	struct sx126x_emul_data *data = CONTAINER_OF(cb, struct sx126x_emul_data, busy_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	k_mutex_lock(&data->lock, K_FOREVER);
	emul_lines_sync(data->target);
	k_mutex_unlock(&data->lock);
}

static void emul_dio1_line_handler(const struct device *port, struct gpio_callback *cb,
				   gpio_port_pins_t pins)
{
	struct sx126x_emul_data *data = CONTAINER_OF(cb, struct sx126x_emul_data, dio1_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	k_mutex_lock(&data->lock, K_FOREVER);
	emul_lines_sync(data->target);
	k_mutex_unlock(&data->lock);
}

static int emul_line_cb_add(const struct gpio_dt_spec *spec, struct gpio_callback *cb,
			    gpio_callback_handler_t handler)
{
	if (!device_is_ready(spec->port)) {
		return -ENODEV;
	}

	gpio_init_callback(cb, handler, BIT(spec->pin));

	return gpio_add_callback(spec->port, cb);
}

static int sx126x_emul_init(const struct emul *target, const struct device *parent)
{
	const struct sx126x_emul_cfg *cfg = target->cfg;
	struct sx126x_emul_data *data = target->data;
	int ret;

	ARG_UNUSED(parent);

	data->target = target;
	k_mutex_init(&data->lock);
	k_work_init_delayable(&data->tx_work, emul_tx_work_handler);
	k_work_init_delayable(&data->rx_work, emul_rx_work_handler);
	k_work_init_delayable(&data->timeout_work, emul_timeout_work_handler);
	k_work_init_delayable(&data->dc_work, emul_dc_work_handler);
	k_work_init_delayable(&data->rtc_work, emul_rtc_work_handler);
	k_timer_init(&data->busy_timer, emul_busy_expiry, NULL);
	data->tx_duration = EMUL_TX_DURATION_DEFAULT;
	data->rssi_inst = -127;

	emul_power_on(target);

	ret = emul_line_cb_add(&cfg->reset, &data->reset_cb, emul_reset_line_handler);
	if (ret < 0) {
		return ret;
	}

	ret = emul_line_cb_add(&cfg->busy, &data->busy_cb, emul_busy_line_handler);
	if (ret < 0) {
		return ret;
	}

	return emul_line_cb_add(&cfg->dio1, &data->dio1_cb, emul_dio1_line_handler);
}

enum sx126x_emul_mode sx126x_emul_mode_get(const struct emul *target)
{
	struct sx126x_emul_data *data = target->data;
	enum sx126x_emul_mode mode;

	k_mutex_lock(&data->lock, K_FOREVER);
	mode = data->mode;
	k_mutex_unlock(&data->lock);

	return mode;
}

uint32_t sx126x_emul_errors_get(const struct emul *target)
{
	struct sx126x_emul_data *data = target->data;
	uint32_t errors;

	k_mutex_lock(&data->lock, K_FOREVER);
	errors = data->errors;
	k_mutex_unlock(&data->lock);

	return errors;
}

void sx126x_emul_errors_clear(const struct emul *target)
{
	struct sx126x_emul_data *data = target->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	data->errors = 0;
	k_mutex_unlock(&data->lock);
}

void sx126x_emul_stats_get(const struct emul *target, struct sx126x_emul_stats *stats)
{
	struct sx126x_emul_data *data = target->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	*stats = data->stats;
	k_mutex_unlock(&data->lock);
}

void sx126x_emul_stats_clear(const struct emul *target)
{
	struct sx126x_emul_data *data = target->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	memset(&data->stats, 0, sizeof(data->stats));
	k_mutex_unlock(&data->lock);
}

void sx126x_emul_radio_get(const struct emul *target, struct sx126x_emul_radio *radio)
{
	struct sx126x_emul_data *data = target->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	emul_radio_fill(data, radio);
	k_mutex_unlock(&data->lock);
}

int sx126x_emul_last_tx_get(const struct emul *target, struct sx126x_emul_tx_packet *pkt)
{
	struct sx126x_emul_data *data = target->data;
	int ret = -ENODATA;

	k_mutex_lock(&data->lock, K_FOREVER);
	if (data->last_tx_valid) {
		*pkt = data->last_tx;
		ret = 0;
	}
	k_mutex_unlock(&data->lock);

	return ret;
}

int sx126x_emul_last_rx_get(const struct emul *target, struct sx126x_emul_radio *radio)
{
	struct sx126x_emul_data *data = target->data;
	int ret = -ENODATA;

	k_mutex_lock(&data->lock, K_FOREVER);
	if (data->last_rx_valid) {
		*radio = data->last_rx;
		ret = 0;
	}
	k_mutex_unlock(&data->lock);

	return ret;
}

int sx126x_emul_rx_packet_set(const struct emul *target, const struct sx126x_emul_rx_packet *pkt)
{
	struct sx126x_emul_data *data = target->data;

	if (pkt->len > SX126X_EMUL_MAX_PAYLOAD) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	data->rx_pending = *pkt;
	data->rx_pending_valid = true;
	if (emul_is_receiving(data)) {
		k_timeout_t delay = data->mode == SX126X_EMUL_MODE_RX ? EMUL_RX_DELAY : K_NO_WAIT;

		/* The new packet replaces one still being detected */
		data->rx_detecting = false;
		(void)k_work_reschedule(&data->rx_work, delay);
	}
	k_mutex_unlock(&data->lock);

	return 0;
}

void sx126x_emul_rssi_set(const struct emul *target, int16_t rssi)
{
	struct sx126x_emul_data *data = target->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	data->rssi_inst = rssi;
	k_mutex_unlock(&data->lock);
}

void sx126x_emul_tx_duration_set(const struct emul *target, k_timeout_t duration)
{
	struct sx126x_emul_data *data = target->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	data->tx_duration = duration;
	k_mutex_unlock(&data->lock);
}

void sx126x_emul_tx_cb_set(const struct emul *target, sx126x_emul_tx_cb_t cb, void *user_data)
{
	struct sx126x_emul_data *data = target->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	data->tx_cb = cb;
	data->tx_cb_user_data = user_data;
	k_mutex_unlock(&data->lock);
}

void sx126x_emul_cmd_cb_set(const struct emul *target, sx126x_emul_cmd_cb_t cb, void *user_data)
{
	struct sx126x_emul_data *data = target->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	data->cmd_cb = cb;
	data->cmd_cb_user_data = user_data;
	k_mutex_unlock(&data->lock);
}

int sx126x_emul_reg_read(const struct emul *target, uint16_t addr, uint8_t *buf, size_t len)
{
	struct sx126x_emul_data *data = target->data;

	if ((size_t)addr + len > EMUL_NUM_REGS) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	memcpy(buf, &data->regs[addr], len);
	k_mutex_unlock(&data->lock);

	return 0;
}

static const struct spi_emul_api sx126x_emul_api = {
	.io = sx126x_emul_io,
};

#define SX126X_EMUL_DEFINE(node_id, variant_in)                                                    \
	static struct sx126x_emul_data sx126x_emul_data_##node_id;                                 \
	static const struct sx126x_emul_cfg sx126x_emul_cfg_##node_id = {                          \
		.reset = GPIO_DT_SPEC_GET(node_id, reset_gpios),                                   \
		.busy = GPIO_DT_SPEC_GET(node_id, busy_gpios),                                     \
		.dio1 = GPIO_DT_SPEC_GET(node_id, dio1_gpios),                                     \
		.variant = variant_in,                                                             \
		.tcxo_fitted = DT_NODE_HAS_PROP(node_id, dio3_tcxo_voltage),                       \
	};                                                                                         \
	EMUL_DT_DEFINE(node_id, sx126x_emul_init, &sx126x_emul_data_##node_id,                     \
		       &sx126x_emul_cfg_##node_id, &sx126x_emul_api, NULL);

DT_FOREACH_STATUS_OKAY_VARGS(semtech_sx1262, SX126X_EMUL_DEFINE, SX126X_SX1262)
DT_FOREACH_STATUS_OKAY_VARGS(semtech_sx1261, SX126X_EMUL_DEFINE, SX126X_SX1261)
DT_FOREACH_STATUS_OKAY_VARGS(semtech_sx1268, SX126X_EMUL_DEFINE, SX126X_SX1268)
DT_FOREACH_STATUS_OKAY_VARGS(semtech_llcc68, SX126X_EMUL_DEFINE, SX126X_LLCC68)
