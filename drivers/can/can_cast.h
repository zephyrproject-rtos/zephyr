/*
 * Copyright (C) 2025 Alif Semiconductor.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CAN_CAN_CAST_H_
#define ZEPHYR_DRIVERS_CAN_CAN_CAST_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/util_macro.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CAN register offsets */

#define CAN_RBUF			(0x00)
#define CAN_TBUF			(0x50)
#define CAN_TTS				(0x98)
#define CAN_CFG_STAT			(0xA0)
#define CAN_TCMD			(0xA1)
#define CAN_TCTRL			(0xA2)
#define CAN_RCTRL			(0xA3)
#define CAN_RTIE			(0xA4)
#define CAN_RTIF			(0xA5)
#define CAN_ERRINT			(0xA6)
#define CAN_LIMIT			(0xA7)
#define CAN_S_SEG_1			(0xA8)
#define CAN_S_SEG_2			(0xA9)
#define CAN_S_SJW			(0xAA)
#define CAN_S_PRESC			(0xAB)
#define CAN_F_SEG_1			(0xAC)
#define CAN_F_SEG_2			(0xAD)
#define CAN_F_SJW			(0xAE)
#define CAN_F_PRESC			(0xAF)
#define CAN_EALCAP			(0xB0)
#define CAN_TDC				(0xB1)
#define CAN_RECNT			(0xB2)
#define CAN_TECNT			(0xB3)
#define CAN_ACFCTRL			(0xB4)
#define CAN_TIMECFG			(0xB5)
#define CAN_ACF_EN_0			(0xB6)
#define CAN_ACF_EN_1			(0xB7)
#define CAN_ACF_0_3_CODE		(0xB8)
#define CAN_ACF_0_3_MASK		(0xB8)
#define CAN_VER_0			(0xBC)
#define CAN_VER_1			(0xBD)

/* CAN Counter register offsets */
#define CAN_CNTR_CTRL			(0x00)
#define CAN_CNTR_LOW			(0x04)
#define CAN_CNTR_HIGH			(0x08)

/* Sync segment time quanta */
#define CAN_SYNC_SEG_TQ			1

#define CAN_MIN_BIT_TIME {		\
		.sjw = 0x1,		\
		.prop_seg = 0x1,	\
		.phase_seg1 = 0x1,	\
		.phase_seg2 = 0x2,	\
		.prescaler = 0x1	\
	}
#define CAN_MAX_BIT_TIME {		\
		.sjw = 0x10,		\
		.prop_seg = 0x30,	\
		.phase_seg1 = 0x10,	\
		.phase_seg2 = 0x10,	\
		.prescaler = 0x4	\
	}
#define CAN_MIN_BIT_TIME_DATA {		\
		.sjw = 0x1,		\
		.prop_seg = 0x0,	\
		.phase_seg1 = 0x0,	\
		.phase_seg2 = 0x2,	\
		.prescaler = 0x1	\
	}
#define CAN_MAX_BIT_TIME_DATA {		\
		.sjw = 0x8,		\
		.prop_seg = 0x8,	\
		.phase_seg1 = 0x8,	\
		.phase_seg2 = 0x8,	\
		.prescaler = 0x4	\
	}

#define CAN_MAX_BITRATE               10000000U
#define CAN_MAX_STB_SLOTS             16U
#define CAN_MAX_ACCEPTANCE_FILTERS    16U
/* The value as per Zephyr Std */
#define CAN_ERROR_WARN_LIMIT          96U
#define CAN_MAX_ERROR_WARN_LIMIT      128U
#define CAN_DECREMENT(x, pos)         (x - pos)
#define CAN_TRANSCEIVER_STANDBY_DELAY 3U
#define CAN_RBUF_AFWL_MAX             15U

/* Macros for acpt filter config */
#define CAN_ACPT_FILTER_CFG_ALL_FRAMES 0U
#define CAN_ACPT_FILTER_CFG_STD_FRAMES 2U
#define CAN_ACPT_FILTER_CFG_EXT_FRAMES 3U

/* Macros for Counter control register bit fields */
#define CAN_CNTR_CTRL_CNTR_CLEAR	(2U)
#define CAN_CNTR_CTRL_CNTR_STOP		(1U)
#define CAN_CNTR_CTRL_CNTR_START	(0U)

/* Macros for Configuration and status register bit fields */
#define CAN_CFG_STAT_RESET		(7U)
#define CAN_CFG_STAT_LBME		(6U)
#define CAN_CFG_STAT_LBMI		(5U)
#define CAN_CFG_STAT_TPSS		(4U)
#define CAN_CFG_STAT_TSSS		(3U)
#define CAN_CFG_STAT_RACTIVE		(2U)
#define CAN_CFG_STAT_TACTIVE		(1U)
#define CAN_CFG_STAT_BUSOFF		(0U)

/* Macros for Command Register bit fields */
#define CAN_TCMD_TBSEL			(7U)
#define CAN_TCMD_LOM			(6U)
#define CAN_TCMD_STBY			(5U)
#define CAN_TCMD_TPE			(4U)
#define CAN_TCMD_TPA			(3U)
#define CAN_TCMD_TSONE			(2U)
#define CAN_TCMD_TSALL			(1U)
#define CAN_TCMD_TSA			(0U)

