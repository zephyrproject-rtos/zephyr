/*
 * Copyright (C) 2020 Samsung Electronics Co., Ltd.
 * Copyright (C) 2023 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util.h>
#include <assert.h>

#if defined(CONFIG_PINCTRL)
#include <zephyr/drivers/pinctrl.h>
#endif

#define NANO_SEC        1000000000ULL
#define BYTES_PER_DWORD 4

LOG_MODULE_REGISTER(i3c_dw, CONFIG_I3C_DW_LOG_LEVEL);

#define DEVICE_CTRL                0x0
#define DEV_CTRL_ENABLE            BIT(31)
#define DEV_CTRL_RESUME            BIT(30)
#define DEV_CTRL_HOT_JOIN_NACK     BIT(8)
#define DEV_CTRL_I2C_SLAVE_PRESENT BIT(7)
#define DEV_CTRL_IBA_INCLUDE       BIT(0)

#define DEVICE_ADDR                    0x4
#define DEVICE_ADDR_DYNAMIC_ADDR_VALID BIT(31)
#define DEVICE_ADDR_DYNAMIC(x)         (((x) << 16) & GENMASK(22, 16))
#define DEVICE_ADDR_STATIC_ADDR_VALID  BIT(15)
#define DEVICE_ADDR_STATIC_MASK        GENMASK(6, 0)
#define DEVICE_ADDR_STATIC(x)          ((x) & DEVICE_ADDR_STATIC_MASK)

#define HW_CAPABILITY                         0x8
#define HW_CAPABILITY_SLV_IBI_CAP             BIT(19)
#define HW_CAPABILITY_SLV_HJ_CAP              BIT(18)
#define HW_CAPABILITY_HDR_TS_EN               BIT(4)
#define HW_CAPABILITY_HDR_DDR_EN              BIT(3)
#define HW_CAPABILITY_DEVICE_ROLE_CONFIG_MASK GENMASK(2, 0)

#define COMMAND_QUEUE_PORT         0xc
#define COMMAND_PORT_TOC           BIT(30)
#define COMMAND_PORT_READ_TRANSFER BIT(28)
#define COMMAND_PORT_SDAP          BIT(27)
#define COMMAND_PORT_ROC           BIT(26)
#define COMMAND_PORT_DBP           BIT(25)
#define COMMAND_PORT_SPEED(x)      (((x) << 21) & GENMASK(23, 21))
#define COMMAND_PORT_SPEED_I2C_FM  0
#define COMMAND_PORT_SPEED_I2C_FMP 1
#define COMMAND_PORT_SPEED_I3C_DDR 6
#define COMMAND_PORT_SPEED_I3C_TS  7
#define COMMAND_PORT_DEV_INDEX(x)  (((x) << 16) & GENMASK(20, 16))
#define COMMAND_PORT_CP            BIT(15)
#define COMMAND_PORT_CMD(x)        (((x) << 7) & GENMASK(14, 7))
#define COMMAND_PORT_TID(x)        (((x) << 3) & GENMASK(6, 3))

#define COMMAND_PORT_ARG_DATA_LEN(x)  (((x) << 16) & GENMASK(31, 16))
#define COMMAND_PORT_ARG_DB(x)        (((x) << 8) & GENMASK(15, 8))
#define COMMAND_PORT_ARG_DATA_LEN_MAX 65536
#define COMMAND_PORT_TRANSFER_ARG     0x01

#define COMMAND_PORT_SDA_DATA_BYTE_3(x) (((x) << 24) & GENMASK(31, 24))
#define COMMAND_PORT_SDA_DATA_BYTE_2(x) (((x) << 16) & GENMASK(23, 16))
#define COMMAND_PORT_SDA_DATA_BYTE_1(x) (((x) << 8) & GENMASK(15, 8))
#define COMMAND_PORT_SDA_BYTE_STRB_3    BIT(5)
#define COMMAND_PORT_SDA_BYTE_STRB_2    BIT(4)
#define COMMAND_PORT_SDA_BYTE_STRB_1    BIT(3)
#define COMMAND_PORT_SHORT_DATA_ARG     0x02

#define COMMAND_PORT_DEV_COUNT(x)   (((x) << 21) & GENMASK(25, 21))
#define COMMAND_PORT_ADDR_ASSGN_CMD 0x03

#define RESPONSE_QUEUE_PORT            0x10
#define RESPONSE_PORT_ERR_STATUS(x)    (((x) & GENMASK(31, 28)) >> 28)
#define RESPONSE_NO_ERROR              0
#define RESPONSE_ERROR_CRC             1
#define RESPONSE_ERROR_PARITY          2
#define RESPONSE_ERROR_FRAME           3
#define RESPONSE_ERROR_IBA_NACK        4
#define RESPONSE_ERROR_ADDRESS_NACK    5
#define RESPONSE_ERROR_OVER_UNDER_FLOW 6
#define RESPONSE_ERROR_TRANSF_ABORT    8
#define RESPONSE_ERROR_I2C_W_NACK_ERR  9
#define RESPONSE_PORT_TID(x)           (((x) & GENMASK(27, 24)) >> 24)
#define RESPONSE_PORT_DATA_LEN(x)      ((x) & GENMASK(15, 0))

#define RX_TX_DATA_PORT              0x14
#define IBI_QUEUE_STATUS             0x18
#define IBI_QUEUE_STATUS_IBI_STS(x)  (((x) & GENMASK(31, 28)) >> 28)
#define IBI_QUEUE_STATUS_IBI_ID(x)   (((x) & GENMASK(15, 8)) >> 8)
#define IBI_QUEUE_STATUS_DATA_LEN(x) ((x) & GENMASK(7, 0))
#define IBI_QUEUE_IBI_ADDR(x)        (IBI_QUEUE_STATUS_IBI_ID(x) >> 1)
#define IBI_QUEUE_IBI_RNW(x)         (IBI_QUEUE_STATUS_IBI_ID(x) & BIT(0))
#define IBI_TYPE_MR(x) \
	((IBI_QUEUE_IBI_ADDR(x) != I3C_HOT_JOIN_ADDR) && !IBI_QUEUE_IBI_RNW(x))
#define IBI_TYPE_HJ(x) \
	((IBI_QUEUE_IBI_ADDR(x) == I3C_HOT_JOIN_ADDR) && !IBI_QUEUE_IBI_RNW(x))
#define IBI_TYPE_SIRQ(x) \
	((IBI_QUEUE_IBI_ADDR(x) != I3C_HOT_JOIN_ADDR) && IBI_QUEUE_IBI_RNW(x))

#define QUEUE_THLD_CTRL               0x1c
#define QUEUE_THLD_CTRL_IBI_STS_MASK  GENMASK(31, 24)
#define QUEUE_THLD_CTRL_RESP_BUF_MASK GENMASK(15, 8)
#define QUEUE_THLD_CTRL_RESP_BUF(x)   (((x) - 1) << 8)

#define DATA_BUFFER_THLD_CTRL        0x20
#define DATA_BUFFER_THLD_CTRL_RX_BUF GENMASK(11, 8)

#define IBI_QUEUE_CTRL     0x24
#define IBI_MR_REQ_REJECT  0x2C
#define IBI_SIR_REQ_REJECT 0x30
#define IBI_SIR_REQ_ID(x)  ((((x) & GENMASK(6, 5)) >> 5) + ((x) & GENMASK(4, 0)))
#define IBI_REQ_REJECT_ALL GENMASK(31, 0)

#define RESET_CTRL            0x34
#define RESET_CTRL_IBI_QUEUE  BIT(5)
#define RESET_CTRL_RX_FIFO    BIT(4)
#define RESET_CTRL_TX_FIFO    BIT(3)
#define RESET_CTRL_RESP_QUEUE BIT(2)
#define RESET_CTRL_CMD_QUEUE  BIT(1)
#define RESET_CTRL_SOFT       BIT(0)
#define RESET_CTRL_ALL                                                                             \
	(RESET_CTRL_IBI_QUEUE | RESET_CTRL_RX_FIFO | RESET_CTRL_TX_FIFO | RESET_CTRL_RESP_QUEUE |  \
	 RESET_CTRL_CMD_QUEUE | RESET_CTRL_SOFT)

#define SLV_EVENT_STATUS        0x38
#define SLV_EVENT_STATUS_HJ_EN  BIT(3)
#define SLV_EVENT_STATUS_MR_EN  BIT(1)
#define SLV_EVENT_STATUS_SIR_EN BIT(0)

#define INTR_STATUS               0x3c
#define INTR_STATUS_EN            0x40
#define INTR_SIGNAL_EN            0x44
#define INTR_FORCE                0x48
#define INTR_BUSOWNER_UPDATE_STAT BIT(13)
#define INTR_IBI_UPDATED_STAT     BIT(12)
#define INTR_READ_REQ_RECV_STAT   BIT(11)
#define INTR_DEFSLV_STAT          BIT(10)
#define INTR_TRANSFER_ERR_STAT    BIT(9)
#define INTR_DYN_ADDR_ASSGN_STAT  BIT(8)
#define INTR_CCC_UPDATED_STAT     BIT(6)
#define INTR_TRANSFER_ABORT_STAT  BIT(5)
#define INTR_RESP_READY_STAT      BIT(4)
#define INTR_CMD_QUEUE_READY_STAT BIT(3)
#define INTR_IBI_THLD_STAT        BIT(2)
#define INTR_RX_THLD_STAT         BIT(1)
#define INTR_TX_THLD_STAT         BIT(0)
#define INTR_ALL                                                                                   \
	(INTR_BUSOWNER_UPDATE_STAT | INTR_IBI_UPDATED_STAT | INTR_READ_REQ_RECV_STAT |             \
	 INTR_DEFSLV_STAT | INTR_TRANSFER_ERR_STAT | INTR_DYN_ADDR_ASSGN_STAT |                    \
	 INTR_CCC_UPDATED_STAT | INTR_TRANSFER_ABORT_STAT | INTR_RESP_READY_STAT |                 \
	 INTR_CMD_QUEUE_READY_STAT | INTR_IBI_THLD_STAT | INTR_TX_THLD_STAT | INTR_RX_THLD_STAT)

#ifdef CONFIG_I3C_USE_IBI
#define INTR_MASTER_MASK (INTR_TRANSFER_ERR_STAT | INTR_RESP_READY_STAT | INTR_IBI_THLD_STAT)
#else
#define INTR_MASTER_MASK (INTR_TRANSFER_ERR_STAT | INTR_RESP_READY_STAT)
#endif
#define INTR_SLAVE_MASK                                                                            \
	(INTR_TRANSFER_ERR_STAT | INTR_IBI_UPDATED_STAT | INTR_READ_REQ_RECV_STAT |                \
	 INTR_DYN_ADDR_ASSGN_STAT | INTR_RESP_READY_STAT)

#define QUEUE_STATUS_LEVEL             0x4c
#define QUEUE_STATUS_IBI_STATUS_CNT(x) (((x) & GENMASK(28, 24)) >> 24)
#define QUEUE_STATUS_IBI_BUF_BLR(x)    (((x) & GENMASK(23, 16)) >> 16)
#define QUEUE_STATUS_LEVEL_RESP(x)     (((x) & GENMASK(15, 8)) >> 8)
#define QUEUE_STATUS_LEVEL_CMD(x)      ((x) & GENMASK(7, 0))

#define DATA_BUFFER_STATUS_LEVEL       0x50
#define DATA_BUFFER_STATUS_LEVEL_RX(x) (((x) & GENMASK(23, 16)) >> 16)
#define DATA_BUFFER_STATUS_LEVEL_TX(x) ((x) & GENMASK(7, 0))

#define PRESENT_STATE                0x54
#define PRESENT_STATE_CURRENT_MASTER BIT(2)

#define CCC_DEVICE_STATUS          0x58
#define DEVICE_ADDR_TABLE_POINTER  0x5c
#define DEVICE_ADDR_TABLE_DEPTH(x) (((x) & GENMASK(31, 16)) >> 16)
#define DEVICE_ADDR_TABLE_ADDR(x)  ((x) & GENMASK(15, 0))

#define DEV_CHAR_TABLE_POINTER      0x60
#define DEVICE_CHAR_TABLE_ADDR(x)   ((x) & GENMASK(11, 0))
#define VENDOR_SPECIFIC_REG_POINTER 0x6c

#define SLV_MIPI_ID_VALUE                      0x70
#define SLV_MIPI_ID_VALUE_SLV_MIPI_MFG_ID_MASK GENMASK(15, 1)
#define SLV_MIPI_ID_VALUE_SLV_MIPI_MFG_ID(x)   ((x) & SLV_MIPI_ID_VALUE_SLV_MIPI_MFG_ID_MASK)
#define SLV_MIPI_ID_VALUE_SLV_PROV_ID_SEL      BIT(0)

#define SLV_PID_VALUE 0x74

#define SLV_CHAR_CTRL                      0x78
#define SLV_CHAR_CTRL_MAX_DATA_SPEED_LIMIT BIT(0)
#define SLV_CHAR_CTRL_IBI_REQUEST_CAPABLE  BIT(1)
#define SLV_CHAR_CTRL_IBI_PAYLOAD          BIT(2)
#define SLV_CHAR_CTRL_BCR_MASK             GENMASK(7, 0)
#define SLV_CHAR_CTRL_BCR(x)               ((x) & SLV_CHAR_CTRL_BCR_MASK)
#define SLV_CHAR_CTRL_DCR_MASK             GENMASK(15, 8)
#define SLV_CHAR_CTRL_DCR(x)               (((x) & SLV_CHAR_CTRL_DCR_MASK) >> 8)
#define SLV_CHAR_CTRL_HDR_CAP_MASK         GENMASK(23, 16)
#define SLV_CHAR_CTRL_HDR_CAP(x)           (((x) & SLV_CHAR_CTRL_HDR_CAP_MASK) >> 16)

#define SLV_MAX_LEN        0x7c
#define SLV_MAX_LEN_MRL(x) (((x) & GENMASK(31, 16)) >> 16)
#define SLV_MAX_LEN_MWL(x) ((x) & GENMASK(15, 0))

#define MAX_READ_TURNAROUND                     0x80
#define MAX_READ_TURNAROUND_MXDX_MAX_RD_TURN(x) ((x) & GENMASK(23, 0))

#define MAX_DATA_SPEED   0x84
#define SLV_DEBUG_STATUS 0x88

#define SLV_INTR_REQ                        0x8c
#define SLV_INTR_REQ_SIR_DATA_LENGTH(x)     (((x) << 16) & GENMASK(23, 16))
#define SLV_INTR_REQ_MDB(x)                 (((x) << 8) & GENMASK(15, 8))
#define SLV_INTR_REQ_IBI_STS(x)             (((x) & GENMASK(9, 8)) >> 8)
#define SLV_INTR_REQ_IBI_STS_IBI_ACCEPT     0x01
#define SLV_INTR_REQ_IBI_STS_IBI_NO_ATTEMPT 0x03
#define SLV_INTR_REQ_TS                     BIT(4)
#define SLV_INTR_REQ_MR                     BIT(3)
#define SLV_INTR_REQ_SIR_CTRL(x)            (((x) & GENMASK(2, 1)) >> 1)
#define SLV_INTR_REQ_SIR                    BIT(0)

#define SLV_SIR_DATA          0x94
#define SLV_SIR_DATA_BYTE3(x) (((x) << 24) & GENMASK(31, 24))
#define SLV_SIR_DATA_BYTE2(x) (((x) << 16) & GENMASK(23, 16))
#define SLV_SIR_DATA_BYTE1(x) (((x) << 8) & GENMASK(15, 8))
#define SLV_SIR_DATA_BYTE0(x) ((x) & GENMASK(7, 0))

#define SLV_IBI_RESP                         0x98
#define SLV_IBI_RESP_DATA_LENGTH(x)          (((x) & GENMASK(23, 8)) >> 8)
#define SLV_IBI_RESP_IBI_STS(x)              ((x) & GENMASK(1, 0))
#define SLV_IBI_RESP_IBI_STS_ACK             0x01
#define SLV_IBI_RESP_IBI_STS_EARLY_TERMINATE 0x02
#define SLV_IBI_RESP_IBI_STS_NACK            0x03

#define SLV_NACK_REQ               0x9c
#define SLV_NACK_REQ_NACK_REQ(x)   ((x) & GENMASK(1, 0))
#define SLV_NACK_REQ_NACK_REQ_ACK  0x00
#define SLV_NACK_REQ_NACK_REQ_NACK 0x01

#define DEVICE_CTRL_EXTENDED                           0xb0
#define DEVICE_CTRL_EXTENDED_DEV_OPERATION_MODE(x)     ((x) & GENMASK(1, 0))
#define DEVICE_CTRL_EXTENDED_DEV_OPERATION_MODE_MASTER 0
#define DEVICE_CTRL_EXTENDED_DEV_OPERATION_MODE_SLAVE  1

#define SCL_I3C_OD_TIMING      0xb4
#define SCL_I3C_PP_TIMING      0xb8
#define SCL_I3C_TIMING_HCNT(x) (((x) << 16) & GENMASK(23, 16))
#define SCL_I3C_TIMING_LCNT(x) ((x) & GENMASK(7, 0))
#define SCL_I3C_TIMING_CNT_MIN 5
#define SCL_I3C_TIMING_CNT_MAX 255

#define SCL_I2C_FM_TIMING         0xbc
#define SCL_I2C_FM_TIMING_HCNT(x) (((x) << 16) & GENMASK(31, 16))
#define SCL_I2C_FM_TIMING_LCNT(x) ((x) & GENMASK(15, 0))

#define SCL_I2C_FMP_TIMING         0xc0
#define SCL_I2C_FMP_TIMING_HCNT(x) (((x) << 16) & GENMASK(23, 16))
#define SCL_I2C_FMP_TIMING_LCNT(x) ((x) & GENMASK(15, 0))

#define SCL_EXT_LCNT_TIMING 0xc8
#define SCL_EXT_LCNT_4(x)   (((x) << 24) & GENMASK(31, 24))
#define SCL_EXT_LCNT_3(x)   (((x) << 16) & GENMASK(23, 16))
#define SCL_EXT_LCNT_2(x)   (((x) << 8) & GENMASK(15, 8))
#define SCL_EXT_LCNT_1(x)   ((x) & GENMASK(7, 0))

#define SCL_EXT_TERMN_LCNT_TIMING 0xcc

#define SDA_HOLD_SWITCH_DLY_TIMING                         0xd0
#define SDA_HOLD_SWITCH_DLY_TIMING_SDA_TX_HOLD(x)          (((x)&GENMASK(18, 16)) >> 16)
#define SDA_HOLD_SWITCH_DLY_TIMING_SDA_PP_OD_SWITCH_DLY(x) (((x)&GENMASK(10, 8)) >> 8)
#define SDA_HOLD_SWITCH_DLY_TIMING_SDA_OD_PP_SWITCH_DLY(x) ((x)&GENMASK(2, 0))

#define BUS_FREE_TIMING           0xd4
/* Bus available time of 1us in ns */
#define I3C_BUS_AVAILABLE_TIME_NS 1000U
#define BUS_I3C_MST_FREE(x)       ((x) & GENMASK(15, 0))
#define BUS_I3C_AVAIL_TIME(x)     ((x << 16) & GENMASK(31, 16))

