# Copyright (c) 2020 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

if [ -z "$RUNNING_FROM_MAIN_SCRIPT" ]; then
    echo "Do not run this script directly!"
    echo "Run $ZEPHYR_BASE/scripts/net/run-sample-tests.sh instead."
    exit 1
fi

start_configuration || return $?

start_docker "/usr/local/bin/dnsmasq.sh" || return $?

# First IPv6
start_zephyr "$overlay" "-DCONFIG_NET_IPV4=n" \
             "-DCONFIG_NET_CONFIG_NEED_IPV4=n" \
             "-DCONFIG_MDNS_RESOLVER=n"
wait_zephyr
result=$?
if [ $result -ne 0 ]; then
    echo "IPv6 failed"
    return ${result}
else
    echo "IPv6 ok"
fi

# Then IPv4
start_zephyr "$overlay" "-DCONFIG_NET_IPV6=n" \
             "-DCONFIG_NET_CONFIG_NEED_IPV6=n" \
             "-DCONFIG_MDNS_RESOLVER=n"
wait_zephyr
result=$?
if [ $result -ne 0 ]; then
    echo "IPv4 failed"
    return ${result}
else
    echo "IPv4 ok"
fi

stop_docker

#####
# FIXME: Docker does not pass multicast traffic through, so mDNS tests
# do not work atm. Fix this and then remove the next exit statement to
# let mDNS tests to run.
exit 0

# Then same but with mDNS and avahi

start_docker "/usr/local/bin/avahi-daemon.sh" || return $?

# First IPv6
start_zephyr "$overlay" "-DCONFIG_NET_IPV4=n" \
             "-DCONFIG_NET_CONFIG_NEED_IPV4=n" \
             "-DCONFIG_MDNS_RESOLVER=y" "-DCONFIG_NET_UDP_CHECKSUM=n"
wait_zephyr
result=$?
if [ $result -ne 0 ]; then
    echo "mDNS IPv6 failed"
    return ${result}
else
    echo "mDNS IPv6 ok"
fi

# Then IPv4
start_zephyr "$overlay" "-DCONFIG_NET_IPV6=n" \
             "-DCONFIG_NET_CONFIG_NEED_IPV6=n" \
             "-DCONFIG_MDNS_RESOLVER=y" "-DCONFIG_NET_UDP_CHECKSUM=n"
wait_zephyr
result=$?
if [ $result -ne 0 ]; then
    echo "mDNS IPv4 failed"
    return ${result}
else
    echo "mDNS IPv4 ok"
fi

stop_docker

return 0
