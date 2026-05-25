/*
 * Copyright (c) 2026 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/transceiver.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/renesas_rcar_generic.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(can_rcar_rscanfd, CONFIG_CAN_LOG_LEVEL);

#define DT_DRV_COMPAT renesas_rcar_rscanfd

/* Channel n Nominal Bitrate Configuration Register */
#define RSCANFD_CFDCNNCFG 0x0000
/* Channel n Nominal Bitrate Configuration Register bits */
#define RSCANFD_CFDCNNCFG_NBRP_MASK 0x3FF
#define RSCANFD_CFDCNNCFG_NBRP_SHIFT 0
#define RSCANFD_CFDCNNCFG_NSJW_MASK 0x07F
#define RSCANFD_CFDCNNCFG_NSJW_SHIFT 10
#define RSCANFD_CFDCNNCFG_NTSEG1_MASK 0xFF
#define RSCANFD_CFDCNNCFG_NTSEG1_SHIFT 17
#define RSCANFD_CFDCNNCFG_NTSEG2_MASK 0xFF
#define RSCANFD_CFDCNNCFG_NTSEG2_SHIFT 25

/* Channel n Control Register */
#define RSCANFD_CFDCNCTR 0x0004
/* Channel n Control Register bits */
#define RSCANFD_CFDCNCTR_CTMS_MASK 0x03
#define RSCANFD_CFDCNCTR_CTMS_SHIFT 25
#define RSCANFD_CFDCNCTR_CTMS_LISTEN_ONLY_MODE 0x01
#define RSCANFD_CFDCNCTR_CTMS_INTERNAL_LOOPBACK_MODE 0x03
#define RSCANFD_CFDCNCTR_CTME_MASK 0x01
#define RSCANFD_CFDCNCTR_CTME_SHIFT 24
#define RSCANFD_CFDCNCTR_CTME_DISABLED 0
#define RSCANFD_CFDCNCTR_CTME_ENABLED 1
#define RSCANFD_CFDCNCTR_ALIE BIT(15)
#define RSCANFD_CFDCNCTR_BORIE BIT(12)
#define RSCANFD_CFDCNCTR_BOEIE BIT(11)
#define RSCANFD_CFDCNCTR_EPIE BIT(10)
#define RSCANFD_CFDCNCTR_EWIE BIT(9)
#define RSCANFD_CFDCNCTR_BEIE BIT(8)
#define RSCANFD_CFDCNCTR_RTBO BIT(3)
#define RSCANFD_CFDCNCTR_CHMDC_MASK 0x03
#define RSCANFD_CFDCNCTR_CHMDC_SHIFT 0
#define RSCANFD_CFDCNCTR_CHMDC_CHANNEL_OPERATION_REQUEST 0
#define RSCANFD_CFDCNCTR_CHMDC_CHANNEL_RESET_REQUEST 0x01
#define RSCANFD_CFDCNCTR_CHMDC_CHANNEL_HALT_REQUEST 0x02

/* Channel n Status Register */
#define RSCANFD_CFDCNSTS 0x0008
/* Channel n Status Register bits */
#define RSCANFD_CFDCNSTS_TEC_MASK 0xFF
#define RSCANFD_CFDCNSTS_TEC_SHIFT 24
#define RSCANFD_CFDCNSTS_REC_MASK 0xFF
#define RSCANFD_CFDCNSTS_REC_SHIFT 16
#define RSCANFD_CFDCNSTS_CSLPSTS BIT(2)
#define RSCANFD_CFDCNSTS_CHLTSTS BIT(1)
#define RSCANFD_CFDCNSTS_CRSTSTS BIT(0)

/* Channel n Error Flag Register */
#define RSCANFD_CFDCNERFL 0x000C
/* Channel n Error Flag Register bits */
#define RSCANFD_CFDCNERFL_ADERR BIT(14)
#define RSCANFD_CFDCNERFL_B0ERR BIT(13)
#define RSCANFD_CFDCNERFL_B1ERR BIT(12)
#define RSCANFD_CFDCNERFL_CERR BIT(11)
#define RSCANFD_CFDCNERFL_AERR BIT(10)
#define RSCANFD_CFDCNERFL_FERR BIT(9)
#define RSCANFD_CFDCNERFL_SERR BIT(8)
#define RSCANFD_CFDCNERFL_ALF BIT(7)
#define RSCANFD_CFDCNERFL_BORF BIT(4)
#define RSCANFD_CFDCNERFL_BOEF BIT(3)
#define RSCANFD_CFDCNERFL_EPF BIT(2)
#define RSCANFD_CFDCNERFL_EWF BIT(1)
#define RSCANFD_CFDCNERFL_BEF BIT(0)

/* Global IP Version Register */
#define RSCANFD_CFDGIPV 0x0080

/* Global Control Register */
#define RSCANFD_CFDGCTR 0x0088
/* Global Control Register bits */
#define RSCANFD_CFDGCTR_GMDC_MASK 0x03
#define RSCANFD_CFDGCTR_GMDC_SHIFT 0
#define RSCANFD_CFDGCTR_GMDC_GLOBAL_OPERATION_MODE_REQUEST 0x00
#define RSCANFD_CFDGCTR_GMDC_GLOBAL_RESET_MODE_REQUEST 0x01

/* Global Status Register */
#define RSCANFD_CFDGSTS 0x008C
/* Global Status Register bits */
#define RSCANFD_CFDGSTS_GRAMINIT BIT(3)
#define RSCANFD_CFDGSTS_GRSTSTS BIT(0)

/* Global Acceptance Filter List Entry Control Register */
#define RSCANFD_CFDGAFLECTR 0x0098
/* Global Acceptance Filter List Entry Control Register bits */
#define RSCANFD_CFDGAFLECTR_AFLDAE BIT(8)

/* Global Acceptance Filter List Configuration Register n */
#define RSCANFD_CFDGAFLCFGN 0x009C
/* Global Acceptance Filter List Configuration Register n bits */
#define RSCANFD_CFDGAFLCFGN_RNC_CHANNEL_MASK 0x001F
#define RSCANFD_CFDGAFLCFGN_RNC_CHANNEL_EVEN_SHIFT 16
#define RSCANFD_CFDGAFLCFGN_RNC_CHANNEL_ODD_SHIFT 0

/* Common FIFO Configuration / Control Register n */
#define RSCANFD_CFDCFCCN 0x0120
/* Common FIFO Configuration / Control Register n bits */
#define RSCANFD_CFDCFCCN_CFDC_MASK 0x03
#define RSCANFD_CFDCFCCN_CFDC_SHIFT 21
#define RSCANFD_CFDCFCCN_CFDC_DEPTH_64 0x06
#define RSCANFD_CFDCFCCN_CFDC_DEPTH_128 0x07
#define RSCANFD_CFDCFCCN_CFIM_SHIFT 12
#define RSCANFD_CFDCFCCN_CFIM_EVERY_MESSAGE 1
#define RSCANFD_CFDCFCCN_CFIM_LAST_MESSAGE 0
#define RSCANFD_CFDCFCCN_CFM_MASK 0x03
#define RSCANFD_CFDCFCCN_CFM_SHIFT 8
#define RSCANFD_CFDCFCCN_CFM_RX 0x00
#define RSCANFD_CFDCFCCN_CFM_TX 0x01
#define RSCANFD_CFDCFCCN_CFPLS_SHIFT 4
#define RSCANFD_CFDCFCCN_CFPLS_SIZE_64 0x07
#define RSCANFD_CFDCFCCN_CFTXIE_SHIFT 2
#define RSCANFD_CFDCFCCN_CFTXIE_INT_ENABLED 1
#define RSCANFD_CFDCFCCN_CFRXIE_SHIFT 1
#define RSCANFD_CFDCFCCN_CFRXIE_INT_ENABLED 1
#define RSCANFD_CFDCFCCN_CFE BIT(0)

/* Common FIFO Status Registers n */
#define RSCANFD_CFDCFSTSN 0x01E0
/* Common FIFO Status Registers n bits */
#define RSCANFD_CFDCFSTSN_CFTXIF BIT(4)
#define RSCANFD_CFDCFSTSN_CFRXIF BIT(3)
#define RSCANFD_CFDCFSTSN_CFEMP BIT(0)

/* Common FIFO Pointer Control Register 0 */
#define RSCANFD_CFDCFPCTR0 0x0240

/* TX Queue Configuration / Control Registers 0 n */
#define RSCANFD_CFDTXQCC0N 0x1000
/* TX Queue Configuration / Control Registers 0 n bits */
#define RSCANFD_CFDTXQCC0N_TXQDC_MASK 0x1F
#define RSCANFD_CFDTXQCC0N_TXQDC_SHIFT 8
#define RSCANFD_CFDTXQCC0N_TXQDC_DEPTH_32_MSG 0x1F
#define RSCANFD_CFDTXQCC0N_TXQE BIT(0)

/* TX Queue Pointer Control Registers 0 n */
#define RSCANFD_CFDTXQPCTR0N 0x1040
/* TX Queue Pointer Control Registers 0 n bits */
#define RSCANFD_CFDTXQPCTR0N_TXQPC_MASK 0xFF
#define RSCANFD_CFDTXQPCTR0N_TXQPC_SHIFT 0

/* TX History List Configuration / Control Register n */
#define RSCANFD_CFDTHLCCN 0x1200
/* TX History List Configuration / Control Register n bits */
#define RSCANFD_CFDTHLCCN_THLIM BIT(9)
#define RSCANFD_CFDTHLCCN_THLIE BIT(8)
#define RSCANFD_CFDTHLCCN_THLE BIT(0)

/* TX History List Status Register n */
#define RSCANFD_CFDTHLSTSN 0x1220
/* TX History List Status Register n bits */
#define RSCANFD_CFDTHLSTSN_THLIF BIT(3)
#define RSCANFD_CFDTHLSTSN_THLEMP BIT(0)

/* TX History List Pointer Control Registers n */
#define RSCANFD_CFDTHLPCTRN 0x1240

/* Channel n Data Bitrate Configuration Register */
#define RSCANFD_CFDCNDCFG 0x1400
/* Channel n Data Bitrate Configuration Register bits */
#define RSCANFD_CFDCNDCFG_DSJW_SHIFT 24
#define RSCANFD_CFDCNDCFG_DTSEG2_SHIFT 16
#define RSCANFD_CFDCNDCFG_DTSEG1_SHIFT 8
#define RSCANFD_CFDCNDCFG_DBRP_SHIFT 0

