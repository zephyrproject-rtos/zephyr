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

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_HFP_HF_H_ */
