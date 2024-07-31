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

	/** HF phone number calling request Callback
	 *
	 *  If this callback is provided it will be called whenever a
	 *  new call is requested with specific phone number from HFP unit.
	 *  When the callback is triggered, the application needs to start
	 *  dialing the number with the passed phone number.
	 *  If the callback is invalid, the phone number dialing from HFP unit
	 *  cannot be supported.
	 *
	 *  @param ag HFP AG object.
	 *  @param number Dialing number
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*number_call)(struct bt_hfp_ag *ag, const char *number);

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

	/** HF incoming call is held Callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  incoming call is held but not accepted.
	 *
	 *  @param ag HFP AG object.
	 */
	void (*incoming_held)(struct bt_hfp_ag *ag);

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

	/** Codec negotiate callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  codec negotiation succeeded or failed.
	 *
	 *  @param ag HFP AG object.
	 *  @param err Result of codec negotiation.
	 */
	void (*codec_negotiate)(struct bt_hfp_ag *ag, int err);

	/** Audio connection request callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  audio conenction request is triggered by HF.
	 *  When AT+BCC AT command received, it means the procedure of
	 *  establishment of audio connection is triggered by HF.
	 *  If the callback is provided by application, AG needs to
	 *  start the codec connection procedure by calling
	 *  function `bt_hfp_ag_audio_connect` in application layer.
	 *  Or, the codec conenction procedure will be started with
	 *  default codec id `BT_HFP_AG_CODEC_CVSD`.
	 *
	 *  @param ag HFP AG object.
	 *  @param err Result of codec negotiation.
	 */
	void (*audio_connect_req)(struct bt_hfp_ag *ag);

	/** HF VGM setting callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  VGM gain setting is informed from HF.
	 *
	 *  @param ag HFP AG object.
	 *  @param gain HF microphone gain value.
	 */
	void (*vgm)(struct bt_hfp_ag *ag, uint8_t gain);

	/** HF VGS setting callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  VGS gain setting is informed from HF.
	 *
	 *  @param ag HFP AG object.
	 *  @param gain HF speaker gain value.
	 */
	void (*vgs)(struct bt_hfp_ag *ag, uint8_t gain);

	/** HF ECNR turns off callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  ECNR turning off request is received from HF.
	 *  If the callback is NULL or @kconfig{CONFIG_BT_HFP_AG_ECNR}
	 *  is not enabled, the response result code of AT command
	 *  will be an AT ERROR.
	 *
	 *  @param ag HFP AG object.
	 */
	void (*ecnr_turn_off)(struct bt_hfp_ag *ag);
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

/** @brief Put the incoming call on hold
 *
 *  Put the incoming call on hold.
 *
 *  @param ag HFP AG object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_hold_incoming(struct bt_hfp_ag *ag);

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

/** @brief Set the HF microphone gain
 *
 *  Set the HF microphone gain
 *
 *  @param ag HFP AG object.
 *  @param vgm Microphone gain value.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_vgm(struct bt_hfp_ag *ag, uint8_t vgm);

/** @brief Set the HF speaker gain
 *
 *  Set the HF speaker gain
 *
 *  @param ag HFP AG object.
 *  @param vgs Speaker gain value.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_vgs(struct bt_hfp_ag *ag, uint8_t vgs);

/** @brief Set currently network operator
 *
 *  Set currently network operator.
 *
 *  @param ag HFP AG object.
 *  @param mode Current mode and provides no information with regard
 *              to the name of the operator.
 *  @param name A string in alphanumeric format representing the
 *                  name of the network operator. This string shall
 *                  not exceed 16 characters.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_set_operator(struct bt_hfp_ag *ag, uint8_t mode, char *name);

/** @brief Create audio connection
 *
 *  Create audio conenction by HFP AG. There are two setups included,
 *  Codec connection and audio connection.
 *  The codec connection will be established firstly if the codec
 *  negotiation are supported by both side. If the passed codec id
 *  is not same as the last codec connection, the codec connection
 *  procedure will be triggered.
 *  After the codec conenction is established, the audio conenction
 *  will be started.
 *  The passed codec id could be one of BT_HFP_AG_CODEC_XXX. If the
 *  codec negotiation feature is supported by both side, the codec id
 *  could be one of the bitmaps of `ids` notified by callback `codec`.
 *  Or, the `id` should be BT_HFP_AG_CODEC_CVSD.
 *
 *  @param ag HFP AG object.
 *  @param id Codec Id.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_audio_connect(struct bt_hfp_ag *ag, uint8_t id);

/** @brief Set In-Band Ring Tone
 *
 *  Set In-Band Ring Tone.
 *
 *  @param ag HFP AG object.
 *  @param inband In-band or no in-band.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_inband_ringtone(struct bt_hfp_ag *ag, bool inband);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_HFP_HF_H_ */
