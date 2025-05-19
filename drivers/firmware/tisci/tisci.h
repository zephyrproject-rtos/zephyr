/*
 * Copyright (c) 2025, Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Texas Instruments System Control Interface (TISCI) Protocol
 *
 */

#ifndef INCLUDE_ZEPHYR_DRIVERS_TISCI_PROTOCOL_H_
#define INCLUDE_ZEPHYR_DRIVERS_TISCI_PROTOCOL_H_

#include "zephyr/kernel.h"
#include <stddef.h>
#include <stdint.h>

#ifndef __packed
#define __packed __attribute__((__packed__))
#endif

#define TISCI_MSG_ENABLE_WDT            0x0000
#define TISCI_MSG_WAKE_RESET            0x0001
#define TISCI_MSG_VERSION               0x0002
#define TISCI_MSG_WAKE_REASON           0x0003
#define TISCI_MSG_GOODBYE               0x0004
#define TISCI_MSG_SYS_RESET             0x0005
#define TISCI_MSG_BOARD_CONFIG          0x000b
#define TISCI_MSG_BOARD_CONFIG_RM       0x000c
#define TISCI_MSG_BOARD_CONFIG_SECURITY 0x000d
#define TISCI_MSG_BOARD_CONFIG_PM       0x000e
#define TISCI_MSG_QUERY_MSMC            0x0020

/* Device requests */
#define TISCI_MSG_SET_DEVICE_STATE  0x0200
#define TISCI_MSG_GET_DEVICE_STATE  0x0201
#define TISCI_MSG_SET_DEVICE_RESETS 0x0202

/* Clock requests */
#define TISCI_MSG_SET_CLOCK_STATE       0x0100
#define TISCI_MSG_GET_CLOCK_STATE       0x0101
#define TISCI_MSG_SET_CLOCK_PARENT      0x0102
#define TISCI_MSG_GET_CLOCK_PARENT      0x0103
#define TISCI_MSG_GET_NUM_CLOCK_PARENTS 0x0104
#define TISCI_MSG_SET_CLOCK_FREQ        0x010c
#define TISCI_MSG_QUERY_CLOCK_FREQ      0x010d
#define TISCI_MSG_GET_CLOCK_FREQ        0x010e

/* Processor Control Messages */
#define TISCI_MSG_PROC_REQUEST          0xc000
#define TISCI_MSG_PROC_RELEASE          0xc001
#define TISCI_MSG_PROC_HANDOVER         0xc005
#define TISCI_MSG_SET_PROC_BOOT_CONFIG  0xc100
#define TISCI_MSG_SET_PROC_BOOT_CTRL    0xc101
#define TISCI_MSG_PROC_AUTH_BOOT_IMAGE  0xc120
#define TISCI_MSG_GET_PROC_BOOT_STATUS  0xc400
#define TISCI_MSG_WAIT_PROC_BOOT_STATUS 0xc401

/* Resource Management Requests */
/* RM TISCI message to request a resource range assignment for a host */
#define TISCI_MSG_GET_RESOURCE_RANGE 0x1500
/* RM TISCI message to set an IRQ between a peripheral and host processor */
#define TISCI_MSG_RM_IRQ_SET         (0x1000U)
/* RM TISCI message to release a configured IRQ */
#define TISCI_MSG_RM_IRQ_RELEASE     (0x1001U)

/* NAVSS resource management */
/* Ringacc requests */
#define TISCI_MSG_RM_RING_CFG 0x1110

/* PSI-L requests */
#define TISCI_MSG_RM_PSIL_PAIR   0x1280
#define TISCI_MSG_RM_PSIL_UNPAIR 0x1281

#define TISCI_MSG_RM_UDMAP_TX_ALLOC     0x1200
#define TISCI_MSG_RM_UDMAP_TX_FREE      0x1201
#define TISCI_MSG_RM_UDMAP_RX_ALLOC     0x1210
#define TISCI_MSG_RM_UDMAP_RX_FREE      0x1211
#define TISCI_MSG_RM_UDMAP_FLOW_CFG     0x1220
#define TISCI_MSG_RM_UDMAP_OPT_FLOW_CFG 0x1221

#define TISCI_MSG_RM_UDMAP_TX_CH_CFG            0x1205
#define TISCI_MSG_RM_UDMAP_RX_CH_CFG            0x1215
#define TISCI_MSG_RM_UDMAP_FLOW_SIZE_THRESH_CFG 0x1231

#define TISCI_MSG_FWL_SET          0x9000
#define TISCI_MSG_FWL_GET          0x9001
#define TISCI_MSG_FWL_CHANGE_OWNER 0x9002

/**
 * @struct rx_msg
 * @brief Received message details
 * @param seq: Message sequence number
 * @param size: Message size in bytes
 * @param buf: Buffer for message data
 * @param response_ready_sem: Semaphore to signal when a response is ready
 */
struct rx_msg {
	uint8_t seq;
	size_t size;
	void *buf;
	struct k_sem *response_ready_sem;
};

/**
 * @struct tisci_msg_hdr
 * @brief Generic Message Header for All messages and responses
 * @param type:	Type of messages: One of TISCI_MSG* values
 * @param host:	Host of the message
 * @param seq:	Message identifier indicating a transfer sequence
 * @param flags:	Flag for the message
 */
struct tisci_msg_hdr {
	uint16_t type;
	uint8_t host;
	uint8_t seq;
#define TISCI_MSG_FLAG(val)               (1 << (val))
#define TISCI_FLAG_REQ_GENERIC_NORESPONSE 0x0
#define TISCI_FLAG_REQ_ACK_ON_RECEIVED    TISCI_MSG_FLAG(0)
#define TISCI_FLAG_REQ_ACK_ON_PROCESSED   TISCI_MSG_FLAG(1)
#define TISCI_FLAG_RESP_GENERIC_NACK      0x0
#define TISCI_FLAG_RESP_GENERIC_ACK       TISCI_MSG_FLAG(1)
	/* Additional Flags */
	uint32_t flags;
} __packed;

/**
 * @struct tisci_secure_msg_hdr
 * @brief Header that prefixes all TISCI messages sent
 *				  via secure transport.
 * @param checksum:	crc16 checksum for the entire message
 * @param reserved:	Reserved for future use.
 */
struct tisci_secure_msg_hdr {
	uint16_t checksum;
	uint16_t reserved;
} __packed;

/**
 * @struct tisci_msg_resp_version
 * @brief Response for a message
 *
 * In general, ABI version changes follow the rule that minor version increments
 * are backward compatible. Major revision changes in ABI may not be
 * backward compatible.
 *
 * Response to a generic message with message type TISCI_MSG_VERSION
 * @param hdr:		Generic header
 * @param firmware_description: String describing the firmware
 * @param firmware_revision:	Firmware revision
 * @param abi_major:		Major version of the ABI that firmware supports
 * @param abi_minor:		Minor version of the ABI that firmware supports
 */
struct tisci_msg_resp_version {
	struct tisci_msg_hdr hdr;
	char firmware_description[32];
	uint16_t firmware_revision;
	uint8_t abi_major;
	uint8_t abi_minor;
} __packed;

/**
 * @struct tisci_msg_req_reboot
 * @brief Reboot the SoC
 * @param hdr:	Generic Header
 * @param domain:	Domain to be reset, 0 for full SoC reboot.
 *
 * Request type is TISCI_MSG_SYS_RESET, responded with a generic
 * ACK/NACK message.
 */
struct tisci_msg_req_reboot {
	struct tisci_msg_hdr hdr;
	uint8_t domain;
} __packed;

/**
 * @struct tisci_msg_resp_reboot
 * @brief Response to system reset (generic ACK/NACK)
 */
struct tisci_msg_resp_reboot {
	struct tisci_msg_hdr hdr;
} __packed;

/**
 * @struct tisci_msg_board_config_req
 * @brief Board configuration message
 * @param hdr:		Generic Header
 * @param boardcfgp_low:	Lower 32 bit of the pointer pointing to the board
 *			configuration data
 * @param boardcfgp_high:	Upper 32 bit of the pointer pointing to the board
 *			configuration data
 * @param boardcfg_size:	Size of board configuration data object
 * Request type is TISCI_MSG_BOARD_CONFIG, responded with a generic
 * ACK/NACK message.
 */
struct tisci_msg_board_config_req {
	struct tisci_msg_hdr hdr;
	uint32_t boardcfgp_low;
	uint32_t boardcfgp_high;
	uint16_t boardcfg_size;
} __packed;

/**
 * @struct tisci_msg_board_config_resp
 * @brief Response to board config request (generic ACK/NACK)
 * @param hdr: Generic Header
 */
struct tisci_msg_board_config_resp {
	struct tisci_msg_hdr hdr;
} __packed;

/**
 * @struct tisci_msg_resp_query_msmc
 * @brief Query msmc message response structure
 * @param hdr:		Generic Header
 * @param msmc_start_low:	Lower 32 bit of msmc start
 * @param msmc_start_high:	Upper 32 bit of msmc start
 * @param msmc_end_low:	Lower 32 bit of msmc end
 * @param msmc_end_high:	Upper 32 bit of msmc end
 *
 * @brief Response to a generic message with message type TISCI_MSG_QUERY_MSMC
 */
struct tisci_msg_resp_query_msmc {
	struct tisci_msg_hdr hdr;
	uint32_t msmc_start_low;
	uint32_t msmc_start_high;
	uint32_t msmc_end_low;
	uint32_t msmc_end_high;
} __packed;

