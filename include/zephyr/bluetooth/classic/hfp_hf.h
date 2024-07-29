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

/* AT Commands */
enum bt_hfp_hf_at_cmd {
	BT_HFP_HF_ATA,
	BT_HFP_HF_AT_CHUP,
};

/*
 * Command complete types for the application
 */
#define HFP_HF_CMD_OK             0
#define HFP_HF_CMD_ERROR          1
#define HFP_HF_CMD_CME_ERROR      2
#define HFP_HF_CMD_UNKNOWN_ERROR  4

/** @brief HFP HF Command completion field */
struct bt_hfp_hf_cmd_complete {
	/* Command complete status */
	uint8_t type;
	/* CME error number to be added */
	uint8_t cme;
};

/* HFP CODEC IDs */
#define BT_HFP_HF_CODEC_CVSD    0x01
#define BT_HFP_HF_CODEC_MSBC    0x02
#define BT_HFP_HF_CODEC_LC3_SWB 0x03

/** @brief HFP profile application callback */
struct bt_hfp_hf_cb {
	/** HF connected callback to application
	 *
	 *  If this callback is provided it will be called whenever the
	 *  connection completes.
	 *
	 *  @param conn Connection object.
	 */
	void (*connected)(struct bt_conn *conn);
	/** HF disconnected callback to application
	 *
	 *  If this callback is provided it will be called whenever the
	 *  connection gets disconnected, including when a connection gets
	 *  rejected or cancelled or any error in SLC establishment.
	 *
	 *  @param conn Connection object.
	 */
	void (*disconnected)(struct bt_conn *conn);
	/** HF SCO/eSCO connected Callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  SCO/eSCO connection completes.
	 *
	 *  @param conn Connection object.
	 *  @param sco_conn SCO/eSCO Connection object.
	 */
	void (*sco_connected)(struct bt_conn *conn, struct bt_conn *sco_conn);
	/** HF SCO/eSCO disconnected Callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  SCO/eSCO connection gets disconnected.
	 *
	 *  @param conn Connection object.
	 *  @param reason BT_HCI_ERR_* reason for the disconnection.
	 */
	void (*sco_disconnected)(struct bt_conn *sco_conn, uint8_t reason);
	/** HF indicator Callback
	 *
	 *  This callback provides service indicator value to the application
	 *
	 *  @param conn Connection object.
	 *  @param value service indicator value received from the AG.
	 */
	void (*service)(struct bt_conn *conn, uint32_t value);
	/** HF indicator Callback
	 *
	 *  This callback provides call indicator value to the application
	 *
	 *  @param conn Connection object.
	 *  @param value call indicator value received from the AG.
	 */
	void (*call)(struct bt_conn *conn, uint32_t value);
	/** HF indicator Callback
	 *
	 *  This callback provides call setup indicator value to the application
	 *
	 *  @param conn Connection object.
	 *  @param value call setup indicator value received from the AG.
	 */
	void (*call_setup)(struct bt_conn *conn, uint32_t value);
	/** HF indicator Callback
	 *
	 *  This callback provides call held indicator value to the application
	 *
	 *  @param conn Connection object.
	 *  @param value call held indicator value received from the AG.
	 */
	void (*call_held)(struct bt_conn *conn, uint32_t value);
	/** HF indicator Callback
	 *
	 *  This callback provides signal indicator value to the application
	 *
	 *  @param conn Connection object.
	 *  @param value signal indicator value received from the AG.
	 */
	void (*signal)(struct bt_conn *conn, uint32_t value);
	/** HF indicator Callback
	 *
	 *  This callback provides roaming indicator value to the application
	 *
	 *  @param conn Connection object.
	 *  @param value roaming indicator value received from the AG.
	 */
	void (*roam)(struct bt_conn *conn, uint32_t value);
	/** HF indicator Callback
	 *
	 *  This callback battery service indicator value to the application
	 *
	 *  @param conn Connection object.
	 *  @param value battery indicator value received from the AG.
	 */
	void (*battery)(struct bt_conn *conn, uint32_t value);
	/** HF incoming call Ring indication callback to application
	 *
	 *  If this callback is provided it will be called whenever there
	 *  is an incoming call.
	 *
	 *  @param conn Connection object.
	 */
	void (*ring_indication)(struct bt_conn *conn);
	/** HF notify command completed callback to application
	 *
	 *  The command sent from the application is notified about its status
	 *
	 *  @param conn Connection object.
	 *  @param cmd structure contains status of the command including cme.
	 */
	void (*cmd_complete_cb)(struct bt_conn *conn,
			      struct bt_hfp_hf_cmd_complete *cmd);
	/** HF calling line identification notification callback to application
	 *
	 *  If this callback is provided it will be called whenever there
	 *  is a unsolicited result code +CLIP.
	 *  If @kconfig{CONFIG_BT_HFP_HF_CLI} is not enabled, the unsolicited
	 *  result code +CLIP will be ignored. And the callback will not be
	 *  notified.
	 *
	 *  @param conn Connection object.
	 *  @param number Notified phone number.
	 *  @param type Specify the format of the phone number.
	 */
	void (*clip)(struct bt_conn *conn, char *number, uint8_t type);
	/** HF microphone gain notification callback to application
	 *
	 *  If this callback is provided it will be called whenever there
	 *  is a unsolicited result code +VGM.
	 *  If @kconfig{CONFIG_BT_HFP_HF_VOLUME} is not enabled, the unsolicited
	 *  result code +VGM will be ignored. And the callback will not be
	 *  notified.
	 *
	 *  @param conn Connection object.
	 *  @param gain Microphone gain.
	 */
	void (*vgm)(struct bt_conn *conn, uint8_t gain);
	/** HF speaker gain notification callback to application
	 *
	 *  If this callback is provided it will be called whenever there
	 *  is a unsolicited result code +VGS.
	 *  If @kconfig{CONFIG_BT_HFP_HF_VOLUME} is not enabled, the unsolicited
	 *  result code +VGS will be ignored. And the callback will not be
	 *  notified.
	 *
	 *  @param conn Connection object.
	 *  @param gain Speaker gain.
	 */
	void (*vgs)(struct bt_conn *conn, uint8_t gain);
	/** HF in-band ring tone notification callback to application
	 *
	 *  If this callback is provided it will be called whenever there
	 *  is a unsolicited result code +BSIR issued by the AG to
	 *  indicate to the HF that the in-band ring tone setting
	 *  has been locally changed.
	 *
	 *  @param conn Connection object.
	 *  @param inband In-band ring tone status from the AG.
	 */
	void (*inband_ring)(struct bt_conn *conn, bool inband);
	/** HF network operator notification callback to application
	 *
	 *  If this callback is provided it will be called whenever there
	 *  is a response code +COPS issued by the AG to
	 *  response the AT+COPS? command issued by the HF by calling
	 *  function `bt_hfp_hf_get_operator`.
	 *
	 *  @param conn Connection object.
	 *  @param mode Current mode.
	 *  @param format Format of the `operator` parameter string.
	 *                It should be zero.
	 *  @param operator A string in alphanumeric format
	 *                  representing the name of the network
	 *                  operator.
	 */
	void (*operator)(struct bt_conn *conn, uint8_t mode, uint8_t format, char *operator);
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
	 *  @param conn Connection object.
	 *  @param id Negotiated Codec ID.
	 */
	void (*codec_negotiate)(struct bt_conn *conn, uint8_t id);
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

/** @brief Handsfree client Send AT
 *
 *  Send specific AT commands to handsfree client profile.
 *
 *  @param conn Connection object.
 *  @param cmd AT command to be sent.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_send_cmd(struct bt_conn *conn, enum bt_hfp_hf_at_cmd cmd);

/** @brief Handsfree HF enable/disable Calling Line Identification (CLI) Notification
 *
 *  Enable/disable Calling Line Identification (CLI) Notification.
 *  The AT command `AT+CLIP` will be sent to the AG to enable/disable the CLI
 *  unsolicited result code +CLIP when calling the function.
 *  If @kconfig{CONFIG_BT_HFP_HF_CLI} is not enabled, the error `-ENOTSUP` will
 *  be returned if the function called.
 *
 *  @param conn Connection object.
 *  @param enable Enable/disable CLI.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_cli(struct bt_conn *conn, bool enable);

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
 *  @param conn Connection object.
 *  @param gain Gain of microphone.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_vgm(struct bt_conn *conn, uint8_t gain);

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
 *  @param conn Connection object.
 *  @param gain Gain of speaker.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_vgs(struct bt_conn *conn, uint8_t gain);

/** @brief Handsfree HF requests currently selected operator
 *
 *  Send the AT+COPS? (Read) command to find the currently
 *  selected operator.
 *
 *  @param conn Connection object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_get_operator(struct bt_conn *conn);

/** @brief Handsfree HF accept the incoming call
 *
 *  Send the ATA command to accept the incoming call.
 *
 *  @param conn Connection object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_accept(struct bt_conn *conn);

/** @brief Handsfree HF reject the incoming call
 *
 *  Send the AT+CHUP command to reject the incoming call.
 *
 *  @param conn Connection object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_reject(struct bt_conn *conn);

/** @brief Handsfree HF setup audio connection
 *
 *  Setup audio conenction by sending AT+BCC.
 *  If @kconfig{CONFIG_BT_HFP_HF_CODEC_NEG} is not enabled, the error
 *  `-ENOTSUP` will be returned if the function called.
 *
 *  @param conn Connection object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_audio_connect(struct bt_conn *conn);

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
 *  @param conn Connection object.
 *  @param codec_id Selected codec id.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_select_codec(struct bt_conn *conn, uint8_t codec_id);

/** @brief Handsfree HF set supported codec ids
 *
 *  Set supported codec ids by sending AT+BAC. This function is used
 *  to notify AG the supported Codec IDs of HF.
 *  If @kconfig{CONFIG_BT_HFP_HF_CODEC_NEG} is not enabled, the error
 *  `-ENOTSUP` will be returned if the function called.
 *
 *  @param conn Connection object.
 *  @param codec_ids Supported codec IDs.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_set_codecs(struct bt_conn *conn, uint8_t codec_ids);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_HFP_HF_H_ */