/* Channel n CAN FD Configuration Register */
#define RSCANFD_CFDCNFDCFG 0x1404
/* Channel n CAN FD Configuration Register bits */
#define RSCANFD_CFDCNFDCFG_CLOE BIT(30)
#define RSCANFD_CFDCNFDCFG_FDOE BIT(28)
#define RSCANFD_CFDCNFDCFG_TDCO_MASK 0xFF
#define RSCANFD_CFDCNFDCFG_TDCO_SHIFT 16
#define RSCANFD_CFDCNFDCFG_TDCE BIT(9)
#define RSCANFD_CFDCNFDCFG_TDCOC BIT(8)

/* Global Acceptance Filter List ID Register n */
#define RSCANFD_CFDGAFLIDN 0x1800

/* Global Acceptance Filter List Mask Register n */
#define RSCANFD_CFDGAFLMN 0x1804

/* Global Acceptance Filter List Pointer 0 Register n */
#define RSCANFD_CFDGAFLP0N 0x1808

/* Global Acceptance Filter List Pointer 1 Register N */
#define RSCANFD_CFDGAFLP1N 0x180C

/* Common FIFO Access ID Registers b n */
#define RSCANFD_CFDCFIDBN 0x6400
/* Common FIFO Access ID Registers b n bits */
#define RSCANFD_CFDCFIDBN_CFIDE BIT(31)
#define RSCANFD_CFDCFIDBN_CFRTR BIT(30)
#define RSCANFD_CFDCFIDBN_CFID_MASK 0x1FFFFFFF
#define RSCANFD_CFDCFIDBN_CFID_SHIFT 0

/* Common FIFO Access Pointer Registers b n */
#define RSCANFD_CFDCFPTRBN 0x6404
/* Common FIFO Access Pointer Registers b n bits */
#define RSCANFD_CFDCFPTRBN_CFDLC_MASK 0x0F
#define RSCANFD_CFDCFPTRBN_CFDLC_SHIFT 28
#define RSCANFD_CFDCFPTRBN_CFTS_MASK 0xFFFF
#define RSCANFD_CFDCFPTRBN_CFTS_SHIFT 0

/* Common FIFO Access CAN FD Control/Status Register b n */
#define RSCANFD_CFDCFFDCSTSBN 0x6408
/* Common FIFO Access CAN FD Control/Status Register b n bits */
#define RSCANFD_CFDCFFDCSTSBN_CFFDF BIT(2)
#define RSCANFD_CFDCFFDCSTSBN_CFBRS BIT(1)
#define RSCANFD_CFDCFFDCSTSBN_CFESI BIT(0)

/* Common FIFO Access Data Field 0 Registers b n */
#define RSCANFD_CFDCFDF0BN 0x640C

/* Channel n TX History List Access Registers 1 */
#define RSCANFD_CFDTHLACC1N 0x8004
/* Channel n TX History List Access Registers 1 bits */
#define RSCANFD_CFDTHLACC1N_TID_MASK 0xFFFF
#define RSCANFD_CFDTHLACC1N_TID_SHIFT 0

/* TX Message Buffer ID Registers b_n */
#define RSCANFD_CFDTMIDBN 0x10000
/* TX Message Buffer ID Registers b_n bits */
#define RSCANFD_CFDTMIDBN_TMIDE BIT(31)
#define RSCANFD_CFDTMIDBN_TMRTR BIT(30)
#define RSCANFD_CFDTMIDBN_THLEN BIT(29)
#define RSCANFD_CFDTMIDBN_TMID_MASK 0x1FFFFFFF
#define RSCANFD_CFDTMIDBN_TMID_SHIFT 0

/* TX Message Buffer Pointer Registers b n */
#define RSCANFD_CFDTMPTRBN 0x10004
/* TX Message Buffer Pointer Registers b n bits */
#define RSCANFD_CFDTMPTRBN_TMDLC_MASK 0x0F
#define RSCANFD_CFDTMPTRBN_TMDLC_SHIFT 28

/* TX Message Buffer CAN-FD Control Register b n */
#define RSCANFD_CFDTMFDCTRBN 0x10008
/* TX Message Buffer CAN-FD Control Register b n bits */
#define RSCANFD_CFDTMFDCTRBN_TMPTR_MASK 0xFFFF
#define RSCANFD_CFDTMFDCTRBN_TMPTR_SHIFT 16
#define RSCANFD_CFDTMFDCTRBN_TMFDF BIT(2)
#define RSCANFD_CFDTMFDCTRBN_TMBRS BIT(1)
#define RSCANFD_CFDTMFDCTRBN_TMESI BIT(0)

/* TX Message Buffer Data Field 0 Registers b n */
#define RSCANFD_CFDTMDF0BN 0x1000C

#define CAN_RCAR_RSCANFD_MODULE_CLOCK_RATE 80000000

/* The amount of channels per controller. */
#define CAN_RCAR_RSCANFD_CHANNELS_COUNT 8

/* How many Common FIFO are assigned to each channel. */
#define CAN_RCAR_RSCANFD_COMMON_FIFO_PER_CHANNEL 3

/* How many TX Queue are assigned to each channel. */
#define CAN_RCAR_RSCANFD_TX_QUEUE_PER_CHANNEL 4

/** How many transmission buffers are configured for each TX queue. */
#define CAN_RCAR_RSCANFD_TX_QUEUE_BUFFERS_COUNT 32

/* There are four consecutive 4-byte registers per entry */
#define CAN_RCAR_RSCANFD_AFL_ENTRY_SIZE 16

/* There are 32 consecutive 4-byte registers per entry (only the first 19 are used). */
#define CAN_RCAR_RSCANFD_CFMBCP_ENTRY_SIZE 128

/* The number of bytes per entry. */
#define CAN_RCAR_RSCANFD_TMBCP_ENTRY_SIZE 2048

/* There are four consecutive 4-byte registers for each CAN 2.0 channel configuration */
#define CAN_RCAR_RSCANFD_CAN_CHANNEL_REGISTERS_GROUP_SIZE 16

/* There are eight consecutive 4-byte registers for each CAN FD channel configuration */
#define CAN_RCAR_RSCANFD_CANFD_CHANNEL_REGISTERS_GROUP_SIZE 32

/* There are two 4-byte consecutive TX history list access registers. */
#define CAN_RCAR_RSCANFD_TX_HISTORY_LIST_ACCESS_REGISTERS_GROUP_SIZE 8

struct can_rcar_rscanfd_global_config {
	uint32_t reg;
	const struct device *clock_dev;
	rcar_generic_clk_t global_clk;
	rcar_generic_clk_t module_clk;
	uint32_t num_enabled_channels;
};

struct can_rcar_rscanfd_global_data {
	struct k_mutex list_mutex;
	const struct device *channel_dev[CAN_RCAR_RSCANFD_CHANNELS_COUNT];
	uint32_t enabled_channels_count;
};

struct can_rcar_rscanfd_config {
	const struct can_driver_config common;
	const struct device *global_dev;
	uint32_t reg;
	uint32_t channel;
	const struct pinctrl_dev_config *pcfg;
	void (*configure_irq)(void);
};

struct can_rcar_rscanfd_tx_callback {
	can_tx_callback_t callback;
	void *user_data;
};

struct can_rcar_rscanfd_data {
	struct can_driver_data common;
	struct k_mutex inst_mutex;
	/*
	 * Allow a O(1) access to the next free index in the TX user callbacks array.
	 * Use a Stack object because it should be faster than a queue or a list, and the order
	 * of the values is not important.
	 */
	struct k_stack *tx_free_index_stack;
	struct can_rcar_rscanfd_tx_callback tx_callbacks[CAN_RCAR_RSCANFD_TX_QUEUE_BUFFERS_COUNT];
	struct k_sem tx_sem;
	can_rx_callback_t rx_callback[CONFIG_CAN_RCAR_RSCANFD_MAX_FILTERS];
	void *rx_callback_user_data[CONFIG_CAN_RCAR_RSCANFD_MAX_FILTERS];
	struct can_filter filter[CONFIG_CAN_RCAR_RSCANFD_MAX_FILTERS];
	enum can_state state;
	struct k_spinlock irq_spinlock;
};

static int can_rcar_rscanfd_set_mode(const struct device *dev, can_mode_t mode);

static inline uint32_t can_rcar_rscanfd_read(const struct device *dev, uint32_t offset)
{
	const struct can_rcar_rscanfd_config *config = dev->config;

	return sys_read32(config->reg + offset);
}

static inline void can_rcar_rscanfd_write(const struct device *dev,
					  uint32_t offset, uint32_t value)
{
	const struct can_rcar_rscanfd_config *config = dev->config;

	sys_write32(value, config->reg + offset);
}

/* Note : this function can sleep, do not use it in interrupt context. */
static inline int can_rcar_rscanfd_busy_wait(mem_addr_t reg, uint32_t bit_mask, bool expect_one)
{
	const int TIMEOUT = 2000; /* Don't wait for too long */
	bool success;

	if (expect_one) {
		success = WAIT_FOR(sys_read32(reg) & bit_mask, TIMEOUT, k_sleep(K_USEC(10)));
	} else {
		success = WAIT_FOR(!(sys_read32(reg) & bit_mask), TIMEOUT, k_sleep(K_USEC(10)));
	}

	if (!success) {
		LOG_DBG("Busy wait timed out. reg=0x%08lX, bit_mask=%08X, expect_one=%d",
			reg, bit_mask, expect_one);
		return -EAGAIN;
	}

	return 0;
}

/* Note : the reset state is applied at controller level, impacting all channels. */
static int can_rcar_rscanfd_enter_reset_mode(const struct can_rcar_rscanfd_global_config *config)
{
	/* Request the global reset mode, resetting all other fields in the same time */
	sys_write32(RSCANFD_CFDGCTR_GMDC_GLOBAL_RESET_MODE_REQUEST, config->reg + RSCANFD_CFDGCTR);

	return can_rcar_rscanfd_busy_wait(config->reg + RSCANFD_CFDGSTS,
		RSCANFD_CFDGSTS_GRSTSTS, 1);
}

/* Note : the operation state is applied at controller level, impacting all channels. */
static int can_rcar_rscanfd_enter_operation_mode(
	const struct can_rcar_rscanfd_global_config *config)
{
	uint32_t val;

	val = sys_read32(config->reg + RSCANFD_CFDGCTR);
	val &= ~(RSCANFD_CFDGCTR_GMDC_MASK << RSCANFD_CFDGCTR_GMDC_SHIFT);
	val |= RSCANFD_CFDGCTR_GMDC_GLOBAL_OPERATION_MODE_REQUEST << RSCANFD_CFDGCTR_GMDC_SHIFT;
	sys_write32(val, config->reg + RSCANFD_CFDGCTR);

	return can_rcar_rscanfd_busy_wait(config->reg + RSCANFD_CFDGSTS,
		RSCANFD_CFDGSTS_GRSTSTS, 0);
}

