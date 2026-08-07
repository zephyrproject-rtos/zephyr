/*
 * Copyright (c) 2026 Vilhelm Engström
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/dsa_core.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/phy.h>
#include <zephyr/sys/atomic.h>

#include "dsa_ksz8463.h"

LOG_MODULE_REGISTER(ksz8463, CONFIG_ETHERNET_LOG_LEVEL);

/* The driver should adhere to the following conventions
 *
 * - Variables/fields named dev always refer to the switch device
 * - Variables/fields named portdev, or variations thereof, refer to switch ports
 * - Variables/fields named phydev refer to PHYs
 *
 * The above implies that
 *
 * dev->data        is always a struct ksz8463_data *
 * dev->config      is always a const struct ksz8463_config *
 * portdev->data    is always a struct dsa_switch_context *
 * portdev->config  is always a const struct dsa_port_config *
 * phydev->data     is always a struct ksz8463_phy_data *
 * phydev->config   is always a const struct ksz8463_phy_config *
 */

/* compatible = "microchip,ksz8463" */
#define DT_DRV_COMPAT microchip_ksz8463

/* Maximum time to wait for initial reply from the chip, in ms */
#define KSZ8463_SPI_OPERATIONAL_TIMEOUT_MS CONFIG_DSA_MICROCHIP_KSZ8463_SPI_OPERATIONAL_TIMEOUT

/* Maximum time to wait for initial reply from the chip as a k_timeout_t */
#define KSZ8463_SPI_OPERATIONAL_TIMEOUT K_MSEC(KSZ8463_SPI_OPERATIONAL_TIMEOUT_MS)

/* Interval at which link state is to be polled if no int-gpios is available */
#define KSZ8463_LINK_POLL_INTVL K_MSEC(CONFIG_DSA_MICROCHIP_KSZ8463_LINK_STATE_POLL_INTERVAL)

/* Interval at which to poll for auto-negotiation completion */
#define KSZ8463_PHY_AUTONEG_POLL_INTVL                                                             \
	K_MSEC(CONFIG_DSA_MICROCHIP_KSZ8463_PHY_AUTONEG_POLL_INTERVAL)

/* Maximum time to wait for auto-negotiation completion */
#define KSZ8463_PHY_AUTONEG_TIMEOUT K_MSEC(CONFIG_DSA_MICROCHIP_KSZ8463_PHY_AUTONEG_TIMEOUT)

BUILD_ASSERT(CONFIG_DSA_MICROCHIP_KSZ8463_PHY_AUTONEG_POLL_INTERVAL <
	     CONFIG_DSA_MICROCHIP_KSZ8463_PHY_AUTONEG_TIMEOUT);

/* Switch mutable state */
struct ksz8463_data {
	/* Chip ID, 0x04 for MII, 0x05 for RMII */
	uint8_t chip_id;

	/* Work scheduled on chip IRQ */
	struct k_work_delayable chip_isr_dwork;

	/* Mutex protecting SPI transactions */
	struct k_mutex spi_mutex;

	/* Callback invoked on chip IRQ */
	struct gpio_callback chip_cb;

	/* Array of port devices. Disabled ports are left as NULL */
	const struct device **const portdevs;
};

/* Switch configuration */
struct ksz8463_config {

	/* Whether or not MLD snooping should be enabled */
	bool mld_snoop_en;

	/* Whether or not IGMP snooping should be enabled */
	bool igmp_snoop_en;

	/* Whether or not to enable legal max packet size check */
	bool pkt_sz_chk_en;

	/* Number of port devices in the data portdevs array */
	uint8_t num_portdevs;

	/* Port LED mode */
	uint8_t led_mode;

	/* Reset GPIO. NULL if not set in the dts */
	struct gpio_dt_spec *rst_gpio;

	/* IRQ GPIO. NULL if not set in the dts */
	struct gpio_dt_spec *irq_gpio;

	/* For SPI bus access */
	struct spi_dt_spec spi;

	/* Associated switch device */
	const struct device *dev;
};

/* Private data available in the dsa_switch_context */
struct ksz8463_prv_data {

	/* Associated switch device */
	const struct device *const dev;
};

/* Per-port configuration */
struct ksz8463_port_config {

	/* Whether or not to disable EEE */
	const bool disable_eee;

	/* Whether or not EEE is currently enabled */
	bool eee_enabled;

	/* Associated switch device */
	const struct device *const dev;
};

/* PHY mutable state */
struct ksz8463_phy_data {

	/* Whether or not auto-negotiation is currently enabled */
	bool autoneg_en;

	/* Whether or not auto-negotiation is allowed */
	bool use_fixed_link;

	/* Fixed link speed */
	uint8_t fixed_link_speed;

	/* Atomically assessible struct phy_link_state */
	atomic_t link_state;

	/* Auto-negotiation expiration point */
	k_timepoint_t autoneg_expiry;

	/* Auto-negotiation work */
	struct k_work_delayable dwork;

	/* Mutex protecting API-related state */
	struct k_mutex api_mutex;

	/* Callback to invoke on link state change */
	phy_callback_t phy_cb;

	/* Data to include in link state change callback */
	void *phy_cb_data;

	/* Associated PHY device */
	const struct device *const phydev;
};

/* PHY configuration */
struct ksz8463_phy_config {

	/* Associated port device */
	const struct device *portdev;
};

static inline const struct device *ksz8463_port_to_switch_dev(const struct device *portdev)
{
	const struct dsa_port_config *dsacfg = portdev->config;
	const struct ksz8463_port_config *portcfg = dsacfg->prv_config;

	return portcfg->dev;
}

static inline const struct device *ksz8463_phy_to_port_dev(const struct device *phydev)
{
	const struct ksz8463_phy_config *phycfg = phydev->config;

	return phycfg->portdev;
}

static inline const struct device *ksz8463_phy_to_switch_dev(const struct device *phydev)
{
	const struct device *portdev = ksz8463_phy_to_port_dev(phydev);

	return ksz8463_port_to_switch_dev(portdev);
}

static inline const struct device *ksz8463_port_to_phy_dev(const struct device *portdev)
{
	const struct dsa_port_config *dsacfg = portdev->config;

	return dsacfg->phy_dev;
}

static inline const struct device *ksz8463_spi_to_switch_dev(const struct spi_dt_spec *spi)
{
	const struct ksz8463_config *cfg = CONTAINER_OF(spi, struct ksz8463_config, spi);

	return cfg->dev;
}

static inline const struct device *
ksz8463_switch_ctx_to_switch_dev(const struct dsa_switch_context *dsa_switch_ctx)
{
	const struct ksz8463_prv_data *prv_data = dsa_switch_ctx->prv_data;

	return prv_data->dev;
}

static inline atomic_val_t ksz8463_link_state(enum phy_link_speed speed, bool is_up)
{
	return (speed << 1) | !!is_up;
}

static inline enum phy_link_speed ksz8463_link_state_speed(atomic_val_t state)
{
	return state >> 1;
}

static inline bool ksz8463_link_state_up(atomic_val_t state)
{
	return state & 1;
}

static inline bool ksz8463_have_hard_reset(const struct device *dev)
{
	const struct ksz8463_config *cfg = dev->config;

	return !!cfg->rst_gpio;
}

static inline bool ksz8463_have_irq_gpio(const struct device *dev)
{
	const struct ksz8463_config *cfg = dev->config;

	return !!cfg->irq_gpio;
}

static int ksz8463_spi_lock(const struct spi_dt_spec *spi)
{
	struct ksz8463_data *data;
	const struct device *dev = ksz8463_spi_to_switch_dev(spi);

	data = dev->data;

	return k_mutex_lock(&data->spi_mutex, K_MSEC(50));
}

static void ksz8463_spi_unlock(const struct spi_dt_spec *spi)
{
	struct ksz8463_data *data;
	const struct device *dev = ksz8463_spi_to_switch_dev(spi);

	data = dev->data;
	k_mutex_unlock(&data->spi_mutex);
}

