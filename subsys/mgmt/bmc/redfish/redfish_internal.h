/*
 * SPDX-FileCopyrightText: Copyright (c) 2025-2026 Tenstorrent USA, Inc.
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_MGMT_BMC_REDFISH_REDFISH_INTERNAL_H_
#define ZEPHYR_SUBSYS_MGMT_BMC_REDFISH_REDFISH_INTERNAL_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/data/json.h>
#include <zephyr/mgmt/bmc/redfish.h>

/* URIs of the singleton resources, referenced from several of the schemas. */
#define REDFISH_URI_ROOT            "/redfish/v1/"
#define REDFISH_URI_ACCOUNT_SERVICE "/redfish/v1/AccountService"
#define REDFISH_URI_ACCOUNTS        REDFISH_URI_ACCOUNT_SERVICE "/Accounts"
#define REDFISH_URI_ADMIN_ACCOUNT   REDFISH_URI_ACCOUNTS "/1"
#define REDFISH_URI_MANAGERS        "/redfish/v1/Managers"
#define REDFISH_URI_MANAGER         REDFISH_URI_MANAGERS "/bmc"
#define REDFISH_URI_ETHERNET_IFS    REDFISH_URI_MANAGER "/EthernetInterfaces"
#define REDFISH_URI_ETHERNET_IF     REDFISH_URI_ETHERNET_IFS "/eth0"
#define REDFISH_URI_NETWORK_PROTO   REDFISH_URI_MANAGER "/NetworkProtocol"
#define REDFISH_URI_SYSTEMS         "/redfish/v1/Systems"
#define REDFISH_URI_SYSTEM          REDFISH_URI_SYSTEMS "/system"
#define REDFISH_URI_SYSTEM_RESET    REDFISH_URI_SYSTEM "/Actions/ComputerSystem.Reset"
#define REDFISH_URI_CHASSIS_COLL    "/redfish/v1/Chassis"
#define REDFISH_URI_CHASSIS         REDFISH_URI_CHASSIS_COLL "/1"
#define REDFISH_URI_SENSORS         REDFISH_URI_CHASSIS "/Sensors"

/** A `@odata.id` reference nested inside another resource. */
struct redfish_link {
	const char *odata_id;
};

extern const struct json_obj_descr redfish_link_descr[1];

/**
 * @brief Start encoding a Redfish collection.
 *
 * Collections are streamed rather than encoded from a fixed array so that the
 * member count is not capped at build time.
 *
 * @param ctx Transaction context.
 * @param odata_id URI of the collection.
 * @param odata_type Redfish type of the collection.
 * @param name Display name of the collection.
 * @param count Number of members that will follow.
 *
 * @return 0 on success, negative errno otherwise.
 */
int redfish_collection_open(struct bmc_redfish_ctx *ctx, const char *odata_id,
			    const char *odata_type, const char *name, size_t count);

/**
 * @brief Add one member URI to the collection being encoded.
 *
 * @param ctx Transaction context.
 * @param first Whether this is the first member, which controls the separator.
 * @param uri Member URI, formatted from @p uri and the remaining arguments.
 * @param ... Arguments for @p uri.
 *
 * @return 0 on success, negative errno otherwise.
 */
int redfish_collection_add(struct bmc_redfish_ctx *ctx, bool first, const char *uri, ...);

/**
 * @brief Finish encoding a Redfish collection.
 *
 * @param ctx Transaction context.
 *
 * @return 0 on success, negative errno otherwise.
 */
int redfish_collection_close(struct bmc_redfish_ctx *ctx);

/**
 * @brief Encode a resource and let vendor extensions append to it.
 *
 * @param ctx Transaction context.
 * @param descr JSON descriptor array.
 * @param descr_len Number of entries in @p descr.
 * @param val Object to encode.
 * @param target The @ref bmc_redfish_oem_target this resource represents.
 *
 * @return 0 on success, negative errno otherwise.
 */
int redfish_encode_with_oem(struct bmc_redfish_ctx *ctx, const struct json_obj_descr *descr,
			    size_t descr_len, const void *val, uint8_t target);

/**
 * @brief Current time as a Redfish DateTime string.
 *
 * Not reentrant, the result is only valid until the next call.
 *
 * @return The formatted time.
 */
const char *redfish_iso_time(void);

#endif /* ZEPHYR_SUBSYS_MGMT_BMC_REDFISH_REDFISH_INTERNAL_H_ */
