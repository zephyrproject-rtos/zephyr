/*
 * Copyright (c) 2024 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_i2c

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(renesas_ra_i2c);

#include "i2c-priv.h"

struct i2c_ra_cfg {
	mem_addr_t regs;
	const struct device *clock_dev;
	clock_control_subsys_t clock_id;
	const struct pinctrl_dev_config *pcfg;
	uint32_t bitrate;
};

#define REG_MASK(reg) (BIT_MASK(_CONCAT(reg, _LEN)) << _CONCAT(reg, _POS))

/* Registers */
#define ICCR1  0x00 /*!< I2C Bus Control Register 1           */
#define ICCR2  0x01 /*!< I2C Bus Control Register 2           */
#define ICMR1  0x02 /*!< I2C Bus Mode Register 1           */
#define ICMR2  0x03 /*!< I2C Bus Mode Register 2           */
#define ICMR3  0x04 /*!< I2C Bus Mode Register 3           */
#define ICFER  0x05 /*!< I2C Bus Function Enable Register */
#define ICSER  0x06 /*!< I2C Bus Status Enable Register */
#define ICIER  0x07 /*!< I2C Bus Interrupt Enable Register */
#define ICSR1  0x08 /*!< I2C Bus Status Register 1 */
#define ICSR2  0x09 /*!< I2C Bus Status Register 2 */
#define ICWUR  0x16 /*!< I2C Bus Wakeup Unit Register */
#define ICWUR2 0x17 /*!< I2C Bus Wakeup Unit Register 2 */
#define SARL0  0x0A /*!< Slave Address Register L0            */
#define SARU0  0x0B /*!< Slave Address Register U0            */
#define SARL1  0x0C /*!< Slave Address Register L1            */
#define SARU1  0x0D /*!< Slave Address Register U1            */
#define SARL2  0x0E /*!< Slave Address Register L2            */
#define SARU2  0x0F /*!< Slave Address Register U2            */
#define ICBRL  0x10 /*!< I2C Bus Bit Rate Low-Level Register */
#define ICBRH  0x11 /*!< I2C Bus Bit Rate High-Level Register */
#define ICDRT  0x12 /*!< I2C Bus Transmit Data Register */
#define ICDRR  0x13 /*!< I2C Bus Receive Data Register */

/**
 * ICCR1  (I2C Bus Control Register 1)
 *
 * - CKE[0..2]:  Clock Enable
 * - TEIE[2..3]: Transmit End Interrupt Enable
 * - MPIE[3..4]: Multi-Processor Interrupt Enable (Valid in asynchronous
 * - RE[4..5]:   Receive Enable
 * - TE[5..6]:   Transmit Enable
 * - RIE[6..7]:  Receive Interrupt Enable
 * - TIE[7..8]:  Transmit Interrupt Enable
 */
#define ICCR1_SDAI_POS   (0)
#define ICCR1_SDAI_LEN   (1)
#define ICCR1_SCLI_POS   (1)
#define ICCR1_SCLI_LEN   (1)
#define ICCR1_SDAO_POS   (2)
#define ICCR1_SDAO_LEN   (1)
#define ICCR1_SCLO_POS   (3)
#define ICCR1_SCLO_LEN   (1)
#define ICCR1_SOWP_POS   (4)
#define ICCR1_SOWP_LEN   (1)
#define ICCR1_CLO_POS    (5)
#define ICCR1_CLO_LEN    (1)
#define ICCR1_IICRST_POS (6)
#define ICCR1_IICRST_LEN (1)
#define ICCR1_ICE_POS    (7)
#define ICCR1_ICE_LEN    (1)

/**
 * ICCR2  (I2C Bus Control Register 2)
 *
 * - ST[1..2]:   Start Condition Issuance Request
 * - RS[2..3]:   Restart Condition Issuance Request
 * - SP[3..4]:   Stop Condition Issuance Request
 * - TRS[5..6]:  Transmit/Receive Mode
 * - MST[6..7]:  Master/Slave Mode
 * - BBSY[7..8]  Bus Busy Detection Flag
 */
#define ICCR2_ST_POS   (1)
#define ICCR2_ST_LEN   (1)
#define ICCR2_RS_POS   (2)
#define ICCR2_RS_LEN   (1)
#define ICCR2_SP_POS   (3)
#define ICCR2_SP_LEN   (1)
#define ICCR2_TRS_POS  (5)
#define ICCR2_TRS_LEN  (1)
#define ICCR2_MST_POS  (6)
#define ICCR2_MST_LEN  (1)
#define ICCR2_BBSY_POS (7)
#define ICCR2_BBSY_LEN (1)

