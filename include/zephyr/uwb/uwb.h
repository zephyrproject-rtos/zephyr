/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __UWB_API_FIRA_H__
#define __UWB_API_FIRA_H__

#include "zephyr/uwb/uwb_core.h"
#include <zephyr/uwb/types.h>
#include "zephyr/uwb/uwb_types.h"

/**
 * @brief UWB APIs
 * @defgroup uwb_api
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Initialize vendor UWB stack
 *
 * This function is called from \fn uwb_api_initialize after initializing
 * TML, transport and UCI core layers.
 *
 * The implementation of this function is vendor specific.
 */
extern int uwb_vendor_initialize(void);

/**
 * \brief De-initialize vendor UWB stack
 *
 * This function is called from \fn uwb_api_deinitialize before de-initializing
 * TML, transport and UCI core layers.
 *
 * The implementation of this function is vendor specific.
 */
extern void uwb_vendor_deinitialize(void);

/**
 * \brief Initialize the UWB API
 *
 * This function initializes the UWB subsystem including UCI layer, TML layer,
 * and registers application callbacks for receiving notifications.
 *
 * \param[in] pApplicationCallback Pointer to application callback structure for UCI notifications
 *
 * \retval kUwb_StatusCode_Success Initialization successful
 * \retval kUwb_StatusCode_InvalidArgument Invalid arguments passed
 * \retval kUwb_StatusCode_Failed Initialization failed
 */
uwb_status_code_t uwb_api_initialize(uwb_uci_callback_t *pApplicationCallback);

/**
 * \brief Deinitialize the UWB API
 *
 * This function performs cleanup and deinitialization of the UWB subsystem.
 *
 * \retval kUwb_StatusCode_Success Deinitialization successful
 */
uwb_status_code_t uwb_api_deinitialize(void);

/**
 * \brief Reset the UWB device
 *
 * This function sends a device reset command and waits for the device status
 * notification to confirm the device is in ready state after reset.
 *
 * \param[in] resetConfig Reset configuration parameter
 *
 * \retval kUwb_StatusCode_Success Device reset successful and device is ready
 * \retval kUwb_StatusCode_Failed Reset failed or device not ready after reset
 */
uwb_status_code_t uwb_api_core_device_reset(const uint8_t resetConfig);

/**
 * \brief Get UWB device information
 *
 * This function retrieves device information including UCI version, MAC version,
 * PHY version, test version, vendor specific information, etc.
 *
 * \param[out] pdevInfo Pointer to structure to store device information
 *
 * \retval kUwb_StatusCode_Success Device info retrieved successfully
 * \retval kUwb_StatusCode_InvalidArgument Invalid arguments passed
 * \retval kUwb_StatusCode_Failed Failed to retrieve or parse device info
 */
uwb_status_code_t uwb_api_core_get_device_info(uwb_device_info_t *pdevInfo);

/**
 * \brief Get UWB device capabilities
 *
 * This function retrieves the capabilities supported by the UWB device.
 *
 * \param[out] pDevCap Pointer to structure to store device capabilities
 *
 * \retval kUwb_StatusCode_Success Capabilities retrieved successfully
 * \retval kUwb_StatusCode_InvalidArgument Invalid arguments passed
 * \retval kUwb_StatusCode_Failed Failed to retrieve or parse capabilities
 */
uwb_status_code_t uwb_api_core_get_caps_info(uwb_dev_caps_t *pDevCap);

/**
 * \brief Set core configuration parameters
 *
 * This function sets one or more core configuration parameters on the UWB device.
 * If any parameter fails to set, the error code will be updated in the configs structure.
 *
 * \param[in,out] configs Array of configuration parameters to set. Output value contains status
 * code for failed configurations
 * \param[in] num_configs Number of configuration parameters in the array
 *
 * \retval kUwb_StatusCode_Success Success
 * \retval kUwb_StatusCode_InvalidArgument Invalid arguments passed
 * \retval kUwb_StatusCode_Failed Failed to set one or more configurations
 */
uwb_status_code_t uwb_api_core_set_config(uwb_config_t *const configs, const uint8_t num_configs);

/**
 * \brief Get core configuration parameters
 *
 * This function retrieves one or more core configuration parameters from the UWB device.
 *
 * \param[in,out] configs Array of configuration parameters to retrieve
 * \param[in] num_configs Number of configuration parameters in the array
 *
 * \retval kUwb_StatusCode_Success Success
 * \retval kUwb_StatusCode_InvalidArgument Invalid arguments passed
 * \retval kUwb_StatusCode_Failed Failed to retrieve configurations
 */
uwb_status_code_t uwb_api_core_get_config(uwb_config_t *const configs, const uint8_t num_configs);

/**
 * \brief Query UWBS timestamp
 *
 * This function queries the current UWBS (UWB Subsystem) timestamp from the device.
 *
 * \param[out] pTimestampValue Buffer to store the timestamp value
 * \param[in] len Length of the timestamp buffer (must be at least 8 bytes)
 *
 * \retval kUwb_StatusCode_Success Timestamp retrieved successfully
 * \retval kUwb_StatusCode_InvalidArgument Invalid arguments passed
 * \retval kUwb_StatusCode_InsufficientBuffer Response data exceeds buffer size
 * \retval kUwb_StatusCode_Failed Failed to retrieve timestamp
 */
uwb_status_code_t uwb_api_core_query_uwbs_timestamp(uint8_t *const pTimestampValue,
						    const uint8_t len);

/**
 * \brief Get current UWB device state
 *
 * This function retrieves the current state of the UWB device.
 *
 * \param[out] pDeviceState Pointer to store the device state
 *
 * \retval kUwb_StatusCode_Success Device state retrieved successfully
 * \retval kUwb_StatusCode_InvalidArgument Invalid arguments passed
 * \retval kUwb_StatusCode_Failed Failed to retrieve device state
 */