static uint16_t ksz8463_spi_cmd(uint16_t reg, size_t size)
{
	/* Chip employs a 4-byte aligned, 11 bit address. Accessing unaligned addresses
	 * requires a wider read at the nearest, aligned address.
	 *
	 * Address is sent in bits 6:0 of the first byte in the command phase and bits
	 * 7:5 of the second.
	 *
	 * See Table 3-14 in the data sheet.
	 */
	reg = (reg >> 2u) << 4u;
	switch (size) {
	case 1u:
		reg |= (1u << (reg & 0x03u));
		break;
	case 2u:
		reg |= (0x03 << (reg & 0x02));
		break;
	default:
		reg |= 0x0fu;
		break;
	}

	return reg << 2u;
}

/* Read n bytes over SPI. The SPI mutex should be held */
static int ksz8463_spi_read_raw(const struct spi_dt_spec *spi, uint16_t addr, void *dst, size_t n)
{
	int ret;
	uint8_t cmd[sizeof(addr)];

	const struct spi_buf tx_buf[] = {
		/* clang-format and checkpatch disagree */
		/* clang-format off */
		[0] = {
			.buf = cmd,
			.len = sizeof(cmd),
		},
		/* clang-format on */
	};

	const struct spi_buf rx_buf[] = {
		/* clang-format off */
		[0] = {
			.buf = cmd,
			.len = sizeof(cmd),
		},
		[1] = {
			.buf = dst,
			.len = n
		},
		/* clang-format on */
	};

	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf),
	};

	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(rx_buf),
	};

	if (unlikely(n > KSZ8463_SPI_CMD_MAX_DATA_PH_SIZE)) {
		return -ENOBUFS;
	}

	addr = ksz8463_spi_cmd(addr, n);
	sys_put_be16(addr, cmd);

	ret = spi_transceive_dt(spi, &tx, &rx);

	return ret < 0 ? ret : 0;
}

/* Read 4 bytes over SPI. The SPI lock should be held */
static int ksz8463_spi_read32_raw(const struct spi_dt_spec *spi, uint16_t addr, uint32_t *out)
{
	int ret;

	/* Address must be 4-byte aligned */
	if (unlikely(addr & 0x03)) {
		return -EFAULT;
	}

	ret = ksz8463_spi_read_raw(spi, addr, out, sizeof(*out));
	if (ret != 0) {
		return ret;
	}

	LOG_DBG("Read 0x%04" PRIx16 " (0x%04" PRIx16 "): 0x%08" PRIx32, addr,
		ksz8463_spi_cmd(addr, sizeof(*out)), *out);
	return 0;
}

/* Like ksz8463_spi_read32_raw but with automatic lock acquisition */
static inline int ksz8463_spi_read32(const struct spi_dt_spec *spi, uint16_t addr, uint32_t *out)
{
	int ret = ksz8463_spi_lock(spi);

	if (ret != 0) {
		return ret;
	}

	ret = ksz8463_spi_read32_raw(spi, addr, out);
	ksz8463_spi_unlock(spi);
	return ret;
}

/* Read 16 bits from register, aligning addr if required. The SPI mutex should be held */
static int ksz8463_spi_read16_raw(const struct spi_dt_spec *spi, uint16_t addr, uint16_t *out)
{
	int ret;
	uint32_t be32;

	/* Must be 2-byte aligned */
	if (unlikely(addr & 0x01u)) {
		return -EFAULT;
	}

	/* Driver supports access only at 4 byte boundaries */
	if (addr & 0x02u) {
		ret = ksz8463_spi_read32_raw(spi, addr & ~0x02u, &be32);
		if (ret != 0) {
			return ret;
		}

		*out = (uint16_t)(be32 >> 16u);
		return 0;
	}

	ret = ksz8463_spi_read_raw(spi, addr, out, sizeof(*out));
	if (ret != 0) {
		return ret;
	}

	LOG_DBG("Read 0x%04" PRIx16 " (0x%04" PRIx16 "): 0x%04" PRIx16, addr,
		ksz8463_spi_cmd(addr, sizeof(*out)), *out);
	return 0;
}

/* Like ksz8463_spi_read16_raw but with automatic lock acquisition */
static inline int ksz8463_spi_read16(const struct spi_dt_spec *spi, uint16_t addr, uint16_t *out)
{
	int ret = ksz8463_spi_lock(spi);

	if (ret != 0) {
		return ret;
	}

	ret = ksz8463_spi_read16_raw(spi, addr, out);
	ksz8463_spi_unlock(spi);
	return ret;
}

/* Read 8 bytes from register, aligning addr if required. The SPI mutex should be held */
static int ksz8463_spi_read8_raw(const struct spi_dt_spec *spi, uint16_t addr, uint8_t *out)
{
	int ret;
	uint16_t be16;

	/* Device supports accesses only at 4 byte boundaries */
	if (addr & 0x03u) {
		ret = ksz8463_spi_read16_raw(spi, addr & ~0x01u, &be16);
		if (ret != 0) {
			return ret;
		}

		*out = (uint8_t)(be16 >> ((addr & 0x01) << 0x03u));
		return 0;
	}

	ret = ksz8463_spi_read_raw(spi, addr, out, sizeof(*out));
	if (ret != 0) {
		return ret;
	}

	LOG_DBG("Read 0x%04" PRIx16 "(0x%04" PRIx16 "): 0x%02" PRIx8, addr,
		ksz8463_spi_cmd(addr, sizeof(*out)), *out);
	return 0;
}

/* Like ksz8463_spi_read8_raw but with automatic acquisition of the SPI mutex */
static inline int ksz8463_spi_read8(const struct spi_dt_spec *spi, uint16_t addr, uint8_t *out)
{
	int ret = ksz8463_spi_lock(spi);

	if (ret != 0) {
		return ret;
	}

	ret = ksz8463_spi_read8_raw(spi, addr, out);
	ksz8463_spi_unlock(spi);
	return ret;
}

/* Write n bytes over SPI. The SPI mutex should be held */
static int ksz8463_spi_write_raw(const struct spi_dt_spec *spi, uint16_t addr, void *value,
				 size_t n)
{
	int ret;
	uint8_t cmd[sizeof(addr)];
	const struct spi_buf spibuf[] = {
		/* clang-format and checkpatch disagree */
		/* clang-format off */
		[0] = {
			.buf = cmd,
			.len = sizeof(cmd)
		},
		[1] = {
			.buf = value,
			.len = n
		}
		/* clang-format on */
	};

	struct spi_buf_set tx = {
		.buffers = spibuf,
		.count = ARRAY_SIZE(spibuf),
	};

	if (unlikely(n > KSZ8463_SPI_CMD_MAX_DATA_PH_SIZE)) {
		return -ENOTSUP;
	}

	addr = ksz8463_spi_cmd(addr, n);
	sys_put_be16(addr, cmd);
	cmd[0] |= KSZ8463_SPI_CMD_WR;

	ret = spi_write_dt(spi, &tx);

	return ret < 0 ? ret : 0;
}

/* Write 32 bits over SPI. The SPI mutex should be held */
static int ksz8463_spi_write32_raw(const struct spi_dt_spec *spi, uint16_t addr, uint32_t value)
{
	/* Must be 4-byte aligned */
	if (unlikely(addr & 0x03u)) {
		return -EFAULT;
	}

	LOG_DBG("Write 0x%04" PRIx16 "(0x%04" PRIx16 "): 0x%08" PRIx32, addr,
		ksz8463_spi_cmd(addr, sizeof(value)), value);
	return ksz8463_spi_write_raw(spi, addr, &value, sizeof(value));
}

/* Like ksz8463_spi_write32_raw but acquires the SPI mutex */
static inline int ksz8463_spi_write32(const struct spi_dt_spec *spi, uint16_t addr, uint32_t value)
{
	int ret = ksz8463_spi_lock(spi);

	if (ret != 0) {
		return ret;
	}

	ret = ksz8463_spi_write32_raw(spi, addr, value);
	ksz8463_spi_unlock(spi);
	return ret;
}

/* Update bits in 32 bit register. The new value is passed in the bits parameter
 * whereas the mask determines which bits are to be updated. The SPI mutex
 * should be held
 */
static int ksz8463_spi_update_bits32_raw(const struct spi_dt_spec *spi, uint16_t addr,
					 uint32_t bits, uint32_t mask)
{
	int ret;
	uint32_t be32;

	/* Mask must cover all set bits */
	if (unlikely(bits & ~mask)) {
		return -EINVAL;
	}

	ret = ksz8463_spi_read32_raw(spi, addr, &be32);
	if (ret != 0) {
		return ret;
	}

	be32 &= ~mask;
	be32 |= bits;

	return ksz8463_spi_write32_raw(spi, addr, be32);
}