#define BUS_IDLE_TIMING      0xd8
/* Bus Idle time of 1ms in ns */
#define I3C_BUS_IDLE_TIME_NS 1000000U
#define BUS_I3C_IDLE_TIME(x) ((x) & GENMASK(19, 0))

#define I3C_VER_ID          0xe0
#define I3C_VER_TYPE        0xe4
#define EXTENDED_CAPABILITY 0xe8
#define SLAVE_CONFIG        0xec

#define QUEUE_SIZE_CAPABILITY                        0xe8
#define QUEUE_SIZE_CAPABILITY_IBI_BUF_DWORD_SIZE(x)  (2 << (((x) & GENMASK(19, 16)) >> 16))
#define QUEUE_SIZE_CAPABILITY_RESP_BUF_DWORD_SIZE(x) (2 << (((x) & GENMASK(15, 12)) >> 12))
#define QUEUE_SIZE_CAPABILITY_CMD_BUF_DWORD_SIZE(x)  (2 << (((x) & GENMASK(11, 8)) >> 8))
#define QUEUE_SIZE_CAPABILITY_RX_BUF_DWORD_SIZE(x)   (2 << (((x) & GENMASK(7, 4)) >> 4))
#define QUEUE_SIZE_CAPABILITY_TX_BUF_DWORD_SIZE(x)   (2 << ((x) & GENMASK(3, 0)))

#define DEV_ADDR_TABLE_LEGACY_I2C_DEV    BIT(31)
#define DEV_ADDR_TABLE_DYNAMIC_ADDR_MASK GENMASK(23, 16)
#define DEV_ADDR_TABLE_DYNAMIC_ADDR(x)   (((x) << 16) & GENMASK(23, 16))
#define DEV_ADDR_TABLE_SIR_REJECT        BIT(13)
#define DEV_ADDR_TABLE_IBI_WITH_DATA     BIT(12)
#define DEV_ADDR_TABLE_STATIC_ADDR(x)    ((x) & GENMASK(6, 0))
#define DEV_ADDR_TABLE_LOC(start, idx)   ((start) + ((idx) << 2))

#define DEV_CHAR_TABLE_LOC1(start, idx) ((start) + ((idx) << 4))
#define DEV_CHAR_TABLE_MSB_PID(x)       ((x) & GENMASK(31, 16))
#define DEV_CHAR_TABLE_LSB_PID(x)       ((x) & GENMASK(15, 0))
#define DEV_CHAR_TABLE_LOC2(start, idx) ((DEV_CHAR_TABLE_LOC1(start, idx)) + 4)
#define DEV_CHAR_TABLE_LOC3(start, idx) ((DEV_CHAR_TABLE_LOC1(start, idx)) + 8)
#define DEV_CHAR_TABLE_DCR(x)           ((x) & GENMASK(7, 0))
#define DEV_CHAR_TABLE_BCR(x)           (((x) & GENMASK(15, 8)) >> 8)

#define I3C_BUS_SDR1_SCL_RATE       8000000
#define I3C_BUS_SDR2_SCL_RATE       6000000
#define I3C_BUS_SDR3_SCL_RATE       4000000
#define I3C_BUS_SDR4_SCL_RATE       2000000
#define I3C_BUS_I2C_FM_TLOW_MIN_NS  1300
#define I3C_BUS_I2C_FMP_TLOW_MIN_NS 500
#define I3C_BUS_THIGH_MAX_NS        41
#define I3C_PERIOD_NS               1000000000ULL

#define I3C_BUS_MAX_I3C_SCL_RATE     12900000
#define I3C_BUS_TYP_I3C_SCL_RATE     12500000
#define I3C_BUS_I2C_FM_PLUS_SCL_RATE 1000000
#define I3C_BUS_I2C_FM_SCL_RATE      400000
#define I3C_BUS_TLOW_OD_MIN_NS       200

#define I3C_HOT_JOIN_ADDR 0x02

#define DW_I3C_MAX_DEVS         32
#define DW_I3C_MAX_CMD_BUF_SIZE 16

/* Snps I3C/I2C Device Private Data */
struct dw_i3c_i2c_dev_data {
	/* Device id within the retaining registers. This is set after bus initialization by the
	 * controller.
	 */
	uint8_t id;
};

struct dw_i3c_cmd {
	uint32_t cmd_lo;
	uint32_t cmd_hi;
	void *buf;
	uint16_t tx_len;
	uint16_t rx_len;
	uint8_t error;
};

struct dw_i3c_xfer {
	int32_t ret;
	uint32_t ncmds;
	struct dw_i3c_cmd cmds[DW_I3C_MAX_CMD_BUF_SIZE];
};

struct dw_i3c_config {
	struct i3c_driver_config common;
	const struct device *clock;
	uint32_t regs;

	/* Initial clk configuration */
	/* Maximum OD high clk pulse length */
	uint32_t od_thigh_max_ns;
	/* Minimum OD low clk pulse length */
	uint32_t od_tlow_min_ns;

	void (*irq_config_func)();

#if defined(CONFIG_PINCTRL)
	const struct pinctrl_dev_config *pcfg;
#endif
};

struct dw_i3c_data {
	struct i3c_driver_data common;
	uint32_t free_pos;

	uint16_t datstartaddr;
	uint16_t dctstartaddr;
	uint16_t maxdevs;

	/* fifo depth is in words (32b) */
	uint8_t ibififodepth;
	uint8_t respfifodepth;
	uint8_t cmdfifodepth;
	uint8_t rxfifodepth;
	uint8_t txfifodepth;

	enum i3c_bus_mode mode;

	struct i3c_target_config *target_config;

	struct k_sem sem_xfer;
	struct k_mutex mt;

#ifdef CONFIG_I3C_USE_IBI
	struct k_sem ibi_sts_sem;
	struct k_sem sem_hj;
#endif

	struct dw_i3c_xfer xfer;

	struct dw_i3c_i2c_dev_data dw_i3c_i2c_priv_data[DW_I3C_MAX_DEVS];
};

static uint8_t get_free_pos(uint32_t free_pos)
{
	return find_lsb_set(free_pos) - 1;
}

/**
 * @brief Read data from the Receive FIFO of the I3C device.
 *
 * This function reads data from the Receive FIFO of the I3C device specified by
 * the given device structure and stores it in the provided buffer.
 *
 * @param dev Pointer to the I3C device structure.
 * @param buf Pointer to the buffer where the received data will be stored.
 * @param nbytes Number of bytes to read from the Receive FIFO.
 */
static void read_rx_fifo(const struct device *dev, uint8_t *buf, int32_t nbytes)
{
	__ASSERT((buf != NULL), "Rx buffer should not be NULL");

	const struct dw_i3c_config *config = dev->config;
	int32_t i;
	uint32_t tmp;

	if (nbytes >= 4) {
		for (i = 0; i <= nbytes - 4; i += 4) {
			tmp = sys_read32(config->regs + RX_TX_DATA_PORT);
			memcpy(buf + i, &tmp, 4);
		}
	}
	if (nbytes & 3) {
		tmp = sys_read32(config->regs + RX_TX_DATA_PORT);
		memcpy(buf + (nbytes & ~3), &tmp, nbytes & 3);
	}
}

