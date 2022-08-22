# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_LORAWAN_SERVICES
        clock_sync.c
        frag_flash.c
        frag_transport.c
        lorawan_services.c
        multicast.c
)
