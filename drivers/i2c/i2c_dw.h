/* dw_i2c.h - header for Design Ware I2C operations */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __DRIVERS_I2C_DW_H
#define __DRIVERS_I2C_DW_H

#include <i2c.h>
#include <stdbool.h>

#ifdef CONFIG_PCI
#include <pci/pci.h>
#include <pci/pci_mgr.h>
#endif /* CONFIG_PCI */

#ifdef __cplusplus
extern "C" {
#endif

#define I2C_DW_MAGIC_KEY			0x44570140


typedef void (*i2c_isr_cb_t)(struct device *port);


#define IC_ACTIVITY                     (1 << 0)
#define IC_ENABLE_BIT                   (1 << 0)


/* dev->state values from IC_DATA_CMD Data transfer mode settings (bit 8) */
#define I2C_DW_STATE_READY                 (0)
#define I2C_DW_CMD_SEND                    (1 << 0)
#define I2C_DW_CMD_RECV                    (1 << 1)
#define I2C_DW_CMD_ERROR                   (1 << 2)
#define I2C_DW_BUSY                        (1 << 3)


#define DW_ENABLE_TX_INT_I2C_MASTER		(DW_INTR_STAT_TX_OVER |  \
						 DW_INTR_STAT_TX_EMPTY | \
						 DW_INTR_STAT_TX_ABRT |  \
						 DW_INTR_STAT_STOP_DET)
#define DW_ENABLE_RX_INT_I2C_MASTER		(DW_INTR_STAT_RX_UNDER | \
						 DW_INTR_STAT_RX_OVER |  \
						 DW_INTR_STAT_RX_FULL | \
						 DW_INTR_STAT_STOP_DET)

#define DW_ENABLE_TX_INT_I2C_SLAVE		(DW_INTR_STAT_RD_REQ | \
						 DW_INTR_STAT_TX_ABRT | \
						 DW_INTR_STAT_STOP_DET)
#define DW_ENABLE_RX_INT_I2C_SLAVE		(DW_INTR_STAT_RX_FULL | \
						 DW_INTR_STAT_STOP_DET)

#define DW_DISABLE_ALL_I2C_INT		0x00000000


/* IC_CON Low count and high count default values */
/* TODO verify values for high and fast speed */
#define I2C_STD_HCNT			(CONFIG_I2C_DW_CLOCK_SPEED * 4)
#define I2C_STD_LCNT			(CONFIG_I2C_DW_CLOCK_SPEED * 5)
#define I2C_FS_HCNT			((CONFIG_I2C_DW_CLOCK_SPEED * 6) / 8)
#define I2C_FS_LCNT			((CONFIG_I2C_DW_CLOCK_SPEED * 7) / 8)
#define I2C_HS_HCNT			((CONFIG_I2C_DW_CLOCK_SPEED * 6) / 8)
#define I2C_HS_LCNT			((CONFIG_I2C_DW_CLOCK_SPEED * 7) / 8)

/*
 * DesignWare speed values don't directly translate from the Zephyr speed
 * selections in include/i2c.h so here we do a little translation
 */
#define I2C_DW_SPEED_STANDARD		0x1
#define I2C_DW_SPEED_FAST		0x2
#define I2C_DW_SPEED_FAST_PLUS		0x2
#define I2C_DW_SPEED_HIGH		0x3


/*
 * These values have been randomly selected.  It would be good to test different
 * watermark levels for performance capabilities
 */
#define I2C_DW_TX_WATERMARK		2
#define I2C_DW_RX_WATERMARK		7
#define I2C_DW_FIFO_DEPTH		16


struct i2c_dw_rom_config {
	u32_t	irq_num;
	u32_t        interrupt_mask;
	i2c_isr_cb_t	config_func;

#ifdef CONFIG_I2C_DW_SHARED_IRQ
	char *shared_irq_dev_name;
#endif /* CONFIG_I2C_DW_SHARED_IRQ */
};


struct i2c_dw_dev_config {
	u32_t base_address;
	struct k_sem		device_sync_sem;
	u32_t app_config;


	u8_t			*xfr_buf;
	u32_t		xfr_len;
	u32_t		rx_pending;

	u16_t		hcnt;
	u16_t		lcnt;

	volatile u8_t	state;  /* last direction of transfer */
	u8_t			request_bytes;
	u8_t			xfr_flags;
	bool			support_hs_mode;
#ifdef CONFIG_PCI
	struct pci_dev_info pci_dev;
#endif /* CONFIG_PCI */
};

#ifdef __cplusplus
}
#endif

#endif /* __DRIVERS_I2C_DW_H */
