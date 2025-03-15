/*
 * Copyright (c) 2020 DENX Software Engineering GmbH
 *               Lukasz Majewski <lukma@denx.de>
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME dsa

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_ETHERNET_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/sys/util.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/linker/sections.h>
#include <zephyr/toolchain/common.h>

#if defined(CONFIG_DSA_SPI)
#include <zephyr/drivers/spi.h>
#else
#error "No communication bus defined"
#endif

#if CONFIG_DSA_KSZ8863
#define DT_DRV_COMPAT microchip_ksz8863
#include "dsa_ksz8863.h"
#elif CONFIG_DSA_KSZ8794
#define DT_DRV_COMPAT microchip_ksz8794
#include "dsa_ksz8794.h"
#elif CONFIG_DSA_KSZ8463
#define DT_DRV_COMPAT microchip_ksz8463
#include "dsa_ksz8463.h"
#else
#error "Unsupported KSZ chipset"
#endif

struct ksz8xxx_data {
	int iface_init_count;
	bool is_init;
#if defined(CONFIG_DSA_SPI)
	struct spi_dt_spec spi;
#endif
};

#define PRV_DATA(ctx) ((struct ksz8xxx_data *const)(ctx)->prv_data)

static void dsa_ksz8xxx_write_reg(const struct ksz8xxx_data *pdev,
				  uint16_t reg_addr, uint8_t value)
{
#if defined(CONFIG_DSA_SPI)
	uint8_t buf[3];

	const struct spi_buf tx_buf = {
		.buf = buf,
		.len = 3
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

#if CONFIG_DSA_KSZ8463
	buf[0] = KSZ8XXX_SPI_CMD_WR | KSZ8463_REG_ADDR_HI_PART(reg_addr);
	buf[1] = KSZ8463_REG_ADDR_LO_PART(reg_addr) | KSZ8463_SPI_BYTE_ENABLE(reg_addr);
	buf[2] = value;
#else
	buf[0] = KSZ8XXX_SPI_CMD_WR | ((reg_addr >> 7) & 0x1F);
	buf[1] = (reg_addr << 1) & 0xFE;
	buf[2] = value;
#endif

	spi_write_dt(&pdev->spi, &tx);
#endif
}

static void dsa_ksz8xxx_read_reg(const struct ksz8xxx_data *pdev,
				 uint16_t reg_addr, uint8_t *value)
{
#if defined(CONFIG_DSA_SPI)
	uint8_t buf[3];

	const struct spi_buf tx_buf = {
		.buf = buf,
		.len = 3
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};
	struct spi_buf rx_buf = {
		.buf = buf,
		.len = 3
	};

	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1
	};

#if CONFIG_DSA_KSZ8463
	buf[0] = KSZ8XXX_SPI_CMD_RD | KSZ8463_REG_ADDR_HI_PART(reg_addr);
	buf[1] = KSZ8463_REG_ADDR_LO_PART(reg_addr) | KSZ8463_SPI_BYTE_ENABLE(reg_addr);
	buf[2] = 0;
#else
	buf[0] = KSZ8XXX_SPI_CMD_RD | ((reg_addr >> 7) & 0x1F);
	buf[1] = (reg_addr << 1) & 0xFE;
	buf[2] = 0x0;
#endif

	if (!spi_transceive_dt(&pdev->spi, &tx, &rx)) {
		*value = buf[2];
	} else {
		LOG_DBG("Failure while reading register 0x%04x", reg_addr);
		*value = 0U;
	}
#endif
}

static bool dsa_ksz8xxx_port_link_status(struct ksz8xxx_data *pdev,
					 uint8_t port)
{
	uint8_t tmp;

	if (port < KSZ8XXX_FIRST_PORT || port > KSZ8XXX_LAST_PORT ||
		port == KSZ8XXX_CPU_PORT) {
		return false;
	}

	dsa_ksz8xxx_read_reg(pdev, KSZ8XXX_STAT2_PORTn(port), &tmp);

	return tmp & KSZ8XXX_STAT2_LINK_GOOD;
}

#if !DT_INST_NODE_HAS_PROP(0, reset_gpios)
static void dsa_ksz8xxx_soft_reset(struct ksz8xxx_data *pdev)
{
	/* reset switch */
	dsa_ksz8xxx_write_reg(pdev, KSZ8XXX_RESET_REG,
			      KSZ8XXX_RESET_SET);
	k_busy_wait(KSZ8XXX_SOFT_RESET_DURATION);
	dsa_ksz8xxx_write_reg(pdev, KSZ8XXX_RESET_REG, KSZ8XXX_RESET_CLEAR);
}
#endif

static int dsa_ksz8xxx_probe(struct ksz8xxx_data *pdev)
{
	uint16_t timeout = 100;
	uint8_t val[2], tmp;

	/*
	 * Wait for SPI of KSZ8794 being fully operational - up to 10 ms
	 */
	for (timeout = 100, tmp = 0;
	     tmp != KSZ8XXX_CHIP_ID0_ID_DEFAULT && timeout > 0; timeout--) {
		dsa_ksz8xxx_read_reg(pdev, KSZ8XXX_CHIP_ID0, &tmp);
		k_busy_wait(100);
	}

	if (timeout == 0) {
		LOG_ERR("KSZ8794: No SPI communication!");
		return -ENODEV;
	}

	dsa_ksz8xxx_read_reg(pdev, KSZ8XXX_CHIP_ID0, &val[0]);
	dsa_ksz8xxx_read_reg(pdev, KSZ8XXX_CHIP_ID1, &val[1]);

	if (val[0] != KSZ8XXX_CHIP_ID0_ID_DEFAULT ||
#if CONFIG_DSA_KSZ8463
	    (val[1] != KSZ8463_CHIP_ID1_ID_DEFAULT &&
	    val[1] != KSZ8463F_CHIP_ID1_ID_DEFAULT)) {
#else
	    val[1] != KSZ8XXX_CHIP_ID1_ID_DEFAULT) {
#endif
		LOG_ERR("Chip ID mismatch. "
			"Expected %02x%02x but found %02x%02x",
			KSZ8XXX_CHIP_ID0_ID_DEFAULT,
			KSZ8XXX_CHIP_ID1_ID_DEFAULT,
			val[0],
			val[1]);
		return -ENODEV;
	}

	LOG_DBG("KSZ8794: ID0: 0x%x ID1: 0x%x timeout: %d", val[0], val[1],
		timeout);

	return 0;
}

