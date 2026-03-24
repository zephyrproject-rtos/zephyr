/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_DRIVERS_AUDIO_WM8960_SRC_WM8960_EMUL_H_
#define ZEPHYR_TESTS_DRIVERS_AUDIO_WM8960_SRC_WM8960_EMUL_H_

#include <zephyr/drivers/emul.h>

/**
 * @brief Get the value of a WM8960 register
 *
 * @param target Pointer to the emulator
 * @param reg_addr Register address to read
 * @return Register value
 */
uint16_t wm8960_emul_get_reg(const struct emul *target, uint8_t reg_addr);

/**
 * @brief Set the value of a WM8960 register
 *
 * @param target Pointer to the emulator
 * @param reg_addr Register address to write
 * @param val Value to write
 */
void wm8960_emul_set_reg(const struct emul *target, uint8_t reg_addr, uint16_t val);

/**
 * @brief Check if the WM8960 is powered up
 *
 * @param target Pointer to the emulator
 * @return true if powered up, false otherwise
 */
bool wm8960_emul_is_powered_up(const struct emul *target);

/**
 * @brief Check if the DAC is enabled
 *
 * @param target Pointer to the emulator
 * @return true if DAC is enabled, false otherwise
 */
bool wm8960_emul_is_dac_enabled(const struct emul *target);

/**
 * @brief Check if the ADC is enabled
 *
 * @param target Pointer to the emulator
 * @return true if ADC is enabled, false otherwise
 */
bool wm8960_emul_is_adc_enabled(const struct emul *target);

/**
 * @brief Check if the headphone output is enabled
 *
 * @param target Pointer to the emulator
 * @return true if headphone output is enabled, false otherwise
 */
bool wm8960_emul_is_headphone_enabled(const struct emul *target);

/**
 * @brief Check if the speaker output is enabled
 *
 * @param target Pointer to the emulator
 * @return true if speaker output is enabled, false otherwise
 */
bool wm8960_emul_is_speaker_enabled(const struct emul *target);

/**
 * @brief Get the volume setting for a specific channel
 *
 * @param target Pointer to the emulator
 * @param channel Channel to get volume for:
 *                0 = Left DAC, 1 = Right DAC
 *                2 = Left Headphone, 3 = Right Headphone
 *                4 = Left Speaker, 5 = Right Speaker
 *                6 = Left ADC, 7 = Right ADC
 * @return Volume setting
 */
uint16_t wm8960_emul_get_volume(const struct emul *target, uint8_t channel);

/**
 * @brief Check if a channel is muted
 *
 * @param target Pointer to the emulator
 * @param channel Channel to check:
 *                0 = Left DAC, 1 = Right DAC
 *                2 = Left Headphone, 3 = Right Headphone
 *                4 = Left Speaker, 5 = Right Speaker
 * @return true if muted, false otherwise
 */
bool wm8960_emul_is_muted(const struct emul *target, uint8_t channel);

/**
 * @brief Get the configured audio interface format
 *
 * @param target Pointer to the emulator
 * @return Audio format (0 = Right Justified, 1 = Left Justified, 2 = I2S, 3 = DSP)
 */
uint8_t wm8960_emul_get_audio_format(const struct emul *target);

/**
 * @brief Get the configured word length
 *
 * @param target Pointer to the emulator
 * @return Word length in bits (16, 20, 24, or 32)
 */
uint8_t wm8960_emul_get_word_length(const struct emul *target);

/**
 * @brief Check if the PLL is enabled
 *
 * @param target Pointer to the emulator
 * @return true if PLL is enabled, false otherwise
 */
bool wm8960_emul_is_pll_enabled(const struct emul *target);

#endif /* ZEPHYR_TESTS_DRIVERS_AUDIO_WM8960_SRC_WM8960_EMUL_H_ */
