/*
 * Copyright 2020 Broadcom
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pcie/endpoint/pcie_ep.h>
#include <zephyr/logging/log.h>

#include "pcie_ep_iproc.h"

LOG_MODULE_DECLARE(iproc_pcie, CONFIG_PCIE_EP_LOG_LEVEL);

/* Helper macro to read 64-bit data using two 32-bit data read */
#define sys_read64(addr)    (((uint64_t)(sys_read32(addr + 4)) << 32) | \
			     sys_read32(addr))

#ifdef PCIE_EP_IPROC_INIT_CFG
void iproc_pcie_msix_config(const struct device *dev)
{
	/*
	 * Configure capability of generating 16 messages,
	 * MSI-X Table offset 0x10000 on BAR2,
	 * MSI-X PBA offset 0x10800 on BAR2.
	 */
	pcie_ep_conf_write(dev, MSIX_CONTROL, (MSIX_TABLE_SIZE - 1));
	pcie_ep_conf_write(dev, MSIX_TBL_OFF_BIR, MSIX_TBL_B2_10000);
	pcie_ep_conf_write(dev, MSIX_PBA_OFF_BIR, MSIX_PBA_B2_10800);
}

void iproc_pcie_msi_config(const struct device *dev)
{
	uint32_t data;

	/* Configure capability of generating 16 messages */
	pcie_ep_conf_read(dev, ID_VAL4_OFFSET, &data);
	data = (data & ~(MSI_COUNT_MASK)) | (MSI_COUNT_VAL << MSI_COUNT_SHIFT);
	pcie_ep_conf_write(dev, ID_VAL4_OFFSET, data);
}
#endif

int iproc_pcie_generate_msi(const struct device *dev, const uint32_t msi_num)
{
	int ret = 0;
#ifdef CONFIG_PCIE_EP_IPROC_V2
	uint64_t addr;
	uint32_t data;

	pcie_ep_conf_read(dev, MSI_ADDR_H, &data);
	addr = ((uint64_t)data) << 32;
	pcie_ep_conf_read(dev, MSI_ADDR_L, &data);
	addr = addr | data;

	if (data == 0) {
		/*
		 * This is mostly the case where the test is being run
		 * from device before host driver sets up MSI.
		 * Returning zero instead of error because of this.
		 */
		LOG_WRN("MSI is not setup, skipping MSI");
		return 0;
	}

	pcie_ep_conf_read(dev, MSI_DATA, &data);
	data |= msi_num;

	ret = pcie_ep_xfer_data_memcpy(dev, addr,
				       (uintptr_t *)&data, sizeof(data),
				       PCIE_OB_LOWMEM, DEVICE_TO_HOST);

#else
	const struct iproc_pcie_ep_config *cfg = dev->config;

	pcie_write32(msi_num, &cfg->base->paxb_pcie_sys_msi_req);
#endif
	return ret;
}

static int generate_msix(const struct device *dev, const uint32_t msix_num)
{
	int ret;
	uint64_t addr;
	uint32_t data;

	addr = sys_read64(MSIX_VECTOR_OFF(msix_num) + MSIX_TBL_ADDR_OFF);

	if (addr == 0) {
		/*
		 * This is mostly the case where the test is being run
		 * from device before host driver has setup MSIX table.
		 * Returning zero instead of error because of this.
		 */
		LOG_WRN("MSIX table is not setup, skipping MSIX\n");
		ret = 0;
		goto out;
	}

	data = sys_read32(MSIX_VECTOR_OFF(msix_num) + MSIX_TBL_DATA_OFF);

	ret = pcie_ep_xfer_data_memcpy(dev, addr,
				       (uintptr_t *)&data, sizeof(data),
				       PCIE_OB_LOWMEM, DEVICE_TO_HOST);

	if (ret < 0) {
		goto out;
	}

	LOG_DBG("msix %d generated\n", msix_num);
out:
	return ret;
}

#ifdef CONFIG_PCIE_EP_IPROC_V2
static bool is_pcie_function_mask(const struct device *dev)
{
	uint32_t data;

	pcie_ep_conf_read(dev, MSIX_CAP, &data);

	return ((data & MSIX_FUNC_MASK) ? true : false);
}

