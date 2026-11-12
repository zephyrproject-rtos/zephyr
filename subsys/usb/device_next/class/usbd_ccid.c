// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright (c) 2026 TeraTron GmbH
 */

/**
 * @file
 * @brief USB CCID APDU-level class with per-interface workers and endpoint state.
 * @ingroup usbd_ccid
 *
 * Specification references below refer to USB CCID revision 1.1:
 * https://www.usb.org/sites/default/files/DWG_Smart-Card_CCID_Rev110.pdf
 */

#define DT_DRV_COMPAT zephyr_ccid

#include <errno.h>
#include <limits.h>
#include <string.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/class/usbd_ccid.h>
#include <zephyr/usb/usbd.h>

LOG_MODULE_REGISTER(usbd_ccid, CONFIG_USBD_CCID_LOG_LEVEL);

#define CCID_CLASS_CODE        0x0B
#define CCID_DESCRIPTOR_TYPE   0x21
#define CCID_DESCRIPTOR_LENGTH 0x36
#define CCID_HEADER_SIZE       10U
#define CCID_INSTANCE_COUNT    DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)

BUILD_ASSERT(CCID_INSTANCE_COUNT > 0, "At least one enabled CCID node is required");

/* See CCID rev. 1.1, section 6.1, for Bulk-OUT command message types. */
#define CCID_PC_TO_RDR_ICC_POWER_ON            0x62
#define CCID_PC_TO_RDR_ICC_POWER_OFF           0x63
#define CCID_PC_TO_RDR_GET_SLOT_STATUS         0x65
#define CCID_PC_TO_RDR_XFR_BLOCK               0x6F
#define CCID_PC_TO_RDR_GET_PARAMETERS          0x6C
#define CCID_PC_TO_RDR_RESET_PARAMETERS        0x6D
#define CCID_PC_TO_RDR_SET_PARAMETERS          0x61
#define CCID_PC_TO_RDR_ABORT                   0x72
#define CCID_PC_TO_RDR_ICC_CLOCK               0x6E
#define CCID_PC_TO_RDR_ESCAPE                  0x6B
#define CCID_PC_TO_RDR_SECURE                  0x69
#define CCID_PC_TO_RDR_SET_DATA_RATE_AND_CLOCK 0x73

/* See CCID rev. 1.1, section 6.2, for Bulk-IN response message types. */
#define CCID_RDR_TO_PC_DATA_BLOCK          0x80
#define CCID_RDR_TO_PC_SLOT_STATUS         0x81
#define CCID_RDR_TO_PC_PARAMETERS          0x82
#define CCID_RDR_TO_PC_ESCAPE              0x83
#define CCID_RDR_TO_PC_DATA_RATE_AND_CLOCK 0x84

/* See CCID rev. 1.1, section 6.3.1, for asynchronous slot-change notifications. */
#define CCID_RDR_TO_PC_NOTIFY_SLOT_CHANGE 0x50

/* See CCID rev. 1.1, section 5.3, for class-specific control requests. */
#define CCID_REQ_ABORT                 0x01
#define CCID_REQ_GET_CLOCK_FREQUENCIES 0x02
#define CCID_REQ_GET_DATA_RATES        0x03

/* See CCID rev. 1.1, section 6.2.6, for response status bits and error codes. */
#define CCID_STATUS_ICC_ACTIVE      0x00
#define CCID_STATUS_ICC_INACTIVE    0x01
#define CCID_STATUS_ICC_NOT_PRESENT 0x02
#define CCID_STATUS_CMD_FAILED      BIT(6)
#define CCID_STATUS_TIME_EXTENSION  BIT(7)

#define CCID_ERROR_CMD_NOT_SUPPORTED 0x00
#define CCID_ERROR_BAD_LENGTH        0x01
#define CCID_ERROR_BAD_SLOT          0x05
#define CCID_ERROR_BAD_POWER_SELECT  0x07
#define CCID_ERROR_BAD_PROTOCOL      0x07
#define CCID_ERROR_BAD_CLOCK_COMMAND 0x07
#define CCID_ERROR_BAD_LEVEL         0x08
#define CCID_ERROR_SLOT_BUSY         0xE0
#define CCID_ERROR_XFR_OVERRUN       0xFC
#define CCID_ERROR_ICC_MUTE          0xFE
#define CCID_ERROR_HW_ERROR          0xFB
#define CCID_ERROR_ABORTED           0xFF

#define CCID_DATA_RATE_OFFSET         sizeof(uint32_t)
#define CCID_DATA_RATE_AND_CLOCK_SIZE (CCID_DATA_RATE_OFFSET + sizeof(uint32_t))
#define CCID_SUPPORTED_FEATURES                                                                    \
	(USBD_CCID_AUTO_PARAMETERS | USBD_CCID_AUTO_ACTIVATE | USBD_CCID_AUTO_VOLTAGE |            \
	 USBD_CCID_AUTO_CLOCK | USBD_CCID_AUTO_DATA_RATE | USBD_CCID_AUTO_NEGOTIATION |            \
	 USBD_CCID_AUTO_PPS | USBD_CCID_CLOCK_STOP | USBD_CCID_NONZERO_NAD | USBD_CCID_AUTO_IFSD | \
	 USBD_CCID_SHORT_APDU | USBD_CCID_EXTENDED_APDU)
#define CCID_SUPPORTED_PROTOCOLS                                                                   \
	(USBD_CCID_PROTOCOL_MASK(USBD_CCID_PROTOCOL_T0) |                                          \
	 USBD_CCID_PROTOCOL_MASK(USBD_CCID_PROTOCOL_T1))
#define CCID_SUPPORTED_VOLTAGES                                                                    \
	(USBD_CCID_VOLTAGE_MASK(USBD_CCID_VOLTAGE_5V) |                                            \
	 USBD_CCID_VOLTAGE_MASK(USBD_CCID_VOLTAGE_3V) |                                            \
	 USBD_CCID_VOLTAGE_MASK(USBD_CCID_VOLTAGE_1V8))
#define CCID_PROTOCOL_MASK_BITS (sizeof(uint32_t) * CHAR_BIT)

#define CCID_SPECIFIC_SIZE                 3U
#define CCID_BUSY_SLOT_COUNT               1U
#define CCID_SPECIFIC_STATUS_OFFSET        0U
#define CCID_SPECIFIC_ERROR_OFFSET         1U
#define CCID_SPECIFIC_CHAIN_OFFSET         2U
#define CCID_SLOT_ZERO                     0U
#define CCID_SPECIFIC_POWER_SELECT_OFFSET  0U
#define CCID_SPECIFIC_PROTOCOL_OFFSET      0U
#define CCID_SPECIFIC_LEVEL_LOW_OFFSET     1U
#define CCID_SPECIFIC_CLOCK_COMMAND_OFFSET 0U
#define CCID_VERSION_1_1                   0x0110
#define CCID_FS_BULK_PACKET_SIZE           64U
#define CCID_HS_BULK_PACKET_SIZE           512U
#define CCID_INTERRUPT_PACKET_SIZE         4U
#define CCID_FS_INTERRUPT_INTERVAL         16U
#define CCID_HS_INTERRUPT_INTERVAL         7U
#define CCID_NOTIFY_SLOT_ZERO_CHANGED      BIT(1)
#define CCID_NOTIFY_SLOT_ZERO_PRESENT      BIT(0)
#define CCID_SHORT_COMMAND_MAX             261U
#define CCID_SHORT_RESPONSE_MAX            258U
#define CCID_IO_RETRY_MS                   10U
#define CCID_RESPONSE_MAX                  65538U
#define CCID_APDU_STATUS_SIZE              2U
#define CCID_NOTIFY_SLOT_CHANGE_SIZE       2U
#define CCID_ABORT_SLOT_MASK               UINT8_MAX
#define CCID_ABORT_SEQUENCE_SHIFT          8U
#define CCID_ABORT_SEQ_NONE                (-1)

/* Extended APDU wLevelParameter values (section 6.1.4). */
enum ccid_apdu_level {
	CCID_APDU_LEVEL_COMPLETE = 0x00,
	CCID_APDU_LEVEL_FIRST = 0x01,
	CCID_APDU_LEVEL_LAST = 0x02,
	CCID_APDU_LEVEL_MIDDLE = 0x03,
	CCID_APDU_LEVEL_CONTINUE = 0x10,
};

/* Extended APDU bChainParameter values (section 6.2.1). */
enum ccid_chain_parameter {
	CCID_CHAIN_COMPLETE = 0x00,
	CCID_CHAIN_FIRST = 0x01,
	CCID_CHAIN_LAST = 0x02,
	CCID_CHAIN_MIDDLE = 0x03,
	CCID_CHAIN_COMMAND_ACK = 0x10,
};

/* bClockCommand values (section 6.1.9). */
enum ccid_clock_command {
	CCID_CLOCK_RESTART = 0x00,
	CCID_CLOCK_STOP = 0x01,
};

enum ccid_endpoint {
	CCID_EP_BULK_OUT,
	CCID_EP_BULK_IN,
	CCID_EP_INTERRUPT_IN,
	CCID_EP_COUNT,
};

enum ccid_apdu_state {
	CCID_APDU_IDLE,
	CCID_APDU_COMMAND,
	CCID_APDU_RESPONSE,
};

enum ccid_state_bit {
	CCID_ENABLED,
	CCID_NOTIFY_PENDING,
	CCID_SUSPENDED,
	CCID_NEEDS_POWER_ON,
};

/* Control and Bulk-OUT ABORT requests may arrive in either order. */
enum ccid_abort_state {
	CCID_ABORT_IDLE,
	CCID_ABORT_WAIT_BULK,
	CCID_ABORT_MATCHED,
};

enum ccid_in_kind {
	CCID_IN_FINAL,
	CCID_IN_AUXILIARY,
	CCID_IN_ABORT,
	CCID_IN_EXTENSION,
	CCID_IN_STALE,
};

/* See CCID rev. 1.1, section 5.1, table 5.1-1: Smart Card Device Class Descriptors. */
struct ccid_class_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t bcdCCID;
	uint8_t bMaxSlotIndex;
	uint8_t bVoltageSupport;
	uint32_t dwProtocols;
	uint32_t dwDefaultClock;
	uint32_t dwMaximumClock;
	uint8_t bNumClockSupported;
	uint32_t dwDataRate;
	uint32_t dwMaxDataRate;
	uint8_t bNumDataRatesSupported;
	uint32_t dwMaxIFSD;
	uint32_t dwSynchProtocols;
	uint32_t dwMechanical;
	uint32_t dwFeatures;
	uint32_t dwMaxCCIDMessageLength;
	uint8_t bClassGetResponse;
	uint8_t bClassEnvelope;
	uint16_t wLcdLayout;
	uint8_t bPINSupport;
	uint8_t bMaxCCIDBusySlots;
} __packed;

BUILD_ASSERT(sizeof(struct ccid_class_descriptor) == CCID_DESCRIPTOR_LENGTH);

/* Group the interface (section 4.3) and endpoint (section 5.2) descriptors for supported speeds. */
struct ccid_descriptor {
	struct usb_if_descriptor iface;
	struct ccid_class_descriptor ccid;
	struct usb_ep_descriptor fs_ep[CCID_EP_COUNT];
#if USBD_SUPPORTS_HIGH_SPEED
	struct usb_ep_descriptor hs_ep[CCID_EP_COUNT];
#endif
	struct usb_desc_header nil_desc;
};

/* See CCID rev. 1.1, section 6: common 10-byte header for Bulk-OUT and Bulk-IN messages. */
struct ccid_bulk_header {
	uint8_t message_type;
	uint32_t length;
	uint8_t slot;
	uint8_t seq;
	uint8_t specific[CCID_SPECIFIC_SIZE];
} __packed;

BUILD_ASSERT(sizeof(struct ccid_bulk_header) == CCID_HEADER_SIZE);

/* Fixed per-interface bindings live in ROM, separately from runtime state. */
struct ccid_instance {
	struct ccid_descriptor *desc;
	const struct usb_desc_header **fs_desc;
#if USBD_SUPPORTS_HIGH_SPEED
	const struct usb_desc_header **hs_desc;
#endif
	struct usbd_desc_node *if_string;
	const char *thread_name;
	unsigned int id;
	unsigned int stack_index;
};

/* One USB message workspace is reused after command consumption. Backend
 * input and output remain independent; all large buffers are zero-initialized.
 */
struct ccid_buffers {
	uint8_t message[CONFIG_USBD_CCID_MAX_MESSAGE_SIZE];
	uint8_t apdu_command[CONFIG_USBD_CCID_MAX_APDU_SIZE];
	uint8_t apdu_response[CONFIG_USBD_CCID_MAX_APDU_SIZE];
};

