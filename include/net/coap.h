/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief CoAP implementation for Zephyr.
 */

#ifndef __COAP_H__
#define __COAP_H__

#include <zephyr/types.h>
#include <stddef.h>
#include <stdbool.h>
#include <net/net_ip.h>

#include <misc/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief COAP library
 * @defgroup coap COAP Library
 * @ingroup networking
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
enum coap_option_num {
	COAP_OPTION_IF_MATCH = 1,
	COAP_OPTION_URI_HOST = 3,
	COAP_OPTION_ETAG = 4,
	COAP_OPTION_IF_NONE_MATCH = 5,
	COAP_OPTION_OBSERVE = 6,
	COAP_OPTION_URI_PORT = 7,
	COAP_OPTION_LOCATION_PATH = 8,
	COAP_OPTION_URI_PATH = 11,
	COAP_OPTION_CONTENT_FORMAT = 12,
	COAP_OPTION_MAX_AGE = 14,
	COAP_OPTION_URI_QUERY = 15,
	COAP_OPTION_ACCEPT = 17,
	COAP_OPTION_LOCATION_QUERY = 20,
	COAP_OPTION_BLOCK2 = 23,
	COAP_OPTION_BLOCK1 = 27,
	COAP_OPTION_SIZE2 = 28,
	COAP_OPTION_PROXY_URI = 35,
	COAP_OPTION_PROXY_SCHEME = 39,
	COAP_OPTION_SIZE1 = 60,
};

/**
 * @brief Available request methods.
 *
 * To be used when creating a request or a response.
 */
enum coap_method {
	COAP_METHOD_GET = 1,
	COAP_METHOD_POST = 2,
	COAP_METHOD_PUT = 3,
	COAP_METHOD_DELETE = 4,
};

#define COAP_REQUEST_MASK 0x07

/**
 * @brief CoAP packets may be of one of these types.
 */
enum coap_msgtype {
	/**
	 * Confirmable message.
	 *
	 * The packet is a request or response the destination end-point must
	 * acknowledge.
	 */
	COAP_TYPE_CON = 0,
	/**
	 * Non-confirmable message.
	 *
	 * The packet is a request or response that doesn't
	 * require acknowledgements.
	 */
	COAP_TYPE_NON_CON = 1,
	/**
	 * Acknowledge.
	 *
	 * Response to a confirmable message.
	 */
	COAP_TYPE_ACK = 2,
	/**
	 * Reset.
	 *
	 * Rejecting a packet for any reason is done by sending a message
	 * of this type.
	 */
	COAP_TYPE_RESET = 3
};

#define coap_make_response_code(clas, det) ((clas << 5) | (det))

/**
 * @brief Set of response codes available for a response packet.
 *
 * To be used when creating a response.
 */
enum coap_response_code {
	COAP_RESPONSE_CODE_OK = coap_make_response_code(2, 0),
	COAP_RESPONSE_CODE_CREATED = coap_make_response_code(2, 1),
	COAP_RESPONSE_CODE_DELETED = coap_make_response_code(2, 2),
	COAP_RESPONSE_CODE_VALID = coap_make_response_code(2, 3),
	COAP_RESPONSE_CODE_CHANGED = coap_make_response_code(2, 4),
	COAP_RESPONSE_CODE_CONTENT = coap_make_response_code(2, 5),
	COAP_RESPONSE_CODE_CONTINUE = coap_make_response_code(2, 31),
	COAP_RESPONSE_CODE_BAD_REQUEST = coap_make_response_code(4, 0),
	COAP_RESPONSE_CODE_UNAUTHORIZED = coap_make_response_code(4, 1),
	COAP_RESPONSE_CODE_BAD_OPTION = coap_make_response_code(4, 2),
	COAP_RESPONSE_CODE_FORBIDDEN = coap_make_response_code(4, 3),
	COAP_RESPONSE_CODE_NOT_FOUND = coap_make_response_code(4, 4),
	COAP_RESPONSE_CODE_NOT_ALLOWED = coap_make_response_code(4, 5),
	COAP_RESPONSE_CODE_NOT_ACCEPTABLE = coap_make_response_code(4, 6),
	COAP_RESPONSE_CODE_INCOMPLETE = coap_make_response_code(4, 8),
	COAP_RESPONSE_CODE_PRECONDITION_FAILED = coap_make_response_code(4, 12),
	COAP_RESPONSE_CODE_REQUEST_TOO_LARGE = coap_make_response_code(4, 13),
	COAP_RESPONSE_CODE_UNSUPPORTED_CONTENT_FORMAT =
						coap_make_response_code(4, 15),
	COAP_RESPONSE_CODE_INTERNAL_ERROR = coap_make_response_code(5, 0),
	COAP_RESPONSE_CODE_NOT_IMPLEMENTED = coap_make_response_code(5, 1),
	COAP_RESPONSE_CODE_BAD_GATEWAY = coap_make_response_code(5, 2),
	COAP_RESPONSE_CODE_SERVICE_UNAVAILABLE = coap_make_response_code(5, 3),
	COAP_RESPONSE_CODE_GATEWAY_TIMEOUT = coap_make_response_code(5, 4),
	COAP_RESPONSE_CODE_PROXYING_NOT_SUPPORTED =
						coap_make_response_code(5, 5)
};

