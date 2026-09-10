/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_MAX20356_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_MAX20356_H_

/**
 * @file
 * @ingroup mfd_max20356
 * @brief ADI MAX20356 PMIC MFD properties and APIs.
 */

#include <stdint.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup mfd_max20356 MAX20356/MAX20358 API
 * @ingroup mfd_interfaces
 * @brief Public API for MAX20356/MAX20358 MFD driver.
 * @{
 */

/**
 * @brief Device variant.
 *
 * Selected from the devicetree compatible string at build time.
 */
enum max20356_variant {
	/** MAX20356 */
	MAX20356_VARIANT_MAX20356,
	/** MAX20358 (adds the LDO4Cfg.LDO4RTC feature) */
	MAX20356_VARIANT_MAX20358,
};

/**
 * @brief Power-mode / reset commands issued through PwrCmd (0x83).
 */
enum max20356_power_cmd {
	/** Turn the PMIC off (PwrCmd 0xB2) */
	MAX20356_PWR_OFF,
	/** Hard reset (PwrCmd 0xC3) */
	MAX20356_PWR_HARD_RESET,
	/** Soft reset (PwrCmd 0xD4) */
	MAX20356_PWR_SOFT_RESET,
	/** Seal the device (PwrCmd 0xE5) */
	MAX20356_PWR_SEAL,
};

/**
 * @brief Watchdog reset-type action (WDCntl.WDRstType).
 *
 * Fallback for behavior the standard wdt subsystem API cannot express, in
 * particular the charger/limiter-only reset action.
 */
enum max20356_wdt_rsttype {
	/** Watchdog off (WDRstType 0b00) */
	MAX20356_WDT_OFF = 0x0,
	/** Charger + limiter register reset (WDRstType 0b01) */
	MAX20356_WDT_CHG_LIM_RST = 0x1,
	/** Soft reset (WDRstType 0b10) */
	MAX20356_WDT_SOFT_RESET = 0x2,
	/** Hard reset (WDRstType 0b11) */
	MAX20356_WDT_HARD_RESET = 0x3,
};

/**
 * @brief Password-protected lock domains.
 *
 * Each domain corresponds to one bit of a LockMsk register and its matching
 * LockUnlock password register. mfd_max20356_reg_update_locked() unmasks a single
 * domain, unlocks it, performs the write and re-locks. Domains BUCK1..LDO4 live in
 * LockMsk1/LockUnlock1; CHG/LIM/WD live in LockMsk3/LockUnlock3.
 *
 * The power-up sequencing field of each rail ((rail)Ena.(rail)Seq[7:5]) is not
 * guarded by the rail's main domain: it has a separate LockMsk2/LockUnlock2 bit
 * (BUCK1..LDO4) so a write to the seq field of a locked part must unlock the
 * matching *_SEQ domain, not the main one.
 */
enum max20356_lock_domain {
	/** Buck1 registers (LockMsk1.Bk1Lck) */
	MAX20356_LOCK_BUCK1,
	/** Buck2 registers (LockMsk1.Bk2Lck) */
	MAX20356_LOCK_BUCK2,
	/** Buck3 registers (LockMsk1.Bk3Lck) */
	MAX20356_LOCK_BUCK3,
	/** Buck-boost registers (LockMsk1.BbLck) */
	MAX20356_LOCK_BBST,
	/** LDO1 registers (LockMsk1.Ld1Lck) */
	MAX20356_LOCK_LDO1,
	/** LDO2 registers (LockMsk1.Ld2Lck) */
	MAX20356_LOCK_LDO2,
	/** LDO3 registers (LockMsk1.Ld3Lck) */
	MAX20356_LOCK_LDO3,
	/** LDO4 registers (LockMsk1.Ld4Lck) */
	MAX20356_LOCK_LDO4,
	/** Charger registers (LockMsk3.ChgLck) */
	MAX20356_LOCK_CHG,
	/** Input-limiter registers (LockMsk3.LimLck) */
	MAX20356_LOCK_LIM,
	/** Watchdog registers (LockMsk3.WdLck) */
	MAX20356_LOCK_WD,
	/** Buck1 sequencing field (LockMsk2.Bk1SeqLck) */
	MAX20356_LOCK_BUCK1_SEQ,
	/** Buck2 sequencing field (LockMsk2.Bk2SeqLck) */
	MAX20356_LOCK_BUCK2_SEQ,
	/** Buck3 sequencing field (LockMsk2.Bk3SeqLck) */
	MAX20356_LOCK_BUCK3_SEQ,
	/** Buck-boost sequencing field (LockMsk2.BbSeqLck) */
	MAX20356_LOCK_BBST_SEQ,
	/** LDO1 sequencing field (LockMsk2.Ld1SeqLck) */
	MAX20356_LOCK_LDO1_SEQ,
	/** LDO2 sequencing field (LockMsk2.Ld2SeqLck) */
	MAX20356_LOCK_LDO2_SEQ,
	/** LDO3 sequencing field (LockMsk2.Ld3SeqLck) */
	MAX20356_LOCK_LDO3_SEQ,
	/** LDO4 sequencing field (LockMsk2.Ld4SeqLck) */
	MAX20356_LOCK_LDO4_SEQ,
	/** Number of lock domains */
	MAX20356_LOCK_MAX,
};

/**
 * @brief Interrupt event groups dispatched from the INTB trigger.
 *
 * Consumers subscribe to a group with mfd_max20356_add_callback(); the parent
 * unmasks only the sources for which at least one callback is registered.
 */
enum max20356_event {
	/** ChgStat / JEITA / CC1Tmo changes */
	MAX20356_EVT_CHARGER,
	/** UsbOk / UsbOVP */
	MAX20356_EVT_USB,
	/** ThmSD / ThmStat / thermal LDO/buck shutdown */
	MAX20356_EVT_THERMAL,
	/** UVLO / SC / DRP / buck-boost fault, load-switch timeout */
	MAX20356_EVT_REG_FAULT,
	/** Dedicated DVS / PGOOD transition complete */
	MAX20356_EVT_DVS_DONE,
	/** Watchdog timer */
	MAX20356_EVT_WATCHDOG,
	/** Miscellaneous (I2cTmo, StepChg, ...) */
	MAX20356_EVT_MISC,
	/** Number of event groups */
	MAX20356_EVT_MAX,
};

/**
 * @brief Power-Mode Function (PFN) Control pin identifiers.
 */
enum max20356_pfn {
	/** PFN1 pin (PFN.PFN1Pin) */
	MAX20356_PFN1,
	/** PFN2 pin (PFN.PFN2Pin) */
	MAX20356_PFN2,
};

/**
 * @brief Hardware interrupt sources routable to an MPC output pin.
 *
 * The MAX20356 can mirror a buck power-good transition or the USBOK status onto
 * one of the multi-purpose control (MPC) pins configured as an interrupt output.
 */
enum max20356_mpc_int_source {
	/** Buck1 power-good (BK1ITRCfg) */
	MAX20356_MPC_INT_BUCK1_PGOOD,
	/** Buck2 power-good (BK2ITRCfg) */
	MAX20356_MPC_INT_BUCK2_PGOOD,
	/** Buck3 power-good (BK3ITRCfg) */
	MAX20356_MPC_INT_BUCK3_PGOOD,
	/** USBOK status (USBOKITRCfg) */
	MAX20356_MPC_INT_USBOK,
};

/**
 * @brief INTB event callback.
 *
 * Invoked from the trigger workqueue (never from ISR context) when a subscribed
 * event fires.
 *
 * @param dev MAX20356 MFD parent device.
 * @param evt Event group that fired.
 * @param user User data supplied at registration.
 */
typedef void (*max20356_cb_t)(const struct device *dev, enum max20356_event evt, void *user);

/**
 * @brief Read a single register.
 *
 * @param dev MAX20356 MFD parent device.
 * @param reg Register address.
 * @param val Destination for the read byte.
 *
 * @retval 0 On success.
 * @retval -errno Negative errno propagated from the I2C bus.
 */
int mfd_max20356_reg_read(const struct device *dev, uint8_t reg, uint8_t *val);

/**
 * @brief Write a single register.
 *
 * @param dev MAX20356 MFD parent device.
 * @param reg Register address.
 * @param val Byte to write.
 *
 * @retval 0 On success.
 * @retval -errno Negative errno propagated from the I2C bus.
 */
int mfd_max20356_reg_write(const struct device *dev, uint8_t reg, uint8_t val);

/**
 * @brief Read-modify-write selected bits of a register.
 *
 * The read-modify-write is performed as a single bus-locked I2C transaction.
 *
 * @param dev MAX20356 MFD parent device.
 * @param reg Register address.
 * @param mask Mask of bits to modify.
 * @param val New value for the masked bits.
 *
 * @retval 0 On success.
 * @retval -errno Negative errno propagated from the I2C bus.
 */
int mfd_max20356_reg_update(const struct device *dev, uint8_t reg, uint8_t mask, uint8_t val);

/**
 * @brief Read-modify-write a password-protected register.
 *
 * Performs the REQ-LOCK-001 sequence: unmask @p domain in its LockMsk register,
 * write the unlock password (0x55) to the matching LockUnlock register, apply the
 * masked update, then re-apply the lock password (0xAA). The whole sequence is
 * serialized against other lock users by the parent.
 *
 * @param dev MAX20356 MFD parent device.
 * @param domain Lock domain guarding @p reg.
 * @param reg Register address.
 * @param mask Mask of bits to modify.
 * @param val New value for the masked bits.
 *
 * @retval 0 On success.
 * @retval -EINVAL Invalid lock domain.
 * @retval -errno Negative errno propagated from the I2C bus.
 */
int mfd_max20356_reg_update_locked(const struct device *dev, enum max20356_lock_domain domain,
				   uint8_t reg, uint8_t mask, uint8_t val);

/**
 * @brief Clock one SPI DVS command byte to the part.
 *
 * Writes a single {AD[1:0], VLT[5:0]} byte to the SPI controller referenced by
 * the parent's spi-dvs phandle (wired to MPC0/1/2). Used by the regulator driver
 * to drive a buck's voltage in SPI DVS mode (Mode 2); the command encoding is the
 * caller's responsibility.
 *
 * @param dev MAX20356 MFD parent device.
 * @param cmd Command byte to clock out.
 *
 * @retval 0 On success.
 * @retval -ENOTSUP SPI support not enabled (CONFIG_SPI).
 * @retval -ENODEV SPI device not present or not ready.
 * @retval -errno Negative errno propagated from the SPI bus.
 */
int mfd_max20356_dvs_spi_write(const struct device *dev, uint8_t cmd);

/**
 * @brief Get the device variant.
 *
 * @param dev MAX20356 MFD parent device.
 *
 * @return The variant selected by the devicetree compatible string.
 */
enum max20356_variant mfd_max20356_get_variant(const struct device *dev);

/**
 * @brief Issue a power-mode / reset command.
 *
 * @param dev MAX20356 MFD parent device.
 * @param cmd Command to issue.
 *
 * @retval 0 On success.
 * @retval -EINVAL Invalid command.
 * @retval -errno Negative errno propagated from the I2C bus.
 */
int mfd_max20356_power_command(const struct device *dev, enum max20356_power_cmd cmd);

/**
 * @brief Select the IVMON monitor-mux channel.
 *
 * @param dev MAX20356 MFD parent device.
 * @param channel Monitor channel (MONCfg.MONCtr, 0..15).
 * @param ratio Divider ratio selection (MONCfg.MONRatioCfg, 0..3).
 *
 * @retval 0 On success.
 * @retval -EINVAL Channel or ratio out of range.
 * @retval -errno Negative errno propagated from the I2C bus.
 */
int mfd_max20356_mon_select(const struct device *dev, uint8_t channel, uint8_t ratio);

/**
 * @brief Read a pushbutton (PFN) pin status.
 *
 * @param dev MAX20356 MFD parent device.
 * @param pfn Pushbutton pin to read.
 * @param active Destination, set true when the pin reads asserted (PFN bit = 1).
 *
 * @retval 0 On success.
 * @retval -EINVAL Invalid @p pfn or NULL @p active.
 * @retval -errno Negative errno propagated from the I2C bus.
 */
int mfd_max20356_pfn_status(const struct device *dev, enum max20356_pfn pfn, bool *active);

/**
 * @brief Route a hardware interrupt source to an MPC output pin.
 *
 * Programs the source's ITRCfg register so a transition of @p source drives the
 * selected MPC pin(s) and, optionally, sets the source's own interrupt-enable
 * bit. The MPC pin must be configured as an interrupt output separately.
 *
 * @param dev MAX20356 MFD parent device.
 * @param source Interrupt source to route.
 * @param mpc_mask Bitmask of MPC pins (BIT(0)..BIT(6)) the source drives; MPC7
 *                 has no select bit and is ignored.
 * @param int_enable True to also set the source's interrupt-enable bit.
 *
 * @retval 0 On success.
 * @retval -EINVAL Invalid @p source.
 * @retval -errno Negative errno propagated from the I2C bus.
 */
int mfd_max20356_mpc_int_route(const struct device *dev, enum max20356_mpc_int_source source,
			       uint8_t mpc_mask, bool int_enable);

#ifdef CONFIG_WDT_MAX20356
/**
 * @brief Set the watchdog reset-type action (WDCntl.WDRstType).
 *
 * @param dev MAX20356 MFD parent device.
 * @param rsttype Reset-type action.
 *
 * @retval 0 On success.
 * @retval -errno Negative errno propagated from the I2C bus.
 */
int mfd_max20356_wdt_set_rsttype(const struct device *dev, enum max20356_wdt_rsttype rsttype);

/**
 * @brief Feed the watchdog.
 *
 * The MAX20356 watchdog is fed by reading Int5.WDTmr. Int5 is clear-on-read and
 * also carries I2cTmoInt, so this helper reads the whole register under the
 * parent lock and, when INTB trigger support is enabled, dispatches the MISC
 * event group if an I2cTmoInt was pending, so the feed does not silently discard
 * that source. Used by the watchdog child; see REQ-WDT-004.
 *
 * @param dev MAX20356 MFD parent device.
 *
 * @retval 0 On success.
 * @retval -errno Negative errno propagated from the I2C bus.
 */
int mfd_max20356_wdt_feed(const struct device *dev);

/**
 * @brief Claim or release exclusive INTB ownership for the watchdog.
 *
 * The watchdog feed reads Int5, which is clear-on-read and also carries
 * I2cTmoInt, so an armed watchdog and the INTB event dispatch cannot safely
 * share the interrupt path. While claimed, mfd_max20356_add_callback() is
 * refused with -EBUSY; conversely a claim is refused with -EBUSY if any event
 * callback is already registered. The I2cTmoInt source is exempt: it only shares
 * Int5 and is expected to be silently consumed by the feed.
 *
 * @param dev MAX20356 MFD parent device.
 * @param claim True to acquire exclusive ownership, false to release it.
 *
 * @retval 0 On success.
 * @retval -EBUSY An event callback is registered (on claim)
 */
int mfd_max20356_wdt_claim(const struct device *dev, bool claim);

#endif /* CONFIG_WDT_MAX20356 */

#ifdef CONFIG_MFD_MAX20356_TRIGGER
/**
 * @brief Register an INTB event callback.
 *
 * The parent unmasks the hardware sources backing @p evt when the first callback
 * for that group is registered.
 *
 * @param dev MAX20356 MFD parent device.
 * @param evt Event group to subscribe to.
 * @param cb Callback to invoke.
 * @param user User data passed back to the callback.
 *
 * @retval 0 On success.
 * @retval -EINVAL Invalid event or callback.
 * @retval -ENOTSUP Trigger support not enabled (CONFIG_MFD_MAX20356_TRIGGER).
 * @retval -errno Negative errno from the I2C bus while unmasking.
 */
int mfd_max20356_add_callback(const struct device *dev, enum max20356_event evt,
			      max20356_cb_t cb, void *user);

/**
 * @brief Remove an INTB event callback.
 *
 * @param dev MAX20356 MFD parent device.
 * @param evt Event group the callback was registered for.
 * @param cb Callback to remove.
 *
 * @retval 0 On success.
 * @retval -EINVAL Callback not found.
 * @retval -ENOTSUP Trigger support not enabled (CONFIG_MFD_MAX20356_TRIGGER).
 */
int mfd_max20356_remove_callback(const struct device *dev, enum max20356_event evt,
				 max20356_cb_t cb);
#endif /* CONFIG_MFD_MAX20356_TRIGGER */

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_MAX20356_H_ */
