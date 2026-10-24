/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>

#include <soc.h>

#include "mspi_mchp_qspi_g1.h"

/* Timeout and polling constants */
#define TIMEOUT_VALUE_US		1000
#define TIMEOUT_VALUE_TWO_SEC		2000000
#define DELAY_US			2
#define QSPI_EXTENDED_TIMEOUT_US	(10 * TIMEOUT_VALUE_US)
#define CLOCK_FREQ_RETRY_COUNT		4
#define CLOCK_FREQ_POLL_INTERVAL_MS	100

/* Hardware limits */
#define QSPI_BAUD_MAX			255
#define FREQ_ERROR_TOLERANCE_PERCENT	10
#define QSPI_DUMMY_MAX			0x1F
#define QSPI_CMD_1BYTE_MAX		0xFF
#define QSPI_CMD_2BYTE_MAX		0xFFFF
#define QSPI_MEM_BOUNDARY_MAX		0x01000000
#define QSPI_DUMMY_CYCLE_BYTE		0xFF
#define QSPI_RX_CLOCK_BYTE		0xFF
#define QSPI_CTRLB_WDRBT_BIT		(1U << 2)
#define QSPI_MAX_CHANNEL_NUM		1
#define QSPI_CMD_BYTE_SHIFT		8
#define QSPI_DEVICE_READY_WAIT_US	150
#define MSPI_BITS_ROUND_MASK		(BITS_PER_BYTE - 1)
#define MSPI_MCHP_CONTROLLER_MAX_FREQ	DT_PROP(DT_NODELABEL(qspi0), clock_frequency)
#define INSTRFRAME_CACHED_MASK		(QSPI_INSTRFRAME_WIDTH_Msk | \
					 QSPI_INSTRFRAME_ADDRLEN_Msk | \
					 QSPI_INSTRFRAME_DDREN_Msk)

#define DT_DRV_COMPAT microchip_qspi_g1_mspi
#include <zephyr/devicetree.h>

#include <errno.h>

LOG_MODULE_REGISTER(microchip_qspi_g1_mspi, CONFIG_MSPI_LOG_LEVEL);

/* Default timeout in milliseconds if user doesn't specify one (0 = not set) */
#define MSPI_DEFAULT_TIMEOUT_MS  1000

/* Get effective timeout - use default if user specified 0 */
#define MSPI_EFFECTIVE_TIMEOUT(user_timeout) \
	((user_timeout) == 0 ? MSPI_DEFAULT_TIMEOUT_MS : (user_timeout))

#define CLOCK_NODE DT_NODELABEL(clock)

typedef void (*irq_config_func_t)(const struct device *dev);
static int mspi_mchp_qspi_init(const struct device *controller);

/* Forward declarations for async work handlers */
static void mspi_async_timeout_timer_handler(struct k_timer *timer);
static void mspi_async_timeout_work_handler(struct k_work *work);
static void mspi_async_packet_work_handler(struct k_work *work);
static int mspi_start_next_packet(const struct device *dev);

/* Forward declarations for assert/de-assert */
static int gpio_cs_assert(const struct device *controller, uint16_t dev_idx);
static int gpio_cs_deassert(const struct device *controller, uint16_t dev_idx);

/* Enums */

enum mspi_data_rate_t {
	MCHP_HAL_MSPI_DATA_RATE_SDR = 0,
	MCHP_HAL_MSPI_DATA_RATE_DDR = 1,
};

enum mchp_hal_mspi_instr_e {
	MCHP_HAL_MSPI_CMD_1_BYTE = 1,
	MCHP_HAL_MSPI_CMD_2_BYTE = 2
};

enum mchp_hal_mspi_addr {
	MCHP_HAL_MSPI_ADDR_3_BYTE = 3,
	MCHP_HAL_MSPI_ADDR_4_BYTE = 4
};

/* Structs */

struct mchp_qspi_reg_config {
	qspi_registers_t *regs;
	uint32_t pads;
};

struct mspi_mchp_cached_hw_cfg {
	uint32_t baud_reg;
	uint32_t ctrlb_reg;
	uint32_t instrframe_bits;
	bool is_configured;
};

struct mchp_mspi_clock {
	/* Main clock subsystem. */
	clock_control_subsys_t mclk_apb;
	clock_control_subsys_t mclk_ahb;
	clock_control_subsys_t mclk_ahb2x;
};

/*
 * Description of a child device connected to the QSPI peripheral.
 *
 * This structure contains the static configuration parameters for a child
 * device connected to the QSPI peripheral.
 */
struct mspi_child_desc {
	uint16_t cs;          /* Chip-select index */
	bool is_serial_mode;  /* Use serial memory mode (vs SPI mode) */
};

/*
 * A ready-to-apply config derived from DT (maps to your mspi_dev_cfg)
 */
struct mspi_child_cfg {
	struct mspi_dev_id id;
	struct mspi_dev_cfg cfg;
	uint8_t dlybs;   /* Delay before SCK (BAUD.DLYBS) */
	uint8_t dlycs;   /* Min inactive CS delay (CTRLB.DLYCS) */
	uint8_t dlybct;  /* Delay between transfers (CTRLB.DLYBCT) */
};

/*
 * Run-time data structure for the QSPI.
 *
 * This structure contains the Run-time data parameters for the QSPI
 * peripheral.
 */
struct mspi_context {
	const struct mspi_dev_id *owner;
	struct mspi_xfer xfer;
	int packets_done;
	mspi_callback_handler_t callback;
	struct mspi_callback_context *callback_ctx;
	bool asynchronous;
	struct k_sem lock;
};

/*
 * Device PIO transfer structure for the QSPI.
 *
 * This structure contains the Run-time data parameters for the QSPI
 * peripheral.
 */
struct mchp_hal_mspi_pio_transfer {
	uint32_t xfer_len;
	bool scrambling;
	bool dcx_enabled;
	enum mspi_xfer_direction direction;
	bool send_addr;
	uint8_t addrlen;
	uint32_t device_addr;
	bool send_instr;
	uint8_t device_instr;
	bool rx_dummy_en;
	bool tx_dummy_en;
	uint8_t rx_dummy;
	uint8_t tx_dummy;
	bool quad_cmd_enabled;
	bool continue_crmode;
	uint32_t *data_ptr;
};

struct mchp_hal_spi_pio_transfer {
	/* SPI mode */
	void *tx_buffer;
	void *rx_buffer;
	uint8_t addrlen;
	size_t tx_size;
	size_t rx_size;
	size_t rx_buffer_bytes;
	size_t rx_count;
	size_t tx_count;
	size_t blocks;
	size_t dummy_bytes;
	size_t dummy_bytes_fix;
	size_t rx_drop;
	bool transfer_is_busy;
	uint8_t cmd_bytes_remaining;
};

/*
 * Static configuration structure for the QSPI.
 *
 * This structure contains the static configuration parameters for the QSPI
 * peripheral.
 */
struct mspi_sam_qspi_cfg {
	struct mchp_qspi_reg_config reg_cfg;
	uint32_t reg_size;
	struct mspi_cfg mspicfg;
	const struct pinctrl_dev_config *pcfg;

	irq_config_func_t irq_config_func;
	struct mchp_mspi_clock mspi_clock;

	/* children of QSPI module */
	const struct mspi_child_desc *child_desc;
	uint16_t num_children;
	const struct mspi_child_cfg *child_cfg;

	/* GPIO chip selects for multi-device support */
	const struct gpio_dt_spec *ce_gpios;
	uint8_t ce_gpios_len;
	bool sw_multi_periph;

#if defined(CONFIG_MSPI_SCRAMBLE)
	uint32_t scramble_key;
	bool scramble_random_disable;
#endif
};

/*
 * Run-time data structure for the QSPI.
 *
 * This structure contains the Run-time data parameters for the QSPI
 * peripheral.
 */
struct mspi_sam_qspi_data {
	const struct mspi_dev_id *dev_id;
	struct k_mutex lock_init;
	struct k_mutex lock_dev;
	mspi_callback_handler_t (*cbs)[MSPI_BUS_EVENT_MAX];
	struct mspi_callback_context *(*cb_ctxs)[MSPI_BUS_EVENT_MAX];
	struct mspi_context ctx;
	/* for SPI mode */
	struct mchp_hal_spi_pio_transfer qspi_obj;

	/* SPI interrupt transfer state */
	bool is_last_byte_xfer_in_progress;

	/* k_work async infrastructure */
	const struct device *dev;		/* For CONTAINER_OF in work handlers */
	struct k_work async_packet_work;	/* Packet completion work */
	struct k_timer async_timer;		/* Per-transfer timeout */
	struct k_work async_timeout_work;	/* Timeout handler (deferred from timer) */
	struct k_sem async_sync_sem;		/* For sync-mode completion */

	/* Async transfer state */
	bool serial_mode_active;		/* Current transfer mode (serial vs SPI) */
	bool restore_spi_mode_on_done;
	int async_result;			/* Result from ISR/work chain */

	/* Async race condition coordination flags */
	bool async_packet_complete;		/* Set by ISR when packet completes */
	bool async_timed_out;			/* Set by timeout handler when it runs */

	/* Multi-device support */
	uint16_t active_dev_idx;		/* Currently selected device */
	struct mspi_mchp_cached_hw_cfg *hw_cache;  /* Array[num_children] of cached configs */

#if defined(CONFIG_MSPI_SCRAMBLE)
	struct mspi_scramble_cfg scramble_cfg;	/* Cached runtime scramble state */
#endif

#if defined(CONFIG_MSPI_MEMMAP)
	uint16_t xip_enabled;				/* Bitmap: which devices have XIP enabled */
	struct mspi_memmap_cfg *memmap_cfg; /* Per-device config array for memory mapping */
#endif
};

static inline int qspi_swrst(qspi_registers_t *q)
{
	q->QSPI_CTRLA |= QSPI_CTRLA_SWRST_Msk;
	if (WAIT_FOR(((q->QSPI_CTRLA & QSPI_CTRLA_SWRST_Msk) == 0U),
		     QSPI_EXTENDED_TIMEOUT_US, k_busy_wait(DELAY_US)) == 0U) {
		return -ETIMEDOUT;
	}
	return 0;
}

static inline void qspi_set_baud(qspi_registers_t *q, uint32_t baud)
{
	q->QSPI_BAUD = (q->QSPI_BAUD & ~(QSPI_BAUD_BAUD_Msk)) | QSPI_BAUD_BAUD(baud);
}

static inline uint8_t qspi_get_mode(qspi_registers_t *q)
{
	uint8_t phase = (q->QSPI_BAUD & QSPI_BAUD_CPHA_Msk) >> QSPI_BAUD_CPHA_Pos;
	uint8_t polarity = (q->QSPI_BAUD & QSPI_BAUD_CPOL_Msk) >> QSPI_BAUD_CPOL_Pos;

	return (uint8_t)(polarity | (phase << 1));
}

static inline void qspi_set_dummy_bits(qspi_registers_t *q, uint32_t dummy)
{
	q->QSPI_INSTRFRAME = (q->QSPI_INSTRFRAME & ~QSPI_INSTRFRAME_DUMMYLEN_Msk) |
			     QSPI_INSTRFRAME_DUMMYLEN(dummy);
}

static inline void qspi_set_serial_memory_mode(qspi_registers_t *q)
{
	q->QSPI_CTRLB = (q->QSPI_CTRLB & ~((QSPI_CTRLB_CSMODE_Msk) | (QSPI_CTRLB_MODE_Msk))) |
			(QSPI_CTRLB_MODE(QSPI_CTRLB_MODE_MEMORY_Val) |
			 QSPI_CTRLB_CSMODE(QSPI_CTRLB_CSMODE_NORELOAD_Val));
}

static inline void qspi_set_spi_mode(qspi_registers_t *q)
{
	q->QSPI_CTRLB = (q->QSPI_CTRLB & ~((QSPI_CTRLB_CSMODE_Msk) | (QSPI_CTRLB_MODE_Msk))) |
			(QSPI_CTRLB_MODE(QSPI_CTRLB_MODE_SPI_Val) |
			 QSPI_CTRLB_CSMODE(QSPI_CTRLB_CSMODE_LASTXFER_Val));
}

static inline bool qspi_is_cs_deassert(qspi_registers_t *q)
{
	return ((q->QSPI_STATUS & QSPI_STATUS_CSSTATUS_Msk) != 0);
}

static inline void qspi_clear_all_interrupts(qspi_registers_t *q)
{
	q->QSPI_INTFLAG = QSPI_INTFLAG_RXC_Msk | QSPI_INTFLAG_DRE_Msk | QSPI_INTFLAG_TXC_Msk |
			  QSPI_INTFLAG_ERROR_Msk | QSPI_INTFLAG_CSRISE_Msk |
			  QSPI_INTFLAG_INSTREND_Msk;
}

static inline void qspi_disable_all_interrupts(qspi_registers_t *q)
{
	q->QSPI_INTENCLR =
		(QSPI_INTENCLR_RXC_Msk | QSPI_INTENCLR_DRE_Msk | QSPI_INTENCLR_TXC_Msk |
		 QSPI_INTENCLR_ERROR_Msk | QSPI_INTENCLR_CSRISE_Msk | QSPI_INTENCLR_INSTREND_Msk);
}

static inline void qspi_end_transfer(qspi_registers_t *q)
{
	q->QSPI_CTRLA = QSPI_CTRLA_ENABLE_Msk | QSPI_CTRLA_LASTXFER_Msk;
}

static int qspi_set_enabled(qspi_registers_t *q, bool enable)
{
	uint32_t want = 0;

	if (enable) {
		want = QSPI_STATUS_ENABLE_Msk;
		q->QSPI_CTRLA |= QSPI_CTRLA_ENABLE_Msk;
	} else {
		q->QSPI_CTRLA &= ~QSPI_CTRLA_ENABLE_Msk;
	}

	if (WAIT_FOR(((q->QSPI_STATUS & QSPI_STATUS_ENABLE_Msk) == want), TIMEOUT_VALUE_US,
		      k_busy_wait(DELAY_US)) == 0) {
		LOG_ERR("QSPI %s flag setting timed out", enable ? "enable" : "disable");
		return -ETIMEDOUT;
	}

	return 0;
}

/*
 * Assert GPIO chip select for a device.
 *
 * Activates the chip select (CS) GPIO for the specified device index
 * in software multi-peripheral mode. If the device uses hardware CS
 * (ce_gpios entry is NULL), this is a no-op.
 *
 * Takes controller (pointer to MSPI controller device) and dev_idx
 * (chip select index of the target device).
 *
 * Returns 0 on success, -EINVAL if device index is out of range.
 */
static int gpio_cs_assert(const struct device *controller, uint16_t dev_idx)
{
	const struct mspi_sam_qspi_cfg *cfg = controller->config;
	const struct gpio_dt_spec *ce;

	if (cfg->sw_multi_periph == false) {
		return 0;  /* Not in multi-peripheral mode, hardware CS only */
	}

	if (dev_idx >= cfg->ce_gpios_len) {
		LOG_ERR("Invalid device index %d (max %d)", dev_idx, cfg->ce_gpios_len - 1);
		return -EINVAL;
	}

	ce = &cfg->ce_gpios[dev_idx];

	if (ce->port == NULL) {
		return 0;  /* No GPIO CS for this device, uses hardware CS */
	}

	/* Assert CS (gpio_pin_set_dt handles active-low/active-high via DT flags) */
	return gpio_pin_set_dt(ce, 1);
}

/*
 * De-assert GPIO chip select for a device.
 *
 * Deactivates the chip select (CS) GPIO for the specified device index
 * in software multi-peripheral mode. If the device uses hardware CS
 * (ce_gpios entry is NULL), this is a no-op.
 *
 * Takes controller (pointer to MSPI controller device) and dev_idx
 * (chip select index of the target device).
 *
 * Returns 0 on success, -EINVAL if device index is out of range.
 */
static int gpio_cs_deassert(const struct device *controller, uint16_t dev_idx)
{
	const struct mspi_sam_qspi_cfg *cfg = controller->config;
	const struct gpio_dt_spec *ce;

	if (cfg->sw_multi_periph == false) {
		return 0;  /* Not in multi-peripheral mode, hardware CS only */
	}

	if (dev_idx >= cfg->ce_gpios_len) {
		LOG_ERR("Invalid device index %d (max %d)", dev_idx, cfg->ce_gpios_len - 1);
		return -EINVAL;
	}

	ce = &cfg->ce_gpios[dev_idx];

	if (ce->port == NULL) {
		return 0;  /* No GPIO CS for this device, uses hardware CS */
	}

	/* De-assert CS (gpio_pin_set_dt handles active-low/active-high via DT flags) */
	return gpio_pin_set_dt(ce, 0);
}

/* Clear interrupt error flag if exists before
 * waiting for DRE,RXC,TXC.
 */
static int qspi_check_and_clear_error(qspi_registers_t *q)
{
	if ((q->QSPI_INTFLAG & QSPI_INTFLAG_ERROR_Msk) != 0) {
		/* clear error flag if set*/
		q->QSPI_INTFLAG = QSPI_INTFLAG_ERROR_Msk;

		/* flush out any stale RXDATA */
		(void)q->QSPI_RXDATA;

		LOG_ERR("QSPI transfer error detected");
		return -EIO;
	}

	return 0;
}

/*
 * Find if child node is on SPI mode or serial mode.
 *
 * Takes children (pointer to MSPI child descriptor array), dev_idx (chip select
 * index of child node), and num_of_children (total number of children for the
 * MSPI controller).
 *
 * Returns true if child is SPI-NOR (serial memory mode), false otherwise.
 */
static int mspi_find_serial_mode_of_child(const struct mspi_child_desc *children, uint16_t dev_idx,
				      uint16_t num_of_children, bool *serial_mode)
{
	if ((num_of_children == 0) || (serial_mode == NULL) || (children == NULL)) {
		LOG_ERR("Invalid child lookup parameters");
		return -EINVAL;
	}

	for (uint16_t i = 0; i < num_of_children; i++) {
		if (children[i].cs == dev_idx) {

			*serial_mode = children[i].is_serial_mode;
			return 0;
		}
	}
	LOG_ERR("No child node found for dev_idx %d", dev_idx);

	return -ENODEV;
}

/*
 * Validate the chip-select (CE) index against controller limits.
 *
 * When enabled through the mask, checks that the device chip-select index
 * does not exceed the number of chip-select GPIOs supported by the MSPI
 * controller. This also handles the case where no external CE GPIOs are
 * defined.
 *
 * Takes mask (bitmask indicating whether CE index validation should be
 * performed via MSPI_DEVICE_CONFIG_CE_NUM), dev_id (pointer to device
 * identifier containing the chip-select index), and ccfg (pointer to
 * controller configuration providing the number of supported CE GPIOs).
 *
 * Returns 0 on success, or -EINVAL if the chip-select index is invalid.
 */
static int mspi_check_cs_index_limit(uint32_t mask, const struct mspi_dev_id *dev_id,
				const struct mspi_sam_qspi_cfg *ccfg)
{
	if ((mask & MSPI_DEVICE_CONFIG_CE_NUM) != 0) {
		/* greater than max CE lines */
		if (dev_id->dev_idx > ccfg->mspicfg.num_ce_gpios) {
			LOG_ERR("Invalid CE number");
			return -EINVAL;
		}
	}

	return 0;
}

