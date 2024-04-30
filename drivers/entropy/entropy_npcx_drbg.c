/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcx_drbg

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/pm/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(entropy_npcx_drbg, CONFIG_ENTROPY_LOG_LEVEL);

#include "soc_ncl.h"

/* Reseed after 100 number generations */
#define NPCX_DRBG_SECURITY_STRENGTH                                                                \
	((enum ncl_drbg_security_strength)CONFIG_ENTROPY_NPCX_DRBG_SECURITY_STRENGTH)
#define NPCX_DRBG_RESEED_INTERVAL CONFIG_ENTROPY_NPCX_DRBG_RESEED_INTERVAL

#define NPCX_DRBG_HANDLE_SIZE DT_INST_PROP(0, context_buffer_size)
struct entropy_npcx_drbg_dev_data {
	struct k_sem sem_lock;
	uint8_t handle[NPCX_DRBG_HANDLE_SIZE] __aligned(4);
};

/*
 * The base address of the table that holds the function pointer for each
 * DRBG API in ROM.
 */
#define NPCX_NCL_DRBG_BASE_ADDR ((const struct npcx_ncl_drbg *)DT_INST_REG_ADDR_BY_IDX(0, 0))
/* The following table holds the function pointer for each DRBG API in NPCX ROM. */
struct npcx_ncl_drbg {
	/* Get the DRBG context size required by DRBG APIs. */
	uint32_t (*get_context_size)(void);
	/* Initialize DRBG context. */
	enum ncl_status (*init_context)(void *ctx);
	/* Power on/off DRBG module. */
	enum ncl_status (*power)(void *ctx, uint8_t enable);
	/* Finalize DRBG context. */
	enum ncl_status (*finalize_context)(void *ctx);
	/* Initialize the DRBG hardware module and enable interrupts. */
	enum ncl_status (*init)(void *ctx, bool int_enable);
	/*
	 * Configure DRBG, pres_resistance enables/disables (1/0) prediction
	 * resistance
	 */
	enum ncl_status (*config)(void *ctx, uint32_t reseed_interval, uint8_t pred_resistance);
	/*
	 * This routine creates a first instantiation of the DRBG mechanism
	 * parameters. The routine pulls an initial seed from the HW RNG module
	 * and resets the reseed counter. DRBG and SHA modules should be
	 * activated prior to the this operation.
	 */
	enum ncl_status (*instantiate)(void *ctx, enum ncl_drbg_security_strength sec_strength,
				       const uint8_t *pers_string, uint32_t pers_string_len);
	/* Uninstantiate DRBG module */
	enum ncl_status (*uninstantiate)(void *ctx);
	/* Reseeds the internal state of the given instantce */
	enum ncl_status (*reseed)(void *ctc, uint8_t *add_data, uint32_t add_data_len);
	/* Generates a random number from the current internal state. */
	enum ncl_status (*generate)(void *ctx, const uint8_t *add_data, uint32_t add_data_len,
				    uint8_t *out_buff, uint32_t out_buff_len);
	/* Clear all DRBG SSPs (Sensitive Security Parameters) in HW & driver */
	enum ncl_status (*clear)(void *ctx);
};
#define NPCX_NCL_DRBG ((const struct npcx_ncl_drbg *)NPCX_NCL_DRBG_BASE_ADDR)

/* The 2nd index of the reg property holds the address of NCL_SHA_Power ROM API */
#define NPCX_NCL_SHA_POWER_ADDR ((const struct npcx_ncl_drbg *)DT_INST_REG_ADDR_BY_IDX(0, 1))
struct npcx_ncl_sha {
	/* Power on/off SHA module. */
	enum ncl_status (*power)(void *ctx, uint8_t on);
};
#define NPCX_NCL_SHA_POWER ((const struct npcx_ncl_sha *)NPCX_NCL_SHA_POWER_ADDR)

static int entropy_npcx_drbg_enable_sha_power(void *ctx, bool enable)
{
	enum ncl_status ncl_ret;

	ncl_ret = NPCX_NCL_SHA_POWER->power(ctx, enable);
	if (ncl_ret != NCL_STATUS_OK) {
		LOG_ERR("Fail to %s SHA power: err 0x%02x", enable ? "enable" : "disable", ncl_ret);
		return -EIO;
	}

	return 0;
}

static int entropy_npcx_drbg_enable_drbg_power(void *ctx, bool enable)
{
	enum ncl_status ncl_ret;

	ncl_ret = NPCX_NCL_DRBG->power(ctx, enable);
	if (ncl_ret != NCL_STATUS_OK) {
		LOG_ERR("Fail to %s DRBG power: err 0x%02x", enable ? "enable" : "disable",
			ncl_ret);
		return -EIO;
	}

	return 0;
}