#define ICMR1_BC_POS   (0)
#define ICMR1_BC_LEN   (3)
#define ICMR1_BCWP_POS (3)
#define ICMR1_BCWP_LEN (1)
#define ICMR1_CKS_POS  (4)
#define ICMR1_CKS_LEN  (3)
#define ICMR1_MTWP_POS (7)
#define ICMR1_MTWP_LEN (1)

#define ICMR2_TMOS_POS (0)
#define ICMR2_TMOS_LEN (1)
#define ICMR2_TMOL_POS (1)
#define ICMR2_TMOL_LEN (1)
#define ICMR2_TMOH_POS (2)
#define ICMR2_TMOH_LEN (1)
#define ICMR2_SDDL_POS (4)
#define ICMR2_SDDL_LEN (3)
#define ICMR2_DLCS_POS (7)
#define ICMR2_DLCS_LEN (1)

#define ICMR3_NF_POS    (0)
#define ICMR3_NF_LEN    (2)
#define ICMR3_ACKBR_POS (2)
#define ICMR3_ACKBR_LEN (1)
#define ICMR3_ACKBT_POS (3)
#define ICMR3_ACKBT_LEN (1)
#define ICMR3_ACKWP_POS (4)
#define ICMR3_ACKWP_LEN (1)
#define ICMR3_RDRFS_POS (5)
#define ICMR3_RDRFS_LEN (1)
#define ICMR3_WAIT_POS  (6)
#define ICMR3_WAIT_LEN  (1)
#define ICMR3_SMBS_POS  (7)
#define ICMR3_SMBS_LEN  (1)

#define ICFER_TMOE_POS  (0)
#define ICFER_TMOE_LEN  (1)
#define ICFER_MALE_POS  (1)
#define ICFER_MALE_LEN  (1)
#define ICFER_NALE_POS  (2)
#define ICFER_NALE_LEN  (1)
#define ICFER_SALE_POS  (3)
#define ICFER_SALE_LEN  (1)
#define ICFER_NACKE_POS (4)
#define ICFER_NACKE_LEN (1)
#define ICFER_NFE_POS   (5)
#define ICFER_NFE_LEN   (1)
#define ICFER_SCLE_POS  (6)
#define ICFER_SCLE_LEN  (1)

#define ICSER_SAR0E_POS (0)
#define ICSER_SAR0E_LEN (1)
#define ICSER_SAR1E_POS (1)
#define ICSER_SAR1E_LEN (1)
#define ICSER_SAR2E_POS (2)
#define ICSER_SAR2E_LEN (1)
#define ICSER_GCAE_POS  (3)
#define ICSER_GCAE_LEN  (1)
#define ICSER_DIDE_POS  (5)
#define ICSER_DIDE_LEN  (1)
#define ICSER_HOAE_POS  (7)
#define ICSER_HOAE_LEN  (1)

#define ICIER_TMOIE_POS (0)
#define ICIER_TMOIE_LEN (1)
#define ICIER_ALIE_POS  (1)
#define ICIER_ALIE_LEN  (1)
#define ICIER_STIE_POS  (2)
#define ICIER_STIE_LEN  (1)
#define ICIER_SPIE_POS  (3)
#define ICIER_SPIE_LEN  (1)
#define ICIER_NAKIE_POS (4)
#define ICIER_NAKIE_LEN (1)
#define ICIER_RIE_POS   (5)
#define ICIER_RIE_LEN   (1)
#define ICIER_TEIE_POS  (6)
#define ICIER_TEIE_LEN  (1)
#define ICIER_TIE_POS   (7)
#define ICIER_TIE_LEN   (1)

#define ICSR1_AAS0_POS (0)
#define ICSR1_AAS0_LEN (1)
#define ICSR1_AAS1_POS (1)
#define ICSR1_AAS1_LEN (1)
#define ICSR1_AAS2_POS (2)
#define ICSR1_AAS2_LEN (1)
#define ICSR1_GCA_POS  (3)
#define ICSR1_GCA_LEN  (1)
#define ICSR1_DID_POS  (5)
#define ICSR1_DID_LEN  (1)
#define ICSR1_HOA_POS  (7)
#define ICSR1_HOA_LEN  (1)

