#!/bin/sh
#
# Copyright (c) 2018 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Setup virtual lan (VLAN) in Linux side that would work with this sample.

if [ -z "$1" ]; then
    echo "Network interface is missing."
    echo "This is the network interface where the VLAN will be running."
    echo "Example: eth0, tap0 etc."
    exit 1
fi

if [ `id -u` != 0 ]; then
    echo "Run this script as a root user!"
    exit 2
fi

IFACE="$1"

VLAN_NAME_PREFIX="vlan"

PREFIX_1_IPV6="2001:db8:100"
PREFIXLEN_1_IPV6="64"
PREFIX_2_IPV6="2001:db8:200"
PREFIXLEN_2_IPV6="64"

# From RFC 5737
PREFIX_1_IPV4="198.51.100"
PREFIXLEN_1_IPV4="24"
PREFIX_2_IPV4="203.0.113"
PREFIXLEN_2_IPV4="24"

ip link add link ${IFACE} name ${VLAN_NAME_PREFIX}.100 type vlan id 100
ip link add link ${IFACE} name ${VLAN_NAME_PREFIX}.200 type vlan id 200

ip link set ${VLAN_NAME_PREFIX}.100 up
ip link set ${VLAN_NAME_PREFIX}.200 up

ip -6 addr add ${PREFIX_1_IPV6}::2 dev ${VLAN_NAME_PREFIX}.100
ip -6 route add ${PREFIX_1_IPV6}::/${PREFIXLEN_1_IPV6} \
   dev ${VLAN_NAME_PREFIX}.100

ip -6 addr add ${PREFIX_2_IPV6}::2 dev ${VLAN_NAME_PREFIX}.200
ip -6 route add ${PREFIX_2_IPV6}::/${PREFIXLEN_2_IPV6} \
   dev ${VLAN_NAME_PREFIX}.200

ip addr add ${PREFIX_1_IPV4}.2 dev ${VLAN_NAME_PREFIX}.100
ip route add ${PREFIX_1_IPV4}/${PREFIXLEN_1_IPV4} dev ${VLAN_NAME_PREFIX}.100

ip addr add ${PREFIX_2_IPV4}.2 dev ${VLAN_NAME_PREFIX}.200
ip route add ${PREFIX_2_IPV4}/${PREFIXLEN_2_IPV4} dev ${VLAN_NAME_PREFIX}.200


# You can remove the virtual interface like this
#    ip link del link eth0 dev vlan.100
#    ip link del link eth0 dev vlan.200

# If your devices HW MAC address changes when rebooting or flashing,
# then you can flush the neighbor cache in Linux like this:
#    ip neigh flush dev vlan.100
#    ip neigh flush dev vlan.200
