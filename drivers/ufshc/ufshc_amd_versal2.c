/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/ufshc/ufshc.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/ufs/ufs.h>
#include <zephyr/ufs/unipro.h>

LOG_MODULE_REGISTER(ufshc_amd_versal2, CONFIG_UFSHC_LOG_LEVEL);

#define DT_DRV_COMPAT amd_versal2_ufs

/* UFS Clock Divider Register offset */
#define VERSAL2_UFS_REG_HCLKDIV_OFFSET			0xFCU

/* UFS Reset Register offset */
#define VERSAL2_UFS_CRP_RST_OFFSET			0x340U

/* UFS Calibration Efuse Register offset */
#define VERSAL2_UFS_EFUSE_CAL_OFFSET			0xBE8U

/* SRAM Control and Status Register (CSR) offset */
#define VERSAL2_UFS_IOU_SLCR_SRAM_CSR_OFFSET		0x104CU

#define VERSAL2_UFS_SRAM_CSR_BYPASS_MASK		0x4U
#define VERSAL2_UFS_SRAM_CSR_EXTID_DONE_MASK		0x2U
#define VERSAL2_UFS_SRAM_CSR_INIT_DONE_MASK		0x1U

/* PHY Reset Register offset */
#define VERSAL2_UFS_IOU_SLCR_PHY_RST_OFFSET		0x1050U

/* Transmit/Receive Config Ready Register */
#define VERSAL2_UFS_IOU_SLCR_TX_RX_CFGRDY_OFFSET	0x1054U

#define VERSAL2_UFS_TX_RX_CFGRDY_MASK			GENMASK(3, 0)

/* RMMI Attributes */
#define CBREFCLKCTRL2					0x8132
#define CBCRCTRL					0x811F
#define CBC10DIRECTCONF2				0x810E
#define CBCREGADDRLSB					0x8116
#define CBCREGADDRMSB					0x8117
#define CBCREGWRLSB					0x8118
#define CBCREGWRMSB					0x8119
#define CBCREGRDLSB					0x811A
#define CBCREGRDMSB					0x811B
#define CBCREGRDWRSEL					0x811C

#define CBREFREFCLK_GATE_OVR_EN				BIT(7)

/* M-PHY Attributes */
#define MTX_FSM_STATE					0x41
#define MRX_FSM_STATE					0xC1

#define VERSAL2_UFS_HIBERN8_STATE			0x1U
#define VERSAL2_UFS_SLEEP_STATE				0x2U
#define VERSAL2_UFS_LS_BURST_STATE			0x4U

/* M-PHY registers */
#define FAST_FLAGS(n)				(0x401C + ((n) * 0x100))
#define RX_AFE_ATT_IDAC(n)			(0x4000 + ((n) * 0x100))
#define RX_AFE_CTLE_IDAC(n)			(0x4001 + ((n) * 0x100))
#define FW_CALIB_CCFG(n)			(0x404D + ((n) * 0x100))

#define MPHY_FAST_RX_AFE_CAL				BIT(2)
#define MPHY_FW_CALIB_CFG_VAL				BIT(8)

/**
 * @brief Configuration for the Versal Gen2 UFS Host Controller.
 */
struct ufshc_versal2_config {
	mem_addr_t mmio_base;       /**< Base address for the UFS controller memory-mapped I/O */
	uint32_t core_clk_rate;     /**< UFS core clock rate in Hz */
	uint32_t irq_id;            /**< IRQ line for the UFS controller interrupt */
	mem_addr_t reg_iou_slcr;    /**< IOU SLCR register address for UFS configuration */
	mem_addr_t reg_efuse_cache; /**< eFuse cache register address */
	mem_addr_t reg_ufs_crp;     /**< UFS CRP register address */
};

/**
 * @brief Runtime data for the Versal Gen2 UFS Host Controller.
 */