/* Macros for Transmit Control Register bit fields */
#define CAN_TCTRL_FD_ISO		(7U)
#define CAN_TCTRL_TSNEXT		(6U)
#define CAN_TCTRL_TSMODE		(5U)
#define CAN_TCTRL_TSSTAT_Pos		(0U)
#define CAN_TCTRL_TSSTAT_Msk		(3U << CAN_TCTRL_TSSTAT_Pos)
#define CAN_TCTRL_SEC_BUF_FULL		(3U)

/* Macros for Reception Control Register bit fields */
#define CAN_RCTRL_SACK			(7U)
#define CAN_RCTRL_ROM			(6U)
#define CAN_RCTRL_ROV			(5U)
#define CAN_RCTRL_RREL			(4U)
#define CAN_RCTRL_RBALL			(3U)
#define CAN_RCTRL_RSTAT_Pos		(0U)
#define CAN_RCTRL_RSTAT_Msk		(3U << CAN_RCTRL_RSTAT_Pos)

/* Macros for Receive and Transmit Interrupt enable register bit fields */
#define CAN_RTIE_RIE			(7U)
#define CAN_RTIE_ROIE			(6U)
#define CAN_RTIE_RFIE			(5U)
#define CAN_RTIE_RAFIE			(4U)
#define CAN_RTIE_TPIE			(3U)
#define CAN_RTIE_TSIE			(2U)
#define CAN_RTIE_EIE			(1U)
#define CAN_RTIE_TSFF			(0U)

/* Macros for Receive and Transmit Interrupt Flag register bit fields */
#define CAN_RTIF_REG_Msk		(255U)
#define CAN_RTIF_RIF			(7U)
#define CAN_RTIF_ROIF			(6U)
#define CAN_RTIF_RFIF			(5U)
#define CAN_RTIF_RAFIF			(4U)
#define CAN_RTIF_TPIF			(3U)
#define CAN_RTIF_TSIF			(2U)
#define CAN_RTIF_EIF			(1U)
#define CAN_RTIF_AIF			(0U)

/* Macros for Error Interrupt Enable and Flag Register bit fields */
#define CAN_ERRINT_REG_Msk		(21U) /* Mask for Error Flags */
#define CAN_ERRINT_EN_Msk		(42)  /* Mask for Error interrupt enabling */
#define CAN_ERRINT_EWARN		(7U)
#define CAN_ERRINT_EPASS		(6U)
#define CAN_ERRINT_EPIE			(5U)
#define CAN_ERRINT_EPIF			(4U)
#define CAN_ERRINT_ALIE			(3U)
#define CAN_ERRINT_ALIF			(2U)
#define CAN_ERRINT_BEIE			(1U)
#define CAN_ERRINT_BEIF			(0U)

/* Macros for Warning Limits Register bit fields */
#define CAN_LIMIT_AFWL_Pos		(4U)
#define CAN_LIMIT_AFWL_Msk		(0xFU << CAN_LIMIT_AFWL_Pos) /* Max limit is 15 */
#define CAN_LIMIT_AFWL(x)		((x << CAN_LIMIT_AFWL_Pos) & (CAN_LIMIT_AFWL_Msk))

#define CAN_LIMIT_EWL_Pos		(0U)
#define CAN_LIMIT_EWL_Msk		(15U << CAN_LIMIT_EWL_Pos)
#define CAN_LIMIT_EWL(x)		((x << CAN_LIMIT_EWL_Pos) & (CAN_LIMIT_EWL_Msk))

/* Macros for Error and Arbitration Lost Capture Register bit fields */
#define CAN_EALCAP_KOER_Pos		(5U)
#define CAN_EALCAP_KOER_Msk		(7U << CAN_EALCAP_KOER_Pos)

/* Macros for Kind Errors */
#define CAN_EALCAP_KOER_NONE		(0U)
#define CAN_EALCAP_KOER_BIT		(1U)
#define CAN_EALCAP_KOER_FORM		(2U)
#define CAN_EALCAP_KOER_STUFF		(3U)
#define CAN_EALCAP_KOER_ACK		(4U)
#define CAN_EALCAP_KOER_CRC		(5U)
#define CAN_EALCAP_KOER_OTHER		(6U)

/* Macros for Arbitration Lost Capture Register bit fields */
#define CAN_EALCAP_ALC_Pos		(0U)
#define CAN_EALCAP_ALC_Msk		(0x1FU << CAN_EALCAP_ALC_Pos)

/* Macros for Transmitter Delay Compensation Register bit fields */
#define CAN_TDC_TDCEN			(7U)
#define CAN_TDC_SSPOFF_Pos		(0U)
#define CAN_TDC_SSPOFF_Msk		(0x7FU << CAN_TDC_SSPOFF_Pos)

/* Macros for Acceptance Filter Control Register bit fields */
#define CAN_ACFCTRL_SELMASK		(5U)
#define CAN_ACFCTRL_ACFADR_Pos		(0U)
#define CAN_ACFCTRL_ACFADR_Msk		(0xFU << CAN_ACFCTRL_ACFADR_Pos)

/* Macros for Acceptance filter Enable Register bit fields */
#define CAN_ACF_EN_0_AE_X_MAX_Msk	(0xFFFFU << 0U)

