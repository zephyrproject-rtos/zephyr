/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef STM87MXX_APP_SRV_H
#define STM87MXX_APP_SRV_H

#define APP_MDM_CMD_TIMEOUT			K_SECONDS(180)

/* GNSS configuration */
#define GNSS_NB_MAX_POSITION			100		/* Max number of sample positions */
#define GNSS_CONSTELLATION_ID			0		/* GNSS constellation selection:
									0: GPS
									1: GALLILEO
									2: GPS+GALLILEO */

#define GNSS_FORMAT_TYPE			0		/* GNSS data format selection:
									0: ST_AT
									1: NMEA */

/* Parameter used if ST_AT format enabled for GNSS */
#define GNSS_FORMAT_ST_POSITION			1		/* Position data	*/
#define GNSS_FORMAT_ST_ACCURACY			0		/* Accuracy data	*/
#define GNSS_FORMAT_ST_SATELLITES		0		/* Satellites info	*/
#define GNSS_FORMAT_ST_ORIENTATION		0		/* Orientation info	*/

/* Parameter used if NMEA format enabled for GNSS */
#define GNSS_NMEA_GPGGA				1		/* $GPGGA info		*/
#define GNSS_NMEA_GPGSA				0		/* $GPGSA info		*/
#define GNSS_NMEA_GPGSV				0		/* $GPGSV info		*/
#define GNSS_NMEA_GPGLL				1		/* $GPGLL info		*/
#define GNSS_NMEA_GPRMC				0		/* $GPRMS info		*/
#define GNSS_NMEA_GPVTG				1		/* $GPVTG info		*/

/* WIFI scanning configuration */
#define URC_MODE		1		/* To avoid too much data coming again and again on UART,
						the Host has the possibility to set a filter mode
						to display only one time the BSSID found during the full time
						of Wi-Fi scanning.
						Possible values:
							0: Only the new SSID after one loop (Default mode)
							1: In infinite loop URC */
#define HOPPING_TIME	1024			/* Hopping time is the scan duration in ms before
							swapping to next channel*/
#define ANT_SEL			1		/* Selects the suitable ST87 input for antenna (GNSS or NB-IOT).
						Possible values:
							0: GNSS ant. input (default mode)
							1: NB-IOT ant. input*/

/* On-going sequence states */
typedef enum  {
	SEQUENCE_NONE		= 0,
	SEQUENCE_ONGOING	= 1,
	SEQUENCE_TIMED_OUT	= 2
} sequence_state_t;

typedef struct{
	sequence_state_t sequence_state;
} st87mxx_state_t;

/* Callback that catches the WSCAN data when it is available. */
/* Example of WSCAN data: 1,-73,48:29:52:14:3E:B0,Livebox-3EB0 */
/* that corresponds to: channel number, RxLevel, MAC address (BSSID), SSID name */
typedef void (st87mxx_get_beacon_data_callback_t) (char const * const string);

typedef struct {
	char *channel_list;		/* Pointer to the string list of Wifi channels,
						in the range [1,14], e.g. "1,6,11".*/
	uint32_t nb_scan_iterations;/* Number of times the scan will run the channel list.
					Value 0 means the system will run in an Infinite loop.*/
	st87mxx_get_beacon_data_callback_t *get_beacon_data_callback_func;  /* Pointer to
					callback  function called each time beacon data is available. */
	/* The beacon format is: Channel number, RxLevel, MAC address (BSSID) and SSID name */
	uint32_t timeout;		/*Timeout in sec. after which the request is cancelled.*/
} st87mxx_wifiscan_params_t;

/* Exported functions ---------------------------------------------------------*/
/**
* API to start the application services (GNSS, WSCAN, RSSI capture).
* This function should be called prior to any application services.
**/
void st87mxx_app_services_init(void);

typedef void (st87mxx_get_pos_callback_t) (char const * const string);

/**
* API to get ST87Mxx GNSS position.
*
* The position returned by the callback is in ST_AT format by default
* and can be set to NMEA format in the config file.
*
* For instance for ST_AT: #GNSSFIX: 0,2291,461282763,48.15370,-01.56719,130.2,
* 33.6,00.0,00.0,52.2,2.4,9.5,7.7,5,11,2.4,30,-0.1,20,1.4,09,1.7,06,-0.3
* In detail: " #GNSSFIX: <validity>,<week_number>,<time_of_week>,<latitude>,<longitude>,
* <altitude>,<accuracy>
* [,<std_dev_altitude>,<hdop>,<gdop>,<pdop>] <- If accuracy data is enabled	in config
* [,<number_of_satellites>,<satellite1_id>,<satellite1_residual>,...,
* <satelliten_id>,<satelliten_residual>] <- If satellites info is enabled  in config
* [,<orientation_degree>,<semi_major>,<semi_minor>] <- If orientation
* info is enabled  in config
*
* For instance for NMEA: $GPGLL,4809.2231,N,00134.281,W,091218,A,A*6C
* For further information: https://en.wikipedia.org/wiki/NMEA_0183
*
* param nb_position: Number of GNSS position to sample. Value 0 means infinite.
* param get_pos_callback_func: Pointer to the output callback function
* param timeout: Timeout in sec after which the request is cancelled
* retval Execution status
*/
int st87mxx_gnss_getfix(uint32_t nb_position,
	st87mxx_get_pos_callback_t *get_pos_callback_func, uint32_t timeout);

/**
* API to stop the GNSS feature.
**/
void st87mxx_gnss_stop(void);

/**
* API to start the Wifi scan feature for positioning purpose.
* The Wi-Fi scan feature is typically used to determine a coarse position when GNSS is
* not usable, such as in indoor environments.
* The primary purpose of the Wi-Fi scan feature is to detect and list all available
* Wi-Fi networks in the vicinity.
* When a Wi-Fi scan is initiated, the ST87MXX discovers nearby Wi-Fi networks.
* It listens to beacons from Wi-Fi access points (APs) that provide network
* information. The resulting list of detected Wi-Fi networks can be sent to a
* 3rd party localization server to get a position.
* param wscan_params: structure containing the parameters to start the WSCAN feature.
* retval Execution status
*/
int st87mxx_wifiscan(st87mxx_wifiscan_params_t *wscan_params);

/**
* API to stop the Wifi scan.
**/
void st87mxx_wifiscan_stop(void);

typedef void (st87mxx_get_rssi_callback_t) (char const * const string);

/**
* API to get the #STENG URC to get the requested information (e.g. RSSI) in an asynchronous way.
*
* param get_rssi_callback_func: Pointer to the output callback function
* retval API execution status
*/
int st87mxx_getrssi(st87mxx_get_rssi_callback_t *get_rssi_callback_func);

typedef void (st87mxx_get_energy_callback_t) (char const * const string);

/**
* API to get the state of the application services sequences.
*
* param state: variable in which the state will be written
* retval API execution status
*/
int st87mxx_app_services_getstate(st87mxx_state_t * state);

#endif /* STM87MXX_APP_SRV_H */
