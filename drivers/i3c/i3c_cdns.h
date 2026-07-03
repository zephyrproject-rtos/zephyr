/*
 * Copyright (c) 2022 Meta Platforms, Inc. and its affiliates.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_I3C_I3C_CDNS_H_
#define ZEPHYR_DRIVERS_I3C_I3C_CDNS_H_

#include <errno.h>
#include <stdint.h>

#include <zephyr/drivers/i3c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#define DEV_ID            0x0
#define DEV_ID_I3C_MASTER 0x5034

#define CONF_STATUS0                      0x4
#define CONF_STATUS0_CMDR_DEPTH(x)        (4 << (((x) & GENMASK(31, 29)) >> 29))
#define CONF_STATUS0_ECC_CHK              BIT(28)
#define CONF_STATUS0_INTEG_CHK            BIT(27)
#define CONF_STATUS0_CSR_DAP_CHK          BIT(26)
#define CONF_STATUS0_TRANS_TOUT_CHK       BIT(25)
#define CONF_STATUS0_PROT_FAULTS_CHK      BIT(24)
#define CONF_STATUS0_GPO_NUM(x)           (((x) & GENMASK(23, 16)) >> 16)
#define CONF_STATUS0_GPI_NUM(x)           (((x) & GENMASK(15, 8)) >> 8)
#define CONF_STATUS0_IBIR_DEPTH(x)        (4 << (((x) & GENMASK(7, 6)) >> 6))
/* CONF_STATUS0_SUPPORTS_DDR moved to CONF_STATUS1 in rev >= 1p7 */
#define CONF_STATUS0_SUPPORTS_DDR         BIT(5)
#define CONF_STATUS0_SEC_MASTER           BIT(4)
/* And it was replaced with a Dev Role mask */
#define CONF_STATUS0_DEV_ROLE(x)          (((x) & GENMASK(5, 4)) >> 4)
#define CONF_STATUS0_DEV_ROLE_MAIN_MASTER 0
#define CONF_STATUS0_DEV_ROLE_SEC_MASTER  1
#define CONF_STATUS0_DEV_ROLE_SLAVE       2
#define CONF_STATUS0_DEVS_NUM(x)          ((x) & GENMASK(3, 0))

#define CONF_STATUS1                     0x8
#define CONF_STATUS1_IBI_HW_RES(x)       ((((x) & GENMASK(31, 28)) >> 28) + 1)
#define CONF_STATUS1_CMD_DEPTH(x)        (4 << (((x) & GENMASK(27, 26)) >> 26))
#define CONF_STATUS1_SLV_DDR_RX_DEPTH(x) (8 << (((x) & GENMASK(25, 21)) >> 21))
#define CONF_STATUS1_SLV_DDR_TX_DEPTH(x) (8 << (((x) & GENMASK(20, 16)) >> 16))
#define CONF_STATUS1_SUPPORTS_DDR        BIT(14)
#define CONF_STATUS1_ALT_MODE            BIT(13)
#define CONF_STATUS1_IBI_DEPTH(x)        (2 << (((x) & GENMASK(12, 10)) >> 10))
#define CONF_STATUS1_RX_DEPTH(x)         (8 << (((x) & GENMASK(9, 5)) >> 5))
#define CONF_STATUS1_TX_DEPTH(x)         (8 << ((x) & GENMASK(4, 0)))

#define REV_ID               0xc
#define REV_ID_VID(id)       (((id) & GENMASK(31, 20)) >> 20)
#define REV_ID_PID(id)       (((id) & GENMASK(19, 8)) >> 8)
#define REV_ID_REV(id)       ((id) & GENMASK(7, 0))
#define REV_ID_VERSION(m, n) (((m) << 5) | (n))
#define REV_ID_REV_MAJOR(id) (((id) & GENMASK(7, 5)) >> 5)
#define REV_ID_REV_MINOR(id) ((id) & GENMASK(4, 0))

#define CTRL                     0x10
#define CTRL_DEV_EN              BIT(31)
#define CTRL_HALT_EN             BIT(30)
#define CTRL_MCS                 BIT(29)
#define CTRL_MCS_EN              BIT(28)
#define CTRL_I3C_11_SUPP         BIT(26)
#define CTRL_THD_DELAY(x)        (((x) << 24) & GENMASK(25, 24))
#define CTRL_TC_EN               BIT(9)
#define CTRL_HJ_DISEC            BIT(8)
#define CTRL_MST_ACK             BIT(7)
#define CTRL_HJ_ACK              BIT(6)
#define CTRL_HJ_INIT             BIT(5)
#define CTRL_MST_INIT            BIT(4)
#define CTRL_AHDR_OPT            BIT(3)
#define CTRL_PURE_BUS_MODE       0
#define CTRL_MIXED_FAST_BUS_MODE 2
#define CTRL_MIXED_SLOW_BUS_MODE 3
#define CTRL_BUS_MODE_MASK       GENMASK(1, 0)
#define THD_DELAY_MAX            3

#define PRESCL_CTRL0         0x14
#define PRESCL_CTRL0_I2C(x)  ((x) << 16)
#define PRESCL_CTRL0_I3C(x)  (x)
#define PRESCL_CTRL0_I3C_MAX GENMASK(9, 0)
#define PRESCL_CTRL0_I2C_MAX GENMASK(15, 0)