/* Macros for Acceptance CODE and MASK Register bit fields */
#define CAN_ACF0_3_AMASK_ACODE_X_Pos (0U)
#define CAN_ACF0_3_AMASK_ACODE_X_Msk (0x1FFFFFFFU << CAN_ACF0_3_AMASK_ACODE_X_Pos)
#define CAN_ACF0_3_ACODE_X(x)        (x & CAN_ACF0_3_AMASK_ACODE_X_Msk)
#define CAN_ACF0_3_AMASK_X_Msk(x)    (x & CAN_ACF0_3_AMASK_ACODE_X_Msk)

#define CAN_ACF_3_MASK_AIDEE		(30U)
#define CAN_ACF_3_MASK_AIDE		(29U)

/* Macros for Time config Register */
#define CAN_TIMECFG_TIMEPOS		(1U)
#define CAN_TIMECFG_TIMEEN		(0U)

/* Macros for CAN Msg access */
#define CAN_MSG_TTSEN			(1U << 31U)
#define CAN_MSG_ESI_Pos			(31U)
#define CAN_MSG_ESI_Msk			(1U << CAN_MSG_ESI_Pos)
#define CAN_MSG_ESI(x)			(x << 31U)
#define CAN_MSG_IDE_Pos			(7U)
#define CAN_MSG_IDE_Msk			(1U << CAN_MSG_IDE_Pos)
#define CAN_MSG_IDE(x)			((x << CAN_MSG_IDE_Pos) & CAN_MSG_IDE_Msk)
#define CAN_MSG_RTR_Pos			(6U)
#define CAN_MSG_RTR_Msk			(1U << CAN_MSG_RTR_Pos)
#define CAN_MSG_RTR(x)			((x << CAN_MSG_RTR_Pos) & CAN_MSG_RTR_Msk)
#define CAN_MSG_FDF_Pos			(5U)
#define CAN_MSG_FDF_Msk			(1U << CAN_MSG_FDF_Pos)
#define CAN_MSG_FDF(x)			((x << CAN_MSG_FDF_Pos) & CAN_MSG_FDF_Msk)
#define CAN_MSG_BRS_Pos			(4U)
#define CAN_MSG_BRS_Msk			(1U << CAN_MSG_BRS_Pos)
#define CAN_MSG_BRS(x)			((x << CAN_MSG_BRS_Pos) & CAN_MSG_BRS_Msk)
#define CAN_MSG_DLC_Pos			(0U)
#define CAN_MSG_DLC_Msk			(15U << CAN_MSG_DLC_Pos)
#define CAN_MSG_DLC(x)			(x & CAN_MSG_DLC_Msk)

#define CAN_TX_ABORT_EVENT                  BIT(0U)
#define CAN_ERROR_EVENT                     BIT(1U)
#define CAN_SECONDARY_BUF_TX_COMPLETE_EVENT BIT(2U)
#define CAN_PRIMARY_BUF_TX_COMPLETE_EVENT   BIT(3U)
#define CAN_RBUF_ALMOST_FULL_EVENT          BIT(4U)
#define CAN_RBUF_FULL_EVENT                 BIT(5U)
#define CAN_RBUF_OVERRUN_EVENT              BIT(6U)
#define CAN_RBUF_AVAILABLE_EVENT            BIT(7U)
#define CAN_BUS_ERROR_EVENT                 BIT(8U)
#define CAN_ARBTR_LOST_EVENT                BIT(10U)
#define CAN_ERROR_PASSIVE_EVENT             BIT(12U)

/* CAN Driver state*/
struct can_driver_state {
	uint32_t initialized: 1;       /* Driver Initialized			*/
	uint32_t filter_configured: 1; /* Driver Acceptance filter Configured   */
	uint32_t reserved: 30;
};

/* CAN Bus Status */
enum can_bus_status {
	CAN_BUS_STATUS_ON = 0x0,
	CAN_BUS_STATUS_OFF = 0x1
};

/* CAN Acceptance filter status */
enum can_acpt_fltr_status {
	CAN_ACPT_FLTR_STATUS_NONE = 0x0,
	CAN_ACPT_FLTR_STATUS_FREE = 0x1,
	CAN_ACPT_FLTR_STATUS_OCCUPIED = 0x2
};

/* CAN Acceptance filter structure */
struct can_acpt_fltr_t {
	uint32_t ac_code;
	uint32_t ac_mask;
	uint8_t filter;
	uint8_t frame_type;
};

/* CAN Transmit Buffer Registers' structure:
 * for Hardware register access
 */
struct tbuf_regs_t {
	uint32_t can_id;
	uint32_t control;
	uint8_t data[CAN_MAX_DLEN];
};

/* CAN Receive Buffer Registers' structure:
 * for Hardware register access
 */
struct rbuf_regs_t {
	uint32_t can_id;
	uint8_t control;
	uint8_t status;
	uint8_t reserved[2U];
	uint8_t data[CAN_MAX_DLEN];
	uint32_t rx_timestamp[2U]; /* 64-bit Rx Timestamp  */
};

/*
 * Message frame
 */
struct can_cast_tx_cb_t {
	can_tx_callback_t cb;
	void *cb_arg;
};

/*
 * Tx Message Queue structure
 */
