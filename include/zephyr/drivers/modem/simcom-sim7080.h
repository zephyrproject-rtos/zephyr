/*
 * Copyright (C) 2021 metraTec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MODEM_SIMCOM_SIM7080_H
#define ZEPHYR_INCLUDE_DRIVERS_MODEM_SIMCOM_SIM7080_H

#include <zephyr/types.h>

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SIM7080_GNSS_DATA_UTC_LEN 20
#define SIM7080_SMS_MAX_LEN 160

struct sim7080_gnss_data {
	/**
	 * Whether gnss is powered or not.
	 */
	bool run_status;
	/**
	 * Whether fix is acquired or not.
	 */
	bool fix_status;
	/**
	 * UTC in format yyyyMMddhhmmss.sss
	 */
	char utc[SIM7080_GNSS_DATA_UTC_LEN];
	/**
	 * Latitude in 10^-7 degree.
	 */
	int32_t lat;
	/**
	 * Longitude in 10^-7 degree.
	 */
	int32_t lon;
	/**
	 * Altitude in mm.
	 */
	int32_t alt;
	/**
	 * Horizontal dilution of precision in 10^-2.
	 */
	uint16_t hdop;
	/**
	 * Course over ground un 10^-2 degree.
	 */
	uint16_t cog;
	/**
	 * Speed in 10^-1 km/h.
	 */
	uint16_t kmh;
};

/**
 * Possible sms states in memory.
 */
enum sim7080_sms_stat {
	SIM7080_SMS_STAT_REC_UNREAD = 0,
	SIM7080_SMS_STAT_REC_READ,
	SIM7080_SMS_STAT_STO_UNSENT,
	SIM7080_SMS_STAT_STO_SENT,
	SIM7080_SMS_STAT_ALL,
};

/**
 * Possible ftp return codes.
 */
enum sim7080_ftp_rc {
	/* Operation finished correctly. */
	SIM7080_FTP_RC_OK = 0,
	/* Session finished. */
	SIM7080_FTP_RC_FINISHED,
	/* An error occurred. */
	SIM7080_FTP_RC_ERROR,
};

/**
 * Buffer structure for sms.
 */
struct sim7080_sms {
	/* First octet of the sms. */
	uint8_t first_octet;
	/* Message protocol identifier. */
	uint8_t tp_pid;
	/* Status of the sms in memory. */
	enum sim7080_sms_stat stat;
	/* Index of the sms in memory. */
	uint16_t index;
	/* Time the sms was received. */
	struct {
		uint8_t year;
		uint8_t month;
		uint8_t day;
		uint8_t hour;
		uint8_t minute;
		uint8_t second;
		uint8_t timezone;
	} time;
	/* Buffered sms. */
	char data[SIM7080_SMS_MAX_LEN + 1];
	/* Length of the sms in buffer. */
	uint8_t data_len;
};

/**
 * Buffer structure for sms reads.
 */
struct sim7080_sms_buffer {
	/* sms structures to read to. */
	struct sim7080_sms *sms;
	/* Number of sms structures. */
	uint8_t nsms;
};

/**
 * @brief Power on the Sim7080.
 *
 * @return 0 on success. Otherwise -1 is returned.
 */
int mdm_sim7080_power_on(void);

/**
 * @brief Power off the Sim7080.
 *
 * @return 0 on success. Otherwise -1 is returned.
 */
int mdm_sim7080_power_off(void);

/**
 * @brief Starts the modem in network operation mode.
 *
 * @return 0 on success. Otherwise <0 is returned.
 */
int mdm_sim7080_start_network(void);

/**
 * @brief Starts the modem in gnss operation mode.
 *
 * @return 0 on success. Otherwise <0 is returned.
 */
int mdm_sim7080_start_gnss(void);

/**
 * @brief Query gnss position form the modem.
 *
 * @return 0 on success. If no fix is acquired yet -EAGAIN is returned.
 *         Otherwise <0 is returned.
 */
int mdm_sim7080_query_gnss(struct sim7080_gnss_data *data);

/**
 * Get the sim7080 manufacturer.
 */
const char *mdm_sim7080_get_manufacturer(void);

/**
 * Get the sim7080 model information.
 */
const char *mdm_sim7080_get_model(void);

/**
 * Get the sim7080 revision.
 */
const char *mdm_sim7080_get_revision(void);

/**
 * Get the sim7080 imei number.
 */
const char *mdm_sim7080_get_imei(void);

/**
 * Read sms from sim module.
 *
 * @param buffer Buffer structure for sms.
 * @return Number of sms read on success. Otherwise -1 is returned.
 *
 * @note The buffer structure needs to be initialized to
 * the size of the sms buffer. When this function finishes
 * successful, nsms will be set to the number of sms read.
 * If the whole structure is filled a subsequent read may
 * be needed.
 */
int mdm_sim7080_read_sms(struct sim7080_sms_buffer *buffer);

/**
 * Delete a sms at a given index.
 *
 * @param index The index of the sms in memory.
 * @return 0 on success. Otherwise -1 is returned.
 */
int mdm_sim7080_delete_sms(uint16_t index);

/**
 * Start a ftp get session.
 *
 * @param server The ftp servers address.
 * @param user User name for the ftp server.
 * @param passwd Password for the ftp user.
 * @param file File to be downloaded.
 * @param path Path to the file on the server.
 * @return 0 if the session was started. Otherwise -1 is returned.
 */
int mdm_sim7080_ftp_get_start(const char *server, const char *user, const char *passwd,
				  const char *file, const char *path);

/**
 * Read data from a ftp get session.
 *
 * @param dst The destination buffer.
 * @param size Initialize to the size of dst. Gets set to the number
 *             of bytes actually read.
 * @return According sim7080_ftp_rc.
 */
int mdm_sim7080_ftp_get_read(char *dst, size_t *size);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MODEM_SIMCOM_SIM7080_H */