/* Helper function to calculate the baud rate divisor */
static int qspi_calc_baud(uint32_t clk_freq, uint32_t target_freq,
			   uint8_t *baud_out)
{
	uint32_t baud_calc;
	uint32_t actual_freq;
	uint32_t error_percent;

	if ((target_freq == 0) || (target_freq > MSPI_MCHP_CONTROLLER_MAX_FREQ)) {
		LOG_ERR("Invalid frequency");
		return -EINVAL;
	}

	baud_calc = (clk_freq + (target_freq / 2)) / target_freq - 1;

	if (baud_calc > QSPI_BAUD_MAX) {
		LOG_ERR("Requested frequency %u Hz too low (min ~%u Hz)",
			target_freq, clk_freq / (QSPI_BAUD_MAX + 1));
		return -EINVAL;
	}

	actual_freq = clk_freq / ((uint8_t)baud_calc + 1);

	if (actual_freq > target_freq) {
		error_percent = ((actual_freq - target_freq) * 100) / target_freq;
	} else {
		error_percent = ((target_freq - actual_freq) * 100) / target_freq;
	}

	if (error_percent > FREQ_ERROR_TOLERANCE_PERCENT) {
		LOG_WRN("Frequency error %u%% exceeds %u%% tolerance",
			error_percent, FREQ_ERROR_TOLERANCE_PERCENT);
	}

	*baud_out = (uint8_t)baud_calc;
	return 0;
}

/*
 * Configure the MSPI baud rate (serial clock frequency).
 *
 * Retrieves the QSPI AHB clock frequency, validates the requested device
 * frequency, computes the corresponding baud divider, and programs the
 * QSPI BAUD register when frequency configuration is enabled.
 *
 * Takes mask (bitmask indicating whether frequency configuration should be
 * applied via MSPI_DEVICE_CONFIG_FREQUENCY), cfg (pointer to device
 * configuration containing the target MSPI clock frequency), ccfg (pointer
 * to controller configuration providing clock and maximum frequency info),
 * and q (pointer to the QSPI hardware register block).
 *
 * Returns 0 on success, -EINVAL for invalid frequency, -ETIMEDOUT if clock
 * rate retrieval times out, or a propagated error from the clock control driver.
 */
static int mspi_set_baud(uint32_t mask, const struct mspi_dev_cfg *cfg,
			 const struct mspi_sam_qspi_cfg *ccfg, qspi_registers_t *q)
{
	int ret;
	uint8_t baud = 0;
	uint32_t ahb_clk_freq = 0;

	/* wait for QSPI_AHB to get its frequency from clock driver */
	uint32_t tries = CLOCK_FREQ_RETRY_COUNT;

	do {
		ret = clock_control_get_rate(DEVICE_DT_GET(CLOCK_NODE), ccfg->mspi_clock.mclk_ahb,
					     &ahb_clk_freq);
		if (ret != 0) {
			return ret;
		}
		k_sleep(K_MSEC(CLOCK_FREQ_POLL_INTERVAL_MS));
		if (--tries == 0) {
			LOG_ERR("Timed out getting frequency of clock AHB");
			return -ETIMEDOUT;
		}
	} while (ahb_clk_freq == 0);

	if ((mask & MSPI_DEVICE_CONFIG_FREQUENCY) != 0) {
		if (cfg->freq == 0 || cfg->freq > MSPI_MCHP_CONTROLLER_MAX_FREQ) {
			LOG_ERR("Invalid frequency");
			return -EINVAL;
		}

		ret = qspi_calc_baud(ahb_clk_freq, cfg->freq, &baud);
		if (ret != 0) {
			LOG_ERR("Baud rate divisor error");
			return ret;
		}
		qspi_set_baud(q, baud);
	}

	return 0;
}

/*
 * Configure the MSPI data bus width (I/O mode).
 *
 * Applies the requested MSPI I/O mode when enabled through the mask,
 * translates it to the controller-specific width encoding, and programs
 * the WIDTH field in the QSPI INSTRFRAME register. Unsupported I/O modes
 * return an error.
 *
 * Takes mask (bitmask indicating whether I/O mode configuration should be
 * applied via MSPI_DEVICE_CONFIG_IO_MODE), cfg (pointer to device
 * configuration containing the MSPI I/O mode), and q (pointer to the QSPI
 * hardware register block).
 *
 * Returns 0 on success, or -EINVAL if the requested I/O mode is unsupported.
 */
static int mspi_set_width(uint32_t mask, const struct mspi_dev_cfg *cfg, qspi_registers_t *q)
{
	uint8_t width = 0;

	if ((mask & MSPI_DEVICE_CONFIG_IO_MODE) != 0) {
		switch (cfg->io_mode) {
		case MSPI_IO_MODE_SINGLE:
			width = 0;
			break;

		case MSPI_IO_MODE_DUAL_1_1_2:
			width = 1;
			break;

		case MSPI_IO_MODE_QUAD_1_1_4:
			width = 2;
			break;

		case MSPI_IO_MODE_DUAL_1_2_2:
			width = 3;
			break;

		case MSPI_IO_MODE_QUAD_1_4_4:
			width = 4;
			break;

		case MSPI_IO_MODE_DUAL:
			width = 5;
			break;

		case MSPI_IO_MODE_QUAD:
			width = 6;
			break;

		case MSPI_IO_MODE_OCTAL:
		case MSPI_IO_MODE_OCTAL_1_1_8:
		case MSPI_IO_MODE_OCTAL_1_8_8:
		case MSPI_IO_MODE_HEX:
		case MSPI_IO_MODE_HEX_8_8_16:
		case MSPI_IO_MODE_HEX_8_16_16:
		default:
			LOG_ERR("MCU does not support above quad IO mode");
			return -EINVAL;
		}

		/* set WIDTH bitfields in INSTRFRAME register */
		q->QSPI_INSTRFRAME =
			(q->QSPI_INSTRFRAME & ~QSPI_INSTRFRAME_WIDTH_Msk) |
			QSPI_INSTRFRAME_WIDTH(width);
	}

	return 0;
}

/*
 * Configure QSPI clock polarity and phase (CPOL/CPHA).
 *
 * Applies the CPP (Clock Polarity/Phase) mode from the device
 * configuration to the QSPI_BAUD register when the corresponding
 * mask bit (MSPI_DEVICE_CONFIG_CPP) is set.
 *
 * Takes mask (bitmask indicating which MSPI configuration fields should be
 * applied), cfg (pointer to device configuration containing the CPP mode),
 * and q (pointer to QSPI register block).
 *
 * Returns 0 on success, or -EINVAL if an invalid CPP mode is provided.
 */
static int set_clock_pol_pha(uint32_t mask, const struct mspi_dev_cfg *cfg, qspi_registers_t *q)
{
	if ((mask & MSPI_DEVICE_CONFIG_CPP) != 0) {
		switch (cfg->cpp) {
		case MSPI_CPP_MODE_0:
			q->QSPI_BAUD =
				(q->QSPI_BAUD & ~(QSPI_BAUD_CPOL_Msk | QSPI_BAUD_CPHA_Msk)) |
				(QSPI_BAUD_CPOL(0) | QSPI_BAUD_CPHA(0));
			break;

		case MSPI_CPP_MODE_1:
			q->QSPI_BAUD =
				(q->QSPI_BAUD & ~(QSPI_BAUD_CPOL_Msk | QSPI_BAUD_CPHA_Msk)) |
				(QSPI_BAUD_CPOL(0) | QSPI_BAUD_CPHA(1));
			break;

		case MSPI_CPP_MODE_2:
			q->QSPI_BAUD =
				(q->QSPI_BAUD & ~(QSPI_BAUD_CPOL_Msk | QSPI_BAUD_CPHA_Msk)) |
				(QSPI_BAUD_CPOL(1) | QSPI_BAUD_CPHA(0));
			break;

		case MSPI_CPP_MODE_3:
			q->QSPI_BAUD =
				(q->QSPI_BAUD & ~(QSPI_BAUD_CPOL_Msk | QSPI_BAUD_CPHA_Msk)) |
				(QSPI_BAUD_CPOL(1) | QSPI_BAUD_CPHA(1));
			break;

		default:
			LOG_ERR("Invalid CPP mode");
			return -EINVAL;
		}
	}

	return 0;
}

/*
 * Validate and report the chip-select (CE) polarity configuration.
 *
 * Checks whether CE polarity should be applied based on the mask and
 * verifies that the provided polarity value is valid (0 = active low,
 * 1 = active high).
 *
 * Takes mask (bitmask indicating which MSPI configuration fields are active,
 * e.g., MSPI_DEVICE_CONFIG_CE_POL) and cfg (pointer to device configuration
 * containing the CE polarity value).
 *
 * Returns 0 on success, or -EINVAL if the CE polarity is invalid.
 */
static int mspi_check_cs_polarity(uint32_t mask, const struct mspi_dev_cfg *cfg)
{
	if ((mask & MSPI_DEVICE_CONFIG_CE_POL) != 0) {
		if (cfg->ce_polarity > MSPI_CE_ACTIVE_HIGH) {
			LOG_ERR("Invalid CE polarity");
			return -EINVAL;
		}
	}

	return 0;
}

/*
 * Check whether DQS mode is requested and report unsupported usage.
 *
 * Validates the configuration mask and returns an error if DQS
 * (Data Strobe) mode is requested, as this MCU does not support it.
 *
 * Takes mask (bitmask indicating which MSPI configuration options are being
 * applied, e.g., MSPI_DEVICE_CONFIG_DQS).
 *
 * Returns 0 if DQS is not requested, or -EINVAL if DQS mode is unsupported.
 */
static int mspi_check_dqs_support(uint32_t mask)
{
	if ((mask & MSPI_DEVICE_CONFIG_DQS) != 0) {
		LOG_ERR("DQS mode not supported by MCU");
		return -EINVAL;
	}

	return 0;
}

/*
 * Validate and apply TX/RX dummy cycle configuration.(Serial memory mode only)
 *
 * Verifies that TX and RX dummy cycle values are within the supported
 * 5-bit range (0-31). When enabled through the mask, the function programs
 * the dummy cycle bits in the QSPI register block for both TX and RX paths.
 *
 * Takes mask (bitmask indicating which dummy-cycle configuration fields
 * should be applied via MSPI_DEVICE_CONFIG_TX_DUMMY and/or
 * MSPI_DEVICE_CONFIG_RX_DUMMY), cfg (pointer to device configuration
 * containing TX and RX dummy cycle values), and q (pointer to the QSPI
 * hardware register block).
 *
 * Returns 0 on success, or -EINVAL if a dummy cycle value exceeds the
 * 5-bit range.
 */
static int mspi_check_rx_tx_dummy(uint32_t mask, const struct mspi_dev_cfg *cfg,
				   qspi_registers_t *q)
{
	if ((mask & MSPI_DEVICE_CONFIG_TX_DUMMY) != 0) {
		/* max 5-bit value */
		if (cfg->tx_dummy > QSPI_DUMMY_MAX) {
			LOG_ERR("Invalid TX dummy cycles");
			return -EINVAL;
		}

		/* same bitfields for both tx and rx dummy cycles(tx dummy value can be accessed */
		/* anytime from .cfg) */
		qspi_set_dummy_bits(q, cfg->tx_dummy);
	}
	if ((mask & MSPI_DEVICE_CONFIG_RX_DUMMY) != 0) {
		/* max 5-bit value */
		if (cfg->rx_dummy > QSPI_DUMMY_MAX) {
			LOG_ERR("Invalid RX dummy cycles");
			return -EINVAL;
		}

		/* same bitfields for both tx and rx dummy cycles(rx dummy value can be accessed */
		/* anytime from .cfg) */
		qspi_set_dummy_bits(q, cfg->rx_dummy);
	}

	return 0;
}

/*
 * Validate read and write command opcodes.
 *
 * Ensures that the configured read and write commands are valid 1-byte
 * or 2 bytes opcodes when the corresponding mask bits are set. Logs the selected
 * commands after successful validation.
 *
 * Takes mask (bitmask indicating which opcode fields should be validated
 * via MSPI_DEVICE_CONFIG_READ_CMD and/or MSPI_DEVICE_CONFIG_WRITE_CMD)
 * and cfg (pointer to device configuration containing the read and write
 * command values).
 *
 * Returns 0 on success, or -EINVAL if any opcode exceeds the 1 or 2 byte range.
 */
static int mspi_check_read_write_cmd(uint32_t mask, const struct mspi_dev_cfg *cfg)
{
	uint32_t max_cmd = 0U;

	switch (cfg->cmd_length) {
	case 0:
		max_cmd = 0U;
		break;
	case 1:
		max_cmd = QSPI_CMD_1BYTE_MAX;
		break;
	case 2:
		max_cmd = QSPI_CMD_2BYTE_MAX;
		break;
	default:
		/* Greater than 2 is checked by mspi_check_cmd_length() function */
		LOG_ERR("Invalid read/write command");
		return -EINVAL;
	}

	/* Read Command input sanity check */
	if ((mask & MSPI_DEVICE_CONFIG_READ_CMD) != 0) {
		if (cfg->read_cmd > max_cmd) {
			LOG_ERR("Invalid read command");
			return -EINVAL;
		}
	}

	/* Write Command input sanity check */
	if ((mask & MSPI_DEVICE_CONFIG_WRITE_CMD) != 0) {
		if (cfg->write_cmd > max_cmd) {
			LOG_ERR("Invalid write command");
			return -EINVAL;
		}
	}

	return 0;
}

/*
 * Validate the SPI command length.
 *
 * Checks whether the command length configuration is enabled in the mask
 * and verifies that the provided length does not exceed 2 bytes.
 *
 * Takes mask (bitmask indicating whether command-length configuration
 * should be applied via MSPI_DEVICE_CONFIG_CMD_LEN) and cfg (pointer to
 * device configuration containing the command length).
 *
 * Returns 0 on success, or -EINVAL if the command length is greater than
 * 2 bytes.
 */
static int mspi_check_cmd_length(uint32_t mask, const struct mspi_dev_cfg *cfg)
{
	if ((mask & MSPI_DEVICE_CONFIG_CMD_LEN) != 0) {
		/* max 2 bytes */
		if (cfg->cmd_length > 2) {
			LOG_ERR("Invalid command length");
			return -EINVAL;
		}
	}

	return 0;
}

/*
 * Validate and apply the address length(In Serial memory mode) configuration.
 *
 * Ensures that the address length is either 3 or 4 bytes when the
 * corresponding mask bit is set. Converts the byte value into the
 * controller-specific format and programs the ADDRLEN field in the
 * INSTRFRAME register.
 *
 * Takes mask (bitmask indicating whether address-length configuration
 * should be applied via MSPI_DEVICE_CONFIG_ADDR_LEN), cfg (pointer to
 * device configuration containing the address length in bytes), and q
 * (pointer to the QSPI hardware register block).
 *
 * Returns 0 on success, or -EINVAL if the address length is invalid.
 */
static int mspi_check_addr_length(uint32_t mask, const struct mspi_dev_cfg *cfg,
				   qspi_registers_t *q)
{
	int addr_len;

	if ((mask & MSPI_DEVICE_CONFIG_ADDR_LEN) != 0) {
		/* max 4 bytes,normally 3 or 4 bytes address length(when only instruction to be */
		/* sent, then address length = 0) */
		if (cfg->addr_length < MCHP_HAL_MSPI_ADDR_3_BYTE ||
		    cfg->addr_length > MCHP_HAL_MSPI_ADDR_4_BYTE) {
			LOG_ERR("Invalid address length- "
				"Address length can be either 3 or 4 bytes");
			return -EINVAL;
		}
		addr_len = cfg->addr_length;

		if (cfg->addr_length == MCHP_HAL_MSPI_ADDR_3_BYTE) {
			addr_len = 0;
		} else if (cfg->addr_length == MCHP_HAL_MSPI_ADDR_4_BYTE) {
			addr_len = 1;
		}
		/* configure ADDRLEN bitfield in INSTRFRAME register */
		q->QSPI_INSTRFRAME = (q->QSPI_INSTRFRAME & ~QSPI_INSTRFRAME_ADDRLEN_Msk) |
				     QSPI_INSTRFRAME_ADDRLEN(addr_len);
	}

	return 0;
}

/*
 * Validate the configured memory boundary.
 *
 * When enabled through the mask, verifies that the memory boundary falls
 * within the supported valid range (0 to 16 MB). Logs the configured
 * boundary on success.
 *
 * Takes mask (bitmask indicating whether memory-boundary configuration
 * should be applied via MSPI_DEVICE_CONFIG_MEM_BOUND) and cfg (pointer
 * to device configuration containing the memory boundary value in bytes).
 *
 * Returns 0 on success, or -EINVAL if the boundary is outside the valid range.
 */
static int mspi_check_mem_bound(uint32_t mask, const struct mspi_dev_cfg *cfg)
{
	if ((mask & MSPI_DEVICE_CONFIG_MEM_BOUND) != 0) {
		/* max 16MB */
		if (cfg->mem_boundary > QSPI_MEM_BOUNDARY_MAX) {
			LOG_ERR("Invalid memory boundary");
			return -EINVAL;
		}
	}

	return 0;
}

/*
 * Release the MSPI context after transfer completion.
 *
 * Clears transfer state and releases the semaphore to allow the next transfer.
 */
static inline void mspi_context_release(struct mspi_context *ctx)
{
	ctx->owner = NULL;
	ctx->xfer.num_packet = 0;
	k_sem_give(&ctx->lock);
}

/*
 * Invoke user callback with properly populated context.
 * Called from thread context (work handlers) - safe to do anything here.
 *
 * Takes dev (Device pointer), evt_type (Event type: MSPI_BUS_XFER_COMPLETE,
 * MSPI_BUS_ERROR, MSPI_BUS_TIMEOUT), packet_idx (Index of the packet that
 * triggered this callback), and status (Status code: 0 for success, negative
 * errno for error).
 */
static void mspi_invoke_user_callback(const struct device *dev,
				 enum mspi_bus_event evt_type,
				 uint32_t packet_idx,
				 int status)
{
	struct mspi_sam_qspi_data *data = dev->data;
	struct mspi_context *ctx = &data->ctx;
	const struct mspi_xfer_packet *packet;
	struct mspi_callback_context *cb_ctx;

	/* Validate packet index */
	if (packet_idx >= ctx->xfer.num_packet) {
		LOG_ERR("Invalid packet index %u (max %u)", packet_idx,
			ctx->xfer.num_packet);
		return;
	}

	packet = &ctx->xfer.packets[packet_idx];
	cb_ctx = data->cb_ctxs[data->active_dev_idx][evt_type];

	/* Check if callback is registered for this device and event */
	if (data->cbs[data->active_dev_idx][evt_type] == NULL || cb_ctx == NULL) {
		return;
	}

	/* Check if this packet requests callback for this event type */
	if (!(packet->cb_mask & BIT(evt_type))) {
		return;
	}

	/* Populate callback context */
	cb_ctx->mspi_evt.evt_type = evt_type;
	cb_ctx->mspi_evt.evt_data.controller = dev;
	cb_ctx->mspi_evt.evt_data.dev_id = data->dev_id;
	cb_ctx->mspi_evt.evt_data.packet = packet;
	cb_ctx->mspi_evt.evt_data.packet_idx = packet_idx;
	cb_ctx->mspi_evt.evt_data.status = status;

	/* Invoke the callback(2nd argument tells if non-zero address
	 * length exists or not for the transfer)
	 */
	data->cbs[data->active_dev_idx][evt_type](cb_ctx, (int)(ctx->xfer.addr_length > 0));
}

