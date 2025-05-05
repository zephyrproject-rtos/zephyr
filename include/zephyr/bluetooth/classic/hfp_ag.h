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
struct bt_hfp_ag_call;

/** @typedef bt_hfp_ag_query_subscriber_func_t
 *  @brief Query subscriber number callback function
 *
 *  When AG wants to send subscriber number information, all information
 *  will be passed through the callback. And the subscriber number
 *  information will be sent out in this function.
 *
 *  @param ag HFP AG object.
 *  @param number Subscriber number.
 *  @param type Type of subscriber number specifies the format of the phone number provided,
 *              and can be one of the following values:
 *              - values 128-143: The phone number format may be a national or international
 *                format, and may contain prefix and/or escape digits. No changes on the number
 *                presentation are required.
 *              - values 144-159: The phone number format is an international number, including
 *                the country code prefix. If the plus sign ("+") is not included as part of the
 *                number and shall be added by the AG as needed.
 *              - values 160-175: National number. No prefix nor escape digits included.
 *  @param service Service of subscriber number indicates which service this phone number relates
 *                 to. Shall be either 4 (voice) or 5 (fax).
 *
 *  @return 0 if should continue to the next subscriber number information.
 *  @return negative value to stop.
 */
typedef int (*bt_hfp_ag_query_subscriber_func_t)(struct bt_hfp_ag *ag, char *number, uint8_t type,
						 uint8_t service);

/* HF indicators */
enum hfp_ag_hf_indicators {
	HFP_AG_ENHANCED_SAFETY_IND = 1, /* Enhanced Safety */
	HFP_AG_BATTERY_LEVEL_IND = 2,   /* Remaining level of Battery */
};

/** @brief HFP profile AG application callback */
struct bt_hfp_ag_cb {
	/** HF AG connected callback to application
	 *
	 *  If this callback is provided it will be called whenever the
	 *  AG connection completes.
	 *
	 *  @param conn Connection object.
	 *  @param ag HFP AG object.
	 */
	void (*connected)(struct bt_conn *conn, struct bt_hfp_ag *ag);
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
	 *  @param conn SCO/eSCO Connection object.
	 *  @param reason BT_HCI_ERR_* reason for the disconnection.
	 */
	void (*sco_disconnected)(struct bt_conn *sco_conn, uint8_t reason);

	/** HF memory dialing request Callback
	 *
	 *  If this callback is provided it will be called whenever a
	 *  new call is requested with memory dialing from HF.
	 *  Get the phone number according to the given AG memory location.
	 *
	 *  @param ag HFP AG object.
	 *  @param location AG memory location
	 *  @param number Dialing number
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*memory_dial)(struct bt_hfp_ag *ag, const char *location, char **number);

	/** HF phone number calling request Callback
	 *
	 *  If this callback is provided it will be called whenever a
	 *  new call is requested with specific phone number from HF.
	 *  When the callback is triggered, the application needs to start
	 *  dialing the number with the passed phone number.
	 *  If the callback is invalid, the phone number dialing from HF
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
	 *  @param call HFP AG call object.
	 *  @param number Dialing number
	 */
	void (*outgoing)(struct bt_hfp_ag *ag, struct bt_hfp_ag_call *call, const char *number);

	/** HF incoming Callback
	 *
	 *  If this callback is provided it will be called whenever a
	 *  new call is incoming.
	 *
	 *  @param ag HFP AG object.
	 *  @param call HFP AG call object.
	 *  @param number Incoming number
	 */
	void (*incoming)(struct bt_hfp_ag *ag, struct bt_hfp_ag_call *call, const char *number);

	/** HF incoming call is held Callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  incoming call is held but not accepted.
	 *
	 *  @param call HFP AG call object.
	 */
	void (*incoming_held)(struct bt_hfp_ag_call *call);

	/** HF ringing Callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  call is in the ringing
	 *
	 *  @param call HFP AG call object.
	 *  @param in_bond true - in-bond ringing, false - No in-bond ringing
	 */
	void (*ringing)(struct bt_hfp_ag_call *call, bool in_band);

	/** HF call accept Callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  call is accepted.
	 *
	 *  @param call HFP AG call object.
	 */
	void (*accept)(struct bt_hfp_ag_call *call);

