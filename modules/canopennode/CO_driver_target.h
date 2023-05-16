/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 * Copyright (c) 2023 Silpion IT Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODULES_CANOPENNODE_CO_DRIVER_H
#define ZEPHYR_MODULES_CANOPENNODE_CO_DRIVER_H

/*
 * Zephyr RTOS CAN driver interface and configuration for CANopenNode
 * CANopen protocol stack.
 *
 * See CANopenNode/301/CO_driver.h for API description.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/toolchain.h>

struct canopen_context {
	const struct device *dev;
};


/** Major version number of CANopenNode */
#define CO_VERSION_MAJOR 2
/** Minor version number of CANopenNode */
#define CO_VERSION_MINOR 0


/**
 * Configuration of @ref CO_NMT_Heartbeat.
 *
 * Possible flags, can be ORed:
 * - CO_CONFIG_NMT_CALLBACK_CHANGE - Enable custom callback after NMT
 *   state changes. Callback is configured by
 *   CO_NMT_initCallbackChanged().
 * - CO_CONFIG_NMT_MASTER - Enable simple NMT master
 * - #CO_CONFIG_FLAG_CALLBACK_PRE - Enable custom callback after preprocessing
 *   received NMT CAN message.
 *   Callback is configured by CO_NMT_initCallbackPre().
 * - #CO_CONFIG_FLAG_TIMERNEXT - Enable calculation of timerNext_us variable
 *   inside CO_NMT_process().
 */
#define CO_CONFIG_NMT (\
	((true == CONFIG_CANOPENNODE_NMT_FLAG_CALLBACK_PRE) ? CO_CONFIG_FLAG_CALLBACK_PRE : 0)|\
	((true == CONFIG_CANOPENNODE_NMT_FLAG_TIMERNEXT) ? CO_CONFIG_FLAG_TIMERNEXT : 0)|\
	(CONFIG_CANOPENNODE_NMT_CALLBACK_CHANGE) ? CO_CONFIG_NMT_CALLBACK_CHANGE : 0 |\
	(CONFIG_CANOPENNODE_NMT_MASTER) ? CO_CONFIG_NMT_MASTER : 0)

/**
 * Configuration of @ref CO_HBconsumer
 *
 * Possible flags, can be ORed:
 * - #CO_CONFIG_FLAG_CALLBACK_PRE - Enable custom callback after preprocessing
 *   received heartbeat CAN message.
 *   Callback is configured by CO_HBconsumer_initCallbackPre().
 * - #CO_CONFIG_FLAG_TIMERNEXT - Enable calculation of timerNext_us variable
 *   inside CO_HBconsumer_process().
 * - CO_CONFIG_HB_CONS_ENABLE - Enable heartbeat consumer.
 * - CO_CONFIG_HB_CONS_CALLBACK_CHANGE - Enable custom common callback after NMT
 *   state of the monitored node changes. Callback is configured by
 *   CO_HBconsumer_initCallbackNmtChanged().
 * - CO_CONFIG_HB_CONS_CALLBACK_MULTI - Enable multiple custom callbacks, which
 *   can be configured individually for each monitored node. Callbacks are
 *   configured by CO_HBconsumer_initCallbackNmtChanged(),
 *   CO_HBconsumer_initCallbackHeartbeatStarted(),
 *   CO_HBconsumer_initCallbackTimeout() and
 *   CO_HBconsumer_initCallbackRemoteReset() functions.
 * - CO_CONFIG_HB_CONS_QUERY_FUNCT - Enable functions for query HB state or
 *   NMT state of the specific monitored node.
 * Note that CO_CONFIG_HB_CONS_CALLBACK_CHANGE and
 * CO_CONFIG_HB_CONS_CALLBACK_MULTI cannot be set simultaneously.
 */
#define CO_CONFIG_HB_CONS (\
	((true == CONFIG_CANOPENNODE_HB_CONS_FLAG_CALLBACK_PRE) ? CO_CONFIG_FLAG_CALLBACK_PRE : 0)|\
	((true == CONFIG_CANOPENNODE_HB_CONS_FLAG_TIMERNEXT) ? CO_CONFIG_FLAG_TIMERNEXT : 0)|\
	((true == CONFIG_CANOPENNODE_HB_CONS_ENABLE) ? CO_CONFIG_HB_CONS_ENABLE : 0) |\
	((true == CONFIG_CANOPENNODE_HB_CONS_CALLBACK_CHANGE) ? CO_CONFIG_HB_CONS_CALLBACK_CHANGE : 0)|\
	((true == CONFIG_CANOPENNODE_HB_CONS_CALLBACK_MULTI) ? CO_CONFIG_HB_CONS_CALLBACK_MULTI : 0)|\
	((true == CONFIG_CANOPENNODE_HB_CONS_QUERY_FUNCT) ? CO_CONFIG_HB_CONS_QUERY_FUNCT : 0))

#define CO_CONFIG_HB_CONS_SIZE CONFIG_CANOPENNODE_HB_CONS_SIZE

