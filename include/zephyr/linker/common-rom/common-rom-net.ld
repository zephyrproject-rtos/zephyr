/* SPDX-License-Identifier: Apache-2.0 */

#include <zephyr/linker/iterable_sections.h>

#if defined(CONFIG_NETWORKING)
	ITERABLE_SECTION_ROM(net_l3_register, Z_LINK_ITERABLE_SUBALIGN)
#endif

#if defined(CONFIG_NET_SOCKETS)
	ITERABLE_SECTION_ROM(net_socket_register, Z_LINK_ITERABLE_SUBALIGN)
#endif

#if defined(CONFIG_NET_L2_PPP)
	ITERABLE_SECTION_ROM(ppp_protocol_handler, Z_LINK_ITERABLE_SUBALIGN)
#endif

#if defined(CONFIG_DNS_SD)
	ITERABLE_SECTION_ROM(dns_sd_rec, Z_LINK_ITERABLE_SUBALIGN)
#endif

#if defined(CONFIG_HTTP_SERVER)
	ITERABLE_SECTION_ROM(http_service_desc, Z_LINK_ITERABLE_SUBALIGN)
#endif

#if defined(CONFIG_COAP_SERVER)
	ITERABLE_SECTION_ROM(coap_service, Z_LINK_ITERABLE_SUBALIGN)
#endif

#if defined(CONFIG_NET_MGMT_EVENT)
	ITERABLE_SECTION_ROM(net_mgmt_event_static_handler, Z_LINK_ITERABLE_SUBALIGN)
#endif

#if defined(CONFIG_NET_SOCKETS_SERVICE)
	ITERABLE_SECTION_ROM(net_socket_service_desc, Z_LINK_ITERABLE_SUBALIGN)
#endif