struct ccid_usb_state {
	struct usbd_class_data *class_data;
	struct net_buf *notify_buf;
};

struct ccid_backend_state {
	struct usbd_ccid_backend *object;
	struct usbd_ccid_data_rate_and_clock rate;
	uint8_t clock_status;
	uint8_t protocol;
};

struct ccid_worker_state {
	struct k_thread thread;
	struct k_sem sem;
	size_t command_length;
	uint32_t generation;
	atomic_t started;
	struct ccid_bulk_header request;
	bool busy;
	bool reset_pending;
};

struct ccid_rx_state {
	uint64_t length;
	struct net_buf *buf;
	struct ccid_bulk_header header;
	bool expect_zlp;
	bool store_payload;
};

struct ccid_tx_state {
	int64_t extension_deadline;
	struct net_buf *buf;
	size_t length;
	enum ccid_in_kind kind;
	uint8_t auxiliary[CCID_HEADER_SIZE];
	bool auxiliary_ready;
};

struct ccid_abort {
	enum ccid_abort_state state;
	int16_t last_seq; /* CCID_ABORT_SEQ_NONE unless the last message was ABORT. */
	uint8_t seq;
};

struct ccid_apdu {
	size_t command_length;
	size_t response_length;
	size_t response_offset;
	enum ccid_apdu_state state;
};

struct ccid_context {
	const struct ccid_instance *instance;
	struct ccid_buffers *buffers;
	struct ccid_usb_state usb;
	struct ccid_backend_state backend;
	struct ccid_worker_state worker;
	struct ccid_rx_state rx;
	struct ccid_tx_state tx;
	struct ccid_abort abort;
	struct ccid_apdu apdu;
	struct k_mutex lock;
	struct k_work_delayable io_work;
	atomic_t state;
	uint32_t generation;
};

static struct ccid_context ccid_contexts[CCID_INSTANCE_COUNT];
static struct ccid_buffers ccid_buffers[CCID_INSTANCE_COUNT];
K_THREAD_STACK_ARRAY_DEFINE(ccid_stacks, CCID_INSTANCE_COUNT, CONFIG_USBD_CCID_THREAD_STACK_SIZE);

/**
 * @brief Acquire the instance mutex and log an acquisition failure.
 * @param ccid Private state of the addressed CCID interface.
 *
 * @return True if the mutex was acquired; false if locking failed.
 */
static bool ccid_lock(struct ccid_context *ccid)
{
	int ret = k_mutex_lock(&ccid->lock, K_FOREVER);

	if (ret != 0) {
		LOG_ERR("CCID %u: mutex lock failed (%d)", ccid->instance->id, ret);
		return false;
	}
	return true;
}

/**
 * @brief Release the instance mutex and log a release failure.
 * @param ccid Private state of the addressed CCID interface.
 *
 * @pre The calling thread owns the instance mutex.
 */
static void ccid_unlock(struct ccid_context *ccid)
{
	int ret = k_mutex_unlock(&ccid->lock);

	if (ret != 0) {
		LOG_ERR("CCID %u: mutex unlock failed (%d)", ccid->instance->id, ret);
	}
}

/**
 * @brief Schedule interface I/O and log work-queue failures.
 * @param ccid Private state of the addressed CCID interface.
 * @param delay Time before I/O runs, or K_NO_WAIT for immediate submission.
 *
 * @retval 0 if work was queued, scheduled or already pending.
 * @return Negative work-queue error; failures are logged before returning.
 */
static int ccid_schedule_io(struct ccid_context *ccid, k_timeout_t delay)
{
	int ret = k_work_reschedule(&ccid->io_work, delay);

	if (ret < 0) {
		LOG_ERR("CCID %u: I/O scheduling failed (%d)", ccid->instance->id, ret);
		return ret;
	}
	return 0;
}

/**
 * @brief Select the endpoint descriptor for the negotiated USB bus speed.
 *
 * @param c_data USB class instance owning the interface and its endpoints.
 * @param ep Endpoint role selecting bulk-IN, bulk-OUT or interrupt-IN.
 *
 * @return Instance-owned endpoint descriptor for the active bus speed; never NULL.
 */
static const struct usb_ep_descriptor *ccid_ep_desc(struct usbd_class_data *c_data,
						    enum ccid_endpoint ep)
{
	struct ccid_context *ccid = usbd_class_get_private(c_data);

#if USBD_SUPPORTS_HIGH_SPEED
	if (usbd_bus_speed(usbd_class_get_ctx(c_data)) == USBD_SPEED_HS) {
		return &ccid->instance->desc->hs_ep[ep];
	}
#endif
	return &ccid->instance->desc->fs_ep[ep];
}

/**
 * @brief Return the assigned endpoint address, including its direction bit.
 *
 * @param c_data USB class instance owning the interface and its endpoints.
 * @param ep Endpoint role selecting bulk-IN, bulk-OUT or interrupt-IN.
 *
 * @return USB endpoint address containing the endpoint number and direction bit.
 */
static uint8_t ccid_ep_addr(struct usbd_class_data *c_data, enum ccid_endpoint ep)
{
	return ccid_ep_desc(c_data, ep)->bEndpointAddress;
}

/**
 * @brief Convert backend presence and power state to a CCID ICC status.
 * @param ccid Private state of the addressed CCID interface.
 *
 * @retval CCID_STATUS_ICC_ACTIVE if the backend is active and no new power-on is required.
 * @retval CCID_STATUS_ICC_INACTIVE if a card is present but inactive or needs power-on.
 * @retval CCID_STATUS_ICC_NOT_PRESENT if no card or no registered backend is present.
 */
static uint8_t ccid_card_status(struct ccid_context *ccid)
{
	enum usbd_ccid_card_state state;

	if (ccid->backend.object == NULL) {
		return CCID_STATUS_ICC_NOT_PRESENT;
	}

	state = ccid->backend.object->ops->get_state(ccid->backend.object);
	if (state == USBD_CCID_CARD_ACTIVE && !atomic_test_bit(&ccid->state, CCID_NEEDS_POWER_ON)) {
		return CCID_STATUS_ICC_ACTIVE;
	}
	if (state != USBD_CCID_CARD_ABSENT) {
		return CCID_STATUS_ICC_INACTIVE;
	}

	return CCID_STATUS_ICC_NOT_PRESENT;
}

/**
 * @brief Check whether the interface can currently exchange USB data.
 * @param ccid Private state of the addressed CCID interface.
 *
 * @return True if enabled, resumed and backed by registered operations; false otherwise.
 */
static bool ccid_available(struct ccid_context *ccid)
{
	return atomic_test_bit(&ccid->state, CCID_ENABLED) &&
	       !atomic_test_bit(&ccid->state, CCID_SUSPENDED) && ccid->backend.object != NULL;
}

/**
 * @brief Read the active bulk-OUT packet size from the speed-specific descriptor.
 *
 * @param c_data USB class instance owning the interface and its endpoints.
 *
 * @return Maximum bulk-OUT packet size in bytes, converted to CPU byte order.
 */
static size_t ccid_bulk_mps(struct usbd_class_data *c_data)
{
	return sys_le16_to_cpu(ccid_ep_desc(c_data, CCID_EP_BULK_OUT)->wMaxPacketSize);
}

/**
 * @brief Track a transfer buffer before handing it to the USB stack.
 *
 * The instance lock must be held. On enqueue failure, clear the owner pointer
 * and release the buffer; successful transfers are released by completion.
 * UDC completion events are delivered asynchronously by the USB stack.
 *
 * @param ccid Private state of the addressed CCID interface.
 * @param buf Transfer buffer to enqueue; released on enqueue failure.
 * @param owner Instance pointer tracking the buffer until completion; cleared on failure.
 *
 * @retval 0 if the buffer was queued and is tracked by *owner until completion.
 * @return On failure, the error from usbd_ep_enqueue(); the buffer is released
 *         and *owner is cleared to NULL.
 */
static int ccid_enqueue_owned(struct ccid_context *ccid, struct net_buf *buf,
			      struct net_buf **owner)
{
	int ret;

	*owner = buf;
	ret = usbd_ep_enqueue(ccid->usb.class_data, buf);
	if (ret != 0) {
		*owner = NULL;
		net_buf_unref(buf);
	}
	return ret;
}

/**
 * @brief Arm one packet-sized bulk-OUT transfer unless one is already pending.
 *
 * @param ccid Private state of the addressed CCID interface.
 *
 * @retval 0 if an OUT transfer was queued or one is already pending.
 * @retval -EACCES if the interface cannot exchange USB data.
 * @retval -ENOMEM if no transfer buffer could be allocated.
 * @return Other enqueue errors from usbd_ep_enqueue().
 */
static int ccid_submit_out(struct ccid_context *ccid)
{
	struct net_buf *buf;

	if (!ccid_available(ccid)) {
		return -EACCES;
	}
	if (ccid->rx.buf != NULL) {
		return 0;
	}
	/* Completing one packet at a time also handles an OUT message ending at
	 * an exact MPS multiple without requiring a host-supplied ZLP.
	 */
	buf = usbd_ep_buf_alloc(ccid->usb.class_data,
				ccid_ep_addr(ccid->usb.class_data, CCID_EP_BULK_OUT),
				ccid_bulk_mps(ccid->usb.class_data));
	if (buf == NULL) {
		return -ENOMEM;
	}
	return ccid_enqueue_owned(ccid, buf, &ccid->rx.buf);
}

/**
 * @brief Copy a reply into bulk-IN storage and request a ZLP when required.
 *
 * The completion kind determines which pending reply a successful transfer
 * consumes. The caller retains its reply data for retries.
 *
 * @param ccid Private state of the addressed CCID interface.
 * @param data Reply bytes copied into the USB transfer buffer before returning.
 * @param len Number of reply bytes to copy and transmit.
 * @param kind Completion action identifying which pending reply the transfer consumes.
 *
 * @retval 0 if the reply was queued for transmission.
 * @retval -EACCES if the interface cannot exchange USB data.
 * @retval -EBUSY if another bulk-IN transfer is still pending.
 * @retval -ENOMEM if no transfer buffer could be allocated.
 * @return Other enqueue errors from usbd_ep_enqueue().
 */
static int ccid_submit_packet(struct ccid_context *ccid, const void *data, size_t len,
			      enum ccid_in_kind kind)
{
	struct usbd_class_data *c_data = ccid->usb.class_data;
	struct net_buf *buf;
	size_t mps;

	if (!ccid_available(ccid)) {
		return -EACCES;
	}
	if (ccid->tx.buf != NULL) {
		return -EBUSY;
	}
	buf = usbd_ep_buf_alloc(c_data, ccid_ep_addr(c_data, CCID_EP_BULK_IN), len);
	if (buf == NULL) {
		return -ENOMEM;
	}
	(void)net_buf_add_mem(buf, data, len);
	mps = ccid_bulk_mps(c_data);
	/* CCID 3.1.3 requires a ZLP after messages that exactly fill USB packets. */
	if (len % mps == 0) {
		udc_ep_buf_set_zlp(buf);
	}
	ccid->tx.kind = kind;
	return ccid_enqueue_owned(ccid, buf, &ccid->tx.buf);
}

/**
 * @brief Encode the CCID response header for the host command.
 * @param buffer Destination for the ten-byte response header.
 * @param type CCID response message type.
 * @param request Validated CCID command header from the host.
 * @param payload_length Response payload length in bytes.
 * @param status Combined CCID command and card status bits.
 * @param error CCID error code.
 * @param specific Message-specific response byte.
 */
static void ccid_encode_header(void *buffer, uint8_t type, const struct ccid_bulk_header *request,
			       size_t payload_length, uint8_t status, uint8_t error,
			       uint8_t specific)
{
	struct ccid_bulk_header *response = buffer;

	response->message_type = type;
	response->length = sys_cpu_to_le32(payload_length);
	response->slot = request->slot;
	response->seq = request->seq;
	response->specific[CCID_SPECIFIC_STATUS_OFFSET] = status;
	response->specific[CCID_SPECIFIC_ERROR_OFFSET] = error;
	response->specific[CCID_SPECIFIC_CHAIN_OFFSET] = specific;
}

/**
 * @brief Validate message length and the single USB slot number.
 * @param request Received CCID command header to validate.
 * @param command_length Number of bytes in the complete CCID command.
 *
 * @retval 0 if slot and message length are valid.
 * @retval CCID_ERROR_BAD_SLOT if the requested slot does not exist.
 * @retval -EMSGSIZE if dwLength exceeds capacity or disagrees with the received length.
 */