#define PRESCL_CTRL1             0x18
#define PRESCL_CTRL1_PP_LOW_MASK GENMASK(15, 8)
#define PRESCL_CTRL1_PP_LOW(x)   ((x) << 8)
#define PRESCL_CTRL1_OD_LOW_MASK GENMASK(7, 0)
#define PRESCL_CTRL1_OD_LOW(x)   (x)

#define SLV_STATUS4                 0x1C
#define SLV_STATUS4_BUSCON_FILL_LVL GENMASK(16, 8)
#define SLV_STATUS5_BUSCON_DATA     GENMASK(7, 0)

#define MST_IER           0x20
#define MST_IDR           0x24
#define MST_IMR           0x28
#define MST_ICR           0x2c
#define MST_ISR           0x30
#define MST_INT_HALTED    BIT(18)
#define MST_INT_MR_DONE   BIT(17)
#define MST_INT_IMM_COMP  BIT(16)
#define MST_INT_TX_THR    BIT(15)
#define MST_INT_TX_OVF    BIT(14)
#define MST_INT_C_REF_ROV BIT(13)
#define MST_INT_IBID_THR  BIT(12)
#define MST_INT_IBID_UNF  BIT(11)
#define MST_INT_IBIR_THR  BIT(10)
#define MST_INT_IBIR_UNF  BIT(9)
#define MST_INT_IBIR_OVF  BIT(8)
#define MST_INT_RX_THR    BIT(7)
#define MST_INT_RX_UNF    BIT(6)
#define MST_INT_CMDD_EMP  BIT(5)
#define MST_INT_CMDD_THR  BIT(4)
#define MST_INT_CMDD_OVF  BIT(3)
#define MST_INT_CMDR_THR  BIT(2)
#define MST_INT_CMDR_UNF  BIT(1)
#define MST_INT_CMDR_OVF  BIT(0)
#define MST_INT_MASK      GENMASK(18, 0)

#define MST_STATUS0             0x34
#define MST_STATUS0_IDLE        BIT(18)
#define MST_STATUS0_HALTED      BIT(17)
#define MST_STATUS0_MASTER_MODE BIT(16)
#define MST_STATUS0_TX_FULL     BIT(13)
#define MST_STATUS0_IBID_FULL   BIT(12)
#define MST_STATUS0_IBIR_FULL   BIT(11)
#define MST_STATUS0_RX_FULL     BIT(10)
#define MST_STATUS0_CMDD_FULL   BIT(9)
#define MST_STATUS0_CMDR_FULL   BIT(8)
#define MST_STATUS0_TX_EMP      BIT(5)
#define MST_STATUS0_IBID_EMP    BIT(4)
#define MST_STATUS0_IBIR_EMP    BIT(3)
#define MST_STATUS0_RX_EMP      BIT(2)
#define MST_STATUS0_CMDD_EMP    BIT(1)
#define MST_STATUS0_CMDR_EMP    BIT(0)

#define CMDR                    0x38
#define CMDR_NO_ERROR           0
#define CMDR_DDR_PREAMBLE_ERROR 1
#define CMDR_DDR_PARITY_ERROR   2
#define CMDR_DDR_RX_FIFO_OVF    3
#define CMDR_DDR_TX_FIFO_UNF    4
#define CMDR_M0_ERROR           5
#define CMDR_M1_ERROR           6
#define CMDR_M2_ERROR           7
#define CMDR_MST_ABORT          8
#define CMDR_NACK_RESP          9
#define CMDR_INVALID_DA         10
#define CMDR_DDR_DROPPED        11
#define CMDR_ERROR(x)           (((x) & GENMASK(27, 24)) >> 24)
#define CMDR_XFER_BYTES(x)      (((x) & GENMASK(19, 8)) >> 8)
#define CMDR_CMDID_HJACK_DISEC  0xfe
#define CMDR_CMDID_HJACK_ENTDAA 0xff
#define CMDR_CMDID(x)           ((x) & GENMASK(7, 0))

#define IBIR               0x3c
#define IBIR_ACKED         BIT(12)
#define IBIR_SLVID(x)      (((x) & GENMASK(11, 8)) >> 8)
#define IBIR_SLVID_INV     0xF
#define IBIR_ERROR         BIT(7)
#define IBIR_XFER_BYTES(x) (((x) & GENMASK(6, 2)) >> 2)
#define IBIR_TYPE_IBI      0
#define IBIR_TYPE_HJ       1
#define IBIR_TYPE_MR       2
#define IBIR_TYPE(x)       ((x) & GENMASK(1, 0))

