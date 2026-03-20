/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_nvmctrl_g2

#include <soc.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/sys/clock.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>
#include <zephyr/drivers/flash/mchp_flash.h>

LOG_MODULE_REGISTER(flash_mchp_nvmctrl_g2);

#define SOC_NV_FLASH_COMPAT(node_id)                                                               \
	COND_CODE_1(DT_NODE_HAS_COMPAT(node_id, soc_nv_flash), (node_id), ())

#define SOC_NV_FLASH_NODE(n)             DT_INST_FOREACH_CHILD_STATUS_OKAY(n, SOC_NV_FLASH_COMPAT)
#define SOC_NV_FLASH_BASE_ADDR(n)        DT_REG_ADDR(SOC_NV_FLASH_NODE(n))
#define SOC_NV_FLASH_SIZE(n)             DT_REG_SIZE(SOC_NV_FLASH_NODE(n))
#define SOC_NV_FLASH_WRITE_BLOCK_SIZE(n) DT_PROP(SOC_NV_FLASH_NODE(n), write_block_size)
#define SOC_NV_FLASH_ERASE_BLOCK_SIZE(n) DT_PROP(SOC_NV_FLASH_NODE(n), erase_block_size)

#define SOC_NV_DATA_FLASH_START_ADDR(n) DT_PROP(SOC_NV_FLASH_NODE(n), data_flash_start_addr)
#define SOC_NV_USER_ROW_START_ADDR(n)   DT_PROP(SOC_NV_FLASH_NODE(n), user_row_start_addr)

#define SOC_NV_FLASH_UNSUPPORTED_REGION_COUNT(n)                                                   \
	DT_PROP_LEN(SOC_NV_FLASH_NODE(n), unsupported_region) / 2
#define SOC_NV_FLASH_UNSUPPORTED_REGIONS(n) DT_PROP(SOC_NV_FLASH_NODE(n), unsupported_region)

#define TIMEOUT_VALUE_US 100000U

#define MCHP_FLASH_G2_WAIT_STATE_0_FREQ_MHZ 21000000U
#define MCHP_FLASH_WAITSTATE_0              0U
#define MCHP_FLASH_G2_WAIT_STATE_1_FREQ_MHZ 42000000U
#define MCHP_FLASH_WAITSTATE_1              1U
#define MCHP_FLASH_G2_WAIT_STATE_2_FREQ_MHZ 48000000U
#define MCHP_FLASH_WAITSTATE_2              2U

#define DELAY_US 2U

#define IS_MCHP_FLASH_G2_ALIGNED(value, alignment) (((value) & ((alignment) - 1)) == 0U)

enum flash_kind {
	MAIN_FLASH = 0U,
	DATA_FLASH = 1U,
	USER_ROW = 2U,
};

enum flash_operation {
	OPERATION_READ = 0U,
	OPERATION_WRITE = 1U,
	OPERATION_ERASE = 2U,
};

struct flash_mchp_g2_clock {
	const struct device *clock_dev;
	clock_control_subsys_t mclk_sys;
};

struct nvmctrl_mchp_g2_config {
	nvmctrl_registers_t *regs;
	struct flash_mchp_g2_clock flash_clock;

	struct flash_parameters parameters;
#if CONFIG_FLASH_PAGE_LAYOUT
	struct flash_pages_layout page_layout;
#endif
	uint32_t base_addr;
	uint32_t size;
	uint32_t write_block_size;
	uint32_t erase_block_size;
	uint32_t data_flash_start_addr;
	uint32_t user_row_start_addr;
	uint32_t unimplemented_region_count;
	uint32_t *unimplemented_regions;
};

struct nvmctrl_mchp_g2_data {
	struct k_sem sem_lock;
};

static inline void nvmctrl_intflag_ready_wait(nvmctrl_registers_t *regs)
{
	if (WAIT_FOR(((regs->NVMCTRL_INTFLAG & NVMCTRL_INTFLAG_READY_Msk) ==
		      NVMCTRL_INTFLAG_READY_Msk),
		     TIMEOUT_VALUE_US, k_busy_wait(DELAY_US)) == false) {
		LOG_WRN("NVMCTRL_STATUS_READY wait timed out");
	}
}