struct ufshc_versal2_data {
	struct ufs_host_controller ufshc; /**< UFS host controller structure */
	uint32_t rx_att_comp_val_l0;      /**< Receive AFE compensation value for lane 0 */
	uint32_t rx_att_comp_val_l1;      /**< Receive AFE compensation value for lane 1 */
	uint32_t rx_ctle_comp_val_l0;     /**< Receive CTLE compensation value for lane 0 */
	uint32_t rx_ctle_comp_val_l1;     /**< Receive CTLE compensation value for lane 1 */
};

/**
 * @brief Perform variant-specific initialization of the UFS Host Controller.
 *
 * This function performs a sequence of initialization steps
 * including asserting resets, configuring the SRAM and clock dividers,
 * and programming the necessary calibration values.
 *
 * @param dev Pointer to the UFS device handle.
 */
static void ufshc_versal2_initialization(const struct device *dev)
{
	const struct ufshc_versal2_config *cfg = dev->config;
	struct ufshc_versal2_data *drvdata = dev->data;
	struct ufs_host_controller *ufshc = &drvdata->ufshc;
	uint32_t read_reg;

	/* Assert UFS Host Controller Reset */
	sys_write32((1U), ((cfg->reg_ufs_crp) + (mem_addr_t)(VERSAL2_UFS_CRP_RST_OFFSET)));

	/* Assert PHY Reset */
	sys_write32((1U),
		    ((cfg->reg_iou_slcr) +
		    (mem_addr_t)(VERSAL2_UFS_IOU_SLCR_PHY_RST_OFFSET)));

	/* Set ROM Mode (SRAM Bypass) */
	read_reg = sys_read32(((cfg->reg_iou_slcr) +
			      (mem_addr_t)(VERSAL2_UFS_IOU_SLCR_SRAM_CSR_OFFSET)));
	read_reg |= VERSAL2_UFS_SRAM_CSR_BYPASS_MASK;
	read_reg &= ~VERSAL2_UFS_SRAM_CSR_EXTID_DONE_MASK;
	sys_write32((read_reg),
		    ((cfg->reg_iou_slcr) +
		    (mem_addr_t)(VERSAL2_UFS_IOU_SLCR_SRAM_CSR_OFFSET)));

	/* Release UFS Host Controller Reset */
	sys_write32((0U),
		    ((cfg->reg_ufs_crp) +
		    (mem_addr_t)(VERSAL2_UFS_CRP_RST_OFFSET)));

	/* Program the HCLK_DIV based on the input reference clock */
	ufshc_write_reg(ufshc,
			VERSAL2_UFS_REG_HCLKDIV_OFFSET,
			(cfg->core_clk_rate / 1000000U));

	/* Read the calibration values from the eFuse cache */
	read_reg = sys_read32(((cfg->reg_efuse_cache) +
			      (mem_addr_t)(VERSAL2_UFS_EFUSE_CAL_OFFSET)));
	drvdata->rx_att_comp_val_l0 = (uint8_t)read_reg;
	drvdata->rx_att_comp_val_l1 = (uint8_t)(read_reg >> 8U);
	drvdata->rx_ctle_comp_val_l0 = (uint8_t)(read_reg >> 16U);
	drvdata->rx_ctle_comp_val_l1 = (uint8_t)(read_reg >> 24U);
}

/**
 * @brief Notify the UFS controller about link startup.
 *
 * This function sends a UIC command to change the connection state.
 *
 * @param dev Pointer to the UFS device handle.
 * @param status Notification status (pre-change or post-change).
 *
 * @return 0 on success, or a negative error code on failure.
 */
static int32_t ufshc_versal2_link_startup_notify(const struct device *dev,
						 uint8_t status)
{
	struct ufshc_versal2_data *drvdata = dev->data;
	struct ufs_host_controller *ufshc = &drvdata->ufshc;
	struct ufshc_uic_cmd uic_cmd = {0};
	int32_t err = 0;

	if (status == (uint8_t)POST_CHANGE) {
		/* change the connection state to the ready state */
		ufshc_fill_uic_cmd(&uic_cmd,
				   ((uint32_t)T_CONNECTIONSTATE << 16U),
				   1U, 0U,
				   (uint32_t)UFSHC_DME_SET_OPCODE);
		err = ufshc_send_uic_cmd(ufshc, &uic_cmd);
		if (err != 0) {
			LOG_ERR("Connection setup failed (%d)", err);
		}
	}

	return err;
}

