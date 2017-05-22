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

#include <zephyr/types.h>
#include <stddef.h>
#include <stdbool.h>
#include <net/net_ip.h>

#include <misc/slist.h>

/**
 * @brief COAP library
 * @defgroup zoap COAP Library
 * @{
 */

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
 * @typedef zoap_method_t
 * @brief Type of the callback being called when a resource's method is
 * invoked by the remote entity.
 */
typedef int (*zoap_method_t)(struct zoap_resource *resource,
			     struct zoap_packet *request,
			     const struct sockaddr *from);

/**
 * @typedef zoap_notify_t
 * @brief Type of the callback being called when a resource's has observers
 * to be informed when an update happens.
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
	/** Which function to be called for each CoAP method */
	zoap_method_t get, post, put, del;
	zoap_notify_t notify;
	const char * const *path;
	void *user_data;
	sys_slist_t observers;
	int age;
};

/**
 * @brief Represents a remote device that is observing a local resource.
 */
struct zoap_observer {
	sys_snode_t list;
	struct sockaddr addr;
	u8_t token[8];
	u8_t tkl;
};

/**
 * @brief Representation of a CoAP packet.
 */
struct zoap_packet {
	struct net_pkt *pkt;
	u8_t *start; /* Start of the payload */
	u16_t total_size;
};

/**
 * @typedef zoap_reply_t
 * @brief Helper function to be called when a response matches the
 * a pending request.
 */
typedef int (*zoap_reply_t)(const struct zoap_packet *response,
			    struct zoap_reply *reply,
			    const struct sockaddr *from);

/**
 * @brief Represents a request awaiting for an acknowledgment (ACK).
 */
struct zoap_pending {
	struct net_pkt *pkt;
	struct sockaddr addr;
	s32_t timeout;
	u16_t id;
};

/**
 * @brief Represents the handler for the reply of a request, it is
 * also used when observing resources.
 */
struct zoap_reply {
	zoap_reply_t reply;
	void *user_data;
	int age;
	u8_t token[8];
	u8_t tkl;
};

/**
 * @brief Indicates that the remote device referenced by @a addr, with
 * @a request, wants to observe a resource.
 *
 * @param observer Observer to be initialized
 * @param request Request on which the observer will be based
 * @param addr Address of the remote device
 */
void zoap_observer_init(struct zoap_observer *observer,
			const struct zoap_packet *request,
			const struct sockaddr *addr);

/**
 * @brief After the observer is initialized, associate the observer
 * with an resource.
 *
 * @param resource Resource to add an observer
 * @param observer Observer to be added
 *
 * @return true if this is the first observer added to this resource.
 */
bool zoap_register_observer(struct zoap_resource *resource,
			    struct zoap_observer *observer);

/**
 * @brief Remove this observer from the list of registered observers
 * of that resource.
 *
 * @param resource Resource in which to remove the observer
 * @param observer Observer to be removed
 */
void zoap_remove_observer(struct zoap_resource *resource,
			  struct zoap_observer *observer);

/**
 * @brief Returns the observer that matches address @a addr.
 *
 * @param observers Pointer to the array of observers
 * @param len Size of the array of observers
 * @param addr Address of the endpoint observing a resource
 *
 * @return A pointer to a observer if a match is found, NULL
 * otherwise.
 */
struct zoap_observer *zoap_find_observer_by_addr(
	struct zoap_observer *observers, size_t len,
	const struct sockaddr *addr);

/**
 * @brief Returns the next available observer representation.
 *
 * @param observers Pointer to the array of observers
 * @param len Size of the array of observers
 *
 * @return A pointer to a observer if there's an available observer,
 * NULL otherwise.
 */
struct zoap_observer *zoap_observer_next_unused(
	struct zoap_observer *observers, size_t len);

/**
 * @brief Indicates that a reply is expected for @a request.
 *
 * @param reply Reply structure to be initialized
 * @param request Request from which @a reply will be based
 */
void zoap_reply_init(struct zoap_reply *reply,
		     const struct zoap_packet *request);

/**
 * @brief Represents the value of a CoAP option.
 *
 * To be used with zoap_find_options().
 */
struct zoap_option {
	u8_t *value;
	u16_t len;
};

/**
 * @brief Parses the CoAP packet in @a pkt, validating it and
 * initializing @a zpkt. @a pkt must remain valid while @a zpkt is used.
 *
 * @param zpkt Packet to be initialized from received @a pkt.
 * @param pkt Network Packet containing a CoAP packet, its @a data pointer is
 * positioned on the start of the CoAP packet.
 *
 * @return 0 in case of success or negative in case of error.
 */
