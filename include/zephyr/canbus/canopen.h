/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for the CANopen protocol stack (CiA 301)
 */

#ifndef ZEPHYR_INCLUDE_CANBUS_CANOPEN_H_
#define ZEPHYR_INCLUDE_CANBUS_CANOPEN_H_

/**
 * @brief CANopen protocol stack
 * @defgroup canopen CANopen
 * @ingroup connectivity
 * @since 4.6
 * @version 0.1.0
 * @{
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/canbus/canopen_od.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief NMT states, encoded as in the heartbeat protocol. */
enum canopen_nmt_state {
	/** Initialisation, boot-up in progress */
	CANOPEN_NMT_INITIALISING = 0x00,
	/** Stopped */
	CANOPEN_NMT_STOPPED = 0x04,
	/** Operational */
	CANOPEN_NMT_OPERATIONAL = 0x05,
	/** Pre-operational */
	CANOPEN_NMT_PRE_OPERATIONAL = 0x7F,
};

/** @brief NMT node control commands. */
enum canopen_nmt_cmd {
	/** Start remote node */
	CANOPEN_NMT_CMD_START = 0x01,
	/** Stop remote node */
	CANOPEN_NMT_CMD_STOP = 0x02,
	/** Enter pre-operational */
	CANOPEN_NMT_CMD_ENTER_PRE_OPERATIONAL = 0x80,
	/** Reset node */
	CANOPEN_NMT_CMD_RESET_NODE = 0x81,
	/** Reset communication */
	CANOPEN_NMT_CMD_RESET_COMMUNICATION = 0x82,
};

struct canopen_node;

/**
 * @brief Callback for NMT state changes.
 *
 * Called from the CANopen thread, or from the thread calling @ref canopen_node_start or
 * @ref canopen_nmt_local_cmd.
 *
 * @param node      CANopen node.
 * @param state     New NMT state.
 * @param user_data User data from the node configuration.
 */
typedef void (*canopen_nmt_state_cb_t)(struct canopen_node *node, enum canopen_nmt_state state,
				       void *user_data);

/**
 * @brief Callback for NMT reset node.
 *
 * The application sets the values of its object dictionary entries to their power-on values.
 *
 * @param node      CANopen node.
 * @param user_data User data from the node configuration.
 */
typedef void (*canopen_reset_app_cb_t)(struct canopen_node *node, void *user_data);

/**
 * @brief Callback for a received RPDO.
 *
 * Called from the CANopen thread after the mapped objects have been written.
 *
 * @param node      CANopen node.
 * @param pdo       RPDO number, starting at 1.
 * @param user_data User data from the node configuration.
 */
typedef void (*canopen_rpdo_cb_t)(struct canopen_node *node, uint16_t pdo, void *user_data);

/**
 * @brief Callback for a SYNC event.
 *
 * Called from the CANopen thread after the synchronous PDOs have been processed.
 *
 * @param node      CANopen node.
 * @param counter   SYNC counter, 0 if the SYNC message has no counter.
 * @param user_data User data from the node configuration.
 */
typedef void (*canopen_sync_cb_t)(struct canopen_node *node, uint8_t counter, void *user_data);

/** @brief Events reported by the heartbeat consumer. */
enum canopen_hb_event {
	/** First heartbeat, changed NMT state or heartbeat resumed after a timeout */
	CANOPEN_HB_EVENT_STATE,
	/** No heartbeat within the consumer heartbeat time */
	CANOPEN_HB_EVENT_TIMEOUT,
};

/**
 * @brief Callback for heartbeat consumer events.
 *
 * Called from the CANopen thread.
 *
 * @param node      CANopen node.
 * @param remote_id Node-ID of the monitored node.
 * @param event     Event.
 * @param state     Last NMT state reported by the monitored node.
 * @param user_data User data from the node configuration.
 */
typedef void (*canopen_hb_cb_t)(struct canopen_node *node, uint8_t remote_id,
				enum canopen_hb_event event, enum canopen_nmt_state state,
				void *user_data);

/**
 * @name Error register bits (object 1001h)
 * @{
 */
/** Generic error, set while any error is active */
#define CANOPEN_ERR_REG_GENERIC       BIT(0)
/** Current */
#define CANOPEN_ERR_REG_CURRENT       BIT(1)
/** Voltage */
#define CANOPEN_ERR_REG_VOLTAGE       BIT(2)
/** Temperature */
#define CANOPEN_ERR_REG_TEMPERATURE   BIT(3)
/** Communication error */
#define CANOPEN_ERR_REG_COMMUNICATION BIT(4)
/** Device profile specific */
#define CANOPEN_ERR_REG_PROFILE       BIT(5)
/** Manufacturer-specific */
#define CANOPEN_ERR_REG_MANUFACTURER  BIT(7)
/** @} */