/**
 * @brief Configures RMMI (Remote Memory-Mapped Interface).
 *
 * This function configures the UFS host controller's RMMI settings by
 * issuing a series of UIC commands to the controller.
 *
 * @param ufshc Pointer to the UFS host controller driver structure.
 *
 * @return 0 (success) or non-zero value on failure.
 */
static int32_t ufshc_versal2_set_rmmi(struct ufs_host_controller *ufshc)
{
	struct ufshc_uic_cmd uic_cmd = {0};
	int32_t ret = 0;

	/* Configure the clock reference control to enable ref clock gating */
	ufshc_fill_uic_cmd(&uic_cmd,
			   ((uint32_t)CBREFCLKCTRL2 << 16U),
			   (uint32_t)CBREFREFCLK_GATE_OVR_EN, 0U,
			   (uint32_t)UFSHC_DME_SET_OPCODE);
	ret = ufshc_send_uic_cmd(ufshc, &uic_cmd);
	if (ret != 0) {
		goto out;
	}

	/* Set specific clock configuration */
	ufshc_fill_uic_cmd(&uic_cmd,
			   ((uint32_t)CBCRCTRL << 16U),
			   1U, 0U,
			   (uint32_t)UFSHC_DME_SET_OPCODE);
	ret = ufshc_send_uic_cmd(ufshc, &uic_cmd);
	if (ret != 0) {
		goto out;
	}

	/* Configure for direct PHY interface mode */
	ufshc_fill_uic_cmd(&uic_cmd,
			   ((uint32_t)CBC10DIRECTCONF2 << 16U),
			   1U, 0U,
			   (uint32_t)UFSHC_DME_SET_OPCODE);
	ret = ufshc_send_uic_cmd(ufshc, &uic_cmd);
	if (ret != 0) {
		goto out;
	}

	/* Update the M-PHY configuration to apply changes */
	ufshc_fill_uic_cmd(&uic_cmd,
			   ((uint32_t)VS_MPHYCFGUPDT << 16U),
			   1U, 0U,
			   (uint32_t)UFSHC_DME_SET_OPCODE);
	ret = ufshc_send_uic_cmd(ufshc, &uic_cmd);

out:
	return ret;
}

/**
 * @brief Writes to mPHY registers.
 *
 * This function writes a 16-bit value to the mPHY register specified by the address.
 *
 * @param ufshc Pointer to the UFS host controller driver structure.
 * @param uic_cmd_ptr Pointer to the UIC command structure to be filled with commands.
 * @param address The address of the mPHY register to write.
 * @param value The value to write to the specified mPHY register.
 *
 * @return 0 (success) or non-zero value on failure.
 */