struct can_cast_tx_queue_t {
	uint8_t head;
	uint8_t tail;
	struct can_cast_tx_cb_t cb_list[CAN_MAX_STB_SLOTS];
};

struct can_cast_filter_t {
	can_rx_callback_t rx_cb;
	void *cb_arg;
	struct can_filter rx_filter;
};

/*
 * Device config structure. Includes:
 *    1. Device MMIO information
 */
struct can_cast_config {
	DEVICE_MMIO_NAMED_ROM(can_reg);
	DEVICE_MMIO_NAMED_ROM(can_cnt_reg);
	struct can_driver_config common;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(clocks)
	/* clock controller dev instance */
	const struct device *clk_dev;
	/* Clock ID for CAN */
	clock_control_subsys_t clk_id;
#endif
	uint32_t clk_freq;
	const struct pinctrl_dev_config *pcfg;
	bool can_fd;
	uint32_t can_fd_ctrl_reg;
	uint8_t can_fd_bit;
	void (*irq_config_func)(const struct device *dev);
};

/*
 * Device data structure. Includes:
 */
struct can_cast_data {
	DEVICE_MMIO_NAMED_RAM(can_reg);
	DEVICE_MMIO_NAMED_RAM(can_cnt_reg);
	struct can_driver_data common;
	struct k_mutex inst_mutex;
	struct can_driver_state state;
	struct can_cast_tx_queue_t tx_queue;
	struct can_cast_filter_t filter[CONFIG_CAN_MAX_FILTER];
	volatile enum can_state err_state;
};

/**
 * @fn          static inline void can_cast_reset_enable(uint32_t can_base)
 * @brief       Resets CAN instance.
 * @param[in]   can_base : Base address of CAN register map
 * @return      none
 */
static inline void can_cast_reset_enable(uint32_t can_base)
{
	uint8_t temp = sys_read8(can_base + CAN_CFG_STAT);

	temp |= BIT(CAN_CFG_STAT_RESET);
	/* Resets CAN module*/
	sys_write8(temp, (can_base + CAN_CFG_STAT));
}

/**
 * @fn          static inline void can_cast_reset_disable(uint32_t can_base)
 * @brief       Releases from Reset of CAN instance.
 * @param[in]   can_base : Base address of CAN register map
 * @return      none
 */
static inline void can_cast_reset_disable(uint32_t can_base)
{
	uint8_t temp = sys_read8(can_base + CAN_CFG_STAT);

	temp &= ~BIT(CAN_CFG_STAT_RESET);
	/* Release Reset bit of CAN module*/
	sys_write8(temp, (can_base + CAN_CFG_STAT));
}

/**
 * @fn          static inline void can_cast_set_iso_spec(uint32_t can_base)
 * @brief       Sets CAN Spec to ISO CAN FD mode (ISO 11898-1:2015)
 * @param[in]   can_base : Base address of CAN Counter register map
 * @return      none
 */
static inline void can_cast_set_iso_spec(uint32_t can_base)
{
	uint8_t temp = sys_read8(can_base + CAN_TCTRL);

	/* Set ISO mode*/
	temp |= BIT(CAN_TCTRL_FD_ISO);
	sys_write8(temp, (can_base + CAN_TCTRL));
}

#if CONFIG_CAN_RX_TIMESTAMP
/**
 * @fn          static inline void can_cast_enable_timestamp(uint32_t can_base)
 * @brief       Enables the CAN Timestamp
 * @param[in]   can_base : Base address of CAN register map
 * @return      None
 */
static inline void can_cast_enable_timestamp(uint32_t can_base)
{
	/* Enables timestamp at SOF */
	sys_write8(BIT(CAN_TIMECFG_TIMEEN), (can_base + CAN_TIMECFG));
}

/**
 * @fn          static inline void can_cast_counter_start(uint32_t can_cnt_base)
 * @brief       Starts the CAN timer counter
 * @param[in]   can_cnt_base : Base address of CAN Counter register map
 * @return      None
 */
static inline void can_cast_counter_start(uint32_t can_cnt_base)
{
	sys_write8(BIT(CAN_CNTR_CTRL_CNTR_START), (can_cnt_base + CAN_CNTR_CTRL));
}

/**
 * @fn          static inline void can_cast_counter_stop(uint32_t can_cnt_base)
 * @brief       Stops the CAN timer counter
 * @param[in]   can_cnt_base : Base address of CAN Counter register map
 * @return      None
 */
static inline void can_cast_counter_stop(uint32_t can_cnt_base)
{
	sys_write8(BIT(CAN_CNTR_CTRL_CNTR_STOP), (can_cnt_base + CAN_CNTR_CTRL));
}

/**
 * @fn          static inline void can_cast_counter_set(uint32_t can_cnt_base,
 *                                                      const uint32_t value)
 * @brief       Sets the CAN low timer counter
 * @param[in]   can_cnt_base : Base address of CAN Counter register map
 * @param[in]   value        : Counter value
 * @return      None
 */
static inline void can_cast_counter_set(uint32_t can_cnt_base, const uint32_t value)
{
	can_cast_counter_stop(can_cnt_base);
	sys_write32(value, (can_cnt_base + CAN_CNTR_LOW));
}
#endif

