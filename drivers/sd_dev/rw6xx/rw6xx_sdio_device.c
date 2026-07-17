/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_rw6xx_sdio_device

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/sd_dev.h>

#ifdef CONFIG_PINCTRL
#include <zephyr/drivers/pinctrl.h>
#endif

#ifdef CONFIG_SD_DEV_RW6XX_SDIO
#include "fsl_adapter_sdu.h"
#endif

LOG_MODULE_REGISTER(rw6xx_sdio_device, LOG_LEVEL_INF);

/**
 * @brief RW6xx SDIO Device-side driver
 *
 * - Implements Zephyr driver model (config / data / api)
 * - Device-side SDIO (slave)
 * - No dependency on subsys headers
 * - Interaction only via drivers/sd_dev.h
 */

/**
 * @brief Runtime data structure (read-write)
 */
struct rw6xx_sdio_data {
	void *card;
	atomic_t prev_state;
	atomic_t state;
	struct k_spinlock reg_lock;
	sd_dev_recv_pkt recv_pkt;
};

/**
 * @brief Configuration structure (read-only, from Devicetree)
 */
struct rw6xx_sdio_config {
	const struct pinctrl_dev_config *pcfg;
	int irq;
	uint8_t num_funcs;
	uint16_t vendor_id;
	uint16_t device_id;
	uint8_t fun_num;
	uint8_t cpu_num;
	uint8_t used_port_num;
	uint8_t cmd_tx_format;
	uint8_t cmd_rd_format;
	uint8_t data_tx_format;
	uint8_t data_rd_format;
	struct sd_dev_cfg sd_dev_cfg;
};

/**
 * @brief IRQ handler for SDIO device
 *
 * @param dev Pointer to device structure
 */
static void rw6xx_sdio_isr(const struct device *dev)
{
	ARG_UNUSED(dev);
	SDU_DriverIRQHandler();
}

/**
 * @brief Convert SDIO address to RW612 physical address
 *
 * RW612 SDU address is not identical to standard SDIO, so offset is needed
 *
 * @param dev Pointer to device structure
 * @param sdio_addr SDIO address
 *
 * @return Physical address or -1 on error
 */
static uint32_t sdio_to_rw612_addr(const struct device *dev, uint32_t sdio_addr)
{
	uint32_t offset = sdio_addr & 0xFF;
	const struct rw6xx_sdio_config *cfg = dev->config;
	const struct sdio_dev_cfg *sdio_cfg = &cfg->sd_dev_cfg.sdio_cfg;
	uint32_t phy_off = 0;

	if ((sdio_addr >= sdio_cfg->funcs[1].fbr_addr) &&
	    (sdio_addr <= (sdio_cfg->funcs[1].fbr_addr + 0x12))) {
		switch (offset) {
		case 0x20:
		case 0x21:
		case 0x22:
			phy_off = offset - 0x20; /* -> 0x00~0x02 */
			break;
		case 0x29:
			phy_off = 0x05;
			break;
		case 0x2A:
			phy_off = 0x06;
			break;
		case 0x2B:
			phy_off = 0x07;
			break;
		case 0x30:
			phy_off = 0x08;
			break;
		case 0x31:
			phy_off = 0x09;
			break;
		default:
			return 0;
		}
		return sdio_cfg->funcs[0].fbr_addr + phy_off;
	} else {
		return sdio_addr;
	}
}

/**
 * @brief Get SDEV configuration
 *
 * @param dev Pointer to device structure
 *
 * @return Pointer to SDEV configuration
 */
static inline const struct sd_dev_cfg *rw6xx_sdio_get_config(const struct device *dev)
{
	const struct rw6xx_sdio_config *cfg = dev->config;

	return &cfg->sd_dev_cfg;
}

/**
 * @brief Read 8-bit SDIO register
 *
 * @param dev Pointer to device structure
 * @param addr Register address
 * @param val Pointer to store read value
 *
 * @retval 0 on success
 * @retval -EINVAL if val is NULL
 */