/**
 * @brief Write data to the Transmit FIFO of the I3C device.
 *
 * This function writes data to the Transmit FIFO of the I3C device specified by
 * the given device structure from the provided buffer.
 *
 * @param dev Pointer to the I3C device structure.
 * @param buf Pointer to the buffer containing the data to be written.
 * @param nbytes Number of bytes to write to the Transmit FIFO.
 */
static void write_tx_fifo(const struct device *dev, const uint8_t *buf, int32_t nbytes)
{
	__ASSERT((buf != NULL), "Tx buffer should not be NULL");

	const struct dw_i3c_config *config = dev->config;
	int32_t i;
	uint32_t tmp;

	if (nbytes >= 4) {
		for (i = 0; i <= nbytes - 4; i += 4) {
			memcpy(&tmp, buf + i, 4);
			sys_write32(tmp, config->regs + RX_TX_DATA_PORT);
		}
	}

	if (nbytes & 3) {
		tmp = 0;
		memcpy(&tmp, buf + (nbytes & ~3), nbytes & 3);
		sys_write32(tmp, config->regs + RX_TX_DATA_PORT);
	}
}

#ifdef CONFIG_I3C_USE_IBI
/**
 * @brief Read data from the In-Band Interrupt (IBI) FIFO of the I3C device.
 *
 * This function reads data from the In-Band Interrupt (IBI) FIFO of the I3C device
 * specified by the given device structure and stores it in the provided buffer.
 *
 * @param dev Pointer to the I3C device structure.
 * @param buf Pointer to the buffer where the received IBI data will be stored.
 * @param nbytes Number of bytes to read from the IBI FIFO.
 */
static void read_ibi_fifo(const struct device *dev, uint8_t *buf, int32_t nbytes)
{
	__ASSERT((buf != NULL), "Rx IBI buffer should not be NULL");

	const struct dw_i3c_config *config = dev->config;
	int32_t i;
	uint32_t tmp;

	if (nbytes >= 4) {
		for (i = 0; i <= nbytes - 4; i += 4) {
			tmp = sys_read32(config->regs + IBI_QUEUE_STATUS);
			memcpy(buf + i, &tmp, 4);
		}
	}
	if (nbytes & 3) {
		tmp = sys_read32(config->regs + IBI_QUEUE_STATUS);
		memcpy(buf + (nbytes & ~3), &tmp, nbytes & 3);
	}
}
#endif

/**
 * @brief End the I3C transfer and process responses.
 *
 * This function is responsible for ending the I3C transfer on the specified
 * I3C device. It processes the responses received from the I3C bus, updating the
 * status and error information in the transfer structure.
 *
 * @param dev Pointer to the I3C device structure.
 */
static void dw_i3c_end_xfer(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	struct dw_i3c_xfer *xfer = &data->xfer;
	struct dw_i3c_cmd *cmd;
	uint32_t nresp, resp, rx_data;
	int32_t i, j, k, ret = 0;

	nresp = QUEUE_STATUS_LEVEL_RESP(sys_read32(config->regs + QUEUE_STATUS_LEVEL));
	for (i = 0; i < nresp; i++) {
		uint8_t tid;

		resp = sys_read32(config->regs + RESPONSE_QUEUE_PORT);
		tid = RESPONSE_PORT_TID(resp);
		if (tid == 0xf) {
			/* TODO: handle vendor extension ccc or hdr header in target mode */
			continue;
		}

		cmd = &xfer->cmds[tid];
		cmd->rx_len = RESPONSE_PORT_DATA_LEN(resp);
		cmd->error = RESPONSE_PORT_ERR_STATUS(resp);

		/* if we are in target mode */
		if (!(sys_read32(config->regs + PRESENT_STATE) & PRESENT_STATE_CURRENT_MASTER)) {
			const struct i3c_target_callbacks *target_cb =
				data->target_config->callbacks;

			for (j = 0; j < cmd->rx_len; j += 4) {
				rx_data = sys_read32(config->regs + RX_TX_DATA_PORT);
				/* Call write received cb for each remaining byte  */
				for (k = 0; k < MIN(4, cmd->rx_len - j); k++) {
					target_cb->write_received_cb(data->target_config,
								    (rx_data >> (8 * k)) & 0xff);
				}
			}

			if (target_cb != NULL && target_cb->stop_cb != NULL) {
				/*
				 * TODO: modify API to include status, such as success or aborted
				 * transfer
				 */
				target_cb->stop_cb(data->target_config);
			}
		}
	}

	for (i = 0; i < nresp; i++) {
		switch (xfer->cmds[i].error) {
		case RESPONSE_NO_ERROR:
			break;
		case RESPONSE_ERROR_PARITY:
		case RESPONSE_ERROR_IBA_NACK:
		case RESPONSE_ERROR_TRANSF_ABORT:
		case RESPONSE_ERROR_CRC:
		case RESPONSE_ERROR_FRAME:
			ret = -EIO;
			break;
		case RESPONSE_ERROR_OVER_UNDER_FLOW:
			ret = -ENOSPC;
			break;
		case RESPONSE_ERROR_I2C_W_NACK_ERR:
		case RESPONSE_ERROR_ADDRESS_NACK:
			ret = -ENXIO;
			break;
		default:
			ret = -EINVAL;
			break;
		}
	}
	xfer->ret = ret;

	if (ret < 0) {
		sys_write32(RESET_CTRL_RX_FIFO | RESET_CTRL_TX_FIFO | RESET_CTRL_RESP_QUEUE |
			    RESET_CTRL_CMD_QUEUE,
			    config->regs + RESET_CTRL);
		sys_write32(sys_read32(config->regs + DEVICE_CTRL) | DEV_CTRL_RESUME,
			    config->regs + DEVICE_CTRL);
	}
	k_sem_give(&data->sem_xfer);
}

/**
 * @brief Start an I3C transfer on the specified device.
 *
 * This function initiates an I3C transfer on the specified I3C device by pushing
 * data to the Transmit FIFO (TXFIFO) and enqueuing commands to the command queue.
 *
 * @param dev Pointer to the I3C device structure.
 */
static void start_xfer(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	struct dw_i3c_xfer *xfer = &data->xfer;
	struct dw_i3c_cmd *cmd;
	uint32_t thld_ctrl, present_state;
	int32_t i;

	present_state = sys_read32(config->regs + PRESENT_STATE);

	/* Push data to TXFIFO */
	for (i = 0; i < xfer->ncmds; i++) {
		cmd = &xfer->cmds[i];
		/* Not all the commands use write_tx_fifo function */
		if (cmd->buf != NULL) {
			write_tx_fifo(dev, cmd->buf, cmd->tx_len);
		}
	}

	thld_ctrl = sys_read32(config->regs + QUEUE_THLD_CTRL);
	thld_ctrl &= ~QUEUE_THLD_CTRL_RESP_BUF_MASK;
	thld_ctrl |= QUEUE_THLD_CTRL_RESP_BUF(xfer->ncmds);
	sys_write32(thld_ctrl, config->regs + QUEUE_THLD_CTRL);

	/* Enqueue CMD */
	for (i = 0; i < xfer->ncmds; i++) {
		cmd = &xfer->cmds[i];
		/* Only cmd_lo is used when it is a target */
		if (present_state & PRESENT_STATE_CURRENT_MASTER) {
			sys_write32(cmd->cmd_hi, config->regs + COMMAND_QUEUE_PORT);
		}
		sys_write32(cmd->cmd_lo, config->regs + COMMAND_QUEUE_PORT);
	}
}

/**
 * @brief Get the position of an I3C device with the specified address.
 *
 * This function retrieves the position (ID) of an I3C device with the specified
 * address on the I3C bus associated with the provided I3C device structure. This
 * utilizes the controller private data for where the id reg is stored.
 *
 * @param dev Pointer to the I3C device structure.
 * @param addr I3C address of the device whose position is to be retrieved.
 * @param sa True if looking up by Static Address, False if by Dynamic Address
 *
 * @return The position (ID) of the device on success, or a negative error code
 *         if the device with the given address is not found.
 */
static int get_i3c_addr_pos(const struct device *dev, uint8_t addr, bool sa)
{
	struct dw_i3c_i2c_dev_data *dw_i3c_device_data;
	struct i3c_device_desc *desc = sa ? i3c_dev_list_i3c_static_addr_find(dev, addr)
					  : i3c_dev_list_i3c_addr_find(dev, addr);

	if (desc == NULL) {
		return -ENODEV;
	}

	dw_i3c_device_data = desc->controller_priv;

	return dw_i3c_device_data->id;
}

/**
 * @brief Transfer messages in I3C mode.
 *
 * @param dev Pointer to device driver instance.
 * @param target Pointer to target device descriptor.
 * @param msgs Pointer to I3C messages.
 * @param num_msgs Number of messages to transfers.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -EINVAL Address not registered
 */
static int dw_i3c_xfers(const struct device *dev, struct i3c_device_desc *target,
			struct i3c_msg *msgs, uint8_t num_msgs)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	struct dw_i3c_xfer *xfer = &data->xfer;
	int32_t ret, i, pos, nrxwords = 0, ntxwords = 0;
	uint32_t present_state;

	present_state = sys_read32(config->regs + PRESENT_STATE);
	if (!(present_state & PRESENT_STATE_CURRENT_MASTER)) {
		return -EACCES;
	}

	if (num_msgs > data->cmdfifodepth) {
		return -ENOTSUP;
	}

	pos = get_i3c_addr_pos(dev, target->dynamic_addr, false);
	if (pos < 0) {
		LOG_ERR("%s: Invalid slave device", dev->name);
		return -EINVAL;
	}

	for (i = 0; i < num_msgs; i++) {
		if (msgs[i].flags & I2C_MSG_READ) {
			nrxwords += DIV_ROUND_UP(msgs[i].len, 4);
		} else {
			ntxwords += DIV_ROUND_UP(msgs[i].len, 4);
		}
	}

	if (ntxwords > data->txfifodepth || nrxwords > data->rxfifodepth) {
		return -ENOTSUP;
	}

	ret = k_mutex_lock(&data->mt, K_MSEC(1000));
	if (ret) {
		LOG_ERR("%s: Mutex err (%d)", dev->name, ret);
		return ret;
	}

	pm_device_busy_set(dev);

	memset(xfer, 0, sizeof(struct dw_i3c_xfer));

	xfer->ncmds = num_msgs;
	xfer->ret = -1;

	for (i = 0; i < num_msgs; i++) {
		struct dw_i3c_cmd *cmd = &xfer->cmds[i];

		cmd->cmd_hi = COMMAND_PORT_ARG_DATA_LEN(msgs[i].len) | COMMAND_PORT_TRANSFER_ARG;
		cmd->cmd_lo = COMMAND_PORT_TID(i) | COMMAND_PORT_DEV_INDEX(pos) | COMMAND_PORT_ROC;

		cmd->buf = msgs[i].buf;

		if (msgs[i].flags & I3C_MSG_NBCH) {
			sys_write32(sys_read32(config->regs + DEVICE_CTRL) & ~DEV_CTRL_IBA_INCLUDE,
				    config->regs + DEVICE_CTRL);
		} else {
			sys_write32(sys_read32(config->regs + DEVICE_CTRL) | DEV_CTRL_IBA_INCLUDE,
				    config->regs + DEVICE_CTRL);
		}

		if (msgs[i].flags & I3C_MSG_READ) {
			uint8_t rd_speed;

			if (msgs[i].flags & I3C_MSG_HDR) {
				/* Set read command bit for DDR and TS */
				cmd->cmd_lo |= COMMAND_PORT_CP |
					       COMMAND_PORT_CMD(BIT(7) | (msgs[i].hdr_cmd_code &
									  GENMASK(6, 0)));
				if (msgs[i].hdr_mode & I3C_MSG_HDR_DDR) {
					if (data->common.ctrl_config.supported_hdr &
					    I3C_MSG_HDR_DDR) {
						rd_speed = COMMAND_PORT_SPEED_I3C_DDR;
					} else {
						/* DDR support not configured with this */
						LOG_ERR("%s: HDR-DDR not supported", dev->name);
						ret = -ENOTSUP;
						goto error;
					}
				} else if (msgs[i].hdr_mode & I3C_MSG_HDR_TSP ||
					   msgs[i].hdr_mode & I3C_MSG_HDR_TSL) {
					if (data->common.ctrl_config.supported_hdr &
					    (I3C_MSG_HDR_TSP | I3C_MSG_HDR_TSL)) {
						rd_speed = COMMAND_PORT_SPEED_I3C_TS;
					} else {
						/* TS support not configured with this */
						LOG_ERR("%s: HDR-TS not supported", dev->name);
						ret = -ENOTSUP;
						goto error;
					}
				} else {
					LOG_ERR("%s: HDR %d not supported", dev->name,
						msgs[i].hdr_mode);
					ret = -ENOTSUP;
					goto error;
				}
			} else {
				rd_speed = I3C_CCC_GETMXDS_MAXRD_MAX_SDR_FSCL(
					target->data_speed.maxrd);
			}

			cmd->cmd_lo |= (COMMAND_PORT_READ_TRANSFER | COMMAND_PORT_SPEED(rd_speed));
			cmd->rx_len = msgs[i].len;
		} else {
			uint8_t wr_speed;

			if (msgs[i].flags & I3C_MSG_HDR) {
				cmd->cmd_lo |=
					COMMAND_PORT_CP |
					COMMAND_PORT_CMD(msgs[i].hdr_cmd_code & GENMASK(6, 0));
				if (msgs[i].hdr_mode & I3C_MSG_HDR_DDR) {
					if (data->common.ctrl_config.supported_hdr &
					    I3C_MSG_HDR_DDR) {
						wr_speed = COMMAND_PORT_SPEED_I3C_DDR;
					} else {
						/* DDR support not configured with this */
						LOG_ERR("%s: HDR-DDR not supported", dev->name);
						ret = -ENOTSUP;
						goto error;
					}
				} else if (msgs[i].hdr_mode & I3C_MSG_HDR_TSP ||
					   msgs[i].hdr_mode & I3C_MSG_HDR_TSL) {
					if (data->common.ctrl_config.supported_hdr &
					    (I3C_MSG_HDR_TSP | I3C_MSG_HDR_TSL)) {
						wr_speed = COMMAND_PORT_SPEED_I3C_TS;
					} else {
						/* TS support not configured with this */
						LOG_ERR("%s: HDR-TS not supported", dev->name);
						ret = -ENOTSUP;
						goto error;
					}
				} else {
					LOG_ERR("%s: HDR %d not supported", dev->name,
						msgs[i].hdr_mode);
					ret = -ENOTSUP;
					goto error;
				}
			} else {
				wr_speed = I3C_CCC_GETMXDS_MAXWR_MAX_SDR_FSCL(
					target->data_speed.maxwr);
			}

			cmd->cmd_lo |= COMMAND_PORT_SPEED(wr_speed);
			cmd->tx_len = msgs[i].len;
		}

		if (i == (num_msgs - 1)) {
			cmd->cmd_lo |= COMMAND_PORT_TOC;
		}
	}

	start_xfer(dev);

	ret = k_sem_take(&data->sem_xfer, K_MSEC(CONFIG_I3C_DW_RW_TIMEOUT_MS));
	if (ret) {
		LOG_ERR("%s: Semaphore err (%d)", dev->name, ret);
		goto error;
	}

	for (i = 0; i < xfer->ncmds; i++) {
		msgs[i].num_xfer = (msgs[i].flags & I3C_MSG_READ) ? xfer->cmds[i].rx_len
								  : xfer->cmds[i].tx_len;
		if (xfer->cmds[i].rx_len && !xfer->cmds[i].error) {
			read_rx_fifo(dev, xfer->cmds[i].buf, xfer->cmds[i].rx_len);
		}
	}

	ret = xfer->ret;

