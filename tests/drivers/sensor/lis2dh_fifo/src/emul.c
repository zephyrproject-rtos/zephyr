/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lis2dh

#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/spi_emul.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#include "lis2dh.h"
#include "emul.h"

struct lis2dh_test_config {
	struct gpio_dt_spec irq;
};

void lis2dh_test_reset(const struct emul *emul)
{
	struct lis2dh_test_bus *bus = emul->data;

	bus->fail_mask = 0U;
	bus->fail_all = false;
	bus->operations = 0U;
	bus->reads = 0U;
	bus->diagnostic_reads = 0U;
	bus->writes = 0U;
	bus->bursts = 0U;
	bus->block_burst = false;
	bus->regs[LIS2DH_REG_FIFO_SRC] = LIS2DH_FIFO_EMPTY;
	k_sem_init(&bus->burst_entered, 0, 1);
	k_sem_init(&bus->burst_release, 0, 1);
}

void lis2dh_test_fill(const struct emul *emul, unsigned int count)
{
	struct lis2dh_test_bus *bus = emul->data;

	bus->regs[LIS2DH_REG_FIFO_SRC] =
		count == 0U ? LIS2DH_FIFO_EMPTY : (count == 32U ? LIS2DH_FIFO_OVRN | 31U : count);
	if (count >= 16U) {
		bus->regs[LIS2DH_REG_FIFO_SRC] |= LIS2DH_FIFO_WTM;
	}
}

static int transfer(const struct emul *emul, uint8_t reg, uint8_t *bytes, size_t len, bool read)
{
	struct lis2dh_test_bus *bus = emul->data;
	const struct lis2dh_test_config *cfg = emul->cfg;
	unsigned int operation = bus->operations++;

	if (read && ((reg == LIS2DH_REG_CTRL1 && len == 5U) ||
		     (reg == LIS2DH_REG_FIFO_CTRL && len == 2U))) {
		bus->diagnostic_reads++;
	}
	if (bus->fail_all || (operation < 64U && (bus->fail_mask & BIT64(operation)) != 0U)) {
		return -EIO;
	}
	if (read) {
		bus->reads++;
		if (reg == LIS2DH_REG_ACCEL_X_LSB && len >= 6U) {
			bus->bursts++;
			bus->last_len = len;
			if (bus->block_burst) {
				k_sem_give(&bus->burst_entered);
				zassert_ok(k_sem_take(&bus->burst_release, K_SECONDS(1)));
			}
			for (size_t i = 0; i < len / 2U; i++) {
				sys_put_le16((i + 1U) << 4, bytes + i * 2U);
			}
			lis2dh_test_fill(emul, 0U);
			(void)gpio_emul_input_set(cfg->irq.port, cfg->irq.pin, 0);
		} else {
			zassert_true(reg + len <= sizeof(bus->regs));
			memcpy(bytes, bus->regs + reg, len);
		}
	} else {
		bus->writes++;
		zassert_true(reg + len <= sizeof(bus->regs));
		memcpy(bus->regs + reg, bytes, len);
		if (reg == LIS2DH_REG_FIFO_CTRL && (*bytes & 0xc0U) == 0U) {
			lis2dh_test_fill(emul, 0U);
			(void)gpio_emul_input_set(cfg->irq.port, cfg->irq.pin, 0);
		}
	}
	return 0;
}

static int transfer_i2c(const struct emul *emul, struct i2c_msg *msgs, int num_msgs, int addr)
{
	struct lis2dh_test_bus *bus = emul->data;
	uint8_t cmd = msgs[0].buf[0];

	ARG_UNUSED(addr);
	if (num_msgs == 1) {
		zassert_equal(msgs[0].len, 2);
		return transfer(emul, cmd & 0x7fU, msgs[0].buf + 1, 1, false);
	}
	zassert_equal(num_msgs, 2);
	zassert_equal(msgs[0].len, 1);
	bus->last_cmd = cmd;
	if (msgs[1].len > 1U) {
		zassert_true((cmd & BIT(7)) != 0U);
	}
	return transfer(emul, cmd & 0x7fU, msgs[1].buf, msgs[1].len,
			(msgs[1].flags & I2C_MSG_READ) != 0U);
}

static int transfer_spi(const struct emul *emul, const struct spi_config *cfg,
			const struct spi_buf_set *tx, const struct spi_buf_set *rx)
{
	struct lis2dh_test_bus *bus = emul->data;
	uint8_t cmd = *(uint8_t *)tx->buffers[0].buf;
	bool read = (cmd & BIT(7)) != 0U;
	size_t len;

	ARG_UNUSED(cfg);
	bus->last_cmd = cmd;
	if (read) {
		zassert_not_null(rx);
		zassert_equal(rx->count, 2);
		zassert_is_null(rx->buffers[0].buf);
		zassert_equal(rx->buffers[0].len, 1);
		len = rx->buffers[1].len;
	} else {
		zassert_equal(tx->count, 2);
		zassert_equal(tx->buffers[0].len, 1);
		len = tx->buffers[1].len;
	}
	if (len > 1U) {
		zassert_true((cmd & BIT(6)) != 0U);
	}
	return transfer(emul, cmd & 0x3fU, read ? rx->buffers[1].buf : tx->buffers[1].buf, len,
			read);
}

static int init(const struct emul *emul, const struct device *parent)
{
	struct lis2dh_test_bus *bus = emul->data;

	ARG_UNUSED(parent);
	bus->regs[LIS2DH_REG_WAI] = LIS2DH_CHIP_ID;
	lis2dh_test_reset(emul);
	return 0;
}

static const struct i2c_emul_api i2c_api = {.transfer = transfer_i2c};
static const struct spi_emul_api spi_api = {.io = transfer_spi};

#define EMUL_DEFINE(inst)                                                                          \
	static struct lis2dh_test_bus bus_##inst;                                                  \
	static const struct lis2dh_test_config cfg_##inst = {                                      \
		.irq = GPIO_DT_SPEC_INST_GET(inst, irq_gpios),                                     \
	};                                                                                         \
	EMUL_DT_INST_DEFINE(inst, init, &bus_##inst, &cfg_##inst,                                  \
			    COND_CODE_1(DT_INST_ON_BUS(inst, spi), (&spi_api), (&i2c_api)), NULL);

DT_INST_FOREACH_STATUS_OKAY(EMUL_DEFINE)