/**
 * @struct tisci_msg_req_set_device_state
 * @brief Set the desired state of the device
 * @param hdr:		Generic header
 * @param id:	Indicates which device to modify
 * @param reserved: Reserved space in message, must be 0 for backward compatibility
 * @param state: The desired state of the device.
 *
 * Certain flags can also be set to alter the device state:
 *  MSG_FLAG_DEVICE_WAKE_ENABLED - Configure the device to be a wake source.
 * The meaning of this flag will vary slightly from device to device and from
 * SoC to SoC but it generally allows the device to wake the SoC out of deep
 * suspend states.
 *  MSG_FLAG_DEVICE_RESET_ISO - Enable reset isolation for this device.
 *  MSG_FLAG_DEVICE_EXCLUSIVE - Claim this device exclusively. When passed
 * with STATE_RETENTION or STATE_ON, it will claim the device exclusively.
 * If another host already has this device set to STATE_RETENTION or STATE_ON,
 * the message will fail. Once successful, other hosts attempting to set
 * STATE_RETENTION or STATE_ON will fail.
 *
 * Request type is TISCI_MSG_SET_DEVICE_STATE, responded with a generic
 * ACK/NACK message.
 */
struct tisci_msg_req_set_device_state {
	/* Additional hdr->flags options */
#define MSG_FLAG_DEVICE_WAKE_ENABLED TISCI_MSG_FLAG(8)
#define MSG_FLAG_DEVICE_RESET_ISO    TISCI_MSG_FLAG(9)
#define MSG_FLAG_DEVICE_EXCLUSIVE    TISCI_MSG_FLAG(10)
	struct tisci_msg_hdr hdr;
	uint32_t id;
	uint32_t reserved;

#define MSG_DEVICE_SW_STATE_AUTO_OFF  0
#define MSG_DEVICE_SW_STATE_RETENTION 1
#define MSG_DEVICE_SW_STATE_ON        2
	uint8_t state;
} __packed;

/**
 * @struct tisci_msg_resp_set_device_state
 * @brief Response to set device state (generic ACK/NACK)
 */
struct tisci_msg_resp_set_device_state {
	struct tisci_msg_hdr hdr;
} __packed;

/**
 * @struct tisci_msg_req_get_device_state
 * @brief Request to get device.
 * @param hdr:		Generic header
 * @param id:		Device Identifier
 *
 * Request type is TISCI_MSG_GET_DEVICE_STATE, responded device state
 * information
 */
struct tisci_msg_req_get_device_state {
	struct tisci_msg_hdr hdr;
	uint32_t id;
} __packed;

/**
 * @struct tisci_msg_resp_get_device_state
 * @brief Response to get device request.
 * @param hdr:		Generic header
 * @param context_loss_count: Indicates how many times the device has lost context. A
 *	driver can use this monotonic counter to determine if the device has
 *	lost context since the last time this message was exchanged.
 * @param resets: Programmed state of the reset lines.
 * @param programmed_state:	The state as programmed by set_device.
 *			- Uses the MSG_DEVICE_SW_* macros
 * @param current_state:	The actual state of the hardware.
 *
 * Response to request TISCI_MSG_GET_DEVICE_STATE.
 */
struct tisci_msg_resp_get_device_state {
	struct tisci_msg_hdr hdr;
	uint32_t context_loss_count;
	uint32_t resets;
	uint8_t programmed_state;
#define MSG_DEVICE_HW_STATE_OFF   0
#define MSG_DEVICE_HW_STATE_ON    1
#define MSG_DEVICE_HW_STATE_TRANS 2
	uint8_t current_state;
} __packed;

/**
 * @struct tisci_msg_req_set_device_reset
 * @brief Set the desired resets configuration of the device
 * @param hdr:		Generic header
 * @param id:	Indicates which device to modify
 * @param resets: A bit field of resets for the device. The meaning, behavior,
 *	and usage of the reset flags are device specific. 0 for a bit
 *	indicates releasing the reset represented by that bit while 1
 *	indicates keeping it held.
 *
 * Request type is TISCI_MSG_SET_DEVICE_RESETS, responded with a generic
 * ACK/NACK message.
 */
struct tisci_msg_req_set_device_resets {
	struct tisci_msg_hdr hdr;
	uint32_t id;
	uint32_t resets;
} __packed;

/**
 * @struct tisci_msg_resp_set_device_resets
 * @brief Response to set device resets request (generic ACK/NACK)
 */
struct tisci_msg_resp_set_device_resets {
	struct tisci_msg_hdr hdr;
} __packed;

/**
 * @struct tisci_msg_req_set_clock_state
 * @brief Request to setup a Clock state
 * @param hdr:	Generic Header, Certain flags can be set specific to the clocks:
 *		MSG_FLAG_CLOCK_ALLOW_SSC: Allow this clock to be modified
 *		via spread spectrum clocking.
 *		MSG_FLAG_CLOCK_ALLOW_FREQ_CHANGE: Allow this clock's
 *		frequency to be changed while it is running so long as it
 *		is within the min/max limits.
 *		MSG_FLAG_CLOCK_INPUT_TERM: Enable input termination, this
 *		is only applicable to clock inputs on the SoC pseudo-device.
 * @param dev_id:	Device identifier this request is for
 * @param clk_id:	Clock identifier for the device for this request.
 *		Each device has it's own set of clock inputs. This indexes
 *		which clock input to modify.
 * @param request_state: Request the state for the clock to be set to.
 *		MSG_CLOCK_SW_STATE_UNREQ: The IP does not require this clock,
 *		it can be disabled, regardless of the state of the device
 *		MSG_CLOCK_SW_STATE_AUTO: Allow the System Controller to
 *		automatically manage the state of this clock. If the device
 *		is enabled, then the clock is enabled. If the device is set
 *		to off or retention, then the clock is internally set as not
 *		being required by the device.(default)
 *		MSG_CLOCK_SW_STATE_REQ:  Configure the clock to be enabled,
 *		regardless of the state of the device.
 *
 * Normally, all required clocks are managed by TISCI entity, this is used
 * only for specific control *IF* required. Auto managed state is
 * MSG_CLOCK_SW_STATE_AUTO, in other states, TISCI entity assume remote
 * will explicitly control.
 *
 * Request type is TISCI_MSG_SET_CLOCK_STATE, response is a generic
 * ACK or NACK message.
 */
struct tisci_msg_req_set_clock_state {
	/* Additional hdr->flags options */
#define MSG_FLAG_CLOCK_ALLOW_SSC         TISCI_MSG_FLAG(8)
#define MSG_FLAG_CLOCK_ALLOW_FREQ_CHANGE TISCI_MSG_FLAG(9)
#define MSG_FLAG_CLOCK_INPUT_TERM        TISCI_MSG_FLAG(10)
	struct tisci_msg_hdr hdr;
	uint32_t dev_id;
	uint8_t clk_id;
#define MSG_CLOCK_SW_STATE_UNREQ 0
#define MSG_CLOCK_SW_STATE_AUTO  1
#define MSG_CLOCK_SW_STATE_REQ   2
	uint8_t request_state;
} __packed;

/**
 * @struct tisci_msg_resp_set_clock_state
 * @brief Response to set clock state (generic ACK/NACK)
 */
struct tisci_msg_resp_set_clock_state {
	struct tisci_msg_hdr hdr;
} __packed;

/**
 * @struct tisci_msg_req_get_clock_state
 * @brief Request for clock state
 * @param hdr:	Generic Header
 * @param dev_id:	Device identifier this request is for
 * @param clk_id:	Clock identifier for the device for this request.
 *		Each device has it's own set of clock inputs. This indexes
 *		which clock input to get state of.
 *
 * Request type is TISCI_MSG_GET_CLOCK_STATE, response is state
 * of the clock
 */
struct tisci_msg_req_get_clock_state {
	struct tisci_msg_hdr hdr;
	uint32_t dev_id;
	uint8_t clk_id;
} __packed;

/**
 * @struct tisci_msg_resp_get_clock_state
 * @brief Response to get clock state
 * @param hdr:	Generic Header
 * @param programmed_state: Any programmed state of the clock. This is one of
 *		MSG_CLOCK_SW_STATE* values.
 * @param current_state: Current state of the clock. This is one of:
 *		MSG_CLOCK_HW_STATE_NOT_READY: Clock is not ready
 *		MSG_CLOCK_HW_STATE_READY: Clock is ready
 *
 * Response to TISCI_MSG_GET_CLOCK_STATE.
 */
struct tisci_msg_resp_get_clock_state {
	struct tisci_msg_hdr hdr;
	uint8_t programmed_state;
#define MSG_CLOCK_HW_STATE_NOT_READY 0
#define MSG_CLOCK_HW_STATE_READY     1
	uint8_t current_state;
} __packed;

/**
 * @struct tisci_msg_req_set_clock_parent
 * @brief Set the clock parent
 * @param hdr:	Generic Header
 * @param dev_id:	Device identifier this request is for
 * @param clk_id:	Clock identifier for the device for this request.
 *		Each device has it's own set of clock inputs. This indexes
 *		which clock input to modify.
 * @param parent_id:	The new clock parent is selectable by an index via this
 *		parameter.
 *
 * Request type is TISCI_MSG_SET_CLOCK_PARENT, response is generic
 * ACK / NACK message.
 */
struct tisci_msg_req_set_clock_parent {
	struct tisci_msg_hdr hdr;
	uint32_t dev_id;
	uint8_t clk_id;
	uint8_t parent_id;
} __packed;

/**
 * @struct tisci_msg_resp_set_clock_parent
 * @brief Response to set clock parent (generic ACK/NACK)
 */
struct tisci_msg_resp_set_clock_parent {
	struct tisci_msg_hdr hdr;
} __packed;

/**
 * @struct tisci_msg_req_get_clock_parent
 * @brief Get the clock parent
 * @param hdr:	Generic Header
 * @param dev_id:	Device identifier this request is for
 * @param clk_id:	Clock identifier for the device for this request.
 *		Each device has it's own set of clock inputs. This indexes
 *		which clock input to get the parent for.
 *
 * Request type is TISCI_MSG_GET_CLOCK_PARENT, response is parent information
 */
struct tisci_msg_req_get_clock_parent {
	struct tisci_msg_hdr hdr;
	uint32_t dev_id;
	uint8_t clk_id;
} __packed;

