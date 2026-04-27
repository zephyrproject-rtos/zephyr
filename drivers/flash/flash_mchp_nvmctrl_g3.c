/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_nvmctrl_g3

#include <soc.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/sys/clock.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>
#include <zephyr/drivers/flash/mchp_flash.h>

LOG_MODULE_REGISTER(flash_mchp_nvmctrl_g3);

#define SOC_NV_FLASH_COMPAT(node_id)                                                               \
	COND_CODE_1(DT_NODE_HAS_COMPAT(node_id, soc_nv_flash), (node_id), ())

#define SOC_NV_FLASH_NODE(n)                                                                       \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, SOC_NV_FLASH_COMPAT)
#define SOC_NV_FLASH_BASE_ADDR(n)            DT_REG_ADDR(SOC_NV_FLASH_NODE(n))
#define SOC_NV_FLASH_SIZE(n)                 DT_REG_SIZE(SOC_NV_FLASH_NODE(n))
#define SOC_NV_FLASH_WRITE_BLOCK_SIZE(n)     DT_PROP(SOC_NV_FLASH_NODE(n), write_block_size)
#define SOC_NV_FLASH_WRITE_BLOCK_SIZE_QDW(n) DT_PROP(SOC_NV_FLASH_NODE(n), write_block_size_quad_dw)
#define SOC_NV_FLASH_WRITE_BLOCK_SIZE_ROW(n) DT_PROP(SOC_NV_FLASH_NODE(n), write_block_size_row)
#define SOC_NV_FLASH_ERASE_BLOCK_SIZE(n)     DT_PROP(SOC_NV_FLASH_NODE(n), erase_block_size)

#define SOC_NV_FLASH_UNIMPLEMENTED_REGION_COUNT(n)                                                 \
	DT_PROP_LEN(SOC_NV_FLASH_NODE(n), unimplemented_region) / 2
#define SOC_NV_FLASH_UNIMPLEMENTED_REGIONS(n) DT_PROP(SOC_NV_FLASH_NODE(n), unimplemented_region)

#define TIMEOUT_VALUE_US     100000U
#define TIMEOUT_DONE_WAIT_MS 50U
#define DELAY_US             2U

#define IS_MCHP_FLASH_G3_ALIGNED(value, alignment) (((value) & ((alignment) - 1)) == 0)

enum fcw_operation_mode {
	PROGRAM_ERASE_OPERATION = 0x7U,
	PAGE_ERASE_OPERATION = 0x4U,
	ROW_PROGRAM_OPERATION = 0x3U,
	QUAD_DOUBLE_WORD_PROGRAM_OPERATION = 0x2U,
	SINGLE_DOUBLE_WORD_PROGRAM_OPERATION = 0x1U,
	NO_OPERATION = 0x0U,
};

enum fcw_unlock_key {
	FCW_UNLOCK_WRKEY = 0x91C32C01U,
	FCW_UNLOCK_SWAPKEY = 0x91C32C02U,
	FCW_UNLOCK_CFGKEY = 0x91C32C04U,
};

enum flash_operation {
	OPERATION_READ = 0U,
	OPERATION_WRITE = 1U,
	OPERATION_ERASE = 2U,
};

struct flash_mchp_g3_clock {
	clock_control_subsys_t mclk_ahb; /* AHB clock control subsystem */
	clock_control_subsys_t mclk_apb; /* APB clock control subsystem */
};

struct nvmctrl_config {
	const struct device *clock_dev;                    /* Clock driver device */
	void (*irq_config_func)(const struct device *dev); /* IRQ Function ptr */
	struct fcw_config {
		fcw_registers_t *regs;            /* Pointer to Flash peripheral registers */
		struct flash_mchp_g3_clock clock; /* Flash clock control configuration */
	} fcw;

	struct fcr_config {
		fcr_registers_t *regs;            /* Pointer to Flash peripheral registers */
		struct flash_mchp_g3_clock clock; /* Flash clock control configuration */
	} fcr;

