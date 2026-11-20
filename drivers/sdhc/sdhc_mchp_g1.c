/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_sdhc_g1

#include <zephyr/device.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <string.h>

LOG_MODULE_REGISTER(sdhc_mchp_g1, CONFIG_SDHC_LOG_LEVEL);

#define SDHC_REG(dev) ((sdhc_registers_t *)((const struct sdhc_mchp_cfg *)(dev)->config)->regs)

#define SDHC_RESET_TIMEOUT_US         1000
#define SDHC_CLOCK_STABLE_TIMEOUT_US  1000
#define SDHC_CMD_IDLE_TIMEOUT_US      1000
#define SDHC_CMD_DEFAULT_TIMEOUT_MS   200
#define SDHC_DATA_DEFAULT_TIMEOUT_MS  10000
#define SDHC_DATA_TIMEOUT_COUNTER_VAL 0xEU
#define SDHC_GCLK_READY_TIMEOUT_MS    1000 /* 1 sec */

/* Convert SDHC max_blk_len bitfield value to actual byte length.
 * returns Maximum block length in bytes, or 0 if reserved.
 */
#define SDHC_MAX_BLK_LEN_TO_BYTES(code)                                                            \
	((code) == 0 ? 512 : (code) == 1 ? 1024 : (code) == 2 ? 2048 : 0)

#define SDHC_XFER_STATUS_CMD_COMPLETED  (0x01U)
#define SDHC_XFER_STATUS_DATA_COMPLETED (0x02U)
#define SDHC_XFER_STATUS_CARD_INSERTED  (0x04U)
#define SDHC_XFER_STATUS_CARD_REMOVED   (0x08U)

#define SDHC_ADMA_DESC_TABLE_ATTR_XFER_DATA (0x02U << 4U)
#define SDHC_ADMA_DESC_TABLE_ATTR_VALID     (1U << 0U)
#define SDHC_ADMA_DESC_TABLE_ATTR_END       (1UL << 1U)
#define SDHC_ADMA_DESC_TABLE_ATTR_INTR      (1U << 2U)

/* Max size per descriptor: keep it perfectly divisible by both 4-byte sector word boundaries
 * and your active CONFIG_SDHC_BUFFER_ALIGNMENT(ie, cache line size) alignment requirements.
 */
#define SDHC_ADMA2_DESC_MAX_BYTES ((64 * 1024) - CONFIG_SDHC_BUFFER_ALIGNMENT)
#define SDHC_DMASEL_ADMA2         2U

/* Each standard row in the sdhc_adma_descr structure takes exactly 8 bytes */
#define SDHC_ADMA2_DESC_LINE_SIZE 8U
#define SDHC_MCHP_DESC_TABLE_SIZE(n)                                                               \
	(SDHC_ADMA2_DESC_LINE_SIZE * CONFIG_SDHC_MCHP_G1_ADMA_DESC_COUNT)

#if defined(CONFIG_SDHC_BUFFER_ALIGNMENT)
/* Bind directly to Zephyr's block layer device alignment configuration */
#define SDHC_MCHP_AGNOSTIC_ALIGN_SIZE CONFIG_SDHC_BUFFER_ALIGNMENT
#elif defined(CONFIG_DCACHE_LINE_SIZE) && (CONFIG_DCACHE_LINE_SIZE > 0)
/* Fallback directly to the compile-time physical hardware line size (32) */
#define SDHC_MCHP_AGNOSTIC_ALIGN_SIZE CONFIG_DCACHE_LINE_SIZE
#else
/* Safe fallback baseline limit for hardware ADMA2 parsing word boundaries */
#define SDHC_MCHP_AGNOSTIC_ALIGN_SIZE 4U
#endif

#define SDHC_MCHP_DESC_TABLE_SIZE_CACHE_ALIGN(n)                                                   \
	(SDHC_MCHP_DESC_TABLE_SIZE(n) +                                                            \
	 ((SDHC_MCHP_DESC_TABLE_SIZE(n) % SDHC_MCHP_AGNOSTIC_ALIGN_SIZE)                           \
		  ? (SDHC_MCHP_AGNOSTIC_ALIGN_SIZE -                                               \
		     (SDHC_MCHP_DESC_TABLE_SIZE(n) % SDHC_MCHP_AGNOSTIC_ALIGN_SIZE))               \
		  : 0U))

enum sdhc_speed_mode {
	SDHC_SPEED_MODE_NORMAL = 0,
	SDHC_SPEED_MODE_HIGH
};

enum sdhc_read_response_reg {
	SDHC_READ_RESP_REG_0 = 0,
	SDHC_READ_RESP_REG_1,
	SDHC_READ_RESP_REG_2,
	SDHC_READ_RESP_REG_3,
	SDHC_READ_RESP_REG_ALL
};

enum sdhc_reset_type {
	SDHC_RESET_ALL = 0x01,
	SDHC_RESET_CMD = 0x02,
	SDHC_RESET_DAT = 0x04
};

enum sdhc_clk_mode {
	SDHC_DIVIDED_CLK_MODE = 0,
	SDHC_PROGRAMMABLE_CLK_MODE
};

enum sdhc_data_transfer_type {
	SDHC_DATA_TRANSFER_TYPE_SINGLE = 0,
	SDHC_DATA_TRANSFER_TYPE_MULTI,
	SDHC_DATA_TRANSFER_MMC_STREAM,
	SDHC_DATA_TRANSFER_SDIO_BYTE,
	SDHC_DATA_TRANSFER_SDIO_BLOCK,
};

enum sdhc_data_transfer_dir {
	SDHC_DATA_TRANSFER_DIR_WRITE = 0,
	SDHC_DATA_TRANSFER_DIR_READ
};

struct sdhc_data_transfer_flags {
	bool is_data_present;
	enum sdhc_data_transfer_dir transfer_dir;
	enum sdhc_data_transfer_type transfer_type;
};

struct sdhc_adma_descr {
	uint16_t attribute;
	uint16_t length;
	uint32_t address;
};

struct sdhc_object {
	bool is_cmd_in_progress;
	bool is_data_in_progress;
	bool cmdc_received;
	bool trfc_received;
	uint16_t error_status;
	uint32_t response_type;
};

struct sdhc_mchp_data {
	struct k_mutex bus_mutex;
	struct k_sem transfer_done;
	struct sdhc_host_props props;
	struct sdhc_command *cmd;
	int sdhc_response[4];
	volatile struct sdhc_object sdhc_obj;
	struct sdhc_adma_descr *dma_desc_table;
	bool card_supports_cmd23; /* Card supports CMD23 (from SCR) */
	/* Handle un-aligned cmd/+data transfers:
	 * Static bounce buffer for sub-cacheline control structures (CSD, CID, SCR, etc.)
	 */
	uint8_t dma_ctrl_aligned_buffer[SDHC_MCHP_AGNOSTIC_ALIGN_SIZE]
		__aligned(SDHC_MCHP_AGNOSTIC_ALIGN_SIZE);
	/* Handle un-aligned cmd/+data transfers:
	 * Dynamic bounce buffer pointer for unaligned filesystem/disk data payloads
	 */
	uint8_t *dma_data_aligned_buffer;
	/* Tracking metrics to safely bridge dma_setup and dma_teardown execution blocks */
	uint8_t *dma_original_data_ptr;
	size_t dma_cache_aligned_size;
	bool dma_used_static_aligned_buffer;
	bool dma_used_dynamic_aligned_buffer;
};

struct mchp_sdhc_clock {
	const struct device *clock_dev;
	clock_control_subsys_t mclk_sys;
	clock_control_subsys_t gclk_sys;
	clock_control_subsys_t gclk_slow_sys;
};

struct sdhc_mchp_cfg {
	const struct pinctrl_dev_config *pcfg;
	uintptr_t regs;
	struct mchp_sdhc_clock sdhc_clock;
	int power_delay_ms;
	uint32_t min_bus_freq;
	uint32_t max_bus_freq;
	uint16_t bus_width; /* Width of the SDIO bus */
	bool auto_cmd12;
	bool auto_cmd23;
	void (*irq_config_func)(const struct device *dev);
};

