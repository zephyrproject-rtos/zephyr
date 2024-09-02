/** @file
 *  @brief Handsfree Profile handling.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_HFP_HF_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_HFP_HF_H_

/**
 * @brief Hands Free Profile (HFP)
 * @defgroup bt_hfp Hands Free Profile (HFP)
 * @ingroup bluetooth
 * @{
 */

#include <zephyr/bluetooth/bluetooth.h>

#ifdef __cplusplus
extern "C" {
#endif

/* HFP CODEC IDs */
#define BT_HFP_HF_CODEC_CVSD    0x01
#define BT_HFP_HF_CODEC_MSBC    0x02
#define BT_HFP_HF_CODEC_LC3_SWB 0x03

struct bt_hfp_hf;

struct bt_hfp_hf_call;

/** @brief HFP profile application callback */
struct bt_hfp_hf_cb {
	/** HF connected callback to application
	 *
	 *  If this callback is provided it will be called whenever the
	 *  connection completes.
	 *
	 *  @param conn Connection object.
	 *  @param hf HFP HF object.
	 */
	void (*connected)(struct bt_conn *conn, struct bt_hfp_hf *hf);
	/** HF disconnected callback to application
	 *
	 *  If this callback is provided it will be called whenever the
	 *  connection gets disconnected, including when a connection gets
	 *  rejected or cancelled or any error in SLC establishment.
	 *  And the HFP HF object will be freed after the registered
	 *  callback `disconnected` returned.
	 *
	 *  @param hf HFP HF object.
	 */
	void (*disconnected)(struct bt_hfp_hf *hf);
	/** HF SCO/eSCO connected Callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  SCO/eSCO connection completes.
	 *
	 *  @param hf HFP HF object.
	 *  @param sco_conn SCO/eSCO Connection object.
	 */
	void (*sco_connected)(struct bt_hfp_hf *hf, struct bt_conn *sco_conn);
	/** HF SCO/eSCO disconnected Callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  SCO/eSCO connection gets disconnected.
	 *
	 *  @param conn SCO/eSCO Connection object.
	 *  @param reason BT_HCI_ERR_* reason for the disconnection.
	 */
	void (*sco_disconnected)(struct bt_conn *sco_conn, uint8_t reason);
	/** HF indicator Callback
	 *
	 *  This callback provides service indicator value to the application
	 *
	 *  @param hf HFP HF object.
	 *  @param value service indicator value received from the AG.
	 */
	void (*service)(struct bt_hfp_hf *hf, uint32_t value);
	/** HF call outgoing Callback
	 *
	 *  This callback provides the outgoing call status to
	 *  the application.
	 *
	 *  @param hf HFP HF object.
	 *  @param call HFP HF call object.
	 */
	void (*outgoing)(struct bt_hfp_hf *hf, struct bt_hfp_hf_call *call);
	/** HF call outgoing call is ringing Callback
	 *
	 *  This callback provides the outgoing call is ringing
	 *  status to the application.
	 *
	 *  @param call HFP HF call object.
	 */
	void (*remote_ringing)(struct bt_hfp_hf_call *call);
	/** HF call incoming Callback
	 *
	 *  This callback provides the incoming call status to
	 *  the application.
	 *
	 *  @param hf HFP HF object.
	 *  @param call HFP HF call object.
	 */
	void (*incoming)(struct bt_hfp_hf *hf, struct bt_hfp_hf_call *call);
	/** HF incoming call on hold Callback
	 *
	 *  This callback provides the incoming call on hold status to
	 *  the application.
	 *
	 *  @param call HFP HF call object.
	 */
	void (*incoming_held)(struct bt_hfp_hf_call *call);
	/** HF call accept Callback
	 *
	 *  This callback provides the incoming/outgoing call active
	 *  status to the application.
	 *
	 *  @param call HFP HF call object.
	 */
	void (*accept)(struct bt_hfp_hf_call *call);
	/** HF call reject Callback
	 *
	 *  This callback provides the incoming/outgoing call reject
	 *  status to the application.
	 *
	 *  @param call HFP HF call object.
	 */
	void (*reject)(struct bt_hfp_hf_call *call);
	/** HF call terminate Callback
	 *
	 *  This callback provides the incoming/outgoing call terminate
	 *  status to the application.
	 *
	 *  @param call HFP HF call object.
	 */
	void (*terminate)(struct bt_hfp_hf_call *call);
	/** HF call held Callback
	 *
	 *  This callback provides call held to the application
	 *
	 *  @param call HFP HF call object.
	 */
	void (*held)(struct bt_hfp_hf_call *call);
	/** HF call retrieve Callback
	 *
	 *  This callback provides call retrieved to the application
	 *
	 *  @param call HFP HF call object.
	 */
	void (*retrieve)(struct bt_hfp_hf_call *call);
	/** HF indicator Callback
	 *
	 *  This callback provides signal indicator value to the application
	 *
	 *  @param hf HFP HF object.
	 *  @param value signal indicator value received from the AG.
	 */
	void (*signal)(struct bt_hfp_hf *hf, uint32_t value);
	/** HF indicator Callback
	 *
	 *  This callback provides roaming indicator value to the application
	 *
	 *  @param hf HFP HF object.
	 *  @param value roaming indicator value received from the AG.
	 */
	void (*roam)(struct bt_hfp_hf *hf, uint32_t value);
	/** HF indicator Callback
	 *
	 *  This callback battery service indicator value to the application
	 *
	 *  @param hf HFP HF object.
	 *  @param value battery indicator value received from the AG.
	 */
	void (*battery)(struct bt_hfp_hf *hf, uint32_t value);
	/** HF incoming call Ring indication callback to application
	 *
	 *  If this callback is provided it will be called whenever there
	 *  is an incoming call.
	 *
	 *  @param call HFP HF call object.
	 */
	void (*ring_indication)(struct bt_hfp_hf_call *call);
	/** HF call dialing Callback
	 *
	 *  This callback provides call dialing result to the application.
	 *
	 *  @param hf HFP HF object.
	 *  @param err Result of calling dialing.
	 */
	void (*dialing)(struct bt_hfp_hf *hf, int err);
	/** HF calling line identification notification callback to application
	 *
	 *  If this callback is provided it will be called whenever there
	 *  is a unsolicited result code +CLIP.
	 *  If @kconfig{CONFIG_BT_HFP_HF_CLI} is not enabled, the unsolicited
	 *  result code +CLIP will be ignored. And the callback will not be
	 *  notified.
	 *
	 *  @param call HFP HF call object.
	 *  @param number Notified phone number.
	 *  @param type Specify the format of the phone number.
	 */
	void (*clip)(struct bt_hfp_hf_call *call, char *number, uint8_t type);
	/** HF microphone gain notification callback to application
	 *
	 *  If this callback is provided it will be called whenever there
	 *  is a unsolicited result code +VGM.
	 *  If @kconfig{CONFIG_BT_HFP_HF_VOLUME} is not enabled, the unsolicited
	 *  result code +VGM will be ignored. And the callback will not be
	 *  notified.
	 *
	 *  @param hf HFP HF object.
	 *  @param gain Microphone gain.
	 */
	void (*vgm)(struct bt_hfp_hf *hf, uint8_t gain);
	/** HF speaker gain notification callback to application
	 *
	 *  If this callback is provided it will be called whenever there
	 *  is a unsolicited result code +VGS.
	 *  If @kconfig{CONFIG_BT_HFP_HF_VOLUME} is not enabled, the unsolicited
	 *  result code +VGS will be ignored. And the callback will not be
	 *  notified.
	 *
	 *  @param hf HFP HF object.
	 *  @param gain Speaker gain.
	 */
	void (*vgs)(struct bt_hfp_hf *hf, uint8_t gain);
	/** HF in-band ring tone notification callback to application
	 *
	 *  If this callback is provided it will be called whenever there
	 *  is a unsolicited result code +BSIR issued by the AG to
	 *  indicate to the HF that the in-band ring tone setting
	 *  has been locally changed.
	 *
	 *  @param hf HFP HF object.
	 *  @param inband In-band ring tone status from the AG.
	 */
	void (*inband_ring)(struct bt_hfp_hf *hf, bool inband);
	/** HF network operator notification callback to application
	 *
	 *  If this callback is provided it will be called whenever there
	 *  is a response code +COPS issued by the AG to
	 *  response the AT+COPS? command issued by the HF by calling
	 *  function `bt_hfp_hf_get_operator`.
	 *
	 *  @param hf HFP HF object.
	 *  @param mode Current mode.
	 *  @param format Format of the `operator` parameter string.
	 *                It should be zero.
	 *  @param operator A string in alphanumeric format
	 *                  representing the name of the network
	 *                  operator.
	 */
	void (*operator)(struct bt_hfp_hf *hf, uint8_t mode, uint8_t format, char *operator);
	/** Codec negotiate callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  unsolicited codec negotiation response received.
	 *  There are two cases when the callback triggered,
	 *  Case 1, the codec id can be accepted, the function
	 *  `bt_hfp_hf_select_codec` should be called to accept the codec
	 *  id.
	 *  Case 2, the codec id can not be accepted, the function
	 *  `bt_hfp_hf_set_codecs` should be called to trigger codec ID
	 *  to be re-selected.
	 *  If the callback is not provided by application, the function
	 *  `bt_hfp_hf_select_codec` will be called to accept the codec
	 *  id.
	 *  Refers to BT_HFP_HF_CODEC_XXX for codec id value.
	 *  If @kconfig{CONFIG_BT_HFP_HF_CODEC_NEG} is not enabled, the
	 *  unsolicited result code +BCS will be ignored. And the callback
	 *  will not be notified.
	 *
	 *  @param hf HFP HF object.
	 *  @param id Negotiated Codec ID.
	 */
	void (*codec_negotiate)(struct bt_hfp_hf *hf, uint8_t id);
	/** HF ECNR turns off callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  response of ECNR turning off is received from AG.
	 *  If @kconfig{CONFIG_BT_HFP_HF_ECNR} is not enabled, the
	 *  callback will not be notified.
	 *
	 *  @param hf HFP HF object.
	 *  @param err The result of request.
	 */
	void (*ecnr_turn_off)(struct bt_hfp_hf *hf, int err);
	/** HF call waiting notification callback to application
	 *
	 *  If this callback is provided it will be called whenever there
	 *  is a unsolicited result code +CCWA.
	 *  This notification can be enabled/disabled by calling function
	 *  `bt_hfp_hf_call_waiting_notify`.
	 *  If @kconfig{CONFIG_BT_HFP_HF_3WAY_CALL} is not enabled, the
	 *  unsolicited result code +CCWA will be ignored. And the callback
	 *  will not be notified.
	 *
	 *  @param call HFP HF call object.
	 *  @param number Notified phone number.
	 *  @param type Specify the format of the phone number.
	 */
	void (*call_waiting)(struct bt_hfp_hf_call *call, char *number, uint8_t type);
	/** Voice recognition activation/deactivation callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  unsolicited result code +BVRA is notified the HF when the
	 *  voice recognition function in the AG is activated/deactivated
	 *  autonomously from the AG.
	 *  If @kconfig{CONFIG_BT_HFP_HF_VOICE_RECG} is not enabled, the
	 *  unsolicited result code +BVRA will be ignored. And the callback
	 *  will not be notified.
	 *
	 *  @param hf HFP HF object.
	 *  @param activate Voice recognition activation/deactivation.
	 */
	void (*voice_recognition)(struct bt_hfp_hf *hf, bool activate);
	/** Voice recognition engine state callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  unsolicited result code `+BVRA: 1,<vrecstate>` is received from AG.
	 *  `<vrecstate>`: Bitmask that reflects the current state of the voice
	 *  recognition engine on the AG.
	 *  Bit 0 - If it is 1, the AG is ready to accept audio input
	 *  Bit 1 - If it is 1, the AG is sending audio to the HF
	 *  Bit 2 - If it is 1, the AG is processing the audio input
	 *  If @kconfig{CONFIG_BT_HFP_HF_ENH_VOICE_RECG} is not enabled, the
	 *  unsolicited result code +BVRA will be ignored. And the callback
	 *  will not be notified.
	 *
	 *  @param hf HFP HF object.
	 *  @param state Value of `<vrecstate>`.
	 */
	void (*vre_state)(struct bt_hfp_hf *hf, uint8_t state);
	/** Textual representation callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  unsolicited result code `+BVRA: 1,<vrecstate>,
	 *  <textualRepresentation>` is received from AG.
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
	 *  If @kconfig{CONFIG_BT_HFP_HF_VOICE_RECG_TEXT} is not enabled, the
	 *  unsolicited result code +BVRA will be ignored. And the callback
	 *  will not be notified.
	 *
	 *  @param hf HFP HF object.
	 *  @param id Value of `<textID>`.
	 *  @param type Value of `<textType>`.
	 *  @param operation Value of `<textOperation>`.
	 *  @param text Value of `<string>`.
	 */
	void (*textual_representation)(struct bt_hfp_hf *hf, char *id, uint8_t type,
				       uint8_t operation, char *text);
	/** Request phone number callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  result code `+BINP: <Phone number>` is received from AG.
	 *  If the request is failed, the `number` will be NULL.
	 *
	 *  @param hf HFP HF object.
	 *  @param number Value of `<Phone number>`.
	 */
	void (*request_phone_number)(struct bt_hfp_hf *hf, const char *number);

	/** Query subscriber number callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  result code `+CUNM: [<alpha>],<number>, <type>,[<speed> ,<service>]`
	 *  is received from AG.
	 *  `<alpha>`: This optional field is not supported, and shall be left
	 *  blank.
	 *  `<number>`: Quoted string containing the phone number in the format
	 *  specified by `<type>`.
	 *  `<type>` field specifies the format of the phone number provided,
	 *  and can be one of the following values:
	 *  - values 128-143: The phone number format may be a national or
	 *  international format, and may contain prefix and/or escape digits.
	 *  No changes on the number presentation are required.
	 *  - values 144-159: The phone number format is an international
	 *  number, including the country code prefix. If the plus sign ("+")
	 *  is not included as part of the number and shall be added by the AG
	 *  as needed.
	 *  - values 160-175: National number. No prefix nor escape digits
	 *  included.
	 *  `<speed>`: This optional field is not supported, and shall be left
	 *  blank.
	 *  `<service>`: Indicates which service this phone number relates to.
	 *  Shall be either 4 (voice) or 5 (fax).
	 *
	 *  @param hf HFP HF object.
	 *  @param number Value of `<number>` without quotes.
	 *  @param type Value of `<type>`.
	 *  @param service Value of `<service>`.
	 */
	void (*subscriber_number)(struct bt_hfp_hf *hf, const char *number, uint8_t type,
				  uint8_t service);
};

/** @brief Register HFP HF profile
 *
 *  Register Handsfree profile callbacks to monitor the state and get the
 *  required HFP details to display.
 *
 *  @param cb callback structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_register(struct bt_hfp_hf_cb *cb);

/** @brief Initiate the service level connection establishment procedure
 *
 *  Initiate the service level connection establishment procedure on the
 *  ACL connection specified by the parameter `conn` using the specific
 *  RFCOMM channel discovered by the function `bt_br_discovery_start`.
 *
 *  The parameter `hf` is a output parameter. When the service level
 *  connection establishment procedure is initiated without any error,
 *  the HFP HF object is allocated and it will be returned via the parameter
 *  `hf` if the parameter `hf` is not a NULL pointer.
 *
 *  When service level conenction is established, the registered callback
 *  `connected` will be triggered to notify the application that the service
 *  level connection establishment procedure is done. And the HFP HF object
 *  is valid at this time. It means after the function is called without
 *  any error, all interfaces provided by HFP HF can only be called after
 *  the registered callback `connected` is triggered.
 *
 *  @param conn ACL connection object.
 *  @param hf Created HFP HF object.
 *  @param channel Peer RFCOMM channel to be connected.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_connect(struct bt_conn *conn, struct bt_hfp_hf **hf, uint8_t channel);

/** @brief Release the service level connection
 *
 *  Release the service level connection from the peer device.
 *
 *  The function can only be called after the registered callback `connected`
 *  is triggered.
 *
 *  If the function is called without any error, the HFP HF object is
 *  invalid at this time. All interfaces provided by HFP HF should not
 *  be called anymore.
 *
 *  If the service level connection is released, the registered callback
 *  `disconnected` will be triggered to notify the application that the
 *  service level connection release procedure is done. And the HFP HF
 *  object will be freed after the registered callback `disconnected`
 *  returned.
 *
 *  @param hf HFP HF object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_disconnect(struct bt_hfp_hf *hf);

/** @brief Handsfree HF enable/disable Calling Line Identification (CLI) Notification
 *
 *  Enable/disable Calling Line Identification (CLI) Notification.
 *  The AT command `AT+CLIP` will be sent to the AG to enable/disable the CLI
 *  unsolicited result code +CLIP when calling the function.
 *  If @kconfig{CONFIG_BT_HFP_HF_CLI} is not enabled, the error `-ENOTSUP` will
 *  be returned if the function called.
 *
 *  @param hf HFP HF object.
 *  @param enable Enable/disable CLI.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_cli(struct bt_hfp_hf *hf, bool enable);

/** @brief Handsfree HF report Gain of Microphone (VGM)
 *
 *  Report Gain of Microphone (VGM).
 *  The AT command `AT+VGM=<gain>` will be sent to the AG to report its
 *  current microphone gain level setting to the AG.
 *  `<gain>` is a decimal numeric constant, relating to a particular
 *  (implementation dependent) volume level controlled by the HF.
 *  This command does not change the microphone gain of the AG; it simply
 *  indicates the current value of the microphone gain in the HF.
 *  If @kconfig{CONFIG_BT_HFP_HF_VOLUME} is not enabled, the error `-ENOTSUP`
 *  will be returned if the function called.
 *  For "Volume Level Synchronization", the HF application could call
 *  the function to set VGM gain value in HF connection callback
 *  function. Then after the HF connection callback returned, VGM gain
 *  will be sent to HFP AG.
 *
 *  @param hf HFP HF object.
 *  @param gain Gain of microphone.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_vgm(struct bt_hfp_hf *hf, uint8_t gain);

/** @brief Handsfree HF report Gain of Speaker (VGS)
 *
 *  Report Gain of Speaker (VGS).
 *  The AT command `AT+VGS=<gain>` will be sent to the AG to report its
 *  current speaker gain level setting to the AG.
 *  `<gain>` is a decimal numeric constant, relating to a particular
 *  (implementation dependent) volume level controlled by the HF.
 *  This command does not change the speaker gain of the AG; it simply
 *  indicates the current value of the speaker gain in the HF.
 *  If @kconfig{CONFIG_BT_HFP_HF_VOLUME} is not enabled, the error `-ENOTSUP`
 *  will be returned if the function called.
 *  For "Volume Level Synchronization", the HF application could call
 *  the function to set VGS gain value in HF connection callback
 *  function. Then after the HF connection callback returned, VGS gain
 *  will be sent to HFP AG.
 *
 *  @param hf HFP HF object.
 *  @param gain Gain of speaker.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_vgs(struct bt_hfp_hf *hf, uint8_t gain);

/** @brief Handsfree HF requests currently selected operator
 *
 *  Send the AT+COPS? (Read) command to find the currently
 *  selected operator.
 *
 *  @param hf HFP HF object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_get_operator(struct bt_hfp_hf *hf);

/** @brief Handsfree HF accept the incoming call
 *
 *  Send the ATA command to accept the incoming call.
 *  OR, send the AT+BTRH=1 command to accept a held incoming
 *  call.
 *
 *  @note It cannot be used when multiple calls are ongoing.
 *
 *  @param call HFP HF call object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_accept(struct bt_hfp_hf_call *call);

/** @brief Handsfree HF reject the incoming call
 *
 *  Send the AT+CHUP command to reject the incoming call.
 *  OR, send the AT+BTRH=2 command to reject a held incoming
 *  call.
 *
 *  @note It cannot be used when multiple calls are ongoing.
 *
 *  @param call HFP HF call object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_reject(struct bt_hfp_hf_call *call);

/** @brief Handsfree HF terminate the incoming call
 *
 *  Send the AT+CHUP command to terminate the incoming call.
 *
 *  @note It cannot be used when multiple calls are ongoing.
 *
 *  @param call HFP HF call object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_terminate(struct bt_hfp_hf_call *call);

/** @brief Handsfree HF put the incoming call on hold
 *
 *  Send the AT+BTRH=0 command to put the incoming call on hold.
 *  If the incoming call has been held, the callback `on_hold` will
 *  be triggered.
 *
 *  @note It cannot be used when multiple calls are ongoing.
 *
 *  @param call HFP HF call object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_hold_incoming(struct bt_hfp_hf_call *call);

/** @brief Handsfree HF query respond and hold status of AG
 *
 *  Send the AT+BTRH? command to query respond and hold status of AG.
 *  The status respond and hold will be notified through callback
 *  `on_hold`.
 *
 *  @param hf HFP HF object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_query_respond_hold_status(struct bt_hfp_hf *hf);

/** @brief Handsfree HF phone number call
 *
 *  Initiate outgoing voice calls by providing the destination phone
 *  number to the AG.
 *  Send the ATDdd…dd command to start phone number call.
 *  The result of the command will be notified through the callback
 *  `dialing`.
 *
 *  @param hf HFP HF object.
 *  @param number Phone number.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_number_call(struct bt_hfp_hf *hf, const char *number);

/** @brief Handsfree HF memory dialing call
 *
 *  Initiate outgoing voice calls using the memory dialing feature
 *  of the AG.
 *  Send the ATD>Nan... command to start memory dialing.
 *  The result of the command will be notified through the callback
 *  `dialing`.
 *
 *  @param hf HFP HF object.
 *  @param location Memory location.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_memory_dial(struct bt_hfp_hf *hf, const char *location);

/** @brief Handsfree HF redial last number
 *
 *  Initiate outgoing voice calls by recalling the last number
 *  dialed by the AG.
 *  Send the AT+BLDN command to recall the last number.
 *  The result of the command will be notified through the callback
 *  `dialing`.
 *
 *  @param hf HFP HF object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_redial(struct bt_hfp_hf *hf);

/** @brief Handsfree HF setup audio connection
 *
 *  Setup audio conenction by sending AT+BCC.
 *  If @kconfig{CONFIG_BT_HFP_HF_CODEC_NEG} is not enabled, the error
 *  `-ENOTSUP` will be returned if the function called.
 *
 *  @param hf HFP HF object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_audio_connect(struct bt_hfp_hf *hf);

/** @brief Handsfree HF set selected codec id
 *
 *  Set selected codec id by sending AT+BCS. The function is used to
 *  response the codec negotiation request notified by callback
 *  `codec_negotiate`. The parameter `codec_id` should be same as
 *  `id` of callback `codec_negotiate` if the id could be supported.
 *  Or, call `bt_hfp_hf_set_codecs` to notify the AG Codec IDs supported
 *  by HFP HF.
 *  If @kconfig{CONFIG_BT_HFP_HF_CODEC_NEG} is not enabled, the error
 *  `-ENOTSUP` will be returned if the function called.
 *
 *  @param hf HFP HF object.
 *  @param codec_id Selected codec id.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_select_codec(struct bt_hfp_hf *hf, uint8_t codec_id);

/** @brief Handsfree HF set supported codec ids
 *
 *  Set supported codec ids by sending AT+BAC. This function is used
 *  to notify AG the supported Codec IDs of HF.
 *  If @kconfig{CONFIG_BT_HFP_HF_CODEC_NEG} is not enabled, the error
 *  `-ENOTSUP` will be returned if the function called.
 *
 *  @param hf HFP HF object.
 *  @param codec_ids Supported codec IDs.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_set_codecs(struct bt_hfp_hf *hf, uint8_t codec_ids);

/** @brief Handsfree HF turns off AG's EC and NR
 *
 *  Turn off the AG's EC and NR by sending `AT+NREC=0`.
 *  The result of the command is notified through the callback
 *  `ecnr_turn_off`.
 *  If @kconfig{CONFIG_BT_HFP_HF_ECNR} is not enabled, the error
 *  `-ENOTSUP` will be returned if the function called.
 *
 *  @param hf HFP HF object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_turn_off_ecnr(struct bt_hfp_hf *hf);

/** @brief Handsfree HF enable/disable call waiting notification
 *
 *  Enable call waiting notification by sending `AT+CCWA=1`.
 *  Disable call waiting notification by sending `AT+CCWA=0`.
 *  If @kconfig{CONFIG_BT_HFP_HF_3WAY_CALL} is not enabled, the error
 *  `-ENOTSUP` will be returned if the function called.
 *
 *  @param hf HFP HF object.
 *  @param enable Enable/disable.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_call_waiting_notify(struct bt_hfp_hf *hf, bool enable);

/** @brief Handsfree HF release all held calls
 *
 *  Release all held calls by sending `AT+CHLD=0`.
 *  If @kconfig{CONFIG_BT_HFP_HF_3WAY_CALL} is not enabled, the error
 *  `-ENOTSUP` will be returned if the function called.
 *
 *  @param hf HFP HF object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_release_all_held(struct bt_hfp_hf *hf);

/** @brief Handsfree HF set User Determined User Busy (UDUB) for a waiting call
 *
 *  Set User Determined User Busy (UDUB) for a waiting call
 *  by sending `AT+CHLD=0`.
 *  If @kconfig{CONFIG_BT_HFP_HF_3WAY_CALL} is not enabled, the error
 *  `-ENOTSUP` will be returned if the function called.
 *
 *  @param hf HFP HF object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_set_udub(struct bt_hfp_hf *hf);

/** @brief Handsfree HF release all active calls and accept other call
 *
 *  Release all active calls (if any exist) and accepts the other
 *  (held or waiting) call by sending `AT+CHLD=1`.
 *  If @kconfig{CONFIG_BT_HFP_HF_3WAY_CALL} is not enabled, the error
 *  `-ENOTSUP` will be returned if the function called.
 *
 *  @param hf HFP HF object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_release_active_accept_other(struct bt_hfp_hf *hf);

/** @brief Handsfree HF hold all active calls and accept other call
 *
 *  Hold all active calls (if any exist) and accepts the other
 *  (held or waiting) call by sending `AT+CHLD=2`.
 *  If @kconfig{CONFIG_BT_HFP_HF_3WAY_CALL} is not enabled, the error
 *  `-ENOTSUP` will be returned if the function called.
 *
 *  @param hf HFP HF object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_hold_active_accept_other(struct bt_hfp_hf *hf);

/** @brief Handsfree HF add a held call to the conversation
 *
 *  Add a held call to the conversation by sending `AT+CHLD=3`.
 *  If @kconfig{CONFIG_BT_HFP_HF_3WAY_CALL} is not enabled, the error
 *  `-ENOTSUP` will be returned if the function called.
 *
 *  @param hf HFP HF object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_join_conversation(struct bt_hfp_hf *hf);

/** @brief Handsfree HF explicit call transfer
 *
 *  Connects the two calls and disconnects the subscriber from
 *  both calls (Explicit Call Transfer) by sending `AT+CHLD=4`.
 *  If @kconfig{CONFIG_BT_HFP_HF_3WAY_CALL} is not enabled, the error
 *  `-ENOTSUP` will be returned if the function called.
 *
 *  @param hf HFP HF object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_explicit_call_transfer(struct bt_hfp_hf *hf);

/** @brief Handsfree HF release call with specified index
 *
 *  Release call with specified index by sending `AT+CHLD=1<idx>`.
 *  `<idx>` is index of specified call.
 *  If @kconfig{CONFIG_BT_HFP_HF_ECC} is not enabled, the error
 *  `-ENOTSUP` will be returned if the function called.
 *
 *  @param call HFP HF call object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_release_specified_call(struct bt_hfp_hf_call *call);

/** @brief Handsfree HF request private consultation mode with specified call
 *
 *  Request private consultation mode with specified call (Place all calls
 *  on hold EXCEPT the call indicated by `<idx>`.) by sending
 *  `AT+CHLD=2<idx>`.
 *  `<idx>` is index of specified call.
 *  If @kconfig{CONFIG_BT_HFP_HF_ECC} is not enabled, the error
 *  `-ENOTSUP` will be returned if the function called.
 *
 *  @param call HFP HF call object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_private_consultation_mode(struct bt_hfp_hf_call *call);

/** @brief Handsfree HF enable/disable the voice recognition function
 *
 *  Enables/disables the voice recognition function in the AG.
 *  If @kconfig{CONFIG_BT_HFP_HF_VOICE_RECG} is not enabled, the error
 *  `-ENOTSUP` will be returned if the function called.
 *
 *  @param hf HFP HF object.
 *  @param activate Activate/deactivate the voice recognition function.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_voice_recognition(struct bt_hfp_hf *hf, bool activate);

/** @brief Handsfree HF indicate that the HF is ready to accept audio
 *
 *  This value indicates that the HF is ready to accept audio when
 *  the Audio Connection is first established. The HF shall only send
 *  this value if the eSCO link has been established.
 *  If @kconfig{CONFIG_BT_HFP_HF_ENH_VOICE_RECG} is not enabled, the error
 *  `-ENOTSUP` will be returned if the function called.
 *
 *  @param hf HFP HF object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_ready_to_accept_audio(struct bt_hfp_hf *hf);

/** @brief Handsfree HF attach a phone number for a voice tag
 *
 *  Send AT command "AT+BINP=1" to request phone number to the AG.
 *
 *  @param hf HFP HF object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_request_phone_number(struct bt_hfp_hf *hf);

/** @brief Handsfree HF Transmit A specific DTMF Code
 *
 *  During an ongoing call, the HF transmits the AT+VTS command to
 *  instruct the AG to transmit a specific DTMF code to its network
 *  connection.
 *  The set of the code is "0-9,#,*,A-D".
 *
 *  @param call HFP HF call object.
 *  @param code A specific DTMF code.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_transmit_dtmf_code(struct bt_hfp_hf_call *call, char code);

/** @brief Handsfree HF Query Subscriber Number Information
 *
 *  It allows HF to query the AG subscriber number by sending `AT+CNUM`.
 *
 *  @param hf HFP HF object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_query_subscriber(struct bt_hfp_hf *hf);

/* HFP HF Indicators */
enum hfp_hf_ag_indicators {
	HF_SERVICE_IND = 0, /* AG service indicator */
	HF_CALL_IND,        /* AG call indicator */
	HF_CALL_SETUP_IND,  /* AG call setup indicator */
	HF_CALL_HELD_IND,   /* AG call held indicator */
	HF_SIGNAL_IND,      /* AG signal indicator */
	HF_ROAM_IND,        /* AG roaming indicator */
	HF_BATTERY_IND      /* AG battery indicator */
};

/** @brief Handsfree HF set AG indicator activated/deactivated status
 *
 *  It allows HF to issue the AT+BIA command if it needs to change the
 *  activated/deactivated status of indicators in the AG.
 *  The index of all indicators can be activated/deactivated are
 *  defined in `enum hfp_hf_ag_indicators`.
 *  The each bit of parameter `status` represents the indicator status
 *  corresponding to the index. Such as, value 0b111110 of `status`
 *  means the AG indicator `service` is required to be deactivated.
 *  Others are required to be activated.
 *
 *  @param hf HFP HF object.
 *  @param status The activated/deactivated bitmap status of AG indicators.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_indicator_status(struct bt_hfp_hf *hf, uint8_t status);

/** @brief Handsfree HF enable/disable enhanced safety
 *
 *  It allows HF to transfer of HF indicator enhanced safety value.
 *  If @kconfig{CONFIG_BT_HFP_HF_HF_INDICATOR_ENH_SAFETY} is not enabled,
 *  the error `-ENOTSUP` will be returned if the function called.
 *
 *  @param hf HFP HF object.
 *  @param enable The enhanced safety is enabled/disabled.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_enhanced_safety(struct bt_hfp_hf *hf, bool enable);

/** @brief Handsfree HF remaining battery level
 *
 *  It allows HF to transfer of HF indicator remaining battery level value.
 *  If @kconfig{CONFIG_BT_HFP_HF_HF_INDICATOR_BATTERY} is not enabled,
 *  the error `-ENOTSUP` will be returned if the function called.
 *
 *  @param hf HFP HF object.
 *  @param level The remaining battery level.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_battery(struct bt_hfp_hf *hf, uint8_t level);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_HFP_HF_H_ */