/**
 * @struct tisci_msg_resp_get_clock_parent
 * @brief Response with clock parent
 * @param hdr:	Generic Header
 * @param parent_id:	The current clock parent
 *
 * Response to TISCI_MSG_GET_CLOCK_PARENT.
 */
struct tisci_msg_resp_get_clock_parent {
	struct tisci_msg_hdr hdr;
	uint8_t parent_id;
} __packed;

/**
 * @struct tisci_msg_req_get_clock_num_parents
 * @brief Request to get clock parents
 * @param hdr:	Generic header
 * @param dev_id:	Device identifier this request is for
 * @param clk_id:	Clock identifier for the device for this request.
 *
 * This request provides information about how many clock parent options
 * are available for a given clock to a device. This is typically used
 * for input clocks.
 *
 * Request type is TISCI_MSG_GET_NUM_CLOCK_PARENTS, response is appropriate
 * message, or NACK in case of inability to satisfy request.
 */
struct tisci_msg_req_get_clock_num_parents {
	struct tisci_msg_hdr hdr;
	uint32_t dev_id;
	uint8_t clk_id;
} __packed;

/**
 * @struct tisci_msg_resp_get_clock_num_parents
 * @brief Response for get clk parents
 * @param hdr:		Generic header
 * @param num_parents:	Number of clock parents
 *
 * Response to TISCI_MSG_GET_NUM_CLOCK_PARENTS
 */
struct tisci_msg_resp_get_clock_num_parents {
	struct tisci_msg_hdr hdr;
	uint8_t num_parents;
} __packed;

/**
 * @struct tisci_msg_req_query_clock_freq
 * @brief Request to query a frequency
 * @param hdr:	Generic Header
 * @param dev_id:	Device identifier this request is for
 * @param min_freq_hz: The minimum allowable frequency in Hz. This is the minimum
 *		allowable programmed frequency and does not account for clock
 *		tolerances and jitter.
 * @param target_freq_hz: The target clock frequency. A frequency will be found
 *		as close to this target frequency as possible.
 * @param max_freq_hz: The maximum allowable frequency in Hz. This is the maximum
 *		allowable programmed frequency and does not account for clock
 *		tolerances and jitter.
 * @param clk_id:	Clock identifier for the device for this request.
 *
 * NOTE: Normally clock frequency management is automatically done by TISCI
 * entity. In case of specific requests, TISCI evaluates capability to achieve
 * requested frequency within provided range and responds with
 * result message.
 *
 * Request type is TISCI_MSG_QUERY_CLOCK_FREQ, response is appropriate message,
 * or NACK in case of inability to satisfy request.
 */
struct tisci_msg_req_query_clock_freq {
	struct tisci_msg_hdr hdr;
	uint32_t dev_id;
	uint64_t min_freq_hz;
	uint64_t target_freq_hz;
	uint64_t max_freq_hz;
	uint8_t clk_id;
} __packed;

/**
 * @struct tisci_msg_resp_query_clock_freq
 * @brief Response to a clock frequency query
 * @param hdr:	Generic Header
 * @param freq_hz:	Frequency that is the best match in Hz.
 *
 * Response to request type TISCI_MSG_QUERY_CLOCK_FREQ. NOTE: if the request
 * cannot be satisfied, the message will be of type NACK.
 */
struct tisci_msg_resp_query_clock_freq {
	struct tisci_msg_hdr hdr;
	uint64_t freq_hz;
} __packed;

/**
 * @struct tisci_msg_req_set_clock_freq
 * @brief Request to setup a clock frequency
 * @param hdr:	Generic Header
 * @param dev_id:	Device identifier this request is for
 * @param min_freq_hz: The minimum allowable frequency in Hz. This is the minimum
 *		allowable programmed frequency and does not account for clock
 *		tolerances and jitter.
 * @param target_freq_hz: The target clock frequency. The clock will be programmed
 *		at a rate as close to this target frequency as possible.
 * @param max_freq_hz: The maximum allowable frequency in Hz. This is the maximum
 *		allowable programmed frequency and does not account for clock
 *		tolerances and jitter.
 * @param clk_id:	Clock identifier for the device for this request.
 *
 * NOTE: Normally clock frequency management is automatically done by TISCI
 * entity. In case of specific requests, TISCI evaluates capability to achieve
 * requested range and responds with success/failure message.
 *
 * This sets the desired frequency for a clock within an allowable
 * range. This message will fail on an enabled clock unless
 * MSG_FLAG_CLOCK_ALLOW_FREQ_CHANGE is set for the clock. Additionally,
 * if other clocks have their frequency modified due to this message,
 * they also must have the MSG_FLAG_CLOCK_ALLOW_FREQ_CHANGE or be disabled.
 *
 * Calling set frequency on a clock input to the SoC pseudo-device will
 * inform the PMMC of that clock's frequency. Setting a frequency of
 * zero will indicate the clock is disabled.
 *
 * Calling set frequency on clock outputs from the SoC pseudo-device will
 * function similarly to setting the clock frequency on a device.
 *
 * Request type is TISCI_MSG_SET_CLOCK_FREQ, response is a generic ACK/NACK
 * message.
 */
struct tisci_msg_req_set_clock_freq {
	struct tisci_msg_hdr hdr;
	uint32_t dev_id;
	uint64_t min_freq_hz;
	uint64_t target_freq_hz;
	uint64_t max_freq_hz;
	uint8_t clk_id;
} __packed;

/**
 * @struct tisci_msg_resp_set_clock_freq
 * @brief Response to set clock frequency (generic ACK/NACK)
 */
struct tisci_msg_resp_set_clock_freq {
	struct tisci_msg_hdr hdr;
} __packed;

/**
 * @struct tisci_msg_req_get_clock_freq
 * @brief Request to get the clock frequency
 * @param hdr:	Generic Header
 * @param dev_id:	Device identifier this request is for
 * @param clk_id:	Clock identifier for the device for this request.
 *
 * NOTE: Normally clock frequency management is automatically done by TISCI
 * entity. In some cases, clock frequencies are configured by host.
 *
 * Request type is TISCI_MSG_GET_CLOCK_FREQ, responded with clock frequency
 * that the clock is currently at.
 */
struct tisci_msg_req_get_clock_freq {
	struct tisci_msg_hdr hdr;
	uint32_t dev_id;
	uint8_t clk_id;
} __packed;

/**
 * @struct tisci_msg_resp_get_clock_freq
 * @brief Response of clock frequency request
 * @param hdr:	Generic Header
 * @param freq_hz:	Frequency that the clock is currently on, in Hz.
 *
 * Response to request type TISCI_MSG_GET_CLOCK_FREQ.
 */
struct tisci_msg_resp_get_clock_freq {
	struct tisci_msg_hdr hdr;
	uint64_t freq_hz;
} __packed;

#define TISCI_IRQ_SECONDARY_HOST_INVALID 0xff

/**
 * @struct tisci_msg_req_get_resource_range
 * @brief Request to get a host's assigned
 *					      range of resources.
 * @param hdr:		Generic Header
 * @param type:		Unique resource assignment type
 * @param subtype:		Resource assignment subtype within the resource type.
 * @param secondary_host:	Host processing entity to which the resources are
 *			allocated. This is required only when the destination
 *			host id id different from ti sci interface host id,
 *			else TISCI_IRQ_SECONDARY_HOST_INVALID can be passed.
 *
 * Request type is TISCI_MSG_GET_RESOURCE_RANGE. Responded with requested
 * resource range which is of type TISCI_MSG_GET_RESOURCE_RANGE.
 */
struct tisci_msg_req_get_resource_range {
	struct tisci_msg_hdr hdr;
#define MSG_RM_RESOURCE_TYPE_MASK    GENMASK(9, 0)
#define MSG_RM_RESOURCE_SUBTYPE_MASK GENMASK(5, 0)
	uint16_t type;
	uint8_t subtype;
	uint8_t secondary_host;
} __packed;

/**
 * @struct tisci_msg_resp_get_resource_range
 * @brief Response to resource get range.
 * @param hdr:		Generic Header
 * @param range_start:	Start index of the resource range.
 * @param range_num:		Number of resources in the range.
 *
 * Response to request TISCI_MSG_GET_RESOURCE_RANGE.
 */
struct tisci_msg_resp_get_resource_range {
	struct tisci_msg_hdr hdr;
	uint16_t range_start;
	uint16_t range_num;
} __packed;
#define TISCI_ADDR_LOW_MASK   GENMASK64(31, 0)
#define TISCI_ADDR_HIGH_MASK  GENMASK64(63, 32)
#define TISCI_ADDR_HIGH_SHIFT 32

/**
 * @struct tisci_msg_req_proc_request
 * @brief Request a processor
 *
 * @param hdr:		Generic Header
 * @param processor_id:	ID of processor
 *
 * Request type is TISCI_MSG_PROC_REQUEST, response is a generic ACK/NACK
 * message.
 */
struct tisci_msg_req_proc_request {
	struct tisci_msg_hdr hdr;
	uint8_t processor_id;
} __packed;

/**
 * @struct tisci_msg_resp_proc_request
 * @brief Response to processor request (generic ACK/NACK)
 */
struct tisci_msg_resp_proc_request {
	struct tisci_msg_hdr hdr;
} __packed;

/**
 * @struct tisci_msg_req_proc_release
 * @brief Release a processor
 *
 * @param hdr:		Generic Header
 * @param processor_id:	ID of processor
 *
 * Request type is TISCI_MSG_PROC_RELEASE, response is a generic ACK/NACK
 * message.
 */
struct tisci_msg_req_proc_release {
	struct tisci_msg_hdr hdr;
	uint8_t processor_id;
} __packed;

/**
 * @struct tisci_msg_resp_proc_release
 * @brief Response to processor release (generic ACK/NACK)
 */
struct tisci_msg_resp_proc_release {
	struct tisci_msg_hdr hdr;
} __packed;