static int ccid_validate_request(const struct ccid_bulk_header *request, size_t command_length)
{
	size_t payload_length = sys_le32_to_cpu(request->length);

	if (request->slot != CCID_SLOT_ZERO) {
		return CCID_ERROR_BAD_SLOT;
	}
	if (payload_length > CONFIG_USBD_CCID_MAX_MESSAGE_SIZE - CCID_HEADER_SIZE ||
	    payload_length != command_length - CCID_HEADER_SIZE) {
		return -EMSGSIZE;
	}

	return 0;
}

/**
 * @brief Check a protocol against the addressed reader's advertised capabilities.
 *
 * @param ccid Private state of the addressed CCID interface.
 * @param protocol CCID protocol number used as a bit index in the advertised mask.
 *
 * @return True if the protocol number has a valid bit in the advertised mask;
 *         false otherwise.
 */
static bool ccid_protocol_supported(const struct ccid_context *ccid, uint8_t protocol)
{
	return protocol < CCID_PROTOCOL_MASK_BITS &&
	       (ccid->backend.object->config.protocols & BIT(protocol)) != 0;
}

/**
 * @brief Check whether a requested clock/rate is listed by the configured reader.
 *
 * @param table Clock-frequency or data-rate table to search.
 * @param count Number of entries in the table.
 * @param value Requested value in the same units as the table entries.
 *
 * @return True if value occurs in the table; false otherwise.
 */
static bool ccid_table_contains(const uint32_t *table, uint8_t count, uint32_t value)
{
	for (size_t i = 0; i < count; i++) {
		if (table[i] == value) {
			return true;
		}
	}
	return false;
}

/**
 * @brief Require both clock and data rate to occur in the advertised tables.
 *
 * @param ccid Private state of the addressed CCID interface.
 * @param rate Settings to validate: clock in kHz and data rate in bits per second.
 *
 * @return True if both settings are advertised; false if either is unsupported.
 */
static bool ccid_rate_valid(const struct ccid_context *ccid,
			    const struct usbd_ccid_data_rate_and_clock *rate)
{
	return ccid_table_contains(ccid->backend.object->config.clock_frequencies,
				   ccid->backend.object->config.clock_count, rate->clock_khz) &&
	       ccid_table_contains(ccid->backend.object->config.data_rates,
				   ccid->backend.object->config.data_rate_count,
				   rate->data_rate_bps);
}

/**
 * @brief Refresh cached clock/rate settings after validating backend values.
 *
 * @param ccid Private state of the addressed CCID interface.
 *
 * @retval 0 if the backend settings were validated and cached.
 * @retval -EIO if the backend reported settings outside the advertised tables.
 * @return The backend's error if its settings could not be read.
 */
static int ccid_refresh_rate(struct ccid_context *ccid)
{
	struct usbd_ccid_data_rate_and_clock rate = {0};
	int ret;

	ret = ccid->backend.object->ops->get_data_rate_and_clock(ccid->backend.object, &rate);
	if (ret != 0) {
		return ret;
	}
	if (!ccid_rate_valid(ccid, &rate)) {
		return -EIO;
	}
	ccid->backend.rate = rate;
	return 0;
}

/**
 * @brief Validate backend-owned response data before copying it into a USB buffer.
 *
 * @param ccid Private state of the addressed CCID interface.
 * @param parameters Backend reply containing the protocol number, data pointer and byte length.
 *
 * @return True for a supported protocol and a data block within the reply capacity,
 *         with non-NULL storage when nonempty; false otherwise.
 */
static bool ccid_parameters_valid(const struct ccid_context *ccid,
				  const struct usbd_ccid_parameters *parameters)
{
	return ccid_protocol_supported(ccid, parameters->protocol) &&
	       parameters->length <= sizeof(ccid->buffers->message) - CCID_HEADER_SIZE &&
	       (parameters->length == 0 || parameters->data != NULL);
}

/**
 * @brief Discard APDU chaining state without clearing the buffer storage.
 *
 * @param ccid Private state of the addressed CCID interface.
 */
static void ccid_reset_apdu(struct ccid_context *ccid)
{
	ccid->apdu.state = CCID_APDU_IDLE;
	ccid->apdu.command_length = 0;
	ccid->apdu.response_length = 0;
	ccid->apdu.response_offset = 0;
}

/**
 * @brief Select the response family for a command, including failure replies.
 *
 * Unknown commands use SlotStatus.
 *
 * @param command Host command message type from the CCID bulk header.
 *
 * @return CCID_RDR_TO_PC_* message type; unknown commands map to SlotStatus.
 */
static uint8_t ccid_response_type(uint8_t command)
{
	switch (command) {
	case CCID_PC_TO_RDR_ICC_POWER_ON:
	case CCID_PC_TO_RDR_XFR_BLOCK:
	case CCID_PC_TO_RDR_SECURE:
		return CCID_RDR_TO_PC_DATA_BLOCK;
	case CCID_PC_TO_RDR_GET_PARAMETERS:
	case CCID_PC_TO_RDR_SET_PARAMETERS:
	case CCID_PC_TO_RDR_RESET_PARAMETERS:
		return CCID_RDR_TO_PC_PARAMETERS;
	case CCID_PC_TO_RDR_ESCAPE:
		return CCID_RDR_TO_PC_ESCAPE;
	case CCID_PC_TO_RDR_SET_DATA_RATE_AND_CLOCK:
		return CCID_RDR_TO_PC_DATA_RATE_AND_CLOCK;
	default:
		return CCID_RDR_TO_PC_SLOT_STATUS;
	}
}

/**
 * @brief Translate a backend error into a CCID failure response.
 * @param ccid Private state of the addressed CCID interface.
 * @param request Validated CCID command header from the host.
 * @param error Negative backend errno or positive CCID bError.
 *
 * @return Encoded failure reply length in bytes, including the CCID header.
 *         The error is carried in the reply, not returned as an errno.
 */
static size_t ccid_error_response(struct ccid_context *ccid, const struct ccid_bulk_header *request,
				  int error)
{
	uint8_t type = ccid_response_type(request->message_type);
	uint8_t ccid_error = CCID_ERROR_HW_ERROR;
	size_t payload_length = 0;
	uint8_t specific = 0;
	uint8_t status;

	switch (type) {
	case CCID_RDR_TO_PC_PARAMETERS:
		specific = ccid->backend.protocol;
		break;
	case CCID_RDR_TO_PC_SLOT_STATUS:
		specific = ccid->backend.clock_status;
		break;
	case CCID_RDR_TO_PC_DATA_RATE_AND_CLOCK:
		payload_length = CCID_DATA_RATE_AND_CLOCK_SIZE;
		sys_put_le32(ccid->backend.rate.clock_khz,
			     ccid->buffers->message + CCID_HEADER_SIZE);
		sys_put_le32(ccid->backend.rate.data_rate_bps,
			     ccid->buffers->message + CCID_HEADER_SIZE + CCID_DATA_RATE_OFFSET);
		break;
	default:
		break;
	}

	if (error > 0 && error <= UINT8_MAX) {
		ccid_error = error;
	} else if (error == -ENOTSUP) {
		ccid_error = CCID_ERROR_CMD_NOT_SUPPORTED;
	} else if (error == -EMSGSIZE) {
		ccid_error = CCID_ERROR_BAD_LENGTH;
	} else if (error == -EOVERFLOW) {
		ccid_error = CCID_ERROR_XFR_OVERRUN;
	} else if (error == -ECANCELED) {
		ccid_error = CCID_ERROR_ABORTED;
	} else if (error == -ETIMEDOUT || error == -ENODEV || error == -ENOTCONN) {
		ccid_error = CCID_ERROR_ICC_MUTE;
	}

	LOG_DBG("CCID %u: command 0x%02x seq %u failed (%d, bError 0x%02x)", ccid->instance->id,
		request->message_type, request->seq, error, ccid_error);

	status = request->slot == CCID_SLOT_ZERO ? ccid_card_status(ccid)
						 : CCID_STATUS_ICC_NOT_PRESENT;

	ccid_encode_header(ccid->buffers->message, type, request, payload_length,
			   status | CCID_STATUS_CMD_FAILED, ccid_error, specific);
	return CCID_HEADER_SIZE + payload_length;
}

/**
 * @brief Validate power selection, reset the card and return its ATR.
 * @param ccid Private state of the addressed CCID interface.
 * @param request Validated CCID command header from the host.
 *
 * @return Encoded success or failure reply length in bytes, including the CCID header
 *         and the ATR on successful power-on.
 */
static size_t ccid_handle_power_on(struct ccid_context *ccid,
				   const struct ccid_bulk_header *request)
{
	size_t atr_length = sizeof(ccid->buffers->message) - CCID_HEADER_SIZE;
	struct usbd_ccid_parameters parameters = {0};
	uint8_t voltage;
	int ret;

	ccid_reset_apdu(ccid);
	voltage = request->specific[CCID_SPECIFIC_POWER_SELECT_OFFSET];

	if (voltage > USBD_CCID_VOLTAGE_1V8 ||
	    (voltage == USBD_CCID_VOLTAGE_AUTO &&
	     (ccid->backend.object->config.features & USBD_CCID_AUTO_VOLTAGE) == 0) ||
	    (voltage != USBD_CCID_VOLTAGE_AUTO && (ccid->backend.object->config.voltage_support &
						   USBD_CCID_VOLTAGE_MASK(voltage)) == 0)) {
		ccid_encode_header(ccid->buffers->message, CCID_RDR_TO_PC_DATA_BLOCK, request, 0,
				   ccid_card_status(ccid) | CCID_STATUS_CMD_FAILED,
				   CCID_ERROR_BAD_POWER_SELECT, 0);
		return CCID_HEADER_SIZE;
	}

	ret = ccid->backend.object->ops->power_on(ccid->backend.object, voltage,
						  ccid->buffers->message + CCID_HEADER_SIZE,
						  &atr_length);
	if (ret != 0) {
		return ccid_error_response(ccid, request, ret);
	}
	if (atr_length == 0 || atr_length > sizeof(ccid->buffers->message) - CCID_HEADER_SIZE) {
		return ccid_error_response(ccid, request, -EIO);
	}

	/* Power-on may select a different protocol according to the backend's ATR. */
	ret = ccid->backend.object->ops->get_parameters(ccid->backend.object, &parameters);
	if (ret != 0 || !ccid_parameters_valid(ccid, &parameters)) {
		return ccid_error_response(ccid, request, ret != 0 ? ret : -EIO);
	}
	ccid->backend.protocol = parameters.protocol;
	ret = ccid_refresh_rate(ccid);
	if (ret != 0) {
		return ccid_error_response(ccid, request, ret);
	}
	ccid->backend.clock_status = USBD_CCID_CLOCK_RUNNING;
	ccid_encode_header(ccid->buffers->message, CCID_RDR_TO_PC_DATA_BLOCK, request, atr_length,
			   CCID_STATUS_ICC_ACTIVE, 0, 0);
	return CCID_HEADER_SIZE + atr_length;
}

/**
 * @brief Power off the card and return its slot status.
 * @param ccid Private state of the addressed CCID interface.
 * @param request Validated CCID command header from the host.
 *
 * @return CCID_HEADER_SIZE; success or failure is encoded in the response header.
 */
static size_t ccid_handle_power_off(struct ccid_context *ccid,
				    const struct ccid_bulk_header *request)
{
	int ret;

	ccid_reset_apdu(ccid);
	ret = ccid->backend.object->ops->power_off(ccid->backend.object);

	if (ret != 0) {
		return ccid_error_response(ccid, request, ret);
	}

	ccid->backend.clock_status = USBD_CCID_CLOCK_STOPPED_LOW;
	ccid_encode_header(ccid->buffers->message, CCID_RDR_TO_PC_SLOT_STATUS, request, 0,
			   ccid_card_status(ccid), 0, ccid->backend.clock_status);
	return CCID_HEADER_SIZE;
}

/**
 * @brief Emit the next response fragment and reset chaining after the last one.
 *
 * @param ccid Private state of the addressed CCID interface.
 * @param request Validated CCID command header from the host.
 *
 * @return Encoded fragment length in bytes, including the CCID header.
 */