static int can_rcar_rscanfd_channel_leave_sleep_mode(const struct device *dev)
{
	const struct can_rcar_rscanfd_config *config = dev->config;
	uint32_t base_offset = config->channel * CAN_RCAR_RSCANFD_CAN_CHANNEL_REGISTERS_GROUP_SIZE;
	int ret;

	/* Release the channel from sleep mode, resetting all other fields at the same time. */
	can_rcar_rscanfd_write(dev, base_offset + RSCANFD_CFDCNCTR,
		RSCANFD_CFDCNCTR_CHMDC_CHANNEL_RESET_REQUEST << RSCANFD_CFDCNCTR_CHMDC_SHIFT);

	ret = can_rcar_rscanfd_busy_wait(config->reg + base_offset + RSCANFD_CFDCNSTS,
		RSCANFD_CFDCNSTS_CSLPSTS, 0);
	if (ret != 0) {
		LOG_ERR("Leaving the sleep mode for %s took too long.", dev->name);
		return ret;
	}

	return 0;
}

static int can_rcar_rscanfd_channel_enter_reset_mode(const struct device *dev)
{
	const struct can_rcar_rscanfd_config *config = dev->config;
	uint32_t base_offset, val;
	int ret;

	base_offset = config->channel * CAN_RCAR_RSCANFD_CAN_CHANNEL_REGISTERS_GROUP_SIZE;

	val = can_rcar_rscanfd_read(dev, base_offset + RSCANFD_CFDCNCTR);
	val &= ~(RSCANFD_CFDCNCTR_CHMDC_MASK << RSCANFD_CFDCNCTR_CHMDC_SHIFT);
	val |= RSCANFD_CFDCNCTR_CHMDC_CHANNEL_RESET_REQUEST << RSCANFD_CFDCNCTR_CHMDC_SHIFT;
	can_rcar_rscanfd_write(dev, base_offset + RSCANFD_CFDCNCTR, val);

	ret = can_rcar_rscanfd_busy_wait(config->reg + base_offset + RSCANFD_CFDCNSTS,
		RSCANFD_CFDCNSTS_CRSTSTS, 1);
	if (ret != 0) {
		LOG_ERR("Entering the reset mode for %s took too long.", dev->name);
		return ret;
	}

	return 0;
}

/* Note : the halt state is applied at the channel level, not impacting the other channels. */
static int can_rcar_rscanfd_channel_enter_halt_mode(const struct device *dev)
{
	const struct can_rcar_rscanfd_config *config = dev->config;
	uint32_t base_offset, val;

	base_offset = config->channel * CAN_RCAR_RSCANFD_CAN_CHANNEL_REGISTERS_GROUP_SIZE;

	val = can_rcar_rscanfd_read(dev, base_offset + RSCANFD_CFDCNCTR);
	val &= ~(RSCANFD_CFDCNCTR_CHMDC_MASK << RSCANFD_CFDCNCTR_CHMDC_SHIFT);
	val |= RSCANFD_CFDCNCTR_CHMDC_CHANNEL_HALT_REQUEST << RSCANFD_CFDCNCTR_CHMDC_SHIFT;
	can_rcar_rscanfd_write(dev, base_offset + RSCANFD_CFDCNCTR, val);

	return can_rcar_rscanfd_busy_wait(config->reg + base_offset + RSCANFD_CFDCNSTS,
		RSCANFD_CFDCNSTS_CHLTSTS, 1);
}

static int can_rcar_rscanfd_channel_enter_operation_mode(const struct device *dev)
{
	const struct can_rcar_rscanfd_config *config = dev->config;
	uint32_t base_offset, val;
	int ret;

	base_offset = config->channel * CAN_RCAR_RSCANFD_CAN_CHANNEL_REGISTERS_GROUP_SIZE;

	val = can_rcar_rscanfd_read(dev, base_offset + RSCANFD_CFDCNCTR);
	val &= ~(RSCANFD_CFDCNCTR_CHMDC_MASK << RSCANFD_CFDCNCTR_CHMDC_SHIFT);
	val |= RSCANFD_CFDCNCTR_CHMDC_CHANNEL_OPERATION_REQUEST << RSCANFD_CFDCNCTR_CHMDC_SHIFT;
	can_rcar_rscanfd_write(dev, base_offset + RSCANFD_CFDCNCTR, val);

	ret = can_rcar_rscanfd_busy_wait(config->reg + base_offset + RSCANFD_CFDCNSTS,
		RSCANFD_CFDCNSTS_CRSTSTS, 0);
	if (ret != 0) {
		LOG_ERR("Going from reset to operation mode for %s took too long.", dev->name);
		return ret;
	}
	ret = can_rcar_rscanfd_busy_wait(config->reg + base_offset + RSCANFD_CFDCNSTS,
		RSCANFD_CFDCNSTS_CHLTSTS, 0);
	if (ret != 0) {
		LOG_ERR("Going from halt to operation mode for %s took too long.", dev->name);
		return ret;
	}

	return 0;
}

static void can_rcar_rscanfd_set_communication_enabled(const struct device *dev, bool enabled)
{
	const struct can_rcar_rscanfd_config *config = dev->config;
	uint32_t base_offset, val;

	/* The first Common FIFO dedicated to the channel is used for reception */
	base_offset = config->channel * sizeof(uint32_t) *
		CAN_RCAR_RSCANFD_COMMON_FIFO_PER_CHANNEL;
	val = can_rcar_rscanfd_read(dev, base_offset + RSCANFD_CFDCFCCN);
	if (enabled) {
		val |= RSCANFD_CFDCFCCN_CFE;
	} else {
		val &= ~RSCANFD_CFDCFCCN_CFE;
	}
	can_rcar_rscanfd_write(dev, base_offset + RSCANFD_CFDCFCCN, val);

	/* Enable the TX queue */
	base_offset = config->channel * sizeof(uint32_t);
	val = can_rcar_rscanfd_read(dev, base_offset + RSCANFD_CFDTXQCC0N);
	if (enabled) {
		val |= RSCANFD_CFDTXQCC0N_TXQE;
	} else {
		val &= ~RSCANFD_CFDTXQCC0N_TXQE;
	}
	can_rcar_rscanfd_write(dev, base_offset + RSCANFD_CFDTXQCC0N, val);

	/* Enable the corresponding TX history list */
	val = can_rcar_rscanfd_read(dev, base_offset + RSCANFD_CFDTHLCCN);
	if (enabled) {
		val |= RSCANFD_CFDTHLCCN_THLE;
	} else {
		val &= ~RSCANFD_CFDTHLCCN_THLE;
	}
	can_rcar_rscanfd_write(dev, base_offset + RSCANFD_CFDTHLCCN, val);
}

/*
 * Configure the rules table by creating one rule that matches them all (reception frames).
 * The reception filters are currently implemented in software.
 */
static inline void can_rcar_rscanfd_configure_acceptance_filter_list(const struct device *dev)
{
	const struct can_rcar_rscanfd_config *config = dev->config;
	uint32_t base_offset, val, shift;

	/*
	 * Enable write access for the page 0 (set the page index 0 value in the same time).
	 * As all channels will use a single rule, only the first page is needed.
	 */
	can_rcar_rscanfd_write(dev, RSCANFD_CFDGAFLECTR, RSCANFD_CFDGAFLECTR_AFLDAE);

	/* There are two consecutive channel rules per CFDGAFLCFGn register */
	base_offset = RSCANFD_CFDGAFLCFGN + (config->channel / 2) * sizeof(uint32_t);
	/* Address the correct channel within the register */
	val = can_rcar_rscanfd_read(dev, base_offset);
	if (config->channel & 1) { /* Odd channel */
		shift = RSCANFD_CFDGAFLCFGN_RNC_CHANNEL_ODD_SHIFT;
	} else {
		shift = RSCANFD_CFDGAFLCFGN_RNC_CHANNEL_EVEN_SHIFT;
	}
	val &= ~(RSCANFD_CFDGAFLCFGN_RNC_CHANNEL_MASK << shift);
	/* Configure one rule for the channel */
	val |= 1 << shift;
	can_rcar_rscanfd_write(dev, base_offset, val);

	/* A page contains 16 consecutive entries */
	base_offset = config->channel * CAN_RCAR_RSCANFD_AFL_ENTRY_SIZE;
	/* Clear the CAN ID as it won't be taken into account by the mask register */
	can_rcar_rscanfd_write(dev, base_offset + RSCANFD_CFDGAFLIDN, 0);
	/* Accept all received CAN frames */
	can_rcar_rscanfd_write(dev, base_offset + RSCANFD_CFDGAFLMN, 0);
	/* Disable the DLC check */
	can_rcar_rscanfd_write(dev, base_offset + RSCANFD_CFDGAFLP0N, 0);
	/* Use the first Common FIFO dedicated to each channel as target for reception */
	val = config->channel * CAN_RCAR_RSCANFD_COMMON_FIFO_PER_CHANNEL;
	can_rcar_rscanfd_write(dev, base_offset + RSCANFD_CFDGAFLP1N, 0x100 << val);

	/* Disable write access for page 0 */
	can_rcar_rscanfd_write(dev, RSCANFD_CFDGAFLECTR, 0);
}

static inline void can_rcar_rscanfd_configure_communication_path(const struct device *dev)
{
	const struct can_rcar_rscanfd_config *config = dev->config;
	uint32_t base_offset;

	/* Use the TX Queue 0 for transmission */
	base_offset = config->channel * sizeof(uint32_t);

	/* Configure the queue depth */
	can_rcar_rscanfd_write(dev, base_offset + RSCANFD_CFDTXQCC0N,
		RSCANFD_CFDTXQCC0N_TXQDC_DEPTH_32_MSG << RSCANFD_CFDTXQCC0N_TXQDC_SHIFT);

	/* Configure the TX history list, which is used to identify the transmitted frames */
	can_rcar_rscanfd_write(dev, base_offset + RSCANFD_CFDTHLCCN,
		RSCANFD_CFDTHLCCN_THLIE | /* Enable the TX History List interrupt */
		RSCANFD_CFDTHLCCN_THLIM); /* Generate an interrupt for every stored entry */

	/* Use the first Common FIFO for reception */
	base_offset = config->channel * sizeof(uint32_t) *
		CAN_RCAR_RSCANFD_COMMON_FIFO_PER_CHANNEL;
	can_rcar_rscanfd_write(dev, base_offset + RSCANFD_CFDCFCCN,
		RSCANFD_CFDCFCCN_CFDC_DEPTH_64 << RSCANFD_CFDCFCCN_CFDC_SHIFT |
		RSCANFD_CFDCFCCN_CFIM_EVERY_MESSAGE << RSCANFD_CFDCFCCN_CFIM_SHIFT |
		RSCANFD_CFDCFCCN_CFM_RX << RSCANFD_CFDCFCCN_CFM_SHIFT |
		RSCANFD_CFDCFCCN_CFPLS_SIZE_64 << RSCANFD_CFDCFCCN_CFPLS_SHIFT |
		RSCANFD_CFDCFCCN_CFRXIE_INT_ENABLED << RSCANFD_CFDCFCCN_CFRXIE_SHIFT);
}