/**
 * Configuration of @ref CO_Emergency
 *
 * Possible flags, can be ORed:
 * - CO_CONFIG_EM_PRODUCER - Enable emergency producer.
 * - CO_CONFIG_EM_PROD_CONFIGURABLE - Emergency producer COB-ID is configurable,
 *   OD object 0x1014. If not configurable, then 0x1014 is read-only, COB_ID
 *   is set to CO_CAN_ID_EMERGENCY + nodeId and write is not verified.
 * - CO_CONFIG_EM_PROD_INHIBIT - Enable inhibit timer on emergency producer,
 *   OD object 0x1015.
 * - CO_CONFIG_EM_HISTORY - Enable error history, OD object 0x1003,
 *   "Pre-defined error field"
 * - CO_CONFIG_EM_CONSUMER - Enable simple emergency consumer with callback.
 * - CO_CONFIG_EM_STATUS_BITS - Access @ref CO_EM_errorStatusBits_t from OD.
 * - #CO_CONFIG_FLAG_CALLBACK_PRE - Enable custom callback after preprocessing
 *   emergency condition by CO_errorReport() or CO_errorReset() call.
 *   Callback is configured by CO_EM_initCallbackPre().
 * - #CO_CONFIG_FLAG_TIMERNEXT - Enable calculation of timerNext_us variable
 *   inside CO_EM_process().
 */
#define CO_CONFIG_EM (\
	((true == CONFIG_CANOPENNODE_EM_FLAG_CALLBACK_PRE) ? CO_CONFIG_FLAG_CALLBACK_PRE : 0)|\
	((true == CONFIG_CANOPENNODE_EM_FLAG_TIMERNEXT) ? CO_CONFIG_FLAG_TIMERNEXT : 0)|\
	((true == CONFIG_CANOPENNODE_EM_PRODUCER) ? CO_CONFIG_EM_PRODUCER : 0 )|\
	((true == CONFIG_CANOPENNODE_EM_PROD_CONFIGURABLE) ? CO_CONFIG_EM_PROD_CONFIGURABLE : 0 )|\
	((true == CONFIG_CANOPENNODE_EM_PROD_INHIBIT) ? CO_CONFIG_EM_PROD_INHIBIT : 0 )|\
	((true == CONFIG_CANOPENNODE_EM_HISTORY) ? CO_CONFIG_EM_HISTORY : 0 )|\
	((true == CONFIG_CANOPENNODE_EM_STATUS_BITS) ? CO_CONFIG_EM_STATUS_BITS : 0 )|\
	((true == CONFIG_CANOPENNODE_EM_CONSUMER) ? CO_CONFIG_EM_CONSUMER : 0))

/**
 * Maximum number of @ref CO_EM_errorStatusBits_t
 *
 * Stack uses 6*8 = 48 @ref CO_EM_errorStatusBits_t, others are free to use by
 * manufacturer. Allowable value range is from 48 to 256 bits in steps of 8.
 * Default is 80.
 */
#define CO_CONFIG_EM_ERR_STATUS_BITS_COUNT (CONFIG_CANOPENNODE_EM_ERR_STATUS_BITS_COUNT*8)

/**
 * Size of the internal buffer, where emergencies are stored after error
 * indication with @ref CO_error() function. Each emergency has to be post-
 * processed by the @ref CO_EM_process() function. In case of overflow, error is
 * indicated but emergency message is not sent.
 *
 * The same buffer is also used for OD object 0x1003, "Pre-defined error field".
 *
 * Each buffer element consumes 8 bytes. Valid values are 1 to 254.
 */
#define CO_CONFIG_EM_BUFFER_SIZE CONFIG_CANOPENNODE_EM_BUFFER_SIZE

/**
 * Condition for calculating CANopen Error register, "generic" error bit.
 *
 * Condition must observe suitable @ref CO_EM_errorStatusBits_t and use
 * corresponding member of errorStatusBits array from CO_EM_t to calculate the
 * condition. See also @ref CO_errorRegister_t.
 *
 * @warning Size of @ref CO_CONFIG_EM_ERR_STATUS_BITS_COUNT must be large
 * enough. (CO_CONFIG_EM_ERR_STATUS_BITS_COUNT/8) must be larger than index of
 * array member in em->errorStatusBits[index].
 *
 * em->errorStatusBits[5] should be included in the condition, because they are
 * used by the stack.
 */
#define CO_CONFIG_ERR_CONDITION_GENERIC CONFIG_CANOPENNODE_ERR_CONDITION_GENERIC

/**
 * Condition for calculating CANopen Error register, "current" error bit.
 * See @ref CO_CONFIG_ERR_CONDITION_GENERIC for description.
 * Macro is not defined by default, so no error is verified.
 */
#define CO_CONFIG_ERR_CONDITION_CURRENT CONFIG_CANOPENNODE_ERR_CONDITION_CURRENT


/**
 * Condition for calculating CANopen Error register, "voltage" error bit.
 * See @ref CO_CONFIG_ERR_CONDITION_GENERIC for description.
 * Macro is not defined by default, so no error is verified.
 */
