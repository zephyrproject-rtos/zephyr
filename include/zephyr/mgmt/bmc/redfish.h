/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MGMT_BMC_REDFISH_H_
#define ZEPHYR_INCLUDE_MGMT_BMC_REDFISH_H_

/**
 * @file
 * @brief Redfish service hosted by the BMC.
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/data/json.h>
#include <zephyr/mgmt/bmc/http.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief BMC Redfish service
 * @defgroup bmc_redfish BMC Redfish service
 * @ingroup bmc_api
 * @{
 *
 * The BMC implements the Redfish resources that describe the BMC itself, the
 * managed system and its chassis. Everything product specific is supplied by
 * the application: the displayed names come from a @ref bmc_redfish_identity,
 * extra resources are added with BMC_REDFISH_RESOURCE_DEFINE() and vendor
 * extensions to the standard resources with BMC_REDFISH_OEM_DEFINE().
 */

/**
 * @brief State of one in-flight Redfish transaction.
 *
 * Handlers receive a pointer to this and use the bmc_redfish_request_*() and
 * bmc_redfish_reply_*() helpers to read the request payload and build the
 * response. The layout is internal to the BMC and may change.
 */
struct bmc_redfish_ctx {
	/** @cond INTERNAL_HIDDEN */
	const char *url;
	bool started;
	size_t size;
	size_t data_len;
	uint8_t *data_buffer;
	/** @endcond */
};

/**
 * @brief Get the URL the request was made to.
 *
 * Only useful for resources registered under a wildcard URL, which need to
 * work out which object is addressed.
 *
 * @param ctx Transaction context.
 *
 * @return The request URL, query string included.
 */
static inline const char *bmc_redfish_request_url(const struct bmc_redfish_ctx *ctx)
{
	return ctx->url;
}

/**
 * @brief Redfish handler callback.
 *
 * @param ctx Transaction context.
 *
 * @return 0 on success, or an @c http_status value to return to the client.
 */
typedef int (*bmc_redfish_handler_t)(struct bmc_redfish_ctx *ctx);

/**
 * @brief Product identity reported over Redfish.
 *
 * Every field may be NULL, in which case the corresponding CONFIG_BMC_REDFISH_*
 * Kconfig default is used.
 */
struct bmc_redfish_identity {
	/** System name shown in Redfish and on the dashboard. */
	const char *product_name;
	/** System and chassis manufacturer. */
	const char *manufacturer;
	/** System and chassis model. */
	const char *model;
	/** System and chassis serial number. */
	const char *serial_number;
	/** System and chassis part number. May be NULL to omit. */
	const char *part_number;
	/** BMC firmware version. Defaults to the Zephyr version. */
	const char *firmware_version;
	/** Host processor model. */
	const char *processor_model;
	/** Number of host processors. */
	uint16_t processor_count;
	/** Total host memory in GiB. 0 is reported as unknown. */
	uint32_t memory_gib;
};

/**
 * @brief Install the product identity reported over Redfish.
 *
 * A later registration replaces an earlier one. The @p identity structure must
 * remain valid for as long as it is registered.
 *
 * @param identity Identity to install, or NULL to restore the Kconfig defaults.
 */
void bmc_redfish_identity_register(const struct bmc_redfish_identity *identity);

/**
 * @brief Get the active product identity.
 *
 * Unset fields are filled in from Kconfig, so no field of the returned
 * structure is NULL except @c part_number.
 *
 * @return The identity in effect. Never NULL.
 */
const struct bmc_redfish_identity *bmc_redfish_identity_get(void);

/**
 * @brief Append raw bytes to the response body.
 *
 * @param ctx Transaction context.
 * @param data Bytes to append.
 * @param len Number of bytes.
 *
 * @retval 0 on success.
 * @retval -ENOSPC if the response buffer is full.
 */
int bmc_redfish_reply_append(struct bmc_redfish_ctx *ctx, const void *data, size_t len);

/**
 * @brief Encode a JSON object into the response body.
 *
 * @param ctx Transaction context.
 * @param descr JSON descriptor array.
 * @param descr_len Number of entries in @p descr.
 * @param val Object to encode.
 *
 * @return 0 on success, negative errno otherwise.
 */
int bmc_redfish_reply_encode(struct bmc_redfish_ctx *ctx, const struct json_obj_descr *descr,
			     size_t descr_len, const void *val);