uwb_status_code_t uwb_api_get_device_state(uint8_t *const pDeviceState);

/**
 * \brief Initialize a UWB session
 *
 * This function initializes a new UWB session with the specified session ID and type.
 * It waits for the session status notification to confirm the session is initialized.
 *
 * \param[in] session_id Session identifier (must be 0x00 for UWBD_RFTEST session type)
 * \param[in] session_type Type of session to initialize
 * \param[out] p_session_handle Pointer to store the session handle returned by the device
 *
 * \retval kUwb_StatusCode_Success Session initialized successfully
 * \retval kUwb_StatusCode_InvalidArgument Invalid parameters or pSessionHandle is NULL
 * \retval kUwb_StatusCode_Failed Session initialization failed or notification timeout
 */
uwb_status_code_t uwb_api_session_init(const uint32_t session_id,
				       const uwb_session_type_t session_type,
				       uint32_t *const p_session_handle);

/**
 * \brief Deinitialize a UWB session
 *
 * This function deinitialize an existing UWB session.
 * It waits for the session status notification to confirm the session is deinitialize.
 *
 * \param[in] sessionHandle Handle of the session to deinitialize
 *
 * \retval kUwb_StatusCode_Success Session deinitialize successfully
 * \retval kUwb_StatusCode_Failed Session deinitialization failed or notification timeout
 */
uwb_status_code_t uwb_api_session_deinit(const uint32_t sessionHandle);

/**
 * \brief Set application configuration parameters for a session
 *
 * This function sets one or more application-specific configuration parameters
 * for the specified UWB session. Extended MAC address modes are not supported.
 *
 * \param[in] sessionHandle Handle of the session to configure
 * \param[in,out] configs Array of configuration parameters to set
 * \param[in] num_configs Number of configuration parameters in the array
 *
 * \retval kUwb_StatusCode_Success All configurations set successfully
 * \retval kUwb_StatusCode_InvalidArgument Invalid arguments passed
 * \retval kUwb_StatusCode_Failed Failed to set one or more configurations
 */
uwb_status_code_t uwb_api_set_app_configs(const uint32_t sessionHandle, uwb_config_t *const configs,
					  const uint8_t num_configs);
/**
 * \brief Get application configuration parameters for a session
 *
 * This function retrieves one or more application-specific configuration parameters
 * for the specified UWB session.
 *
 * \param[in] sessionHandle Handle of the session to query
 * \param[in] noOfparams Number of parameters to retrieve
 * \param[in,out] appParams Array of application parameters to retrieve
 *
 * \retval kUwb_StatusCode_Success All parameters retrieved successfully
 * \retval kUwb_StatusCode_InvalidArgument Invalid arguments passed
 * \retval kUwb_StatusCode_InsufficientBuffer Insufficient buffer for parameter value
 * \retval kUwb_StatusCode_Failed Failed to retrieve parameters
 */
uwb_status_code_t uwb_api_get_app_configs(const uint32_t sessionHandle, uwb_config_t *const configs,
					  const uint8_t num_configs);

/**
 * \brief Get number of sessions in UWBS
 *
 * \param[out] pSessionCount Pointer to session count
 *
 * \retval kUwb_StatusCode_Success Success
 * \retval kUwb_StatusCode_InvalidArgument Invalid parameters are passed
 * \retval kUwb_StatusCode_Failed Otherwise
 */
uwb_status_code_t uwb_api_session_get_count(uint8_t *const pSessionCount);

/**
 * \brief Get Session State
 *
 * \param[in] sessionHandle      Initialized Session Handle
 * \param[out] sessionState   Session Status
 *
 * \retval kUwb_StatusCode_Success Success
 * \retval kUwb_StatusCode_InvalidArgument Invalid parameters are passed
 * \retval kUwb_StatusCode_Failed Otherwise
 */
uwb_status_code_t uwb_api_session_get_state(const uint32_t sessionHandle,
					    uint8_t *const pSessionState);

/**
 * \brief Update Controller Multicast List.
 *
 * \param[in] sessionHandle Session handle
 * \param[in] action        Add or delete controlee
 * \param[inout] pControleeList List of controlees to be updated in multicast for this controller.
 * Output contains status of multicast update for each controlee
 * \param[in] num_controlees Number of controlees to be updated in multicast list. This parameter is
 * the size of \p pControleeList array
 *
 * \retval kUwb_StatusCode_Success              Success
 * \retval kUwb_StatusCode_NotInitialized       UWB stack is not initialized
 * \retval kUwb_StatusCode_InvalidArgument      Invalid parameters are passed
 * \retval kUwb_StatusCode_Failed               Otherwise
 */
uwb_status_code_t uwb_api_update_controller_multicast_list(
	const uint32_t sessionHandle, const uwb_multicast_controller_actions_t action,
	uwb_multicast_controlee_list_context_t *const pControleeList, const uint8_t num_controlees);

/**
 * \brief Update the active rounds during the DL-TDoA Session for a initiator or responder device.
 *
 * \param[in]    sessionHandle      Unique Session Handle
 * \param[in]    nActiveRounds      Number of active rounds
 * \param[in]    macAddressingMode  MAC addressing mode- 2/8 bytes
 * \param[in]    roundConfigList    List/array of size nActiveRounds of round index + role tuple
 * \param[out]   pFailedRounds      List of round indices which failed
 * \param[inout] pNumFailedRounds   Number of failed rounds. Input is buffer length of \p
 * pFailedRounds. Output value is number of failed rounds
 *
 * \retval kUwb_StatusCode_Success         success
 * \retval kUwb_StatusCode_NotInitialized  UWB stack is not initialized
 * \retval kUwb_StatusCode_InvalidArgument      Invalid parameters are passed
 * \retval kUwb_StatusCode_Failed Otherwise
 */