#define CO_CONFIG_ERR_CONDITION_VOLTAGE CONFIG_CANOPENNODE_ERR_CONDITION_VOLTAGE

/**
 * Condition for calculating CANopen Error register, "temperature" error bit.
 * See @ref CO_CONFIG_ERR_CONDITION_GENERIC for description.
 * Macro is not defined by default, so no error is verified.
 */
#define CO_CONFIG_ERR_CONDITION_TEMPERATURE CONFIG_CANOPENNODE_ERR_CONDITION_TEMPERATURE

/**
 * Condition for calculating CANopen Error register, "communication" error bit.
 * See @ref CO_CONFIG_ERR_CONDITION_GENERIC for description.
 *
 * em->errorStatusBits[2] and em->errorStatusBits[3] must be included in the
 * condition, because they are used by the stack.
 */
#define CO_CONFIG_ERR_CONDITION_COMMUNICATION CONFIG_CANOPENNODE_ERR_CONDITION_COMMUNICATION

/**
 * Condition for calculating CANopen Error register, "device profile" error bit.
 * See @ref CO_CONFIG_ERR_CONDITION_GENERIC for description.
 * Macro is not defined by default, so no error is verified.
 */
#define CO_CONFIG_ERR_CONDITION_DEV_PROFILE CONFIG_CANOPENNODE_ERR_CONDITION_DEV_PROFILE

/**
 * Condition for calculating CANopen Error register, "manufacturer" error bit.
 * See @ref CO_CONFIG_ERR_CONDITION_GENERIC for description.
 *
 * em->errorStatusBits[8] and em->errorStatusBits[8] are pre-defined, but can
 * be changed.
 */
#define CO_CONFIG_ERR_CONDITION_MANUFACTURER CONFIG_CANOPENNODE_ERR_CONDITION_MANUFACTURER

/**
 * Configuration of @ref CO_SDOserver
 *
 * Possible flags, can be ORed:
 * - CO_CONFIG_SDO_SRV_SEGMENTED - Enable SDO server segmented transfer.
 * - CO_CONFIG_SDO_SRV_BLOCK - Enable SDO server block transfer. If set, then
 *   CO_CONFIG_SDO_SRV_SEGMENTED must also be set.
 * - #CO_CONFIG_FLAG_CALLBACK_PRE - Enable custom callback after preprocessing
 *   received SDO CAN message.
 *   Callback is configured by CO_SDOserver_initCallbackPre().
 * - #CO_CONFIG_FLAG_TIMERNEXT - Enable calculation of timerNext_us variable
 *   inside CO_SDOserver_process().
 * - #CO_CONFIG_FLAG_OD_DYNAMIC - Enable dynamic configuration of additional SDO
 *   servers (Writing to object 0x1201+ re-configures the additional server).
 */
#define CO_CONFIG_SDO_SRV (\
	((true == CONFIG_CANOPENNODE_SDO_SRV_FLAG_CALLBACK_PRE) ? CO_CONFIG_FLAG_CALLBACK_PRE : 0)|\
	((true == CONFIG_CANOPENNODE_SDO_SRV_FLAG_TIMERNEXT) ? CO_CONFIG_FLAG_TIMERNEXT : 0)|\
	((true == CONFIG_CANOPENNODE_SDO_SRV_FLAG_OD_DYNAMIC) ? CO_CONFIG_FLAG_OD_DYNAMIC : 0)|\
	((true == CONFIG_CANOPENNODE_SDO_SRV_SEGMENTED) ? CO_CONFIG_SDO_SRV_SEGMENTED : 0)|\
	((true == CONFIG_CANOPENNODE_SDO_SRV_BLOCK) ? CO_CONFIG_SDO_SRV_BLOCK : 0))

/**
 * Size of the internal data buffer for the SDO server.
 *
 * If size is less than size of some variables in Object Dictionary, then data
 * will be transferred to internal buffer in several segments. Minimum size is
 * 8 or 899 (127*7) for block transfer.
 */
#define CO_CONFIG_SDO_SRV_BUFFER_SIZE CONFIG_CANOPENNODE_SDO_BUFFER_SIZE

/**
 * Configuration of @ref CO_SDOclient
 *
 * Possible flags, can be ORed:
 * - CO_CONFIG_SDO_CLI_ENABLE - Enable SDO client.
 * - CO_CONFIG_SDO_CLI_SEGMENTED - Enable SDO client segmented transfer.
 * - CO_CONFIG_SDO_CLI_BLOCK - Enable SDO client block transfer. If set, then
 *   CO_CONFIG_SDO_CLI_SEGMENTED, CO_CONFIG_FIFO_ALT_READ and
 *   CO_CONFIG_FIFO_CRC16_CCITT must also be set.
 * - CO_CONFIG_SDO_CLI_LOCAL - Enable local transfer, if Node-ID of the SDO
 *   server is the same as node-ID of the SDO client. (SDO client is the same
 *   device as SDO server.) Transfer data directly without communication on CAN.
 * - #CO_CONFIG_FLAG_CALLBACK_PRE - Enable custom callback after preprocessing
 *   received SDO CAN message.
 *   Callback is configured by CO_SDOclient_initCallbackPre().
 * - #CO_CONFIG_FLAG_TIMERNEXT - Enable calculation of timerNext_us variable
 *   inside CO_SDOclientDownloadInitiate(), CO_SDOclientDownload(),
 *   CO_SDOclientUploadInitiate(), CO_SDOclientUpload().
 * - #CO_CONFIG_FLAG_OD_DYNAMIC - Enable dynamic configuration of SDO clients
 *   (Writing to object 0x1280+ re-configures the client).
 */