/* Write 16 bit register over SPI, aligning accesses as required. The SPI mutex should be held */
static int ksz8463_spi_write16_raw(const struct spi_dt_spec *spi, uint16_t addr, uint16_t value)
{
	/* Access supported only at 4 byte boundaries */
	if (addr & 0x02) {
		return ksz8463_spi_update_bits32_raw(spi, addr & ~0x02u, value << 16u, 0xffff0000u);
	}

	LOG_DBG("Write 0x%04" PRIx16 "(0x%04" PRIx16 "): 0x%04" PRIx16, addr,
		ksz8463_spi_cmd(addr, sizeof(value)), value);
	return ksz8463_spi_write_raw(spi, addr, &value, sizeof(value));
}

/* Like ksz8463_spi_write16_raw but with acquisition of the SPI mutex */
static inline int ksz8463_spi_write16(const struct spi_dt_spec *spi, uint16_t addr, uint16_t value)
{
	int ret = ksz8463_spi_lock(spi);

	if (ret != 0) {
		return ret;
	}

	ret = ksz8463_spi_write16_raw(spi, addr, value);
	ksz8463_spi_unlock(spi);
	return ret;
}

/* Like ksz8463_spi_update_bits32_raw but with 16 bit registers */
static int ksz8463_spi_update_bits16_raw(const struct spi_dt_spec *spi, uint16_t addr,
					 uint16_t bits, uint16_t mask)
{
	int ret;
	uint16_t be16;

	if (unlikely(bits & ~mask)) {
		return -EINVAL;
	}

	ret = ksz8463_spi_read16_raw(spi, addr, &be16);
	if (ret != 0) {
		return ret;
	}

	be16 &= ~mask;
	be16 |= bits;

	return ksz8463_spi_write16_raw(spi, addr, be16);
}

/* Like ksz8463_spi_update_bits16_raw but with automatic SPI lock acquisition */
static inline int ksz8463_spi_update_bits16(const struct spi_dt_spec *spi, uint16_t addr,
					    uint16_t bits, uint16_t mask)
{
	int ret = ksz8463_spi_lock(spi);

	if (ret != 0) {
		return ret;
	}

	ret = ksz8463_spi_update_bits16_raw(spi, addr, bits, mask);
	ksz8463_spi_unlock(spi);
	return ret;
}

/* Write 8 bits over SPI, aligning access as required. The SPI mutex should be held */
static int ksz8463_spi_write8_raw(const struct spi_dt_spec *spi, uint16_t addr, uint8_t value)
{
	if (addr & 0x03u) {
		return ksz8463_spi_update_bits16_raw(spi, addr & ~0x01u,
						     value << ((addr & 0x01u) << 3u),
						     0xff << ((addr & 0x01u) << 3u));
	}

	LOG_DBG("Write 0x%04" PRIx16 "(0x%04" PRIx16 "): 0x%02" PRIx8, addr,
		ksz8463_spi_cmd(addr, sizeof(value)), value);
	return ksz8463_spi_write_raw(spi, addr, &value, sizeof(value));
}

/* Like ksz8463_spi_write8_raw but with automatic SPI lock acquisition */
static inline int ksz8463_spi_write8(const struct spi_dt_spec *spi, uint16_t addr, uint8_t value)
{
	int ret = ksz8463_spi_lock(spi);

	if (ret != 0) {
		return ret;
	}

	ret = ksz8463_spi_write8_raw(spi, addr, value);
	ksz8463_spi_unlock(spi);
	return ret;
}

/* Like ksz8463_spi_update_bits16_raw but with 8 bit registers */
static int ksz8463_spi_update_bits8_raw(const struct spi_dt_spec *spi, uint16_t addr, uint8_t bits,
					uint8_t mask)
{
	int ret;
	uint8_t b;

	if (unlikely(bits & ~mask)) {
		return -EINVAL;
	}

	ret = ksz8463_spi_read8_raw(spi, addr, &b);
	if (ret != 0) {
		return ret;
	}

	b &= ~mask;
	b |= bits;

	return ksz8463_spi_write8_raw(spi, addr, b);
}

/* Lock the SPI mutex and invoke ksz8463_spi_update_bits8_raw */
static inline int ksz8463_spi_update_bits8(const struct spi_dt_spec *spi, uint16_t addr,
					   uint8_t bits, uint8_t mask)
{
	int ret = ksz8463_spi_lock(spi);

	if (ret != 0) {
		return ret;
	}

	ret = ksz8463_spi_update_bits8_raw(spi, addr, bits, mask);
	ksz8463_spi_unlock(spi);
	return ret;
}

static bool ksz8463_is_cpu_port(const struct device *portdev)
{
	struct ethernet_context *eth_ctx;
	struct net_if *iface = net_if_lookup_by_dev(portdev);

	if (unlikely(!iface)) {
		return false;
	}

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return false;
	}

	eth_ctx = net_if_l2_data(iface);
	return eth_ctx->dsa_port == DSA_CPU_PORT;
}

static int ksz8463_eee_on_off(const struct device *portdev, bool enable, bool lock_spi)
{
	int ret;
	const struct ksz8463_config *cfg;
	const struct dsa_port_config *dsacfg = portdev->config;
	struct ksz8463_port_config *portcfg = dsacfg->prv_config;
	const struct device *dev = ksz8463_port_to_switch_dev(portdev);

	cfg = dev->config;

	if (unlikely(dsacfg->port_idx >= cfg->num_portdevs)) {
		return -EINVAL;
	}

	if (lock_spi) {
		ret = ksz8463_spi_lock(&cfg->spi);
		if (ret != 0) {
			return ret;
		}
	}
	if (portcfg->eee_enabled != enable) {
		BUILD_ASSERT(KSZ8463_PCSEEEC_P1_NEXT_PG_EN == BIT(0));
		BUILD_ASSERT(KSZ8463_PCSEEEC_P2_NEXT_PG_EN == BIT(1));

		ret = ksz8463_spi_update_bits8_raw(&cfg->spi, KSZ8463_REG_PCSEEEC,
						   enable ? BIT(dsacfg->port_idx) : 0,
						   BIT(dsacfg->port_idx));
		if (ret == 0) {
			portcfg->eee_enabled = enable;
		}
	}
	if (lock_spi) {
		ksz8463_spi_unlock(&cfg->spi);
	}

	return ret;
}

static inline int ksz8463_eee_disable(const struct device *portdev)
{
	return ksz8463_eee_on_off(portdev, false, true);
}

static inline int ksz8463_eee_disable_raw(const struct device *portdev)
{
	return ksz8463_eee_on_off(portdev, false, false);
}

static inline int ksz8463_eee_enable_raw(const struct device *portdev)
{
	return ksz8463_eee_on_off(portdev, true, false);
}

static int ksz8463_phy_validate_mode(const struct device *phydev)
{
	char const *modes = "rmii";
	const struct ksz8463_data *data;
	const struct dsa_port_config *dsacfg;
	const struct ksz8463_port_config *portcfg;
	const struct device *dev = ksz8463_phy_to_switch_dev(phydev);
	const struct device *portdev = ksz8463_phy_to_port_dev(phydev);

	data = dev->data;
	dsacfg = portdev->config;
	portcfg = dsacfg->prv_config;

	if (!ksz8463_is_cpu_port(portdev)) {
		/* User ports use internal PHYs */
		return strcmp(dsacfg->phy_mode, "internal") ? -EINVAL : 0;
	}

	switch (data->chip_id) {
	case KSZ8463_MII_CHIP_ID:
		/* "rmii" -> "mii" */
		++modes;
		break;
	case KSZ8463_RMII_CHIP_ID:
		break;
	default:
		LOG_ERR("Unexpected chip ID: 0x%x", (unsigned int)data->chip_id);
		return -ENOTSUP;
	}

	return strcmp(dsacfg->phy_mode, modes) ? -EINVAL : 0;
}

