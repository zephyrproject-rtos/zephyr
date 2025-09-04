/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_I3C_I3C_NCT_H_
#define ZEPHYR_DRIVERS_I3C_I3C_NCT_H_

#include <zephyr/toolchain.h>
#include <zephyr/types.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/gpio.h>

/* Support 8 - 4095 bytes */
#define MAX_I3C_PAYLOAD_SIZE	(MAX_I3C_DATA_SIZE + 0)
#define MAX_I3C_DATA_SIZE		256							// 252 + 1 pec, or 256 

/* MIPI I3C MDB definition: see https://www.mipi.org/MIPI_I3C_mandatory_data_byte_values_public */
#define IBI_MDB_ID(grp, id)		((((grp) << 5) & GENMASK(7, 5)) | ((id) & GENMASK(4, 0)))
#define IBI_MDB_GET_GRP(m)		(((m) & GENMASK(7, 5)) >> 5)
#define IBI_MDB_GET_ID(m)		((m) & GENMASK(4, 0))

#define IBI_MDB_GRP_PENDING_READ_NOTIF	0x5
#define IS_MDB_PENDING_READ_NOTIFY(m)	(IBI_MDB_GET_GRP(m) == IBI_MDB_GRP_PENDING_READ_NOTIF)
#define IBI_MDB_MIPI_DBGDATAREADY		IBI_MDB_ID(IBI_MDB_GRP_PENDING_READ_NOTIF, 0xd)
#define IBI_MDB_MCTP					IBI_MDB_ID(IBI_MDB_GRP_PENDING_READ_NOTIF, 0xe)
/* Interrupt ID 0x10 to 0x1F are for vendor specific */

/**
 * @brief IBI callback function structure
 * @param write_requested callback function to return a memory block for receiving IBI data
 * @param write_done callback function to process the received IBI data
 */
struct i3c_ibi_callbacks {
	struct i3c_ibi_payload* (*write_requested)(struct i3c_device_desc *i3cdev);
	void (*write_done)(struct i3c_device_desc *i3cdev);
};

/* slave driver structure */
struct i3c_slave_payload {
	int size;
	void *buf;
};

/**
 * @brief slave callback function structure
 * @param write_requested callback function to return a memory block for receiving data sent from
 *                        the master device
 * @param write_done callback function to process the received IBI data
 */
struct i3c_slave_callbacks {
	struct i3c_slave_payload* (*write_requested)(const struct device *dev);
	void (*write_done)(const struct device *dev);
};

struct i3c_slave_setup {
	int max_payload_len;
	const struct device *dev;
	const struct i3c_slave_callbacks *callbacks;
};

struct i3c_nct_ibi_priv {
	int pos;
	struct {
		int enable;
		struct i3c_ibi_callbacks *callbacks;
		struct i3c_device_desc *context;
		struct i3c_ibi_payload *incomplete;
	} ibi;
};

/* slave events */
#define I3C_SLAVE_EVENT_SIR		BIT(0)
#define I3C_SLAVE_EVENT_MR		BIT(1)
#define I3C_SLAVE_EVENT_HJ		BIT(2)

// i3c APIs defined by ASPEED in v2.6, but changed to i3c.h by intel from v3.2
//#define i3c_master_attach_device		i3c_nct_master_attach_device
//#define i3c_master_detach_device		i3c_nct_master_detach_device
//#define i3c_master_send_ccc				i3c_nct_master_send_ccc
//#define i3c_master_priv_xfer			i3c_nct_master_priv_xfer
#define i3c_master_request_ibi			i3c_nct_master_request_ibi
//#define i3c_master_enable_ibi			i3c_nct_master_enable_ibi
//#define i3c_master_send_entdaa			i3c_nct_master_send_entdaa
#define i3c_slave_register				i3c_nct_slave_register
//#define i3c_slave_send_sir				i3c_npcm4xx_slave_send_sir
#define i3c_slave_put_read_data			i3c_nct_slave_put_read_data
#define i3c_slave_get_dynamic_addr		i3c_nct_slave_get_dynamic_addr
#define i3c_slave_get_event_enabling	i3c_nct_slave_get_event_enabling

int i3c_nct_master_request_ibi(struct i3c_device_desc *i3cdev, struct i3c_ibi_callbacks *cb);
int i3c_nct_slave_register(const struct device *dev, struct i3c_slave_setup *slave_data);
int i3c_nct_slave_put_read_data(const struct device *dev, struct i3c_slave_payload *data,
				   struct i3c_ibi_payload *ibi_notify);
