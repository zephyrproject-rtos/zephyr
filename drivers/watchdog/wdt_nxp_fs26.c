/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_fs26_wdog

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/sys/byteorder.h>

#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt_nxp_fs26);

#include "wdt_nxp_fs26.h"

#if defined(CONFIG_BIG_ENDIAN)
#define SWAP_ENDIANNESS
#endif

#define FS26_CRC_TABLE_SIZE 256U
#define FS26_CRC_INIT 0xff
#define FS26_FS_WD_TOKEN_DEFAULT 0x5ab2
#define FS26_INIT_FS_TIMEOUT_MS 1000U

/* Helper macros to set register values from Kconfig options */
#define WD_ERR_LIMIT(x)	CONCAT(WD_ERR_LIMIT_, x)
#define WD_RFR_LIMIT(x)	CONCAT(WD_RFR_LIMIT_, x)
#define WDW_PERIOD(x)	CONCAT(CONCAT(WDW_PERIOD_, x), MS)

#define BAD_WD_REFRESH_ERROR_STRING(x)					\
	((((x) & BAD_WD_DATA) ? "error in the data" :			\
		(((x) & BAD_WD_TIMING) ? "error in the timing (window)"	\
		: "unknown error")))

enum fs26_wd_type {
	FS26_WD_SIMPLE,
	FS26_WD_CHALLENGER
};

struct fs26_spi_rx_frame {
	union {
		struct {
			uint8_t m_aval : 1;
			uint8_t fs_en  : 1;
			uint8_t fs_g   : 1;
			uint8_t com_g  : 1;
			uint8_t wio_g  : 1;
			uint8_t vsup_g : 1;
			uint8_t reg_g  : 1;
			uint8_t tsd_g  : 1;
		};
		uint8_t raw;
	} status;
	uint16_t data;
};

struct fs26_spi_tx_frame {
	bool write;
	uint8_t addr;
	uint16_t data;
};

struct wdt_nxp_fs26_config {
	struct spi_dt_spec spi;
	enum fs26_wd_type wd_type;
	struct gpio_dt_spec int_gpio;
};

struct wdt_nxp_fs26_data {
	wdt_callback_t callback;
	uint16_t token;	/* local copy of the watchdog token */
	bool timeout_installed;
	uint8_t window_period;
	uint8_t window_duty_cycle;
	uint8_t fs_reaction;
	struct gpio_callback int_gpio_cb;
	struct k_sem int_sem;
	struct k_thread int_thread;

	K_KERNEL_STACK_MEMBER(int_thread_stack, CONFIG_WDT_NXP_FS26_INT_THREAD_STACK_SIZE);
};

/*
 * Allowed values for watchdog period and duty cycle (CLOSED window).
 * The index is the value to write to the register. Keep values in ascending order.
 */
static const uint32_t fs26_period_values[] = {
	0, 1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 64, 128, 256, 512, 1024
};

static const double fs26_dc_closed_values[] = {
	0.3125, 0.375, 0.5, 0.625, 0.6875, 0.75, 0.8125
};