static int dsa_ksz8xxx_write_static_mac_table(struct ksz8xxx_data *pdev,
					      uint16_t entry_addr, uint8_t *p)
{
	/*
	 * According to KSZ8794 manual - write to static mac address table
	 * requires write to indirect registers:
	 * Write register 0x71 (113)
	 * ....
	 * Write register 0x78 (120)
	 *
	 * Then:
	 * Write to Register 110 with 0x00 (write static table selected)
	 * Write to Register 111 with 0x0x (trigger the write operation, to
	 * table entry x)
	 */
	dsa_ksz8xxx_write_reg(pdev, KSZ8XXX_REG_IND_DATA_7, p[7]);
	dsa_ksz8xxx_write_reg(pdev, KSZ8XXX_REG_IND_DATA_6, p[6]);
	dsa_ksz8xxx_write_reg(pdev, KSZ8XXX_REG_IND_DATA_5, p[5]);
	dsa_ksz8xxx_write_reg(pdev, KSZ8XXX_REG_IND_DATA_4, p[4]);
	dsa_ksz8xxx_write_reg(pdev, KSZ8XXX_REG_IND_DATA_3, p[3]);
	dsa_ksz8xxx_write_reg(pdev, KSZ8XXX_REG_IND_DATA_2, p[2]);
	dsa_ksz8xxx_write_reg(pdev, KSZ8XXX_REG_IND_DATA_1, p[1]);
	dsa_ksz8xxx_write_reg(pdev, KSZ8XXX_REG_IND_DATA_0, p[0]);

	dsa_ksz8xxx_write_reg(pdev, KSZ8XXX_REG_IND_CTRL_0, 0x00);
	dsa_ksz8xxx_write_reg(pdev, KSZ8XXX_REG_IND_CTRL_1, entry_addr);

	return 0;
}

static int dsa_ksz8xxx_set_static_mac_table(struct ksz8xxx_data *pdev,
					    const uint8_t *mac, uint8_t fw_port,
					    uint16_t entry_idx)
{
	/*
	 * The data in uint8_t buf[] buffer is stored in the little endian
	 * format, as it eases programming proper KSZ8794 registers.
	 */
	uint8_t buf[8];

	buf[7] = 0;
	/* Prepare entry for static MAC address table */
	buf[5] = mac[0];
	buf[4] = mac[1];
	buf[3] = mac[2];
	buf[2] = mac[3];
	buf[1] = mac[4];
	buf[0] = mac[5];

	buf[6] = fw_port;
	buf[6] |= KSZ8XXX_STATIC_MAC_TABLE_VALID;
	buf[6] |= KSZ8XXX_STATIC_MAC_TABLE_OVRD;

	dsa_ksz8xxx_write_static_mac_table(pdev, entry_idx, buf);

	return 0;
}

static int dsa_ksz8xxx_read_static_mac_table(struct ksz8xxx_data *pdev,
					      uint16_t entry_addr, uint8_t *p)
{
	/*
	 * According to KSZ8794 manual - read from static mac address table
	 * requires reads from indirect registers:
	 *
	 * Write to Register 110 with 0x10 (read static table selected)
	 * Write to Register 111 with 0x0x (trigger the read operation, to
	 * table entry x)
	 *
	 * Then:
	 * Write register 0x71 (113)
	 * ....
	 * Write register 0x78 (120)
	 *
	 */

	dsa_ksz8xxx_write_reg(pdev, KSZ8XXX_REG_IND_CTRL_0, 0x10);
	dsa_ksz8xxx_write_reg(pdev, KSZ8XXX_REG_IND_CTRL_1, entry_addr);

	dsa_ksz8xxx_read_reg(pdev, KSZ8XXX_REG_IND_DATA_7, &p[7]);
	dsa_ksz8xxx_read_reg(pdev, KSZ8XXX_REG_IND_DATA_6, &p[6]);
	dsa_ksz8xxx_read_reg(pdev, KSZ8XXX_REG_IND_DATA_5, &p[5]);
	dsa_ksz8xxx_read_reg(pdev, KSZ8XXX_REG_IND_DATA_4, &p[4]);
	dsa_ksz8xxx_read_reg(pdev, KSZ8XXX_REG_IND_DATA_3, &p[3]);
	dsa_ksz8xxx_read_reg(pdev, KSZ8XXX_REG_IND_DATA_2, &p[2]);
	dsa_ksz8xxx_read_reg(pdev, KSZ8XXX_REG_IND_DATA_1, &p[1]);
	dsa_ksz8xxx_read_reg(pdev, KSZ8XXX_REG_IND_DATA_0, &p[0]);

	return 0;
}

#if defined(CONFIG_DSA_KSZ_PORT_ISOLATING)
static int dsa_ksz8xxx_port_isolate(const struct ksz8xxx_data *pdev)
{
	uint8_t tmp, i;

	for (i = KSZ8XXX_FIRST_PORT; i < KSZ8XXX_LAST_PORT; i++) {
		dsa_ksz8xxx_read_reg(pdev, KSZ8XXX_CTRL1_PORTn(i), &tmp);
		tmp &= KSZ8XXX_CTRL1_VLAN_PORTS_MASK;
		tmp |= 1 << KSZ8XXX_CPU_PORT | 1 << i;
		dsa_ksz8xxx_write_reg(pdev, KSZ8XXX_CTRL1_PORTn(i), tmp);
	}

	dsa_ksz8xxx_read_reg(pdev, KSZ8XXX_CTRL1_PORTn(KSZ8XXX_CPU_PORT),
			     &tmp);
	tmp |= ~KSZ8XXX_CTRL1_VLAN_PORTS_MASK;
	dsa_ksz8xxx_write_reg(pdev, KSZ8XXX_CTRL1_PORTn(KSZ8XXX_CPU_PORT),
			      tmp);

	return 0;
}
#endif