void mspi_cleanup(struct mspi_sam_qspi_data *data, int async_result)
{
	int ret;

	const struct device *dev = data->dev;
	struct mspi_context *ctx = &data->ctx;
	uint32_t packet_idx = ctx->packets_done;
	qspi_registers_t *q = ((const struct mspi_sam_qspi_cfg *)dev->config)->reg_cfg.regs;

	/* All packets done or error occurred */
	if (async_result < 0) {
		mspi_invoke_user_callback(dev, MSPI_BUS_ERROR, packet_idx, async_result);
	}

	/* Note: Final MSPI_BUS_XFER_COMPLETE callback was already invoked above
	 * for the last packet if its cb_mask was set.
	 */

	/* Cancel any stale timeout work(It includes scenarios for any timeout that happens after
	 * ISR finishes and before work thread stops the timer)
	 */
	ret = k_work_cancel(&(data->async_timeout_work));
	if (ret < 0) {
		LOG_ERR("Failed to cancel work(Async packet work handler): %d", ret);
	}

	if (data->restore_spi_mode_on_done) {
		qspi_set_spi_mode(q);
		data->restore_spi_mode_on_done = false;
	}

	/* De-assert GPIO CS */
	gpio_cs_deassert(dev, data->active_dev_idx);

	/* Release context lock */
	mspi_context_release(ctx);
}

/*
 * Timer handler for async transfer timeout.
 * Called in timer/ISR context - must be fast, just submits work.
 */
static void mspi_async_timeout_timer_handler(struct k_timer *timer)
{
	struct mspi_sam_qspi_data *data =
		CONTAINER_OF(timer, struct mspi_sam_qspi_data, async_timer);
	int ret;

	/* Defer timeout handling to thread context */
	ret = k_work_submit(&data->async_timeout_work);
	if (ret < 0) {
		LOG_ERR("Async timeout timer handler error: %d", ret);
	}
}

/*
 * Work handler for timeout - runs in thread context.
 * Cleans up transfer, invokes MSPI_BUS_TIMEOUT callback.
 */
static void mspi_async_timeout_work_handler(struct k_work *work)
{
	struct mspi_sam_qspi_data *data =
		CONTAINER_OF(work, struct mspi_sam_qspi_data, async_timeout_work);
	const struct device *dev = data->dev;
	const struct mspi_sam_qspi_cfg *cfg = dev->config;
	struct mspi_context *ctx = &data->ctx;
	qspi_registers_t *q = cfg->reg_cfg.regs;

	/* Check if ISR already fired (packet actually completed) - stale timeout */
	if (data->async_packet_complete) {
		return;
	}

	/* Mark that timeout handler is taking over */
	data->async_timed_out = true;

	LOG_ERR("Async transfer timed out at packet %d", ctx->packets_done);

	/* Stop hardware - disable all interrupts */
	qspi_disable_all_interrupts(q);

	/* End the transfer (deassert CS) */
	qspi_end_transfer(q);

	if (data->restore_spi_mode_on_done) {
		qspi_set_spi_mode(q);
		data->restore_spi_mode_on_done = false;
	}

	/* deassert CS */
	gpio_cs_deassert(dev, data->active_dev_idx);

	/* Reset transfer state */
	data->qspi_obj.transfer_is_busy = false;
	data->is_last_byte_xfer_in_progress = false;

	/* Invoke timeout callback */
	mspi_invoke_user_callback(dev, MSPI_BUS_TIMEOUT, ctx->packets_done, -ETIMEDOUT);

	/* Release context lock */
	mspi_context_release(ctx);
}

/*
 * Work handler for packet completion - runs in thread context.
 * Called when ISR signals packet done via k_work_submit.
 *
 * Flow:
 * 1. Stop timeout timer
 * 2. Check for error from ISR
 * 3. Invoke per-packet callback if cb_mask set
 * 4. Increment packets_done
 * 5. If more packets: start next one
 * 6. If all done or error: invoke final callback, release context
 */
static void mspi_async_packet_work_handler(struct k_work *work)
{
	struct mspi_sam_qspi_data *data =
		CONTAINER_OF(work, struct mspi_sam_qspi_data, async_packet_work);
	const struct device *dev = data->dev;
	struct mspi_context *ctx = &data->ctx;
	uint32_t packet_idx = ctx->packets_done;
	int rc = 0;

	/* Check if timeout handler already ran and released context */
	if (data->async_timed_out) {
		return;
	}

	/* Stop timeout timer for this packet */
	k_timer_stop(&data->async_timer);

	/* Check for error from ISR */
	rc = data->async_result;
	if (rc < 0) {
		LOG_ERR("Packet %u failed with error %d", packet_idx, rc);
		mspi_cleanup(data, rc);
		return;
	}

	/* Per-packet callback if requested (cb_mask check is inside mspi_invoke_user_callback) */
	mspi_invoke_user_callback(dev, MSPI_BUS_XFER_COMPLETE, packet_idx, 0);

	/* Move to next packet */
	ctx->packets_done++;

	/* More packets to process? */
	if (ctx->packets_done < ctx->xfer.num_packet) {
		/* Reset completion flag for next packet */
		data->async_packet_complete = false;

		/*
		 * Restart timeout timer for next packet.
		 *
		 * TIMEOUT BEHAVIOR: Each packet gets its own independent timeout.
		 * Example: If timeout=10ms and there are 5 packets:
		 *   - Packet 1 has 10ms to complete before timeout
		 *   - Packet 2 has 10ms to complete (timer restarts after packet 1)
		 *   - ... and so on for each packet
		 *   - Total transfer could take up to 50ms (5 packets x 10ms each)
		 *     without timing out, as long as no single packet exceeds 10ms.
		 *
		 * This is NOT a total transfer timeout. If you need a hard limit on
		 * total transfer time, implement it in the application layer.
		 */
		k_timer_start(&data->async_timer,
			      K_MSEC(MSPI_EFFECTIVE_TIMEOUT(ctx->xfer.timeout)), K_NO_WAIT);

		/* Start next packet */
		rc = mspi_start_next_packet(dev);
		if (rc == 0) {
			return;  /* Continue async chain - ISR will call us again */
		}

		/* Failed to start next packet */
		packet_idx = ctx->packets_done;
		LOG_ERR("Failed to start packet %u: %d", packet_idx, rc);
	}

	mspi_cleanup(data, rc);
}

/*
 * Handle serial memory mode ISR (INSTREND interrupt).
 * NOTE: ISR only fires for async transfers (sync uses polling).
 */
static void mspi_isr_handle_serial_mode(const struct device *dev, uint32_t flags)
{
	const struct mspi_sam_qspi_cfg *cfg = dev->config;
	struct mspi_sam_qspi_data *data = dev->data;
	qspi_registers_t *q = cfg->reg_cfg.regs;

	int ret;

	/* Handle error flags */
	if ((flags & QSPI_INTFLAG_ERROR_Msk) != 0) {
		q->QSPI_INTFLAG = QSPI_INTFLAG_ERROR_Msk;

		/* Disable all interrupts - transfer will be aborted */
		qspi_disable_all_interrupts(q);
		data->async_result = -EIO;
		ret = k_work_submit(&data->async_packet_work);
		if (ret < 0) {
			LOG_ERR("Failed to submit async work(Serial mode ERROR ISR): %d", ret);
		}
		return;  /* Don't process other flags after error */
	}

	if ((flags & QSPI_INTFLAG_INSTREND_Msk) != 0) {
		q->QSPI_INTFLAG = QSPI_INTFLAG_INSTREND_Msk;
		q->QSPI_INTENCLR |= QSPI_INTENCLR_INSTREND(1);

		/* Mark packet complete and cancel any pending timeout work */
		data->async_packet_complete = true;
		ret = k_work_cancel(&data->async_timeout_work);
		if (ret < 0) {
			LOG_ERR("Failed to cancel work(Serial mode INSTREND ISR): %d", ret);
		}

		/* Async transfer complete - defer to workqueue thread */
		data->async_result = 0;
		ret = k_work_submit(&data->async_packet_work);
		if (ret < 0) {
			LOG_ERR("Failed to submit async work(Serial Mode): %d", ret);
		}
	}
}

/*
 * Queue the next TX byte based on transfer state.
 * Handles: address bytes -> pre-dummy -> dummy -> data.
 * Returns true if a byte was queued to TXDATA.
 */
static inline bool mspi_isr_queue_next_tx_byte(qspi_registers_t *q,
					  struct mchp_hal_spi_pio_transfer *obj,
					  const struct mspi_xfer_packet *packet)
{
	if (obj->cmd_bytes_remaining > 0) {
		q->QSPI_TXDATA = (uint8_t)(packet->cmd);
		obj->cmd_bytes_remaining--;
		obj->blocks++;
		return true;
	}

	if (obj->addrlen > 0) {
		q->QSPI_TXDATA = (uint8_t)((packet->address >>
					    (8 * (obj->addrlen - 1))) & 0xFF);
		obj->addrlen--;
		obj->blocks++;
		return true;
	}
	if (obj->dummy_bytes > 0) {
		q->QSPI_TXDATA = QSPI_DUMMY_CYCLE_BYTE;
		obj->dummy_bytes--;
		obj->blocks++;
		return true;
	}
	if (obj->rx_buffer_bytes > 0) {
		q->QSPI_TXDATA = QSPI_RX_CLOCK_BYTE;
		obj->rx_buffer_bytes--;
		obj->blocks++;
		return true;
	}
	if ((obj->tx_count < packet->num_bytes) && (packet->dir == MSPI_TX)) {
		q->QSPI_TXDATA = packet->data_buf[obj->tx_count];
		obj->tx_count++;
		obj->blocks++;
		return true;
	}
	return false;
}

/*
 * Handle RXC (Receive Complete) interrupt.
 * Returns 0 on success, negative error code on failure.
 */
static inline int mspi_isr_spi_handle_rxc(qspi_registers_t *q,
				     struct mchp_hal_spi_pio_transfer *obj,
				     const struct mspi_xfer_packet *packet)
{
	/* RXC flag clears when data read from RXDATA */
	uint32_t rx_data = (q->QSPI_RXDATA & QSPI_RXDATA_DATA_Msk) >> QSPI_RXDATA_DATA_Pos;

	if (obj->rx_drop != 0) {
		obj->rx_drop--;
	} else if ((packet->dir == MSPI_RX) && (obj->rx_count < packet->num_bytes)) {
		if (packet->data_buf == NULL) {
			return -EINVAL;
		}
		packet->data_buf[obj->rx_count] = rx_data;
		obj->rx_count++;
	} else if ((packet->dir == MSPI_RX) && (obj->rx_count >= packet->num_bytes)) {
		/* RX buffer overflow - should not happen if transfer configured correctly */
		return -EOVERFLOW;
	}

	return 0;
}

/*
 * Handle DRE (Data Register Empty) interrupt - transmit next byte.
 * Returns 0 on success, negative error code on failure.
 */
static inline int mspi_isr_spi_handle_dre(qspi_registers_t *q,
				     struct mspi_sam_qspi_data *data,
				     const struct mspi_xfer *xfer,
				     const struct mspi_xfer_packet *packet)
{
	struct mchp_hal_spi_pio_transfer *obj = &data->qspi_obj;

	size_t total_bytes = xfer->cmd_length + xfer->addr_length +
			     obj->dummy_bytes_fix + packet->num_bytes;

	q->QSPI_INTENCLR = QSPI_INTENCLR_DRE_Msk;

	/* Check for NULL data_buf before accessing it */
	if ((packet->dir == MSPI_TX) && (obj->tx_count < packet->num_bytes)) {
		if (packet->data_buf == NULL) {
			return -EINVAL;
		}
	}

	mspi_isr_queue_next_tx_byte(q, obj, packet);

	/* Check if more bytes to send */
	if (obj->blocks != total_bytes) {
		q->QSPI_INTENSET = QSPI_INTENSET_RXC_Msk | QSPI_INTENSET_DRE_Msk;
	}

	/* Check if last byte queued */
	if ((obj->blocks == total_bytes) &&
	    (data->is_last_byte_xfer_in_progress != true)) {
		data->is_last_byte_xfer_in_progress = true;
	} else if (((obj->rx_count == packet->num_bytes) && (packet->dir == MSPI_RX)) ||
		   ((obj->rx_drop == 0) && (packet->dir == MSPI_TX))) {
		q->QSPI_INTENCLR = QSPI_INTENCLR_RXC_Msk;
		q->QSPI_INTENSET = QSPI_INTENSET_DRE_Msk;
	}

	return 0;
}

/*
 * Handle TXC (Transmit Complete) interrupt - end transfer.
 * NOTE: ISR only fires for async transfers (sync uses polling).
 */
static inline void mspi_isr_spi_handle_txc(qspi_registers_t *q,
				      struct mspi_sam_qspi_data *data,
				      struct mspi_context *ctx,
				      const struct mspi_xfer *xfer,
				      const struct mspi_xfer_packet *packet)
{

	int ret;
	struct mchp_hal_spi_pio_transfer *obj = &data->qspi_obj;
	size_t total_bytes = xfer->cmd_length + xfer->addr_length +
			     obj->dummy_bytes_fix + packet->num_bytes;

	if (((obj->rx_count == packet->num_bytes) && (packet->dir == MSPI_RX)) ||
	    ((obj->blocks == total_bytes) && (packet->dir == MSPI_TX))) {
		obj->transfer_is_busy = false;
		qspi_end_transfer(q);
		q->QSPI_INTENCLR = QSPI_INTENCLR_DRE_Msk | QSPI_INTENCLR_RXC_Msk |
				   QSPI_INTENCLR_TXC_Msk;
		data->is_last_byte_xfer_in_progress = false;

		/* Mark packet complete and cancel any pending timeout work */
		data->async_packet_complete = true;
		ret = k_work_cancel(&data->async_timeout_work);
		if (ret < 0) {
			LOG_ERR("Failed to cancel pending timeout work(SPI mode ISR TXC): %d", ret);
		}

		/* Async transfer complete - defer to workqueue thread */
		data->async_result = 0;
		ret = k_work_submit(&data->async_packet_work);
		if (ret < 0) {
			LOG_ERR("Failed to submit async work(SPI mode TXC ISR): %d", ret);
		}
	}
}

/*
 * Handle SPI mode ISR (RXC/DRE/TXC/ERROR interrupts).
 */
static void mspi_isr_handle_spi_mode(const struct device *dev, uint32_t flags)
{
	const struct mspi_sam_qspi_cfg *cfg = dev->config;
	struct mspi_sam_qspi_data *data = dev->data;
	struct mspi_context *ctx = &data->ctx;
	const struct mspi_xfer *xfer = &data->ctx.xfer;
	qspi_registers_t *q = cfg->reg_cfg.regs;
	const struct mspi_xfer_packet *packet;

	int ret;

	/* Safety check: callback_ctx should never be NULL in async ISR, but verify */
	if (ctx->callback_ctx == NULL) {
		LOG_ERR("ISR called with NULL callback_ctx - disabling all interrupts");
		qspi_disable_all_interrupts(q);
		return;
	}

	packet = ctx->callback_ctx->mspi_evt.evt_data.packet;

	/* Handle error flags */
	if ((flags & QSPI_INTFLAG_ERROR_Msk) != 0) {
		q->QSPI_INTFLAG = QSPI_INTFLAG_ERROR_Msk;

		/* Disable all interrupts - transfer will be aborted */
		qspi_disable_all_interrupts(q);
		(void)q->QSPI_RXDATA;
		data->async_result = -EIO;
		ret = k_work_submit(&data->async_packet_work);
		if (ret < 0) {
			LOG_ERR("Failed to submit async work(SPI mode ERROR ISR): %d", ret);
		}
		return;  /* Don't process other flags after error */
	}

	/* RXC: Received data available */
	if ((flags & QSPI_INTFLAG_RXC_Msk) != 0) {
		ret = mspi_isr_spi_handle_rxc(q, &data->qspi_obj, packet);

		if (ret != 0) {
			qspi_disable_all_interrupts(q);
			(void)q->QSPI_RXDATA;
			data->async_result = ret;
			ret = k_work_submit(&data->async_packet_work);
			if (ret < 0) {
				LOG_ERR("Failed to submit async work(SPI mode RXC ERROR): %d", ret);
			}
			return;
		}
	}

	/* DRE: Data register empty - ready to transmit */
	if ((flags & QSPI_INTFLAG_DRE_Msk) != 0) {
		ret = mspi_isr_spi_handle_dre(q, data, xfer, packet);

		if (ret != 0) {
			qspi_disable_all_interrupts(q);
			(void)q->QSPI_RXDATA;
			data->async_result = ret;
			ret = k_work_submit(&data->async_packet_work);
			if (ret < 0) {
				LOG_ERR("Failed to submit async work(SPI mode DRE ERROR): %d", ret);
			}
			return;
		}
	}

	/* TXC: Transfer complete */
	if (((flags & QSPI_INTFLAG_TXC_Msk) != 0) &&
	    (data->is_last_byte_xfer_in_progress)) {
		mspi_isr_spi_handle_txc(q, data, ctx, xfer, packet);
	}

	/* Enable TXC interrupt for last byte */
	if (data->is_last_byte_xfer_in_progress) {
		q->QSPI_INTENSET = QSPI_INTENSET_TXC_Msk;
	}
}

/*
 * MSPI interrupt handler - dispatches to mode-specific handlers.
 */
static void qspi_isr(const void *arg)
{
	const struct device *dev = arg;
	const struct mspi_sam_qspi_cfg *cfg = dev->config;
	qspi_registers_t *q = cfg->reg_cfg.regs;
	uint32_t flags = q->QSPI_INTFLAG;

	if ((q->QSPI_CTRLB & QSPI_CTRLB_MODE_Msk) != 0) {
		mspi_isr_handle_serial_mode(dev, flags);
	} else {
		mspi_isr_handle_spi_mode(dev, flags);
	}
}

static inline int qspi_validate_controller(const struct device *controller)
{
	if ((controller == NULL) || (controller->config == NULL) ||
	    (controller->data == NULL)) {
		LOG_ERR("Invalid QSPI controller");
		return -EINVAL;
	}

	return 0;
}

/*
 * Disable/De-initialize features of QSPI.
 * This function disables the QSPI module and in future may disable power control.
 *
 * Takes controller (pointer to the MSPI device structure).
 *
 * Returns 0 on success, negative error on failure.
 */
static int qspi_deinit(const struct device *controller)
{
	const struct mspi_sam_qspi_cfg *cfg;
	int ret;

	qspi_registers_t *q;

	if (qspi_validate_controller(controller) != 0) {
		return -EINVAL;
	}

	cfg = controller->config;
	q = cfg->reg_cfg.regs;

	if (q == NULL) {
		LOG_ERR("QSPI register pointer is NULL");
		return -EINVAL;
	}

	/* disable QSPI(try to call fn for this and use data->mspiHandle as argument in that) */
	ret = qspi_set_enabled(q, false);

	return ret;
}