error:
	pm_device_busy_clear(dev);
	k_mutex_unlock(&data->mt);

	return ret;
}

static int dw_i3c_i2c_attach_device(const struct device *dev, struct i3c_i2c_device_desc *desc)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	uint8_t pos;

	pos = get_free_pos(data->free_pos);
	if (pos < 0) {
		return -ENOSPC;
	}

	data->dw_i3c_i2c_priv_data[pos].id = pos;
	desc->controller_priv = &(data->dw_i3c_i2c_priv_data[pos]);
	data->free_pos &= ~BIT(pos);

	sys_write32(DEV_ADDR_TABLE_LEGACY_I2C_DEV | DEV_ADDR_TABLE_STATIC_ADDR(desc->addr),
		    config->regs + DEV_ADDR_TABLE_LOC(data->datstartaddr, pos));

	return 0;
}

static void dw_i3c_i2c_detach_device(const struct device *dev, struct i3c_i2c_device_desc *desc)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	struct dw_i3c_i2c_dev_data *dw_i2c_device_data = desc->controller_priv;

	__ASSERT_NO_MSG(dw_i2c_device_data != NULL);

	sys_write32(0,
		    config->regs + DEV_ADDR_TABLE_LOC(data->datstartaddr, dw_i2c_device_data->id));
	data->free_pos |= BIT(dw_i2c_device_data->id);
	desc->controller_priv = NULL;
}

/**
 * @brief Transfer messages in I2C mode.
 *
 * @param dev Pointer to device driver instance.
 * @param target Pointer to target device descriptor.
 * @param msgs Pointer to I2C messages.
 * @param num_msgs Number of messages to transfers.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -EINVAL Address not registered
 */
static int dw_i3c_i2c_transfer(const struct device *dev, struct i3c_i2c_device_desc *target,
			       struct i2c_msg *msgs, uint8_t num_msgs)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	struct dw_i3c_xfer *xfer = &data->xfer;
	int32_t ret, i, pos, nrxwords = 0, ntxwords = 0;
	uint32_t present_state;

	present_state = sys_read32(config->regs + PRESENT_STATE);
	if (!(present_state & PRESENT_STATE_CURRENT_MASTER)) {
		return -EACCES;
	}

	if (num_msgs > data->cmdfifodepth) {
		return -ENOTSUP;
	}

	for (i = 0; i < num_msgs; i++) {
		if (msgs[i].flags & I2C_MSG_READ) {
			nrxwords += DIV_ROUND_UP(msgs[i].len, 4);
		} else {
			ntxwords += DIV_ROUND_UP(msgs[i].len, 4);
		}
	}

	if (ntxwords > data->txfifodepth || nrxwords > data->rxfifodepth) {
		return -ENOTSUP;
	}

	ret = k_mutex_lock(&data->mt, K_MSEC(1000));
	if (ret) {
		LOG_ERR("%s: Mutex err (%d)", dev->name, ret);
		return ret;
	}

	pm_device_busy_set(dev);

	/* In order limit the number of retaining registers occupied by connected devices,
	 * I2C devices are only configured during transfers. This allows the number of devices
	 * to be larger than the number of retaining registers on mixed buses.
	 */
	ret = dw_i3c_i2c_attach_device(dev, target);
	if (ret != 0) {
		LOG_ERR("%s: Failed to attach I2C device (%d)", dev->name, ret);
		goto error_attach;
	}
	pos = ((struct dw_i3c_i2c_dev_data *)target->controller_priv)->id;

	memset(xfer, 0, sizeof(struct dw_i3c_xfer));

	xfer->ncmds = num_msgs;
	xfer->ret = -1;

	for (i = 0; i < num_msgs; i++) {
		struct dw_i3c_cmd *cmd = &xfer->cmds[i];

		cmd->cmd_hi = COMMAND_PORT_ARG_DATA_LEN(msgs[i].len) | COMMAND_PORT_TRANSFER_ARG;
		cmd->cmd_lo = COMMAND_PORT_TID(i) | COMMAND_PORT_DEV_INDEX(pos) | COMMAND_PORT_ROC;

		cmd->buf = msgs[i].buf;

		if (msgs[i].flags & I2C_MSG_READ) {
			uint8_t rd_speed = I3C_LVR_I2C_MODE(target->lvr) == I3C_LVR_I2C_FM_MODE
						   ? COMMAND_PORT_SPEED_I2C_FM
						   : COMMAND_PORT_SPEED_I2C_FMP;

			cmd->cmd_lo |= (COMMAND_PORT_READ_TRANSFER | COMMAND_PORT_SPEED(rd_speed));
			cmd->rx_len = msgs[i].len;
		} else {
			uint8_t wr_speed = I3C_LVR_I2C_MODE(target->lvr) == I3C_LVR_I2C_FM_MODE
						   ? COMMAND_PORT_SPEED_I2C_FM
						   : COMMAND_PORT_SPEED_I2C_FMP;

			cmd->cmd_lo |= COMMAND_PORT_SPEED(wr_speed);
			cmd->tx_len = msgs[i].len;
		}

		if (i == (num_msgs - 1)) {
			cmd->cmd_lo |= COMMAND_PORT_TOC;
		}
	}

	/* Do not send broadcast address (0x7E) with I2C transfers */
	sys_write32(sys_read32(config->regs + DEVICE_CTRL) & ~DEV_CTRL_IBA_INCLUDE,
		    config->regs + DEVICE_CTRL);

	start_xfer(dev);

	ret = k_sem_take(&data->sem_xfer, K_MSEC(CONFIG_I3C_DW_RW_TIMEOUT_MS));
	if (ret) {
		LOG_ERR("%s: Semaphore err (%d)", dev->name, ret);
		goto error;
	}

	for (i = 0; i < xfer->ncmds; i++) {
		if (xfer->cmds[i].rx_len && !xfer->cmds[i].error) {
			read_rx_fifo(dev, xfer->cmds[i].buf, xfer->cmds[i].rx_len);
		}
	}

	ret = xfer->ret;

error:
	dw_i3c_i2c_detach_device(dev, target);
error_attach:
	pm_device_busy_clear(dev);
	k_mutex_unlock(&data->mt);

	return ret;
}

/**
 * Find a registered I2C target device.
 *
 * Controller only API.
 *
 * This returns the I2C device descriptor of the I2C device
 * matching the device address @p addr.
 *
 * @param dev Pointer to controller device driver instance.
 * @param id I2C target device address.
 *
 * @return @see i3c_i2c_device_find.
 */
static struct i3c_i2c_device_desc *dw_i3c_i2c_device_find(const struct device *dev, uint16_t addr)
{
	return i3c_dev_list_i2c_addr_find(dev, addr);
}

/**
 * @brief Transfer messages in I2C mode.
 *
 * @see i2c_transfer
 *
 * @param dev Pointer to device driver instance.
 * @param msgs Pointer to I2C messages.
 * @param num_msgs Number of messages to transfers.
 * @param addr Address of the I2C target device.
 *
 * @return @see i2c_transfer
 */
static int dw_i3c_i2c_api_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
				   uint16_t addr)
{
	struct i3c_i2c_device_desc *i2c_dev = dw_i3c_i2c_device_find(dev, addr);

	if (i2c_dev == NULL) {
		return -ENODEV;
	}

	return dw_i3c_i2c_transfer(dev, i2c_dev, msgs, num_msgs);
}

#ifdef CONFIG_I3C_USE_IBI
static int dw_i3c_controller_ibi_hj_response(const struct device *dev, bool ack)
{
	const struct dw_i3c_config *config = dev->config;
	uint32_t ctrl = sys_read32(config->regs + DEVICE_CTRL);

	if (ack) {
		ctrl &= ~DEV_CTRL_HOT_JOIN_NACK;
	} else {
		ctrl |= DEV_CTRL_HOT_JOIN_NACK;
	}

	sys_write32(ctrl, config->regs + DEVICE_CTRL);

	return 0;
}

static int i3c_dw_endis_ibi(const struct device *dev, struct i3c_device_desc *target, bool en)
{
	struct dw_i3c_data *data = dev->data;
	const struct dw_i3c_config *config = dev->config;
	uint32_t bitpos, sir_con;
	struct i3c_ccc_events i3c_events;
	int ret;
	int pos;

	pos = get_i3c_addr_pos(dev, target->dynamic_addr, false);
	if (pos < 0) {
		LOG_ERR("%s: Invalid Slave address", dev->name);
		return pos;
	}

	uint32_t reg = sys_read32(config->regs + DEV_ADDR_TABLE_LOC(data->datstartaddr, pos));

	if (i3c_ibi_has_payload(target)) {
		reg |= DEV_ADDR_TABLE_IBI_WITH_DATA;
	} else {
		reg &= ~DEV_ADDR_TABLE_IBI_WITH_DATA;
	}
	if (en) {
		reg &= ~DEV_ADDR_TABLE_SIR_REJECT;
	} else {
		reg |= DEV_ADDR_TABLE_SIR_REJECT;
	}
	sys_write32(reg, config->regs + DEV_ADDR_TABLE_LOC(data->datstartaddr, pos));

	sir_con = sys_read32(config->regs + IBI_SIR_REQ_REJECT);
	/* TODO: what is this macro doing?? */
	bitpos = IBI_SIR_REQ_ID(target->dynamic_addr);

	if (en) {
		sir_con &= ~BIT(bitpos);
	} else {
		sir_con |= BIT(bitpos);
	}
	sys_write32(sir_con, config->regs + IBI_SIR_REQ_REJECT);

	/* Tell target to enable IBI */
	i3c_events.events = I3C_CCC_EVT_INTR;
	ret = i3c_ccc_do_events_set(target, en, &i3c_events);
	if (ret != 0) {
		LOG_ERR("%s: Error sending IBI ENEC for 0x%02x (%d)", dev->name,
			target->dynamic_addr, ret);
		return ret;
	}

	return 0;
}

static int dw_i3c_controller_enable_ibi(const struct device *dev, struct i3c_device_desc *target)
{
	return i3c_dw_endis_ibi(dev, target, true);
}

static int dw_i3c_controller_disable_ibi(const struct device *dev, struct i3c_device_desc *target)
{
	return i3c_dw_endis_ibi(dev, target, false);
}