#define SLV_IER             0x40
#define SLV_IDR             0x44
#define SLV_IMR             0x48
#define SLV_ICR             0x4c
#define SLV_ISR             0x50
#define SLV_INT_CHIP_RST    BIT(31)
#define SLV_INT_PERIPH_RST  BIT(30)
#define SLV_INT_FLUSH_DONE  BIT(29)
#define SLV_INT_RST_DAA     BIT(28)
#define SLV_INT_BUSCON_UP   BIT(26)
#define SLV_INT_MRL_UP      BIT(25)
#define SLV_INT_MWL_UP      BIT(24)
#define SLV_INT_IBI_THR     BIT(23)
#define SLV_INT_IBI_DONE    BIT(22)
#define SLV_INT_DEFSLVS     BIT(21)
#define SLV_INT_TM          BIT(20)
#define SLV_INT_ERROR       BIT(19)
#define SLV_INT_EVENT_UP    BIT(18)
#define SLV_INT_HJ_DONE     BIT(17)
#define SLV_INT_MR_DONE     BIT(16)
#define SLV_INT_DA_UPD      BIT(15)
#define SLV_INT_SDR_FAIL    BIT(14)
#define SLV_INT_DDR_FAIL    BIT(13)
#define SLV_INT_M_RD_ABORT  BIT(12)
#define SLV_INT_DDR_RX_THR  BIT(11)
#define SLV_INT_DDR_TX_THR  BIT(10)
#define SLV_INT_SDR_RX_THR  BIT(9)
#define SLV_INT_SDR_TX_THR  BIT(8)
#define SLV_INT_DDR_RX_UNF  BIT(7)
#define SLV_INT_DDR_TX_OVF  BIT(6)
#define SLV_INT_SDR_RX_UNF  BIT(5)
#define SLV_INT_SDR_TX_OVF  BIT(4)
#define SLV_INT_DDR_RD_COMP BIT(3)
#define SLV_INT_DDR_WR_COMP BIT(2)
#define SLV_INT_SDR_RD_COMP BIT(1)
#define SLV_INT_SDR_WR_COMP BIT(0)

#define SLV_STATUS0                   0x54
#define SLV_STATUS0_IBI_XFRD_BYTEs(s) (((s) & GENMASK(31, 24)) >> 24)
#define SLV_STATUS0_REG_ADDR(s)       (((s) & GENMASK(23, 16)) >> 16)
#define SLV_STATUS0_XFRD_BYTES(s)     ((s) & GENMASK(15, 0))

#define SLV_STATUS1              0x58
#define SLV_STATUS1_SCL_IN_RST   BIT(31)
#define SLV_STATUS1_HJ_IN_USE    BIT(30)
#define SLV_STATUS1_NACK_NXT_PW  BIT(29)
#define SLV_STATUS1_NACK_NXT_PR  BIT(28)
#define SLV_STATUS1_MR_PEND      BIT(27)
#define SLV_STATUS1_HJ_PEND      BIT(26)
#define SLV_STATUS1_IBI_PEND     BIT(25)
#define SLV_STATUS1_IBI_DIS      BIT(24)
#define SLV_STATUS1_BUS_VAR      BIT(23)
#define SLV_STATUS1_TCAM0_DIS    BIT(22)
#define SLV_STATUS1_AS(s)        (((s) & GENMASK(21, 20)) >> 20)
#define SLV_STATUS1_VEN_TM       BIT(19)
#define SLV_STATUS1_HJ_DIS       BIT(18)
#define SLV_STATUS1_MR_DIS       BIT(17)
#define SLV_STATUS1_PROT_ERR     BIT(16)
#define SLV_STATUS1_DA(s)        (((s) & GENMASK(15, 9)) >> 9)
#define SLV_STATUS1_HAS_DA       BIT(8)
#define SLV_STATUS1_DDR_RX_FULL  BIT(7)
#define SLV_STATUS1_DDR_TX_FULL  BIT(6)
#define SLV_STATUS1_DDR_RX_EMPTY BIT(5)
#define SLV_STATUS1_DDR_TX_EMPTY BIT(4)
#define SLV_STATUS1_SDR_RX_FULL  BIT(3)
#define SLV_STATUS1_SDR_TX_FULL  BIT(2)
#define SLV_STATUS1_SDR_RX_EMPTY BIT(1)
#define SLV_STATUS1_SDR_TX_EMPTY BIT(0)

#define SLV_IBI_CTRL               0x5c
#define SLV_IBI_TCAM_EVNT(x)       ((x) << 27)
#define SLV_IBI_PL(x)              ((x) << 16)
#define SLV_IBI_TCAM0              BIT(9)
#define SLV_IBI_REQ                BIT(8)
#define SLV_IBI_AUTO_CLR_IBI       1
#define SLV_IBI_AUTO_CLR_PR        2
#define SLV_IBI_AUTO_CLR_IBI_OR_PR 3
#define SLV_IBI_CLEAR_TRIGGER(x)   ((x) << 4)

#define CMD0_FIFO                   0x60
#define CMD0_FIFO_IS_DDR            BIT(31)
#define CMD0_FIFO_IS_CCC            BIT(30)
#define CMD0_FIFO_BCH               BIT(29)
#define XMIT_BURST_STATIC_SUBADDR   0
#define XMIT_SINGLE_INC_SUBADDR     1
#define XMIT_SINGLE_STATIC_SUBADDR  2
#define XMIT_BURST_WITHOUT_SUBADDR  3
#define CMD0_FIFO_PRIV_XMIT_MODE(m) ((m) << 27)
#define CMD0_FIFO_SBCA              BIT(26)
#define CMD0_FIFO_RSBC              BIT(25)
#define CMD0_FIFO_IS_10B            BIT(24)
#define CMD0_FIFO_PL_LEN(l)         ((l) << 12)
#define CMD0_FIFO_IS_DB             BIT(11)
#define CMD0_FIFO_PL_LEN_MAX        4095
#define CMD0_FIFO_DEV_ADDR(a)       ((a) << 1)
#define CMD0_FIFO_RNW               BIT(0)

#define CMD1_FIFO            0x64
#define CMD1_FIFO_CMDID(id)  ((id) << 24)
#define CMD1_FIFO_DB(db)     FIELD_PREP(GENMASK(15, 8), (db))
#define CMD1_FIFO_CSRADDR(a) (a)
#define CMD1_FIFO_CCC(id)    (id)