#define ICSR2_TMOF_POS  (0)
#define ICSR2_TMOF_LEN  (1)
#define ICSR2_AL_POS    (1)
#define ICSR2_AL_LEN    (1)
#define ICSR2_START_POS (2)
#define ICSR2_START_LEN (1)
#define ICSR2_STOP_POS  (3)
#define ICSR2_STOP_LEN  (1)
#define ICSR2_NACKF_POS (4)
#define ICSR2_NACKF_LEN (1)
#define ICSR2_RDRF_POS  (5)
#define ICSR2_RDRF_LEN  (1)
#define ICSR2_TEND_POS  (6)
#define ICSR2_TEND_LEN  (1)
#define ICSR2_TDRE_POS  (7)
#define ICSR2_TDRE_LEN  (1)

#define ICWUR_WUAFA_POS (0)
#define ICWUR_WUAFA_LEN (1)
#define ICWUR_WUACK_POS (4)
#define ICWUR_WUACK_LEN (1)
#define ICWUR_WUF_POS   (5)
#define ICWUR_WUF_LEN   (1)
#define ICWUR_WUIE_POS  (6)
#define ICWUR_WUIE_LEN  (1)
#define ICWUR_WUE_POS   (7)
#define ICWUR_WUE_LEN   (1)

#define ICWUR2_WUSEN_POS  (0)
#define ICWUR2_WUSEN_LEN  (1)
#define ICWUR2_WUASYF_POS (1)
#define ICWUR2_WUASYF_LEN (1)
#define ICWUR2_WUSYF_POS  (2)
#define ICWUR2_WUSYF_LEN  (1)

#define SARL0_SVA0_POS (0)
#define SARL0_SVA0_LEN (1)
#define SARL0_SVA_POS  (1)
#define SARL0_SVA_LEN  (7)
#define SARL1_SVA0_POS SARL0_SVA0_POS
#define SARL1_SVA0_LEN SARL0_SVA0_LEN
#define SARL1_SVA_POS  SARL0_SVA_POS
#define SARL1_SVA_LEN  SARL0_SVA_LEN
#define SARL2_SVA0_POS SARL0_SVA0_POS
#define SARL2_SVA0_LEN SARL0_SVA0_LEN
#define SARL2_SVA_POS  SARL0_SVA_POS
#define SARL2_SVA_LEN  SARL0_SVA_LEN

#define SARU0_FS_POS  (0)
#define SARU0_FS_LEN  (1)
#define SARU0_SVA_POS (1)
#define SARU0_SVA_LEN (2)
#define SARU1_FS_POS  SARU0_FS_POS
#define SARU1_FS_LEN  SARU0_FS_LEN
#define SARU1_SVA_POS SARU0_SVA_POS
#define SARU1_SVA_LEN SARU0_SVA_LEN
#define SARU2_FS_POS  SARU0_FS_POS
#define SARU2_FS_LEN  SARU0_FS_LEN
#define SARU2_SVA_POS SARU0_SVA_POS
#define SARU2_SVA_LEN SARU0_SVA_LEN

#define ICBRL_BRL_POS      (0)
#define ICBRL_BRL_LEN      (5)
#define ICBRL_RESERVED_POS (5)
#define ICBRL_RESERVED_LEN (3)

#define ICBRH_BRL_POS      (0)
#define ICBRH_BRL_LEN      (5)
#define ICBRH_RESERVED_POS (5)
#define ICBRH_RESERVED_LEN (3)

#define ICCR1_DEFAULT (0x1f)

/* Helper Functions */

static inline uint8_t i2c_ra_read_8(const struct device *dev, uint32_t offs)
{
	const struct i2c_ra_cfg *config = dev->config;

	return sys_read8(config->regs + offs);
}

static inline void i2c_ra_write_8(const struct device *dev, uint32_t offs, uint8_t value)
{
	const struct i2c_ra_cfg *config = dev->config;

	sys_write8(value, config->regs + offs);
}

static inline uint8_t wait_for_turn_on(const struct device *dev, uint32_t offs, uint8_t bits)
{
	uint8_t regval;

	do {
		regval = i2c_ra_read_8(dev, offs);
	} while (!(regval & bits));

	return regval;
}