static int32_t ufshc_versal2_write_phy_reg(struct ufs_host_controller *ufshc,
	struct ufshc_uic_cmd *uic_cmd_ptr,
	uint32_t address, uint32_t value)
{
	int32_t ret = 0;

	/* Write the least significant byte of the register address */
	ufshc_fill_uic_cmd(uic_cmd_ptr,
			   ((uint32_t)CBCREGADDRLSB << 16U),
			   (uint8_t)address, 0U,
			   (uint32_t)UFSHC_DME_SET_OPCODE);
	ret = ufshc_send_uic_cmd(ufshc, uic_cmd_ptr);
	if (ret != 0) {
		goto out;
	}

	/* Write the most significant byte of the register address */
	ufshc_fill_uic_cmd(uic_cmd_ptr,
			   ((uint32_t)CBCREGADDRMSB << 16U),
			   (uint8_t)(address >> 8U), 0U,
			   (uint32_t)UFSHC_DME_SET_OPCODE);
	ret = ufshc_send_uic_cmd(ufshc, uic_cmd_ptr);
	if (ret != 0) {
		goto out;
	}

	/* Write the least significant byte of the register value */
	ufshc_fill_uic_cmd(uic_cmd_ptr,
			   ((uint32_t)CBCREGWRLSB << 16U),
			   (uint8_t)value, 0U,
			   (uint32_t)UFSHC_DME_SET_OPCODE);
	ret = ufshc_send_uic_cmd(ufshc, uic_cmd_ptr);
	if (ret != 0) {
		goto out;
	}

	/* Write the most significant byte of the register value */
	ufshc_fill_uic_cmd(uic_cmd_ptr,
			   ((uint32_t)CBCREGWRMSB << 16U),
			   (uint8_t)(value >> 8U), 0U,
			   (uint32_t)UFSHC_DME_SET_OPCODE);
	ret = ufshc_send_uic_cmd(ufshc, uic_cmd_ptr);
	if (ret != 0) {
		goto out;
	}

	/* Set the read/write select bit in the register */
	ufshc_fill_uic_cmd(uic_cmd_ptr,
			   ((uint32_t)CBCREGRDWRSEL << 16U),
			   1U, 0U,
			   (uint32_t)UFSHC_DME_SET_OPCODE);
	ret = ufshc_send_uic_cmd(ufshc, uic_cmd_ptr);
	if (ret != 0) {
		goto out;
	}

	/* Update the M-PHY configuration to apply changes */
	ufshc_fill_uic_cmd(uic_cmd_ptr,
			   ((uint32_t)VS_MPHYCFGUPDT << 16U),
			   1U, 0U,
			   (uint32_t)UFSHC_DME_SET_OPCODE);
	ret = ufshc_send_uic_cmd(ufshc, uic_cmd_ptr);

out:
	return ret;
}

/**
 * @brief Reads from mPHY registers.
 *
 * This function reads a 16-bit value from the mPHY register specified by the address.
 *
 * @param ufshc Pointer to the UFS host controller driver structure.
 * @param uic_cmd_ptr Pointer to the UIC command structure to be filled with commands.
 * @param address The address of the mPHY register to read.
 * @param value Pointer to the variable where the read value will be stored.
 *
 * @return 0 (success) or non-zero value on failure.
 */
static int32_t ufshc_versal2_read_phy_reg(struct ufs_host_controller *ufshc,
					  struct ufshc_uic_cmd *uic_cmd_ptr,
					  uint32_t address, uint32_t *value)
{
	int32_t ret = 0;

	/* Write the least significant byte of the register address */
	ufshc_fill_uic_cmd(uic_cmd_ptr,
			   ((uint32_t)CBCREGADDRLSB << 16U),
			   (uint8_t)address, 0U,
			   (uint32_t)UFSHC_DME_SET_OPCODE);
	ret = ufshc_send_uic_cmd(ufshc, uic_cmd_ptr);
	if (ret != 0) {
		goto out;
	}

	/* Write the most significant byte of the register address */
	ufshc_fill_uic_cmd(uic_cmd_ptr,
			   ((uint32_t)CBCREGADDRMSB << 16U),
			   (uint8_t)(address >> 8U), 0U,
			   (uint32_t)UFSHC_DME_SET_OPCODE);
	ret = ufshc_send_uic_cmd(ufshc, uic_cmd_ptr);
	if (ret != 0) {
		goto out;
	}

	/* Set the read/write select bit in the register */
	ufshc_fill_uic_cmd(uic_cmd_ptr,
			   ((uint32_t)CBCREGRDWRSEL << 16U),
			   0U, 0U,
			   (uint32_t)UFSHC_DME_SET_OPCODE);
	ret = ufshc_send_uic_cmd(ufshc, uic_cmd_ptr);
	if (ret != 0) {
		goto out;
	}

	/* Update the M-PHY configuration to apply changes */
	ufshc_fill_uic_cmd(uic_cmd_ptr,
			   ((uint32_t)VS_MPHYCFGUPDT << 16U),
			   1U, 0U,
			   (uint32_t)UFSHC_DME_SET_OPCODE);
	ret = ufshc_send_uic_cmd(ufshc, uic_cmd_ptr);
	if (ret != 0) {
		goto out;
	}

	/* Read the least significant byte of the register value */
	ufshc_fill_uic_cmd(uic_cmd_ptr,
			   ((uint32_t)CBCREGRDLSB << 16U),
			   0U, 0U,
			   (uint32_t)UFSHC_DME_GET_OPCODE);
	ret = ufshc_send_uic_cmd(ufshc, uic_cmd_ptr);
	if (ret != 0) {
		goto out;
	}

	/* Read the most significant byte of the register value */
	*value = uic_cmd_ptr->mib_value;
	ufshc_fill_uic_cmd(uic_cmd_ptr,
			   ((uint32_t)CBCREGRDMSB << 16U),
			   0U, 0U,
			   (uint32_t)UFSHC_DME_GET_OPCODE);
	ret = ufshc_send_uic_cmd(ufshc, uic_cmd_ptr);
	if (ret != 0) {
		goto out;
	}

	/* Combine to form the complete register value */
	*value |= (uic_cmd_ptr->mib_value << 8U);

out:
	return ret;
}