#define TX_FIFO 0x68

#define TX_FIFO_STATUS 0x6C

#define IMD_CMD0             0x70
#define IMD_CMD0_PL_LEN(l)   ((l) << 12)
#define IMD_CMD0_DEV_ADDR(a) ((a) << 1)
#define IMD_CMD0_RNW         BIT(0)

#define IMD_CMD1         0x74
#define IMD_CMD1_CCC(id) (id)

#define IMD_DATA                    0x78
#define RX_FIFO                     0x80
#define IBI_DATA_FIFO               0x84
#define SLV_DDR_TX_FIFO             0x88
#define SLV_DDR_RX_FIFO             0x8c
#define DDR_PREAMBLE_MASK           GENMASK(19, 18)
#define DDR_PREAMBLE_CMD_CRC        (0x1 << 18)
#define DDR_PREAMBLE_DATA_ABORT     (0x2 << 18)
#define DDR_PREAMBLE_DATA_ABORT_ALT (0x3 << 18)
#define DDR_DATA(x)                 (((x) & GENMASK(17, 2)) >> 2)
#define DDR_EVEN_PARITY             BIT(0)
#define DDR_ODD_PARITY              BIT(1)
#define DDR_CRC_AND_HEADER_SIZE     0x4
#define DDR_CONVERT_BUF_LEN(x)      (4 * (x))

#define HDR_CMD_RD         BIT(15)
#define HDR_CMD_CODE(c)    (((c) & GENMASK(6, 0)) << 8)
#define DDR_CRC_TOKEN      (0xC << 14)
#define DDR_CRC_TOKEN_MASK GENMASK(17, 14)
#define DDR_CRC(t)         (((t) & (GENMASK(13, 9))) >> 9)
#define DDR_CRC_WR_SETUP   BIT(8)

#define CMD_IBI_THR_CTRL 0x90
#define IBIR_THR(t)      ((t) << 24)
#define CMDR_THR(t)      ((t) << 16)
#define CMDR_THR_MASK    (GENMASK(20, 16))
#define IBI_THR(t)       ((t) << 8)
#define CMD_THR(t)       (t)

#define TX_RX_THR_CTRL 0x94
#define RX_THR(t)      ((t) << 16)
#define RX_THR_MASK    (GENMASK(31, 16))
#define TX_THR(t)      (t)
#define TX_THR_MASK    (GENMASK(15, 0))

#define SLV_DDR_TX_RX_THR_CTRL 0x98
#define SLV_DDR_RX_THR(t)      ((t) << 16)
#define SLV_DDR_TX_THR(t)      (t)

#define FLUSH_CTRL            0x9c
#define FLUSH_IBI_RESP        BIT(24)
#define FLUSH_CMD_RESP        BIT(23)
#define FLUSH_SLV_DDR_RX_FIFO BIT(22)
#define FLUSH_SLV_DDR_TX_FIFO BIT(21)
#define FLUSH_IMM_FIFO        BIT(20)
#define FLUSH_IBI_FIFO        BIT(19)
#define FLUSH_RX_FIFO         BIT(18)
#define FLUSH_TX_FIFO         BIT(17)
#define FLUSH_CMD_FIFO        BIT(16)

#define SLV_CTRL 0xA0

#define SLV_PROT_ERR_TYPE 0xA4
#define SLV_ERR6_IBI      BIT(9)
#define SLV_ERR6_PR       BIT(8)
#define SLV_ERR_GETCCC    BIT(7)
#define SLV_ERR5          BIT(6)
#define SLV_ERR4          BIT(5)
#define SLV_ERR3          BIT(4)
#define SLV_ERR2_PW       BIT(3)
#define SLV_ERR2_SETCCC   BIT(2)
#define SLV_ERR1          BIT(1)
#define SLV_ERR0          BIT(0)

#define SLV_STATUS2        0xA8
#define SLV_STATUS2_MRL(s) (((s) & GENMASK(23, 8)) >> 8)

#define SLV_STATUS3           0xAC
#define SLV_STATUS3_BC_FSM(s) (((s) & GENMASK(26, 16)) >> 16)
#define SLV_STATUS3_MWL(s)    ((s) & GENMASK(15, 0))

#define TTO_PRESCL_CTRL0               0xb0
#define TTO_PRESCL_CTRL0_PRESCL_I2C(x) ((x) << 16)
#define TTO_PRESCL_CTRL0_PRESCL_I3C(x) (x)

#define TTO_PRESCL_CTRL1           0xb4
#define TTO_PRESCL_CTRL1_DIVB(x)   ((x) << 16)
#define TTO_PRESCL_CTRL1_DIVA(x)   (x)
#define TTO_PRESCL_CTRL1_PP_LOW(x) ((x) << 8)
#define TTO_PRESCL_CTRL1_OD_LOW(x) (x)

#define DEVS_CTRL                  0xb8
#define DEVS_CTRL_DEV_CLR_SHIFT    16
#define DEVS_CTRL_DEV_CLR_ALL      GENMASK(31, 16)
#define DEVS_CTRL_DEV_CLR(dev)     BIT(16 + (dev))
#define DEVS_CTRL_DEV_ACTIVE(dev)  BIT(dev)
#define DEVS_CTRL_DEVS_ACTIVE_MASK GENMASK(15, 0)
#define MAX_DEVS                   16