uwb_status_code_t
uwb_api_update_dt_anchor_ranging_round(const uint32_t sessionHandle, const uint8_t nActiveRounds,
				       const uwb_mac_addr_mode_t macAddressingMode,
				       const uwb_active_rounds_config_t *const roundConfigList,
				       uint8_t *const pFailedRounds,
				       uint8_t *const pNumFailedRounds);

/**
 * \brief Update the active rounds during the DL-TDoA Session for a receiver device.
 *
 * \param[in]  sessionHandle         Session Handle
 * \param[in]  nActiveRounds         Number of active rounds
 * \param[in]  pRangingRoundIndices  Array ranging rounds to be activated. This array must be of \p
 * nActiveRounds length
 * \param[out] pFailedRounds         Array containing failed round configurations
 * \param[inout] pNumFailedRounds    Number of failed round configurations. Input value is length on
 * \p pFailedRounds array, output value is number of failed rounds returned
 *
 * \retval kUwb_StatusCode_Success          success
 * \retval kUwb_StatusCode_NotInitialized   UWB stack is not initialized
 * \retval kUwb_StatusCode_InvalidArgument      Invalid parameters are passed
 * \retval kUwb_StatusCode_Failed Otherwise
 */
uwb_status_code_t uwb_api_update_dt_tag_ranging_round(const uint32_t sessionHandle,
						      const uint8_t nActiveRounds,
						      const uint8_t *const pRangingRoundIndices,
						      uint8_t *const pFailedRounds,
						      uint8_t *const pNumFailedRounds);

/**
 * \brief API to get max data size that can be transferred during a single ranging round.
 *
 * \param[in] connectionIdentifier  Connection Identifier for which to query data size
 * \param[out] size                 Maximum data size returned by UWBS
 *
 * \retval kUwb_StatusCode_Success               on success
 * \retval kUwb_StatusCode_NotInitialized  if UWB stack is not initialized
 * \retval kUwb_StatusCode_InvalidArgument    if invalid parameters are passed
 * \retval #kUwb_StatusCode_Failed           otherwise
 *
 * \note This API must be called in session active state.
 */
uwb_status_code_t uwb_api_query_data_size_in_ranging(const uint32_t connectionIdentifier,
						     uint16_t *const size);

/**
 * \brief Frames the HUS session config in TLV format for Controller.
 *
 * \param[in] sessionHandle  Session handle of primary session
 * \param[in] num_phases Number of secondary session phases to configure
 * \param[in] pHusSecondarySessionConfig Pointer to structure of HUS Controller secondary session
 * configuration
 *
 * \retval kUwb_StatusCode_Success Success
 * \retval kUwb_StatusCode_NotInitialized UWB stack is not initialized
 * \retval kUwb_StatusCode_InvalidArgument      Invalid parameters are passed
 * \retval kUwb_StatusCode_Failed Otherwise
 */
uwb_status_code_t uwb_api_set_controller_hus_session(
	const uint32_t sessionHandle, const uint8_t num_phases,
	const uwb_hus_controller_secondary_session_config_t *const pHusSecondarySessionConfig);

/**
 * \brief Frames the HUS session config in TLV format for Controlee.
 *
 * \param[in] sessionHandle Session handle of primary session
 * \param[in] num_phases Number of secondary session phases to configure
 * \param[in] pHusSecondarySessionHandles Array of secondary session handles
 *
 * \retval kUwb_StatusCode_Success Success
 * \retval kUwb_StatusCode_InvalidArgument      Invalid parameters are passed
 * \retval kUwb_StatusCode_Failed Otherwise
 */
uwb_status_code_t
uwb_api_set_controlee_hus_session(const uint32_t sessionHandle, const uint8_t num_phases,
				  const uint32_t *const pHusSecondarySessionHandles);

/**
 * \brief Frames the Data Transfer Phase Control Message in TLV format.
 *
 * \param[in] sessionHandle Session handle to data transfer phase/session to be configured
 * \param[in] dtpcmRepetition Data repetition field
 * \param[in] dataTransferControl Data transfer control field
 * \param[in] dtpml Array of DTPML configurations
 * \param[in] dtpmlSize Size of \p dtpml array
 *
 * \retval kUwb_StatusCode_Success Success
 * \retval kUwb_StatusCode_InvalidArgument Invalid arguments passed
 * \retval kUwb_StatusCode_Failed otherwise
 */
uwb_status_code_t uwb_api_dtpcm_config(const uint32_t sessionHandle, const uint8_t dtpcmRepetition,
				       const uint8_t dataTransferControl,
				       const uwb_data_tx_phase_mng_list_t *const dtpml,
				       const uint8_t dtpmlSize);

/**
 * \brief Start Ranging for a session. Before Invoking Start ranging its
 * mandatory to set all the ranging configurations.
 *
 * \param[in] sessionHandle   Initialized Session Handle
 *
 * \retval kUwb_StatusCode_Success success
 * \retval kUwb_StatusCode_InvalidArgument Invalid arguments passed
 * \retval kUwb_StatusCode_Failed otherwise
 */
uwb_status_code_t uwb_api_session_start(const uint32_t sessionHandle);

/**
 * \brief Stop Ranging for a session
 *
 * \param[in] sessionHandle   Initialized Session Handle
 *
 * \retval kUwb_StatusCode_Success               on success
 * \retval kUwb_StatusCode_InvalidArgument      Invalid parameters are passed
 * \retval kUwb_StatusCode_Failed Otherwise
 *
 */
uwb_status_code_t uwb_api_session_stop(const uint32_t sessionHandle);