static void sdhc_reset_error(const struct device *dev, enum sdhc_reset_type reset_type)
{
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);

	sdhc_regs->SDHC_SRR = (uint8_t)reset_type;

	if (!WAIT_FOR(((sdhc_regs->SDHC_SRR & (uint8_t)reset_type) == 0U), SDHC_RESET_TIMEOUT_US,
		      k_busy_wait(1))) {
		LOG_WRN("SDHC reset timed out");
	}
}

static inline void sdhc_set_buswidth(const struct device *dev, enum sdhc_bus_width bus_width)
{
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);

	if (bus_width == SDHC_BUS_WIDTH4BIT) {
		sdhc_regs->SDHC_HC1R |= SDHC_HC1R_DW_4BIT;
	} else {
		sdhc_regs->SDHC_HC1R &= (uint8_t)(~SDHC_HC1R_DW_4BIT);
	}
}

static inline void sdhc_set_speedmode(const struct device *dev, enum sdhc_speed_mode speed_mode)
{
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);

	if (speed_mode == SDHC_SPEED_MODE_HIGH) {
		sdhc_regs->SDHC_HC1R |= SDHC_HC1R_HSEN_Msk;
	} else {
		sdhc_regs->SDHC_HC1R &= (uint8_t)(~SDHC_HC1R_HSEN_Msk);
	}
}

static inline bool sdhc_is_cmdline_busy(const struct device *dev)
{
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);

	return (((sdhc_regs->SDHC_PSR & SDHC_PSR_CMDINHC_Msk) == SDHC_PSR_CMDINHC_Msk) ? true
										       : false);
}

static inline bool sdhc_is_dataline_busy(const struct device *dev)
{
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);

	return (((sdhc_regs->SDHC_PSR & SDHC_PSR_CMDINHD_Msk) == SDHC_PSR_CMDINHD_Msk) ? true
										       : false);
}

static inline bool sdhc_is_card_attached(const struct device *dev)
{
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);

	return ((sdhc_regs->SDHC_PSR & SDHC_PSR_CARDINS_Msk) == SDHC_PSR_CARDINS_Msk) ? true
										      : false;
}

static void sdhc_set_blocksize(const struct device *dev, uint16_t block_size)
{
	struct sdhc_mchp_data *const dev_data = dev->data;
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);
	uint16_t max_block_size = 0U;

	max_block_size = SDHC_MAX_BLK_LEN_TO_BYTES(dev_data->props.host_caps.max_blk_len);
	if (block_size == 0U) {
		block_size = 1U;
	} else if (block_size > max_block_size) {
		block_size = max_block_size;
	} else {
		/* block_size is within the valid range; use as-is */
	}

	sdhc_regs->SDHC_BSR = (SDHC_BSR_BOUNDARY_4K | SDHC_BSR_BLOCKSIZE(block_size));
}

static inline void sdhc_set_blockcount(const struct device *dev, uint16_t num_blocks)
{
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);
	const struct sdhc_mchp_cfg *cfg = dev->config;

	sdhc_regs->SDHC_BCR = num_blocks;
	if (cfg->auto_cmd23) {
		sdhc_regs->SDHC_SSAR = num_blocks;
	}
}

static void sdhc_clock_enable(const struct device *dev)
{
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);

	/* Start the internal clock */
	sdhc_regs->SDHC_CCR |= SDHC_CCR_INTCLKEN_Msk;

	/* wait for the internal clock to stabilize */
	if (!WAIT_FOR(((sdhc_regs->SDHC_CCR & SDHC_CCR_INTCLKS_Msk) != 0U),
		      SDHC_CLOCK_STABLE_TIMEOUT_US, k_busy_wait(1))) {
		LOG_WRN("Internal clock stabilization timed out");
	}

	/* Enable the SD clock */
	sdhc_regs->SDHC_CCR |= SDHC_CCR_SDCLKEN_Msk;
}

static inline void sdhc_clock_disable(const struct device *dev)
{
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);

	sdhc_regs->SDHC_CCR &= (uint16_t)(~(SDHC_CCR_INTCLKEN_Msk | SDHC_CCR_SDCLKEN_Msk));
}

static void sdhc_set_transfermode(const struct device *dev, int opcode,
				  struct sdhc_data_transfer_flags transfer_flags)
{
	const struct sdhc_mchp_cfg *cfg = dev->config;
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);
	uint16_t transfer_mode = 0U;
	uint16_t auto_cmd_mode = 0U;

	/* Multi block R/W: Determine auto command mode
	 * Use CMD23 only if both host supports it (DTS) AND card supports it (SCR).
	 * Fall back to CMD12 otherwise - all cards support CMD12.
	 */
	struct sdhc_mchp_data *const dev_data = dev->data;

	if (cfg->auto_cmd23 && dev_data->card_supports_cmd23) {
		auto_cmd_mode = SDHC_TMR_ACMDEN_CMD23;
	} else if (cfg->auto_cmd12) {
		auto_cmd_mode = SDHC_TMR_ACMDEN_CMD12;
	}

	switch (opcode) {
	case SD_APP_SEND_SCR:
	case SD_APP_SET_BUS_WIDTH:
	case SD_READ_SINGLE_BLOCK:
		transfer_mode = (SDHC_TMR_DMAEN_ENABLE | SDHC_TMR_DTDSEL_Msk);
		break;

	case SD_READ_MULTIPLE_BLOCK:
		transfer_mode = (SDHC_TMR_DMAEN_ENABLE | SDHC_TMR_DTDSEL_Msk | SDHC_TMR_MSBSEL_Msk |
				 SDHC_TMR_BCEN_Msk | auto_cmd_mode);
		break;

	case SD_WRITE_SINGLE_BLOCK:
		transfer_mode = SDHC_TMR_DMAEN_ENABLE;
		break;

	case SD_WRITE_MULTIPLE_BLOCK:
		transfer_mode = (SDHC_TMR_DMAEN_ENABLE | SDHC_TMR_MSBSEL_Msk | SDHC_TMR_BCEN_Msk |
				 auto_cmd_mode);
		break;

	case SDIO_RW_EXTENDED:
		if (transfer_flags.transfer_type == SDHC_DATA_TRANSFER_SDIO_BLOCK) {
			transfer_mode = SDHC_TMR_MSBSEL_Msk | SDHC_TMR_BCEN_Msk;
		}

		if (transfer_flags.transfer_dir == SDHC_DATA_TRANSFER_DIR_READ) {
			transfer_mode |= SDHC_TMR_DTDSEL_Msk;
		}

		transfer_mode |= SDHC_TMR_DMAEN_ENABLE;
		break;

	default:
		break;
	}

	sdhc_regs->SDHC_TMR = transfer_mode;
}

static int sdhc_dma_adma2_configure(const struct device *dev, uint8_t *buffer, int num_bytes)
{
	struct sdhc_mchp_data *const dev_data = dev->data;
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);
	uint32_t remaining;
	uint32_t addr;
	uint32_t i = 0U;
	size_t desc_flush_size;

	if ((buffer == NULL) || (num_bytes <= 0)) {
		LOG_ERR("Invalid buffer pointer or transfer size");
		return -EINVAL;
	}

	if (dev_data->props.host_caps.adma_2_support == false) {
		LOG_ERR("ADMA2 is not supported by this SDHC controller");
		return -ENOTSUP;
	}

	/* Guard: Verify that the overall transfer size doesn't overflow table bounds */
	if ((uint32_t)num_bytes >
	    (CONFIG_SDHC_MCHP_G1_ADMA_DESC_COUNT * SDHC_ADMA2_DESC_MAX_BYTES)) {
		LOG_ERR("Transfer size %d exceeds chained ADMA2 capacity %u", num_bytes,
			(unsigned int)(CONFIG_SDHC_MCHP_G1_ADMA_DESC_COUNT *
				       SDHC_ADMA2_DESC_MAX_BYTES));
		return -ENOMEM;
	}

	remaining = (uint32_t)num_bytes;
	addr = (uint32_t)(uintptr_t)buffer;

	while (remaining > 0U) {
		uint32_t chunk = (remaining > SDHC_ADMA2_DESC_MAX_BYTES) ? SDHC_ADMA2_DESC_MAX_BYTES
									 : remaining;
		uint16_t attr =
			SDHC_ADMA_DESC_TABLE_ATTR_XFER_DATA | SDHC_ADMA_DESC_TABLE_ATTR_VALID;

		dev_data->dma_desc_table[i].address = addr;
		dev_data->dma_desc_table[i].length = (uint16_t)chunk;

		remaining -= chunk;
		addr += chunk;

		if (remaining == 0U) {
			attr |= SDHC_ADMA_DESC_TABLE_ATTR_INTR | SDHC_ADMA_DESC_TABLE_ATTR_END;
		}
		dev_data->dma_desc_table[i].attribute = attr;
		i++;
	}

	desc_flush_size = i * sizeof(dev_data->dma_desc_table[0]);
	desc_flush_size = ROUND_UP(desc_flush_size, SDHC_MCHP_AGNOSTIC_ALIGN_SIZE);