static inline uint8_t wait_for_turn_off(const struct device *dev, uint32_t offs, uint8_t bits)
{
	uint8_t regval;

	do {
		regval = i2c_ra_read_8(dev, offs);
	} while (regval & bits);

	return regval;
}

static int i2c_ra_process_msg_write(const struct device *dev, struct i2c_msg *msg, uint16_t addr)
{
	if (i2c_ra_read_8(dev, ICSR2) & REG_MASK(ICSR2_NACKF)) {
		return -1;
	}

	/* Wait TX Empty */
	wait_for_turn_on(dev, ICSR2, REG_MASK(ICSR2_TDRE));
	i2c_ra_write_8(dev, ICDRT, ((addr & 0x7F) << 1) | I2C_MSG_WRITE);

	for (uint32_t i = 0; i < msg->len; i++) {
		/* Wait TX Empty */
		wait_for_turn_on(dev, ICSR2, REG_MASK(ICSR2_TDRE));
		i2c_ra_write_8(dev, ICDRT, msg->buf[i]);

		if (i2c_ra_read_8(dev, ICSR2) & REG_MASK(ICSR2_NACKF)) {
			return -1;
		}
	}

	return 0;
}

static int i2c_ra_process_msg_read(const struct device *dev, struct i2c_msg *msg, uint16_t addr)
{
	uint8_t regval;

	/* Wait TX Empty */
	wait_for_turn_on(dev, ICSR2, REG_MASK(ICSR2_TDRE));
	i2c_ra_write_8(dev, ICDRT, ((addr & 0x7F) << 1) | I2C_MSG_READ);

	/* Wait response */
	wait_for_turn_on(dev, ICSR2, REG_MASK(ICSR2_RDRF));
	if (i2c_ra_read_8(dev, ICSR2) & REG_MASK(ICSR2_NACKF)) {
		return -1;
	}

	/* dummy read */
	(void)i2c_ra_read_8(dev, ICDRR);

	for (int i = 0; i < (msg->len - 1); i++) {
		/* Wait data */
		wait_for_turn_on(dev, ICSR2, REG_MASK(ICSR2_RDRF));

		if (i == (msg->len - 3)) {
			regval = i2c_ra_read_8(dev, ICMR3);
			i2c_ra_write_8(dev, ICMR3, regval & REG_MASK(ICMR3_WAIT));
		} else if (i == (msg->len - 2)) {
			/* Set ACKBT */
			regval = i2c_ra_read_8(dev, ICMR3);
			i2c_ra_write_8(dev, ICMR3,
				       regval | REG_MASK(ICMR3_ACKWP) | REG_MASK(ICMR3_ACKBT));
		}

		msg->buf[i] = i2c_ra_read_8(dev, ICDRR);
	}

	return 0;
}

static int i2c_ra_process_msg(const struct device *dev, struct i2c_msg *msg, uint16_t addr)
{
	uint8_t regval;
	int ret;

	if (msg->flags & I2C_MSG_RESTART) {
		/* Wait busy */
		if (!(i2c_ra_read_8(dev, ICCR2) & REG_MASK(ICCR2_BBSY))) {
			return -1;
		}

		/* set restart */
		i2c_ra_write_8(dev, ICCR2, REG_MASK(ICCR2_RS));

		wait_for_turn_off(dev, ICCR2, REG_MASK(ICCR2_RS));
	} else {
		/* Wait busy */
		wait_for_turn_off(dev, ICCR2, REG_MASK(ICCR2_BBSY));

		/* set start */
		i2c_ra_write_8(dev, ICCR2, REG_MASK(ICCR2_ST));

		wait_for_turn_off(dev, ICCR2, REG_MASK(ICCR2_ST));
	}

	if (msg->flags & I2C_MSG_READ) {
		ret = i2c_ra_process_msg_read(dev, msg, addr);
		wait_for_turn_on(dev, ICSR2, REG_MASK(ICSR2_RDRF));
	} else {
		ret = i2c_ra_process_msg_write(dev, msg, addr);
		wait_for_turn_on(dev, ICSR2, REG_MASK(ICSR2_TEND));
	}

	if (msg->flags & I2C_MSG_STOP) {
		regval = i2c_ra_read_8(dev, ICSR2);
		i2c_ra_write_8(dev, ICSR2, regval & ~REG_MASK(ICSR2_STOP));
		regval = i2c_ra_read_8(dev, ICCR2);
		i2c_ra_write_8(dev, ICCR2, REG_MASK(ICCR2_SP));
	}

	if (msg->flags & I2C_MSG_READ) {
		regval = i2c_ra_read_8(dev, ICDRR);

		if (ret == 0) {
			msg->buf[msg->len - 1] = regval;
		}

		regval = i2c_ra_read_8(dev, ICMR3);
		i2c_ra_write_8(dev, ICMR3, regval & ~REG_MASK(ICMR3_WAIT));
	}

	if (msg->flags & I2C_MSG_STOP) {
		/* Ensure stop */
		regval = wait_for_turn_on(dev, ICSR2, REG_MASK(ICSR2_STOP));

		i2c_ra_write_8(dev, ICSR2,
			       regval & ~(REG_MASK(ICSR2_STOP) | REG_MASK(ICSR2_NACKF)));
	}

	return ret;
}