#define CO_CONFIG_SDO_CLI (\
	((true == CONFIG_CANOPENNODE_SDO_CLI_FLAG_CALLBACK_PRE) ? CO_CONFIG_FLAG_CALLBACK_PRE : 0)|\
	((true == CONFIG_CANOPENNODE_SDO_CLI_FLAG_TIMERNEXT) ? CO_CONFIG_FLAG_TIMERNEXT : 0)|\
	((true == CONFIG_CANOPENNODE_SDO_CLI_FLAG_OD_DYNAMIC) ? CO_CONFIG_FLAG_OD_DYNAMIC : 0)|\
	((true == CONFIG_CANOPENNODE_SDO_CLI_ENABLE) ? CO_CONFIG_SDO_CLI_ENABLE : 0)|\
	((true == CONFIG_CANOPENNODE_SDO_CLI_SEGMENTED) ? CO_CONFIG_SDO_CLI_SEGMENTED : 0)|\
	((true == CONFIG_CANOPENNODE_SDO_CLI_BLOCK) ? CO_CONFIG_SDO_CLI_BLOCK : 0)|\
	((true == CONFIG_CANOPENNODE_SDO_CLI_LOCAL) ? CO_CONFIG_SDO_CLI_LOCAL : 0))

/**
 * Size of the internal data buffer for the SDO client.
 *
 * Circular buffer is used for SDO communication. It can be read or written
 * between successive SDO calls. So size of the buffer can be lower than size of
 * the actual size of data transferred. If only segmented transfer is used, then
 * buffer size can be as low as 7 bytes, if data are read/written each cycle. If
 * block transfer is used, buffer size should be set to at least 1000 bytes, so
 * maximum blksize can be used (blksize is calculated from free buffer space).
 * Default value for block transfer is 1000, otherwise 32.
 */
#define CO_CONFIG_SDO_CLI_BUFFER_SIZE CONFIG_CANOPENNODE_SDO_CLI_BUFFER_SIZE

/**
 * Configuration of @ref CO_TIME
 *
 * Possible flags, can be ORed:
 * - CO_CONFIG_TIME_ENABLE - Enable TIME object and TIME consumer.
 * - CO_CONFIG_TIME_PRODUCER - Enable TIME producer.
 * - #CO_CONFIG_FLAG_CALLBACK_PRE - Enable custom callback after preprocessing
 *   received TIME CAN message.
 *   Callback is configured by CO_TIME_initCallbackPre().
 */
#define CO_CONFIG_TIME (\
	((true == CONFIG_CANOPENNODE_TIME_FLAG_CALLBACK_PRE) ? CO_CONFIG_FLAG_CALLBACK_PRE : 0)|\
	((true == CONFIG_CANOPENNODE_TIME_ENABLE) ? CO_CONFIG_TIME_ENABLE : 0) |\
	((true == CONFIG_CANOPENNODE_TIME_PRODUCER) ? CO_CONFIG_TIME_PRODUCER : 0))

/**
 * Configuration of @ref CO_SYNC
 *
 * Possible flags, can be ORed:
 * - CO_CONFIG_SYNC_ENABLE - Enable SYNC object and SYNC consumer.
 * - CO_CONFIG_SYNC_PRODUCER - Enable SYNC producer.
 * - #CO_CONFIG_FLAG_CALLBACK_PRE - Enable custom callback after preprocessing
 *   received SYNC CAN message.
 *   Callback is configured by CO_SYNC_initCallbackPre().
 * - #CO_CONFIG_FLAG_TIMERNEXT - Enable calculation of timerNext_us variable
 *   inside CO_SYNC_process().
 */
#define CO_CONFIG_SYNC (\
	((true == CONFIG_CANOPENNODE_SYNC_FLAG_CALLBACK_PRE) ? CO_CONFIG_FLAG_CALLBACK_PRE : 0)|\
	((true == CONFIG_CANOPENNODE_SYNC_FLAG_TIMERNEXT) ? CO_CONFIG_FLAG_TIMERNEXT : 0)|\
	((true == CONFIG_CANOPENNODE_SYNC_ENABLE) ? CO_CONFIG_SYNC_ENABLE : 0)|\
	((true == CONFIG_CANOPENNODE_SYNC_PRODUCER) ? CO_CONFIG_SYNC_PRODUCER : 0))

