#!/bin/sh
#
# SPDX-FileCopyrightText: Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0
#

INTERFACE="zeth0"
INTERFACE2="zeth1"

HWADDR="00:00:5e:00:53:fe"
HWADDR2="00:00:5e:00:53:ff"

IPV6_ADDR_1="2001:db8::2"
IPV6_ROUTE_1="2001:db8::/64"
IPV6_ADDR_2="2001:db8::3"
IPV6_ROUTE_2="2001:db8::/64"

IPV4_ADDR_1="192.0.2.2/24"
IPV4_ROUTE_1="192.0.2.0/24"
IPV4_ADDR_2="192.0.2.3/24"
IPV4_ROUTE_2="192.0.2.0/24"

NETNS="ns0"
NETNS_EXEC="ip netns exec $NETNS"

if [ "$1" = "pre-setup" ]; then
	# interface 1
	echo "Create ${INTERFACE}..."
	ip tuntap add $INTERFACE mode tap
	ip link set dev $INTERFACE up
	ip link set dev $INTERFACE address $HWADDR
	ip -6 address add $IPV6_ADDR_1 dev $INTERFACE nodad
	ip -6 route add $IPV6_ROUTE_1 dev $INTERFACE
	ip address add $IPV4_ADDR_1 dev $INTERFACE
	ip route add $IPV4_ROUTE_1 dev $INTERFACE > /dev/null 2>&1

	# interface 2
	echo "Create ${INTERFACE2}..."
	ip tuntap add $INTERFACE2 mode tap
	ip link set dev $INTERFACE2 up
elif [ "$1" = "post-setup" ]; then
	# move interface 2 to netns
	echo "Move ${INTERFACE} to netns ${NETNS}..."
	ip netns add $NETNS
	ip link set ${INTERFACE2} netns $NETNS

	$NETNS_EXEC ip link set dev $INTERFACE2 up
	$NETNS_EXEC ip link set dev $INTERFACE2 address $HWADDR2
	$NETNS_EXEC ip -6 address add $IPV6_ADDR_2 dev $INTERFACE2 nodad
	$NETNS_EXEC ip -6 route add $IPV6_ROUTE_2 dev $INTERFACE2
	$NETNS_EXEC ip address add $IPV4_ADDR_2 dev $INTERFACE2
	$NETNS_EXEC ip route add $IPV4_ROUTE_2 dev $INTERFACE2 > /dev/null 2>&1
elif [ "$1" = "clean" ]; then
	echo "Clean up..."
	ip tuntap del $INTERFACE mode tap
	ip netns delete $NETNS
fi
