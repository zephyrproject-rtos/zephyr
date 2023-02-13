/*
 * Copyright (c) 2022 Meta Platforms, Inc. and its affiliates.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/drivers/i3c.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/sys_io.h>

#define DEV_ID		  0x0
#define DEV_ID_I3C_MASTER 0x5034

#define CONF_STATUS0		     0x4
#define CONF_STATUS0_CMDR_DEPTH(x)   (4 << (((x)&GENMASK(31, 29)) >> 29))
#define CONF_STATUS0_ECC_CHK	     BIT(28)
#define CONF_STATUS0_INTEG_CHK	     BIT(27)
#define CONF_STATUS0_CSR_DAP_CHK     BIT(26)
#define CONF_STATUS0_TRANS_TOUT_CHK  BIT(25)
#define CONF_STATUS0_PROT_FAULTS_CHK BIT(24)
#define CONF_STATUS0_GPO_NUM(x)	     (((x)&GENMASK(23, 16)) >> 16)
#define CONF_STATUS0_GPI_NUM(x)	     (((x)&GENMASK(15, 8)) >> 8)
#define CONF_STATUS0_IBIR_DEPTH(x)   (4 << (((x)&GENMASK(7, 6)) >> 7))
#define CONF_STATUS0_SUPPORTS_DDR    BIT(5)
#define CONF_STATUS0_SEC_MASTER	     BIT(4)
#define CONF_STATUS0_DEVS_NUM(x)     ((x)&GENMASK(3, 0))

#define CONF_STATUS1			0x8
#define CONF_STATUS1_IBI_HW_RES(x)	((((x)&GENMASK(31, 28)) >> 28) + 1)
#define CONF_STATUS1_CMD_DEPTH(x)	(4 << (((x)&GENMASK(27, 26)) >> 26))
#define CONF_STATUS1_SLVDDR_RX_DEPTH(x) (8 << (((x)&GENMASK(25, 21)) >> 21))
#define CONF_STATUS1_SLVDDR_TX_DEPTH(x) (8 << (((x)&GENMASK(20, 16)) >> 16))
#define CONF_STATUS1_IBI_DEPTH(x)	(2 << (((x)&GENMASK(12, 10)) >> 10))
#define CONF_STATUS1_RX_DEPTH(x)	(8 << (((x)&GENMASK(9, 5)) >> 5))
#define CONF_STATUS1_TX_DEPTH(x)	(8 << ((x)&GENMASK(4, 0)))

#define REV_ID		     0xc
#define REV_ID_VID(id)	     (((id)&GENMASK(31, 20)) >> 20)
#define REV_ID_PID(id)	     (((id)&GENMASK(19, 8)) >> 8)
#define REV_ID_REV(id)	     ((id)&GENMASK(7, 0))
#define REV_ID_VERSION(m, n) ((m << 5) | (n))
#define REV_ID_REV_MAJOR(id) (((id)&GENMASK(7, 5)) >> 5)
#define REV_ID_REV_MINOR(id) ((id)&GENMASK(4, 0))

#define CTRL			 0x10
#define CTRL_DEV_EN		 BIT(31)
#define CTRL_HALT_EN		 BIT(30)
#define CTRL_MCS		 BIT(29)
#define CTRL_MCS_EN		 BIT(28)
#define CTRL_I3C_11_SUPP	 BIT(26)
#define CTRL_THD_DELAY(x)	 (((x) << 24) & GENMASK(25, 24))
#define CTRL_HJ_DISEC		 BIT(8)
#define CTRL_MST_ACK		 BIT(7)
#define CTRL_HJ_ACK		 BIT(6)
#define CTRL_HJ_INIT		 BIT(5)
#define CTRL_MST_INIT		 BIT(4)
#define CTRL_AHDR_OPT		 BIT(3)
#define CTRL_PURE_BUS_MODE	 0
#define CTRL_MIXED_FAST_BUS_MODE 2
#define CTRL_MIXED_SLOW_BUS_MODE 3
#define CTRL_BUS_MODE_MASK	 GENMASK(1, 0)
#define THD_DELAY_MAX		 3

#define PRESCL_CTRL0	    0x14
#define PRESCL_CTRL0_I2C(x) ((x) << 16)
#define PRESCL_CTRL0_I3C(x) (x)
#define PRESCL_CTRL0_MAX    GENMASK(9, 0)

#define PRESCL_CTRL1		 0x18
#define PRESCL_CTRL1_PP_LOW_MASK GENMASK(15, 8)
#define PRESCL_CTRL1_PP_LOW(x)	 ((x) << 8)
#define PRESCL_CTRL1_OD_LOW_MASK GENMASK(7, 0)
#define PRESCL_CTRL1_OD_LOW(x)	 (x)

#define MST_IER		 0x20
#define MST_IDR		 0x24
#define MST_IMR		 0x28
#define MST_ICR		 0x2c
#define MST_ISR		 0x30
#define MST_INT_HALTED	 BIT(18)
#define MST_INT_MR_DONE	 BIT(17)
#define MST_INT_IMM_COMP BIT(16)
#define MST_INT_TX_THR	 BIT(15)
#define MST_INT_TX_OVF	 BIT(14)
#define MST_INT_IBID_THR BIT(12)
#define MST_INT_IBID_UNF BIT(11)
#define MST_INT_IBIR_THR BIT(10)
#define MST_INT_IBIR_UNF BIT(9)
#define MST_INT_IBIR_OVF BIT(8)
#define MST_INT_RX_THR	 BIT(7)
#define MST_INT_RX_UNF	 BIT(6)
#define MST_INT_CMDD_EMP BIT(5)
#define MST_INT_CMDD_THR BIT(4)
#define MST_INT_CMDD_OVF BIT(3)
#define MST_INT_CMDR_THR BIT(2)
#define MST_INT_CMDR_UNF BIT(1)
#define MST_INT_CMDR_OVF BIT(0)
#define MST_INT_MASK	 GENMASK(18, 0)

#define MST_STATUS0		0x34
#define MST_STATUS0_IDLE	BIT(18)
#define MST_STATUS0_HALTED	BIT(17)
#define MST_STATUS0_MASTER_MODE BIT(16)
#define MST_STATUS0_TX_FULL	BIT(13)
#define MST_STATUS0_IBID_FULL	BIT(12)
#define MST_STATUS0_IBIR_FULL	BIT(11)
#define MST_STATUS0_RX_FULL	BIT(10)
#define MST_STATUS0_CMDD_FULL	BIT(9)
#define MST_STATUS0_CMDR_FULL	BIT(8)
#define MST_STATUS0_TX_EMP	BIT(5)
#define MST_STATUS0_IBID_EMP	BIT(4)
#define MST_STATUS0_IBIR_EMP	BIT(3)
#define MST_STATUS0_RX_EMP	BIT(2)
#define MST_STATUS0_CMDD_EMP	BIT(1)
#define MST_STATUS0_CMDR_EMP	BIT(0)

#define CMDR			0x38
#define CMDR_NO_ERROR		0
#define CMDR_DDR_PREAMBLE_ERROR 1
#define CMDR_DDR_PARITY_ERROR	2
#define CMDR_DDR_RX_FIFO_OVF	3
#define CMDR_DDR_TX_FIFO_UNF	4
#define CMDR_M0_ERROR		5
#define CMDR_M1_ERROR		6
#define CMDR_M2_ERROR		7
#define CMDR_MST_ABORT		8
#define CMDR_NACK_RESP		9
#define CMDR_INVALID_DA		10
#define CMDR_DDR_DROPPED	11
#define CMDR_ERROR(x)		(((x)&GENMASK(27, 24)) >> 24)
#define CMDR_XFER_BYTES(x)	(((x)&GENMASK(19, 8)) >> 8)
#define CMDR_CMDID_HJACK_DISEC	0xfe
#define CMDR_CMDID_HJACK_ENTDAA 0xff
#define CMDR_CMDID(x)		((x)&GENMASK(7, 0))

#define IBIR		   0x3c
#define IBIR_ACKED	   BIT(12)
#define IBIR_SLVID(x)	   (((x)&GENMASK(11, 8)) >> 8)
#define IBIR_SLVID_INV	   0xF
#define IBIR_ERROR	   BIT(7)
#define IBIR_XFER_BYTES(x) (((x)&GENMASK(6, 2)) >> 2)
#define IBIR_TYPE_IBI	   0
#define IBIR_TYPE_HJ	   1
#define IBIR_TYPE_MR	   2
#define IBIR_TYPE(x)	   ((x)&GENMASK(1, 0))

#define SLV_IER		    0x40
#define SLV_IDR		    0x44
#define SLV_IMR		    0x48
#define SLV_ICR		    0x4c
#define SLV_ISR		    0x50
#define SLV_INT_DEFSLVS	    BIT(21)
#define SLV_INT_TM	    BIT(20)
#define SLV_INT_ERROR	    BIT(19)
#define SLV_INT_EVENT_UP    BIT(18)
#define SLV_INT_HJ_DONE	    BIT(17)
#define SLV_INT_MR_DONE	    BIT(16)
#define SLV_INT_DA_UPD	    BIT(15)
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
#define SLV_INT_MASK	    GENMASK(20, 0)

#define SLV_STATUS0		  0x54
#define SLV_STATUS0_REG_ADDR(s)	  (((s)&GENMASK(23, 16)) >> 16)
#define SLV_STATUS0_XFRD_BYTES(s) ((s)&GENMASK(15, 0))

#define SLV_STATUS1		 0x58
#define SLV_STATUS1_AS(s)	 (((s)&GENMASK(21, 20)) >> 20)
#define SLV_STATUS1_VEN_TM	 BIT(19)
#define SLV_STATUS1_HJ_DIS	 BIT(18)
#define SLV_STATUS1_MR_DIS	 BIT(17)
#define SLV_STATUS1_PROT_ERR	 BIT(16)
#define SLV_STATUS1_DA(s)	 (((s)&GENMASK(15, 9)) >> 9)
#define SLV_STATUS1_HAS_DA	 BIT(8)
#define SLV_STATUS1_DDR_RX_FULL	 BIT(7)
#define SLV_STATUS1_DDR_TX_FULL	 BIT(6)
#define SLV_STATUS1_DDR_RX_EMPTY BIT(5)
#define SLV_STATUS1_DDR_TX_EMPTY BIT(4)
#define SLV_STATUS1_SDR_RX_FULL	 BIT(3)
#define SLV_STATUS1_SDR_TX_FULL	 BIT(2)
#define SLV_STATUS1_SDR_RX_EMPTY BIT(1)
#define SLV_STATUS1_SDR_TX_EMPTY BIT(0)

#define CMD0_FIFO		    0x60
#define CMD0_FIFO_IS_DDR	    BIT(31)
#define CMD0_FIFO_IS_CCC	    BIT(30)
#define CMD0_FIFO_BCH		    BIT(29)
#define XMIT_BURST_STATIC_SUBADDR   0
#define XMIT_SINGLE_INC_SUBADDR	    1
#define XMIT_SINGLE_STATIC_SUBADDR  2
#define XMIT_BURST_WITHOUT_SUBADDR  3
#define CMD0_FIFO_PRIV_XMIT_MODE(m) ((m) << 27)
#define CMD0_FIFO_SBCA		    BIT(26)
#define CMD0_FIFO_RSBC		    BIT(25)
#define CMD0_FIFO_IS_10B	    BIT(24)
#define CMD0_FIFO_PL_LEN(l)	    ((l) << 12)
#define CMD0_FIFO_PL_LEN_MAX	    4095
#define CMD0_FIFO_DEV_ADDR(a)	    ((a) << 1)
#define CMD0_FIFO_RNW		    BIT(0)

#define CMD1_FIFO	     0x64
#define CMD1_FIFO_CMDID(id)  ((id) << 24)
#define CMD1_FIFO_CSRADDR(a) (a)
#define CMD1_FIFO_CCC(id)    (id)

#define TX_FIFO 0x68

#define IMD_CMD0	     0x70
#define IMD_CMD0_PL_LEN(l)   ((l) << 12)
#define IMD_CMD0_DEV_ADDR(a) ((a) << 1)
#define IMD_CMD0_RNW	     BIT(0)

#define IMD_CMD1	 0x74
#define IMD_CMD1_CCC(id) (id)

#define IMD_DATA	0x78
#define RX_FIFO		0x80
#define IBI_DATA_FIFO	0x84
#define SLV_DDR_TX_FIFO 0x88
#define SLV_DDR_RX_FIFO 0x8c

#define CMD_IBI_THR_CTRL 0x90
#define IBIR_THR(t)	 ((t) << 24)
#define CMDR_THR(t)	 ((t) << 16)
#define CMDR_THR_MASK	 (GENMASK(20, 16))
#define IBI_THR(t)	 ((t) << 8)
#define CMD_THR(t)	 (t)

#define TX_RX_THR_CTRL 0x94
#define RX_THR(t)      ((t) << 16)
#define RX_THR_MASK    (GENMASK(31, 16))
#define TX_THR(t)      (t)
#define TX_THR_MASK    (GENMASK(15, 0))

#define SLV_DDR_TX_RX_THR_CTRL 0x98
#define SLV_DDR_RX_THR(t)      ((t) << 16)
#define SLV_DDR_TX_THR(t)      (t)

#define FLUSH_CTRL	      0x9c
#define FLUSH_IBI_RESP	      BIT(23)
#define FLUSH_CMD_RESP	      BIT(22)
#define FLUSH_SLV_DDR_RX_FIFO BIT(22)
#define FLUSH_SLV_DDR_TX_FIFO BIT(21)
#define FLUSH_IMM_FIFO	      BIT(20)
#define FLUSH_IBI_FIFO	      BIT(19)
#define FLUSH_RX_FIFO	      BIT(18)
#define FLUSH_TX_FIFO	      BIT(17)
#define FLUSH_CMD_FIFO	      BIT(16)

#define TTO_PRESCL_CTRL0	       0xb0
#define TTO_PRESCL_CTRL0_PRESCL_I2C(x) ((x) << 16)
#define TTO_PRESCL_CTRL0_PRESCL_I3C(x) (x)

#define TTO_PRESCL_CTRL1	   0xb4
#define TTO_PRESCL_CTRL1_DIVB(x)   ((x) << 16)
#define TTO_PRESCL_CTRL1_DIVA(x)   (x)
#define TTO_PRESCL_CTRL1_PP_LOW(x) ((x) << 8)
#define TTO_PRESCL_CTRL1_OD_LOW(x) (x)

#define DEVS_CTRL		   0xb8
#define DEVS_CTRL_DEV_CLR_SHIFT	   16
#define DEVS_CTRL_DEV_CLR_ALL	   GENMASK(31, 16)
#define DEVS_CTRL_DEV_CLR(dev)	   BIT(16 + (dev))
#define DEVS_CTRL_DEV_ACTIVE(dev)  BIT(dev)
#define DEVS_CTRL_DEVS_ACTIVE_MASK GENMASK(15, 0)
#define MAX_DEVS		   16

#define DEV_ID_RR0(d)		   (0xc0 + ((d)*0x10))
#define DEV_ID_RR0_LVR_EXT_ADDR	   BIT(11)
#define DEV_ID_RR0_HDR_CAP	   BIT(10)
#define DEV_ID_RR0_IS_I3C	   BIT(9)
#define DEV_ID_RR0_DEV_ADDR_MASK   (GENMASK(6, 0) | GENMASK(15, 13))
#define DEV_ID_RR0_SET_DEV_ADDR(a) (((a)&GENMASK(6, 0)) | (((a)&GENMASK(9, 7)) << 6))
#define DEV_ID_RR0_GET_DEV_ADDR(x) ((((x) >> 1) & GENMASK(6, 0)) | (((x) >> 6) & GENMASK(9, 7)))

#define DEV_ID_RR1(d)		(0xc4 + ((d)*0x10))
#define DEV_ID_RR1_PID_MSB(pid) (pid)

#define DEV_ID_RR2(d)		(0xc8 + ((d)*0x10))
#define DEV_ID_RR2_PID_LSB(pid) ((pid) << 16)
#define DEV_ID_RR2_BCR(bcr)	((bcr) << 8)
#define DEV_ID_RR2_DCR(dcr)	(dcr)
#define DEV_ID_RR2_LVR(lvr)	(lvr)

#define SIR_MAP(x)		 (0x180 + ((x)*4))
#define SIR_MAP_DEV_REG(d)	 SIR_MAP((d) / 2)
#define SIR_MAP_DEV_SHIFT(d, fs) ((fs) + (((d) % 2) ? 16 : 0))
#define SIR_MAP_DEV_CONF_MASK(d) (GENMASK(15, 0) << (((d) % 2) ? 16 : 0))
#define SIR_MAP_DEV_CONF(d, c)	 ((c) << (((d) % 2) ? 16 : 0))
#define DEV_ROLE_SLAVE		 0
#define DEV_ROLE_MASTER		 1
#define SIR_MAP_DEV_ROLE(role)	 ((role) << 14)
#define SIR_MAP_DEV_SLOW	 BIT(13)
#define SIR_MAP_DEV_PL(l)	 ((l) << 8)
#define SIR_MAP_PL_MAX		 GENMASK(4, 0)
#define SIR_MAP_DEV_DA(a)	 ((a) << 1)
#define SIR_MAP_DEV_ACK		 BIT(0)

#define GPIR_WORD(x)	 (0x200 + ((x)*4))
#define GPI_REG(val, id) (((val) >> (((id) % 4) * 8)) & GENMASK(7, 0))

#define GPOR_WORD(x)	 (0x220 + ((x)*4))
#define GPO_REG(val, id) (((val) >> (((id) % 4) * 8)) & GENMASK(7, 0))

#define ASF_INT_STATUS	      0x300
#define ASF_INT_RAW_STATUS    0x304
#define ASF_INT_MASK	      0x308
#define ASF_INT_TEST	      0x30c
#define ASF_INT_FATAL_SELECT  0x310
#define ASF_INTEGRITY_ERR     BIT(6)
#define ASF_PROTOCOL_ERR      BIT(5)
#define ASF_TRANS_TIMEOUT_ERR BIT(4)
#define ASF_CSR_ERR	      BIT(3)
#define ASF_DAP_ERR	      BIT(2)
#define ASF_SRAM_UNCORR_ERR   BIT(1)
#define ASF_SRAM_CORR_ERR     BIT(0)

#define ASF_SRAM_CORR_FAULT_STATUS	0x320
#define ASF_SRAM_UNCORR_FAULT_STATUS	0x324
#define ASF_SRAM_CORR_FAULT_INSTANCE(x) ((x) >> 24)
#define ASF_SRAM_CORR_FAULT_ADDR(x)	((x)&GENMASK(23, 0))

#define ASF_SRAM_FAULT_STATS	       0x328
#define ASF_SRAM_FAULT_UNCORR_STATS(x) ((x) >> 16)
#define ASF_SRAM_FAULT_CORR_STATS(x)   ((x)&GENMASK(15, 0))

#define ASF_TRANS_TOUT_CTRL   0x330
#define ASF_TRANS_TOUT_EN     BIT(31)
#define ASF_TRANS_TOUT_VAL(x) (x)

#define ASF_TRANS_TOUT_FAULT_MASK      0x334
#define ASF_TRANS_TOUT_FAULT_STATUS    0x338
#define ASF_TRANS_TOUT_FAULT_APB       BIT(3)
#define ASF_TRANS_TOUT_FAULT_SCL_LOW   BIT(2)
#define ASF_TRANS_TOUT_FAULT_SCL_HIGH  BIT(1)
#define ASF_TRANS_TOUT_FAULT_FSCL_HIGH BIT(0)

#define ASF_PROTO_FAULT_MASK		0x340
#define ASF_PROTO_FAULT_STATUS		0x344
#define ASF_PROTO_FAULT_SLVSDR_RD_ABORT BIT(31)
#define ASF_PROTO_FAULT_SLVDDR_FAIL	BIT(30)
#define ASF_PROTO_FAULT_S(x)		BIT(16 + (x))
#define ASF_PROTO_FAULT_MSTSDR_RD_ABORT BIT(15)
#define ASF_PROTO_FAULT_MSTDDR_FAIL	BIT(14)
#define ASF_PROTO_FAULT_M(x)		BIT(x)

/*******************************************************************************
 * Local Constants Definition
 ******************************************************************************/
