/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief CoAP implementation for Zephyr.
 */

#ifndef __ZOAP_H__
#define __ZOAP_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <misc/slist.h>

/**
 * @brief Set of CoAP packet options we are aware of.
 *
 * Users may add options other than these to their packets, provided
 * they know how to format them correctly. The only restriction is
 * that all options must be added to a packet in numeric order.
 *
 * Refer to RFC 7252, section 12.2 for more information.
 */
enum zoap_option_num {
	ZOAP_OPTION_IF_MATCH = 1,
	ZOAP_OPTION_URI_HOST = 3,
	ZOAP_OPTION_ETAG = 4,
	ZOAP_OPTION_IF_NONE_MATCH = 5,
	ZOAP_OPTION_OBSERVE = 6,
	ZOAP_OPTION_URI_PORT = 7,
	ZOAP_OPTION_LOCATION_PATH = 8,
	ZOAP_OPTION_URI_PATH = 11,
	ZOAP_OPTION_CONTENT_FORMAT = 12,
	ZOAP_OPTION_MAX_AGE = 14,
	ZOAP_OPTION_URI_QUERY = 15,
	ZOAP_OPTION_ACCEPT = 17,
	ZOAP_OPTION_LOCATION_QUERY = 20,
	ZOAP_OPTION_BLOCK2 = 23,
	ZOAP_OPTION_BLOCK1 = 27,
	ZOAP_OPTION_SIZE2 = 28,
	ZOAP_OPTION_PROXY_URI = 35,
	ZOAP_OPTION_PROXY_SCHEME = 39,
	ZOAP_OPTION_SIZE1 = 60,
};

/**
 * @brief Available request methods.
 *
 * To be used with zoap_header_set_code() when creating a request
 * or a response.
 */
enum zoap_method {
	ZOAP_METHOD_GET = 1,
	ZOAP_METHOD_POST = 2,
	ZOAP_METHOD_PUT = 3,
	ZOAP_METHOD_DELETE = 4,
};

#define ZOAP_REQUEST_MASK 0x07

/**
 * @brief CoAP packets may be of one of these types.
 */
enum zoap_msgtype {
	/**
	 * Confirmable message.
	 *
	 * The packet is a request or response the destination end-point must
	 * acknowledge.
	 */
	ZOAP_TYPE_CON = 0,
	/**
	 * Non-confirmable message.
	 *
	 * The packet is a request or response that doesn't
	 * require acknowledgements.
	 */
	ZOAP_TYPE_NON_CON = 1,
	/**
	 * Acknowledge.
	 *
	 * Response to a confirmable message.
	 */
	ZOAP_TYPE_ACK = 2,
	/**
	 * Reset.
	 *
	 * Rejecting a packet for any reason is done by sending a message
	 * of this type.
	 */
	ZOAP_TYPE_RESET = 3
};

#define zoap_make_response_code(clas, det) ((clas << 5) | (det))

/**
 * @brief Set of response codes available for a response packet.
 *
 * To be used with zoap_header_set_code() when creating a response.
 */
