/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * $FreeBSD$
 */

#ifndef _RPMSG_INTERNAL_H_
#define _RPMSG_INTERNAL_H_

#include <stdint.h>
#include <openamp/rpmsg.h>

#if defined __cplusplus
extern "C" {
#endif

#ifdef RPMSG_DEBUG
#define RPMSG_ASSERT(_exp, _msg) do { \
		if (!(_exp)) { \
			openamp_print("FATAL: %s - _msg", __func__); \
			while (1) { \
				; \
			} \
		} \
	} while (0)
#else
#define RPMSG_ASSERT(_exp, _msg) do { \
		if (!(_exp)) \
			while (1) { \
				; \
			} \
	} while (0)
#endif

#define RPMSG_LOCATE_DATA(p) ((unsigned char *)(p) + sizeof(struct rpmsg_hdr))
/**
 * enum rpmsg_ns_flags - dynamic name service announcement flags
 *
 * @RPMSG_NS_CREATE: a new remote service was just created
 * @RPMSG_NS_DESTROY: a known remote service was just destroyed
 * @RPMSG_NS_CREATE_WITH_ACK: a new remote service was just created waiting
 *                            acknowledgment.
 */
enum rpmsg_ns_flags {
	RPMSG_NS_CREATE = 0,
	RPMSG_NS_DESTROY = 1,
};

/**
 * struct rpmsg_hdr - common header for all rpmsg messages
 * @src: source address
 * @dst: destination address
 * @reserved: reserved for future use
 * @len: length of payload (in bytes)
 * @flags: message flags
 *
 * Every message sent(/received) on the rpmsg bus begins with this header.
 */
OPENAMP_PACKED_BEGIN
struct rpmsg_hdr {
	uint32_t src;
	uint32_t dst;
	uint32_t reserved;
	uint16_t len;
	uint16_t flags;
} OPENAMP_PACKED_END;

/**
 * struct rpmsg_ns_msg - dynamic name service announcement message
 * @name: name of remote service that is published
 * @addr: address of remote service that is published
 * @flags: indicates whether service is created or destroyed
 *
 * This message is sent across to publish a new service, or announce
 * about its removal. When we receive these messages, an appropriate
 * rpmsg channel (i.e device) is created/destroyed. In turn, the ->probe()
 * or ->remove() handler of the appropriate rpmsg driver will be invoked
 * (if/as-soon-as one is registered).
 */
OPENAMP_PACKED_BEGIN
struct rpmsg_ns_msg {
	char name[RPMSG_NAME_SIZE];
	uint32_t addr;
	uint32_t flags;
} OPENAMP_PACKED_END;

int rpmsg_send_ns_message(struct rpmsg_endpoint *ept, unsigned long flags);

struct rpmsg_endpoint *rpmsg_get_endpoint(struct rpmsg_device *rvdev,
					  const char *name, uint32_t addr,
					  uint32_t dest_addr);
int rpmsg_register_endpoint(struct rpmsg_device *rdev,
			    struct rpmsg_endpoint *ept);

static inline struct rpmsg_endpoint *
rpmsg_get_ept_from_addr(struct rpmsg_device *rdev, uint32_t addr)
{
	return rpmsg_get_endpoint(rdev, NULL, addr, RPMSG_ADDR_ANY);
}

#if defined __cplusplus
}
#endif

#endif /* _RPMSG_INTERNAL_H_ */