#define DIV_ROUND_UP(n, d) (((n) + (d)-1) / (d))

/* TODO: this needs to be configurable in the dts...somehow */
#define I3C_CONTROLLER_ADDR 0x08

/* Maximum i3c devices that the IP can be built with */
#define I3C_MAX_DEVS		  11
#define I3C_MAX_MSGS		  10
#define I3C_SIR_DEFAULT_DA	  0x7F
#define I3C_MAX_IDLE_WAIT_RETRIES 50
#define I3C_PRESCL_REG_SCALE	  (4)
#define I2C_PRESCL_REG_SCALE	  (5)

/* Target T_LOW period in open-drain mode. */
#define I3C_BUS_TLOW_OD_MIN_NS 200

/* MIPI I3C v1.1.1 Spec defines tsco max as 12ns */
#define I3C_TSCO_DEFAULT_NS 10

/* Interrupt thresholds. */
/* command response fifo threshold */
#define I3C_CMDR_THR 1
/* command tx fifo threshold - unused */
#define I3C_CMDD_THR 1
/* in-band-interrupt data fifo threshold - unused */
#define I3C_IBID_THR 1
/* in-band-interrupt response queue threshold */
#define I3C_IBIR_THR 1
/* tx data threshold - unused */
#define I3C_TX_THR   1

#define LOG_MODULE_NAME I3C_CADENCE
LOG_MODULE_REGISTER(I3C_CADENCE, CONFIG_I3C_CADENCE_LOG_LEVEL);

/*******************************************************************************
 * Local Types Definition
 ******************************************************************************/

/** Describes peripheral HW configuration determined from CONFx registers. */
struct cdns_i3c_hw_config {
	/* The maxiumum command queue depth. */
	uint32_t cmd_mem_depth;
	/* The maxiumum command response queue depth. */
	uint32_t cmdr_mem_depth;
	/* The maximum RX FIFO depth. */
	uint32_t rx_mem_depth;
	/* The maximum TX FIFO depth. */
	uint32_t tx_mem_depth;
	/* The maximum IBIR FIFO depth. */
	uint32_t ibir_mem_depth;
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
	uint32_t len;
	void *buf;
	uint32_t error;
};

/* Transfer data */
struct cdns_i3c_xfer {
	struct k_sem complete;
	int ret;
	int num_cmds;
	struct cdns_i3c_cmd cmds[I3C_MAX_MSGS];
};

/* Driver config */
struct cdns_i3c_config {
	/** base address of the controller */
	uintptr_t base;
	/** input frequency to the I3C Cadence */
	uint32_t input_frequency;
	/** Interrupt configuration function. */
	void (*irq_config_func)(const struct device *dev);
	/** I3C/I2C device list struct. */
	struct i3c_dev_list device_list;
};

