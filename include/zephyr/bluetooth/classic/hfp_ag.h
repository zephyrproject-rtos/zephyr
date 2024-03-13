/** @file
 *  @brief Handsfree Profile Audio Gateway handling.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_HFP_AG_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_HFP_AG_H_

/**
 * @brief Hands Free Profile - Audio Gateway (HFP-AG)
 * @defgroup bt_hfp_ag Hands Free Profile - Audio Gateway (HFP-AG)
 * @ingroup bluetooth
 * @{
 */

#include <zephyr/bluetooth/bluetooth.h>

#ifdef __cplusplus
extern "C" {
#endif

/* HFP AG Indicators */
enum bt_hfp_ag_indicator {
	BT_HFP_AG_SERVICE_IND = 0,    /* Service availability indicator */
	BT_HFP_AG_CALL_IND = 1,       /* call status indicator */
	BT_HFP_AG_CALL_SETUP_IND = 2, /* Call set up status indicator */
	BT_HFP_AG_CALL_HELD_IND = 3,  /* Call hold status indicator */
	BT_HFP_AG_SIGNAL_IND = 4,    /* Signal strength indicator */
	BT_HFP_AG_ROAM_IND = 5,       /* Roaming status indicator */
	BT_HFP_AG_BATTERY_IND = 6,    /* Battery change indicator */
	BT_HFP_AG_IND_MAX             /* Indicator MAX value */
};

/* HFP CODEC */
#define BT_HFP_AG_CODEC_CVSD    0x01
#define BT_HFP_AG_CODEC_MSBC    0x02
#define BT_HFP_AG_CODEC_LC3_SWB 0x03

struct bt_hfp_ag;

/** @brief HFP profile AG application callback */
struct bt_hfp_ag_cb {
	/** HF AG connected callback to application
	 *
	 *  If this callback is provided it will be called whenever the
	 *  AG connection completes.
	 *
	 *  @param ag HFP AG object.
	 */
	void (*connected)(struct bt_hfp_ag *ag);
	/** HF disconnected callback to application
	 *
	 *  If this callback is provided it will be called whenever the
	 *  connection gets disconnected, including when a connection gets
	 *  rejected or cancelled or any error in SLC establishment.
	 *
	 *  @param ag HFP AG object.
	 */
	void (*disconnected)(struct bt_hfp_ag *ag);
	/** HF SCO/eSCO connected Callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  SCO/eSCO connection completes.
	 *
	 *  @param ag HFP AG object.
	 *  @param sco_conn SCO/eSCO Connection object.
	 */
	void (*sco_connected)(struct bt_hfp_ag *ag, struct bt_conn *sco_conn);
	/** HF SCO/eSCO disconnected Callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  SCO/eSCO connection gets disconnected.
	 *
	 *  @param ag HFP AG object.
	 *  @param sco_conn SCO/eSCO Connection object.
	 */
	void (*sco_disconnected)(struct bt_hfp_ag *ag);

	/** HF memory dialing request Callback
	 *
	 *  If this callback is provided it will be called whenever a
	 *  new call is requested with memory dialing from HFP unit.
	 *  Get the phone number according to the given AG memory location.
	 *
	 *  @param ag HFP AG object.
	 *  @param location AG memory location
	 *  @param number Dailing number
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*memory_dial)(struct bt_hfp_ag *ag, const char *location, char **number);

	/** HF outgoing Callback
	 *
	 *  If this callback is provided it will be called whenever a
	 *  new call is outgoing.
	 *
	 *  @param ag HFP AG object.
	 *  @param number Dailing number
	 */
	void (*outgoing)(struct bt_hfp_ag *ag, const char *number);

	/** HF incoming Callback
	 *
	 *  If this callback is provided it will be called whenever a
	 *  new call is incoming.
	 *
	 *  @param ag HFP AG object.
	 *  @param number Incoming number
	 */
	void (*incoming)(struct bt_hfp_ag *ag, const char *number);

	/** HF ringing Callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  call is in the ringing
	 *
	 *  @param ag HFP AG object.
	 *  @param in_bond true - in-bond ringing, false - No in-bond ringing
	 */
	void (*ringing)(struct bt_hfp_ag *ag, bool in_band);

	/** HF call accept Callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  call is accepted.
	 *
	 *  @param ag HFP AG object.
	 */
	void (*accept)(struct bt_hfp_ag *ag);

	/** HF call reject Callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  call is rejected.
	 *
	 *  @param ag HFP AG object.
	 */
	void (*reject)(struct bt_hfp_ag *ag);

	/** HF call terminate Callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  call is terminated.
	 *
	 *  @param ag HFP AG object.
	 */
	void (*terminate)(struct bt_hfp_ag *ag);

	/** Supported codec Ids callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  supported codec ids are updated.
	 *
	 *  @param ag HFP AG object.
	 */
	void (*codec)(struct bt_hfp_ag *ag, uint32_t ids);
};

/** @brief Register HFP AG profile
 *
 *  Register Handsfree profile AG callbacks to monitor the state and get the
 *  required HFP details to display.
 *
 *  @param cb callback structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_register(struct bt_hfp_ag_cb *cb);

/** @brief Create the hfp ag session
 *
 *  Create the hfp ag session
 *
 *  @param conn ACL connection object.
 *  @param ag Created HFP AG object.
 *  @param channel Peer rfcomm channel to be connected.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_connect(struct bt_conn *conn, struct bt_hfp_ag **ag, uint8_t channel);

/** @brief Disconnect the hfp ag session
 *
 *  Disconnect the hfp ag session
 *
 *  @param ag HFP AG object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_disconnect(struct bt_hfp_ag *ag);

/** @brief Notify HFP Unit of an incoming call
 *
 *  Notify HFP Unit of an incoming call.
 *
 *  @param ag HFP AG object.
 *  @param number Dailing number.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_remote_incoming(struct bt_hfp_ag *ag, const char *number);

/** @brief Reject the incoming call
 *
 *  Reject the incoming call.
 *
 *  @param ag HFP AG object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_reject(struct bt_hfp_ag *ag);

/** @brief Accept the incoming call
 *
 *  Accept the incoming call.
 *
 *  @param ag HFP AG object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_accept(struct bt_hfp_ag *ag);

/** @brief Terminate the active/hold call
 *
 *  Terminate the active/hold call.
 *
 *  @param ag HFP AG object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_terminate(struct bt_hfp_ag *ag);

/** @brief Dial a call
 *
 *  Dial a call.
 *
 *  @param ag HFP AG object.
 *  @param number Dailing number.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_outgoing(struct bt_hfp_ag *ag, const char *number);

/** @brief Notify HFP Unit that the remote starts ringing
 *
 *  Notify HFP Unit that the remote starts ringing.
 *
 *  @param ag HFP AG object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_remote_ringing(struct bt_hfp_ag *ag);

/** @brief Notify HFP Unit that the remote rejects the call
 *
 *  Notify HFP Unit that the remote rejects the call.
 *
 *  @param ag HFP AG object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_remote_reject(struct bt_hfp_ag *ag);

/** @brief Notify HFP Unit that the remote accepts the call
 *
 *  Notify HFP Unit that the remote accepts the call.
 *
 *  @param ag HFP AG object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_remote_accept(struct bt_hfp_ag *ag);

/** @brief Notify HFP Unit that the remote terminates the active/hold call
 *
 *  Notify HFP Unit that the remote terminates the active/hold call.
 *
 *  @param ag HFP AG object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_remote_terminate(struct bt_hfp_ag *ag);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_HFP_HF_H_ */