/**
 * @fn          static inline void can_cast_enable_tx_interrupts(uint32_t can_base)
 * @brief       Enables CAN Tx interrupts
 * @param[in]   can_base : Base address of CAN register map
 * @return      none
 */
static inline void can_cast_enable_tx_interrupts(uint32_t can_base)
{
	uint8_t temp = sys_read8(can_base + CAN_RTIE);

	/* Enables CAN Tx interrupt */
	temp &= BIT(CAN_RTIE_TPIE);
	temp |= BIT(CAN_RTIE_TSIE);
	sys_write8(temp, (can_base + CAN_RTIE));
}

/**
 * @fn          static inline void can_cast_disable_tx_interrupts(uint32_t can_base)
 * @brief       Disables CAN Tx interrupts
 * @param[in]   can_base : Base address of CAN register map
 * @return      none
 */
static inline void can_cast_disable_tx_interrupts(uint32_t can_base)
{
	uint8_t temp = sys_read8(can_base + CAN_RTIE);

	/* Disables CAN Tx interrupts */
	temp &= ~(BIT(CAN_RTIE_TPIE) | BIT(CAN_RTIE_TSIE));
	sys_write8(temp, (can_base + CAN_RTIE));
}

/**
 * @fn          static inline void can_cast_enable_rx_interrupts(uint32_t can_base)
 * @brief       Enables CAN Rx interrupts
 * @param[in]   can_base : Base address of CAN register map
 * @return      none
 */
static inline void can_cast_enable_rx_interrupts(uint32_t can_base)
{
	uint8_t temp = sys_read8(can_base + CAN_RTIE);

	/* Enables CAN Rx interrupts */
	temp |= (BIT(CAN_RTIE_RIE) | BIT(CAN_RTIE_ROIE) |
		BIT(CAN_RTIE_RFIE) | BIT(CAN_RTIE_RAFIE));
	sys_write8(temp, (can_base + CAN_RTIE));
}

/**
 * @fn          static inline void can_cast_disable_rx_interrupts(uint32_t can_base)
 * @brief       Disables CAN Rx interrupts
 * @param[in]   can_base : Base address of CAN register map
 * @return      none
 */
static inline void can_cast_disable_rx_interrupts(uint32_t can_base)
{
	uint8_t temp = sys_read8(can_base + CAN_RTIE);

	/* Disables CAN Rx interrupts */
	temp &= ~(BIT(CAN_RTIE_RIE) | BIT(CAN_RTIE_ROIE) |
		 BIT(CAN_RTIE_RFIE) | BIT(CAN_RTIE_RAFIE));
	sys_write8(temp, (can_base + CAN_RTIE));
}

/**
 * @fn          static inline void can_cast_enable_error_interrupts(uint32_t can_base)
 * @brief       Enables CAN error interrupts
 * @param[in]   can_base : Base address of CAN register map
 * @return      none
 */
static inline void can_cast_enable_error_interrupts(uint32_t can_base)
{
	uint8_t temp = sys_read8(can_base + CAN_RTIE);

	/* Enables error interrupts */
	temp |= BIT(CAN_RTIE_EIE);
	sys_write8(temp, (can_base + CAN_RTIE));

	temp = sys_read8(can_base + CAN_ERRINT);
	temp |= (BIT(CAN_ERRINT_EPIE) |
		BIT(CAN_ERRINT_ALIE)  |
		BIT(CAN_ERRINT_BEIE));
	sys_write8(temp, (can_base + CAN_ERRINT));
}

/**
 * @fn          static inline void can_cast_disable_error_interrupts(uint32_t can_base)
 * @brief       Disables CAN error interrupts
 * @param[in]   can_base : Base address of CAN register map
 * @return      none
 */
static inline void can_cast_disable_error_interrupts(uint32_t can_base)
{
	uint8_t temp = sys_read8(can_base + CAN_RTIE);

	/* Disables error interrupts */
	temp &= ~(BIT(CAN_RTIE_EIE));
	sys_write8(temp, (can_base + CAN_RTIE));

	temp = sys_read8(can_base + CAN_ERRINT);
	temp &= ~(BIT(CAN_ERRINT_EPIE) |
		BIT(CAN_ERRINT_ALIE)   |
		BIT(CAN_ERRINT_BEIE));
	sys_write8(temp, (can_base + CAN_ERRINT));
}

/**
 * @fn          static inline void can_cast_clear_interrupts(uint32_t can_base)
 * @brief       Clears all CAN interrupts
 * @param[in]   can_base : Base address of CAN register map
 * @return      none
 */
static inline void can_cast_clear_interrupts(uint32_t can_base)
{
	uint8_t temp = sys_read8(can_base + CAN_ERRINT);

	temp &= ~CAN_ERRINT_REG_Msk;
	sys_write8(temp, (can_base + CAN_ERRINT));

	/* Clears data and error interrupts */
	sys_write8(0x0, (can_base + CAN_RTIF));
}

/**
 * @fn          static inline void can_cast_enable_standby_mode(uint32_t can_base)
 * @brief       Enables Standby Mode.
 * @param[in]   can_base : Base address of CAN register map
 * @return      none
 */
static inline void can_cast_enable_standby_mode(uint32_t can_base)
{
	uint8_t temp = sys_read8(can_base + CAN_TCMD);

	temp |= BIT(CAN_TCMD_STBY);
	/* Enables Transceiver Standby mode */
	sys_write8(temp, (can_base + CAN_TCMD));
}