/**
 * \brief Get the number of times ranging has been attempted during the ranging session
 *
 * \param sessionHandle Session handle for which ranging count is requested
 * \param pRangingCount Pointer to store the ranging count value
 *
 * \retval kUwb_StatusCode_Success Success
 * \retval kUwb_StatusCode_InvalidArgument      Invalid parameters are passed
 * \retval kUwb_StatusCode_Failed Otherwise
 */
uwb_status_code_t uwb_api_session_get_ranging_count(const uint32_t sessionHandle,
						    uint32_t *const pRangingCount);

/**
 * \brief Creates Logical Link for Data Transfer.
 *
 * \param[in]    pLogicalLinkCreateCmd - Pointer to structure that contains params to create Logical
 * Link.
 * \param[out]   pLogicalLinkConnectId  - Pointer to uint32 that return the Logical Link connect ID.
 *
 * \param[in] sessionHandle Session handle for data transfer session
 * \param[in] linkLayerModeSelector Link layer mode selector
 * \param[in] destination_address Logical destination address of controlee
 * \param[in] logicalLinkClass Logical link class - must be 0x00
 * \param[out] pLogicalLinkConnectId Connection ID of created link
 *
 * \retval kUwb_StatusCode_Success Success
 * \retval kUwb_StatusCode_InvalidArgument Invalid arguments passed
 * \retval kUwb_StatusCode_Failed otherwise
 *
 * \note : The logical Link can be created in either in UWBAPI_SESSION_IDLE or
 * UWBAPI_STATUS_SESSION_ACTIVE, In case of Connection Oriented mode if it is called in
 * UWBAPI_SESSION_IDLE the application has to take care of the synchronization of uwb_api_send_data
 * till the LOGICAL_LINK_CREATE_NTF with LOGICAL_CO_LINK_CONNECTED is received.
 *
 * \note In case of SHORT_ADDR mode is used, then each Octet from octets 2 - 7 shall be set to 0x00.
 */
uwb_status_code_t
uwb_api_logical_link_create(const uint32_t sessionHandle,
			    const uwb_link_layer_mode_selector_t linkLayerModeSelector,
			    const uint8_t *const destination_address,
			    const uint8_t logicalLinkClass, uint32_t *const pLogicalLinkConnectId);

/**
 * \brief Closes Logical Link for Data Transfer, that was established before.
 *
 * \param[in] logicalLinkConnectId Logical link connection ID to be closed
 *
 * \retval kUwb_StatusCode_Success Success
 * \retval kUwb_StatusCode_InvalidArgument Invalid arguments passed
 * \retval kUwb_StatusCode_Failed Otherwise
 */
uwb_status_code_t uwb_api_logical_link_close(const uint32_t logicalLinkConnectId);

/**
 * \brief Gets the Logical Link parameters for the given connect ID. The API can be called to get
 * the Logical Link parameters used for the logical link associated to the LL_CONNECT_ID or the
 * Session Handle associated to the session.
 *
 * \param[in] ConnectionIdentifier       - This can be either the CONNECT_ID or the Session Handle.
 * Identifies the link or the data transfer session/phase params.
 * \param[out] phLogicalLinkGetParamsRsp - Pointer to structure that contains
 * params for Logical Link.
 *
 * \retval kUwb_StatusCode_Success Success
 * \retval kUwb_StatusCode_InvalidArgument Invalid arguments passed
 * \retval kUwb_StatusCode_Failed Otherwise
 *
 * \note To get the Link parameters of the Link created, this API needs to be called in
 * UWBAPI_STATUS_SESSION_ACTIVE with the LL_CONNECT_ID. To get the default Link Params for the
 * session, this API is to be called in either in UWBAPI_STATUS_SESSION_ACTIVE or
 * UWBAPI_STATUS_SESSION_IDLE with session handle of the session.
 */
uwb_status_code_t
uwb_api_logical_link_get_param(uint32_t connectionIdentifier,
			       uwb_logical_link_get_params_rsp_t *pLogicalLinkGetParamsRsp);

/**
 * \brief Send data in bypass mode
 *
 * \param[in] connectionIdentifier Connection identifier (Session handle) for data transfer
 * \param[in] destinationAddress Logical address of data recipient
 * \param[in] sequenceNumber Sequence number of data message
 * \param[in] data Buffer containing data
 * \param[in] dataSize Size of \p data
 *
 * \retval kUwb_StatusCode_Success success
 * \retval kUwb_StatusCode_InvalidArgument Invalid arguments passed
 * \retval kUwb_StatusCode_Failed otherwise
 */
uwb_status_code_t uwb_api_send_data(const uint32_t connectionIdentifier,
				    const uint8_t *const destinationAddress,
				    const uint16_t sequenceNumber, const uint8_t *const data,
				    const uint16_t dataSize);

/**
 * \brief Send data in logical link mode
 *
 * \param[in] connectionIdentifier Connection identifier (LL connect ID) for data transfer
 * \param[in] sequenceNumber Sequence number of data message
 * \param[in] data Buffer containing data
 * \param[in] dataSize Size of \p data
 *
 * \retval kUwb_StatusCode_Success success
 * \retval kUwb_StatusCode_InvalidArgument Invalid arguments passed
 * \retval kUwb_StatusCode_Failed otherwise
 */
uwb_status_code_t uwb_api_logical_link_send_data(const uint32_t connectionIdentifier,
						 const uint16_t sequenceNumber,
						 const uint8_t *const data,
						 const uint16_t dataSize);

/** Helper APIs for UWB command and responder parsing */

/**
 *
 * \brief uwb_serialize_get_core_config_payload
 *
 * Description      serialize Get Core Config Payload
 *
 * \retval Length of payload
 *
 */