#define DEV_ID_RR0(d)              (0xc0 + ((d) * 0x10))
#define DEV_ID_RR0_LVR_EXT_ADDR    BIT(11)
#define DEV_ID_RR0_HDR_CAP         BIT(10)
#define DEV_ID_RR0_IS_I3C          BIT(9)
#define DEV_ID_RR0_DEV_ADDR_MASK   (GENMASK(7, 1) | GENMASK(15, 13))
#define DEV_ID_RR0_SET_DEV_ADDR(a) (((a << 1) & GENMASK(7, 1)) | (((a) & GENMASK(9, 7)) << 13))
#define DEV_ID_RR0_GET_DEV_ADDR(x) ((((x) >> 1) & GENMASK(6, 0)) | (((x) >> 6) & GENMASK(9, 7)))

#define DEV_ID_RR1(d)           (0xc4 + ((d) * 0x10))
#define DEV_ID_RR1_PID_MSB(pid) (pid)

#define DEV_ID_RR2(d)           (0xc8 + ((d) * 0x10))
#define DEV_ID_RR2_PID_LSB(pid) ((pid) << 16)
#define DEV_ID_RR2_BCR(bcr)     ((bcr) << 8)
#define DEV_ID_RR2_DCR(dcr)     (dcr)
#define DEV_ID_RR2_LVR(lvr)     (lvr)

#define SIR_MAP(x)               (0x180 + ((x) * 4))
#define SIR_MAP_DEV_REG(d)       SIR_MAP((d) / 2)
#define SIR_MAP_DEV_SHIFT(d, fs) ((fs) + (((d) % 2) ? 16 : 0))
#define SIR_MAP_DEV_CONF_MASK(d) (GENMASK(15, 0) << (((d) % 2) ? 16 : 0))
#define SIR_MAP_DEV_CONF(d, c)   ((c) << (((d) % 2) ? 16 : 0))
#define DEV_ROLE_SLAVE           0
#define DEV_ROLE_MASTER          1
#define SIR_MAP_DEV_ROLE(role)   ((role) << 14)
#define SIR_MAP_DEV_SLOW         BIT(13)
#define SIR_MAP_DEV_PL(l)        ((l) << 8)
#define SIR_MAP_PL_MAX           GENMASK(4, 0)
#define SIR_MAP_DEV_DA(a)        ((a) << 1)
#define SIR_MAP_DEV_ACK          BIT(0)

#define GRPADDR_LIST 0x198

#define GRPADDR_CS 0x19C

#define GPIR_WORD(x)     (0x200 + ((x) * 4))
#define GPI_REG(val, id) (((val) >> (((id) % 4) * 8)) & GENMASK(7, 0))

#define GPOR_WORD(x)     (0x220 + ((x) * 4))
#define GPO_REG(val, id) (((val) >> (((id) % 4) * 8)) & GENMASK(7, 0))

#define ASF_INT_STATUS        0x300
#define ASF_INT_RAW_STATUS    0x304
#define ASF_INT_MASK          0x308
#define ASF_INT_TEST          0x30c
#define ASF_INT_FATAL_SELECT  0x310
#define ASF_INTEGRITY_ERR     BIT(6)
#define ASF_PROTOCOL_ERR      BIT(5)
#define ASF_TRANS_TIMEOUT_ERR BIT(4)
#define ASF_CSR_ERR           BIT(3)
#define ASF_DAP_ERR           BIT(2)
#define ASF_SRAM_UNCORR_ERR   BIT(1)
#define ASF_SRAM_CORR_ERR     BIT(0)

#define ASF_SRAM_CORR_FAULT_STATUS      0x320
#define ASF_SRAM_UNCORR_FAULT_STATUS    0x324
#define ASF_SRAM_CORR_FAULT_INSTANCE(x) ((x) >> 24)
#define ASF_SRAM_CORR_FAULT_ADDR(x)     ((x) & GENMASK(23, 0))

#define ASF_SRAM_FAULT_STATS           0x328
#define ASF_SRAM_FAULT_UNCORR_STATS(x) ((x) >> 16)
#define ASF_SRAM_FAULT_CORR_STATS(x)   ((x) & GENMASK(15, 0))

#define ASF_TRANS_TOUT_CTRL   0x330
#define ASF_TRANS_TOUT_EN     BIT(31)
#define ASF_TRANS_TOUT_VAL(x) (x)

#define ASF_TRANS_TOUT_FAULT_MASK      0x334
#define ASF_TRANS_TOUT_FAULT_STATUS    0x338
#define ASF_TRANS_TOUT_FAULT_APB       BIT(3)
#define ASF_TRANS_TOUT_FAULT_SCL_LOW   BIT(2)
#define ASF_TRANS_TOUT_FAULT_SCL_HIGH  BIT(1)
#define ASF_TRANS_TOUT_FAULT_FSCL_HIGH BIT(0)

#define ASF_PROTO_FAULT_MASK            0x340
#define ASF_PROTO_FAULT_STATUS          0x344
#define ASF_PROTO_FAULT_SLVSDR_RD_ABORT BIT(31)
#define ASF_PROTO_FAULT_SLVDDR_FAIL     BIT(30)
#define ASF_PROTO_FAULT_S(x)            BIT(16 + (x))
#define ASF_PROTO_FAULT_MSTSDR_RD_ABORT BIT(15)
#define ASF_PROTO_FAULT_MSTDDR_FAIL     BIT(14)
#define ASF_PROTO_FAULT_M(x)            BIT(x)

