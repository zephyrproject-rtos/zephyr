/** @file
 * @brief Simcom SIM7080 modem public API header file.
 * @ingroup simcom_sim7080_interface
 *
 * Copyright (C) 2021 metraTec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MODEM_SIMCOM_SIM7080_H
#define ZEPHYR_INCLUDE_DRIVERS_MODEM_SIMCOM_SIM7080_H

#include <zephyr/types.h>

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup simcom_sim7080_interface SIMCom SIM7080
 * @brief SIMCom SIM7080 cellular modems.
 * @ingroup cellular_interface_ext
 * @{
 */

/** Maximum Length of GNSS UTC data */
#define SIM7080_GNSS_DATA_UTC_LEN 20
/** Maximum SMS length */
#define SIM7080_SMS_MAX_LEN 160
/** Maximum UE system information band size */
#define SIM7080_UE_SYS_INFO_BAND_SIZE 32
/** Maximum number of DNS retries */
#define SIM7080_DNS_MAX_RECOUNT 10
/** Maximum timeout for DNS queries in milliseconds */
#define SIM7080_DNS_MAX_TIMEOUT_MS 60000

/** Channel Mask for NB-IoT */
#define SIM7080_LTE_CHANNEL_MASK_NB1 0x7BEFFU
/** Channel Mask for CAT-M1 */
#define SIM7080_LTE_CHANNEL_MASK_M1  0x5FFFFU
/** Number of LTE channels supported by the modem */
#define SIM7080_NUM_LTE_CHANNELS     19U

/** Sim7080 modem state */
enum sim7080_state {
	SIM7080_STATE_INIT = 0, /**< Initial modem state */
	SIM7080_STATE_IDLE, /**< Modem idle */
	SIM7080_STATE_NETWORKING, /**< Network active */
	SIM7080_STATE_GNSS, /**< GNSS active */
	SIM7080_STATE_OFF, /**< Modem off */
};

/** Modem radio access technology */
enum sim7080_rat {
	SIM7080_RAT_LTE_NB1,  /**< NB-IoT only */
	SIM7080_RAT_LTE_M1,   /**< LTE CAT M1 only */
	SIM7080_RAT_LTE_AUTO, /**< Modem automatically selects M1 or NB1 */
	SIM7080_RAT_GSM,      /**< GSM */
};

/** Supported LTE channels */
enum sim7080_lte_chan {
	SIM7080_LTE_CHAN_B1 = BIT(0),   /**< LTE Band 1 */
	SIM7080_LTE_CHAN_B2 = BIT(1),   /**< LTE Band 2 */
	SIM7080_LTE_CHAN_B3 = BIT(2),   /**< LTE Band 3 */
	SIM7080_LTE_CHAN_B4 = BIT(3),   /**< LTE Band 4 */
	SIM7080_LTE_CHAN_B5 = BIT(4),   /**< LTE Band 5 */
	SIM7080_LTE_CHAN_B8 = BIT(5),   /**< LTE Band 8 */
	SIM7080_LTE_CHAN_B12 = BIT(6),  /**< LTE Band 12 */
	SIM7080_LTE_CHAN_B13 = BIT(7),  /**< LTE Band 13 */
	SIM7080_LTE_CHAN_B14 = BIT(8),  /**< LTE Band 14 */
	SIM7080_LTE_CHAN_B18 = BIT(9),  /**< LTE Band 18 */
	SIM7080_LTE_CHAN_B19 = BIT(10), /**< LTE Band 19 */
	SIM7080_LTE_CHAN_B20 = BIT(11), /**< LTE Band 20 */
	SIM7080_LTE_CHAN_B25 = BIT(12), /**< LTE Band 25 */
	SIM7080_LTE_CHAN_B26 = BIT(13), /**< LTE Band 26 */
	SIM7080_LTE_CHAN_B27 = BIT(14), /**< LTE Band 27 */
	SIM7080_LTE_CHAN_B28 = BIT(15), /**< LTE Band 28 */
	SIM7080_LTE_CHAN_B66 = BIT(16), /**< LTE Band 66 */
	SIM7080_LTE_CHAN_B71 = BIT(17), /**< LTE Band 71 */
	SIM7080_LTE_CHAN_B85 = BIT(18), /**< LTE Band 85 */
};