	struct flash_parameters parameters;
#if CONFIG_FLASH_PAGE_LAYOUT
	struct flash_pages_layout page_layout;
#endif
	uint32_t base_addr;
	uint32_t size;
	uint32_t write_block_size;
	uint32_t write_block_size_qdw;
	uint32_t write_block_size_row;
	uint32_t erase_block_size;
	uint32_t unimplemented_region_count;
	uint32_t *unimplemented_regions;
};

struct nvmctrl_data {
	struct k_sem fcw_sem_lock;  /* Semaphore used for locking mechanism */
	struct k_sem done_flag_sem; /* Semaphore used for synching flash write done */
	uint32_t interrupt_status;  /* Flash controller interrupt status flags */
};

static inline void fcw_ready_wait(fcw_registers_t *regs)
{
	if (WAIT_FOR(((regs->FCW_STATUS & FCW_STATUS_BUSY_Msk) == 0), TIMEOUT_VALUE_US,
		     k_busy_wait(DELAY_US)) == false) {
		LOG_WRN("FCW_STATUS_BUSY wait timed out");
	}
}

static inline void fcr_ready_wait(fcr_registers_t *regs)
{
	if (WAIT_FOR(((regs->FCR_STATUS & FCR_STATUS_PRM_Msk) == 0), TIMEOUT_VALUE_US,
		     k_busy_wait(DELAY_US)) == false) {
		LOG_WRN("FCR_STATUS_PRM wait timed out");
	}
}

static void enable_fcw_interrupts(fcw_registers_t *regs)
{
	regs->FCW_INTENSET |=
		(FCW_INTENSET_KEYERR_Msk | FCW_INTENSET_CFGERR_Msk | FCW_INTENSET_FIFOERR_Msk |
		 FCW_INTENSET_BUSERR_Msk | FCW_INTENSET_WPERR_Msk | FCW_INTENSET_OPERR_Msk |
		 FCW_INTENSET_SECERR_Msk | FCW_INTENSET_BORERR_Msk | FCW_INTENSET_WRERR_Msk |
		 FCW_INTENSET_DONE_Msk);
}

static void disable_fcr_interrupts(fcr_registers_t *regs)
{
	regs->FCR_INTENCLR = FCR_INTENCLR_Msk;
}

static void clear_fcw_error_status(fcw_registers_t *regs, uint32_t error_mask)
{
	regs->FCW_INTFLAG |= error_mask;
}

static void clear_fcr_error_status(fcr_registers_t *regs, uint32_t error_mask)
{
	regs->FCR_INTFLAG |= error_mask;
}

static uint32_t get_fcw_interrupt_status_flags(fcw_registers_t *fcw_regs)
{
	uint32_t fcw_status =
		(fcw_regs->FCW_INTFLAG &
		 (FCW_INTFLAG_KEYERR_Msk | FCW_INTFLAG_CFGERR_Msk | FCW_INTFLAG_FIFOERR_Msk |
		  FCW_INTFLAG_BUSERR_Msk | FCW_INTFLAG_WPERR_Msk | FCW_INTFLAG_OPERR_Msk |
		  FCW_INTFLAG_SECERR_Msk | FCW_INTFLAG_BORERR_Msk | FCW_INTFLAG_WRERR_Msk |
		  FCW_INTFLAG_DONE_Msk));

	return fcw_status;
}