/*
 * Register an MSPI bus event callback for a controller / target pair.
 *
 * Associates a user callback with the specified MSPI controller and device ID
 * so it is invoked when the given bus event type occurs (e.g., transfer
 * completion, error, or status change).
 *
 * The driver stores cb and the optional ctx pointer and will pass them back
 * to the handler when the event fires. Handlers may be called from ISR or
 * driver thread context depending on the backend; they must be fast and
 * non-blocking. If a callback was previously registered for the same
 * {controller, dev_id, evt_type}, the new one replaces it.
 *
 * Takes controller (MSPI controller device handle), dev_id (target device
 * identifier on the MSPI bus), evt_type (event to subscribe to), cb (callback
 * function to invoke on evt_type, must be non-NULL), and ctx (optional user
 * context passed back to cb, may be NULL).
 *
 * Returns 0 on success, -ENOTSUP if callbacks are not supported for evt_type
 * or backend.
 */
static int mspi_mchp_register_callback(const struct device *controller,
				       const struct mspi_dev_id *dev_id,
				       const enum mspi_bus_event evt_type,
				       mspi_callback_handler_t cb,
				       struct mspi_callback_context *ctx)
{
	const struct mspi_sam_qspi_cfg *config;
	struct mspi_sam_qspi_data *data;

	if (qspi_validate_controller(controller) != 0) {
		return -EINVAL;
	}

	if ((dev_id == NULL) || (cb == NULL) || (ctx == NULL)) {
		LOG_ERR("%s: Device ID, callback function, and callback context must be non-NULL",
			__func__);
		return -EINVAL;
	}

	config = controller->config;
	data = controller->data;

	if (dev_id->dev_idx >= config->num_children) {
		LOG_ERR("%s: Invalid device index %u (max %u)", __func__,
			dev_id->dev_idx, config->num_children - 1);
		return -EINVAL;
	}

	if (evt_type != MSPI_BUS_XFER_COMPLETE &&
	    evt_type != MSPI_BUS_ERROR &&
	    evt_type != MSPI_BUS_TIMEOUT) {
		LOG_ERR("%s: callback type %d not supported", __func__, evt_type);
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock_dev, K_FOREVER);
	data->cbs[dev_id->dev_idx][evt_type] = cb;
	data->cb_ctxs[dev_id->dev_idx][evt_type] = ctx;
	k_mutex_unlock(&data->lock_dev);

	return 0;
}

/*
 * Device-specific configuration for QSPI module.
 *
 * This function configures QSPI module by setting device specific parameters
 * such as frequency/baud rate, width of instruction/address/data, data rate,
 * clock polarity, clock phase, chip select polarity, dummy cycles,
 * read/write commands, command/address length, memory boundary,
 * and time to break up a transfer.
 *
 * Takes controller (pointer to the MSPI device structure), dev_id (pointer
 * to the MSPI device ID structure), param_mask (bitmask of parameters to
 * configure), and cfg (pointer to the MSPI device configuration structure).
 *
 * Returns 0 on success, negative error code on failure.
 */
static int mspi_mchp_dev_config(const struct device *controller, const struct mspi_dev_id *dev_id,
				const enum mspi_dev_cfg_mask param_mask,
				const struct mspi_dev_cfg *cfg)
{
	int ret;
	bool serial_mode = false;
	uint32_t mask = (uint32_t)param_mask;
	uint8_t baud;
	uint32_t ahb2x_clk_freq;
	uint32_t tries;

	const struct mspi_sam_qspi_cfg *ctrl_cfg;
	struct mspi_sam_qspi_data *ctrl_data;
	qspi_registers_t *q;

	if (qspi_validate_controller(controller) != 0) {
		return -EINVAL;
	}

	if ((cfg == NULL) || (dev_id == NULL)) {
		LOG_ERR("Invalid QSPI controller parameters");
		return -EINVAL;
	}

	ctrl_cfg = controller->config;
	ctrl_data = controller->data;
	q = ctrl_cfg->reg_cfg.regs;

	ret = mspi_find_serial_mode_of_child(ctrl_cfg->child_desc, dev_id->dev_idx,
					     ctrl_cfg->num_children, &serial_mode);
	if (ret != 0) {
		/* Invalid child lookup parameters */
		LOG_ERR("%s: Invalid child lookup parameters", __func__);
		return ret;
	}

	/* generic command-length validation first */
	ret = mspi_check_cmd_length(mask, cfg);
	if (ret != 0) {
		return ret;
	}

	/* Chip Select Polarity Sanity check */
	ret = mspi_check_cs_polarity(mask, cfg);
	if (ret != 0) {
		return ret;
	}

	/* Check Data Strobe support */
	ret = mspi_check_dqs_support(mask);
	if (ret != 0) {
		return ret;
	}

	/* Read and write command sanity check */
	ret = mspi_check_read_write_cmd(mask, cfg);
	if (ret != 0) {
		return ret;
	}

	/* Memory Boundary input sanity check */
	ret = mspi_check_mem_bound(mask, cfg);
	if (ret != 0) {
		return ret;
	}

	ret = mspi_check_cs_index_limit(mask, dev_id, ctrl_cfg);
	if (ret != 0) {
		return ret;
	}

	/* ATSAME54 serial memory mode supports only 1-byte instruction */
	if ((serial_mode) && (cfg->cmd_length > 1U)) {
		LOG_ERR("Only 1 byte commands allowed in serial mode");
		return -ENOTSUP;
	}

	/* Lock before hardware-changing configuration */
	k_mutex_lock(&ctrl_data->lock_dev, K_FOREVER);

	/* disable QSPI before changing QSPI registers */
	ret = qspi_set_enabled(q, false);
	if (ret != 0) {
		k_mutex_unlock(&ctrl_data->lock_dev);
		LOG_ERR("QSPI disablement failed");
		return ret;
	}

	/* Set baud rate */
	ret = mspi_set_baud(mask, cfg, ctrl_cfg, q);
	if (ret != 0) {
		k_mutex_unlock(&ctrl_data->lock_dev);
		LOG_ERR("mspi_set_baud failed");
		return ret;
	}

	/* Set WIDTH */
	ret = mspi_set_width(mask, cfg, q);
	if (ret != 0) {
		k_mutex_unlock(&ctrl_data->lock_dev);
		LOG_ERR("mspi_set_width failed");
		return ret;
	}

	/* Set DATA RATE */
	baud = 0;
	ahb2x_clk_freq = 0;
	tries = CLOCK_FREQ_RETRY_COUNT;

	if ((mask & MSPI_DEVICE_CONFIG_DATA_RATE) != 0) {
		switch (cfg->data_rate) {
		case MSPI_DATA_RATE_SINGLE:
			q->QSPI_INSTRFRAME &= ~QSPI_INSTRFRAME_DDREN_Msk;
			break;

		case MSPI_DATA_RATE_S_S_D:
			/* only in serial memory mode */
			if (serial_mode) {
				/* CLK_QSPI2X_AHB clock must be enabled in mspi_config() before
				 * enabling the DDREN bit.
				 */
				ret = clock_control_on(DEVICE_DT_GET(CLOCK_NODE),
						       ctrl_cfg->mspi_clock.mclk_ahb2x);
				if ((ret != 0) && (ret != -EALREADY)) {
					k_mutex_unlock(&ctrl_data->lock_dev);
					LOG_ERR("MCLK AHB2X not ready");
					return ret;
				}

				/* wait for QSPI_AHB2X to get its frequency from clock driver */
				tries = CLOCK_FREQ_RETRY_COUNT;
				do {
					ret = clock_control_get_rate(
						DEVICE_DT_GET(CLOCK_NODE),
						ctrl_cfg->mspi_clock.mclk_ahb2x,
						&ahb2x_clk_freq);
					if (ret != 0) {
						k_mutex_unlock(&ctrl_data->lock_dev);
						LOG_ERR("clock_control_get_rate failed");
						return ret;
					}
					k_sleep(K_MSEC(CLOCK_FREQ_POLL_INTERVAL_MS));
					if (--tries == 0) {
						k_mutex_unlock(&ctrl_data->lock_dev);
						LOG_ERR("Timed out getting frequency of "
							"clock AHB2X");
						return -ETIMEDOUT;
					}
				} while (ahb2x_clk_freq == 0);
				LOG_DBG("AHB2X clock freq %d", ahb2x_clk_freq);

				/* set DDREN bit when AHB2X clock is up and running */
				if (clock_control_get_status(DEVICE_DT_GET(CLOCK_NODE),
						     ctrl_cfg->mspi_clock.mclk_ahb2x) !=
				    CLOCK_CONTROL_STATUS_ON) {
					k_mutex_unlock(&ctrl_data->lock_dev);
					LOG_ERR("MCLK AHB2X is not ON");
					return -EINVAL;
				}

				if ((cfg->freq == 0) ||
				    (cfg->freq > MSPI_MCHP_CONTROLLER_MAX_FREQ)) {
					k_mutex_unlock(&ctrl_data->lock_dev);
					LOG_ERR("Invalid frequency for S_S_D data rate");
					return -EINVAL;
					k_mutex_unlock(&ctrl_data->lock_dev);
					LOG_ERR("Invalid frequency for S_S_D data rate");
					return -EINVAL;
				}

				/* Enable double data rate (DDREN) for S_S_D transfers */
				q->QSPI_INSTRFRAME |= QSPI_INSTRFRAME_DDREN_Msk;

				/* Calculate baud with rounding to nearest achievable frequency */
				ret = qspi_calc_baud(ahb2x_clk_freq, cfg->freq, &baud);
				if (ret != 0) {
					k_mutex_unlock(&ctrl_data->lock_dev);
					LOG_ERR("Baud rate divisor error");
					return ret;
				}
				qspi_set_baud(q, baud);
			} else {
				k_mutex_unlock(&ctrl_data->lock_dev);
				LOG_ERR("S_S_D mode only for serial memory mode");
				return -EINVAL;
			}
			break;

		/* only for readmemory instruction */
		case MSPI_DATA_RATE_S_D_D:
			k_mutex_unlock(&ctrl_data->lock_dev);
			LOG_ERR("S_D_D not supported: DDR applies to data phase only "
				"(no per-phase DDR control)");
			return -ENOTSUP;

		case MSPI_DATA_RATE_DUAL:
			k_mutex_unlock(&ctrl_data->lock_dev);
			LOG_ERR("Full DDR not supported: DDR is read-only on this MCU");
			return -ENOTSUP;

		default:
			k_mutex_unlock(&ctrl_data->lock_dev);
			LOG_ERR("Unknown data rate: %d", cfg->data_rate);
			return -EINVAL;
		}
	}

	/* Set Clock Polarity and Phase */
	ret = set_clock_pol_pha(mask, cfg, q);
	if (ret != 0) {
		k_mutex_unlock(&ctrl_data->lock_dev);
		LOG_ERR("set_clock_pol_pha failed");
		return ret;
	}

	/* Set Address & its Length input sanity check() */
	if (serial_mode) {
		ret = mspi_check_addr_length(mask, cfg, q);
		if (ret != 0) {
			k_mutex_unlock(&ctrl_data->lock_dev);
			LOG_ERR("mspi_check_addr_length failed!");
			return ret;
		}

		/* Dummy Cycles input sanity check(Serial memory mode only)
		 * Set default dummy cycles
		 */
		ret = mspi_check_rx_tx_dummy(mask, cfg, q);
		if (ret != 0) {
			k_mutex_unlock(&ctrl_data->lock_dev);
			LOG_ERR("mspi_check_rx_tx_dummy failed!");
			return ret;
		}
	}

	if (serial_mode) {
		if (qspi_get_mode(q) != 0) {
			k_mutex_unlock(&ctrl_data->lock_dev);
			LOG_ERR("Only Mode 0 allowed in serial mode");
			return -EINVAL;
		}
		qspi_set_serial_memory_mode(q);
		LOG_DBG("Serial memory mode enabled");
	}

	/* if communicating with SPI device(not serial memory)
	 * 1 - set to SPI mode
	 * 2 - set CSMODE to 1 to foolproof any CS de-asserts when next byte transfer begins
	 */
	else {
		qspi_set_spi_mode(q);
	}

	/* STATUS register */
	if (qspi_is_cs_deassert(q) == false) {
		k_mutex_unlock(&ctrl_data->lock_dev);
		LOG_ERR("CS line not de-asserted");
		return -EIO;
	}

	ret = qspi_set_enabled(q, true);
	if (ret != 0) {
		k_mutex_unlock(&ctrl_data->lock_dev);
		LOG_ERR("QSPI enablement failed!");
		return ret;
	}

	ctrl_data->dev_id = dev_id;
	k_mutex_unlock(&ctrl_data->lock_dev);

	return ret;
}

/*
 * Configure timing parameters for QSPI controller.
 *
 * This function configures timing-related parameters such as delays and clock
 * settings for the QSPI controller. Timing configuration is controller-level
 * on SAM D5x/E5x, not per-device.
 *
 * Takes controller (pointer to the MSPI device structure), dev_id (pointer to
 * the MSPI device ID structure, unused on this hardware), param_mask (bitmask
 * of timing parameters to configure), and cfg (pointer to timing configuration).
 *
 * Returns 0 on success, negative error code on failure.
 */
static int mspi_mchp_timing_config(const struct device *controller,
				   const struct mspi_dev_id *dev_id,
				   const uint32_t param_mask, void *cfg)
{
	const struct mspi_sam_qspi_cfg *config;
	struct mspi_sam_qspi_data *data;
	qspi_registers_t *q;
	uint32_t supported_delay_masks;
	struct mchp_qspi_timing_cfg *timing_params;

	int ret;

	ARG_UNUSED(dev_id); /* Timing is handled by controller not device on SAMD5x/SAME5x*/

	if (param_mask == 0) {
		LOG_WRN("No timing to configure");
		return 0;
	}

	supported_delay_masks = MSPI_MCHP_TIMING_DLYBS |
									 MSPI_MCHP_TIMING_DLYCS |
									 MSPI_MCHP_TIMING_DLYBCT;

	timing_params = (struct mchp_qspi_timing_cfg *)cfg;

	if (qspi_validate_controller(controller) != 0) {
		return -EINVAL;
	}

	config = controller->config;
	data = controller->data;
	q = config->reg_cfg.regs;

	if (timing_params == NULL) {
		LOG_ERR("NULL delay configuration input");
		return -EINVAL;
	}

	if ((param_mask & ~(supported_delay_masks)) != 0) {
		LOG_ERR("Invalid delay parameter masks");
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock_dev, K_FOREVER);

	/* Runtime timing configuration requires QSPI to be disabled.
	 * CTRLB and BAUD register writes may be ignored or corrupt hardware state
	 * if QSPI is enabled, especially in Serial Memory mode (MODE=1).
	 */
	ret = qspi_set_enabled(q, false);
	if (ret != 0) {
		LOG_ERR("Failed to disable QSPI for timing config: %d", ret);
		k_mutex_unlock(&data->lock_dev);
		return ret;
	}

	if ((param_mask & MSPI_MCHP_TIMING_DLYBS) != 0) {
		q->QSPI_BAUD = ((q->QSPI_BAUD) & ~(QSPI_BAUD_DLYBS_Msk)) |
			       QSPI_BAUD_DLYBS(timing_params->dlybs);
	}

	if ((param_mask & MSPI_MCHP_TIMING_DLYCS) != 0) {
		q->QSPI_CTRLB = ((q->QSPI_CTRLB) & ~(QSPI_CTRLB_DLYCS_Msk)) |
					QSPI_CTRLB_DLYCS(timing_params->dlycs);
	}

	if ((param_mask & MSPI_MCHP_TIMING_DLYBCT) != 0) {
		q->QSPI_CTRLB = ((q->QSPI_CTRLB) & ~(QSPI_CTRLB_DLYBCT_Msk)) |
					QSPI_CTRLB_DLYBCT(timing_params->dlybct);
	}

	/* Re-enable QSPI after timing configuration */
	ret = qspi_set_enabled(q, true);
	if (ret != 0) {
		LOG_ERR("Failed to re-enable QSPI after timing config: %d", ret);
		k_mutex_unlock(&data->lock_dev);
		return ret;
	}

	k_mutex_unlock(&data->lock_dev);
	return 0;
}

/*
 * Apply timing configuration from devicetree during initialization.
 *
 * This function configures timing parameters (DLYBS, DLYCS, DLYBCT) from
 * devicetree properties during boot-time initialization. Called once per
 * child device during mspi_apply_child_configs().
 *
 * Note: WDRBT bit is always set unconditionally to prevent RX overrun.
 */
static void qspi_apply_timing(qspi_registers_t *q, const struct mspi_child_cfg *child)
{
	/* Set DLYBS (Delay Before SCK) in BAUD register */
	if (child->dlybs != 0) {
		q->QSPI_BAUD = (q->QSPI_BAUD & ~QSPI_BAUD_DLYBS_Msk) |
			       QSPI_BAUD_DLYBS(child->dlybs);
	}

	/* Set DLYCS (Min Inactive CS Delay) in CTRLB register */
	if (child->dlycs != 0) {
		q->QSPI_CTRLB = (q->QSPI_CTRLB & ~QSPI_CTRLB_DLYCS_Msk) |
				QSPI_CTRLB_DLYCS(child->dlycs);
	}

	/* Set DLYBCT (Delay Between Consecutive Transfers) in CTRLB register */
	if (child->dlybct != 0) {
		q->QSPI_CTRLB = (q->QSPI_CTRLB & ~QSPI_CTRLB_DLYBCT_Msk) |
				QSPI_CTRLB_DLYBCT(child->dlybct);
	}

	/* WDRBT: Wait Data Read Before Transfer - prevents RX overrun */
#ifdef QSPI_CTRLB_WDRBT_Msk
	q->QSPI_CTRLB |= QSPI_CTRLB_WDRBT_Msk;
#else
	q->QSPI_CTRLB |= QSPI_CTRLB_WDRBT_BIT;
#endif
}

/* Configure the child devices attached to QSPI module */
static int mspi_apply_child_configs(const struct device *controller)
{
	const struct mspi_sam_qspi_cfg *cfg;
	const struct mspi_child_cfg *child_cfg_ptr;
	int ret;

	if (qspi_validate_controller(controller) != 0) {
		return -EINVAL;
	}

	cfg = controller->config;
	child_cfg_ptr = cfg->child_cfg;

	if ((child_cfg_ptr == NULL) || (cfg->num_children == 0U)) {
		LOG_ERR("No children or invalid child configuration");
		return -EINVAL;
	}

	for (uint16_t i = 0U; i < cfg->num_children; i++) {
		ret = mspi_mchp_dev_config(controller,
					   &child_cfg_ptr[i].id,
					   (MSPI_DEVICE_CONFIG_ALL & ~(MSPI_DEVICE_CONFIG_DQS)),
					   &child_cfg_ptr[i].cfg);
		if (ret != 0) {
			LOG_ERR("Device config failed for dev_idx %d",
				child_cfg_ptr[i].id.dev_idx);
			return ret;
		}

		/* Apply timing configuration from devicetree */
		qspi_apply_timing(cfg->reg_cfg.regs, &child_cfg_ptr[i]);
	}

	return 0;
}

/* MSPI hardware intializer
 * Enable APB clock
 * Enable AHB clock
 * Configure QSPI controller pins
 * Reset QSPI controller
 * Clear and disable all QSPI interrupts
 */