#define COAP_CODE_EMPTY (0)

struct coap_observer;
struct coap_packet;
struct coap_pending;
struct coap_reply;
struct coap_resource;

/**
 * @typedef coap_method_t
 * @brief Type of the callback being called when a resource's method is
 * invoked by the remote entity.
 */
typedef int (*coap_method_t)(struct coap_resource *resource,
			     struct coap_packet *request);

/**
 * @typedef coap_notify_t
 * @brief Type of the callback being called when a resource's has observers
 * to be informed when an update happens.
 */
typedef void (*coap_notify_t)(struct coap_resource *resource,
			      struct coap_observer *observer);

/**
 * @brief Description of CoAP resource.
 *
 * CoAP servers often want to register resources, so that clients can act on
 * them, by fetching their state or requesting updates to them.
 */
struct coap_resource {
	/** Which function to be called for each CoAP method */
	coap_method_t get, post, put, del;
	coap_notify_t notify;
	const char * const *path;
	void *user_data;
	sys_slist_t observers;
	int age;
};

/**
 * @brief Represents a remote device that is observing a local resource.
 */
struct coap_observer {
	sys_snode_t list;
	struct sockaddr addr;
	u8_t token[8];
	u8_t tkl;
};

/**
 * @brief Representation of a CoAP packet.
 */
struct coap_packet {
	struct net_pkt *pkt;
	struct net_buf *frag; /* Where CoAP header resides */
	u16_t offset; /* Where CoAP header starts.*/
	u8_t hdr_len; /* CoAP header length */
	u8_t opt_len; /* Total options length (delta + len + value) */
	u16_t last_delta; /* Used only when preparing CoAP packet */
};

/**
 * @typedef coap_reply_t
 * @brief Helper function to be called when a response matches the
 * a pending request.
 */
typedef int (*coap_reply_t)(const struct coap_packet *response,
			    struct coap_reply *reply,
			    const struct sockaddr *from);

/**
 * @brief Represents a request awaiting for an acknowledgment (ACK).
 */
struct coap_pending {
	struct net_pkt *pkt;
	struct sockaddr addr;
	s32_t timeout;
	u16_t id;
};

/**
 * @brief Represents the handler for the reply of a request, it is
 * also used when observing resources.
 */