/**
 * @brief Configures and sets up UFS Versal Gen2 PHY
 *
 * This function is used to configure and set up the UFS Versal Gen2 PHY.
 * It starts with bypassing the RX-AFE (ATT/CTLE) offset calibrations,
 * then programs the ATT and CTLE compensation values.
 * Finally, it enables the RX-AFE calibration for Firmware control.
 *
 * @param dev A pointer to the device structure representing the UFS device.
 *
 * @return 0 if the setup is successful, or an error code in case of failure.
 */
static int32_t ufs_versal2_setup_phy(const struct device *dev)
{
	struct ufshc_versal2_data *drvdata = dev->data;
	struct ufs_host_controller *ufshc = &drvdata->ufshc;
	struct ufshc_uic_cmd uic_cmd = {0};
	int32_t ret = 0;
	uint32_t read_reg = 0U;

	/* Bypass RX-AFE Calibration for both lanes */
	ret = ufshc_versal2_read_phy_reg(ufshc, &uic_cmd,
					 FAST_FLAGS(0),
					 &read_reg);
	if (ret != 0) {
		return ret;
	}

	ret = ufshc_versal2_write_phy_reg(ufshc, &uic_cmd,
					  FAST_FLAGS(0),
					  (read_reg | (uint32_t)MPHY_FAST_RX_AFE_CAL));
	if (ret != 0) {
		return ret;
	}

	ret = ufshc_versal2_read_phy_reg(ufshc, &uic_cmd,
					 FAST_FLAGS(1),
					 &read_reg);
	if (ret != 0) {
		return ret;
	}

	ret = ufshc_versal2_write_phy_reg(ufshc, &uic_cmd,
					  FAST_FLAGS(1),
					  (read_reg | (uint32_t)MPHY_FAST_RX_AFE_CAL));
	if (ret != 0) {
		return ret;
	}

	/* Program ATT Compensation Value for RX */
	if ((drvdata->rx_att_comp_val_l0 != (uint32_t)0x0U)
	    && (drvdata->rx_att_comp_val_l0 != (uint32_t)0xFFU)) {
		ret = ufshc_versal2_write_phy_reg(ufshc, &uic_cmd,
						  RX_AFE_ATT_IDAC(0),
						  drvdata->rx_att_comp_val_l0);
		if (ret != 0) {
			return ret;
		}
	}

	if ((drvdata->rx_att_comp_val_l1 != (uint32_t)0x0U)
	    && (drvdata->rx_att_comp_val_l1 != (uint32_t)0xFFU)) {
		ret = ufshc_versal2_write_phy_reg(ufshc, &uic_cmd,
						  RX_AFE_ATT_IDAC(1),
						  drvdata->rx_att_comp_val_l1);
		if (ret != 0) {
			return ret;
		}
	}

	/* Program CTLE Compensation Value for RX */
	if ((drvdata->rx_ctle_comp_val_l0 != (uint32_t)0x0U)
	    && (drvdata->rx_ctle_comp_val_l0 != (uint32_t)0xFFU)) {
		ret = ufshc_versal2_write_phy_reg(ufshc, &uic_cmd,
						  RX_AFE_CTLE_IDAC(0),
						  drvdata->rx_ctle_comp_val_l0);
		if (ret != 0) {
			return ret;
		}
	}

	if ((drvdata->rx_ctle_comp_val_l1 != (uint32_t)0x0U)
	    && (drvdata->rx_ctle_comp_val_l1 != (uint32_t)0xFFU)) {
		ret = ufshc_versal2_write_phy_reg(ufshc, &uic_cmd,
						  RX_AFE_CTLE_IDAC(1),
						  drvdata->rx_ctle_comp_val_l1);
		if (ret != 0) {
			return ret;
		}
	}

	/* Program FW Calibration Config */
	ret = ufshc_versal2_read_phy_reg(ufshc, &uic_cmd,
					 FW_CALIB_CCFG(0),
					 &read_reg);
	if (ret != 0) {
		return ret;
	}

	ret = ufshc_versal2_write_phy_reg(ufshc, &uic_cmd,
					  FW_CALIB_CCFG(0),
					  (read_reg | (uint32_t)MPHY_FW_CALIB_CFG_VAL));
	if (ret != 0) {
		return ret;
	}

	ret = ufshc_versal2_read_phy_reg(ufshc, &uic_cmd,
					 FW_CALIB_CCFG(1),
					 &read_reg);
	if (ret != 0) {
		return ret;
	}

	ret = ufshc_versal2_write_phy_reg(ufshc, &uic_cmd,
					  FW_CALIB_CCFG(1),
					  (read_reg | (uint32_t)MPHY_FW_CALIB_CFG_VAL));

	return ret;
}

