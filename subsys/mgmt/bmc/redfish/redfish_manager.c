/*
 * Redfish Manager resources describing the BMC itself: its clock, its network
 * interface and the network protocols it runs.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2025-2026 Tenstorrent USA, Inc.
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/bmc.h>
#include <zephyr/mgmt/bmc/config.h>
#include <zephyr/mgmt/bmc/redfish.h>
#include <zephyr/net/hostname.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>

#include "bmc_internal.h"
#include "redfish_internal.h"

LOG_MODULE_DECLARE(bmc, CONFIG_BMC_LOG_LEVEL);

/*** /redfish/v1/Managers ***/
static int managers_collection_get(struct bmc_redfish_ctx *ctx)
{
	int ret;

	ret = redfish_collection_open(ctx, REDFISH_URI_MANAGERS,
				      "#ManagerCollection.ManagerCollection", "Manager Collection",
				      1);
	if (ret == 0) {
		ret = redfish_collection_add(ctx, true, REDFISH_URI_MANAGER);
	}

	if (ret == 0) {
		ret = redfish_collection_close(ctx);
	}

	return (ret < 0) ? HTTP_500_INTERNAL_SERVER_ERROR : 0;
}

BMC_REDFISH_RESOURCE_DEFINE(redfish_managers, REDFISH_URI_MANAGERS, true,
			    managers_collection_get, NULL, NULL);

/*** /redfish/v1/Managers/bmc ***/
struct redfish_manager {
	const char *odata_id;
	const char *odata_type;
	const char *id;
	const char *name;
	const char *manager_type;
	const char *uuid;
	const char *date_time;
	const char *firmware_version;
	struct redfish_link ethernet_interfaces;
	struct redfish_link network_protocol;
};

static const struct json_obj_descr manager_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_manager, "@odata.id", odata_id,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_manager, "@odata.type", odata_type,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_manager, "Id", id, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_manager, "Name", name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_manager, "ManagerType", manager_type,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_manager, "UUID", uuid, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_manager, "DateTime", date_time,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_manager, "FirmwareVersion", firmware_version,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct redfish_manager, "EthernetInterfaces",
				    ethernet_interfaces, redfish_link_descr),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct redfish_manager, "NetworkProtocol", network_protocol,
				    redfish_link_descr),
};

static int manager_patch(struct bmc_redfish_ctx *ctx)
{
	struct redfish_manager payload;
	int ret;

	memset(&payload, 0, sizeof(payload));

	ret = bmc_redfish_request_parse(ctx, manager_descr, ARRAY_SIZE(manager_descr), &payload);
	if (ret < 0) {
		LOG_ERR("Manager: malformed JSON (err=%d)", ret);
		return HTTP_400_BAD_REQUEST;
	}

	if (payload.date_time != NULL) {
		ret = bmc_time_set_from_iso_str(payload.date_time);
		if (ret < 0) {
			LOG_ERR("Could not set the date and time (err=%d)", ret);
			return HTTP_500_INTERNAL_SERVER_ERROR;
		}
	}

	return 0;
}

static int manager_get(struct bmc_redfish_ctx *ctx)
{
	const struct bmc_redfish_identity *identity = bmc_redfish_identity_get();
	const struct redfish_manager manager = {
		.odata_id = REDFISH_URI_MANAGER,
		.odata_type = "#Manager.v1_11_0.Manager",
		.id = "bmc",
		.name = "BMC",
		.manager_type = "BMC",
		.uuid = bmc_uuid_get(),
		.date_time = redfish_iso_time(),
		.firmware_version = identity->firmware_version,
		.ethernet_interfaces = {.odata_id = REDFISH_URI_ETHERNET_IFS},
		.network_protocol = {.odata_id = REDFISH_URI_NETWORK_PROTO},
	};

	if (redfish_encode_with_oem(ctx, manager_descr, ARRAY_SIZE(manager_descr), &manager,
				    BMC_REDFISH_OEM_MANAGER) < 0) {
		return HTTP_500_INTERNAL_SERVER_ERROR;
	}

	return 0;
}

BMC_REDFISH_RESOURCE_DEFINE(redfish_manager, REDFISH_URI_MANAGER, true, manager_get,
			    manager_patch, NULL);