/**
 * @struct tisci_msg_req_proc_handover
 * @brief Handover a processor to a host
 *
 * @param hdr:		Generic Header
 * @param processor_id:	ID of processor
 * @param host_id:		New Host we want to give control to
 *
 * Request type is TISCI_MSG_PROC_HANDOVER, response is a generic ACK/NACK
 * message.
 */
struct tisci_msg_req_proc_handover {
	struct tisci_msg_hdr hdr;
	uint8_t processor_id;
	uint8_t host_id;
} __packed;

/**
 * @struct tisci_msg_resp_proc_handover
 * @brief Response to processor handover (generic ACK/NACK)
 */
struct tisci_msg_resp_proc_handover {
	struct tisci_msg_hdr hdr;
} __packed;

/* A53 Config Flags */
#define PROC_BOOT_CFG_FLAG_ARMV8_DBG_EN      0x00000001
#define PROC_BOOT_CFG_FLAG_ARMV8_DBG_NIDEN   0x00000002
#define PROC_BOOT_CFG_FLAG_ARMV8_DBG_SPIDEN  0x00000004
#define PROC_BOOT_CFG_FLAG_ARMV8_DBG_SPNIDEN 0x00000008
#define PROC_BOOT_CFG_FLAG_ARMV8_AARCH32     0x00000100

/* R5 Config Flags */
#define PROC_BOOT_CFG_FLAG_R5_DBG_EN      0x00000001
#define PROC_BOOT_CFG_FLAG_R5_DBG_NIDEN   0x00000002
#define PROC_BOOT_CFG_FLAG_R5_LOCKSTEP    0x00000100
#define PROC_BOOT_CFG_FLAG_R5_TEINIT      0x00000200
#define PROC_BOOT_CFG_FLAG_R5_NMFI_EN     0x00000400
#define PROC_BOOT_CFG_FLAG_R5_TCM_RSTBASE 0x00000800
#define PROC_BOOT_CFG_FLAG_R5_BTCM_EN     0x00001000
#define PROC_BOOT_CFG_FLAG_R5_ATCM_EN     0x00002000

/**
 * @struct tisci_msg_req_set_proc_boot_config
 * @brief Set Processor boot configuration
 * @param hdr:		Generic Header
 * @param processor_id:	ID of processor
 * @param bootvector_low:	Lower 32bit (Little Endian) of boot vector
 * @param bootvector_high:	Higher 32bit (Little Endian) of boot vector
 * @param config_flags_set:	Optional Processor specific Config Flags to set.
 *			Setting a bit here implies required bit sets to 1.
 * @param config_flags_clear:	Optional Processor specific Config Flags to clear.
 *			Setting a bit here implies required bit gets cleared.
 *
 * Request type is TISCI_MSG_SET_PROC_BOOT_CONFIG, response is a generic
 * ACK/NACK message.
 */
struct tisci_msg_req_set_proc_boot_config {
	struct tisci_msg_hdr hdr;
	uint8_t processor_id;
	uint32_t bootvector_low;
	uint32_t bootvector_high;
	uint32_t config_flags_set;
	uint32_t config_flags_clear;
} __packed;

/**
 * @struct tisci_msg_resp_set_proc_boot_config
 * @brief Response to set processor boot config (generic ACK/NACK)
 */
struct tisci_msg_resp_set_proc_boot_config {
	struct tisci_msg_hdr hdr;
} __packed;

/* R5 Control Flags */
#define PROC_BOOT_CTRL_FLAG_R5_CORE_HALT 0x00000001

/**
 * @struct tisci_msg_req_set_proc_boot_ctrl
 * @brief Set Processor boot control flags
 * @param hdr:		Generic Header
 * @param processor_id:	ID of processor
 * @param control_flags_set:	Optional Processor specific Control Flags to set.
 *			Setting a bit here implies required bit sets to 1.
 * @param control_flags_clear:Optional Processor specific Control Flags to clear.
 *			Setting a bit here implies required bit gets cleared.
 *
 * Request type is TISCI_MSG_SET_PROC_BOOT_CTRL, response is a generic ACK/NACK
 * message.
 */
struct tisci_msg_req_set_proc_boot_ctrl {
	struct tisci_msg_hdr hdr;
	uint8_t processor_id;
	uint32_t control_flags_set;
	uint32_t control_flags_clear;
} __packed;

/**
 * @struct tisci_msg_resp_set_proc_boot_ctrl
 * @brief Response to set processor boot control (generic ACK/NACK)
 */
struct tisci_msg_resp_set_proc_boot_ctrl {
	struct tisci_msg_hdr hdr;
} __packed;

/**
 * @struct tisci_msg_req_proc_auth_start_image
 * @brief Authenticate and start image
 * @param hdr:		Generic Header
 * @param cert_addr_low:	Lower 32bit (Little Endian) of certificate
 * @param cert_addr_high:	Higher 32bit (Little Endian) of certificate
 *
 * Request type is TISCI_MSG_PROC_AUTH_BOOT_IMAGE, response is a generic
 * ACK/NACK message.
 */
struct tisci_msg_req_proc_auth_boot_image {
	struct tisci_msg_hdr hdr;
	uint32_t cert_addr_low;
	uint32_t cert_addr_high;
} __packed;

struct tisci_msg_resp_proc_auth_boot_image {
	struct tisci_msg_hdr hdr;
	uint32_t image_addr_low;
	uint32_t image_addr_high;
	uint32_t image_size;
} __packed;

/**
 * @struct tisci_msg_req_get_proc_boot_status
 * @brief Get processor boot status
 * @param hdr:		Generic Header
 * @param processor_id:	ID of processor
 *
 * Request type is TISCI_MSG_GET_PROC_BOOT_STATUS, response is appropriate
 * message, or NACK in case of inability to satisfy request.
 */
struct tisci_msg_req_get_proc_boot_status {
	struct tisci_msg_hdr hdr;
	uint8_t processor_id;
} __packed;

/* ARMv8 Status Flags */
#define PROC_BOOT_STATUS_FLAG_ARMV8_WFE 0x00000001
#define PROC_BOOT_STATUS_FLAG_ARMV8_WFI 0x00000002

/* R5 Status Flags */
#define PROC_BOOT_STATUS_FLAG_R5_WFE                0x00000001
#define PROC_BOOT_STATUS_FLAG_R5_WFI                0x00000002
#define PROC_BOOT_STATUS_FLAG_R5_CLK_GATED          0x00000004
#define PROC_BOOT_STATUS_FLAG_R5_LOCKSTEP_PERMITTED 0x00000100

/**
 * @struct tisci_msg_resp_get_proc_boot_status
 * @brief Processor boot status response
 * @param hdr:		Generic Header
 * @param processor_id:	ID of processor
 * @param bootvector_low:	Lower 32bit (Little Endian) of boot vector
 * @param bootvector_high:	Higher 32bit (Little Endian) of boot vector
 * @param config_flags:	Optional Processor specific Config Flags set.
 * @param control_flags:	Optional Processor specific Control Flags.
 * @param status_flags:	Optional Processor specific Status Flags set.
 *
 * Response to TISCI_MSG_GET_PROC_BOOT_STATUS.
 */
struct tisci_msg_resp_get_proc_boot_status {
	struct tisci_msg_hdr hdr;
	uint8_t processor_id;
	uint32_t bootvector_low;
	uint32_t bootvector_high;
	uint32_t config_flags;
	uint32_t control_flags;
	uint32_t status_flags;
} __packed;

/**
 * @struct tisci_msg_req_wait_proc_boot_status
 * @brief Wait for a processor boot status
 * @param hdr:			Generic Header
 * @param processor_id:		ID of processor
 * @param num_wait_iterations:	Total number of iterations we will check before
 *				we will timeout and give up
 * @param num_match_iterations:	How many iterations should we have continued
 *				status to account for status bits glitching.
 *				This is to make sure that match occurs for
 *				consecutive checks. This implies that the
 *				worst case should consider that the stable
 *				time should at the worst be num_wait_iterations
 *				num_match_iterations to prevent timeout.
 * @param delay_per_iteration_us:	Specifies how long to wait (in micro seconds)
 *				between each status checks. This is the minimum
 *				duration, and overhead of register reads and
 *				checks are on top of this and can vary based on
 *				varied conditions.
 * @param delay_before_iterations_us:	Specifies how long to wait (in micro seconds)
 *				before the very first check in the first
 *				iteration of status check loop. This is the
 *				minimum duration, and overhead of register
 *				reads and checks are.
 * @param status_flags_1_set_all_wait:If non-zero, Specifies that all bits of the
 *				status matching this field requested MUST be 1.
 * @param status_flags_1_set_any_wait:If non-zero, Specifies that at least one of the
 *				bits matching this field requested MUST be 1.
 * @param status_flags_1_clr_all_wait:If non-zero, Specifies that all bits of the
 *				status matching this field requested MUST be 0.
 * @param status_flags_1_clr_any_wait:If non-zero, Specifies that at least one of the
 *				bits matching this field requested MUST be 0.
 *
 * Request type is TISCI_MSG_WAIT_PROC_BOOT_STATUS, response is appropriate
 * message, or NACK in case of inability to satisfy request.
 */
struct tisci_msg_req_wait_proc_boot_status {
	struct tisci_msg_hdr hdr;
	uint8_t processor_id;
	uint8_t num_wait_iterations;
	uint8_t num_match_iterations;
	uint8_t delay_per_iteration_us;
	uint8_t delay_before_iterations_us;
	uint32_t status_flags_1_set_all_wait;
	uint32_t status_flags_1_set_any_wait;
	uint32_t status_flags_1_clr_all_wait;
	uint32_t status_flags_1_clr_any_wait;
} __packed;