/**
 * @brief Enable the mphy for the Versal Gen2 UFS host controller.
 *
 * This function performs the necessary steps to enable the mphy in the Versal Gen2 UFS
 * host controller, including sending UIC commands to disable the mphy, update the mphy
 * configuration, and wait for the Rx/Tx state machines to de-assert, ensuring that the
 * controller is ready for further operations.
 *
 * @param ufshc Pointer to the UFS host controller structure.
 *
 * @return 0 (success) or non-zero value on failure.
 */
static int32_t ufshc_versal2_enable_mphy(struct ufs_host_controller *ufshc)
{
	struct ufshc_uic_cmd uic_cmd = {0};
	int32_t ret = 0;
	uint32_t time_out;
	uint32_t val;
	uint32_t index;

	/* Disable the mphy (de-assert mphy_disable) */
	ufshc_fill_uic_cmd(&uic_cmd,
			   ((uint32_t)VS_MPHYDISABLE << 16U),
			   0U, 0U,
			   (uint32_t)UFSHC_DME_SET_OPCODE);
	ret = ufshc_send_uic_cmd(ufshc, &uic_cmd);
	if (ret != 0) {
		goto out;
	}

	/* Update the mphy configuration with new settings */
	ufshc_fill_uic_cmd(&uic_cmd,
			   ((uint32_t)VS_MPHYCFGUPDT << 16U),
			   1U, 0U,
			   (uint32_t)UFSHC_DME_SET_OPCODE);
	ret = ufshc_send_uic_cmd(ufshc, &uic_cmd);
	if (ret != 0) {
		goto out;
	}

	/* Wait for both Tx and Rx state machines to exit busy state */
	for (index = 0U; index < 2U; index++) {
		time_out = UFS_TIMEOUT_US;
		do {
			time_out = time_out - 1U;
			(void)k_usleep(1);
			/* Read TX_FSM_STATE for the current index */
			ufshc_fill_uic_cmd(&uic_cmd,
					   (((uint32_t)MTX_FSM_STATE << 16U) | index),
					   0U, 0U,
					   (uint32_t)UFSHC_DME_GET_OPCODE);
			ret = ufshc_send_uic_cmd(ufshc, &uic_cmd);
			if (ret != 0) {
				goto out;
			}
			val = uic_cmd.mib_value;
			/* Check if the state machine is in a valid state */
			if ((val == VERSAL2_UFS_HIBERN8_STATE) ||
			    (val == VERSAL2_UFS_SLEEP_STATE) ||
			    (val == VERSAL2_UFS_LS_BURST_STATE)) {
				break;
			}
		} while (time_out != 0U);

		if (time_out == 0U) {
			LOG_ERR("Invalid Tx FSM state.");
			ret = -ETIMEDOUT;
			goto out;
		}

		time_out = UFS_TIMEOUT_US;
		do {
			time_out = time_out - 1U;
			(void)k_usleep(1);
			/* Read RX_FSM_STATE for the current index */
			ufshc_fill_uic_cmd(&uic_cmd,
					   (((uint32_t)MRX_FSM_STATE << 16U) | (4U + index)),
					   0U, 0U,
					   (uint32_t)UFSHC_DME_GET_OPCODE);
			ret = ufshc_send_uic_cmd(ufshc, &uic_cmd);
			if (ret != 0) {
				goto out;
			}
			val = uic_cmd.mib_value;
			/* Check if the state machine is in a valid state */
			if ((val == VERSAL2_UFS_HIBERN8_STATE) ||
			    (val == VERSAL2_UFS_SLEEP_STATE) ||
			    (val == VERSAL2_UFS_LS_BURST_STATE)) {
				break;
			}
		} while (time_out != 0U);

		if (time_out == 0U) {
			LOG_ERR("Invalid Rx FSM state.");
			ret = -ETIMEDOUT;
			goto out;
		}
	}

out:
	return ret;
}