/*** /redfish/v1/Managers/bmc/EthernetInterfaces ***/
static int ethernet_collection_get(struct bmc_redfish_ctx *ctx)
{
	int ret;

	ret = redfish_collection_open(
		ctx, REDFISH_URI_ETHERNET_IFS,
		"#EthernetInterfaceCollection.EthernetInterfaceCollection",
		"Ethernet Interface Collection", 1);
	if (ret == 0) {
		ret = redfish_collection_add(ctx, true, REDFISH_URI_ETHERNET_IF);
	}

	if (ret == 0) {
		ret = redfish_collection_close(ctx);
	}

	return (ret < 0) ? HTTP_500_INTERNAL_SERVER_ERROR : 0;
}

BMC_REDFISH_RESOURCE_DEFINE(redfish_ethernet_interfaces, REDFISH_URI_ETHERNET_IFS, true,
			    ethernet_collection_get, NULL, NULL);

/*** /redfish/v1/Managers/bmc/EthernetInterfaces/eth0 ***/
struct redfish_dhcp_v4 {
	uint8_t dhcp_enabled;
};

static const struct json_obj_descr dhcp_v4_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_dhcp_v4, "DHCPEnabled", dhcp_enabled,
				  JSON_TOK_TRUE),
};

struct redfish_ipv4_addr {
	const char *address;
	const char *subnet_mask;
	const char *gateway;
	const char *address_origin;
};

static const struct json_obj_descr ipv4_addr_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_ipv4_addr, "Address", address, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_ipv4_addr, "SubnetMask", subnet_mask,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_ipv4_addr, "Gateway", gateway, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_ipv4_addr, "AddressOrigin", address_origin,
				  JSON_TOK_STRING),
};

struct redfish_ethernet_interface {
	const char *odata_id;
	const char *odata_type;
	const char *id;
	const char *name;
	const char *host_name;
	struct redfish_dhcp_v4 dhcp_v4;
	struct redfish_ipv4_addr ipv4_addresses[1];
	size_t ipv4_count;
	struct redfish_ipv4_addr ipv4_static_addresses[1];
	size_t ipv4_static_count;
};

static const struct json_obj_descr ethernet_interface_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_ethernet_interface, "@odata.id", odata_id,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_ethernet_interface, "@odata.type", odata_type,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_ethernet_interface, "Id", id, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_ethernet_interface, "Name", name,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_ethernet_interface, "HostName", host_name,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct redfish_ethernet_interface, "DHCPv4", dhcp_v4,
				    dhcp_v4_descr),
	JSON_OBJ_DESCR_OBJ_ARRAY_NAMED(struct redfish_ethernet_interface, "IPv4Addresses",
				       ipv4_addresses, 1, ipv4_count, ipv4_addr_descr,
				       ARRAY_SIZE(ipv4_addr_descr)),
	JSON_OBJ_DESCR_OBJ_ARRAY_NAMED(struct redfish_ethernet_interface, "IPv4StaticAddresses",
				       ipv4_static_addresses, 1, ipv4_static_count,
				       ipv4_addr_descr, ARRAY_SIZE(ipv4_addr_descr)),
};

static int ethernet_patch(struct bmc_redfish_ctx *ctx)
{
	struct redfish_ethernet_interface payload;
	int ret;

	memset(&payload, 0, sizeof(payload));
	payload.dhcp_v4.dhcp_enabled = 0xff; /* sentinel for "not present" */
	payload.ipv4_static_count = SIZE_MAX;

	ret = bmc_redfish_request_parse(ctx, ethernet_interface_descr,
					ARRAY_SIZE(ethernet_interface_descr), &payload);
	if (ret < 0) {
		LOG_ERR("EthernetInterface: malformed JSON (err=%d)", ret);
		return HTTP_400_BAD_REQUEST;
	}

	if (payload.host_name != NULL) {
		ret = bmc_config_hostname_set(payload.host_name);
		if (ret < 0) {
			LOG_ERR("Could not set the hostname (err=%d)", ret);
			return HTTP_500_INTERNAL_SERVER_ERROR;
		}
	}

	if (payload.dhcp_v4.dhcp_enabled != 0xff) {
		ret = bmc_config_use_dhcp4_set(payload.dhcp_v4.dhcp_enabled != 0);
		if (ret < 0) {
			LOG_ERR("Could not change the DHCPv4 setting (err=%d)", ret);
			return HTTP_500_INTERNAL_SERVER_ERROR;
		}
	}

	if (payload.ipv4_static_count != SIZE_MAX) {
		const struct redfish_ipv4_addr *addr = &payload.ipv4_static_addresses[0];

		/* A count of 0 leaves the addresses NULL, which clears them. */
		ret = bmc_config_static_ip4_set(addr->address);
		if (ret == 0) {
			ret = bmc_config_static_ip4_netmask_set(addr->subnet_mask);
		}

		if (ret == 0) {
			ret = bmc_config_static_ip4_gateway_set(addr->gateway);
		}

		if (ret < 0) {
			LOG_ERR("Could not set the static IPv4 configuration (err=%d)", ret);
			return HTTP_500_INTERNAL_SERVER_ERROR;
		}
	}

	return 0;
}

