/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 * Copyright (c) 2026 Igalia S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "cyw43_configport.h"
#include "cyw43.h"
#include "cyw43_internal.h"
#include "cyw43_spi.h"
#include "rpi_pico_cyw43_spi.h"

#define DT_DRV_COMPAT raspberrypi_cyw43_wifi

LOG_MODULE_DECLARE(rpi_pico_cyw43_drv, CONFIG_WIFI_LOG_LEVEL);

/*
 * #define SPI_DEBUG
 * #define SPI_DEBUG_DUMP
 */

#ifndef SPI_DEBUG
#undef LOG_DBG
#define LOG_DBG(...) (void)0
#endif

#ifndef SPI_DEBUG_DUMP
#undef LOG_HEXDUMP_DBG
#define LOG_HEXDUMP_DBG(...) (void)0
#endif

#define PINCTRL_STATE_HOST_WAKE PINCTRL_STATE_PRIV_START
PINCTRL_DT_INST_DEFINE(0);

#define POWER_OFF_CBUCK_WAIT_MS 15
#define POWER_ON_POR_WAIT_MS 200


#define CYW43_WIFI_SPI_OPERATION (SPI_WORD_SET(DT_PROP_OR(DT_DRV_INST(0), spi_word_size, 8)) \
					| SPI_HALF_DUPLEX \
					| SPI_TRANSFER_MSB)

struct cyw43_wifi_dev_config cyw43_wifi_dev_cfg = {
	.bus_spi = SPI_DT_SPEC_GET(DT_DRV_INST(0), CYW43_WIFI_SPI_OPERATION),
	.wl_on_gpio = GPIO_DT_SPEC_GET_OR(
		DT_DRV_INST(0), wl_on_gpios, {0}),
	.bus_select_gpio = GPIO_DT_SPEC_GET_OR(
		DT_DRV_INST(0), bus_select_gpios, {0}),
	.host_wake_gpio = GPIO_DT_SPEC_GET_OR(
		DT_DRV_INST(0), host_wake_gpios, {0}),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};

struct gpio_dt_spec *rp_gpio[RPI_CYW43_GPIOS] = {
	[CYW43_PIN_WL_REG_ON] = &cyw43_wifi_dev_cfg.wl_on_gpio,
	[CYW43_PIN_WL_HOST_WAKE] = &cyw43_wifi_dev_cfg.host_wake_gpio,
};


static inline const char *func_name(int fn)
{
	switch (fn) {
	case BUS_FUNCTION:
		return "BUS_FUNCTION";
	case BACKPLANE_FUNCTION:
		return "BACKPLANE_FUNCTION";
	case WLAN_FUNCTION:
		return "WLAN_FUNCTION";
	default:
		return "UNKNOWN";
	}
}

#undef SWAP32
static inline uint32_t __swap16x2(uint32_t a)
{
	__asm("rev16 %0, %0" : "+l" (a) : : );
	return a;
}
#define SWAP32(a) __swap16x2(a)

static inline uint32_t make_cmd(bool write, bool inc, uint32_t fn,
				uint32_t addr, uint32_t sz)
{
	return write << 31 | inc << 30 | fn << 28 | (addr & 0x1ffff) << 11 | sz;
}

int cyw43_spi_transfer(cyw43_int_t *self, const uint8_t *tx,
			size_t tx_length, uint8_t *rx, size_t rx_length)
{
	struct spi_buf rx_buf[2];
	struct spi_buf_set rx_set;
	struct cyw43_wifi_dev_config *cfg = self->bus_data;
	int ret;

	LOG_DBG("tx: %p, tx_len: %d, rx: %p, rx_len: %d",
		(void *)tx, tx_length, (void *)rx, rx_length);
	if ((tx == NULL) && (rx == NULL)) {
		return -EINVAL;
	}

	cyw43_irq_enable(cfg, false);

	if (tx == NULL && tx_length > 0 && rx_length >= tx_length) {
		tx = rx;
	}
	const struct spi_buf tx_buf = {
		.buf = (uint8_t *)tx,
		.len = tx_length
	};
	const struct spi_buf_set tx_set = {
		.buffers = &tx_buf,
		.count = 1
	};
	if (rx != NULL) {
		rx += tx_length;
	}
	if (rx_length >= tx_length) {
		rx_length -= tx_length;
	} else {
		rx_length = 0;
	}
	rx_buf[0].buf = rx;
	rx_buf[0].len = rx_length;
	rx_set.buffers = rx_buf;
	rx_set.count = 1;

	LOG_HEXDUMP_DBG(tx, tx_length, "SPI TX:");
	ret = spi_transceive_dt(&cfg->bus_spi, &tx_set, &rx_set);
	if (ret) {
		LOG_ERR("spi_transceive_dt: %d", ret);
	}
	LOG_HEXDUMP_DBG(rx, rx_length, "SPI RX:");

	cyw43_irq_enable(cfg, true);

	return ret;
}