static void dw_i3c_handle_tir(const struct device *dev, uint32_t ibi_status)
{
	uint8_t ibi_data[CONFIG_I3C_IBI_MAX_PAYLOAD_SIZE];
	uint8_t addr, len;
	int pos;

	addr = IBI_QUEUE_IBI_ADDR(ibi_status);
	len = IBI_QUEUE_STATUS_DATA_LEN(ibi_status);

	pos = get_i3c_addr_pos(dev, addr, false);
	if (pos < 0) {
		LOG_ERR("%s: Invalid Slave address", dev->name);
		return;
	}

	struct i3c_device_desc *desc = i3c_dev_list_i3c_addr_find(dev, addr);

	if (desc == NULL) {
		return;
	}

	if (len > 0) {
		read_ibi_fifo(dev, ibi_data, len);
	}

	if (i3c_ibi_work_enqueue_target_irq(desc, ibi_data, len) != 0) {
		LOG_ERR("%s: Error enqueue IBI IRQ work", dev->name);
	}
}

static void dw_i3c_handle_hj(const struct device *dev, uint32_t ibi_status)
{
	if (IBI_QUEUE_STATUS_IBI_STS(ibi_status) & BIT(3)) {
		LOG_DBG("%s: NAK for HJ", dev->name);
		return;
	}

	if (i3c_ibi_work_enqueue_hotjoin(dev) != 0) {
		LOG_ERR("%s: Error enqueue IBI HJ work", dev->name);
	}
}

static void ibis_handle(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;
	uint32_t nibis, ibi_stat;
	int32_t i;

	nibis = sys_read32(config->regs + QUEUE_STATUS_LEVEL);
	nibis = QUEUE_STATUS_IBI_BUF_BLR(nibis);
	for (i = 0; i < nibis; i++) {
		ibi_stat = sys_read32(config->regs + IBI_QUEUE_STATUS);
		if (IBI_TYPE_SIRQ(ibi_stat)) {
			dw_i3c_handle_tir(dev, ibi_stat);
		} else if (IBI_TYPE_HJ(ibi_stat)) {
			dw_i3c_handle_hj(dev, ibi_stat);
		} else {
			LOG_DBG("%s: Secondary Master Request Not implemented", dev->name);
		}
	}
}

static int dw_i3c_target_ibi_raise_hj(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	int ret;

	if (!(sys_read32(config->regs + HW_CAPABILITY) & HW_CAPABILITY_SLV_HJ_CAP)) {
		LOG_ERR("%s: HJ not supported", dev->name);
		return -ENOTSUP;
	}
	if (sys_read32(config->regs + DEVICE_ADDR) & DEVICE_ADDR_DYNAMIC_ADDR_VALID) {
		LOG_ERR("%s: HJ not available, DA already assigned", dev->name);
		return -EACCES;
	}
	/* if this is set, then it is assumed it is already trying */
	if ((sys_read32(config->regs + SLV_EVENT_STATUS) & SLV_EVENT_STATUS_HJ_EN)) {
		LOG_ERR("%s: HJ requests are currently disabled by DISEC", dev->name);
		return -EAGAIN;
	}

	/*
	 * This is issued auto-magically by the IP when certain conditions are meet.
	 * These include:
	 * 1. SLV_EVENT_STATUS[HJ_EN] = 1 (or a controller issues Enables HJ events with
	 * the CCC ENEC, This can be set to 0 with CCC DISEC from a controller)
	 * 2. The Dynamic address is invalid. (not assigned yet)
	 * 3. Bus Idle condition is met (1ms) as programmed in the Bus Timing Register
	 */

	/* enable HJ */
	sys_write32(sys_read32(config->regs + SLV_EVENT_STATUS) | SLV_EVENT_STATUS_HJ_EN,
		    config->regs + SLV_EVENT_STATUS);

	ret = k_sem_take(&data->sem_hj, K_MSEC(CONFIG_I3C_DW_RW_TIMEOUT_MS));
	if (ret) {
		return ret;
	}

	return 0;
}

static int dw_i3c_target_ibi_raise_tir(const struct device *dev, struct i3c_ibi *request)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	int status;
	uint32_t slv_intr_req, slv_ibi_resp;

	if (!(sys_read32(config->regs + HW_CAPABILITY) & HW_CAPABILITY_SLV_IBI_CAP)) {
		LOG_ERR("%s: IBI TIR not supported", dev->name);
		return -ENOTSUP;
	}

	if (!(sys_read32(config->regs + DEVICE_ADDR) & DEVICE_ADDR_DYNAMIC_ADDR_VALID)) {
		LOG_ERR("%s: IBI TIR not available, DA not assigned", dev->name);
		return -EACCES;
	}

	if (!(sys_read32(config->regs + SLV_EVENT_STATUS) & SLV_EVENT_STATUS_SIR_EN)) {
		LOG_ERR("%s: IBI TIR requests are currently disabled by DISEC", dev->name);
		return -EAGAIN;
	}

	slv_intr_req = sys_read32(config->regs + SLV_INTR_REQ);
	if (sys_read32(config->regs + SLV_CHAR_CTRL) & SLV_CHAR_CTRL_IBI_PAYLOAD) {
		uint32_t tir_data = 0;

		/* max support length is DA + MDB (1 byte) + 4 data bytes, MDB must be at least
		 * included
		 */
		if ((request->payload_len > 5) || (request->payload_len == 0)) {
			return -EINVAL;
		}

		/* MDB should be the first byte of the payload */
		slv_intr_req |= SLV_INTR_REQ_MDB(request->payload[0]) |
				SLV_INTR_REQ_SIR_DATA_LENGTH(request->payload_len - 1);

		/* program the tir data packet */
		tir_data |=
			SLV_SIR_DATA_BYTE0((request->payload_len > 1) ? request->payload[1] : 0);
		tir_data |=
			SLV_SIR_DATA_BYTE1((request->payload_len > 2) ? request->payload[2] : 0);
		tir_data |=
			SLV_SIR_DATA_BYTE2((request->payload_len > 3) ? request->payload[3] : 0);
		tir_data |=
			SLV_SIR_DATA_BYTE3((request->payload_len > 4) ? request->payload[4] : 0);
		sys_write32(tir_data, config->regs + SLV_SIR_DATA);
	}

	/* kick off the ibi tir request */
	slv_intr_req |= SLV_INTR_REQ_SIR;
	sys_write32(slv_intr_req, config->regs + SLV_INTR_REQ);

	/* wait for SLV_IBI_RESP update */
	status = k_sem_take(&data->ibi_sts_sem, K_MSEC(100));
	if (status != 0) {
		return -ETIMEDOUT;
	}

	slv_ibi_resp = sys_read32(config->regs + SLV_INTR_REQ);
	switch (SLV_IBI_RESP_IBI_STS(slv_ibi_resp)) {
	case SLV_IBI_RESP_IBI_STS_ACK:
		LOG_DBG("%s: Controller ACKed IBI TIR", dev->name);
		return 0;
	case SLV_IBI_RESP_IBI_STS_NACK:
		LOG_ERR("%s: Controller NACKed IBI TIR", dev->name);
		return -EAGAIN;
	case SLV_IBI_RESP_IBI_STS_EARLY_TERMINATE:
		LOG_ERR("%s: Controller aborted IBI TIR with %lu remaining", dev->name,
			SLV_IBI_RESP_DATA_LENGTH(slv_ibi_resp));
		return -EIO;
	default:
		return -EIO;
	}
}

static int dw_i3c_target_ibi_raise(const struct device *dev, struct i3c_ibi *request)
{
	if (request == NULL) {
		return -EINVAL;
	}

	switch (request->ibi_type) {
	case I3C_IBI_TARGET_INTR:
		return dw_i3c_target_ibi_raise_tir(dev, request);
	case I3C_IBI_CONTROLLER_ROLE_REQUEST:
		/* TODO: Synopsys I3C can support CR, but not implemented yet */
		return -ENOTSUP;
	case I3C_IBI_HOTJOIN:
		return dw_i3c_target_ibi_raise_hj(dev);
	default:
		return -EINVAL;
	}
}

#endif /* CONFIG_I3C_USE_IBI */

static int i3c_dw_irq(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	uint32_t status;
	uint32_t present_state;

	status = sys_read32(config->regs + INTR_STATUS);
	if (status & (INTR_TRANSFER_ERR_STAT | INTR_RESP_READY_STAT)) {
		dw_i3c_end_xfer(dev);

		if (status & INTR_TRANSFER_ERR_STAT) {
			sys_write32(INTR_TRANSFER_ERR_STAT, config->regs + INTR_STATUS);
		}
	}

	if (status & INTR_IBI_THLD_STAT) {
#ifdef CONFIG_I3C_USE_IBI
		ibis_handle(dev);
#endif /* CONFIG_I3C_USE_IBI */
	}

	/* target mode related interrupts */
	present_state = sys_read32(config->regs + PRESENT_STATE);
	if (!(present_state & PRESENT_STATE_CURRENT_MASTER)) {
		const struct i3c_target_callbacks *target_cb =
			data->target_config ? data->target_config->callbacks : NULL;

		/* Read Requested when the CMDQ is empty*/
		if (status & INTR_READ_REQ_RECV_STAT) {
			if (target_cb != NULL && target_cb->read_requested_cb != NULL) {
				/* Inform app so that it can send data. */
				target_cb->read_requested_cb(data->target_config, NULL);
			}
			sys_write32(INTR_READ_REQ_RECV_STAT, config->regs + INTR_STATUS);
		}
#ifdef CONFIG_I3C_USE_IBI
		/* IBI TIR request register is addressed and status is updated*/
		if (status & INTR_IBI_UPDATED_STAT) {
			k_sem_give(&data->ibi_sts_sem);
			sys_write32(INTR_IBI_UPDATED_STAT, config->regs + INTR_STATUS);
		}
		/* DA has been assigned, could happen after a IBI HJ request */
		if (status & INTR_DYN_ADDR_ASSGN_STAT) {
			/* TODO: handle IBI HJ with semaphore */
			sys_write32(INTR_DYN_ADDR_ASSGN_STAT, config->regs + INTR_STATUS);
		}
#endif /* CONFIG_I3C_USE_IBI */
	}

	return 0;
}

static int init_scl_timing(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	uint32_t scl_timing, hcnt, lcnt, core_rate;

	if (clock_control_get_rate(config->clock, NULL, &core_rate) != 0) {
		LOG_ERR("%s: get clock rate failed", dev->name);
		return -EINVAL;
	}

	/* I3C_OD */
	hcnt = DIV_ROUND_UP(config->od_thigh_max_ns * (uint64_t)core_rate, I3C_PERIOD_NS) - 1;
	hcnt = CLAMP(hcnt, SCL_I3C_TIMING_CNT_MIN, SCL_I3C_TIMING_CNT_MAX);

	lcnt = DIV_ROUND_UP(config->od_tlow_min_ns * (uint64_t)core_rate, I3C_PERIOD_NS);
	lcnt = CLAMP(lcnt, SCL_I3C_TIMING_CNT_MIN, SCL_I3C_TIMING_CNT_MAX);

	scl_timing = SCL_I3C_TIMING_HCNT(hcnt) | SCL_I3C_TIMING_LCNT(lcnt);
	sys_write32(scl_timing, config->regs + SCL_I3C_OD_TIMING);

	/* Set bus free timing to match tlow setting for OD clk config. */
	sys_write32(BUS_I3C_MST_FREE(lcnt), config->regs + BUS_FREE_TIMING);

	/* I3C_PP */
	hcnt = DIV_ROUND_UP(I3C_BUS_THIGH_MAX_NS * (uint64_t)core_rate, I3C_PERIOD_NS) - 1;
	hcnt = CLAMP(hcnt, SCL_I3C_TIMING_CNT_MIN, SCL_I3C_TIMING_CNT_MAX);

	lcnt = DIV_ROUND_UP(core_rate, data->common.ctrl_config.scl.i3c) - hcnt;
	lcnt = CLAMP(lcnt, SCL_I3C_TIMING_CNT_MIN, SCL_I3C_TIMING_CNT_MAX);

	scl_timing = SCL_I3C_TIMING_HCNT(hcnt) | SCL_I3C_TIMING_LCNT(lcnt);
	sys_write32(scl_timing, config->regs + SCL_I3C_PP_TIMING);

	/* I3C */
	lcnt = DIV_ROUND_UP(core_rate, I3C_BUS_SDR1_SCL_RATE) - hcnt;
	scl_timing = SCL_EXT_LCNT_1(lcnt);
	lcnt = DIV_ROUND_UP(core_rate, I3C_BUS_SDR2_SCL_RATE) - hcnt;
	scl_timing |= SCL_EXT_LCNT_2(lcnt);
	lcnt = DIV_ROUND_UP(core_rate, I3C_BUS_SDR3_SCL_RATE) - hcnt;
	scl_timing |= SCL_EXT_LCNT_3(lcnt);
	lcnt = DIV_ROUND_UP(core_rate, I3C_BUS_SDR4_SCL_RATE) - hcnt;
	scl_timing |= SCL_EXT_LCNT_4(lcnt);
	sys_write32(scl_timing, config->regs + SCL_EXT_LCNT_TIMING);

	/* I2C FM+ */
	lcnt = DIV_ROUND_UP(I3C_BUS_I2C_FMP_TLOW_MIN_NS * (uint64_t)core_rate, I3C_PERIOD_NS);
	hcnt = DIV_ROUND_UP(core_rate, I3C_BUS_I2C_FM_PLUS_SCL_RATE) - lcnt;
	scl_timing = SCL_I2C_FMP_TIMING_HCNT(hcnt) | SCL_I2C_FMP_TIMING_LCNT(lcnt);
	sys_write32(scl_timing, config->regs + SCL_I2C_FMP_TIMING);

	/* I2C FM */
	lcnt = DIV_ROUND_UP(I3C_BUS_I2C_FM_TLOW_MIN_NS * (uint64_t)core_rate, I3C_PERIOD_NS);
	hcnt = DIV_ROUND_UP(core_rate, I3C_BUS_I2C_FM_SCL_RATE) - lcnt;
	scl_timing = SCL_I2C_FM_TIMING_HCNT(hcnt) | SCL_I2C_FM_TIMING_LCNT(lcnt);
	sys_write32(scl_timing, config->regs + SCL_I2C_FM_TIMING);

	if (data->mode != I3C_BUS_MODE_PURE) {
		sys_write32(BUS_I3C_MST_FREE(lcnt), config->regs + BUS_FREE_TIMING);
		sys_write32(sys_read32(config->regs + DEVICE_CTRL) | DEV_CTRL_I2C_SLAVE_PRESENT,
			    config->regs + DEVICE_CTRL);
	}
	/* I3C Bus Available Time */
	scl_timing = DIV_ROUND_UP(I3C_BUS_AVAILABLE_TIME_NS * (uint64_t)core_rate,
					I3C_PERIOD_NS);
	sys_write32(BUS_I3C_AVAIL_TIME(scl_timing), config->regs + BUS_FREE_TIMING);

	/* I3C Bus Idle Time */
	scl_timing =
		DIV_ROUND_UP(I3C_BUS_IDLE_TIME_NS * (uint64_t)core_rate, I3C_PERIOD_NS);
	sys_write32(BUS_I3C_IDLE_TIME(scl_timing), config->regs + BUS_IDLE_TIMING);

	return 0;
}