/* CRC lookup table */
static const uint8_t FS26_CRC_TABLE[FS26_CRC_TABLE_SIZE] = {
	0x00u, 0x1du, 0x3au, 0x27u, 0x74u, 0x69u, 0x4eu, 0x53u, 0xe8u,
	0xf5u, 0xd2u, 0xcfu, 0x9cu, 0x81u, 0xa6u, 0xbbu, 0xcdu, 0xd0u,
	0xf7u, 0xeau, 0xb9u, 0xa4u, 0x83u, 0x9eu, 0x25u, 0x38u, 0x1fu,
	0x02u, 0x51u, 0x4cu, 0x6bu, 0x76u, 0x87u, 0x9au, 0xbdu, 0xa0u,
	0xf3u, 0xeeu, 0xc9u, 0xd4u, 0x6fu, 0x72u, 0x55u, 0x48u, 0x1bu,
	0x06u, 0x21u, 0x3cu, 0x4au, 0x57u, 0x70u, 0x6du, 0x3eu, 0x23u,
	0x04u, 0x19u, 0xa2u, 0xbfu, 0x98u, 0x85u, 0xd6u, 0xcbu, 0xecu,
	0xf1u, 0x13u, 0x0eu, 0x29u, 0x34u, 0x67u, 0x7au, 0x5du, 0x40u,
	0xfbu, 0xe6u, 0xc1u, 0xdcu, 0x8fu, 0x92u, 0xb5u, 0xa8u, 0xdeu,
	0xc3u, 0xe4u, 0xf9u, 0xaau, 0xb7u, 0x90u, 0x8du, 0x36u, 0x2bu,
	0x0cu, 0x11u, 0x42u, 0x5fu, 0x78u, 0x65u, 0x94u, 0x89u, 0xaeu,
	0xb3u, 0xe0u, 0xfdu, 0xdau, 0xc7u, 0x7cu, 0x61u, 0x46u, 0x5bu,
	0x08u, 0x15u, 0x32u, 0x2fu, 0x59u, 0x44u, 0x63u, 0x7eu, 0x2du,
	0x30u, 0x17u, 0x0au, 0xb1u, 0xacu, 0x8bu, 0x96u, 0xc5u, 0xd8u,
	0xffu, 0xe2u, 0x26u, 0x3bu, 0x1cu, 0x01u, 0x52u, 0x4fu, 0x68u,
	0x75u, 0xceu, 0xd3u, 0xf4u, 0xe9u, 0xbau, 0xa7u, 0x80u, 0x9du,
	0xebu, 0xf6u, 0xd1u, 0xccu, 0x9fu, 0x82u, 0xa5u, 0xb8u, 0x03u,
	0x1eu, 0x39u, 0x24u, 0x77u, 0x6au, 0x4du, 0x50u, 0xa1u, 0xbcu,
	0x9bu, 0x86u, 0xd5u, 0xc8u, 0xefu, 0xf2u, 0x49u, 0x54u, 0x73u,
	0x6eu, 0x3du, 0x20u, 0x07u, 0x1au, 0x6cu, 0x71u, 0x56u, 0x4bu,
	0x18u, 0x05u, 0x22u, 0x3fu, 0x84u, 0x99u, 0xbeu, 0xa3u, 0xf0u,
	0xedu, 0xcau, 0xd7u, 0x35u, 0x28u, 0x0fu, 0x12u, 0x41u, 0x5cu,
	0x7bu, 0x66u, 0xddu, 0xc0u, 0xe7u, 0xfau, 0xa9u, 0xb4u, 0x93u,
	0x8eu, 0xf8u, 0xe5u, 0xc2u, 0xdfu, 0x8cu, 0x91u, 0xb6u, 0xabu,
	0x10u, 0x0du, 0x2au, 0x37u, 0x64u, 0x79u, 0x5eu, 0x43u, 0xb2u,
	0xafu, 0x88u, 0x95u, 0xc6u, 0xdbu, 0xfcu, 0xe1u, 0x5au, 0x47u,
	0x60u, 0x7du, 0x2eu, 0x33u, 0x14u, 0x09u, 0x7fu, 0x62u, 0x45u,
	0x58u, 0x0bu, 0x16u, 0x31u, 0x2cu, 0x97u, 0x8au, 0xadu, 0xb0u,
	0xe3u, 0xfeu, 0xd9u, 0xc4u
};

static uint8_t fs26_calcrc(const uint8_t *data, size_t size)
{
	uint8_t crc;
	uint8_t tableidx;
	uint8_t i;

	/* Set CRC token value */
	crc = FS26_CRC_INIT;

	for (i = size; i > 0; i--) {
		tableidx = crc ^ data[i];
		crc = FS26_CRC_TABLE[tableidx];
	}

	return crc;
}

static int fs26_spi_transceive(const struct spi_dt_spec *spi,
			       struct fs26_spi_tx_frame *tx_frame,
			       struct fs26_spi_rx_frame *rx_frame)
{
	uint32_t tx_buf;
	uint32_t rx_buf;
	uint8_t crc;
	int retval;

	struct spi_buf spi_tx_buf = {
		.buf = &tx_buf,
		.len = sizeof(tx_buf)
	};
	struct spi_buf spi_rx_buf = {
		.buf = &rx_buf,
		.len = sizeof(rx_buf)
	};
	struct spi_buf_set spi_tx_set = {
		.buffers = &spi_tx_buf,
		.count = 1U
	};
	struct spi_buf_set spi_rx_set = {
		.buffers = &spi_rx_buf,
		.count = 1U
	};