/**
 * @fn          static inline void can_cast_disable_standby_mode(uint32_t can_base)
 * @brief       Disables Standby Mode.
 * @param[in]   can_base : Base address of CAN register map
 * @return      none
 */
static inline void can_cast_disable_standby_mode(uint32_t can_base)
{
	uint8_t temp = sys_read8(can_base + CAN_TCMD);

	temp &= ~(BIT(CAN_TCMD_STBY));
	/* Disables Transceiver Standby mode */
	sys_write8(temp, (can_base + CAN_TCMD));
}

/**
 * @fn          static inline void can_cast_enable_normal_mode(uint32_t can_base)
 * @brief       Enables Normal Mode operation of CAN instance.
 * @param[in]   can_base : Base address of CAN register map
 * @return      none
 */
static inline void can_cast_enable_normal_mode(uint32_t can_base)
{
	uint8_t temp = sys_read8(can_base + CAN_CFG_STAT);

	/* Disables Internal and External loopback*/
	temp &= ~(BIT(CAN_CFG_STAT_LBMI) | BIT(CAN_CFG_STAT_LBME));
	sys_write8(temp, (can_base + CAN_CFG_STAT));

	temp = sys_read8(can_base + CAN_TCMD);

	/* Disables Listen only mode */
	temp &= ~BIT(CAN_TCMD_LOM);
	sys_write8(temp, (can_base + CAN_TCMD));
}

/**
 * @fn          static inline void can_cast_enable_internal_loop_back_mode(uint32_t can_base)
 * @brief       Enables Internal LoopBack Mode of CAN instance.
 * @param[in]   can_base : Base address of CAN register map
 * @return      none
 */
static inline void can_cast_enable_internal_loop_back_mode(uint32_t can_base)
{
	uint8_t temp = sys_read8(can_base + CAN_CFG_STAT);

	/* Disables External loopback*/
	temp &= ~BIT(CAN_CFG_STAT_LBME);
	sys_write8(temp, (can_base + CAN_CFG_STAT));

	/* Disables Listen only mode */
	temp = sys_read8(can_base + CAN_TCMD);
	temp &= ~BIT(CAN_TCMD_LOM);
	sys_write8(temp, (can_base + CAN_TCMD));

	/* Enables internal loopback */
	temp = sys_read8(can_base + CAN_CFG_STAT);
	temp |= BIT(CAN_CFG_STAT_LBMI);
	sys_write8(temp, (can_base + CAN_CFG_STAT));
}

/**
 * @fn          static inline void can_cast_enable_listen_only_mode(uint32_t can_base)
 * @brief       Enables Listen only Mode of CAN instance.
 * @param[in]   can_base : Base address of CAN register map
 * @return      none
 */
static inline void can_cast_enable_listen_only_mode(uint32_t can_base)
{
	uint8_t temp = sys_read8(can_base + CAN_CFG_STAT);

	/* Disables Internal and External loopback*/
	temp &= ~(BIT(CAN_CFG_STAT_LBMI) | BIT(CAN_CFG_STAT_LBME));
	sys_write8(temp, (can_base + CAN_CFG_STAT));

	/* Enables Listen only mode */
	temp = sys_read8(can_base + CAN_TCMD);
	temp |= BIT(CAN_TCMD_LOM);
	sys_write8(temp, (can_base + CAN_TCMD));
}

/**
 * @fn          static inline void can_cast_set_tbuf_op_mode_priority(uint32_t can_base)
 * @brief       Sets Tx buf operation mode to Priority
 * @param[in]   can_base :  Base address of CAN register map
 * @return      none
 */
static inline void can_cast_set_tbuf_op_mode_priority(uint32_t can_base)
{
	/* Set Secondary Tbuf Operation mode to priority */
	uint8_t temp = sys_read8(can_base + CAN_TCTRL);

	temp |= BIT(CAN_TCTRL_TSMODE);
	sys_write8(temp, (can_base + CAN_TCTRL));
}

/**
 * @fn          static inline bool can_cast_comm_active(uint32_t can_base)
 * @brief       Fetches message communication status
 * @param[in]   can_base :  Base address of CAN register map
 * @return      Message comm status(Comm active/Inactive)
 */
static inline bool can_cast_comm_active(uint32_t can_base)
{
	uint8_t temp = sys_read8(can_base + CAN_CFG_STAT);

	return (temp & (BIT(CAN_CFG_STAT_TACTIVE) | BIT(CAN_CFG_STAT_RACTIVE)));
}

/**
 * @fn          static inline bool can_cast_tx_active(uint32_t can_base)
 * @brief       Fetches the Transmit buffer Tx status
 * @param[in]   can_base :  Base address of CAN register map
 * @return      transmission status - Active/Inactive
 */
static inline bool can_cast_tx_active(uint32_t can_base)
{
	return (sys_read8(can_base + CAN_CFG_STAT) & BIT(CAN_CFG_STAT_TACTIVE));
}

/**
 * @fn          static inline bool can_cast_stb_free(uint32_t can_base)
 * @brief       Fetches the Secondary Transmit buffer Free status
 * @param[in]   can_base :  Base address of CAN register map
 * @return      Buffer status - Free/Full
 */
