/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Netfeasa Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MODEM_HL78XX_GNSS_AUX_PARSE_H_
#define ZEPHYR_DRIVERS_MODEM_HL78XX_GNSS_AUX_PARSE_H_

#include <zephyr/types.h>
#include <zephyr/drivers/modem/hl78xx_apis.h>

/**
 * @brief Parse a GSA NMEA argument list into HL78XX auxiliary GNSS data.
 *
 * Expected argument layout:
 * argv[0]: $xxGSA
 * argv[1]: mode
 * argv[2]: fix type
 * argv[3-14]: satellite IDs
 * argv[15]: PDOP
 * argv[16]: HDOP
 * argv[17]: VDOP
 *
 * @param argv NMEA argument vector.
 * @param argc Number of arguments in argv.
 * @param aux_data Destination auxiliary GNSS data.
 *
 * @retval 0 on success.
 * @retval -EINVAL if mandatory input is invalid.
 */
int hl78xx_gnss_parse_gsa(char **argv, uint16_t argc, struct hl78xx_gnss_nmea_aux_data *aux_data);

/**
 * @brief Parse a GST NMEA argument list into HL78XX auxiliary GNSS data.
 *
 * Expected argument layout:
 * argv[0]: $xxGST
 * argv[1]: UTC time
 * argv[2]: RMS error
 * argv[3]: semi-major error
 * argv[4]: semi-minor error
 * argv[5]: orientation
 * argv[6]: latitude error
 * argv[7]: longitude error
 * argv[8]: altitude error
 *
 * Optional numeric fields are defaulted to zero when empty or invalid.
 * The UTC field is mandatory.
 *
 * @param argv NMEA argument vector.
 * @param argc Number of arguments in argv.
 * @param aux_data Destination auxiliary GNSS data.
 *
 * @retval 0 on success.
 * @retval -EINVAL if mandatory input is invalid.
 */
int hl78xx_gnss_parse_gst(char **argv, uint16_t argc, struct hl78xx_gnss_nmea_aux_data *aux_data);

/**
 * @brief Parse a PSEPU NMEA argument list into HL78XX auxiliary GNSS data.
 *
 * Expected argument layout:
 * argv[0]: $PSEPU
 * argv[1]: position 3D uncertainty
 * argv[2]: position 2D uncertainty
 * argv[3]: position latitude uncertainty
 * argv[4]: position longitude uncertainty
 * argv[5]: position altitude uncertainty
 * argv[6]: velocity 3D uncertainty
 * argv[7]: velocity 2D uncertainty
 * argv[8]: velocity heading uncertainty
 * argv[9]: velocity east uncertainty
 * argv[10]: velocity north uncertainty
 * argv[11]: velocity up uncertainty
 *
 * Empty or invalid optional numeric fields are defaulted to zero. This allows
 * an empty PSEPU sentence to still act as an end-of-aux-frame marker.
 *
 * @param argv NMEA argument vector.
 * @param argc Number of arguments in argv.
 * @param aux_data Destination auxiliary GNSS data.
 *
 * @retval 0 on success.
 * @retval -EINVAL if mandatory input is invalid or too few fields are present.
 */
int hl78xx_gnss_parse_epu(char **argv, uint16_t argc, struct hl78xx_gnss_nmea_aux_data *aux_data);

#endif /* ZEPHYR_DRIVERS_MODEM_HL78XX_GNSS_AUX_PARSE_H_ */
