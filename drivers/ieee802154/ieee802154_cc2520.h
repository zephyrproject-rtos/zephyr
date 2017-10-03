/* ieee802154_cc2520.h - Registers definition for TI CC2520 */

/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __IEEE802154_CC2520_H__
#define __IEEE802154_CC2520_H__

#include <linker/sections.h>
#include <atomic.h>
#include <spi.h>

#include <ieee802154/cc2520.h>

/* Runtime context structure
 ***************************
 */
struct cc2520_spi {
	struct device *dev;
	u32_t slave;
	/**
	 * cmd_buf will use at most 9 bytes:
	 * dummy bytes + 8 ieee address bytes
	 */
	u8_t cmd_buf[12];
};

struct cc2520_context {
	struct net_if *iface;
	/**************************/
	struct cc2520_gpio_configuration *gpios;
	struct gpio_callback sfd_cb;
	struct gpio_callback fifop_cb;
	struct cc2520_spi spi;
	u8_t mac_addr[8];
	/************TX************/
	struct k_sem tx_sync;
	atomic_t tx;
	/************RX************/
	K_THREAD_STACK_MEMBER(cc2520_rx_stack,
			      CONFIG_IEEE802154_CC2520_RX_STACK_SIZE);
	struct k_thread cc2520_rx_thread;
	struct k_sem rx_lock;
#ifdef CONFIG_IEEE802154_CC2520_CRYPTO
	struct k_sem access_lock;
#endif
	bool overflow;
};

#include "ieee802154_cc2520_regs.h"


/* Registers useful routines
 ***************************
 */

u8_t _cc2520_read_reg(struct cc2520_spi *spi,
			 bool freg, u8_t addr);
bool _cc2520_write_reg(struct cc2520_spi *spi, bool freg,
		       u8_t addr, u8_t value);

#define DEFINE_REG_READ(__reg_name, __reg_addr, __freg)			\
	static inline u8_t read_reg_##__reg_name(struct cc2520_spi *spi) \
	{								\
		return _cc2520_read_reg(spi, __freg, __reg_addr);	\
	}

#define DEFINE_REG_WRITE(__reg_name, __reg_addr, __freg)		\
	static inline bool write_reg_##__reg_name(struct cc2520_spi *spi, \
						  u8_t val)		\
	{								\
		return _cc2520_write_reg(spi, __freg, __reg_addr, val);	\
	}

#define DEFINE_FREG_READ(__reg_name, __reg_addr)	\
	DEFINE_REG_READ(__reg_name, __reg_addr, true)
#define DEFINE_FREG_WRITE(__reg_name, __reg_addr)	\
	DEFINE_REG_WRITE(__reg_name, __reg_addr, true)

#define DEFINE_SREG_READ(__reg_name, __reg_addr)	\
	DEFINE_REG_READ(__reg_name, __reg_addr, false)
#define DEFINE_SREG_WRITE(__reg_name, __reg_addr)	\
	DEFINE_REG_WRITE(__reg_name, __reg_addr, false)


DEFINE_FREG_READ(excflag0, CC2520_FREG_EXCFLAG0)
DEFINE_FREG_READ(excflag1, CC2520_FREG_EXCFLAG1)
DEFINE_FREG_READ(excflag2, CC2520_FREG_EXCFLAG2)
DEFINE_FREG_READ(gpioctrl0, CC2520_FREG_GPIOCTRL0)
DEFINE_FREG_READ(gpioctrl1, CC2520_FREG_GPIOCTRL1)
DEFINE_FREG_READ(gpioctrl2, CC2520_FREG_GPIOCTRL2)
DEFINE_FREG_READ(gpioctrl3, CC2520_FREG_GPIOCTRL3)
DEFINE_FREG_READ(gpioctrl4, CC2520_FREG_GPIOCTRL4)
DEFINE_FREG_READ(gpioctrl5, CC2520_FREG_GPIOCTRL5)
DEFINE_FREG_READ(gpiopolarity, CC2520_FREG_GPIOPOLARITY)
DEFINE_FREG_READ(gpioctrl, CC2520_FREG_GPIOCTRL)
DEFINE_FREG_READ(txfifocnt, CC2520_FREG_TXFIFOCNT)
DEFINE_FREG_READ(rxfifocnt, CC2520_FREG_RXFIFOCNT)
DEFINE_FREG_READ(dpustat, CC2520_FREG_DPUSTAT)