static int qspi_hw_init(const struct device *controller)
{
	const struct mspi_sam_qspi_cfg *cfg;
	qspi_registers_t *q;
	const struct device *device_clk = DEVICE_DT_GET(CLOCK_NODE);
	int ret;

	if (qspi_validate_controller(controller) != 0) {
		return -EINVAL;
	}

	cfg = controller->config;
	q = cfg->reg_cfg.regs;

	if (q == NULL) {
		LOG_ERR("QSPI register pointer is NULL");
		return -EINVAL;
	}

	if (cfg->pcfg == NULL) {
		LOG_ERR("Invalid pinctrl configuration");
		return -EINVAL;
	}

	if (device_is_ready(device_clk) == false) {
		LOG_WRN("Clocks not ready");
		return -ENODEV;
	}

	ret = clock_control_on(device_clk, cfg->mspi_clock.mclk_apb);
	if ((ret < 0) && (ret != -EALREADY)) {
		LOG_ERR("MCLK APB not ready");
		return ret;
	}

	ret = clock_control_on(device_clk, cfg->mspi_clock.mclk_ahb);
	if ((ret < 0) && (ret != -EALREADY)) {
		LOG_ERR("MCLK AHB not ready");
		return ret;
	}

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("Pin control apply state failed=%d", ret);
		return ret;
	}

	/* Initialize GPIO chip selects in software multi-peripheral mode */
	if (cfg->sw_multi_periph) {
		for (uint8_t i = 0; i < cfg->ce_gpios_len; i++) {
			const struct gpio_dt_spec *ce = &cfg->ce_gpios[i];

			if (ce->port == NULL) {
				continue;  /* Skip if no GPIO CS for this device */
			}

			if (device_is_ready(ce->port) == false) {
				LOG_ERR("GPIO port for CE[%d] not ready", i);
				return -ENODEV;
			}

			ret = gpio_pin_configure_dt(ce, GPIO_OUTPUT_INACTIVE);
			if (ret != 0) {
				LOG_ERR("Failed to configure CE[%d] GPIO: %d", i, ret);
				return ret;
			}
		}
	}

	ret = qspi_swrst(q);
	if (ret != 0) {
		LOG_ERR("QSPI software reset timed out=%d", ret);
		return ret;
	}
	qspi_clear_all_interrupts(q);
	qspi_disable_all_interrupts(q);

	return 0;
}

/*
 * Basic configuration for QSPI module.
 *
 * This function brings up QSPI module by enabling clocks, pin control,
 * clears any pending interrupt flags, disables interrupt, and enables
 * the QSPI module.
 *
 * Takes spec (pointer to the MSPI device tree specification).
 * Controller-level re-init at runtime: mspi_config()
 * Returns 0 on success, negative error code on failure.
 */
static int mspi_mchp_config(const struct mspi_dt_spec *spec)
{
	const struct mspi_cfg *config;
	const struct mspi_sam_qspi_cfg *cfg;
	struct mspi_sam_qspi_data *data;

	qspi_registers_t *q;

	int ret;

	if ((spec == NULL) || (spec->bus == NULL)) {
		LOG_ERR("Invalid(NULL) MSPI controller specs");
		return -EINVAL;
	}

	if ((spec->bus->config == NULL) || (spec->bus->data == NULL)) {
		LOG_ERR("Invalid(NULL) peripheral static/run-time configuration");
		return -EINVAL;
	}

	config = &spec->config;
	cfg = spec->bus->config;
	data = spec->bus->data;
	q = cfg->reg_cfg.regs;

	if (q == NULL) {
		LOG_ERR("QSPI register pointer is NULL");
		return -EINVAL;
	}
	if (cfg->irq_config_func == NULL) {
		LOG_ERR("Pointer to MSPI ISR is NULL");
		return -EINVAL;
	}

	ret = 0;

	/* Arguments sanity checks */
	if (config->channel_num > QSPI_MAX_CHANNEL_NUM) {
		LOG_ERR("%s: Channel number %u exceeds max %d", __func__,
			config->channel_num, QSPI_MAX_CHANNEL_NUM);
		return -ENOTSUP;
	}
	if (config->op_mode != MSPI_OP_MODE_CONTROLLER) {
		LOG_ERR("%s: Only controller mode supported (got %d)", __func__, config->op_mode);
		return -ENOTSUP;
	}
	if (config->max_freq > MSPI_MCHP_CONTROLLER_MAX_FREQ) {
		LOG_ERR("%s: Requested freq %u exceeds max %u", __func__,
			config->max_freq, MSPI_MCHP_CONTROLLER_MAX_FREQ);
		return -ENOTSUP;
	}
	if (config->dqs_support) {
		LOG_ERR("%s: DQS not supported on this hardware", __func__);
		return -ENOTSUP;
	}
	if (config->duplex != MSPI_HALF_DUPLEX) {
		LOG_ERR("%s: Only half-duplex supported (MSPI API limitation)", __func__);
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock_init, K_FOREVER);

	/* interrupt flags clear and disable interrupt */
	qspi_clear_all_interrupts(q);

	/* interrupt disable */
	qspi_disable_all_interrupts(q);

	if (config->re_init) {
		/* disable qspi and re-init */
		ret = qspi_deinit(spec->bus);
		if (ret != 0) {
			k_mutex_unlock(&data->lock_init);
			LOG_ERR("Failed to de-initialize MSPI controller");
			return ret;
		}
		ret = qspi_hw_init(spec->bus);
		if (ret != 0) {
			k_mutex_unlock(&data->lock_init);
			LOG_ERR("Failed to re-initialize MSPI controller hardware");
			return ret;
		}

		ret = mspi_apply_child_configs(spec->bus);
		if (ret != 0) {
			k_mutex_unlock(&data->lock_init);
			LOG_ERR("Failed to re-initialize MSPI children hardware");
			return ret;
		}
	}

	/* enable QSPI module */
	ret = qspi_set_enabled(q, true);
	if (ret != 0) {
		k_mutex_unlock(&data->lock_init);
		return ret;
	}

	/* Connects the hardware interrupt to your ISR
	 * Enables the interrupt in the NVIC / interrupt controller
	 * (Actually calls mspi_mchp_irq_config_##inst function)
	 */
	cfg->irq_config_func(spec->bus);

	k_mutex_unlock(&data->lock_init);

	return ret;
}

#if defined(CONFIG_MSPI_MEMMAP)
/**
 * Check if memory access is within XIP bounds for the active device.
 * Returns 0 if allowed, negative error if rejected.
 * If XIP not enabled for this device, allows all operations (pass-through).
 */
static int mspi_check_memory_bounds(const struct mspi_sam_qspi_data *data,
				     const struct mspi_xfer_packet *packet)
{
	struct mspi_memmap_cfg *cfg;

	uint32_t start_address;
	uint32_t end_address;

	if (data == NULL) {
		LOG_ERR("%s: Data pointer is NULL", __func__);
		return -EINVAL;
	}

	cfg = &data->memmap_cfg[data->active_dev_idx];
	start_address = cfg->address_offset;
	end_address = start_address + cfg->size;

	/* verify is XIP flag is enabled or not for memory read*/
	if (((data->xip_enabled & BIT(data->active_dev_idx)) == 0) &&
		(packet->dir == MSPI_RX)) {
		LOG_WRN("Please enable xip_enable flag for memory read for the "
			"device id=%u", data->active_dev_idx);
		return 0;
	}

	/* Verify permissions for MSPI_RX */
	if ((packet->dir == MSPI_RX) &&
	    (data->memmap_cfg[data->active_dev_idx].permission !=
	     MSPI_MEMMAP_READ_ONLY)) {
		LOG_WRN("Permission not set to READ only memory read for the "
			"device id=%u", data->active_dev_idx);
		return 0;
	}

	/* Memory and address bounds check for memory read and memory write */

	/*1- Verify is packet address is within bounds of memory map address */
	if ((packet->address < start_address) || (packet->address > end_address)) {
		LOG_ERR("Address is out of bounds of memory map address");
		return -EINVAL;
	}

	/*2- Verify is packet size is within memory map bounds */
	if ((packet->address + packet->num_bytes > QSPI_MEM_BOUNDARY_MAX) ||
		(packet->address + packet->num_bytes > end_address)) {
		LOG_ERR("Packet size is out of memory map bounds");
		return -EINVAL;
	}
	return 0;

}
#endif

/*
 * 32-bit memcpy.
 *
 * This function copies memory in 32-bit chunks. Takes dest (pointer to
 * destination memory), src (pointer to source memory), and len (length
 * of data to copy in number of 32-bit words).
 */
static void qspi_memcpy_32bit(uint32_t *dest, uint32_t *src, uint32_t len)
{
	while (len > 0) {
		*dest = *src;
		dest++;
		src++;
		len--;
	}
}

/*
 * 8-bit memcpy.
 *
 * This function copies memory in 8-bit chunks. Takes dest (pointer to
 * destination memory), src (pointer to source memory), and len (length
 * of data to copy in number of bytes).
 */

static void qspi_memcpy_8bit(uint8_t *dest, uint8_t *src, uint32_t len)
{
	while (len > 0) {
		*dest = *src;
		dest++;
		src++;
		len--;
	}
}

/*
 * Setup QSPI transfer.
 *
 * This function sets up a QSPI transfer.
 *
 * Takes trans (pointer to the transfer structure), tfr_type (transfer type),
 * and q (pointer to the QSPI registers).
 *
 * Returns true on successful configuration of the QSPI INSTR/INSTRFRAME
 * registers; false on failure.
 */
static bool qspi_setup_transfer(struct mchp_hal_mspi_pio_transfer *trans, uint32_t tfr_type,
				qspi_registers_t *q)
{
	uint32_t mask = 0;

	/* Set instruction address register if address and no data
	 * if address and data both are present,address is defined by QSPI memory map address
	 * thenbelow write is of no effect.
	 */
	q->QSPI_INSTRADDR = QSPI_INSTRADDR_ADDR(trans->device_addr);

	/* Set Instruction code register */
	q->QSPI_INSTRCTRL = (QSPI_INSTRCTRL_INSTR((uint32_t)trans->device_instr));

	/* Set Instruction Frame register */

	if (trans->send_addr) {
		if (trans->addrlen == MCHP_HAL_MSPI_ADDR_3_BYTE) {
			mask |= (uint32_t)QSPI_INSTRFRAME_ADDRLEN_24BITS_Val;
		}
		/* addrlen = 4 */
		else {
			mask |= (uint32_t)QSPI_INSTRFRAME_ADDRLEN_32BITS_Val;
		}
	}

	/* continuous read mode */
	if (trans->continue_crmode) {
		mask |= QSPI_INSTRFRAME_CRMODE_Msk;
	}

	if (trans->direction == MSPI_RX) {
		mask |= QSPI_INSTRFRAME_DUMMYLEN((uint32_t)trans->rx_dummy);
	} else {
		mask |= QSPI_INSTRFRAME_DUMMYLEN((uint32_t)trans->tx_dummy);
	}

	mask |= QSPI_INSTRFRAME_INSTREN_Msk | QSPI_INSTRFRAME_ADDREN_Msk |
		QSPI_INSTRFRAME_DATAEN_Msk;

	mask |= QSPI_INSTRFRAME_TFRTYPE(tfr_type);

	/* Preserve WIDTH and DDREN bits from current INSTRFRAME */
	mask |= (q->QSPI_INSTRFRAME & (QSPI_INSTRFRAME_WIDTH_Msk | QSPI_INSTRFRAME_DDREN_Msk));

	q->QSPI_INSTRFRAME = mask;

	/* To synchronize APB and AHB accesses */
	(uint32_t)q->QSPI_INSTRFRAME;

	return true;
}

static inline bool qspi_wait_instrend(qspi_registers_t *q, const char *ctx)
{
	if (WAIT_FOR(((q->QSPI_INTFLAG & QSPI_INTFLAG_INSTREND_Msk) ==
		       QSPI_INTFLAG_INSTREND_Msk),
		      QSPI_EXTENDED_TIMEOUT_US, k_busy_wait(DELAY_US)) == 0) {
		LOG_ERR("QSPI %s timeout", ctx);
		return false;
	}

	q->QSPI_INTFLAG |= QSPI_INTFLAG_INSTREND_Msk;
	return true;
}

/*
 * Write command to QSPI register.
 *
 * This function writes a command to a QSPI register.
 *
 * Takes packet (pointer to the transfer packet containing cmd, address, data
 * buffer, number of bytes and callback mask), xfer (pointer to the transfer
 * descriptor with cmd/addr lengths, dummy lengths, async flag, direction),
 * and q (pointer to the QSPI register block).
 *
 * Returns true if the command was successfully issued (or queued for async);
 * false on error or timeout.
 */
static bool qspi_command_write(const struct mspi_xfer_packet *packet, const struct mspi_xfer *xfer,
			      qspi_registers_t *q)
{
	uint32_t mask = 0;
	bool async = xfer->async;

	/* Configure address */
	if (xfer->addr_length != 0) {
		q->QSPI_INSTRADDR = QSPI_INSTRADDR_ADDR(packet->address);

		mask |= QSPI_INSTRFRAME_ADDREN_Msk;
		if (xfer->addr_length == MCHP_HAL_MSPI_ADDR_3_BYTE) {
			mask |= (uint32_t)QSPI_INSTRFRAME_ADDRLEN_24BITS_Val;
		}
		/* addrlen = 4 */
		else {
			mask |= (uint32_t)QSPI_INSTRFRAME_ADDRLEN_32BITS_Val;
		}
	}

	/* Configure instruction */
	q->QSPI_INSTRCTRL = (QSPI_INSTRCTRL_INSTR((uint32_t)packet->cmd));

	/* Configure instruction frame */
	mask |= QSPI_INSTRFRAME_INSTREN_Msk;
	mask |= QSPI_INSTRFRAME_TFRTYPE(QSPI_INSTRFRAME_TFRTYPE_READ_Val);

	/* Preserve WIDTH and DDREN bits from current INSTRFRAME */
	mask |= (q->QSPI_INSTRFRAME & (QSPI_INSTRFRAME_WIDTH_Msk | QSPI_INSTRFRAME_DDREN_Msk));

	q->QSPI_INSTRFRAME = mask;

	if (async == false) {
		if (!qspi_wait_instrend(q, "command write")) {
			return false;
		}
	}

	return true;
}

/*
 * Read or write data to a QSPI register (no address phase).
 *
 * This function reads or writes data to a QSPI register.
 * The transfer direction is determined by packet->dir (MSPI_RX or MSPI_TX).
 *
 * Takes packet (pointer to the transfer packet containing cmd, address, data
 * buffer, number of bytes and callback mask), xfer (pointer to the transfer
 * descriptor with cmd/addr lengths, dummy lengths, async flag, direction),
 * and q (pointer to the QSPI registers).
 *
 * Returns true if the command was successfully issued (or queued for async);
 * false on error or timeout.
 */
static bool qspi_register_transfer(const struct mspi_xfer_packet *packet,
				   const struct mspi_xfer *xfer,
				   qspi_registers_t *q)
{
	uint32_t *qspi_buffer = (uint32_t *)QSPI_ADDR;
	uint32_t mask = 0;
	bool async = xfer->async;
	uint32_t tfrtype;

	/* Configure Instruction */
	q->QSPI_INSTRCTRL = QSPI_INSTRCTRL_INSTR((uint32_t)packet->cmd);

	/* Configure Instruction Frame */
	if (packet->dir == MSPI_RX) {
		mask |= QSPI_INSTRFRAME_DUMMYLEN((uint32_t)xfer->rx_dummy);
		tfrtype = QSPI_INSTRFRAME_TFRTYPE_READ_Val;
	} else {
		mask |= QSPI_INSTRFRAME_DUMMYLEN((uint32_t)xfer->tx_dummy);
		tfrtype = QSPI_INSTRFRAME_TFRTYPE_WRITE_Val;
	}

	mask |= QSPI_INSTRFRAME_INSTREN_Msk | QSPI_INSTRFRAME_DATAEN_Msk;
	mask |= QSPI_INSTRFRAME_TFRTYPE(tfrtype);

	/* Preserve WIDTH and DDREN bits from current INSTRFRAME */
	mask |= q->QSPI_INSTRFRAME & (QSPI_INSTRFRAME_WIDTH_Msk | QSPI_INSTRFRAME_DDREN_Msk);

	q->QSPI_INSTRFRAME = mask;

	/* Synchronize APB and AHB accesses */
	(void)(volatile uint32_t)q->QSPI_INSTRFRAME;

	if (packet->dir == MSPI_RX) {
		qspi_memcpy_8bit((uint8_t *)packet->data_buf,
				 (uint8_t *)qspi_buffer, packet->num_bytes);
	} else {
		qspi_memcpy_8bit((uint8_t *)qspi_buffer,
				 (uint8_t *)packet->data_buf, packet->num_bytes);
	}

	__DSB();

	qspi_end_transfer(q);

	/* not interrupt driven */
	if (async == false) {
		const char *ctx;

		if (packet->dir == MSPI_RX) {
			ctx = "register read";
		} else {
			ctx = "register write";
		}

		if (!qspi_wait_instrend(q, ctx)) {
			return false;
		}
	}

	return true;
}

/*
 * Read or write data to memory-mapped QSPI flash.
 *
 * This function reads or writes data to a memory-mapped QSPI flash device.
 * The transfer direction is determined by packet->dir (MSPI_RX or MSPI_TX).
 *
 * Takes packet (pointer to the transfer packet containing cmd, address, data
 * buffer, number of bytes and callback mask), xfer (pointer to the transfer
 * descriptor with cmd/addr lengths, dummy lengths, async flag, direction),
 * and q (pointer to the QSPI registers).
 *
 * Returns true if the command was successfully issued (or queued for async);
 * false on error or timeout.
 */
static bool qspi_memory_transfer(const struct mspi_xfer_packet *packet,
				 const struct mspi_xfer *xfer,
				 qspi_registers_t *q)
{
	uint32_t *qspi_mem = (uint32_t *)(QSPI_ADDR | packet->address);
	uint32_t *current_buffer_ptr = (uint32_t *)packet->data_buf;
	uint32_t length_32bit, length_8bit;

	bool status = false;
	bool async = xfer->async;
	uint32_t tfrtype;

	struct mchp_hal_mspi_pio_transfer trans = {0};

	/* "trans" containing "packet" and transfer descriptor "xfer" info */
	trans.device_instr = packet->cmd;
	trans.send_addr = (xfer->addr_length != 0);
	if (trans.send_addr) {
		trans.addrlen = xfer->addr_length;
		trans.device_addr = packet->address;
	}
	trans.direction = packet->dir;
	trans.rx_dummy = xfer->rx_dummy;
	trans.tx_dummy = xfer->tx_dummy;

	tfrtype = (packet->dir == MSPI_RX) ? QSPI_INSTRFRAME_TFRTYPE_READMEMORY_Val
					   : QSPI_INSTRFRAME_TFRTYPE_WRITEMEMORY_Val;

	if (qspi_setup_transfer(&trans, tfrtype, q)) {
		length_32bit = (packet->num_bytes) / 4UL;
		length_8bit = packet->num_bytes & 0x03U;

		/* Copy in 32-bit chunks for faster transfer */
		if (length_32bit > 0U) {
			if (packet->dir == MSPI_RX) {
				qspi_memcpy_32bit(current_buffer_ptr, qspi_mem, length_32bit);
			} else {
				qspi_memcpy_32bit(qspi_mem, current_buffer_ptr, length_32bit);
			}
		}
		current_buffer_ptr = current_buffer_ptr + length_32bit;
		qspi_mem = qspi_mem + length_32bit;

		/* left over buffer data copy in 8-bit chunks */
		if (length_8bit > 0U) {
			if (packet->dir == MSPI_RX) {
				qspi_memcpy_8bit((uint8_t *)current_buffer_ptr,
						 (uint8_t *)qspi_mem, length_8bit);
			} else {
				qspi_memcpy_8bit((uint8_t *)qspi_mem,
						 (uint8_t *)current_buffer_ptr, length_8bit);
			}
		}

		__DSB();

		qspi_end_transfer(q);

		/* not interrupt driven */
		if (async == false) {
			const char *ctx;

			if (packet->dir == MSPI_RX) {
				ctx = "memory read";
			} else {
				ctx = "memory write";
			}

			if (!qspi_wait_instrend(q, ctx)) {
				return false;
			}
		}

		status = true;
	}
	return status;
}

