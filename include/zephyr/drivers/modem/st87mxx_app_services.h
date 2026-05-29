/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ST87Mxx application services public API.
 */

#ifndef STM87MXX_APP_SRV_H
#define STM87MXX_APP_SRV_H

/**
 * @defgroup st87mxx_constants ST87Mxx Constants
 * @{
 */

/** Default timeout for application services sequences */
#define APP_MDM_CMD_TIMEOUT			K_SECONDS(180)

/** @} */

/**
 * @defgroup st87mxx_gnss_configuration GNSS configuration
 * @{
 */

/** Max number of sample positions */
#define GNSS_NB_MAX_POSITION			100
/** GNSS constellation selection:\n
 *	0: GPS\n
 *	1: GALLILEO\n
 *	2: GPS+GALLILEO
 */
#define GNSS_CONSTELLATION_ID			0
/** GNSS data format selection:\n
 *	0: ST_AT\n
 *	1: NMEA
 */
#define GNSS_FORMAT_TYPE			0

/**
 * Parameters used if ST_AT format enabled for GNSS.
 */
/** Position data	*/
#define GNSS_FORMAT_ST_POSITION			1
/** Accuracy data	*/
#define GNSS_FORMAT_ST_ACCURACY			0
/** Satellites info	*/
#define GNSS_FORMAT_ST_SATELLITES		0
/** Orientation info	*/
#define GNSS_FORMAT_ST_ORIENTATION		0

/**
 * Parameters used if NMEA format enabled for GNSS.
 */
/** $GPGGA info		*/
#define GNSS_NMEA_GPGGA				1
/** $GPGSA info		*/
#define GNSS_NMEA_GPGSA				0
/** $GPGSV info		*/
#define GNSS_NMEA_GPGSV				0
/** $GPGLL info		*/
#define GNSS_NMEA_GPGLL				1
/** $GPRMS info		*/
#define GNSS_NMEA_GPRMC				0
/** $GPVTG info		*/
#define GNSS_NMEA_GPVTG				1

/** @} */

/**
 * @defgroup st87mxx_wscan_configuration WIFI scanning configuration
 * @{
 */

/** To avoid too much data coming again and again on UART,
 * the Host has the possibility to set a filter mode
 * to display only one time the BSSID found during the full time
 * of WIFI scanning.\n
 * Possible values:\n
 * 0: Only the new SSID after one loop (Default mode)\n
 * 1: In infinite loop URC
 */
#define URC_MODE		1
/** Hopping time is the scan duration in ms before
 * swapping to next channel
 */
#define HOPPING_TIME	1024
/** Selects the suitable ST87 input for antenna (GNSS or NB-IOT).\n
 * Possible values:\n
 * 0: GNSS ant. input (default mode)\n
 * 1: NB-IOT ant. input
 */
#define ANT_SEL			1

/** @} */


/**
 * @brief Application services sequence state.
 *
 * Indicates whether no application service sequence is running,
 * one is in progress, or it timed out.
 */
typedef enum {
	/** No sequence is started */
	SEQUENCE_NONE		= 0,
	/** A sequence is on-going */
	SEQUENCE_ONGOING	= 1,
	/** The timeout for a sequence has elapsed. */
	SEQUENCE_TIMED_OUT	= 2
} sequence_state;

/**
 * @brief Callback that catches the WIFI scan data when it is available.
 *
 * Example of WSCAN data: 1,-73,48:29:52:14:3E:B0,Livebox-3EB0
 * that corresponds to: channel number, RxLevel, MAC address (BSSID), SSID name.
 *
 * @param string String in which the data will be written
 */
typedef void (st87mxx_get_beacon_data_callback_t) (char const * const string);

/**
 * @brief Parameters for the ST87Mxx WIFI scan operation.
 *
 * This structure holds all relevant parameters needed to execute a WIFI scan,
 * including the list of channels, scan iterations, beacon data callback,
 * and a timeout.
 */