/* Driver instance data */
struct cdns_i3c_data {
	struct i3c_config_controller ctrl_config;
	struct i3c_addr_slots addr_slots;
	struct cdns_i3c_hw_config hw_cfg;
	struct k_mutex bus_lock;
	struct cdns_i3c_i2c_dev_data cdns_i3c_i2c_priv_data[I3C_MAX_DEVS];
	struct cdns_i3c_xfer xfer;
	struct i3c_target_config *target_config;
	struct k_sem ibi_hj_complete;
	uint32_t free_rr_slots;
	uint8_t max_devs;
};

/*******************************************************************************
 * Global Variables Declaration
 ******************************************************************************/

/*******************************************************************************
 * Local Functions Declaration
 ******************************************************************************/

/*******************************************************************************
 * Private Functions Code
 ******************************************************************************/

/* Computes and sets parity */
/* Returns [7:1] 7-bit addr, [0] even/xor parity */
static uint8_t cdns_i3c_even_parity(uint8_t byte)
{
	uint8_t parity = 0;
	uint8_t b = byte;

	while (b) {
		parity = !parity;
		b = b & (b - 1);
	}
	b = (byte << 1) | !parity;

	return b;
}

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

/* FIFO mgmt */
static void cdns_i3c_write_tx_fifo(const struct cdns_i3c_config *config, const void *buf,
				   uint32_t len)
{
	const uint32_t *ptr = buf;
	uint32_t remain, val;

	for (remain = len; remain >= 4; remain -= 4) {
		val = *ptr++;
		sys_write32(val, config->base + TX_FIFO);
	}

	if (remain > 0) {
		val = 0;
		memcpy(&val, ptr, remain);
		sys_write32(val, config->base + TX_FIFO);
	}
}

static int cdns_i3c_read_rx_fifo(const struct cdns_i3c_config *config, void *buf, uint32_t len)
{
	uint32_t *ptr = buf;
	uint32_t remain, val;

	for (remain = len; remain >= 4; remain -= 4) {
		if (cdns_i3c_rx_fifo_empty(config)) {
			return -EIO;
		}
		val = sys_le32_to_cpu(sys_read32(config->base + RX_FIFO));
		*ptr++ = val;
	}

	if (remain > 0) {
		if (cdns_i3c_rx_fifo_empty(config)) {
			return -EIO;
		}
		val = sys_le32_to_cpu(sys_read32(config->base + RX_FIFO));
		memcpy(ptr, &val, remain);
	}

	return 0;
}

static int cdns_i3c_read_ibi_fifo(const struct cdns_i3c_config *config, void *buf, uint32_t len)
{
	uint32_t *ptr = buf;
	uint32_t remain, val;

	for (remain = len; remain >= 4; remain -= 4) {
		if (cdns_i3c_ibi_fifo_empty(config)) {
			return -EIO;
		}
		val = sys_le32_to_cpu(sys_read32(config->base + IBI_DATA_FIFO));
		*ptr++ = val;
	}

	if (remain > 0) {
		if (cdns_i3c_ibi_fifo_empty(config)) {
			return -EIO;
		}
		val = sys_le32_to_cpu(sys_read32(config->base + IBI_DATA_FIFO));
		memcpy(ptr, &val, remain);
	}

	return 0;
}

static void cdns_i3c_set_prescalers(const struct device *dev)
{
	struct cdns_i3c_data *data = dev->data;
	const struct cdns_i3c_config *config = dev->config;
	struct i3c_config_controller *ctrl_config = &data->ctrl_config;

	/* These formulas are from section 6.2.1 of the Cadence I3C Master User Guide. */
	uint32_t prescl_i3c = DIV_ROUND_UP(config->input_frequency,
					   (ctrl_config->scl.i3c * I3C_PRESCL_REG_SCALE)) -
			      1;
	uint32_t prescl_i2c = DIV_ROUND_UP(config->input_frequency,
					   (ctrl_config->scl.i2c * I2C_PRESCL_REG_SCALE)) -
			      1;

	/* update with actual value */
	ctrl_config->scl.i3c = config->input_frequency / ((prescl_i3c + 1) * I3C_PRESCL_REG_SCALE);
	ctrl_config->scl.i2c = config->input_frequency / ((prescl_i2c + 1) * I2C_PRESCL_REG_SCALE);

	LOG_DBG("%s: I3C speed = %u, PRESCL_CTRL0.i3c = 0x%x", dev->name, ctrl_config->scl.i3c,
		prescl_i3c);
	LOG_DBG("%s: I2C speed = %u, PRESCL_CTRL0.i2c = 0x%x", dev->name, ctrl_config->scl.i2c,
		prescl_i2c);

	/* Calculate the OD_LOW value assuming a desired T_low period of 210ns. */
	uint32_t pres_step = 1000000000 / (ctrl_config->scl.i3c * 4);
	int32_t od_low = DIV_ROUND_UP(I3C_BUS_TLOW_OD_MIN_NS, pres_step) - 2;

	if (od_low < 0) {
		od_low = 0;
	}
	LOG_DBG("%s: PRESCL_CTRL1.od_low = 0x%x", dev->name, od_low);

	/* disable in order to update timing */
	uint32_t ctrl = sys_read32(config->base + CTRL);

	if (ctrl & CTRL_DEV_EN) {
		sys_write32(~CTRL_DEV_EN & ctrl, config->base + CTRL);
	}

	sys_write32(PRESCL_CTRL0_I3C(prescl_i3c) | PRESCL_CTRL0_I2C(prescl_i2c),
		    config->base + PRESCL_CTRL0);

	/* Sets the open drain low time relative to the push-pull. */
	sys_write32(PRESCL_CTRL1_OD_LOW(od_low & PRESCL_CTRL1_OD_LOW_MASK),
		    config->base + PRESCL_CTRL1);

	/* reenable */
	if (ctrl & CTRL_DEV_EN) {
		sys_write32(CTRL_DEV_EN | ctrl, config->base + CTRL);
	}
}

/**
 * @brief Compute RR0 Value from addr
 *
 * @param addr Address of the target
 *
 * @return RR0 value
 */
static uint32_t prepare_rr0_dev_address(uint16_t addr)
{
	/* RR0[7:1] = addr[6:0] | parity^[0] */
	uint32_t ret = cdns_i3c_even_parity(addr);

	if (addr & GENMASK(9, 7)) {
		/* RR0[15:13] = addr[9:7] */
		ret |= (addr & GENMASK(9, 7)) << 6;
		/* RR0[11] = 10b lvr addr */
		ret |= DEV_ID_RR0_LVR_EXT_ADDR;
	}

	return ret;
}

/**
 * @brief Program Retaining Registers with device lists
 *
 * This will reprogram all retaining registers with I3C devices, I2C devices,
 * and the controller itself.
 *
 * @param dev Pointer to controller device driver instance.
 */
static void cdns_i3c_program_retaining_regs(const struct device *dev)
{
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_data *data = dev->data;

	/* Clear all retaining regs */
	sys_write32(DEVS_CTRL_DEV_CLR_ALL, config->base + DEVS_CTRL);

	uint32_t dev_id_rr0;
	uint32_t dev_id_rr1;
	uint32_t dev_id_rr2;

	/* program I2C devices */
	for (int i = 0; i < config->device_list.num_i2c; i++) {
		struct i3c_i2c_device_desc *i2c_device = &(config->device_list.i2c[i]);
		struct cdns_i3c_i2c_dev_data *cdns_i2c_device_data = i2c_device->controller_priv;

		if (cdns_i2c_device_data == NULL) {
			LOG_ERR("%s: device not attached", dev->name);
		}

		/* Mark the address as I2C device */
		i3c_addr_slots_mark_i2c(&data->addr_slots, i2c_device->addr);

		dev_id_rr0 = prepare_rr0_dev_address(i2c_device->addr);
		dev_id_rr2 = DEV_ID_RR2_LVR(i2c_device->lvr);

		sys_write32(dev_id_rr0, config->base + DEV_ID_RR0(cdns_i2c_device_data->id));
		sys_write32(0, config->base + DEV_ID_RR1(cdns_i2c_device_data->id));
		sys_write32(dev_id_rr2, config->base + DEV_ID_RR2(cdns_i2c_device_data->id));

		sys_write32(sys_read32(config->base + DEVS_CTRL) |
				    DEVS_CTRL_DEV_ACTIVE(cdns_i2c_device_data->id),
			    config->base + DEVS_CTRL);
	}

	/* program I3C devices */
	for (int i = 0; i < config->device_list.num_i3c; i++) {
		struct i3c_device_desc *i3c_device = &(config->device_list.i3c[i]);
		struct cdns_i3c_i2c_dev_data *cdns_i3c_device_data = i3c_device->controller_priv;

		if (cdns_i3c_device_data == NULL) {
			LOG_ERR("%s: %s: device not attached", dev->name, i3c_device->dev->name);
			continue;
		}

		/*
		 * Use the desired dynamic address as the new dynamic address
		 * if the slot is free.
		 */
		uint8_t dynamic_addr;

		if (i3c_device->init_dynamic_addr != 0U) {
			/* initial dynamic address is requested */
			if (i3c_device->static_addr != 0) {
				if (i3c_addr_slots_is_free(&data->addr_slots,
							   i3c_device->init_dynamic_addr)) {
					/* Set DA during ENTDAA */
					dynamic_addr = i3c_device->init_dynamic_addr;
				} else {
					/* address is not free, get the next one */
					dynamic_addr =
						i3c_addr_slots_next_free_find(&data->addr_slots);
				}
			} else {
				/* Use the init dynamic address as it's DA, but the RR will need to
				 * be first set with it's SA to run SETDASA, the RR address will
				 * need be updated after SETDASA with the request dynamic address
				 */
				dynamic_addr = i3c_device->static_addr;
			}
		} else {
			/* no init dynamic address is requested */
			if (i3c_device->static_addr != 0) {
				/* static exists, set DA with same SA during SETDASA*/
				dynamic_addr = i3c_device->static_addr;
			} else {
				/* pick a DA to use */
				dynamic_addr = i3c_addr_slots_next_free_find(&data->addr_slots);
			}
		}
		/* Mark the address as I3C device */
		i3c_addr_slots_mark_i3c(&data->addr_slots, dynamic_addr);

		dev_id_rr0 = DEV_ID_RR0_IS_I3C | prepare_rr0_dev_address(dynamic_addr);
		dev_id_rr1 = DEV_ID_RR1_PID_MSB((i3c_device->pid & 0xFFFFFFFF0000) >> 16);
		dev_id_rr2 = DEV_ID_RR2_PID_LSB(i3c_device->pid & 0xFFFF);

		sys_write32(dev_id_rr0, config->base + DEV_ID_RR0(cdns_i3c_device_data->id));
		sys_write32(dev_id_rr1, config->base + DEV_ID_RR1(cdns_i3c_device_data->id));
		sys_write32(dev_id_rr2, config->base + DEV_ID_RR2(cdns_i3c_device_data->id));

		/** Mark Devices as active, devices that will be found and marked active during DAA,
		 * it will be given the exact DA programmed in it's RR if the PID matches and marked
		 * as active duing ENTDAA, otherwise they get set as active here
		 */
		if (!((i3c_device->static_addr == 0) ||
		      ((i3c_device->init_dynamic_addr != 0) &&
		       (i3c_device->static_addr != i3c_device->init_dynamic_addr)))) {
			sys_write32(sys_read32(config->base + DEVS_CTRL) |
					    DEVS_CTRL_DEV_ACTIVE(cdns_i3c_device_data->id),
				    config->base + DEVS_CTRL);
		}
	}

	/* Set controller retaining register */
	uint8_t controller_da = I3C_CONTROLLER_ADDR;

	if (!i3c_addr_slots_is_free(&data->addr_slots, controller_da)) {
		controller_da = i3c_addr_slots_next_free_find(&data->addr_slots);
		LOG_DBG("%s: 0x%02x DA selected for controller", dev->name, controller_da);
	}
	sys_write32(prepare_rr0_dev_address(controller_da), config->base + DEV_ID_RR0(0));
	/* Mark the address as I3C device */
	i3c_addr_slots_mark_i3c(&data->addr_slots, controller_da);
}