static int cyw43_write_reg(cyw43_int_t *self, uint32_t fn,
			uint32_t reg, uint32_t val, unsigned int size)
{
	LOG_DBG("fn: %d (%s), reg: %#x, val: %#x, size: %d",
		fn, func_name(fn), reg, val, size);
	uint32_t buf[2] = {
		[0] = make_cmd(true, true, fn, reg, size),
		[1] = val,
	};
	int ret = cyw43_spi_transfer(self, (uint8_t *)buf, 8, NULL, 0);

	return ret;
}

static uint32_t cyw43_read_reg(cyw43_int_t *self, uint32_t fn,
				uint32_t reg, unsigned int size)
{
	BUILD_ASSERT(CYW43_BACKPLANE_READ_PAD_LEN_BYTES % 4 == 0, "");
	int index = (CYW43_BACKPLANE_READ_PAD_LEN_BYTES / 4) + 1 + 1;
	uint32_t buf32[index];
	uint8_t *buf = (uint8_t *)buf32;
	uint32_t result;
	/* Response delay */
	const uint32_t padding = (fn == BACKPLANE_FUNCTION) ?
		CYW43_BACKPLANE_READ_PAD_LEN_BYTES : 0;
	int ret;

	LOG_DBG("fn: %d (%s), reg: %#x, size: %d",
		fn, func_name(fn), reg, size);
	buf32[0] = make_cmd(false, true, fn, reg, size);
	ret = cyw43_spi_transfer(self, NULL, 4, buf, 8 + padding);
	if (ret) {
		return ret;
	}
	result = buf32[padding > 0 ? index - 1 : 1];
	LOG_DBG("result: %#x", result);
	return result;
}


/***************************
 * Core cyw43 driver SPI API
 ***************************/

int cyw43_spi_init(cyw43_int_t *self)
{
	struct cyw43_wifi_dev_config *cfg = &cyw43_wifi_dev_cfg;

	if (!device_is_ready(cfg->wl_on_gpio.port)) {
		LOG_ERR("wl-on-gpio not ready");
		return -1;
	}
	if (!spi_is_ready_dt(&cfg->bus_spi)) {
		LOG_ERR("SPI device is not ready");
		return -1;
	}
	self->bus_data = cfg;

	pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_HOST_WAKE);
	return 0;
}

void cyw43_spi_gpio_setup(void)
{
	LOG_DBG("");
}

void cyw43_spi_reset(void)
{
	struct cyw43_wifi_dev_config *cfg = &cyw43_wifi_dev_cfg;

	gpio_pin_configure_dt(&cfg->wl_on_gpio, GPIO_OUTPUT_INACTIVE);
	k_msleep(POWER_OFF_CBUCK_WAIT_MS);
	/*
	 * CYW43439 pin strap config: select gSPI interface
	 * Pull bus-select (SDIO_DATA2) low before power on
	 */
	gpio_pin_configure_dt(&cfg->bus_select_gpio, GPIO_OUTPUT_INACTIVE);
	/* Power on */
	LOG_DBG("Power on");
	gpio_pin_set_dt(&cfg->wl_on_gpio, 1);
	k_msleep(POWER_ON_POR_WAIT_MS);
}