enum zoap_response_code {
	ZOAP_RESPONSE_CODE_OK = zoap_make_response_code(2, 0),
	ZOAP_RESPONSE_CODE_CREATED = zoap_make_response_code(2, 1),
	ZOAP_RESPONSE_CODE_DELETED = zoap_make_response_code(2, 2),
	ZOAP_RESPONSE_CODE_VALID = zoap_make_response_code(2, 3),
	ZOAP_RESPONSE_CODE_CHANGED = zoap_make_response_code(2, 4),
	ZOAP_RESPONSE_CODE_CONTENT = zoap_make_response_code(2, 5),
	ZOAP_RESPONSE_CODE_CONTINUE = zoap_make_response_code(2, 31),
	ZOAP_RESPONSE_CODE_BAD_REQUEST = zoap_make_response_code(4, 0),
	ZOAP_RESPONSE_CODE_UNAUTHORIZED = zoap_make_response_code(4, 1),
	ZOAP_RESPONSE_CODE_BAD_OPTION = zoap_make_response_code(4, 2),
	ZOAP_RESPONSE_CODE_FORBIDDEN = zoap_make_response_code(4, 3),
	ZOAP_RESPONSE_CODE_NOT_FOUND = zoap_make_response_code(4, 4),
	ZOAP_RESPONSE_CODE_NOT_ALLOWED = zoap_make_response_code(4, 5),
	ZOAP_RESPONSE_CODE_NOT_ACCEPTABLE = zoap_make_response_code(4, 6),
	ZOAP_RESPONSE_CODE_INCOMPLETE = zoap_make_response_code(4, 8),
	ZOAP_RESPONSE_CODE_PRECONDITION_FAILED = zoap_make_response_code(4, 12),
	ZOAP_RESPONSE_CODE_REQUEST_TOO_LARGE = zoap_make_response_code(4, 13),
	ZOAP_RESPONSE_CODE_UNSUPPORTED_CONTENT_FORMAT = zoap_make_response_code(4, 15),
	ZOAP_RESPONSE_CODE_INTERNAL_ERROR = zoap_make_response_code(5, 0),
	ZOAP_RESPONSE_CODE_NOT_IMPLEMENTED = zoap_make_response_code(5, 1),
	ZOAP_RESPONSE_CODE_BAD_GATEWAY = zoap_make_response_code(5, 2),
	ZOAP_RESPONSE_CODE_SERVICE_UNAVAILABLE = zoap_make_response_code(5, 3),
	ZOAP_RESPONSE_CODE_GATEWAY_TIMEOUT = zoap_make_response_code(5, 4),
	ZOAP_RESPONSE_CODE_PROXYING_NOT_SUPPORTED = zoap_make_response_code(5, 5)
};

#define ZOAP_CODE_EMPTY (0)

struct zoap_observer;
struct zoap_packet;
struct zoap_pending;
struct zoap_reply;
struct zoap_resource;

/**
 * Type of the callback being called when a resource's method is invoked by
 * remote entity.
 */
typedef int (*zoap_method_t)(struct zoap_resource *resource,
			     struct zoap_packet *request,
			     const struct sockaddr *from);

/**
 * Type of the callback being called when a resource's has observers to be
 * informed when an update happens.
 */
typedef void (*zoap_notify_t)(struct zoap_resource *resource,
			      struct zoap_observer *observer);

/**
 * @brief Description of CoAP resource.
 *
 * CoAP servers often want to register resources, so that clients can act on
 * them, by fetching their state or requesting updates to them.
 */
struct zoap_resource {
	zoap_method_t get, post, put, del;
	zoap_notify_t notify;
	const char * const *path;
	void *user_data;
	sys_slist_t observers;
	int age;
};

/**
 * Represents a remote device that is observing a local resource.
 */
struct zoap_observer {
	sys_snode_t list;
	struct sockaddr addr;
	uint8_t token[8];
	uint8_t tkl;
};

/**
 * Representation of a CoAP packet.
 */
struct zoap_packet {
	struct net_buf *buf;
	uint8_t *start; /* Start of the payload */
};

/**
 * Helper function to be called when a response matches the
 * a pending request.
 */
typedef int (*zoap_reply_t)(const struct zoap_packet *response,
			    struct zoap_reply *reply,
			    const struct sockaddr *from);

/**
 * Represents a request awaiting for an acknowledgment (ACK).
 */
struct zoap_pending {
	struct zoap_packet request;
	int32_t timeout;
};

/**
 * Represents the handler for the reply of a request, it is also used when
 * observing resources.
 */
struct zoap_reply {
	zoap_reply_t reply;
	void *user_data;
	int age;
	uint8_t token[8];
	uint8_t tkl;
};

/**
 * Indicates that the remote device referenced by @a addr, with @a request,
 * wants to observe a resource.
 */
void zoap_observer_init(struct zoap_observer *observer,
			const struct zoap_packet *request,
			const struct sockaddr *addr);

/**
 * After the observer is initialized, associate the observer with an resource.
 * Returns whether this is the first observer added to this resource.
 */
bool zoap_register_observer(struct zoap_resource *resource,
			    struct zoap_observer *observer);

/**
 * Remove this observer from the list of registered observers of
 * that resource.
 */
void zoap_remove_observer(struct zoap_resource *resource,
			  struct zoap_observer *observer);

/**
 * Returns the observer that matches address @a addr.
 */
struct zoap_observer *zoap_find_observer_by_addr(
	struct zoap_observer *observers, size_t len,
	const struct sockaddr *addr);

/**
 * Returns the next available observer representation.
 */
struct zoap_observer *zoap_observer_next_unused(
	struct zoap_observer *observers, size_t len);