/*******************************************************************************
 * Local Constants Definition
 ******************************************************************************/

/* Maximum i3c devices that the IP can be built with */
#define I3C_MAX_DEVS                     11
#define I3C_MAX_MSGS                     10
#define I3C_SIR_DEFAULT_DA               0x7F
#define I3C_MAX_IDLE_CANCEL_WAIT_RETRIES 50
#define I3C_PRESCL_REG_SCALE             (4)
#define I2C_PRESCL_REG_SCALE             (5)
#define I3C_WAIT_FOR_IDLE_STATE_US       CONFIG_I3C_CADENCE_IDLE_TIMEOUT_US
#define I3C_IDLE_TIMEOUT_CYC                                                                       \
	(I3C_WAIT_FOR_IDLE_STATE_US * (sys_clock_hw_cycles_per_sec() / USEC_PER_SEC))

/*
 * MIPI I3C v1.1.1 Spec defines SDA Signal Data Hold in Push Pull max as the
 * minimum of the clock rise and fall time plus 3ns
 */
#define I3C_HD_PP_DEFAULT_NS 10

/* Interrupt thresholds. */
/* command response fifo threshold */
#define I3C_CMDR_THR 1
/* command tx fifo threshold - unused */
#define I3C_CMDD_THR 1
/* in-band-interrupt response queue threshold */
#define I3C_IBIR_THR 1
/* tx data threshold - unused */
#define I3C_TX_THR   1

/*******************************************************************************
 * Local Types Definition
 ******************************************************************************/

/** Describes peripheral HW configuration determined from CONFx registers. */
struct cdns_i3c_hw_config {
	/* Revision ID */
	uint32_t rev_id;
	/* The maximum command queue depth. */
	uint32_t cmd_mem_depth;
	/* The maximum command response queue depth. */
	uint32_t cmdr_mem_depth;
	/* The maximum RX FIFO depth. */
	uint32_t rx_mem_depth;
	/* The maximum TX FIFO depth. */
	uint32_t tx_mem_depth;
	/* The maximum DDR RX FIFO depth. */
	uint32_t ddr_rx_mem_depth;
	/* The maximum DDR TX FIFO depth. */
	uint32_t ddr_tx_mem_depth;
	/* The maximum IBIR FIFO depth. */
	uint32_t ibir_mem_depth;
	/* The maximum IBI FIFO depth. */
	uint32_t ibi_mem_depth;
};

/* Cadence I3C/I2C Device Private Data */
struct cdns_i3c_i2c_dev_data {
	/* Device id within the retaining registers. This is set after bus initialization by the
	 * controller.
	 */
	uint8_t id;
};

/* Single command/transfer */
struct cdns_i3c_cmd {
	uint32_t cmd0;
	uint32_t cmd1;
	uint32_t ddr_header;
	uint32_t ddr_crc;
	uint32_t len;
	uint32_t *num_xfer;
	void *buf;
	uint32_t error;
	enum i3c_sdr_controller_error_types *sdr_err;
	enum i3c_data_rate hdr;
};

/* Transfer data */
struct cdns_i3c_xfer {
	struct k_sem complete;
	int ret;
	int num_cmds;
	struct cdns_i3c_cmd cmds[I3C_MAX_MSGS];
};

#ifdef CONFIG_I3C_USE_IBI
/* IBI transferred data */
struct cdns_i3c_ibi_buf {
	uint8_t ibi_data[CONFIG_I3C_IBI_MAX_PAYLOAD_SIZE];
	uint8_t ibi_data_cnt;
};
#endif

/* Driver config */
struct cdns_i3c_config {
	struct i3c_driver_config common;
	/** base address of the controller */
	uintptr_t base;
	/** input frequency to the I3C Cadence */
	uint32_t input_frequency;
	/** Interrupt configuration function. */
	void (*irq_config_func)(const struct device *dev);
	/** IBID Threshold value */
	uint8_t ibid_thr;
};

#ifdef CONFIG_I3C_CADENCE_RTIO
#include <zephyr/drivers/i2c/rtio.h>
#include <zephyr/drivers/i3c/rtio.h>
#include <zephyr/sys/mpsc_lockfree.h>

/*
 * Per-sqe record kept while a transaction is in flight so that the CMDR
 * drain ISR can route response bytes / errors back to the originating sqe.
 *
 * sqe is the originating rtio_iodev_sqe; for direct CCCs (one CMD per
 * target) ccc_target_idx points back at payload->targets.payloads[N]; for
 * data sqes it is unused.
 *
 * For DDR cmds (is_ddr=true) the pre-built header word and CRC token are
 * stashed during start so streaming TX/RX can consume them without
 * recomputation. ddr_crc carries a running CRC5 used by the RX decoder.
 */
struct cdns_i3c_rtio_cmd {
	struct rtio_iodev_sqe *sqe;
	uint16_t len;
	bool is_read;
	bool is_ccc_target;
	bool is_ddr;
	uint8_t ccc_target_idx;
	uint32_t ddr_header;
	uint32_t ddr_crc_word;
	uint8_t ddr_crc;
};

/*
 * DDR TX streaming phase. One per active TX cursor — DDR cmds emit
 * header → data... → crc → done across multiple TX_THR IRQs.
 */
enum cdns_i3c_rtio_ddr_tx_phase {
	DDR_TX_HEADER = 0,
	DDR_TX_DATA,
	DDR_TX_CRC,
	DDR_TX_DONE,
};