static void fill_active_address(struct redfish_ethernet_interface *iface_info, char *ip_str,
				char *nm_str, char *gw_str, size_t str_len)
{
	struct redfish_ipv4_addr *addr_info = &iface_info->ipv4_addresses[0];
	struct net_if *iface = net_if_get_default();
	const struct net_if_ipv4 *ipv4;

	if (iface == NULL || iface->config.ip.ipv4 == NULL) {
		return;
	}

	ipv4 = iface->config.ip.ipv4;

	for (int i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		const struct net_if_addr_ipv4 *addr = &ipv4->unicast[i];

		if (!addr->ipv4.is_used) {
			continue;
		}

		iface_info->ipv4_count = 1;

		net_addr_ntop(AF_INET, &addr->ipv4.address.in_addr, ip_str, str_len);
		addr_info->address = ip_str;

		net_addr_ntop(AF_INET, &addr->netmask, nm_str, str_len);
		addr_info->subnet_mask = nm_str;

		if (addr->ipv4.addr_type == NET_ADDR_DHCP) {
			addr_info->address_origin = "DHCP";
		} else if (addr->ipv4.addr_type == NET_ADDR_MANUAL ||
			   addr->ipv4.addr_type == NET_ADDR_OVERRIDABLE) {
			addr_info->address_origin = "Static";
		} else {
			addr_info->address_origin = "Unknown";
		}

		/* Keep looking unless this is the preferred address. */
		if (addr->ipv4.addr_state == NET_ADDR_PREFERRED) {
			break;
		}
	}

	if (iface_info->ipv4_count == 1) {
		const struct net_in_addr gw = net_if_ipv4_get_gw(iface);

		if (gw.s_addr != 0) {
			net_addr_ntop(AF_INET, &gw, gw_str, str_len);
			addr_info->gateway = gw_str;
		} else {
			addr_info->gateway = "0.0.0.0";
		}
	}
}

static int ethernet_get(struct bmc_redfish_ctx *ctx)
{
	char ip_str[NET_IPV4_ADDR_LEN];
	char nm_str[NET_IPV4_ADDR_LEN];
	char gw_str[NET_IPV4_ADDR_LEN];
	char static_ip_str[NET_IPV4_ADDR_LEN];
	char static_nm_str[NET_IPV4_ADDR_LEN];
	char static_gw_str[NET_IPV4_ADDR_LEN];
	struct redfish_ethernet_interface iface_info = {
		.odata_id = REDFISH_URI_ETHERNET_IF,
		.odata_type = "#EthernetInterface.v1_5_0.EthernetInterface",
		.id = "eth0",
		.name = "BMC Ethernet Interface",
		.host_name = net_hostname_get(),
		.dhcp_v4 = {.dhcp_enabled = bmc_config_use_dhcp4()},
		.ipv4_count = 0,
		.ipv4_static_count = 0,
		.ipv4_static_addresses = {{
			.address = "0.0.0.0",
			.subnet_mask = "0.0.0.0",
			.gateway = "0.0.0.0",
		}},
	};

	fill_active_address(&iface_info, ip_str, nm_str, gw_str, NET_IPV4_ADDR_LEN);

	if (bmc_config_static_ip4() != 0) {
		struct redfish_ipv4_addr *addr = &iface_info.ipv4_static_addresses[0];
		uint32_t value;

		iface_info.ipv4_static_count = 1;

		value = bmc_config_static_ip4();
		net_addr_ntop(AF_INET, &value, static_ip_str, sizeof(static_ip_str));
		addr->address = static_ip_str;

		value = bmc_config_static_ip4_netmask();
		net_addr_ntop(AF_INET, &value, static_nm_str, sizeof(static_nm_str));
		addr->subnet_mask = static_nm_str;

		value = bmc_config_static_ip4_gateway();
		net_addr_ntop(AF_INET, &value, static_gw_str, sizeof(static_gw_str));
		addr->gateway = static_gw_str;
	}

	if (bmc_redfish_reply_encode(ctx, ethernet_interface_descr,
				     ARRAY_SIZE(ethernet_interface_descr), &iface_info) < 0) {
		return HTTP_500_INTERNAL_SERVER_ERROR;
	}

	return 0;
}

