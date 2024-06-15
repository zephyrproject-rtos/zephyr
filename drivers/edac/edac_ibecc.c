/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_ibecc

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pcie/pcie.h>

#include <zephyr/drivers/edac.h>
#include "ibecc.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(edac_ibecc, CONFIG_EDAC_LOG_LEVEL);

#define DEVICE_NODE DT_NODELABEL(ibecc)

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

	if ((errsts & (ERRSTS_IBECC_COR | ERRSTS_IBECC_UC)) == 0) {
		return;
	}

	pcie_conf_write(bdf, ERRSTS_REG, errsts);
}

static void parse_ecclog(const struct device *dev, const uint64_t ecclog,
			 struct ibecc_error *error_data)
{
	struct ibecc_data *data = dev->data;

	if (ecclog == 0) {
		return;
	}

	error_data->type = ECC_ERROR_ERRTYPE(ecclog);
	error_data->address = ECC_ERROR_ERRADD(ecclog);
	error_data->syndrome = ECC_ERROR_ERRSYND(ecclog);

	if ((ecclog & ECC_ERROR_MERRSTS) != 0) {
		data->errors_uc++;
	}

	if ((ecclog & ECC_ERROR_CERRSTS) != 0) {
		data->errors_cor++;
	}
}

#if defined(CONFIG_EDAC_ERROR_INJECT)
static int inject_set_param1(const struct device *dev, uint64_t addr)
{
	if ((addr & ~INJ_ADDR_BASE_MASK) != 0) {
		return -EINVAL;
	}

	ibecc_write_reg64(dev, IBECC_INJ_ADDR_BASE, addr);

	return 0;
}

static int inject_get_param1(const struct device *dev, uint64_t *value)
{
	*value = ibecc_read_reg64(dev, IBECC_INJ_ADDR_BASE);

	return 0;
}

static int inject_set_param2(const struct device *dev, uint64_t mask)
{
	if ((mask & ~INJ_ADDR_BASE_MASK_MASK) != 0) {
		return -EINVAL;
	}

	ibecc_write_reg64(dev, IBECC_INJ_ADDR_MASK, mask);

	return 0;
}

static int inject_get_param2(const struct device *dev, uint64_t *value)
{
	*value = ibecc_read_reg64(dev, IBECC_INJ_ADDR_MASK);

	return 0;
}

static int inject_set_error_type(const struct device *dev,
				 uint32_t error_type)
{
	struct ibecc_data *data = dev->data;

	data->error_type = error_type;

	return 0;
}

