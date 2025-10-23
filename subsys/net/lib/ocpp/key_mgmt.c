/*
 * Copyright (c) 2025 Linumiz GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ocpp_i.h"

#if defined(CONFIG_OCPP_PROFILE_SMART_CHARGE)
#define CP_SC	",SmartCharging"
#else
#define CP_SC ""
#endif

#if defined(CONFIG_OCPP_PROFILE_REMOTE_TRIG)
#define CP_RT	",RemoteTrigger"
#else
#define CP_RT	""
#endif

#if defined(CONFIG_OCPP_PROFILE_RESERVATION)
#define CP_RE	",Reservation"
#else
#define CP_RE	""
#endif

#if defined(CONFIG_OCPP_PROFILE_LOCAL_AUTH_LIST)
#define CP_LAL	",LocalAuthListManagement"
#else
#define CP_LAL	""
#endif

#if defined(CONFIG_OCPP_PROFILE_FIRMWARE_MGNT)
#define CP_FM	",FirmwareManagement"
#else
#define CP_FM	""
#endif

#define CP_CORE "Core"

struct ocpp_cfg_info {
	enum ocpp_key_type type;
	bool is_rw;
	enum ocpp_key key;
	char *skey;
	union ocpp_keyval val;
};

#define FILL_KEY_TABLE(_type, _rw, _key, _skey, _val)[_key] = \
	{.type = _type, .is_rw = _rw, .key = _key, .skey = _skey, .val = {_val}}

static struct ocpp_cfg_info cached_key_cfg[OCPP_CFG_END] = {
	FILL_KEY_TABLE(KEY_TYPE_BOOL,	1, CFG_ALLOW_OFFLINE_TX_FOR_UNKN_ID,
			   "AllowOfflineTxForUnknownId", false),
	FILL_KEY_TABLE(KEY_TYPE_BOOL,	1, CFG_AUTH_CACHE_ENABLED,
			   "AuthorizationCacheEnabled", false),
	FILL_KEY_TABLE(KEY_TYPE_BOOL,	0, CFG_AUTH_REMOTE_TX_REQ,
			   "AuthorizeRemoteTxRequests", true),
	FILL_KEY_TABLE(KEY_TYPE_INT,	1, CFG_BLINK_REPEAT, "BlinkRepeat", 0),
	FILL_KEY_TABLE(KEY_TYPE_INT,	1, CFG_CLK_ALIGN_DATA_INTERVAL,
			   "ClockAlignedDataInterval", 0),
	FILL_KEY_TABLE(KEY_TYPE_INT,	1, CFG_CONN_TIMEOUT,
			   "ConnectionTimeOut", 0),
	FILL_KEY_TABLE(KEY_TYPE_INT,	0, CFG_GETCFG_MAX_KEY,
			   "GetConfigurationMaxKeys", 1),
	FILL_KEY_TABLE(KEY_TYPE_INT,	1, CFG_HEARTBEAT_INTERVAL,
			   "HeartbeatInterval", 0),
	FILL_KEY_TABLE(KEY_TYPE_INT,	1, CFG_LIGHT_INTENSITY,
			   "LightIntensity", 0),
	FILL_KEY_TABLE(KEY_TYPE_BOOL,	1, CFG_LOCAL_AUTH_OFFLINE,
			   "LocalAuthorizeOffline", 0),
	FILL_KEY_TABLE(KEY_TYPE_BOOL,	1, CFG_LOCAL_PREAUTH,
			   "LocalPreAuthorize", 0),
	FILL_KEY_TABLE(KEY_TYPE_INT,	1, CFG_MAX_ENERGYON_INVL_ID,
			   "MaxEnergyOnInvalidId", 0),
	FILL_KEY_TABLE(KEY_TYPE_CSL,	1, CFG_MTR_VAL_ALGIN_DATA,
			   "MeterValuesAlignedData", .str = "0"),
	FILL_KEY_TABLE(KEY_TYPE_INT,	0, CFG_MTR_VAL_ALGIN_DATA_MAXLEN,
			   "MeterValuesAlignedDataMaxLength", 1),
	FILL_KEY_TABLE(KEY_TYPE_CSL,	1, CFG_MTR_VAL_SAMPLED_DATA,
			   "MeterValuesSampledData", .str = "0"),
	FILL_KEY_TABLE(KEY_TYPE_INT,	0, CFG_MTR_VAL_SAMPLED_DATA_MAXLEN,
			   "MeterValuesSampledDataMaxLength", 1),
	FILL_KEY_TABLE(KEY_TYPE_INT,	1, CFG_MTR_VAL_SAMPLE_INTERVAL,
			   "MeterValueSampleInterval", 5),
	FILL_KEY_TABLE(KEY_TYPE_INT,	1, CFG_MIN_STATUS_DURATION,
			   "MinimumStatusDuration", 0),
	FILL_KEY_TABLE(KEY_TYPE_INT,	0, CFG_NO_OF_CONNECTORS,
			   "NumberOfConnectors", 0),
	FILL_KEY_TABLE(KEY_TYPE_INT,	1, CFG_REST_RETRIES,
			   "ResetRetries", 0),
	FILL_KEY_TABLE(KEY_TYPE_CSL,	1, CFG_CONN_PHASE_ROT,
			   "ConnectorPhaseRotation", .str = "0"),
	FILL_KEY_TABLE(KEY_TYPE_INT,	0, CFG_CONN_PHASE_ROT_MAXLEN,
			   "ConnectorPhaseRotationMaxLength", 1),
	FILL_KEY_TABLE(KEY_TYPE_BOOL,	1, CFG_STOP_TXN_ON_EVSIDE_DISCON,
			   "StopTransactionOnEVSideDisconnect", 0),
	FILL_KEY_TABLE(KEY_TYPE_BOOL,	1, CFG_STOP_TXN_ON_INVL_ID,
			   "StopTransactionOnInvalidId", 0),
	FILL_KEY_TABLE(KEY_TYPE_CSL,	1, CFG_STOP_TXN_ALIGNED_DATA,
			   "StopTxnAlignedData", .str = "0"),
	FILL_KEY_TABLE(KEY_TYPE_INT,	0, CFG_STOP_TXN_ALIGNED_DATA_MAXLEN,
			   "StopTxnAlignedDataMaxLength", 1),
	FILL_KEY_TABLE(KEY_TYPE_CSL,	0, CFG_SUPPORTED_FEATURE_PROFILE,
			   "SupportedFeatureProfiles",
			   .str = CP_CORE CP_SC CP_RT CP_RE CP_LAL CP_FM),
	FILL_KEY_TABLE(KEY_TYPE_INT,	0, CFG_SUPPORTED_FEATURE_PROFILE_MAXLEN,
			   "SupportedFeatureProfilesMaxLength", 6),
	FILL_KEY_TABLE(KEY_TYPE_INT,	1, CFG_TXN_MSG_ATTEMPTS,
			   "TransactionMessageAttempts", 0),
	FILL_KEY_TABLE(KEY_TYPE_INT,	1, CFG_TXN_MSG_RETRY_INTERVAL,
			   "TransactionMessageRetryInterval", 0),
	FILL_KEY_TABLE(KEY_TYPE_BOOL,	1, CFG_UNLOCK_CONN_ON_EVSIDE_DISCON,
			   "UnlockConnectorOnEVSideDisconnect", 0),
	FILL_KEY_TABLE(KEY_TYPE_INT,	1, CFG_WEBSOCK_PING_INTERVAL,
			   "WebSocketPingInterval", 0),

	/* optional */
};