	/* Create frame to Tx, always for Fail Safe */
	tx_buf = (uint32_t)(FS26_SET_REG_ADDR(tx_frame->addr)
		| FS26_SET_DATA(tx_frame->data)
		| (tx_frame->write ? FS26_RW : 0));

	crc = fs26_calcrc((uint8_t *)&tx_buf, sizeof(tx_buf) - 1);
	tx_buf |= (uint32_t)FS26_SET_CRC(crc);

#if defined(SWAP_ENDIANNESS)
	tx_buf = __builtin_bswap32(tx_buf);
#endif

	retval = spi_transceive_dt(spi, &spi_tx_set, &spi_rx_set);
	if (retval) {
		goto error;
	}

#if defined(SWAP_ENDIANNESS)
	rx_buf = __builtin_bswap32(rx_buf);
#endif

	/* Verify CRC of Rx frame */
	crc = fs26_calcrc((uint8_t *)&rx_buf, sizeof(rx_buf) - 1);
	if (crc != ((uint8_t)FS26_GET_CRC(rx_buf))) {
		LOG_ERR("Rx invalid CRC");
		retval = -EIO;
		goto error;
	}

	if (rx_frame) {
		rx_frame->status.raw = (uint8_t)FS26_GET_DEV_STATUS(rx_buf);
		rx_frame->data = (uint16_t)FS26_GET_DATA(rx_buf);
	}

error:
	return retval;
}

/**
 * @brief Get value of register with address @p addr
 *
 * @param spi SPI specs for interacting with the device
 * @param addr Register address
 * @param rx_frame SPI frame containing read data and device status flags
 *
 * @return 0 on success, error code otherwise
 */
static int fs26_getreg(const struct spi_dt_spec *spi, uint8_t addr,
		       struct fs26_spi_rx_frame *rx_frame)
{
	struct fs26_spi_tx_frame tx_frame = {
		.addr = addr,
		.write = 0,
		.data = 0
	};

	return fs26_spi_transceive(spi, &tx_frame, rx_frame);
}

/**
 * @brief Set @p regval value in register with address @p addr
 *
 * @param spi SPI specs for interacting with the device
 * @param addr Register address
 * @param regval Register value to set
 *
 * @return 0 on success, error code otherwise
 */
static int fs26_setreg(const struct spi_dt_spec *spi, uint8_t addr, uint16_t regval)
{
	struct fs26_spi_tx_frame tx_frame = {
		.addr = addr,
		.write = true,
		.data = regval
	};

	return fs26_spi_transceive(spi, &tx_frame, NULL);
}

/**
 * @brief Calculate watchdog answer based on received token
 *
 * @return answer value to write to FS_WD_ANSWER
 */
static inline uint16_t fs26_wd_compute_answer(uint16_t token)
{
	uint32_t tmp = token;

	tmp *= 4U;
	tmp += 6U;
	tmp -= 4U;
	tmp = ~tmp;
	tmp /= 4U;

	return (uint16_t)tmp;
}

/**
 * @brief Refresh the watchdog and verify the refresh was good.
 *
 * @return 0 on success, error code otherwise
 */
static int fs26_wd_refresh(const struct device *dev)
{
	const struct wdt_nxp_fs26_config *config = dev->config;
	struct wdt_nxp_fs26_data *data = dev->data;
	int retval = 0;
	int key;
	uint16_t answer;
	struct fs26_spi_rx_frame rx_frame;

	if (config->wd_type == FS26_WD_SIMPLE) {
		if (fs26_setreg(&config->spi, FS26_FS_WD_ANSWER, data->token) == 0) {
			LOG_ERR("Failed to write answer");
			retval = -EIO;
		}
	} else if (config->wd_type == FS26_WD_CHALLENGER) {
		key = irq_lock();

		/* Read challenge token generated by the device */
		if (fs26_getreg(&config->spi, FS26_FS_WD_TOKEN, &rx_frame)) {
			LOG_ERR("Failed to obtain watchdog token");
			retval = -EIO;
		} else {
			data->token = rx_frame.data;
			LOG_DBG("Watchdog token is %x", data->token);

			answer = fs26_wd_compute_answer(data->token);
			if (fs26_setreg(&config->spi, FS26_FS_WD_ANSWER, answer)) {
				LOG_ERR("Failed to write answer");
				retval = -EIO;
			}
		}

		irq_unlock(key);
	} else {
		retval = -EINVAL;
	}

	/* Check if watchdog refresh was successful */
	if (!retval) {
		if (!fs26_getreg(&config->spi, FS26_FS_GRL_FLAGS, &rx_frame)) {
			if ((rx_frame.data & FS_WD_G_MASK) == FS_WD_G) {
				if (!fs26_getreg(&config->spi, FS26_FS_DIAG_SAFETY1, &rx_frame)) {
					LOG_ERR("Bad watchdog refresh, %s",
						BAD_WD_REFRESH_ERROR_STRING(rx_frame.data));
				}
				retval = -EIO;
			} else {
				LOG_DBG("Refreshed the watchdog");
			}
		}
	}

	return retval;
}