uint16_t uwb_serialize_get_core_config_payload(const uint8_t noOfParams, const uint8_t paramLen,
					       const uint8_t *const paramId,
					       uint8_t *const pCmdBuf);

/**
 *
 * \brief uwb_serialize_session_init_payload
 *
 * Description      serialize Session Init Payload
 *
 * \retval Length of payload
 *
 */
uint16_t uwb_serialize_session_init_payload(const uint32_t sessionID,
					    const uwb_session_type_t sessionType,
					    uint8_t *const pCmdBuf);

/**
 *
 * \brief uwb_serialize_app_config_payload
 *
 * Description      serialize App Config Payload
 *
 * \retval Length of payload
 *
 */
uint16_t uwb_serialize_app_config_payload(uint32_t sessionHandle, uint8_t noOfParams,
					  uint16_t paramLen, uint8_t *pCmdBuf);

/**
 *
 * \brief uwb_parse_generic_device_info
 *
 * Description      Parse Generic Device Info. Extracts device information such as
 *                  version, manufacturer ID, and hardware/firmware details from
 *                  raw response data and populates the device info structure.
 *
 * Parameters       pdevInfo - Pointer to device info structure to populate
 *                  deviceInfoData - Pointer to raw device info data
 *                  deviceInfoLength - Length of device info data in bytes
 *
 * \retval true if parsing successful, false otherwise
 *
 */
bool uwb_parse_generic_device_info(uwb_device_info_t *const pdevInfo,
				   const uint8_t *const deviceInfoData,
				   const uint16_t deviceInfoLength);

/**
 *
 * \brief uwb_parse_generic_capability_info
 *
 * Description      Parse Generic Capability Info. Extracts device capabilities
 *                  including supported features, protocols, channels, and
 *                  operational parameters from raw capabilities data.
 *
 * Parameters       pDevCap - Pointer to device capabilities structure to populate
 *                  capsInfoData - Pointer to raw capabilities data
 *                  capsInfoLen - Length of capabilities data in bytes
 *
 * \retval Status code indicating success or failure of parsing
 *
 */
uwb_status_code_t uwb_parse_generic_capability_info(uwb_dev_caps_t *const pDevCap,
						    const uint8_t *const capsInfoData,
						    const uint16_t capsInfoLen);

/**
 *
 * \brief uwb_calculate_configs_command_buffer_length
 *
 * Description      Calculate Configs Command Buffer Length. Determines the
 *                  required buffer size for a configuration command based on
 *                  the number and size of configuration parameters.
 *
 * Parameters       configs - Pointer to array of configuration structures
 *                  num_configs - Number of configurations in the array
 *
 * \retval Required buffer length in bytes
 *
 */
uint16_t uwb_calculate_configs_command_buffer_length(const uwb_config_t *const configs,
						     const uint8_t num_configs);

/**
 *
 * \brief Calculate response buffer size required for CoreConfig or AppConfig commands
 *
 *  Calculate Configs Response Buffer Length. Determines the
 *  required buffer size for receiving a configuration response
 *  based on the number of configurations.
 *
 * \param[in] configs Array of all the config parameters sent in command
 * \param[in] num_configs Size of \p configs array
 *
 * \retval Required buffer length of response in bytes
 *
 */
uint16_t uwb_calculate_configs_response_buffer_length(const uwb_config_t *const configs,
						      const uint8_t num_configs);

/**
 *
 * \brief uwb_serialize_set_config_payload
 *
 * Description      Serialize Set Config Payload. Serializes multiple configuration
 *                  parameters into a command payload for setting UWB device or
 *                  session configurations.
 *
 * Parameters       configs - Pointer to array of configuration structures
 *                  num_configs - Number of configurations to serialize
 *                  pBuffer - Pointer to output buffer for serialized data
 *                  pBufferLen - Pointer to buffer length (input: max size, output: actual size)
 *
 * \retval Status code indicating success or failure of serialization
 *
 */
uwb_status_code_t uwb_serialize_set_config_payload(const uwb_config_t *const configs,
						   const uint8_t num_configs,
						   uint8_t *const pBuffer, uint16_t *pBufferLen);

/**
 *
 * \brief uwb_calculate_controller_hus_session_payload_length
 *
 * Calculate Controller HUS Session Payload Length. Determines
 * the required payload size for configuring a Hybrid UWB Session
 * (HUS) controller based on the session configuration parameters.
 *
 * \param[in] pHusSecondarySessionConfig Pointer to HUS secondary session configuration structure
 * \param[in] num_phases Number of configurations in \p pHusSecondarySessionConfig
 *
 * \retval Required command payload length in bytes
 */
uint16_t uwb_calculate_controller_hus_session_payload_length(
	const uwb_hus_controller_secondary_session_config_t *const pHusSecondarySessionConfig,
	const uint8_t num_phases);

/**
 *
 * \brief uwb_serialize_controller_hus_session_payload
 *
 * Serialize Controller HUS Session Payload. Serializes the
 * configuration for a Hybrid UWB Session controller, preparing
 * the device to act as the HUS controller.
 *
 * \param[in]       sessionHandle Session handle of primary session
 * \param[in]       num_phases Number of secondary session phases to configure
 * \param[in]       pHusSecondarySessionConfig Pointer to HUS secondary session configuration
 * structure
 * \param[out]      pCmdBuf Pointer to command buffer for serialized output
 * \param[inout]    pCmdBufLen Pointer to length of \p pCmdBuf. Input value is total size allocated
 * for buffer, output value is number of bytes populated
 *
 * \retval kUwb_StatusCode_Success Success
 * \retval kUwb_StatusCode_InsufficientBuffer Not enough buffer allocated
 */
uwb_status_code_t uwb_serialize_controller_hus_session_payload(
	const uint32_t sessionHandle, const uint8_t num_phases,
	const uwb_hus_controller_secondary_session_config_t *const pHusSecondarySessionConfig,
	uint8_t *const pCmdBuf, uint16_t *const pCmdBufLen);