static size_t ccid_apdu_response(struct ccid_context *ccid, const struct ccid_bulk_header *request)
{
	size_t available = ccid->apdu.response_length - ccid->apdu.response_offset;
	bool first = ccid->apdu.response_offset == 0 ? true : false;
	uint8_t chain = CCID_CHAIN_COMPLETE;
	size_t length;

	length = MIN(available, sizeof(ccid->buffers->message) - CCID_HEADER_SIZE);
	memcpy(ccid->buffers->message + CCID_HEADER_SIZE,
	       ccid->buffers->apdu_response + ccid->apdu.response_offset, length);
	ccid->apdu.response_offset += length;
	if (ccid->apdu.response_offset < ccid->apdu.response_length) {
		chain = first ? CCID_CHAIN_FIRST : CCID_CHAIN_MIDDLE;
	} else {
		chain = first ? CCID_CHAIN_COMPLETE : CCID_CHAIN_LAST;
		ccid_reset_apdu(ccid);
	}
	ccid_encode_header(ccid->buffers->message, CCID_RDR_TO_PC_DATA_BLOCK, request, length,
			   ccid_card_status(ccid), 0, chain);
	return CCID_HEADER_SIZE + length;
}

/**
 * @brief Discard partial APDU state before encoding a command failure.
 *
 * @param ccid Private state of the addressed CCID interface.
 * @param request Validated CCID command header from the host.
 * @param error Negative backend errno or positive CCID bError.
 *
 * @return Encoded failure reply length in bytes, including the CCID header.
 */
static size_t ccid_apdu_error(struct ccid_context *ccid, const struct ccid_bulk_header *request,
			      int error)
{
	ccid_reset_apdu(ccid);
	return ccid_error_response(ccid, request, error);
}

/**
 * @brief Assemble an APDU, exchange it once, and segment the response.
 * @param ccid Private state of the addressed CCID interface.
 * @param request Validated CCID command header from the host.
 * @param payload Command data following the CCID header.
 * @param payload_length Command data length in bytes.
 *
 * @return Encoded acknowledgment, response fragment or failure length in bytes,
 *         including the CCID header.
 */
static size_t ccid_handle_xfr_block(struct ccid_context *ccid,
				    const struct ccid_bulk_header *request, const uint8_t *payload,
				    size_t payload_length)
{
	bool extended = (ccid->backend.object->config.features & USBD_CCID_EXTENDED_APDU) != 0;
	uint16_t level = sys_get_le16(request->specific + CCID_SPECIFIC_LEVEL_LOW_OFFSET);
	size_t response_length = sizeof(ccid->buffers->apdu_response);
	int ret;

	if (ccid_card_status(ccid) != CCID_STATUS_ICC_ACTIVE) {
		return ccid_apdu_error(ccid, request, -ETIMEDOUT);
	}
	if (!extended && level != CCID_APDU_LEVEL_COMPLETE) {
		return ccid_apdu_error(ccid, request, CCID_ERROR_BAD_LEVEL);
	}
	if (!extended && payload_length > CCID_SHORT_COMMAND_MAX) {
		return ccid_apdu_error(ccid, request, -EMSGSIZE);
	}
	if (level == CCID_APDU_LEVEL_CONTINUE) {
		if (ccid->apdu.state != CCID_APDU_RESPONSE || payload_length != 0) {
			return ccid_apdu_error(ccid, request, CCID_ERROR_BAD_LEVEL);
		}
		return ccid_apdu_response(ccid, request);
	}
	if (level > CCID_APDU_LEVEL_MIDDLE) {
		return ccid_apdu_error(ccid, request, CCID_ERROR_BAD_LEVEL);
	}
	if (level == CCID_APDU_LEVEL_COMPLETE || level == CCID_APDU_LEVEL_FIRST) {
		if (ccid->apdu.state != CCID_APDU_IDLE) {
			return ccid_apdu_error(ccid, request, CCID_ERROR_BAD_LEVEL);
		}
		ccid->apdu.command_length = 0;
	} else if (ccid->apdu.state != CCID_APDU_COMMAND) {
		return ccid_apdu_error(ccid, request, CCID_ERROR_BAD_LEVEL);
	}
	if (payload_length > sizeof(ccid->buffers->apdu_command) - ccid->apdu.command_length) {
		return ccid_apdu_error(ccid, request, -EOVERFLOW);
	}
	memcpy(ccid->buffers->apdu_command + ccid->apdu.command_length, payload, payload_length);
	ccid->apdu.command_length += payload_length;
	if (level == CCID_APDU_LEVEL_FIRST || level == CCID_APDU_LEVEL_MIDDLE) {
		ccid->apdu.state = CCID_APDU_COMMAND;
		ccid_encode_header(ccid->buffers->message, CCID_RDR_TO_PC_DATA_BLOCK, request, 0,
				   ccid_card_status(ccid), 0, CCID_CHAIN_COMMAND_ACK);
		return CCID_HEADER_SIZE;
	}

	ret = ccid->backend.object->ops->transceive(
		ccid->backend.object, ccid->buffers->apdu_command, ccid->apdu.command_length,
		ccid->buffers->apdu_response, &response_length);
	if (ret != 0) {
		return ccid_apdu_error(ccid, request, ret);
	}
	if (response_length < CCID_APDU_STATUS_SIZE ||
	    response_length > sizeof(ccid->buffers->apdu_response) ||
	    response_length > CCID_RESPONSE_MAX) {
		return ccid_apdu_error(ccid, request, -EIO);
	}
	if (!extended && response_length > CCID_SHORT_RESPONSE_MAX) {
		return ccid_apdu_error(ccid, request, -EOVERFLOW);
	}
	ccid->apdu.state = CCID_APDU_RESPONSE;
	ccid->apdu.response_length = response_length;
	ccid->apdu.response_offset = 0;
	return ccid_apdu_response(ccid, request);
}

/**
 * @brief Forward protocol parameter commands to the addressed card backend.
 * @param ccid Private state of the addressed CCID interface.
 * @param request Validated CCID command header from the host.
 * @param payload Command data following the CCID header.
 * @param payload_length Command data length in bytes.
 *
 * @return Encoded parameter or failure reply length in bytes, including the CCID
 *         header and any returned current parameters.
 */
static size_t ccid_handle_parameters(struct ccid_context *ccid,
				     const struct ccid_bulk_header *request, const uint8_t *payload,
				     size_t payload_length)
{
	struct usbd_ccid_parameters parameters = {0};
	struct usbd_ccid_parameters requested;
	uint8_t error = 0;
	int ret;

	requested = (struct usbd_ccid_parameters){
		.protocol = request->specific[CCID_SPECIFIC_PROTOCOL_OFFSET],
		.data = payload,
		.length = payload_length,
	};

	if (request->message_type == CCID_PC_TO_RDR_SET_PARAMETERS) {
		if (!ccid_protocol_supported(ccid, requested.protocol)) {
			ret = CCID_ERROR_BAD_PROTOCOL;
		} else {
			ret = ccid->backend.object->ops->set_parameters(ccid->backend.object,
									&requested, &parameters);
		}
		if (ret > 0 && ret <= UINT8_MAX) {
			/* CCID 6.2.3 requires unchanged current settings on parameter errors. */
			error = ret;
			ret = ccid->backend.object->ops->get_parameters(ccid->backend.object,
									&parameters);
			if (ret != 0) {
				return ccid_error_response(ccid, request, error);
			}
		}
	} else {
		if (payload_length != 0) {
			return ccid_error_response(ccid, request, -EMSGSIZE);
		}
		if (request->message_type == CCID_PC_TO_RDR_GET_PARAMETERS) {
			ret = ccid->backend.object->ops->get_parameters(ccid->backend.object,
									&parameters);
		} else {
			ret = ccid->backend.object->ops->reset_parameters(ccid->backend.object,
									  &parameters);
		}
	}
	if (ret != 0) {
		return ccid_error_response(ccid, request, ret);
	}

	if (!ccid_parameters_valid(ccid, &parameters)) {
		return ccid_error_response(ccid, request, -EIO);
	}
	ccid->backend.protocol = parameters.protocol;
	if (error == 0 && request->message_type != CCID_PC_TO_RDR_GET_PARAMETERS) {
		ccid_reset_apdu(ccid);
	}
	if (parameters.length != 0) {
		memmove(ccid->buffers->message + CCID_HEADER_SIZE, parameters.data,
			parameters.length);
	}
	ccid_encode_header(ccid->buffers->message, CCID_RDR_TO_PC_PARAMETERS, request,
			   parameters.length,
			   ccid_card_status(ccid) | (error != 0 ? CCID_STATUS_CMD_FAILED : 0),
			   error, parameters.protocol);
	return CCID_HEADER_SIZE + parameters.length;
}

/**
 * @brief Apply supported host clock/rate selections, honoring automatic settings.
 *
 * @param ccid Private state of the addressed CCID interface.
 * @param request Validated CCID command header from the host.
 * @param payload Command data following the CCID header.
 * @param payload_length Command data length in bytes.
 *
 * @return Encoded clock/rate reply length in bytes, including the header and settings,
 *         for both success and failure.
 */
static size_t ccid_handle_rate(struct ccid_context *ccid, const struct ccid_bulk_header *request,
			       const uint8_t *payload, size_t payload_length)
{
	struct usbd_ccid_data_rate_and_clock requested;
	int ret;

	if (payload_length != CCID_DATA_RATE_AND_CLOCK_SIZE) {
		return ccid_error_response(ccid, request, -EMSGSIZE);
	}
	ret = ccid_refresh_rate(ccid);
	if (ret != 0) {
		return ccid_error_response(ccid, request, ret);
	}
	requested.clock_khz = sys_get_le32(payload);
	requested.data_rate_bps = sys_get_le32(payload + CCID_DATA_RATE_OFFSET);
	if (ccid->backend.object->config.features & USBD_CCID_AUTO_CLOCK) {
		requested.clock_khz = ccid->backend.rate.clock_khz;
	}
	if (ccid->backend.object->config.features & USBD_CCID_AUTO_DATA_RATE) {
		requested.data_rate_bps = ccid->backend.rate.data_rate_bps;
	}
	if (!ccid_rate_valid(ccid, &requested)) {
		return ccid_error_response(ccid, request, -ENOTSUP);
	}
	if (requested.clock_khz != ccid->backend.rate.clock_khz ||
	    requested.data_rate_bps != ccid->backend.rate.data_rate_bps) {
		if (ccid->backend.object->ops->set_data_rate_and_clock == NULL) {
			return ccid_error_response(ccid, request, -ENOTSUP);
		}
		ret = ccid->backend.object->ops->set_data_rate_and_clock(ccid->backend.object,
									 &requested);
		if (ret != 0) {
			return ccid_error_response(ccid, request, ret);
		}
		if (!ccid_rate_valid(ccid, &requested)) {
			return ccid_error_response(ccid, request, -EIO);
		}
		ccid->backend.rate = requested;
	}
	sys_put_le32(ccid->backend.rate.clock_khz, ccid->buffers->message + CCID_HEADER_SIZE);
	sys_put_le32(ccid->backend.rate.data_rate_bps,
		     ccid->buffers->message + CCID_HEADER_SIZE + CCID_DATA_RATE_OFFSET);
	ccid_encode_header(ccid->buffers->message, CCID_RDR_TO_PC_DATA_RATE_AND_CLOCK, request,
			   CCID_DATA_RATE_AND_CLOCK_SIZE, ccid_card_status(ccid), 0, 0);
	return CCID_HEADER_SIZE + CCID_DATA_RATE_AND_CLOCK_SIZE;
}

/**
 * @brief Forward clock start/stop requests and validate the reported clock status.
 *
 * @param ccid Private state of the addressed CCID interface.
 * @param request Validated CCID command header from the host.
 *
 * @return CCID_HEADER_SIZE; success or failure is encoded in the response header.
 */
static size_t ccid_handle_clock(struct ccid_context *ccid, const struct ccid_bulk_header *request)
{
	uint8_t action = request->specific[CCID_SPECIFIC_CLOCK_COMMAND_OFFSET];
	uint8_t status = ccid->backend.clock_status;
	int ret;

	if (action > CCID_CLOCK_STOP) {
		return ccid_error_response(ccid, request, CCID_ERROR_BAD_CLOCK_COMMAND);
	}
	if (ccid->backend.object->ops->set_clock == NULL ||
	    (action == CCID_CLOCK_STOP &&
	     (ccid->backend.object->config.features & USBD_CCID_CLOCK_STOP) == 0)) {
		return ccid_error_response(ccid, request, -ENOTSUP);
	}
	ret = ccid->backend.object->ops->set_clock(ccid->backend.object, action == CCID_CLOCK_STOP,
						   &status);
	if (ret != 0) {
		return ccid_error_response(ccid, request, ret);
	}
	if (status > USBD_CCID_CLOCK_STOPPED_UNKNOWN ||
	    (action == CCID_CLOCK_RESTART && status != USBD_CCID_CLOCK_RUNNING) ||
	    (action == CCID_CLOCK_STOP && status == USBD_CCID_CLOCK_RUNNING)) {
		return ccid_error_response(ccid, request, -EIO);
	}
	ccid->backend.clock_status = status;
	ccid_encode_header(ccid->buffers->message, CCID_RDR_TO_PC_SLOT_STATUS, request, 0,
			   ccid_card_status(ccid), 0, status);
	return CCID_HEADER_SIZE;
}

