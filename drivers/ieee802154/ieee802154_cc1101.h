/* ieee802154_cc1101.h - Registers definition for TI CC1101 */

/*
 * Copyright (c) 2018 Intel Corporation.
 * Copyright (c) 2018 Matthias Boesl.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __IEEE802154_CC1101_H__
#define __IEEE802154_CC1101_H__

#include <linker/sections.h>
#include <atomic.h>
#include <spi.h>

#include <ieee802154/cc1101.h>

/* Runtime context structure
 ***************************
 */

struct cc1101_context {
	struct net_if *iface;
	/**************************/
	struct cc1101_gpio_configuration *gpios;
	struct gpio_callback rx_tx_cb;
	struct device *spi;
	struct spi_config spi_cfg;
	u8_t mac_addr[8];
	/************RF************/
	const struct cc1101_rf_registers_set *rf_settings;
	/************TX************/
	struct k_sem tx_sync;
	atomic_t tx;
	atomic_t tx_start;
	/************RX************/
	K_THREAD_STACK_MEMBER(rx_stack,
			      CONFIG_IEEE802154_CC1101_RX_STACK_SIZE);
	struct k_thread rx_thread;
	struct k_sem rx_lock;
	atomic_t rx;
};

#include "ieee802154_cc1101_regs.h"

/* Registers useful routines
 ***************************
 */

bool _cc1101_access_reg(struct cc1101_context *ctx, bool read, u8_t addr,
			void *data, size_t length, bool burst);

static inline u8_t _cc1101_read_single_reg(struct cc1101_context *ctx,
					   u8_t addr)
{
	u8_t val;

	if (_cc1101_access_reg(ctx, true, addr, &val, 1, false)) {
		return val;
	}

	return 0;
}

static inline bool _cc1101_write_single_reg(struct cc1101_context *ctx,
					    u8_t addr, u8_t val)
{
	return _cc1101_access_reg(ctx, false, addr, &val, 1, false);
}

static inline bool _cc1101_instruct(struct cc1101_context *ctx, u8_t addr)
{
	return _cc1101_access_reg(ctx, false, addr, NULL, 0, false);
}

#define DEFINE_REG_READ(__reg_name, __reg_addr)				     \
	static inline u8_t read_reg_##__reg_name(struct cc1101_context *ctx) \
	{								     \
		/*SYS_LOG_DBG("");*/					     \
		return _cc1101_read_single_reg(ctx, __reg_addr);	     \
	}

#define DEFINE_REG_WRITE(__reg_name, __reg_addr)			      \
	static inline bool write_reg_##__reg_name(struct cc1101_context *ctx, \
						  u8_t val)		      \
	{								      \
		/*SYS_LOG_DBG("");*/					      \
		return _cc1101_write_single_reg(ctx, __reg_addr,	      \
						val);			      \
	}

#define DEFINE_STROBE_INSTRUCTION(__ins_name, __ins_addr)		     \
	static inline bool instruct_##__ins_name(struct cc1101_context *ctx) \
	{								     \
		/*SYS_LOG_DBG("");*/					     \
		return _cc1101_instruct(ctx, __ins_addr);		     \
	}

DEFINE_REG_WRITE(iocfg0, CC1101_REG_IOCFG0)
DEFINE_REG_WRITE(iocfg1, CC1101_REG_IOCFG1)
DEFINE_REG_WRITE(iocfg2, CC1101_REG_IOCFG2)
DEFINE_REG_WRITE(channel, CC1101_REG_CHANNEL)

DEFINE_REG_READ(iocfg0, CC1101_REG_IOCFG0)
DEFINE_REG_READ(iocfg1, CC1101_REG_IOCFG1)
DEFINE_REG_READ(iocfg2, CC1101_REG_IOCFG2)

DEFINE_STROBE_INSTRUCTION(sres, CC1101_INS_SRES)
DEFINE_STROBE_INSTRUCTION(sfstxon, CC1101_INS_SFSTXON)
DEFINE_STROBE_INSTRUCTION(sxoff, CC1101_INS_SXOFF)
DEFINE_STROBE_INSTRUCTION(scal, CC1101_INS_SCAL)
DEFINE_STROBE_INSTRUCTION(srx, CC1101_INS_SRX)
DEFINE_STROBE_INSTRUCTION(stx, CC1101_INS_STX)
DEFINE_STROBE_INSTRUCTION(sidle, CC1101_INS_SIDLE)
DEFINE_STROBE_INSTRUCTION(safc, CC1101_INS_SAFC)
DEFINE_STROBE_INSTRUCTION(swor, CC1101_INS_SWOR)
DEFINE_STROBE_INSTRUCTION(spwd, CC1101_INS_SPWD)
DEFINE_STROBE_INSTRUCTION(sfrx, CC1101_INS_SFRX)
DEFINE_STROBE_INSTRUCTION(sftx, CC1101_INS_SFTX)
DEFINE_STROBE_INSTRUCTION(sworrst, CC1101_INS_SWORRST)
DEFINE_STROBE_INSTRUCTION(snop, CC1101_INS_SNOP)

#endif /* __IEEE802154_CC1101_H__ */