/**
 * Configuration of @ref CO_PDO
 *
 * Possible flags, can be ORed:
 * - CO_CONFIG_RPDO_ENABLE - Enable receive PDO objects.
 * - CO_CONFIG_TPDO_ENABLE - Enable transmit PDO objects.
 * - CO_CONFIG_PDO_SYNC_ENABLE - Enable SYNC in PDO objects.
 * - CO_CONFIG_RPDO_CALLS_EXTENSION - Enable calling configured extension
 *   callbacks when received RPDO CAN message modifies OD entries.
 * - CO_CONFIG_TPDO_CALLS_EXTENSION - Enable calling configured extension
 *   callbacks before TPDO CAN message is sent.
 * - #CO_CONFIG_FLAG_CALLBACK_PRE - Enable custom callback after preprocessing
 *   received RPDO CAN message.
 *   Callback is configured by CO_RPDO_initCallbackPre().
 * - #CO_CONFIG_FLAG_TIMERNEXT - Enable calculation of timerNext_us variable
 *   inside CO_TPDO_process().
 */
#define CO_CONFIG_PDO (\
	((true == CONFIG_CANOPENNODE_PDO_FLAG_CALLBACK_PRE) ? CO_CONFIG_FLAG_CALLBACK_PRE : 0)|\
	((true == CONFIG_CANOPENNODE_PDO_FLAG_TIMERNEXT) ? CO_CONFIG_FLAG_TIMERNEXT : 0)|\
	((true == CONFIG_CANOPENNODE_RPDO_ENABLE) ? CO_CONFIG_RPDO_ENABLE : 0)|\
	((true == CONFIG_CANOPENNODE_TPDO_ENABLE) ? CO_CONFIG_TPDO_ENABLE : 0)|\
	((true == CONFIG_CANOPENNODE_PDO_SYNC_ENABLE) ? CO_CONFIG_PDO_SYNC_ENABLE : 0)|\
	((true == CONFIG_CANOPENNODE_RPDO_CALLS_EXTENSION) ? CO_CONFIG_RPDO_CALLS_EXTENSION : 0)|\
	((true == CONFIG_CANOPENNODE_TPDO_CALLS_EXTENSION) ? CO_CONFIG_TPDO_CALLS_EXTENSION : 0))

/**
 * Configuration of @ref CO_LEDs
 *
 * Possible flags, can be ORed:
 * - CO_CONFIG_LEDS_ENABLE - Enable calculation of the CANopen LED indicators.
 * - #CO_CONFIG_FLAG_TIMERNEXT - Enable calculation of timerNext_us variable
 *   inside CO_NMT_process().
 */
#define CO_CONFIG_LEDS (\
	((true == CONFIG_CANOPENNODE_LEDS_FLAG_TIMERNEXT) ? CO_CONFIG_FLAG_TIMERNEXT : 0)|\
	((true == CONFIG_CANOPENNODE_LEDS_ENABLE) ? CO_CONFIG_LEDS_ENABLE : 0))

/**
 * Configuration of @ref CO_GFC
 *
 * Possible flags, can be ORed:
 * - CO_CONFIG_GFC_ENABLE - Enable the GFC object
 * - CO_CONFIG_GFC_CONSUMER - Enable the GFC consumer
 * - CO_CONFIG_GFC_PRODUCER - Enable the GFC producer
 */
#define CO_CONFIG_GFC \
	(((true == CONFIG_CANOPENNODE_GFC_ENABLE) ? CO_CONFIG_GFC_ENABLE : 0)|\
	((true == CONFIG_CANOPENNODE_GFC_CONSUMER) ? CO_CONFIG_GFC_CONSUMER : 0)|\
	((true == CONFIG_CANOPENNODE_GFC_PRODUCER) ? CO_CONFIG_GFC_PRODUCER : 0))

/**
 * Configuration of @ref CO_SRDO
 *
 * Possible flags, can be ORed:
 * - CO_CONFIG_SRDO_ENABLE - Enable the SRDO object.
 * - CO_CONFIG_SRDO_CHECK_TX - Enable checking data before sending.
 * - CO_CONFIG_RSRDO_CALLS_EXTENSION - Enable calling configured extension
 *   callbacks when received RSRDO CAN message modifies OD entries.
 * - CO_CONFIG_TRSRDO_CALLS_EXTENSION - Enable calling configured extension
 *   callbacks before TSRDO CAN message is sent.
 * - #CO_CONFIG_FLAG_CALLBACK_PRE - Enable custom callback after preprocessing
 *   received RSRDO CAN message.
 *   Callback is configured by CO_SRDO_initCallbackPre().
 * - #CO_CONFIG_FLAG_TIMERNEXT - Enable calculation of timerNext_us variable
 *   inside CO_SRDO_process() (Tx SRDO only).
 */