/**
 * @brief Validate and dispatch one complete CCID bulk command.
 * @param ccid Private state of the addressed CCID interface.
 *
 * @retval 0 if the received command is too short to supply a complete CCID header.
 * @return Otherwise, the encoded success or failure reply length in bytes,
 *         including the CCID header.
 */
static size_t ccid_process_command(struct ccid_context *ccid)
{
	const uint8_t *payload = ccid->buffers->message + CCID_HEADER_SIZE;
	const struct ccid_bulk_header *request = &ccid->worker.request;
	size_t payload_length;
	int ret;

	if (ccid->worker.command_length < CCID_HEADER_SIZE) {
		return 0;
	}

	LOG_DBG("CCID %u: command 0x%02x seq %u, %zu bytes", ccid->instance->id,
		request->message_type, request->seq, ccid->worker.command_length);
	ret = ccid_validate_request(request, ccid->worker.command_length);
	if (ret != 0) {
		return ccid_error_response(ccid, request, ret);
	}
	payload_length = sys_le32_to_cpu(request->length);

	switch (request->message_type) {
	case CCID_PC_TO_RDR_ICC_POWER_ON:
	case CCID_PC_TO_RDR_ICC_POWER_OFF:
	case CCID_PC_TO_RDR_GET_SLOT_STATUS:
	case CCID_PC_TO_RDR_ICC_CLOCK:
	case CCID_PC_TO_RDR_ABORT:
		if (payload_length != 0) {
			return ccid_error_response(ccid, request, -EMSGSIZE);
		}
		break;
	default:
		break;
	}

	switch (request->message_type) {
	case CCID_PC_TO_RDR_ICC_POWER_ON:
		return ccid_handle_power_on(ccid, request);
	case CCID_PC_TO_RDR_ICC_POWER_OFF:
		return ccid_handle_power_off(ccid, request);
	case CCID_PC_TO_RDR_GET_SLOT_STATUS:
		ccid_encode_header(ccid->buffers->message, CCID_RDR_TO_PC_SLOT_STATUS, request, 0,
				   ccid_card_status(ccid), 0, ccid->backend.clock_status);
		return CCID_HEADER_SIZE;
	case CCID_PC_TO_RDR_XFR_BLOCK:
		return ccid_handle_xfr_block(ccid, request, payload, payload_length);
	case CCID_PC_TO_RDR_GET_PARAMETERS:
	case CCID_PC_TO_RDR_RESET_PARAMETERS:
	case CCID_PC_TO_RDR_SET_PARAMETERS:
		return ccid_handle_parameters(ccid, request, payload, payload_length);
	case CCID_PC_TO_RDR_SET_DATA_RATE_AND_CLOCK:
		return ccid_handle_rate(ccid, request, payload, payload_length);
	case CCID_PC_TO_RDR_ICC_CLOCK:
		return ccid_handle_clock(ccid, request);
	case CCID_PC_TO_RDR_ABORT:
		ccid_encode_header(ccid->buffers->message, CCID_RDR_TO_PC_SLOT_STATUS, request, 0,
				   ccid_card_status(ccid), 0, 0);
		return CCID_HEADER_SIZE;
	default:
		return ccid_error_response(ccid, request, -ENOTSUP);
	}
}

/**
 * @brief Allow result publication only in its active, non-aborted USB generation.
 *
 * @param ccid Private state of the addressed CCID interface.
 * @param generation USB session generation captured when the command was queued.
 *
 * @return True if USB is available, the generation matches and no ABORT is pending;
 *         false if the result must be discarded.
 */
static bool ccid_command_current(struct ccid_context *ccid, uint32_t generation)
{
	return ccid_available(ccid) && ccid->abort.state == CCID_ABORT_IDLE &&
	       generation == ccid->generation;
}

/**
 * @brief Request cancellation through the optional nonblocking backend hook.
 * @param ccid Private state of the addressed CCID interface.
 *
 * @pre The instance lock is held.
 */
static void ccid_cancel_backend(struct ccid_context *ccid)
{
	int ret;

	if (ccid->worker.busy && ccid->backend.object->ops->abort != NULL) {
		ret = ccid->backend.object->ops->abort(ccid->backend.object);
		if (ret != 0) {
			LOG_WRN("CCID %u: backend cancellation failed (%d)", ccid->instance->id,
				ret);
		}
	}
}

/**
 * @brief Execute one queued command or deferred power-off and publish current results.
 *
 * Backend callbacks run outside the instance lock. Their buffers remain owned
 * by the worker until the callback returns, even after USB invalidation.
 *
 * @param ccid Private state of the addressed CCID interface.
 */
static void ccid_worker_step(struct ccid_context *ccid)
{
	const struct ccid_bulk_header *response = (const void *)ccid->buffers->message;
	const struct ccid_bulk_header *request = &ccid->worker.request;
	uint32_t generation;
	size_t length = 0;
	bool cleanup;
	bool execute;
	int ret;

	if (!ccid_lock(ccid)) {
		return;
	}
	generation = ccid->worker.generation;
	cleanup = ccid->worker.reset_pending;
	if (cleanup) {
		ccid->worker.reset_pending = false;
		ccid->worker.busy = true;
	} else if (!ccid->worker.busy) {
		ccid_unlock(ccid);
		return;
	}
	execute = !cleanup && ccid_command_current(ccid, generation);

	ccid_unlock(ccid);
	if (cleanup) {
		ret = ccid->backend.object->ops->power_off(ccid->backend.object);
		if (ret != 0) {
			LOG_WRN("CCID %u: deferred power-off failed (%d)", ccid->instance->id, ret);
		}
	} else if (execute) {
		length = ccid_process_command(ccid);
	}
	if (!ccid_lock(ccid)) {
		return;
	}
	ccid->worker.busy = false;
	if (execute && ccid_command_current(ccid, generation)) {
		ccid->tx.length = length;

		if (length != 0 && request->message_type == CCID_PC_TO_RDR_ICC_POWER_ON &&
		    (response->specific[CCID_SPECIFIC_STATUS_OFFSET] & CCID_STATUS_CMD_FAILED) ==
			    0) {
			atomic_clear_bit(&ccid->state, CCID_NEEDS_POWER_ON);
		}
	} else {
		ccid_reset_apdu(ccid);
	}
	if (cleanup) {
		ccid->backend.protocol = ccid->backend.object->config.default_protocol;
		if (ret == 0) {
			ccid->backend.clock_status = USBD_CCID_CLOCK_STOPPED_LOW;
		}
	}
	if (ccid->worker.reset_pending) {
		k_sem_give(&ccid->worker.sem);
	}
	(void)ccid_schedule_io(ccid, K_NO_WAIT);
	ccid_unlock(ccid);
}

/**
 * @brief Wait for commands and deferred cleanup on this interface's dedicated thread.
 *
 * @param instance_context CCID interface context supplied as the first thread argument.
 * @param unused_argument_2 Unused second argument required by the Zephyr thread entry signature.
 * @param unused_argument_3 Unused third argument required by the Zephyr thread entry signature.
 */
static void ccid_thread_fn(void *instance_context, void *unused_argument_2, void *unused_argument_3)
{
	struct ccid_context *ccid = instance_context;

	ARG_UNUSED(unused_argument_2);
	ARG_UNUSED(unused_argument_3);
	while (true) {
		int ret = k_sem_take(&ccid->worker.sem, K_FOREVER);

		if (ret != 0) {
			LOG_ERR("CCID %u: worker wait failed (%d)", ccid->instance->id, ret);
			continue;
		}
		ccid_worker_step(ccid);
	}
}

/**
 * @brief Build a failure header without touching the worker-owned message workspace.
 *
 * @param ccid Private state of the addressed CCID interface.
 * @param request Received command header supplying the reply type, slot and sequence number.
 * @param error CCID bError; replaced by CCID_ERROR_BAD_SLOT for an invalid slot.
 */
static void ccid_auxiliary(struct ccid_context *ccid, const struct ccid_bulk_header *request,
			   uint8_t error)
{
	uint8_t status = CCID_STATUS_ICC_NOT_PRESENT;

	if (request->slot != CCID_SLOT_ZERO) {
		error = CCID_ERROR_BAD_SLOT;
	} else {
		status = ccid_card_status(ccid);
	}
	LOG_DBG("CCID %u: command 0x%02x seq %u rejected, bError 0x%02x", ccid->instance->id,
		request->message_type, request->seq, error);
	ccid_encode_header(ccid->tx.auxiliary, ccid_response_type(request->message_type), request,
			   0, status | CCID_STATUS_CMD_FAILED, error, 0);
	ccid->tx.auxiliary_ready = true;
}

/**
 * @brief Send a coalesced slot-state notification, retaining it on submission failure.
 *
 * @param ccid Private state of the addressed CCID interface.
 *
 * @retval 0 if a notification was queued, one is already pending, or none is needed.
 * @retval -EACCES if the interface cannot exchange USB data.
 * @retval -ENOMEM if no transfer buffer could be allocated.
 * @return Other enqueue errors from usbd_ep_enqueue().
 */
static int ccid_submit_notification(struct ccid_context *ccid)
{
	struct net_buf *buf;
	int ret;

	if (!ccid_available(ccid)) {
		return -EACCES;
	}
	if (ccid->usb.notify_buf != NULL ||
	    !atomic_test_and_clear_bit(&ccid->state, CCID_NOTIFY_PENDING)) {
		return 0;
	}
	buf = usbd_ep_buf_alloc(ccid->usb.class_data,
				ccid_ep_addr(ccid->usb.class_data, CCID_EP_INTERRUPT_IN),
				CCID_NOTIFY_SLOT_CHANGE_SIZE);
	if (buf == NULL) {
		atomic_set_bit(&ccid->state, CCID_NOTIFY_PENDING);
		return -ENOMEM;
	}
	(void)net_buf_add_u8(buf, CCID_RDR_TO_PC_NOTIFY_SLOT_CHANGE);
	(void)net_buf_add_u8(buf, CCID_NOTIFY_SLOT_ZERO_CHANGED |
					  (ccid_card_status(ccid) != CCID_STATUS_ICC_NOT_PRESENT
						   ? CCID_NOTIFY_SLOT_ZERO_PRESENT
						   : 0));
	ret = ccid_enqueue_owned(ccid, buf, &ccid->usb.notify_buf);
	if (ret != 0) {
		atomic_set_bit(&ccid->state, CCID_NOTIFY_PENDING);
	}
	return ret;
}

/**
 * @brief Check that neither backend work nor a bulk-IN reply owns the message workspace.
 *
 * @param ccid Private state of the addressed CCID interface.
 *
 * @return True if no ABORT, cleanup, worker operation or bulk-IN reply blocks reuse;
 *         false otherwise.
 */
static bool ccid_workspace_available(const struct ccid_context *ccid)
{
	return ccid->abort.state == CCID_ABORT_IDLE && !ccid->worker.busy &&
	       !ccid->worker.reset_pending && ccid->tx.length == 0 && ccid->tx.buf == NULL;
}

/**
 * @brief Decide whether OUT can accept a command, ABORT, partial frame or trailing ZLP.
 *
 * @param ccid Private state of the addressed CCID interface.
 *
 * @return True to arm bulk-OUT, including header-only reception while busy;
 *         false to pause reception.
 */
static bool ccid_can_receive(const struct ccid_context *ccid)
{
	if (ccid->tx.auxiliary_ready) {
		return false;
	}
	/* The Bulk ABORT half remains receivable while cleanup is pending. */
	if (ccid->abort.state == CCID_ABORT_WAIT_BULK) {
		return true;
	}
	if (ccid->worker.reset_pending) {
		return false;
	}
	if (ccid->rx.expect_zlp || ccid->rx.length != 0) {
		return true;
	}
	return ccid_workspace_available(ccid);
}

/**
 * @brief Submit the highest-priority pending reply or a due time extension.
 *
 * Priority: failure reply, matched ABORT, final response, time extension.
 * Packet buffers copy these bytes; the pending reply is retained until a
 * successful completion so allocation/enqueue/transfer errors can retry it.
 *
 * @param ccid Private state of the addressed CCID interface.
 * @param[in,out] next_extension Time until the next extension check, in milliseconds.
 *                              Updated when an eligible extension is checked;
 *                              otherwise left unchanged.
 * @retval 0 if a reply was queued, one is already in flight, or no action is due.
 * @return An error from the selected submission helper if allocation or enqueue failed.
 */