static inline int rw6xx_sdio_read_reg8(const struct device *dev, uint32_t addr, uint8_t *val)
{
	struct rw6xx_sdio_data *data = dev->data;
	k_spinlock_key_t key;
	volatile uint8_t *reg;

	if (val == NULL) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->reg_lock);
	reg = (volatile uint8_t *)sdio_to_rw612_addr(dev, addr);
	if (!reg) {
		LOG_ERR("%s: rw612 addr convert addr wrong", __func__);
		return -EINVAL;
	}
	*val = *reg;
	k_spin_unlock(&data->reg_lock, key);

	return 0;
}

/**
 * @brief Read 32-bit SDIO register
 *
 * @param dev Pointer to device structure
 * @param addr Register address (must be 4-byte aligned)
 * @param val Pointer to store read value
 *
 * @retval 0 on success
 * @retval -EINVAL if val is NULL or addr is not aligned
 */
static inline int rw6xx_sdio_read_reg32(const struct device *dev, uint32_t addr, uint32_t *val)
{
	struct rw6xx_sdio_data *data = dev->data;
	k_spinlock_key_t key;
	volatile const uint32_t *reg;
	uint32_t raw;

	if (val == NULL) {
		return -EINVAL;
	}

	if ((addr & 0x3U) != 0U) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->reg_lock);
	reg = (volatile const uint32_t *)sdio_to_rw612_addr(dev, addr);
	if (!reg) {
		LOG_ERR("%s: rw612 addr convert addr wrong", __func__);
		return -EINVAL;
	}
	raw = *reg;
	*val = sys_le32_to_cpu(raw);
	k_spin_unlock(&data->reg_lock, key);

	return 0;
}

/**
 * @brief Write 8-bit SDIO register
 *
 * @param dev Pointer to device structure
 * @param addr Register address
 * @param val Value to write
 *
 * @retval 0 on success
 */
static inline int rw6xx_sdio_write_reg8(const struct device *dev, uint32_t addr, uint8_t val)
{
	struct rw6xx_sdio_data *data = dev->data;
	k_spinlock_key_t key;
	volatile uint8_t *reg;

	key = k_spin_lock(&data->reg_lock);
	reg = (volatile uint8_t *)sdio_to_rw612_addr(dev, addr);
	if (!reg) {
		LOG_ERR("%s: rw612 addr convert addr wrong", __func__);
		return -EINVAL;
	}
	*reg = val;
	k_spin_unlock(&data->reg_lock, key);

	return 0;
}

/**
 * @brief Write 32-bit SDIO register
 *
 * @param dev Pointer to device structure
 * @param addr Register address (must be 4-byte aligned)
 * @param val Value to write
 *
 * @retval 0 on success
 * @retval -EINVAL if addr is not aligned
 */
static inline int rw6xx_sdio_write_reg32(const struct device *dev, uint32_t addr, uint32_t val)
{
	struct rw6xx_sdio_data *data = dev->data;
	k_spinlock_key_t key;
	volatile uint32_t *reg;
	uint32_t raw;

	if ((addr & 0x3U) != 0U) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->reg_lock);
	raw = sys_cpu_to_le32(val);
	reg = (volatile uint32_t *)sdio_to_rw612_addr(dev, addr);
	if (!reg) {
		LOG_ERR("%s: rw612 addr convert addr wrong", __func__);
		return -EINVAL;
	}
	*reg = raw;
	k_spin_unlock(&data->reg_lock, key);

	return 0;
}

/**
 * @brief Configure CIS tuple
 *
 * @param dev Pointer to device structure
 *
 * @retval 0 on success
 */
static int rw6xx_cis_tuple_configurate(const struct device *dev)
{
	const struct rw6xx_sdio_config *config = dev->config;
	const struct sd_dev_cfg *sd_dev_cfg = &config->sd_dev_cfg;
	const struct sdio_dev_cfg *sdio_cfg = &sd_dev_cfg->sdio_cfg;

	SDU_GetDefaultCISTable(sdio_cfg->cccr_addr);

	return 0;
}

/**
 * @brief Set device state
 *
 * @param dev Pointer to device structure
 * @param state New state value
 */
static void rw6xx_sdio_set_state(const struct device *dev, unsigned int state)
{
	struct rw6xx_sdio_data *data = dev->data;

	atomic_set(&data->state, state);
}