static int validate_flash_parameters(const struct nvmctrl_config *const config, off_t offset,
				     size_t size, enum flash_operation op)
{
	if ((offset < 0) || (offset > config->size) || (size > config->size) ||
	    (offset > (config->size - size))) {
		LOG_WRN("Offset+Size is beyond flash addressable range."
			" Off: %#08x, Sz: %#08x, MaxSz: %#08x",
			(uint32_t)offset, size, config->size);
		return -EINVAL;
	}

	for (int i = 0; i < config->unimplemented_region_count; i++) {
		uint32_t unimp_region_start = config->unimplemented_regions[(i * 2)];
		uint32_t unimp_region_end = config->unimplemented_regions[((i * 2) + 1)];

		if ((config->base_addr + offset + size) > unimp_region_start &&
		    (config->base_addr + offset + size) <= unimp_region_end) {
			LOG_WRN("Offset+Size lies in unimplemented flash range."
				" Off: %#08x, Sz: %#08x",
				(uint32_t)offset, size);
			return -EINVAL;
		}
	}

	switch (op) {
	case OPERATION_READ:
		break;
	case OPERATION_WRITE:
		if (!(IS_MCHP_FLASH_G3_ALIGNED(offset, config->write_block_size))) {
			LOG_WRN("WRITE: Offset should be multiples of write-block size."
				" Off: %#08x",
				(uint32_t)offset);
			return -EINVAL;
		}
		if (!(IS_MCHP_FLASH_G3_ALIGNED(size, config->write_block_size))) {
			LOG_WRN("WRITE: Size should be multiples of write-block size."
				" Sz: %#08x",
				size);
			return -EINVAL;
		}
		break;
	case OPERATION_ERASE:
		if (size < config->erase_block_size) {
			LOG_WRN("ERASE: Cannot erase less than the size of an erase-block."
				" Sz: %#08x",
				size);
			return -EINVAL;
		}
		if (!IS_MCHP_FLASH_G3_ALIGNED(size, config->erase_block_size)) {
			LOG_WRN("ERASE: Size should be multiples of erase-block size."
				" Sz: %#08x",
				size);
			return -EINVAL;
		}
		if (!IS_MCHP_FLASH_G3_ALIGNED(offset, config->erase_block_size)) {
			LOG_WRN("ERASE: Offset should be multiples of erase-block size."
				" Sz: %#08x",
				size);
			return -EINVAL;
		}
		break;
	default:
		LOG_ERR("Unsupported operation");
		return -EINVAL;
	}

	return 0;
}

static int exec_flash_operation(fcw_registers_t *regs, struct nvmctrl_data *nvmctr_data,
				uint32_t address, enum fcw_operation_mode operation)
{
	int wait_err;

	fcw_ready_wait(regs);
	regs->FCW_MUTEX = (FCW_MUTEX_LOCK(1) | FCW_MUTEX_OWNER(1));
	regs->FCW_ADDR = address;

	clear_fcw_error_status(regs, FCW_INTFLAG_Msk);
	nvmctr_data->interrupt_status = 0;

	regs->FCW_KEY = (uint32_t)FCW_UNLOCK_WRKEY;
	k_sem_init(&nvmctr_data->done_flag_sem, 0, 1);
	regs->FCW_CTRLA = FCW_CTRLA_PREPG_Msk | FCW_CTRLA_NVMOP(operation);

	wait_err = k_sem_take(&nvmctr_data->done_flag_sem, K_MSEC(TIMEOUT_DONE_WAIT_MS));
	if (wait_err != 0) {
		LOG_ERR("Error waiting for Done flag");
		return -EIO;
	}

	fcw_ready_wait(regs);
	regs->FCW_MUTEX = (FCW_MUTEX_LOCK(0) | FCW_MUTEX_OWNER(0));

	nvmctr_data->interrupt_status &= ~FCW_INTENSET_DONE_Msk;

	if ((nvmctr_data->interrupt_status & FCW_INTFLAG_KEYERR_Msk) != 0) {
		LOG_ERR("Key Error Flag Bit is set");
	}
	if ((nvmctr_data->interrupt_status & FCW_INTFLAG_CFGERR_Msk) != 0) {
		LOG_ERR("Configuration Error Flag Bit is set");
	}
	if ((nvmctr_data->interrupt_status & FCW_INTFLAG_FIFOERR_Msk) != 0) {
		LOG_ERR("FIFO Underrun During Row Write Flag Bit is set");
	}
	if ((nvmctr_data->interrupt_status & FCW_INTFLAG_BUSERR_Msk) != 0) {
		LOG_ERR("AHB Bus Error During Row Write Flag Bit is set");
	}
	if ((nvmctr_data->interrupt_status & FCW_INTFLAG_WPERR_Msk) != 0) {
		LOG_ERR("Write Protection Error Flag Bit is set");
	}
	if ((nvmctr_data->interrupt_status & FCW_INTFLAG_OPERR_Msk) != 0) {
		LOG_ERR("NVMOP Error Flag Bit is set");
	}
	if ((nvmctr_data->interrupt_status & FCW_INTFLAG_SECERR_Msk) != 0) {
		LOG_ERR("Security Violation Error Bit is set");
	}
	if ((nvmctr_data->interrupt_status & FCW_INTFLAG_BORERR_Msk) != 0) {
		LOG_ERR("Reset or Brown Out Detect Error Flag Bit is set");
	}
	if ((nvmctr_data->interrupt_status & FCW_INTFLAG_WRERR_Msk) != 0) {
		LOG_ERR("Write Error Flag Bit is set");
	}

	return ((nvmctr_data->interrupt_status == 0) ? 0 : -EIO);
}