#define CO_CONFIG_SRDO (\
	((true == CONFIG_CANOPENNODE_SRDO_FLAG_CALLBACK_PRE) ? CO_CONFIG_FLAG_CALLBACK_PRE : 0)|\
	((true == CONFIG_CANOPENNODE_SRDO_FLAG_TIMERNEXT) ? CO_CONFIG_FLAG_TIMERNEXT : 0)|\
	((true == CONFIG_CANOPENNODE_SRDO_ENABLE) ? CO_CONFIG_SRDO_ENABLE : 0)|\
	((true == CONFIG_CANOPENNODE_SRDO_CHECK_TX) ? CO_CONFIG_SRDO_CHECK_TX : 0)|\
	((true == CONFIG_CANOPENNODE_RSRDO_CALLS_EXTENSION) ? CO_CONFIG_RSRDO_CALLS_EXTENSION : 0)|\
	((true == CONFIG_CANOPENNODE_TSRDO_CALLS_EXTENSION) ? CO_CONFIG_TSRDO_CALLS_EXTENSION : 0))

/**
 * SRDO Tx time delay
 *
 * minimum time between the first and second SRDO (Tx) message
 * in us
 */
#define CO_CONFIG_SRDO_MINIMUM_DELAY CONFIG_CANOPENNODE_SRDO_MINIMUM_DELAY

/**
 * Configuration of @ref CO_LSS
 *
 * Possible flags, can be ORed:
 * - CO_CONFIG_LSS_SLAVE - Enable LSS slave
 * - CO_CONFIG_LSS_SLAVE_FASTSCAN_DIRECT_RESPOND - Send LSS fastscan respond
 *   directly from CO_LSSslave_receive() function.
 * - CO_CONFIG_LSS_MASTER - Enable LSS master
 * - #CO_CONFIG_FLAG_CALLBACK_PRE - Enable custom callback after preprocessing
 *   received CAN message.
 *   Callback is configured by CO_LSSmaster_initCallbackPre().
 */
#define CO_CONFIG_LSS (\
	((true == CONFIG_CANOPENNODE_LSS_FLAG_CALLBACK_PRE) ? CO_CONFIG_FLAG_CALLBACK_PRE : 0)|\
	((true == CONFIG_CANOPENNODE_LSS_SLAVE) ? CO_CONFIG_LSS_SLAVE : 0)|\
	((true == CONFIG_CANOPENNODE_LSS_SLAVE_FASTSCAN_DIRECT_RESPOND) ? \
	CO_CONFIG_LSS_SLAVE_FASTSCAN_DIRECT_RESPOND : 0)|\
	((true == CONFIG_CANOPENNODE_LSS_MASTER) ? CO_CONFIG_LSS_MASTER : 0))

/**
 * Configuration of @ref CO_CANopen_309_3
 *
 * Gateway object is covered by standard CiA 309 - CANopen access from other
 * networks. It enables usage of the NMT master, SDO client and LSS master as a
 * gateway device.
 *
 * Possible flags, can be ORed:
 * - CO_CONFIG_GTW_MULTI_NET - Enable multiple network interfaces in gateway
 *   device. This functionality is currently not implemented.
 * - CO_CONFIG_GTW_ASCII - Enable gateway device with ASCII mapping (CiA 309-3)
 *   If set, then CO_CONFIG_FIFO_ASCII_COMMANDS must also be set.
 * - CO_CONFIG_GTW_ASCII_SDO - Enable SDO client. If set, then
 *   CO_CONFIG_FIFO_ASCII_DATATYPES must also be set.
 * - CO_CONFIG_GTW_ASCII_NMT - Enable NMT master
 * - CO_CONFIG_GTW_ASCII_LSS - Enable LSS master
 * - CO_CONFIG_GTW_ASCII_LOG - Enable non-standard message log read
 * - CO_CONFIG_GTW_ASCII_ERROR_DESC - Print error description as additional
 *   comments in gateway-ascii device for SDO and gateway errors.
 * - CO_CONFIG_GTW_ASCII_PRINT_HELP - use non-standard command "help" to print
 *   help usage.
 * - CO_CONFIG_GTW_ASCII_PRINT_LEDS - Display "red" and "green" CANopen status
 *   LED diodes on terminal.
 */
#define CO_CONFIG_GTW \
	(((true == CONFIG_CANOPENNODE_GTW_MULTI_NET) ? CO_CONFIG_GTW_MULTI_NET : 0)|\
	((true == CONFIG_CANOPENNODE_GTW_ASCII) ? CO_CONFIG_GTW_ASCII : 0)|\
	((true == CONFIG_CANOPENNODE_GTW_ASCII_SDO) ? CO_CONFIG_GTW_ASCII_SDO : 0)|\
	((true == CONFIG_CANOPENNODE_GTW_ASCII_NMT) ? CO_CONFIG_GTW_ASCII_NMT : 0)|\
	((true == CONFIG_CANOPENNODE_GTW_ASCII_LSS) ? CO_CONFIG_GTW_ASCII_LSS : 0)|\
	((true == CONFIG_CANOPENNODE_GTW_ASCII_LOG) ? CO_CONFIG_GTW_ASCII_LOG : 0)|\
	((true == CONFIG_CANOPENNODE_GTW_ASCII_ERROR_DESC) ? CO_CONFIG_GTW_ASCII_ERROR_DESC : 0)|\
	((true == CONFIG_CANOPENNODE_GTW_ASCII_PRINT_HELP) ? CO_CONFIG_GTW_ASCII_PRINT_HELP : 0)|\
	((true == CONFIG_CANOPENNODE_GTW_ASCII_PRINT_LEDS) ? CO_CONFIG_GTW_ASCII_PRINT_LEDS : 0))