typedef struct {
	/** Pointer to the string list of Wifi channels,
	 *  in the range [1,14], e.g. "1,6,11".
	 */
	char *channel_list;
	/** Number of times the scan will run the channel list.
	 *  Value 0 means the system will run in an Infinite loop.
	 */
	uint32_t nb_scan_iterations;
	/** Pointer to callback  function called each time beacon data is available.
	 * The beacon format is: Channel number, RxLevel, MAC address (BSSID) and SSID name
	 */
	st87mxx_get_beacon_data_callback_t *get_beacon_data_callback_func;
	/** Timeout in sec. after which the request is cancelled. */
	uint32_t timeout;
} st87mxx_wifiscan_params;


/**
 * @brief API to start the application services (GNSS, WSCAN, RSSI capture).
 *
 * This function should be called prior to any application services.\n
 * Note: only one application service can run at a time.
 */
void st87mxx_app_services_init(void);

/**
 * @brief Callback to retrieve the positions.
 *
 * @param string String in which the position will be written.
 */
typedef void (st87mxx_get_pos_callback_t) (char const *const string);

/**
 * @brief API to get ST87Mxx GNSS position.
 *
 * The position returned by the callback is in ST_AT format by default
 * and can be set to NMEA format by changing GNSS_CONSTELLATION_ID constant.
 *
 * For instance for ST_AT: \#GNSSFIX: 0,2291,461282763,48.15370,-01.56719,130.2,
 * 33.6,00.0,00.0,52.2,2.4,9.5,7.7,5,11,2.4,30,-0.1,20,1.4,09,1.7,06,-0.3\n
 * In detail: " \#GNSSFIX: validity,week_number,time_of_week,latitude,longitude,
 * altitude,accuracy
 * [,std_dev_altitude,hdop,gdop,pdop] <- If GNSS_FORMAT_ST_ACCURACY is 1\n
 * [,number_of_satellites,satellite1_id,satellite1_residual,...,\n
 * satelliten_id,satelliten_residual] <- If GNSS_FORMAT_ST_SATELLITES is 1\n
 * [,orientation_degree,semi_major,semi_minor] <- If GNSS_FORMAT_ST_ORIENTATION is 1\n
 *
 * For instance for NMEA: $GPGLL,4809.2231,N,00134.281,W,091218,A,A*6C \n
 * For further information: https://en.wikipedia.org/wiki/NMEA_0183
 *
 * @param nb_position Number of GNSS position to sample. Value 0 means infinite.
 * @param get_pos_callback_func Pointer to the output callback function
 * @param timeout Timeout in sec after which the request is cancelled
 * @retval Execution status
 */
int st87mxx_gnss_getfix(uint32_t nb_position,
	st87mxx_get_pos_callback_t *get_pos_callback_func, uint32_t timeout);

/**
 * @brief API to stop the GNSS feature.
 */
void st87mxx_gnss_stop(void);

/**
 * @brief API to start the WIFI scan (=WSCAN) feature for positioning purpose.
 *
 * The WIFI scan feature is typically used to determine a coarse position when GNSS is\n
 * not usable, such as in indoor environments.\n
 * The primary purpose of the WIFI scan feature is to detect and list all available\n
 * WIFI networks in the vicinity.\n
 * When a WIFI scan is initiated, the ST87MXX discovers nearby WIFI networks.\n
 * It listens to beacons from WIFI access points (APs) that provide network\n
 * information. The resulting list of detected WIFI networks can be sent to a\n
 * 3rd party localization server to get a position.
 *
 * @param wscan_params Structure containing the parameters to start the WSCAN feature.
 * @retval Execution status
 */
int st87mxx_wifiscan(st87mxx_wifiscan_params *wscan_params);

/**
 * @brief API to stop the WIFI scan.
 */
void st87mxx_wifiscan_stop(void);

/**
 * @brief Callback to retrieve the RSSI values.
 *
 * @param string String in which the RSSI will be written.
 */
typedef void (st87mxx_get_rssi_callback_t) (char const *const string);

/**
 * @brief API to get the RSSI in an asynchronous way.
 *
 * @param get_rssi_callback_func Pointer to the output callback function
 * @retval API execution status
 */
int st87mxx_getrssi(st87mxx_get_rssi_callback_t *get_rssi_callback_func);

/**
 * @brief API to get the state of the application services sequence.
 *
 * @retval State of the current sequence
 */
sequence_state st87mxx_app_services_getstate(void);

#endif /* STM87MXX_APP_SRV_H */