/*
 * Single per-controller RTIO state. Both i3c and i2c sqes flow through one
 * unified mpsc queue (unified_q), preserving submit-time ordering. The
 * active transaction's "kind" is implicit in active_head->sqe.iodev->api.
 *
 * i3c_ctx and i2c_ctx are retained only so the blocking helpers
 * (i3c_rtio_transfer, i2c_rtio_transfer) work — their ctx->iodev field is
 * the bridge into our iodev_submit callbacks. The ctxes' own io_q /
 * txn_head / txn_curr fields go unused on the cdns path; unified_q is the
 * sole queue we drive from.
 *
 * active_head non-NULL means "controller is busy driving the bus right
 * now". Submit-side checks this under slock to decide whether to kick
 * start or defer to the in-flight ISR's completion-tail.
 */
struct cdns_i3c_rtio_state {
	struct i3c_rtio *i3c_ctx;
	struct i2c_rtio *i2c_ctx;

	struct k_spinlock slock;
	struct mpsc unified_q;
	struct rtio_iodev_sqe *active_head;

	struct rtio_iodev_sqe *tx_sqe;
	uint32_t tx_off;

	struct rtio_iodev_sqe *rx_sqe;
	uint32_t rx_off;

	/*
	 * Index into cmds[] tracking which entry the TX/RX cursor is on.
	 * For DDR streaming we need to look up per-cmd state (header, CRC,
	 * phase) without scanning. Non-DDR (SDR/CCC) doesn't strictly need
	 * these but we update them anyway for symmetry.
	 */
	uint8_t tx_cmd_idx;
	uint8_t rx_cmd_idx;

	enum cdns_i3c_rtio_ddr_tx_phase ddr_tx_phase;

	uint8_t num_cmds;
	int xfer_status;

	struct cdns_i3c_rtio_cmd cmds[I3C_MAX_MSGS];
};
#endif /* CONFIG_I3C_CADENCE_RTIO */

/* Driver instance data */
struct cdns_i3c_data {
	/* common must be first! */
	struct i3c_driver_data common;
	struct cdns_i3c_hw_config hw_cfg;
	struct k_mutex bus_lock;
#ifdef CONFIG_I3C_CONTROLLER
	struct cdns_i3c_i2c_dev_data cdns_i3c_i2c_priv_data[I3C_MAX_DEVS];
#if defined(CONFIG_I3C_CALLBACK) || defined(CONFIG_I2C_CALLBACK)
	struct k_timer timeout;
	i3c_callback_t cb;
	void *userdata;
	struct k_sem async_sem;
#endif
#endif /* CONFIG_I3C_CONTROLLER */
	struct cdns_i3c_xfer xfer;
#ifdef CONFIG_I3C_TARGET
	struct i3c_target_config *target_config;
#ifdef CONFIG_I3C_TARGET_BUFFER_MODE
	uint8_t target_rx_buf[CONFIG_I3C_CADENCE_TARGET_RX_BUF_SIZE];
	uint32_t target_rx_len;
#endif /* CONFIG_I3C_TARGET_BUFFER_MODE */
#endif /* CONFIG_I3C_TARGET */
#ifdef CONFIG_I3C_USE_IBI
	struct cdns_i3c_ibi_buf ibi_buf;
#ifdef CONFIG_I3C_TARGET
	struct k_sem ibi_hj_complete;
#ifdef CONFIG_I3C_CONTROLLER
	struct k_sem ibi_cr_complete;
#endif /* CONFIG_I3C_CONTROLLER */
#endif /* CONFIG_I3C_TARGET */
#endif /* CONFIG_I3C_USE_IBI*/
#if (defined(CONFIG_I3C_CONTROLLER) && defined(CONFIG_I3C_TARGET)) ||                              \
	defined(CONFIG_I3C_CALLBACK) || defined(CONFIG_I2C_CALLBACK)
	const struct device *dev;
#endif
#if defined(CONFIG_I3C_CONTROLLER) && defined(CONFIG_I3C_TARGET)
	struct k_work deftgts_work;
	struct k_sem ch_complete;
#endif /* CONFIG_I3C_CONTROLLER && CONFIG_I3C_TARGET */
	uint32_t free_rr_slots;
	uint16_t fifo_bytes_read;
	uint8_t max_devs;
#ifdef CONFIG_I3C_CADENCE_RTIO
	struct cdns_i3c_rtio_state *rtio;
#endif
};

/*******************************************************************************
 * FIFO Status Helpers (inline)
 ******************************************************************************/

/* Check if command response fifo is empty */
static inline bool cdns_i3c_cmd_rsp_fifo_empty(const struct cdns_i3c_config *config)
{
	uint32_t mst_st = sys_read32(config->base + MST_STATUS0);

	return ((mst_st & MST_STATUS0_CMDR_EMP) ? true : false);
}

/* Check if command fifo is empty */
static inline bool cdns_i3c_cmd_fifo_empty(const struct cdns_i3c_config *config)
{
	uint32_t mst_st = sys_read32(config->base + MST_STATUS0);

	return ((mst_st & MST_STATUS0_CMDD_EMP) ? true : false);
}

/* Check if command fifo is full */
static inline bool cdns_i3c_cmd_fifo_full(const struct cdns_i3c_config *config)
{
	uint32_t mst_st = sys_read32(config->base + MST_STATUS0);

	return ((mst_st & MST_STATUS0_CMDD_FULL) ? true : false);
}

