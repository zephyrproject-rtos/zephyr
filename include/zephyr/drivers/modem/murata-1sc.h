/*
 * Copyright (c) 2023 T-Mobile USA, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <zephyr/net/buf.h>

#define MDM_1SC_IMEI_LENGTH               16
#define MDM_1SC_IMSI_LENGTH               16
#define MDM_1SC_ICCID_LENGTH              32
#define MDM_1SC_PHN_LENGTH                16
#define TLS_MURATA_USE_PROFILE            99
#define TLS_MURATA_CLIENT_VERIFY          100
#define MAX_IP_RESP_SIZE                  256
#define MDM_1SC_GOLDEN_LEN                16
#define MAX_PSM_RESP_SIZE                 128
#define MAX_EDRX_RESP_SIZE                128

struct init_fw_data_t {
	char *imagename;
	uint32_t imagesize;
	uint32_t imagecrc;
};

struct send_fw_data_t {
	char *data;
	int more;
	size_t len;
};

enum murata_1sc_io_ctl {
	GET_IPV4_CONF = 0x200,
	GET_ATCMD_RESP,
	INIT_FW_XFER,
	SEND_FW_HEADER,
	SEND_FW_DATA,
	INIT_FW_UPGRADE,
	GET_CHKSUM_ABILITY,
	GET_FILE_MODE,
	RESET_MODEM,
	AT_MODEM_CFUN_SET,
	AT_MODEM_PSM_SET,
	AT_MODEM_PSM_GET,
	AT_MODEM_EDRX_SET,
	AT_MODEM_EDRX_GET,
	AT_MODEM_EDRX_PTW_SET,
	AT_MODEM_EDRX_PTW_GET,
	STORE_CERT,
	DEL_CERT,
	CHECK_CERT,
	DELETE_CERT_PROFILE,
	CREATE_CERT_PROFILE,
};

struct set_cpsms_params  {
	uint8_t mode;
	uint8_t t3412;
	uint8_t t3324;
};

struct set_cedrxs_params  {
	uint8_t mode;
	uint8_t act_type;
	uint8_t time_mask;
};

struct murata_cert_params {
	/** Certificate filename */
	char *filename;
	/** Certificate structure */
	struct tls_credential *cert;
};

struct murata_tls_profile_params {
	/** ID for profile, can be any value between 1 and 255 */
	uint8_t profile_id_num;
	/** Filename for CA certificate */
	char *ca_file;
	/** Filename for CA certificate */
	char *ca_path;
	/** Filename for device certificate */
	char *dev_cert;
	/** Filename for device private key */
	char *dev_key;
	/** Filename for PSK ID */
	char *psk_id;
	/** Filename for PSK key */
	char *psk_key;
};

union params_cmd {
	char cfun;
	struct set_cpsms_params psm;
	struct set_cedrxs_params edrx;
	struct murata_cert_params cparams;
	struct murata_tls_profile_params profile;
	char response[128];
};

enum mdmdata_e {
	apn_e,
	awake_e,
	connsts_e,
	edrx_e,
	golden_e,
	iccid_e,
	imei_e,
	imsi_e,
	ip_e,
	ip6_e,
	msisdn_e,
	psm_e,
	sim_info_e,
	sleep_e,
	ssi_e,
	version_e,
	wake_e,
	invalid
};

struct mdmdata_cmd_t {
	const char *str;
	enum mdmdata_e atcmd;
};

extern const struct mdmdata_cmd_t cmd_pool[];

#define TMO_MODEM_ALSEEP_STR "ASLEEP"
#define TMO_MODEM_AWAKE_STR "AWAKE"
#define TMO_MODEM_UNKNOWN_STR "UNKNOWN"
