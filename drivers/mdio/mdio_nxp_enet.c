/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_enet_mdio

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/net/mdio.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/drivers/ethernet/eth_nxp_enet.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys_clock.h>

struct nxp_enet_mdio_config {
	const struct pinctrl_dev_config *pincfg;
	const struct device *module_dev;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	uint32_t mdc_freq;
	bool disable_preamble;
};

struct nxp_enet_mdio_data {
	ENET_Type *base;
	struct k_mutex mdio_mutex;
	struct k_sem mdio_sem;
	bool interrupt_up;
};

/*
 * This function is used for both read and write operations
 * in order to wait for the completion of an MDIO transaction.
 * It returns -ETIMEDOUT if timeout occurs as specified in DT,
 * otherwise returns 0 if EIR MII bit is set indicting completed
 * operation, otherwise -EIO.
 */
static int nxp_enet_mdio_wait_xfer(const struct device *dev)
{
	struct nxp_enet_mdio_data *data = dev->data;

	/* This function will not make sense from IRQ context */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	if (!data->interrupt_up) {
		/* If the interrupt is not available to use yet, just busy wait */
		k_busy_wait(CONFIG_MDIO_NXP_ENET_TIMEOUT);
		k_sem_give(&data->mdio_sem);
	}

	/* Wait for the MDIO transaction to finish or time out */
	k_sem_take(&data->mdio_sem, K_USEC(CONFIG_MDIO_NXP_ENET_TIMEOUT));

	return 0;
}

/* MDIO Read API implementation */
static int nxp_enet_mdio_read(const struct device *dev,
			uint8_t prtad, uint8_t regad, uint16_t *read_data)
{
	struct nxp_enet_mdio_data *data = dev->data;
	int ret;

	/* Only one MDIO bus operation attempt at a time */
	(void)k_mutex_lock(&data->mdio_mutex, K_FOREVER);

	/*
	 * Clear the bit (W1C) that indicates MDIO transfer is ready to
	 * prepare to wait for it to be set once this read is done
	 */
	data->base->EIR = ENET_EIR_MII_MASK;

	/*
	 * Write MDIO frame to MII management register which will
	 * send the read command and data out to the MDIO bus as this frame:
	 * ST = start, 1 means start
	 * OP = operation, 2 means read
	 * PA = PHY/Port address
	 * RA = Register/Device Address
	 * TA = Turnaround, must be 2 to be valid
	 * data = data to be written to the PHY register
	 */
	data->base->MMFR = ENET_MMFR_ST(0x1U) |
				ENET_MMFR_OP(MDIO_OP_C22_READ) |
				ENET_MMFR_PA(prtad) |
				ENET_MMFR_RA(regad) |
				ENET_MMFR_TA(0x2U);

	ret = nxp_enet_mdio_wait_xfer(dev);
	if (ret) {
		(void)k_mutex_unlock(&data->mdio_mutex);
		return ret;
	}

	/* The data is received in the same register that we wrote the command to */
	*read_data = (data->base->MMFR & ENET_MMFR_DATA_MASK) >> ENET_MMFR_DATA_SHIFT;

	/* Clear the same bit as before because the event has been handled */
	data->base->EIR = ENET_EIR_MII_MASK;

	/* This MDIO interaction is finished */
	(void)k_mutex_unlock(&data->mdio_mutex);

	return ret;
}

/* MDIO Write API implementation */
static int nxp_enet_mdio_write(const struct device *dev,
			uint8_t prtad, uint8_t regad, uint16_t write_data)
{
	struct nxp_enet_mdio_data *data = dev->data;
	int ret;

	/* Only one MDIO bus operation attempt at a time */
	(void)k_mutex_lock(&data->mdio_mutex, K_FOREVER);

	/*
	 * Clear the bit (W1C) that indicates MDIO transfer is ready to
	 * prepare to wait for it to be set once this write is done
	 */
	data->base->EIR = ENET_EIR_MII_MASK;

	/*
	 * Write MDIO frame to MII management register which will
	 * send the write command and data out to the MDIO bus as this frame:
	 * ST = start, 1 means start
	 * OP = operation, 1 means write
	 * PA = PHY/Port address
	 * RA = Register/Device Address
	 * TA = Turnaround, must be 2 to be valid
	 * data = data to be written to the PHY register
	 */
	data->base->MMFR = ENET_MMFR_ST(0x1U) |
				ENET_MMFR_OP(MDIO_OP_C22_WRITE) |
				ENET_MMFR_PA(prtad) |
				ENET_MMFR_RA(regad) |
				ENET_MMFR_TA(0x2U) |
				write_data;

	ret = nxp_enet_mdio_wait_xfer(dev);
	if (ret) {
		(void)k_mutex_unlock(&data->mdio_mutex);
		return ret;
	}

	/* Clear the same bit as before because the event has been handled */
	data->base->EIR = ENET_EIR_MII_MASK;

	/* This MDIO interaction is finished */
	(void)k_mutex_unlock(&data->mdio_mutex);

	return ret;
}

