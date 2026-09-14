/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Flash driver for the FCW write controller and FCR read controller.
 *
 * Program flash is a single panel, so every erase and program stalls the CPU
 * on its next instruction fetch until the operation finishes. Running the
 * wait loop from SRAM under irq_lock would behave the same, because any
 * interrupt handler stalls on its own fetch, so the driver simply polls.
 */

#define DT_DRV_COMPAT microchip_nvmctrl_g4

#include <string.h>

#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(flash_mchp_nvmctrl_g4, CONFIG_FLASH_LOG_LEVEL);

#define FLASH_MCHP_NVMCTRL_G4_TIMEOUT_US 100000

#define FLASH_MCHP_NVMCTRL_G4_KEY_WRKEY (FCW_KEY_FEATURE_WRKEY | FCW_KEY_CODE(0x91C32C))

struct flash_mchp_nvmctrl_g4_config {
	fcw_registers_t *fcw;
	fcr_registers_t *fcr;
	uint32_t base;
	uint32_t size;
	uint32_t erase_block_size;
	struct flash_parameters parameters;
	struct flash_pages_layout page_layout;
};

struct flash_mchp_nvmctrl_g4_data {
	struct k_mutex lock;
};

static const struct flash_parameters *
flash_mchp_nvmctrl_g4_get_parameters(const struct device *dev)
{
	const struct flash_mchp_nvmctrl_g4_config *cfg = dev->config;

	return &cfg->parameters;
}

static int flash_mchp_nvmctrl_g4_read(const struct device *dev, off_t offset, void *data,
				      size_t len)
{
	const struct flash_mchp_nvmctrl_g4_config *cfg = dev->config;
	size_t uoffset;

	if (len == 0) {
		return 0;
	}

	if (offset < 0) {
		return -EINVAL;
	}

	uoffset = (size_t)offset;
	if (uoffset + len < uoffset || uoffset + len > cfg->size) {
		return -EINVAL;
	}

	memcpy(data, (const void *)(cfg->base + uoffset), len);

	return 0;
}

/*
 * One erase or program: MUTEX, ADDR, DATA for a program, KEY, then CTRLOP.
 * Writing CTRLOP starts the operation and clears KEY, so every operation
 * needs its own key. data is NULL for an erase and four words for a quad
 * write.
 */
static int flash_mchp_nvmctrl_g4_op(const struct device *dev, uint32_t nvmop, uint32_t addr,
				    const uint32_t data[4])
{
	const struct flash_mchp_nvmctrl_g4_config *cfg = dev->config;
	struct flash_mchp_nvmctrl_g4_data *drv_data = dev->data;
	fcw_registers_t *fcw = cfg->fcw;
	uint32_t intflag;
	int ret = 0;

	k_mutex_lock(&drv_data->lock, K_FOREVER);

	fcw->FCW_MUTEX = FCW_MUTEX_LOCK_LOCK | FCW_MUTEX_OWNER_SYSTEM;

	fcw->FCW_ADDR = addr;
	if (data != NULL) {
		fcw->FCW_DATA[0] = data[0];
		fcw->FCW_DATA[1] = data[1];
		fcw->FCW_DATA[2] = data[2];
		fcw->FCW_DATA[3] = data[3];
	}
	fcw->FCW_KEY = FLASH_MCHP_NVMCTRL_G4_KEY_WRKEY;
	fcw->FCW_CTRLOP = FCW_CTRLOP_NVMOP(nvmop) | FCW_CTRLOP_PREPG_ENABLE;

	if (!WAIT_FOR((fcw->FCW_INTFLAG & FCW_INTFLAG_DONE_Msk) != 0,
		      FLASH_MCHP_NVMCTRL_G4_TIMEOUT_US, k_busy_wait(1))) {
		LOG_ERR("op 0x%x at 0x%08x timed out: STATUS 0x%08x INTFLAG 0x%08x", nvmop, addr,
			fcw->FCW_STATUS, fcw->FCW_INTFLAG);
		fcw->FCW_MUTEX = 0;
		k_mutex_unlock(&drv_data->lock);
		return -ETIMEDOUT;
	}

	intflag = fcw->FCW_INTFLAG;
	fcw->FCW_INTFLAG = intflag;

	fcw->FCW_MUTEX = 0;

	if (intflag & FCW_INTFLAG_WPERR_Msk) {
		ret = -EACCES;
	} else if (intflag & (FCW_INTFLAG_KEYERR_Msk | FCW_INTFLAG_CFGERR_Msk |
			      FCW_INTFLAG_OPERR_Msk | FCW_INTFLAG_BUSERR_Msk |
			      FCW_INTFLAG_FIFOERR_Msk | FCW_INTFLAG_WRERR_Msk |
			      FCW_INTFLAG_RSTERR_Msk | FCW_INTFLAG_HTDPGM_Msk)) {
		ret = -EIO;
	}

	if (ret != 0) {
		LOG_ERR("op 0x%x at 0x%08x failed: STATUS 0x%08x INTFLAG 0x%08x", nvmop, addr,
			fcw->FCW_STATUS, intflag);
		k_mutex_unlock(&drv_data->lock);
		return ret;
	}

	/* Nothing invalidates the FCR cache when flash changes underneath it. */
	cfg->fcr->FCR_CTRLB |= FCR_CTRLB_CHEINV_INV;

	k_mutex_unlock(&drv_data->lock);

	return 0;
}