static int ccid_submit_pending(struct ccid_context *ccid, int64_t *next_extension)
{
	const struct ccid_bulk_header *request = &ccid->worker.request;
	struct ccid_bulk_header reply;
	int ret;

	if (ccid->tx.buf != NULL) {
		return 0;
	}
	if (ccid->tx.auxiliary_ready) {
		return ccid_submit_packet(ccid, ccid->tx.auxiliary, CCID_HEADER_SIZE,
					  CCID_IN_AUXILIARY);
	}
	if (ccid->abort.state == CCID_ABORT_MATCHED && !ccid->worker.busy &&
	    !ccid->worker.reset_pending) {
		reply = (struct ccid_bulk_header){
			.message_type = CCID_RDR_TO_PC_SLOT_STATUS,
			.seq = ccid->abort.seq,
			.specific = {ccid_card_status(ccid), 0, ccid->backend.clock_status},
		};
		ccid_reset_apdu(ccid);
		return ccid_submit_packet(ccid, &reply, sizeof(reply), CCID_IN_ABORT);
	}
	if (ccid->tx.length != 0) {
		return ccid_submit_packet(ccid, ccid->buffers->message, ccid->tx.length,
					  CCID_IN_FINAL);
	}
	if (!ccid->worker.busy || ccid->worker.reset_pending ||
	    !ccid_command_current(ccid, ccid->worker.generation) ||
	    ccid->backend.object->config.time_extension_interval_ms == 0) {
		return 0;
	}
	*next_extension = ccid->tx.extension_deadline - k_uptime_get();
	if (*next_extension > 0) {
		return 0;
	}
	ccid_encode_header(&reply, ccid_response_type(request->message_type), request, 0,
			   CCID_STATUS_TIME_EXTENSION | ccid_card_status(ccid),
			   ccid->backend.object->config.time_extension_multiplier, 0);
	ret = ccid_submit_packet(ccid, &reply, sizeof(reply), CCID_IN_EXTENSION);
	*next_extension = ccid->backend.object->config.time_extension_interval_ms;
	ccid->tx.extension_deadline = k_uptime_get() + *next_extension;
	return ret;
}

/**
 * @brief Drive pending replies, OUT reception and notifications under the instance lock.
 *
 * Reschedule after transient I/O failures or at the next time-extension deadline.
 *
 * @param work Work item embedded in the interface's delayable I/O work.
 */
static void ccid_io_work(struct k_work *work)
{
	int64_t next_extension = 0;
	struct ccid_context *ccid;
	int notify_ret;
	int ret;

	ccid = CONTAINER_OF(k_work_delayable_from_work(work), struct ccid_context, io_work);
	if (!ccid_lock(ccid)) {
		return;
	}
	if (!ccid_available(ccid)) {
		ccid_unlock(ccid);
		return;
	}
	ret = ccid_submit_pending(ccid, &next_extension);
	if (ret == 0 && ccid_can_receive(ccid)) {
		ret = ccid_submit_out(ccid);
	}
	notify_ret = ccid_submit_notification(ccid);

	if (ret != 0 || notify_ret != 0) {
		LOG_DBG("CCID %u: retrying I/O (%d), notification (%d)", ccid->instance->id, ret,
			notify_ret);
		(void)ccid_schedule_io(ccid, K_MSEC(CCID_IO_RETRY_MS));
	} else if (next_extension > 0) {
		(void)ccid_schedule_io(ccid, K_MSEC(next_extension));
	}
	ccid_unlock(ccid);
}

/**
 * @brief Resolve a logical devicetree ID without allocating storage for ID gaps.
 *
 * @param instance Logical instance ID from the enabled CCID devicetree node.
 *
 * @return Matching context, or NULL when no enabled devicetree node has this ID.
 */
static struct ccid_context *ccid_get_instance(unsigned int instance)
{
	for (size_t i = 0; i < CCID_INSTANCE_COUNT; i++) {
		if (ccid_contexts[i].instance->id == instance) {
			return &ccid_contexts[i];
		}
	}

	return NULL;
}

/**
 * @brief Require a nonempty capability table with nonzero, strictly ascending entries.
 *
 * @param table Clock-frequency or data-rate capability table; NULL is rejected.
 * @param count Number of entries in the table.
 *
 * @return True for a nonempty, nonzero, strictly ascending table; false otherwise.
 */
static bool ccid_table_valid(const uint32_t *table, uint8_t count)
{
	if (table == NULL || count == 0 || table[0] == 0) {
		return false;
	}
	for (size_t i = 1; i < count; i++) {
		if (table[i] <= table[i - 1]) {
			return false;
		}
	}
	return true;
}

/**
 * @brief Validate advertised APDU capabilities and their required optional callbacks.
 *
 * @param config Reader capabilities to validate; NULL is rejected.
 * @param ops Non-NULL backend operation table checked for capability-dependent callbacks.
 *
 * @return True if capabilities meet class limits and the required clock-stop hook
 *         is present when advertised; false otherwise.
 */
static bool ccid_config_valid(const struct usbd_ccid_reader_config *config,
			      const struct usbd_ccid_card_ops *ops)
{
	uint32_t negotiation;
	uint32_t level;

	if (config == NULL || config->protocols == 0 ||
	    (config->protocols & ~CCID_SUPPORTED_PROTOCOLS) != 0 ||
	    config->default_protocol >= CCID_PROTOCOL_MASK_BITS ||
	    (config->protocols & BIT(config->default_protocol)) == 0 ||
	    (config->voltage_support & ~CCID_SUPPORTED_VOLTAGES) != 0 ||
	    (config->features & ~CCID_SUPPORTED_FEATURES) != 0 ||
	    !ccid_table_valid(config->clock_frequencies, config->clock_count) ||
	    !ccid_table_valid(config->data_rates, config->data_rate_count) ||
	    (config->time_extension_interval_ms != 0 && config->time_extension_multiplier == 0)) {
		return false;
	}
	level = config->features & (USBD_CCID_SHORT_APDU | USBD_CCID_EXTENDED_APDU);
	negotiation = config->features & (USBD_CCID_AUTO_NEGOTIATION | USBD_CCID_AUTO_PPS);
	if ((level != USBD_CCID_SHORT_APDU && level != USBD_CCID_EXTENDED_APDU) ||
	    (negotiation != USBD_CCID_AUTO_NEGOTIATION && negotiation != USBD_CCID_AUTO_PPS) ||
	    (config->features & USBD_CCID_AUTO_PARAMETERS) == 0 ||
	    ((config->features & USBD_CCID_CLOCK_STOP) != 0 && ops->set_clock == NULL)) {
		return false;
	}
	if (config->protocols & USBD_CCID_PROTOCOL_MASK(USBD_CCID_PROTOCOL_T1)) {
		return config->max_ifsd > 0 && config->max_ifsd <= USBD_CCID_T1_IFS_MAX;
	}
	return config->max_ifsd == 0;
}

/**
 * @brief Validate and bind an application-owned backend to an unused CCID interface.
 *
 * @param instance Logical instance ID from the enabled CCID devicetree node.
 * @param backend Application-owned backend whose object and referenced data must remain valid
 *                for the class lifetime.
 *
 * @retval 0 if the backend was bound and descriptor capabilities were initialized.
 * @retval -ENODEV if no enabled devicetree node has this instance ID.
 * @retval -EBUSY if the interface is already enabled.
 * @retval -EINVAL if the backend, capabilities or mandatory callbacks are invalid.
 * @retval -EALREADY if the instance or backend object is already bound.
 */
int usbd_ccid_register_backend(unsigned int instance, struct usbd_ccid_backend *backend)
{
	struct ccid_context *ccid = ccid_get_instance(instance);
	const struct usbd_ccid_reader_config *config;
	const struct usbd_ccid_card_ops *ops;

	if (ccid == NULL) {
		return -ENODEV;
	}
	if (atomic_test_bit(&ccid->state, CCID_ENABLED)) {
		return -EBUSY;
	}
	if (backend == NULL) {
		return -EINVAL;
	}
	ops = backend->ops;
	config = &backend->config;
	if (ops == NULL || !ccid_config_valid(config, ops) || ops->get_state == NULL ||
	    ops->power_on == NULL || ops->power_off == NULL || ops->transceive == NULL ||
	    ops->get_parameters == NULL || ops->set_parameters == NULL ||
	    ops->reset_parameters == NULL || ops->get_data_rate_and_clock == NULL) {
		LOG_ERR("CCID %u: invalid backend capabilities or callbacks", instance);
		return -EINVAL;
	}
	if (ccid->backend.object != NULL) {
		return -EALREADY;
	}

	for (size_t i = 0; i < CCID_INSTANCE_COUNT; i++) {
		if (ccid_contexts[i].backend.object == backend) {
			return -EALREADY;
		}
	}
	ccid->backend.object = backend;
	ccid->backend.protocol = config->default_protocol;
	ccid->backend.rate.clock_khz = config->clock_frequencies[0];
	ccid->backend.rate.data_rate_bps = config->data_rates[0];
	ccid->instance->desc->ccid.dwProtocols = sys_cpu_to_le32(config->protocols);
	ccid->instance->desc->ccid.dwFeatures = sys_cpu_to_le32(config->features);
	ccid->instance->desc->ccid.bVoltageSupport = config->voltage_support;
	ccid->instance->desc->ccid.dwMaxIFSD = sys_cpu_to_le32(config->max_ifsd);
	ccid->instance->desc->ccid.dwDefaultClock = sys_cpu_to_le32(config->clock_frequencies[0]);
	ccid->instance->desc->ccid.dwMaximumClock =
		sys_cpu_to_le32(config->clock_frequencies[config->clock_count - 1]);
	ccid->instance->desc->ccid.bNumClockSupported = config->clock_count;
	ccid->instance->desc->ccid.dwDataRate = sys_cpu_to_le32(config->data_rates[0]);
	ccid->instance->desc->ccid.dwMaxDataRate =
		sys_cpu_to_le32(config->data_rates[config->data_rate_count - 1]);
	ccid->instance->desc->ccid.bNumDataRatesSupported = config->data_rate_count;
	return 0;
}

/**
 * @brief Queue a slot notification after the backend has published its new card state.
 *
 * @param instance Logical instance ID from the enabled CCID devicetree node.
 *
 * @retval 0 if notification work was queued or coalesced; USB delivery is asynchronous.
 * @retval -ENODEV if the instance ID has no enabled devicetree node or no registered backend.
 * @retval -EACCES if the CCID interface is not enabled.
 * @return Negative work-queue error if notification work could not be queued.
 */
int usbd_ccid_card_changed(unsigned int instance)
{
	struct ccid_context *ccid = ccid_get_instance(instance);

	if (ccid == NULL) {
		return -ENODEV;
	}
	if (ccid->backend.object == NULL) {
		return -ENODEV;
	}
	if (!atomic_test_bit(&ccid->state, CCID_ENABLED)) {
		return -EACCES;
	}

	atomic_set_bit(&ccid->state, CCID_NOTIFY_PENDING);
	return ccid_schedule_io(ccid, K_NO_WAIT);
}

/**
 * @brief Dispatch a completed frame or handle it without taking the message workspace.
 *
 * Busy/ABORT traffic retains just its header and drains payloads. A normal
 * command is dispatched only if its complete payload was stored.
 * @param ccid Private state of the addressed CCID interface.
 * @param length Total received frame length in bytes, including the CCID header.
 *
 * @pre The instance lock is held.
 */
static void ccid_accept_message(struct ccid_context *ccid, size_t length)
{
	const struct ccid_bulk_header *request = &ccid->rx.header;
	int valid = ccid_validate_request(request, length);

	if (valid != 0) {
		LOG_WRN("CCID %u: invalid command frame, slot %u seq %u (%d)", ccid->instance->id,
			request->slot, request->seq, valid);
	}
	ccid->abort.last_seq = CCID_ABORT_SEQ_NONE;
	if (valid == 0 && request->message_type == CCID_PC_TO_RDR_ABORT && request->length == 0) {
		ccid->abort.last_seq = request->seq;
		if (ccid->abort.state != CCID_ABORT_IDLE) {
			if (request->seq == ccid->abort.seq) {
				ccid->abort.state = CCID_ABORT_MATCHED;
			} else {
				ccid_auxiliary(ccid, request, CCID_ERROR_ABORTED);
			}
		}
	} else if (ccid->abort.state != CCID_ABORT_IDLE) {
		ccid_auxiliary(ccid, request, CCID_ERROR_ABORTED);
	} else if (ccid->rx.store_payload && ccid_workspace_available(ccid)) {
		/* Capture before the worker can replace the message with its reply.
		 * Time extensions and power-on completion use this immutable header.
		 */
		ccid->worker.request = *request;
		ccid->worker.command_length = length;
		ccid->worker.generation = ccid->generation;
		ccid->worker.busy = true;
		ccid->tx.extension_deadline =
			k_uptime_get() + ccid->backend.object->config.time_extension_interval_ms;
		k_sem_give(&ccid->worker.sem);
	} else {
		ccid_auxiliary(ccid, request, CCID_ERROR_SLOT_BUSY);
	}
}

