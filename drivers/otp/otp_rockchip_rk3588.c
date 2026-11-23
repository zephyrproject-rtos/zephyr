/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT rockchip_rk3588_otp

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(otp_rockchip_rk3588, CONFIG_OTP_LOG_LEVEL);

/* RK3588 OTP controller register offsets */
#define RK3588_OTPC_AUTO_CTRL 0x04U
#define RK3588_OTPC_AUTO_EN   0x08U
#define RK3588_OTPC_DOUT0     0x20U
#define RK3588_OTPC_INT_ST    0x84U

#define RK3588_OTPC_AUTO_CTRL_ADDR_MASK  GENMASK(31, 16)
#define RK3588_OTPC_AUTO_CTRL_BURST_MASK GENMASK(15, 8)
#define RK3588_OTPC_BURST_WORDS          1U
#define RK3588_OTPC_AUTO_EN_BIT          BIT(0)
#define RK3588_OTPC_INT_ST_RD_DONE       BIT(1)

/* Non-secure OTP region starts at word offset 0x300 (byte offset 0xc00) and spans 1 KiB. */
#define RK3588_OTPC_NS_OFFSET_WORDS 0x300U
#define RK3588_OTPC_NS_SIZE         0x400U
#define RK3588_OTPC_WORD_SIZE       sizeof(uint32_t)
#define RK3588_OTPC_TIMEOUT_US      10000U

struct otp_rk3588_config {
	DEVICE_MMIO_ROM;
	uint32_t ns_offset;
	size_t size;
};

struct otp_rk3588_data {
	DEVICE_MMIO_RAM;
	struct k_mutex lock;
};

static int otp_rk3588_read_word(mm_reg_t base, uint32_t word, uint32_t *out)
{
	uint32_t ctrl = FIELD_PREP(RK3588_OTPC_AUTO_CTRL_ADDR_MASK, word) |
			FIELD_PREP(RK3588_OTPC_AUTO_CTRL_BURST_MASK, RK3588_OTPC_BURST_WORDS);

	sys_write32(ctrl, base + RK3588_OTPC_AUTO_CTRL);
	sys_write32(RK3588_OTPC_AUTO_EN_BIT, base + RK3588_OTPC_AUTO_EN);

	if (!WAIT_FOR((sys_read32(base + RK3588_OTPC_INT_ST) & RK3588_OTPC_INT_ST_RD_DONE) != 0U,
		      RK3588_OTPC_TIMEOUT_US, k_busy_wait(1))) {
		return -ETIMEDOUT;
	}

	/* Clear read-done interrupt status */
	sys_write32(RK3588_OTPC_INT_ST_RD_DONE, base + RK3588_OTPC_INT_ST);
	*out = sys_read32(base + RK3588_OTPC_DOUT0);

	return 0;
}

static int otp_rk3588_read(const struct device *dev, off_t offset, void *buf, size_t size)
{
	const struct otp_rk3588_config *config = dev->config;
	struct otp_rk3588_data *data = dev->data;
	mm_reg_t base = DEVICE_MMIO_GET(dev);
	uint8_t *out = buf;
	size_t done = 0U;
	int ret;

	if (buf == NULL || offset < 0 || (size_t)offset > config->size ||
	    size > config->size - (size_t)offset) {
		return -EINVAL;
	}

	if (size == 0U) {
		return 0;
	}

	/* One read at a time: the controller has a single command and data window. */
	ret = k_mutex_lock(&data->lock, K_FOREVER);
	if (ret != 0) {
		return ret;
	}

	while (done < size) {
		size_t pos = (size_t)offset + done;
		size_t skip = pos % RK3588_OTPC_WORD_SIZE;
		size_t len = MIN(RK3588_OTPC_WORD_SIZE - skip, size - done);
		uint32_t word = (uint32_t)(pos / RK3588_OTPC_WORD_SIZE) + config->ns_offset;
		uint8_t word_buf[RK3588_OTPC_WORD_SIZE];
		uint32_t value;

		ret = otp_rk3588_read_word(base, word, &value);
		if (ret < 0) {
			LOG_ERR("Timeout reading OTP word %u", word);
			break;
		}

		sys_put_le32(value, word_buf);
		memcpy(out + done, word_buf + skip, len);
		done += len;
	}

	(void)k_mutex_unlock(&data->lock);

	return ret;
}

static int otp_rk3588_init(const struct device *dev)
{
	struct otp_rk3588_data *data = dev->data;
	int ret;

	ret = k_mutex_init(&data->lock);
	if (ret != 0) {
		return ret;
	}

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	return 0;
}

static DEVICE_API(otp, otp_rk3588_api) = {
	.read = otp_rk3588_read,
};

#define OTP_RK3588_INIT(n)                                                                         \
	static const struct otp_rk3588_config otp_rk3588_config_##n = {                            \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.ns_offset = RK3588_OTPC_NS_OFFSET_WORDS,                                          \
		.size = RK3588_OTPC_NS_SIZE,                                                       \
	};                                                                                         \
                                                                                                   \
	static struct otp_rk3588_data otp_rk3588_data_##n;                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, otp_rk3588_init, NULL, &otp_rk3588_data_##n,                      \
			      &otp_rk3588_config_##n, POST_KERNEL, CONFIG_OTP_INIT_PRIORITY,       \
			      &otp_rk3588_api);

DT_INST_FOREACH_STATUS_OKAY(OTP_RK3588_INIT)