static int flash_mchp_read(const struct device *dev, off_t offset, void *data_buff,
			   size_t no_of_bytes)
{
	const struct nvmctrl_config *const config = dev->config;
	struct nvmctrl_data *data = dev->data;
	int ret = validate_flash_parameters(config, offset, no_of_bytes, OPERATION_READ);

	if (ret != 0) {
		LOG_ERR("Flash Parameters validation failed");
		return ret;
	}
	if (data_buff == NULL) {
		LOG_ERR("Data Buffer is NULL");
		return -EINVAL;
	}

	k_sem_take(&data->fcw_sem_lock, K_FOREVER);
	(void)memcpy(data_buff, (uint8_t *)(config->base_addr + offset), no_of_bytes);
	k_sem_give(&data->fcw_sem_lock);

	return 0;
}

static int flash_mchp_write(const struct device *dev, off_t offset, const void *data_buff,
			    size_t no_of_bytes)
{
	const struct nvmctrl_config *const config = dev->config;
	struct nvmctrl_data *data = dev->data;
	fcw_registers_t *regs = config->fcw.regs;
	int nvm_error = -EIO;
	const uint8_t *src_data_buff_read_ptr = (const uint8_t *)data_buff;

	if (no_of_bytes == 0) {
		return 0;
	}
	nvm_error = validate_flash_parameters(config, offset, no_of_bytes, OPERATION_WRITE);
	if (nvm_error != 0) {
		LOG_ERR("Flash Parameters validation failed");
		return nvm_error;
	}
	if (data_buff == NULL) {
		LOG_ERR("Data Buffer is NULL");
		return -EINVAL;
	}
	k_sem_take(&data->fcw_sem_lock, K_FOREVER);
	while (no_of_bytes > 0) {
		fcw_ready_wait(regs);
		LOG_DBG("Number of bytes yet to be written %u", no_of_bytes);
		if (no_of_bytes >= config->write_block_size_row) {
			regs->FCW_SRCADDR = (uint32_t)src_data_buff_read_ptr;
			LOG_DBG("Writing row-block at address %#08x",
				(uint32_t)(config->base_addr + offset));
			nvm_error = exec_flash_operation(regs, data, (config->base_addr + offset),
							 ROW_PROGRAM_OPERATION);
			if (nvm_error != 0) {
				break;
			}
			no_of_bytes -= config->write_block_size_row;
			offset += config->write_block_size_row;
			src_data_buff_read_ptr += config->write_block_size_row;
		} else if (no_of_bytes >= config->write_block_size_qdw) {
			for (uint32_t i = 0; i < (config->write_block_size_qdw / sizeof(uint32_t));
			     i++) {
				regs->FCW_DATA[i] =
					*((uint32_t *)((uint32_t)src_data_buff_read_ptr +
						       (i * sizeof(uint32_t))));
			}
			LOG_DBG("Writing quad-double-word at address %#08x",
				(uint32_t)(config->base_addr + offset));
			nvm_error = exec_flash_operation(regs, data, (config->base_addr + offset),
							 QUAD_DOUBLE_WORD_PROGRAM_OPERATION);
			if (nvm_error != 0) {
				break;
			}
			no_of_bytes -= config->write_block_size_qdw;
			offset += config->write_block_size_qdw;
			src_data_buff_read_ptr += config->write_block_size_qdw;
		} else if (no_of_bytes >= config->write_block_size) {
			for (uint32_t i = 0; i < (config->write_block_size / sizeof(uint32_t));
			     i++) {
				regs->FCW_DATA[i] =
					*((uint32_t *)((uint32_t)src_data_buff_read_ptr +
						       (i * sizeof(uint32_t))));
			}
			LOG_DBG("Writing double-word at address %#08x",
				(uint32_t)(config->base_addr + offset));
			nvm_error = exec_flash_operation(regs, data,
							 (uint32_t)(config->base_addr + offset),
							 SINGLE_DOUBLE_WORD_PROGRAM_OPERATION);
			if (nvm_error != 0) {
				break;
			}
			no_of_bytes -= config->write_block_size;
			offset += config->write_block_size;
			src_data_buff_read_ptr += config->write_block_size;
		} else {
			LOG_ERR("Alignment error - no_of_bytes is not aligned to write-block size "
				"%#08x",
				no_of_bytes);
			nvm_error = -EIO;
			break;
		}
	}
	k_sem_give(&data->fcw_sem_lock);

	return nvm_error;
}