/**
 * Number of loops of #CO_SDOclientDownload() in case of block download
 *
 * If SDO clint has block download in progress and OS has buffer for CAN tx
 * messages, then #CO_SDOclientDownload() functionion can be called multiple
 * times within own loop (up to 127). This can speed-up SDO block transfer.
 */
#define CO_CONFIG_GTW_BLOCK_DL_LOOP CONFIG_CANOPENNODE_GTW_BLOCK_DL_LOOP

/**
 * Size of command buffer in ASCII gateway object.
 *
 * If large amount of data is transferred (block transfer), then this should be
 * increased to 1000 or more. Buffer may be refilled between the block transfer.
 */
#define CO_CONFIG_GTWA_COMM_BUF_SIZE CONFIG_CANOPENNODE_GTWA_COMM_BUF_SIZE

/**
 * Size of message log buffer in ASCII gateway object.
 */
#define CO_CONFIG_GTWA_LOG_BUF_SIZE CONFIG_CANOPENNODE_GTWA_LOG_BUF_SIZE

/**
 * Configuration of @ref CO_crc16_ccitt calculation
 *
 * Possible flags, can be ORed:
 * - CO_CONFIG_CRC16_ENABLE - Enable CRC16 calculation
 * - CO_CONFIG_CRC16_EXTERNAL - CRC functions are defined externally
 */
#define CO_CONFIG_CRC16 \
	(((true == CONFIG_CANOPENNODE_CRC16_ENABLE) ? CO_CONFIG_CRC16_ENABLE : 0)|\
	((true == CONFIG_CANOPENNODE_CRC16_EXTERNAL) ? CO_CONFIG_CRC16_EXTERNAL : 0))

/**
 * Configuration of @ref CO_CANopen_301_fifo
 *
 * FIFO buffer is basically a simple first-in first-out circular data buffer. It
 * is used by the SDO client and by the CANopen gateway. It has additional
 * advanced functions for data passed to FIFO.
 *
 *
 * Possible flags, can be ORed:
 * - CO_CONFIG_FIFO_ENABLE - Enable FIFO buffer
 * - CO_CONFIG_FIFO_ALT_READ - This must be enabled, when SDO client has
 *   CO_CONFIG_SDO_CLI_BLOCK enabled. See @ref CO_fifo_altRead().
 * - CO_CONFIG_FIFO_CRC16_CCITT - This must be enabled, when SDO client has
 *   CO_CONFIG_SDO_CLI_BLOCK enabled. It enables CRC calculation on data.
 * - CO_CONFIG_FIFO_ASCII_COMMANDS - This must be enabled, when CANopen gateway
 *   has CO_CONFIG_GTW_ASCII enabled. It adds command handling functions.
 * - CO_CONFIG_FIFO_ASCII_DATATYPES - This must be enabled, when CANopen gateway
 *   has CO_CONFIG_GTW_ASCII and CO_CONFIG_GTW_ASCII_SDO enabled. It adds
 *   datatype transform functions between binary and ascii, which are necessary
 *   for SDO client.
 */
#define CO_CONFIG_FIFO \
	(((true == CONFIG_CANOPENNODE_FIFO_ENABLE) ? CO_CONFIG_FIFO_ENABLE : 0)|\
	((true == CONFIG_CANOPENNODE_FIFO_ALT_READ) ? CO_CONFIG_FIFO_ALT_READ : 0)|\
	((true == CONFIG_CANOPENNODE_FIFO_CRC16_CCITT) ? CO_CONFIG_FIFO_CRC16_CCITT : 0)|\
	((true == CONFIG_CANOPENNODE_FIFO_ASCII_COMMANDS) ? CO_CONFIG_FIFO_ASCII_COMMANDS : 0)|\
	((true == CONFIG_CANOPENNODE_FIFO_ASCII_DATATYPES) ? CO_CONFIG_FIFO_ASCII_DATATYPES : 0))

/**
 * Configuration of @ref CO_trace for recording variables over time.
 *
 * Possible flags, can be ORed:
 * - CO_CONFIG_TRACE_ENABLE - Enable Trace recorder
 * - CO_CONFIG_TRACE_OWN_INTTYPES - If set, then macros PRIu32("u" or "lu")
 *   and PRId32("d" or "ld") must be set. (File inttypes.h can not be included).
 */
#define CO_CONFIG_TRACE \
	(((true == CONFIG_CANOPENNODE_TRACE_ENABLE) ? CO_CONFIG_TRACE_ENABLE : 0)|\
	((true == CONFIG_CANOPENNODE_TRACE_OWN_INTTYPES) ? CO_CONFIG_TRACE_OWN_INTTYPES : 0))