/**
 * Determine I3C bus mode from the i2c devices on the bus
 *
 * Reads the LVR of all I2C devices and returns the I3C bus
 * Mode
 *
 * @param dev_list Pointer to device list
 *
 * @return @see enum i3c_bus_mode.
 */
static enum i3c_bus_mode i3c_bus_mode(const struct i3c_dev_list *dev_list)
{
	enum i3c_bus_mode mode = I3C_BUS_MODE_PURE;

	for (int i = 0; i < dev_list->num_i2c; i++) {
		switch (I3C_LVR_I2C_DEV_IDX(dev_list->i2c[i].lvr)) {
		case I3C_LVR_I2C_DEV_IDX_0:
			if (mode < I3C_BUS_MODE_MIXED_FAST) {
				mode = I3C_BUS_MODE_MIXED_FAST;
			}
			break;
		case I3C_LVR_I2C_DEV_IDX_1:
			if (mode < I3C_BUS_MODE_MIXED_LIMITED) {
				mode = I3C_BUS_MODE_MIXED_LIMITED;
			}
			break;
		case I3C_LVR_I2C_DEV_IDX_2:
			if (mode < I3C_BUS_MODE_MIXED_SLOW) {
				mode = I3C_BUS_MODE_MIXED_SLOW;
			}
			break;
		default:
			mode = I3C_BUS_MODE_INVALID;
			break;
		}
	}
	return mode;
}

static int dw_i3c_attach_device(const struct device *dev, struct i3c_device_desc *desc)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	uint8_t pos = get_free_pos(data->free_pos);
	uint8_t addr = desc->dynamic_addr ? desc->dynamic_addr : desc->static_addr;

	if (pos < 0) {
		LOG_ERR("%s: no space for i3c device: %s", dev->name, desc->dev->name);
		return -ENOSPC;
	}

	data->dw_i3c_i2c_priv_data[pos].id = pos;
	desc->controller_priv = &(data->dw_i3c_i2c_priv_data[pos]);
	data->free_pos &= ~BIT(pos);

	LOG_DBG("%s: Attaching %s", dev->name, desc->dev->name);

	sys_write32(DEV_ADDR_TABLE_DYNAMIC_ADDR(addr),
		    config->regs + DEV_ADDR_TABLE_LOC(data->datstartaddr, pos));

	return 0;
}

static int dw_i3c_reattach_device(const struct device *dev, struct i3c_device_desc *desc,
				  uint8_t old_dyn_addr)
{
	ARG_UNUSED(old_dyn_addr);

	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	struct dw_i3c_i2c_dev_data *dw_i3c_device_data = desc->controller_priv;
	uint32_t dat;

	if (dw_i3c_device_data == NULL) {
		LOG_ERR("%s: %s: device not attached", dev->name, desc->dev->name);
		return -EINVAL;
	}
	/* TODO: investigate clearing table beforehand */

	LOG_DBG("Reattaching %s", desc->dev->name);

	dat = sys_read32(config->regs +
			 DEV_ADDR_TABLE_LOC(data->datstartaddr, dw_i3c_device_data->id));
	dat &= ~DEV_ADDR_TABLE_DYNAMIC_ADDR_MASK;
	sys_write32(DEV_ADDR_TABLE_DYNAMIC_ADDR(desc->dynamic_addr) | dat,
		    config->regs + DEV_ADDR_TABLE_LOC(data->datstartaddr, dw_i3c_device_data->id));

	return 0;
}

static int dw_i3c_detach_device(const struct device *dev, struct i3c_device_desc *desc)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	struct dw_i3c_i2c_dev_data *dw_i3c_device_data = desc->controller_priv;

	if (dw_i3c_device_data == NULL) {
		LOG_ERR("%s: %s: device not attached", dev->name, desc->dev->name);
		return -EINVAL;
	}

	LOG_DBG("%s: Detaching %s", dev->name, desc->dev->name);

	sys_write32(0,
		    config->regs + DEV_ADDR_TABLE_LOC(data->datstartaddr, dw_i3c_device_data->id));
	data->free_pos |= BIT(dw_i3c_device_data->id);
	desc->controller_priv = NULL;

	return 0;
}

static int set_controller_info(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	uint8_t controller_da;

	if (config->common.primary_controller_da) {
		if (!i3c_addr_slots_is_free(&data->common.attached_dev.addr_slots,
					    config->common.primary_controller_da)) {
			controller_da = i3c_addr_slots_next_free_find(
				&data->common.attached_dev.addr_slots, 0);
			LOG_WRN("%s: 0x%02x DA selected for controller as 0x%02x is unavailable",
				dev->name, controller_da, config->common.primary_controller_da);
		} else {
			controller_da = config->common.primary_controller_da;
		}
	} else {
		controller_da =
			i3c_addr_slots_next_free_find(&data->common.attached_dev.addr_slots, 0);
	}

	sys_write32(DEVICE_ADDR_DYNAMIC_ADDR_VALID | DEVICE_ADDR_DYNAMIC(controller_da),
		    config->regs + DEVICE_ADDR);
	/* Mark the address as I3C device */
	i3c_addr_slots_mark_i3c(&data->common.attached_dev.addr_slots, controller_da);

	return 0;
}

static void enable_interrupts(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;
	uint32_t thld_ctrl;

	config->irq_config_func();

	thld_ctrl = sys_read32(config->regs + QUEUE_THLD_CTRL);
	thld_ctrl &= (~QUEUE_THLD_CTRL_RESP_BUF_MASK & ~QUEUE_THLD_CTRL_IBI_STS_MASK);
	sys_write32(thld_ctrl, config->regs + QUEUE_THLD_CTRL);

	thld_ctrl = sys_read32(config->regs + DATA_BUFFER_THLD_CTRL);
	thld_ctrl &= ~DATA_BUFFER_THLD_CTRL_RX_BUF;
	sys_write32(thld_ctrl, config->regs + DATA_BUFFER_THLD_CTRL);

	sys_write32(INTR_ALL, config->regs + INTR_STATUS);

	sys_write32(INTR_SLAVE_MASK | INTR_MASTER_MASK, config->regs + INTR_STATUS_EN);
	sys_write32(INTR_SLAVE_MASK | INTR_MASTER_MASK, config->regs + INTR_SIGNAL_EN);
}

/**
 * @brief Calculate the odd parity of a byte.
 *
 * This function calculates the odd parity of the input byte, returning 1 if the
 * number of set bits is odd and 0 otherwise.
 *
 * @param p The byte for which odd parity is to be calculated.
 *
 * @return The odd parity result (1 if odd, 0 if even).
 */
static uint8_t odd_parity(uint8_t p)
{
	p ^= p >> 4;
	p &= 0xf;
	return (0x9669 >> p) & 1;
}

/**
 * @brief Send Common Command Code (CCC).
 *
 * @see i3c_do_ccc
 *
 * @param dev Pointer to controller device driver instance.
 * @param payload Pointer to CCC payload.
 *
 * @return @see i3c_do_ccc
 */
static int dw_i3c_do_ccc(const struct device *dev, struct i3c_ccc_payload *payload)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	struct dw_i3c_xfer *xfer = &data->xfer;
	struct dw_i3c_cmd *cmd;
	int ret, i, pos;
	uint32_t present_state;

	present_state = sys_read32(config->regs + PRESENT_STATE);
	if (!(present_state & PRESENT_STATE_CURRENT_MASTER)) {
		return -EACCES;
	}

	ret = k_mutex_lock(&data->mt, K_MSEC(1000));
	if (ret) {
		LOG_DBG("%s: Mutex err (%d)", dev->name, ret);
		return ret;
	}

	pm_device_busy_set(dev);

	memset(xfer, 0, sizeof(struct dw_i3c_xfer));
	xfer->ret = -1;

	/* in the case of multiple targets in a CCC, each command queue must have the same CCC ID
	 * loaded along with different dev index fields pointing to the targets
	 */
	if (i3c_ccc_is_payload_broadcast(payload)) {
		xfer->ncmds = 1;
		cmd = &xfer->cmds[0];
		cmd->buf = payload->ccc.data;

		cmd->cmd_hi = COMMAND_PORT_ARG_DATA_LEN(payload->ccc.data_len) |
			      COMMAND_PORT_TRANSFER_ARG;
		cmd->cmd_lo = COMMAND_PORT_CP | COMMAND_PORT_TOC | COMMAND_PORT_ROC |
			      COMMAND_PORT_CMD(payload->ccc.id);

		if ((payload->targets.payloads) && (payload->targets.payloads[0].rnw)) {
			cmd->cmd_lo |= COMMAND_PORT_READ_TRANSFER;
			cmd->rx_len = payload->ccc.data_len;
		} else {
			cmd->tx_len = payload->ccc.data_len;
		}
	} else {
		if (!(payload->targets.payloads)) {
			LOG_ERR("%s: Direct CCC Payload structure Empty", dev->name);
			ret = -EINVAL;
			goto error;
		}
		xfer->ncmds = payload->targets.num_targets;
		for (i = 0; i < payload->targets.num_targets; i++) {
			cmd = &xfer->cmds[i];
			/* Look up position, SETDASA will perform the look up by static addr */
			pos = get_i3c_addr_pos(dev, payload->targets.payloads[i].addr,
					       payload->ccc.id == I3C_CCC_SETDASA);
			if (pos < 0) {
				LOG_ERR("%s: Invalid Slave address with pos %d", dev->name, pos);
				ret = -ENOSPC;
				goto error;
			}
			cmd->buf = payload->targets.payloads[i].data;

			cmd->cmd_hi =
				COMMAND_PORT_ARG_DATA_LEN(payload->targets.payloads[i].data_len) |
				COMMAND_PORT_TRANSFER_ARG;
			cmd->cmd_lo = COMMAND_PORT_CP | COMMAND_PORT_DEV_INDEX(pos) |
				      COMMAND_PORT_ROC | COMMAND_PORT_CMD(payload->ccc.id);
			/* last command queue with multiple targets must have TOC set */
			if (i == (payload->targets.num_targets - 1)) {
				cmd->cmd_lo |= COMMAND_PORT_TOC;
			}
			/* If there is a defining byte for direct CCC */
			if (payload->ccc.data_len == 1) {
				cmd->cmd_lo |= COMMAND_PORT_DBP;
				cmd->cmd_hi |= COMMAND_PORT_ARG_DB(payload->ccc.data[0]);
			} else if (payload->ccc.data_len > 1) {
				LOG_ERR("%s: direct CCCs defining byte >1", dev->name);
				ret = -EINVAL;
				goto error;
			}

			if (payload->targets.payloads[i].rnw) {
				cmd->cmd_lo |= COMMAND_PORT_READ_TRANSFER;
				cmd->rx_len = payload->targets.payloads[i].data_len;
			} else {
				cmd->tx_len = payload->targets.payloads[i].data_len;
			}
		}
	}

	start_xfer(dev);

	ret = k_sem_take(&data->sem_xfer, K_MSEC(CONFIG_I3C_DW_RW_TIMEOUT_MS));
	if (ret) {
		LOG_ERR("%s: Semaphore err (%d)", dev->name, ret);
		goto error;
	}

	/* the only way data_len would not equal num_xfer would be if an abort happened */
	payload->ccc.num_xfer = payload->ccc.data_len;
	for (i = 0; i < xfer->ncmds; i++) {
		/* if this is a direct ccc, then write back the number of bytes tx or rx */
		if (!i3c_ccc_is_payload_broadcast(payload)) {
			payload->targets.payloads[i].num_xfer = payload->targets.payloads[i].rnw
									? xfer->cmds[i].rx_len
									: xfer->cmds[i].tx_len;
		}
		if (xfer->cmds[i].rx_len && !xfer->cmds[i].error) {
			read_rx_fifo(dev, xfer->cmds[i].buf, xfer->cmds[i].rx_len);
		}
	}

	ret = xfer->ret;