static int flash_mchp_erase(const struct device *dev, off_t offset, size_t no_of_bytes)
{
	const struct nvmctrl_config *const config = dev->config;
	struct nvmctrl_data *data = dev->data;
	fcw_registers_t *regs = config->fcw.regs;
	int nvm_error = -EIO;

	if (no_of_bytes == 0) {
		return 0;
	}
	nvm_error = validate_flash_parameters(config, offset, no_of_bytes, OPERATION_ERASE);
	if (nvm_error != 0) {
		return nvm_error;
	}

	while (no_of_bytes > 0U) {
		LOG_DBG("Erasing block at address %#08x", (uint32_t)(config->base_addr + offset));
		k_sem_take(&data->fcw_sem_lock, K_FOREVER);
		nvm_error = exec_flash_operation(regs, data, (uint32_t)(config->base_addr + offset),
						 PAGE_ERASE_OPERATION);
		k_sem_give(&data->fcw_sem_lock);
		if (nvm_error != 0) {
			LOG_ERR("Error erasing the block. Err code: %d", nvm_error);
			break;
		}
		no_of_bytes -= config->erase_block_size;
		offset += config->erase_block_size;
	}

	return nvm_error;
}

#if CONFIG_FLASH_EX_OP_ENABLED
static int flash_mchp_ex_op(const struct device *dev, uint16_t opr_code, const uintptr_t exop_info,
			    void *out)
{
	ARG_UNUSED(out);
	const struct nvmctrl_config *const config = dev->config;
	struct nvmctrl_data *data = dev->data;
	fcw_registers_t *regs = config->fcw.regs;
	struct pfm_exop *pfm_exop_data;
	struct bfm_exop *bfm_exop_data;
	uint8_t region;
	int nvm_err = -EINVAL;

	k_sem_take(&data->fcw_sem_lock, K_FOREVER);
	switch (opr_code) {
	case FLASH_EX_OP_PFM_REGION_WRITE_PROTECT_ENABLE:
		pfm_exop_data = (struct pfm_exop *)exop_info;
		fcw_ready_wait(regs);
		regs->FCW_KEY = FCW_UNLOCK_CFGKEY;
		region = (uint8_t)(pfm_exop_data->region);
		regs->FCW_PWP[region] = (FCW_PWP_PWPBASE(pfm_exop_data->base_addr) |
					 FCW_PWP_PWPSIZE(pfm_exop_data->region_size) |
					 FCW_PWP_PWPMIR(pfm_exop_data->mirror_enable));
		regs->FCW_PWP[region] |= FCW_PWP_PWPEN_Msk;
		nvm_err = 0;
		break;
	case FLASH_EX_OP_PFM_REGION_WRITE_PROTECT_DISABLE:
		pfm_exop_data = (struct pfm_exop *)(exop_info);
		fcw_ready_wait(regs);
		regs->FCW_KEY = FCW_UNLOCK_CFGKEY;
		region = (uint8_t)(pfm_exop_data->region);
		regs->FCW_PWP[region] = (FCW_PWP_PWPBASE(pfm_exop_data->base_addr) |
					 FCW_PWP_PWPSIZE(pfm_exop_data->region_size) |
					 FCW_PWP_PWPMIR(pfm_exop_data->mirror_enable));
		regs->FCW_PWP[region] |= FCW_PWP_PWPLOCK_Msk;
		nvm_err = 0;
		break;
	case FLASH_EX_OP_PFM_REGION_WRITE_PROTECT_LOCK:
		pfm_exop_data = (struct pfm_exop *)(exop_info);
		fcw_ready_wait(regs);
		regs->FCW_KEY = FCW_UNLOCK_CFGKEY;
		region = (uint8_t)(pfm_exop_data->region);
		regs->FCW_PWP[region] = (FCW_PWP_PWPBASE(pfm_exop_data->base_addr) |
					 FCW_PWP_PWPSIZE(pfm_exop_data->region_size) |
					 FCW_PWP_PWPMIR(pfm_exop_data->mirror_enable));
		regs->FCW_PWP[region] &= ~FCW_PWP_PWPEN_Msk;
		nvm_err = 0;
		break;
	case FLASH_EX_OP_BFM_WRITE_PROTECT_ENABLE:
		bfm_exop_data = (struct bfm_exop *)(exop_info);
		fcw_ready_wait(regs);
		regs->FCW_KEY = FCW_UNLOCK_CFGKEY;
		if (bfm_exop_data->boot_bank == BOOT_FLASH_BANK_1) {
			regs->FCW_LBWP |= bfm_exop_data->wp_page;
		} else {
			regs->FCW_UBWP |= bfm_exop_data->wp_page;
		}
		nvm_err = 0;
		break;
	case FLASH_EX_OP_BFM_WRITE_PROTECT_DISABLE:
		bfm_exop_data = (struct bfm_exop *)(exop_info);
		fcw_ready_wait(regs);
		regs->FCW_KEY = FCW_UNLOCK_CFGKEY;
		if (bfm_exop_data->boot_bank == BOOT_FLASH_BANK_1) {
			regs->FCW_LBWP &= ~bfm_exop_data->wp_page;
		} else {
			regs->FCW_UBWP &= ~bfm_exop_data->wp_page;
		}
		nvm_err = 0;
		break;
	case FLASH_EX_OP_BFM_WRITE_PROTECT_LOCK:
		bfm_exop_data = (struct bfm_exop *)(exop_info);
		fcw_ready_wait(regs);
		regs->FCW_KEY = FCW_UNLOCK_CFGKEY;
		if (bfm_exop_data->boot_bank == BOOT_FLASH_BANK_1) {
			regs->FCW_LBWP |= FCW_LBWP_LBWPLOCK_Msk;
		} else {
			regs->FCW_UBWP |= FCW_UBWP_UBWPLOCK_Msk;
		}
		nvm_err = 0;
		break;
	default:
		LOG_ERR("Unsupported operation");
		nvm_err = -EINVAL;
	}
	k_sem_give(&data->fcw_sem_lock);

	return nvm_err;
}
#endif /* CONFIG_FLASH_EX_OP_ENABLED */