/**
 * Indicates that a reply is expected for @a request.
 */
void zoap_reply_init(struct zoap_reply *reply,
		     const struct zoap_packet *request);

/**
 * Represents the value of a CoAP option.
 *
 * To be used with zoap_find_options().
 */
struct zoap_option {
	uint8_t *value;
	uint16_t len;
};

/**
 * Parses the CoAP packet in @a buf, validating it and initializing @a pkt.
 * @a buf must remain valid while @a pkt is used. Used when receiving packets.
 */
int zoap_packet_parse(struct zoap_packet *pkt, struct net_buf *buf);

/**
 * Creates a new CoAP packet from a net_buf. @a buf must remain valid while
 * @a pkt is used. Used when creating packets to be sent.
 */
int zoap_packet_init(struct zoap_packet *pkt, struct net_buf *buf);

/**
 * Initialize a pending request with a request. The request's fields are
 * copied into the pending struct, so @a request doesn't have to live for as
 * long as the pending struct lives, but net_buf needs to live for at least
 * that long.
 */
int zoap_pending_init(struct zoap_pending *pending,
		      const struct zoap_packet *request);

/**
 * Returns the next available pending struct, that can be used to track
 * the retransmission status of a request.
 */
struct zoap_pending *zoap_pending_next_unused(
	struct zoap_pending *pendings, size_t len);

/**
 * Returns the next available reply struct, so it can be used to track replies
 * and notifications received.
 */
struct zoap_reply *zoap_reply_next_unused(
	struct zoap_reply *replies, size_t len);

/**
 * After a response is received, clear all pending retransmissions related to
 * that response.
 */
struct zoap_pending *zoap_pending_received(
	const struct zoap_packet *response,
	struct zoap_pending *pendings, size_t len);

/**
 * After a response is received, clear all pending retransmissions related to
 * that response.
 */
struct zoap_reply *zoap_response_received(
	const struct zoap_packet *response,
	const struct sockaddr *from,
	struct zoap_reply *replies, size_t len);

/**
 * Returns the next pending about to expire, pending->timeout informs how many
 * ms to next expiration.
 */
struct zoap_pending *zoap_pending_next_to_expire(
	struct zoap_pending *pendings, size_t len);

/**
 * After a request is sent, user may want to cycle the pending retransmission
 * so the timeout is updated. Returns false if this is the last
 * retransmission.
 */
bool zoap_pending_cycle(struct zoap_pending *pending);

/**
 * Cancels the pending retransmission, so it again becomes available.
 */
void zoap_pending_clear(struct zoap_pending *pending);

/**
 * Cancels awaiting for this reply, so it becomes available again.
 */
void zoap_reply_clear(struct zoap_reply *reply);

/**
 * When a request is received, call the appropriate methods of the
 * matching resources.
 */
int zoap_handle_request(struct zoap_packet *pkt,
			struct zoap_resource *resources,
			const struct sockaddr *from);

/**
 * Indicates that this resource was updated and that the @a notify callback
 * should be called for every registered observer.
 */
int zoap_resource_notify(struct zoap_resource *resource);

/**
 * Returns if this request is enabling observing a resource.
 */
bool zoap_request_is_observe(const struct zoap_packet *request);

/**
 * Returns a pointer to the start of the payload, and how much memory
 * is available (to the payload), it will also insert the
 * COAP_MARKER (0xFF).
 */
uint8_t *zoap_packet_get_payload(struct zoap_packet *pkt, uint16_t *len);

/**
 * Returns the internal buffer of the CoAP packet, appending the
 * COAP_MARKER to the buffer if necessary.
 */
struct net_buf *zoap_packet_get_buf(struct zoap_packet *pkt);

/**
 * Sets how much space was used by the payload.
 */
int zoap_packet_set_used(struct zoap_packet *pkt, uint16_t len);

/**
 * Adds an option to the packet. Note that options must be added
 * in numeric order of their codes.
 */
int zoap_add_option(struct zoap_packet *pkt, uint16_t code,
		    const void *value, uint16_t len);

/**
 * Converts an option to its integer representation. It assumes that
 * the number is encoded in the network byte order in the option.
 */
unsigned int zoap_option_value_to_int(const struct zoap_option *option);

/**
 * Adds an integer value option to the packet. The option must be
 * added in numeric order of their codes, and the least amount of
 * bytes will be used to encode the value.
 */