static enum flash_kind
get_flash_kind_from_address(const struct nvmctrl_mchp_g2_config *const config,
			    uint32_t absolute_address)
{
	if (absolute_address >= config->user_row_start_addr) {
		return USER_ROW;
	} else if (absolute_address >= config->data_flash_start_addr) {
		return DATA_FLASH;
	}
	return MAIN_FLASH;
}
static void init_nvmctrl_reg_defaults(nvmctrl_registers_t *regs, uint8_t flash_wait_states)
{
	regs->NVMCTRL_CTRLB =
		(NVMCTRL_CTRLB_CACHEDIS_CACHE_DF_DIS_MAIN_EN |
		 NVMCTRL_CTRLB_READMODE_NO_MISS_PENALTY | NVMCTRL_CTRLB_SLEEPPRM_WAKEONACCESS |
		 NVMCTRL_CTRLB_RWS(flash_wait_states) | NVMCTRL_CTRLB_MANW_Msk);
	regs->NVMCTRL_INTENCLR = NVMCTRL_INTENCLR_Msk;
}

static uint16_t get_err_status(nvmctrl_registers_t *regs)
{
	uint16_t status =
		(regs->NVMCTRL_STATUS &
		 (NVMCTRL_STATUS_NVME_Msk | NVMCTRL_STATUS_LOCKE_Msk | NVMCTRL_STATUS_PROGE_Msk));

	regs->NVMCTRL_STATUS |= status;

	return status;
}

static int execute_cmd(const struct device *dev, uint16_t cmd)
{
	const struct nvmctrl_mchp_g2_config *const config = dev->config;
	uint16_t nvm_error;
	nvmctrl_registers_t *regs = config->regs;

	nvmctrl_intflag_ready_wait(regs);
	regs->NVMCTRL_CTRLA = cmd | NVMCTRL_CTRLA_CMDEX_KEY;
	nvmctrl_intflag_ready_wait(regs);

	nvm_error = get_err_status(regs);
	if (nvm_error != 0) {
		if ((nvm_error & NVMCTRL_STATUS_NVME_Msk) != 0) {
			LOG_ERR("At least one error has been registered from the NVM Controller");
		}
		if ((nvm_error & NVMCTRL_STATUS_LOCKE_Msk) != 0) {
			LOG_ERR("Programming attempt on at least one locked lock region has "
				"happened");
		}
		if ((nvm_error & NVMCTRL_STATUS_PROGE_Msk) != 0) {
			LOG_ERR("An invalid command and/or a bad keyword was/were written in the "
				"NVM Command register");
		}
		return -EIO;
	}

	return 0;
}

static int set_addr_execute_cmd(const struct device *dev, uint32_t address, uint16_t cmd)
{
	const struct nvmctrl_mchp_g2_config *const config = dev->config;

	config->regs->NVMCTRL_ADDR = address >> 1U;

	return execute_cmd(dev, cmd);
}

static int get_nvmctrl_commands(enum flash_kind f_kind, enum flash_operation op, uint16_t *command)
{
	int ret = -EINVAL;

	switch (op) {
	case OPERATION_WRITE:
		if (f_kind == MAIN_FLASH) {
			*command = NVMCTRL_CTRLA_CMD_WP;
			ret = 0;
		} else if (f_kind == DATA_FLASH) {
			*command = NVMCTRL_CTRLA_CMD_DFWP;
			ret = 0;
		} else if (f_kind == USER_ROW) {
			*command = NVMCTRL_CTRLA_CMD_WAP;
			ret = 0;
		} else {
			LOG_ERR("Unsupported Flash Type");
		}
		break;
	case OPERATION_ERASE:
		if (f_kind == MAIN_FLASH) {
			*command = NVMCTRL_CTRLA_CMD_ER;
			ret = 0;
		} else if (f_kind == DATA_FLASH) {
			*command = NVMCTRL_CTRLA_CMD_DFER;
			ret = 0;
		} else if (f_kind == USER_ROW) {
			*command = NVMCTRL_CTRLA_CMD_EAR;
			ret = 0;
		} else {
			LOG_ERR("Unsupported Flash Type");
		}
		break;
	default:
		LOG_ERR("Unsupported Operation");
	}
	return ret;
}