static int ksz8463_phy_link_is_up(const struct device *phydev, bool *is_up)
{
	int ret;
	uint8_t pxsr;
	const struct ksz8463_config *cfg;
	const struct dsa_port_config *dsacfg;
	const struct device *dev = ksz8463_phy_to_switch_dev(phydev);
	const struct device *portdev = ksz8463_phy_to_port_dev(phydev);

	dsacfg = portdev->config;
	cfg = dev->config;

	if (unlikely(ksz8463_is_cpu_port(portdev))) {
		/* CPU link should always be up */
		*is_up = true;
		return 0;
	}

	ret = ksz8463_spi_read8(&cfg->spi, KSZ8463_REG_PxSR_LO(dsacfg->port_idx), &pxsr);
	if (ret != 0) {
		return ret;
	}

	*is_up = !!(pxsr & KSZ8463_PxSR_LO_LINK_STATUS);
	return 0;
}

static int ksz8463_phy_partner_auto_negotiation_capable(const struct device *phydev, bool *capable)
{
	int ret;
	uint8_t pxeeecs;
	const struct ksz8463_config *cfg;
	const struct dsa_port_config *dsacfg;
	const struct device *dev = ksz8463_phy_to_switch_dev(phydev);
	const struct device *portdev = ksz8463_phy_to_port_dev(phydev);

	cfg = dev->config;
	dsacfg = portdev->config;

	if (unlikely(ksz8463_is_cpu_port(portdev))) {
		/* CPU port should use fix link */
		*capable = false;
		return 0;
	}

	ret = ksz8463_spi_read8(&cfg->spi, KSZ8463_REG_PxEEECS_LO(dsacfg->port_idx), &pxeeecs);
	if (ret != 0) {
		return ret;
	}

	*capable = !!(pxeeecs & KSZ8463_PxEEECS_LO_LNK_AUTONEG_CPBL);
	return 0;
}

static int ksz8463_phy_link_configured(const struct device *phydev)
{
	uint8_t pxsr;
	struct phy_link_state state;
	int ret, base100, full_duplex;
	const struct ksz8463_config *cfg;
	const struct dsa_port_config *dsacfg;
	struct ksz8463_phy_data *phydata = phydev->data;
	const struct device *dev = ksz8463_phy_to_switch_dev(phydev);
	const struct device *portdev = ksz8463_phy_to_port_dev(phydev);

	cfg = dev->config;
	dsacfg = portdev->config;

	ret = ksz8463_spi_read8(&cfg->spi, KSZ8463_REG_PxSR_HI(dsacfg->port_idx), &pxsr);
	if (ret != 0) {
		return ret;
	}

	base100 = !!(pxsr & KSZ8463_PxSR_HI_OPER_SPEED_100MBPS);
	full_duplex = !!(pxsr & KSZ8463_PxSR_HI_OPER_FULL_DUPLEX);

	state.speed = 0;
	switch ((base100 << 1) | full_duplex) {
	case KSZ8463_SPEED_100BASE_FULL_DUPLEX:
		state.speed = LINK_FULL_100BASE;
		break;
	case KSZ8463_SPEED_100BASE_HALF_DUPLEX:
		state.speed = LINK_HALF_100BASE;
		break;
	case KSZ8463_SPEED_10BASE_FULL_DUPLEX:
		state.speed = LINK_FULL_10BASE;
		break;
	case KSZ8463_SPEED_10BASE_HALF_DUPLEX:
		state.speed = LINK_HALF_10BASE;
		break;
	default:
		CODE_UNREACHABLE;
	}

	LOG_INF("Speed 10%sMbps, %s-duplex", PHY_LINK_IS_SPEED_100M(state.speed) ? "0" : "",
		PHY_LINK_IS_FULL_DUPLEX(state.speed) ? "full" : "half");

	ret = k_mutex_lock(&phydata->api_mutex, K_FOREVER);
	if (ret != 0) {
		return ret;
	}

	/* Writes require locks to ensure assignments and callback invocations
	 * are not mixed on concurrent PHY state changes
	 */
	atomic_set(&phydata->link_state, ksz8463_link_state(state.speed, true));

	state.is_up = true;
	if (phydata->phy_cb) {
		phydata->phy_cb(phydev, &state, phydata->phy_cb_data);
	}

	k_mutex_unlock(&phydata->api_mutex);
	return ret;
}

static int ksz8463_phy_auto_negotiation_on_off(const struct device *phydev, bool enable)
{
	int ret, rr;
	bool disable_eee;
	const struct ksz8463_config *cfg;
	const struct dsa_port_config *dsacfg;
	const struct ksz8463_port_config *portcfg;
	struct ksz8463_phy_data *phydata = phydev->data;
	const uint8_t bit = KSZ8463_PxCR4_LO_AUTONEG_EN;
	const struct device *dev = ksz8463_phy_to_switch_dev(phydev);
	const struct device *portdev = ksz8463_phy_to_port_dev(phydev);

	cfg = dev->config;
	dsacfg = portdev->config;
	portcfg = dsacfg->prv_config;

	if (enable && phydata->use_fixed_link) {
		return -ENOTSUP;
	}

	if (phydata->autoneg_en == enable) {
		return 0;
	}

	ret = ksz8463_spi_lock(&cfg->spi);
	if (ret != 0) {
		return ret;
	}

	/* Errata 4 fromKZ8463_MLI_FMLI_PROD_0.3_errata. Temporarily disable EEE when disabling
	 * auto-negotiation
	 */
	disable_eee = portcfg->eee_enabled && !enable;
	if (disable_eee) {
		ret = ksz8463_eee_disable_raw(portdev);
	}

	if (ret == 0) {
		ret = ksz8463_spi_update_bits8_raw(
			&cfg->spi, KSZ8463_REG_PxCR4_LO(dsacfg->port_idx), enable ? bit : 0, bit);

		if (disable_eee) {
			rr = ksz8463_eee_enable_raw(portdev);

			if (rr != 0) {
				LOG_ERR("Error reenabling EEE on port %d: %d", dsacfg->port_idx,
					-rr);
			}
		}
	}
	if (ret == 0) {
		phydata->autoneg_en = enable;
	}

	ksz8463_spi_unlock(&cfg->spi);
	return ret;
}

static inline int ksz8463_phy_enable_auto_negotiation(const struct device *phydev)
{
	return ksz8463_phy_auto_negotiation_on_off(phydev, true);
}

static inline int ksz8463_phy_disable_auto_negotiation(const struct device *phydev)
{
	return ksz8463_phy_auto_negotiation_on_off(phydev, false);
}

static int ksz8463_phy_restart_auto_negotiation(const struct device *phydev)
{
	const struct ksz8463_config *cfg;
	const struct dsa_port_config *dsacfg;
	const struct device *dev = ksz8463_phy_to_switch_dev(phydev);
	const struct device *portdev = ksz8463_phy_to_port_dev(phydev);

	cfg = dev->config;
	dsacfg = portdev->config;

	return ksz8463_spi_update_bits8(&cfg->spi, KSZ8463_REG_PxCR4_HI(dsacfg->port_idx),
					KSZ8463_PxCR4_HI_AUTONEG_RESTART,
					KSZ8463_PxCR4_HI_AUTONEG_RESTART);
}

static int ksz8463_phy_configure_fixed_link(const struct device *phydev)
{
	int ret;
	uint8_t bits;
	const struct ksz8463_config *cfg;
	const struct dsa_port_config *dsacfg;
	const struct ksz8463_phy_data *phydata = phydev->data;
	const struct device *dev = ksz8463_phy_to_switch_dev(phydev);
	const struct device *portdev = ksz8463_phy_to_port_dev(phydev);
	const uint8_t mask =
		KSZ8463_PxMBCR_HI_FORCE_100BASE_TX | KSZ8463_PxMBCR_HI_FORCE_FULL_DUPLEX;

	dsacfg = portdev->config;
	cfg = dev->config;
	bits = 0;

	ret = ksz8463_phy_disable_auto_negotiation(phydev);
	if (ret != 0) {
		return ret;
	}

	if (phydata->fixed_link_speed & KSZ8463_SPEED_FULL_DUPLEX_BIT) {
		bits |= KSZ8463_PxMBCR_HI_FORCE_FULL_DUPLEX;
	}

	if (phydata->fixed_link_speed & KSZ8463_SPEED_100BASE_BIT) {
		bits |= KSZ8463_PxMBCR_HI_FORCE_100BASE_TX;
	}

	ret = ksz8463_spi_update_bits8(&cfg->spi, KSZ8463_REG_PxMBCR_HI(dsacfg->port_idx), bits,
				       mask);
	if (ret != 0) {
		return ret;
	}

	return ksz8463_phy_link_configured(phydev);
}

