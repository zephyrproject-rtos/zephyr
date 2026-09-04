/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Platform abstraction for SX crypto driver library.
 */
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <soc_crypto.h>

#include <sl_code_classification.h>
#include <sl_status.h>
#include <sli_crypto_s3.h>
#include <sli_sxsymcrypt.h>

volatile sl_status_t sli_crypto_preempted_status = SL_STATUS_NOT_INITIALIZED;
struct sxdesc;
struct crypto_selection {
	const struct device *dev;
	struct k_mutex lock;
	atomic_t busy;
};

static struct crypto_selection selection = {
	.dev = NULL,
	.lock = Z_MUTEX_INITIALIZER(selection.lock),
	.busy = ATOMIC_INIT(0),
};

static const struct device *crypto_dev_from_instance(unsigned int instance)
{
	switch (instance) {
	case SLI_CRYPTO_HOSTSYMCRYPTO:
		return DEVICE_DT_GET_OR_NULL(DT_NODELABEL(symcrypto));
	case SLI_CRYPTO_LPWAES:
		return DEVICE_DT_GET_OR_NULL(DT_NODELABEL(lpwaes));
	default:
		return NULL;
	}
}

sl_status_t sli_sxsymcrypt_lock_cryptomaster_selection(unsigned int instance, bool yield)
{
	const struct device *dev = crypto_dev_from_instance(instance);
	int ret;

	if (!dev) {
		return SL_STATUS_INVALID_PARAMETER;
	}

	if (k_is_in_isr()) {
		if (instance != SLI_CRYPTO_LPWAES) {
			/* Only LPWAES is supported from ISR */
			return SL_STATUS_NOT_SUPPORTED;
		}

		ret = soc_crypto_enable(dev, false);
		if (ret < 0) {
			return SL_STATUS_FAIL;
		}

		/* Store status of previous operation */
		if (soc_crypto_wait_busy(dev)) {
			sli_crypto_preempted_status = SL_STATUS_OK;
		} else {
			sli_crypto_preempted_status = SL_STATUS_FAIL;
		}
		return SL_STATUS_ISR;
	}

	ret = k_mutex_lock(&selection.lock, K_FOREVER);
	if (ret < 0) {
		return SL_STATUS_FAIL;
	}

	atomic_set(&selection.busy, 1);

	ret = soc_crypto_get(dev);
	if (ret < 0) {
		goto cleanup_get;
	}

	ret = soc_crypto_enable(dev, yield);
	if (ret < 0) {
		goto cleanup_enable;
	}

	selection.dev = dev;

	return SL_STATUS_OK;

cleanup_enable:
	soc_crypto_put(dev);
cleanup_get:
	atomic_set(&selection.busy, 0);
	k_mutex_unlock(&selection.lock);
	return SL_STATUS_FAIL;
}

sl_status_t sli_sxsymcrypt_unlock_cryptomaster_selection(void)
{
	int ret;

	if (k_is_in_isr()) {
		return SL_STATUS_OK;
	}

	atomic_set(&selection.busy, 0);

	ret = k_mutex_unlock(&selection.lock);
	if (ret < 0) {
		return SL_STATUS_FAIL;
	}

	return SL_STATUS_OK;
}

SL_CODE_CLASSIFY(SL_CODE_COMPONENT_SXSYMCRYPT, SL_CODE_CLASS_TIME_CRITICAL)
void sx_cmdma_release_hw(struct sx_regs *regs)
{
	const struct device *dev = crypto_dev_from_instance(regs->instance_index);
	int ret;

	if (k_is_in_isr()) {
		return;
	}

	if (!atomic_get(&selection.busy) &&
	    ((sli_crypto_preempted_status == SL_STATUS_NOT_INITIALIZED) ||
	     (regs->instance_index != SLI_CRYPTO_LPWAES))) {
		ret = soc_crypto_disable(dev);
		__ASSERT_NO_MSG(ret == 0);
	}
	ret = soc_crypto_put(dev);
	__ASSERT_NO_MSG(ret == 0);
}

void sli_cmdma_release_hw(struct sx_regs *regs)
{
	sx_cmdma_release_hw(regs);
}

SL_CODE_CLASSIFY(SL_CODE_COMPONENT_SXSYMCRYPT, SL_CODE_CLASS_TIME_CRITICAL)
void sx_cmdma_wait(struct sx_regs *regs)
{
	const struct device *dev = crypto_dev_from_instance(regs->instance_index);

	soc_crypto_wait(dev);
}

SL_CODE_CLASSIFY(SL_CODE_COMPONENT_SXSYMCRYPT, SL_CODE_CLASS_TIME_CRITICAL)
struct sx_regs *sx_hw_find_regs(unsigned int idx)
{
	const struct device *dev = selection.dev;