static int validate_flash_parameters(const struct nvmctrl_mchp_g2_config *const config,
				     off_t offset, size_t size, enum flash_operation op)
{
	if (offset > config->size || size > config->size || (offset + size) > config->size) {
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
		if (!(IS_MCHP_FLASH_G2_ALIGNED(offset, config->write_block_size))) {
			LOG_WRN("WRITE: Offset should be multiples of write block size."
				" Off: %#08x",
				(uint32_t)offset);
			return -EINVAL;
		}
		if (!(IS_MCHP_FLASH_G2_ALIGNED(size, config->write_block_size))) {
			LOG_WRN("WRITE: Size should be multiples of write block size."
				" Sz: %#08x",
				size);
			return -EINVAL;
		}
		break;
	case OPERATION_ERASE:
		if (size < config->erase_block_size) {
			LOG_WRN("ERASE: Cannot erase less than a size of an erase block."
				" Sz: %#08x",
				size);
			return -EINVAL;
		}
		if (!IS_MCHP_FLASH_G2_ALIGNED(size, config->erase_block_size)) {
			LOG_WRN("ERASE: Size should be multiples of erase block size."
				" Sz: %#08x",
				size);
			return -EINVAL;
		}
		if (!IS_MCHP_FLASH_G2_ALIGNED(offset, config->erase_block_size)) {
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

static int flash_mchp_read(const struct device *dev, off_t offset, void *data_buff,
			   size_t no_of_bytes)
{
	const struct nvmctrl_mchp_g2_config *const config = dev->config;
	struct nvmctrl_mchp_g2_data *data = dev->data;
	int ret;

	if (no_of_bytes == 0) {
		return 0;
	}

	ret = validate_flash_parameters(config, offset, no_of_bytes, OPERATION_READ);

	if (ret != 0) {
		return ret;
	}

	k_sem_take(&data->sem_lock, K_FOREVER);
	memcpy(data_buff, (uint8_t *)(config->base_addr + offset), no_of_bytes);
	k_sem_give(&data->sem_lock);

	return 0;
}

static int flash_mchp_write(const struct device *dev, off_t offset, const void *data_buff,
			    size_t no_of_bytes)
{
	const struct nvmctrl_mchp_g2_config *const config = dev->config;
	struct nvmctrl_mchp_g2_data *data = dev->data;
	int nvm_error = -EIO;
	uint16_t command;
	const uint8_t *src = (const uint8_t *)data_buff;
	uint32_t *dst = (uint32_t *)(config->base_addr + offset);

	if (no_of_bytes == 0) {
		return 0;
	}

	nvm_error = validate_flash_parameters(config, offset, no_of_bytes, OPERATION_WRITE);
	if (nvm_error != 0) {
		return nvm_error;
	}

	nvm_error = get_nvmctrl_commands(
		get_flash_kind_from_address(config, (offset + config->base_addr)), OPERATION_WRITE,
		&command);
	if (nvm_error != 0) {
		return -EIO;
	}

	while (no_of_bytes > 0) {
		LOG_DBG("Writing block at address %#08x, Size: %#08x",
			(uint32_t)(config->base_addr + offset), no_of_bytes);

		k_sem_take(&data->sem_lock, K_FOREVER);
		for (uint32_t i = 0U; i < (config->write_block_size / 4U); i++) {
			uint32_t value;

			memcpy(&value, src, sizeof(value));
			*dst = value;
			dst++;
			src += 4;
		}
		nvm_error = set_addr_execute_cmd(dev, (config->base_addr + offset), command);
		k_sem_give(&data->sem_lock);
		if (nvm_error != 0) {
			break;
		}

		no_of_bytes -= config->write_block_size;
		offset += config->write_block_size;
	}

	return nvm_error;
}

static int flash_mchp_erase(const struct device *dev, off_t offset, size_t no_of_bytes)
{
	const struct nvmctrl_mchp_g2_config *const config = dev->config;
	struct nvmctrl_mchp_g2_data *data = dev->data;
	int nvm_error = -EIO;
	uint16_t command;

	if (no_of_bytes == 0) {
		return 0;
	}
	nvm_error = validate_flash_parameters(config, offset, no_of_bytes, OPERATION_ERASE);
	if (nvm_error != 0) {
		return nvm_error;
	}
	nvm_error = get_nvmctrl_commands(
		get_flash_kind_from_address(config, (offset + config->base_addr)), OPERATION_ERASE,
		&command);
	if (nvm_error != 0) {
		return -EIO;
	}

	while (no_of_bytes > 0U) {
		LOG_DBG("Erasing block at address %#08x", (uint32_t)(config->base_addr + offset));
		k_sem_take(&data->sem_lock, K_FOREVER);
		nvm_error = set_addr_execute_cmd(dev, (config->base_addr + offset), command);
		k_sem_give(&data->sem_lock);
		if (nvm_error != 0) {
			printk("Err..");
			break;
		}
		no_of_bytes -= config->erase_block_size;
		offset += config->erase_block_size;
	}

	return nvm_error;
}

#if CONFIG_FLASH_EX_OP_ENABLED

static int flash_mchp_ex_op(const struct device *dev, uint16_t opr_code, const uintptr_t offset,
			    void *out)
{
	const struct nvmctrl_mchp_g2_config *const config = dev->config;
	struct nvmctrl_mchp_g2_data *data = dev->data;
	int ret;
	uint32_t address;

	k_sem_take(&data->sem_lock, K_FOREVER);
	switch (opr_code) {
	/* Lock a specific region of flash memory */
	case FLASH_EX_OP_REGION_LOCK:
		address = config->base_addr + *(uint32_t *)offset;
		ret = set_addr_execute_cmd(dev, address, NVMCTRL_CTRLA_CMD_LR);
		break;
	/* Unlock a specific region of flash memory */
	case FLASH_EX_OP_REGION_UNLOCK:
		address = config->base_addr + *(uint32_t *)offset;
		ret = set_addr_execute_cmd(dev, address, NVMCTRL_CTRLA_CMD_UR);
		break;
	/* Sets the Power Reduction mode of flash memory */
	case FLASH_EX_OP_SET_PWR_REDUCTION_MODE:
		ret = execute_cmd(dev, NVMCTRL_CTRLA_CMD_SPRM);
		break;
	/* Clears the Power Reduction mode of flash memory */
	case FLASH_EX_OP_CLR_PWR_REDUCTION_MODE:
		ret = execute_cmd(dev, NVMCTRL_CTRLA_CMD_CPRM);
		break;
	/* Clears the flash page buffer */
	case FLASH_EX_OP_CLR_PAGE_BUFFER:
		ret = execute_cmd(dev, NVMCTRL_CTRLA_CMD_PBC);
		break; /* Set Security Bit */
	case FLASH_EX_OP_SET_SECURITY_BIT:
		ret = execute_cmd(dev, NVMCTRL_CTRLA_CMD_SSB);
		break;
	/* Invalidates all cache lines */
	case FLASH_EX_OP_INVALIDATE_CACHE_LINES:
		ret = execute_cmd(dev, NVMCTRL_CTRLA_CMD_INVALL);
		break;
	/* Set Chip Erase Hard Lock */
	case FLASH_EX_OP_SET_CHIP_ERASE_HARDLOCK:
		ret = execute_cmd(dev, NVMCTRL_CTRLA_CMD_SCEHL);
		break;
	default:
		ret = -EINVAL;
	}
	k_sem_give(&data->sem_lock);

	return ret;
}

#endif /* CONFIG_FLASH_EX_OP_ENABLED */

static int nvmctrl_init(const struct device *nvmctrl_dev)
{
	const struct nvmctrl_mchp_g2_config *config = nvmctrl_dev->config;
	struct nvmctrl_mchp_g2_data *data = nvmctrl_dev->data;
	uint32_t operating_freq;
	uint8_t flash_wait_states;
	int ret;

	/* Turn-On Global clock for NVMCTRL */
	ret = clock_control_on(config->flash_clock.clock_dev, config->flash_clock.mclk_sys);
	if ((ret != 0) && (ret != -EALREADY)) {
		LOG_ERR("Failed to enable the GCLK for NVMCTRL: %d", ret);
		return ret;
	}

	ret = clock_control_get_rate(config->flash_clock.clock_dev, config->flash_clock.mclk_sys,
				     &operating_freq);
	if (ret != 0) {
		/* If clock freq could not be retrieved, set the waitstate to max */
		flash_wait_states = MCHP_FLASH_WAITSTATE_2;
	} else if (operating_freq <= MCHP_FLASH_G2_WAIT_STATE_0_FREQ_MHZ) {
		flash_wait_states = MCHP_FLASH_WAITSTATE_0;
	} else if (operating_freq <= MCHP_FLASH_G2_WAIT_STATE_1_FREQ_MHZ) {
		flash_wait_states = MCHP_FLASH_WAITSTATE_1;
	} else {
		flash_wait_states = MCHP_FLASH_WAITSTATE_2;
	}
	init_nvmctrl_reg_defaults(config->regs, flash_wait_states);
	k_sem_init(&data->sem_lock, 1, 1);

	return 0;
}

#if CONFIG_FLASH_PAGE_LAYOUT
#define FLASH_MCHP_G2_CONFIG_PAGE_LAYOUT(n)                                                        \
	.page_layout.pages_count = SOC_NV_FLASH_SIZE(n) / SOC_NV_FLASH_ERASE_BLOCK_SIZE(n),        \
	.page_layout.pages_size = SOC_NV_FLASH_ERASE_BLOCK_SIZE(n),
#else
#define FLASH_MCHP_G2_CONFIG_PAGE_LAYOUT(n)
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

#if CONFIG_FLASH_PAGE_LAYOUT
void flash_mchp_page_layout(const struct device *dev, const struct flash_pages_layout **layout,
			    size_t *layout_size)
{
	const struct nvmctrl_mchp_g2_config *const config = dev->config;

	*layout = &config->page_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_parameters *flash_mchp_get_parameters(const struct device *dev)
{
	const struct nvmctrl_mchp_g2_config *const config = dev->config;

	return &config->parameters;
}

static DEVICE_API(flash, flash_mchp_g2_driver_api) = {
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

#define MCHP_NVMCTRL_G2_INIT(n)                                                                    \
	static struct nvmctrl_mchp_g2_data nvmctrl_data_##n;                                       \
	static uint32_t dt_unimplemented_regions##n[] = SOC_NV_FLASH_UNSUPPORTED_REGIONS(n);       \
	static const struct nvmctrl_mchp_g2_config nvmctrl_config_##n = {                          \
		.regs = (nvmctrl_registers_t *)DT_INST_REG_ADDR(n),                                \
		.flash_clock.clock_dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, clock_parent)),          \
		.flash_clock.mclk_sys = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, subsystem)), \
		.parameters =                                                                      \
			{                                                                          \
				.write_block_size = SOC_NV_FLASH_WRITE_BLOCK_SIZE(n),              \
				.erase_value = 0xff,                                               \
			},                                                                         \
		.base_addr = SOC_NV_FLASH_BASE_ADDR(n),                                            \
		.size = SOC_NV_FLASH_SIZE(n),                                                      \
		.write_block_size = SOC_NV_FLASH_WRITE_BLOCK_SIZE(n),                              \
		.erase_block_size = SOC_NV_FLASH_ERASE_BLOCK_SIZE(n),                              \
		.data_flash_start_addr = SOC_NV_DATA_FLASH_START_ADDR(n),                          \
		.user_row_start_addr = SOC_NV_USER_ROW_START_ADDR(n),                              \
		.unimplemented_region_count = SOC_NV_FLASH_UNSUPPORTED_REGION_COUNT(n),            \
		.unimplemented_regions = dt_unimplemented_regions##n,                              \
		FLASH_MCHP_G2_CONFIG_PAGE_LAYOUT(n)};                                              \
	DEVICE_DT_INST_DEFINE(n, nvmctrl_init, NULL, &nvmctrl_data_##n, &nvmctrl_config_##n,       \
			      POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY, &flash_mchp_g2_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MCHP_NVMCTRL_G2_INIT)