/**
 * @struct tisci_msg_rm_ring_cfg_req
 * @brief Configure a Navigator Subsystem ring
 *
 * Configures the non-real-time registers of a Navigator Subsystem ring.
 * @param hdr:	Generic Header
 * @param valid_params: Bitfield defining validity of ring configuration parameters.
 *	The ring configuration fields are not valid, and will not be used for
 *	ring configuration, if their corresponding valid bit is zero.
 *	Valid bit usage:
 *	0 - Valid bit for tisci_msg_rm_ring_cfg_req addr_lo
 *	1 - Valid bit for tisci_msg_rm_ring_cfg_req addr_hi
 *	2 - Valid bit for tisci_msg_rm_ring_cfg_req count
 *	3 - Valid bit for tisci_msg_rm_ring_cfg_req mode
 *	4 - Valid bit for tisci_msg_rm_ring_cfg_req size
 *	5 - Valid bit for tisci_msg_rm_ring_cfg_req order_id
 * @param nav_id: Device ID of Navigator Subsystem from which the ring is allocated
 * @param index: ring index to be configured.
 * @param addr_lo: 32 LSBs of ring base address to be programmed into the ring's
 *	RING_BA_LO register
 * @param addr_hi: 16 MSBs of ring base address to be programmed into the ring's
 *	RING_BA_HI register.
 * @param count: Number of ring elements. Must be even if mode is CREDENTIALS or QM
 *	modes.
 * @param mode: Specifies the mode the ring is to be configured.
 * @param size: Specifies encoded ring element size. To calculate the encoded size use
 *	the formula (log2(size_bytes) - 2), where size_bytes cannot be
 *	greater than 256.
 * @param order_id: Specifies the ring's bus order ID.
 */
struct tisci_msg_rm_ring_cfg_req {
	struct tisci_msg_hdr hdr;
	uint32_t valid_params;
	uint16_t nav_id;
	uint16_t index;
	uint32_t addr_lo;
	uint32_t addr_hi;
	uint32_t count;
	uint8_t mode;
	uint8_t size;
	uint8_t order_id;
} __packed;

/**
 * @struct tisci_msg_rm_ring_cfg_resp
 * @brief Response to configuring a ring.
 *
 * @param hdr:	Generic Header
 */
struct tisci_msg_rm_ring_cfg_resp {
	struct tisci_msg_hdr hdr;
} __packed;

/**
 * @struct tisci_msg_rm_ring_get_cfg_req
 * @brief Get RA ring's configuration
 *
 * Gets the configuration of the non-real-time register fields of a ring.  The
 * host, or a supervisor of the host, who owns the ring must be the requesting
 * host.  The values of the non-real-time registers are returned in
 * tisci_msg_rm_ring_get_cfg_resp.
 *
 * @param hdr: Generic Header
 * @param nav_id: Device ID of Navigator Subsystem from which the ring is allocated
 * @param index: ring index.
 */
struct tisci_msg_rm_ring_get_cfg_req {
	struct tisci_msg_hdr hdr;
	uint16_t nav_id;
	uint16_t index;
} __packed;

/**
 * @struct tisci_msg_rm_ring_get_cfg_resp
 * @brief  Ring get configuration response
 *
 * Response received by host processor after RM has handled
 * @ref tisci_msg_rm_ring_get_cfg_req. The response contains the ring's
 * non-real-time register values.
 *
 * @param hdr: Generic Header
 * @param addr_lo: Ring 32 LSBs of base address
 * @param addr_hi: Ring 16 MSBs of base address.
 * @param count: Ring number of elements.
 * @param mode: Ring mode.
 * @param size: encoded Ring element size
 * @param order_id: ing order ID.
 */
struct tisci_msg_rm_ring_get_cfg_resp {
	struct tisci_msg_hdr hdr;
	uint32_t addr_lo;
	uint32_t addr_hi;
	uint32_t count;
	uint8_t mode;
	uint8_t size;
	uint8_t order_id;
} __packed;

/**
 * @struct tisci_msg_psil_pair_req
 * @brief Pairs a PSI-L source thread to a destination
 *				 thread
 * @param hdr:	Generic Header
 * @param nav_id:	SoC Navigator Subsystem device ID whose PSI-L config proxy is
 *		used to pair the source and destination threads.
 * @param src_thread:	PSI-L source thread ID within the PSI-L System thread map.
 *
 * UDMAP transmit channels mapped to source threads will have their
 * TCHAN_THRD_ID register programmed with the destination thread if the pairing
 * is successful.

 * @param dst_thread: PSI-L destination thread ID within the PSI-L System thread map.
 * PSI-L destination threads start at index 0x8000.  The request is NACK'd if
 * the destination thread is not greater than or equal to 0x8000.
 *
 * UDMAP receive channels mapped to destination threads will have their
 * RCHAN_THRD_ID register programmed with the source thread if the pairing
 * is successful.
 *
 * Request type is TISCI_MSG_RM_PSIL_PAIR, response is a generic ACK or NACK
 * message.
 */
struct tisci_msg_psil_pair_req {
	struct tisci_msg_hdr hdr;
	uint32_t nav_id;
	uint32_t src_thread;
	uint32_t dst_thread;
} __packed;

/**
 * @struct tisci_msg_psil_pair_resp
 * @brief Response to PSI-L thread pair request (generic ACK/NACK)
 * @param hdr: Generic Header
 */
struct tisci_msg_psil_pair_resp {
	struct tisci_msg_hdr hdr;
} __packed;

/**
 * @struct tisci_msg_psil_unpair_req
 * @brief Unpairs a PSI-L source thread from a destination thread
 * @param hdr:	Generic Header
 * @param nav_id:	SoC Navigator Subsystem device ID whose PSI-L config proxy is
 *		used to unpair the source and destination threads.
 * @param src_thread:	PSI-L source thread ID within the PSI-L System thread map.
 *
 * UDMAP transmit channels mapped to source threads will have their
 * TCHAN_THRD_ID register cleared if the unpairing is successful.
 *
 * @param dst_thread: PSI-L destination thread ID within the PSI-L System thread map.
 * PSI-L destination threads start at index 0x8000.  The request is NACK'd if
 * the destination thread is not greater than or equal to 0x8000.
 *
 * UDMAP receive channels mapped to destination threads will have their
 * RCHAN_THRD_ID register cleared if the unpairing is successful.
 *
 * Request type is TISCI_MSG_RM_PSIL_UNPAIR, response is a generic ACK or NACK
 * message.
 */
struct tisci_msg_psil_unpair_req {
	struct tisci_msg_hdr hdr;
	uint32_t nav_id;
	uint32_t src_thread;
	uint32_t dst_thread;
} __packed;

/**
 * @struct tisci_msg_psil_unpair_resp
 * @brief Response to PSI-L thread unpair request (generic ACK/NACK)
 * @param hdr: Generic Header
 */
struct tisci_msg_psil_unpair_resp {
	struct tisci_msg_hdr hdr;
} __packed;

/**
 * Configures a Navigator Subsystem UDMAP transmit channel
 *
 * Configures the non-real-time registers of a Navigator Subsystem UDMAP
 * transmit channel.  The channel index must be assigned to the host defined
 * in the TISCI header via the RM board configuration resource assignment
 * range list.
 *
 * @param hdr: Generic Header
 *
 * @param valid_params: Bitfield defining validity of tx channel configuration
 * parameters. The tx channel configuration fields are not valid, and will not
 * be used for ch configuration, if their corresponding valid bit is zero.
 * Valid bit usage:
 *    0 - Valid bit for tisci_msg_rm_udmap_tx_ch_cfg::tx_pause_on_err
 *    1 - Valid bit for tisci_msg_rm_udmap_tx_ch_cfg::tx_atype
 *    2 - Valid bit for tisci_msg_rm_udmap_tx_ch_cfg::tx_chan_type
 *    3 - Valid bit for tisci_msg_rm_udmap_tx_ch_cfg::tx_fetch_size
 *    4 - Valid bit for tisci_msg_rm_udmap_tx_ch_cfg::txcq_qnum
 *    5 - Valid bit for tisci_msg_rm_udmap_tx_ch_cfg::tx_priority
 *    6 - Valid bit for tisci_msg_rm_udmap_tx_ch_cfg::tx_qos
 *    7 - Valid bit for tisci_msg_rm_udmap_tx_ch_cfg::tx_orderid
 *    8 - Valid bit for tisci_msg_rm_udmap_tx_ch_cfg::tx_sched_priority
 *    9 - Valid bit for tisci_msg_rm_udmap_tx_ch_cfg::tx_filt_einfo
 *   10 - Valid bit for tisci_msg_rm_udmap_tx_ch_cfg::tx_filt_pswords
 *   11 - Valid bit for tisci_msg_rm_udmap_tx_ch_cfg::tx_supr_tdpkt
 *   12 - Valid bit for tisci_msg_rm_udmap_tx_ch_cfg::tx_credit_count
 *   13 - Valid bit for tisci_msg_rm_udmap_tx_ch_cfg::fdepth
 *   14 - Valid bit for tisci_msg_rm_udmap_tx_ch_cfg::tx_burst_size
 *   15 - Valid bit for tisci_msg_rm_udmap_tx_ch_cfg::tx_tdtype
 *   16 - Valid bit for tisci_msg_rm_udmap_tx_ch_cfg::extended_ch_type
 *
 * @param nav_id: SoC device ID of Navigator Subsystem where tx channel is located
 *
 * @param index: UDMAP transmit channel index.
 *
 * @param tx_pause_on_err: UDMAP transmit channel pause on error configuration to
 * be programmed into the tx_pause_on_err field of the channel's TCHAN_TCFG
 * register.
 *
 * @param tx_filt_einfo: UDMAP transmit channel extended packet information passing
 * configuration to be programmed into the tx_filt_einfo field of the
 * channel's TCHAN_TCFG register.
 *
 * @param tx_filt_pswords: UDMAP transmit channel protocol specific word passing
 * configuration to be programmed into the tx_filt_pswords field of the
 * channel's TCHAN_TCFG register.
 *
 * @param tx_atype: UDMAP transmit channel non Ring Accelerator access pointer
 * interpretation configuration to be programmed into the tx_atype field of
 * the channel's TCHAN_TCFG register.
 *
 * @param tx_chan_type: UDMAP transmit channel functional channel type and work
 * passing mechanism configuration to be programmed into the tx_chan_type
 * field of the channel's TCHAN_TCFG register.
 *
 * @param tx_supr_tdpkt: UDMAP transmit channel teardown packet generation suppression
 * configuration to be programmed into the tx_supr_tdpkt field of the channel's
 * TCHAN_TCFG register.
 *
 * @param tx_fetch_size: UDMAP transmit channel number of 32-bit descriptor words to
 * fetch configuration to be programmed into the tx_fetch_size field of the
 * channel's TCHAN_TCFG register.  The user must make sure to set the maximum
 * word count that can pass through the channel for any allowed descriptor type.
 *
 * @param tx_credit_count: UDMAP transmit channel transfer request credit count
 * configuration to be programmed into the count field of the TCHAN_TCREDIT
 * register.  Specifies how many credits for complete TRs are available.
 *
 * @param txcq_qnum: UDMAP transmit channel completion queue configuration to be
 * programmed into the txcq_qnum field of the TCHAN_TCQ register. The specified
 * completion queue must be assigned to the host, or a subordinate of the host,
 * requesting configuration of the transmit channel.
 *
 * @param tx_priority: UDMAP transmit channel transmit priority value to be programmed
 * into the priority field of the channel's TCHAN_TPRI_CTRL register.
 *
 * @param tx_qos: UDMAP transmit channel transmit qos value to be programmed into the
 * qos field of the channel's TCHAN_TPRI_CTRL register.
 *
 * @param tx_orderid: UDMAP transmit channel bus order id value to be programmed into
 * the orderid field of the channel's TCHAN_TPRI_CTRL register.
 *
 * @param fdepth: UDMAP transmit channel FIFO depth configuration to be programmed
 * into the fdepth field of the TCHAN_TFIFO_DEPTH register. Sets the number of
 * Tx FIFO bytes which are allowed to be stored for the channel. Check the UDMAP
 * section of the TRM for restrictions regarding this parameter.
 *
 * @param tx_sched_priority: UDMAP transmit channel tx scheduling priority
 * configuration to be programmed into the priority field of the channel's
 * TCHAN_TST_SCHED register.
 *
 * @param tx_burst_size: UDMAP transmit channel burst size configuration to be
 * programmed into the tx_burst_size field of the TCHAN_TCFG register.
 *
 * @param tx_tdtype: UDMAP transmit channel teardown type configuration to be
 * programmed into the tdtype field of the TCHAN_TCFG register:
 * 0 - Return immediately
 * 1 - Wait for completion message from remote peer
 *
 * @param extended_ch_type: Valid for BCDMA.
 * 0 - the channel is split tx channel (tchan)
 * 1 - the channel is block copy channel (bchan)
 */