#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
	sys_cache_data_flush_range(dev_data->dma_desc_table, desc_flush_size);
#endif

#if defined(CONFIG_SOC_FAMILY_MICROCHIP_PIC32CK_SG_GC) ||                                          \
	defined(CONFIG_SOC_FAMILY_MICROCHIP_PIC32CZ_CA)
	sdhc_regs->SDHC_ASAR = (uint32_t)(uintptr_t)&dev_data->dma_desc_table[0];
#else
	sdhc_regs->SDHC_ASAR[0] = (uint32_t)(uintptr_t)&dev_data->dma_desc_table[0];
#endif
	return 0;
}

static void sdhc_dma_teardown(const struct device *dev, struct sdhc_command *cmd,
			      struct sdhc_data *data)
{
	struct sdhc_mchp_data *const dev_data = dev->data;

	if (data == NULL || dev_data->dma_original_data_ptr == NULL) {
		LOG_DBG("No DMA teardown required: data or original buffer pointer is NULL");
		return;
	}

	bool is_tx =
		(cmd->opcode == SD_WRITE_SINGLE_BLOCK || cmd->opcode == SD_WRITE_MULTIPLE_BLOCK ||
		 (cmd->opcode == SDIO_RW_EXTENDED &&
		  ((cmd->arg & BIT(SDIO_CMD_ARG_RW_SHIFT)) != 0U)));

	/*
	 * Rx Path : Invalidate cache lines immediately after hardware dma completion
	 * to discard speculative CPU pre-fetches and force reading pristine data from SRAM.
	 * This prevents reading the old, stale data currently sitting in its cache lines instead
	 * of the new data the DMA engine just wrote.
	 */
	if (!is_tx) {
#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
		sys_cache_data_invd_range((void *)data->data, dev_data->dma_cache_aligned_size);
#endif

		/* Relocate clean data payloads from safe bounce regions back to application memory
		 * space
		 */
		if (dev_data->dma_used_static_aligned_buffer ||
		    dev_data->dma_used_dynamic_aligned_buffer) {
			size_t total_bytes = (size_t)data->blocks * (size_t)data->block_size;
			(void)memcpy(dev_data->dma_original_data_ptr, data->data, total_bytes);
#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
			/* Invalidate the final user destination memory to guarantee cache coherence
			 */
			sys_cache_data_invd_range((void *)dev_data->dma_original_data_ptr,
						  total_bytes);
#endif
		}
	}
	/* point the subsystem framework back to the host memory location */
	data->data = dev_data->dma_original_data_ptr;

	/* Release the dynamically allocated heap block if a large fallback allocation occurred */
	if (dev_data->dma_used_dynamic_aligned_buffer &&
	    dev_data->dma_data_aligned_buffer != NULL) {
		k_free(dev_data->dma_data_aligned_buffer);
		dev_data->dma_data_aligned_buffer = NULL;
	}

	/* Clear tracking metrics */
	dev_data->dma_original_data_ptr = NULL;
	dev_data->dma_cache_aligned_size = 0;
	dev_data->dma_used_static_aligned_buffer = false;
	dev_data->dma_used_dynamic_aligned_buffer = false;
}

static int sdhc_dma_setup(const struct device *dev, struct sdhc_command *cmd,
			  struct sdhc_data *data)
{
	struct sdhc_mchp_data *const dev_data = dev->data;
	size_t align_mask = SDHC_MCHP_AGNOSTIC_ALIGN_SIZE - 1U;
	size_t total_bytes = (size_t)data->blocks * (size_t)data->block_size;
	int rc = 0;
	bool is_tx =
		(cmd->opcode == SD_WRITE_SINGLE_BLOCK || cmd->opcode == SD_WRITE_MULTIPLE_BLOCK ||
		 (cmd->opcode == SDIO_RW_EXTENDED &&
		  ((cmd->arg & BIT(SDIO_CMD_ARG_RW_SHIFT)) != 0U)));

	/* Initialize properties tracked across the instance context */
	dev_data->dma_original_data_ptr = (uint8_t *)data->data;
	dev_data->dma_used_static_aligned_buffer = false;
	dev_data->dma_used_dynamic_aligned_buffer = false;
	dev_data->dma_cache_aligned_size = 0;

	/* Check whether the data buffer and total transfer size are cache line aligned */
	if (((uintptr_t)data->data & align_mask) != 0U || (total_bytes & align_mask) != 0U) {

		if (total_bytes < SDHC_MCHP_AGNOSTIC_ALIGN_SIZE) {
			dev_data->dma_used_static_aligned_buffer = true;
			dev_data->dma_cache_aligned_size = SDHC_MCHP_AGNOSTIC_ALIGN_SIZE;

			if (is_tx) {
				(void)memcpy(dev_data->dma_ctrl_aligned_buffer,
					     dev_data->dma_original_data_ptr, total_bytes);
				(void)memset(&dev_data->dma_ctrl_aligned_buffer[total_bytes], 0,
					     SDHC_MCHP_AGNOSTIC_ALIGN_SIZE - total_bytes);
			}
			data->data = dev_data->dma_ctrl_aligned_buffer;
		} else {
			dev_data->dma_used_dynamic_aligned_buffer = true;
			dev_data->dma_cache_aligned_size =
				ROUND_UP(total_bytes, SDHC_MCHP_AGNOSTIC_ALIGN_SIZE);

			dev_data->dma_data_aligned_buffer = k_aligned_alloc(
				SDHC_MCHP_AGNOSTIC_ALIGN_SIZE, dev_data->dma_cache_aligned_size);
			if (dev_data->dma_data_aligned_buffer == NULL) {
				LOG_ERR("SDHC DMA: Failed to allocate buffer to handle unaligned "
					"data.Try increasing CONFIG_HEAP_MEM_POOL_SIZE or reduce "
					"transfer size.");
				return -ENOMEM;
			}

			if (is_tx) {
				(void)memcpy(dev_data->dma_data_aligned_buffer,
					     dev_data->dma_original_data_ptr, total_bytes);
			}
			data->data = dev_data->dma_data_aligned_buffer;
		}
	} else {
		dev_data->dma_cache_aligned_size = total_bytes;
	}

#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
	/* Flush the cache for transmit operations:
	 * Writes modified data(cache lines) back to physical memory.before DMA
	 */
	if (is_tx) {
		sys_cache_data_flush_range((void *)data->data, dev_data->dma_cache_aligned_size);
	}
#endif

	sdhc_set_blocksize(dev, (uint16_t)data->block_size);
	sdhc_set_blockcount(dev, (uint16_t)data->blocks);

	/* Configure hardware DMA (ADMA2)*/
	rc = sdhc_dma_adma2_configure(dev, (uint8_t *)data->data, (int)total_bytes);
	if (rc != 0) {
		if (dev_data->dma_used_dynamic_aligned_buffer &&
		    dev_data->dma_data_aligned_buffer != NULL) {
			k_free(dev_data->dma_data_aligned_buffer);
			dev_data->dma_data_aligned_buffer = NULL;
		}
		data->data = dev_data->dma_original_data_ptr;
	}

	return rc;
}