/**
 * @brief Assemble OUT frames, drain unaccepted payloads and dispatch complete messages.
 * @param ccid Private state of the addressed CCID interface.
 * @param buf Completed bulk-OUT packet whose bytes are consumed without taking ownership.
 *
 * @pre The instance lock is held.
 */
static void ccid_receive_packet(struct ccid_context *ccid, const struct net_buf *buf)
{
	size_t mps = ccid_bulk_mps(ccid->usb.class_data);

	ccid->rx.expect_zlp = false;
	if (buf->len > mps) {
		ccid->rx.length = 0;
		return;
	}
	if (ccid->rx.length == 0) {
		/* Keep this decision for the whole frame, even if a pending reply
		 * completes between packets. Previously discarded bytes are gone.
		 */
		ccid->rx.store_payload = ccid_workspace_available(ccid);
	}
	if (ccid->rx.length < sizeof(ccid->rx.header)) {
		size_t offset = (size_t)ccid->rx.length;
		size_t copy;

		copy = MIN((size_t)buf->len, sizeof(ccid->rx.header) - offset);
		memcpy((uint8_t *)&ccid->rx.header + offset, buf->data, copy);
	}
	if (ccid->rx.store_payload && ccid->rx.length < sizeof(ccid->buffers->message)) {
		size_t offset = (size_t)ccid->rx.length;
		size_t copy;

		copy = MIN((size_t)buf->len, sizeof(ccid->buffers->message) - offset);
		memcpy(ccid->buffers->message + offset, buf->data, copy);
	}
	ccid->rx.length += buf->len;
	if (ccid->rx.length >= CCID_HEADER_SIZE) {
		uint64_t expected =
			(uint64_t)sys_le32_to_cpu(ccid->rx.header.length) + CCID_HEADER_SIZE;

		if (ccid->rx.length >= expected || buf->len < mps) {
			if (ccid->rx.length > sizeof(ccid->buffers->message)) {
				LOG_WRN("CCID %u: oversized OUT frame, seq %u", ccid->instance->id,
					ccid->rx.header.seq);
				/* Keep the header and drain oversized payloads with 64-bit
				 * accounting, including dwLength == UINT32_MAX.
				 */
				ccid->abort.last_seq = CCID_ABORT_SEQ_NONE;
				ccid_auxiliary(ccid, &ccid->rx.header,
					       ccid->abort.state != CCID_ABORT_IDLE
						       ? CCID_ERROR_ABORTED
						       : CCID_ERROR_BAD_LENGTH);
			} else {
				ccid_accept_message(ccid, (size_t)ccid->rx.length);
			}
			ccid->rx.length = 0;
			/* A host appending an OUT ZLP waits for its completion before
			 * reading IN. Accept it while the backend or response is pending.
			 */
			ccid->rx.expect_zlp = buf->len == mps;
		}
	} else if (buf->len < mps) {
		/* A short transfer without a complete header has no slot/sequence
		 * for a reply. This also ignores an optional trailing OUT ZLP.
		 */
		ccid->rx.length = 0;
	}
}

/**
 * @brief Complete USB transfers and rearm the appropriate endpoint.
 * @param c_data USB class instance handling this request.
 * @param buf USB endpoint transfer buffer.
 * @param err Zero or a negative operation error.
 *
 * @retval 0 if the completed transfer buffer was released successfully.
 * @return Otherwise, the error from usbd_ep_buf_free(). The transfer status in err
 *         is processed internally and is not returned.
 */
static int ccid_request_handler(struct usbd_class_data *c_data, struct net_buf *buf, const int err)
{
	struct ccid_context *ccid = usbd_class_get_private(c_data);
	int ret;

	if (!ccid_lock(ccid)) {
		return usbd_ep_buf_free(usbd_class_get_ctx(c_data), buf);
	}
	if (err != 0 && err != -ECONNABORTED && ccid_available(ccid)) {
		LOG_WRN("CCID %u: endpoint 0x%02x transfer failed (%d)", ccid->instance->id,
			udc_get_buf_info(buf)->ep, err);
	}
	/* Match the actual buffer, not just its endpoint: a cancelled transfer
	 * from an earlier generation may complete after reconfiguration.
	 */
	if (buf == ccid->rx.buf) {
		ccid->rx.buf = NULL;
		if (err == 0 && ccid_available(ccid)) {
			ccid_receive_packet(ccid, buf);
		} else {
			ccid->rx.length = 0;
			ccid->rx.expect_zlp = false;
		}
	} else if (buf == ccid->tx.buf) {
		ccid->tx.buf = NULL;
		if (err == 0) {
			switch (ccid->tx.kind) {
			case CCID_IN_FINAL:
				ccid->tx.length = 0;
				break;
			case CCID_IN_AUXILIARY:
				ccid->tx.auxiliary_ready = false;
				break;
			case CCID_IN_ABORT:
				ccid->abort.state = CCID_ABORT_IDLE;
				ccid->abort.last_seq = CCID_ABORT_SEQ_NONE;
				break;
			case CCID_IN_EXTENSION:
			case CCID_IN_STALE:
				break;
			}
		}
	} else if (buf == ccid->usb.notify_buf) {
		ccid->usb.notify_buf = NULL;
		if (err != 0) {
			atomic_set_bit(&ccid->state, CCID_NOTIFY_PENDING);
		}
	}
	ccid_unlock(ccid);
	ret = usbd_ep_buf_free(usbd_class_get_ctx(c_data), buf);

	(void)ccid_schedule_io(ccid, K_NO_WAIT);
	return ret;
}

/**
 * @brief Answer CCID class-specific host read requests.
 * @param c_data USB class instance handling this request.
 * @param setup Decoded USB control request.
 *
 * @return Response buffer owned by the USB stack, or NULL if the request is
 *         unsupported or the response buffer could not be allocated.
 */
static struct net_buf *ccid_control_to_host(struct usbd_class_data *c_data,
					    const struct usb_setup_packet *setup)
{
	struct ccid_context *ccid = usbd_class_get_private(c_data);
	const uint32_t *table;
	struct net_buf *buf;
	size_t remaining;
	uint8_t count;

	if (setup->RequestType.recipient != USB_REQTYPE_RECIPIENT_INTERFACE ||
	    setup->wIndex != ccid->instance->desc->iface.bInterfaceNumber || setup->wValue != 0 ||
	    ccid->backend.object == NULL) {
		return NULL;
	}
	if (setup->bRequest == CCID_REQ_GET_CLOCK_FREQUENCIES) {
		table = ccid->backend.object->config.clock_frequencies;
		count = ccid->backend.object->config.clock_count;
	} else if (setup->bRequest == CCID_REQ_GET_DATA_RATES) {
		table = ccid->backend.object->config.data_rates;
		count = ccid->backend.object->config.data_rate_count;
	} else {
		return NULL;
	}
	remaining = MIN((size_t)setup->wLength, (size_t)count * sizeof(uint32_t));
	buf = usbd_ep_ctrl_data_in_alloc(usbd_class_get_ctx(c_data), remaining);
	if (buf == NULL) {
		LOG_ERR("CCID %u: control response allocation failed", ccid->instance->id);
		return NULL;
	}
	for (size_t i = 0; remaining != 0; i++) {
		uint32_t value = sys_cpu_to_le32(table[i]);
		size_t length;

		length = MIN(remaining, sizeof(value));
		(void)net_buf_add_mem(buf, &value, length);
		remaining -= length;
	}
	return buf;
}

/**
 * @brief Handle CCID class-specific host write requests.
 * @param c_data USB class instance handling this request.
 * @param setup Decoded USB control request.
 * @param buf USB endpoint transfer buffer.
 *
 * @retval 0 ABORT control request accepted.
 * @retval -ENOTSUP Unsupported request or unavailable interface.
 * @retval -EIO Mutex acquisition failed.
 */
static int ccid_control_to_dev(struct usbd_class_data *c_data, const struct usb_setup_packet *setup,
			       const struct net_buf *buf)
{
	struct ccid_context *ccid = usbd_class_get_private(c_data);

	ARG_UNUSED(buf);

	if (setup->RequestType.recipient != USB_REQTYPE_RECIPIENT_INTERFACE ||
	    setup->bRequest != CCID_REQ_ABORT || setup->wLength != 0 ||
	    setup->wIndex != ccid->instance->desc->iface.bInterfaceNumber ||
	    (setup->wValue & CCID_ABORT_SLOT_MASK) != CCID_SLOT_ZERO ||
	    ccid->backend.object == NULL) {
		return -ENOTSUP;
	}
	if (!ccid_lock(ccid)) {
		return -EIO;
	}
	if (!ccid_available(ccid)) {
		ccid_unlock(ccid);
		return -ENOTSUP;
	}
	ccid->generation++;
	ccid->rx.length = 0;
	ccid->rx.expect_zlp = false;
	ccid->abort.seq = setup->wValue >> CCID_ABORT_SEQUENCE_SHIFT;
	ccid->abort.state =
		ccid->abort.last_seq == ccid->abort.seq ? CCID_ABORT_MATCHED : CCID_ABORT_WAIT_BULK;
	LOG_DBG("CCID %u: ABORT seq %u", ccid->instance->id, ccid->abort.seq);
	ccid->tx.length = 0;
	ccid->tx.auxiliary_ready = false;
	/* An already queued packet may finish, but its completion must not
	 * consume a new reply belonging to the replacement ABORT handshake.
	 */
	ccid->tx.kind = CCID_IN_STALE;
	/* The optional backend hook only requests cancellation. The handshake
	 * itself works without it and waits until the worker has quiesced.
	 */
	ccid_cancel_backend(ccid);
	(void)ccid_schedule_io(ccid, K_NO_WAIT);
	ccid_unlock(ccid);
	return 0;
}

/**
 * @brief Return descriptors for the requested USB speed.
 * @param c_data USB class instance handling this request.
 * @param speed Negotiated USB bus speed.
 *
 * @return Instance-owned descriptor pointer array for the requested speed;
 *         unsupported high-speed selection falls back to full speed.
 */
static const void *ccid_get_desc(struct usbd_class_data *c_data, enum usbd_speed speed)
{
	struct ccid_context *ccid = usbd_class_get_private(c_data);

#if USBD_SUPPORTS_HIGH_SPEED
	if (speed == USBD_SPEED_HS) {
		return ccid->instance->hs_desc;
	}
#else
	ARG_UNUSED(speed);
#endif

	return ccid->instance->fs_desc;
}

/**
 * @brief Enable the CCID interface and begin receiving commands.
 * @param c_data USB class instance handling this request.
 */
static void ccid_enable(struct usbd_class_data *c_data)
{
	struct ccid_context *ccid = usbd_class_get_private(c_data);

	if (!ccid_lock(ccid)) {
		return;
	}
	atomic_set_bit(&ccid->state, CCID_ENABLED);
	atomic_clear_bit(&ccid->state, CCID_SUSPENDED);
	if (ccid_card_status(ccid) != CCID_STATUS_ICC_NOT_PRESENT) {
		atomic_set_bit(&ccid->state, CCID_NOTIFY_PENDING);
	}
	(void)ccid_schedule_io(ccid, K_NO_WAIT);
	LOG_DBG("CCID %u: enabled", ccid->instance->id);
	ccid_unlock(ccid);
}

/**
 * @brief Invalidate USB work and defer power-off until the backend has quiesced.
 * @param ccid Private state of the addressed CCID interface.
 *
 * @pre The instance lock is held.
 */
static void ccid_invalidate(struct ccid_context *ccid)
{
	ccid->generation++;
	ccid->rx.length = 0;
	ccid->rx.expect_zlp = false;
	ccid->rx.buf = NULL;
	ccid->tx.buf = NULL;
	ccid->usb.notify_buf = NULL;
	ccid->tx.length = 0;
	ccid->tx.auxiliary_ready = false;
	ccid->abort.state = CCID_ABORT_IDLE;
	ccid->abort.last_seq = CCID_ABORT_SEQ_NONE;
	ccid->worker.reset_pending = true;
	atomic_set_bit(&ccid->state, CCID_NEEDS_POWER_ON);
	k_sem_give(&ccid->worker.sem);
	/* Generation checks reject late work; the cancellation state is unused. */
	(void)k_work_cancel_delayable(&ccid->io_work);
	ccid_cancel_backend(ccid);
}