int zoap_packet_parse(struct zoap_packet *zpkt, struct net_pkt *pkt);

/**
 * @brief Creates a new CoAP packet from a net_pkt. @a pkt must remain
 * valid while @a zpkt is used.
 *
 * @param zpkt New packet to be initialized using the storage from @a
 * pkt.
 * @param pkt Network Packet that will contain a CoAP packet
 *
 * @return 0 in case of success or negative in case of error.
 */
int zoap_packet_init(struct zoap_packet *zpkt, struct net_pkt *pkt);

/**
 * @brief Initialize a pending request with a request.
 *
 * The request's fields are copied into the pending struct, so @a
 * request doesn't have to live for as long as the pending struct
 * lives, but net_pkt needs to live for at least that long.
 *
 * @param pending Structure representing the waiting for a
 * confirmation message, initialized with data from @a request
 * @param request Message waiting for confirmation
 * @param addr Address to send the retransmission
 *
 * @return 0 in case of success or negative in case of error.
 */
int zoap_pending_init(struct zoap_pending *pending,
		      const struct zoap_packet *request,
		      const struct sockaddr *addr);

/**
 * @brief Returns the next available pending struct, that can be used
 * to track the retransmission status of a request.
 *
 * @param pendings Pointer to the array of #zoap_pending structures
 * @param len Size of the array of #zoap_pending structures
 *
 * @return pointer to a free #zoap_pending structure, NULL in case
 * none could be found.
 */
struct zoap_pending *zoap_pending_next_unused(
	struct zoap_pending *pendings, size_t len);

/**
 * @brief Returns the next available reply struct, so it can be used
 * to track replies and notifications received.
 *
 * @param replies Pointer to the array of #zoap_reply structures
 * @param len Size of the array of #zoap_reply structures
 *
 * @return pointer to a free #zoap_reply structure, NULL in case
 * none could be found.
 */
struct zoap_reply *zoap_reply_next_unused(
	struct zoap_reply *replies, size_t len);

/**
 * @brief After a response is received, clear all pending
 * retransmissions related to that response.
 *
 * @param response The received response
 * @param pendings Pointer to the array of #zoap_reply structures
 * @param len Size of the array of #zoap_reply structures
 *
 * @return pointer to the associated #zoap_pending structure, NULL in
 * case none could be found.
 */
struct zoap_pending *zoap_pending_received(
	const struct zoap_packet *response,
	struct zoap_pending *pendings, size_t len);

/**
 * @brief After a response is received, clear all pending
 * retransmissions related to that response.
 *
 * @param response A response received
 * @param from Address from which the response was received
 * @param replies Pointer to the array of #zoap_reply structures
 * @param len Size of the array of #zoap_reply structures
 *
 * @return Pointer to the reply matching the packet received, NULL if
 * none could be found.
 */
struct zoap_reply *zoap_response_received(
	const struct zoap_packet *response,
	const struct sockaddr *from,
	struct zoap_reply *replies, size_t len);

/**
 * @brief Returns the next pending about to expire, pending->timeout
 * informs how many ms to next expiration.
 *
 * @param pendings Pointer to the array of #zoap_pending structures
 * @param len Size of the array of #zoap_pending structures
 *
 * @return The next #zoap_pending to expire, NULL if none is about to
 * expire.
 */
struct zoap_pending *zoap_pending_next_to_expire(
	struct zoap_pending *pendings, size_t len);

/**
 * @brief After a request is sent, user may want to cycle the pending
 * retransmission so the timeout is updated.
 *
 * @param pending Pending representation to have its timeout updated
 *
 * @return false if this is the last retransmission.
 */
bool zoap_pending_cycle(struct zoap_pending *pending);

/**
 * @brief Cancels the pending retransmission, so it again becomes
 * available.
 *
 * @param pending Pending representation to be canceled
 */
void zoap_pending_clear(struct zoap_pending *pending);

/**
 * @brief Cancels awaiting for this reply, so it becomes available
 * again.
 *
 * @param reply The reply to be canceled
 */
void zoap_reply_clear(struct zoap_reply *reply);

/**
 * @brief When a request is received, call the appropriate methods of
 * the matching resources.
 *
 * @param zpkt Packet received
 * @param resources Array of known resources
 * @param from Address from which the packet was received
 *
 * @return 0 in case of success or negative in case of error.
 */
int zoap_handle_request(struct zoap_packet *zpkt,
			struct zoap_resource *resources,
			const struct sockaddr *from);

/**
 * @brief Indicates that this resource was updated and that the @a
 * notify callback should be called for every registered observer.
 *
 * @param resource Resource that was updated
 *
 * @return 0 in case of success or negative in case of error.
 */