/**
 * @brief Wait for state machine to be at in INIT_FS state
 *
 * @return 0 on success, -ETIMEDOUT if timedout
 */
static int fs26_poll_for_init_fs_state(const struct device *dev)
{
	const struct wdt_nxp_fs26_config *config = dev->config;
	struct fs26_spi_rx_frame rx_frame;
	uint32_t regval = 0;
	int64_t timeout;
	int64_t now;

	timeout = k_uptime_get() + FS26_INIT_FS_TIMEOUT_MS;

	do {
		if (!fs26_getreg(&config->spi, FS26_FS_STATES, &rx_frame)) {
			regval = rx_frame.data;
		}
		k_sleep(K_MSEC(1));
		now = k_uptime_get();
	} while ((now < timeout) && (regval & FS_STATES_MASK) != FS_STATES_INIT_FS);

	if (now >= timeout) {
		LOG_ERR("Timedout waiting for INIT_FS state");
		return -ETIMEDOUT;
	}

	return 0;
}

/**
 * @brief Go to INIT_FS state from any FS state after INIT_FS
 *
 * After INIT_FS closure, it is possible to come back to INIT_FS with the
 * GOTO_INIT bit in FS_SAFE_IOS_1 register from any FS state after INIT_FS.
 *
 * @return 0 on success, error code otherwise
 */
static int fs26_goto_init_fs_state(const struct device *dev)
{
	const struct wdt_nxp_fs26_config *config = dev->config;
	struct fs26_spi_rx_frame rx_frame;
	uint32_t current_state;
	int retval = -EIO;

	if (!fs26_getreg(&config->spi, FS26_FS_STATES, &rx_frame)) {
		current_state = rx_frame.data & FS_STATES_MASK;
		if (current_state < FS_STATES_INIT_FS) {
			LOG_ERR("Cannot go to INIT_FS from current state %x", current_state);
			retval = -EIO;
		} else if (current_state == FS_STATES_INIT_FS) {
			retval = 0;
		} else {
			fs26_setreg(&config->spi, FS26_FS_SAFE_IOS_1, (uint32_t)FS_GOTO_INIT);
			retval = fs26_poll_for_init_fs_state(dev);
		}
	}

	return retval;
}

/**
 * @brief Close INIT_FS phase with a (good) watchdog refresh.
 *
 * @return 0 on success, error code otherwise
 */
static inline int fs26_exit_init_fs_state(const struct device *dev)
{
	return fs26_wd_refresh(dev);
}

static int wdt_nxp_fs26_feed(const struct device *dev, int channel_id)
{
	struct wdt_nxp_fs26_data *data = dev->data;

	if (channel_id != 0) {
		LOG_ERR("Invalid channel ID");
		return -EINVAL;
	}

	if (!data->timeout_installed) {
		LOG_ERR("No timeout installed");
		return -EINVAL;
	}

	return fs26_wd_refresh(dev);
}