#ifdef CONFIG_I3C_USE_IBI
static int cdns_i3c_controller_ibi_enable(const struct device *dev, struct i3c_device_desc *target)
{
	uint32_t sir_map;
	uint32_t sir_cfg;
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_i2c_dev_data *cdns_i3c_device_data = target->controller_priv;
	struct i3c_ccc_events i3c_events;
	int ret = 0;

	if (!i3c_device_is_ibi_capable(target)) {
		ret = -EINVAL;
		return ret;
	}

	/* TODO: check for duplicate in SIR */

	sir_cfg = SIR_MAP_DEV_ROLE(I3C_BCR_DEVICE_ROLE(target->bcr)) |
		  SIR_MAP_DEV_DA(target->dynamic_addr) |
		  SIR_MAP_DEV_PL(target->data_length.max_ibi);
	if (target->ibi_cb != NULL) {
		sir_cfg |= SIR_MAP_DEV_ACK;
	}
	if (target->bcr & I3C_BCR_MAX_DATA_SPEED_LIMIT) {
		sir_cfg |= SIR_MAP_DEV_SLOW;
	}

	LOG_DBG("%s: IBI enabling for 0x%02x (BCR 0x%02x)", dev->name, target->dynamic_addr,
		target->bcr);

	/* Tell target to enable IBI */
	i3c_events.events = I3C_CCC_EVT_INTR;
	ret = i3c_ccc_do_events_set(target, true, &i3c_events);
	if (ret != 0) {
		LOG_ERR("%s: Error sending IBI ENEC for 0x%02x (%d)", dev->name,
			target->dynamic_addr, ret);
		return ret;
	}

	sir_map = sys_read32(config->base + SIR_MAP_DEV_REG(cdns_i3c_device_data->id - 1));
	sir_map &= ~SIR_MAP_DEV_CONF_MASK(cdns_i3c_device_data->id - 1);
	sir_map |= SIR_MAP_DEV_CONF(cdns_i3c_device_data->id - 1, sir_cfg);

	sys_write32(sir_map, config->base + SIR_MAP_DEV_REG(cdns_i3c_device_data->id - 1));

	return ret;
}

static int cdns_i3c_controller_ibi_disable(const struct device *dev, struct i3c_device_desc *target)
{
	uint32_t sir_map;
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_i2c_dev_data *cdns_i3c_device_data = target->controller_priv;
	struct i3c_ccc_events i3c_events;
	int ret = 0;

	if (!i3c_device_is_ibi_capable(target)) {
		ret = -EINVAL;
		return ret;
	}

	/* Tell target to disable IBI */
	i3c_events.events = I3C_CCC_EVT_INTR;
	ret = i3c_ccc_do_events_set(target, false, &i3c_events);
	if (ret != 0) {
		LOG_ERR("%s: Error sending IBI DISEC for 0x%02x (%d)", dev->name,
			target->dynamic_addr, ret);
		return ret;
	}

	sir_map = sys_read32(config->base + SIR_MAP_DEV_REG(cdns_i3c_device_data->id - 1));
	sir_map &= ~SIR_MAP_DEV_CONF_MASK(cdns_i3c_device_data->id - 1);
	sir_map |=
		SIR_MAP_DEV_CONF(cdns_i3c_device_data->id - 1, SIR_MAP_DEV_DA(I3C_BROADCAST_ADDR));
	sys_write32(sir_map, config->base + SIR_MAP_DEV_REG(cdns_i3c_device_data->id - 1));

	return ret;
}

static int cdns_i3c_target_ibi_raise_hj(const struct device *dev)
{
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_data *data = dev->data;
	struct i3c_config_controller *ctrl_config = &data->ctrl_config;

	/* HJ requests should not be done by primary controllers */
	if (!ctrl_config->is_secondary) {
		LOG_ERR("%s: controller is primary, HJ not available", dev->name);
		return -ENOTSUP;
	}
	/* Check if target already has a DA assigned to it */
	if (sys_read32(config->base + SLV_STATUS1) & SLV_STATUS1_HAS_DA) {
		LOG_ERR("%s: HJ not available, DA already assigned", dev->name);
		return -EACCES;
	}
	/* Check if HJ requests DISEC CCC with DISHJ field set has been received */
	if (sys_read32(config->base + SLV_STATUS1) & SLV_STATUS1_HJ_DIS) {
		LOG_ERR("%s: HJ requests are currently disabled by DISEC", dev->name);
		return -EAGAIN;
	}

	sys_write32(CTRL_HJ_INIT | sys_read32(config->base + CTRL), config->base + CTRL);
	k_sem_reset(&data->ibi_hj_complete);
	if (k_sem_take(&data->ibi_hj_complete, K_MSEC(500)) != 0) {
		LOG_ERR("%s: timeout waiting for DAA after HJ", dev->name);
		return -ETIMEDOUT;
	}
	return 0;
}

static int cdns_i3c_target_ibi_raise(const struct device *dev, struct i3c_ibi *request)
{
	if (request == NULL) {
		return -EINVAL;
	}

	switch (request->ibi_type) {
	case I3C_IBI_TARGET_INTR:
		return -ENOTSUP;
	case I3C_IBI_CONTROLLER_ROLE_REQUEST:
		/* TODO: Cadence I3C can support CR, but not implemented yet */
		return -ENOTSUP;
	case I3C_IBI_HOTJOIN:
		return cdns_i3c_target_ibi_raise_hj(dev);
	default:
		return -EINVAL;
	}
}
#endif

static void cdns_i3c_cancel_transfer(const struct device *dev)
{
	struct cdns_i3c_data *data = dev->data;
	const struct cdns_i3c_config *config = dev->config;
	uint32_t val;
	uint32_t retry_count;

	/* Disable further interrupts */
	sys_write32(MST_INT_CMDD_EMP, config->base + MST_IDR);

	/* Ignore if no pending transfer */
	if (data->xfer.num_cmds == 0)
		return;

	data->xfer.num_cmds = 0;

	/* Clear main enable bit to disable further transactions */
	sys_write32(~CTRL_DEV_EN & sys_read32(config->base + CTRL), config->base + CTRL);

	/**
	 * Spin waiting for device to go idle. It is unlikely that this will
	 * actually take any time since we only get here if a transaction didn't
	 * complete in a long time.
	 */
	retry_count = I3C_MAX_IDLE_WAIT_RETRIES;
	while (retry_count--) {
		val = sys_read32(config->base + MST_STATUS0);
		if (val & MST_STATUS0_IDLE) {
			break;
		}
		k_msleep(10);
	}
	if (retry_count == 0) {
		data->xfer.ret = -ETIMEDOUT;
	}

	/**
	 * Flush all queues.
	 */
	sys_write32(FLUSH_RX_FIFO | FLUSH_TX_FIFO | FLUSH_CMD_FIFO | FLUSH_CMD_RESP,
		    config->base + FLUSH_CTRL);

	/* Re-enable device */
	sys_write32(CTRL_DEV_EN | sys_read32(config->base + CTRL), config->base + CTRL);
}

/**
 * @brief Start a I3C/I2C Transfer
 *
 * This is to be called from a I3C/I2C transfer function. This will write
 * all data to tx and cmd fifos
 *
 * @param dev Pointer to controller device driver instance.
 */