struct coap_reply {
	coap_reply_t reply;
	void *user_data;
	int age;
	u8_t token[8];
	u16_t id;
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
void coap_observer_init(struct coap_observer *observer,
			const struct coap_packet *request,
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
bool coap_register_observer(struct coap_resource *resource,
			    struct coap_observer *observer);

/**
 * @brief Remove this observer from the list of registered observers
 * of that resource.
 *
 * @param resource Resource in which to remove the observer
 * @param observer Observer to be removed
 */
void coap_remove_observer(struct coap_resource *resource,
			  struct coap_observer *observer);

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
struct coap_observer *coap_find_observer_by_addr(
	struct coap_observer *observers, size_t len,
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
struct coap_observer *coap_observer_next_unused(
	struct coap_observer *observers, size_t len);

/**
 * @brief Indicates that a reply is expected for @a request.
 *
 * @param reply Reply structure to be initialized
 * @param request Request from which @a reply will be based
 */
void coap_reply_init(struct coap_reply *reply,
		     const struct coap_packet *request);

/**
 * @brief Represents the value of a CoAP option.
 *
 * To be used with coap_find_options().
 */
struct coap_option {
	u16_t delta;

#if defined(CONFIG_COAP_EXTENDED_OPTIONS_LEN)
	u16_t len;
	u8_t value[CONFIG_COAP_EXTENDED_OPTIONS_LEN_VALUE];
#else
	u8_t len;
	u8_t value[12];
#endif
};

/**
 * @brief Parses the CoAP packet in @a pkt, validating it and
 * initializing @a cpkt. @a pkt must remain valid while @a cpkt is used.
 *
 * @param cpkt Packet to be initialized from received @a pkt.
 * @param pkt Network Packet containing a CoAP packet, its @a data pointer is
 * positioned on the start of the CoAP packet.
 * @param options Parse options and cache its details.
 * @param opt_num Number of options
 *
 * @return 0 in case of success or negative in case of error.
 */
int coap_packet_parse(struct coap_packet *cpkt, struct net_pkt *pkt,
		      struct coap_option *options, u8_t opt_num);

/**
 * @brief Creates a new CoAP packet from a net_pkt. @a pkt must remain
 * valid while @a cpkt is used.
 *
 * @param cpkt New packet to be initialized using the storage from @a
 * pkt.
 * @param pkt Network Packet that will contain a CoAP packet
 * @param ver CoAP header version
 * @param type CoAP header type
 * @param tokenlen CoAP header token length
 * @param token CoAP header token
 * @param code CoAP header code
 * @param id CoAP header message id
 *
 * @return 0 in case of success or negative in case of error.
 */
int coap_packet_init(struct coap_packet *cpkt, struct net_pkt *pkt,
		     u8_t ver, u8_t type, u8_t tokenlen,
		     u8_t *token, u8_t code, u16_t id);

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
int coap_pending_init(struct coap_pending *pending,
		      const struct coap_packet *request,
		      const struct sockaddr *addr);

/**
 * @brief Returns the next available pending struct, that can be used
 * to track the retransmission status of a request.
 *
 * @param pendings Pointer to the array of #coap_pending structures
 * @param len Size of the array of #coap_pending structures
 *
 * @return pointer to a free #coap_pending structure, NULL in case
 * none could be found.
 */
struct coap_pending *coap_pending_next_unused(
	struct coap_pending *pendings, size_t len);

/**
 * @brief Returns the next available reply struct, so it can be used
 * to track replies and notifications received.
 *
 * @param replies Pointer to the array of #coap_reply structures
 * @param len Size of the array of #coap_reply structures
 *
 * @return pointer to a free #coap_reply structure, NULL in case
 * none could be found.
 */
struct coap_reply *coap_reply_next_unused(
	struct coap_reply *replies, size_t len);

/**
 * @brief After a response is received, clear all pending
 * retransmissions related to that response.
 *
 * @param response The received response
 * @param pendings Pointer to the array of #coap_reply structures
 * @param len Size of the array of #coap_reply structures
 *
 * @return pointer to the associated #coap_pending structure, NULL in
 * case none could be found.
 */
struct coap_pending *coap_pending_received(
	const struct coap_packet *response,
	struct coap_pending *pendings, size_t len);

/**
 * @brief After a response is received, call coap_reply_t handler
 * registered in #coap_reply structure
 *
 * @param response A response received
 * @param from Address from which the response was received
 * @param replies Pointer to the array of #coap_reply structures
 * @param len Size of the array of #coap_reply structures
 *
 * @return Pointer to the reply matching the packet received, NULL if
 * none could be found.
 */
struct coap_reply *coap_response_received(
	const struct coap_packet *response,
	const struct sockaddr *from,
	struct coap_reply *replies, size_t len);

/**
 * @brief Returns the next pending about to expire, pending->timeout
 * informs how many ms to next expiration.
 *
 * @param pendings Pointer to the array of #coap_pending structures
 * @param len Size of the array of #coap_pending structures
 *
 * @return The next #coap_pending to expire, NULL if none is about to
 * expire.
 */
struct coap_pending *coap_pending_next_to_expire(
	struct coap_pending *pendings, size_t len);

/**
 * @brief After a request is sent, user may want to cycle the pending
 * retransmission so the timeout is updated.
 *
 * @param pending Pending representation to have its timeout updated
 *
 * @return false if this is the last retransmission.
 */
bool coap_pending_cycle(struct coap_pending *pending);

/**
 * @brief Cancels the pending retransmission, so it again becomes
 * available.
 *
 * @param pending Pending representation to be canceled
 */
void coap_pending_clear(struct coap_pending *pending);

/**
 * @brief Cancels awaiting for this reply, so it becomes available
 * again.
 *
 * @param reply The reply to be canceled
 */
void coap_reply_clear(struct coap_reply *reply);

/**
 * @brief When a request is received, call the appropriate methods of
 * the matching resources.
 *
 * @param cpkt Packet received
 * @param resources Array of known resources
 * @param options Parsed options from coap_packet_parse()
 * @param opt_num Number of options
 *
 * @return 0 in case of success or negative in case of error.
 */
int coap_handle_request(struct coap_packet *cpkt,
			struct coap_resource *resources,
			struct coap_option *options,
			u8_t opt_num);

/**
 * @brief Indicates that this resource was updated and that the @a
 * notify callback should be called for every registered observer.
 *
 * @param resource Resource that was updated
 *
 * @return 0 in case of success or negative in case of error.
 */
int coap_resource_notify(struct coap_resource *resource);

/**
 * @brief Returns if this request is enabling observing a resource.
 *
 * @param request Request to be checked
 *
 * @return True if the request is enabling observing a resource, False
 * otherwise
 */
bool coap_request_is_observe(const struct coap_packet *request);

/**
 * @brief Append payload marker to CoAP packet
 *
 * @param cpkt Packet to append the payload marker (0xFF)
 *
 * @return 0 in case of success or negative in case of error.
 */
int coap_packet_append_payload_marker(struct coap_packet *cpkt);

/**
 * @brief Append payload to CoAP packet
 *
 * @param cpkt Packet to append the payload
 * @param payload CoAP packet payload
 * @param payload_len CoAP packet payload len
 *
 * @return 0 in case of success or negative in case of error.
 */
int coap_packet_append_payload(struct coap_packet *cpkt, u8_t *payload,
			       u16_t payload_len);

/**
 * @brief Appends an option to the packet.
 *
 * Note: options must be added in numeric order of their codes. Otherwise
 * error will be returned.
 * TODO: Add support for placing options according to its delta value.
 *
 * @param cpkt Packet to be updated
 * @param code Option code to add to the packet, see #coap_option_num
 * @param value Pointer to the value of the option, will be copied to the packet
 * @param len Size of the data to be added
 *
 * @return 0 in case of success or negative in case of error.
 */
int coap_packet_append_option(struct coap_packet *cpkt, u16_t code,
			      const u8_t *value, u16_t len);

/**
 * @brief Converts an option to its integer representation.
 *
 * Assumes that the number is encoded in the network byte order in the
 * option.
 *
 * @param option Pointer to the option value, retrieved by
 * coap_find_options()
 *
 * @return The integer representation of the option
 */
unsigned int coap_option_value_to_int(const struct coap_option *option);

/**
 * @brief Appends an integer value option to the packet.
 *
 * The option must be added in numeric order of their codes, and the
 * least amount of bytes will be used to encode the value.
 *
 * @param cpkt Packet to be updated
 * @param code Option code to add to the packet, see #coap_option_num
 * @param val Integer value to be added
 *
 * @return 0 in case of success or negative in case of error.
 */
int coap_append_option_int(struct coap_packet *cpkt, u16_t code,
			   unsigned int val);

/**
 * @brief Return the values associated with the option of value @a
 * code.
 *
 * @param cpkt CoAP packet representation
 * @param code Option number to look for
 * @param options Array of #coap_option where to store the value
 * of the options found
 * @param veclen Number of elements in the options array
 *
 * @return The number of options found in packet matching code,
 * negative on error.
 */
int coap_find_options(const struct coap_packet *cpkt, u16_t code,
		      struct coap_option *options, u16_t veclen);

/**
 * Represents the size of each block that will be transferred using
 * block-wise transfers [RFC7959]:
 *
 * Each entry maps directly to the value that is used in the wire.
 *
 * https://tools.ietf.org/html/rfc7959
 */
enum coap_block_size {
	COAP_BLOCK_16,
	COAP_BLOCK_32,
	COAP_BLOCK_64,
	COAP_BLOCK_128,
	COAP_BLOCK_256,
	COAP_BLOCK_512,
	COAP_BLOCK_1024,
};

/**
 * @brief Helper for converting the enumeration to the size expressed
 * in bytes.
 *
 * @param block_size The block size to be converted
 *
 * @return The size in bytes that the block_size represents
 */
static inline u16_t coap_block_size_to_bytes(
	enum coap_block_size block_size)
{
	return (1 << (block_size + 4));
}

/**
 * @brief Represents the current state of a block-wise transaction.
 */
struct coap_block_context {
	size_t total_size;
	size_t current;
	enum coap_block_size block_size;
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
int coap_block_transfer_init(struct coap_block_context *ctx,
			     enum coap_block_size block_size,
			     size_t total_size);

/**
 * @brief Append BLOCK1 option to the packet.
 *
 * @param cpkt Packet to be updated
 * @param ctx Block context from which to retrieve the
 * information for the Block1 option
 *
 * @return 0 in case of success or negative in case of error.
 */
int coap_append_block1_option(struct coap_packet *cpkt,
			      struct coap_block_context *ctx);

/**
 * @brief Append BLOCK2 option to the packet.
 *
 * @param cpkt Packet to be updated
 * @param ctx Block context from which to retrieve the
 * information for the Block2 option
 *
 * @return 0 in case of success or negative in case of error.
 */
int coap_append_block2_option(struct coap_packet *cpkt,
			      struct coap_block_context *ctx);

/**
 * @brief Append SIZE1 option to the packet.
 *
 * @param cpkt Packet to be updated
 * @param ctx Block context from which to retrieve the
 * information for the Size1 option
 *
 * @return 0 in case of success or negative in case of error.
 */
int coap_append_size1_option(struct coap_packet *cpkt,
			     struct coap_block_context *ctx);

/**
 * @brief Append SIZE2 option to the packet.
 *
 * @param cpkt Packet to be updated
 * @param ctx Block context from which to retrieve the
 * information for the Size2 option
 *
 * @return 0 in case of success or negative in case of error.
 */
int coap_append_size2_option(struct coap_packet *cpkt,
			     struct coap_block_context *ctx);

/**
 * @brief Retrieves BLOCK{1,2} and SIZE{1,2} from @a cpkt and updates
 * @a ctx accordingly.
 *
 * @param cpkt Packet in which to look for block-wise transfers options
 * @param ctx Block context to be updated
 *
 * @return 0 in case of success or negative in case of error.
 */
int coap_update_from_block(const struct coap_packet *cpkt,
			   struct coap_block_context *ctx);

/**
 * @brief Updates @a ctx so after this is called the current entry
 * indicates the correct offset in the body of data being
 * transferred.
 *
 * @param cpkt Packet in which to look for block-wise transfers options
 * @param ctx Block context to be updated
 *
 * @return The offset in the block-wise transfer, 0 if the transfer
 * has finished.
 */
size_t coap_next_block(const struct coap_packet *cpkt,
		       struct coap_block_context *ctx);

/**
 * @brief Returns the version present in a CoAP packet.
 *
 * @param cpkt CoAP packet representation
 *
 * @return the CoAP version in packet
 */
u8_t coap_header_get_version(const struct coap_packet *cpkt);

/**
 * @brief Returns the type of the CoAP packet.
 *
 * @param cpkt CoAP packet representation
 *
 * @return the type of the packet
 */
u8_t coap_header_get_type(const struct coap_packet *cpkt);

/**
 * @brief Returns the token (if any) in the CoAP packet.
 *
 * @param cpkt CoAP packet representation
 * @param token Where to store the token
 *
 * @return Token length in the CoAP packet.
 */
u8_t coap_header_get_token(const struct coap_packet *cpkt, u8_t *token);

/**
 * @brief Returns the code of the CoAP packet.
 *
 * @param cpkt CoAP packet representation
 *
 * @return the code present in the packet
 */
u8_t coap_header_get_code(const struct coap_packet *cpkt);

/**
 * @brief Returns the message id associated with the CoAP packet.
 *
 * @param cpkt CoAP packet representation
 *
 * @return the message id present in the packet
 */
u16_t coap_header_get_id(const struct coap_packet *cpkt);

/**
 * @brief Helper to generate message ids
 *
 * @return a new message id
 */
static inline u16_t coap_next_id(void)
{
	static u16_t message_id;

	return ++message_id;
}

/**
 * @brief Returns the fragment pointer and offset where payload starts
 * in the CoAP packet.
 *
 * @param cpkt CoAP packet representation
 * @param offset Stores the offset value where payload starts
 * @param len Total length of CoAP payload
 *
 * @return the net_buf fragment pointer and offset value if payload exists
 *         NULL pointer and offset set to 0 in case there is no payload
 *         NULL pointer and offset value 0xffff in case of an error
 */
struct net_buf *coap_packet_get_payload(const struct coap_packet *cpkt,
					u16_t *offset,
					u16_t *len);


/**
 * @brief Returns a randomly generated array of 8 bytes, that can be
 * used as a message's token.
 *
 * @return a 8-byte pseudo-random token.
 */
u8_t *coap_next_token(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __COAP_H__ */