/** Sim7080 gnss data structure */
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

/** Possible sms states in memory. */
enum sim7080_sms_stat {
	SIM7080_SMS_STAT_REC_UNREAD = 0, /**< Message unread */
	SIM7080_SMS_STAT_REC_READ, /**< Message read*/
	SIM7080_SMS_STAT_STO_UNSENT, /**< Message stored unsent */
	SIM7080_SMS_STAT_STO_SENT, /**< Message stored sent */
	SIM7080_SMS_STAT_ALL, /**< Status count */
};

/** Possible ftp return codes. */
enum sim7080_ftp_rc {
	SIM7080_FTP_RC_OK = 0, /**< Operation finished correctly. */
	SIM7080_FTP_RC_FINISHED, /**< Session finished. */
	SIM7080_FTP_RC_ERROR, /**< An error occurred. */
};

/**
 * Buffer structure for sms.
 */
struct sim7080_sms {
	/** First octet of the sms. */
	uint8_t first_octet;
	/** Message protocol identifier. */
	uint8_t tp_pid;
	/** Status of the sms in memory. */
	enum sim7080_sms_stat stat;
	/** Index of the sms in memory. */
	uint16_t index;
	/** Time the sms was received. */
	struct {
		uint8_t year; /**< Current Year */
		uint8_t month; /**< Month of the year */
		uint8_t day; /**< Day of the month */
		uint8_t hour; /**< Hour of the day */
		uint8_t minute; /**< Minute */
		uint8_t second; /**< Second */
		uint8_t timezone; /**< Current timezone */
	} time;
	/** Buffered sms. */
	char data[SIM7080_SMS_MAX_LEN + 1];
	/** Length of the sms in buffer. */
	uint8_t data_len;
};

/**
 * Buffer structure for sms reads.
 */
struct sim7080_sms_buffer {
	/** sms structures to read to. */
	struct sim7080_sms *sms;
	/** Number of sms structures. */
	uint8_t nsms;
};

/** UE system mode */
enum sim7080_ue_sys_mode {
	SIM7080_UE_SYS_MODE_NO_SERVICE, /**< No service */
	SIM7080_UE_SYS_MODE_GSM, /**< GSM */
	SIM7080_UE_SYS_MODE_LTE_CAT_M1, /**< LTE CAT M1 */
	SIM7080_UE_SYS_MODE_LTE_NB_IOT, /**< LTE NB IOT */
};

/** UE operating mode */
enum sim7080_ue_op_mode {
	SIM7080_UE_OP_MODE_ONLINE, /**< Online */
	SIM7080_UE_OP_MODE_OFFLINE, /**< Offline */
	SIM7080_UE_OP_MODE_FACTORY_TEST_MODE, /**< Factory test mode */
	SIM7080_UE_OP_MODE_RESET, /**< Reset */
	SIM7080_UE_OP_MODE_LOW_POWER_MODE, /**< Low power mode */
};

/**
 * Sim7080 ue system information structure for gsm.
 */
struct sim7080_ue_sys_info_gsm {
	/** Mobile country code */
	uint16_t mcc;
	/** Mobile network code */
	uint16_t mcn;
	/** Location area code */
	uint16_t lac;
	/** Cell ID */
	uint16_t cid;
	/** Absolute radio frequency channel number */
	uint8_t arfcn[SIM7080_UE_SYS_INFO_BAND_SIZE + 1];
	/** RX level in dBm */
	int16_t rx_lvl;
	/** Track LO adjust */
	int16_t track_lo_adjust;
	/** C1 coefficient */
	uint16_t c1;
	/** C2 coefficient */
	uint16_t c2;
};

/**
 * Sim7080 ue system information structure for LTE.
 */