#if CONFIG_DSA_KSZ8463
static int dsa_ksz8xxx_switch_setup(struct ksz8xxx_data *pdev)
{
	uint8_t tmp, i;

	dsa_ksz8xxx_read_reg(pdev, KSZ8XXX_CHIP_ID1, &tmp);

	if (tmp == KSZ8463F_CHIP_ID1_ID_DEFAULT) {
		dsa_ksz8xxx_read_reg(pdev, KSZ8463_CFGR_L, &tmp);
		tmp &= ~KSZ8463_P1_COPPER_MODE;
		tmp &= ~KSZ8463_P2_COPPER_MODE;
		dsa_ksz8xxx_write_reg(pdev, KSZ8463_CFGR_L, tmp);
		dsa_ksz8xxx_read_reg(pdev, KSZ8463_DSP_CNTRL_6H, &tmp);
		tmp &= ~KSZ8463_RECV_ADJ;
		dsa_ksz8xxx_write_reg(pdev, KSZ8463_DSP_CNTRL_6H, tmp);
	}

	/*
	 * Loop through ports - The same setup when tail tagging is enabled or
	 * disabled.
	 */
	for (i = KSZ8XXX_FIRST_PORT; i <= KSZ8XXX_LAST_PORT; i++) {

		/* Enable transmission, reception and switch address learning */
		dsa_ksz8xxx_read_reg(pdev, KSZ8463_CTRL2H_PORTn(i), &tmp);
		tmp |= KSZ8463_CTRL2_TRANSMIT_EN;
		tmp |= KSZ8463_CTRL2_RECEIVE_EN;
		tmp &= ~KSZ8463_CTRL2_LEARNING_DIS;
		dsa_ksz8xxx_write_reg(pdev, KSZ8463_CTRL2H_PORTn(i), tmp);
	}

#if defined(CONFIG_DSA_KSZ_TAIL_TAGGING)
	/* Enable tail tag feature */
	dsa_ksz8xxx_read_reg(pdev, KSZ8463_GLOBAL_CTRL_8H, &tmp);
	tmp |= KSZ8463_GLOBAL_CTRL1_TAIL_TAG_EN;
	dsa_ksz8xxx_write_reg(pdev, KSZ8463_GLOBAL_CTRL_8H, tmp);
#else
	/* Disable tail tag feature */
	dsa_ksz8xxx_read_reg(pdev, KSZ8463_GLOBAL_CTRL_8H, &tmp);
	tmp &= ~KSZ8463_GLOBAL_CTRL1_TAIL_TAG_EN;
	dsa_ksz8xxx_write_reg(pdev, KSZ8463_GLOBAL_CTRL_8H, tmp);
#endif

	dsa_ksz8xxx_read_reg(pdev, KSZ8463_GLOBAL_CTRL_2L, &tmp);
	tmp &= ~KSZ8463_GLOBAL_CTRL2_LEG_MAX_PKT_SIZ_CHK_ENA;
	dsa_ksz8xxx_write_reg(pdev, KSZ8463_GLOBAL_CTRL_2L, tmp);

	return 0;
}
#endif

#if CONFIG_DSA_KSZ8863
static int dsa_ksz8xxx_switch_setup(const struct ksz8xxx_data *pdev)
{
	uint8_t tmp, i;

	/*
	 * Loop through ports - The same setup when tail tagging is enabled or
	 * disabled.
	 */
	for (i = KSZ8XXX_FIRST_PORT; i <= KSZ8XXX_LAST_PORT; i++) {

		/* Enable transmission, reception and switch address learning */
		dsa_ksz8xxx_read_reg(pdev, KSZ8863_CTRL2_PORTn(i), &tmp);
		tmp |= KSZ8863_CTRL2_TRANSMIT_EN;
		tmp |= KSZ8863_CTRL2_RECEIVE_EN;
		tmp &= ~KSZ8863_CTRL2_LEARNING_DIS;
		dsa_ksz8xxx_write_reg(pdev, KSZ8863_CTRL2_PORTn(i), tmp);
	}

#if defined(CONFIG_DSA_KSZ_TAIL_TAGGING)
	/* Enable tail tag feature */
	dsa_ksz8xxx_read_reg(pdev, KSZ8863_GLOBAL_CTRL1, &tmp);
	tmp |= KSZ8863_GLOBAL_CTRL1_TAIL_TAG_EN;
	dsa_ksz8xxx_write_reg(pdev, KSZ8863_GLOBAL_CTRL1, tmp);
#else
	/* Disable tail tag feature */
	dsa_ksz8xxx_read_reg(pdev, KSZ8863_GLOBAL_CTRL1, &tmp);
	tmp &= ~KSZ8863_GLOBAL_CTRL1_TAIL_TAG_EN;
	dsa_ksz8xxx_write_reg(pdev, KSZ8863_GLOBAL_CTRL1, tmp);
#endif

	dsa_ksz8xxx_read_reg(pdev, KSZ8863_GLOBAL_CTRL2, &tmp);
	tmp &= ~KSZ8863_GLOBAL_CTRL2_LEG_MAX_PKT_SIZ_CHK_ENA;
	dsa_ksz8xxx_write_reg(pdev, KSZ8863_GLOBAL_CTRL2, tmp);

	return 0;
}
#endif

#if CONFIG_DSA_KSZ8794
static int dsa_ksz8xxx_switch_setup(struct ksz8xxx_data *pdev)
{
	uint8_t tmp, i;

	/*
	 * Loop through ports - The same setup when tail tagging is enabled or
	 * disabled.
	 */
	for (i = KSZ8XXX_FIRST_PORT; i <= KSZ8XXX_LAST_PORT; i++) {
		/* Enable transmission, reception and switch address learning */
		dsa_ksz8xxx_read_reg(pdev, KSZ8794_CTRL2_PORTn(i), &tmp);
		tmp |= KSZ8794_CTRL2_TRANSMIT_EN;
		tmp |= KSZ8794_CTRL2_RECEIVE_EN;
		tmp &= ~KSZ8794_CTRL2_LEARNING_DIS;
		dsa_ksz8xxx_write_reg(pdev, KSZ8794_CTRL2_PORTn(i), tmp);
	}

#if defined(CONFIG_DSA_KSZ_TAIL_TAGGING)
	/* Enable tail tag feature */
	dsa_ksz8xxx_read_reg(pdev, KSZ8794_GLOBAL_CTRL10, &tmp);
	tmp |= KSZ8794_GLOBAL_CTRL10_TAIL_TAG_EN;
	dsa_ksz8xxx_write_reg(pdev, KSZ8794_GLOBAL_CTRL10, tmp);
#else
	/* Disable tail tag feature */
	dsa_ksz8xxx_read_reg(pdev, KSZ8794_GLOBAL_CTRL10, &tmp);
	tmp &= ~KSZ8794_GLOBAL_CTRL10_TAIL_TAG_EN;
	dsa_ksz8xxx_write_reg(pdev, KSZ8794_GLOBAL_CTRL10, tmp);
#endif

	dsa_ksz8xxx_read_reg(pdev, KSZ8794_PORT4_IF_CTRL6, &tmp);
	LOG_DBG("KSZ8794: CONTROL6: 0x%x port4", tmp);

	dsa_ksz8xxx_read_reg(pdev, KSZ8794_PORT4_CTRL2, &tmp);
	LOG_DBG("KSZ8794: CONTROL2: 0x%x port4", tmp);

	dsa_ksz8xxx_read_reg(pdev, KSZ8794_GLOBAL_CTRL2, &tmp);
	tmp |= KSZ8794_GLOBAL_CTRL2_LEG_MAX_PKT_SIZ_CHK_DIS;
	dsa_ksz8xxx_write_reg(pdev, KSZ8794_GLOBAL_CTRL2, tmp);

	return 0;
}