static bool sdhc_set_clock(const struct device *dev, int speed)
{
	struct sdhc_mchp_data *const dev_data = dev->data;
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);
	uint32_t base_clk_freq_hz = 0U;
	uint16_t divider = 0U;
	uint32_t clk_mul = 0U;

	if ((sdhc_regs->SDHC_CCR & SDHC_CCR_SDCLKEN_Msk) != 0U) {
		if (!WAIT_FOR(((sdhc_regs->SDHC_PSR &
				(SDHC_PSR_CMDINHC_Msk | SDHC_PSR_CMDINHD_Msk)) == 0U),
			      SDHC_CMD_IDLE_TIMEOUT_US, k_busy_wait(1))) {
			LOG_WRN("SDHC bus did not become idle before clock change");
		}
		sdhc_regs->SDHC_CCR &= (uint16_t)(~SDHC_CCR_SDCLKEN_Msk);
	}

	base_clk_freq_hz = dev_data->props.host_caps.sd_base_clk * MHZ(1); /* Convert MHz to Hz */
	clk_mul = dev_data->props.host_caps.clk_multiplier;
	if (clk_mul == 0U) {
		/* Calculate divider for divided clock mode */
		/* F_SDCLK = (F_BASECLK /(2 * DIV) */
		/* For a given F_SDCLK, DIV = [F_BASECLK /(F_SDCLK * 2)] */
		/* Ensure output frequency is below the target threshold by rounding up */
		/* the divider value */

		uint32_t divisor = (speed * 2U);

		divider = (uint16_t)DIV_ROUND_UP(base_clk_freq_hz, divisor);
		sdhc_regs->SDHC_CCR &= (uint16_t)(~SDHC_CCR_CLKGSEL_Msk);
	} else {
		/* Calculate divider for programmable clock mode */
		/* F_SDCLK = F_MULTCLK/(DIV+1), where F_MULTCLK = F_BASECLK x (CLKMULT+1) */
		/* F_SDCLK = (F_BASECLK x (CLKMULT + 1))/(DIV + 1) */
		/* For a given F_SDCLK, DIV = [(F_BASECLK x (CLKMULT + 1))/F_SDCLK] - 1 */
		/* Ensure output frequency is below the target threshold by rounding up */
		/* the divider value */

		uint32_t multi_clk_freq_hz = (base_clk_freq_hz * (clk_mul + 1U));

		divider = (uint16_t)DIV_ROUND_UP(multi_clk_freq_hz, speed);
		if (divider > 0U) {
			divider = divider - 1U;
		}
		/* Select Programmable Clock mode (MULTCLK is used to generate SDCLK) */
		sdhc_regs->SDHC_CCR |= SDHC_CCR_CLKGSEL_Msk;
	}

	if (speed > SD_CLOCK_25MHZ) {
		/* Enable the high speed mode */
		sdhc_set_speedmode(dev, SDHC_SPEED_MODE_HIGH);
	} else {
		/* clear the high speed mode */
		sdhc_set_speedmode(dev, SDHC_SPEED_MODE_NORMAL);
	}

	if (((sdhc_regs->SDHC_HC1R & SDHC_HC1R_HSEN_Msk) != 0U) && (divider == 0U)) {
		/* IP limitation, if high speed mode is active divider must be non zero */
		divider = 1;
	}

	sdhc_regs->SDHC_CCR &= (uint16_t)(~(SDHC_CCR_SDCLKFSEL_Msk | SDHC_CCR_USDCLKFSEL_Msk));
	sdhc_regs->SDHC_CCR |=
		SDHC_CCR_SDCLKFSEL((divider & 0xFFU)) | SDHC_CCR_USDCLKFSEL((divider >> 8));

	sdhc_regs->SDHC_CCR |= SDHC_CCR_INTCLKEN_Msk;

	if (!WAIT_FOR(((sdhc_regs->SDHC_CCR & SDHC_CCR_INTCLKS_Msk) != 0U),
		      SDHC_CLOCK_STABLE_TIMEOUT_US, k_busy_wait(1))) {
		LOG_WRN("Internal clock stabilization timed out in set_clock");
	}

	sdhc_regs->SDHC_CCR |= SDHC_CCR_SDCLKEN_Msk;

	return true;
}

static void sdhc_read_response(const struct device *dev, enum sdhc_read_response_reg resp_reg,
			       int *response)
{
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);

	switch (resp_reg) {
	case SDHC_READ_RESP_REG_1:
		*response = sdhc_regs->SDHC_RR[1];
		break;

	case SDHC_READ_RESP_REG_2:
		*response = sdhc_regs->SDHC_RR[2];
		break;

	case SDHC_READ_RESP_REG_3:
		*response = sdhc_regs->SDHC_RR[3];
		break;

	case SDHC_READ_RESP_REG_ALL:
		response[0] = sdhc_regs->SDHC_RR[0];
		response[1] = sdhc_regs->SDHC_RR[1];
		response[2] = sdhc_regs->SDHC_RR[2];
		response[3] = sdhc_regs->SDHC_RR[3];
		break;

	case SDHC_READ_RESP_REG_0:
	default:
		*response = sdhc_regs->SDHC_RR[0];
		break;
	}
}

static void sdhc_send_command(const struct device *dev, uint8_t op_code, int argument,
			      uint8_t resp_type, struct sdhc_data_transfer_flags transfer_flags)
{
	struct sdhc_mchp_data *const dev_data = dev->data;
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);
	uint16_t cmd = 0U;
	uint16_t normal_int_sig_enable = 0U;
	uint16_t flags = 0U;

	dev_data->sdhc_obj.is_cmd_in_progress = false;
	dev_data->sdhc_obj.is_data_in_progress = false;
	dev_data->sdhc_obj.error_status = 0U;
	dev_data->sdhc_obj.cmdc_received = false;
	dev_data->sdhc_obj.trfc_received = false;
	dev_data->sdhc_obj.response_type = resp_type;

	normal_int_sig_enable = (SDHC_NISIER_CINS_Msk | SDHC_NISIER_CREM_Msk);

	switch (resp_type) {
	case SD_RSP_TYPE_R1:
	case SD_RSP_TYPE_R5:
	case SD_RSP_TYPE_R6:
	case SD_RSP_TYPE_R7:
		flags = (SDHC_CR_RESPTYP_48_BIT_Val | SDHC_CR_CMDCCEN_Msk | SDHC_CR_CMDICEN_Msk);
		break;

	case SD_RSP_TYPE_R3:
	case SD_RSP_TYPE_R4:
		flags = SDHC_CR_RESPTYP_48_BIT_Val;
		break;

	case SD_RSP_TYPE_R1b:
		flags = (SDHC_CR_RESPTYP_48_BIT_BUSY_Val | SDHC_CR_CMDCCEN_Msk |
			 SDHC_CR_CMDICEN_Msk);

		/* Enable both CMDC (Command Complete) and TRFC (Transfer Complete) when handling */
		/* R1b responses to avoid reading stale response data or causing interrupt storms */
		normal_int_sig_enable |= SDHC_NISIER_TRFC_Msk;
		break;

	case SD_RSP_TYPE_R2:
		flags = (SDHC_CR_RESPTYP_136_BIT_Val | SDHC_CR_CMDCCEN_Msk);
		break;

	default:
		flags = SDHC_CR_RESPTYP_NONE_Val;
		break;
	}

	/* Enable CMDC for all response types */
	normal_int_sig_enable |= SDHC_NISIER_CMDC_Msk;

	if (transfer_flags.is_data_present == true) {
		dev_data->sdhc_obj.is_data_in_progress = true;
		sdhc_set_transfermode(dev, op_code, transfer_flags);
		normal_int_sig_enable |= (SDHC_NISIER_TRFC_Msk | SDHC_NISIER_DMAINT_Msk);
	} else {
		sdhc_regs->SDHC_TMR = 0U;
	}

	sdhc_regs->SDHC_NISIER = normal_int_sig_enable;
	sdhc_regs->SDHC_EISIER = SDHC_EISIER_Msk;

	sdhc_regs->SDHC_ARG1R = argument;

	dev_data->sdhc_obj.is_cmd_in_progress = true;

	cmd = (SDHC_CR_CMDIDX((uint16_t)op_code) |
	       (transfer_flags.is_data_present ? (1U << SDHC_CR_DPSEL_Pos) : 0U) | (uint16_t)flags);
	sdhc_regs->SDHC_CR = cmd;
}