DEFINE_FREG_WRITE(frmctrl0, CC2520_FREG_FRMCTRL0)
DEFINE_FREG_WRITE(frmctrl1, CC2520_FREG_FRMCTRL1)
DEFINE_FREG_WRITE(excflag0, CC2520_FREG_EXCFLAG0)
DEFINE_FREG_WRITE(excflag1, CC2520_FREG_EXCFLAG1)
DEFINE_FREG_WRITE(excflag2, CC2520_FREG_EXCFLAG2)
DEFINE_FREG_WRITE(frmfilt0, CC2520_FREG_FRMFILT0)
DEFINE_FREG_WRITE(frmfilt1, CC2520_FREG_FRMFILT1)
DEFINE_FREG_WRITE(srcmatch, CC2520_FREG_SRCMATCH)
DEFINE_FREG_WRITE(fifopctrl, CC2520_FREG_FIFOPCTRL)
DEFINE_FREG_WRITE(freqctrl, CC2520_FREG_FREQCTRL)
DEFINE_FREG_WRITE(txpower, CC2520_FREG_TXPOWER)
DEFINE_FREG_WRITE(ccactrl0, CC2520_FREG_CCACTRL0)

DEFINE_SREG_WRITE(mdmctrl0, CC2520_SREG_MDMCTRL0)
DEFINE_SREG_WRITE(mdmctrl1, CC2520_SREG_MDMCTRL1)
DEFINE_SREG_WRITE(rxctrl, CC2520_SREG_RXCTRL)
DEFINE_SREG_WRITE(fsctrl, CC2520_SREG_FSCTRL)
DEFINE_SREG_WRITE(fscal1, CC2520_SREG_FSCAL1)
DEFINE_SREG_WRITE(agcctrl1, CC2520_SREG_AGCCTRL1)
DEFINE_SREG_WRITE(adctest0, CC2520_SREG_ADCTEST0)
DEFINE_SREG_WRITE(adctest1, CC2520_SREG_ADCTEST1)
DEFINE_SREG_WRITE(adctest2, CC2520_SREG_ADCTEST2)
DEFINE_SREG_WRITE(extclock, CC2520_SREG_EXTCLOCK)

/* Memory useful routines
 ************************
 */

bool _cc2520_write_ram(struct cc2520_spi *spi, u16_t addr,
		       u8_t *data_buf, u8_t len);

#define DEFINE_MEM_WRITE(__mem_name, __addr, __sz)			\
	static inline bool write_mem_##__mem_name(struct cc2520_spi *spi, \
						  u8_t *buf)		\
	{								\
		return _cc2520_write_ram(spi, __addr, buf, __sz);	\
	}

DEFINE_MEM_WRITE(short_addr, CC2520_MEM_SHORT_ADDR, 2)
DEFINE_MEM_WRITE(pan_id, CC2520_MEM_PAN_ID, 2)
DEFINE_MEM_WRITE(ext_addr, CC2520_MEM_EXT_ADDR, 8)


/* Instructions useful routines
 ******************************
 */

static inline bool _cc2520_command_strobe(struct cc2520_spi *spi,
					  u8_t instruction)
{
	spi_slave_select(spi->dev, spi->slave);

	return (spi_write(spi->dev, &instruction, 1) == 0);
}

static inline bool _cc2520_command_strobe_snop(struct cc2520_spi *spi,
					       u8_t instruction)
{
	u8_t ins[2] = {
		instruction,
		CC2520_INS_SNOP
	};

	spi_slave_select(spi->dev, spi->slave);

	return (spi_write(spi->dev, ins, 2) == 0);
}

#define DEFINE_STROBE_INSTRUCTION(__ins_name, __ins)			\
	static inline bool instruct_##__ins_name(struct cc2520_spi *spi) \
	{								\
		return _cc2520_command_strobe(spi, __ins);		\
	}

#define DEFINE_STROBE_SNOP_INSTRUCTION(__ins_name, __ins)		\
	static inline bool instruct_##__ins_name(struct cc2520_spi *spi) \
	{								\
		return _cc2520_command_strobe_snop(spi, __ins);		\
	}

DEFINE_STROBE_INSTRUCTION(srxon, CC2520_INS_SRXON)
DEFINE_STROBE_INSTRUCTION(srfoff, CC2520_INS_SRFOFF)
DEFINE_STROBE_INSTRUCTION(stxon, CC2520_INS_STXON)
DEFINE_STROBE_INSTRUCTION(stxoncca, CC2520_INS_STXONCCA)
DEFINE_STROBE_INSTRUCTION(sflushrx, CC2520_INS_SFLUSHRX)
DEFINE_STROBE_INSTRUCTION(sflushtx, CC2520_INS_SFLUSHTX)
DEFINE_STROBE_INSTRUCTION(sxoscoff, CC2520_INS_SXOSCOFF)

DEFINE_STROBE_SNOP_INSTRUCTION(sxoscon, CC2520_INS_SXOSCON)

#endif /* __IEEE802154_CC2520_H__ */