#if DT_INST_NODE_HAS_PROP(0, workaround)
/*
 * Workaround 0x01
 * Solution for Short Cable Problems with the KSZ8795 Family
 *
 * Title
 * Solution for Short Cable Problems with the KSZ8795 Family
 *
 * https://microchipsupport.force.com/s/article/Solution-for-Short-Cable-
 * Problems-with-the-KSZ8795-Family
 *
 * Problem Description:
 * 1) The KSZ8795 family parts might be not link when connected through a few
 *   type of short cable (<3m).
 * 2) There may be a link-up issue in the capacitor AC coupling mode for port
 *   to port or board to board cases.
 *
 * Answer
 * Root Cause:
 * KSZ8795 family switches with integrated Ethernet PHY that has a DSP based
 * equalizer EQ that can balance the signal received to adapt various cable
 * length characteristics. The equalizer default settings amplify the signal
 * coming in to get more accurate readings from low amplitude signals.
 * When using some type of short cable (for example, CAT-6 cable with low
 * attenuation to high frequencies signal vs. CAT-5 cable) or board to board
 * connection, or port to port with capacitor AC coupling connection, the signal
 * is amplified too much and cause the link-up failed with same boost setting in
 * the equalizer EQ.
 *
 * Solution/Workaround:
 * Write a DSP control register that is indirect register (0x3c) to optimize the
 * equalizer EQ to cover above corner cases.
 *  w 6e a0 //write the indirect register
 *  w 6f 3c //assign the indirect hidden register address (0x3c)
 *  w a0 15 //write 0x15 to REG (0x3c) to optimize the EQ. The default is 0x0a.
 * Based on testing and practical application, this register setting above can
 * solve the issue for all type of the short cables and the capacitor AC
 * coupling mode.
 *
 * The indirect DSP register (0x3c) is an 8-bit register, the bits describe as
 * follows,
 *
 * Bits Bit Name            Description                   Mode  Default  Setting
 *								0x0a     0x15
 * 7-5  Reserved                                          RO    000      000
 * 4    Cpu_EQ_Done_Cond1   How to judge EQ is finished,
 *			  there are two ways to judge
 *			  if EQ is finished, can set
 *			  either way                      R/W   0        1
 * 3-1  Cpu_EQ_CP_Points  Control of EQ training is
 *			  over-boosted or
 *     [2:0]              under-boosted, that means to
 *			  compensate signal attenuation
 *			  more or less.                   R/W   101      010
 * 0    Cpu_STOP_RUN      after EQ training completed,
 *					 stop adaptation  R/W   0        1
 *
 * Explanation:
 * The above register change makes equalizer’s compensation range wider, and
 * therefore cables with various characteristics can be tolerated. Adjust
 * equalizer EQ training algorithm to cover a few type of short cables issue.
 * Also is appropriate for the board to board connection and port to port
 * connection with the capacitor AC coupling mode.
 *
 * Basically, it decides how much signal amplitude to compensate accurately
 * to the different type of short cable’s characteristics. The current default
 * value in the indirect register (0x3c) can cover all general standard
 * Ethernet short cables like CAT-5, CAT-5e without any problem.
 * Based on tests, a more optimized equalizer adjustment value 0x15 is better
 * for all corner cases of the short cable and short distance connection for
 * port to port or board to board cases.
 */
static int dsa_ksz8794_phy_workaround_0x01(struct ksz8xxx_data *pdev)
{
	uint8_t indirect_type = 0x0a;
	uint8_t indirect_addr = 0x3c;
	uint8_t indirect_data = 0x15;

	dsa_ksz8xxx_write_reg(pdev, KSZ8794_REG_IND_CTRL_0, indirect_type);
	dsa_ksz8xxx_write_reg(pdev, KSZ8794_REG_IND_CTRL_1, indirect_addr);
	dsa_ksz8xxx_write_reg(pdev, KSZ8794_IND_BYTE, indirect_data);
	LOG_INF("apply workarkound 0x01 for short connections on KSZ8794");
	return 0;
}