static void cdns_i3c_start_transfer(const struct device *dev)
{
	struct cdns_i3c_data *data = dev->data;
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_xfer *xfer = &data->xfer;

	/* Ensure no pending command response queue threshold interrupt */
	sys_write32(MST_INT_CMDD_EMP, config->base + MST_ICR);

	/* Make sure RX FIFO is empty. */
	while (!cdns_i3c_rx_fifo_empty(config)) {
		(void)sys_read32(config->base + RX_FIFO);
	}
	/* Make sure CMDR FIFO is empty too */
	while (!cdns_i3c_cmd_rsp_fifo_empty(config)) {
		(void)sys_read32(config->base + CMDR);
	}

	/* Write all tx data to fifo */
	for (unsigned int i = 0; i < xfer->num_cmds; i++) {
		if (!(xfer->cmds[i].cmd0 & CMD0_FIFO_RNW)) {
			cdns_i3c_write_tx_fifo(config, xfer->cmds[i].buf, xfer->cmds[i].len);
		}
	}

	/* Write all data to cmd fifos */
	for (unsigned int i = 0; i < xfer->num_cmds; i++) {
		/* The command ID is just the msg index. */
		xfer->cmds[i].cmd1 |= CMD1_FIFO_CMDID(i);
		sys_write32(xfer->cmds[i].cmd1, config->base + CMD1_FIFO);
		sys_write32(xfer->cmds[i].cmd0, config->base + CMD0_FIFO);
	}

	/* kickoff transfer */
	sys_write32(CTRL_MCS | sys_read32(config->base + CTRL), config->base + CTRL);
	sys_write32(MST_INT_CMDD_EMP, config->base + MST_IER);
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
static int cdns_i3c_do_ccc(const struct device *dev, struct i3c_ccc_payload *payload)
{
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_data *data = dev->data;
	struct cdns_i3c_cmd *dcmd = &data->xfer.cmds[0];
	int ret = 0;
	int num_cmds = 0;

	/* make sure we are currently the active controller */
	if (!(sys_read32(config->base + MST_STATUS0) & MST_STATUS0_MASTER_MODE)) {
		return -EACCES;
	}

	if (payload == NULL) {
		return -EINVAL;
	}

	/*
	 * Ensure data will fit within FIFOs.
	 *
	 * TODO: This limitation prevents burst transfers greater than the
	 *       FIFO sizes and should be replaced with an implementation that
	 *       utilizes the RX/TX data threshold interrupts.
	 */
	uint32_t num_msgs =
		1 + ((payload->ccc.data_len > 0) ? payload->targets.num_targets
						 : MAX(payload->targets.num_targets - 1, 0));
	if (num_msgs > data->hw_cfg.cmd_mem_depth || num_msgs > data->hw_cfg.cmdr_mem_depth) {
		LOG_ERR("%s: Too many messages", dev->name);
		return -ENOMEM;
	}

	uint32_t rxsize = 0;
	uint32_t txsize = ROUND_UP(payload->ccc.data_len, 4);

	for (int i = 0; i < payload->targets.num_targets; i++) {
		if (payload->targets.payloads[i].rnw) {
			rxsize += ROUND_UP(payload->targets.payloads[i].data_len, 4);
		} else {
			txsize += ROUND_UP(payload->targets.payloads[i].data_len, 4);
		}
	}
	if ((rxsize > data->hw_cfg.rx_mem_depth) || (txsize > data->hw_cfg.tx_mem_depth)) {
		LOG_ERR("%s: Total RX and/or TX transfer larger than FIFO", dev->name);
		return -ENOMEM;
	}

	LOG_DBG("%s: CCC[0x%02x]", dev->name, payload->ccc.id);

	k_mutex_lock(&data->bus_lock, K_FOREVER);

	dcmd->cmd1 = CMD1_FIFO_CCC(payload->ccc.id);
	dcmd->cmd0 = CMD0_FIFO_IS_CCC;
	dcmd->len = 0;

	size_t idx = 0;

	if (payload->ccc.data_len > 0) {
		/* Write additional data for CCC if needed */
		dcmd->buf = payload->ccc.data;
		dcmd->len = payload->ccc.data_len;
		dcmd->cmd0 |= CMD0_FIFO_PL_LEN(payload->ccc.data_len);
	} else if (payload->targets.num_targets > 0) {
		dcmd->buf = payload->targets.payloads[0].data;
		dcmd->len = payload->targets.payloads[0].data_len;
		dcmd->cmd0 |= CMD0_FIFO_DEV_ADDR(payload->targets.payloads[0].addr) |
			      CMD0_FIFO_PL_LEN(payload->targets.payloads[0].data_len);
		if (payload->targets.payloads[0].rnw) {
			dcmd->cmd0 |= CMD0_FIFO_RNW;
		}
		idx++;
	}
	num_cmds++;

	if (!i3c_ccc_is_payload_broadcast(payload)) {
		/*
		 * If there are payload(s) for each target,
		 * RESTART and then send payload for each target.
		 */
		while (idx < payload->targets.num_targets) {
			num_cmds++;
			struct cdns_i3c_cmd *cmd = &data->xfer.cmds[idx + 1];
			struct i3c_ccc_target_payload *tgt_payload =
				&payload->targets.payloads[idx];
			/* Send repeated start on all transfers except the last */
			if (idx < (payload->targets.num_targets - 1)) {
				cmd->cmd0 |= CMD0_FIFO_RSBC;
			}
			cmd->cmd0 |= CMD0_FIFO_DEV_ADDR(tgt_payload->addr);
			if (tgt_payload->rnw) {
				cmd->cmd0 |= CMD0_FIFO_RNW;
			}

			cmd->buf = tgt_payload->data;
			cmd->len = tgt_payload->data_len;

			idx++;
		}
	}

	data->xfer.ret = -ETIMEDOUT;
	data->xfer.num_cmds = num_cmds;

	cdns_i3c_start_transfer(dev);

	if (k_sem_take(&data->xfer.complete, K_MSEC(1000)) != 0) {
		cdns_i3c_cancel_transfer(dev);
	}

	if (data->xfer.ret < 0) {
		LOG_ERR("%s: CCC[0x%02x] error (%d)", dev->name, payload->ccc.id, data->xfer.ret);
	}

	ret = data->xfer.ret;
	k_mutex_unlock(&data->bus_lock);

	return ret;
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
static int cdns_i3c_do_daa(const struct device *dev)
{
	struct cdns_i3c_data *data = dev->data;
	const struct cdns_i3c_config *config = dev->config;
	struct i3c_config_controller *ctrl_config = &data->ctrl_config;

	/* DAA should not be done by secondary controllers */
	if (ctrl_config->is_secondary) {
		return -ENOTSUP;
	}

	/* read dev active reg */
	uint32_t olddevs = sys_read32(config->base + DEVS_CTRL) & DEVS_CTRL_DEVS_ACTIVE_MASK;
	/* ignore the controller register */
	olddevs |= BIT(0);

	/* the Cadence I3C IP will assign an address for it from the RR */
	struct i3c_ccc_payload entdaa_ccc;

	memset(&entdaa_ccc, 0, sizeof(entdaa_ccc));
	entdaa_ccc.ccc.id = I3C_CCC_ENTDAA;

	int status = cdns_i3c_do_ccc(dev, &entdaa_ccc);

	if (status != 0) {
		return status;
	}

	/* read again dev active reg  */
	uint32_t newdevs = sys_read32(config->base + DEVS_CTRL) & DEVS_CTRL_DEVS_ACTIVE_MASK;
	/* look for new bits that were set */
	newdevs &= ~olddevs;

	if (newdevs) {
		/* loop through each set bit for new devices */
		for (uint8_t i = find_lsb_set(newdevs); i <= find_msb_set(newdevs); i++) {
			uint8_t rr_idx = i - 1;

			if (newdevs & BIT(rr_idx)) {
				/* Read RRx registers */
				uint32_t dev_id_rr0 = sys_read32(config->base + DEV_ID_RR0(rr_idx));
				uint32_t dev_id_rr1 = sys_read32(config->base + DEV_ID_RR1(rr_idx));
				uint32_t dev_id_rr2 = sys_read32(config->base + DEV_ID_RR2(rr_idx));

				uint64_t pid = ((uint64_t)dev_id_rr1 << 16) + (dev_id_rr2 >> 16);
				uint8_t dyn_addr = (dev_id_rr0 & 0xFE) >> 1;
				uint8_t bcr = dev_id_rr2 >> 8;
				uint8_t dcr = dev_id_rr2 & 0xFF;

				const struct i3c_device_id i3c_id = I3C_DEVICE_ID(pid);
				struct i3c_device_desc *target = i3c_device_find(dev, &i3c_id);

				if (target == NULL) {
					LOG_INF("%s: PID 0x%012llx is not in registered device "
						"list, given DA 0x%02x",
						dev->name, pid, dyn_addr);
					i3c_addr_slots_mark_i3c(&data->addr_slots, dyn_addr);
				} else {
					target->dynamic_addr = dyn_addr;
					target->bcr = bcr;
					target->dcr = dcr;

					LOG_DBG("%s: PID 0x%012llx assigned dynamic address 0x%02x",
						dev->name, pid, dyn_addr);
				}
			}
		}
	} else {
		LOG_DBG("%s: ENTDAA: No devices found", dev->name);
	}

	/* mark slot as not free, may already be set if already attached */
	data->free_rr_slots &= ~newdevs;

	/* Unmask Hot-Join request interrupts. HJ will send DISEC HJ from the CTRL value */
	struct i3c_ccc_events i3c_events;

	i3c_events.events = I3C_CCC_EVT_HJ;
	status = i3c_ccc_do_events_all_set(dev, true, &i3c_events);
	if (status != 0) {
		LOG_DBG("%s: Broadcast ENEC was NACK", dev->name);
	}

	return 0;
}

/**
 * @brief Configure I2C hardware.
 *
 * @param dev Pointer to controller device driver instance.
 * @param config Value of the configuration parameters.
 *
 * @retval 0 If successful.
 * @retval -EINVAL If invalid configure parameters.
 * @retval -EIO General Input/Output errors.
 * @retval -ENOSYS If not implemented.
 */
static int cdns_i3c_i2c_api_configure(const struct device *dev, uint32_t config)
{
	struct cdns_i3c_data *data = dev->data;
	struct i3c_config_controller *ctrl_config = &data->ctrl_config;

	switch (I2C_SPEED_GET(config)) {
	case I2C_SPEED_STANDARD:
		ctrl_config->scl.i2c = 100000;
		break;
	case I2C_SPEED_FAST:
		ctrl_config->scl.i2c = 400000;
		break;
	case I2C_SPEED_FAST_PLUS:
		ctrl_config->scl.i2c = 1000000;
		break;
	case I2C_SPEED_HIGH:
		ctrl_config->scl.i2c = 3400000;
		break;
	case I2C_SPEED_ULTRA:
		ctrl_config->scl.i2c = 5000000;
		break;
	default:
		break;
	}

	cdns_i3c_set_prescalers(dev);

	return 0;
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
static int cdns_i3c_configure(const struct device *dev, enum i3c_config_type type, void *config)
{
	struct cdns_i3c_data *data = dev->data;
	struct i3c_config_controller *ctrl_cfg = config;

	if ((ctrl_cfg->scl.i2c == 0U) || (ctrl_cfg->scl.i3c == 0U)) {
		return -EINVAL;
	}

	data->ctrl_config.scl.i3c = ctrl_cfg->scl.i3c;
	data->ctrl_config.scl.i2c = ctrl_cfg->scl.i2c;
	cdns_i3c_set_prescalers(dev);

	return 0;
}

/**
 * @brief Complete a I3C/I2C Transfer
 *
 * This is to be called from an ISR when the Command Response FIFO
 * is Empty. This will check each Command Response reading the RX
 * FIFO if message was a RnW and if any message had an error.
 *
 * @param dev Pointer to controller device driver instance.
 */
static void cdns_i3c_complete_transfer(const struct device *dev)
{
	struct cdns_i3c_data *data = dev->data;
	const struct cdns_i3c_config *config = dev->config;
	uint32_t cmdr;
	uint32_t id = 0;
	uint32_t rx = 0;
	int ret = 0;
	struct cdns_i3c_cmd *cmd;

	/* Disable further interrupts */
	sys_write32(MST_INT_CMDD_EMP, config->base + MST_IDR);

	/* Ignore if no pending transfer */
	if (data->xfer.num_cmds == 0) {
		return;
	}

	/* Process all results in fifo */
	for (uint32_t status0 = sys_read32(config->base + MST_STATUS0);
	     !(status0 & MST_STATUS0_CMDR_EMP); status0 = sys_read32(config->base + MST_STATUS0)) {
		cmdr = sys_read32(config->base + CMDR);
		id = CMDR_CMDID(cmdr);

		if (id == CMDR_CMDID_HJACK_DISEC || id == CMDR_CMDID_HJACK_ENTDAA ||
		    id >= data->xfer.num_cmds) {
			continue;
		}

		cmd = &data->xfer.cmds[id];

		/* Read any rx data into buffer */
		if (cmd->cmd0 & CMD0_FIFO_RNW) {
			rx = MIN(CMDR_XFER_BYTES(cmdr), cmd->len);
			ret = cdns_i3c_read_rx_fifo(config, cmd->buf, rx);
		}

		/* Record error */
		cmd->error = CMDR_ERROR(cmdr);
	}

	for (int i = 0; i < data->xfer.num_cmds; i++) {
		switch (data->xfer.cmds->error) {
		case CMDR_NO_ERROR:
			break;

		case CMDR_DDR_PREAMBLE_ERROR:
		case CMDR_DDR_PARITY_ERROR:
		case CMDR_M0_ERROR:
		case CMDR_M1_ERROR:
		case CMDR_M2_ERROR:
		case CMDR_MST_ABORT:
		case CMDR_NACK_RESP:
		case CMDR_DDR_DROPPED:
			ret = -EIO;
			break;

		case CMDR_DDR_RX_FIFO_OVF:
		case CMDR_DDR_TX_FIFO_UNF:
			ret = -ENOSPC;
			break;

		case CMDR_INVALID_DA:
		default:
			ret = -EINVAL;
			break;
		}
	}

	data->xfer.ret = ret;

	/* Indicate no transfer is pending */
	data->xfer.num_cmds = 0;

	k_sem_give(&data->xfer.complete);
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
static int cdns_i3c_i2c_transfer(const struct device *dev, struct i3c_i2c_device_desc *i2c_dev,
				 struct i2c_msg *msgs, uint8_t num_msgs)
{
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_data *data = dev->data;
	uint32_t txsize = 0;
	uint32_t rxsize = 0;
	int ret;

	/* make sure we are currently the active controller */
	if (!(sys_read32(config->base + MST_STATUS0) & MST_STATUS0_MASTER_MODE)) {
		return -EACCES;
	}

	if (num_msgs == 0) {
		return 0;
	}

	if (num_msgs > data->hw_cfg.cmd_mem_depth || num_msgs > data->hw_cfg.cmdr_mem_depth) {
		LOG_ERR("%s: Too many messages", dev->name);
		return -ENOMEM;
	}

	/*
	 * Ensure data will fit within FIFOs
	 */
	for (unsigned int i = 0; i < num_msgs; i++) {
		if ((msgs[i].flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
			rxsize += ROUND_UP(msgs[i].len, 4);
		} else {
			txsize += ROUND_UP(msgs[i].len, 4);
		}
	}
	if ((rxsize > data->hw_cfg.rx_mem_depth) || (txsize > data->hw_cfg.tx_mem_depth)) {
		LOG_ERR("%s: Total RX and/or TX transfer larger than FIFO", dev->name);
		return -ENOMEM;
	}

	k_mutex_lock(&data->bus_lock, K_FOREVER);

	for (unsigned int i = 0; i < num_msgs; i++) {
		struct cdns_i3c_cmd *cmd = &data->xfer.cmds[i];

		cmd->len = msgs[i].len;
		cmd->buf = msgs[i].buf;

		cmd->cmd0 = CMD0_FIFO_PRIV_XMIT_MODE(XMIT_BURST_WITHOUT_SUBADDR);
		cmd->cmd0 |= CMD0_FIFO_DEV_ADDR(i2c_dev->addr);
		cmd->cmd0 |= CMD0_FIFO_PL_LEN(msgs[i].len);

		/* Send repeated start on all transfers except the last or those marked STOP. */
		if ((i < (num_msgs - 1)) && ((msgs[i].flags & I2C_MSG_STOP) == 0)) {
			cmd->cmd0 |= CMD0_FIFO_RSBC;
		}

		if (msgs[i].flags & I2C_MSG_ADDR_10_BITS) {
			cmd->cmd0 |= CMD0_FIFO_IS_10B;
		}

		if ((msgs[i].flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
			cmd->cmd0 |= CMD0_FIFO_RNW;
		}
	}

	data->xfer.ret = -ETIMEDOUT;
	data->xfer.num_cmds = num_msgs;

	cdns_i3c_start_transfer(dev);
	if (k_sem_take(&data->xfer.complete, K_MSEC(1000)) != 0) {
		cdns_i3c_cancel_transfer(dev);
	}

	ret = data->xfer.ret;
	k_mutex_unlock(&data->bus_lock);

	return ret;
}

static int cdns_i3c_master_get_rr_slot(const struct device *dev, uint8_t dyn_addr)
{
	struct cdns_i3c_data *data = dev->data;
	const struct cdns_i3c_config *config = dev->config;

	if (dyn_addr == 0) {
		if (!data->free_rr_slots)
			return -ENOSPC;

		return find_lsb_set(data->free_rr_slots) - 1;
	}

	uint32_t activedevs = sys_read32(config->base + DEVS_CTRL) & DEVS_CTRL_DEVS_ACTIVE_MASK;

	activedevs &= ~BIT(0);

	/* loop through each set bit for new devices */
	for (uint8_t i = find_lsb_set(activedevs); i <= find_msb_set(activedevs); i++) {
		if (activedevs & BIT(i)) {
			uint32_t rr = sys_read32(config->base + DEV_ID_RR0(i));

			if (!(rr & DEV_ID_RR0_IS_I3C) || DEV_ID_RR0_GET_DEV_ADDR(rr) != dyn_addr) {
				continue;
			}
			return i;
		}
	}

	return -EINVAL;
}

static int cdns_i3c_attach_device(const struct device *dev, struct i3c_device_desc *desc)
{
	struct cdns_i3c_data *data = dev->data;

	int slot = cdns_i3c_master_get_rr_slot(dev, desc->dynamic_addr);

	if (slot < 0) {
		LOG_ERR("%s: no space for i3c device: %s", dev->name, desc->dev->name);
		return slot;
	}

	k_mutex_lock(&data->bus_lock, K_FOREVER);

	data->cdns_i3c_i2c_priv_data[slot].id = slot;
	desc->controller_priv = &(data->cdns_i3c_i2c_priv_data[slot]);
	data->free_rr_slots &= ~BIT(slot);

	k_mutex_unlock(&data->bus_lock);

	return 0;
}

static int cdns_i3c_i2c_attach_device(const struct device *dev, struct i3c_i2c_device_desc *desc)
{
	struct cdns_i3c_data *data = dev->data;

	int slot = cdns_i3c_master_get_rr_slot(dev, 0);

	if (slot < 0) {
		LOG_ERR("%s: no space for i2c device: addr 0x%02x", dev->name, desc->addr);
		return slot;
	}

	k_mutex_lock(&data->bus_lock, K_FOREVER);

	data->cdns_i3c_i2c_priv_data[slot].id = slot;
	desc->controller_priv = &(data->cdns_i3c_i2c_priv_data[slot]);
	data->free_rr_slots &= ~BIT(slot);

	k_mutex_unlock(&data->bus_lock);

	return 0;
}

/**
 * @brief Transfer messages in I3C mode.
 *
 * @see i3c_transfer
 *
 * @param dev Pointer to device driver instance.
 * @param target Pointer to target device descriptor.
 * @param msgs Pointer to I3C messages.
 * @param num_msgs Number of messages to transfers.
 *
 * @return @see i3c_transfer
 */
static int cdns_i3c_transfer(const struct device *dev, struct i3c_device_desc *target,
			     struct i3c_msg *msgs, uint8_t num_msgs)
{
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_data *data = dev->data;
	int txsize = 0;
	int rxsize = 0;
	int ret;

	/* make sure we are currently the active controller */
	if (!(sys_read32(config->base + MST_STATUS0) & MST_STATUS0_MASTER_MODE)) {
		return -EACCES;
	}

	if (num_msgs == 0) {
		return 0;
	}

	if (num_msgs > data->hw_cfg.cmd_mem_depth || num_msgs > data->hw_cfg.cmdr_mem_depth) {
		LOG_ERR("%s: Too many messages", dev->name);
		return -ENOMEM;
	}

	/*
	 * Ensure data will fit within FIFOs.
	 *
	 * TODO: This limitation prevents burst transfers greater than the
	 *       FIFO sizes and should be replaced with an implementation that
	 *       utilizes the RX/TX data interrupts.
	 */
	for (int i = 0; i < num_msgs; i++) {
		if ((msgs[i].flags & I3C_MSG_RW_MASK) == I3C_MSG_READ) {
			rxsize += ROUND_UP(msgs[i].len, 4);
		} else {
			txsize += ROUND_UP(msgs[i].len, 4);
		}
	}
	if ((rxsize > data->hw_cfg.rx_mem_depth) || (txsize > data->hw_cfg.tx_mem_depth)) {
		LOG_ERR("%s: Total RX and/or TX transfer larger than FIFO", dev->name);
		return -ENOMEM;
	}

	k_mutex_lock(&data->bus_lock, K_FOREVER);

	/*
	 * Prepare transfer commands. Currently there is only a single transfer
	 * in-flight but it would be possible to keep a queue of transfers. If so,
	 * this preparation could be completed outside of the bus lock allowing
	 * greater parallelism.
	 */
	bool send_broadcast = true;

	for (int i = 0; i < num_msgs; i++) {
		struct cdns_i3c_cmd *cmd = &data->xfer.cmds[i];
		uint32_t pl = msgs[i].len;

		cmd->len = pl;
		cmd->buf = msgs[i].buf;

		cmd->cmd0 = CMD0_FIFO_PRIV_XMIT_MODE(XMIT_BURST_WITHOUT_SUBADDR);
		cmd->cmd0 |= CMD0_FIFO_DEV_ADDR(target->dynamic_addr);
		if ((msgs[i].flags & I3C_MSG_RW_MASK) == I3C_MSG_READ) {
			cmd->cmd0 |= CMD0_FIFO_RNW;
			/*
			 * For I3C_XMIT_MODE_NO_ADDR reads in SDN mode,
			 * CMD0_FIFO_PL_LEN specifies the abort limit not bytes to read
			 */
			cmd->cmd0 |= CMD0_FIFO_PL_LEN(pl + 1);
		} else {
			cmd->cmd0 |= CMD0_FIFO_PL_LEN(pl);
		}

		/* Send broadcast header on first transfer or after a STOP. */
		if (!(msgs[i].flags & I3C_MSG_NBCH) && (send_broadcast)) {
			cmd->cmd0 |= CMD0_FIFO_BCH;
			send_broadcast = false;
		}

		/* Send repeated start on all transfers except the last or those marked STOP. */
		if ((i < (num_msgs - 1)) && ((msgs[i].flags & I3C_MSG_STOP) == 0)) {
			cmd->cmd0 |= CMD0_FIFO_RSBC;
		} else {
			send_broadcast = true;
		}
	}

	data->xfer.ret = -ETIMEDOUT;
	data->xfer.num_cmds = num_msgs;

	cdns_i3c_start_transfer(dev);
	if (k_sem_take(&data->xfer.complete, K_MSEC(1000)) != 0) {
		LOG_ERR("%s: transfer timed out", dev->name);
		cdns_i3c_cancel_transfer(dev);
	}

	ret = data->xfer.ret;
	k_mutex_unlock(&data->bus_lock);

	return ret;
}

static void cdns_i3c_handle_ibi(const struct device *dev, uint32_t ibir)
{
	const struct cdns_i3c_config *config = dev->config;

	uint8_t ibi_data[CONFIG_I3C_IBI_MAX_PAYLOAD_SIZE];

	/* The slave ID returned here is the device ID in the SIR map NOT the device ID
	 * in the RR map.
	 */
	uint8_t slave_id = IBIR_SLVID(ibir);

	if (slave_id == IBIR_SLVID_INV) {
		/* DA does not match any value among SIR map */
		return;
	}

	uint32_t dev_id_rr0 = sys_read32(config->base + DEV_ID_RR0(slave_id + 1));
	uint8_t dyn_addr = DEV_ID_RR0_GET_DEV_ADDR(dev_id_rr0);
	struct i3c_device_desc *desc = i3c_dev_list_i3c_addr_find(&config->device_list, dyn_addr);

	/*
	 * Check for NAK or error conditions.
	 *
	 * Note: The logging is for debugging only so will be compiled out in most cases.
	 * However, if the log level for this module is DEBUG and log mode is IMMEDIATE or MINIMAL,
	 * this option is also set this may cause problems due to being inside an ISR.
	 */
	if (!(IBIR_ACKED & ibir)) {
		LOG_DBG("%s: NAK for slave ID %u", dev->name, (unsigned int)slave_id);
		return;
	}
	if (ibir & IBIR_ERROR) {
		LOG_ERR("%s: Data overflow", dev->name);
		return;
	}

	/* Read out any payload bytes */
	uint8_t ibi_len = IBIR_XFER_BYTES(ibir);

	if (ibi_len > 0) {
		if (cdns_i3c_read_ibi_fifo(config, ibi_data, ibi_len) < 0) {
			LOG_ERR("%s: Failed to get payload", dev->name);
		}
	}

	if (i3c_ibi_work_enqueue_target_irq(desc, ibi_data, ibi_len) != 0) {
		LOG_ERR("%s: Error enqueue IBI IRQ work", dev->name);
	}
}

static void cdns_i3c_handle_hj(const struct device *dev, uint32_t ibir)
{
	if (!(IBIR_ACKED & ibir)) {
		LOG_DBG("%s: NAK for HJ", dev->name);
		return;
	}

	if (i3c_ibi_work_enqueue_hotjoin(dev) != 0) {
		LOG_ERR("%s: Error enqueue IBI HJ work", dev->name);
	}
}

static void cnds_i3c_master_demux_ibis(const struct device *dev)
{
	const struct cdns_i3c_config *config = dev->config;

	for (uint32_t status0 = sys_read32(config->base + MST_STATUS0);
	     !(status0 & MST_STATUS0_IBIR_EMP); status0 = sys_read32(config->base + MST_STATUS0)) {
		uint32_t ibir = sys_read32(config->base + IBIR);

		switch (IBIR_TYPE(ibir)) {
		case IBIR_TYPE_IBI:
			cdns_i3c_handle_ibi(dev, ibir);
			break;
		case IBIR_TYPE_HJ:
			cdns_i3c_handle_hj(dev, ibir);
			break;
		case IBIR_TYPE_MR:
			/* not implemented */
			break;
		default:
			break;
		}
	}
}

static void cdns_i3c_target_ibi_hj_complete(const struct device *dev)
{
	struct cdns_i3c_data *data = dev->data;

	k_sem_give(&data->ibi_hj_complete);
}

static void cdns_i3c_irq_handler(const struct device *dev)
{
	const struct cdns_i3c_config *config = dev->config;

	if (sys_read32(config->base + MST_STATUS0) & MST_STATUS0_MASTER_MODE) {
		uint32_t int_st = sys_read32(config->base + MST_ISR);

		/* Command queue empty */
		if (int_st & MST_INT_HALTED) {
			LOG_WRN("Core Halted, 2 read aborts");
			sys_write32(MST_INT_HALTED, config->base + MST_ICR);
		}

		/* Command queue empty */
		if (int_st & MST_INT_CMDD_EMP) {
			cdns_i3c_complete_transfer(dev);
			sys_write32(MST_INT_CMDD_EMP, config->base + MST_ICR);
		}

		/* Command queue threshold */
		if (int_st & MST_INT_CMDD_THR) {
			sys_write32(MST_INT_CMDD_THR, config->base + MST_ICR);
		}

		/* Command response threshold hit */
		if (int_st & MST_INT_CMDR_THR) {
			sys_write32(MST_INT_CMDR_THR, config->base + MST_ICR);
		}

		/* RX data ready */
		if (int_st & MST_INT_RX_THR) {
			sys_write32(MST_INT_RX_THR, config->base + MST_ICR);
		}

		/* In-band interrupt */
		if (int_st & MST_INT_IBIR_THR) {
			sys_write32(MST_INT_IBIR_THR, config->base + MST_ICR);
			cnds_i3c_master_demux_ibis(dev);
		}

		/* In-band interrupt data */
		if (int_st & MST_INT_IBID_THR) {
			sys_write32(MST_INT_IBID_THR, config->base + MST_ICR);
		}

		/* In-band interrupt data */
		if (int_st & MST_INT_TX_OVF) {
			sys_write32(MST_INT_TX_OVF, config->base + MST_ICR);
			LOG_ERR("%s: controller tx buffer overflow,", dev->name);
		}

		/* In-band interrupt data */
		if (int_st & MST_INT_RX_UNF) {
			sys_write32(MST_INT_RX_UNF, config->base + MST_ICR);
			LOG_ERR("%s: controller rx buffer underflow,", dev->name);
		}

		/* In-band interrupt data */
		if (int_st & MST_INT_IBID_THR) {
			sys_write32(MST_INT_IBID_THR, config->base + MST_ICR);
		}
	} else {
		uint32_t int_sl = sys_read32(config->base + SLV_ISR);
		struct cdns_i3c_data *data = dev->data;
		const struct i3c_target_callbacks *target_cb = data->target_config->callbacks;

		/* SLV SDR rx fifo threshold */
		if (int_sl & SLV_INT_SDR_RX_THR) {
			/* while rx fifo is not empty */
			while (!(sys_read32(config->base + SLV_STATUS1) &
				 SLV_STATUS1_SDR_RX_EMPTY)) {
				/* Target writes only write to the first byte of the 32 bit width
				 * fifo
				 */
				uint8_t rx_data = (uint8_t)sys_read32(config->base + RX_FIFO);
				/* call function pointer for write */
				if (target_cb != NULL && target_cb->write_received_cb != NULL) {
					target_cb->write_received_cb(data->target_config, rx_data);
				}
			}
		}

		/* SLV SDR tx fifo threshold */
		if (int_sl & SLV_INT_SDR_TX_THR) {
			int status = 0;

			if (target_cb != NULL && target_cb->read_processed_cb) {
				/* while tx fifo is not full and there is still data available */
				while ((!(sys_read32(config->base + SLV_STATUS1) &
					  SLV_STATUS1_SDR_TX_FULL)) &&
				       (status == 0)) {
					/* call function pointer for read */
					uint8_t byte;
					/* will return negative if no data left to transmit and 0 if
					 * data available
					 */
					status = target_cb->read_processed_cb(data->target_config,
									      &byte);
					if (status == 0) {
						cdns_i3c_write_tx_fifo(config, &byte, sizeof(byte));
					}
				}
			}
		}

		/* SLV SDR rx complete */
		if (int_sl & SLV_INT_SDR_RD_COMP) {
			/* a read needs to be done on slv_status 0 else a NACK will happen */
			(void)sys_read32(config->base + SLV_STATUS0);
			/* call stop function pointer */
			if (target_cb != NULL && target_cb->stop_cb) {
				target_cb->stop_cb(data->target_config);
			}
		}

		/* SLV SDR tx complete */
		if (int_sl & SLV_INT_SDR_WR_COMP) {
			/* a read needs to be done on slv_status 0 else a NACK will happen */
			(void)sys_read32(config->base + SLV_STATUS0);
			/* call stop function pointer */
			if (target_cb != NULL && target_cb->stop_cb) {
				target_cb->stop_cb(data->target_config);
			}
		}

		/* DA has been updated */
		if (int_sl & SLV_INT_DA_UPD) {
			LOG_INF("%s: DA updated to 0x%02lx", dev->name,
				SLV_STATUS1_DA(sys_read32(config->base + SLV_STATUS1)));
			/* HJ could send a DISEC which would trigger the SLV_INT_EVENT_UP bit,
			 * but it's still expected to eventually send a DAA
			 */
			cdns_i3c_target_ibi_hj_complete(dev);
		}

		/* HJ complete and DA has been assigned */
		if (int_sl & SLV_INT_HJ_DONE) {
		}

		/* Controllership has been been given */
		if (int_sl & SLV_INT_MR_DONE) {
			/* TODO: implement support for controllership handoff */
		}

		/* EISC or DISEC has been received */
		if (int_sl & SLV_INT_EVENT_UP) {
		}

		/* sdr transfer aborted by controller */
		if (int_sl & SLV_INT_M_RD_ABORT) {
			/* TODO: consider flushing tx buffer? */
		}

		/* SLV SDR rx fifo underflow */
		if (int_sl & SLV_INT_SDR_RX_UNF) {
			LOG_ERR("%s: slave sdr rx buffer underflow", dev->name);
		}

		/* SLV SDR tx fifo overflow */
		if (int_sl & SLV_INT_SDR_TX_OVF) {
			LOG_ERR("%s: slave sdr tx buffer overflow,", dev->name);
		}

		sys_write32(int_sl, config->base + SLV_ICR);
	}
}

static void cdns_i3c_read_hw_cfg(const struct device *dev)
{
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_data *data = dev->data;

	uint32_t devid = sys_read32(config->base + DEV_ID);
	uint32_t revid = sys_read32(config->base + REV_ID);

	LOG_DBG("%s: Device info:\r\n"
		"  vid: 0x%03lX, pid: 0x%03lX\r\n"
		"  revision: major = %lu, minor = %lu\r\n"
		"  device ID: 0x%04X",
		dev->name, REV_ID_VID(revid), REV_ID_PID(revid), REV_ID_REV_MAJOR(revid),
		REV_ID_REV_MINOR(revid), devid);

	/*
	 * Depths are specified as number of words (32bit), convert to bytes
	 */
	uint32_t cfg0 = sys_read32(config->base + CONF_STATUS0);
	uint32_t cfg1 = sys_read32(config->base + CONF_STATUS1);

	data->hw_cfg.cmdr_mem_depth = CONF_STATUS0_CMDR_DEPTH(cfg0) * 4;
	data->hw_cfg.cmd_mem_depth = CONF_STATUS1_CMD_DEPTH(cfg1) * 4;
	data->hw_cfg.rx_mem_depth = CONF_STATUS1_RX_DEPTH(cfg1) * 4;
	data->hw_cfg.tx_mem_depth = CONF_STATUS1_TX_DEPTH(cfg1) * 4;
	data->hw_cfg.ibir_mem_depth = CONF_STATUS0_IBIR_DEPTH(cfg0) * 4;

	LOG_DBG("%s: FIFO info:\r\n"
		"  cmd_mem_depth = %u\r\n"
		"  cmdr_mem_depth = %u\r\n"
		"  rx_mem_depth = %u\r\n"
		"  tx_mem_depth = %u\r\n"
		"  ibir_mem_depth = %u",
		dev->name, data->hw_cfg.cmd_mem_depth, data->hw_cfg.cmdr_mem_depth,
		data->hw_cfg.rx_mem_depth, data->hw_cfg.tx_mem_depth, data->hw_cfg.ibir_mem_depth);

	/* Regardless of the cmd depth size we are limited by our cmd array length. */
	data->hw_cfg.cmd_mem_depth = MIN(data->hw_cfg.cmd_mem_depth, ARRAY_SIZE(data->xfer.cmds));
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
static int cdns_i3c_config_get(const struct device *dev, enum i3c_config_type type, void *config)
{
	struct cdns_i3c_data *data = dev->data;
	int ret = 0;

	if (config == NULL) {
		ret = -EINVAL;
		goto out_configure;
	}

	(void)memcpy(config, &data->ctrl_config, sizeof(data->ctrl_config));

out_configure:
	return ret;
}

/**
 * @brief Writes to the Target's TX FIFO
 *
 * The Cadence I3C will then ACK read requests to it's TX FIFO from a
 * Controller
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
static int cdns_i3c_target_tx_write(const struct device *dev, uint8_t *buf, uint16_t len)
{
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_data *data = dev->data;

	/* check if we are currently a target */
	if (sys_read32(config->base + MST_STATUS0) & MST_STATUS0_MASTER_MODE) {
		return -EACCES;
	}

	/* check if there is space available in the tx fifo */
	if (sys_read32(config->base + SLV_STATUS1) & SLV_STATUS1_SDR_TX_FULL) {
		return -ENOSPC;
	}

	k_mutex_lock(&data->bus_lock, K_FOREVER);

	/* write as much as you can to the fifo */
	uint16_t i;

	for (i = 0;
	     i < len && (!(sys_read32(config->base + SLV_STATUS1) & SLV_STATUS1_SDR_TX_FULL));
	     i++) {
		sys_write32((uint32_t)buf[i], config->base + TX_FIFO);
	}

	/* setup THR interrupt */
	uint32_t thr_ctrl = sys_read32(config->base + TX_RX_THR_CTRL);

	/* TODO: investigate if setting to THR to 1 is good enough */
	thr_ctrl &= ~TX_THR_MASK;
	thr_ctrl |= TX_THR(MIN((data->hw_cfg.tx_mem_depth / 4) - 1, i - 1));
	sys_write32(thr_ctrl, config->base + TX_RX_THR_CTRL);

	k_mutex_unlock(&data->bus_lock);

	/* return total bytes written */
	return i;
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
static int cdns_i3c_target_register(const struct device *dev, struct i3c_target_config *cfg)
{
	struct cdns_i3c_data *data = dev->data;

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
static int cdns_i3c_target_unregister(const struct device *dev, struct i3c_target_config *cfg)
{
	/* no way to disable? maybe write DA to 0? */
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
static struct i3c_device_desc *cdns_i3c_device_find(const struct device *dev,
						    const struct i3c_device_id *id)
{
	const struct cdns_i3c_config *config = dev->config;

	return i3c_dev_list_find(&config->device_list, id);
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
static struct i3c_i2c_device_desc *cdns_i3c_i2c_device_find(const struct device *dev, uint16_t addr)
{
	const struct cdns_i3c_config *config = dev->config;

	return i3c_dev_list_i2c_addr_find(&config->device_list, addr);
}

/**
 * @brief Transfer messages in I2C mode.
 *
 * @see i2c_transfer
 *
 * @param dev Pointer to device driver instance.
 * @param target Pointer to target device descriptor.
 * @param msgs Pointer to I2C messages.
 * @param num_msgs Number of messages to transfers.
 *
 * @return @see i2c_transfer
 */
static int cdns_i3c_i2c_api_transfer(const struct device *dev, struct i2c_msg *msgs,
				     uint8_t num_msgs, uint16_t addr)
{
	struct i3c_i2c_device_desc *i2c_dev = cdns_i3c_i2c_device_find(dev, addr);
	int ret;

	if (i2c_dev == NULL) {
		ret = -ENODEV;
	} else {
		ret = cdns_i3c_i2c_transfer(dev, i2c_dev, msgs, num_msgs);
	}

	return ret;
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
		switch (I3C_DCR_I2C_DEV_IDX(dev_list->i2c[i].lvr)) {
		case I3C_DCR_I2C_DEV_IDX_0:
			if (mode < I3C_BUS_MODE_MIXED_FAST) {
				mode = I3C_BUS_MODE_MIXED_FAST;
			}
			break;
		case I3C_DCR_I2C_DEV_IDX_1:
			if (mode < I3C_BUS_MODE_MIXED_LIMITED) {
				mode = I3C_BUS_MODE_MIXED_LIMITED;
			}
			break;
		case I3C_DCR_I2C_DEV_IDX_2:
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

/**
 * Determine THD_DEL value for CTRL register
 *
 * @param dev Pointer to device driver instance.
 *
 * @return Value to be written to THD_DEL
 */
static uint8_t cdns_i3c_clk_to_data_turnaround(const struct device *dev)
{
	const struct cdns_i3c_config *config = dev->config;
	uint32_t input_clock_frequency = config->input_frequency;
	uint8_t thd_delay =
		DIV_ROUND_UP(I3C_TSCO_DEFAULT_NS, (NSEC_PER_SEC / input_clock_frequency));

	if (thd_delay > THD_DELAY_MAX) {
		thd_delay = THD_DELAY_MAX;
	}

	return (THD_DELAY_MAX - thd_delay);
}

/**
 * @brief Initialize the hardware.
 *
 * @param dev Pointer to controller device driver instance.
 */
static int cdns_i3c_bus_init(const struct device *dev)
{
	struct cdns_i3c_data *data = dev->data;
	const struct cdns_i3c_config *config = dev->config;
	struct i3c_config_controller *ctrl_config = &data->ctrl_config;

	int ret = i3c_addr_slots_init(&data->addr_slots, &config->device_list);

	if (ret != 0) {
		return ret;
	}

	uint32_t conf0 = sys_read32(config->base + CONF_STATUS0);

	data->max_devs = CONF_STATUS0_DEVS_NUM(conf0);
	data->free_rr_slots = GENMASK(data->max_devs, 1);
	ctrl_config->is_secondary = (conf0 & CONF_STATUS0_SEC_MASTER) ? true : false;
	ctrl_config->supported_hdr = (conf0 & CONF_STATUS0_SUPPORTS_DDR) ? I3C_MSG_HDR_DDR : 0;

	k_mutex_init(&data->bus_lock);
	k_sem_init(&data->xfer.complete, 0, 1);
	k_sem_init(&data->ibi_hj_complete, 0, 1);

	cdns_i3c_interrupts_disable(config);
	cdns_i3c_interrupts_clear(config);

	config->irq_config_func(dev);

	/* Ensure the bus is disabled. */
	sys_write32(~CTRL_DEV_EN & sys_read32(config->base + CTRL), config->base + CTRL);

	cdns_i3c_read_hw_cfg(dev);

	/* determine prescaler timings for i3c and i2c scl */
	cdns_i3c_set_prescalers(dev);

	enum i3c_bus_mode mode = i3c_bus_mode(&config->device_list);

	LOG_DBG("%s: i3c bus mode %d", dev->name, mode);
	int cdns_mode;

	switch (mode) {
	case I3C_BUS_MODE_PURE:
		cdns_mode = CTRL_PURE_BUS_MODE;
		break;
	case I3C_BUS_MODE_MIXED_FAST:
		cdns_mode = CTRL_MIXED_FAST_BUS_MODE;
		break;
	case I3C_BUS_MODE_MIXED_LIMITED:
	case I3C_BUS_MODE_MIXED_SLOW:
		cdns_mode = CTRL_MIXED_SLOW_BUS_MODE;
		break;
	default:
		return -EINVAL;
	}

	/*
	 * When a Hot-Join request happens, disable all events coming from this device.
	 * We will issue ENTDAA afterwards from the threaded IRQ handler.
	 * Set HJ ACK later after bus init to prevent targets from indirect DAA enforcement.
	 *
	 * Set the I3C Bus Mode based on the LVR of the I2C devices
	 */
	uint32_t ctrl = CTRL_HJ_DISEC | CTRL_MCS_EN | (CTRL_BUS_MODE_MASK & cdns_mode) |
			CTRL_THD_DELAY(cdns_i3c_clk_to_data_turnaround(dev));
	/* Disable Controllership requests as it is not supported yet by the driver */
	ctrl &= ~CTRL_MST_ACK;

	/*
	 * Cadence I3C release r105v1p0 and above support I3C v1.1 timing change
	 * for tCASHr_min = tCAS_min / 2, otherwise tCASr_min = tCAS_min (as
	 * per MIPI spec v1.0)
	 */
	uint32_t rev_id = sys_read32(config->base + REV_ID);

	if (REV_ID_REV(rev_id) >= REV_ID_VERSION(1, 5)) {
		ctrl |= CTRL_I3C_11_SUPP;
	}

	/* write ctrl register value */
	sys_write32(ctrl, config->base + CTRL);

	/* enable Core */
	sys_write32(CTRL_DEV_EN | ctrl, config->base + CTRL);

	/* Set fifo thresholds. */
	sys_write32(CMD_THR(I3C_CMDD_THR) | IBI_THR(I3C_IBID_THR) | CMDR_THR(I3C_CMDR_THR) |
			    IBIR_THR(I3C_IBIR_THR),
		    config->base + CMD_IBI_THR_CTRL);

	/* Set TX/RX interrupt thresholds. */
	if (sys_read32(config->base + MST_STATUS0) & MST_STATUS0_MASTER_MODE) {
		sys_write32(TX_THR(I3C_TX_THR) | RX_THR(data->hw_cfg.rx_mem_depth),
			    config->base + TX_RX_THR_CTRL);
	} else {
		sys_write32(TX_THR(1) | RX_THR(1), config->base + TX_RX_THR_CTRL);
	}

	/* enable target interrupts */
	sys_write32(SLV_INT_DA_UPD | SLV_INT_SDR_RD_COMP | SLV_INT_SDR_WR_COMP |
			    SLV_INT_SDR_RX_THR | SLV_INT_SDR_TX_THR | SLV_INT_SDR_RX_UNF |
			    SLV_INT_SDR_TX_OVF | SLV_INT_HJ_DONE,
		    config->base + SLV_IER);

	/* Enable IBI interrupts. */
	sys_write32(MST_INT_IBIR_THR | MST_INT_RX_UNF | MST_INT_HALTED | MST_INT_TX_OVF,
		    config->base + MST_IER);

	/* attach i3c devices */
	for (int i = 0; i < config->device_list.num_i3c; i++) {
		cdns_i3c_attach_device(dev, &config->device_list.i3c[i]);
	}
	/* attach i2c devices */
	for (int i = 0; i < config->device_list.num_i2c; i++) {
		cdns_i3c_i2c_attach_device(dev, &config->device_list.i2c[i]);
	}

	/* Program retaining regs. */
	cdns_i3c_program_retaining_regs(dev);

	/* only primary controllers are responsible for initializing the bus */
	if (!ctrl_config->is_secondary) {
		/* Perform bus initialization */
		ret = i3c_bus_init(dev, &config->device_list);
		/* Bus Initialization Complete, allow HJ ACKs */
		sys_write32(CTRL_HJ_ACK | sys_read32(config->base + CTRL), config->base + CTRL);
	}

	return 0;
}

static struct i3c_driver_api api = {
	.i2c_api.configure = cdns_i3c_i2c_api_configure,
	.i2c_api.transfer = cdns_i3c_i2c_api_transfer,

	.configure = cdns_i3c_configure,
	.config_get = cdns_i3c_config_get,

	.do_daa = cdns_i3c_do_daa,
	.do_ccc = cdns_i3c_do_ccc,

	.i3c_device_find = cdns_i3c_device_find,

	.i3c_xfers = cdns_i3c_transfer,

	.target_tx_write = cdns_i3c_target_tx_write,
	.target_register = cdns_i3c_target_register,
	.target_unregister = cdns_i3c_target_unregister,

#ifdef CONFIG_I3C_USE_IBI
	.ibi_enable = cdns_i3c_controller_ibi_enable,
	.ibi_disable = cdns_i3c_controller_ibi_disable,
	.ibi_raise = cdns_i3c_target_ibi_raise,
#endif
};

#define CADENCE_I3C_INSTANTIATE(n)                                                                 \
	static void cdns_i3c_config_func_##n(const struct device *dev);                            \
	static struct i3c_device_desc cdns_i3c_device_array_##n[] = I3C_DEVICE_ARRAY_DT_INST(n);   \
	static struct i3c_i2c_device_desc cdns_i3c_i2c_device_array_##n[] =                        \
		I3C_I2C_DEVICE_ARRAY_DT_INST(n);                                                   \
	static const struct cdns_i3c_config i3c_config_##n = {                                     \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.input_frequency = DT_INST_PROP(n, input_clock_frequency),                         \
		.irq_config_func = cdns_i3c_config_func_##n,                                       \
		.device_list.i3c = cdns_i3c_device_array_##n,                                      \
		.device_list.num_i3c = ARRAY_SIZE(cdns_i3c_device_array_##n),                      \
		.device_list.i2c = cdns_i3c_i2c_device_array_##n,                                  \
		.device_list.num_i2c = ARRAY_SIZE(cdns_i3c_i2c_device_array_##n),                  \
	};                                                                                         \
	static struct cdns_i3c_data i3c_data_##n = {                                               \
		.ctrl_config.scl.i3c = DT_INST_PROP_OR(n, i3c_scl_hz, 0),                          \
		.ctrl_config.scl.i2c = DT_INST_PROP_OR(n, i2c_scl_hz, 0),                          \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, cdns_i3c_bus_init, NULL, &i3c_data_##n, &i3c_config_##n,          \
			      POST_KERNEL, CONFIG_I3C_CONTROLLER_INIT_PRIORITY, &api);             \
	static void cdns_i3c_config_func_##n(const struct device *dev)                             \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), cdns_i3c_irq_handler,       \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	};

#define DT_DRV_COMPAT cdns_i3c
DT_INST_FOREACH_STATUS_OKAY(CADENCE_I3C_INSTANTIATE)