int zoap_resource_notify(struct zoap_resource *resource);

/**
 * @brief Returns if this request is enabling observing a resource.
 *
 * @param request Request to be checked
 *
 * @return True if the request is enabling observing a resource, False
 * otherwise
 */
bool zoap_request_is_observe(const struct zoap_packet *request);

/**
 * @brief Returns a pointer to the start of the payload and its size
 *
 * It will insert the COAP_MARKER (0xFF), if its not set, and return the
 * available size for the payload.
 *
 * @param zpkt Packet to get (or insert) the payload
 * @param len Amount of space for the payload
 *
 * @return pointer to the start of the payload, NULL in case of error.
 */
u8_t *zoap_packet_get_payload(struct zoap_packet *zpkt, u16_t *len);

/**
 * @brief Sets how much space was used by the payload.
 *
 * Used for outgoing packets, after zoap_packet_get_payload(), to
 * update the internal representation with the amount of data that was
 * added to the packet.
 *
 * @param zpkt Packet to be updated
 * @param len Amount of data that was added to the payload
 *
 * @return 0 in case of success or negative in case of error.
 */
int zoap_packet_set_used(struct zoap_packet *zpkt, u16_t len);

/**
 * @brief Adds an option to the packet.
 *
 * Note: options must be added in numeric order of their codes.
 *
 * @param zpkt Packet to be updated
 * @param code Option code to add to the packet, see #zoap_option_num
 * @param value Pointer to the value of the option, will be copied to the packet
 * @param len Size of the data to be added
 *
 * @return 0 in case of success or negative in case of error.
 */
int zoap_add_option(struct zoap_packet *zpkt, u16_t code,
		    const void *value, u16_t len);

/**
 * @brief Converts an option to its integer representation.
 *
 * Assumes that the number is encoded in the network byte order in the
 * option.
 *
 * @param option Pointer to the option value, retrieved by
 * zoap_find_options()
 *
 * @return The integer representation of the option
 */
unsigned int zoap_option_value_to_int(const struct zoap_option *option);

/**
 * @brief Adds an integer value option to the packet.
 *
 * The option must be added in numeric order of their codes, and the
 * least amount of bytes will be used to encode the value.
 *
 * @param zpkt Packet to be updated
 * @param code Option code to add to the packet, see #zoap_option_num
 * @param val Integer value to be added
 *
 * @return 0 in case of success or negative in case of error.
 */
int zoap_add_option_int(struct zoap_packet *zpkt, u16_t code,
			unsigned int val);

/**
 * @brief Return the values associated with the option of value @a
 * code.
 *
 * @param zpkt CoAP packet representation
 * @param code Option number to look for
 * @param options Array of #zoap_option where to store the value
 * of the options found
 * @param veclen Number of elements in the options array
 *
 * @return The number of options found in packet matching code,
 * negative on error.
 */
int zoap_find_options(const struct zoap_packet *zpkt, u16_t code,
		      struct zoap_option *options, u16_t veclen);

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
 * @brief Helper for converting the enumeration to the size expressed
 * in bytes.
 *
 * @param block_size The block size to be converted
 *
 * @return The size in bytes that the block_size represents
 */
static inline u16_t zoap_block_size_to_bytes(
	enum zoap_block_size block_size)
{
	return (1 << (block_size + 4));
}

/**
 * @brief Represents the current state of a block-wise transaction.
 */
struct zoap_block_context {
	size_t total_size;
	size_t current;
	enum zoap_block_size block_size;
};

/**
 * @brief Initializes the context of a block-wise transfer.
 *
 * @param ctx The context to be initialized
 * @param block_size The size of the block
 * @param total_size The total size of the transfer, if known
 *
 * @return 0 in case of success or negative in case of error.
 */
int zoap_block_transfer_init(struct zoap_block_context *ctx,
			     enum zoap_block_size block_size,
			     size_t total_size);

/**
 * @brief Add BLOCK1 option to the packet.
 *
 * @param zpkt Packet to be updated
 * @param ctx Block context from which to retrieve the
 * information for the Block1 option
 *
 * @return 0 in case of success or negative in case of error.
 */
int zoap_add_block1_option(struct zoap_packet *zpkt,
			   struct zoap_block_context *ctx);

/**
 * @brief Add BLOCK2 option to the packet.
 *
 * @param zpkt Packet to be updated
 * @param ctx Block context from which to retrieve the
 * information for the Block2 option
 *
 * @return 0 in case of success or negative in case of error.
 */
int zoap_add_block2_option(struct zoap_packet *zpkt,
			   struct zoap_block_context *ctx);