/**
 * @brief Get device state
 *
 * @param dev Pointer to device structure
 *
 * @return Current device state
 */
static int rw6xx_sdio_get_state(const struct device *dev)
{
	struct rw6xx_sdio_data *data = dev->data;

	return atomic_get(&data->state);
}

/**
 * @brief Set device ready status
 *
 * @param dev Pointer to device structure
 *
 * @retval 0 on success
 * @retval negative errno code on failure
 */
static int rw6xx_set_dev_ready(const struct device *dev)
{
	ARG_UNUSED(dev);

	return (int)SDU_SetFwReady();
}

/**
 * @brief Send data packet via SDIO
 *
 * @param dev Pointer to device structure
 * @param pkt Pointer to packet structure
 *
 * @retval 0 on success
 * @retval negative errno code on failure
 */
static int rw6xx_send_data(const struct device *dev, sd_dev_pkt_t *pkt)
{
	int ret;

	ARG_UNUSED(dev);

	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, 0);

	sd_dev_pkt_get(pkt);

	ret = SDU_Send(*(uint32_t *)pkt->data, pkt->data, pkt->len);
	if (ret != 0) {
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, 0);
		LOG_DBG("%s: sd_dev send data fail", __func__);
	}

	sd_dev_pkt_free(pkt);

	return ret;
}

/**
 * @brief Register RX callback implementation for RW6xx driver
 *
 * Stores the callback and context so that the driver can notify
 * upper layers when new data is received from hardware.
 *
 * @param dev Device instance
 * @param cb Callback function pointer
 *
 * @retval 0 on success
 * @retval -EINVAL if cb is NULL
 */
static int rw6xx_register_rx_cb(const struct device *dev, sd_dev_recv_pkt cb)
{
	struct rw6xx_sdio_data *data = dev->data;

	if (cb == NULL) {
		return -EINVAL;
	}

	data->recv_pkt = cb;

	return 0;
}

/**
 * @brief Dispatch received packet to upper layer
 *
 * @param data Pointer to received data
 * @param len Length of received data
 */
void rw6xx_rx_pkt_dispatch(void *data, size_t len)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	struct rw6xx_sdio_data *rw6xx_data = dev->data;
	sd_dev_pkt_t *pkt = sd_dev_pkt_alloc(SDEV_PKT_RX);

	memcpy(pkt->data, data, len);
	pkt->len = len;
	pkt->fn = 1;

	/*
	 * Pass the received packet to the upper layer.
	 *
	 * Ownership / lifetime contract:
	 * - The driver retains its reference to @pkt and remains responsible
	 *   for calling sd_dev_pkt_free().
	 * - The recv_pkt() callback may take its own reference if it needs to
	 *   use the packet beyond the callback context.
	 * - The packet will only be freed when the last reference is released.
	 *
	 * This allows both driver and subsystem to safely share the packet
	 * using reference counting without strict ownership transfer.
	 */
	rw6xx_data->recv_pkt(dev, pkt);

	sd_dev_pkt_free(pkt);

	LOG_DBG("%s: data=%p len=%zu", __func__, data, len);
}

/**
 * @brief Driver API table
 */
static DEVICE_API(sd_dev, rw6xx_sdio_api) = {
	.get_config = rw6xx_sdio_get_config,
	.read_reg8 = rw6xx_sdio_read_reg8,
	.write_reg8 = rw6xx_sdio_write_reg8,
	.read_reg32 = rw6xx_sdio_read_reg32,
	.write_reg32 = rw6xx_sdio_write_reg32,
	.cis_tuple_configurate = rw6xx_cis_tuple_configurate,
	.set_dev_ready = rw6xx_set_dev_ready,
	.send_data = rw6xx_send_data,
	.set_state = rw6xx_sdio_set_state,
	.get_state = rw6xx_sdio_get_state,
	.register_rx_cb = rw6xx_register_rx_cb,
};

/**
 * @brief Initialize SDIO hardware
 *
 * @param dev Pointer to device structure
 *
 * @retval 0 on success
 * @retval -EINVAL if dev or cfg is NULL
 * @retval negative errno code on failure
 */
