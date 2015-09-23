/* dw_i2c.h - header for Design Ware I2C operations */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __DRIVERS_I2C_DW_H
#define __DRIVERS_I2C_DW_H

#include <i2c.h>
#include <stdbool.h>

#ifdef CONFIG_PCI
#include <pci/pci.h>
#include <pci/pci_mgr.h>
#endif /* CONFIG_PCI */

#define I2C_DW_MAGIC_KEY			0x44570140


typedef void (*i2c_isr_cb_t)(struct device *port);


#define IC_ACTIVITY                     (1 << 0)
#define IC_ENABLE_BIT                   (1 << 0)


/* dev->state values from IC_DATA_CMD Data transfer mode settings (bit 8) */
#define I2C_DW_STATE_READY                 (0)
#define I2C_DW_CMD_SEND                    (1 << 0)
#define I2C_DW_CMD_RECV                    (1 << 1)
#define I2C_DW_CMD_ERROR                   (1 << 2)


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
#define I2C_STD_HCNT			(CONFIG_I2C_CLOCK_SPEED * 4)
#define I2C_STD_LCNT			(CONFIG_I2C_CLOCK_SPEED * 5)
#define I2C_FS_HCNT			((CONFIG_I2C_CLOCK_SPEED * 6) / 8)
#define I2C_FS_LCNT			((CONFIG_I2C_CLOCK_SPEED * 7) / 8)
#define I2C_HS_HCNT			((CONFIG_I2C_CLOCK_SPEED * 6) / 8)
#define I2C_HS_LCNT			((CONFIG_I2C_CLOCK_SPEED * 7) / 8)

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
	uint32_t        base_address;
	uint32_t        interrupt_vector;
	uint32_t        interrupt_mask;
#ifdef CONFIG_PCI
	struct pci_dev_info pci_dev;
#endif /* CONFIG_PCI */
	i2c_isr_cb_t	config_func;
};


struct i2c_dw_dev_config {
	union dev_config	app_config;

	volatile uint8_t	state;  /* last direction of transfer */
	uint8_t			slave_mode;
	uint8_t			rx_len;
	uint8_t			*rx_buffer;
	uint8_t			tx_len;
	uint8_t			*tx_buffer;
	uint8_t			rx_tx_len;

	bool			support_hs_mode;
	uint16_t		hcnt;
	uint16_t		lcnt;
};

void i2c_dw_isr(struct device *port);

extern int i2c_dw_initialize(struct device *port);

#endif /* __DRIVERS_I2C_DW_H */