/**
 * @brief Add SIZE1 option to the packet.
 *
 * @param zpkt Packet to be updated
 * @param ctx Block context from which to retrieve the
 * information for the Size1 option
 *
 * @return 0 in case of success or negative in case of error.
 */
int zoap_add_size1_option(struct zoap_packet *zpkt,
			 struct zoap_block_context *ctx);

/**
 * @brief Add SIZE2 option to the packet.
 *
 * @param zpkt Packet to be updated
 * @param ctx Block context from which to retrieve the
 * information for the Size2 option
 *
 * @return 0 in case of success or negative in case of error.
 */
int zoap_add_size2_option(struct zoap_packet *zpkt,
			 struct zoap_block_context *ctx);

/**
 * @brief Retrieves BLOCK{1,2} and SIZE{1,2} from @a zpkt and updates
 * @a ctx accordingly.
 *
 * @param zpkt Packet in which to look for block-wise transfers options
 * @param ctx Block context to be updated
 *
 * @return 0 in case of success or negative in case of error.
 */
int zoap_update_from_block(const struct zoap_packet *zpkt,
			   struct zoap_block_context *ctx);

/**
 * @brief Updates @a ctx so after this is called the current entry
 * indicates the correct offset in the body of data being
 * transferred.
 *
 * @param ctx Block context to be updated
 *
 * @return The offset in the block-wise transfer, 0 if the transfer
 * has finished.
 */
size_t zoap_next_block(struct zoap_block_context *ctx);

/**
 * @brief Returns the version present in a CoAP packet.
 *
 * @param zpkt CoAP packet representation
 *
 * @return the CoAP version in packet
 */
u8_t zoap_header_get_version(const struct zoap_packet *zpkt);

/**
 * @brief Returns the type of the CoAP packet.
 *
 * @param zpkt CoAP packet representation
 *
 * @return the type of the packet
 */
u8_t zoap_header_get_type(const struct zoap_packet *zpkt);

/**
 * @brief Returns the token (if any) in the CoAP packet.
 *
 * @param zpkt CoAP packet representation
 * @param len Where to store the length of the token
 *
 * @return pointer to the start of the token in the CoAP packet.
 */
const u8_t *zoap_header_get_token(const struct zoap_packet *zpkt,
				     u8_t *len);

/**
 * @brief Returns the code of the CoAP packet.
 *
 * @param zpkt CoAP packet representation
 *
 * @return the code present in the packet
 */
u8_t zoap_header_get_code(const struct zoap_packet *zpkt);

/**
 * @brief Returns the message id associated with the CoAP packet.
 *
 * @param zpkt CoAP packet representation
 *
 * @return the message id present in the packet
 */
u16_t zoap_header_get_id(const struct zoap_packet *zpkt);

/**
 * @brief Sets the version of the CoAP packet.
 *
 * @param zpkt CoAP packet representation
 * @param ver The CoAP version to set in the packet
 */
void zoap_header_set_version(struct zoap_packet *zpkt, u8_t ver);

/**
 * @brief Sets the type of the CoAP packet.
 *
 * @param zpkt CoAP packet representation
 * @param type The packet type to set
 */
void zoap_header_set_type(struct zoap_packet *zpkt, u8_t type);

/**
 * @brief Sets the token in the CoAP packet.
 *
 * @param zpkt CoAP packet representation
 * @param token Token to set in the packet, will be copied
 * @param tokenlen Size of the token to be set, 8 bytes maximum
 *
 * @return 0 in case of success or negative in case of error.
 */
int zoap_header_set_token(struct zoap_packet *zpkt, const u8_t *token,
			  u8_t tokenlen);

/**
 * @brief Sets the code present in the CoAP packet.
 *
 * @param zpkt CoAP packet representation
 * @param code The code set in the packet
 */
void zoap_header_set_code(struct zoap_packet *zpkt, u8_t code);

/**
 * @brief Sets the message id present in the CoAP packet.
 *
 * @param zpkt CoAP packet representation
 * @param id The message id to set in the packet
 */
void zoap_header_set_id(struct zoap_packet *zpkt, u16_t id);

/**
 * @brief Helper to generate message ids
 *
 * @return a new message id
 */
static inline u16_t zoap_next_id(void)
{
	static u16_t message_id;

	return ++message_id;
}

/**
 * @brief Returns a randomly generated array of 8 bytes, that can be
 * used as a message's token.
 *
 * @return a 8-byte pseudo-random token.
 */
u8_t *zoap_next_token(void);

/**
 * @}
 */

#endif /* __ZOAP_H__ */
