/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_ibecc

#include <zephyr.h>
#include <device.h>
#include <drivers/pcie/pcie.h>

#include <drivers/edac.h>
#include "ibecc.h"

/**
 * In the driver 64 bit registers are used and not all of then at the
 * moment may be correctly logged.
 */
#include <logging/log.h>
LOG_MODULE_REGISTER(edac_ibecc, CONFIG_EDAC_LOG_LEVEL);

#define DEVICE_NODE DT_NODELABEL(ibecc)
#define PCI_HOST_BRIDGE PCIE_BDF(0, 0, 0)

struct ibecc_data {
	mem_addr_t mchbar;
	edac_notify_callback_f cb;
	uint32_t error_type;

	/* Error count */
	unsigned int errors_cor;
	unsigned int errors_uc;
};

static void ibecc_write_reg64(const struct device *dev,
			      uint16_t reg, uint64_t value)
{
	struct ibecc_data *data = dev->data;
	mem_addr_t reg_addr = data->mchbar + reg;

	sys_write64(value, reg_addr);
}

static uint64_t ibecc_read_reg64(const struct device *dev, uint16_t reg)
{
	struct ibecc_data *data = dev->data;
	mem_addr_t reg_addr = data->mchbar + reg;

	return sys_read64(reg_addr);
}

#if defined(CONFIG_EDAC_ERROR_INJECT)
static void ibecc_write_reg32(const struct device *dev,
			      uint16_t reg, uint32_t value)
{
	struct ibecc_data *data = dev->data;
	mem_addr_t reg_addr = data->mchbar + reg;

	sys_write32(value, reg_addr);
}
#endif

static uint32_t ibecc_read_reg32(const struct device *dev, uint16_t reg)
{
	struct ibecc_data *data = dev->data;
	mem_addr_t reg_addr = data->mchbar + reg;

	return sys_read32(reg_addr);
}

static bool ibecc_enabled(const pcie_bdf_t bdf)
{
	return !!(pcie_conf_read(bdf, CAPID0_C_REG) & CAPID0_C_IBECC_ENABLED);
}

static void ibecc_errcmd_setup(const pcie_bdf_t bdf, bool enable)
{
	uint32_t errcmd;

	errcmd = pcie_conf_read(bdf, ERRCMD_REG);

	if (enable) {
		errcmd |= (ERRCMD_IBECC_COR | ERRCMD_IBECC_UC) << 16;
	} else {
		errcmd &= ~(ERRCMD_IBECC_COR | ERRCMD_IBECC_UC) << 16;
	}

	pcie_conf_write(bdf, ERRCMD_REG, errcmd);
}

static void ibecc_errsts_clear(const pcie_bdf_t bdf)
{
	uint32_t errsts;

	errsts = pcie_conf_read(bdf, ERRSTS_REG);

	if (!(errsts & (ERRSTS_IBECC_COR | ERRSTS_IBECC_UC))) {
		return;
	}

	pcie_conf_write(bdf, ERRSTS_REG, errsts);
}

static const char *get_ddr_type(uint8_t type)
{
	switch (type) {
	case 0:
		return "DDR4";
	case 3:
		return "LPDDR4";
	default:
		return "Unknown";
	}
}

static const char *get_dimm_width(uint8_t type)
{
	switch (type) {
	case 0:
		return "X8";
	case 1:
		return "X16";
	case 2:
		return "X32";
	default:
		return "Unknown";
	}
}

static void mchbar_regs_dump(const struct device *dev)
{
	uint32_t mad_inter_chan, chan_hash;

	/* Memory configuration */

	chan_hash = ibecc_read_reg32(dev, CHANNEL_HASH);
	LOG_DBG("Channel hash %x", chan_hash);

	mad_inter_chan = ibecc_read_reg32(dev, MAD_INTER_CHAN);
	LOG_DBG("DDR memory type %s",
	       get_ddr_type(INTER_CHAN_DDR_TYPE(mad_inter_chan)));

	for (int ch = 0; ch < DRAM_MAX_CHANNELS; ch++) {
		uint32_t intra_ch = ibecc_read_reg32(dev, MAD_INTRA_CH(ch));
		uint32_t dimm_ch = ibecc_read_reg32(dev, MAD_DIMM_CH(ch));
		uint64_t l_size = DIMM_L_SIZE(dimm_ch);
		uint64_t s_size = DIMM_S_SIZE(dimm_ch);
		uint8_t l_map = DIMM_L_MAP(intra_ch);

		LOG_DBG("channel %d: l_size 0x%llx s_size 0x%llx l_map %d\n",
			ch, l_size, s_size, l_map);

		for (int d = 0; d < DRAM_MAX_DIMMS; d++) {
			uint64_t size;
			const char *type;

			if (d ^ l_map) {
				type = get_dimm_width(DIMM_S_WIDTH(dimm_ch));
				size = s_size;
			} else {
				type = get_dimm_width(DIMM_L_WIDTH(dimm_ch));
				size = l_size;
			}

			if (!size) {
				continue;
			}

			LOG_DBG("Channel %d DIMM %d size %llu GiB width %s",
				ch, d, size >> 30, type);
		}
	}
}

