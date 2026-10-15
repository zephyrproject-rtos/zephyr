/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file cs47l63_emul.h
 * @brief Recording SPI emulator for the CS47L63 and the reference configuration, for the
 *        driver's own tests
 */

#ifndef CS47L63_EMUL_H_
#define CS47L63_EMUL_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/gpio.h>

#define CS47L63_EMUL_MCLK_HZ   6144000U
#define CS47L63_EMUL_WORD_BITS 16U
#define CS47L63_EMUL_CHANNELS  2U
#define CS47L63_EMUL_RATE_HZ   48000U

/** IRQ1_MASK_1, _2 and _6 at reset (DS1249F2 table 6-1). */
#define CS47L63_EMUL_MASK_1_DEFAULT 0x00021F10U
#define CS47L63_EMUL_MASK_2_DEFAULT 0x00000004U
#define CS47L63_EMUL_MASK_6_DEFAULT 0x0000030FU

struct cs47l63_emul_xfer {
	uint32_t addr;
	/** Value written, or value served back on a read. */
	uint32_t val;
	bool write;
};

/** Installs a healthy part: the datasheet DEVID, OTP variant 8, boot-done latched. */
void cs47l63_emul_reset(const struct emul *target);

void cs47l63_emul_set_reg(const struct emul *target, uint32_t addr, uint32_t val);

uint32_t cs47l63_emul_get_reg(const struct emul *target, uint32_t addr);

uint32_t cs47l63_emul_xfer_count(const struct emul *target);

bool cs47l63_emul_xfer_get(const struct emul *target, uint32_t idx, struct cs47l63_emul_xfer *out);

uint32_t cs47l63_emul_write_count(const struct emul *target, uint32_t addr);

uint32_t cs47l63_emul_read_count(const struct emul *target, uint32_t addr);

/** Returns -1 for no such write; indices order transactions against each other. */
int cs47l63_emul_write_index(const struct emul *target, uint32_t addr, uint32_t nth);

int cs47l63_emul_read_index(const struct emul *target, uint32_t addr, uint32_t nth);

bool cs47l63_emul_nth_write(const struct emul *target, uint32_t addr, uint32_t nth, uint32_t *val);

/** Armed until the next reset, for one address at a time. */
void cs47l63_emul_fail_at(const struct emul *target, uint32_t addr);

/** Like cs47l63_emul_fail_at(), for writes to addr only; reads succeed. */
void cs47l63_emul_fail_write_at(const struct emul *target, uint32_t addr);

/** Makes every accepted write return @p ret, until the next reset. */
void cs47l63_emul_write_returns(const struct emul *target, int ret);

/** Runs @p fn once, from inside the next accepted write to @p addr, until the next reset. */
void cs47l63_emul_once_on_write(const struct emul *target, uint32_t addr, void (*fn)(void *arg),
				void *arg);

/**
 * From now on, after every register change, drives @p irq to the OR of the flags the masks leave
 * enabled, as the part's IRQ pin does. Dropped by cs47l63_emul_reset().
 */
void cs47l63_emul_irq_attach(const struct emul *target, const struct gpio_dt_spec *irq);

/** Holds the pin asserted whatever the flags say, until the next reset. */
void cs47l63_emul_irq_force(const struct emul *target, bool forced);

bool cs47l63_emul_get_irq_asserted(const struct emul *target);

/**
 * ORs @p val into @p inject_addr once, right after the @p nth read of @p read_addr returns
 * (counted from this call, 0 for the next one). Dropped by cs47l63_emul_reset().
 */
void cs47l63_emul_inject_on_read(const struct emul *target, uint32_t read_addr, uint32_t nth,
				 uint32_t inject_addr, uint32_t val);

/** Transfers the armed failure refused since the last reset. */
uint32_t cs47l63_emul_fail_hits(const struct emul *target);

/** The reference stream: 48 kHz stereo 16-bit I2S from a 6.144 MHz MCLK, the host driving both
 *  clocks.
 */
struct audio_codec_cfg cs47l63_test_cfg(audio_route_t route);

int cs47l63_test_configure(const struct device *dev, audio_route_t route);

#endif /* CS47L63_EMUL_H_ */