	/** HF call held Callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  call is held.
	 *
	 *  @param call HFP AG call object.
	 */
	void (*held)(struct bt_hfp_ag_call *call);

	/** HF call retrieve Callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  call is retrieved.
	 *
	 *  @param call HFP AG call object.
	 */
	void (*retrieve)(struct bt_hfp_ag_call *call);

	/** HF call reject Callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  call is rejected.
	 *
	 *  @param call HFP AG call object.
	 */
	void (*reject)(struct bt_hfp_ag_call *call);

	/** HF call terminate Callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  call is terminated.
	 *
	 *  @param call HFP AG call object.
	 */
	void (*terminate)(struct bt_hfp_ag_call *call);

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

	/** HF explicit call transfer callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  AT+CHLD=4 is sent from HF.
	 *  When the callback is notified, the application should connect
	 *  the two calls and disconnects the subscriber from both calls
	 *  (Explicit Call Transfer).
	 *  After the callback returned, the call objects will be invalid.
	 *  If the callback is NULL, the response result code of AT command
	 *  will be an AT ERROR.
	 *  If @kconfig{CONFIG_BT_HFP_AG_3WAY_CALL} is not enabled, the
	 *  callback will not be notified.
	 *
	 *  @param ag HFP AG object.
	 */
	void (*explicit_call_transfer)(struct bt_hfp_ag *ag);

	/** Voice recognition activation/deactivation callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  voice recognition activation is changed.
	 *  If voice recognition is activated, the upper layer should
	 *  call `bt_hfp_ag_audio_connect` with appropriate codec ID to
	 *  setup audio connection.
	 *  If the callback is not provided by upper layer, the function
	 *  `bt_hfp_ag_audio_connect` will be called with default codec
	 *  ID `BT_HFP_AG_CODEC_CVSD`.
	 *  If @kconfig{CONFIG_BT_HFP_AG_VOICE_RECG} is not enabled,
	 *  the callback will not be notified.
	 *
	 *  @param ag HFP AG object.
	 *  @param activate Voice recognition activation/deactivation.
	 */
	void (*voice_recognition)(struct bt_hfp_ag *ag, bool activate);

	/** Ready to accept audio callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  HF is ready to accept audio.
	 *  If the feature `Enhanced Voice Recognition Status` is supported
	 *  by HF, the callback will be notified if the AT command `AT+BVRA=2`
	 *  is received. The HF may send this value during an ongoing VR
	 *  (Voice Recognition) session to terminate audio output from the
	 *  AG (if there is any) and prepare the AG for new audio input.
	 *  Or, the callback will be notified after the voice recognition
	 *  is activated.
	 *  If @kconfig{CONFIG_BT_HFP_AG_ENH_VOICE_RECG} is not enabled,
	 *  the callback will not be notified.
	 *
	 *  @param ag HFP AG object.
	 */
	void (*ready_to_accept_audio)(struct bt_hfp_ag *ag);

	/** Request phone number callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  AT command `AT+BINP=1` is received.
	 *  If the upper layer accepts the request, it shall obtain a
	 *  phone number.
	 *  If the upper layer rejects the request, it shall return a
	 *  an error.
	 *  If @kconfig{CONFIG_BT_HFP_AG_VOICE_TAG} is not enabled,
	 *  the callback will not be notified.
	 *
	 *  @param ag HFP AG object.
	 *  @param number Phone number of voice tag.
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*request_phone_number)(struct bt_hfp_ag *ag, char **number);

	/** Transmit a DTMF Code callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  AT command `AT+VTS=<code>` is received.
	 *  During an ongoing call, the HF transmits the AT+VTS command
	 *  to instruct the AG to transmit a specific DTMF code to its
	 *  network connection.
	 *
	 *  @param ag HFP AG object.
	 *  @param code A specific DTMF code.
	 */
	void (*transmit_dtmf_code)(struct bt_hfp_ag *ag, char code);