static void parse_ecclog(const struct device *dev, const uint64_t ecclog,
			 struct ibecc_error *error_data)
{
	struct ibecc_data *data = dev->data;

	if (!ecclog) {
		return;
	}

	error_data->type = ECC_ERROR_ERRTYPE(ecclog);
	error_data->address = ECC_ERROR_ERRADD(ecclog);
	error_data->syndrome = ECC_ERROR_ERRSYND(ecclog);

	if (ecclog & ECC_ERROR_MERRSTS) {
		data->errors_uc++;
	}

	if (ecclog & ECC_ERROR_CERRSTS) {
		data->errors_cor++;
	}
}

#if defined(CONFIG_EDAC_ERROR_INJECT)
static int inject_set_param1(const struct device *dev, uint64_t addr)
{
	if (addr & ~INJ_ADDR_BASE_MASK) {
		return -EINVAL;
	}

	ibecc_write_reg64(dev, IBECC_INJ_ADDR_BASE, addr);

	return 0;
}

static uint64_t inject_get_param1(const struct device *dev)
{
	return ibecc_read_reg64(dev, IBECC_INJ_ADDR_BASE);
}

static int inject_set_param2(const struct device *dev, uint64_t mask)
{
	if (mask & ~INJ_ADDR_BASE_MASK_MASK) {
		return -EINVAL;
	}

	ibecc_write_reg64(dev, IBECC_INJ_ADDR_MASK, mask);

	return 0;
}

static uint64_t inject_get_param2(const struct device *dev)
{
	return ibecc_read_reg64(dev, IBECC_INJ_ADDR_MASK);
}

static int inject_set_error_type(const struct device *dev,
				 uint32_t error_type)
{
	struct ibecc_data *data = dev->data;

	data->error_type = error_type;

	return 0;
}

static uint32_t inject_get_error_type(const struct device *dev)
{
	struct ibecc_data *data = dev->data;

	return data->error_type;
}

static int inject_error_trigger(const struct device *dev)
{
	struct ibecc_data *data = dev->data;
	uint32_t ctrl = 0;

	switch (data->error_type) {
	case EDAC_ERROR_TYPE_DRAM_COR:
		ctrl |= INJ_CTRL_COR;
		break;
	case EDAC_ERROR_TYPE_DRAM_UC:
		ctrl |= INJ_CTRL_UC;
		break;
	default:
		/* This would clear error injection */
		break;
	}

	ibecc_write_reg32(dev, IBECC_INJ_ADDR_CTRL, ctrl);

	return 0;
}
#endif /* CONFIG_EDAC_ERROR_INJECT */

static uint64_t ecc_error_log_get(const struct device *dev)
{
	return ibecc_read_reg64(dev, IBECC_ECC_ERROR_LOG);
}

static void ecc_error_log_clear(const struct device *dev)
{
	/* Clear all error bits */
	ibecc_write_reg64(dev, IBECC_ECC_ERROR_LOG,
			  ECC_ERROR_MERRSTS | ECC_ERROR_CERRSTS);
}

static uint64_t parity_error_log_get(const struct device *dev)
{
	return ibecc_read_reg64(dev, IBECC_PARITY_ERROR_LOG);
}

static void parity_error_log_clear(const struct device *dev)
{
	ibecc_write_reg64(dev, IBECC_PARITY_ERROR_LOG, PARITY_ERROR_ERRSTS);
}

static unsigned int errors_cor_get(const struct device *dev)
{
	struct ibecc_data *data = dev->data;

	return data->errors_cor;
}

static unsigned int errors_uc_get(const struct device *dev)
{
	struct ibecc_data *data = dev->data;

	return data->errors_uc;
}

static int notify_callback_set(const struct device *dev,
			       edac_notify_callback_f cb)
{
	struct ibecc_data *data = dev->data;
	int key = irq_lock();

	data->cb = cb;
	irq_unlock(key);

	return 0;
}

static const struct edac_driver_api api = {
#if defined(CONFIG_EDAC_ERROR_INJECT)
	/* Error Injection functions */
	.inject_set_param1 = inject_set_param1,
	.inject_get_param1 = inject_get_param1,
	.inject_set_param2 = inject_set_param2,
	.inject_get_param2 = inject_get_param2,
	.inject_set_error_type = inject_set_error_type,
	.inject_get_error_type = inject_get_error_type,
	.inject_error_trigger = inject_error_trigger,
#endif /* CONFIG_EDAC_ERROR_INJECT */

	/* Error reporting & clearing functions */
	.ecc_error_log_get = ecc_error_log_get,
	.ecc_error_log_clear = ecc_error_log_clear,
	.parity_error_log_get = parity_error_log_get,
	.parity_error_log_clear = parity_error_log_clear,

	/* Get error stats */
	.errors_cor_get = errors_cor_get,
	.errors_uc_get = errors_uc_get,

	/* Notification callback set */
	.notify_cb_set = notify_callback_set,
};