enum ocpp_key_type ocpp_get_keyval_type(enum ocpp_key key)
{
	return cached_key_cfg[key].type;
}

enum ocpp_key ocpp_key_to_cfg(const char *skey)
{
	int i;
	struct ocpp_cfg_info *cfg = cached_key_cfg;

	for (i = 0; i < OCPP_CFG_END; i++) {
		if (strncmp(cfg[i].skey, skey, strlen(cfg[i].skey)) == 0) {
			break;
		}
	}

	return i;
}

bool ocpp_is_key_rw(enum ocpp_key key)
{
	return cached_key_cfg[key].is_rw;
}

union ocpp_keyval *ocpp_get_key_val(enum ocpp_key key)
{
	if (key >= OCPP_CFG_END) {
		return NULL;
	}

	return &cached_key_cfg[key].val;
}

char *ocpp_get_key_literal(enum ocpp_key key)
{
	if (key >= OCPP_CFG_END) {
		return NULL;
	}

	return cached_key_cfg[key].skey;
}

int ocpp_set_cfg_val(enum ocpp_key key, union ocpp_keyval *val)
{
	enum ocpp_key_type type;
	struct ocpp_cfg_info *key_cfg;

	key_cfg = &cached_key_cfg[key];
	type = ocpp_get_keyval_type(key);
	if (type < KEY_TYPE_STR) {
		key_cfg->val.ival = val->ival;
	} else {
		if (key_cfg->val.str) {
			free(key_cfg->val.str);
		}
		key_cfg->val.str = strdup(val->str);
	}

	return 0;
}

int ocpp_update_cfg_val(enum ocpp_key key, union ocpp_keyval *val)
{

	if (key >= OCPP_CFG_END ||
	    !cached_key_cfg[key].is_rw) {
		return -EINVAL;
	}

	return ocpp_set_cfg_val(key, val);
}