static const struct mdio_driver_api nxp_enet_mdio_api = {
	.read = nxp_enet_mdio_read,
	.write = nxp_enet_mdio_write,
};

static void nxp_enet_mdio_isr_cb(const struct device *dev)
{
	struct nxp_enet_mdio_data *data = dev->data;

	data->base->EIR = ENET_EIR_MII_MASK;

	k_sem_give(&data->mdio_sem);
}

static void nxp_enet_mdio_post_module_reset_init(const struct device *dev)
{
	const struct nxp_enet_mdio_config *config = dev->config;
	struct nxp_enet_mdio_data *data = dev->data;
	uint32_t enet_module_clock_rate;

	/* Set up MSCR register */
	(void) clock_control_get_rate(config->clock_dev, config->clock_subsys,
							&enet_module_clock_rate);
	uint32_t mii_speed = (enet_module_clock_rate + 2 * config->mdc_freq - 1) /
					(2 * config->mdc_freq) - 1;
	uint32_t holdtime = (10 + NSEC_PER_SEC / enet_module_clock_rate - 1) /
					(NSEC_PER_SEC / enet_module_clock_rate) - 1;
	uint32_t mscr = ENET_MSCR_MII_SPEED(mii_speed) | ENET_MSCR_HOLDTIME(holdtime) |
			(config->disable_preamble ? ENET_MSCR_DIS_PRE_MASK : 0);
	data->base->MSCR = mscr;
}

void nxp_enet_mdio_callback(const struct device *dev,
				enum nxp_enet_callback_reason event, void *cb_data)
{
	struct nxp_enet_mdio_data *data = dev->data;

	ARG_UNUSED(cb_data);

	switch (event) {
	case NXP_ENET_MODULE_RESET:
		nxp_enet_mdio_post_module_reset_init(dev);
		break;
	case NXP_ENET_INTERRUPT:
		nxp_enet_mdio_isr_cb(dev);
		break;
	case NXP_ENET_INTERRUPT_ENABLED:
		/* IRQ was enabled in NVIC, now enable in enet */
		data->interrupt_up = true;
		data->base->EIMR |= ENET_EIMR_MII_MASK;
		break;
	default:
		break;
	}
}

static int nxp_enet_mdio_init(const struct device *dev)
{
	const struct nxp_enet_mdio_config *config = dev->config;
	struct nxp_enet_mdio_data *data = dev->data;
	int ret = 0;

	data->base = (ENET_Type *)DEVICE_MMIO_GET(config->module_dev);

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}

	ret = k_mutex_init(&data->mdio_mutex);
	if (ret) {
		return ret;
	}

	ret = k_sem_init(&data->mdio_sem, 0, 1);
	if (ret) {
		return ret;
	}

	/* All operations done after module reset should be done during device init too */
	nxp_enet_mdio_post_module_reset_init(dev);

	return ret;
}

#define NXP_ENET_MDIO_INIT(inst)							\
	PINCTRL_DT_INST_DEFINE(inst);							\
											\
	static const struct nxp_enet_mdio_config nxp_enet_mdio_cfg_##inst = {		\
		.module_dev = DEVICE_DT_GET(DT_INST_PARENT(inst)),			\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),				\
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_INST_PARENT(inst))),	\
		.clock_subsys = (void *) DT_CLOCKS_CELL_BY_IDX(				\
							DT_INST_PARENT(inst), 0, name),	\
		.disable_preamble = DT_INST_PROP(inst, suppress_preamble),		\
		.mdc_freq = DT_INST_PROP(inst, clock_frequency),			\
	};										\
											\
	static struct nxp_enet_mdio_data nxp_enet_mdio_data_##inst;			\
											\
	DEVICE_DT_INST_DEFINE(inst, &nxp_enet_mdio_init, NULL,				\
			      &nxp_enet_mdio_data_##inst, &nxp_enet_mdio_cfg_##inst,	\
			      POST_KERNEL, CONFIG_MDIO_INIT_PRIORITY,			\
			      &nxp_enet_mdio_api);


DT_INST_FOREACH_STATUS_OKAY(NXP_ENET_MDIO_INIT)