/*
 * Populate MSPI context for a transfer.
 *
 * Initializes the context with the transfer parameters. The controller
 * lock (ctx->lock) must already be held by the caller (acquired in
 * mspi_mchp_transceive()). It is released by mspi_mchp_transceive() on
 * sync completion and on the SPI/serial transfer failure paths, or by
 * mspi_context_release() from mspi_cleanup()/the timeout handler once an
 * async transfer completes.
 *
 * Returns 0 on success.
 */
static int mspi_context_populate(struct mspi_context *ctx,
				 const struct mspi_dev_id *dev_id,
				 const struct mspi_xfer *xfer,
				 mspi_callback_handler_t callback,
				 struct mspi_callback_context *callback_ctx)
{
	/* Initialize context for this transfer (lock already held by caller) */
	ctx->owner = dev_id;
	ctx->xfer = *xfer;
	ctx->packets_done = 0;
	ctx->callback = callback;
	ctx->callback_ctx = callback_ctx;

	return 0;
}

/*
 * Perform a blocking MSPI transfer.
 *
 * This function handles a blocking transfer over the MSPI interface.
 *
 * Takes controller (pointer to the MSPI device structure), packet (pointer
 * to the transfer packet containing cmd, address, data buffer, number of
 * bytes and callback mask), and xfer (pointer to the transfer descriptor
 * with cmd/addr lengths, dummy lengths, async flag, direction).
 *
 * Returns 0 on success, negative error code on failure.
 */
static int mspi_serial_mode_execute_packet(const struct device *controller,
				  const struct mspi_xfer_packet *packet,
				  const struct mspi_xfer *xfer)
{
	const struct mspi_sam_qspi_cfg *cfg = controller->config;
	struct mspi_sam_qspi_data *data = controller->data;
	qspi_registers_t *q = cfg->reg_cfg.regs;
	bool ok;
	int ret;

	/* Sanity checks */
	if (((xfer->cmd_length == 0) && (xfer->addr_length != 0)) ||
	    ((packet->num_bytes > 0) && (packet->data_buf == NULL))) {
		LOG_ERR("Invalid packet: addr without cmd, or num_bytes without data_buf");
		return -EINVAL;
	}

	if ((packet->dir == MSPI_TX) && (xfer->cmd_length != 0) && (packet->num_bytes == 0) &&
	    (packet->data_buf == NULL)) {
		/* Command write only - TX with instruction, with/without address, no data */
		ok = qspi_command_write(packet, xfer, q);
	} else if ((packet->dir == MSPI_RX) && (xfer->cmd_length != 0) &&
		   (xfer->addr_length == 0) && (packet->num_bytes != 0) &&
		   (packet->data_buf != NULL)) {
		/* Register read - RX with instruction, no address, has data */
		ok = qspi_register_transfer(packet, xfer, q);
	} else if ((packet->dir == MSPI_TX) && (xfer->cmd_length != 0) &&
		   (xfer->addr_length == 0) && (packet->num_bytes != 0) &&
		   (packet->data_buf != NULL)) {
		/* Register write - TX with instruction, no address, has data */
		ok = qspi_register_transfer(packet, xfer, q);
	} else if ((packet->dir == MSPI_TX) && (xfer->cmd_length != 0) &&
		   (xfer->addr_length != 0) && (packet->num_bytes != 0) &&
		   (packet->data_buf != NULL)) {
		/* Memory write - TX with instruction, address, and data */
		ok = qspi_memory_transfer(packet, xfer, q);
	} else if ((packet->dir == MSPI_RX) && (xfer->cmd_length != 0) &&
		   (xfer->addr_length != 0) && (packet->num_bytes != 0) &&
		   (packet->data_buf != NULL)) {
		#if defined(CONFIG_MSPI_MEMMAP)
		ret = mspi_check_memory_bounds(data, packet);
		if (ret != 0) {
			return ret;
		}
		#endif
		/* Memory read - RX with instruction, address, and data */
		ok = qspi_memory_transfer(packet, xfer, q);
	} else {
		LOG_ERR("Unsupported packet configuration");
		return -EINVAL;
	}

	return ok ? 0 : -EIO;
}

/*
 * Perform a PIO-based MSPI transfer.
 *
 * This function handles a PIO-based transfer over the MSPI interface.
 *
 * Takes controller (pointer to the MSPI device structure), xfer (pointer to
 * the MSPI transfer structure), cb (callback function to be called upon
 * transfer completion), and cb_ctx (context to be passed to the callback
 * function).
 *
 * Returns 0 on success, negative error code on failure.
 */
static int mspi_pio_serial_transceive(const struct device *controller, const struct mspi_xfer *xfer,
				      mspi_callback_handler_t cb,
				      struct mspi_callback_context *cb_ctx)
{
	struct mspi_sam_qspi_data *data = controller->data;

	/* data->ctx(.callback, .callback_ctx) is initialized in init function with 0s(zeroes) &
	 * rest of fields are filled in mspi_context_populate and in else async part with
	 * "ctx->callback_ctx->mspi_evt"
	 */
	struct mspi_context *ctx = &data->ctx;
	const struct mspi_xfer_packet *packet;
	uint32_t packet_idx;
	int ret;

	if (xfer->num_packet == 0 || (xfer->packets == NULL) ||
	    (xfer->timeout > CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE)) {
		return -EFAULT;
	}

	/* Populate the context with transfer info. The controller lock is already
	 * held by the caller (mspi_mchp_transceive) and is released there for sync
	 * and failure paths, or by mspi_cleanup()/the timeout handler once an async
	 * transfer completes.
	 */
	mspi_context_populate(ctx, data->dev_id, xfer, cb, cb_ctx);

	if (ctx->xfer.async) {
		/* ASYNC: Start first packet only, return immediately.
		 * Work handler (mspi_async_packet_work_handler) will chain subsequent packets.
		 */
		data->serial_mode_active = true;
		data->async_result = 0;
		data->async_packet_complete = false;
		data->async_timed_out = false;
		ctx->packets_done = 0;

		/* Start timeout timer */
		k_timer_start(&data->async_timer,
			      K_MSEC(MSPI_EFFECTIVE_TIMEOUT(xfer->timeout)),
			      K_NO_WAIT);

		/* Start first packet - work handler will continue from here */
		ret = mspi_start_next_packet(controller);
		if (ret != 0) {
			k_timer_stop(&data->async_timer);
			LOG_ERR("Failed to start async serial transfer: %d", ret);
			return ret;
		}

		/* Return immediately - caller is free, ISR/workqueue handles the rest */
		return 0;
	}

	/* SYNC: Loop through all packets using polling */
	while (ctx->packets_done < ctx->xfer.num_packet) {
		packet_idx = ctx->packets_done;
		packet = &ctx->xfer.packets[packet_idx];

		ret = mspi_serial_mode_execute_packet(controller, packet, xfer);

		ctx->packets_done++;
		if (ret != 0) {
			LOG_ERR("Serial transceive failed!");
			return -EIO;
		}
	}

	return ret;
}

/*
 * Perform a blocking SPI transceive using polling on the Microchip QSPI peripheral.
 *
 * Handles sending command/address/dummy bytes and reading/writing payload bytes by
 * polling QSPI status (RXC) and moving data between the peripheral and the packet buffer.
 *
 * Takes controller (pointer to the Zephyr device instance for the QSPI controller),
 * packet (pointer to the transfer packet containing cmd, address, data buffer,
 * number of bytes and callback mask), and xfer (pointer to the transfer descriptor
 * with cmd/addr lengths, dummy lengths, timeout, async flag).
 *
 * Returns 0 on success, negative errno on failure (e.g. -EIO, -EINVAL, -EFAULT).
 */

static int spi_transceive_poll(const struct device *controller,
			       const struct mspi_xfer_packet *packet, const struct mspi_xfer *xfer)
{
	int ret;
	uint32_t timeout_us = 0;
	bool cmdwriteonly = false;

	uint16_t rx_clocks_remaining;
	uint16_t dummy_bytes;
	uint16_t dummy_bytes_tmp;
	uint8_t *tx_temp;
	uint8_t *rx_temp;

	uint32_t rx_drop;
	uint8_t address_len;
	uint8_t cmd_bytes_remaining;

	uint32_t received_data;
	uint32_t tx_packets_remaining = packet->num_bytes;
	uint32_t blocks;

	uint32_t tx_index;
	uint32_t rx_index;

	const struct mspi_sam_qspi_cfg *cfg = controller->config;
	qspi_registers_t *q = cfg->reg_cfg.regs;

	/* Verify DATALEN is configured for 8-bit transfers */
	if ((q->QSPI_CTRLB & QSPI_CTRLB_DATALEN_Msk) != QSPI_CTRLB_DATALEN_8BITS) {
		LOG_ERR("DATALEN not configured for 8-bit transfers");
		return -EINVAL;
	}

	/* Validate data buffer for non-zero transfers */
	if ((packet->num_bytes > 0) && (packet->data_buf == NULL)) {
		LOG_ERR("NULL data buffer with num_bytes=%u", packet->num_bytes);
		return -EINVAL;
	}

	/* Validate: data transfer requires a command to initiate */
	if ((packet->num_bytes > 0) && (xfer->cmd_length == 0)) {
		LOG_ERR("Data transfer requires cmd_length > 0");
		return -EINVAL;
	}

	/* Validate: at minimum, a command must be provided */
	if (xfer->cmd_length == 0) {
		LOG_ERR("cmd_length must be > 0 for SPI transfer");
		return -EINVAL;
	}

	/* Calculate effective timeout */
	if (xfer->timeout > 0) {
		timeout_us = xfer->timeout * 1000;/* ms to us */
	} else {
		timeout_us = TIMEOUT_VALUE_US;
	}

	rx_clocks_remaining = packet->num_bytes;
	dummy_bytes = 0;
	dummy_bytes_tmp = 0;
	tx_temp = (uint8_t *)packet->data_buf;
	rx_temp = (uint8_t *)packet->data_buf;

	rx_drop = 0;
	address_len = xfer->addr_length;
	cmd_bytes_remaining = 0;

	tx_packets_remaining = packet->num_bytes;
	blocks = 0;

	tx_index = 0;
	rx_index = 0;

	if (packet->dir == MSPI_RX) {
		dummy_bytes = (xfer->rx_dummy + MSPI_BITS_ROUND_MASK) / BITS_PER_BYTE;
	} else {
		dummy_bytes = (xfer->tx_dummy + MSPI_BITS_ROUND_MASK) / BITS_PER_BYTE;
	}
	dummy_bytes_tmp = dummy_bytes;
	rx_drop = xfer->cmd_length + xfer->addr_length + dummy_bytes;

	/* Flush out any unread data in SPI read buffer */
	(void)q->QSPI_RXDATA;

	if (xfer->cmd_length != 0) {
		/* command write only, just break out of loop */
		if ((packet->dir == MSPI_TX) && (xfer->addr_length == 0) &&
		    (packet->num_bytes == 0) && (packet->data_buf == NULL)) {
			cmdwriteonly = true;
		}
		/* initiate the transfer - send first command byte (MSB for 2-byte cmds) */
		if (xfer->cmd_length == 2) {
			q->QSPI_TXDATA = (uint8_t)(packet->cmd >> QSPI_CMD_BYTE_SHIFT);
			cmd_bytes_remaining = 1;
		} else {
			q->QSPI_TXDATA = (uint8_t)packet->cmd;
		}
	}

	while (blocks <
	       (xfer->cmd_length + xfer->addr_length + packet->num_bytes + dummy_bytes)) {
		blocks++;

		ret = qspi_check_and_clear_error(q);
		if (ret != 0) {
			return ret;
		}
		if (WAIT_FOR(((q->QSPI_INTFLAG & QSPI_INTFLAG_RXC_Msk) == QSPI_INTFLAG_RXC_Msk),
			      timeout_us, k_busy_wait(DELAY_US)) == 0) {
			LOG_ERR("Wait for received data to come in RXDATA from shifter timed out");
			return -ETIMEDOUT;
		}

		/* reading RXDATA clears RXC flag */
		received_data = ((q->QSPI_RXDATA & QSPI_RXDATA_DATA_Msk) >> QSPI_RXDATA_DATA_Pos);

		/* drop cmd,address,pre-dummy cycles */
		if (rx_drop != 0) {
			rx_drop--;
			if (cmdwriteonly) {
				qspi_end_transfer(q);
				break;
			}
		} else {
			if (packet->dir == MSPI_RX) {
				rx_temp[rx_index] = received_data;
				rx_index++;
				if (blocks == (xfer->cmd_length + xfer->addr_length +
					       packet->num_bytes + dummy_bytes)) {
					qspi_end_transfer(q);
					break;
				}
			}
			/* for MSPI_TX if addresses and data has been sent, end and exit */
			else if ((address_len == 0) && (tx_packets_remaining == 0)) {
				qspi_end_transfer(q);
				break;
			}
		}

		ret = qspi_check_and_clear_error(q);
		if (ret != 0) {
			return ret;
		}
		if (WAIT_FOR(((q->QSPI_INTFLAG & QSPI_INTFLAG_DRE_Msk) == QSPI_INTFLAG_DRE_Msk),
			      timeout_us, k_busy_wait(DELAY_US)) == 0) {
			LOG_ERR("Wait for data to move from TXDATA to shifter timed out");
			return -ETIMEDOUT;
		}

		/* Check if this is the last block for TX transfers */
		if ((blocks == (xfer->cmd_length + xfer->addr_length + packet->num_bytes +
				dummy_bytes)) &&
		    (packet->dir == MSPI_TX)) {
			qspi_end_transfer(q);
			break;
		}

		if (cmd_bytes_remaining > 0) {
			/* Send remaining command byte(s) - low byte for 2-byte commands */
			q->QSPI_TXDATA = (uint8_t)packet->cmd;
			cmd_bytes_remaining--;
		} else if (address_len > 0) {
			q->QSPI_TXDATA = (uint8_t)(((packet->address &
						     (0x000000FF << (8 * (address_len - 1))))) >>
						   (8 * (address_len - 1)));
			address_len--;
		} else if (dummy_bytes_tmp > 0) {
			q->QSPI_TXDATA = QSPI_DUMMY_CYCLE_BYTE;
			dummy_bytes_tmp--;
		} else if ((rx_clocks_remaining > 0) && (packet->dir == MSPI_RX)) {
			/* Send dummy byte to clock in RX data */
			q->QSPI_TXDATA = QSPI_RX_CLOCK_BYTE;
			rx_clocks_remaining--;
		} else if (tx_packets_remaining > 0) {
			q->QSPI_TXDATA = tx_temp[tx_index];
			tx_index++;
			tx_packets_remaining--;
		} else {
			/* Safety: no more data to send, clock out dummy byte */
			q->QSPI_TXDATA = QSPI_RX_CLOCK_BYTE;
			LOG_WRN("Loop iteration with no data to send");
		}
	}

	/* Wait for transmit complete - ensures last byte fully shifted out */
	if (WAIT_FOR(((q->QSPI_INTFLAG & QSPI_INTFLAG_TXC_Msk) == QSPI_INTFLAG_TXC_Msk),
		      timeout_us, k_busy_wait(DELAY_US)) == 0) {
		LOG_ERR("TXC timeout - transfer may be incomplete");
		return -ETIMEDOUT;
	}

	/* Verify CS is deasserted after transfer */
	if (!qspi_is_cs_deassert(q)) {
		LOG_WRN("CS still asserted after transfer complete");
	}

	return ret;
}

/*
 * Perform an interrupt-driven SPI transceive on the Microchip QSPI peripheral.
 *
 * Initiates the SPI transceive process by sending the first byte out of TXDATA.
 * Then qspi_isr (interrupt handler) takes over handling the sending of
 * command/address/dummy bytes and reading/writing payload bytes and moving
 * data between the peripheral and the packet buffer.
 *
 * Takes controller (pointer to the Zephyr device instance for the QSPI controller),
 * packet (pointer to the transfer packet containing cmd, address, data buffer,
 * number of bytes and callback mask), and xfer (pointer to the transfer descriptor
 * with cmd/addr lengths, dummy lengths, timeout, async flag).
 *
 * Returns 0 on success, negative errno on failure (e.g. -EIO, -EINVAL, -EFAULT).
 */