/*
 * Workaround 0x02 and 0x4
 * Solution for Using CAT-5E or CAT-6 Short Cable with a Link Issue for the
 * KSZ8795 Family
 *
 * Title
 * Solution for Using CAT-5E or CAT-6 Short Cable with a Link Issue for the
 * KSZ8795 Family
 * https://microchipsupport.force.com/s/article/Solution-for-Using-CAT-5E-or
 * -CAT-6-Short-Cable- with-a-Link-Issue-for-the-KSZ8795-Family
 *
 * Question
 * Possible Problem Description:
 * 1) KSZ8795 family includes KSZ8795CLX, KSZ8775CLX, KSZ8765CLX and KSZ8794CNX.
 * 2) The KSZ8795 family copper parts may not link well when connected through a
 * short CAT-5E or CAT-6 cable (about <=30 meter). The failure rate may be about
 * 2-5%.
 *
 * Answer
 * Root Cause:
 * Basically, KSZ8795 10/100 Ethernet switch family was designed based on CAT-5
 * cable. With the application of more type of cables, specially two types
 * cables of CAT-5E and CAT-6, both cables have wider bandwidth that has
 * different frequency characteristics than CAT-5 cable. More higher frequency
 * component of the CAT-5E or CAT-6 will be amplified in the receiving amplifier
 * and will cause the received signal distortion due to too much high frequency
 * components receiving signal amplitude and cause the link-up failure with
 * short cables.
 *
 * Solution/Workaround:
 * 1) dsa_ksz8794_phy_workaround_0x02()
 *  Based on the root cause above, adjust the receiver low pass filter to reduce
 *  the high frequency component to keep the receive signal within a reasonable
 *  range when using CAT-5E and CAT-6 cable.
 *
 *  Set the indirect register as follows for the receiver low pass filter.
 *  Format is w [Register address] [8-bit data]
 *   w 6e a0 //write the indirect register
 *   w 6f 4c //write/assign the internal used indirect register address (0x4c)
 *   w a0 40 //write 0x40 to indirect register (0x4c) to reduce low pass filter
 *   bandwidth.
 *
 * The register 0x4c bits [7:6] for receiver low pass filter bandwidth control.
 *
 *  The default value is ‘00’, change to ‘01’.
 *  Based on testing and practical application, this register setting above can
 *  solve the link issue if using CAT-5E and CAT-6 short cables.
 *
 *  The indirect register (0x4C) is an 8-bit register. The bits [7:6] are
 *  described in the table below.
 *
 *
 * Bits Bit Name            Description                   Mode  Default  Setting
 *								 0x00     0x40
 * 7-6     RX BW control    Low pass filter bandwidth      R/W   00       01
 *			  00 = 90MHz
 *			  01 = 62MHz
 *			  10 = 55MHz
 *			  11 = 44MHz
 * 5       Enable           Near-end loopback              R/W   0        0
 * 4-3     BTRT             Additional reduce              R/W   00       00
 * 2       SD               Ext register                   R/W   0        0
 * 1-0     FXD              reference setting 1.7V, 2V,
 *			  1.4V
 *							   R/W   00       00
 *
 * Solution/Workaround:
 * 2) dsa_ksz8794_phy_workaround_0x04()
 *  For the wider bandwidth cables or on-board capacitor AC coupling
 * application, we recommend adding/setting the indirect register (0x08) from
 * default 0x0f to 0x00 that means to change register (0x08) bits [5:0] from
 * 0x0f to 0x00 to reduce equalizer’s (EQ) initial value to 0x00 for more
 * short cable or on-board capacitors AC coupling application.
 *
 *  Set the indirect register as follows for EQ with 0x00 initial value.
 *  Format is w [Register address] [8-bit data]
 *   w 6e a0 //write the indirect register
 *   w 6f 08 //write/assign the internal used indirect register address (0x08)
 *   w a0 00 //write 0x00 to indirect register (0x08) to make EQ initial value
 *	     equal to 0x00 for very short cable (For example, 0.1m or less)
 *	     or connect two ports directly through capacitors for a capacitor
 *	     AC couple.
 *
 * The indirect DSP register (0x08) is an 8-bit register. The bits [5:0] are
 * described in the table below.
 *
 * Bits Bit Name            Description                   Mode  Default  Setting
 *								0x0f     0x00
 * 7    Park EQ Enable      Park Equalizer function enable R/W  0        0
 * 6    Reserved                                             R  0        0
 * 5-0  Cpu_EQ_Index        Equalizer index control
 *			  interface                       R/W   001111   000000
 *			  from 0 to 55, set EQ initial value
 *  Conclusion:
 *  Due to CAT-5E and CAT-6 cable having wider bandwidth, more high frequency
 *  components will pass the low pass filter into the receiving amplifier and
 *  cause the received signal amplitude to be too high.
 *  Reducing the receiver low pass filter bandwidth will be the best way to
 *  reduce the high frequency components to meet CAT-5E and CAT-6 short cable
 *  link issue and doesn’t affect CAT-5 cable because CAT-5 is not a wider
 *  bandwidth cable.
 *
 *  The DSP register (0X08) bits [5:0] are for EQ initial value. Its current
 *  default value is 0x0F, which assumes the need to equalize regardless of the
 *  cable length. This 0x0f initial equalize value in EQ isn’t needed when
 *  using very short cable or an on-board direct connection like capacitors AC
 *  coupling mode. As the cable length increases, the device will equalize
 *  automatic accordingly from 0x00 EQ initial value.
 *
 *  So, it is better to set both register (0x4c) to 0x40 and register (0x08) to
 *  0x00 for compatibility with all Ethernet cable types and Ethernet cable
 *  lengths.
 */

static int dsa_ksz8794_phy_workaround_0x02(struct ksz8xxx_data *pdev)
{
	uint8_t indirect_type = 0x0a;
	uint8_t indirect_addr = 0x4c;
	uint8_t indirect_data = 0x40;

	dsa_ksz8xxx_write_reg(pdev, KSZ8794_REG_IND_CTRL_0, indirect_type);
	dsa_ksz8xxx_write_reg(pdev, KSZ8794_REG_IND_CTRL_1, indirect_addr);
	dsa_ksz8xxx_write_reg(pdev, KSZ8794_IND_BYTE, indirect_data);
	LOG_INF("apply workarkound 0x02 link issue CAT-5E/6 on KSZ8794");
	return 0;
}

static int dsa_ksz8794_phy_workaround_0x04(struct ksz8xxx_data *pdev)
{
	uint8_t indirect_type = 0x0a;
	uint8_t indirect_addr = 0x08;
	uint8_t indirect_data = 0x00;

	dsa_ksz8xxx_write_reg(pdev, KSZ8794_REG_IND_CTRL_0, indirect_type);
	dsa_ksz8xxx_write_reg(pdev, KSZ8794_REG_IND_CTRL_1, indirect_addr);
	dsa_ksz8xxx_write_reg(pdev, KSZ8794_IND_BYTE, indirect_data);
	LOG_INF("apply workarkound 0x04 link issue CAT-5E/6 on KSZ8794");
	return 0;
}

static int dsa_ksz8794_apply_workarounds(struct ksz8xxx_data *pdev)
{
	int workaround = DT_INST_PROP(0, workaround);

	if (workaround & 0x01) {
		dsa_ksz8794_phy_workaround_0x01(pdev);
	}
	if (workaround & 0x02) {
		dsa_ksz8794_phy_workaround_0x02(pdev);
	}
	if (workaround & 0x04) {
		dsa_ksz8794_phy_workaround_0x04(pdev);
	}
	return 0;
}
#endif