static inline bool can_cast_stb_free(uint32_t can_base)
{
	return ((sys_read8(can_base + CAN_TCTRL) &
		CAN_TCTRL_TSSTAT_Msk) != CAN_TCTRL_SEC_BUF_FULL);
}

/**
 * @fn          static inline bool can_cast_stb_empty(uint32_t can_base)
 * @brief       Fetches the Secondary Transmit buffer empty status
 * @param[in]   can_base :  Base address of CAN register map
 * @return      Buffer status - Empty/Non_empty
 */
static inline bool can_cast_stb_empty(uint32_t can_base)
{
	return ((sys_read8(can_base + CAN_TCTRL) & CAN_TCTRL_TSSTAT_Msk) == 0);
}

/**
 * @fn          static inline bool can_cast_stb_single_shot_mode(uint32_t can_base)
 * @brief       Checks whether STB is in single shot mode or not
 * @param[in]   can_base :  Base address of CAN register map
 * @return      STB Tx mode status (Single shot/Retransmission)
 */
static inline bool can_cast_stb_single_shot_mode(uint32_t can_base)
{
	return ((sys_read8(can_base + CAN_CFG_STAT) & CAN_CFG_STAT_TSSS) != 0);
}

/**
 * @fn          static inline void can_cast_abort_tx(uint32_t can_base)
 * @brief       Aborts current transmission
 * @param[in]   can_base :  Base address of CAN register map
 * @return      none
 */
static inline void can_cast_abort_tx(uint32_t can_base)
{
	/* Abort message Tx from Sec Tx buf */
	uint8_t temp = sys_read8(can_base + CAN_TCMD);

	temp |= BIT(CAN_TCMD_TSA);
	sys_write8(temp, (can_base + CAN_TCMD));
}

/**
 * @fn          static inline uint8_t can_cast_get_tx_error_count(uint32_t can_base)
 * @brief       Fetches the latest Transmission error count
 * @param[in]   can_base :  Base address of CAN register map
 * @return      Transmission error count
 */
static inline uint8_t can_cast_get_tx_error_count(uint32_t can_base)
{
	return sys_read8(can_base + CAN_TECNT);
}

/**
 * @fn          static inline uint8_t can_cast_get_rx_error_count(uint32_t can_base)
 * @brief       Fetches the latest Reception error count
 * @param[in]   can_base :  Base address of CAN register map
 * @return      Reception error count
 */
static inline uint8_t can_cast_get_rx_error_count(uint32_t can_base)
{
	return sys_read8(can_base + CAN_RECNT);
}

/**
 * @fn          static inline void can_cast_set_nominal_bit_time(uint32_t can_base,
 *							const struct can_timing timing)
 *
 * @brief       Sets the normal bit-timing of CAN instance.
 * @param[in]   can_base : Base address of CAN register map
 * @param[in]   timing   : Segments - Propagation, Phase, sjw, prescaler
 * @return      none
 */
static inline void can_cast_set_nominal_bit_time(uint32_t can_base,
					const struct can_timing timing)
{
	/* Configures Nominal bit rate registers */
	sys_write8(CAN_DECREMENT(timing.phase_seg1, 2U), (can_base + CAN_S_SEG_1));
	sys_write8(CAN_DECREMENT(timing.phase_seg2, 1U), (can_base + CAN_S_SEG_2));
	sys_write8(CAN_DECREMENT(timing.sjw, 1U), (can_base + CAN_S_SJW));
	sys_write8(CAN_DECREMENT(timing.prescaler, 1U), (can_base + CAN_S_PRESC));
}

/**
 * @fn          static inline void can_cast_set_fd_bit_time(uint32_t can_base,
 *						const struct can_timing timing)
 *						const uint8_t tdc)
 * @brief       Sets the Fast-Data bit-timing of CAN instance
 * @param[in]   can_base : Base address of CAN register map
 * @param[in]   timing   : Segments - Propagation, Phase, sjw, prescaler
 * @param[in]   tdc      : Transmitter delay compensation
 * @return      none
 */
static inline void can_cast_set_fd_bit_time(uint32_t can_base,
				const struct can_timing timing,
				const uint8_t tdc)
{
	/* Configures Fast-Data bit rate registers */
	sys_write8(CAN_DECREMENT(timing.phase_seg1, 2U), (can_base + CAN_F_SEG_1));
	sys_write8(CAN_DECREMENT(timing.phase_seg2, 1U), (can_base + CAN_F_SEG_2));
	sys_write8(CAN_DECREMENT(timing.sjw, 1U), (can_base + CAN_F_SJW));
	sys_write8(CAN_DECREMENT(timing.prescaler, 1U), (can_base + CAN_F_PRESC));

	sys_write8((BIT(CAN_TDC_TDCEN) | (tdc & CAN_TDC_SSPOFF_Msk)), (can_base + CAN_TDC));
}

/**
 * @fn          static inline void can_reset_acpt_fltr(uint32_t can_base)
 * @brief       Resets all acceptance filters
 * @param[in]   can_base  : Base address of CAN register map
 * @return      none
 */