static inline void can_rcar_rscanfd_configure_errors(const struct device *dev)
{
	const struct can_rcar_rscanfd_config *config = dev->config;
	uint32_t base_offset, val;

	base_offset = config->channel * CAN_RCAR_RSCANFD_CAN_CHANNEL_REGISTERS_GROUP_SIZE;

	val = can_rcar_rscanfd_read(dev, base_offset + RSCANFD_CFDCNCTR);
	val |= RSCANFD_CFDCNCTR_ALIE | RSCANFD_CFDCNCTR_BORIE | RSCANFD_CFDCNCTR_BOEIE |
		RSCANFD_CFDCNCTR_EPIE | RSCANFD_CFDCNCTR_EWIE | RSCANFD_CFDCNCTR_BEIE;
	can_rcar_rscanfd_write(dev, base_offset + RSCANFD_CFDCNCTR, val);
}

/*
 * Configure the CAN 2.0B timings (not the CAN FD ones).
 * Note : the channel must already be in Reset or Halt state.
 */
static void can_rcar_rscanfd_configure_timing(const struct device *dev,
					      const struct can_timing *timing)
{
	const struct can_rcar_rscanfd_config *config = dev->config;
	uint32_t base_offset;

	LOG_DBG("Set timing for %s, sjw=%u, prop_seg=%u, seg1=%u, seg2=%u, presc=%u.",
		dev->name, timing->sjw, timing->prop_seg, timing->phase_seg1,
		timing->phase_seg2, timing->prescaler);

	/* Each channel is handled by a group of four 4-byte registers */
	base_offset = config->channel * CAN_RCAR_RSCANFD_CAN_CHANNEL_REGISTERS_GROUP_SIZE;

	can_rcar_rscanfd_write(dev, base_offset + RSCANFD_CFDCNNCFG,
		(timing->phase_seg1 + timing->prop_seg - 1) << RSCANFD_CFDCNNCFG_NTSEG1_SHIFT |
		(timing->phase_seg2 - 1) << RSCANFD_CFDCNNCFG_NTSEG2_SHIFT |
		timing->sjw << RSCANFD_CFDCNNCFG_NSJW_SHIFT |
		(timing->prescaler - 1) << RSCANFD_CFDCNNCFG_NBRP_SHIFT);
}

static void can_rcar_rscanfd_get_error_count(const struct device *dev,
					     struct can_bus_err_cnt *err_cnt)
{
	const struct can_rcar_rscanfd_config *config = dev->config;
	uint32_t base_offset, val;

	base_offset = config->channel * CAN_RCAR_RSCANFD_CAN_CHANNEL_REGISTERS_GROUP_SIZE;

	val = can_rcar_rscanfd_read(dev, base_offset + RSCANFD_CFDCNSTS);
	err_cnt->tx_err_cnt = (val >> RSCANFD_CFDCNSTS_TEC_SHIFT) & RSCANFD_CFDCNSTS_TEC_MASK;
	err_cnt->rx_err_cnt = (val >> RSCANFD_CFDCNSTS_REC_SHIFT) & RSCANFD_CFDCNSTS_REC_MASK;
}

static void can_rcar_rscanfd_state_change(const struct device *dev, uint32_t new_state)
{
	struct can_rcar_rscanfd_data *data = dev->data;
	const can_state_change_callback_t callback = data->common.state_change_cb;
	struct can_bus_err_cnt err_cnt;

	if (data->state == new_state) {
		return;
	}

	LOG_DBG("Changing %s state from %u to %u.\n", dev->name, data->state, new_state);

	data->state = new_state;

	if (callback != NULL) {
		can_rcar_rscanfd_get_error_count(dev, &err_cnt);
		callback(dev, new_state, err_cnt, data->common.state_change_cb_user_data);
	}
}

static int can_rcar_rscanfd_get_capabilities(const struct device *dev, can_mode_t *cap)
{
	ARG_UNUSED(dev);

	*cap = CAN_MODE_NORMAL | CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY;

#ifdef CONFIG_CAN_FD_MODE
	*cap |= CAN_MODE_FD;
#endif

#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
	*cap |= CAN_MODE_MANUAL_RECOVERY;
#endif

	return 0;
}