#if DT_INST_NODE_HAS_PROP(0, mii_lowspeed_drivestrength)
static int dsa_ksz8794_set_lowspeed_drivestrength(struct ksz8xxx_data *pdev)
{
	int mii_lowspeed_drivestrength =
		DT_INST_PROP(0, mii_lowspeed_drivestrength);

	uint8_t tmp, val;
	int ret = 0;

	switch (mii_lowspeed_drivestrength) {
	case 2:
		val = KSZ8794_GLOBAL_CTRL20_LOWSPEED_2MA;
		break;
	case 4:
		val = KSZ8794_GLOBAL_CTRL20_LOWSPEED_4MA;
		break;
	case 8:
		val = KSZ8794_GLOBAL_CTRL20_LOWSPEED_8MA;
		break;
	case 12:
		val = KSZ8794_GLOBAL_CTRL20_LOWSPEED_12MA;
		break;
	case 16:
		val = KSZ8794_GLOBAL_CTRL20_LOWSPEED_16MA;
		break;
	case 20:
		val = KSZ8794_GLOBAL_CTRL20_LOWSPEED_20MA;
		break;
	case 24:
		val = KSZ8794_GLOBAL_CTRL20_LOWSPEED_24MA;
		break;
	case 28:
		val = KSZ8794_GLOBAL_CTRL20_LOWSPEED_28MA;
		break;
	default:
		ret = -1;
		LOG_ERR("KSZ8794: unsupported drive strength %dmA",
			mii_lowspeed_drivestrength);
		break;
	}

	if (ret == 0) {
		/* set Low-Speed Interface Drive Strength for MII and RMMI */
		dsa_ksz8xxx_read_reg(pdev, KSZ8794_GLOBAL_CTRL20, &tmp);
		tmp &= ~KSZ8794_GLOBAL_CTRL20_LOWSPEED_MASK;
		tmp |= val;
		dsa_ksz8xxx_write_reg(pdev, KSZ8794_GLOBAL_CTRL20, tmp);
		dsa_ksz8xxx_read_reg(pdev, KSZ8794_GLOBAL_CTRL20, &tmp);
		LOG_INF("KSZ8794: set drive strength %dmA",
			mii_lowspeed_drivestrength);
	}
	return ret;
}
#endif
#endif

#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
static int dsa_ksz8xxx_gpio_reset(void)
{
	struct gpio_dt_spec reset_gpio = GPIO_DT_SPEC_INST_GET(0, reset_gpios);

	if (!gpio_is_ready_dt(&reset_gpio)) {
		LOG_ERR("Reset GPIO device not ready");
		return -ENODEV;
	}
	gpio_pin_configure_dt(&reset_gpio, GPIO_OUTPUT_ACTIVE);
	k_msleep(10);

	gpio_pin_set_dt(&reset_gpio, 0);

	return 0;
}
#endif

/* Low level initialization code for DSA PHY */
int dsa_hw_init(struct ksz8xxx_data *pdev)
{
	int rc;

	if (pdev->is_init) {
		return 0;
	}

	/* Hard reset */
#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	dsa_ksz8xxx_gpio_reset();

	/* Time needed for chip to completely power up (100ms) */
	k_busy_wait(KSZ8XXX_HARD_RESET_WAIT);
#endif

#if defined(CONFIG_DSA_SPI)
	if (!spi_is_ready_dt(&pdev->spi)) {
		LOG_ERR("SPI bus %s is not ready",
			pdev->spi.bus->name);
		return -ENODEV;
	}
#endif

	/* Probe attached PHY */
	rc = dsa_ksz8xxx_probe(pdev);
	if (rc < 0) {
		return rc;
	}

#if !DT_INST_NODE_HAS_PROP(0, reset_gpios)
	/* Soft reset */
	dsa_ksz8xxx_soft_reset(pdev);
#endif

	/* Setup KSZ8794 */
	dsa_ksz8xxx_switch_setup(pdev);

#if defined(CONFIG_DSA_KSZ_PORT_ISOLATING)
	dsa_ksz8xxx_port_isolate(pdev);
#endif

#if DT_INST_NODE_HAS_PROP(0, mii_lowspeed_drivestrength)
	dsa_ksz8794_set_lowspeed_drivestrength(pdev);
#endif

#if DT_INST_NODE_HAS_PROP(0, workaround)
	/* apply workarounds */
	dsa_ksz8794_apply_workarounds(pdev);
#endif

	pdev->is_init = true;

	return 0;
}

static void dsa_delayed_work(struct k_work *item)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(item);
	struct dsa_context *context =
		CONTAINER_OF(dwork, struct dsa_context, dsa_work);
	struct ksz8xxx_data *pdev = PRV_DATA(context);
	bool link_state;
	uint8_t i;

	for (i = KSZ8XXX_FIRST_PORT; i <= KSZ8XXX_LAST_PORT; i++) {
		/* Skip Switch <-> CPU Port */
		if (i == KSZ8XXX_CPU_PORT) {
			continue;
		}

		link_state = dsa_ksz8xxx_port_link_status(pdev, i);
		if (link_state && !context->link_up[i]) {
			LOG_INF("DSA port: %d link UP!", i);
			net_eth_carrier_on(context->iface_slave[i]);
		} else if (!link_state && context->link_up[i]) {
			LOG_INF("DSA port: %d link DOWN!", i);
			net_eth_carrier_off(context->iface_slave[i]);
		}
		context->link_up[i] = link_state;
	}

	k_work_reschedule(&context->dsa_work, DSA_STATUS_PERIOD_MS);
}

int dsa_port_init(const struct device *dev)
{
	struct dsa_context *data = dev->data;
	struct ksz8xxx_data *pdev = PRV_DATA(data);

	dsa_hw_init(pdev);
	return 0;
}

/* Generic implementation of writing value to DSA register */
static int dsa_ksz8xxx_sw_write_reg(const struct device *dev, uint16_t reg_addr,
				    uint8_t value)
{
	struct dsa_context *data = dev->data;
	struct ksz8xxx_data *pdev = PRV_DATA(data);

	dsa_ksz8xxx_write_reg(pdev, reg_addr, value);
	return 0;
}

/* Generic implementation of reading value from DSA register */
static int dsa_ksz8xxx_sw_read_reg(const struct device *dev, uint16_t reg_addr,
				   uint8_t *value)
{
	struct dsa_context *data = dev->data;
	struct ksz8xxx_data *pdev = PRV_DATA(data);

	dsa_ksz8xxx_read_reg(pdev, reg_addr, value);
	return 0;
}

/**
 * @brief Set entry to DSA MAC address table
 *
 * @param dev DSA device
 * @param mac The MAC address to be set in the table
 * @param fw_port Port number to forward packets
 * @param tbl_entry_idx The index of entry in the table
 * @param flags Flags to be set in the entry
 *
 * @return 0 if ok, < 0 if error
 */
static int dsa_ksz8xxx_set_mac_table_entry(const struct device *dev,
						const uint8_t *mac,
						uint8_t fw_port,
						uint16_t tbl_entry_idx,
						uint16_t flags)
{
	struct dsa_context *data = dev->data;
	struct ksz8xxx_data *pdev = PRV_DATA(data);

	if (flags != 0) {
		return -EINVAL;
	}

	dsa_ksz8xxx_set_static_mac_table(pdev, mac, fw_port,
						tbl_entry_idx);

	return 0;
}

