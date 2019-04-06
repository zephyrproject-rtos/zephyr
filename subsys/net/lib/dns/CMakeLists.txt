# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_library()
zephyr_library_sources(dns_pack.c)

zephyr_library_sources_ifdef(CONFIG_DNS_RESOLVER resolve.c)

if(CONFIG_MDNS_RESPONDER)
  zephyr_library_sources(mdns_responder.c)
  zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/ip)
endif()

if(CONFIG_LLMNR_RESPONDER)
  zephyr_library_sources(llmnr_responder.c)
  zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/ip)
endif()
