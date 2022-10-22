/*
 * Copyright (c) 2022
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief FT8XX audio playback
 */

#ifndef ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_AUDIO_H_
#define ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_AUDIO_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <errno.h>

/**
 * @brief FT8xx audio engine functions
 * @defgroup ft8xx_audio FT8xx Audio Engine
 * @ingroup ft8xx_interface
 * @{
 */



enum ft8xx_audio_formats_t {
    FT8xx_AUDIO_LINEAR  = 0x00,
    FT8xx_AUDIO_ULAW    = 0x01,
    FT8xx_AUDIO_LINEAR  = 0x02,
}

enum ft8xx_audio_synth_sound_t {
    FT8xx_AUDIO_SYNTH_SILENCE       = 0x0,
    FT8xx_AUDIO_SYNTH_SQUARE_WAVE   = 0x1,
    FT8xx_AUDIO_SYNTH_SINE_WAVE_    = 0x2,
    FT8xx_AUDIO_SYNTH_SAWTOOTH_WAVE = 0x3,
    FT8xx_AUDIO_SYNTH_TRIANGLE_WAVE = 0x4,
    FT8xx_AUDIO_SYNTH_BEEPING_      = 0x5,
    FT8xx_AUDIO_SYNTH_ALARM_        = 0x6,
    FT8xx_AUDIO_SYNTH_WARBLE_       = 0x7,
    FT8xx_AUDIO_SYNTH_CAROUSEL_     = 0x8,
    FT8xx_AUDIO_SYNTH_1_SHORT_PIP   = 0x10,
    FT8xx_AUDIO_SYNTH_2_SHORT_PIPS  = 0x11,
    FT8xx_AUDIO_SYNTH_3_SHORT_PIPS  = 0x12,
    FT8xx_AUDIO_SYNTH_4_SHORT_PIPS  = 0x13,
    FT8xx_AUDIO_SYNTH_5_SHORT_PIPS  = 0x14,
    FT8xx_AUDIO_SYNTH_6_SHORT_PIPS  = 0x15,
    FT8xx_AUDIO_SYNTH_7_SHORT_PIPS  = 0x16,
    FT8xx_AUDIO_SYNTH_8_SHORT_PIPS  = 0x17,
    FT8xx_AUDIO_SYNTH_9_SHORT_PIPS  = 0x18,
    FT8xx_AUDIO_SYNTH_10_SHORT_PIPS = 0x19,
    FT8xx_AUDIO_SYNTH_11_SHORT_PIPS = 0x1A,
    FT8xx_AUDIO_SYNTH_12_SHORT_PIPS = 0x1B,
    FT8xx_AUDIO_SYNTH_13_SHORT_PIPS = 0x1C,
    FT8xx_AUDIO_SYNTH_14_SHORT_PIPS = 0x1D,
    FT8xx_AUDIO_SYNTH_15_SHORT_PIPS = 0x1E,
    FT8xx_AUDIO_SYNTH_16_SHORT_PIPS = 0x1F,
    FT8xx_AUDIO_SYNTH_DTMF_HASH     = 0x23,
    FT8xx_AUDIO_SYNTH_DTMF_ASTERISK = 0x2C,
    FT8xx_AUDIO_SYNTH_DTMF_0        = 0x30,
    FT8xx_AUDIO_SYNTH_DTMF_2        = 0x32,
    FT8xx_AUDIO_SYNTH_DTMF_1        = 0x31,
    FT8xx_AUDIO_SYNTH_DTMF_3        = 0x33,
    FT8xx_AUDIO_SYNTH_DTMF_4        = 0x34,
    FT8xx_AUDIO_SYNTH_DTMF_5        = 0x35,
    FT8xx_AUDIO_SYNTH_DTMF_6        = 0x36,
    FT8xx_AUDIO_SYNTH_DTMF_7        = 0x37,
    FT8xx_AUDIO_SYNTH_DTMF_8        = 0x38,
    FT8xx_AUDIO_SYNTH_DTMF_9        = 0x39,
    FT8xx_AUDIO_SYNTH_HARP          = 0x40,
    FT8xx_AUDIO_SYNTH_XYLOPHONE     = 0x41,
    FT8xx_AUDIO_SYNTH_TUBA          = 0x42,
    FT8xx_AUDIO_SYNTH_GLOCKENSPIEL  = 0x43,
    FT8xx_AUDIO_SYNTH_ORGAN         = 0x44,
    FT8xx_AUDIO_SYNTH_TRUMPET       = 0x45,
    FT8xx_AUDIO_SYNTH_PIANO         = 0x46,
    FT8xx_AUDIO_SYNTH_CHIMES        = 0x47,
    FT8xx_AUDIO_SYNTH_MUSIC_BOX     = 0x48,
    FT8xx_AUDIO_SYNTH_BELL          = 0x49,
    FT8xx_AUDIO_SYNTH_CLICK         = 0x50,
    FT8xx_AUDIO_SYNTH_SWITCH        = 0x51,
    FT8xx_AUDIO_SYNTH_COWBELL       = 0x52,
    FT8xx_AUDIO_SYNTH_NOTCH         = 0x53,
    FT8xx_AUDIO_SYNTH_HIHAT         = 0x54,
    FT8xx_AUDIO_SYNTH_KICKDRUM      = 0x55,
    FT8xx_AUDIO_SYNTH_POP           = 0x56,
    FT8xx_AUDIO_SYNTH_CLACK         = 0x57,
    FT8xx_AUDIO_SYNTH_CHACK         = 0x58,
    FT8xx_AUDIO_SYNTH_MUTE          = 0x60,
    FT8xx_AUDIO_SYNTH_UNMUTE        = 0x61,

}