static int flash_mchp_nvmctrl_g4_write(const struct device *dev, off_t offset, const void *data,
				       size_t len)
{
	const struct flash_mchp_nvmctrl_g4_config *cfg = dev->config;
	const uint8_t *src = data;
	uint32_t write_block_size = cfg->parameters.write_block_size;
	size_t uoffset;
	int ret;

	if (len == 0) {
		return 0;
	}

	if (offset < 0) {
		return -EINVAL;
	}
	uoffset = (size_t)offset;

	if ((uoffset % write_block_size) != 0 || (len % write_block_size) != 0) {
		return -EINVAL;
	}

	if (uoffset + len < uoffset || uoffset + len > cfg->size) {
		return -EINVAL;
	}

	for (size_t off = 0; off < len; off += write_block_size) {
		uint32_t words[4];

		memcpy(words, src + off, sizeof(words));

		ret = flash_mchp_nvmctrl_g4_op(dev, FCW_CTRLOP_NVMOP_QUADWRITE_Val,
					       cfg->base + uoffset + off, words);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

static int flash_mchp_nvmctrl_g4_erase(const struct device *dev, off_t offset, size_t size)
{
	const struct flash_mchp_nvmctrl_g4_config *cfg = dev->config;
	size_t uoffset;
	int ret;

	if (size == 0) {
		return 0;
	}

	if (offset < 0) {
		return -EINVAL;
	}
	uoffset = (size_t)offset;

	if ((uoffset % cfg->erase_block_size) != 0 || (size % cfg->erase_block_size) != 0) {
		return -EINVAL;
	}

	if (uoffset + size < uoffset || uoffset + size > cfg->size) {
		return -EINVAL;
	}

	for (size_t off = uoffset; off < uoffset + size; off += cfg->erase_block_size) {
		ret = flash_mchp_nvmctrl_g4_op(dev, FCW_CTRLOP_NVMOP_PAGEERASE_Val,
					       cfg->base + off, NULL);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void flash_mchp_nvmctrl_g4_page_layout(const struct device *dev,
					      const struct flash_pages_layout **layout,
					      size_t *layout_size)
{
	const struct flash_mchp_nvmctrl_g4_config *cfg = dev->config;

	*layout = &cfg->page_layout;
	*layout_size = 1;
}
#endif

static DEVICE_API(flash, flash_mchp_nvmctrl_g4_api) = {
	.read = flash_mchp_nvmctrl_g4_read,
	.write = flash_mchp_nvmctrl_g4_write,
	.erase = flash_mchp_nvmctrl_g4_erase,
	.get_parameters = flash_mchp_nvmctrl_g4_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_mchp_nvmctrl_g4_page_layout,
#endif
};

static int flash_mchp_nvmctrl_g4_init(const struct device *dev)
{
	const struct flash_mchp_nvmctrl_g4_config *cfg = dev->config;
	struct flash_mchp_nvmctrl_g4_data *drv_data = dev->data;
	fcw_registers_t *fcw = cfg->fcw;
	uint32_t ecc_mode;

	k_mutex_init(&drv_data->lock);

	if (!WAIT_FOR((fcw->FCW_STATUS & FCW_STATUS_BUSY_Msk) == 0,
		      FLASH_MCHP_NVMCTRL_G4_TIMEOUT_US, k_busy_wait(1))) {
		LOG_ERR("still busy at init: STATUS 0x%08x", fcw->FCW_STATUS);
		return -ETIMEDOUT;
	}

	/* RSTERR survives a reset that interrupted an operation. */
	fcw->FCW_INTFLAG = FCW_INTFLAG_Msk;

	/*
	 * Only the DISABLE mode reads never-programmed flash without an ECC
	 * check, and storage layers read erased flash while looking for space.
	 */
	ecc_mode = FIELD_GET(FCR_ECCCTRL_ECCCTL_Msk, cfg->fcr->FCR_ECCCTRL);
	if (ecc_mode != FCR_ECCCTRL_ECCCTL_DISABLE_Val) {
		LOG_WRN("ECC mode %u checks reads: never-programmed flash may raise ECC errors",
			ecc_mode);
	}

	return 0;
}

#define FLASH_MCHP_NVMCTRL_G4_FLASH_NODE DT_NODELABEL(program_flash)
#define FLASH_MCHP_NVMCTRL_G4_BASE       DT_REG_ADDR(FLASH_MCHP_NVMCTRL_G4_FLASH_NODE)
#define FLASH_MCHP_NVMCTRL_G4_SIZE       DT_REG_SIZE(FLASH_MCHP_NVMCTRL_G4_FLASH_NODE)
#define FLASH_MCHP_NVMCTRL_G4_WBS        DT_PROP(FLASH_MCHP_NVMCTRL_G4_FLASH_NODE, write_block_size)
#define FLASH_MCHP_NVMCTRL_G4_EBS        DT_PROP(FLASH_MCHP_NVMCTRL_G4_FLASH_NODE, erase_block_size)

#define FLASH_MCHP_NVMCTRL_G4_DEFINE(n)                                                            \
	BUILD_ASSERT(FLASH_MCHP_NVMCTRL_G4_WBS == 16, "the write block size must be 16");          \
	BUILD_ASSERT(FLASH_MCHP_NVMCTRL_G4_EBS == 4096, "the erase block size must be 4096");      \
	static const struct flash_mchp_nvmctrl_g4_config flash_mchp_nvmctrl_g4_config_##n = {      \
		.fcw = (fcw_registers_t *)DT_INST_REG_ADDR_BY_NAME(n, fcw),                        \
		.fcr = (fcr_registers_t *)DT_INST_REG_ADDR_BY_NAME(n, fcr),                        \
		.base = FLASH_MCHP_NVMCTRL_G4_BASE,                                                \
		.size = FLASH_MCHP_NVMCTRL_G4_SIZE,                                                \
		.erase_block_size = FLASH_MCHP_NVMCTRL_G4_EBS,                                     \
		.parameters = {                                                                    \
			.write_block_size = FLASH_MCHP_NVMCTRL_G4_WBS,                             \
			.erase_value = 0xff,                                                       \
		},                                                                                 \
		.page_layout = {                                                                   \
			.pages_count = FLASH_MCHP_NVMCTRL_G4_SIZE / FLASH_MCHP_NVMCTRL_G4_EBS,     \
			.pages_size = FLASH_MCHP_NVMCTRL_G4_EBS,                                   \
		},                                                                                 \
	};                                                                                         \
	static struct flash_mchp_nvmctrl_g4_data flash_mchp_nvmctrl_g4_data_##n;                   \
	DEVICE_DT_INST_DEFINE(n, flash_mchp_nvmctrl_g4_init, NULL,                                 \
			      &flash_mchp_nvmctrl_g4_data_##n, &flash_mchp_nvmctrl_g4_config_##n,  \
			      PRE_KERNEL_1, CONFIG_FLASH_INIT_PRIORITY,                            \
			      &flash_mchp_nvmctrl_g4_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_MCHP_NVMCTRL_G4_DEFINE)