/**
 * @brief Get DSA MAC address table entry
 *
 * @param dev DSA device
 * @param buf The buffer for data read from the table
 * @param tbl_entry_idx The index of entry in the table
 *
 * @return 0 if ok, < 0 if error
 */
static int dsa_ksz8xxx_get_mac_table_entry(const struct device *dev,
						uint8_t *buf,
						uint16_t tbl_entry_idx)
{
	struct dsa_context *data = dev->data;
	struct ksz8xxx_data *pdev = PRV_DATA(data);

	dsa_ksz8xxx_read_static_mac_table(pdev, tbl_entry_idx, buf);

	return 0;
}

#if defined(CONFIG_DSA_KSZ_TAIL_TAGGING)
#define DSA_KSZ8795_TAIL_TAG_OVRD	BIT(6)
#define DSA_KSZ8795_TAIL_TAG_LOOKUP	BIT(7)

#define DSA_KSZ8794_EGRESS_TAG_LEN 1
#define DSA_KSZ8794_INGRESS_TAG_LEN 1

#define DSA_MIN_L2_FRAME_SIZE 64
#define DSA_L2_FCS_SIZE 4

struct net_pkt *dsa_ksz8xxx_xmit_pkt(struct net_if *iface, struct net_pkt *pkt)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);
	struct net_eth_hdr *hdr = NET_ETH_HDR(pkt);
	struct net_linkaddr lladst;
	uint8_t port_idx, *dbuf;
	struct net_buf *buf;
	size_t len, pad = 0;

	lladst.len = sizeof(hdr->dst.addr);
	lladst.addr = &hdr->dst.addr[0];

	len = net_pkt_get_len(pkt);
	/*
	 * For KSZ8794 one needs to 'pad' the L2 frame to its minimal size
	 * (64B) before appending TAIL TAG and FCS
	 */
	if (len < (DSA_MIN_L2_FRAME_SIZE - DSA_L2_FCS_SIZE)) {
		/* Calculate number of bytes needed for padding */
		pad = DSA_MIN_L2_FRAME_SIZE - DSA_L2_FCS_SIZE - len;
	}

	buf = net_buf_alloc_len(net_buf_pool_get(pkt->buffer->pool_id),
				pad + DSA_KSZ8794_INGRESS_TAG_LEN, K_NO_WAIT);
	if (!buf) {
		LOG_ERR("DSA cannot allocate new data buffer");
		return NULL;
	}

	/*
	 * Get the pointer to struct's net_buf_simple data and zero out the
	 * padding and tag byte placeholder
	 */
	dbuf = net_buf_simple_tail(&(buf->b));
	memset(dbuf, 0x0, pad + DSA_KSZ8794_INGRESS_TAG_LEN);

	/*
	 * For master port (eth0) set the bit 7 to use look-up table to pass
	 * packet to correct interface (bits [0..6] _are_ ignored).
	 *
	 * For slave ports (lan1..3) just set the tag properly:
	 * bit 0 -> eth1, bit 1 -> eth2. bit 2 -> eth3
	 * It may be also necessary to set bit 6 to "anyhow send packets to
	 * specified port in Bits[3:0]". This may be needed for RSTP
	 * implementation (when the switch port is disabled, but shall handle
	 * LLDP packets).
	 */
	if (dsa_is_port_master(iface)) {
		port_idx = DSA_KSZ8795_TAIL_TAG_LOOKUP;
	} else {
		port_idx = (1 << (ctx->dsa_port_idx));
	}

	LOG_DBG("TT - port: 0x%x[%p] LEN: %d 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x",
		port_idx, iface, len, lladst.addr[0], lladst.addr[1],
		lladst.addr[2], lladst.addr[3], lladst.addr[4], lladst.addr[5]);

	/* The tail tag shall be placed after the padding (if present) */
	dbuf[pad] = port_idx;

	/* Set proper len member for the actual struct net_buf_simple */
	net_buf_add(buf, pad + DSA_KSZ8794_INGRESS_TAG_LEN);

	/* Append struct net_buf to packet data */
	net_buf_frag_add(pkt->buffer, buf);

	return pkt;
}

/**
 * @brief DSA function to get proper interface
 *
 * This is the function for assigning proper slave interface after receiving
 * the packet on master.
 *
 * @param iface Network interface
 * @param pkt Network packet
 *
 * Returns:
 *  - Pointer to struct net_if
 */
static struct net_if *dsa_ksz8xxx_get_iface(struct net_if *iface,
					    struct net_pkt *pkt)
{
	struct ethernet_context *ctx;
	struct net_if *iface_sw;
	size_t plen;
	uint8_t pnum;

	if (!(net_eth_get_hw_capabilities(iface) &
	      (ETHERNET_DSA_SLAVE_PORT | ETHERNET_DSA_MASTER_PORT))) {
		return iface;
	}

	net_pkt_set_overwrite(pkt, true);
	net_pkt_cursor_init(pkt);
	plen = net_pkt_get_len(pkt);

	net_pkt_skip(pkt, plen - DSA_KSZ8794_EGRESS_TAG_LEN);
	net_pkt_read_u8(pkt, &pnum);

	net_pkt_update_length(pkt, plen - DSA_KSZ8794_EGRESS_TAG_LEN);

	/*
	 * NOTE:
	 * The below approach is only for ip_k66f board as we do know
	 * that eth0 is on position (index) 1, then we do have lan1 with
	 * index 2, lan2 with 3 and lan3 with 4.
	 *
	 * This is caused by eth interfaces placing order by linker and
	 * may vary on other boards, where are for example two eth
	 * interfaces available.
	 */
	iface_sw = net_if_get_by_index(pnum + 2);

	ctx = net_if_l2_data(iface);
	LOG_DBG("TT - plen: %d pnum: %d pos: 0x%p dsa_port_idx: %d",
		plen - DSA_KSZ8794_EGRESS_TAG_LEN, pnum,
		net_pkt_cursor_get_pos(pkt), ctx->dsa_port_idx);

	return iface_sw;
}
#endif