/**
 * Configuration of debug messages from different parts of the stack, which can
 * be logged according to target specific function.
 *
 * Possible flags, can be ORed:
 * - CO_CONFIG_DEBUG_COMMON - Define default CO_DEBUG_COMMON(msg) macro. This
 *   macro is target specific. This macro is then used as default macro in all
 *   other defined CO_DEBUG_XXX(msg) macros.
 * - CO_CONFIG_DEBUG_SDO_CLIENT - Define default CO_DEBUG_SDO_CLIENT(msg) macro.
 * - CO_CONFIG_DEBUG_SDO_SERVER - Define default CO_DEBUG_SDO_SERVER(msg) macro.
 */
#define CO_CONFIG_DEBUG \
	(((true == CONFIG_CANOPENNODE_DEBUG_COMMON) ? CO_CONFIG_DEBUG_COMMON : 0)|\
	((true == CONFIG_CANOPENNODE_DEBUG_SDO_CLIENT) ? CO_CONFIG_DEBUG_SDO_CLIENT : 0)|\
	((true == CONFIG_CANOPENNODE_DEBUG_SDO_SERVER) ? CO_CONFIG_DEBUG_SDO_SERVER : 0))

/* Basic definitions. If big endian, CO_SWAP_xx macros must swap bytes. */
#ifdef CONFIG_LITTLE_ENDIAN
	#define CO_LITTLE_ENDIAN
	#define CO_SWAP_16(x) x
	#define CO_SWAP_32(x) x
	#define CO_SWAP_64(x) x
#else
	#define CO_BIG_ENDIAN
	#include <../../../modules/lib/picolibc/newlib/libc/include/byteswap.h>
	#define CO_SWAP_16(x) bswap_16(x)
	#define CO_SWAP_32(x) bswap_32(x)
	#define CO_SWAP_64(x) bswap_64(x)
#endif


/* Use Kconfig to configure static variables or calloc() */
#if (true == CONFIG_CANOPENNODE_USE_GLOBALS)
#define CO_USE_GLOBALS
#endif


/* Use trace buffer size from Kconfig */
#define CO_TRACE_BUFFER_SIZE_FIXED CONFIG_CANOPENNODE_TRACE_BUFFER_SIZE

typedef bool          bool_t;
typedef float         float32_t;
typedef long double   float64_t;
typedef char          char_t;
typedef unsigned char oChar_t;
typedef unsigned char domain_t;

typedef struct canopen_rx_msg {
	uint8_t data[8];
	uint16_t ident;
	uint8_t DLC;
} CO_CANrxMsg_t;

typedef void (*CO_CANrxBufferCallback_t)(void *object,
					 void *message);

typedef struct canopen_rx {
	int filter_id;
	void *object;
	CO_CANrxBufferCallback_t pFunct;
	uint16_t ident;
	uint16_t mask;
} CO_CANrx_t;

typedef struct canopen_tx {
	uint8_t data[8];
	uint16_t ident;
	uint8_t DLC;
	bool_t rtr : 1;
	bool_t bufferFull : 1;
	bool_t syncFlag : 1;
} CO_CANtx_t;

typedef struct canopen_module {
	const struct device *dev;
	CO_CANrx_t *rx_array;
	CO_CANtx_t *tx_array;
	uint16_t rx_size;
	uint16_t tx_size;
	uint32_t errors;
	uint16_t CANerrorStatus;
	void *em;
	bool_t configured : 1;
	bool_t CANnormal : 1;
	bool_t first_tx_msg : 1;
} CO_CANmodule_t;


void canopen_send_lock(void);
void canopen_send_unlock(void);
#define CO_LOCK_CAN_SEND()   canopen_send_lock()
#define CO_UNLOCK_CAN_SEND() canopen_send_unlock()

void canopen_emcy_lock(void);
void canopen_emcy_unlock(void);
#define CO_LOCK_EMCY()   canopen_emcy_lock()
#define CO_UNLOCK_EMCY() canopen_emcy_unlock()

void canopen_od_lock(void);
void canopen_od_unlock(void);
#define CO_LOCK_OD()   canopen_od_lock()
#define CO_UNLOCK_OD() canopen_od_unlock()

/* Synchronization between CAN receive and message processing threads. */
#define CO_MemoryBarrier()
#define CO_FLAG_READ(rxNew) ((rxNew) != NULL)
#define CO_FLAG_SET(rxNew) {CO_MemoryBarrier(); rxNew = (void *)1L; }
#define CO_FLAG_CLEAR(rxNew) {CO_MemoryBarrier(); rxNew = NULL; }

/* Access to received CAN message */
static inline uint16_t CO_CANrxMsg_readIdent(const CO_CANrxMsg_t *rxMsg)
{
	return (uint16_t)rxMsg->ident;
}

static inline uint8_t CO_CANrxMsg_readDLC(const CO_CANrxMsg_t *rxMsg)
{
	return (uint8_t)rxMsg->DLC;
}

static inline uint8_t *CO_CANrxMsg_readData(const CO_CANrxMsg_t *rxMsg)
{
	return (uint8_t *)rxMsg->data;
}


#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_MODULES_CANOPENNODE_CO_DRIVER_H */