/**
 * @brief Initializes the PHY for the Versal Gen2 UFS Host Controller
 *
 * This function performs the initialization of the PHY for the Versal Gen2 UFS Host Controller.
 * It involves several steps including waiting for various status registers to de-assert,
 * de-asserting the PHY reset, waiting for SRAM initialization, performing calibration
 * bypasses, and programming various compensation values. Additionally, firmware calibration
 * settings are configured, and the mPHY is enabled.
 *
 * @param dev Pointer to the device structure representing the UFS device.
 *
 * @return 0 (success) or non-zero value on failure.
 */
static int32_t ufshc_versal2_phy_init(const struct device *dev)
{
	const struct ufshc_versal2_config *cfg = dev->config;
	struct ufshc_versal2_data *drvdata = dev->data;
	struct ufs_host_controller *ufshc = &drvdata->ufshc;
	int32_t ret = 0;
	uint32_t read_reg;
	uint32_t time_out;

	/*
	 * Wait for the Tx/Rx CfgRdyn signal to de-assert, indicating that
	 * configuration of the UFS Tx/Rx lanes is complete and stable. This
	 * ensures that the UFS PHY is ready for further initialization.
	 */
	time_out = UFS_TIMEOUT_US;
	do {
		read_reg = sys_read32(((cfg->reg_iou_slcr) +
				      (mem_addr_t)(VERSAL2_UFS_IOU_SLCR_TX_RX_CFGRDY_OFFSET)));
		if ((read_reg & (uint32_t)VERSAL2_UFS_TX_RX_CFGRDY_MASK) == 0U) {
			break;
		}
		time_out = time_out - 1U;
		(void)k_usleep(1);
	} while (time_out != 0U);

	if (time_out == 0U) {
		LOG_ERR("Tx/Rx configuration signal busy.");
		ret = -ETIMEDOUT;
		goto out;
	}

	ret = ufshc_versal2_set_rmmi(ufshc);
	if (ret != 0) {
		goto out;
	}

	/* De-assert PHY reset */
	sys_write32((0U),
		    ((cfg->reg_iou_slcr) +
		    (mem_addr_t)(VERSAL2_UFS_IOU_SLCR_PHY_RST_OFFSET)));

	/* Wait for SRAM initialization to complete */
	time_out = UFS_TIMEOUT_US;
	do {
		read_reg = sys_read32(((cfg->reg_iou_slcr) +
				      (mem_addr_t)(VERSAL2_UFS_IOU_SLCR_SRAM_CSR_OFFSET)));
		if ((read_reg & VERSAL2_UFS_SRAM_CSR_INIT_DONE_MASK) != 0U) {
			break;
		}
		time_out = time_out - 1U;
		(void)k_usleep(1);
	} while (time_out != 0U);

	if (time_out == 0U) {
		LOG_ERR("SRAM initialization failed.");
		ret = -ETIMEDOUT;
		goto out;
	}

	/* Setup calibration settings */
	ret = ufs_versal2_setup_phy(dev);
	if (ret != 0) {
		goto out;
	}

	/* Enable mPHY */
	ret = ufshc_versal2_enable_mphy(ufshc);
	if (ret != 0) {
		goto out;
	}

out:
	return ret;
}