static int wdt_nxp_fs26_setup(const struct device *dev, uint8_t options)
{
	const struct wdt_nxp_fs26_config *config = dev->config;
	struct wdt_nxp_fs26_data *data = dev->data;
	uint32_t regval;

	if (!data->timeout_installed) {
		LOG_ERR("No timeout installed");
		return -EINVAL;
	}

	if ((options & WDT_OPT_PAUSE_IN_SLEEP) || (options & WDT_OPT_PAUSE_HALTED_BY_DBG)) {
		return -ENOTSUP;
	}

	/*
	 * Apply fail-safe reaction configuration on RSTB and/or the safety output(s),
	 * configurable during the initialization phase.
	 */
	if (fs26_goto_init_fs_state(dev)) {
		LOG_ERR("Failed to go to INIT_FS");
		return -EIO;
	}

	regval = WD_ERR_LIMIT(CONFIG_WDT_NXP_FS26_ERROR_COUNTER_LIMIT)
		| WD_RFR_LIMIT(CONFIG_WDT_NXP_FS26_REFRESH_COUNTER_LIMIT)
		| ((data->fs_reaction << WD_FS_REACTION_SHIFT) & WD_FS_REACTION_MASK);

	fs26_setreg(&config->spi, FS26_FS_I_WD_CFG, regval);
	fs26_setreg(&config->spi, FS26_FS_I_NOT_WD_CFG, ~regval);

	/* Apply watchdog window configuration, configurable during any FS state */
	regval = ((data->window_period << WDW_PERIOD_SHIFT) & WDW_PERIOD_MASK)
		| ((data->window_duty_cycle << WDW_DC_SHIFT) & WDW_DC_MASK)
		| WDW_RECOVERY_DISABLE;

	fs26_setreg(&config->spi, FS26_FS_WDW_DURATION, regval);
	fs26_setreg(&config->spi, FS26_FS_NOT_WDW_DURATION, ~regval);

	/*
	 * The new watchdog window is effective after the next watchdog refresh,
	 * so feed the watchdog once to make it effective after exiting this
	 * function. Also it's required to close init phase.
	 */
	if (fs26_exit_init_fs_state(dev)) {
		LOG_ERR("Failed to close INIT_FS");
		return -EIO;
	}

	return 0;
}

static int wdt_nxp_fs26_install_timeout(const struct device *dev,
					const struct wdt_timeout_cfg *cfg)
{
	struct wdt_nxp_fs26_data *data = dev->data;
	uint32_t window_min;
	uint8_t i;

	if (data->timeout_installed) {
		LOG_ERR("No more timeouts can be installed");
		return -ENOMEM;
	}

	if ((cfg->window.max == 0) || (cfg->window.max > 1024)
			|| (cfg->window.max <= cfg->window.min)) {
		LOG_ERR("Invalid timeout value");
		return -EINVAL;
	}

	/* Find nearest period value (rounded up) */
	for (i = 0; i < ARRAY_SIZE(fs26_period_values); i++) {
		if (fs26_period_values[i] >= cfg->window.max) {
			break;
		}
	}
	data->window_period = i;
	LOG_DBG("window.max requested %d ms, using %d ms",
		cfg->window.max, fs26_period_values[data->window_period]);

	/*
	 * Find nearest duty cycle value based on new period, that results in a
	 * window's minimum near the requested (rounded up)
	 */
	for (i = 0; i < ARRAY_SIZE(fs26_dc_closed_values); i++) {
		window_min = (uint32_t)(fs26_dc_closed_values[i]
					* fs26_period_values[data->window_period]);
		if (window_min >= cfg->window.min) {
			break;
		}
	}
	if (i >= ARRAY_SIZE(fs26_dc_closed_values)) {
		LOG_ERR("Watchdog opened window too small");
		return -EINVAL;
	}
	data->window_duty_cycle = i;

	LOG_DBG("window.min requested %d ms, using %d ms (%.2f%%)",
		cfg->window.min, window_min,
		fs26_dc_closed_values[data->window_duty_cycle] * 100);

	/* Fail-safe reaction configuration */
	switch (cfg->flags) {
	case WDT_FLAG_RESET_SOC:
		__fallthrough;
	case WDT_FLAG_RESET_CPU_CORE:
		data->fs_reaction = WD_FS_REACTION_RSTB_FS0B >> WD_FS_REACTION_SHIFT;
		LOG_DBG("Configuring reset mode");
		break;
	case WDT_FLAG_RESET_NONE:
		data->fs_reaction = WD_FS_REACTION_NO_ACTION >> WD_FS_REACTION_SHIFT;
		LOG_DBG("Configuring non-reset mode");
		break;
	default:
		LOG_ERR("Unsupported watchdog configuration flag");
		return -EINVAL;
	}

	data->callback = cfg->callback;
	data->timeout_installed = true;

	/* Always return channel ID equal to 0 */
	return 0;
}