static void sdhc_init_module(const struct device *dev)
{
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);

	sdhc_regs->SDHC_SRR |= SDHC_SRR_SWRSTALL_Msk;
	if (!WAIT_FOR(((sdhc_regs->SDHC_SRR & SDHC_SRR_SWRSTALL_Msk) == 0U), SDHC_RESET_TIMEOUT_US,
		      k_busy_wait(1))) {
		LOG_WRN("SDHC software reset timed out");
	}

	sdhc_regs->SDHC_EISTR = SDHC_EISTR_Msk;
	sdhc_regs->SDHC_NISTR = SDHC_NISTR_Msk;
	sdhc_regs->SDHC_NISTER = SDHC_NISTER_Msk;
	sdhc_regs->SDHC_EISTER = SDHC_EISTER_Msk;
	sdhc_regs->SDHC_TCR = SDHC_TCR_DTCVAL(SDHC_DATA_TIMEOUT_COUNTER_VAL);
	sdhc_regs->SDHC_HC1R |= SDHC_HC1R_DMASEL(SDHC_DMASEL_ADMA2);
	sdhc_regs->SDHC_PCR = (SDHC_PCR_SDBVSEL_3V3 | SDHC_PCR_SDBPWR_ON);

	(void)sdhc_set_clock(dev, SDMMC_CLOCK_400KHZ);

	sdhc_regs->SDHC_HC1R &= (uint8_t)(~(SDHC_HC1R_HSEN_Msk | SDHC_HC1R_DW_Msk));
	sdhc_regs->SDHC_NISIER = (SDHC_NISIER_CINS_Msk | SDHC_NISIER_CREM_Msk);
}

/**
 * @brief Blocks on the transfer semaphore and handles the HW errors comprehensively
 */
static int sdhc_wait_completion(struct sdhc_mchp_data *const dev_data, k_timeout_t timeout)
{
	int rc = 0;
	/* Establish an un-extendable absolute wait expiration */
	k_timepoint_t xfer_deadline = sys_timepoint_calc(timeout);

	/*
	 * Keep waiting until the ISR flags indicate that both command and data
	 * phases are no longer in progress. Safely absorbs multiple semaphore wakeup tokens or
	 * combined ticks.
	 */
	while (dev_data->sdhc_obj.is_cmd_in_progress || dev_data->sdhc_obj.is_data_in_progress) {

		/* Recalculate exactly how much time slice remains before the deadline */
		k_timeout_t remaining = sys_timepoint_timeout(xfer_deadline);

		/* If our absolute tracking deadline has expired, fail immediately */
		if (K_TIMEOUT_EQ(remaining, K_NO_WAIT)) {
			LOG_ERR("Timeout: Transfer completion flags did not clear within software "
				"timeout");
			return -ETIMEDOUT;
		}

		/* Sleep on the semaphore until the next phase concludes or a timeout hits */
		rc = k_sem_take(&dev_data->transfer_done, remaining);
		if (rc != 0) {
			LOG_ERR("Timeout: Transfer completion flags did not clear within software "
				"timeout");
			return -ETIMEDOUT;
		}

		/* Early Break: If the ISR registered a bus exception, parse it immediately */
		if (dev_data->sdhc_obj.error_status != 0U) {
			break;
		}
	}

	/* Check accumulated register errors captured inside the ISR */
	uint16_t hw_err = dev_data->sdhc_obj.error_status;

	if (hw_err != 0U) {
		/* Handle Timeout errors */
		if ((hw_err & SDHC_EISTR_CMDTEO_Msk) != 0U) {
			LOG_ERR("Hardware Error: Command Timeout (CMDTEO) detected");
			return -ETIMEDOUT;
		}
		if ((hw_err & SDHC_EISTR_DATTEO_Msk) != 0U) {
			LOG_ERR("Hardware Error: Data Timeout (DATTEO) detected");
			return -ETIMEDOUT;
		}

		/* Handle Signal Integrity Failures (Bad CRC Math) */
		if ((hw_err & SDHC_EISTR_CMDCRC_Msk) != 0U) {
			LOG_ERR("Hardware Error: Command CRC Mismatch (CMDCRC)");
			return -EBADMSG; /* Bad Message / Corrupted framing */
		}
		if ((hw_err & SDHC_EISTR_DATCRC_Msk) != 0U) {
			LOG_ERR("Hardware Error: Data CRC Mismatch (DATCRC)");
			return -EBADMSG;
		}

		/* Handle Protocol / Framing Format Failures */
		if ((hw_err & SDHC_EISTR_CMDIDX_Msk) != 0U) {
			LOG_ERR("Hardware Error: Command Index Mismatch (CMDIDX)");
			return -EILSEQ; /* Illegal byte sequence / Protocol structural breakdown */
		}
		if ((hw_err & SDHC_EISTR_CMDEND_Msk) != 0U) {
			LOG_ERR("Hardware Error: Command End Bit Violation (CMDEND)");
			return -EILSEQ;
		}
		if ((hw_err & SDHC_EISTR_DATEND_Msk) != 0U) {
			LOG_ERR("Hardware Error: Data End Bit Violation (DATEND)");
			return -EILSEQ;
		}

		/* Handle DMA Descriptor / Memory Boundary Faults */
		if ((hw_err & SDHC_EISTR_ADMA_Msk) != 0U) {
			LOG_ERR("Hardware Error: ADMA Descriptor/Bus Error (ADMAE)");
			return -EFAULT; /* Bad Memory Address / Descriptor Access Fault */
		}

		/* Fallback for completely unexpected hardware exceptions */
		LOG_ERR("Hardware Error: General transaction failure (EISTR=0x%04x)", hw_err);
		return -EIO;
	}

	return 0; /* Completed cleanly without any errors */
}

static int sdhc_init_props(const struct device *dev)
{
	struct sdhc_mchp_data *const dev_data = dev->data;
	const struct sdhc_mchp_cfg *cfg = dev->config;
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);
	uint32_t base_clk_freq_Mhz = 0U;
	uint32_t gclk_freq_hz = 0U;
	int ret = 0;

	uint32_t cap0 = sdhc_regs->SDHC_CA0R;
	uint32_t cap1 = sdhc_regs->SDHC_CA1R;

	memset(&dev_data->props, 0, sizeof(dev_data->props));
	dev_data->props.f_min = cfg->min_bus_freq;
	dev_data->props.f_max = cfg->max_bus_freq;
	dev_data->props.power_delay = cfg->power_delay_ms;
	dev_data->props.host_caps.timeout_clk_freq =
		(cap0 & SDHC_CA0R_TEOCLKF_Msk) >> SDHC_CA0R_TEOCLKF_Pos;
	dev_data->props.host_caps.timeout_clk_unit =
		(cap0 & SDHC_CA0R_TEOCLKU_Msk) >> SDHC_CA0R_TEOCLKU_Pos;
	/* Get base clock from hardware capability register */
	base_clk_freq_Mhz = (cap0 & (SDHC_CA0R_BASECLKF_Msk)) >> SDHC_CA0R_BASECLKF_Pos;
	if (base_clk_freq_Mhz == 0U) {
		/* Fallback: get clock rate from the GCLK control subsystem.
		 * Make sure the GCLK(SDHC core) is enabled before reading its rate.
		 * clock_control_get_rate() can succeed (ret == 0) yet report 0 Hz if the
		 * clock tree is misconfigured or the rate cannot be resolved. A 0 Hz
		 * rate would give divider 0 and clock the card at the undivided rate
		 * instead of the required <=400 kHz identification speed, so treat it
		 * as an error.
		 */
		if (!WAIT_FOR((clock_control_get_status(cfg->sdhc_clock.clock_dev,
							cfg->sdhc_clock.gclk_sys) ==
			       CLOCK_CONTROL_STATUS_ON),
			      (SDHC_GCLK_READY_TIMEOUT_MS * 1000), k_busy_wait(1000))) {
			LOG_ERR("SDHC GCLK did not come up within timeout, so failed to configure "
				"base clock");
			return -ETIMEDOUT;
		}

		ret = clock_control_get_rate(cfg->sdhc_clock.clock_dev, cfg->sdhc_clock.gclk_sys,
					     &gclk_freq_hz);
		if (ret != 0) {
			LOG_ERR("Failed to read glck rate/freq %u", gclk_freq_hz);
			return -EIO;
		}
		if (gclk_freq_hz == 0U) {
			LOG_ERR("SDHC GCLK reported ON but rate is 0 Hz");
			return -EIO;
		}
		/* Base clock is GCLK/2. With CA1R.CLKMULT = 1 the controller runs in
		 * programmable clock mode: MULTCLK = BASECLK * (CLKMULT + 1) = GCLK.
		 */
		base_clk_freq_Mhz = (gclk_freq_hz / 2);
		base_clk_freq_Mhz = base_clk_freq_Mhz / MHZ(1); /* Convert Hz to MHz */
		LOG_DBG("SDHC GCLK(sdhc core) rate: %u Hz", gclk_freq_hz);
		LOG_DBG("SDHC base clock: %u MHz", base_clk_freq_Mhz);
	}
	dev_data->props.host_caps.sd_base_clk = base_clk_freq_Mhz;