static int ksz8463_phy_start_auto_negotiation(const struct device *phydev)
{
	int ret;
	const struct dsa_port_config *dsacfg;
	struct ksz8463_phy_data *phydata = phydev->data;
	const struct device *portdev = ksz8463_phy_to_port_dev(phydev);

	dsacfg = portdev->config;

	ret = ksz8463_phy_enable_auto_negotiation(phydev);
	if (ret != 0) {
		return ret;
	}

	ret = ksz8463_phy_restart_auto_negotiation(phydev);
	if (ret != 0) {
		return ret;
	}

	phydata->autoneg_expiry = sys_timepoint_calc(KSZ8463_PHY_AUTONEG_TIMEOUT);
	k_work_reschedule(&phydata->dwork, KSZ8463_PHY_AUTONEG_POLL_INTVL);
	return 0;
}

static void ksz8463_phy_dwork(struct k_work *work)
{
	int ret;
	bool link_up;
	uint8_t pxmbsr;
	const struct ksz8463_config *cfg;
	const struct dsa_port_config *dsacfg;
	const struct device *dev, *portdev, *phydev;
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct ksz8463_phy_data *phydata = CONTAINER_OF(dwork, struct ksz8463_phy_data, dwork);

	phydev = phydata->phydev;
	dev = ksz8463_phy_to_switch_dev(phydev);
	portdev = ksz8463_phy_to_port_dev(phydev);

	cfg = dev->config;
	dsacfg = portdev->config;

	ret = ksz8463_spi_read8(&cfg->spi, KSZ8463_REG_PxMBSR_LO(dsacfg->port_idx), &pxmbsr);
	if (ret == 0 && (pxmbsr & KSZ8463_PxMBSR_AUTONEG_CPLT)) {
		LOG_DBG("Auto-negotiation completed on port %d", dsacfg->port_idx);

		phydata->autoneg_expiry = sys_timepoint_calc(K_NO_WAIT);
		ret = ksz8463_phy_link_configured(phydev);
	} else if (sys_timepoint_expired(phydata->autoneg_expiry)) {
		LOG_INF("Auto-negotiation timed out");

		ret = ksz8463_phy_link_is_up(phydev, &link_up);

		if (ret == 0) {
			if (link_up) {
				/* Fall back on fixed link */
				ret = ksz8463_phy_configure_fixed_link(phydev);
			} else {
				LOG_DBG("Link severed during auto-negotiation");
			}
		}
	} else {
		if (ret != 0) {
			LOG_ERR("Could not read P%dMBSR: %d", dsacfg->port_idx, -ret);
		} else {
			LOG_DBG("Auto-negotiation in process, rescheduling");
		}
		ret = k_work_reschedule(&phydata->dwork, KSZ8463_PHY_AUTONEG_POLL_INTVL);
	}

	if (ret < 0) {
		LOG_ERR("Error when pulling for auto-negotiation completion: %d", -ret);
	}
}

static int ksz8463_phy_configure_link(const struct device *phydev)
{
	int ret;
	bool autoneg_capable;
	const struct ksz8463_phy_data *phydata = phydev->data;

	autoneg_capable = false;
	if (!phydata->use_fixed_link) {
		ret = ksz8463_phy_partner_auto_negotiation_capable(phydev, &autoneg_capable);
		if (ret != 0) {
			return ret;
		}

		if (!autoneg_capable) {
			LOG_INF("Link partner does not support auto-negotiation");
		}
	}

	if (autoneg_capable) {
		ret = ksz8463_phy_start_auto_negotiation(phydev);
	} else {
		ret = ksz8463_phy_configure_fixed_link(phydev);
	}

	return ret;
}

static int ksz8463_phy_init(const struct device *phydev)
{
	int ret;
	bool link_up;
	struct ksz8463_phy_data *phydata = phydev->data;

	k_work_init_delayable(&phydata->dwork, ksz8463_phy_dwork);
	phydata->autoneg_expiry = sys_timepoint_calc(K_NO_WAIT);

	ret = k_mutex_init(&phydata->api_mutex);
	if (ret != 0) {
		return ret;
	}

	ret = ksz8463_phy_validate_mode(phydev);
	if (ret != 0) {
		return ret;
	}

	ret = ksz8463_phy_link_is_up(phydev, &link_up);
	if (ret != 0) {
		return ret;
	}

	if (link_up) {
		atomic_set(&phydata->link_state, ksz8463_link_state(0, true));
		ret = ksz8463_phy_configure_link(phydev);
	}

	return ret;
}

static int ksz8463_phy_link_down(const struct device *phydev)
{
	int ret;
	atomic_val_t state;
	struct phy_link_state link_state;
	struct ksz8463_phy_data *phydata = phydev->data;

	ret = k_mutex_lock(&phydata->api_mutex, K_FOREVER);
	if (ret != 0) {
		return ret;
	}

	/* Assignments must be done in critical sections despite atomicity to ensure CPU
	 * doesn't preempt before callback
	 */
	state = atomic_get(&phydata->link_state);
	if (state) {
		atomic_set(&phydata->link_state, ksz8463_link_state(0, false));

		link_state.is_up = false;
		link_state.speed = 0;

		if (phydata->phy_cb) {
			phydata->phy_cb(phydev, &link_state, phydata->phy_cb_data);
		}
	}

	k_mutex_unlock(&phydata->api_mutex);

	k_work_cancel_delayable(&phydata->dwork);
	phydata->autoneg_expiry = sys_timepoint_calc(K_NO_WAIT);

	return 0;
}

static inline int ksz8463_phy_link_up(const struct device *phydev)
{
	struct ksz8463_phy_data *phydata = phydev->data;

	atomic_set(&phydata->link_state, ksz8463_link_state(0, true));
	return ksz8463_phy_configure_link(phydev);
}

static int ksz8463_phy_get_link(const struct device *phydev, struct phy_link_state *state)
{
	atomic_val_t link_state;
	const struct ksz8463_phy_data *phydata = phydev->data;

	link_state = atomic_get(&phydata->link_state);
	state->is_up = ksz8463_link_state_up(link_state);
	state->speed = ksz8463_link_state_speed(link_state);

	return 0;
}

static void ksz8463_phy_fixed_link_from_advertise(const struct device *phydev,
						  enum phy_link_speed adv_speeds)
{
	struct ksz8463_phy_data *phydata = phydev->data;

	phydata->fixed_link_speed = 0;
	/* Use highest advertised speed */
	if (adv_speeds & (LINK_FULL_10BASE | LINK_FULL_100BASE)) {
		phydata->fixed_link_speed |= KSZ8463_SPEED_FULL_DUPLEX_BIT;
	}

	if (adv_speeds & (LINK_HALF_100BASE | LINK_FULL_100BASE)) {
		phydata->fixed_link_speed |= KSZ8463_SPEED_100BASE_BIT;
	}
}

static int ksz8463_phy_configure_advertise(const struct device *phydev,
					   enum phy_link_speed adv_speeds)
{
	uint16_t bits;
	const struct ksz8463_config *cfg;
	const struct dsa_port_config *dsacfg;
	const struct device *dev = ksz8463_phy_to_switch_dev(phydev);
	const struct device *portdev = ksz8463_phy_to_port_dev(phydev);
	const uint16_t mask = KSZ8463_PxANAR_ADV_100BASE_TX_FULL_DUPLEX |
			      KSZ8463_PxANAR_ADV_100BASE_TX_HALF_DUPLEX |
			      KSZ8463_PxANAR_ADV_10BASE_T_FULL_DUPLEX |
			      KSZ8463_PxANAR_ADV_10BASE_T_HALF_DUPLEX;

	cfg = dev->config;
	dsacfg = portdev->config;

	bits = 0;
	if (adv_speeds & LINK_HALF_10BASE) {
		bits |= KSZ8463_PxANAR_ADV_10BASE_T_HALF_DUPLEX;
	}