static int spi_transceive_async(const struct device *controller,
				const struct mspi_xfer_packet *packet, const struct mspi_xfer *xfer)
{
	int ret = 0;
	struct mchp_hal_spi_pio_transfer *qspi_obj;

	const struct mspi_sam_qspi_cfg *cfg = controller->config;
	struct mspi_sam_qspi_data *data = controller->data;
	qspi_registers_t *q = cfg->reg_cfg.regs;

	uint16_t dummy_bytes_tmp = 0;

	/* all fields of qspi_obj cleared/initialized in mspi_mchp_qspi_init function */
	memset(&data->qspi_obj, 0, sizeof(struct mchp_hal_spi_pio_transfer));
	qspi_obj = &data->qspi_obj;

	if (packet->dir == MSPI_RX) {
		qspi_obj->dummy_bytes =
			(xfer->rx_dummy + MSPI_BITS_ROUND_MASK) / BITS_PER_BYTE;
		qspi_obj->rx_buffer_bytes = packet->num_bytes;
	} else {
		qspi_obj->dummy_bytes =
			(xfer->tx_dummy + MSPI_BITS_ROUND_MASK) / BITS_PER_BYTE;
	}
	dummy_bytes_tmp = qspi_obj->dummy_bytes;
	qspi_obj->dummy_bytes_fix = qspi_obj->dummy_bytes;

	qspi_obj->rx_count = 0;
	qspi_obj->tx_count = 0;

	qspi_obj->transfer_is_busy = false;
	qspi_obj->addrlen = xfer->addr_length;

	if (packet->dir == MSPI_TX) {
		qspi_obj->rx_drop = xfer->cmd_length + xfer->addr_length + qspi_obj->dummy_bytes +
				   packet->num_bytes;
	} else {
		qspi_obj->rx_drop = xfer->cmd_length + xfer->addr_length + qspi_obj->dummy_bytes;
	}

	if (qspi_obj->transfer_is_busy == false) {
		/* Flush out any unread data in SPI read buffer */
		(void)q->QSPI_RXDATA;

		if (xfer->cmd_length != 0) {
			/* initiate the transfer with cmd/opcode */
			if (xfer->cmd_length == 2) {
				q->QSPI_TXDATA = (uint8_t)(
					(packet->cmd & 0xFF00) >> QSPI_CMD_BYTE_SHIFT);
				qspi_obj->cmd_bytes_remaining = xfer->cmd_length - 1;
			} else {
				q->QSPI_TXDATA = packet->cmd;
			}
			qspi_obj->blocks++;
		}

		/* Used less frequently */
		else if (xfer->addr_length != 0) {
			/* initiate the transfer with address */
			q->QSPI_TXDATA = (uint8_t)((packet->address &
						   (0x000000FF << (8 * (qspi_obj->addrlen - 1)))) >>
						   (8 * (qspi_obj->addrlen - 1)));
			qspi_obj->addrlen--;
			qspi_obj->blocks++;
		} else if (dummy_bytes_tmp > 0) {
			q->QSPI_TXDATA = QSPI_DUMMY_CYCLE_BYTE;
			dummy_bytes_tmp--;
			qspi_obj->dummy_bytes = dummy_bytes_tmp;
			qspi_obj->blocks++;
		} else if (qspi_obj->rx_buffer_bytes > 0) {
			/* send dummy byte to read data */
			q->QSPI_TXDATA = QSPI_RX_CLOCK_BYTE;
			qspi_obj->rx_buffer_bytes--;
			qspi_obj->blocks++;
		}
		/* initiate transfer directly with data */
		else if (packet->dir == MSPI_TX) {
			q->QSPI_TXDATA = packet->data_buf[qspi_obj->tx_count];
			qspi_obj->tx_count++;
			qspi_obj->blocks++;
		}
		/* Used less frequently */

		if (packet->dir == MSPI_RX) {
			/* enable RXC interrupt to get into qspi_isr */
			q->QSPI_INTENSET = QSPI_INTENSET_RXC_Msk |
					   QSPI_INTENSET_ERROR_Msk;
		} else if ((packet->dir == MSPI_TX) && (packet->num_bytes > 0)) {
			q->QSPI_INTENSET = QSPI_INTENSET_RXC_Msk |
					   QSPI_INTENSET_DRE_Msk |
					   QSPI_INTENSET_ERROR_Msk;
		} else if (packet->dir == MSPI_TX) {
			/* enable DRE interrupt to get into qspi_isr */
			q->QSPI_INTENSET = QSPI_INTENSET_DRE_Msk |
					   QSPI_INTENSET_ERROR_Msk;
		}
	} else {
		LOG_ERR("QSPI transfer is busy or invalid packet");
	}

	return ret;
}

/*
 * Start the next packet in the transfer sequence.
 *
 * Called for first packet from transceive, subsequent packets from work handler.
 * Uses data->serial_mode_active to dispatch to the correct mode.
 * Uses ctx->packets_done as the current packet index.
 *
 * Returns 0 on success, negative errno on error.
 */
static int mspi_start_next_packet(const struct device *dev)
{
	struct mspi_sam_qspi_data *data = dev->data;
	const struct mspi_sam_qspi_cfg *cfg = dev->config;
	struct mspi_context *ctx = &data->ctx;
	qspi_registers_t *q = cfg->reg_cfg.regs;
	const struct mspi_xfer_packet *packet;
	uint32_t packet_idx;
	int ret;

	/* Get current packet */
	packet_idx = ctx->packets_done;
	if (packet_idx >= ctx->xfer.num_packet) {
		LOG_ERR("No more packets to process");
		return -EINVAL;
	}
	packet = &ctx->xfer.packets[packet_idx];

	/* Populate callback context for this packet (status = ~0 means in-progress) */
	if (ctx->callback_ctx != NULL) {
		ctx->callback_ctx->mspi_evt.evt_type = MSPI_BUS_XFER_COMPLETE;
		ctx->callback_ctx->mspi_evt.evt_data.controller = dev;
		ctx->callback_ctx->mspi_evt.evt_data.dev_id = ctx->owner;
		ctx->callback_ctx->mspi_evt.evt_data.packet = packet;
		ctx->callback_ctx->mspi_evt.evt_data.packet_idx = packet_idx;
		ctx->callback_ctx->mspi_evt.evt_data.status = ~0;
	}

	if (data->serial_mode_active) {
		/* Serial memory mode: enable INSTREND interrupt, start transfer */
		q->QSPI_INTENSET |= QSPI_INTENSET_INSTREND(1);
		ret = mspi_serial_mode_execute_packet(dev, packet, &ctx->xfer);
	} else {
		/* SPI mode: start async transfer (enables DRE/RXC/TXC interrupts) */
		ret = spi_transceive_async(dev, packet, &ctx->xfer);
	}

	if (ret != 0) {
		LOG_ERR("Failed to start packet %u: %d", packet_idx, ret);
	}

	return ret;
}

/*
 * PIO-mode SPI transceive for the MSPI controller - process all packets in xfer
 * using SPI-mode PIO.
 *
 * Locks the MSPI context, iterates the packets in xfer and performs per-packet
 * transfers in SPI mode (either blocking polling or non-blocking/interrupt-driven),
 * and releases the context.
 *
 * Takes controller (pointer to the MSPI controller device), xfer (pointer to the
 * transfer descriptor containing packets, mode and timeout), cb (transfer-completion
 * callback, may be NULL if not used), and cb_ctx (context passed to cb, may be NULL).
 *
 * Returns 0 on success; negative errno on failure (e.g. -EINVAL, -EFAULT, -EIO).
 */

static int mspi_pio_spi_transceive(const struct device *controller, const struct mspi_xfer *xfer,
				   mspi_callback_handler_t cb, struct mspi_callback_context *cb_ctx)
{
	struct mspi_sam_qspi_data *data = controller->data;

	int ret;

	struct mspi_context *ctx = &data->ctx;
	const struct mspi_xfer_packet *packet = NULL;
	uint32_t packet_idx;

	if (xfer->num_packet == 0 || (xfer->packets == NULL) ||
	    (xfer->timeout > CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE)) {
		return -EFAULT;
	}

	/* Populate the context with transfer info. The controller lock is already
	 * held by the caller (mspi_mchp_transceive) and is released there for sync
	 * and failure paths, or by mspi_cleanup()/the timeout handler once an async
	 * transfer completes.
	 */
	mspi_context_populate(ctx, data->dev_id, xfer, cb, cb_ctx);

	if (ctx->xfer.async) {
		/* ASYNC: Start first packet only, return immediately.
		 * Work handler (mspi_async_packet_work_handler) will chain subsequent packets.
		 */
		data->serial_mode_active = false;  /* SPI mode */
		data->async_result = 0;
		data->async_packet_complete = false;
		data->async_timed_out = false;
		ctx->packets_done = 0;

		/* Start timeout timer */
		k_timer_start(&data->async_timer,
			      K_MSEC(MSPI_EFFECTIVE_TIMEOUT(xfer->timeout)),
			      K_NO_WAIT);

		/* Start first packet - work handler will continue from here */
		ret = mspi_start_next_packet(controller);
		if (ret != 0) {
			k_timer_stop(&data->async_timer);
			LOG_ERR("Failed to start async SPI transfer: %d", ret);
			return ret;
		}

		/* Return immediately - caller is free, ISR/workqueue handles the rest */
		return 0;
	}

	/* SYNC: Loop through all packets using polling */
	while (ctx->packets_done < ctx->xfer.num_packet) {
		packet_idx = ctx->packets_done;
		packet = &ctx->xfer.packets[packet_idx];

		ret = spi_transceive_poll(controller, packet, xfer);

		ctx->packets_done++;
		if (ret != 0) {
			LOG_ERR("SPI transceive failed: %d", ret);
			return ret;
		}
	}

	return ret;
}

static int mspi_validate_transceive_parameters(
	const struct device *controller,
	const struct mspi_dev_id *dev_id,
	const struct mspi_xfer *xfer,
	const struct mspi_sam_qspi_cfg **cfg_out,
	struct mspi_sam_qspi_data **data_out,
	qspi_registers_t **q_out)
{
	const struct mspi_sam_qspi_cfg *cfg;
	struct mspi_sam_qspi_data *data;
	qspi_registers_t *q;

	if (qspi_validate_controller(controller) != 0) {
		return -EINVAL;
	}

	if ((dev_id == NULL) || (xfer == NULL)) {
		LOG_ERR("Invalid QSPI parameters");
		return -EINVAL;
	}

	cfg = controller->config;
	data = controller->data;
	q = cfg->reg_cfg.regs;

	if (q == NULL) {
		LOG_ERR("QSPI register pointer is NULL");
		return -EINVAL;
	}

	if (dev_id->dev_idx >= cfg->num_children) {
		LOG_ERR("%s:Device index is greater than number of children", __func__);
		return -EINVAL;
	}

	if (xfer->xfer_mode != MSPI_PIO) {
		LOG_ERR("Only MSPI_PIO transfer mode is supported");
		return -ENOTSUP;
	}

	if (xfer->async) {
		mspi_callback_handler_t cb = data->cbs[dev_id->dev_idx][MSPI_BUS_XFER_COMPLETE];
		struct mspi_callback_context *cb_ctx =
			data->cb_ctxs[dev_id->dev_idx][MSPI_BUS_XFER_COMPLETE];

		if ((cb == NULL) || (cb_ctx == NULL)) {
			LOG_ERR("%s: Callback handler or context is NULL for "
				"async mode", __func__);
			return -EINVAL;
		}
	}

	*cfg_out = cfg;
	*data_out = data;
	*q_out = q;

	return 0;
}

static int mspi_switch_to_device(const struct device *controller, const struct mspi_dev_id *dev_id)
{
	const struct mspi_sam_qspi_cfg *cfg = controller->config;
	struct mspi_sam_qspi_data *data = controller->data;
	qspi_registers_t *q = cfg->reg_cfg.regs;
	uint32_t frame;
	int ret;

	/* If current device is not the active device */
	if (dev_id->dev_idx != data->active_dev_idx) {
		/* De-assert OLD device CS */
		ret = gpio_cs_deassert(controller, data->active_dev_idx);
		if (ret != 0) {
			LOG_ERR("Failed to de-assert CS for device %d: %d",
				data->active_dev_idx, ret);
			return ret;
		}

		if (data->hw_cache[dev_id->dev_idx].is_configured) {
			/* Fast path: restore cached registers */
			q->QSPI_BAUD = data->hw_cache[dev_id->dev_idx].baud_reg;
			q->QSPI_CTRLB = data->hw_cache[dev_id->dev_idx].ctrlb_reg;

			/* Restore INSTRFRAME bits (preserve other fields) */
			frame = q->QSPI_INSTRFRAME;

			/* clear the existing DDREN,ADDRLEN and WIDTH bits */
			frame &= ~INSTRFRAME_CACHED_MASK;

			/* put back the cached DDREN,ADDRLEN and WIDTH bits */
			frame |= data->hw_cache[dev_id->dev_idx].instrframe_bits;
			q->QSPI_INSTRFRAME = frame;

			LOG_DBG("Restored cached config for device %d", dev_id->dev_idx);
		} else {
			/* Slow path: configure from scratch */
			ret = mspi_mchp_dev_config(
				controller, dev_id,
				(MSPI_DEVICE_CONFIG_ALL &
				 ~(MSPI_DEVICE_CONFIG_DQS)),
				&cfg->child_cfg[dev_id->dev_idx].cfg);
			if (ret != 0) {
				LOG_ERR("Failed to configure device %d: %d",
					dev_id->dev_idx, ret);

				/* Re-assert old device CS before returning error */
				gpio_cs_assert(controller, data->active_dev_idx);
				return ret;
			}

			/* Cache the registers for next time */
			data->hw_cache[dev_id->dev_idx].baud_reg = q->QSPI_BAUD;
			data->hw_cache[dev_id->dev_idx].ctrlb_reg = q->QSPI_CTRLB;
			data->hw_cache[dev_id->dev_idx].instrframe_bits =
				q->QSPI_INSTRFRAME & INSTRFRAME_CACHED_MASK;
			data->hw_cache[dev_id->dev_idx].is_configured = true;

			LOG_DBG("Configured and cached device %d", dev_id->dev_idx);
		}

		/* Update active device index */
		data->active_dev_idx = dev_id->dev_idx;

		/* Assert NEW device CS */
		ret = gpio_cs_assert(controller, data->active_dev_idx);
		if (ret != 0) {
			LOG_ERR("Failed to assert CS for device %d: %d",
				data->active_dev_idx, ret);
			return ret;
		}
	} else {
		/* If current device is the active device -just assert CS */
		ret = gpio_cs_assert(controller, data->active_dev_idx);
		if (ret != 0) {
			LOG_ERR("Failed to assert CS for device %d", data->active_dev_idx);
			return ret;
		}
	}

	return 0;
}

/*
 * Transceive data over MSPI.
 *
 * This function initiates a data transfer over the MSPI interface.
 *
 * Takes controller (pointer to the MSPI device structure), dev_id (pointer to
 * the MSPI device ID structure), and xfer (pointer to the MSPI transfer
 * structure).
 *
 * Returns 0 on success, negative error code on failure.
 */
static int mspi_mchp_transceive(const struct device *controller, const struct mspi_dev_id *dev_id,
				const struct mspi_xfer *xfer)
{
	const struct mspi_sam_qspi_cfg *cfg;
	struct mspi_sam_qspi_data *data;

	mspi_callback_handler_t cb;
	struct mspi_callback_context *cb_ctx;
	qspi_registers_t *q;

	uint8_t width;
	bool already_serial_mode;
	bool needs_serial_path;
	bool was_spi_mode;

	int ret;

	ret = mspi_validate_transceive_parameters(controller, dev_id, xfer, &cfg, &data, &q);
	if (ret < 0) {
		LOG_ERR("mspi_validate_transceive_parameters failed");
		return ret;
	}

	cb = data->cbs[dev_id->dev_idx][MSPI_BUS_XFER_COMPLETE];
	cb_ctx = data->cb_ctxs[dev_id->dev_idx][MSPI_BUS_XFER_COMPLETE];

	/* Acquire the controller lock before switching devices. This protects
	 * active_dev_idx, the CS lines and the shared QSPI registers for the whole
	 * switch + transfer sequence. On a successful async start the lock stays
	 * held and is released by mspi_cleanup()/the timeout handler; every other
	 * path releases it explicitly below.
	 */
	k_sem_take(&data->ctx.lock, K_FOREVER);

	/* Switch to current device */
	ret = mspi_switch_to_device(controller, dev_id);
	if (ret < 0) {
		LOG_ERR("Failed to switch to current device");
		k_sem_give(&data->ctx.lock);
		return ret;
	}

	/* wait for 150us for device to be ready to read/write etc.(can also be ran from app code)
	 */
	k_busy_wait(QSPI_DEVICE_READY_WAIT_US);

	/* Clear all interrupt flags */
	qspi_clear_all_interrupts(q);

	if (((q->QSPI_STATUS & QSPI_STATUS_CSSTATUS_Msk) == 0) ||
	    ((q->QSPI_STATUS & QSPI_STATUS_ENABLE_Msk) == 0)) {
		LOG_ERR("QSPI module not enabled or CS line not de-asserted");
		gpio_cs_deassert(controller, data->active_dev_idx);
		k_sem_give(&data->ctx.lock);
		return -EIO;
	}

	/* Based on user input start config the registers below */

	/* CSMODE is 0 */
	/* SMEMREG is 0 */

	/* Determine which path to use:
	 * 1. If device is already in Serial Memory Mode (CTRLB.MODE=1), use serial path
	 * 2. If WIDTH != 0 (multi-width transfer), use serial path with temp mode switch
	 * 3. Otherwise use SPI mode path
	 */
	width = (uint8_t)((q->QSPI_INSTRFRAME & QSPI_INSTRFRAME_WIDTH_Msk) >>
			 QSPI_INSTRFRAME_WIDTH_Pos);
	already_serial_mode = (q->QSPI_CTRLB & QSPI_CTRLB_MODE_Msk) != 0;
	needs_serial_path = (already_serial_mode) || (width != 0);

	if (xfer->xfer_mode == MSPI_PIO) {
		/* decide whether to choose SPI mode or serial memory mode */
		if (needs_serial_path == false) {
			/* SPI mode implementation - single-width only */
			ret = mspi_pio_spi_transceive(controller, xfer, cb, cb_ctx);
			if (ret != 0) {
				LOG_ERR("mspi_pio_spi_transceive failed");

				/* De-assert GPIO CS if transfer fails */
				gpio_cs_deassert(controller, data->active_dev_idx);
				k_sem_give(&data->ctx.lock);
				return ret;
			}
		} else {
			/* Serial Memory Mode - proper per-phase width control
			 * Multi-width transfers require Serial Memory Mode (CTRLB.MODE=1)
			 * for AHB memory-mapped access. Switch temporarily if currently
			 * in SPI mode, then restore after transfer.
			 */
			was_spi_mode = (already_serial_mode == false);

			if (was_spi_mode) {
				qspi_set_serial_memory_mode(q);
			}

			ret = mspi_pio_serial_transceive(controller, xfer, cb, cb_ctx);

			/* Restore SPI mode if we switched */
			if (was_spi_mode) {
				if ((ret != 0) || (xfer->async == false)) {
					q->QSPI_CTRLB = (q->QSPI_CTRLB &
						~((QSPI_CTRLB_CSMODE_Msk) |
						  (QSPI_CTRLB_MODE_Msk))) |
						(QSPI_CTRLB_MODE(0) |
						 QSPI_CTRLB_CSMODE(1));
				} else {
					/* If async, restore SPI mode during mspi_cleanup/timeout */
					data->restore_spi_mode_on_done = true;
				}
			}

			if (ret != 0) {
				LOG_ERR("mspi_pio_serial_transceive failed");

				/* De-assert GPIO CS if transfer fails */
				gpio_cs_deassert(controller, data->active_dev_idx);
				k_sem_give(&data->ctx.lock);
				return ret;
			}
		}
	} else {
		/* Invalid xfer_mode */
		LOG_ERR("Invalid xfer_mode");
		gpio_cs_deassert(controller, data->active_dev_idx);
		k_sem_give(&data->ctx.lock);
		return -ENODEV;
	}

	if (xfer->async == false) {
		/* SYNC: de-assert CS and release the controller lock */
		gpio_cs_deassert(controller, data->active_dev_idx);
		k_sem_give(&data->ctx.lock);
	}

	/* ASYNC: lock stays held; released by mspi_cleanup()/timeout handler */
	return ret;
}

static bool mspi_is_transfer_in_progress(struct k_sem *lock)
{
	return (k_sem_count_get(lock) == 0);
}

/*
 * Get the status of MSPI channel.
 *
 * This function returns the current operational status of the specified MSPI
 * channel, indicating whether a transfer is in progress. On SAM D5x/E5x, this
 * checks both the driver's transfer state and hardware chip select status.
 *
 * Takes controller (pointer to the MSPI device structure) and ch (channel number,
 * unused as SAM D5x/E5x has single QSPI controller).
 *
 * Returns 0 if channel is idle, 1 if busy, negative error code on failure.
 */