/**
 * @brief Stop class I/O, clear notifications and invalidate the current USB session.
 *
 * @param c_data USB class instance owning the interface and its endpoints.
 */
static void ccid_disable(struct usbd_class_data *c_data)
{
	struct ccid_context *ccid = usbd_class_get_private(c_data);

	if (!atomic_get(&ccid->worker.started)) {
		return;
	}
	if (!ccid_lock(ccid)) {
		return;
	}
	atomic_clear_bit(&ccid->state, CCID_ENABLED);
	atomic_clear_bit(&ccid->state, CCID_SUSPENDED);
	atomic_clear_bit(&ccid->state, CCID_NOTIFY_PENDING);
	ccid_invalidate(ccid);
	LOG_DBG("CCID %u: disabled", ccid->instance->id);
	ccid_unlock(ccid);
}

/**
 * @brief Invalidate the session and dequeue transfers on all three endpoints.
 *
 * @param c_data USB class instance owning the interface and its endpoints.
 */
static void ccid_suspended(struct usbd_class_data *c_data)
{
	struct ccid_context *ccid = usbd_class_get_private(c_data);
	struct usbd_context *ctx = usbd_class_get_ctx(c_data);

	if (!ccid_lock(ccid)) {
		return;
	}
	atomic_set_bit(&ccid->state, CCID_SUSPENDED);
	ccid_invalidate(ccid);
	for (enum ccid_endpoint ep = CCID_EP_BULK_OUT; ep < CCID_EP_COUNT; ep++) {
		uint8_t address = ccid_ep_addr(c_data, ep);
		int ret = usbd_ep_dequeue(ctx, address);

		if (ret != 0) {
			LOG_WRN("CCID %u: endpoint 0x%02x dequeue failed (%d)", ccid->instance->id,
				address, ret);
		}
	}
	LOG_DBG("CCID %u: suspended", ccid->instance->id);
	ccid_unlock(ccid);
}

/**
 * @brief Resume class I/O and report slot state while retaining the power-on requirement.
 *
 * @param c_data USB class instance owning the interface and its endpoints.
 */
static void ccid_resumed(struct usbd_class_data *c_data)
{
	struct ccid_context *ccid = usbd_class_get_private(c_data);

	if (!ccid_lock(ccid)) {
		return;
	}
	atomic_clear_bit(&ccid->state, CCID_SUSPENDED);
	atomic_set_bit(&ccid->state, CCID_NOTIFY_PENDING);
	(void)ccid_schedule_io(ccid, K_NO_WAIT);
	LOG_DBG("CCID %u: resumed", ccid->instance->id);
	ccid_unlock(ccid);
}

/**
 * @brief Bind descriptors and start the worker for this CCID interface.
 * @param c_data USB class instance handling this request.
 *
 * @retval 0 if initialization succeeded or the worker was already started.
 * @retval -ENODEV if no backend is registered for the interface.
 * @return Negative synchronization or descriptor-registration error.
 */
static int ccid_init(struct usbd_class_data *c_data)
{
	struct ccid_context *ccid = usbd_class_get_private(c_data);
	int ret;

	if (ccid->backend.object == NULL) {
		LOG_ERR("CCID %u: no backend registered", ccid->instance->id);
		return -ENODEV;
	}
	if (ccid->instance->desc->iface.iInterface == 0) {
		int err =
			usbd_add_descriptor(usbd_class_get_ctx(c_data), ccid->instance->if_string);

		if (err) {
			LOG_ERR("CCID %u: interface string registration failed (%d)",
				ccid->instance->id, err);
			return err;
		}
		ccid->instance->desc->iface.iInterface =
			usbd_str_desc_get_idx(ccid->instance->if_string);
	}

	ccid->usb.class_data = c_data;

	if (!atomic_cas(&ccid->worker.started, 0, 1)) {
		return 0;
	}

	ret = k_mutex_init(&ccid->lock);
	if (ret != 0) {
		goto init_failed;
	}
	k_work_init_delayable(&ccid->io_work, ccid_io_work);
	ret = k_sem_init(&ccid->worker.sem, 0, 1);
	if (ret != 0) {
		goto init_failed;
	}

	(void)k_thread_create(&ccid->worker.thread, ccid_stacks[ccid->instance->stack_index],
			      K_THREAD_STACK_SIZEOF(ccid_stacks[ccid->instance->stack_index]),
			      ccid_thread_fn, ccid, NULL, NULL,
			      K_PRIO_PREEMPT(CONFIG_USBD_CCID_THREAD_PRIORITY), 0, K_NO_WAIT);

	if (IS_ENABLED(CONFIG_THREAD_NAME)) {
		ret = k_thread_name_set(&ccid->worker.thread, ccid->instance->thread_name);
		if (ret != 0) {
			LOG_WRN("CCID %u: worker naming failed (%d)", ccid->instance->id, ret);
		}
	}
	LOG_INF("CCID %u initialized", ccid->instance->id);

	return 0;

init_failed:
	LOG_ERR("CCID %u: worker synchronization init failed (%d)", ccid->instance->id, ret);
	(void)atomic_set(&ccid->worker.started, 0);
	return ret;
}

static const struct usbd_class_api ccid_api = {
	.control_to_host = ccid_control_to_host,
	.control_to_dev = ccid_control_to_dev,
	.request = ccid_request_handler,
	.get_desc = ccid_get_desc,
	.enable = ccid_enable,
	.disable = ccid_disable,
	.suspended = ccid_suspended,
	.resumed = ccid_resumed,
	.shutdown = ccid_disable,
	.init = ccid_init,
};

/* Keep nested USB descriptor initializers compact and aligned. */
/* clang-format off */
#define CCID_EP_DESC(direction, type, mps, interval) { \
	.bLength = sizeof(struct usb_ep_descriptor), \
	.bDescriptorType = USB_DESC_ENDPOINT, \
	.bEndpointAddress = (direction), \
	.bmAttributes = (type), \
	.wMaxPacketSize = sys_cpu_to_le16(mps), \
	.bInterval = (interval), \
}

static struct ccid_descriptor ccid_desc[CCID_INSTANCE_COUNT] = {
	[0 ... CCID_INSTANCE_COUNT - 1] = {
		.iface = {
			.bLength = sizeof(struct usb_if_descriptor),
			.bDescriptorType = USB_DESC_INTERFACE,
			.bNumEndpoints = CCID_EP_COUNT,
			.bInterfaceClass = CCID_CLASS_CODE,
		},
		.ccid = {
			.bLength = sizeof(struct ccid_class_descriptor),
			.bDescriptorType = CCID_DESCRIPTOR_TYPE,
			.bcdCCID = sys_cpu_to_le16(CCID_VERSION_1_1),
			.bMaxSlotIndex = CCID_SLOT_ZERO,
			.dwMaxCCIDMessageLength =
				sys_cpu_to_le32(CONFIG_USBD_CCID_MAX_MESSAGE_SIZE),
			.bMaxCCIDBusySlots = CCID_BUSY_SLOT_COUNT,
		},
		.fs_ep = {
			[CCID_EP_BULK_OUT] = CCID_EP_DESC(USB_EP_DIR_OUT, USB_EP_TYPE_BULK,
				 CCID_FS_BULK_PACKET_SIZE, 0),
			[CCID_EP_BULK_IN] = CCID_EP_DESC(USB_EP_DIR_IN, USB_EP_TYPE_BULK,
				CCID_FS_BULK_PACKET_SIZE, 0),
			[CCID_EP_INTERRUPT_IN] = CCID_EP_DESC(USB_EP_DIR_IN, USB_EP_TYPE_INTERRUPT,
				CCID_INTERRUPT_PACKET_SIZE, CCID_FS_INTERRUPT_INTERVAL),
		},
#if USBD_SUPPORTS_HIGH_SPEED
		.hs_ep = {
			[CCID_EP_BULK_OUT] = CCID_EP_DESC(USB_EP_DIR_OUT, USB_EP_TYPE_BULK,
				 CCID_HS_BULK_PACKET_SIZE, 0),
			[CCID_EP_BULK_IN] = CCID_EP_DESC(USB_EP_DIR_IN, USB_EP_TYPE_BULK,
				CCID_HS_BULK_PACKET_SIZE, 0),
			[CCID_EP_INTERRUPT_IN] = CCID_EP_DESC(USB_EP_DIR_IN, USB_EP_TYPE_INTERRUPT,
				CCID_INTERRUPT_PACKET_SIZE, CCID_HS_INTERRUPT_INTERVAL),
		},
#endif
		.nil_desc = {0},
	},
};
/* clang-format on */

#if USBD_SUPPORTS_HIGH_SPEED
#define CCID_HS_DESC_DEFINE(n)                                                                     \
	static const struct usb_desc_header *ccid_hs_desc_##n[] = {                                \
		(void *)&ccid_desc[n].iface,                                                       \
		(void *)&ccid_desc[n].ccid,                                                        \
		(void *)&ccid_desc[n].hs_ep[CCID_EP_BULK_OUT],                                     \
		(void *)&ccid_desc[n].hs_ep[CCID_EP_BULK_IN],                                      \
		(void *)&ccid_desc[n].hs_ep[CCID_EP_INTERRUPT_IN],                                 \
		(void *)&ccid_desc[n].nil_desc}
#else
#define CCID_HS_DESC_DEFINE(n)
#endif

#define CCID_INSTANCE_DEFINE_INTERNAL(n, id)                                                       \
	static const char ccid_thread_name_##n[] = STRINGIFY(ccid_##id);                           \
	USBD_DESC_STRING_DEFINE(ccid_string_##n, DT_INST_PROP_OR(n, label, "CCID"),                \
				USBD_DUT_STRING_INTERFACE);                                        \
	static const struct usb_desc_header *ccid_fs_desc_##n[] = {                                \
		(void *)&ccid_desc[n].iface,                                                       \
		(void *)&ccid_desc[n].ccid,                                                        \
		(void *)&ccid_desc[n].fs_ep[CCID_EP_BULK_OUT],                                     \
		(void *)&ccid_desc[n].fs_ep[CCID_EP_BULK_IN],                                      \
		(void *)&ccid_desc[n].fs_ep[CCID_EP_INTERRUPT_IN],                                 \
		(void *)&ccid_desc[n].nil_desc};                                                   \
	CCID_HS_DESC_DEFINE(n);                                                                    \
	USBD_DEFINE_CLASS(ccid_##id, &ccid_api, &ccid_contexts[n], NULL)

/* Expand the devicetree property before using the ID in a class identifier.
 * Duplicate IDs produce duplicate class definitions and fail compilation.
 */
#define CCID_INSTANCE_DEFINE_ID(n, id) CCID_INSTANCE_DEFINE_INTERNAL(n, id)
#define CCID_INSTANCE_DEFINE(n, ...)   CCID_INSTANCE_DEFINE_ID(n, DT_INST_PROP(n, instance_id))

LISTIFY(CCID_INSTANCE_COUNT, CCID_INSTANCE_DEFINE, (;));

/* clang-format off */
#define CCID_INSTANCE(n)                                                                           \
	[n] = {                                                                                    \
		.desc = &ccid_desc[n],                                                             \
		.fs_desc = ccid_fs_desc_##n,                                                       \
		IF_ENABLED(USBD_SUPPORTS_HIGH_SPEED, (.hs_desc = ccid_hs_desc_##n,))              \
		.if_string = &ccid_string_##n,                                                     \
		.thread_name = ccid_thread_name_##n,                                               \
		.id = DT_INST_PROP(n, instance_id),                                                \
		.stack_index = n,                                                                  \
	},
/* clang-format on */

static const struct ccid_instance ccid_instances[CCID_INSTANCE_COUNT] = {
	DT_INST_FOREACH_STATUS_OKAY(CCID_INSTANCE)};

#define CCID_CONTEXT(n)                                                                            \
	[n] = {                                                                                    \
		.instance = &ccid_instances[n],                                                    \
		.buffers = &ccid_buffers[n],                                                       \
		.abort.last_seq = CCID_ABORT_SEQ_NONE,                                             \
	},

static struct ccid_context ccid_contexts[CCID_INSTANCE_COUNT] = {
	DT_INST_FOREACH_STATUS_OKAY(CCID_CONTEXT)};