/**
 *
 * \brief uwb_parse_config_response
 *
 * Description      Parse Config Response. Parses the response received after
 *                  setting configurations and populates the configuration
 *                  structures with status information for each parameter.
 *
 * Parameters       pBuffer - Pointer to response buffer
 *                  bufferLength - Length of response data in bytes
 *                  configs - Pointer to array of config structures to populate
 *                  num_configs - Number of configurations to parse
 *
 * \retval None
 *
 */
void uwb_parse_config_response(const uint8_t *const pBuffer, const uint16_t bufferLength,
			       uwb_config_t *const configs, const uint8_t num_configs);

/**
 *
 * \brief uwb_calculate_get_config_command_buffer_length
 *
 * Description      Calculate Get Config Command Buffer Length. Determines the
 *                  required buffer size for a Get Config command based on the
 *                  number of configuration parameters to retrieve.
 *
 * Parameters       configs - Pointer to array of configuration structures
 *                  num_configs - Number of configurations to retrieve
 *
 * \retval Required buffer length in bytes
 *
 */
uint16_t uwb_calculate_get_config_command_buffer_length(const uwb_config_t *const configs,
							const uint8_t num_configs);

/**
 *
 * \brief serialize_get_config_payload
 *
 * Description      Serialize Get Config Payload. Serializes a Get Config command
 *                  payload to retrieve current configuration values from the UWB
 *                  device or session.
 *
 * Parameters       configs - Pointer to array of configuration structures
 *                  num_configs - Number of configurations to retrieve
 *                  pBuffer - Pointer to output buffer for serialized data
 *                  pBufferLen - Pointer to buffer length (input: max size, output: actual size)
 *
 * \retval Status code indicating success or failure of serialization
 *
 */
uwb_status_code_t serialize_get_config_payload(uwb_config_t *const configs,
					       const uint8_t num_configs, uint8_t *const pBuffer,
					       uint16_t *pBufferLen);

/**
 *
 * \brief uwb_parse_get_config_response
 *
 * Description      Parse Get Config Response. Parses the response received after
 *                  requesting configuration values and populates the configuration
 *                  structures with the retrieved parameter values.
 *
 * Parameters       pBuffer - Pointer to response buffer
 *                  bufferLength - Length of response data in bytes
 *                  configs - Pointer to array of config structures to populate
 *                  num_configs - Number of configurations in response
 *
 * \retval None
 *
 */
void uwb_parse_get_config_response(const uint8_t *const pBuffer, const uint16_t bufferLength,
				   uwb_config_t *const configs, const uint8_t num_configs);

/**
 *
 * \brief uwb_calculate_set_controlee_hus_session_payload_length
 *
 * Calculate Set Controlee HUS Session Payload Length. Determines
 * the required payload size for configuring a controlee Hybrid
 * UWB Session based on the session configuration parameters.
 *
 * \param[in] num_phases Number of secondary session phases to configure
 *
 * \retval Required payload length in bytes
 */
uint16_t uwb_calculate_set_controlee_hus_session_payload_length(const uint8_t num_phases);

/**
 *
 * \brief uwb_serialize_controlee_hus_session_payload
 *
 * Serialize Controlee HUS Session Payload. Serializes the
 * configuration for a Hybrid UWB Session controlee, preparing
 * the device to act as the HUS controlee.
 *
 * \param[in] sessionHandle Session handle of primary session
 * \param[in] num_phases Number of secondary session phases to configure
 * \param[in] pHusSecondarySessionHandles Array of secondary session handles
 * \param[out] pCmdBuf Pointer to command buffer for serialized output
 * \param[inout] pCmdBufLen Pointer to length of \p pCmdBuf. Input value is allocated length of
 * buffer, output value is populated length
 *
 * \retval kUwb_StatusCode_Success Success
 * \retval kUwb_StatusCode_InsufficientBuffer Not enough buffer allocated
 */
uwb_status_code_t
uwb_serialize_controlee_hus_session_payload(const uint32_t sessionHandle, const uint8_t num_phases,
					    const uint32_t *const pHusSecondarySessionHandles,
					    uint8_t *const pCmdBuf, uint16_t *const pCmdBufLen);

/**
 *
 * \brief uwb_calculate_dtpcm_payload_length
 *
 * Calculate DTPCM Payload Length. Determines the required
 * payload size for Data Transfer Phase Configuration Message
 * based on the data TX phase configuration parameters.
 *
 * \param[in] dataTransferControl Data Transfer Control field of
 * SESSION_DATA_TRANSFER_PHASE_CONFIGURATION command
 * \param[in] dtpmlSize Number of devices configured in this command
 *
 * \retval Required payload length in bytes
 *
 */
uint16_t uwb_calculate_dtpcm_payload_length(const uint8_t dataTransferControl,
					    const uint8_t dtpmlSize);

/**
 *
 * \brief uwb_serialize_dtpcm_payload
 *
 * Description      Serialize DTPCM Payload. Serializes the Data Transfer Phase
 *                  Configuration Message payload for configuring data transfer
 *                  phase parameters in a UWB session.
 *
 * \param[in] sessionHandle Session handle to data transfer phase/session to be configured
 * \param[in] dtpcmRepetition Data repetition field
 * \param[in] dataTransferControl Data transfer control field
 * \param[in] dtpml Array of DTPML configurations
 * \param[in] dtpmlSize Size of \p dtpml array
 * \param[in] pCmdBuf Pointer to command buffer for serialized output
 * \param[inout] pCmdBufLen Pointer to size of \p pCmdBuf. Input value is allocated size of buffer,
 * output value is populated length
 *
 * \retval kUwb_StatusCode_Success Success
 * \retval kUwb_StatusCode_InsufficientBuffer Not enough buffer allocated
 */