static int mspi_mchp_get_channel_status(const struct device *controller, uint8_t ch)
{
	ARG_UNUSED(ch);

	const struct mspi_sam_qspi_cfg *config;
	struct mspi_sam_qspi_data *data;
	qspi_registers_t *q;

	if (qspi_validate_controller(controller) != 0) {
		return -EINVAL;
	}

	config = controller->config;
	data = controller->data;
	q = config->reg_cfg.regs;

	if (q == NULL) {
		LOG_ERR("%s: QSPI register pointer is NULL", __func__);
		return -EINVAL;
	}

	/* Check if a SPI mode or serial mode transfer is in progress */
	if (mspi_is_transfer_in_progress(&data->ctx.lock)) {
		LOG_ERR("%s: A transfer is in progress", __func__);
		return -EBUSY;
	}

	/* Check CSSTATUS in STATUS register */
	if ((q->QSPI_STATUS & QSPI_STATUS_CSSTATUS_Msk) == 0) {
		LOG_ERR("%s: CS is asserted", __func__);
		return -EBUSY;
	}

	/* Channel is ready */
	return 0;
}

#if defined(CONFIG_MSPI_SCRAMBLE)
/*
 * Configure QSPI scrambling.
 *
 * Enables or disables hardware scrambling for memory transfers. The scramble
 * key comes from devicetree (scramble-key property). Scrambling applies only
 * to READMEMORY/WRITEMEMORY transfers (serial memory mode), not regular SPI.
 *
 * Returns 0 on success, negative error code on failure.
 */
static int mspi_mchp_scramble_config(const struct device *controller,
				     const struct mspi_dev_id *dev_id,
				     const struct mspi_scramble_cfg *cfg)
{
	const struct mspi_sam_qspi_cfg *config;
	struct mspi_sam_qspi_data *data;
	qspi_registers_t *q;
	uint32_t ctrl;

	if (qspi_validate_controller(controller) != 0) {
		return -EINVAL;
	}

	if (cfg == NULL) {
		LOG_ERR("%s: Invalid scramble config", __func__);
		return -EINVAL;
	}

	config = controller->config;
	data = controller->data;
	q = config->reg_cfg.regs;

	/* Validate dev_id matches current owner */
	if (dev_id->dev_idx != data->active_dev_idx) {
		LOG_ERR("%s: dev_id mismatch", __func__);
		return -ESTALE;
	}

	/*
	 * Check if any transfer is in progress (sync or async, SPI or serial mode).
	 * The ctx.lock semaphore is held during all transfer types.
	 */
	if (mspi_is_transfer_in_progress(&data->ctx.lock)) {
		LOG_ERR("%s: Transfer in progress", __func__);
		return -EBUSY;
	}

	/* Also check hardware CS status */
	if ((q->QSPI_STATUS & QSPI_STATUS_CSSTATUS_Msk) == 0) {
		LOG_ERR("%s: CS is asserted", __func__);
		return -EBUSY;
	}

	/* Require scramble-key in DT if enabling */
	if ((cfg->enable) && (config->scramble_key == 0)) {
		LOG_ERR("%s: scramble-key not set in devicetree", __func__);
		return -EINVAL;
	}

	/*
	 * No need to disable QSPI - datasheet section 37.6.9 says:
	 * "The scrambling and unscrambling are performed on-the-fly without
	 * impacting the throughput."
	 *
	 * The busy checks above ensure no transfer is in progress, which is
	 * sufficient to avoid scrambling only part of a transfer.
	 */

	if (cfg->enable) {
		/* Set scramble key from devicetree */
		q->QSPI_SCRAMBKEY = config->scramble_key;

		/* Enable scrambling, optionally disable random value */
		ctrl = QSPI_SCRAMBCTRL_ENABLE_Msk;

		if (config->scramble_random_disable) {
			ctrl |= QSPI_SCRAMBCTRL_RANDOMDIS_Msk;
		}
		q->QSPI_SCRAMBCTRL = ctrl;

		if (config->scramble_random_disable) {
			LOG_DBG("Scramble random value disabled");
		} else {
			LOG_DBG("Scramble random value enabled");
		}

	} else {
		/* Disable scrambling */
		q->QSPI_SCRAMBCTRL = 0;
		LOG_DBG("Scramble disabled");
	}

	/* Cache configuration */
	data->scramble_cfg = *cfg;

	return 0;
}
#endif /* CONFIG_MSPI_SCRAMBLE */

#if defined(CONFIG_MSPI_MEMMAP)
static int mspi_mchp_memmap_config(const struct device *controller,
				     const struct mspi_dev_id *dev_id,
					 const struct mspi_memmap_cfg *cfg)
{
	const struct mspi_sam_qspi_cfg *config;
	struct mspi_sam_qspi_data *data;
	qspi_registers_t *q;

	bool serial_mode = false;
	const char *str;
	int ret;

	if (qspi_validate_controller(controller) != 0) {
		return -EINVAL;
	}

	if (cfg == NULL) {
		LOG_ERR("Memory map user config data NULL");
		return -EINVAL;
	}

	config = controller->config;
	data = controller->data;
	q = config->reg_cfg.regs;

	LOG_DBG("active_dev_idx=%d, num_children=%d", data->active_dev_idx, config->num_children);

	ret = mspi_find_serial_mode_of_child(config->child_desc, data->active_dev_idx,
					     config->num_children, &serial_mode);
	if ((ret != 0) || (serial_mode == false)) {
		LOG_ERR("%s: Serial mode not supported",  __func__);
		return -ENOTSUP;
	}

	/* Device ID mismatch */
	if (dev_id->dev_idx != data->active_dev_idx) {
		LOG_ERR("%s: dev_id mismatch", __func__);
		return -ESTALE;
	}

	/* Check if a transfer is in progress */
	if (((q->QSPI_STATUS & QSPI_STATUS_CSSTATUS_Msk) == 0) ||
		(mspi_is_transfer_in_progress(&data->ctx.lock))) {
		LOG_ERR("%s: A transfer is in progress", __func__);
		return -EBUSY;
	}

	if (cfg->enable == false) {
		/* clear xip_enabled flag */
		data->xip_enabled &= ~BIT(data->active_dev_idx);
		memset(&data->memmap_cfg[data->active_dev_idx], 0, sizeof(struct mspi_memmap_cfg));
		return 0;
	}

	qspi_set_serial_memory_mode(q);

	/* Enabling software XIP flag for the active device */
	data->xip_enabled |= BIT(data->active_dev_idx);
	data->memmap_cfg[data->active_dev_idx] = *cfg;
	if (cfg->permission == MSPI_MEMMAP_READ_WRITE) {
		str = "READ_WRITE";
	} else {
		str = "READ_ONLY";
	}
	LOG_DBG("XIP enabled for device %d, address offset = 0x%08x, "
		"size = 0x%08x, permission = %s",
		data->active_dev_idx, cfg->address_offset, cfg->size, str);

	return 0;
}
#endif

/* Driver API table */
static DEVICE_API(mspi, mspi_sam_qspi_api) = {
	.config = mspi_mchp_config,
	.dev_config = mspi_mchp_dev_config,
	.transceive = mspi_mchp_transceive,
	.register_callback = mspi_mchp_register_callback,
	.timing_config = mspi_mchp_timing_config,
	.get_channel_status = mspi_mchp_get_channel_status,
#if defined(CONFIG_MSPI_SCRAMBLE)
	.scramble_config = mspi_mchp_scramble_config,
#endif
#if defined(CONFIG_MSPI_MEMMAP)
	.memmap_config = mspi_mchp_memmap_config,
#endif
};

/*
 * Initialize QSPI module.
 *
 * Takes controller (pointer to the MSPI device structure).
 *
 * Returns 0 on success, negative error code on failure.
 */
static int mspi_mchp_qspi_init(const struct device *controller)
{
	struct mspi_sam_qspi_data *data;
	const struct mspi_sam_qspi_cfg *cfg;
	const struct mspi_child_cfg *child_cfg_ptr;

	int ret;

	if (qspi_validate_controller(controller) != 0) {
		LOG_ERR("Invalid QSPI controller parameters");
		return -EINVAL;
	}

	data = controller->data;
	cfg = controller->config;
	child_cfg_ptr = cfg->child_cfg;

	if ((child_cfg_ptr == NULL) || (cfg->num_children == 0)) {
		LOG_ERR("No children or invalid child configuration");
		return -EINVAL;
	}

	#if defined(CONFIG_MSPI_MEMMAP)
	data->xip_enabled = 0;
	data->memmap_cfg = k_calloc(cfg->num_children, sizeof(struct mspi_memmap_cfg));
	if (data->memmap_cfg == NULL) {
		LOG_ERR("Unable to allocate memmap_cfg array");
		return -ENOMEM;
	}
	#endif

	/* One-time runtime object initialization */
	k_mutex_init(&data->lock_init);
	k_mutex_init(&data->lock_dev);
	k_sem_init(&data->ctx.lock, 1, 1);

	memset(&data->qspi_obj, 0, sizeof(struct mchp_hal_spi_pio_transfer));

	/* Note: cbs and cb_ctxs arrays are static and zero-initialized */

	data->ctx.owner = NULL;

	/* init the data->ctx.xfer values */
	data->ctx.xfer.async = false;
	data->ctx.xfer.xfer_mode = MSPI_PIO;
	data->ctx.xfer.tx_dummy = 0;
	data->ctx.xfer.rx_dummy = 0;
	data->ctx.xfer.cmd_length = 0;
	data->ctx.xfer.addr_length = 0;
	data->ctx.xfer.priority = 0;
	data->ctx.xfer.packets = NULL;
	data->ctx.xfer.num_packet = 0;

	data->ctx.packets_done = 0;

	data->ctx.callback = NULL;
	data->ctx.callback_ctx = NULL;

	data->ctx.asynchronous = false;

	data->is_last_byte_xfer_in_progress = false;

	/* Initialize async infrastructure */
	data->dev = controller;
	k_sem_init(&data->async_sync_sem, 0, 1);
	k_timer_init(&data->async_timer, mspi_async_timeout_timer_handler, NULL);
	k_work_init(&data->async_timeout_work, mspi_async_timeout_work_handler);
	k_work_init(&data->async_packet_work, mspi_async_packet_work_handler);
	data->serial_mode_active = false;
	data->async_result = 0;
	data->async_packet_complete = false;
	data->async_timed_out = false;

	/* Initialize hw_cache array - mark all devices as unconfigured */
	for (uint8_t i = 0; i < cfg->num_children; i++) {
		data->hw_cache[i].is_configured = false;
	}

	/* Hardware-only initialization */
	k_mutex_lock(&data->lock_init, K_FOREVER);
	ret = qspi_hw_init(controller);

	if (ret != 0) {
		k_mutex_unlock(&data->lock_init);
		LOG_ERR("QSPI hardware init failed");
		return ret;
	}

	/* iterate over all children and configure each child device */
	ret = mspi_apply_child_configs(controller);
	if (ret != 0) {
		k_mutex_unlock(&data->lock_init);
		LOG_ERR("Failed to initialize child hardware");
		return ret;
	}
	k_mutex_unlock(&data->lock_init);

	return ret;
}

/*
 * One entry per enabled child under this controller instance
 */
#define CHILD_DESC(child)                                                                          \
	{                                                                                          \
		.cs = (uint8_t)DT_REG_ADDR(child),                                                 \
		.is_serial_mode = DT_PROP(child, serial_memory_mode),                              \
	}

/*
 * Build child config (id + cfg)
 */
#define MSPI_MCHP_CHILD_CFG(node)                                                                  \
	{                                                                                          \
		.id =                                                                              \
			{                                                                          \
				.dev_idx = (uint8_t)DT_REG_ADDR(node),                     \
			},                                                                         \
		.cfg = {.freq = DT_PROP_OR(node, mspi_max_frequency, 0),                           \
			.io_mode = DT_ENUM_IDX_OR(node, mspi_io_mode, 0),                          \
			.data_rate = DT_ENUM_IDX_OR(node, mspi_data_rate, 0),                      \
			.cpp = DT_ENUM_IDX_OR(node, mspi_cpp_mode, 0),                             \
			.ce_polarity = DT_ENUM_IDX_OR(node, mspi_ce_polarity, 0),                  \
			.dqs_enable = false,                                                       \
			.cmd_length = DT_ENUM_IDX_OR(node, command_length, 1),                     \
			.addr_length = DT_ENUM_IDX_OR(node, address_length, 3),                    \
			.read_cmd = DT_PROP_OR(node, read_command, 0x0B),                          \
			.write_cmd = DT_PROP_OR(node, write_command, 0x02),                        \
			.rx_dummy = DT_PROP_OR(node, rx_dummy, 8),                                 \
			.tx_dummy = DT_PROP_OR(node, tx_dummy, 0),                                 \
			.mem_boundary = DT_PROP_BY_IDX(node, ce_break_config, 0),                  \
			.time_to_break = DT_PROP_BY_IDX(node, ce_break_config, 1),                 \
		},                                                                                 \
		.dlybs = DT_PROP_OR(node, dlybs, 0),                                               \
		.dlycs = DT_PROP_OR(node, dlycs, 0),                                               \
		.dlybct = DT_PROP_OR(node, dlybct, 0),                                             \
	}

/*
 * Controller instantiation from DT (is meant for mspi_config() if user doesn't
 * call mspi_config())
 */
#define MSPI_MCHP_CONFIG(inst)                                                                     \
	{                                                                                          \
		.channel_num = 0,                                                                  \
		.op_mode = DT_ENUM_IDX_OR(DT_DRV_INST(inst), op_mode, MSPI_OP_MODE_CONTROLLER),    \
		.duplex = DT_ENUM_IDX_OR(DT_DRV_INST(inst), duplex, MSPI_HALF_DUPLEX),             \
		.max_freq = DT_INST_PROP_OR(                                                       \
			inst, clock_frequency,                                                     \
			120000000), /* 120Mhz for CLK_QSPI_AHB not CLK_QSPI_AHB2X */               \
		.dqs_support = DT_INST_PROP_OR(inst, dqs_support, false),                          \
		.num_periph = DT_INST_CHILD_NUM(inst),                                             \
		.sw_multi_periph = DT_INST_PROP(inst, software_multiperipheral),                   \
	}

/*
 * Generate child configuration arrays per controller instance
 */
#define MSPI_MCHP_GEN_CHILD_TABLES(inst)                                                           \
	static const struct mspi_child_desc mspi_child_desc_##inst[] = {                           \
		DT_FOREACH_CHILD_STATUS_OKAY(DT_DRV_INST(inst), CHILD_DESC)};                      \
	static const struct mspi_child_cfg mspi_child_cfg_##inst[] = {                             \
		DT_FOREACH_CHILD_STATUS_OKAY(DT_DRV_INST(inst), MSPI_MCHP_CHILD_CFG)};

/*
 * Macro to configure and enable MSPI IRQ
 */
#define MCHP_MSPI_IRQ_CONNECT(inst, m)                                                             \
	do {                                                                                       \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, m, irq),                                      \
			    DT_INST_IRQ_BY_IDX(inst, m, priority), qspi_isr,                  \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQ_BY_IDX(inst, m, irq));                                      \
	} while (false)

/*
 * Configure MSPI interrupts for instance inst.
 *
 * This function sets up the interrupt handlers for the MSPI instance
 * specified by inst.
 */
#define MSPI_MCHP_IRQ_HANDLER(inst)                                                                \
	static void mspi_mchp_irq_config_##inst(const struct device *dev)                          \
	{                                                                                          \
		MCHP_MSPI_IRQ_CONNECT(inst, 0);                                                    \
	}

/*
 * Macro to declare an IRQ handler function
 */
#define MSPI_MCHP_IRQ_HANDLER_DECL(inst)                                                           \
	static void mspi_mchp_irq_config_##inst(const struct device *dev)
#define MSPI_MCHP_IRQ_HANDLER_FUNC(inst) .irq_config_func = mspi_mchp_irq_config_##inst,

/*
 * Macro that declares and initializes a Microchip MSPI device instance:
 * allocates/initializes driver data, configures hardware resources and
 * registers the device with the OS.
 */
#define MSPI_MCHP_DEVICE_INIT(inst)                                                                \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	MSPI_MCHP_IRQ_HANDLER_DECL(inst);                                                          \
	static struct mspi_mchp_cached_hw_cfg hw_cache_##inst[DT_INST_CHILD_NUM(inst)];           \
	static mspi_callback_handler_t cbs_##inst[DT_INST_CHILD_NUM(inst)][MSPI_BUS_EVENT_MAX];    \
	static struct mspi_callback_context *cb_ctxs_##inst                                        \
		[DT_INST_CHILD_NUM(inst)][MSPI_BUS_EVENT_MAX];                                     \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, ce_gpios),                                          \
		(static const struct gpio_dt_spec ce_gpios_##inst[] = {                            \
			DT_INST_FOREACH_PROP_ELEM_SEP(inst, ce_gpios, GPIO_DT_SPEC_GET_BY_IDX, (,))\
		};))                                                                               \
	static struct mspi_sam_qspi_data mspi_sam_qspi_data_##inst = {                             \
		.dev_id = NULL,                                                                    \
		.lock_init = Z_MUTEX_INITIALIZER(mspi_sam_qspi_data_##inst.lock_init),             \
		.lock_dev = Z_MUTEX_INITIALIZER(mspi_sam_qspi_data_##inst.lock_dev),               \
		.qspi_obj = {0},                                                                   \
		.active_dev_idx = 0,                                                               \
		.hw_cache = hw_cache_##inst,                                                       \
		.cbs = cbs_##inst,                                                                 \
		.cb_ctxs = cb_ctxs_##inst,                                                         \
	};                                                                                         \
	static const struct mspi_sam_qspi_cfg mspi_sam_qspi_cfg_##inst = {                         \
		.reg_cfg = {.regs = (qspi_registers_t *)DT_INST_REG_ADDR(inst), .pads = 0},        \
		.reg_size = DT_INST_REG_SIZE(inst),                                                \
		.mspicfg = MSPI_MCHP_CONFIG(inst),                                                 \
		.mspicfg.re_init = false,                                                          \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		MSPI_MCHP_IRQ_HANDLER_FUNC(inst).mspi_clock =                                      \
			{                                                                          \
				.mclk_apb = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(   \
					inst, apb, subsystem),                                     \
				.mclk_ahb = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(   \
					inst, ahb, subsystem),                                     \
				.mclk_ahb2x = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME( \
					inst, ahb2x, subsystem),                                   \
			},                                                                         \
		.child_desc = mspi_child_desc_##inst,                                              \
		.num_children = ARRAY_SIZE(mspi_child_desc_##inst),                                \
		.child_cfg = mspi_child_cfg_##inst,                                                \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, ce_gpios),                                  \
			(.ce_gpios = ce_gpios_##inst,                                              \
			 .ce_gpios_len = ARRAY_SIZE(ce_gpios_##inst),))                            \
		.sw_multi_periph = DT_INST_PROP(inst, software_multiperipheral),                   \
		IF_ENABLED(CONFIG_MSPI_SCRAMBLE,                                                   \
			(.scramble_key = DT_INST_PROP_OR(inst, scramble_key, 0),                   \
			 .scramble_random_disable = DT_INST_PROP(inst, scramble_random_disable),)) \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, mspi_mchp_qspi_init, NULL, &mspi_sam_qspi_data_##inst,         \
			      &mspi_sam_qspi_cfg_##inst, POST_KERNEL,                              \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &mspi_sam_qspi_api);             \
	MSPI_MCHP_IRQ_HANDLER(inst)

DT_INST_FOREACH_STATUS_OKAY(MSPI_MCHP_GEN_CHILD_TABLES)
DT_INST_FOREACH_STATUS_OKAY(MSPI_MCHP_DEVICE_INIT)