int zoap_add_option_int(struct zoap_packet *pkt, uint16_t code,
			unsigned int val);

/**
 * Return the values associated with the option of value @a code.
 */
int zoap_find_options(const struct zoap_packet *pkt, uint16_t code,
		      struct zoap_option *options, uint16_t veclen);

/**
 * Represents the size of each block that will be transferred using
 * block-wise transfers [RFC7959]:
 *
 * Each entry maps directly to the value that is used in the wire.
 *
 * https://tools.ietf.org/html/rfc7959
 */
enum zoap_block_size {
	ZOAP_BLOCK_16,
	ZOAP_BLOCK_32,
	ZOAP_BLOCK_64,
	ZOAP_BLOCK_128,
	ZOAP_BLOCK_256,
	ZOAP_BLOCK_512,
	ZOAP_BLOCK_1024,
};

/**
 * Helper for converting the enumeration to the size expressed in bytes.
 */
static inline uint16_t zoap_block_size_to_bytes(
	enum zoap_block_size block_size)
{
	return (1 << (block_size + 4));
}

/**
 * Represents the current state of a block-wise transaction.
 */
struct zoap_block_context {
	size_t total_size;
	size_t current;
	enum zoap_block_size block_size;
};

/**
 * Initializes the context of a block-wise transfer.
 */
int zoap_block_transfer_init(struct zoap_block_context *ctx,
			      enum zoap_block_size block_size,
			      size_t total_size);

/**
 * Add BLOCK1 option to the packet.
 */
int zoap_add_block1_option(struct zoap_packet *pkt,
			   struct zoap_block_context *ctx);

/**
 * Add BLOCK2 option to the packet.
 */
int zoap_add_block2_option(struct zoap_packet *pkt,
			   struct zoap_block_context *ctx);

/**
 * Add SIZE1 option to the packet.
 */
int zoap_add_size1_option(struct zoap_packet *pkt,
			 struct zoap_block_context *ctx);

/**
 * Add SIZE2 option to the packet.
 */
int zoap_add_size2_option(struct zoap_packet *pkt,
			 struct zoap_block_context *ctx);

/**
 * Retrieves BLOCK{1,2} and SIZE{1,2} from @a pkt and updates
 * @a ctx accordingly.
 *
 * Returns an error if the packet contains invalid options.
 */
int zoap_update_from_block(struct zoap_packet *pkt,
			   struct zoap_block_context *ctx);
/**
 * Updates @a ctx so after this is called the current entry
 * indicates the correct offset in the body of data being
 * transferred.
 */
size_t zoap_next_block(struct zoap_block_context *ctx);

/**
 * Returns the version present in a CoAP packet.
 */
uint8_t zoap_header_get_version(const struct zoap_packet *pkt);

/**
 * Returns the type of the packet present in the CoAP packet.
 */
uint8_t zoap_header_get_type(const struct zoap_packet *pkt);

/**
 * Returns the token associated with a CoAP packet.
 */
const uint8_t *zoap_header_get_token(const struct zoap_packet *pkt,
				     uint8_t *len);

/**
 * Returns the code present in the header of a CoAP packet.
 */
uint8_t zoap_header_get_code(const struct zoap_packet *pkt);

/**
 * Returns the message id associated with a CoAP packet.
 */
uint16_t zoap_header_get_id(const struct zoap_packet *pkt);

/**
 * Sets the CoAP version present in the CoAP header of a packet.
 */
void zoap_header_set_version(struct zoap_packet *pkt, uint8_t ver);

/**
 * Sets the type of a CoAP message.
 */
void zoap_header_set_type(struct zoap_packet *pkt, uint8_t type);

/**
 * Sets the token present in the CoAP header of a packet.
 */
int zoap_header_set_token(struct zoap_packet *pkt, const uint8_t *token,
			  uint8_t tokenlen);

/**
 * Sets the code present in the header of a CoAP packet.
 */
void zoap_header_set_code(struct zoap_packet *pkt, uint8_t code);

/**
 * Sets the message id associated with a CoAP packet.
 */
void zoap_header_set_id(struct zoap_packet *pkt, uint16_t id);

static inline uint16_t zoap_next_id(void)
{
	static uint16_t message_id;

	return ++message_id;
}

/**
 * Returns a randomly generated array of 8 bytes, that can be used as a
 * message's token.
 */
uint8_t *zoap_next_token(void);

#endif /* __ZOAP_H__ */