#ifdef CONFIG_SOC_FAMILY_MICROCHIP_SAM_D5X_E5X
	dev_data->props.host_caps.max_blk_len =
		(cap0 & SDHC_CA0R_MAXBLKL_Msk) >> SDHC_CA0R_MAXBLKL_Pos;
#else
	dev_data->props.host_caps.max_blk_len = 0; /* 512 bytes */
#endif

	dev_data->props.host_caps.bus_8_bit_support = (bool)(cfg->bus_width == SDHC_BUS_WIDTH8BIT);
	dev_data->props.host_caps.adma_2_support = (bool)(cap0 & SDHC_CA0R_ADMA2SUP_Msk);
	dev_data->props.host_caps.high_spd_support = (bool)(cap0 & SDHC_CA0R_HSSUP_Msk);
	dev_data->props.host_caps.sdma_support = (bool)(cap0 & SDHC_CA0R_SDMASUP_Msk);
	dev_data->props.host_caps.suspend_res_support = (bool)(cap0 & SDHC_CA0R_SRSUP_Msk);
	dev_data->props.host_caps.vol_330_support = (bool)(cap0 & SDHC_CA0R_V33VSUP_Msk);
	dev_data->props.host_caps.vol_300_support = (bool)(cap0 & SDHC_CA0R_V30VSUP_Msk);
	dev_data->props.host_caps.vol_180_support = false;
	dev_data->props.host_caps.address_64_bit_support_v4 = false;
	dev_data->props.host_caps.address_64_bit_support_v3 = false;
	dev_data->props.host_caps.sdio_async_interrupt_support =
		(bool)(cap0 & SDHC_CA0R_ASINTSUP_Msk);
	dev_data->props.host_caps.slot_type = (cap0 & SDHC_CA0R_SLTYPE_Msk) >> SDHC_CA0R_SLTYPE_Pos;

	/* UHS related props */
	dev_data->props.host_caps.sdr50_support = false;
	dev_data->props.host_caps.sdr104_support = false;
	dev_data->props.host_caps.ddr50_support = false;
	dev_data->props.host_caps.uhs_2_support = false;
	dev_data->props.host_caps.drv_type_a_support = false;
	dev_data->props.host_caps.drv_type_c_support = false;
	dev_data->props.host_caps.drv_type_d_support = false;
	dev_data->props.host_caps.retune_timer_count = 0;
	dev_data->props.host_caps.sdr50_needs_tuning = false;
	dev_data->props.host_caps.retuning_mode = 0;

	dev_data->props.host_caps.clk_multiplier =
		(cap1 & SDHC_CA1R_CLKMULT_Msk) >> SDHC_CA1R_CLKMULT_Pos;
	dev_data->props.host_caps.adma3_support = false;
	dev_data->props.host_caps.vdd2_180_support = false;
	dev_data->props.bus_4_bit_support = (bool)(cfg->bus_width == SDHC_BUS_WIDTH4BIT);
	dev_data->props.hs200_support = false;
	dev_data->props.hs400_support = false;
	dev_data->props.is_spi = false;

	return 0;
}

static void sdhc_mchp_isr(const struct device *dev)
{
	struct sdhc_mchp_data *const dev_data = dev->data;
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);
	uint16_t nistr = 0U;
	uint16_t eistr = 0U;
	uint32_t xfer_status = 0U;
	bool error = false;

	nistr = sdhc_regs->SDHC_NISTR;
	eistr = sdhc_regs->SDHC_EISTR;

	/* Accumulate hardware error bits for the main thread to safely parse */
	dev_data->sdhc_obj.error_status |= eistr;

	if ((nistr & SDHC_NISTR_CINS_Msk) != 0U) {
		xfer_status |= SDHC_XFER_STATUS_CARD_INSERTED;
	}

	if ((nistr & SDHC_NISTR_CREM_Msk) != 0U) {
		xfer_status |= SDHC_XFER_STATUS_CARD_REMOVED;
	}

	/* Handle hardware fault/error interrupts */
	if ((nistr & SDHC_NISTR_ERRINT_Msk) != 0U) {
		error = true;
		if (dev_data->sdhc_obj.is_cmd_in_progress) {
			sdhc_reset_error(dev, SDHC_RESET_CMD);
			dev_data->sdhc_obj.is_cmd_in_progress = false;
			xfer_status |= SDHC_XFER_STATUS_CMD_COMPLETED;
		}
		if (dev_data->sdhc_obj.is_data_in_progress) {
			sdhc_reset_error(dev, SDHC_RESET_DAT);
			dev_data->sdhc_obj.is_data_in_progress = false;
			xfer_status |= SDHC_XFER_STATUS_DATA_COMPLETED;
		}
	}

	/* Handle command completion interrupts */
	if (dev_data->sdhc_obj.is_cmd_in_progress == true) {
		bool cmd_done = false;

		/* Track which interrupts have fired */
		if ((nistr & SDHC_NISTR_CMDC_Msk) != 0U) {
			dev_data->sdhc_obj.cmdc_received = true;
		}

		if ((nistr & SDHC_NISTR_TRFC_Msk) != 0U) {
			dev_data->sdhc_obj.trfc_received = true;
		}

		/*
		 * R1b: wait for both CMDC and TRFC before completing.
		 * This handles the case where TRFC fires before CMDC (in devices like PIC32CK
		 * SG/GC). Non-R1b: complete on either CMDC or TRFC.
		 */
		if (dev_data->sdhc_obj.response_type == SD_RSP_TYPE_R1b) {
			cmd_done = (dev_data->sdhc_obj.cmdc_received &&
				    dev_data->sdhc_obj.trfc_received);
		} else {
			/* Non R1b: Complete on CMDC or TRFC */
			cmd_done = (dev_data->sdhc_obj.cmdc_received ||
				    dev_data->sdhc_obj.trfc_received);
		}
		if (cmd_done == true) {
			dev_data->sdhc_obj.is_cmd_in_progress = false;
			xfer_status |= SDHC_XFER_STATUS_CMD_COMPLETED;
		}
	}
	/* Handle data transfer completion interrupts */
	if (dev_data->sdhc_obj.is_data_in_progress && (nistr & SDHC_NISTR_TRFC_Msk) != 0U) {
		dev_data->sdhc_obj.is_data_in_progress = false;
		xfer_status |= SDHC_XFER_STATUS_DATA_COMPLETED;
	}
	/* Clear the Data Timeout error if TRFC has fired, since the data transfer completed
	 * successfully
	 */
	/* This safely clears data timeouts during sequential transfers */
	if ((nistr & SDHC_NISTR_TRFC_Msk) != 0U) {
		dev_data->sdhc_obj.error_status &= (uint16_t)(~SDHC_EISTR_DATTEO_Msk);
	}

	/* Clear evaluated interrupt state bits */
	sdhc_regs->SDHC_NISTR = nistr;
	sdhc_regs->SDHC_EISTR = eistr;

	/*
	 * Synchronization Guard: Trigger a semaphore wakeup token if an error occurred
	 * or one transaction(command or data complete) is finished.
	 */
	if (error || (xfer_status != 0U)) {
		k_sem_give(&dev_data->transfer_done);
	}
}