struct sim7080_ue_sys_info_lte {
	/** Mobile country code */
	uint16_t mcc;
	/** Mobile network code */
	uint16_t mcn;
	/** Tracing area code */
	uint16_t tac;
	/** Serving Cell ID */
	uint32_t sci;
	/** Physical Cell ID */
	uint16_t pci;
	/** Frequency band */
	uint8_t band[SIM7080_UE_SYS_INFO_BAND_SIZE + 1];
	/** E-UTRA absolute radio frequency channel number */
	uint16_t earfcn;
	/** Downlink bandwidth in MHz */
	uint16_t dlbw;
	/** Uplink bandwidth in MHz */
	uint16_t ulbw;
	/** Reference signal received quality in dB */
	int16_t rsrq;
	/** Reference signal received power in dBm */
	int16_t rsrp;
	/** Received signal strength indicator in dBm */
	int16_t rssi;
	/** Reference signal signal to noise ratio in dB */
	int16_t rssnr;
	/** Signal to interference plus noise ratio in dB */
	int16_t sinr;
};

/**
 * Sim7080 ue system information structure.
 */
struct sim7080_ue_sys_info {
	/** Refer to sim7080_ue_sys_mode */
	enum sim7080_ue_sys_mode sys_mode;
	/** Refer to sim7080_ue_op_mode */
	enum sim7080_ue_op_mode op_mode;
	union {
		/** Only set if sys_mode is GSM */
		struct sim7080_ue_sys_info_gsm gsm;
		/** Only set if sys mode is LTE CAT-M1/NB-IOT */
		struct sim7080_ue_sys_info_lte lte;
	} cell; /**< Cell information */
};

/**
 * Get the current state of the modem.
 *
 * @return The current state.
 */
enum sim7080_state mdm_sim7080_get_state(void);

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
 * Forcefully reset the modem by pulling pwrkey for 15 seconds.
 * @note The state of the modem may be undefined after calling
 * this function. Call mdm_sim7080_power_on after force reset.
 */
void mdm_sim7080_force_reset(void);

/**
 * @brief Activates the network operation mode of the modem.
 *
 * @return 0 on success. Otherwise <0 is returned.
 * @note The modem needs to be booted for this function to work.
 * Concurrent use of network and gnss is not possible.
 */
int mdm_sim7080_start_network(void);

/**
 * @brief Stops the networking operation mode of the modem.
 *
 * @return 0 on success. Otherwise <0 is returned.
 */
int mdm_sim7080_stop_network(void);

/**
 * @brief Starts the modem in gnss operation mode.
 *
 * @return 0 on success. Otherwise <0 is returned.
 * @note The modem needs to be booted for this function to work.
 * Concurrent use of network and gnss is not possible.
 */
int mdm_sim7080_start_gnss(void);

/**
 * @brief Starts the modem in gnss operation mode with xtra functionality.
 *
 * @return 0 on success. Otherwise <0 is returned.
 * @note The modem needs to be booted for this function to work.
 * Concurrent use of network and gnss is not possible.
 * @note If enabling xtra functionality fails a normal cold start will be performed.
 * @warning On some modem firmware revisions (e.g. 1951B16SIM7080) power cycling the
 *          gnss functionality with an xtra file can cause a hangup of the modem.
 *          This can be triggered by calling mdm_sim7080_start_gnss_xtra(),
 *          then turning off gnss by calling mdm_sim7080_stop_gnss() and then re-enabling gnss
 *          by calling mdm_sim7080_start_gnss_xtra() again. In this case the modem will not
 *          respond to commands anymore and every call of a modem function will result in a timeout.
 *          In this case the whole modem needs to be power cycled by calling mdm_sim7080_power_off()
 *          and mdm_sim7080_power_on(). It is advised to power cycle the modem before using the gnss
 *          functionality to avoid hangups.
 */
int mdm_sim7080_start_gnss_xtra(void);

/**
 * @brief Stops the modem gnss operation mode.
 *
 * @return 0 on success. Otherwise <0 is returned.
 */
int mdm_sim7080_stop_gnss(void);

/**
 * @brief Download the XTRA file for assisted gnss.
 *
 * @param server_id Id of the server to download XTRA file from.
 * @param f_name The name of the XTRA file to download.
 * @return 0 on success. Otherwise <0 is returned.
 */
int mdm_sim7080_download_xtra(uint8_t server_id, const char *f_name);