static int sdio_hw_init(const struct device *dev)
{
	const struct rw6xx_sdio_config *cfg;
	int ret;

	if (dev == NULL) {
		LOG_ERR("dev is NULL");
		return -EINVAL;
	}

	cfg = dev->config;
	if (cfg == NULL) {
		LOG_ERR("invalid cfg");
		return -EINVAL;
	}

#ifdef CONFIG_PINCTRL
	if (cfg->pcfg != NULL) {
		ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			LOG_ERR("RW6xx SDIO pinctrl init failed (%d)", ret);
			return ret;
		}
	}
#endif

	IRQ_CONNECT(DT_IRQN(DT_DRV_INST(0)), DT_IRQ(DT_DRV_INST(0), priority), rw6xx_sdio_isr,
		    DEVICE_DT_INST_GET(0), 0);

	irq_enable(cfg->irq);

	SDU_InitPhase1();
	SDU_InitPhase2();
	SDU_InitPhase3();

	LOG_INF("SDU init done");

	SDU_InstallCallback(SDU_TYPE_FOR_WRITE_CMD, rw6xx_rx_pkt_dispatch);
	SDU_InstallCallback(SDU_TYPE_FOR_WRITE_DATA, rw6xx_rx_pkt_dispatch);

	LOG_INF("SDU callback installed");

	return 0;
}

/**
 * @brief Initialize RW6xx SDIO device
 *
 * @param dev Pointer to device structure
 *
 * @retval 0 on success
 * @retval -EINVAL if dev is NULL
 * @retval negative errno code on failure
 */
static int rw6xx_sdio_init(const struct device *dev)
{
	int ret;

	if (dev == NULL) {
		LOG_ERR("dev is NULL");
		return -EINVAL;
	}

	LOG_INF("%s: SDIO init start", dev->name);

	sd_dev_set_state(dev, SDEV_DEVICE_INIT);

	ret = sdio_hw_init(dev);
	if (ret < 0) {
		LOG_ERR("%s: sdio_hw_init failed (%d)", dev->name, ret);
		return ret;
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
/**
 * @brief Suspend SDIO device
 *
 * @param dev Pointer to device structure
 *
 * @retval 0 on success
 * @retval -EINVAL if dev is NULL
 * @retval -EBUSY if device is not ready
 */
static int rw6xx_sdio_suspend(const struct device *dev)
{
	if (dev == NULL) {
		LOG_ERR("sdio_suspend: dev is NULL");
		return -EINVAL;
	}

	if (rw6xx_sdio_get_state(dev) != SDEV_DEVICE_READY) {
		return -EBUSY;
	}

	rw6xx_sdio_set_state(dev, SDEV_DEVICE_SUSPEND);

	SDU_EnterSuspend();

	POWER_EnableWakeup(DT_IRQN(DT_NODELABEL(sdio_device0)));

	return 0;
}

/**
 * @brief Resume SDIO device
 *
 * @param dev Pointer to device structure
 *
 * @retval 0 on success
 * @retval -EINVAL if dev is NULL
 */
static int rw6xx_sdio_resume(const struct device *dev)
{
	if (dev == NULL) {
		LOG_ERR("sdio_resume: dev is NULL");
		return -EINVAL;
	}

	SDU_ExitSuspend();

	rw6xx_sdio_set_state(dev, SDEV_DEVICE_READY);

	return 0;
}

/**
 * @brief Turn off SDIO device
 *
 * @param dev Pointer to device structure
 *
 * @retval 0 on success
 * @retval -EINVAL if dev is NULL
 * @retval negative errno code on failure
 */
static int rw6xx_sdio_turn_off(const struct device *dev)
{
	if (dev == NULL) {
		LOG_ERR("sdio_turn_off: dev is NULL");
		return -EINVAL;
	}

	sd_dev_set_state(dev, SDEV_DEVICE_RESET);

	return SDU_EnterPowerDown();
}

/**
 * @brief Turn on SDIO device
 *
 * @param dev Pointer to device structure
 *
 * @retval 0 on success
 * @retval negative errno code on failure
 */
static int rw6xx_sdio_turn_on(const struct device *dev)
{
	SDU_ExitPowerDown();

	return rw6xx_sdio_init(dev);
}

/**
 * @brief Power management action handler
 *
 * @param dev Pointer to device structure
 * @param action Power management action
 *
 * @retval 0 on success
 * @retval -ENOTSUP if action is not supported
 * @retval negative errno code on failure
 */
static int rw6xx_sdio_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		return rw6xx_sdio_suspend(dev);
	case PM_DEVICE_ACTION_RESUME:
		return rw6xx_sdio_resume(dev);
	case PM_DEVICE_ACTION_TURN_OFF:
		return rw6xx_sdio_turn_off(dev);
	case PM_DEVICE_ACTION_TURN_ON:
		return rw6xx_sdio_turn_on(dev);
	default:
		return -ENOTSUP;
	}
}
#endif /* CONFIG_PM_DEVICE */