static int sdhc_mchp_request(const struct device *dev, struct sdhc_command *cmd,
			     struct sdhc_data *data)
{
	int rc = 0;
	struct sdhc_mchp_data *const dev_data = dev->data;
	struct sdhc_data_transfer_flags transfer_flags = {0};
	uint8_t op = (uint8_t)cmd->opcode;
	k_timeout_t cmd_timeout;
	k_timeout_t data_timeout;
	k_timeout_t xfer_timeout;
	uint8_t resp_type;

	__ASSERT_NO_MSG(cmd != NULL);
	dev_data->cmd = cmd;

	if (cmd->timeout_ms == SDHC_TIMEOUT_FOREVER) {
		cmd_timeout = K_FOREVER;
	} else if (cmd->timeout_ms == 0) {
		cmd_timeout = K_MSEC(SDHC_CMD_DEFAULT_TIMEOUT_MS);
	} else {
		cmd_timeout = K_MSEC(cmd->timeout_ms);
	}
	xfer_timeout = cmd_timeout;

	if (data != NULL) {
		if (data->timeout_ms == SDHC_TIMEOUT_FOREVER) {
			data_timeout = K_FOREVER;
		} else if (data->timeout_ms == 0) {
			data_timeout = K_MSEC(SDHC_DATA_DEFAULT_TIMEOUT_MS);
		} else {
			data_timeout = K_MSEC(data->timeout_ms);
		}
		xfer_timeout = K_TIMEOUT_SUM(cmd_timeout, data_timeout);

		rc = sdhc_dma_setup(dev, cmd, data);
		if (rc != 0) {
			LOG_ERR("Error: Failed to setup DMA for data transfer: %d", rc);
			return rc;
		}

		transfer_flags.is_data_present = true;
		if (cmd->opcode == SDIO_RW_EXTENDED) {
			transfer_flags.transfer_type = SDHC_DATA_TRANSFER_SDIO_BLOCK;
			if (IS_BIT_SET(cmd->arg, SDIO_CMD_ARG_RW_SHIFT)) {
				transfer_flags.transfer_dir = SDHC_DATA_TRANSFER_DIR_WRITE;
			} else {
				transfer_flags.transfer_dir = SDHC_DATA_TRANSFER_DIR_READ;
			}
		} else {
			transfer_flags.transfer_type = SDHC_DATA_TRANSFER_TYPE_SINGLE;
		}
	}

	if (k_mutex_lock(&dev_data->bus_mutex, xfer_timeout) != 0) {
		return -EBUSY;
	}

	sdhc_reset_error(dev, SDHC_RESET_CMD | SDHC_RESET_DAT);

	/*
	 * Flush stale completion tokens from previous operations
	 * before issuing the command
	 */
	k_sem_reset(&dev_data->transfer_done);

	/* Clear the previous HW errors */
	dev_data->sdhc_obj.error_status = 0U;

	resp_type = (uint8_t)(cmd->response_type & SDHC_NATIVE_RESPONSE_MASK);
	sdhc_send_command(dev, op, cmd->arg, resp_type, transfer_flags);

	/*
	 * Wait for completion: Pass the timeout directly.
	 * Spurious wakeup filtering and timepoint calculations happen inside this function.
	 */
	rc = sdhc_wait_completion(dev_data, xfer_timeout);
	if (rc != 0) {
		LOG_ERR("SDHC command %d transaction timed out or failed (%d)", op, rc);
		k_mutex_unlock(&dev_data->bus_mutex);
		if (data != NULL) {
			sdhc_dma_teardown(dev, cmd, data);
		}
		return rc;
	}

	sdhc_read_response(dev, SDHC_READ_RESP_REG_ALL, dev_data->sdhc_response);

	/*
	 * R2: hardware stores the 128-bit payload right-shifted by 8 across
	 * RR[3:0] with RR[3] as MSW; sd_parse_csd() expects response[0] as
	 * LSW.  Reverse the order and undo the 8-bit shift.  Cast to uint32_t
	 * to force logical right-shift (sdhc_response[] is signed int).
	 */
	if (cmd->response_type == SD_RSP_TYPE_R2) {
		uint32_t w0 = (uint32_t)dev_data->sdhc_response[0];
		uint32_t w1 = (uint32_t)dev_data->sdhc_response[1];
		uint32_t w2 = (uint32_t)dev_data->sdhc_response[2];
		uint32_t w3 = (uint32_t)dev_data->sdhc_response[3];

		cmd->response[3] = (int)((w3 << 8) | (w2 >> 24));
		cmd->response[2] = (int)((w2 << 8) | (w1 >> 24));
		cmd->response[1] = (int)((w1 << 8) | (w0 >> 24));
		cmd->response[0] = (int)(w0 << 8);
	} else {
		cmd->response[0] = dev_data->sdhc_response[0];
		cmd->response[1] = dev_data->sdhc_response[1];
		cmd->response[2] = dev_data->sdhc_response[2];
		cmd->response[3] = dev_data->sdhc_response[3];
	}

	/* DMA finalisation */
	if (data != NULL) {
		sdhc_dma_teardown(dev, cmd, data);
	}

	/* Parse SCR to determine card's CMD23 support */
	if (cmd->opcode == SD_APP_SEND_SCR && data != NULL && data->data != NULL) {
		uint32_t *scr_data = (uint32_t *)data->data;
		uint32_t scr_word0 = sys_be32_to_cpu(scr_data[0]);

		/* SCR cmd_support field is bits 0-1, bit 1 = CMD23 support */
		dev_data->card_supports_cmd23 = (bool)((scr_word0 & 0x2U) != 0U);
		LOG_DBG("SCR cmd_support: card %s CMD23",
			dev_data->card_supports_cmd23 ? "supports" : "does not support");
	}

	k_mutex_unlock(&dev_data->bus_mutex);

	return rc;
}

static int sdhc_mchp_set_io(const struct device *dev, struct sdhc_io *ios)
{
	struct sdhc_mchp_data *const dev_data = dev->data;

	if (ios->clock != 0U) {
		if ((ios->clock < dev_data->props.f_min) || (ios->clock > dev_data->props.f_max)) {
			LOG_ERR("Requested clock %u out of range (%u..%u)", ios->clock,
				dev_data->props.f_min, dev_data->props.f_max);
			return -EINVAL;
		}

		if (!sdhc_set_clock(dev, ios->clock)) {
			LOG_ERR("Failed to set clock to %u Hz", ios->clock);
			return -EIO;
		}
	}

	if (ios->power_mode == SDHC_POWER_OFF) {
		sdhc_clock_disable(dev);
	} else {
		sdhc_clock_enable(dev);
	}

	if (ios->bus_width != 0) {
		if (ios->bus_width == SDHC_BUS_WIDTH4BIT) {
			sdhc_set_buswidth(dev, SDHC_BUS_WIDTH4BIT);
		} else if (ios->bus_width == SDHC_BUS_WIDTH1BIT) {
			sdhc_set_buswidth(dev, SDHC_BUS_WIDTH1BIT);
		} else {
			/* Unsupported bus width */
			LOG_ERR("Failed to set bus width to %u", ios->bus_width);
			return -ENOTSUP;
		}
	}

	switch (ios->timing) {
	case SDHC_TIMING_LEGACY:
		sdhc_set_speedmode(dev, SDHC_SPEED_MODE_NORMAL);
		break;
	case SDHC_TIMING_HS:
		sdhc_set_speedmode(dev, SDHC_SPEED_MODE_HIGH);
		break;
	default:
		/* UHS-I modes are not supported */
		LOG_ERR("Failed to set timing mode to %u", ios->timing);
		return -ENOTSUP;
	}

	return 0;
}

static int sdhc_mchp_get_card_present(const struct device *dev)
{
	return sdhc_is_card_attached(dev);
}

static int sdhc_mchp_card_busy(const struct device *dev)
{
	return (sdhc_is_cmdline_busy(dev) || sdhc_is_dataline_busy(dev));
}