/**
 * @brief Query the validity of the XTRA file.
 *
 * @param diff_h Difference between the local time and the XTRA inject time in hours.
 * @param duration_h Valid time of the XTRA file in hours.
 * @param inject Injection time of the XTRA file.
 */
int mdm_sim7080_query_xtra_validity(int16_t *diff_h, int16_t *duration_h, struct tm *inject);

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
 * Get the sim7080 iccid number.
 */
const char *mdm_sim7080_get_iccid(void);

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
 * Set the level of one of the module's GPIO pins
 *
 * @param gpio GPIO pin number
 * @param level New logical level of the GPIO
 * @return 0 on success. Otherwise -1 is returned.
 *
 * @note The GPIO will be configured as output implicitly.
 */
int mdm_sim7080_set_gpio(int gpio, int level);

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

/**
 * Read voltage, charge status and battery connection level.
 *
 * @param bcs [out] Charge status.
 * @param bcl [out] Battery connection level.
 * @param voltage [out] Battery voltage in mV.
 * @return 0 on success. Otherwise a negative error is returned.
 */
int mdm_sim7080_get_battery_charge(uint8_t *bcs, uint8_t *bcl, uint16_t *voltage);

/**
 * Read the ue system information
 *
 * @param info Destination buffer for information.
 * @return 0 on success. Otherwise a negative error is returned.
 */
int mdm_sim7080_get_ue_sys_info(struct sim7080_ue_sys_info *info);

/**
 * Get the local time of the modem.
 *
 * @param t Time structure to fill.
 * @return 0 on success. Otherwise a negative error is returned.
 * @note Time is set by network. It may take some time for it to get valid.
 */
int mdm_sim7080_get_local_time(struct tm *t);

/**
 * Set the dns query lookup parameters.
 *
 * @param recount Number of retries per query.
 *				  Maximum @c SIM7080_DNS_MAX_RECOUNT
 * @param timeout Timeout for a dns query in milliseconds.
 *				  Maximum @c SIM7080_DNS_MAX_TIMEOUT_MS
 * @return 0 on success. Otherwise a negative error is returned.
 */
int mdm_sim7080_dns_set_lookup_params(uint8_t recount, uint16_t timeout);

/**
 * Get the dns query lookup parameters.
 *
 * @param recount [out] Number of retries per query.
 * @param timeout [out] Timeout for a dns query in milliseconds.
 */
void mdm_sim7080_dns_get_lookup_params(uint8_t *recount, uint16_t *timeout);

#if defined(CONFIG_MODEM_SIMCOM_SIM7080_SOCKETS_SOCKOPT_TLS)
/**
 * Function to provide bytes to certificate upload functions.
 *
 * @param buffer Destination buffer for the certificate data.
 * @param max_len Maximum number of bytes that can be inserted to the buffer.
 * @param offset Offset to the start of the certificate in bytes.
 * @return The number of bytes copied to the buffer or a negative value on error.
 */
typedef int (*sim7080_tls_cert_read_func)(uint8_t *buf, size_t max_len, size_t offset);

/**
 * Import a root certificate to the modem.
 *
 * @param cert_name The NULL terminated name of the certificate.
 * @param read_func Function to read the certificate data.
 * @param cert_len Length of the certificacte in bytes.
 * @return 0 on success. A negative error code otherwise.
 *
 * @note Certificates need to be X.509 ITU-T encoded. Following formats are
 *       accepted: .pem, .der, .p7b.
 * @note The certificate will be stored permanently on the modem under the given name.
 *       This function only needs to be called only once, except the certificate changes.
 */
int mdm_sim7080_import_root_ca(const char *cert_name, sim7080_tls_cert_read_func read_func,
			       size_t cert_len);

/**
 * Import a client certificate and client key to the modem.
 *
 * @param cert_name The NULL terminated name of the client certificate.
 * @param cert_read_func Function to read the client certificate data.
 * @param cert_len Length of the client certificacte in bytes.
 * @param key_name The NULL terminated name of the client key.
 * @param key_read_func Function to read the client key data.
 * @param key_len Length of the client key in bytes.
 * @param passwd The NULL terminated password of the certificate. May be NULL.
 * @return 0 on success. A negative error code otherwise.
 *
 * @note Certificates need to be X.509 ITU-T encoded. Following formats are
 *       accepted: .pem, .der, .p7b.
 * @note The certificate will be stored permanently on the modem under the given name.
 *       This function only needs to be called only once, except the certificate changes.
 */
