/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Vendor-specific API for the Infineon AutAnalog CTDAC driver.
 *
 * Provides vendor-specific helpers for loading waveform data and
 * accessing DAC range-detection status for waveform channels
 * (HW channels 0-14).
 */

#ifndef SOC_INFINEON_EDGE_COMMON_INFINEON_AUTANALOG_CTDAC_H_
#define SOC_INFINEON_EDGE_COMMON_INFINEON_AUTANALOG_CTDAC_H_

#include <zephyr/device.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Write waveform data to a DAC waveform channel's LUT region.
 *
 * Loads an array of sample values into the DAC look-up table (LUT) at the
 * address range defined by the channel's DTS child node (start-addr to
 * end-addr).  The channel must have been configured via dac_channel_setup()
 * first.
 *
 * This function is only valid for waveform channels (Zephyr channel IDs
 * 0-14, mapping to HW channels 0-14).  It is not valid for the FW channel
 * (channel 15).
 *
 * @param dev        DAC device (infineon,autanalog-ctdac instance).
 * @param channel    Zephyr channel ID (0-14).
 * @param samples    Pointer to the waveform sample array (12-bit values).
 * @param num_samples Number of samples to write.  Must not exceed the
 *                    channel's LUT range (end_addr - start_addr + 1).
 *
 * @retval 0 on success.
 * @retval -EINVAL if parameters are invalid.
 * @retval -EIO if the PDL write failed.
 */
int dac_ifx_autanalog_ctdac_write_waveform(const struct device *dev, uint8_t channel,
					   const int32_t *samples, uint16_t num_samples);

/**
 * @brief Read the DAC range-detection status bitmap.
 *
 * Returns the current range-detection status for waveform channels
 * 0-14. Bit N corresponds to waveform channel N. The FW channel
 * (channel 15) is not represented.
 *
 * @param dev DAC device (infineon,autanalog-ctdac instance).
 * @param channel Zephyr channel ID (0-14).
 * @param status Pointer to the returned status bitmap.
 *
 * @retval 0 on success.
 * @retval -EINVAL if parameters are invalid or the DAC channel has not
 *                 been set up yet.
 */
int dac_ifx_autanalog_ctdac_get_limit_status(const struct device *dev, uint8_t channel,
					     uint16_t *status);

/**
 * @brief Clear DAC range-detection status bits.
 *
 * Clears the selected range-detection status bits for waveform channels
 * 0-14. Bit N in @p channel_mask corresponds to waveform channel N. The
 * FW channel bit (bit 15) is invalid.
 *
 * @param dev DAC device (infineon,autanalog-ctdac instance).
 * @param channel Zephyr channel ID (0-14).
 *
 * @retval 0 on success.
 * @retval -EINVAL if parameters are invalid, the DAC channel has not
 *                 been set up yet, or the FW channel bit is included.
 */
int dac_ifx_autanalog_ctdac_clear_limit_status(const struct device *dev, uint8_t channel);

#ifdef __cplusplus
}
#endif

#endif /* SOC_INFINEON_EDGE_COMMON_INFINEON_AUTANALOG_CTDAC_H_ */