/**
 * @brief Device instance macro (generated from DTS)
 */
#define RW6XX_SDIO_FUNC1_CFG(inst) {\
	.fn = 1,\
	.fbr_addr = DT_INST_REG_ADDR(inst) + 0x20,\
	.fn_code = SDIO_CFG_UNSET,\
	.block_size = 512,\
}

#define RW6XX_SDIO_DEVICE_DEFINE(inst)\
	PINCTRL_DT_INST_DEFINE(inst);\
\
	static struct rw6xx_sdio_data rw6xx_sdio_data_##inst;\
\
	static const struct rw6xx_sdio_config rw6xx_sdio_cfg_##inst = {\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),\
\
		.irq = DT_INST_IRQN(inst),\
		.num_funcs = DT_INST_PROP(inst, num_funcs),\
		.vendor_id = DT_INST_PROP(inst, vendor_id),\
		.device_id = DT_INST_PROP(inst, device_id),\
\
		.cpu_num = 4,\
		.used_port_num = 1,\
		.cmd_tx_format = 1,\
		.cmd_rd_format = 1,\
		.data_tx_format = 1,\
		.data_rd_format = 1,\
\
		.sd_dev_cfg =\
			{\
				.card_type = SDIO_DEVICE_CARD,\
				.sdio_cfg =\
					{\
						.cccr_addr = DT_INST_REG_ADDR(inst),\
						.func_bitmap =\
							BIT_MASK(DT_INST_PROP(inst, num_funcs)),\
						.cccr_version = SDIO_CFG_UNSET,\
						.sdio_version = SDIO_CFG_UNSET,\
						.spec_version = SDIO_CFG_UNSET,\
						.bus_width = SDIO_CFG_UNSET,\
						.cspi_int = SDIO_CFG_UNSET,\
						.multi_block = SDIO_CFG_UNSET,\
						.low_speed = SDIO_CFG_UNSET,\
						.high_speed = SDIO_CFG_UNSET,\
						.uhs_mode = SDIO_CFG_UNSET,\
						.bus_speed = SDIO_CFG_UNSET,\
						.strength = SDIO_CFG_UNSET,\
						.async_intr_support = SDIO_CFG_UNSET,\
						.funcs = {\
							[1] = RW6XX_SDIO_FUNC1_CFG(inst),\
						},\
					},\
			},\
	};\
\
	IF_ENABLED(CONFIG_PM_DEVICE,\
		   (PM_DEVICE_DT_INST_DEFINE(inst,\
					     rw6xx_sdio_pm_action);))\
\
	DEVICE_DT_INST_DEFINE(inst, rw6xx_sdio_init,\
			      COND_CODE_1(CONFIG_PM_DEVICE,\
					  (PM_DEVICE_DT_INST_GET(inst)),\
					  (NULL)), &rw6xx_sdio_data_##inst,\
							     &rw6xx_sdio_cfg_##inst, POST_KERNEL,\
							     CONFIG_KERNEL_INIT_PRIORITY_DEVICE,\
							     &rw6xx_sdio_api);

DT_INST_FOREACH_STATUS_OKAY(RW6XX_SDIO_DEVICE_DEFINE)