static bool is_msix_vector_mask(const int msix_num)
{
	uint32_t data;

	data = sys_read32(MSIX_VECTOR_OFF(msix_num) + MSIX_TBL_VECTOR_CTRL_OFF);

	return ((data & MSIX_VECTOR_MASK) ? true : false);
}

/* Below function will be called from interrupt context */
static int generate_pending_msix(const struct device *dev, const int msix_num)
{
	int is_msix_pending;
	struct iproc_pcie_ep_ctx *ctx = dev->data;
	k_spinlock_key_t key;

	/* check if function mask bit got set by Host */
	if (is_pcie_function_mask(dev)) {
		LOG_DBG("function mask set! %d\n", msix_num);
		return 0;
	}

	key = k_spin_lock(&ctx->pba_lock);

	is_msix_pending = sys_test_bit(PBA_OFFSET(msix_num),
				       PENDING_BIT(msix_num));

	/* check if vector mask bit is cleared for pending msix */
	if (is_msix_pending && !(is_msix_vector_mask(msix_num))) {
		LOG_DBG("msix %d unmasked\n", msix_num);
		/* generate msix and clear pending bit */
		generate_msix(dev, msix_num);
		sys_clear_bit(PBA_OFFSET(msix_num), PENDING_BIT(msix_num));
	}

	k_spin_unlock(&ctx->pba_lock, key);
	return 0;
}

/* Below function will be called from interrupt context */
static int generate_all_pending_msix(const struct device *dev)
{
	int i;

	for (i = 0; i < MSIX_TABLE_SIZE; i++) {
		generate_pending_msix(dev, i);
	}

	return 0;
}

void iproc_pcie_func_mask_isr(void *arg)
{
	const struct device *dev = arg;
	const struct iproc_pcie_ep_config *cfg = dev->config;
	uint32_t data;

	data = pcie_read32(&cfg->base->paxb_pcie_cfg_intr_status);

	LOG_DBG("%s: %x\n", __func__, data);

	if (data & SNOOP_VALID_INTR) {
		pcie_write32(SNOOP_VALID_INTR,
			     &cfg->base->paxb_pcie_cfg_intr_clear);
		if (!is_pcie_function_mask(dev)) {
			generate_all_pending_msix(dev);
		}
	}
}

void iproc_pcie_vector_mask_isr(void *arg)
{
	const struct device *dev = arg;
	int msix_table_update = sys_test_bit(PMON_LITE_PCIE_INTERRUPT_STATUS,
					     WR_ADDR_CHK_INTR_EN);

	LOG_DBG("%s: %x\n", __func__,
		sys_read32(PMON_LITE_PCIE_INTERRUPT_STATUS));

	if (msix_table_update) {
		sys_write32(BIT(WR_ADDR_CHK_INTR_EN),
			    PMON_LITE_PCIE_INTERRUPT_CLEAR);
		generate_all_pending_msix(dev);
	}
}
#endif

int iproc_pcie_generate_msix(const struct device *dev, const uint32_t msix_num)
{
	if (msix_num >= MSIX_TABLE_SIZE) {
		LOG_WRN("Exceeded max supported MSI-X (%d)", MSIX_TABLE_SIZE);
		return -ENOTSUP;
	}

#ifdef CONFIG_PCIE_EP_IPROC_V2
	struct iproc_pcie_ep_ctx *ctx = dev->data;
	k_spinlock_key_t key;

	/*
	 * Read function mask bit/vector mask bit and update pending bit
	 * with spin_lock - aim is not to allow interrupt context
	 * to update PBA during this section
	 * This will make sure of no races between mask bit read
	 * and pending bit update.
	 */

	key = k_spin_lock(&ctx->pba_lock);

	if (is_pcie_function_mask(dev) || is_msix_vector_mask(msix_num)) {
		LOG_DBG("msix %d masked\n", msix_num);
		/* set pending bit and return */
		sys_set_bit(PBA_OFFSET(msix_num), PENDING_BIT(msix_num));
		k_spin_unlock(&ctx->pba_lock, key);
		return -EBUSY;
	}

	k_spin_unlock(&ctx->pba_lock, key);
#endif
	return generate_msix(dev, msix_num);
}