/**
 * @brief Parse the accumulated request body as a JSON object.
 *
 * @param ctx Transaction context.
 * @param descr JSON descriptor array.
 * @param descr_len Number of entries in @p descr.
 * @param val Where to store the parsed object.
 *
 * @return A bitmask of decoded fields on success, negative errno otherwise.
 */
int bmc_redfish_request_parse(struct bmc_redfish_ctx *ctx, const struct json_obj_descr *descr,
			      size_t descr_len, void *val);

/** @brief Standard Redfish resources that accept vendor extensions. */
enum bmc_redfish_oem_target {
	/** `/redfish/v1/Managers/bmc` */
	BMC_REDFISH_OEM_MANAGER,
	/** `/redfish/v1/Systems/system` */
	BMC_REDFISH_OEM_SYSTEM,
	/** `/redfish/v1/Chassis/1` */
	BMC_REDFISH_OEM_CHASSIS,
};

/**
 * @brief A vendor extension to a standard Redfish resource.
 *
 * Do not populate this structure directly, use BMC_REDFISH_OEM_DEFINE().
 */
struct bmc_redfish_oem {
	/** Resource this extension applies to. */
	uint8_t target;
	/**
	 * @brief Append extension members to the resource.
	 *
	 * Called while the enclosing JSON object is still open. Use
	 * bmc_redfish_reply_add_member() to append each member.
	 *
	 * @param ctx Transaction context.
	 *
	 * @return 0 on success, negative errno otherwise.
	 */
	int (*encode)(struct bmc_redfish_ctx *ctx);
};

/**
 * @brief Extend a standard Redfish resource with vendor specific members.
 *
 * @param _sym Unique C symbol name for the extension.
 * @param _target One of @ref bmc_redfish_oem_target.
 * @param _encode Encoder callback.
 */
#define BMC_REDFISH_OEM_DEFINE(_sym, _target, _encode)                                             \
	static const STRUCT_SECTION_ITERABLE(bmc_redfish_oem, _sym) = {                            \
		.target = (_target),                                                               \
		.encode = (_encode),                                                               \
	}

/**
 * @brief Append one named member to the JSON object being built.
 *
 * Intended for @ref bmc_redfish_oem encoders, which run while the enclosing
 * object is still open.
 *
 * @param ctx Transaction context.
 * @param name Member name.
 * @param descr JSON descriptor array for the member value.
 * @param descr_len Number of entries in @p descr.
 * @param val Value to encode.
 *
 * @return 0 on success, negative errno otherwise.
 */
int bmc_redfish_reply_add_member(struct bmc_redfish_ctx *ctx, const char *name,
				 const struct json_obj_descr *descr, size_t descr_len,
				 const void *val);

/**
 * @brief A resource served by the Redfish router.
 *
 * Do not populate this structure directly, use BMC_REDFISH_RESOURCE_DEFINE().
 */
struct bmc_redfish_resource {
	/** URL path of the resource. */
	const char *url;
	/** Whether the resource requires authentication. */
	bool auth;
	/** GET handler, or NULL if the method is not supported. */
	bmc_redfish_handler_t get;
	/** PATCH handler, or NULL if the method is not supported. */
	bmc_redfish_handler_t patch;
	/** POST handler, or NULL if the method is not supported. */
	bmc_redfish_handler_t post;
};

/**
 * @brief Publish a Redfish resource.
 *
 * The BMC routes every `/redfish` request to the resource whose URL matches,
 * so applications only have to supply the handlers. A trailing slash in the
 * request is ignored. A URL ending in `/` @c * matches any single further path
 * segment, which is how collections with a runtime member list are served; the
 * handler then recovers the addressed member with bmc_redfish_request_url().
 * Exact URLs always win over wildcard ones.
 *
 * @param _name Unique C symbol prefix for the resource.
 * @param _url URL path of the resource.
 * @param _auth Whether the resource requires authentication.
 * @param _get GET handler, or NULL if the method is not supported.
 * @param _patch PATCH handler, or NULL if the method is not supported.
 * @param _post POST handler, or NULL if the method is not supported.
 */
#define BMC_REDFISH_RESOURCE_DEFINE(_name, _url, _auth, _get, _patch, _post)                       \
	static const STRUCT_SECTION_ITERABLE(bmc_redfish_resource, _name) = {                      \
		.url = (_url),                                                                     \
		.auth = (_auth),                                                                   \
		.get = (_get),                                                                     \
		.patch = (_patch),                                                                 \
		.post = (_post),                                                                   \
	}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_MGMT_BMC_REDFISH_H_ */
