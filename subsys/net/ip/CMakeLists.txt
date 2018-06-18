zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            net_context.c
            net_core.c
            net_if.c
            net_pkt.c
            net_tc.c
            utils.c
            IFDEF:${CONFIG_NET_6LO}              6lo.c
            IFDEF:${CONFIG_NET_DHCPV4}           dhcpv4.c
            IFDEF:${CONFIG_NET_IPV4_AUTO}        ipv4_autoconf.c
            IFDEF:${CONFIG_NET_IPV4}             icmpv4.c       ipv4.c
            IFDEF:${CONFIG_NET_IPV6}             icmpv6.c nbr.c ipv6.c ipv6_nbr.c
            IFDEF:${CONFIG_NET_IPV6_MLD}         ipv6_mld.c
            IFDEF:${CONFIG_NET_IPV6_FRAGMENT}    ipv6_fragment.c
            IFDEF:${CONFIG_NET_MGMT_EVENT}       net_mgmt.c
            IFDEF:${CONFIG_NET_ROUTE}            route.c
            IFDEF:${CONFIG_NET_RPL}              rpl.c
            IFDEF:${CONFIG_NET_RPL_MRHOF}        rpl-mrhof.c
            IFDEF:${CONFIG_NET_RPL_OF0}          rpl-of0.c
            IFDEF:${CONFIG_NET_SHELL}            net_shell.c
            IFDEF:${CONFIG_NET_STATISTICS}       net_stats.c
            IFDEF:${CONFIG_NET_TCP}              connection.c tcp.c
            IFDEF:${CONFIG_NET_TRICKLE}          trickle.c
            IFDEF:${CONFIG_NET_UDP}              connection.c udp.c
            IFDEF:${CONFIG_NET_PROMISCUOUS_MODE} promiscuous.c
)

target_sources(subsys_net PRIVATE ${PRIVATE_SOURCES})
target_include_directories(subsys_net PRIVATE ${CMAKE_CURRENT_LIST_DIR})