uwb_status_code_t uwb_serialize_dtpcm_payload(const uint32_t sessionHandle,
					      const uint8_t dtpcmRepetition,
					      const uint8_t dataTransferControl,
					      const uwb_data_tx_phase_mng_list_t *const dtpml,
					      const uint8_t dtpmlSize, uint8_t *const pCmdBuf,
					      uint16_t *const pCmdBufLen);

/**
 *
 * \brief uwb_calculate_update_controller_multicast_list_payload_length
 *
 * Description      Calculate Update Controller Multicast List Payload Length.
 *                  Determines the required payload size for updating the controller
 *                  multicast list based on the number of controlees.
 *
 * Parameters       pControleeContext - Pointer to controlee list context structure
 *
 * \retval Required payload length in bytes
 *
 */
uint16_t
uwb_calculate_update_controller_multicast_list_payload_length(const uint8_t action,
							      const uint8_t num_controlees);

/**
 *
 * \brief uwb_calculate_update_controller_multicast_list_payload_length
 *
 * Calculate Update Controller Multicast List Payload Length.
 * Determines the required payload size for updating the controller
 * multicast list based on the number of controlees.
 *
 * \param[in] num_controlees Number of controlees configured in command
 *
 * \retval Required response payload length in bytes
 *
 */
uint16_t
uwb_calculate_update_controller_multicast_list_response_length(const uint8_t num_controlees);

/**
 *
 * \brief uwb_serialize_update_controller_multicast_list_payload
 *
 * Description      Serialize Update Controller Multicast List Payload. Serializes
 *                  the payload for adding or removing controlees from a multicast
 *                  ranging session.
 *
 * Parameters       pControleeContext - Pointer to controlee list context structure
 *                  pCmdBuf - Pointer to command buffer for serialized output
 *
 * \retval Length of the serialized payload in bytes
 *
 */
uwb_status_code_t uwb_serialize_update_controller_multicast_list_payload(
	const uint32_t sessionHandle, const uint8_t action,
	uwb_multicast_controlee_list_context_t *pControleeContext, const uint8_t num_controlees,
	uint8_t *pCmdBuf, uint16_t *const length);

/**
 *
 * \brief uwb_deserialize_update_controller_multicast_list_resp
 *
 * Description      Deserialize Update Controller Multicast List Response. Parses
 *                  the response from updating the controller multicast list and
 *                  populates the response structure with status information.
 *
 * Parameters       pControleeListRsp - Pointer to controlee list response structure to populate
 *                  rsp_data - Pointer to raw response data
 *
 * \retval None
 *
 */
uwb_status_code_t uwb_deserialize_update_controller_multicast_list_resp(
	uwb_multicast_controlee_list_context_t *const pControleeList, const uint8_t num_controlees,
	const uint8_t *const response, const uint16_t response_length);

/**
 *
 * \brief uwb_calculate_dt_anchor_ranging_round_command_length
 *
 * Description      Calculate DT Anchor Ranging Round Command Buffer. Determines
 *                  the required buffer size for DT anchor ranging round command
 *                  based on the number of active rounds and MAC addressing mode.
 *
 * Parameters       nActiveRounds - Number of active ranging rounds
 *                  macAddressingMode - MAC addressing mode (short/extended)
 *                  roundConfigList - Array of round configuration structures
 *
 * \retval Required buffer length in bytes
 *
 */
uint16_t uwb_calculate_dt_anchor_ranging_round_command_length(
	const uint8_t nActiveRounds, const uwb_mac_addr_mode_t macAddressingMode,
	const uwb_active_rounds_config_t *const roundConfigList);

/**
 *
 * \brief uwb_serialize_update_active_rounds_anchor_payload
 *
 * Description      serialize update active rounds Anchor Payload
 *
 * \retval Length of payload
 *
 */
uwb_status_code_t uwb_serialize_update_active_rounds_anchor_payload(
	uint32_t sessionHandle, const uint8_t nActiveRounds,
	const uwb_mac_addr_mode_t macAddressingMode,
	const uwb_active_rounds_config_t *const roundConfigList, uint8_t *const pCmdBuf,
	uint16_t *const payloadLength);
/**
 *
 * \brief uwb_parse_session_update_dt_anchor_ranging_round_rsp
 *
 * Description      Parse Session Update DT Anchor Ranging Round Response. Parses
 *                  the response from updating DT anchor ranging rounds and extracts
 *                  information about rounds that failed to activate.
 *
 * Parameters       response - Pointer to response data
 *                  pNotActivatedRound - Pointer to structure for rounds that failed activation
 *
 * \retval Status code indicating success or failure of parsing
 *
 */
uwb_status_code_t uwb_parse_session_update_dt_anchor_ranging_round_rsp(
	uint8_t *response, uwb_active_rounds_config_t *pNotActivatedRound);

/**
 *
 * \brief uwb_calculate_update_active_rounds_receiver_command_length
 *
 * Calculate Update Active Rounds Receiver Command Length.
 * Determines the required command buffer size for updating
 * receiver active rounds based on the number of rounds.
 *
 * \param[in] nActiveRounds Number of rounds to be activated for DT-Tag
 *
 * \retval Required command payload length in bytes
 *
 */
uint16_t uwb_calculate_update_active_rounds_receiver_command_length(const uint8_t nActiveRounds);

/**
 *
 * \brief uwb_serialize_update_tag_active_rounds_payload
 *
 * Serialize update active rounds receiver Payload
 *
 * \retval Length of payload
 *
 */