static int wdt_nxp_fs26_disable(const struct device *dev)
{
	const struct wdt_nxp_fs26_config *config = dev->config;
	struct wdt_nxp_fs26_data *data = dev->data;
	struct fs26_spi_rx_frame rx_frame;
	uint32_t regval;

	if (fs26_getreg(&config->spi, FS26_FS_WDW_DURATION, &rx_frame)) {
		return -EIO;
	}
	if ((rx_frame.data & WDW_PERIOD_MASK) == WDW_PERIOD_DISABLE) {
		LOG_ERR("Watchdog already disabled");
		return -EFAULT;
	}

	/* The watchdog window can be disabled only during the initialization phase */
	if (fs26_goto_init_fs_state(dev)) {
		LOG_ERR("Failed to go to INIT_FS");
		return -EIO;
	}

	regval = WDW_PERIOD_DISABLE | WDW_RECOVERY_DISABLE;
	fs26_setreg(&config->spi, FS26_FS_WDW_DURATION, regval);
	fs26_setreg(&config->spi, FS26_FS_NOT_WDW_DURATION, ~regval);

	/* The watchdog disabling is effective when the initialization phase is closed */
	if (fs26_exit_init_fs_state(dev)) {
		LOG_ERR("Failed to close INIT_FS");
		return -EIO;
	}

	LOG_DBG("Watchdog disabled");
	data->timeout_installed = false;

	return 0;
}

static void wdt_nxp_fs26_int_thread(const struct device *dev)
{
	const struct wdt_nxp_fs26_config *config = dev->config;
	struct wdt_nxp_fs26_data *data = dev->data;
	struct fs26_spi_rx_frame rx_frame;
	uint32_t regval;

	while (1) {
		k_sem_take(&data->int_sem, K_FOREVER);

		if ((!fs26_getreg(&config->spi, FS26_FS_GRL_FLAGS, &rx_frame))
			&& ((rx_frame.data & FS_WD_G_MASK) == FS_WD_G)) {

			if ((!fs26_getreg(&config->spi, FS26_FS_DIAG_SAFETY1, &rx_frame))
				&& (rx_frame.data & BAD_WD_TIMING)) {

				/* Clear flag */
				regval = BAD_WD_TIMING;
				fs26_setreg(&config->spi, FS26_FS_DIAG_SAFETY1, regval);

				/* Invoke user callback */
				if (data->callback && data->timeout_installed) {
					data->callback(dev, 0);
				}
			}
		}
	}
}

static void wdt_nxp_fs26_int_callback(const struct device *dev,
				      struct gpio_callback *cb,
				      uint32_t pins)
{
	struct wdt_nxp_fs26_data *data = CONTAINER_OF(cb, struct wdt_nxp_fs26_data,
						      int_gpio_cb);

	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	k_sem_give(&data->int_sem);
}