static inline void can_cast_reset_acpt_fltrs(uint32_t can_base)
{
	uint16_t temp = sys_read16(can_base + CAN_ACF_EN_0);

	/* Disable filter */
	temp &= ~CAN_ACF_EN_0_AE_X_MAX_Msk;
	sys_write16(temp, (can_base + CAN_ACF_EN_0));
}

/**
 * @fn          static inline bool can_cast_acpt_fltr_configured(uint32_t can_base)
 * @brief       Returns acceptance filters configured status
 * @param[in]   can_base : Base address of CAN register map
 * @return      filters configured status
 */
static inline bool can_cast_acpt_fltr_configured(uint32_t can_base)
{
	uint16_t temp = sys_read16(can_base + CAN_ACF_EN_0);

	return ((temp & CAN_ACF_EN_0_AE_X_MAX_Msk) != 0);
}

/**
 * @fn          static inline void can_cast_disable_acpt_fltr(uint32_t can_base,
 *                                                         const uint8_t filter)
 * @brief       Resets and disables the particular acceptance filter
 * @param[in]   can_base : Base address of CAN register map
 * @param[in]   filter   : Acceptance filter number
 * @return      none
 */
static inline void can_cast_disable_acpt_fltr(uint32_t can_base, const uint8_t filter)
{
	uint16_t temp = sys_read16(can_base + CAN_ACF_EN_0);

	/* Disable filter */
	temp &= ~(1U << filter);
	sys_write16(temp, (can_base + CAN_ACF_EN_0));
}

/**
 * @fn          static inline void can_cast_set_rbuf_almost_full_warn_limit(uint32_t can_base,
 *                                                                          const uint8_t val)
 * @brief       Sets Rbuf almost full warning limit (AFWL)
 * @param[in]   can_base : Base address of CAN register map
 * @ param[in]  val      : AFWL value
 * @return      none
 */
static inline void can_cast_set_rbuf_almost_full_warn_limit(uint32_t can_base, const uint8_t val)
{
	uint8_t temp = sys_read8(can_base + CAN_LIMIT);

	/* Clears and sets new AFWL value */
	temp &= ~CAN_LIMIT_AFWL_Msk;
	temp |= CAN_LIMIT_AFWL(val);
	sys_write8(temp, (can_base + CAN_LIMIT));
}

/**
 * @fn          static inline bool can_cast_rx_msg_available(uint32_t can_base)
 * @brief       Returns Rx msg availability status
 * @param[in]   can_base : Base address of CAN register map
 * @return      Rx msg availability status
 */
static inline bool can_cast_rx_msg_available(uint32_t can_base)
{
	return ((sys_read8(can_base + CAN_RCTRL) & CAN_RCTRL_RSTAT_Msk) != 0);
}

/**
 * @fn          static inline void can_cast_turn_on_bus(uint32_t can_base)
 * @brief       Turns on the CAN bus
 * @param[in]   can_base : Base address of CAN register map
 * @return      none
 */
static inline void can_cast_turn_on_bus(uint32_t can_base)
{
	uint8_t temp = sys_read8(can_base + CAN_CFG_STAT);

	/* Turns on the bus */
	temp |= BIT(CAN_CFG_STAT_BUSOFF);
	sys_write8(temp, (can_base + CAN_CFG_STAT));
}

/**
 * @fn          static inline enum can_bus_status can_cast_get_bus_status(uint32_t can_base)
 * @brief       Fetches the current bus status
 * @param[in]   can_base   : Base address of CAN register map
 * @return      bus status - CAN_BUS_STATUS_ON/CAN_BUS_STATUS_OFF
 */
static inline enum can_bus_status can_cast_get_bus_status(uint32_t can_base)
{
	/* Returns current bus status*/
	if (sys_read8(can_base + CAN_CFG_STAT) & BIT(CAN_CFG_STAT_BUSOFF)) {
		return CAN_BUS_STATUS_OFF;
	} else {
		return CAN_BUS_STATUS_ON;
	}
}

/**
 * @fn          static inline bool can_cast_error_passive_mode(uint32_t can_base)
 * @brief       Returns the passive mode status
 * @param[in]   can_base   : Base address of CAN register map
 * @return      passive mode status
 */
static inline bool can_cast_error_passive_mode(uint32_t can_base)
{
	return ((sys_read8(can_base + CAN_ERRINT) & BIT(CAN_ERRINT_EPASS)) != 0);
}

/**
 * @fn          static inline bool can_cast_err_warn_limit_reached(uint32_t can_base)
 * @brief       Returns the passive mode status
 * @param[in]   can_base   : Base address of CAN register map
 * @return      Warning limit reached status
 */
static inline bool can_cast_err_warn_limit_reached(uint32_t can_base)
{
	return ((sys_read8(can_base + CAN_ERRINT) & BIT(CAN_ERRINT_EWARN)) != 0);
}

/**
 * @fn          static inline bool can_cast_get_alc(uint32_t can_base)
 * @brief       Returns Bit position value of Arbitration lost
 * @param[in]   can_base   : Base address of CAN register map
 * @return      Bit position value of arbitration lost
 */
static inline uint8_t can_cast_get_alc(uint32_t can_base)
{
	return (sys_read8(can_base + CAN_EALCAP) & CAN_EALCAP_ALC_Msk);
}

#ifdef __cplusplus
}
#endif
#endif /* ZEPHYR_DRIVERS_CAN_CAN_CAST_H_ */