static int can_rcar_rscanfd_start(const struct device *dev)
{
	const struct can_rcar_rscanfd_config *config = dev->config;
	struct can_rcar_rscanfd_data *data = dev->data;
	int ret;
	uint32_t base_offset, val;
	stack_data_t stack_data;

	if (data->common.started) {
		return -EALREADY;
	}

	if (config->common.phy != NULL) {
		ret = can_transceiver_enable(config->common.phy, data->common.mode);
		if (ret != 0) {
			LOG_ERR("Failed to enable the CAN transceiver for %s (%d).",
				dev->name, ret);
			return ret;
		}
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	/* Reset all hardware error counters, along with the TX queue history list */
	if (can_rcar_rscanfd_channel_enter_reset_mode(dev) != 0) {
		ret = -EIO;
		goto exit_unlock;
	}

	CAN_STATS_RESET(dev);
	data->state = CAN_STATE_ERROR_ACTIVE;

	/* Resetting the error counters also resets the potentially applied test mode */
	if (data->common.mode & (CAN_MODE_LISTENONLY | CAN_MODE_LOOPBACK)) {
		/* The test modes can only be set in halt mode */
		ret = can_rcar_rscanfd_channel_enter_halt_mode(dev);
		if (ret != 0) {
			ret = -EIO;
			goto exit_unlock;
		}

		ret = can_rcar_rscanfd_set_mode(dev, data->common.mode);
		if (ret != 0) {
			ret = -EIO;
			goto exit_unlock;
		}
	}

	/* Discard any previous channel interrupt, starting from the error-related ones */
	base_offset = config->channel * CAN_RCAR_RSCANFD_CAN_CHANNEL_REGISTERS_GROUP_SIZE +
		RSCANFD_CFDCNERFL;
	val = can_rcar_rscanfd_read(dev, base_offset);
	val &= (RSCANFD_CFDCNERFL_ALF | RSCANFD_CFDCNERFL_BORF | RSCANFD_CFDCNERFL_BOEF |
		RSCANFD_CFDCNERFL_EPF | RSCANFD_CFDCNERFL_EWF | RSCANFD_CFDCNERFL_BEF);
	can_rcar_rscanfd_write(dev, base_offset, val);

	/* Discard the previous transmission interrupts on the channel TX queue 0 history list */
	base_offset = config->channel * sizeof(uint32_t);
	val = can_rcar_rscanfd_read(dev, base_offset + RSCANFD_CFDTHLSTSN);
	val &= ~RSCANFD_CFDTHLSTSN_THLIF;
	can_rcar_rscanfd_write(dev, base_offset + RSCANFD_CFDTHLSTSN, val);

	/* Discard the previous reception interrupts on the channel first Common FIFO */
	base_offset = config->channel * sizeof(uint32_t) *
		CAN_RCAR_RSCANFD_COMMON_FIFO_PER_CHANNEL;
	val = can_rcar_rscanfd_read(dev, base_offset + RSCANFD_CFDCFSTSN);
	val &= ~RSCANFD_CFDCFSTSN_CFRXIF;
	can_rcar_rscanfd_write(dev, base_offset + RSCANFD_CFDCFSTSN, val);

	/* Free all TX user callbacks array entries */
	/* Clear the current stack content */
	while (k_stack_pop(data->tx_free_index_stack, &stack_data, K_NO_WAIT) == 0) {
		;
	}
	/* Add all available indexes */
	for (uint32_t i = 0; i < CAN_RCAR_RSCANFD_TX_QUEUE_BUFFERS_COUNT; i++) {
		k_stack_push(data->tx_free_index_stack, i);
	}

	if (can_rcar_rscanfd_channel_enter_operation_mode(dev) != 0) {
		ret = -EIO;
		goto exit_unlock;
	}

	/* The FIFO and TX Queue can be enabled only when the channel is in operation mode */
	can_rcar_rscanfd_set_communication_enabled(dev, true);

	data->common.started = true;

	ret = 0;

exit_unlock:
	k_mutex_unlock(&data->inst_mutex);

	return ret;
}

static int can_rcar_rscanfd_stop(const struct device *dev)
{
	const struct can_rcar_rscanfd_config *config = dev->config;
	struct can_rcar_rscanfd_data *data = dev->data;
	int ret;

	if (!data->common.started) {
		return -EALREADY;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	/* Terminate the ongoing transmission */
	ret = can_rcar_rscanfd_channel_enter_halt_mode(dev);
	if (ret != 0) {
		ret = -EIO;
		goto exit_unlock;
	}

	/* Stop transmitting and receiving */
	can_rcar_rscanfd_set_communication_enabled(dev, false);

	if (config->common.phy != NULL) {
		ret = can_transceiver_disable(config->common.phy);
		if (ret != 0) {
			LOG_ERR("Failed to disable the CAN transceiver (%d).", ret);
			goto exit_unlock;
		}
	}

	data->common.started = false;
	data->state = CAN_STATE_STOPPED;

	ret = 0;

exit_unlock:
	k_mutex_unlock(&data->inst_mutex);

	return ret;
}

static int can_rcar_rscanfd_set_mode(const struct device *dev, can_mode_t mode)
{
	const struct can_rcar_rscanfd_config *config = dev->config;
	struct can_rcar_rscanfd_data *data = dev->data;
	uint32_t base_offset, val;
	can_mode_t supported_mode;

	LOG_DBG("Set mode 0x%08X for %s (current mode is 0x%08X).",
		mode, dev->name, data->common.mode);

	if (data->common.started) {
		return -EBUSY;
	}

	/* Safety checks */
	if ((mode & (CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY)) ==
	    (CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY)) {
		LOG_ERR("Enabling both loopback and listen-only modes is not supported.");
		return -ENOTSUP;
	}
	can_rcar_rscanfd_get_capabilities(dev, &supported_mode);
	if (mode & ~supported_mode) {
		LOG_ERR("Unsupported mode.");
		return -ENOTSUP;
	}

	/* Each channel is handled by a group of four 4-byte registers */
	base_offset = config->channel * CAN_RCAR_RSCANFD_CAN_CHANNEL_REGISTERS_GROUP_SIZE;

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	/* Make sure to cancel any previously set mode */
	val = can_rcar_rscanfd_read(dev, base_offset + RSCANFD_CFDCNCTR);
	val &= ~((RSCANFD_CFDCNCTR_CTMS_MASK << RSCANFD_CFDCNCTR_CTMS_SHIFT) |
		(RSCANFD_CFDCNCTR_CTME_MASK << RSCANFD_CFDCNCTR_CTME_SHIFT));

	if (mode & CAN_MODE_LOOPBACK) {
		/* Internally connect TX to RX, unconnecting RX from the physical pin */
		val |= (RSCANFD_CFDCNCTR_CTMS_INTERNAL_LOOPBACK_MODE <<
				RSCANFD_CFDCNCTR_CTMS_SHIFT) |
			(RSCANFD_CFDCNCTR_CTME_ENABLED << RSCANFD_CFDCNCTR_CTME_SHIFT);
	} else if (mode & CAN_MODE_LISTENONLY) {
		/* The can_send() function will prevent any transmission */
		val |= (RSCANFD_CFDCNCTR_CTMS_LISTEN_ONLY_MODE << RSCANFD_CFDCNCTR_CTMS_SHIFT) |
			(RSCANFD_CFDCNCTR_CTME_ENABLED << RSCANFD_CFDCNCTR_CTME_SHIFT);
	} else {
		val |= RSCANFD_CFDCNCTR_CTME_DISABLED << RSCANFD_CFDCNCTR_CTME_SHIFT;
	}
	can_rcar_rscanfd_write(dev, base_offset + RSCANFD_CFDCNCTR, val);

#ifdef CONFIG_CAN_FD_MODE
	base_offset = config->channel * CAN_RCAR_RSCANFD_CANFD_CHANNEL_REGISTERS_GROUP_SIZE;
	val = can_rcar_rscanfd_read(dev, base_offset + RSCANFD_CFDCNFDCFG);

	/* Configure the CAN FD to work alongside the CAN 2.0 */
	if (mode & CAN_MODE_FD) {
		/* Disable both "classic CAN only" and "FD only" modes */
		val &= ~(RSCANFD_CFDCNFDCFG_CLOE | RSCANFD_CFDCNFDCFG_FDOE);

		/* Enable the automatic transceiver delay compensation */
		/* Use the measured compensation + a manually set offset of 0 */
		val &= ~RSCANFD_CFDCNFDCFG_TDCOC;
		val &= ~(RSCANFD_CFDCNFDCFG_TDCO_MASK << RSCANFD_CFDCNFDCFG_TDCO_SHIFT);
		val |= RSCANFD_CFDCNFDCFG_TDCE;
	} else {
		/*
		 * Disable the transceiver delay compensation when on classic CAN is used, as
		 * recommended by the datasheet
		 */
		val &= ~RSCANFD_CFDCNFDCFG_TDCE;
		/* Disable CAN FD */
		val |= RSCANFD_CFDCNFDCFG_CLOE;
	}

	can_rcar_rscanfd_write(dev, base_offset + RSCANFD_CFDCNFDCFG, val);
#endif

	data->common.mode = mode;

	k_mutex_unlock(&data->inst_mutex);

	return 0;
}

static int can_rcar_rscanfd_set_timing(const struct device *dev, const struct can_timing *timing)
{
	struct can_rcar_rscanfd_data *data = dev->data;
	int ret;

	if (data->common.started) {
		return -EBUSY;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	/* The timing registers can only be changed in channel reset or halt modes */
	ret = can_rcar_rscanfd_channel_enter_halt_mode(dev);
	if (ret != 0) {
		ret = -EIO;
		goto exit_unlock;
	}

	can_rcar_rscanfd_configure_timing(dev, timing);

	ret = 0;

exit_unlock:
	k_mutex_unlock(&data->inst_mutex);

	return ret;
}

static int can_rcar_rscanfd_send(const struct device *dev, const struct can_frame *frame,
				 k_timeout_t timeout, can_tx_callback_t callback,
				 void *user_data)
{
	const struct can_rcar_rscanfd_config *config = dev->config;
	struct can_rcar_rscanfd_data *data = dev->data;
	uint32_t base_offset, val, index;

	LOG_DBG("Sending %s, ID: 0x%X, ID type: %s, DLC: %u, flags: 0x%02X.",
		dev->name, frame->id,
		(frame->flags & CAN_FRAME_IDE) != 0 ? "extended" : "standard",
		frame->dlc, frame->flags);

#ifdef CONFIG_CAN_FD_MODE
	/* CAN FD frame */
	if (frame->flags & CAN_FRAME_FDF) {
		if (frame->dlc > CANFD_MAX_DLC) {
			LOG_ERR("CAN FD DLC of %d exceeds maximum of %u on %s.", frame->dlc,
				CANFD_MAX_DLC, dev->name);
			return -EINVAL;
		}
	/* Classic CAN frame */
	} else if (frame->dlc > CAN_MAX_DLC) {
		LOG_ERR("Classic CAN DLC of %d exceeds maximum of %u on %s.", frame->dlc,
			CAN_MAX_DLC, dev->name);
		return -EINVAL;
	}
#else
	if (frame->dlc > CAN_MAX_DLC) {
		LOG_ERR("DLC of %d exceeds maximum of %u on %s.", frame->dlc,
			CAN_MAX_DLC, dev->name);
		return -EINVAL;
	}
#endif

	if ((frame->flags & ~(CAN_FRAME_IDE | CAN_FRAME_RTR
#ifdef CONFIG_CAN_FD_MODE
		| CAN_FRAME_FDF | CAN_FRAME_BRS
#endif
		)) != 0) {
		LOG_ERR("Unsupported CAN frame flags 0x%02X for %s.", frame->flags, dev->name);
		return -ENOTSUP;
	}

	if (!data->common.started) {
		return -ENETDOWN;
	}

	/* The device is not allowed to transmit in listen-only mode */
	if (data->common.mode & CAN_MODE_LISTENONLY) {
		return -ENOTSUP;
	}

#ifdef CONFIG_CAN_FD_MODE
	/* A CAN FD frame can be sent only when the FD mode is enabled */
	if ((frame->flags & CAN_FRAME_FDF) && !(data->common.mode & CAN_MODE_FD)) {
		return -ENOTSUP;
	}
#endif

	/* Wait for an available transmission slot */
	if (k_sem_take(&data->tx_sem, timeout) != 0) {
		return -EAGAIN;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	/* Find the next free slot ID in the tx_callbacks array */
	if (k_stack_pop(data->tx_free_index_stack, (stack_data_t *)&index, K_NO_WAIT) != 0) {
		/*
		 * No free index is available, which means that all transmission buffers are
		 * currently occupied.
		 * The slot availability is synchronized by the tx_sem semaphore, so this error
		 * should never happen.
		 */
		k_mutex_unlock(&data->inst_mutex);
		k_sem_give(&data->tx_sem);
		return -EAGAIN;
	}

	data->tx_callbacks[index].callback = callback;
	data->tx_callbacks[index].user_data = user_data;

	/* The TX Queue 0 is used for transmission */
	base_offset = config->channel * CAN_RCAR_RSCANFD_TX_QUEUE_PER_CHANNEL *
		CAN_RCAR_RSCANFD_TMBCP_ENTRY_SIZE;

	/* ID and flags */
	val = (frame->id & RSCANFD_CFDTMIDBN_TMID_MASK) << RSCANFD_CFDTMIDBN_TMID_SHIFT;
	if (frame->flags & CAN_FRAME_IDE) {
		val |= RSCANFD_CFDTMIDBN_TMIDE;
	}
	if (frame->flags & CAN_FRAME_RTR) {
		val |= RSCANFD_CFDTMIDBN_TMRTR;
	}
	/* After sending the frame create an entry in the history list */
	val |= RSCANFD_CFDTMIDBN_THLEN;
	can_rcar_rscanfd_write(dev, base_offset + RSCANFD_CFDTMIDBN, val);

	/* DLC */
	can_rcar_rscanfd_write(dev, base_offset + RSCANFD_CFDTMPTRBN,
		(frame->dlc & RSCANFD_CFDTMPTRBN_TMDLC_MASK) << RSCANFD_CFDTMPTRBN_TMDLC_SHIFT);

	/*
	 * CAN FD settings.
	 * Note : the controller takes care of setting the ESI bit if the node becomes error
	 * passive, so force the RSCANFD_CFDTMFDCTRBN_TMESI bit to 0 (i.e. to error active) to
	 * let the controller handle the final bit value.
	 */
	val = 0;
	if (frame->flags & CAN_FRAME_FDF) {
		val |= RSCANFD_CFDTMFDCTRBN_TMFDF;
	}
	if (frame->flags & CAN_FRAME_BRS) {
		val |= RSCANFD_CFDTMFDCTRBN_TMBRS;
	}
	/* Add the user callback index to the TX Message Buffer Pointer field */
	val |= (index & RSCANFD_CFDTMFDCTRBN_TMPTR_MASK) << RSCANFD_CFDTMFDCTRBN_TMPTR_SHIFT;
	can_rcar_rscanfd_write(dev, base_offset + RSCANFD_CFDTMFDCTRBN, val);

	/* Data */
	val = can_dlc_to_bytes(frame->dlc);
	for (uint32_t i = 0; i < val; i++) {
		sys_write8(frame->data[i], config->reg + base_offset + RSCANFD_CFDTMDF0BN + i);
	}

	/* Ask for message transmission */
	base_offset = config->channel * sizeof(uint32_t);
	can_rcar_rscanfd_write(dev, base_offset + RSCANFD_CFDTXQPCTR0N, 0xFF);

	k_mutex_unlock(&data->inst_mutex);

	return 0;
}

static int can_rcar_rscanfd_add_rx_filter(const struct device *dev, can_rx_callback_t callback,
					  void *user_data, const struct can_filter *filter)
{
	struct can_rcar_rscanfd_data *data = dev->data;
	int i, ret;
	k_spinlock_key_t key;

	/*
	 * Take the mutex first to allow the scheduler to yield another thread that is also
	 * trying to access a driver API
	 */
	k_mutex_lock(&data->inst_mutex, K_FOREVER);
	/* Prevent the IRQ handler from accessing the rx_callback variables at the same time */
	key = k_spin_lock(&data->irq_spinlock);

	/* Search for the first empty entry */
	for (i = 0; i < CONFIG_CAN_RCAR_RSCANFD_MAX_FILTERS; i++) {
		if (data->rx_callback[i] == NULL) {
			data->rx_callback_user_data[i] = user_data;
			data->filter[i] = *filter;
			data->rx_callback[i] = callback;
			break;
		}
	}

	k_spin_unlock(&data->irq_spinlock, key);
	k_mutex_unlock(&data->inst_mutex);

	if (i == CONFIG_CAN_RCAR_RSCANFD_MAX_FILTERS) {
		ret = -ENOSPC;
	} else {
		ret = i;
	}

	return ret;
}

#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
static int can_rcar_rscanfd_recover(const struct device *dev, k_timeout_t timeout)
{
	const struct can_rcar_rscanfd_config *config = dev->config;
	struct can_rcar_rscanfd_data *data = dev->data;
	uint32_t base_offset, val;
	int64_t start_ticks;
	int ret = -EAGAIN;

	if (!data->common.started) {
		return -ENETDOWN;
	}

	if (!(data->common.mode & CAN_MODE_MANUAL_RECOVERY)) {
		return -ENOTSUP;
	}

	if (data->state != CAN_STATE_BUS_OFF) {
		return 0;
	}

	if (k_mutex_lock(&data->inst_mutex, K_FOREVER)) {
		return -EAGAIN;
	}

	/* Request to return from bus off */
	base_offset = config->channel * CAN_RCAR_RSCANFD_CAN_CHANNEL_REGISTERS_GROUP_SIZE;
	val = can_rcar_rscanfd_read(dev, base_offset + RSCANFD_CFDCNCTR);
	val |= RSCANFD_CFDCNCTR_RTBO;
	can_rcar_rscanfd_write(dev, base_offset + RSCANFD_CFDCNCTR, val);

	start_ticks = k_uptime_ticks();
	while (data->state == CAN_STATE_BUS_OFF) {
		if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
			continue;
		}

		if (k_uptime_ticks() - start_ticks >= timeout.ticks) {
			goto exit_unlock;
		}
	}

	ret = 0;

exit_unlock:
	k_mutex_unlock(&data->inst_mutex);
	return ret;
}
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE */

static void can_rcar_rscanfd_remove_rx_filter(const struct device *dev, int filter_id)
{
	struct can_rcar_rscanfd_data *data = dev->data;
	k_spinlock_key_t key;

	if ((filter_id < 0) || (filter_id >= CONFIG_CAN_RCAR_RSCANFD_MAX_FILTERS)) {
		LOG_ERR("Filter ID %d is out of bounds.", filter_id);
		return;
	}

	/*
	 * Take the mutex first to allow the scheduler to yield another thread that is also
	 * trying to access a driver API
	 */
	k_mutex_lock(&data->inst_mutex, K_FOREVER);
	/* Prevent the IRQ handler from accessing the rx_callback variables at the same time */
	key = k_spin_lock(&data->irq_spinlock);

	data->rx_callback[filter_id] = NULL;

	k_spin_unlock(&data->irq_spinlock, key);
	k_mutex_unlock(&data->inst_mutex);
}

static int can_rcar_rscanfd_get_state(const struct device *dev, enum can_state *state,
				      struct can_bus_err_cnt *err_cnt)
{
	struct can_rcar_rscanfd_data *data = dev->data;

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	if (state != NULL) {
		*state = data->state;
	}

	if (err_cnt != NULL) {
		can_rcar_rscanfd_get_error_count(dev, err_cnt);
	}

	k_mutex_unlock(&data->inst_mutex);

	return 0;
}

static void can_rcar_rscanfd_set_state_change_callback(const struct device *dev,
						       can_state_change_callback_t callback,
						       void *user_data)
{
	struct can_rcar_rscanfd_data *data = dev->data;

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	data->common.state_change_cb = callback;
	data->common.state_change_cb_user_data = user_data;

	k_mutex_unlock(&data->inst_mutex);
}

static int can_rcar_rscanfd_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct can_rcar_rscanfd_config *config = dev->config;
	const struct can_rcar_rscanfd_global_config *global_config = config->global_dev->config;

	return clock_control_get_rate(global_config->clock_dev,
		RCAR_CLOCK_SUBSYS(global_config->global_clk), rate);
}

static int can_rcar_rscanfd_get_max_filters(const struct device *dev, bool ide)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(ide);

	return CONFIG_CAN_RCAR_RSCANFD_MAX_FILTERS;
}

#ifdef CONFIG_CAN_FD_MODE
/*
 * Configure the CAN FD timings (not the CAN 2.0 ones).
 * Note : the channel must already be in Reset or Halt state.
 */
static void can_rcar_rscanfd_configure_timing_data(const struct device *dev,
						   const struct can_timing *timing)
{
	const struct can_rcar_rscanfd_config *config = dev->config;
	uint32_t base_offset;

	LOG_DBG("Set data timing for %s, sjw=%u, prop_seg=%u, seg1=%u, seg2=%u, presc=%u.",
		dev->name, timing->sjw, timing->prop_seg, timing->phase_seg1,
		timing->phase_seg2, timing->prescaler);

	base_offset = config->channel * CAN_RCAR_RSCANFD_CANFD_CHANNEL_REGISTERS_GROUP_SIZE;

	can_rcar_rscanfd_write(dev, base_offset + RSCANFD_CFDCNDCFG,
		(timing->phase_seg1 + timing->prop_seg - 1) << RSCANFD_CFDCNDCFG_DTSEG1_SHIFT |
		(timing->phase_seg2 - 1) << RSCANFD_CFDCNDCFG_DTSEG2_SHIFT |
		timing->sjw << RSCANFD_CFDCNDCFG_DSJW_SHIFT |
		(timing->prescaler - 1) << RSCANFD_CFDCNDCFG_DBRP_SHIFT);
}

static int can_rcar_rscanfd_set_timing_data(const struct device *dev,
					    const struct can_timing *timing)
{
	struct can_rcar_rscanfd_data *data = dev->data;
	int ret;

	if (data->common.started) {
		return -EBUSY;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	/* The timing registers can only be changed in channel reset or halt modes */
	ret = can_rcar_rscanfd_channel_enter_halt_mode(dev);
	if (ret != 0) {
		ret = -EIO;
		goto exit_unlock;
	}

	can_rcar_rscanfd_configure_timing_data(dev, timing);

	ret = 0;

exit_unlock:
	k_mutex_unlock(&data->inst_mutex);

	return ret;
}
#endif /* CONFIG_CAN_FD_MODE */

/* Clearing a CFDCnERFL bus error bit (from bit 14 to bit 8) requires some polling */
static void can_rcar_rscanfd_clear_error_bit(const struct device *dev, uint32_t bit_mask)
{
	const struct can_rcar_rscanfd_config *config = dev->config;
	uint32_t base_offset, val;

	base_offset = config->channel * CAN_RCAR_RSCANFD_CAN_CHANNEL_REGISTERS_GROUP_SIZE +
		RSCANFD_CFDCNERFL;

	/* Set the bit to 0 until it effectively changes to 0 */
	while (1) {
		val = can_rcar_rscanfd_read(dev, base_offset);
		if (val & bit_mask) {
			val &= ~bit_mask;
			can_rcar_rscanfd_write(dev, base_offset, val);
		} else {
			break;
		}
	}
}

static inline void can_rcar_rscanfd_error_isr(const struct device *dev, uint32_t irq_flags)
{
	const struct can_rcar_rscanfd_config *config = dev->config;
	uint32_t base_offset;

	/* Get more precision about a bus error */
	if (irq_flags & RSCANFD_CFDCNERFL_BEF) {
		if (irq_flags & RSCANFD_CFDCNERFL_ADERR) {
			CAN_STATS_ACK_ERROR_INC(dev);
			can_rcar_rscanfd_clear_error_bit(dev, RSCANFD_CFDCNERFL_ADERR);
			irq_flags &= ~RSCANFD_CFDCNERFL_ADERR;
		}
		if (irq_flags & RSCANFD_CFDCNERFL_B0ERR) {
			CAN_STATS_BIT0_ERROR_INC(dev);
			can_rcar_rscanfd_clear_error_bit(dev, RSCANFD_CFDCNERFL_B0ERR);
			irq_flags &= ~RSCANFD_CFDCNERFL_B0ERR;
		}
		if (irq_flags & RSCANFD_CFDCNERFL_B1ERR) {
			CAN_STATS_BIT1_ERROR_INC(dev);
			can_rcar_rscanfd_clear_error_bit(dev, RSCANFD_CFDCNERFL_B1ERR);
			irq_flags &= ~RSCANFD_CFDCNERFL_B1ERR;
		}
		if (irq_flags & RSCANFD_CFDCNERFL_CERR) {
			CAN_STATS_CRC_ERROR_INC(dev);
			can_rcar_rscanfd_clear_error_bit(dev, RSCANFD_CFDCNERFL_CERR);
			irq_flags &= ~RSCANFD_CFDCNERFL_CERR;
		}
		if (irq_flags & RSCANFD_CFDCNERFL_AERR) {
			CAN_STATS_ACK_ERROR_INC(dev);
			can_rcar_rscanfd_clear_error_bit(dev, RSCANFD_CFDCNERFL_AERR);
			irq_flags &= ~RSCANFD_CFDCNERFL_AERR;
		}
		if (irq_flags & RSCANFD_CFDCNERFL_FERR) {
			CAN_STATS_FORM_ERROR_INC(dev);
			can_rcar_rscanfd_clear_error_bit(dev, RSCANFD_CFDCNERFL_FERR);
			irq_flags &= ~RSCANFD_CFDCNERFL_FERR;
		}
		if (irq_flags & RSCANFD_CFDCNERFL_SERR) {
			CAN_STATS_STUFF_ERROR_INC(dev);
			can_rcar_rscanfd_clear_error_bit(dev, RSCANFD_CFDCNERFL_SERR);
			irq_flags &= ~RSCANFD_CFDCNERFL_SERR;
		}
	}

	if (irq_flags & RSCANFD_CFDCNERFL_EWF) {
		LOG_DBG("Error warning interrupt for %s.", dev->name);
		can_rcar_rscanfd_state_change(dev, CAN_STATE_ERROR_WARNING);
	}

	if (irq_flags & RSCANFD_CFDCNERFL_EPF) {
		LOG_DBG("Error passive interrupt for %s.", dev->name);
		can_rcar_rscanfd_state_change(dev, CAN_STATE_ERROR_PASSIVE);
	}

	if (irq_flags & RSCANFD_CFDCNERFL_BOEF) {
		LOG_DBG("Bus-off entry interrupt for %s.", dev->name);
		can_rcar_rscanfd_state_change(dev, CAN_STATE_BUS_OFF);
	}

	if (irq_flags & RSCANFD_CFDCNERFL_BORF) {
		LOG_DBG("Bus-off recover interrupt for %s.", dev->name);
		can_rcar_rscanfd_state_change(dev, CAN_STATE_BUS_OFF);
	}

	base_offset = config->channel * CAN_RCAR_RSCANFD_CAN_CHANNEL_REGISTERS_GROUP_SIZE;

	/* Clear the interrupt flag */
	irq_flags &= ~(RSCANFD_CFDCNERFL_ALF | RSCANFD_CFDCNERFL_BORF | RSCANFD_CFDCNERFL_BOEF |
		RSCANFD_CFDCNERFL_EPF | RSCANFD_CFDCNERFL_EWF | RSCANFD_CFDCNERFL_BEF);
	can_rcar_rscanfd_write(dev, base_offset + RSCANFD_CFDCNERFL, irq_flags);
}

static inline void can_rcar_rscanfd_tx_isr(const struct device *dev)
{
	const struct can_rcar_rscanfd_config *config = dev->config;
	struct can_rcar_rscanfd_data *data = dev->data;
	uint32_t val, base_offset, index;
	struct can_rcar_rscanfd_tx_callback *tx_callback;

	/* Find the index of the first frame that has been transmitted */
	base_offset = config->channel *
		CAN_RCAR_RSCANFD_TX_HISTORY_LIST_ACCESS_REGISTERS_GROUP_SIZE;
	val = can_rcar_rscanfd_read(dev, base_offset + RSCANFD_CFDTHLACC1N);
	index = (val >> RSCANFD_CFDTHLACC1N_TID_SHIFT) & RSCANFD_CFDTHLACC1N_TID_MASK;

	tx_callback = &data->tx_callbacks[index];
	tx_callback->callback(dev, 0, tx_callback->user_data);

	/* Give back the now free index to the stack */
	k_stack_push(data->tx_free_index_stack, index);

	/* Go to the next entry in the TX history list */
	base_offset = config->channel * sizeof(uint32_t);
	can_rcar_rscanfd_write(dev, base_offset + RSCANFD_CFDTHLPCTRN, 0xFF);
}

static inline void can_rcar_rscanfd_rx_isr(const struct device *dev)
{
	const struct can_rcar_rscanfd_config *config = dev->config;
	struct can_rcar_rscanfd_data *data = dev->data;
	uint32_t val, i, bytes_count, base_offset, source_addr;
	struct can_frame frame, user_frame;
	uint8_t *can_data;
	k_spinlock_key_t key;

	/* The channel first Common FIFO is used for reception */
	base_offset = config->channel * CAN_RCAR_RSCANFD_COMMON_FIFO_PER_CHANNEL *
		CAN_RCAR_RSCANFD_CFMBCP_ENTRY_SIZE;

	frame.flags = 0;
	val = can_rcar_rscanfd_read(dev, base_offset + RSCANFD_CFDCFIDBN);
	if (val & RSCANFD_CFDCFIDBN_CFIDE) {
		frame.flags |= CAN_FRAME_IDE;
	}
	if (val & RSCANFD_CFDCFIDBN_CFRTR) {
#ifndef CONFIG_CAN_ACCEPT_RTR
		goto exit_next_frame;
#endif
		frame.flags |= CAN_FRAME_RTR;
	}
	frame.id = (val >> RSCANFD_CFDCFIDBN_CFID_SHIFT) & RSCANFD_CFDCFIDBN_CFID_MASK;

	val = can_rcar_rscanfd_read(dev, base_offset + RSCANFD_CFDCFPTRBN);
	frame.dlc = (val >> RSCANFD_CFDCFPTRBN_CFDLC_SHIFT) & RSCANFD_CFDCFPTRBN_CFDLC_MASK;

	val = can_rcar_rscanfd_read(dev, base_offset + RSCANFD_CFDCFFDCSTSBN);
	if (val & RSCANFD_CFDCFFDCSTSBN_CFESI) {
		frame.flags |= CAN_FRAME_ESI;
	}
	if (val & RSCANFD_CFDCFFDCSTSBN_CFBRS) {
#ifndef CONFIG_CAN_FD_MODE
		/* The bit rate switch is possible only in FD mode */
		goto exit_next_frame;
#endif
		frame.flags |= CAN_FRAME_BRS;
	}
	if (val & RSCANFD_CFDCFFDCSTSBN_CFFDF) {
#ifndef CONFIG_CAN_FD_MODE
		goto exit_next_frame;
#endif
		frame.flags |= CAN_FRAME_FDF;
	}

	/* A CAN 2.0 frame data size is limited to 8 bytes */
	bytes_count = can_dlc_to_bytes(frame.dlc);
	if (!(frame.flags & CAN_FRAME_FDF) && (bytes_count > CAN_MAX_DLC)) {
		goto exit_next_frame;
	}

	/* Retrieve the data */
	source_addr = config->reg + base_offset + RSCANFD_CFDCFDF0BN;
	can_data = frame.data;
	for (i = 0; i < bytes_count; i++) {
		*can_data = sys_read8(source_addr);
		source_addr++;
		can_data++;
	}

	/* Prevent another core from accessing the rx_callback variables at the same time */
	key = k_spin_lock(&data->irq_spinlock);

	/* Check for all matching filters */
	for (i = 0; i < CONFIG_CAN_RCAR_RSCANFD_MAX_FILTERS; i++) {
		if (data->rx_callback[i] == NULL) {
			continue;
		}

		if (!can_frame_matches_filter(&frame, &data->filter[i])) {
			continue;
		}

		/*
		 * Make a temporary copy in case the user modifies the message in a first matching
		 * filter callback, and another filter also matches afterwards.
		 */
		user_frame = frame;
		data->rx_callback[i](dev, &user_frame, data->rx_callback_user_data[i]);
	}

	k_spin_unlock(&data->irq_spinlock, key);

exit_next_frame:
	/* Increment the FIFO read pointer to get access to the next received frame */
	base_offset = config->channel * CAN_RCAR_RSCANFD_COMMON_FIFO_PER_CHANNEL *
		sizeof(uint32_t);
	can_rcar_rscanfd_write(dev, base_offset + RSCANFD_CFDCFPCTR0, 0xFF);
}

static void can_rcar_rscanfd_isr(const struct device *dev)
{
	const struct can_rcar_rscanfd_config *config = dev->config;
	struct can_rcar_rscanfd_data *data = dev->data;
	uint32_t irq_flags, base_offset;

	/* Error */
	base_offset = config->channel * CAN_RCAR_RSCANFD_CAN_CHANNEL_REGISTERS_GROUP_SIZE;
	irq_flags = can_rcar_rscanfd_read(dev, base_offset + RSCANFD_CFDCNERFL);
	if (irq_flags & (RSCANFD_CFDCNERFL_ALF | RSCANFD_CFDCNERFL_BORF | RSCANFD_CFDCNERFL_BOEF |
		RSCANFD_CFDCNERFL_EPF | RSCANFD_CFDCNERFL_EWF | RSCANFD_CFDCNERFL_BEF)) {
		can_rcar_rscanfd_error_isr(dev, irq_flags);
	}

	/*
	 * Frame transmission, using the first TX queue of the channel.
	 * A frame is considered as transmitted when a new entry is added to the TX history list.
	 */
	base_offset = config->channel * sizeof(uint32_t);
	irq_flags = can_rcar_rscanfd_read(dev, base_offset + RSCANFD_CFDTHLSTSN);
	if (irq_flags & RSCANFD_CFDTHLSTSN_THLIF) {
		/* Handle all the frames that have been transmitted */
		do {
			can_rcar_rscanfd_tx_isr(dev);
			k_sem_give(&data->tx_sem);
		} while (!(can_rcar_rscanfd_read(dev, base_offset + RSCANFD_CFDTHLSTSN)
			& RSCANFD_CFDTHLSTSN_THLEMP));

		/* Clear the interrupt flag */
		irq_flags &= ~RSCANFD_CFDTHLSTSN_THLIF;
		can_rcar_rscanfd_write(dev, base_offset + RSCANFD_CFDTHLSTSN, irq_flags);
	}

	/* Frame reception, using the first Common FIFO of the channel */
	base_offset = config->channel * sizeof(uint32_t) *
		CAN_RCAR_RSCANFD_COMMON_FIFO_PER_CHANNEL;
	irq_flags = can_rcar_rscanfd_read(dev, base_offset + RSCANFD_CFDCFSTSN);
	if (irq_flags & RSCANFD_CFDCFSTSN_CFRXIF) {
		/* Handle all the messages contained in the FIFO */
		do {
			can_rcar_rscanfd_rx_isr(dev);
		} while (!(can_rcar_rscanfd_read(dev, base_offset + RSCANFD_CFDCFSTSN)
			& RSCANFD_CFDCFSTSN_CFEMP));

		/* Clear the interrupt flag */
		irq_flags &= ~RSCANFD_CFDCFSTSN_CFRXIF;
		can_rcar_rscanfd_write(dev, base_offset + RSCANFD_CFDCFSTSN, irq_flags);
	}
}

static int can_rcar_rscanfd_init(const struct device *dev)
{
	const struct can_rcar_rscanfd_config *config = dev->config;
	const struct can_rcar_rscanfd_global_config *global_config = config->global_dev->config;
	struct can_rcar_rscanfd_data *data = dev->data;
	struct can_rcar_rscanfd_global_data *global_data = config->global_dev->data;
	struct can_timing timing;
	int ret;

	/* Ensure that the global controller is already initialized */
	if (!device_is_ready(config->global_dev)) {
		LOG_ERR_DEVICE_NOT_READY(config->global_dev);
		return -ENODEV;
	}

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("Failed to apply the pinctrl default state.");
		return ret;
	}

	ret = can_rcar_rscanfd_channel_leave_sleep_mode(dev);
	if (ret != 0) {
		return ret;
	}

	can_rcar_rscanfd_configure_acceptance_filter_list(dev);

	can_rcar_rscanfd_configure_communication_path(dev);

	can_rcar_rscanfd_configure_errors(dev);

	ret = can_calc_timing(dev, &timing, config->common.bitrate, config->common.sample_point);
	if (ret < 0) {
		LOG_ERR("Failed to find a timing for %s bit rate %u bit/s (%d).",
			dev->name, config->common.bitrate, ret);
		return ret;
	}

	can_rcar_rscanfd_configure_timing(dev, &timing);

#ifdef CONFIG_CAN_FD_MODE
	ret = can_calc_timing_data(dev, &timing, config->common.bitrate_data,
		config->common.sample_point_data);
	if (ret < 0) {
		LOG_ERR("Failed to find a timing for %s data bit rate %u bit/s (%d).",
			dev->name, config->common.bitrate_data, ret);
		return ret;
	}

	can_rcar_rscanfd_configure_timing_data(dev, &timing);
#endif

	k_mutex_init(&data->inst_mutex);
	k_sem_init(&data->tx_sem, CAN_RCAR_RSCANFD_TX_QUEUE_BUFFERS_COUNT,
		CAN_RCAR_RSCANFD_TX_QUEUE_BUFFERS_COUNT);
	data->state = CAN_STATE_STOPPED;

	/* Register the channel device into the controller for later use */
	k_mutex_lock(&global_data->list_mutex, K_FOREVER);
	global_data->channel_dev[global_data->enabled_channels_count] = dev;
	k_mutex_unlock(&global_data->list_mutex);

	config->configure_irq();

	/* Switch the controller to operation mode when all channels are initialized */
	global_data->enabled_channels_count++;
	if (global_data->enabled_channels_count == global_config->num_enabled_channels) {
		ret = can_rcar_rscanfd_enter_operation_mode(global_config);
		if (ret) {
			LOG_ERR("Failed to put the controller in operation mode.");
			return ret;
		}

		/*
		 * The channels must go to halt mode while they are not used, also because this
		 * allows some additional channel configuration not possible in reset mode.
		 * The controller needs to be in operation mode.
		 *
		 * Note : no need for mutex here, because only the last registered channel
		 * init function can call this code.
		 */
		for (uint32_t i = 0; i < global_config->num_enabled_channels; i++) {
			ret = can_rcar_rscanfd_channel_enter_halt_mode(
				global_data->channel_dev[i]);
			if (ret != 0) {
				LOG_ERR("Failed to switch %s to halt mode.",
					global_data->channel_dev[i]->name);
				return ret;
			}

			/* Some working modes can only be set when the channel is in halt mode. */
			ret = can_rcar_rscanfd_set_mode(global_data->channel_dev[i],
				CAN_MODE_NORMAL);
			if (ret) {
				LOG_ERR("Failed to set %s mode to normal.",
					global_data->channel_dev[i]->name);
				return ret;
			}
		}

		LOG_DBG("All channels were successfully initialized.");
	}

	return 0;
}

static DEVICE_API(can, can_rcar_rscanfd_driver_api) = {
	.get_capabilities = can_rcar_rscanfd_get_capabilities,
	.start = can_rcar_rscanfd_start,
	.stop = can_rcar_rscanfd_stop,
	.set_mode = can_rcar_rscanfd_set_mode,
	.set_timing = can_rcar_rscanfd_set_timing,
	.send = can_rcar_rscanfd_send,
	.add_rx_filter = can_rcar_rscanfd_add_rx_filter,
	.remove_rx_filter = can_rcar_rscanfd_remove_rx_filter,
#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
	.recover = can_rcar_rscanfd_recover,
#endif
	.get_state = can_rcar_rscanfd_get_state,
	.set_state_change_callback = can_rcar_rscanfd_set_state_change_callback,
	.get_core_clock = can_rcar_rscanfd_get_core_clock,
	.get_max_filters = can_rcar_rscanfd_get_max_filters,
	.timing_min = {
		.sjw = 1,
		.prop_seg = 0,
		.phase_seg1 = 2,
		.phase_seg2 = 2,
		.prescaler = 1
	},
	.timing_max = {
		.sjw = 128,
		.prop_seg = 0,
		.phase_seg1 = 256,
		.phase_seg2 = 128,
		.prescaler = 1024
	},
#ifdef CONFIG_CAN_FD_MODE
	.set_timing_data = can_rcar_rscanfd_set_timing_data,
	.timing_data_min = {
		.sjw = 1,
		.prop_seg = 0,
		.phase_seg1 = 2,
		.phase_seg2 = 2,
		.prescaler = 1
	},
	.timing_data_max = {
		.sjw = 16,
		.prop_seg = 0,
		.phase_seg1 = 32,
		.phase_seg2 = 16,
		.prescaler = 256
	}
#endif
};

#ifdef CONFIG_CAN_FD_MODE
#define RSCANFD_MAX_BITRATE 8000000
#else
#define RSCANFD_MAX_BITRATE 1000000
#endif

/*
 * A channel of the CAN controller.
 */
#define RSCANFD_INIT(n)									\
	PINCTRL_DT_INST_DEFINE(n);							\
											\
	static void can_rcar_rscanfd_configure_irq_##n(void)				\
	{										\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),			\
			    can_rcar_rscanfd_isr, DEVICE_DT_INST_GET(n), 0);		\
		irq_enable(DT_INST_IRQN(n));						\
	}										\
											\
	static const struct can_rcar_rscanfd_config can_rcar_rscanfd_config_##n = {	\
		.common = CAN_DT_DRIVER_CONFIG_INST_GET(n, 0, RSCANFD_MAX_BITRATE),	\
		.global_dev = DEVICE_DT_GET(DT_INST_PARENT(n)),				\
		.reg = DT_REG_ADDR(DT_INST_PARENT(n)),					\
		.channel = DT_INST_PROP(n, channel),					\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),				\
		.configure_irq = can_rcar_rscanfd_configure_irq_##n			\
	};										\
	BUILD_ASSERT(DT_INST_PROP(n, channel) < CAN_RCAR_RSCANFD_CHANNELS_COUNT,	\
		     "Channel number is invalid.");					\
											\
	K_STACK_DEFINE(tx_stack_##n, CAN_RCAR_RSCANFD_TX_QUEUE_BUFFERS_COUNT);		\
	static struct can_rcar_rscanfd_data can_rcar_rscanfd_data_##n = {		\
		.tx_free_index_stack = &tx_stack_##n					\
	};										\
	CAN_DEVICE_DT_INST_DEFINE(n, can_rcar_rscanfd_init,				\
				  NULL,							\
				  &can_rcar_rscanfd_data_##n,				\
				  &can_rcar_rscanfd_config_##n,				\
				  POST_KERNEL,						\
				  CONFIG_CAN_INIT_PRIORITY,				\
				  &can_rcar_rscanfd_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RSCANFD_INIT);