int cyw43_write_reg_u32(cyw43_int_t *self, uint32_t fn, uint32_t reg,
			uint32_t val)
{
	LOG_DBG("fn: %d, reg: %#x, val: %#x", fn, reg, val);
	return cyw43_write_reg(self, fn, reg, val, 4);
}

int cyw43_write_reg_u16(cyw43_int_t *self, uint32_t fn, uint32_t reg,
			uint16_t val)
{
	LOG_DBG("fn: %d, reg: %#x, val: %#x", fn, reg, val);
	return cyw43_write_reg(self, fn, reg, val, 2);
}

int cyw43_write_reg_u8(cyw43_int_t *self, uint32_t fn, uint32_t reg,
			uint32_t val)
{
	LOG_DBG("fn: %d, reg: %#x, val: %#x", fn, reg, val);
	return cyw43_write_reg(self, fn, reg, val, 1);
}

uint32_t cyw43_read_reg_u32(cyw43_int_t *self, uint32_t fn, uint32_t reg)
{
	LOG_DBG("fn: %d, reg: %#x", fn, reg);
	return cyw43_read_reg(self, fn, reg, 4);
}

int cyw43_read_reg_u16(cyw43_int_t *self, uint32_t fn, uint32_t reg)
{
	LOG_DBG("fn: %d, reg: %#x", fn, reg);
	return cyw43_read_reg(self, fn, reg, 2);
}

int cyw43_read_reg_u8(cyw43_int_t *self, uint32_t fn, uint32_t reg)
{
	LOG_DBG("fn: %d, reg: %#x", fn, reg);
	return cyw43_read_reg(self, fn, reg, 1);
}

int cyw43_read_bytes(cyw43_int_t *self, uint32_t fn, uint32_t addr,
			size_t len, uint8_t *buf)
{
	__ASSERT_NO_MSG(fn != BACKPLANE_FUNCTION ||
			len <= CYW43_BUS_MAX_BLOCK_SIZE);
	/* Response delay */
	const uint32_t padding = (fn == BACKPLANE_FUNCTION) ?
		CYW43_BACKPLANE_READ_PAD_LEN_BYTES :
		0;
	size_t aligned_len = (len + 3) & ~3;
	int hdr_offset = (padding > 0) ?
		0 :
		(CYW43_BACKPLANE_READ_PAD_LEN_BYTES / 4);
	int ret;

	__ASSERT_NO_MSG(aligned_len > 0 && aligned_len <= 0x7f8);
	__ASSERT_NO_MSG(buf == self->spid_buf ||
			buf < self->spid_buf ||
			buf >= (self->spid_buf + sizeof(self->spid_buf)));

	LOG_DBG("fn: %d (%s), addr: %#x, len: %d",
		fn, func_name(fn), addr, len);
	self->spi_header[hdr_offset] = make_cmd(false, true, fn, addr, len);
	ret = cyw43_spi_transfer(self, NULL, 4,
		(uint8_t *)&self->spi_header[hdr_offset],
		aligned_len + 4 + padding);
	if (ret) {
		LOG_ERR("cyw43_spi_transfer() error: %d", ret);
		return ret;
	}
	/* Avoid a copy in the usual case just to add the header */
	if (buf != self->spid_buf) {
		memcpy(buf, self->spid_buf, len);
	}

	return 0;
}