int mdm_sim7080_import_client_cert(const char *cert_name, sim7080_tls_cert_read_func cert_read_func,
				   size_t cert_len, const char *key_name,
				   sim7080_tls_cert_read_func key_read_func, size_t key_len,
				   const char *passwd);

/**
 * Import a dtls psk table to the modem.
 *
 * @param psk_name The NULL terminated name of the psk table.
 * @param read_func Function to read the psk data.
 * @param cert_len Length of the psk table in bytes.
 * @return 0 on success. A negative error code otherwise.
 *
 * @note The psk table contains pairs of <identity>:<psk>.
 *       There needs to be exactly one pair per line.
 *       The psk needs to be given as hex string..
 *       For example the credentials (Client_identity, 0x01020304)
 *       needs to be formatted as Client_identity:01020304.
 *		 Multiple pairs can be included in the same file separated by newlines.
 * @note The psk table will be stored permanently on the modem under the given name.
 *       This function only needs to be called only once, except the psk table changes.
 */
int mdm_sim7080_import_dtls_psk(const char *psk_name, sim7080_tls_cert_read_func read_func,
				size_t psk_len);

/**
 * Configure the certificates that should be used by the socket for tls.
 *
 * @param fd The socket file descriptor.
 * @param root_ca The NULL terminated root certificate name.
 * @param client_cert The NULL terminated client certificate name. May be NULL.
 * @return 0 on success. Otherwise a negative error code.
 *
 * @note This function needs tp be called before calling zsock_connect.
 */
int mdm_sim7080_configure_tls_certs(int fd, const char *root_ca, const char *client_cert);

/**
 * Configure the passkey table used by dtls.
 *
 * @param fd The socket file descriptor.
 * @param root_ca The NULL terminated passkey table name
 * @return 0 on success. Otherwise a negative error code.
 *
 * @note This function needs tp be called before calling zsock_connect.
 */
int mdm_sim7080_configure_dtls_psktable(int fd, const char *psktable);
#endif

/**
 * @brief Get the currently selected radio technology.
 *
 * @param rat [out] Gets set to the current technology.
 */
void mdm_sim7080_get_rat(enum sim7080_rat *rat);

/**
 * @brief Set the radio technology.
 *
 * @param rat The radio technology.
 * @retval 0 Success.
 * @retval -EINVAL The selected technology is not available.
 *
 * @note This function needs to be called before mdm_sim7080_start_network().
 *       If networking is already active, the change will not affect the current session.
 *		 To apply changes, networking has to be restarted.
 *		 A power cycle may be needed for the modem to forget the last working configuration.
 */
int mdm_sim7080_set_rat(enum sim7080_rat rat);

/**
 * @brief Get the currently used LTE bands.
 *
 * @param nb1 [out] Used NB-IoT bands. May be NULL.
 * @param m1 [out] Used CAT-M1 bands. May be NULL.
 */
void mdm_sim7080_get_lte_bands(uint32_t *nb1, uint32_t *m1);

/**
 * @brief Set the LTE bands to use.
 *
 * @param nb1 NB-IoT bands to use. 0 to not change the bands.
 * @param m1 CAT-M1 bands to use. 0 to not change the bands.
 * @retval 0 Success.
 * @retval -EINVAL The band selection is illegal.
 *
 * @note This function needs to be called before mdm_sim7080_start_network().
 *       If networking is already active, the change will not affect the current session.
 *		 To apply changes, networking has to be restarted.
 *		 A power cycle may be needed for the modem to forget the last working configuration.
 * @note Available NB-IoT and CAT-M1 differ. Refer to SIM7080_LTE_CHANNEL_MASK_NB1
 *       and SIM7080_LTE_CHANNEL_MASK_M1.
 */
int mdm_sim7080_set_lte_bands(uint32_t nb1, uint32_t m1);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_MODEM_SIMCOM_SIM7080_H */
