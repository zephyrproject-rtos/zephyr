/* ieee802154_cc1200.h - Registers definition for TI CC1200 */

/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_IEEE802154_IEEE802154_CC1200_H_
#define ZEPHYR_DRIVERS_IEEE802154_IEEE802154_CC1200_H_

#include <linker/sections.h>
#include <sys/atomic.h>
#include <drivers/spi.h>

#include <ieee802154/cc1200.h>

/* Runtime context structure
 ***************************
 */

struct cc1200_context {
	struct net_if *iface;
	/**************************/
	struct cc1200_gpio_configuration *gpios;
	struct gpio_callback rx_tx_cb;
	struct device *spi;
	struct spi_config spi_cfg;
	u8_t mac_addr[8];
	/************RF************/
	const struct cc1200_rf_registers_set *rf_settings;
	/************TX************/
	struct k_sem tx_sync;
	atomic_t tx;
	atomic_t tx_start;
	/************RX************/
	K_THREAD_STACK_MEMBER(rx_stack,
			      CONFIG_IEEE802154_CC1200_RX_STACK_SIZE);
	struct k_thread rx_thread;
	struct k_sem rx_lock;
	atomic_t rx;
};

#include "ieee802154_cc1200_regs.h"

/* Registers useful routines
 ***************************
 */

bool z_cc1200_access_reg(struct cc1200_context *ctx, bool read, u8_t addr,
			void *data, size_t length, bool extended, bool burst);

static inline u8_t cc1200_read_single_reg(struct cc1200_context *ctx,
					   u8_t addr, bool extended)
{
	u8_t val;

	if (z_cc1200_access_reg(ctx, true, addr, &val, 1, extended, false)) {
		return val;
	}

	return 0;
}

static inline bool cc1200_write_single_reg(struct cc1200_context *ctx,
					    u8_t addr, u8_t val, bool extended)
{
	return z_cc1200_access_reg(ctx, false, addr, &val, 1, extended, false);
}

static inline bool cc1200_instruct(struct cc1200_context *ctx, u8_t addr)
{
	return z_cc1200_access_reg(ctx, false, addr, NULL, 0, false, false);
}

#define DEFINE_REG_READ(__reg_name, __reg_addr, __ext)			\
	static inline u8_t read_reg_##__reg_name(struct cc1200_context *ctx) \
	{								\
		return cc1200_read_single_reg(ctx, __reg_addr, __ext);	\
	}

#define DEFINE_REG_WRITE(__reg_name, __reg_addr, __ext)			\
	static inline bool write_reg_##__reg_name(struct cc1200_context *ctx, \
						  u8_t val)		\
	{								\
		return cc1200_write_single_reg(ctx, __reg_addr,	\
						val, __ext);		\
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

#define DEFINE_STROBE_INSTRUCTION(__ins_name, __ins_addr)		\
	static inline bool instruct_##__ins_name(struct cc1200_context *ctx) \
	{								\
		return cc1200_instruct(ctx, __ins_addr);		\
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

#endif /* ZEPHYR_DRIVERS_IEEE802154_IEEE802154_CC1200_H_ */