struct tisci_msg_rm_udmap_tx_ch_cfg_req {
	struct tisci_msg_hdr hdr;
	uint32_t valid_params;
	uint16_t nav_id;
	uint16_t index;
	uint8_t tx_pause_on_err;
	uint8_t tx_filt_einfo;
	uint8_t tx_filt_pswords;
	uint8_t tx_atype;
	uint8_t tx_chan_type;
	uint8_t tx_supr_tdpkt;
	uint16_t tx_fetch_size;
	uint8_t tx_credit_count;
	uint16_t txcq_qnum;
	uint8_t tx_priority;
	uint8_t tx_qos;
	uint8_t tx_orderid;
	uint16_t fdepth;
	uint8_t tx_sched_priority;
	uint8_t tx_burst_size;
	uint8_t tx_tdtype;
	uint8_t extended_ch_type;
} __packed;

/**
 * @struct tisci_msg_rm_udmap_tx_ch_cfg_resp
 * @brief Response to UDMAP transmit channel configuration request (generic ACK/NACK)
 * @param hdr: Generic Header
 */
struct tisci_msg_rm_udmap_tx_ch_cfg_resp {
	struct tisci_msg_hdr hdr;
} __packed;

/**
 * Configures a Navigator Subsystem UDMAP receive channel
 *
 * Configures the non-real-time registers of a Navigator Subsystem UDMAP
 * receive channel.  The channel index must be assigned to the host defined
 * in the TISCI header via the RM board configuration resource assignment
 * range list.
 *
 * @param hdr: Generic Header
 *
 * @param valid_params: Bitfield defining validity of rx channel configuration
 * parameters.
 * The rx channel configuration fields are not valid, and will not be used for
 * ch configuration, if their corresponding valid bit is zero.
 * Valid bit usage:
 *    0 - Valid bit for @ref tisci_msg_rm_udmap_rx_ch_cfg_req::rx_pause_on_err
 *    1 - Valid bit for @ref tisci_msg_rm_udmap_rx_ch_cfg_req::rx_atype
 *    2 - Valid bit for @ref tisci_msg_rm_udmap_rx_ch_cfg_req::rx_chan_type
 *    3 - Valid bit for @ref tisci_msg_rm_udmap_rx_ch_cfg_req::rx_fetch_size
 *    4 - Valid bit for @ref tisci_msg_rm_udmap_rx_ch_cfg_req::rxcq_qnum
 *    5 - Valid bit for @ref tisci_msg_rm_udmap_rx_ch_cfg_req::rx_priority
 *    6 - Valid bit for @ref tisci_msg_rm_udmap_rx_ch_cfg_req::rx_qos
 *    7 - Valid bit for @ref tisci_msg_rm_udmap_rx_ch_cfg_req::rx_orderid
 *    8 - Valid bit for @ref tisci_msg_rm_udmap_rx_ch_cfg_req::rx_sched_priority
 *    9 - Valid bit for @ref tisci_msg_rm_udmap_rx_ch_cfg_req::flowid_start
 *   10 - Valid bit for @ref tisci_msg_rm_udmap_rx_ch_cfg_req::flowid_cnt
 *   11 - Valid bit for @ref tisci_msg_rm_udmap_rx_ch_cfg_req::rx_ignore_short
 *   12 - Valid bit for @ref tisci_msg_rm_udmap_rx_ch_cfg_req::rx_ignore_long
 *
 * @param nav_id: SoC device ID of Navigator Subsystem where rx channel is located
 *
 * @param index: UDMAP receive channel index.
 *
 * @param rx_fetch_size: UDMAP receive channel number of 32-bit descriptor words to
 * fetch configuration to be programmed into the rx_fetch_size field of the
 * channel's RCHAN_RCFG register.
 *
 * @param rxcq_qnum: UDMAP receive channel completion queue configuration to be
 * programmed into the rxcq_qnum field of the RCHAN_RCQ register.
 * The specified completion queue must be assigned to the host, or a subordinate
 * of the host, requesting configuration of the receive channel.
 *
 * @param rx_priority: UDMAP receive channel receive priority value to be programmed
 * into the priority field of the channel's RCHAN_RPRI_CTRL register.
 *
 * @param rx_qos: UDMAP receive channel receive qos value to be programmed into the
 * qos field of the channel's RCHAN_RPRI_CTRL register.
 *
 * @param rx_orderid: UDMAP receive channel bus order id value to be programmed into
 * the orderid field of the channel's RCHAN_RPRI_CTRL register.
 *
 * @param rx_sched_priority: UDMAP receive channel rx scheduling priority
 * configuration to be programmed into the priority field of the channel's
 * RCHAN_RST_SCHED register.
 *
 * @param flowid_start: UDMAP receive channel additional flows starting index
 * configuration to program into the flow_start field of the RCHAN_RFLOW_RNG
 * register. Specifies the starting index for flow IDs the receive channel is to
 * make use of beyond the default flow. flowid_start and @ref flowid_cnt must be
 * set as valid and configured together. The starting flow ID set by
 * @ref flowid_cnt must be a flow index within the Navigator Subsystem's subset
 * of flows beyond the default flows statically mapped to receive channels.
 * The additional flows must be assigned to the host, or a subordinate of the
 * host, requesting configuration of the receive channel.
 *
 * @param flowid_cnt: UDMAP receive channel additional flows count configuration to
 * program into the flowid_cnt field of the RCHAN_RFLOW_RNG register.
 * This field specifies how many flow IDs are in the additional contiguous range
 * of legal flow IDs for the channel.  @ref flowid_start and flowid_cnt must be
 * set as valid and configured together. Disabling the valid_params field bit
 * for flowid_cnt indicates no flow IDs other than the default are to be
 * allocated and used by the receive channel. @ref flowid_start plus flowid_cnt
 * cannot be greater than the number of receive flows in the receive channel's
 * Navigator Subsystem.  The additional flows must be assigned to the host, or a
 * subordinate of the host, requesting configuration of the receive channel.
 *
 * @param rx_pause_on_err: UDMAP receive channel pause on error configuration to be
 * programmed into the rx_pause_on_err field of the channel's RCHAN_RCFG
 * register.
 *
 * @param rx_atype: UDMAP receive channel non Ring Accelerator access pointer
 * interpretation configuration to be programmed into the rx_atype field of the
 * channel's RCHAN_RCFG register.
 *
 * @param rx_chan_type: UDMAP receive channel functional channel type and work passing
 * mechanism configuration to be programmed into the rx_chan_type field of the
 * channel's RCHAN_RCFG register.
 *
 * @param rx_ignore_short: UDMAP receive channel short packet treatment configuration
 * to be programmed into the rx_ignore_short field of the RCHAN_RCFG register.
 *
 * @param rx_ignore_long: UDMAP receive channel long packet treatment configuration to
 * be programmed into the rx_ignore_long field of the RCHAN_RCFG register.
 */