	if (adv_speeds & LINK_FULL_10BASE) {
		bits |= KSZ8463_PxANAR_ADV_10BASE_T_FULL_DUPLEX;
	}

	if (adv_speeds & LINK_HALF_100BASE) {
		bits |= KSZ8463_PxANAR_ADV_100BASE_TX_HALF_DUPLEX;
	}

	if (adv_speeds & LINK_FULL_100BASE) {
		bits |= KSZ8463_PxANAR_ADV_100BASE_TX_FULL_DUPLEX;
	}

	return ksz8463_spi_update_bits16(&cfg->spi, KSZ8463_REG_PxANAR(dsacfg->port_idx), bits,
					 mask);
}

static int ksz8463_phy_cfg_link(const struct device *phydev, enum phy_link_speed adv_speeds,
				enum phy_cfg_link_flag flags)
{
	int ret;
	struct ksz8463_phy_data *phydata = phydev->data;
	const uint8_t speedmask =
		LINK_HALF_10BASE | LINK_FULL_10BASE | LINK_HALF_100BASE | LINK_FULL_100BASE;

	if (!(adv_speeds & speedmask)) {
		return -EINVAL;
	}

	ret = k_mutex_lock(&phydata->api_mutex, K_FOREVER);
	if (ret != 0) {
		return ret;
	}

	phydata->use_fixed_link = !!(flags & PHY_FLAG_AUTO_NEGOTIATION_DISABLED);

	ksz8463_phy_fixed_link_from_advertise(phydev, adv_speeds);

	ret = ksz8463_phy_configure_advertise(phydev, adv_speeds);
	if (ret == 0) {
		ret = ksz8463_phy_configure_link(phydev);
	}

	k_mutex_unlock(&phydata->api_mutex);
	return ret;
}

static int ksz8463_phy_link_cb_set(const struct device *phydev, phy_callback_t cb, void *user_data)
{
	int ret;
	atomic_val_t encoded;
	struct phy_link_state state;
	struct ksz8463_phy_data *phydata = phydev->data;

	ret = k_mutex_lock(&phydata->api_mutex, K_FOREVER);
	if (ret != 0) {
		return ret;
	}

	phydata->phy_cb = cb;
	phydata->phy_cb_data = user_data;

	encoded = atomic_get(&phydata->link_state);

	/* Must invoke after setting */

	state.is_up = ksz8463_link_state_up(encoded);
	state.speed = ksz8463_link_state_speed(encoded);
	cb(phydev, &state, user_data);

	k_mutex_unlock(&phydata->api_mutex);

	return 0;
}

static DEVICE_API(ethphy, ksz8463_phy_driver_api) = {
	.get_link = ksz8463_phy_get_link,
	.cfg_link = ksz8463_phy_cfg_link,
	.link_cb_set = ksz8463_phy_link_cb_set,
};

static int ksz8463_link_changed(const struct device *dev)
{
	int ret;
	bool was_up, is_up;
	atomic_val_t link_state;
	const struct dsa_port_config *dsacfg;
	const struct device *portdev, *phydev;
	struct ksz8463_data *data = dev->data;
	const struct ksz8463_phy_data *phydata;
	const struct ksz8463_config *cfg = dev->config;

	ret = 0;
	for (unsigned int i = 0u; ret == 0 && i < cfg->num_portdevs; ++i) {
		portdev = data->portdevs[i];

		/* CPU port's link status won't change, nor will that of disabled ports */
		if (!portdev || ksz8463_is_cpu_port(portdev)) {
			continue;
		}

		dsacfg = portdev->config;
		phydev = ksz8463_port_to_phy_dev(portdev);
		phydata = phydev->data;
		link_state = atomic_get(&phydata->link_state);
		was_up = ksz8463_link_state_up(link_state);

		ret = ksz8463_phy_link_is_up(phydev, &is_up);
		if (ret != 0) {
			break;
		}

		if (was_up && !is_up) {
			/* Chip indicates that the link is bad during auto-negotiation.
			 * Defer potential link down event until auto-negotiation timer has
			 * expired
			 */
			if (sys_timepoint_expired(phydata->autoneg_expiry)) {
				ret = ksz8463_phy_link_down(phydev);
			}
		} else if (!was_up && is_up) {
			ret = ksz8463_phy_link_up(phydev);
		}
	}

	return ret;
}

static void ksz8463_chip_isr(const struct device *gpiodev, struct gpio_callback *cb,
			     gpio_port_pins_t pins)
{
	struct ksz8463_data *data = CONTAINER_OF(cb, struct ksz8463_data, chip_cb);

	k_work_schedule(&data->chip_isr_dwork, K_NO_WAIT);
}

static int ksz8463_handle_chip_isr(const struct device *dev)
{
	int ret;
	uint8_t isr;
	unsigned int key;
	bool lcis_cleared;
	const struct ksz8463_config *cfg = dev->config;

	key = irq_lock();

	/* Chip's internal state may change despite IRQs being disabled */
	do {
		ret = ksz8463_spi_read8(&cfg->spi, KSZ8463_REG_ISR_HI, &isr);
		if (ret != 0) {
			LOG_ERR("Could not read interrupt status: %d", -ret);
			break;
		}

		if (unlikely(!(isr & KSZ8463_ISR_HI_LCIS))) {
			ret = 0;
			break;
		}

		/* Bit is W1C */
		ret = ksz8463_spi_update_bits8(&cfg->spi, KSZ8463_REG_ISR_HI, KSZ8463_ISR_HI_LCIS,
					       KSZ8463_ISR_HI_LCIS);
		lcis_cleared = !ret;
		if (!lcis_cleared) {
			LOG_ERR("Error clearing LCIS: %d", -ret);
		}

		ret = ksz8463_link_changed(dev);
		if (ret != 0) {
			LOG_ERR("Error processing link state change: %d", -ret);
			break;
		}

		/* Assuming LCIS was successfully cleared, loop and read the register. If the chip
		 * has set the bit again, there is a new change to act on
		 */
	} while (lcis_cleared);

	irq_unlock(key);

	return ret;
}

static void ksz8463_chip_isr_dwork(struct k_work *work)
{
	int ret;
	const struct ksz8463_config *cfg;
	const struct device *dev, *portdev;
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct ksz8463_data *data = CONTAINER_OF(dwork, struct ksz8463_data, chip_isr_dwork);

	/* There is at least one non-NULL entry in the portdevs array. Find it */
	for (portdev = data->portdevs[0]; !portdev; ++portdev) {
	}

	dev = ksz8463_port_to_switch_dev(portdev);
	cfg = dev->config;

	(void)ksz8463_handle_chip_isr(dev);

	if (!ksz8463_have_irq_gpio(dev)) {
		ret = k_work_reschedule(&data->chip_isr_dwork, KSZ8463_LINK_POLL_INTVL);

		if (ret < 0) {
			LOG_ERR("Could not reschedule link poll: %d", ret);
		}
	}
}

static int ksz8463_port_enable(const struct device *portdev)
{
	const struct ksz8463_config *cfg;
	const struct dsa_port_config *dsacfg = portdev->config;
	const struct device *dev = ksz8463_port_to_switch_dev(portdev);
	const uint8_t bits = KSZ8463_PxCR2_HI_TX_EN | KSZ8463_PxCR2_HI_RX_EN;
	const uint8_t mask = bits | KSZ8463_PxCR2_HI_LEARN_DIS;

	cfg = dev->config;

	return ksz8463_spi_update_bits8(&cfg->spi, KSZ8463_REG_PxCR2_HI(dsacfg->port_idx), bits,
					mask);
}

static int ksz8463_port_init(const struct device *portdev)
{
	int ret;
	struct ksz8463_data *data;
	const struct ksz8463_config *cfg;
	const struct dsa_port_config *dsacfg = portdev->config;
	const struct device *dev = ksz8463_port_to_switch_dev(portdev);
	const struct device *phydev = ksz8463_port_to_phy_dev(portdev);
	const struct ksz8463_port_config *portcfg = dsacfg->prv_config;

	data = dev->data;
	cfg = dev->config;

	if (unlikely(dsacfg->port_idx >= cfg->num_portdevs)) {
		return -EINVAL;
	}

	ret = ksz8463_port_enable(portdev);
	if (ret != 0) {
		return ret;
	}

	if (!ksz8463_is_cpu_port(portdev)) {
		if (portcfg->disable_eee) {
			ret = ksz8463_eee_disable(portdev);

			if (ret != 0) {
				return ret;
			}
		}

		ret = ksz8463_phy_init(phydev);
	}

	return ret;
}