/**
 * @brief Performs device instance init for Versal Gen2 UFS controller
 *
 * This function initializes the UFS controller instance by setting up
 * the various fields in the host controller structure, configuring the
 * mutexes and events, and performing the required PHY reset and reference
 * clock programming for the UFS device.
 *
 * @param dev Pointer to the device structure for the UFS controller instance
 *
 * @return 0 on success.
 */
static int32_t ufshc_versal2_init(const struct device *dev)
{
	const struct ufshc_versal2_config *cfg = dev->config;
	struct ufshc_versal2_data *drvdata = dev->data;
	struct ufs_host_controller *ufshc = &drvdata->ufshc;

	/* Initialize UFS driver structure and device data */
	(void)memset(ufshc, 0x0, sizeof(*ufshc));
	ufshc->dev = (struct device *)((uintptr_t)dev);
	ufshc->mmio_base = cfg->mmio_base;
	ufshc->is_initialized = false;
	ufshc->irq = cfg->irq_id;
	drvdata->rx_att_comp_val_l0 = 0U;
	drvdata->rx_att_comp_val_l1 = 0U;
	drvdata->rx_ctle_comp_val_l0 = 0U;
	drvdata->rx_ctle_comp_val_l1 = 0U;
	ufshc->is_cache_coherent = 0U;

	/* Initialize and lock UFS card mutex and events */
	k_event_init(&ufshc->irq_event);
	(void)k_mutex_init(&ufshc->ufs_lock);

	/* Perform PHY reset and program reference clock */
	ufshc_versal2_initialization(dev);

	return 0;
}

static DEVICE_API(ufshc, ufshc_versal2_api) = {
	.phy_initialization = ufshc_versal2_phy_init,
	.link_startup_notify = ufshc_versal2_link_startup_notify,
};

/**
 * UFSHC_VERSAL2_INIT - Macro to initialize a UFS host controller instance for Versal Gen2
 * @n: The instance index of the UFS controller
 *
 * This macro initializes the configuration, data structures, and device
 * instance for a UFS host controller instance. Creates a device entry for the
 * UFS host controller in Zephyr device model.
 */
#define UFSHC_VERSAL2_INIT(n)								\
											\
	static const struct ufshc_versal2_config ufshc_versal2_config_##n = {		\
		.mmio_base = DT_INST_REG_ADDR_BY_IDX(n, 0),				\
		.core_clk_rate = DT_PROP(DT_INST_PHANDLE_BY_NAME(n, clocks, core_clk),	\
					 clock_frequency),				\
		.irq_id = DT_INST_IRQN(n),						\
		.reg_iou_slcr = DT_INST_REG_ADDR_BY_IDX(n, 1),				\
		.reg_efuse_cache = DT_INST_REG_ADDR_BY_IDX(n, 2),			\
		.reg_ufs_crp = DT_INST_REG_ADDR_BY_IDX(n, 3),				\
	};										\
											\
	static struct ufshc_versal2_data ufshc_versal2_data_##n = {			\
	};										\
											\
	DEVICE_DT_INST_DEFINE(n, &ufshc_versal2_init, NULL,				\
			      &ufshc_versal2_data_##n, &ufshc_versal2_config_##n,	\
			      POST_KERNEL, CONFIG_UFSHC_INIT_PRIORITY,			\
			      &ufshc_versal2_api);

DT_INST_FOREACH_STATUS_OKAY(UFSHC_VERSAL2_INIT)