static int wdt_nxp_fs26_init(const struct device *dev)
{
	const struct wdt_nxp_fs26_config *config = dev->config;
	struct wdt_nxp_fs26_data *data = dev->data;
	struct fs26_spi_rx_frame rx_frame;
	uint32_t regval;

	/* Validate bus is ready */
	if (!spi_is_ready_dt(&config->spi)) {
		return -ENODEV;
	}

	k_sem_init(&data->int_sem, 0, 1);

	/* Configure GPIO used for INTB signal */
	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("GPIO port %s not ready", config->int_gpio.port->name);
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT)) {
		LOG_ERR("Unable to configure GPIO pin %u", config->int_gpio.pin);
		return -EIO;
	}

	gpio_init_callback(&(data->int_gpio_cb), wdt_nxp_fs26_int_callback,
			   BIT(config->int_gpio.pin));

	if (gpio_add_callback(config->int_gpio.port, &(data->int_gpio_cb))) {
		return -EINVAL;
	}

	if (gpio_pin_interrupt_configure_dt(&config->int_gpio,
					    GPIO_INT_EDGE_FALLING)) {
		return -EINVAL;
	}

	k_thread_create(&data->int_thread, data->int_thread_stack,
			CONFIG_WDT_NXP_FS26_INT_THREAD_STACK_SIZE,
			(k_thread_entry_t)wdt_nxp_fs26_int_thread,
			(void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_WDT_NXP_FS26_INT_THREAD_PRIO),
			0, K_NO_WAIT);

	/* Verify FS BIST before proceeding */
	if (fs26_getreg(&config->spi, FS26_FS_DIAG_SAFETY1, &rx_frame)) {
		return -EIO;
	}

	if ((rx_frame.data & (ABIST1_PASS_MASK | LBIST_STATUS_MASK))
		!= (ABIST1_PASS | LBIST_STATUS_OK)) {

		LOG_ERR("BIST failed 0x%x", rx_frame.data);
		return -EIO;
	}

	/* Get FS state machine state */
	if (fs26_getreg(&config->spi, FS26_FS_STATES, &rx_frame)) {
		return -EIO;
	}

	/* Verify if in DEBUG mode */
	if ((rx_frame.data & DBG_MODE_MASK) == DBG_MODE) {
		if (IS_ENABLED(CONFIG_WDT_NXP_FS26_EXIT_DEBUG_MODE)) {
			LOG_DBG("Exiting DEBUG mode");
			regval = rx_frame.data | EXIT_DBG_MODE;
			fs26_setreg(&config->spi, FS26_FS_STATES, regval);
		} else {
			LOG_ERR("In DEBUG mode, watchdog is disabled");
			return -EIO;
		}
	}

	/* Go to INIT_FS state, if not already there */
	if (fs26_goto_init_fs_state(dev)) {
		LOG_ERR("Failed to go to INIT_FS");
		return -EIO;
	}

	/* Clear pending FS diagnostic flags before initializing */
	regval = BAD_WD_DATA | BAD_WD_TIMING | ABIST2_PASS | ABIST2_DONE
		| SPI_FS_CLK | SPI_FS_REQ | SPI_FS_CRC | FS_OSC_DRIFT;
	fs26_setreg(&config->spi, FS26_FS_DIAG_SAFETY1, regval);

	/*
	 * Perform the following sequence for all INIT_FS registers (FS_I_xxxx)
	 * - Write the desired data in the FS_I_Register_A (data)
	 * - Write the opposite in the FS_I_NOT_Register_A (~data)
	 */

	/* OVUV_SAFE_REACTION1 */
	regval = VMON_PRE_OV_FS_REACTION_NO_EFFECT |
		VMON_PRE_UV_FS_REACTION_NO_EFFECT |
		VMON_CORE_OV_FS_REACTION_NO_EFFECT |
		VMON_CORE_UV_FS_REACTION_NO_EFFECT |
		VMON_LDO1_OV_FS_REACTION_NO_EFFECT |
		VMON_LDO1_UV_FS_REACTION_NO_EFFECT |
		VMON_LDO2_OV_FS_REACTION_NO_EFFECT |
		VMON_LDO2_UV_FS_REACTION_NO_EFFECT;

	fs26_setreg(&config->spi, FS26_FS_I_OVUV_SAFE_REACTION1, regval);
	fs26_setreg(&config->spi, FS26_FS_I_NOT_OVUV_SAFE_REACTION1, ~regval);

	/* OVUV_SAFE_REACTION2 */
	regval = VMON_EXT_OV_FS_REACTION_NO_EFFECT |
		VMON_EXT_UV_FS_REACTION_NO_EFFECT |
		VMON_REF_OV_FS_REACTION_NO_EFFECT |
		VMON_REF_UV_FS_REACTION_NO_EFFECT |
		VMON_TRK2_OV_FS_REACTION_NO_EFFECT |
		VMON_TRK2_UV_FS_REACTION_NO_EFFECT |
		VMON_TRK1_OV_FS_REACTION_NO_EFFECT |
		VMON_TRK1_UV_FS_REACTION_NO_EFFECT;

	fs26_setreg(&config->spi, FS26_FS_I_OVUV_SAFE_REACTION2, regval);
	fs26_setreg(&config->spi, FS26_FS_I_NOT_OVUV_SAFE_REACTION2, ~regval);

	/* FS_I_SAFE_INPUTS */
	regval = FCCU_CFG_NO_MONITORING | ERRMON_ACK_TIME_32MS;

	fs26_setreg(&config->spi, FS26_FS_I_SAFE_INPUTS, regval);
	fs26_setreg(&config->spi, FS26_FS_I_NOT_SAFE_INPUTS, ~regval);

	/* FS_I_FSSM */
	regval = FLT_ERR_REACTION_NO_EFFECT | CLK_MON_DIS | DIS8S;

	fs26_setreg(&config->spi, FS26_FS_I_FSSM, regval);
	fs26_setreg(&config->spi, FS26_FS_I_NOT_FSSM, ~regval);

	/* FS_I_WD_CFG */
	regval = WD_ERR_LIMIT(CONFIG_WDT_NXP_FS26_ERROR_COUNTER_LIMIT)
		| WD_RFR_LIMIT(CONFIG_WDT_NXP_FS26_REFRESH_COUNTER_LIMIT)
		| WD_FS_REACTION_NO_ACTION;

	fs26_setreg(&config->spi, FS26_FS_I_WD_CFG, regval);
	fs26_setreg(&config->spi, FS26_FS_I_NOT_WD_CFG, ~regval);

	/* FS_WDW_DURATION */
	/* Watchdog always disabled at boot */
	regval = WDW_PERIOD_DISABLE | WDW_RECOVERY_DISABLE;

	fs26_setreg(&config->spi, FS26_FS_WDW_DURATION, regval);
	fs26_setreg(&config->spi, FS26_FS_NOT_WDW_DURATION, ~regval);

	/* Set watchdog seed if not using the default */
	if (data->token != FS26_FS_WD_TOKEN_DEFAULT) {
		LOG_DBG("Set seed to %x", data->token);
		fs26_setreg(&config->spi, FS26_FS_WD_TOKEN, data->token);
	}

	/* Mask all Fail-Safe interrupt sources except for watchdog bad refresh */
	regval = ~BAD_WD_M;
	fs26_setreg(&config->spi, FS26_FS_INTB_MASK, regval);

	/* Mask all main interrupt souces */
	regval = 0xffff;
	fs26_setreg(&config->spi, FS26_M_TSD_MSK, regval);
	fs26_setreg(&config->spi, FS26_M_REG_MSK, regval);
	fs26_setreg(&config->spi, FS26_M_VSUP_MSK, regval);
	fs26_setreg(&config->spi, FS26_M_WIO_MSK, regval);
	fs26_setreg(&config->spi, FS26_M_COM_MSK, regval);

	/* INIT_FS must be closed before the 256 ms timeout */
	if (fs26_exit_init_fs_state(dev)) {
		LOG_ERR("Failed to close INIT_FS");
		return -EIO;
	}

	/* After INIT_FS is completed, check for data corruption in init registers */
	if (!fs26_getreg(&config->spi, FS26_FS_STATES, &rx_frame)) {
		if ((rx_frame.data & REG_CORRUPT_MASK) == REG_CORRUPT) {
			LOG_ERR("Data content corruption detected in init registers");
			return -EIO;
		}
	}

	return 0;
}