static void nvmctrl_isr(const struct device *nvmctrl_dev)
{
	const struct nvmctrl_config *const config = nvmctrl_dev->config;
	struct nvmctrl_data *data = nvmctrl_dev->data;

	data->interrupt_status = get_fcw_interrupt_status_flags(config->fcw.regs);
	clear_fcw_error_status(config->fcw.regs, data->interrupt_status);
	if ((data->interrupt_status & FCW_INTFLAG_DONE_Msk) == FCW_INTFLAG_DONE_Msk) {
		k_sem_give(&data->done_flag_sem);
	}
}

static int nvmctrl_init(const struct device *nvmctrl_dev)
{
	const struct nvmctrl_config *const config = nvmctrl_dev->config;
	struct nvmctrl_data *data = nvmctrl_dev->data;
	int nvm_err;

	nvm_err = clock_control_on(config->clock_dev, config->fcw.clock.mclk_ahb);
	if ((nvm_err != 0) && (nvm_err != -EALREADY)) {
		LOG_ERR("Failed to enable FCW AHB Clock: %d", nvm_err);
		return nvm_err;
	}
	nvm_err = clock_control_on(config->clock_dev, config->fcw.clock.mclk_apb);
	if ((nvm_err != 0) && (nvm_err != -EALREADY)) {
		LOG_ERR("Failed to enable FCW APB Clock: %d", nvm_err);
		return nvm_err;
	}
	nvm_err = clock_control_on(config->clock_dev, config->fcr.clock.mclk_ahb);
	if ((nvm_err != 0) && (nvm_err != -EALREADY)) {
		LOG_ERR("Failed to enable FCR AHB Clock: %d", nvm_err);
		return nvm_err;
	}
	nvm_err = clock_control_on(config->clock_dev, config->fcr.clock.mclk_apb);
	if ((nvm_err != 0) && (nvm_err != -EALREADY)) {
		LOG_ERR("Failed to enable FCR APB Clock: %d", nvm_err);
		return nvm_err;
	}

	k_sem_init(&data->fcw_sem_lock, 1, 1);
	k_sem_init(&data->done_flag_sem, 0, 1);
	fcw_ready_wait(config->fcw.regs);
	fcr_ready_wait(config->fcr.regs);
	config->irq_config_func(nvmctrl_dev);
	enable_fcw_interrupts(config->fcw.regs);
	disable_fcr_interrupts(config->fcr.regs);
	clear_fcw_error_status(config->fcw.regs, FCW_INTFLAG_Msk);
	clear_fcr_error_status(config->fcr.regs, FCR_INTFLAG_Msk);

	nvm_err = exec_flash_operation(config->fcw.regs, data, config->fcw.regs->FCW_ADDR,
				       NO_OPERATION);
	if (nvm_err != 0) {
		LOG_ERR("Failed to Initialize NVMCTRL. Err: %d", nvm_err);
	}

	return nvm_err;
}