error:
	pm_device_busy_clear(dev);
	k_mutex_unlock(&data->mt);

	return ret;
}

/**
 * @brief Add a slave device from Dynamic Address Assignment (DAA) information.
 *
 * This function adds a slave device to the I3C controller based on the Dynamic
 * Address Assignment (DAA) information at the specified position. It retrieves
 * the dynamic address, PID (Provisional ID), and additional device characteristics
 * from the corresponding tables and associates the device with a registered device
 * descriptor if the PID is known.
 *
 * @param dev Pointer to the I3C device structure.
 * @param pos Position of the device in the DAA and DCT tables.
 *
 * @return 0 on success, or a negative error code on failure.
 */
static int add_slave_from_daa(const struct device *dev, int32_t pos)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	uint32_t tmp;
	uint64_t pid;
	uint8_t dyn_addr;

	/* retrieve dynamic address assigned */
	tmp = sys_read32(config->regs + DEV_ADDR_TABLE_LOC(data->datstartaddr, pos));
	dyn_addr = (((tmp) & GENMASK(22, 16)) >> 16);

	/* retrieve pid */
	tmp = sys_read32(config->regs + DEV_CHAR_TABLE_LOC1(data->dctstartaddr, pos));
	pid = ((uint64_t)DEV_CHAR_TABLE_MSB_PID(tmp) << 16) + (DEV_CHAR_TABLE_LSB_PID(tmp) << 16);
	tmp = sys_read32(config->regs + DEV_CHAR_TABLE_LOC2(data->dctstartaddr, pos));
	pid |= DEV_CHAR_TABLE_LSB_PID(tmp);

	/* lookup known pids */
	const struct i3c_device_id i3c_id = I3C_DEVICE_ID(pid);
	struct i3c_device_desc *target = i3c_device_find(dev, &i3c_id);

	if (target == NULL) {
		LOG_INF("%s: PID 0x%012llx is not in registered device "
			"list, given DA 0x%02x",
			dev->name, pid, dyn_addr);
	} else {
		target->dynamic_addr = dyn_addr;
		tmp = sys_read32(config->regs + DEV_CHAR_TABLE_LOC3(data->dctstartaddr, pos));
		target->bcr = DEV_CHAR_TABLE_BCR(tmp);
		target->dcr = DEV_CHAR_TABLE_DCR(tmp);

		LOG_DBG("%s: PID 0x%012llx assigned dynamic address 0x%02x", dev->name, pid,
			dyn_addr);
	}
	i3c_addr_slots_mark_i3c(&data->common.attached_dev.addr_slots, dyn_addr);

	return 0;
}

/**
 * @brief Perform Dynamic Address Assignment.
 *
 * @see i3c_do_daa
 *
 * @param dev Pointer to controller device driver instance.
 *
 * @return @see i3c_do_daa
 */
static int dw_i3c_do_daa(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	struct dw_i3c_xfer *xfer = &data->xfer;
	struct dw_i3c_cmd *cmd;
	uint32_t olddevs, newdevs;
	uint8_t p, idx, last_addr = 0;
	int32_t pos, addr, ret;
	uint32_t present_state;

	present_state = sys_read32(config->regs + PRESENT_STATE);
	if (!(present_state & PRESENT_STATE_CURRENT_MASTER)) {
		return -EACCES;
	}

	olddevs = ~(data->free_pos);

	/* Prepare DAT before launching DAA. */
	for (pos = 0; pos < data->maxdevs; pos++) {
		if (olddevs & BIT(pos)) {
			continue;
		}

		addr = i3c_addr_slots_next_free_find(&data->common.attached_dev.addr_slots,
						     last_addr + 1);
		if (addr == 0) {
			return -ENOSPC;
		}

		p = odd_parity(addr);
		last_addr = addr;
		addr |= (p << 7);
		sys_write32(DEV_ADDR_TABLE_DYNAMIC_ADDR(addr),
			    config->regs + DEV_ADDR_TABLE_LOC(data->datstartaddr, pos));
	}

	pos = get_free_pos(data->free_pos);
	if (pos < 0) {
		LOG_ERR("%s: find free pos failed", dev->name);
		return -ENOSPC;
	}

	ret = k_mutex_lock(&data->mt, K_MSEC(1000));
	if (ret) {
		LOG_ERR("%s: Mutex err (%d)", dev->name, ret);
		return ret;
	}

	pm_device_busy_set(dev);

	memset(xfer, 0, sizeof(struct dw_i3c_xfer));

	xfer->ncmds = 1;
	xfer->ret = -1;

	cmd = &xfer->cmds[0];
	cmd->cmd_hi = COMMAND_PORT_TRANSFER_ARG;
	cmd->cmd_lo = COMMAND_PORT_TOC | COMMAND_PORT_ROC |
		      COMMAND_PORT_DEV_COUNT(data->maxdevs - pos) | COMMAND_PORT_DEV_INDEX(pos) |
		      COMMAND_PORT_CMD(I3C_CCC_ENTDAA) | COMMAND_PORT_ADDR_ASSGN_CMD;

	start_xfer(dev);
	ret = k_sem_take(&data->sem_xfer, K_MSEC(CONFIG_I3C_DW_RW_TIMEOUT_MS));

	pm_device_busy_clear(dev);
	k_mutex_unlock(&data->mt);

	if (ret) {
		LOG_ERR("%s: Semaphore err (%d)", dev->name, ret);
		return ret;
	}

	if (data->maxdevs == cmd->rx_len) {
		newdevs = 0;
	} else {
		newdevs = GENMASK(data->maxdevs - cmd->rx_len - 1, 0);
	}
	newdevs &= ~olddevs;

	for (pos = find_lsb_set(newdevs); pos <= find_msb_set(newdevs); pos++) {
		idx = pos - 1;
		if (newdevs & BIT(idx)) {
			add_slave_from_daa(dev, idx);
		}
	}

	return 0;
}

static void dw_i3c_enable_controller(const struct dw_i3c_config *config, bool enable)
{
	uint32_t reg = sys_read32(config->regs + DEVICE_CTRL);

	if (enable) {
		reg |= DEV_CTRL_ENABLE;
	} else {
		reg &= ~DEV_CTRL_ENABLE;
	}

	sys_write32(reg, config->regs + DEVICE_CTRL);
}

/**
 * @brief Get configuration of the I3C hardware.
 *
 * This provides a way to get the current configuration of the I3C hardware.
 *
 * This can return cached config or probed hardware parameters, but it has to
 * be up to date with current configuration.
 *
 * @param[in] dev Pointer to controller device driver instance.
 * @param[in] type Type of configuration parameters being passed
 *                 in @p config.
 * @param[in,out] config Pointer to the configuration parameters.
 *
 * Note that if @p type is @c I3C_CONFIG_CUSTOM, @p config must contain
 * the ID of the parameter to be retrieved.
 *
 * @retval 0 If successful.
 * @retval -EIO General Input/Output errors.
 * @retval -ENOSYS If not implemented.
 */
static int dw_i3c_config_get(const struct device *dev, enum i3c_config_type type, void *config)
{
	const struct dw_i3c_config *dev_config = dev->config;
	struct dw_i3c_data *data = dev->data;
	int ret = 0;

	if (type == I3C_CONFIG_CONTROLLER) {
		(void)memcpy(config, &data->common.ctrl_config, sizeof(data->common.ctrl_config));
	} else if (type == I3C_CONFIG_TARGET) {
		struct i3c_config_target *target_config = config;
		uint32_t reg;

		reg = sys_read32(dev_config->regs + SLV_MAX_LEN);
		target_config->max_read_len = SLV_MAX_LEN_MRL(reg);
		target_config->max_write_len = SLV_MAX_LEN_MWL(reg);

		reg = sys_read32(dev_config->regs + DEVICE_ADDR);
		if (reg & DEVICE_ADDR_STATIC_ADDR_VALID) {
			target_config->static_addr = DEVICE_ADDR_STATIC(reg);
		} else {
			target_config->static_addr = 0x00;
		}

		reg = sys_read32(dev_config->regs + SLV_CHAR_CTRL);
		target_config->bcr = SLV_CHAR_CTRL_BCR(reg);
		target_config->dcr = SLV_CHAR_CTRL_DCR(reg);
		target_config->supported_hdr = SLV_CHAR_CTRL_HDR_CAP(reg);

		reg = sys_read32(dev_config->regs + SLV_MIPI_ID_VALUE);
		target_config->pid = ((uint64_t)reg) << 32;
		target_config->pid_random = (bool)!!(reg & SLV_MIPI_ID_VALUE_SLV_PROV_ID_SEL);
		reg = sys_read32(dev_config->regs + SLV_PID_VALUE);
		target_config->pid |= reg;

		if (!(sys_read32(dev_config->regs + PRESENT_STATE) &
		      PRESENT_STATE_CURRENT_MASTER)) {
			target_config->enabled = true;
		} else {
			target_config->enabled = false;
		}
	} else {
		return -EINVAL;
	}

	return ret;
}

/**
 * @brief Configure I3C hardware.
 *
 * @param dev Pointer to controller device driver instance.
 * @param type Type of configuration parameters being passed
 *             in @p config.
 * @param config Pointer to the configuration parameters.
 *
 * @retval 0 If successful.
 * @retval -EINVAL If invalid configure parameters.
 * @retval -EIO General Input/Output errors.
 * @retval -ENOSYS If not implemented.
 */
static int dw_i3c_configure(const struct device *dev, enum i3c_config_type type, void *config)
{
	const struct dw_i3c_config *dev_config = dev->config;

	if (type == I3C_CONFIG_CONTROLLER) {
		/* struct i3c_config_controller *ctrl_cfg = config; */
		/* TODO: somehow determine i3c rate? snps is complicated */
		return -ENOTSUP;
	} else if (type == I3C_CONFIG_TARGET) {
		struct i3c_config_target *target_cfg = config;
		uint32_t val;

		/* TODO: some how randomly generate pid */
		if (target_cfg->pid_random) {
			return -EINVAL;
		}

		val = SLV_MAX_LEN_MWL(target_cfg->max_write_len) |
		      (SLV_MAX_LEN_MRL(target_cfg->max_read_len) << 16);
		sys_write32(val, dev_config->regs + SLV_MAX_LEN);

		/* set static address */
		val = sys_read32(dev_config->regs + DEVICE_ADDR);
		/* if static address is set to 0x00, then disable static_addr_en */
		if (target_cfg->static_addr != 0x00) {
			val |= DEVICE_ADDR_STATIC_ADDR_VALID;
		} else {
			val &= ~DEVICE_ADDR_STATIC_ADDR_VALID;
		}
		val &= ~DEVICE_ADDR_STATIC_MASK;
		val |= DEVICE_ADDR_STATIC(target_cfg->static_addr);
		sys_write32(val, dev_config->regs + DEVICE_ADDR);

		val = sys_read32(dev_config->regs + SLV_CHAR_CTRL);
		val &= ~(SLV_CHAR_CTRL_BCR_MASK | SLV_CHAR_CTRL_DCR_MASK);
		/* Bridge identifier, offline capable, ibi_payload, ibi_request_capable can not be
		 * written to in bcr
		 */
		val |= SLV_CHAR_CTRL_BCR(target_cfg->bcr);
		val |= SLV_CHAR_CTRL_DCR(target_cfg->dcr) << 8;
		/* HDR CAPs is not settable */
		sys_write32(val, dev_config->regs + SLV_CHAR_CTRL);

		val = sys_read32(dev_config->regs + SLV_MIPI_ID_VALUE);
		val &= ~(SLV_MIPI_ID_VALUE_SLV_MIPI_MFG_ID_MASK |
			 SLV_MIPI_ID_VALUE_SLV_PROV_ID_SEL);
		val |= (uint32_t)(target_cfg->pid >> 16);
		sys_write32(val, dev_config->regs + SLV_MIPI_ID_VALUE);

		val = (uint32_t)(target_cfg->pid & 0xFFFFFFFF);
		sys_write32(val, dev_config->regs + SLV_PID_VALUE);
	}

	return 0;
}

/**
 * @brief Find a registered I3C target device.
 *
 * This returns the I3C device descriptor of the I3C device
 * matching the incoming @p id.
 *
 * @param dev Pointer to controller device driver instance.
 * @param id Pointer to I3C device ID.
 *
 * @return @see i3c_device_find.
 */
static struct i3c_device_desc *dw_i3c_device_find(const struct device *dev,
						  const struct i3c_device_id *id)
{
	const struct dw_i3c_config *config = dev->config;

	return i3c_dev_list_find(&config->common.dev_list, id);
}

/**
 * @brief Writes to the Target's TX FIFO
 *
 * The Synopsys I3C will then ACK read requests to it's TX FIFO from a
 * Controller, if there is no tx cmd in cmd Q. Then it will NACK.
 *
 * @param dev Pointer to the device structure for an I3C controller
 *            driver configured in target mode.
 * @param buf Pointer to the buffer
 * @param len Length of the buffer
 *
 * @retval Total number of bytes written
 * @retval -EACCES Not in Target Mode
 * @retval -ENOSPC No space in Tx FIFO
 */