int cyw43_write_bytes(cyw43_int_t *self, uint32_t fn, uint32_t addr,
			size_t len, const uint8_t *src)
{
	__ASSERT_NO_MSG(fn != BACKPLANE_FUNCTION ||
			len <= CYW43_BUS_MAX_BLOCK_SIZE);
	const size_t aligned_len = (len + 3) & ~3u;
	int hdr_offset = CYW43_BACKPLANE_READ_PAD_LEN_BYTES / 4;

	__ASSERT_NO_MSG(aligned_len > 0 && aligned_len <= 0x7f8);
	LOG_DBG("fn: %d (%s), addr: %#x, len: %d",
		fn, func_name(fn), addr, len);
	if (fn == WLAN_FUNCTION) {
		/* Wait for FIFO to be ready to accept data */
		int f2_ready_attempts = 1000;

		while (f2_ready_attempts-- > 0) {
			uint32_t bus_status = cyw43_read_reg_u32(self,
					BUS_FUNCTION, SPI_STATUS_REGISTER);

			if (bus_status & STATUS_F2_RX_READY) {
				break;
			}
		}
		if (f2_ready_attempts <= 0) {
			LOG_ERR("F2 not ready");
			return -EIO;
		}
	}
	self->spi_header[hdr_offset] = make_cmd(true, true, fn, addr, len);
	/* Avoid a copy in the usual case just to add the header */
	if (src != self->spid_buf) {
		__ASSERT_NO_MSG(src < self->spid_buf ||
			src >= (self->spid_buf + sizeof(self->spid_buf)));
		memcpy(self->spid_buf, src, len);
	}
	return cyw43_spi_transfer(self,
				(uint8_t *)&self->spi_header[hdr_offset],
				aligned_len + 4, NULL, 0);
}

/*
 * Only used to for gSPI setup at boot. The CYW43439 starts in 16-bit
 * little-endian mode, so the command needs to be swapped. Once
 * configured, the rest of the transfers can be done in 32-bit mode.
 */
int write_reg_u32_swap(cyw43_int_t *self, uint32_t fn, uint32_t reg,
			uint32_t val)
{
	uint32_t buf[2] = {
		[0] = SWAP32(make_cmd(true, true, fn, reg, 4)),
		[1] = SWAP32(val),
	};

	LOG_DBG("fn: %d (%s), reg: %#x, cmd (swapped): %#x, val: %#x "
		"(swapped): %#x",
		fn, func_name(fn), reg, buf[0], val, buf[1]);
	return cyw43_spi_transfer(self, (uint8_t *)buf, 8, NULL, 0);
}

uint32_t read_reg_u32_swap(cyw43_int_t *self, uint32_t fn, uint32_t reg)
{
	uint32_t buf[2] = {0};
	int ret;

	LOG_DBG("fn: %d (%s), reg: %#x", fn, func_name(fn), reg);
	__ASSERT_NO_MSG(fn != BACKPLANE_FUNCTION);
	buf[0] = SWAP32(make_cmd(false, true, fn, reg, 4));
	LOG_DBG("cmd (swapped): %#x", buf[0]);
	ret = cyw43_spi_transfer(self, NULL, 4, (uint8_t *)buf, 8);
	if (ret) {
		LOG_ERR("cyw43_spi_transfer: %d", ret);
		return ret;
	}
	LOG_DBG("buf[0]: %#x, buf[1]: %#x", buf[0], buf[1]);

	return SWAP32(buf[1]);
}


/**********************************
 * Internal Zephyr driver functions
 **********************************/

/*
 * Applies the appropriate pinmux configuration to set the
 * host-wake-gpio as an input GPIO for event signaling from the CYW43439
 * or as a PIO pin for SPI transmission.
 */
void cyw43_irq_enable(const struct cyw43_wifi_dev_config *dev_cfg,
				bool enable)
{
	int ret;

	if (enable) {
		ret = pinctrl_apply_state(dev_cfg->pcfg,
					PINCTRL_STATE_HOST_WAKE);
		if (ret) {
			LOG_ERR("Error muxing wifi-host-wake pin as gpio: %d",
				ret);
			return;
		}
		ret = gpio_pin_interrupt_configure_dt(
			&dev_cfg->host_wake_gpio, GPIO_INT_LEVEL_HIGH);
		if (ret) {
			LOG_ERR("Error configuring wifi-host-wake gpio pin: %d",
				ret);
			return;
		}
	} else {
		ret = gpio_pin_interrupt_configure_dt(
			&dev_cfg->host_wake_gpio, GPIO_INT_DISABLE);
		if (ret) {
			LOG_ERR("Error configuring wifi-host-wake gpio pin: %d",
				ret);
			return;
		}
		ret = pinctrl_apply_state(dev_cfg->pcfg,
					PINCTRL_STATE_DEFAULT);
		if (ret) {
			LOG_ERR("Error muxing wifi-host-wake pin for PIO: %d",
				ret);
			return;
		}
	}
}