static int i2c_ra_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			   uint16_t addr)
{
	int ret;

	for (int i = 0; i < num_msgs; i++) {
		ret = i2c_ra_process_msg(dev, &msgs[i], addr);

		if (ret != 0) {
			LOG_ERR("I2C failed to transfer messages\n");
			return ret;
		}
	}

	return 0;
};

static int i2c_ra_configure(const struct device *dev, uint32_t dev_config)
{
	uint8_t brh_value, brl_value;
	uint8_t cks;
	int ret = 0U;

	/* We only support Master mode */
	if ((dev_config & I2C_MODE_CONTROLLER) != I2C_MODE_CONTROLLER) {
		return -ENOTSUP;
	}

	/* We are not supporting 10-bit addressing */
	if ((dev_config & I2C_ADDR_10_BITS) == I2C_ADDR_10_BITS) {
		return -ENOTSUP;
	}

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		/* Use recommended value for 100 kHz bus */
		brh_value = 0x1a;
		brl_value = 0x1b;
		cks = 0x2;
		break;
	case I2C_SPEED_FAST:
		/* Use recommended value for 400 kHz bus */
		brh_value = 0x1a;
		brl_value = 0x1b;
		cks = 0x2;
		break;
	default:
		return -ENOTSUP;
	}

	/* Assert Reset */
	i2c_ra_write_8(dev, ICCR1, ICCR1_DEFAULT | REG_MASK(ICCR1_IICRST));
	i2c_ra_write_8(dev, ICCR1, ICCR1_DEFAULT | REG_MASK(ICCR1_IICRST) | REG_MASK(ICCR1_ICE));

	i2c_ra_write_8(dev, ICMR1, cks << ICMR1_CKS_POS);
	i2c_ra_write_8(dev, ICBRL, REG_MASK(ICBRL_RESERVED) | brl_value);
	i2c_ra_write_8(dev, ICBRH, REG_MASK(ICBRH_RESERVED) | brh_value);

	/* Disable slave address */
	i2c_ra_write_8(dev, ICSER, 0);
	/* Disable interrupts */
	i2c_ra_write_8(dev, ICIER, 0);

	/* Release Reset */
	i2c_ra_write_8(dev, ICCR1, ICCR1_DEFAULT | REG_MASK(ICCR1_ICE));

	return ret;
}

static const struct i2c_driver_api i2c_api = {
	.configure = i2c_ra_configure,
	.transfer = i2c_ra_transfer,
};

static int i2c_ra_init(const struct device *dev)
{
	const struct i2c_ra_cfg *config = dev->config;
	int ret;

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	ret = clock_control_on(config->clock_dev, config->clock_id);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_ra_configure(dev, I2C_MODE_CONTROLLER | i2c_map_dt_bitrate(config->bitrate));
	if (ret != 0) {
		return ret;
	}

	return 0;
}

#define I2C_RA_INIT(n)                                                                             \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static struct i2c_ra_cfg i2c_config_##n = {                                                \
		.regs = DT_INST_REG_ADDR(n),                                                       \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_id = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_IDX(n, 0, id),          \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.bitrate = DT_INST_PROP(n, clock_frequency),                                       \
	};                                                                                         \
	I2C_DEVICE_DT_INST_DEFINE(n, i2c_ra_init, NULL, NULL, &i2c_config_##n, POST_KERNEL,        \
				  CONFIG_I2C_INIT_PRIORITY, &i2c_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_RA_INIT)