/*
 * The CAN controller.
 */
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT renesas_rcar_rscanfd_global

static int can_rcar_rscanfd_global_init(const struct device *dev)
{
	const struct can_rcar_rscanfd_global_config *config = dev->config;
	struct can_rcar_rscanfd_global_data *data = dev->data;
	int ret;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("The clock device is not ready.");
		return -ENODEV;
	}

	ret = clock_control_on(config->clock_dev, RCAR_CLOCK_SUBSYS(config->global_clk));
	if (ret != 0) {
		LOG_ERR("Failed to turn the global clock on.");
		return ret;
	}

	/* Make sure that the clock is fast enough for 8Mbit/s CAN FD */
	ret = clock_control_set_rate(config->clock_dev, RCAR_CLOCK_SUBSYS(config->global_clk),
		(clock_control_subsys_rate_t)CAN_RCAR_RSCANFD_MODULE_CLOCK_RATE);
	if (ret != 0) {
		LOG_ERR("Failed to set the global clock rate to %u Hz.",
			CAN_RCAR_RSCANFD_MODULE_CLOCK_RATE);
		return ret;
	}

	ret = clock_control_on(config->clock_dev, RCAR_CLOCK_SUBSYS(config->module_clk));
	if (ret != 0) {
		LOG_ERR("Failed to turn the module clock on.");
		return ret;
	}

	/* Wait for the CAN RAM initialization to terminate */
	ret = can_rcar_rscanfd_busy_wait(config->reg + RSCANFD_CFDGSTS,
		RSCANFD_CFDGSTS_GRAMINIT, 0);
	if (ret != 0) {
		LOG_ERR("Internal RAM initialization took too long.");
		return ret;
	}

	/* The CAN module registers can be configured only in reset mode */
	ret = can_rcar_rscanfd_enter_reset_mode(config);
	if (ret != 0) {
		LOG_ERR("Failed to enter the reset mode.");
		return ret;
	}

	LOG_DBG("CAN controller IP version: 0x%08X, number of enabled channels: %u.",
		sys_read32(config->reg + RSCANFD_CFDGIPV), config->num_enabled_channels);

	k_mutex_init(&data->list_mutex);

	return 0;
}