static int sdhc_mchp_reset(const struct device *dev)
{
	const struct sdhc_mchp_cfg *cfg = dev->config;
	struct sdhc_mchp_data *const dev_data = dev->data;

	dev_data->sdhc_obj.error_status = 0U;
	dev_data->sdhc_obj.is_cmd_in_progress = false;
	dev_data->sdhc_obj.is_data_in_progress = false;

	/* Reset cached card capabilities */
	dev_data->card_supports_cmd23 = false;

	sdhc_init_module(dev);

	/*
	 * Allow time for the card to power up and respond to commands after reset
	 * before the host attempts initialization.
	 */
	k_msleep(cfg->power_delay_ms);

	return 0;
}

static int sdhc_mchp_get_host_props(const struct device *dev, struct sdhc_host_props *props)
{
	struct sdhc_mchp_data *const dev_data = dev->data;

	memcpy(props, &dev_data->props, sizeof(*props));

	return 0;
}

static int sdhc_mchp_init(const struct device *dev)
{
	int ret = 0;
	struct sdhc_mchp_data *const dev_data = dev->data;
	const struct sdhc_mchp_cfg *cfg = dev->config;

	if (device_is_ready(cfg->sdhc_clock.clock_dev) != true) {
		LOG_ERR("Clock device not ready");
		return -ENODEV;
	}

	if (CLOCK_CONTROL_STATUS_ON !=
	    clock_control_get_status(cfg->sdhc_clock.clock_dev, cfg->sdhc_clock.mclk_sys)) {
		ret = clock_control_on(cfg->sdhc_clock.clock_dev, cfg->sdhc_clock.mclk_sys);
		if ((ret != 0) && (ret != -EALREADY)) {
			LOG_ERR("Failed to enable MCLK: %d", ret);
			return ret;
		}
	}

	if (CLOCK_CONTROL_STATUS_ON !=
	    clock_control_get_status(cfg->sdhc_clock.clock_dev, cfg->sdhc_clock.gclk_sys)) {
		ret = clock_control_on(cfg->sdhc_clock.clock_dev, cfg->sdhc_clock.gclk_sys);
		if ((ret != 0) && (ret != -EALREADY)) {
			LOG_ERR("Failed to enable GCLK: %d", ret);
			return ret;
		}
	}

	if (CLOCK_CONTROL_STATUS_ON !=
	    clock_control_get_status(cfg->sdhc_clock.clock_dev, cfg->sdhc_clock.gclk_slow_sys)) {
		ret = clock_control_on(cfg->sdhc_clock.clock_dev, cfg->sdhc_clock.gclk_slow_sys);
		if ((ret != 0) && (ret != -EALREADY)) {
			LOG_ERR("Failed to enable slow GCLK: %d", ret);
			return ret;
		}
	}

	if (cfg->pcfg != NULL) {
		ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			LOG_ERR("pinctrl apply failed: %d", ret);
			return ret;
		}
	}

	ret = sdhc_init_props(dev);
	if (ret < 0) {
		LOG_ERR("Failed to read and initialize SDHC host properties: %d", ret);
		return ret;
	}

	sdhc_init_module(dev);
	k_mutex_init(&dev_data->bus_mutex);
	k_sem_init(&dev_data->transfer_done, 0, 1);
	cfg->irq_config_func(dev);

	return 0;
}

static DEVICE_API(sdhc, sdhc_mchp_api) = {
	.request = sdhc_mchp_request,
	.set_io = sdhc_mchp_set_io,
	.get_host_props = sdhc_mchp_get_host_props,
	.get_card_present = sdhc_mchp_get_card_present,
	.card_busy = sdhc_mchp_card_busy,
	.reset = sdhc_mchp_reset,
};

/*
 * Per-instance ADMA2 descriptor table and runtime data.
 *
 * The descriptor table must be aligned to 32 bytes so it never shares a
 * cache line with other data, preventing DMA coherency faults on Cortex-M4.
 */
#define SDHC_MCHP_DATA_DEFN(n)                                                                     \
	static struct sdhc_adma_descr                                                              \
		__aligned(SDHC_MCHP_AGNOSTIC_ALIGN_SIZE) sdhc_mchp_dma_desc_##n[(                  \
			SDHC_MCHP_DESC_TABLE_SIZE_CACHE_ALIGN(n) / SDHC_ADMA2_DESC_LINE_SIZE)];    \
	static struct sdhc_mchp_data sdhc_mchp_data_##n = {                                        \
		.dma_desc_table = sdhc_mchp_dma_desc_##n,                                          \
		.card_supports_cmd23 = false,                                                      \
		.sdhc_obj =                                                                        \
			{                                                                          \
				.error_status = 0U,                                                \
				.is_cmd_in_progress = false,                                       \
				.is_data_in_progress = false,                                      \
				.cmdc_received = false,                                            \
				.trfc_received = false,                                            \
			},                                                                         \
	}

#define SDHC_MCHP_CONFIG_DEFN(n)                                                                   \
	static const struct sdhc_mchp_cfg sdhc_mchp_cfg_##n = {                                    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.regs = DT_INST_REG_ADDR(n),                                                       \
		.sdhc_clock.clock_dev = DEVICE_DT_GET(DT_PARENT(DT_INST_CLOCKS_CTLR(n))),          \
		.sdhc_clock.mclk_sys =                                                             \
			(void *)(uintptr_t)DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, subsystem),        \
		.sdhc_clock.gclk_sys =                                                             \
			(void *)(uintptr_t)DT_INST_CLOCKS_CELL_BY_NAME(n, gclk, subsystem),        \
		.sdhc_clock.gclk_slow_sys =                                                        \
			(void *)(uintptr_t)DT_INST_CLOCKS_CELL_BY_NAME(n, gclk_slow, subsystem),   \
		.power_delay_ms = DT_INST_PROP(n, power_delay_ms),                                 \
		.min_bus_freq = DT_INST_PROP(n, min_bus_freq),                                     \
		.max_bus_freq = DT_INST_PROP(n, max_bus_freq),                                     \
		.auto_cmd12 = DT_INST_PROP(n, auto_cmd12),                                         \
		.auto_cmd23 = DT_INST_PROP(n, auto_cmd23),                                         \
		.bus_width = DT_INST_PROP(n, bus_width),                                           \
		.irq_config_func = sdhc_mchp_irq_config_##n,                                       \
	}

#define MCHP_SDHC_IRQ_CONNECT(n, m)                                                                \
	do {                                                                                       \
		((sdhc_registers_t *)(uintptr_t)DT_INST_REG_ADDR(n))->SDHC_NISTR = SDHC_NISTR_Msk; \
		((sdhc_registers_t *)(uintptr_t)DT_INST_REG_ADDR(n))->SDHC_EISTR = SDHC_EISTR_Msk; \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, m, irq), DT_INST_IRQ_BY_IDX(n, m, priority),     \
			    sdhc_mchp_isr, DEVICE_DT_INST_GET(n), 0);                              \
		irq_enable(DT_INST_IRQ_BY_IDX(n, m, irq));                                         \
	} while (false)

#define SDHC_MCHP_IRQ_HANDLER(n)                                                                   \
	static void sdhc_mchp_irq_config_##n(const struct device *dev)                             \
	{                                                                                          \
		MCHP_SDHC_IRQ_CONNECT(n, 0);                                                       \
	}

#define SDHC_MCHP_DEVICE_INIT(n)                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static void sdhc_mchp_irq_config_##n(const struct device *dev);                            \
	SDHC_MCHP_DATA_DEFN(n);                                                                    \
	SDHC_MCHP_CONFIG_DEFN(n);                                                                  \
	DEVICE_DT_INST_DEFINE(n, sdhc_mchp_init, NULL, &sdhc_mchp_data_##n, &sdhc_mchp_cfg_##n,    \
			      POST_KERNEL, CONFIG_SDHC_INIT_PRIORITY, &sdhc_mchp_api);             \
	SDHC_MCHP_IRQ_HANDLER(n)

DT_INST_FOREACH_STATUS_OKAY(SDHC_MCHP_DEVICE_INIT)