int edac_ibecc_init(const struct device *dev)
{
	const pcie_bdf_t bdf = PCI_HOST_BRIDGE;
	struct ibecc_data *data = dev->data;
	uint32_t tolud;
	uint64_t touud, tom, mchbar;

	LOG_INF("EDAC IBECC initialization");

	if (!pcie_probe(bdf, PCIE_ID(PCI_VENDOR_ID_INTEL,
				     PCI_DEVICE_ID_SKU7)) &&
	    !pcie_probe(bdf, PCIE_ID(PCI_VENDOR_ID_INTEL,
				     PCI_DEVICE_ID_SKU12))) {
		LOG_ERR("PCI Probe failed");
		return -ENODEV;
	}

	if (!ibecc_enabled(bdf)) {
		LOG_ERR("IBECC is not enabled");
		return -ENODEV;
	}

	mchbar = pcie_conf_read(bdf, MCHBAR_REG);
	mchbar |= (uint64_t)pcie_conf_read(bdf, MCHBAR_REG + 1) << 32;

	/* Check that MCHBAR is enabled */
	if (!(mchbar & MCHBAR_ENABLE)) {
		LOG_ERR("MCHBAR is not enabled");
		return -ENODEV;
	}

	mchbar &= MCHBAR_MASK;

	/* workaround for 32 bit read */
	touud = pcie_conf_read(bdf, TOUUD_REG);
	touud |= (uint64_t)pcie_conf_read(bdf, TOUUD_REG + 1) << 32;
	touud &= TOUUD_MASK;

	/* workaround for 32 bit read */
	tom = pcie_conf_read(bdf, TOM_REG);
	tom |= (uint64_t)pcie_conf_read(bdf, TOM_REG + 1) << 32;
	tom &= TOM_MASK;

	tolud = pcie_conf_read(bdf, TOLUD_REG) & TOLUD_MASK;

	device_map(&data->mchbar, mchbar, MCH_SIZE, K_MEM_CACHE_NONE);

	LOG_DBG("MCHBAR\t%llx", mchbar);
	LOG_DBG("TOUUD\t%llx", touud);
	LOG_DBG("TOM\t%llx", tom);
	LOG_DBG("TOLUD\t%x", tolud);

	mchbar_regs_dump(dev);

	/* Enable Host Bridge generated SERR event */
	ibecc_errcmd_setup(bdf, true);

	return 0;
}

static struct ibecc_data ibecc_data;

DEVICE_DT_DEFINE(DEVICE_NODE, &edac_ibecc_init,
		 NULL, &ibecc_data, NULL, POST_KERNEL,
		 CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &api);

/**
 * An IBECC error causes SERR_NMI_STS set and is indicated by
 * ERRSTS PCI registers by IBECC_UC and IBECC_COR fields.
 * Following needs to be done:
 *  - Read ECC_ERR_LOG register
 *  - Clear IBECC_UC and IBECC_COR fields of ERRSTS PCI
 *  - Clear MERRSTS & CERRSTS fields of ECC_ERR_LOG register
 */

static struct k_spinlock nmi_lock;

/* NMI handling */

static bool handle_nmi(void)
{
	uint8_t status;

	status = sys_in8(NMI_STS_CNT_REG);

	if (!(status & NMI_STS_SRC_SERR)) {
		LOG_DBG("Skip NMI, NMI_STS_CNT: 0x%x", status);
		/**
		 * We should be able to find that this NMI we
		 * should not handle and return false. However this
		 * does not work for some older SKUs
		 */
		return true;
	}

	LOG_DBG("core: %d status 0x%x", arch_curr_cpu()->id, status);

	/* Re-enable */

	status = (status & NMI_STS_MASK_EN) | NMI_STS_SERR_EN;
	sys_out8(status, NMI_STS_CNT_REG);

	status &= ~NMI_STS_SERR_EN;
	sys_out8(status, NMI_STS_CNT_REG);

	return true;
}

bool z_x86_do_kernel_nmi(const z_arch_esf_t *esf)
{
	const struct device *dev = DEVICE_DT_GET(DEVICE_NODE);
	struct ibecc_data *data = dev->data;
	struct ibecc_error error_data;
	k_spinlock_key_t key;
	bool ret = true;
	uint64_t ecclog;

	key = k_spin_lock(&nmi_lock);

	if (!handle_nmi()) {
		/* Indicate that we do not handle this NMI */
		ret = false;
		goto out;
	}

	/* Skip the same NMI handling for other cores and return handled */
	if (arch_curr_cpu()->id) {
		ret = true;
		goto out;
	}

	ecclog = edac_ecc_error_log_get(dev);
	parse_ecclog(dev, ecclog, &error_data);

	if (data->cb) {
		data->cb(dev, &error_data);
	}

	edac_ecc_error_log_clear(dev);

	ibecc_errsts_clear(PCI_HOST_BRIDGE);

out:
	k_spin_unlock(&nmi_lock, key);

	return ret;
}