/**
 * @name Emergency error codes reported by the stack (CiA 301, Table 26)
 * @{
 */
/** Error reset or no error */
#define CANOPEN_EMCY_NO_ERROR          0x0000U
/** Generic error */
#define CANOPEN_EMCY_GENERIC           0x1000U
/** CAN overrun, objects lost */
#define CANOPEN_EMCY_CAN_OVERRUN       0x8110U
/** CAN in error passive mode */
#define CANOPEN_EMCY_CAN_PASSIVE       0x8120U
/** Life guard or heartbeat error */
#define CANOPEN_EMCY_HEARTBEAT         0x8130U
/** Recovered from bus off */
#define CANOPEN_EMCY_BUS_OFF_RECOVERED 0x8140U
/** PDO not processed due to length error */
#define CANOPEN_EMCY_PDO_LENGTH        0x8210U
/** Unexpected SYNC data length */
#define CANOPEN_EMCY_SYNC_LENGTH       0x8240U
/** RPDO timeout */
#define CANOPEN_EMCY_RPDO_TIMEOUT      0x8250U
/** @} */

/** PDO COB-ID bit marking the PDO as not valid */
#define CANOPEN_PDO_COB_ID_INVALID BIT(31)

/** PDO transmission type: synchronous, acyclic */
#define CANOPEN_PDO_TYPE_SYNC_ACYCLIC  0x00U
/** PDO transmission type: event-driven, manufacturer-specific */
#define CANOPEN_PDO_TYPE_EVENT         0xFEU
/** PDO transmission type: event-driven, device profile specific */
#define CANOPEN_PDO_TYPE_EVENT_PROFILE 0xFFU

/**
 * @brief Encode a PDO mapping entry.
 *
 * @param _index Index of the mapped object.
 * @param _sub   Sub-index of the mapped object.
 * @param _bits  Length of the mapped object in bits.
 */
#define CANOPEN_PDO_MAP(_index, _sub, _bits)                                                       \
	(((uint32_t)(_index) << 16) | ((uint32_t)(_sub) << 8) | (uint32_t)(_bits))

/** @brief Power-on configuration of a PDO. */
struct canopen_pdo_config {
	/** Mapping entries, see @ref CANOPEN_PDO_MAP */
	const uint32_t *map;
	/**
	 * COB-ID. A CAN-ID of 0 selects the CAN-ID of the pre-defined connection set, which
	 * exists for PDO 1 to 4 only, so @ref CANOPEN_PDO_COB_ID_INVALID alone gives a PDO
	 * that is not valid with its pre-defined CAN-ID. Restricted CAN-IDs are refused.
	 */
	uint32_t cob_id;
	/** Inhibit time in multiples of 100 us, TPDO only */
	uint16_t inhibit_time;
	/**
	 * Event timer in milliseconds. TPDO: transmission period of event-driven TPDOs. RPDO:
	 * deadline after which a missing RPDO is reported with EMCY 8250h.
	 */
	uint16_t event_timer;
	/** Transmission type */
	uint8_t transmission_type;
	/** SYNC start value, TPDO only */
	uint8_t sync_start;
	/** Number of mapping entries */
	uint8_t map_count;
};

/** @brief CANopen node configuration. */
struct canopen_node_config {
	/** CAN controller */
	const struct device *can;
	/** Application object dictionary, entries 2000h and above */
	const struct canopen_od *od;
	/** Optional NMT state change callback */
	canopen_nmt_state_cb_t nmt_state_cb;
	/** Optional reset node callback */
	canopen_reset_app_cb_t reset_app_cb;
	/** Optional RPDO callback */
	canopen_rpdo_cb_t rpdo_cb;
	/** Optional SYNC callback */
	canopen_sync_cb_t sync_cb;
	/** Optional heartbeat consumer callback */
	canopen_hb_cb_t hb_cb;
	/**
	 * Power-on values of the consumer heartbeat time entries (1016h), node-ID in bits 16-23
	 * and time in milliseconds in bits 0-15
	 */
	const uint32_t *hb_consumer;
	/** Power-on configuration of TPDO 1 to @p tpdo_count, others are not valid */
	const struct canopen_pdo_config *tpdo;
	/** Power-on configuration of RPDO 1 to @p rpdo_count, others are not valid */
	const struct canopen_pdo_config *rpdo;
	/** Number of entries in @p tpdo */
	uint16_t tpdo_count;
	/** Number of entries in @p rpdo */
	uint16_t rpdo_count;
	/** Number of entries in @p hb_consumer */
	uint8_t hb_consumer_count;
	/** User data passed to the callbacks */
	void *user_data;
	/** Serial number, identity object 1018h sub-index 4 */
	uint32_t serial_number;
	/** Node-ID, 1 to 127 */
	uint8_t node_id;
};