static const struct wdt_driver_api wdt_nxp_fs26_api = {
	.setup = wdt_nxp_fs26_setup,
	.disable = wdt_nxp_fs26_disable,
	.install_timeout = wdt_nxp_fs26_install_timeout,
	.feed = wdt_nxp_fs26_feed,
};

#define FS26_WDT_DEVICE_INIT(n)								\
	COND_CODE_1(DT_INST_ENUM_IDX(n, type),						\
		(BUILD_ASSERT(CONFIG_WDT_NXP_FS26_SEED != 0x0,				\
				"Seed value 0x0000 is not allowed");),			\
		(BUILD_ASSERT((CONFIG_WDT_NXP_FS26_SEED != 0x0)				\
				&& (CONFIG_WDT_NXP_FS26_SEED != 0xffff),		\
				"Seed values 0x0000 and 0xffff are not allowed");))	\
											\
	static struct wdt_nxp_fs26_data wdt_nxp_fs26_data_##n = {			\
		.token = CONFIG_WDT_NXP_FS26_SEED,					\
	};										\
											\
	static const struct wdt_nxp_fs26_config wdt_nxp_fs26_config_##n = {		\
		.spi = SPI_DT_SPEC_INST_GET(n,						\
			SPI_OP_MODE_MASTER | SPI_MODE_CPHA | SPI_WORD_SET(32), 0),	\
		.wd_type = CONCAT(FS26_WD_, DT_INST_STRING_UPPER_TOKEN(n, type)),	\
		.int_gpio = GPIO_DT_SPEC_INST_GET(n, int_gpios),			\
	};										\
											\
	DEVICE_DT_INST_DEFINE(n,							\
			      wdt_nxp_fs26_init,					\
			      NULL,							\
			      &wdt_nxp_fs26_data_##n,					\
			      &wdt_nxp_fs26_config_##n,					\
			      POST_KERNEL,						\
			      CONFIG_WDT_NXP_FS26_INIT_PRIORITY,			\
			      &wdt_nxp_fs26_api);

DT_INST_FOREACH_STATUS_OKAY(FS26_WDT_DEVICE_INIT)
