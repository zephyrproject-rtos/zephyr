/* ieee802154_cc1200.h - Registers definition for TI CC1200 */

/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_IEEE802154_IEEE802154_CC1200_H_
#define ZEPHYR_DRIVERS_IEEE802154_IEEE802154_CC1200_H_

#include <zephyr/linker/sections.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

#include <zephyr/drivers/ieee802154/cc1200.h>

/* Compile time config structure
 *******************************
 */

/* Note for EMK & EM adapter booster pack users:
 * SPI pins are easy, RESET as well, but when it comes to GPIO:
 * CHIP -> EM adapter
 * GPIO0 -> GPIOA
 * GPIO1 -> reserved (it's SPI MISO)
 * GPIO2 -> GPIOB
 * GPIO3 -> GPIO3
 */
struct cc1200_config {
	struct spi_dt_spec bus;
	struct gpio_dt_spec interrupt;
};

/* Runtime context structure
 ***************************
 */

struct cc1200_context {
	struct net_if *iface;
	/**************************/
	struct gpio_callback rx_tx_cb;
	uint8_t mac_addr[8];
	/************RF************/
	const struct cc1200_rf_registers_set *rf_settings;
	/************TX************/
	struct k_sem tx_sync;
	atomic_t tx;
	atomic_t tx_start;
	/************RX************/
	K_KERNEL_STACK_MEMBER(rx_stack,
			      CONFIG_IEEE802154_CC1200_RX_STACK_SIZE);
	struct k_thread rx_thread;
	struct k_sem rx_lock;
	atomic_t rx;
};

#include "ieee802154_cc1200_regs.h"

/* Registers useful routines
 ***************************
 */

bool z_cc1200_access_reg(const struct device *dev, bool read, uint8_t addr,
			 void *data, size_t length, bool extended, bool burst);

static inline uint8_t cc1200_read_single_reg(const struct device *dev,
					     uint8_t addr, bool extended)
{
	uint8_t val;

	if (z_cc1200_access_reg(dev, true, addr, &val, 1, extended, false)) {
		return val;
	}

	return 0;
}

static inline bool cc1200_write_single_reg(const struct device *dev,
					   uint8_t addr, uint8_t val, bool extended)
{
	return z_cc1200_access_reg(dev, false, addr, &val, 1, extended, false);
}

static inline bool cc1200_instruct(const struct device *dev, uint8_t addr)
{
	return z_cc1200_access_reg(dev, false, addr, NULL, 0, false, false);
}

#define DEFINE_REG_READ(__reg_name, __reg_addr, __ext)			      \
	static inline uint8_t read_reg_##__reg_name(const struct device *dev) \
	{								      \
		return cc1200_read_single_reg(dev, __reg_addr, __ext);	      \
	}

#define DEFINE_REG_WRITE(__reg_name, __reg_addr, __ext)			    \
	static inline bool write_reg_##__reg_name(const struct device *dev, \
						  uint8_t val)		    \
	{								    \
		return cc1200_write_single_reg(dev, __reg_addr,		    \
					       val, __ext);		    \
	}

DEFINE_REG_WRITE(iocfg3, CC1200_REG_IOCFG3, false)
DEFINE_REG_WRITE(iocfg2, CC1200_REG_IOCFG2, false)
DEFINE_REG_WRITE(iocfg0, CC1200_REG_IOCFG0, false)
DEFINE_REG_WRITE(pa_cfg1, CC1200_REG_PA_CFG1, false)
DEFINE_REG_WRITE(pkt_len, CC1200_REG_PKT_LEN, false)

DEFINE_REG_READ(fs_cfg, CC1200_REG_FS_CFG, false)
DEFINE_REG_READ(rssi0, CC1200_REG_RSSI0, true)
DEFINE_REG_READ(pa_cfg1, CC1200_REG_PA_CFG1, false)
DEFINE_REG_READ(num_txbytes, CC1200_REG_NUM_TXBYTES, true)
DEFINE_REG_READ(num_rxbytes, CC1200_REG_NUM_RXBYTES, true)


/* Instructions useful routines
 ******************************
 */

#define DEFINE_STROBE_INSTRUCTION(__ins_name, __ins_addr)		   \
	static inline bool instruct_##__ins_name(const struct device *dev) \
	{								   \
		return cc1200_instruct(dev, __ins_addr);		   \
	}

DEFINE_STROBE_INSTRUCTION(sres, CC1200_INS_SRES)
DEFINE_STROBE_INSTRUCTION(sfstxon, CC1200_INS_SFSTXON)
DEFINE_STROBE_INSTRUCTION(sxoff, CC1200_INS_SXOFF)
DEFINE_STROBE_INSTRUCTION(scal, CC1200_INS_SCAL)
DEFINE_STROBE_INSTRUCTION(srx, CC1200_INS_SRX)
DEFINE_STROBE_INSTRUCTION(stx, CC1200_INS_STX)
DEFINE_STROBE_INSTRUCTION(sidle, CC1200_INS_SIDLE)
DEFINE_STROBE_INSTRUCTION(safc, CC1200_INS_SAFC)
DEFINE_STROBE_INSTRUCTION(swor, CC1200_INS_SWOR)
DEFINE_STROBE_INSTRUCTION(spwd, CC1200_INS_SPWD)
DEFINE_STROBE_INSTRUCTION(sfrx, CC1200_INS_SFRX)
DEFINE_STROBE_INSTRUCTION(sftx, CC1200_INS_SFTX)
DEFINE_STROBE_INSTRUCTION(sworrst, CC1200_INS_SWORRST)
DEFINE_STROBE_INSTRUCTION(snop, CC1200_INS_SNOP)

#define CC1200_INVALID_RSSI INT8_MIN

#endif /* ZEPHYR_DRIVERS_IEEE802154_IEEE802154_CC1200_H_ */