struct tisci_msg_rm_udmap_rx_ch_cfg_req {
	struct tisci_msg_hdr hdr;
	uint32_t valid_params;
	uint16_t nav_id;
	uint16_t index;
	uint16_t rx_fetch_size;
	uint16_t rxcq_qnum;
	uint8_t rx_priority;
	uint8_t rx_qos;
	uint8_t rx_orderid;
	uint8_t rx_sched_priority;
	uint16_t flowid_start;
	uint16_t flowid_cnt;
	uint8_t rx_pause_on_err;
	uint8_t rx_atype;
	uint8_t rx_chan_type;
	uint8_t rx_ignore_short;
	uint8_t rx_ignore_long;
} __packed;

/**
 * @struct tisci_msg_rm_udmap_rx_ch_cfg_resp
 * @brief Response to UDMAP receive channel configuration request (generic ACK/NACK)
 * @param hdr: Generic Header
 */
struct tisci_msg_rm_udmap_rx_ch_cfg_resp {
	struct tisci_msg_hdr hdr;
} __packed;

/**
 * Configures a Navigator Subsystem UDMAP receive flow
 *
 * Configures a Navigator Subsystem UDMAP receive flow's registers.
 * Configuration does not include the flow registers which handle size-based
 * free descriptor queue routing.
 *
 * The flow index must be assigned to the host defined in the TISCI header via
 * the RM board configuration resource assignment range list.
 *
 * @param hdr: Standard TISCI header
 *
 * Valid params
 * Bitfield defining validity of rx flow configuration parameters.  The
 * rx flow configuration fields are not valid, and will not be used for flow
 * configuration, if their corresponding valid bit is zero.  Valid bit usage:
 *     0 - Valid bit for reftisci_msg_rm_udmap_flow_cfg_req::rx_einfo_present
 *     1 - Valid bit for reftisci_msg_rm_udmap_flow_cfg_req::rx_psinfo_present
 *     2 - Valid bit for reftisci_msg_rm_udmap_flow_cfg_req::rx_error_handling
 *     3 - Valid bit for reftisci_msg_rm_udmap_flow_cfg_req::rx_desc_type
 *     4 - Valid bit for reftisci_msg_rm_udmap_flow_cfg_req::rx_sop_offset
 *     5 - Valid bit for reftisci_msg_rm_udmap_flow_cfg_req::rx_dest_qnum
 *     6 - Valid bit for reftisci_msg_rm_udmap_flow_cfg_req::rx_src_tag_hi
 *     7 - Valid bit for reftisci_msg_rm_udmap_flow_cfg_req::rx_src_tag_lo
 *     8 - Valid bit for reftisci_msg_rm_udmap_flow_cfg_req::rx_dest_tag_hi
 *     9 - Valid bit for reftisci_msg_rm_udmap_flow_cfg_req::rx_dest_tag_lo
 *    10 - Valid bit for reftisci_msg_rm_udmap_flow_cfg_req::rx_src_tag_hi_sel
 *    11 - Valid bit for reftisci_msg_rm_udmap_flow_cfg_req::rx_src_tag_lo_sel
 *    12 - Valid bit for reftisci_msg_rm_udmap_flow_cfg_req::rx_dest_tag_hi_sel
 *    13 - Valid bit for reftisci_msg_rm_udmap_flow_cfg_req::rx_dest_tag_lo_sel
 *    14 - Valid bit for reftisci_msg_rm_udmap_flow_cfg_req::rx_fdq0_sz0_qnum
 *    15 - Valid bit for reftisci_msg_rm_udmap_flow_cfg_req::rx_fdq1_sz0_qnum
 *    16 - Valid bit for reftisci_msg_rm_udmap_flow_cfg_req::rx_fdq2_sz0_qnum
 *    17 - Valid bit for reftisci_msg_rm_udmap_flow_cfg_req::rx_fdq3_sz0_qnum
 *    18 - Valid bit for reftisci_msg_rm_udmap_flow_cfg_req::rx_ps_location
 *
 * @param nav_id: SoC device ID of Navigator Subsystem from which the receive flow is
 * allocated
 *
 * @param flow_index: UDMAP receive flow index for non-optional configuration.
 *
 * @param rx_einfo_present:
 * UDMAP receive flow extended packet info present configuration to be
 * programmed into the rx_einfo_present field of the flow's RFLOW_RFA register.
 *
 * @param rx_psinfo_present:
 * UDMAP receive flow PS words present configuration to be programmed into the
 * rx_psinfo_present field of the flow's RFLOW_RFA register.
 *
 * @param rx_error_handling:
 * UDMAP receive flow error handling configuration to be programmed into the
 * rx_error_handling field of the flow's RFLOW_RFA register.
 *
 * @param rx_desc_type:
 * UDMAP receive flow descriptor type configuration to be programmed into the
 * rx_desc_type field field of the flow's RFLOW_RFA register.
 *
 * @param rx_sop_offset:
 * UDMAP receive flow start of packet offset configuration to be programmed
 * into the rx_sop_offset field of the RFLOW_RFA register.  See the UDMAP
 * section of the TRM for more information on this setting.  Valid values for
 * this field are 0-255 bytes.
 *
 * @param rx_dest_qnum:
 * UDMAP receive flow destination queue configuration to be programmed into the
 * rx_dest_qnum field of the flow's RFLOW_RFA register.  The specified
 * destination queue must be valid within the Navigator Subsystem and must be
 * owned by the host, or a subordinate of the host, requesting allocation and
 * configuration of the receive flow.
 *
 * @param rx_src_tag_hi:
 * UDMAP receive flow source tag high byte constant configuration to be
 * programmed into the rx_src_tag_hi field of the flow's RFLOW_RFB register.
 * See the UDMAP section of the TRM for more information on this setting.
 *
 * @param rx_src_tag_lo:
 * UDMAP receive flow source tag low byte constant configuration to be
 * programmed into the rx_src_tag_lo field of the flow's RFLOW_RFB register.
 * See the UDMAP section of the TRM for more information on this setting.
 *
 * @param rx_dest_tag_hi:
 * UDMAP receive flow destination tag high byte constant configuration to be
 * programmed into the rx_dest_tag_hi field of the flow's RFLOW_RFB register.
 * See the UDMAP section of the TRM for more information on this setting.
 *
 * @param rx_dest_tag_lo:
 * UDMAP receive flow destination tag low byte constant configuration to be
 * programmed into the rx_dest_tag_lo field of the flow's RFLOW_RFB register.
 * See the UDMAP section of the TRM for more information on this setting.
 *
 * @param rx_src_tag_hi_sel:
 * UDMAP receive flow source tag high byte selector configuration to be
 * programmed into the rx_src_tag_hi_sel field of the RFLOW_RFC register.  See
 * the UDMAP section of the TRM for more information on this setting.
 *
 * @param rx_src_tag_lo_sel:
 * UDMAP receive flow source tag low byte selector configuration to be
 * programmed into the rx_src_tag_lo_sel field of the RFLOW_RFC register.  See
 * the UDMAP section of the TRM for more information on this setting.
 *
 * @param rx_dest_tag_hi_sel:
 * UDMAP receive flow destination tag high byte selector configuration to be
 * programmed into the rx_dest_tag_hi_sel field of the RFLOW_RFC register.  See
 * the UDMAP section of the TRM for more information on this setting.
 *
 * @param rx_dest_tag_lo_sel:
 * UDMAP receive flow destination tag low byte selector configuration to be
 * programmed into the rx_dest_tag_lo_sel field of the RFLOW_RFC register.  See
 * the UDMAP section of the TRM for more information on this setting.
 *
 * @param rx_fdq0_sz0_qnum:
 * UDMAP receive flow free descriptor queue 0 configuration to be programmed
 * into the rx_fdq0_sz0_qnum field of the flow's RFLOW_RFD register.  See the
 * UDMAP section of the TRM for more information on this setting. The specified
 * free queue must be valid within the Navigator Subsystem and must be owned
 * by the host, or a subordinate of the host, requesting allocation and
 * configuration of the receive flow.
 *
 * @param rx_fdq1_qnum:
 * UDMAP receive flow free descriptor queue 1 configuration to be programmed
 * into the rx_fdq1_qnum field of the flow's RFLOW_RFD register.  See the
 * UDMAP section of the TRM for more information on this setting.  The specified
 * free queue must be valid within the Navigator Subsystem and must be owned
 * by the host, or a subordinate of the host, requesting allocation and
 * configuration of the receive flow.
 *
 * @param rx_fdq2_qnum:
 * UDMAP receive flow free descriptor queue 2 configuration to be programmed
 * into the rx_fdq2_qnum field of the flow's RFLOW_RFE register.  See the
 * UDMAP section of the TRM for more information on this setting.  The specified
 * free queue must be valid within the Navigator Subsystem and must be owned
 * by the host, or a subordinate of the host, requesting allocation and
 * configuration of the receive flow.
 *
 * @param rx_fdq3_qnum:
 * UDMAP receive flow free descriptor queue 3 configuration to be programmed
 * into the rx_fdq3_qnum field of the flow's RFLOW_RFE register.  See the
 * UDMAP section of the TRM for more information on this setting.  The specified
 * free queue must be valid within the Navigator Subsystem and must be owned
 * by the host, or a subordinate of the host, requesting allocation and
 * configuration of the receive flow.
 *
 * @param rx_ps_location:
 * UDMAP receive flow PS words location configuration to be programmed into the
 * rx_ps_location field of the flow's RFLOW_RFA register.
 */
struct tisci_msg_rm_udmap_flow_cfg_req {
	struct tisci_msg_hdr hdr;
	uint32_t valid_params;
	uint16_t nav_id;
	uint16_t flow_index;
	uint8_t rx_einfo_present;
	uint8_t rx_psinfo_present;
	uint8_t rx_error_handling;
	uint8_t rx_desc_type;
	uint16_t rx_sop_offset;
	uint16_t rx_dest_qnum;
	uint8_t rx_src_tag_hi;
	uint8_t rx_src_tag_lo;
	uint8_t rx_dest_tag_hi;
	uint8_t rx_dest_tag_lo;
	uint8_t rx_src_tag_hi_sel;
	uint8_t rx_src_tag_lo_sel;
	uint8_t rx_dest_tag_hi_sel;
	uint8_t rx_dest_tag_lo_sel;
	uint16_t rx_fdq0_sz0_qnum;
	uint16_t rx_fdq1_qnum;
	uint16_t rx_fdq2_qnum;
	uint16_t rx_fdq3_qnum;
	uint8_t rx_ps_location;
} __packed;