int i3c_nct_slave_get_dynamic_addr(const struct device *dev, uint8_t *dynamic_addr);
int i3c_nct_slave_get_event_enabling(const struct device *dev, uint32_t *event_en);
/*************************************************************************************************/

/* Operation type */
enum nct_i3c_oper_state {
	I3C_OP_STATE_IDLE,
	I3C_OP_STATE_WR,
	I3C_OP_STATE_RD,
	I3C_OP_STATE_IBI,
	I3C_OP_STATE_CCC,
	I3C_OP_STATE_MAX,
};

/* I3C timing configuration for each i3c/i2c speed */
struct nct_i3c_timing_cfg {
	uint8_t ppbaud;   /* Push-Pull high period */
	uint8_t pplow;    /* Push-Pull low period */
	uint8_t odhpp;    /* Open-Drain high period */
	uint8_t odbaud;   /* Open-Drain low period */
	uint8_t i2c_baud; /* I2C period */
};

/* NCT I3C Configuration */
struct nct_i3c_config {
	/* Common I3C Driver Config */
	struct i3c_driver_config common;

	/* Pointer to controller registers. */
	struct i3c_reg *base;

	/* Clock control subsys related struct. */
	uint32_t clk_cfg;

	/* Pointer to pin control device. */
	const struct pinctrl_dev_config *pincfg;

	/* Interrupt configuration function. */
	void (*irq_config_func)(const struct device *dev);

	struct {
		uint32_t i3c_pp_scl_hz; /* I3C push pull clock frequency in Hz. */
		uint32_t i3c_od_scl_hz; /* I3C open drain clock frequency in Hz. */
		uint32_t i2c_scl_hz;    /* I2C clock frequency in Hz */
	} clocks;

	/* support pec */
	bool priv_xfer_pec;
	bool ibi_append_pec;

#ifdef CONFIG_I3C_NCT_DMA
	struct pdma_dsct_reg *pdma_rx;
	struct pdma_dsct_reg *pdma_tx;
#endif
};

/* NCT I3C Data */
struct nct_i3c_data {
	struct i3c_driver_data common; /* Common i3c driver data */
	struct k_mutex lock_mutex;     /* Mutex of i3c controller */
	struct k_sem sync_sem;         /* Semaphore used for synchronization */
	struct k_sem ibi_lock_sem;     /* Semaphore used for ibi */

	/* Target data */
	struct i3c_target_config *target_config;
	/* Configuration parameters for I3C hardware to act as target device */
	struct i3c_config_target config_target;
	struct k_sem target_lock_sem;       /* Semaphore used for i3c target */
	struct k_sem target_event_lock_sem; /* Semaphore used for i3c target ibi_raise() */

	enum nct_i3c_oper_state oper_state; /* Operation state */

#ifdef CONFIG_I3C_NCT_DMA
	uint8_t dma_rx_buf[MAX_I3C_PAYLOAD_SIZE];
	uint16_t dma_rx_len;
	uint16_t dma_triggered; /* The bit n is set to 1 if the n-th DMA channel is triggered */
	bool tx_valid;          /* Tx has valid data */

	uint8_t pdma_rx_buf[2][MAX_I3C_PAYLOAD_SIZE];
	struct i3c_slave_payload slave_rx_payload[2];
	struct i3c_slave_payload *rx_payload_curr;
	int rx_payload_in;
	int rx_payload_out;

#else
	uint8_t rx_buf[MAX_I3C_PAYLOAD_SIZE];
	uint16_t rx_len;
	uint8_t *tx_buf;
	uint16_t tx_len;
#endif

#ifdef CONFIG_I3C_USE_IBI
	struct {
		/* List of addresses used in the MIBIRULES register. */
		uint8_t addr[5];

		/* Number of valid addresses in MIBIRULES. */
		uint8_t num_addr;

		/* True if all addresses have MSB set. */
		bool msb;

		/* True if all target devices require mandatory byte for IBI. */
		bool has_mandatory_byte;
	} ibi;
#endif

#ifdef CONFIG_I3C_NCT_DMA
	struct pdma_dsct_reg dsct_sg[4] __aligned(4); /* use for dma, 4-bytes align */
#endif

	// for v2.6
	struct i3c_slave_setup slave_data;
	struct i3c_slave_payload *rx_payload;	
};



#endif /* ZEPHYR_DRIVERS_I3C_I3C_NCT_H_ */