static int ksz8463_configure_irq(const struct device *dev)
{
	int ret;
	struct ksz8463_data *data = dev->data;
	const struct ksz8463_config *cfg = dev->config;

	if (unlikely(!cfg->irq_gpio)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(cfg->irq_gpio, GPIO_INPUT);
	if (ret != 0) {
		return ret;
	}

	gpio_init_callback(&data->chip_cb, ksz8463_chip_isr, BIT(cfg->irq_gpio->pin));
	gpio_add_callback(cfg->irq_gpio->port, &data->chip_cb);
	ret = gpio_pin_interrupt_configure_dt(cfg->irq_gpio, GPIO_INT_EDGE_FALLING);
	if (ret != 0) {
		return ret;
	}

	ret = ksz8463_spi_write8(&cfg->spi, KSZ8463_REG_IER_HI, KSZ8463_IER_HI_LCIE);
	if (ret != 0) {
		return ret;
	}

	/* Handle potential interrupt occurring before configuration */
	ret = ksz8463_handle_chip_isr(dev);
	if (ret != 0) {
		return ret;
	}

	LOG_INF("Link-change interrupt configured");
	return 0;
}

static int ksz8463_configure_link_state_poll(const struct device *dev)
{
	int ret;
	struct ksz8463_data *data = dev->data;

	LOG_DBG("No IRQ pin provided, falling back on polling");

	ret = k_work_reschedule(&data->chip_isr_dwork, KSZ8463_LINK_POLL_INTVL);
	return ret < 0 ? ret : 0;
}

static inline int ksz8463_configure_leds(const struct device *dev)
{
	const struct ksz8463_config *cfg = dev->config;

	return ksz8463_spi_update_bits8(&cfg->spi, KSZ8463_REG_SGCR7_HI, cfg->led_mode,
					KSZ8463_SGCR7_HI_PORT_LED_MODE_MASK);
}

static inline int ksz8463_configure_legal_packet_size_check(const struct device *dev)
{
	const struct ksz8463_config *cfg = dev->config;
	const uint8_t bit = KSZ8463_SGCR2_LO_LEGAL_PKT_SZ_CHK_EN;

	return ksz8463_spi_update_bits8(&cfg->spi, KSZ8463_REG_SGCR2_LO,
					cfg->pkt_sz_chk_en ? bit : 0, bit);
}

static int ksz8463_configure_snooping(const struct device *dev)
{
	uint8_t bits;
	const struct ksz8463_config *cfg = dev->config;
	const uint8_t mask = KSZ8463_SGCR2_HI_MLD_SNOOP_EN | KSZ8463_SGCR2_HI_IGMP_SNOOP_EN;

	bits = 0;
	if (cfg->mld_snoop_en) {
		bits |= KSZ8463_SGCR2_HI_MLD_SNOOP_EN;
	}
	if (cfg->igmp_snoop_en) {
		bits |= KSZ8463_SGCR2_HI_IGMP_SNOOP_EN;
	}

	return ksz8463_spi_update_bits8(&cfg->spi, KSZ8463_REG_SGCR2_HI, bits, mask);
}

static int ksz8463_switch_setup(const struct dsa_switch_context *dsa_switch_ctx)
{
	int ret;
	struct ksz8463_data *data;
	const struct device *dev = ksz8463_switch_ctx_to_switch_dev(dsa_switch_ctx);

	data = dev->data;

	k_work_init_delayable(&data->chip_isr_dwork, ksz8463_chip_isr_dwork);
	if (ksz8463_have_irq_gpio(dev)) {
		ret = ksz8463_configure_irq(dev);
	} else {
		ret = ksz8463_configure_link_state_poll(dev);
	}
	if (ret != 0) {
		return ret;
	}

	ret = ksz8463_configure_leds(dev);
	if (ret != 0) {
		return ret;
	}

	ret = ksz8463_configure_legal_packet_size_check(dev);
	if (ret != 0) {
		return ret;
	}

	return ksz8463_configure_snooping(dev);
}

static enum ethernet_hw_caps ksz8463_get_capabilities(const struct device *portdev)
{
	ARG_UNUSED(portdev);

	return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE;
}

struct dsa_api ksz8463_dsa_api = {
	.port_init = ksz8463_port_init,
	.switch_setup = ksz8463_switch_setup,
	.get_capabilities = ksz8463_get_capabilities,
};

static int ksz8463_hard_reset(const struct device *dev)
{
	int ret;
	const struct ksz8463_config *cfg = dev->config;

	if (unlikely(!cfg->rst_gpio || !gpio_is_ready_dt(cfg->rst_gpio))) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(cfg->rst_gpio, GPIO_OUTPUT_ACTIVE);
	if (ret != 0) {
		return ret;
	}

	k_sleep(K_MSEC(10));
	ret = gpio_pin_set_dt(cfg->rst_gpio, 0);
	if (ret != 0) {
		return ret;
	}

	k_sleep(K_MSEC(10));
	LOG_DBG("Hard reset complete");
	return 0;
}

static int ksz8463_read_chip_id(const struct device *dev)
{
	int ret;
	uint16_t cider;
	k_timepoint_t expiry;
	unsigned int interval;
	struct ksz8463_data *data = dev->data;
	const struct ksz8463_config *cfg = dev->config;

	if (!spi_is_ready_dt(&cfg->spi)) {
		return -ENODEV;
	}

	expiry = sys_timepoint_calc(KSZ8463_SPI_OPERATIONAL_TIMEOUT);
	interval = MIN(5, KSZ8463_SPI_OPERATIONAL_TIMEOUT_MS >> 1u);

	do {
		ret = ksz8463_spi_read16(&cfg->spi, KSZ8463_REG_CIDER, &cider);
		if (ret != 0) {
			return ret;
		}

		if (cider >> 8 == KSZ8463_FAMILY_ID) {
			break;
		}

		if (sys_timepoint_expired(expiry)) {
			LOG_ERR("Timed out waiting for SPI");
			return -ETIMEDOUT;
		}

		k_sleep(K_MSEC(interval));
	} while (1);

	LOG_DBG("SPI response received");

	/* Chip ID in bits 7-4 */
	data->chip_id = (uint8_t)cider >> 4;
	LOG_DBG("Chip ID: 0x%x", (unsigned int)data->chip_id);
	return 0;
}

static int ksz8463_soft_reset(const struct device *dev)
{
	int ret;
	const struct ksz8463_config *cfg = dev->config;

	ret = ksz8463_spi_lock(&cfg->spi);
	if (ret != 0) {
		return ret;
	}

	ret = ksz8463_spi_write8_raw(&cfg->spi, KSZ8463_REG_GRR_LO, KSZ8463_GRR_GBL_SOFT_RST);
	if (ret == 0) {
		k_sleep(K_MSEC(10));
		ret = ksz8463_spi_write8_raw(&cfg->spi, KSZ8463_REG_GRR_LO, 0);
	}

	ksz8463_spi_unlock(&cfg->spi);

	if (ret != 0) {
		return ret;
	}

	LOG_DBG("Soft reset complete");
	return 0;
}

static int ksz8463_init(const struct device *dev)
{
	int ret;
	struct ksz8463_data *data = dev->data;

	ret = k_mutex_init(&data->spi_mutex);
	if (ret != 0) {
		return ret;
	}

	if (ksz8463_have_hard_reset(dev)) {
		ret = ksz8463_hard_reset(dev);

		if (ret != 0) {
			return ret;
		}
	}

	ret = ksz8463_read_chip_id(dev);
	if (ret != 0) {
		return ret;
	}

	if (!ksz8463_have_hard_reset(dev)) {
		ret = ksz8463_soft_reset(dev);
	}

	return ret;
}

#define KSZ8463_PHY_INIT(phy_id, port_id)                                                          \
	static struct ksz8463_phy_data ksz8463_phy_data_##port_id = {                              \
		.autoneg_en = true,                                                                \
		.use_fixed_link = DT_PROP(phy_id, microchip_fixed_link),                           \
		.fixed_link_speed = DT_ENUM_IDX(phy_id, microchip_fixed_link_speed),               \
		.phydev = DEVICE_DT_GET(phy_id),                                                   \
	};                                                                                         \
                                                                                                   \
	static const struct ksz8463_phy_config ksz8463_phy_config_##port_id = {                    \
		.portdev = DEVICE_DT_GET(port_id),                                                 \
	};                                                                                         \
                                                                                                   \
	/* PHY device */                                                                           \
	DEVICE_DT_DEFINE(phy_id, NULL, NULL, &ksz8463_phy_data_##port_id,                          \
			 &ksz8463_phy_config_##port_id, POST_KERNEL, CONFIG_PHY_INIT_PRIORITY,     \
			 &ksz8463_phy_driver_api)