/**
 *  Response to configuring a Navigator Subsystem UDMAP receive flow
 *
 * @param hdr: Standard TISCI header
 */
struct tisci_msg_rm_udmap_flow_cfg_resp {
	struct tisci_msg_hdr hdr;
} __packed;

#define FWL_MAX_PRIVID_SLOTS 3U

/**
 * @struct tisci_msg_fwl_set_firewall_region_req
 * @brief Request for configuring the firewall permissions.
 *
 * @param hdr:		Generic Header
 *
 * @param fwl_id:		Firewall ID in question
 * @param region:		Region or channel number to set config info
 *			This field is unused in case of a simple firewall  and must be initialized
 *			to zero.  In case of a region based firewall, this field indicates the
 *			region in question. (index starting from 0) In case of a channel based
 *			firewall, this field indicates the channel in question (index starting
 *			from 0)
 * @param n_permission_regs:	Number of permission registers to set
 * @param control:		Contents of the firewall CONTROL register to set
 * @param permissions:	Contents of the firewall PERMISSION register to set
 * @param start_address:	Contents of the firewall START_ADDRESS register to set
 * @param end_address:	Contents of the firewall END_ADDRESS register to set
 */

struct tisci_msg_fwl_set_firewall_region_req {
	struct tisci_msg_hdr hdr;
	uint16_t fwl_id;
	uint16_t region;
	uint32_t n_permission_regs;
	uint32_t control;
	uint32_t permissions[FWL_MAX_PRIVID_SLOTS];
	uint64_t start_address;
	uint64_t end_address;
} __packed;

/**
 * @struct tisci_msg_fwl_set_firewall_region_resp
 * @brief Response to set firewall region (generic ACK/NACK)
 */
struct tisci_msg_fwl_set_firewall_region_resp {
	struct tisci_msg_hdr hdr;
} __packed;

/**
 * @struct tisci_msg_fwl_get_firewall_region_req
 * @brief Request for retrieving the firewall permissions
 *
 * @param hdr:		Generic Header
 *
 * @param fwl_id:		Firewall ID in question
 * @param region:		Region or channel number to get config info
 *			This field is unused in case of a simple firewall and must be initialized
 *			to zero.  In case of a region based firewall, this field indicates the
 *			region in question (index starting from 0). In case of a channel based
 *			firewall, this field indicates the channel in question (index starting
 *			from 0).
 * @param n_permission_regs:	Number of permission registers to retrieve
 */
struct tisci_msg_fwl_get_firewall_region_req {
	struct tisci_msg_hdr hdr;
	uint16_t fwl_id;
	uint16_t region;
	uint32_t n_permission_regs;
} __packed;

/**
 * @struct tisci_msg_fwl_get_firewall_region_resp
 * @brief Response for retrieving the firewall permissions
 *
 * @param hdr:		Generic Header
 *
 * @param fwl_id:		Firewall ID in question
 * @param region:		Region or channel number to set config info This field is
 *			unused in case of a simple firewall  and must be initialized to zero.  In
 *			case of a region based firewall, this field indicates the region in
 *			question. (index starting from 0) In case of a channel based firewall, this
 *			field indicates the channel in question (index starting from 0)
 * @param n_permission_regs:	Number of permission registers retrieved
 * @param control:		Contents of the firewall CONTROL register
 * @param permissions:	Contents of the firewall PERMISSION registers
 * @param start_address:	Contents of the firewall START_ADDRESS
 * register This is not applicable for
 * channelized firewalls.
 * @param end_address:	Contents of the firewall END_ADDRESS register This is not applicable for
 * channelized firewalls.
 */
struct tisci_msg_fwl_get_firewall_region_resp {
	struct tisci_msg_hdr hdr;
	uint16_t fwl_id;
	uint16_t region;
	uint32_t n_permission_regs;
	uint32_t control;
	uint32_t permissions[FWL_MAX_PRIVID_SLOTS];
	uint64_t start_address;
	uint64_t end_address;
} __packed;

/**
 * @struct tisci_msg_fwl_change_owner_info_req
 * @brief Request for a firewall owner change
 *
 * @param hdr:		Generic Header
 *
 * @param fwl_id:		Firewall ID in question
 * @param region:		Region or channel number if applicable
 * @param owner_index:	New owner index to transfer ownership to
 */
struct tisci_msg_fwl_change_owner_info_req {
	struct tisci_msg_hdr hdr;
	uint16_t fwl_id;
	uint16_t region;
	uint8_t owner_index;
} __packed;

/**
 * @struct tisci_msg_fwl_change_owner_info_resp
 * @brief Response for a firewall owner change
 *
 * @param hdr:		Generic Header
 *
 * @param fwl_id:		Firewall ID specified in request
 * @param region:		Region or channel number specified in request
 * @param owner_index:	Owner index specified in request
 * @param owner_privid:	New owner priv-ID returned by DMSC.
 * @param owner_permission_bits:	New owner permission bits returned by DMSC.
 */

struct tisci_msg_fwl_change_owner_info_resp {
	struct tisci_msg_hdr hdr;
	uint16_t fwl_id;
	uint16_t region;
	uint8_t owner_index;
	uint8_t owner_privid;
	uint16_t owner_permission_bits;
} __packed;

/**
 * @brief Request to set up an interrupt route.
 *
 * Configures peripherals within the interrupt subsystem according to the
 * valid configuration provided.
 *
 * @param hdr                  Standard TISCI header.
 * @param valid_params         Bitfield defining validity of interrupt route set parameters.
 *                             Each bit corresponds to a field's validity.
 * @param src_id               ID of interrupt source peripheral.
 * @param src_index            Interrupt source index within source peripheral.
 * @param dst_id               SoC IR device ID (valid if TISCI_MSG_VALUE_RM_DST_ID_VALID is set).
 * @param dst_host_irq         SoC IR output index (valid if TISCI_MSG_VALUE_RM_DST_HOST_IRQ_VALID
 * is set).
 * @param ia_id                Device ID of interrupt aggregator (valid if
 * TISCI_MSG_VALUE_RM_IA_ID_VALID is set).
 * @param vint                 Virtual interrupt number (valid if TISCI_MSG_VALUE_RM_VINT_VALID is
 * set).
 * @param global_event         Global event mapped to interrupt aggregator (valid if
 * TISCI_MSG_VALUE_RM_GLOBAL_EVENT_VALID is set).
 * @param vint_status_bit_index Virtual interrupt status bit (valid if
 * TISCI_MSG_VALUE_RM_VINT_STATUS_BIT_INDEX_VALID is set).
 * @param secondary_host       Secondary host value (valid if
 * TISCI_MSG_VALUE_RM_SECONDARY_HOST_VALID is set).
 */
struct tisci_msg_rm_irq_set_req {
	struct tisci_msg_hdr hdr;
	uint32_t valid_params;
	uint16_t src_id;
	uint16_t src_index;
	uint16_t dst_id;
	uint16_t dst_host_irq;
	uint16_t ia_id;
	uint16_t vint;
	uint16_t global_event;
	uint8_t vint_status_bit_index;
	uint8_t secondary_host;
} __packed;

/**
 * @brief Response to setting a peripheral to processor interrupt.
 *
 * @param hdr Standard TISCI header.
 */
struct tisci_msg_rm_irq_set_resp {
	struct tisci_msg_hdr hdr;
} __packed;

/**
 * @brief Request to release interrupt peripheral resources.
 *
 * Releases interrupt peripheral resources according to the valid configuration provided.
 *
 * @param hdr                  Standard TISCI header.
 * @param valid_params         Bitfield defining validity of interrupt route release parameters.
 *                             Each bit corresponds to a field's validity.
 * @param src_id               ID of interrupt source peripheral.
 * @param src_index            Interrupt source index within source peripheral.
 * @param dst_id               SoC IR device ID (valid if TISCI_MSG_VALUE_RM_DST_ID_VALID is set).
 * @param dst_host_irq         SoC IR output index (valid if TISCI_MSG_VALUE_RM_DST_HOST_IRQ_VALID
 * is set).
 * @param ia_id                Device ID of interrupt aggregator (valid if
 * TISCI_MSG_VALUE_RM_IA_ID_VALID is set).
 * @param vint                 Virtual interrupt number (valid if TISCI_MSG_VALUE_RM_VINT_VALID is
 * set).
 * @param global_event         Global event mapped to interrupt aggregator (valid if
 * TISCI_MSG_VALUE_RM_GLOBAL_EVENT_VALID is set).
 * @param vint_status_bit_index Virtual interrupt status bit (valid if
 * TISCI_MSG_VALUE_RM_VINT_STATUS_BIT_INDEX_VALID is set).
 * @param secondary_host       Secondary host value (valid if
 * TISCI_MSG_VALUE_RM_SECONDARY_HOST_VALID is set).
 */
struct tisci_msg_rm_irq_release_req {
	struct tisci_msg_hdr hdr;
	uint32_t valid_params;
	uint16_t src_id;
	uint16_t src_index;
	uint16_t dst_id;
	uint16_t dst_host_irq;
	uint16_t ia_id;
	uint16_t vint;
	uint16_t global_event;
	uint8_t vint_status_bit_index;
	uint8_t secondary_host;
} __packed;

/**
 * @brief Response to releasing a peripheral to processor interrupt.
 *
 * @param hdr Standard TISCI header.
 */
struct tisci_msg_rm_irq_release_resp {
	struct tisci_msg_hdr hdr;
} __packed;

#endif /* INCLUDE_ZEPHYR_DRIVERS_MISC_TISCI_H_ */