static int inject_get_error_type(const struct device *dev,
				      uint32_t *error_type)
{
	struct ibecc_data *data = dev->data;

	*error_type = data->error_type;

	return 0;
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

static int ecc_error_log_get(const struct device *dev, uint64_t *value)
{
	*value = ibecc_read_reg64(dev, IBECC_ECC_ERROR_LOG);
	/**
	 * The ECC Error log register is only valid when ECC_ERROR_CERRSTS
	 * or ECC_ERROR_MERRSTS error status bits are set
	 */
	if ((*value & (ECC_ERROR_MERRSTS | ECC_ERROR_CERRSTS)) == 0) {
		return -ENODATA;
	}

	return 0;
}

static int ecc_error_log_clear(const struct device *dev)
{
	/* Clear all error bits */
	ibecc_write_reg64(dev, IBECC_ECC_ERROR_LOG,
			  ECC_ERROR_MERRSTS | ECC_ERROR_CERRSTS);

	return 0;
}

static int parity_error_log_get(const struct device *dev, uint64_t *value)
{
	*value = ibecc_read_reg64(dev, IBECC_PARITY_ERROR_LOG);
	if (*value == 0) {
		return -ENODATA;
	}

	return 0;
}

static int parity_error_log_clear(const struct device *dev)
{
	ibecc_write_reg64(dev, IBECC_PARITY_ERROR_LOG, PARITY_ERROR_ERRSTS);

	return 0;
}

static int errors_cor_get(const struct device *dev)
{
	struct ibecc_data *data = dev->data;

	return data->errors_cor;
}

static int errors_uc_get(const struct device *dev)
{
	struct ibecc_data *data = dev->data;

	return data->errors_uc;
}

static int notify_callback_set(const struct device *dev,
			       edac_notify_callback_f cb)
{
	struct ibecc_data *data = dev->data;
	unsigned int key = irq_lock();

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

static int edac_ibecc_init(const struct device *dev)
{
	const pcie_bdf_t bdf = PCI_HOST_BRIDGE;
	struct ibecc_data *data = dev->data;
	uint64_t mchbar;
	uint32_t conf_data;

	conf_data = pcie_conf_read(bdf, PCIE_CONF_ID);
	switch (conf_data) {
	case PCIE_ID(PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_SKU5):
		__fallthrough;
	case PCIE_ID(PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_SKU6):
		__fallthrough;
	case PCIE_ID(PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_SKU7):
		__fallthrough;
	case PCIE_ID(PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_SKU8):
		__fallthrough;
	case PCIE_ID(PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_SKU9):
		__fallthrough;
	case PCIE_ID(PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_SKU10):
		__fallthrough;
	case PCIE_ID(PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_SKU11):
		__fallthrough;
	case PCIE_ID(PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_SKU12):
		__fallthrough;
	case PCIE_ID(PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_SKU13):
		__fallthrough;
	case PCIE_ID(PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_SKU14):
		__fallthrough;
	case PCIE_ID(PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_SKU15):
		break;
	default:
		LOG_ERR("PCI Probe failed"); /* LCOV_EXCL_BR_LINE */
		return -ENODEV;
	}

	if (!ibecc_enabled(bdf)) {
		LOG_ERR("IBECC is not enabled"); /* LCOV_EXCL_BR_LINE */
		return -ENODEV;
	}

	mchbar = pcie_conf_read(bdf, MCHBAR_REG);
	mchbar |= (uint64_t)pcie_conf_read(bdf, MCHBAR_REG + 1) << 32;

	/* Check that MCHBAR is enabled */
	if ((mchbar & MCHBAR_ENABLE) == 0) {
		LOG_ERR("MCHBAR is not enabled"); /* LCOV_EXCL_BR_LINE */
		return -ENODEV;
	}

	mchbar &= MCHBAR_MASK;

	device_map(&data->mchbar, mchbar, MCH_SIZE, K_MEM_CACHE_NONE);

	/* Enable Host Bridge generated SERR event */
	ibecc_errcmd_setup(bdf, true);

	LOG_INF("IBECC driver initialized"); /* LCOV_EXCL_BR_LINE */

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
	if ((status & NMI_STS_SRC_SERR) == 0) {
		/* For other NMI sources return false to handle it by
		 * Zephyr exception handler
		 */
		return false;
	}

	/* Re-enable SERR# NMI sources */

	status = (status & NMI_STS_MASK_EN) | NMI_STS_SERR_EN;
	sys_out8(status, NMI_STS_CNT_REG);

	status &= ~NMI_STS_SERR_EN;
	sys_out8(status, NMI_STS_CNT_REG);

	return true;
}

bool z_x86_do_kernel_nmi(const struct arch_esf *esf)
{
	const struct device *const dev = DEVICE_DT_GET(DEVICE_NODE);
	struct ibecc_data *data = dev->data;
	struct ibecc_error error_data;
	k_spinlock_key_t key;
	bool ret = true;
	uint64_t ecclog;

	key = k_spin_lock(&nmi_lock);

	/* Skip the same NMI handling for other cores and return handled */
	if (arch_curr_cpu()->id != 0) {
		ret = true;
		goto out;
	}

	if (!handle_nmi()) {
		/* Indicate that we do not handle this NMI */
		ret = false;
		goto out;
	}

	if (edac_ecc_error_log_get(dev, &ecclog) != 0) {
		goto out;
	}

	parse_ecclog(dev, ecclog, &error_data);

	if (data->cb != NULL) {
		data->cb(dev, &error_data);
	}

	edac_ecc_error_log_clear(dev);

	ibecc_errsts_clear(PCI_HOST_BRIDGE);

out:
	k_spin_unlock(&nmi_lock, key);

	return ret;
}