uwb_status_code_t uwb_serialize_update_tag_active_rounds_payload(
	const uint32_t sessionHandle, const uint8_t nActiveRounds,
	const uint8_t *const roundIndexList, uint8_t *const pCmdBuf, uint16_t *const pCmdBufLen);

/**
 *
 * \brief uwb_calculate_logical_link_create_payload_length
 *
 * Calculate Logical Link Create Payload Length. Determines the
 * required payload size for creating a logical link based on
 * the logical link configuration parameters.
 *
 *
 * \retval Required payload length in bytes
 *
 */
uint16_t uwb_calculate_logical_link_create_payload_length(void);

/**
 *
 * \brief uwb_serialize_create_logical_link_cmd
 *
 * Serialize Create Logical Link Command.
 *
 * \param[in] sessionHandle Session handle for data transfer session
 * \param[in] linkLayerModeSelector Link layer mode selector
 * \param[in] destination_address Logical destination address of controlee
 * \param[in] logicalLinkClass Logical link class - must be 0x00
 * \param[out] pCmdBuf Buffer to contain serialized payload
 * \param[inout] pCmdBufLen Size of \p pCmdBuf. Input value is allocated size of buffer, output
 * value is populated size
 *
 * \retval kUwb_StatusCode_Success Success
 * \retval kUwb_StatusCode_InsufficientBuffer Not enough buffer allocated
 */
uwb_status_code_t uwb_serialize_create_logical_link_cmd(
	const uint32_t sessionHandle, const uwb_link_layer_mode_selector_t linkLayerModeSelector,
	const uint8_t *const destination_address, const uint8_t logicalLinkClass,
	uint8_t *const pCmdBuf, uint16_t *const pCmdBufLen);

/**
 *
 * \brief uwb_deserialize_link_get_params_payload
 *
 * De-serialize the Logical Link Get Params Response
 *
 * \param[in] pResponse Response buffer pointing to control field
 * \param[out] pLogicalLinkGetParamsRsp Output structure to be populated with response fields
 *
 * \retval kUwb_StatusCode_Success Success
 */
uwb_status_code_t uwb_deserialize_link_get_params_payload(
	const uint8_t *const pResponse,
	uwb_logical_link_get_params_rsp_t *const pLogicalLinkGetParamsRsp);

/**
 *  \brief Calculate payload length required for creating SendData command
 *
 *  This function calculates the payload length required to create a SendData UCI command in bypass
 * mode. It does not append the UCI header size and must be incorporated by the caller.
 *
 *  \param[in] dataSize Size of data to transmit
 *
 *  \retval 0  if invalid arguments passed
 *  \retval >0 otherwise
 */
uint16_t uwb_calculate_send_data_payload_length(const uint16_t dataSize);

/**
 *  \brief Create payload for SendData command
 *
 *  This function serializes data packet input and populates the output buffer with
 *  payload to be sent for SendData UCI command in bypass mode
 *
 * \param[in] connectionIdentifier Connection identifier (Session handle) for data transfer
 * \param[in] destinationAddress Logical address of data recipient
 * \param[in] sequenceNumber Sequence number of data message
 * \param[in] data Buffer containing data
 * \param[in] dataSize Size of \p data
 * \param[out] payload Output buffer to be populated with SendData payload
 * \param[inout] payload_length Input value is total buffer size available, output value is total
 * populated buffer size
 *
 *  \retval kUwb_StatusCode_Success Successfully serialized SendData payload
 *  \retval kUwb_StatusCode_InsufficientBuffer Passed buffer size was not sufficient to serialize
 * payload
 */
uwb_status_code_t uwb_serialize_send_data_payload(const uint32_t connectionIdentifier,
						  const uint8_t *const destinationAddress,
						  const uint16_t sequenceNumber,
						  const uint8_t *const data,
						  const uint16_t dataSize, uint8_t *const payload,
						  uint16_t *const payload_length);

/**
 *  \brief Calculate payload length required for creating LL Send Data command
 *
 *  This function calculates the payload length required to create a Send Data UCI command in
 * logical link mode. It does not append the UCI header size and must be incorporated by the caller.
 *
 *  \param[in] dataSize Size of data to send
 *
 *  \retval 0  if invalid arguments passed
 *  \retval >0 otherwise
 */
uint16_t uwb_calculate_ll_send_data_payload_length(const uint16_t dataSize);

/**
 *  \brief Create payload for SendData command
 *
 *  This function serializes data packet input and populates the output buffer with
 *  payload to be sent for SendData UCI command in logical link mode
 *
 * \param[in] connectionIdentifier Connection identifier (LL Connect ID) for data transfer
 * \param[in] sequenceNumber Sequence number of data message
 * \param[in] data Buffer containing data
 * \param[in] dataSize Size of \p data
 * \param[out] payload Output buffer to be populated with SendData payload
 * \param[inout] payload_length Input value is total buffer size available, output value is total
 * populated buffer size
 *
 *  \retval kUwb_StatusCode_Success Successfully serialized SendData payload
 *  \retval kUwb_StatusCode_InsufficientBuffer Passed buffer size was not sufficient to serialize
 * payload
 */
uwb_status_code_t uwb_serialize_ll_send_data_payload(const uint32_t connectionIdentifier,
						     const uint16_t sequenceNumber,
						     const uint8_t *const data,
						     const uint16_t dataSize,
						     uint8_t *const payload,
						     uint16_t *const payload_length);

/** Default notification handler definitions */

/**
 * \brief Start notification handler task
 */
int uwb_ntf_handler_start(void);

/**
 * \brief Stop notification handler task
 */
void uwb_ntf_handler_stop(void);

/**
 * \brief Callback for notification handler which should be registered during \fn uwb_api_initialize
 */
void uwb_ntf_callback_handler(const uint8_t *const packet, const uint32_t len);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __UWB_API_FIRA_H__ */