#if CONFIG_FLASH_PAGE_LAYOUT
#define FLASH_MCHP_G3_CONFIG_PAGE_LAYOUT(n)                                                        \
	.page_layout.pages_count = SOC_NV_FLASH_SIZE(n) / SOC_NV_FLASH_ERASE_BLOCK_SIZE(n),        \
	.page_layout.pages_size = SOC_NV_FLASH_ERASE_BLOCK_SIZE(n),
#else
#define FLASH_MCHP_G3_CONFIG_PAGE_LAYOUT(n)
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

#if CONFIG_FLASH_PAGE_LAYOUT
void flash_mchp_page_layout(const struct device *dev, const struct flash_pages_layout **layout,
			    size_t *layout_size)
{
	const struct nvmctrl_config *const config = dev->config;

	*layout = &config->page_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_parameters *flash_mchp_get_parameters(const struct device *dev)
{
	const struct nvmctrl_config *const config = dev->config;

	return &config->parameters;
}

static DEVICE_API(flash, flash_mchp_g3_driver_api) = {
	.read = flash_mchp_read,
	.write = flash_mchp_write,
	.erase = flash_mchp_erase,
	.get_parameters = flash_mchp_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_mchp_page_layout,
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
#ifdef CONFIG_FLASH_EX_OP_ENABLED
	.ex_op = flash_mchp_ex_op,
#endif /*CONFIG_FLASH_EX_OP_ENABLED*/
};
#define MCHP_NVMCTRL_G3_INIT(n)                                                                    \
	static void nvmctrl_g3_irq_config##n(const struct device *dev);                            \
	static void nvmctrl_g3_irq_config##n(const struct device *dev)                             \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, fcw, irq),                                      \
			    DT_INST_IRQ_BY_NAME(n, fcw, priority), nvmctrl_isr,                    \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQ_BY_NAME(n, fcw, irq));                                      \
	}                                                                                          \
                                                                                                   \
	static uint32_t dt_unimplemented_regions##n[] = SOC_NV_FLASH_UNIMPLEMENTED_REGIONS(n);     \
	static struct nvmctrl_data nvmctrl_g3_data##n;                                             \
                                                                                                   \
	static const struct nvmctrl_config nvmctrl_g3_config##n = {                                \
		.clock_dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, clock_parent)),                      \
		.irq_config_func = nvmctrl_g3_irq_config##n,                                       \
                                                                                                   \
		.fcw.regs = (fcw_registers_t *)DT_INST_REG_ADDR_BY_NAME(n, fcw),                   \
		.fcw.clock.mclk_ahb =                                                              \
			(void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk_fcw_ahb, subsystem)),         \
		.fcw.clock.mclk_apb =                                                              \
			(void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk_fcw_apb, subsystem)),         \
                                                                                                   \
		.fcr.regs = (fcr_registers_t *)DT_INST_REG_ADDR_BY_NAME(n, fcr),                   \
		.fcr.clock.mclk_ahb =                                                              \
			(void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk_fcr_ahb, subsystem)),         \
		.fcr.clock.mclk_apb =                                                              \
			(void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk_fcr_apb, subsystem)),         \
                                                                                                   \
		.parameters =                                                                      \
			{                                                                          \
				.write_block_size = SOC_NV_FLASH_WRITE_BLOCK_SIZE(n),              \
				.erase_value = 0xff,                                               \
			},                                                                         \
		.base_addr = SOC_NV_FLASH_BASE_ADDR(n),                                            \
		.size = SOC_NV_FLASH_SIZE(n),                                                      \
		.write_block_size = SOC_NV_FLASH_WRITE_BLOCK_SIZE(n),                              \
		.erase_block_size = SOC_NV_FLASH_ERASE_BLOCK_SIZE(n),                              \
		.write_block_size_qdw = SOC_NV_FLASH_WRITE_BLOCK_SIZE_QDW(n),                      \
		.write_block_size_row = SOC_NV_FLASH_WRITE_BLOCK_SIZE_ROW(n),                      \
		.unimplemented_region_count = SOC_NV_FLASH_UNIMPLEMENTED_REGION_COUNT(n),          \
		.unimplemented_regions = dt_unimplemented_regions##n,                              \
		FLASH_MCHP_G3_CONFIG_PAGE_LAYOUT(n)};                                              \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, nvmctrl_init, NULL, &nvmctrl_g3_data##n, &nvmctrl_g3_config##n,   \
			      POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY, &flash_mchp_g3_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MCHP_NVMCTRL_G3_INIT)