/** @cond INTERNAL_HIDDEN */

struct canopen_sdo_server {
	k_timepoint_t deadline;
	size_t size;
	size_t pos;
	uint16_t index;
	uint8_t sub;
	uint8_t state;
	uint8_t toggle;
	uint8_t buf[CONFIG_CANOPEN_SDO_BUFFER_SIZE];
};

struct canopen_pdo_param {
	uint32_t cob_id;
	uint32_t map[CONFIG_CANOPEN_PDO_MAPPED_MAX];
	uint16_t inhibit_time;
	uint16_t event_timer;
	uint8_t transmission_type;
	uint8_t sync_start;
	uint8_t map_count;
};

struct canopen_pdo {
	k_timepoint_t inhibit_end;
	k_timepoint_t event_next;
	struct canopen_pdo_param param;
	int filter;
	uint8_t sync_count;
	uint8_t flags;
	uint8_t len;
	uint8_t data[CAN_MAX_DLC];
};

struct canopen_sync {
	struct k_timer timer;
	int64_t last;
	uint32_t cob_id;
	uint32_t period;
	uint32_t window;
	int filter;
	uint8_t overflow;
	uint8_t counter;
	bool length_error;
};

struct canopen_emcy {
	k_timepoint_t inhibit_end;
	uint32_t cob_id;
	uint32_t history[CONFIG_CANOPEN_EMCY_HISTORY_SIZE];
	uint16_t active[CONFIG_CANOPEN_EMCY_ACTIVE_MAX];
	uint8_t active_reg[CONFIG_CANOPEN_EMCY_ACTIVE_MAX];
	uint8_t pending[CONFIG_CANOPEN_EMCY_QUEUE_SIZE][CAN_MAX_DLC];
	uint16_t inhibit_time;
	uint8_t history_count;
	uint8_t active_count;
	uint8_t pending_head;
	uint8_t pending_count;
};

struct canopen_hb_consumer {
	k_timepoint_t deadline;
	uint32_t entry;
	int filter;
	uint8_t state;
	uint8_t status;
};

/** @endcond */

/**
 * @brief CANopen node.
 *
 * All members are internal to the stack.
 */
struct canopen_node {
	/** @cond INTERNAL_HIDDEN */
	const struct canopen_node_config *cfg;
	struct k_thread thread;
	struct k_mutex lock;
	struct k_msgq rxq;
	struct k_msgq txq;
	struct k_poll_signal signal;
	struct canopen_sdo_server sdo;
	struct canopen_sync sync;
	struct canopen_emcy emcy;
	struct canopen_hb_consumer hb[CONFIG_CANOPEN_HB_CONSUMER_COUNT];
	struct can_state_change_callback can_state_cb;
	atomic_t can_state;
	struct canopen_pdo tpdo[CONFIG_CANOPEN_TPDO_COUNT];
	struct canopen_pdo rpdo[CONFIG_CANOPEN_RPDO_COUNT];
	k_timepoint_t hb_next;
	atomic_t flags;
	uint32_t sdo_cob_id[2];
	uint16_t hb_producer_ms;
	uint8_t nmt_state;
	uint8_t error_register;
	uint8_t error_behavior;
	uint8_t can_state_last;
	struct can_frame rxq_buf[CONFIG_CANOPEN_RX_QUEUE_SIZE];
	struct can_frame txq_buf[CONFIG_CANOPEN_TX_QUEUE_SIZE];

	K_KERNEL_STACK_MEMBER(stack, CONFIG_CANOPEN_THREAD_STACK_SIZE);
	/** @endcond */
};

/**
 * @brief Initialize a CANopen node.
 *
 * @param node CANopen node.
 * @param cfg  Configuration, must stay valid for the lifetime of the node.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid node-ID, object dictionary or PDO configuration.
 * @retval -ENODEV CAN controller not ready.
 */
int canopen_node_init(struct canopen_node *node, const struct canopen_node_config *cfg);

