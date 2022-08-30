/* SPDX-License-Identifier: Apache-2.0 */

#if defined(CONFIG_NET_SOCKETS)
	ITERABLE_SECTION_ROM(net_socket_register, 4)
#endif

#if defined(CONFIG_NET_L2_PPP)
	ITERABLE_SECTION_ROM(ppp_protocol_handler, 4)
#endif

#if defined(CONFIG_DNS_SD)
	ITERABLE_SECTION_ROM(dns_sd_rec, 4)
#endif