#define KSZ8463_PORT_INIT_PHY(port_id)                                                             \
	COND_CODE_0(DT_NODE_HAS_PROP(port_id, ethernet),                                           \
		(KSZ8463_PHY_INIT(DT_CHILD(port_id, phy), port_id)),                               \
		(EMPTY)                                                                            \
	)

#define KSZ8463_PORT_VERIFY_CHILD_NODE_COUNT(port_id)                                              \
	COND_CODE_1(DT_NODE_HAS_PROP(port_id, ethernet),                                           \
		(BUILD_ASSERT(!DT_CHILD_NUM_STATUS_OKAY(port_id),                                  \
				"CPU port should have no children")),                              \
		(BUILD_ASSERT(DT_CHILD_NUM_STATUS_OKAY(port_id) == 1,                              \
				"Non-CPU ports should have 1 child"))                              \
	)

#define KSZ8463_PORT_GET_PHY_OR_NULL(port_id)                                                      \
	COND_CODE_0(DT_NODE_HAS_PROP(port_id, ethernet),                                           \
		(DEVICE_DT_GET(DT_CHILD(port_id, phy))),                                           \
		(NULL)                                                                             \
	)

#define KSZ8463_PORT_INIT(port_id, inst)                                                           \
	KSZ8463_PORT_VERIFY_CHILD_NODE_COUNT(port_id);                                             \
                                                                                                   \
	KSZ8463_PORT_INIT_PHY(port_id);                                                            \
                                                                                                   \
	static struct ksz8463_port_config ksz8463_##inst##port_id = {                              \
		.disable_eee = DT_PROP(port_id, microchip_disable_eee),                            \
		.eee_enabled = true,                                                               \
		.dev = DEVICE_DT_INST_GET(inst),                                                   \
	};                                                                                         \
                                                                                                   \
	static const struct dsa_port_config ksz8463_port_config##inst##port_id = {                 \
		.mcfg = NET_ETH_MAC_DT_CONFIG_INIT(port_id),                                       \
		.port_idx = DT_REG_ADDR(port_id),                                                  \
		.phy_dev = KSZ8463_PORT_GET_PHY_OR_NULL(port_id),                                  \
		.phy_mode = DT_PROP_OR(port_id, phy_connection_type, "internal"),                  \
		.ethernet_connection = DEVICE_DT_GET_OR_NULL(DT_PHANDLE(port_id, ethernet)),       \
		.prv_config = &ksz8463_##inst##port_id,                                            \
	};                                                                                         \
                                                                                                   \
	/* Port device */                                                                          \
	DSA_PORT_INST_INIT(port_id, inst, &ksz8463_port_config##inst##port_id);

/* clang-format and checkpatch disagree on spacing */
/* clang-format off */
#define KSZ8463_INST_PTDEV_ARRAY(inst)								   \
	(const struct device * [DT_INST_CHILD_NUM(inst)]) { 0 }

/* clang-format on */

#define KSZ8463_GPIO_DT_SPEC_OR_NULL(inst, prop)                                                   \
	COND_CODE_1(DT_INST_PROP_HAS_IDX(inst, prop, 0),					   \
		(&(struct gpio_dt_spec) GPIO_DT_SPEC_GET_BY_IDX(DT_DRV_INST(inst), prop, 0)),	   \
		(NULL))

#define KSZ8463_PORT_GET(port_id) [DT_REG_ADDR(port_id)] = DEVICE_DT_GET(port_id),

/* clang-format off */
#define KSZ8463_INST_PORT_ARRAY_INITIALIZER(inst)                                                  \
	(const struct device * [DT_INST_CHILD_NUM(inst)]) {                                        \
			DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, KSZ8463_PORT_GET)                  \
	}

/* clang-format on */

#define KSZ8463_INIT(inst)                                                                         \
	BUILD_ASSERT(DT_INST_CHILD_NUM(inst) == KSZ8463_NUM_PORTS, "Switch has 3 ports");          \
                                                                                                   \
	/* Only the CPU port should set the ethernet phandle */                                    \
	BUILD_ASSERT(!DT_NODE_HAS_PROP(DT_INST_CHILD_BY_UNIT_ADDR_INT(inst, 0), ethernet),         \
		     "Port 1 must not have an ethernet phandle");                                  \
	BUILD_ASSERT(!DT_NODE_HAS_PROP(DT_INST_CHILD_BY_UNIT_ADDR_INT(inst, 1), ethernet),         \
		     "Port 2 must not have an ethernet phandle");                                  \
	BUILD_ASSERT(DT_NODE_HAS_PROP(DT_INST_CHILD_BY_UNIT_ADDR_INT(inst, 2), ethernet),          \
		     "Port 3 requires an ethernet phandle");                                       \
                                                                                                   \
	/* CPU port should set phy-connection-type */                                              \
	BUILD_ASSERT(                                                                              \
		DT_NODE_HAS_PROP(DT_INST_CHILD_BY_UNIT_ADDR_INT(inst, 2), phy_connection_type),    \
		"Port 3 requires a PHY connection type");                                          \
                                                                                                   \
	BUILD_ASSERT(DT_INST_ENUM_IDX(inst, microchip_port_led_mode) <= BIT_MASK(2),               \
		     "Invalid LED mode");                                                          \
                                                                                                   \
	static struct ksz8463_data ksz8463_data_##inst = {                                         \
		.portdevs = KSZ8463_INST_PORT_ARRAY_INITIALIZER(inst),                             \
	};                                                                                         \
                                                                                                   \
	static const struct ksz8463_config ksz8463_config_##inst = {                               \
		.mld_snoop_en = DT_INST_PROP(inst, microchip_mld_snoop_en),                        \
		.igmp_snoop_en = DT_INST_PROP(inst, microchip_igmp_snoop_en),                      \
		.pkt_sz_chk_en = DT_INST_PROP(inst, microchip_legal_packet_size_check_en),         \
		.num_portdevs = DT_INST_CHILD_NUM(inst),                                           \
		.led_mode = DT_INST_ENUM_IDX(inst, microchip_port_led_mode),                       \
		.rst_gpio = KSZ8463_GPIO_DT_SPEC_OR_NULL(inst, reset_gpios),                       \
		.irq_gpio = KSZ8463_GPIO_DT_SPEC_OR_NULL(inst, int_gpios),                         \
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8)),                                \
		.dev = DEVICE_DT_INST_GET(inst),                                                   \
	};                                                                                         \
                                                                                                   \
	/* Switch device */                                                                        \
	DEVICE_DT_INST_DEFINE(inst, ksz8463_init, NULL, &ksz8463_data_##inst,                      \
			      &ksz8463_config_##inst, POST_KERNEL, CONFIG_ETH_INIT_PRIORITY,       \
			      NULL);                                                               \
                                                                                                   \
	struct ksz8463_prv_data ksz8463_prv_data_##inst = {                                        \
		.dev = DEVICE_DT_INST_GET(inst),                                                   \
	};                                                                                         \
                                                                                                   \
	BUILD_ASSERT(DT_INST_CHILD_NUM_STATUS_OKAY(inst), "No ports enabled");                     \
                                                                                                   \
	DSA_SWITCH_INST_INIT(inst, &ksz8463_dsa_api, &ksz8463_prv_data_##inst, KSZ8463_PORT_INIT)

DT_INST_FOREACH_STATUS_OKAY(KSZ8463_INIT)