enum ft8xx_audio_synth_tone_t {
    FT8xx_AUDIO_SYNTH_TONE_A0           = 0x15, // 27.5Hz
    FT8xx_AUDIO_SYNTH_TONE_A0_SHARP     = 0x16, // 29.1Hz
    FT8xx_AUDIO_SYNTH_TONE_B0           = 0x17, // 30.9Hz
    FT8xx_AUDIO_SYNTH_TONE_C1           = 0x18, // 32.7Hz
    FT8xx_AUDIO_SYNTH_TONE_C1_SHARP     = 0x19, // 34.6Hz
    FT8xx_AUDIO_SYNTH_TONE_D1           = 0x1A, // 36.7Hz
    FT8xx_AUDIO_SYNTH_TONE_D1_SHARP     = 0x1B, // 38.9Hz
    FT8xx_AUDIO_SYNTH_TONE_E1           = 0x1C, // 41.2Hz
    FT8xx_AUDIO_SYNTH_TONE_F1           = 0x1D, // 43.7Hz
    FT8xx_AUDIO_SYNTH_TONE_F1_SHARP     = 0x1E, // 46.2Hz
    FT8xx_AUDIO_SYNTH_TONE_G1           = 0x1F, // 49Hz
    FT8xx_AUDIO_SYNTH_TONE_G1_SHARP     = 0x20, // 51.9Hz
    FT8xx_AUDIO_SYNTH_TONE_A1           = 0x21, // 55Hz
    FT8xx_AUDIO_SYNTH_TONE_A1_SHARP     = 0x22, // 58.3Hz
    FT8xx_AUDIO_SYNTH_TONE_B1           = 0x23, // 61.7Hz
    FT8xx_AUDIO_SYNTH_TONE_C2           = 0x24, // 65.4Hz
    FT8xx_AUDIO_SYNTH_TONE_C2_SHARP     = 0x25, // 69.3Hz
    FT8xx_AUDIO_SYNTH_TONE_D2           = 0x26, // 73.4Hz
    FT8xx_AUDIO_SYNTH_TONE_D2_SHARP     = 0x27, // 77.8Hz
    FT8xx_AUDIO_SYNTH_TONE_E2           = 0x28, // 82.4Hz
    FT8xx_AUDIO_SYNTH_TONE_F2           = 0x29, // 87.3Hz
    FT8xx_AUDIO_SYNTH_TONE_F2_SHARP     = 0x2A, // 92.5Hz
    FT8xx_AUDIO_SYNTH_TONE_G2           = 0x2B, // 98Hz
    FT8xx_AUDIO_SYNTH_TONE_G2_SHARP     = 0x2C, // 103.8Hz
    FT8xx_AUDIO_SYNTH_TONE_A2           = 0x2D, // 110Hz
    FT8xx_AUDIO_SYNTH_TONE_A2_SHARP     = 0x2E, // 116.5Hz
    FT8xx_AUDIO_SYNTH_TONE_B2           = 0x2F, // 123.5Hz
    FT8xx_AUDIO_SYNTH_TONE_C3           = 0x30, // 130.8Hz
    FT8xx_AUDIO_SYNTH_TONE_C3_SHARP     = 0x31, // 138.6Hz
    FT8xx_AUDIO_SYNTH_TONE_D3           = 0x32, // 146.8Hz
    FT8xx_AUDIO_SYNTH_TONE_D3_SHARP     = 0x33, // 155.6Hz
    FT8xx_AUDIO_SYNTH_TONE_E3           = 0x34, // 164.8Hz
    FT8xx_AUDIO_SYNTH_TONE_F3           = 0x35, // 174.6Hz
    FT8xx_AUDIO_SYNTH_TONE_F3_SHARP     = 0x36, // 185Hz
    FT8xx_AUDIO_SYNTH_TONE_G3           = 0x37, // 196Hz
    FT8xx_AUDIO_SYNTH_TONE_G3_SHARP     = 0x38, // 207.7Hz
    FT8xx_AUDIO_SYNTH_TONE_A3           = 0x39, // 220Hz
    FT8xx_AUDIO_SYNTH_TONE_A3_SHARP     = 0x3A, // 233.1Hz
    FT8xx_AUDIO_SYNTH_TONE_B3           = 0x3B, // 246.9Hz
    FT8xx_AUDIO_SYNTH_TONE_C4           = 0x3C, // 261.6Hz
    FT8xx_AUDIO_SYNTH_TONE_C4_SHARP     = 0x3D, // 277.2Hz
    FT8xx_AUDIO_SYNTH_TONE_D4           = 0x3E, // 293.7Hz
    FT8xx_AUDIO_SYNTH_TONE_D4_SHARP     = 0x3F, // 311.1Hz
    FT8xx_AUDIO_SYNTH_TONE_E4           = 0x40, // 329.6Hz
    FT8xx_AUDIO_SYNTH_TONE_F4           = 0x41, // 349.2Hz
    FT8xx_AUDIO_SYNTH_TONE_F4_SHARP     = 0x42, // 370Hz
    FT8xx_AUDIO_SYNTH_TONE_G4           = 0x43, // 392Hz
    FT8xx_AUDIO_SYNTH_TONE_G4_SHARP     = 0x44, // 415.3Hz
    FT8xx_AUDIO_SYNTH_TONE_A4           = 0x45, // 440Hz
    FT8xx_AUDIO_SYNTH_TONE_A4_SHARP     = 0x46, // 466.2Hz
    FT8xx_AUDIO_SYNTH_TONE_B4           = 0x47, // 493.9Hz
    FT8xx_AUDIO_SYNTH_TONE_C5           = 0x48, // 523.3Hz
    FT8xx_AUDIO_SYNTH_TONE_C5_SHARP     = 0x49, // 554.4Hz
    FT8xx_AUDIO_SYNTH_TONE_D5           = 0x4A, // 587.3Hz
    FT8xx_AUDIO_SYNTH_TONE_D5_SHARP     = 0x4B, // 622.3Hz
    FT8xx_AUDIO_SYNTH_TONE_E5           = 0x4C, // 659.3Hz
    FT8xx_AUDIO_SYNTH_TONE_F5           = 0x4D, // 698.5Hz
    FT8xx_AUDIO_SYNTH_TONE_F5_SHARP     = 0x4E, // 740Hz
    FT8xx_AUDIO_SYNTH_TONE_G5           = 0x4F, // 784Hz
    FT8xx_AUDIO_SYNTH_TONE_G5_SHARP     = 0x50, // 830.6Hz
    FT8xx_AUDIO_SYNTH_TONE_A5           = 0x51, // 880Hz
    FT8xx_AUDIO_SYNTH_TONE_A5_SHARP     = 0x52, // 932.3Hz
    FT8xx_AUDIO_SYNTH_TONE_B5           = 0x53, // 987.8Hz
    FT8xx_AUDIO_SYNTH_TONE_C6           = 0x54, // 1046.5Hz
    FT8xx_AUDIO_SYNTH_TONE_C6_SHARP     = 0x55, // 1108.7Hz
    FT8xx_AUDIO_SYNTH_TONE_D6           = 0x56, // 1174.7Hz
    FT8xx_AUDIO_SYNTH_TONE_D6_SHARP     = 0x57, // 1244.5Hz
    FT8xx_AUDIO_SYNTH_TONE_E6           = 0x58, // 1318.5Hz
    FT8xx_AUDIO_SYNTH_TONE_F6           = 0x59, // 1396.9Hz
    FT8xx_AUDIO_SYNTH_TONE_F6_SHARP     = 0x5A, // 1480Hz
    FT8xx_AUDIO_SYNTH_TONE_G6           = 0x5B, // 1568Hz
    FT8xx_AUDIO_SYNTH_TONE_G6_SHARP     = 0x5C, // 1661.2Hz
    FT8xx_AUDIO_SYNTH_TONE_A6           = 0x5D, // 1760Hz
    FT8xx_AUDIO_SYNTH_TONE_A6_SHARP     = 0x5E, // 1864.7Hz
    FT8xx_AUDIO_SYNTH_TONE_B6           = 0x5F, // 1975.5Hz
    FT8xx_AUDIO_SYNTH_TONE_C7           = 0x60, // 2093Hz
    FT8xx_AUDIO_SYNTH_TONE_C7_SHARP     = 0x61, // 2217.5Hz
    FT8xx_AUDIO_SYNTH_TONE_D7           = 0x62, // 2349.3Hz
    FT8xx_AUDIO_SYNTH_TONE_D7_SHARP     = 0x63, // 2489Hz
    FT8xx_AUDIO_SYNTH_TONE_E7           = 0x64, // 2637Hz
    FT8xx_AUDIO_SYNTH_TONE_F7           = 0x65, // 2793.8Hz
    FT8xx_AUDIO_SYNTH_TONE_F7_SHARP     = 0x66, // 2960Hz
    FT8xx_AUDIO_SYNTH_TONE_G7           = 0x67, // 3136Hz
    FT8xx_AUDIO_SYNTH_TONE_G7_SHARP     = 0x68, // 3322.4Hz
    FT8xx_AUDIO_SYNTH_TONE_A7           = 0x69, // 3520Hz
    FT8xx_AUDIO_SYNTH_TONE_A7_SHARP     = 0x6A, // 3729.3Hz
    FT8xx_AUDIO_SYNTH_TONE_B7           = 0x6B, // 3951.1Hz
    FT8xx_AUDIO_SYNTH_TONE_C8           = 0x6C, // 4186Hz
}