/**
 * @brief Start a CANopen node.
 *
 * Installs the CAN filters, starts the CAN controller if it is stopped and runs the NMT
 * boot-up. The node enters NMT state pre-operational.
 *
 * @param node CANopen node.
 *
 * @retval 0 Success.
 * @retval -ENOSPC No free CAN filter.
 * @retval -EIO Failed to start the CAN controller.
 */
int canopen_node_start(struct canopen_node *node);

/**
 * @brief Get the NMT state of a CANopen node.
 *
 * @param node CANopen node.
 *
 * @return Current NMT state.
 */
enum canopen_nmt_state canopen_nmt_get_state(struct canopen_node *node);

/**
 * @brief Execute an NMT command by local control.
 *
 * @param node CANopen node.
 * @param cmd  NMT command.
 *
 * @retval 0 Success.
 * @retval -EINVAL Unknown command.
 */
int canopen_nmt_local_cmd(struct canopen_node *node, enum canopen_nmt_cmd cmd);

/**
 * @brief Signal an application event for a TPDO.
 *
 * An event-driven TPDO is transmitted once its inhibit time has elapsed, a synchronous acyclic
 * TPDO with the next SYNC. The event is ignored for synchronous cyclic TPDOs.
 *
 * @param node CANopen node.
 * @param pdo  TPDO number, starting at 1.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid TPDO number.
 */
int canopen_tpdo_trigger(struct canopen_node *node, uint16_t pdo);

/**
 * @brief Report an application error.
 *
 * The error is added to the error history (1003h), its bits are set in the error register
 * (1001h) and an emergency message is transmitted. The error stays active until it is cleared
 * with @ref canopen_emcy_clear.
 *
 * @param node CANopen node.
 * @param code Emergency error code.
 * @param reg  Error register bits, see CANOPEN_ERR_REG_*.
 * @param msef Optional 5 bytes of manufacturer-specific error information, or NULL.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p code is 0.
 * @retval -EALREADY The error is already active.
 * @retval -ENOSPC Too many active errors, see CONFIG_CANOPEN_EMCY_ACTIVE_MAX.
 */
int canopen_emcy_report(struct canopen_node *node, uint16_t code, uint8_t reg,
			const uint8_t *msef);

/**
 * @brief Clear an active application error.
 *
 * The error register is updated and an emergency message with error code 0000h is
 * transmitted.
 *
 * @param node CANopen node.
 * @param code Emergency error code.
 *
 * @retval 0 Success.
 * @retval -ENOENT The error is not active.
 */
int canopen_emcy_clear(struct canopen_node *node, uint16_t code);

/**
 * @brief Get the NMT state of a node monitored by the heartbeat consumer.
 *
 * @param node      CANopen node.
 * @param remote_id Node-ID of the monitored node.
 * @param state     NMT state of the monitored node.
 *
 * @retval 0 Success.
 * @retval -ENOENT The node is not monitored.
 * @retval -EAGAIN No heartbeat received yet.
 * @retval -ETIMEDOUT The heartbeat timed out, @p state is the last reported state.
 */
int canopen_hb_consumer_state(struct canopen_node *node, uint8_t remote_id,
			      enum canopen_nmt_state *state);

/**
 * @brief Read an object dictionary value.
 *
 * Access attributes are not checked.
 *
 * @param node  CANopen node.
 * @param index Index.
 * @param sub   Sub-index.
 * @param buf   Buffer for the value in native representation.
 * @param len   Size of @p buf, must equal the size of the value.
 *
 * @retval 0 Success.
 * @retval -ENOENT Entry does not exist.
 * @retval -EINVAL @p len does not match the size of the value.
 */
int canopen_od_read(struct canopen_node *node, uint16_t index, uint8_t sub, void *buf, size_t len);

/**
 * @brief Write an object dictionary value.
 *
 * Access attributes are not checked and the write hook is not called. Values of the
 * communication profile area are checked as for an SDO download.
 *
 * @param node  CANopen node.
 * @param index Index.
 * @param sub   Sub-index.
 * @param buf   Value in native representation.
 * @param len   Length of @p buf, must equal the size of the value.
 *
 * @retval 0 Success.
 * @retval -ENOENT Entry does not exist.
 * @retval -EACCES Entry is not backed by writable storage.
 * @retval -EINVAL @p len does not match the size of the value, or the value is rejected.
 */
int canopen_od_write(struct canopen_node *node, uint16_t index, uint8_t sub, const void *buf,
		     size_t len);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_CANBUS_CANOPEN_H_ */
