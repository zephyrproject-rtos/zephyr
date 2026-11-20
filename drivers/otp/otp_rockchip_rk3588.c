/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Rockchip RK3588 OTP driver.
 * Reference: U-Boot drivers/misc/rockchip-otp.c (rockchip_rk3588_otp_read)
 * Reference: Linux drivers/nvmem/rockchip-otp.c (rk3588_otp_read)
 */

#define DT_DRV_COMPAT rockchip_rk3588_otp

#include <zephyr/drivers/otp.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(otp_rockchip_rk3588, CONFIG_OTP_LOG_LEVEL);

/* RK3588 OTP controller register offsets */
#define RK3588_OTPC_AUTO_CTRL 0x04
#define RK3588_OTPC_AUTO_EN   0x08
#define RK3588_OTPC_DOUT0     0x20
#define RK3588_OTPC_INT_ST    0x84

#define RK3588_NBYTES      4
#define RK3588_BURST_NUM   1
#define RK3588_BURST_SHIFT 8
#define RK3588_ADDR_SHIFT  16
#define RK3588_AUTO_EN     BIT(0)
#define RK3588_RD_DONE     BIT(1)

/*
 * Non-secure region offset in word units.
 * Linux uses read_offset = 0xc00 (bytes), dividing by 4 gives 0x300 (words).
 * U-Boot uses RK3588_NO_SECURE_OFFSET = 0x300 directly (word offset).
 */
#define RK3588_NO_SECURE_OFFSET 0x300

/* Maximum OTP read size */
#define RK3588_OTP_SIZE 0x400

#define OTPC_TIMEOUT_US 10000

struct otp_rk3588_config {
	DEVICE_MMIO_ROM;
	uint32_t ns_offset;
	uint32_t size;
};

struct otp_rk3588_data {
	DEVICE_MMIO_RAM;
	struct k_mutex lock;
};

static int otp_rk3588_wait_status(mm_reg_t base, uint32_t reg, uint32_t flag)
{
	int timeout = OTPC_TIMEOUT_US;

	while (!(sys_read32(base + reg) & flag)) {
		k_busy_wait(1);
		if (--timeout <= 0) {
			return -ETIMEDOUT;
		}
	}
	/* Clear interrupt status */
	sys_write32(flag, base + reg);
	return 0;
}

static int otp_rk3588_read(const struct device *dev, off_t offset, void *buf, size_t size)
{
	const struct otp_rk3588_config *config = dev->config;
	struct otp_rk3588_data *data = dev->data;
	mm_reg_t base = DEVICE_MMIO_GET(dev);
	uint8_t *out = buf;
	size_t done = 0U;
	int ret = 0;

	if (offset < 0 || buf == NULL || offset >= config->size ||
	    size > config->size - (size_t)offset) {
		return -EINVAL;
	}

	/* One read at a time: the controller has a single command and data window. */
	k_mutex_lock(&data->lock, K_FOREVER);
	while (done < size) {
		size_t pos = (size_t)offset + done;
		size_t skip = pos % RK3588_NBYTES;
		size_t len = MIN(RK3588_NBYTES - skip, size - done);
		uint32_t word = pos / RK3588_NBYTES + config->ns_offset;
		uint32_t value;

		sys_write32((word << RK3588_ADDR_SHIFT) | (RK3588_BURST_NUM << RK3588_BURST_SHIFT),
			    base + RK3588_OTPC_AUTO_CTRL);
		sys_write32(RK3588_AUTO_EN, base + RK3588_OTPC_AUTO_EN);

		ret = otp_rk3588_wait_status(base, RK3588_OTPC_INT_ST, RK3588_RD_DONE);
		if (ret < 0) {
			LOG_ERR("timeout reading OTP word %u", word);
			break;
		}

		value = sys_read32(base + RK3588_OTPC_DOUT0);
		memcpy(out + done, (const uint8_t *)&value + skip, len);
		done += len;
	}
	k_mutex_unlock(&data->lock);

	return ret;
}

static int otp_rk3588_init(const struct device *dev)
{
	struct otp_rk3588_data *data = dev->data;

	k_mutex_init(&data->lock);
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	return 0;
}

static const struct otp_rk3588_config otp_rk3588_config = {
	DEVICE_MMIO_ROM_INIT(DT_DRV_INST(0)),
	.size = RK3588_OTP_SIZE,
	.ns_offset = RK3588_NO_SECURE_OFFSET,
};

static struct otp_rk3588_data otp_rk3588_data_0;

static DEVICE_API(otp, otp_rk3588_api) = {
	.read = otp_rk3588_read,
};

DEVICE_DT_INST_DEFINE(0, otp_rk3588_init, NULL, &otp_rk3588_data_0, &otp_rk3588_config, POST_KERNEL,
		      CONFIG_OTP_INIT_PRIORITY, &otp_rk3588_api);