/**
 * @brief Load Audio Sample to FT8xx memory
 *
 * @param dev Pointer to device
 * @param start_address Location in G_RAM to store sample (Must be 64 bit aligned)
 * @param sample Pointer to Sample
 * @param sample_length Size of sample in bytes (Must be 64 bit aligned)
 */
 int ft8xx_audio_load(const struct device *dev, uint32_t start_address, uint8_t* sample, uint32_t sample_length);
 
/**
 * @brief Play audio stored in memory
 *
 * @param dev Pointer to device
 * @param start_address Location in G_RAM of sample
 * @param sample_length Size of sample in bytes
 * @param audio_format Format of audio sample
 * @param sample_freq Format of audio sample 
 * @param vol playback volume
 * @param loop Loop playback 
 */ 
 int ft8xx_audio_play(const struct device *dev, uint32_t start_address, uint32_t sample_length, uint8_t audio_format, uint16_t sample_freq, uint8_t vol, bool loop);
 
/**
 * @brief Get audio playback status
 *
 * @param dev Pointer to device
 */ 
 int ft8xx_audio_get_status(const struct device *dev);
 
 /**
 * @brief Stop audio playback
 *
 * @param dev Pointer to device
 */ 
 int ft8xx_audio_stop(const struct device *dev);

 /**
 * @brief Synthesize audio
 *
 * @param dev Pointer to device
 * @param sound Sound to Synthesize
 * @param note Note to use Synthesizing (not all sound support tone adjustment)
 */ 
 int ft8xx_audio_synth_start(const struct device *dev, uint8_t sound, uint8_t note, uint8_t vol);
 
 /**
 * @brief Get audio Synthesis status
 *
 * @param dev Pointer to device
 */ 
 int ft8xx_audio_synth_get_status(const struct device *dev);
 
  /**
 * @brief Stop audio Synthesis
 *
 * @param dev Pointer to device
 */ 
 int ft8xx_audio_synth_stop(const struct device *dev);








/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_AUDIO_H_ */