static void dsa_iface_init(struct net_if *iface)
{
	struct dsa_slave_config *cfg = (struct dsa_slave_config *)
		net_if_get_device(iface)->config;
	struct ethernet_context *ctx = net_if_l2_data(iface);
	const struct device *dm, *dev = net_if_get_device(iface);
	struct dsa_context *context = dev->data;
	struct ksz8xxx_data *pdev = PRV_DATA(context);
	struct ethernet_context *ctx_master;
	int i = pdev->iface_init_count;

	/* Find master port for ksz8794 switch */
	if (context->iface_master == NULL) {
		dm = DEVICE_DT_GET(DT_INST_PHANDLE(0, dsa_master_port));
		context->iface_master = net_if_lookup_by_dev(dm);
		if (context->iface_master == NULL) {
			LOG_ERR("DSA: Master iface NOT found!");
			return;
		}

		/*
		 * Provide pointer to DSA context to master's eth interface
		 * struct ethernet_context
		 */
		ctx_master = net_if_l2_data(context->iface_master);
		ctx_master->dsa_ctx = context;
	}

	if (context->iface_slave[i] == NULL) {
		context->iface_slave[i] = iface;
		net_if_set_link_addr(iface, cfg->mac_addr,
				     sizeof(cfg->mac_addr),
				     NET_LINK_ETHERNET);
		ctx->dsa_port_idx = i;
		ctx->dsa_ctx = context;

		/*
		 * Initialize ethernet context 'work' for this iface to
		 * be able to monitor the carrier status.
		 */
		ethernet_init(iface);
	}

	pdev->iface_init_count++;
	net_if_carrier_off(iface);

	/*
	 * Start DSA work to monitor status of ports (read from switch IC)
	 * only when carrier_work is properly initialized for all slave
	 * interfaces.
	 */
	if (pdev->iface_init_count == context->num_slave_ports) {
		k_work_init_delayable(&context->dsa_work, dsa_delayed_work);
		k_work_reschedule(&context->dsa_work, DSA_STATUS_PERIOD_MS);
	}
}

static enum ethernet_hw_caps dsa_port_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_DSA_SLAVE_PORT | ETHERNET_LINK_10BASE_T |
		ETHERNET_LINK_100BASE_T;
}

const struct ethernet_api dsa_eth_api_funcs = {
	.iface_api.init		= dsa_iface_init,
	.get_capabilities	= dsa_port_get_capabilities,
	.send                   = dsa_tx,
};

static struct dsa_api dsa_api_f = {
	.switch_read = dsa_ksz8xxx_sw_read_reg,
	.switch_write = dsa_ksz8xxx_sw_write_reg,
	.switch_set_mac_table_entry = dsa_ksz8xxx_set_mac_table_entry,
	.switch_get_mac_table_entry = dsa_ksz8xxx_get_mac_table_entry,
#if defined(CONFIG_DSA_KSZ_TAIL_TAGGING)
	.dsa_xmit_pkt = dsa_ksz8xxx_xmit_pkt,
	.dsa_get_iface = dsa_ksz8xxx_get_iface,
#endif
};

/*
 * The order of NET_DEVICE_INIT_INSTANCE() placement IS important.
 *
 * To make the code simpler - the special care needs to be put on
 * the proper placement of eth0, lan1, lan2, lan3, etc - to avoid
 * the need to search for proper interface when each packet is
 * received or sent.
 * The net_if.c has a very fast API to provide access to linked by
 * the linker struct net_if(s) via device or index. As it is already
 * available for use - let's use it.
 *
 * To do that one needs to check how linker places the interfaces.
 * To inspect:
 * objdump -dst ./zephyr/CMakeFiles/zephyr.dir/drivers/ethernet/eth_mcux.c.obj\
 * | grep "__net_if"
 * (The real problem is with eth0 and lanX order)
 *
 * If this approach is not enough for a simple system (like e.g. ip_k66f, one
 * can prepare dedicated linker script for the board to force the
 * order for complicated designs (like ones with eth0, eth1, and lanX).
 *
 * For simple cases it is just good enough.
 */

#define NET_SLAVE_DEVICE_INIT_INSTANCE(slave, n)                           \
	const struct dsa_slave_config dsa_0_slave_##slave##_config = {     \
		.mac_addr = DT_PROP_OR(slave, local_mac_address, {0})      \
	};                                                                 \
	NET_DEVICE_INIT_INSTANCE(CONCAT(dsa_slave_port_, slave),           \
	"lan" STRINGIFY(n),                                                \
	n,                                                                 \
	dsa_port_init,                                                     \
	NULL,                                                              \
	&dsa_context_##n,                                                  \
	&dsa_0_slave_##slave##_config,                                     \
	CONFIG_ETH_INIT_PRIORITY,                                          \
	&dsa_eth_api_funcs,                                                \
	ETHERNET_L2,                                                       \
	NET_L2_GET_CTX_TYPE(ETHERNET_L2),                                  \
	NET_ETH_MTU);

#define NET_SLAVE_DEVICE_0_INIT_INSTANCE(slave)				\
		NET_SLAVE_DEVICE_INIT_INSTANCE(slave, 0)
#define NET_SLAVE_DEVICE_1_INIT_INSTANCE(slave)				\
		NET_SLAVE_DEVICE_INIT_INSTANCE(slave, 1)
#define NET_SLAVE_DEVICE_2_INIT_INSTANCE(slave)				\
		NET_SLAVE_DEVICE_INIT_INSTANCE(slave, 2)
#define NET_SLAVE_DEVICE_3_INIT_INSTANCE(slave)				\
		NET_SLAVE_DEVICE_INIT_INSTANCE(slave, 3)
#define NET_SLAVE_DEVICE_4_INIT_INSTANCE(slave)				\
		NET_SLAVE_DEVICE_INIT_INSTANCE(slave, 4)

#if defined(CONFIG_DSA_SPI)
#define DSA_SPI_BUS_CONFIGURATION(n)					\
	.spi = SPI_DT_SPEC_INST_GET(n,					\
			SPI_WORD_SET(8),				\
			0U)
#else
#define DSA_SPI_BUS_CONFIGURATION(n)
#endif

#define DSA_DEVICE(n)							\
	static struct ksz8xxx_data dsa_device_prv_data_##n = {		\
		.iface_init_count = 0,					\
		.is_init = false,					\
		DSA_SPI_BUS_CONFIGURATION(n),				\
	};								\
	static struct dsa_context dsa_context_##n = {			\
		.num_slave_ports = DT_INST_PROP(0, dsa_slave_ports),	\
		.dapi = &dsa_api_f,					\
		.prv_data = (void *)&dsa_device_prv_data_##n,		\
	};								\
	DT_INST_FOREACH_CHILD_VARGS(n, NET_SLAVE_DEVICE_INIT_INSTANCE, n);

DT_INST_FOREACH_STATUS_OKAY(DSA_DEVICE);