	if (k_is_in_isr()) {
		dev = crypto_dev_from_instance(SLI_CRYPTO_LPWAES);
	}

	if (!dev || idx >= DT_NUM_INST_STATUS_OKAY(silabs_series3_crypto)) {
		return NULL;
	}

	return soc_crypto_get_regs(dev);
}

SL_CODE_CLASSIFY(SL_CODE_COMPONENT_SXSYMCRYPT, SL_CODE_CLASS_TIME_CRITICAL)
struct sx_regs *sx_cmdma_find_available(unsigned int compatible)
{
	ARG_UNUSED(compatible);
	const struct device *dev = selection.dev;

	if (k_is_in_isr()) {
		/* Only LPWAES is supported from ISR */
		dev = crypto_dev_from_instance(SLI_CRYPTO_LPWAES);
	}

	if (!dev) {
		return NULL;
	}

	if (k_is_in_isr()) {
		/* Wait for the engine to be available */
		soc_crypto_wait_busy(dev);
	}

	return soc_crypto_get_regs(dev);
}

void sli_crypto_lpwaes_save_state(sli_cryptomaster_state_t *state)
{
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpwaes))
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(lpwaes));

	soc_crypto_get_state(dev, &state->FETCHADDR, &state->PUSHADDR);
#endif
}

void sli_crypto_lpwaes_restore_state(sli_cryptomaster_state_t *state)
{
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpwaes))
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(lpwaes));

	soc_crypto_set_state(dev, state->FETCHADDR, state->PUSHADDR);
#endif
}

SL_CODE_CLASSIFY(SL_CODE_COMPONENT_SXSYMCRYPT, SL_CODE_CLASS_TIME_CRITICAL)
uint32_t sx_rdreg(struct sx_regs *regs, uint32_t addr)
{
	return sys_read32((mem_addr_t)(regs->base_address + addr));
}

SL_CODE_CLASSIFY(SL_CODE_COMPONENT_SXSYMCRYPT, SL_CODE_CLASS_TIME_CRITICAL)
void sx_wrreg_addr(struct sx_regs *regs, uint32_t addr, struct sxdesc *p)
{
	sys_write32((uint32_t)p, (mem_addr_t)(regs->base_address + addr));
}

SL_CODE_CLASSIFY(SL_CODE_COMPONENT_SXSYMCRYPT, SL_CODE_CLASS_TIME_CRITICAL)
void sx_wrreg(struct sx_regs *regs, uint32_t addr, uint32_t val)
{
	sys_write32((uint32_t)val, (mem_addr_t)(regs->base_address + addr));
}

SL_CODE_CLASSIFY(SL_CODE_COMPONENT_SXSYMCRYPT, SL_CODE_CLASS_TIME_CRITICAL)
void sx_flush_tohw(struct sx_regs *regs, char *cpumem, size_t sz)
{
	ARG_UNUSED(regs);
	ARG_UNUSED(cpumem);
	ARG_UNUSED(sz);
}

SL_CODE_CLASSIFY(SL_CODE_COMPONENT_SXSYMCRYPT, SL_CODE_CLASS_TIME_CRITICAL)
void sx_flush_fromhw(struct sx_regs *regs, char *cpumem, size_t offset, size_t sz)
{
	ARG_UNUSED(regs);
	ARG_UNUSED(cpumem);
	ARG_UNUSED(offset);
	ARG_UNUSED(sz);
}

SL_CODE_CLASSIFY(SL_CODE_COMPONENT_SXSYMCRYPT, SL_CODE_CLASS_TIME_CRITICAL)
char *sx_map_internal(struct sx_regs *regs, char *dma)
{
	ARG_UNUSED(regs);

	return (char *)dma;
}

SL_CODE_CLASSIFY(SL_CODE_COMPONENT_SXSYMCRYPT, SL_CODE_CLASS_TIME_CRITICAL)
char *sx_map_usrdatain(char *s, size_t sz)
{
	ARG_UNUSED(sz);

	return s;
}

SL_CODE_CLASSIFY(SL_CODE_COMPONENT_SXSYMCRYPT, SL_CODE_CLASS_TIME_CRITICAL)
char *sx_map_usrdataout(char *s, size_t sz)
{
	ARG_UNUSED(sz);

	return s;
}

SL_CODE_CLASSIFY(SL_CODE_COMPONENT_SXSYMCRYPT, SL_CODE_CLASS_TIME_CRITICAL)
void sx_trigger_hardfault(void)
{
	k_panic();
}