	/** Get subscriber number callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  AT command `AT+CNUM` is received.
	 *
	 *  @param ag HFP AG object.
	 *  @param func Query subscriber number callback.
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*subscriber_number)(struct bt_hfp_ag *ag, bt_hfp_ag_query_subscriber_func_t func);

	/** HF indicator value callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  AT command `AT+BIEV` is received.
	 *  If @kconfig{CONFIG_BT_HFP_AG_HF_INDICATORS} is not enabled,
	 *  the callback will not be notified.
	 *
	 *  @param ag HFP AG object.
	 *  @param indicator HF indicator
	 *  @param value The value of specific indicator
	 */
	void (*hf_indicator_value)(struct bt_hfp_ag *ag, enum hfp_ag_hf_indicators indicator,
				   uint32_t value);
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
 *  @param number Dialing number.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_remote_incoming(struct bt_hfp_ag *ag, const char *number);

/** @brief Put the incoming call on hold
 *
 *  Put the incoming call on hold.
 *
 *  @param call HFP AG call object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_hold_incoming(struct bt_hfp_ag_call *call);

/** @brief Reject the incoming call
 *
 *  Reject the incoming call.
 *
 *  @param call HFP AG call object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_reject(struct bt_hfp_ag_call *call);

/** @brief Accept the incoming call
 *
 *  Accept the incoming call.
 *
 *  @param call HFP AG call object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_accept(struct bt_hfp_ag_call *call);

/** @brief Terminate the active/hold call
 *
 *  Terminate the active/hold call.
 *
 *  @param call HFP AG call object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_terminate(struct bt_hfp_ag_call *call);

/** @brief Retrieve the held call
 *
 *  Retrieve the held call.
 *
 *  @param call HFP AG call object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_retrieve(struct bt_hfp_ag_call *call);

/** @brief Hold the active call
 *
 *  Hold the active call.
 *
 *  @param call HFP AG call object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_hold(struct bt_hfp_ag_call *call);

/** @brief Dial a call
 *
 *  Dial a call.
 *
 *  @param ag HFP AG object.
 *  @param number Dialing number.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_outgoing(struct bt_hfp_ag *ag, const char *number);

/** @brief Notify HFP Unit that the remote starts ringing
 *
 *  Notify HFP Unit that the remote starts ringing.
 *
 *  @param call HFP AG call object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_remote_ringing(struct bt_hfp_ag_call *call);

/** @brief Notify HFP Unit that the remote rejects the call
 *
 *  Notify HFP Unit that the remote rejects the call.
 *
 *  @param call HFP AG call object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_remote_reject(struct bt_hfp_ag_call *call);

/** @brief Notify HFP Unit that the remote accepts the call
 *
 *  Notify HFP Unit that the remote accepts the call.
 *
 *  @param call HFP AG call object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_remote_accept(struct bt_hfp_ag_call *call);

/** @brief Notify HFP Unit that the remote terminates the active/hold call
 *
 *  Notify HFP Unit that the remote terminates the active/hold call.
 *
 *  @param call HFP AG call object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_remote_terminate(struct bt_hfp_ag_call *call);

/** @brief explicit call transfer
 *
 *  Connects the two calls and disconnects the subscriber from
 *  both calls (Explicit Call Transfer).
 *  If @kconfig{CONFIG_BT_HFP_AG_3WAY_CALL} is not enabled, the error
 *  `-ENOTSUP` will be returned if the function called.
 *
 *  @param ag HFP AG object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_explicit_call_transfer(struct bt_hfp_ag *ag);

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

/** @brief Enable/disable the voice recognition function
 *
 *  Enables/disables the voice recognition function.
 *  If @kconfig{CONFIG_BT_HFP_AG_VOICE_RECG} is not enabled, the error
 *  `-ENOTSUP` will be returned if the function called.
 *
 *  @param ag HFP AG object.
 *  @param activate Activate/deactivate the voice recognition function.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_voice_recognition(struct bt_hfp_ag *ag, bool activate);

/** @brief set voice recognition engine state
 *
 *  It is used to set the voice recognition engine state.
 *  The unsolicited result code `+BVRA: 1,<vrecstate>` will be sent.
 *  `<vrecstate>`: Bitmask that reflects the current state of the voice
 *  recognition engine on the AG.
 *  Bit 0 - If it is 1, the AG is ready to accept audio input
 *  Bit 1 - If it is 1, the AG is sending audio to the HF
 *  Bit 2 - If it is 1, the AG is processing the audio input
 *  If @kconfig{CONFIG_BT_HFP_AG_ENH_VOICE_RECG} is not enabled, the error
 *  `-ENOTSUP` will be returned if the function called.
 *
 *  @param ag HFP AG object.
 *  @param state The value of `<vrecstate>`.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_vre_state(struct bt_hfp_ag *ag, uint8_t state);

/** @brief set voice recognition engine state and textual representation
 *
 *  It is used to set the voice recognition engine state with
 *  textual representation.
 *  unsolicited result code `+BVRA: 1,<vrecstate>,
 *  <textualRepresentation>` will be sent.
 *  `<vrecstate>` is same as parameter `state` of function
 *  `bt_hfp_ag_vre_state`.
 *  `<textualRepresentation>: <textID>,<textType>,<textOperation>,
 *  <string>`.
 *  `<textID>`: Unique ID of the current text as a hexadecimal string
 *  (a maximum of 4 characters in length, but less than 4 characters
 *  in length is valid).
 *  `<textType>`: ID of the textType from the following list:
 *  0 - Text recognized by the AG from the audio input provided by the HF
 *  1 - Text of the audio output from the AG
 *  2 - Text of the audio output from the AG that contains a question
 *  3 - Text of the audio output from the AG that contains an error
 *      description
 *  `<textOperation>`: ID of the operation of the text
 *  1 - NewText: Indicates that a new text started. Shall be used when the
 *               `<textID>` changes
 *  2 - Replace: Replace any existing text with the same `<textID>` and
 *               same `<textType>`
 *  3 - Append: Attach new text to existing text and keep the same
 *              `<textID>` and same `<textType>`
 *  `<string>`: The `<string>` parameter shall be a UTF-8 text string and
 *  shall always be contained within double quotes.
 *  If @kconfig{CONFIG_BT_HFP_AG_VOICE_RECG_TEXT} is not enabled, the error
 *  `-ENOTSUP` will be returned if the function called.
 *
 *  @param ag HFP AG object.
 *  @param state The value of `<vrecstate>`.
 *  @param id Value of `<textID>`.
 *  @param type Value of `<textType>`.
 *  @param operation Value of `<textOperation>`.
 *  @param text Value of `<string>`.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_vre_textual_representation(struct bt_hfp_ag *ag, uint8_t state, const char *id,
					 uint8_t type, uint8_t operation, const char *text);

/** @brief Set signal strength
 *
 *  Set signal strength.
 *
 *  @param ag HFP AG object.
 *  @param strength Signal strength.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_signal_strength(struct bt_hfp_ag *ag, uint8_t strength);

/** @brief Set roaming status
 *
 *  Set roaming status.
 *
 *  @param ag HFP AG object.
 *  @param status Roaming status.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_roaming_status(struct bt_hfp_ag *ag, uint8_t status);

/** @brief Set battery level
 *
 *  Set battery level.
 *
 *  @param ag HFP AG object.
 *  @param level battery level.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_battery_level(struct bt_hfp_ag *ag, uint8_t level);

/** @brief Set service availability
 *
 *  Set service availability.
 *
 *  @param ag HFP AG object.
 *  @param available service availability
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_service_availability(struct bt_hfp_ag *ag, bool available);

/** @brief Activate/deactivate HF indicator
 *
 *  It allows HF to issue the +BIND unsolicited result code to
 *  activate/deactivate of the AG’s supported HF Indicators.
 *  The indicator of supported indicators can be activated/deactivated
 *  are defined in `enum hfp_ag_hf_indicators`.
 *  `BT_HFP_AG_HF_INDICATOR_ENH_SAFETY` is used to support
 *  `Enhanced Safety`. Only the configuration has been enabled, the
 *  `indicator` can be HFP_AG_ENHANCED_SAFETY_IND.
 *  `BT_HFP_AG_HF_INDICATOR_BATTERY` is used to support
 *  `Remaining level of Battery`. Only the configuration has been
 *  enabled, the `indicator` can be HFP_AG_BATTERY_LEVEL_IND.
 *  If @kconfig{CONFIG_BT_HFP_HF_HF_INDICATORS} is not enabled, the error
 *  `-ENOTSUP` will be returned if the function called.
 *
 *  @param ag HFP AG object.
 *  @param indicator The indicator of the AG’s supported HF Indicators.
 *  @param enable enable/disable specific HF Indicator.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_ag_hf_indicator(struct bt_hfp_ag *ag, enum hfp_ag_hf_indicators indicator, bool enable);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_HFP_HF_H_ */