/* Check if ibi response fifo is empty */
static inline bool cdns_i3c_ibi_rsp_fifo_empty(const struct cdns_i3c_config *config)
{
	uint32_t mst_st = sys_read32(config->base + MST_STATUS0);

	return ((mst_st & MST_STATUS0_IBIR_EMP) ? true : false);
}

/* Check if tx fifo is full */
static inline bool cdns_i3c_tx_fifo_full(const struct cdns_i3c_config *config)
{
	uint32_t mst_st = sys_read32(config->base + MST_STATUS0);

	return ((mst_st & MST_STATUS0_TX_FULL) ? true : false);
}

/* Check if rx fifo is full */
static inline bool cdns_i3c_rx_fifo_full(const struct cdns_i3c_config *config)
{
	uint32_t mst_st = sys_read32(config->base + MST_STATUS0);

	return ((mst_st & MST_STATUS0_RX_FULL) ? true : false);
}

/* Check if rx fifo is empty */
static inline bool cdns_i3c_rx_fifo_empty(const struct cdns_i3c_config *config)
{
	uint32_t mst_st = sys_read32(config->base + MST_STATUS0);

	return ((mst_st & MST_STATUS0_RX_EMP) ? true : false);
}

/* Check if ibi fifo is empty */
static inline bool cdns_i3c_ibi_fifo_empty(const struct cdns_i3c_config *config)
{
	uint32_t mst_st = sys_read32(config->base + MST_STATUS0);

	return ((mst_st & MST_STATUS0_IBID_EMP) ? true : false);
}

/* Interrupt handling */
static inline void cdns_i3c_interrupts_disable(const struct cdns_i3c_config *config)
{
	sys_write32(MST_INT_MASK, config->base + MST_IDR);
}

static inline void cdns_i3c_interrupts_clear(const struct cdns_i3c_config *config)
{
	sys_write32(MST_INT_MASK, config->base + MST_ICR);
}

/*******************************************************************************
 * Shared helpers (defined in i3c_cdns_common.c)
 ******************************************************************************/

uint8_t i3c_cdns_crc5(uint8_t crc5, uint16_t word);
uint8_t cdns_i3c_ddr_parity(uint16_t payload);
uint32_t prepare_ddr_word(uint16_t payload);
uint16_t prepare_ddr_cmd_parity_adjustment_bit(uint16_t word);
#ifdef CONFIG_I3C_CONTROLLER
uint8_t cdns_i3c_even_parity_byte(uint8_t byte);
#endif

void cdns_i3c_write_tx_fifo(const struct cdns_i3c_config *config, const void *buf, uint32_t len);
#ifdef CONFIG_I3C_CONTROLLER
int cdns_i3c_read_rx_fifo(const struct cdns_i3c_config *config, void *buf, uint32_t len);
int cdns_i3c_wait_for_idle(const struct device *dev);
#endif

/*******************************************************************************
 * RTIO native submit hooks (defined in i3c_cdns_rtio.c)
 ******************************************************************************/

#ifdef CONFIG_I3C_CADENCE_RTIO
struct rtio_iodev_sqe;

void cdns_i3c_iodev_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);
void cdns_i3c_i2c_iodev_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);
int cdns_i3c_rtio_init(const struct device *dev);
void cdns_i3c_rtio_irq_cmdd_emp(const struct device *dev);
void cdns_i3c_rtio_irq_tx_thr(const struct device *dev);
void cdns_i3c_rtio_irq_rx_thr(const struct device *dev);
#endif /* CONFIG_I3C_CADENCE_RTIO */

/*******************************************************************************
 * Legacy hardware-driving path (defined in i3c_cdns_legacy.c)
 *
 * These funnel through cdns_i3c_start_transfer / cdns_i3c_complete_transfer
 * and back-pressure to the legacy data->xfer.cmds[] queue. Only compiled and
 * referenced when CONFIG_I3C_CADENCE_RTIO is NOT enabled.
 ******************************************************************************/

#ifndef CONFIG_I3C_CADENCE_RTIO

void cdns_i3c_complete_transfer(const struct device *dev);

#if defined(CONFIG_I3C_CALLBACK) || defined(CONFIG_I2C_CALLBACK)
void cdns_i3c_cb_timeout(struct k_timer *timer);
#endif

int cdns_i3c_do_ccc_do(const struct device *dev, struct i3c_ccc_payload *payload, bool async,
		       i3c_callback_t cb, void *userdata);
int cdns_i3c_transfer_do(const struct device *dev, struct i3c_device_desc *target,
			 struct i3c_msg *msgs, uint8_t num_msgs, bool async, i3c_callback_t cb,
			 void *userdata);
int cdns_i3c_i2c_transfer(const struct device *dev, struct i3c_i2c_device_desc *i2c_dev,
			  struct i2c_msg *msgs, uint8_t num_msgs);
#ifdef CONFIG_I2C_CALLBACK
int cdns_i3c_i2c_transfer_cb(const struct device *dev, struct i3c_i2c_device_desc *i2c_dev,
			     struct i2c_msg *msgs, uint8_t num_msgs, i2c_callback_t cb,
			     void *userdata);
#endif
struct i3c_i2c_device_desc *cdns_i3c_i2c_device_find(const struct device *dev, uint16_t addr);

#endif /* !CONFIG_I3C_CADENCE_RTIO */

#endif /* ZEPHYR_DRIVERS_I3C_I3C_CDNS_H_ */