#define RSCANFD_GLOBAL_INIT(n)									  \
	static const struct can_rcar_rscanfd_global_config can_rcar_rscanfd_global_config_##n = { \
		.reg = DT_INST_REG_ADDR(n),							  \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),				  \
		.global_clk = RCAR_DT_INST_CLOCKS_CELL_BY_IDX(n, 0),				  \
		.module_clk = RCAR_DT_INST_CLOCKS_CELL_BY_IDX(n, 1),				  \
		.num_enabled_channels = DT_INST_CHILD_NUM_STATUS_OKAY(n)			  \
	};											  \
												  \
	static struct can_rcar_rscanfd_global_data can_rcar_rscanfd_global_data_##n;		  \
												  \
	DEVICE_DT_INST_DEFINE(n, can_rcar_rscanfd_global_init,					  \
			 NULL,									  \
			 &can_rcar_rscanfd_global_data_##n,					  \
			 &can_rcar_rscanfd_global_config_##n,					  \
			 POST_KERNEL,								  \
			 CONFIG_KERNEL_INIT_PRIORITY_DEVICE,					  \
			 NULL);

DT_INST_FOREACH_STATUS_OKAY(RSCANFD_GLOBAL_INIT);

/* Make sure that the initialization order will be respected */
BUILD_ASSERT(CONFIG_CLOCK_CONTROL_INIT_PRIORITY < CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
	     "The clocks must be initialized before the CAN controller.");
BUILD_ASSERT(CONFIG_KERNEL_INIT_PRIORITY_DEVICE < CONFIG_CAN_INIT_PRIORITY,
	     "The CAN controller must be initialized before its channels.");