static int dw_i3c_target_tx_write(const struct device *dev, uint8_t *buf, uint16_t len,
				  uint8_t hdr_mode)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	struct dw_i3c_xfer *xfer = &data->xfer;
	uint32_t present_state;

	/* check if we are in target mode */
	present_state = sys_read32(config->regs + PRESENT_STATE);
	if (present_state & PRESENT_STATE_CURRENT_MASTER) {
		return -EACCES;
	}

	/*
	 * TODO: if len is greater than fifo size, then it will need to be written to based
	 * on the threshold interrupt
	 */
	if (len > (data->txfifodepth * BYTES_PER_DWORD)) {
		return -ENOSPC;
	}

	k_mutex_lock(&data->mt, K_FOREVER);

	if ((hdr_mode == 0) || (hdr_mode & data->common.ctrl_config.supported_hdr)) {
		/* Write to CMD */
		memset(xfer, 0, sizeof(struct dw_i3c_xfer));
		xfer->ncmds = 1;

		/* TODO: write_tx_fifo needs to check that the fifo doesn't fill up */
		struct dw_i3c_cmd *cmd = &xfer->cmds[0];

		cmd->cmd_hi = 0;
		cmd->cmd_lo = COMMAND_PORT_TID(0) | COMMAND_PORT_ARG_DATA_LEN(len);
		cmd->buf = buf;
		cmd->tx_len = len;

		start_xfer(dev);
	} else {
		k_mutex_unlock(&data->mt);
		LOG_ERR("%s: Unsupported HDR Mode %d", dev->name, hdr_mode);
		return -ENOTSUP;
	}

	k_mutex_unlock(&data->mt);

	/* return total bytes written */
	return (int)len;
}

/**
 * @brief Instructs the I3C Target device to register itself to the I3C Controller
 *
 * This routine instructs the I3C Target device to register itself to the I3C
 * Controller via its parent controller's i3c_target_register() API.
 *
 * @param dev Pointer to target device driver instance.
 * @param cfg Config struct with functions and parameters used by the I3C driver
 * to send bus events
 *
 * @return @see i3c_device_find.
 */
static int dw_i3c_target_register(const struct device *dev, struct i3c_target_config *cfg)
{
	struct dw_i3c_data *data = dev->data;

	data->target_config = cfg;
	return 0;
}

/**
 * @brief Unregisters the provided config as Target device
 *
 * This routine disables I3C target mode for the 'dev' I3C bus driver using
 * the provided 'config' struct containing the functions and parameters
 * to send bus events.
 *
 * @param dev Pointer to target device driver instance.
 * @param cfg Config struct with functions and parameters used by the I3C driver
 * to send bus events
 *
 * @return @see i3c_device_find.
 */
static int dw_i3c_target_unregister(const struct device *dev, struct i3c_target_config *cfg)
{
	/* no way to disable? maybe write DA to 0? */
	return 0;
}

static int dw_i3c_pinctrl_enable(const struct device *dev, bool enable)
{
#ifdef CONFIG_PINCTRL
	const struct dw_i3c_config *config = dev->config;
	uint8_t state = enable ? PINCTRL_STATE_DEFAULT : PINCTRL_STATE_SLEEP;
	int ret;

	ret = pinctrl_apply_state(config->pcfg, state);
	if (ret == -ENOENT) {
		/* State not defined; ignore and return success. */
		ret = 0;
	}

	return ret;
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(enable);
	return 0;
#endif
}

static int dw_i3c_init(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	struct i3c_config_controller *ctrl_config = &data->common.ctrl_config;
	int ret;
	uint32_t hw_capabilities;
	uint32_t queue_capability;
	uint32_t device_ctrl_ext;

	if (!device_is_ready(config->clock)) {
		return -ENODEV;
	}

	ret = clock_control_on(config->clock, NULL);
	if (ret < 0) {
		return ret;
	}

#ifdef CONFIG_I3C_USE_IBI
	k_sem_init(&data->ibi_sts_sem, 0, 1);
#endif
	k_sem_init(&data->sem_xfer, 0, 1);
	k_mutex_init(&data->mt);

	dw_i3c_pinctrl_enable(dev, true);

	data->mode = i3c_bus_mode(&config->common.dev_list);

	/* reset all */
	sys_write32(RESET_CTRL_ALL, config->regs + RESET_CTRL);

	/* get DAT, DCT pointer */
	data->datstartaddr =
		DEVICE_ADDR_TABLE_ADDR(sys_read32(config->regs + DEVICE_ADDR_TABLE_POINTER));
	data->dctstartaddr =
		DEVICE_CHAR_TABLE_ADDR(sys_read32(config->regs + DEV_CHAR_TABLE_POINTER));

	/* get max devices based on table depth */
	data->maxdevs =
		DEVICE_ADDR_TABLE_DEPTH(sys_read32(config->regs + DEVICE_ADDR_TABLE_POINTER));
	data->free_pos = GENMASK(data->maxdevs - 1, 0);

	/* get fifo sizes */
	queue_capability = sys_read32(config->regs + QUEUE_SIZE_CAPABILITY);
	data->txfifodepth = QUEUE_SIZE_CAPABILITY_TX_BUF_DWORD_SIZE(queue_capability);
	data->rxfifodepth = QUEUE_SIZE_CAPABILITY_RX_BUF_DWORD_SIZE(queue_capability);
	data->cmdfifodepth = QUEUE_SIZE_CAPABILITY_CMD_BUF_DWORD_SIZE(queue_capability);
	data->respfifodepth = QUEUE_SIZE_CAPABILITY_RESP_BUF_DWORD_SIZE(queue_capability);
	data->ibififodepth = QUEUE_SIZE_CAPABILITY_IBI_BUF_DWORD_SIZE(queue_capability);

	/* get HDR capabilities */
	ctrl_config->supported_hdr = 0;
	hw_capabilities = sys_read32(config->regs + HW_CAPABILITY);
	if (hw_capabilities & HW_CAPABILITY_HDR_TS_EN) {
		ctrl_config->supported_hdr |= I3C_MSG_HDR_TSP | I3C_MSG_HDR_TSL;
	}
	if (hw_capabilities & HW_CAPABILITY_HDR_DDR_EN) {
		ctrl_config->supported_hdr |= I3C_MSG_HDR_DDR;
	}

	/* if the boot condition starts as a target, then it's a secondary controller */
	device_ctrl_ext = sys_read32(config->regs + DEVICE_CTRL_EXTENDED);
	if (DEVICE_CTRL_EXTENDED_DEV_OPERATION_MODE(device_ctrl_ext) &
	    DEVICE_CTRL_EXTENDED_DEV_OPERATION_MODE_SLAVE) {
		ctrl_config->is_secondary = true;
	} else {
		ctrl_config->is_secondary = false;
	}

	ret = init_scl_timing(dev);
	if (ret != 0) {
		return ret;
	}

	enable_interrupts(dev);

	/* disable ibi */
	sys_write32(IBI_REQ_REJECT_ALL, config->regs + IBI_SIR_REQ_REJECT);
	sys_write32(IBI_REQ_REJECT_ALL, config->regs + IBI_MR_REQ_REJECT);

	/* disable hot-join */
	sys_write32(sys_read32(config->regs + DEVICE_CTRL) | (DEV_CTRL_HOT_JOIN_NACK),
		    config->regs + DEVICE_CTRL);

	ret = i3c_addr_slots_init(dev);
	if (ret != 0) {
		return ret;
	}

	dw_i3c_enable_controller(config, true);

	if (!(ctrl_config->is_secondary)) {
		ret = set_controller_info(dev);
		if (ret) {
			return ret;
		}
		/* Perform bus initialization - skip if no I3C devices are known. */
		if (config->common.dev_list.num_i3c > 0) {
			ret = i3c_bus_init(dev, &config->common.dev_list);
		}
		/* Bus Initialization Complete, allow HJ ACKs */
		sys_write32(sys_read32(config->regs + DEVICE_CTRL) & ~(DEV_CTRL_HOT_JOIN_NACK),
			    config->regs + DEVICE_CTRL);
	}

	return 0;
}

#if defined(CONFIG_PM_DEVICE)
static int dw_i3c_pm_ctrl(const struct device *dev, enum pm_device_action action)
{
	const struct dw_i3c_config *config = dev->config;

	LOG_DBG("PM action: %d", (int)action);

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		dw_i3c_enable_controller(config, false);
		dw_i3c_pinctrl_enable(dev, false);
		break;

	case PM_DEVICE_ACTION_RESUME:
		dw_i3c_pinctrl_enable(dev, true);
		dw_i3c_enable_controller(config, true);
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

static DEVICE_API(i3c, dw_i3c_api) = {
	.i2c_api.transfer = dw_i3c_i2c_api_transfer,
#ifdef CONFIG_I2C_RTIO
	.i2c_api.iodev_submit = i2c_iodev_submit_fallback,
#endif

	.configure = dw_i3c_configure,
	.config_get = dw_i3c_config_get,

	.attach_i3c_device = dw_i3c_attach_device,
	.reattach_i3c_device = dw_i3c_reattach_device,
	.detach_i3c_device = dw_i3c_detach_device,

	.do_daa = dw_i3c_do_daa,
	.do_ccc = dw_i3c_do_ccc,

	.i3c_device_find = dw_i3c_device_find,

	.i3c_xfers = dw_i3c_xfers,

	.target_tx_write = dw_i3c_target_tx_write,
	.target_register = dw_i3c_target_register,
	.target_unregister = dw_i3c_target_unregister,

#ifdef CONFIG_I3C_USE_IBI
	.ibi_hj_response = dw_i3c_controller_ibi_hj_response,
	.ibi_enable = dw_i3c_controller_enable_ibi,
	.ibi_disable = dw_i3c_controller_disable_ibi,
	.ibi_raise = dw_i3c_target_ibi_raise,
#endif /* CONFIG_I3C_USE_IBI */

#ifdef CONFIG_I3C_RTIO
	.iodev_submit = i3c_iodev_submit_fallback,
#endif
};

#define I3C_DW_IRQ_HANDLER(n)                                                                      \
	static void i3c_dw_irq_config_##n(void)                                                    \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), i3c_dw_irq,                 \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

#if defined(CONFIG_PINCTRL)
#define I3C_DW_PINCTRL_DEFINE(n) PINCTRL_DT_INST_DEFINE(n)
#define I3C_DW_PINCTRL_INIT(n)   .pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),
#else
#define I3C_DW_PINCTRL_DEFINE(n)
#define I3C_DW_PINCTRL_INIT(n)
#endif

#define DEFINE_DEVICE_FN(n)                                                                        \
	I3C_DW_IRQ_HANDLER(n)                                                                      \
	I3C_DW_PINCTRL_DEFINE(n);                                                                  \
	static struct i3c_device_desc dw_i3c_device_array_##n[] = I3C_DEVICE_ARRAY_DT_INST(n);     \
	static struct i3c_i2c_device_desc dw_i3c_i2c_device_array_##n[] =                          \
		I3C_I2C_DEVICE_ARRAY_DT_INST(n);                                                   \
	static struct dw_i3c_data dw_i3c_data_##n = {                                              \
		.common.ctrl_config.scl.i3c =                                                      \
			DT_INST_PROP_OR(n, i3c_scl_hz, I3C_BUS_TYP_I3C_SCL_RATE),                  \
		.common.ctrl_config.scl.i2c = DT_INST_PROP_OR(n, i2c_scl_hz, 0),                   \
	};                                                                                         \
	static const struct dw_i3c_config dw_i3c_cfg_##n = {                                       \
		.regs = DT_INST_REG_ADDR(n),                                                       \
		.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                    \
		.od_thigh_max_ns = DT_INST_PROP(n, od_thigh_max_ns),                               \
		.od_tlow_min_ns = DT_INST_PROP(n, od_tlow_min_ns),                                 \
		.irq_config_func = &i3c_dw_irq_config_##n,                                         \
		.common.dev_list.i3c = dw_i3c_device_array_##n,                                    \
		.common.dev_list.num_i3c = ARRAY_SIZE(dw_i3c_device_array_##n),                    \
		.common.dev_list.i2c = dw_i3c_i2c_device_array_##n,                                \
		.common.dev_list.num_i2c = ARRAY_SIZE(dw_i3c_i2c_device_array_##n),                \
		.common.primary_controller_da = DT_INST_PROP_OR(n, primary_controller_da, 0x00),   \
		I3C_DW_PINCTRL_INIT(n)};                                                           \
	PM_DEVICE_DT_INST_DEFINE(n, dw_i3c_pm_action);                                             \
	DEVICE_DT_INST_DEFINE(n, dw_i3c_init, PM_DEVICE_DT_INST_GET(n), &dw_i3c_data_##n,          \
			      &dw_i3c_cfg_##n, POST_KERNEL, CONFIG_I3C_CONTROLLER_INIT_PRIORITY,   \
			      &dw_i3c_api);

#define DT_DRV_COMPAT snps_designware_i3c
DT_INST_FOREACH_STATUS_OKAY(DEFINE_DEVICE_FN);