BMC_REDFISH_RESOURCE_DEFINE(redfish_ethernet, REDFISH_URI_ETHERNET_IF, true, ethernet_get,
			    ethernet_patch, NULL);

/*** /redfish/v1/Managers/bmc/NetworkProtocol ***/
struct redfish_ntp {
	uint8_t protocol_enabled;
	const char *ntp_servers[1];
	size_t ntp_servers_count;
};

static const struct json_obj_descr ntp_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_ntp, "ProtocolEnabled", protocol_enabled,
				  JSON_TOK_TRUE),
	JSON_OBJ_DESCR_ARRAY_NAMED(struct redfish_ntp, "NTPServers", ntp_servers, 1,
				   ntp_servers_count, JSON_TOK_STRING),
};

struct redfish_network_protocol {
	const char *odata_id;
	const char *odata_type;
	const char *id;
	const char *host_name;
	struct redfish_ntp ntp;
};

static const struct json_obj_descr network_protocol_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_network_protocol, "@odata.id", odata_id,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_network_protocol, "@odata.type", odata_type,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_network_protocol, "Id", id, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_network_protocol, "HostName", host_name,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct redfish_network_protocol, "NTP", ntp, ntp_descr),
};

static int network_protocol_patch(struct bmc_redfish_ctx *ctx)
{
	struct redfish_network_protocol payload;
	int ret;

	memset(&payload, 0, sizeof(payload));
	payload.ntp.protocol_enabled = 0xff; /* sentinel for "not present" */
	payload.ntp.ntp_servers_count = SIZE_MAX;

	ret = bmc_redfish_request_parse(ctx, network_protocol_descr,
					ARRAY_SIZE(network_protocol_descr), &payload);
	if (ret < 0) {
		LOG_ERR("NetworkProtocol: malformed JSON (err=%d)", ret);
		return HTTP_400_BAD_REQUEST;
	}

	if (payload.ntp.ntp_servers_count != SIZE_MAX && payload.ntp.ntp_servers_count > 0) {
		ret = bmc_config_ntp_server_set(payload.ntp.ntp_servers[0]);
		if (ret < 0) {
			LOG_ERR("Could not set the NTP server (err=%d)", ret);
			return HTTP_500_INTERNAL_SERVER_ERROR;
		}
	}

	if (payload.ntp.protocol_enabled != 0xff) {
		ret = bmc_config_use_ntp_set(payload.ntp.protocol_enabled != 0);
		if (ret < 0) {
			LOG_ERR("Could not change the NTP setting (err=%d)", ret);
			return HTTP_500_INTERNAL_SERVER_ERROR;
		}
	}

	return 0;
}

static int network_protocol_get(struct bmc_redfish_ctx *ctx)
{
	struct redfish_network_protocol network_protocol = {
		.odata_id = REDFISH_URI_NETWORK_PROTO,
		.odata_type = "#ManagerNetworkProtocol.v1_9_0.ManagerNetworkProtocol",
		.id = "NetworkProtocol",
		.host_name = net_hostname_get(),
		.ntp = {.protocol_enabled = bmc_config_use_ntp()},
	};

	if (bmc_config_ntp_server()[0] != '\0') {
		network_protocol.ntp.ntp_servers[0] = bmc_config_ntp_server();
		network_protocol.ntp.ntp_servers_count = 1;
	}

	if (bmc_redfish_reply_encode(ctx, network_protocol_descr,
				     ARRAY_SIZE(network_protocol_descr), &network_protocol) < 0) {
		return HTTP_500_INTERNAL_SERVER_ERROR;
	}

	return 0;
}

BMC_REDFISH_RESOURCE_DEFINE(redfish_network_protocol, REDFISH_URI_NETWORK_PROTO, true,
			    network_protocol_get, network_protocol_patch, NULL);