static int entropy_npcx_drbg_get_entropy(const struct device *dev, uint8_t *buf, uint16_t len)
{
	struct entropy_npcx_drbg_dev_data *const data = dev->data;
	enum ncl_status ncl_ret;
	void *ctx = data->handle;
	int ret = 0;

	k_sem_take(&data->sem_lock, K_FOREVER);

	ret = entropy_npcx_drbg_enable_sha_power(ctx, true);
	if (ret != 0) {
		goto err_exit;
	}

	ncl_ret = NPCX_NCL_DRBG->generate(ctx, NULL, 0, buf, len);
	if (ncl_ret != NCL_STATUS_OK) {
		LOG_ERR("Fail to generate: err 0x%02x", ncl_ret);
		ret = -EIO;
		goto err_exit;
	}

	ret = entropy_npcx_drbg_enable_sha_power(ctx, false);

err_exit:
	k_sem_give(&data->sem_lock);

	return ret;
}

static int entropy_npcx_drbg_init(const struct device *dev)
{
	struct entropy_npcx_drbg_dev_data *const data = dev->data;
	uint32_t handle_size_required;
	enum ncl_status ncl_ret;
	void *ctx = data->handle;
	int ret;

	handle_size_required = NPCX_NCL_DRBG->get_context_size();
	if (handle_size_required != NPCX_DRBG_HANDLE_SIZE) {
		LOG_ERR("Unexpected NCL DRBG context_size = %d", handle_size_required);
		return -ENOSR;
	}

	ret = entropy_npcx_drbg_enable_sha_power(ctx, true);
	if (ret != 0) {
		return ret;
	}

	ret = entropy_npcx_drbg_enable_drbg_power(ctx, true);
	if (ret != 0) {
		return ret;
	}

	ncl_ret = NPCX_NCL_DRBG->init_context(ctx);
	if (ncl_ret != NCL_STATUS_OK) {
		LOG_ERR("Fail to init ctx: err 0x%02x", ncl_ret);
		return -EIO;
	}

	ncl_ret = NPCX_NCL_DRBG->init(ctx, false);
	if (ncl_ret != NCL_STATUS_OK) {
		LOG_ERR("Fail to init: err 0x%02x", ncl_ret);
		return -EIO;
	}

	ncl_ret = NPCX_NCL_DRBG->config(ctx, NPCX_DRBG_RESEED_INTERVAL, false);
	if (ncl_ret != NCL_STATUS_OK) {
		LOG_ERR("Fail to config: err 0x%02x", ncl_ret);
		return -EIO;
	}

	ncl_ret = NPCX_NCL_DRBG->instantiate(ctx, NPCX_DRBG_SECURITY_STRENGTH, NULL, 0);
	if (ncl_ret != NCL_STATUS_OK) {
		LOG_ERR("Fail to config: err 0x%02x", ncl_ret);
		return -EIO;
	}

	ret = entropy_npcx_drbg_enable_sha_power(ctx, false);
	if (ret != 0) {
		return ret;
	}

	/* Locking semaphore initialized to 1 (unlocked) */
	k_sem_init(&data->sem_lock, 1, 1);

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int entropy_npcx_drbg_suspend(const struct device *dev)
{
	struct entropy_npcx_drbg_dev_data *const data = dev->data;
	void *ctx = data->handle;

	return entropy_npcx_drbg_enable_drbg_power(ctx, false);
}

static int entropy_npcx_drbg_resume(const struct device *dev)
{
	struct entropy_npcx_drbg_dev_data *const data = dev->data;
	void *ctx = data->handle;

	return entropy_npcx_drbg_enable_drbg_power(ctx, true);
}

static int entropy_npcx_drbg_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		return entropy_npcx_drbg_suspend(dev);
	case PM_DEVICE_ACTION_RESUME:
		return entropy_npcx_drbg_resume(dev);
	default:
		return -ENOTSUP;
	}
}
#endif /* CONFIG_PM_DEVICE */

static const struct entropy_driver_api entropy_npcx_drbg_api = {
	.get_entropy = entropy_npcx_drbg_get_entropy,
};

static struct entropy_npcx_drbg_dev_data entropy_npcx_drbg_data;

PM_DEVICE_DT_INST_DEFINE(0, entropy_npcx_drbg_pm_action);

DEVICE_DT_INST_DEFINE(0, entropy_npcx_drbg_init, PM_DEVICE_DT_INST_GET(0), &entropy_npcx_drbg_data,
		      NULL, PRE_KERNEL_1, CONFIG_ENTROPY_INIT_PRIORITY, &entropy_npcx_drbg_api);
