#!/usr/bin/env python3
# Copyright(c) 2024 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

import yaml, sys, socket

data = yaml.safe_load(sys.stdin)

# If pykwalify is installed, then validate the yaml
try:
    import pykwalify.core

    def yaml_validate(data, schema):
        if not schema:
            return
        c = pykwalify.core.Core(source_data=data, schema_files=[schema])
        c.validate(raise_exception=True)

except ImportError as e:
    sys.stderr.write("can't import pykwalify; won't validate YAML (%s)", e)
    def yaml_validate(data, schema):
        pass

# If user has supplied an argument, treat it as a schema file
if len(sys.argv[1:]) > 0:
    yaml_validate(data, sys.argv[1])

net = data['net']
netif = net['network-interfaces']
netif_count = len(netif)
dhcpv4_server_count = 0

def print_string_elem(dict, elem, field, indent):
    if elem in dict.keys():
        elem_value = dict[elem]
        if field != "":
            print(indent + "." + field + " = \"" + elem_value + "\",")
        else:
            print(indent + "\"" + elem_value + "\",")

def print_bool_elem(dict, elem, field, indent):
    if elem in dict.keys():
        elem_value = dict[elem]
        print(indent + "." + field + " = " + str(elem_value).lower() + ",")

def print_enabled_elem(dict, elem, field, indent):
    if elem in dict.keys():
        elem_value = dict[elem]
        if elem_value == "enabled":
            print(indent + "." + field + " = " + str(True).lower() + ",")

def print_int_elem(dict, elem, field, indent):
    if elem in dict.keys():
        elem_value = dict[elem]
        print(indent + "." + field + " = " + str(elem_value) + ",")

def print_hex_elem(dict, elem, field, indent):
    if elem in dict.keys():
        elem_value = dict[elem]
        print(indent + "." + field + " = " + str(hex(elem_value)) + ",")

def print_address_elem(dict, elem, field, family, indent):
    if elem in dict.keys():
        elem_value = dict[elem]
        address_count = len(elem_value)
        if address_count > 0:
            print(indent + "." + field + " = {")
            for i in range(address_count):
                addr = socket.inet_pton(family, elem_value[i])
                if family == socket.AF_INET:
                    print(indent + "\t" + "[" + str(i) + "]" + " = " +
                          "{ { { " + ",".join(str(n) for n in addr) + " } } },")
                else:
                    print(indent + "\t" + "[" + str(i) + "]" + " = " +
                          "{ { { " + ",".join(hex(n) for n in addr) + " } } },")

            print(indent + "},")

def print_ipv6_address_elem(dict, elem, field, family, indent):
    if elem in dict.keys():
        elem_value = dict[elem]
        address_count = len(elem_value)
        if address_count > 0:
            print(indent + "." + field + " = {")
            for i in range(address_count):
                addr = socket.inet_pton(family, elem_value[i])
                print(indent + "\t" + "[" + str(i) + "]" + " = " +
                      "{ { { " + ",".join(hex(n) for n in addr) + " } } },")

            print(indent + "},")

def print_ipv4_address_and_netmask_elem(dict, elem, field, family, indent):
    if elem in dict.keys():
        elem_value = dict[elem]
        address_count = len(elem_value)
        if address_count > 0:
            print(indent + "." + field + " = {")
            for i in range(address_count):
                netmask_len = None
                ipv4 = elem_value[i].split("/")
                addr = socket.inet_pton(family, ipv4[0])
                if len(ipv4) == 2:
                    netmask_len = ipv4[1]
                print(indent + "\t" + "[" + str(i) + "]" + " = { ")
                print(indent + "\t\t" + ".ipv4 = { { { " +
                      ",".join(str(n) for n in addr) + " } } },")
                if netmask_len is not None:
                    print(indent + "\t\t" + ".netmask_len = " + netmask_len + " ,")
                print(indent + "\t" + "},")
            print(indent + "},")

def print_ipv4_address_elem(dict, elem, field, indent):
    if elem in dict.keys():
        elem_value = dict[elem]
        addr = socket.inet_pton(socket.AF_INET, elem_value)
        print(indent + "." + field + " = " +
              "{ { { " + ",".join(str(n) for n in addr) + " } } },")

def print_byte_array_elem(dict, elem, field, indent):
    if elem in dict.keys():
        elem_value = dict[elem]
        print(indent + "." + field + " = " +
              "{ " + ",".join(str(ord(n)) for n in elem_value) + " },")

def print_prefixes(dict, elem, field, indent):
    if elem in dict.keys():
        elem_value = dict[elem]
        prefix_count = len(elem_value)
        if prefix_count > 0:
            print(indent + "." + field + " = {")
            for i in range(prefix_count):
                addr = socket.inet_pton(socket.AF_INET6, elem_value[i]['address'])
                print(indent + "\t" + "[" + str(i) + "]" + " = { ")
                print(indent + "\t\t" + ".address = { { { " + ",".join(hex(n) for n in addr) + " } } },")
                print(indent + "\t\t" + ".len = " + str(elem_value[i]['len']) + " ,")
                print(indent + "\t\t" + ".lifetime = " + str(elem_value[i]['lifetime']) + " ,")
                print(indent + "\t" + "},")
            print(indent + "},")

def print_dhcpv6_elems(dict, elem, name, indent):
    if elem in dict.keys():
        elem_value = dict[elem]
        if elem_value['status'] == "enabled":
            print("#if defined(CONFIG_NET_DHCPV6)")
            print(indent + "."+ name + " = {")
            print_enabled_elem(elem_value, 'status', 'is_enabled', "\t" + indent)
            print_bool_elem(elem_value, 'do-request-address', 'do_request_addr', "\t" + indent)
            print_bool_elem(elem_value, 'do-request-prefix', 'do_request_prefix', "\t" + indent)
            print(indent + "},")
            print("#endif /* CONFIG_NET_DHCPV6 */")

def print_dhcpv4_server_elems(dict, elem, name, indent):
    global dhcpv4_server_count
    if elem in dict.keys():
        elem_value = dict[elem]
        if elem_value['status'] == "enabled":
            print("#if defined(CONFIG_NET_DHCPV4_SERVER)")
            print(indent + "."+ name + " = {")
            print_enabled_elem(elem_value, 'status', 'is_enabled', "\t" + indent)
            print_ipv4_address_elem(elem_value, 'base-address', "base_addr", "\t" + indent)
            print(indent + "},")
            print("#endif /* CONFIG_NET_DHCPV4_SERVER */")
            if elem_value['status'] == "enabled":
                dhcpv4_server_count += 1

def print_ip_elems(dict, elem, name, family, indent):
    if elem in dict.keys():
        elem_value = dict[elem]
        if elem_value['status'] == "enabled":
            if family == socket.AF_INET:
                print("#if defined(CONFIG_NET_IPV4)")
            else:
                print("#if defined(CONFIG_NET_IPV6)")
            print(indent + "."+ name + " = {")
            print_enabled_elem(elem_value, 'status', 'is_enabled', "\t" + indent)
            print_address_elem(elem_value, 'multicast-addresses', 'mcast_address', family,
                               "\t" + indent)
            if elem == 'IPv6':
                print_ipv6_address_elem(elem_value, 'addresses', 'address', family, "\t" + indent)
                print_prefixes(elem_value, 'prefixes', 'prefix', "\t" + indent)
                print_int_elem(elem_value, 'hop-limit', 'hop_limit', "\t" + indent)
                print_int_elem(elem_value, 'multicast-hop-limit', 'mcast_hop_limit',
                               "\t" + indent)
                print_dhcpv6_elems(elem_value, 'DHCPv6', 'dhcpv6', "\t" + indent)
            if elem == 'IPv4':
                print_ipv4_address_and_netmask_elem(elem_value, 'addresses', 'address', family, "\t" + indent)
                print_enabled_elem(elem_value, 'DHCPv4', 'is_dhcpv4', "\t" + indent)
                print_ipv4_address_elem(elem_value, 'gateway', 'gateway', "\t" + indent)
                print_int_elem(elem_value, 'time-to-live', 'ttl', "\t" + indent)
                print_int_elem(elem_value, 'multicast-time-to-live', 'mcast_ttl', "\t" + indent)
                print_dhcpv4_server_elems(elem_value, 'DHCPv4-server', 'dhcpv4_server', "\t" + indent)
            print(indent + "},")
            if family == socket.AF_INET:
                print("#endif /* CONFIG_NET_IPV4 */")
            else:
                print("#endif /* CONFIG_NET_IPV6 */")

def print_vlan_elems(dict, elem, name, indent):
    if elem in dict.keys():
        elem_value = dict[elem]
        if elem_value['status'] == "enabled":
            print("#if defined(CONFIG_NET_VLAN)")
            print(indent + "."+ name + " = {")
            print_enabled_elem(elem_value, 'status', 'is_enabled', "\t" + indent)
            print_int_elem(elem_value, 'tag', 'tag', "\t" + indent)
            print(indent + "},")
            print("#endif /* CONFIG_NET_VLAN */")

def print_flags(dict, elem, indent):
    if elem in dict.keys():
        elem_value = dict[elem]
        print(indent + ".flags = ")
        for _, value in enumerate(elem_value):
            if value == "no-auto-start":
                print(indent + "\t" + "NET_IF_NO_AUTO_START |")
        print(indent + "\t" + "0,")

# Note that all the bind-to fields are added +1 so that we can catch
# the case where the value is not set (0). When taken into use in C code,
# then -1 is added to the value.
def print_bind_to(dict, elem, field, indent):
    if elem in dict.keys():
        elem_value = dict[elem]
        for i in range(netif_count):
            if elem_value == netif[i]:
                if field != "":
                    print(indent + "." + field + " = " + str(i + 1) + ",")
                else:
                    print(indent + str(i + 1) + ",")

def print_ieee802154_security_elems(dict, name, indent):
    print("#if defined(CONFIG_NET_L2_IEEE802154_SECURITY)")
    print(indent + "."+ name + " = {")
    print_byte_array_elem(dict, 'security-key', 'key', "\t" + indent)
    print_int_elem(dict, 'security-key-mode', 'key_mode', "\t" + indent)
    print_int_elem(dict, 'security-level', 'level', "\t" + indent)
    print(indent + "},")
    print("#endif /* CONFIG_NET_L2_IEEE802154_SECURITY */")

def print_ieee802154_elems(dict, elem, name, indent):
    if elem in dict.keys():
        elem_value = dict[elem]
        print("#if defined(CONFIG_NET_L2_IEEE802154)")
        print(indent + "."+ name + " = {")
        print_hex_elem(elem_value, 'pan-id', 'pan_id', "\t" + indent)
        print_int_elem(elem_value, 'channel', 'channel', "\t" + indent)
        print_int_elem(elem_value, 'tx-power', 'tx_power', "\t" + indent)
        print_bind_to(elem_value, 'bind-to', 'bind_to', "\t" + indent)
        print_ieee802154_security_elems(elem_value, 'sec_params', "\t" + indent)
        print(indent + "},")
        print("#endif /* CONFIG_NET_L2_IEEE802154 */")
        print(indent + "." + "is_ieee802154" + " = " + str(True).lower() + ",")
    else:
        print(indent + "." + "is_ieee802154" + " = " + str(False).lower() + ",")

def print_sntp_elems(dict, elem, name, indent):
    if elem in dict.keys():
        elem_value = dict[elem]
        print("#if defined(CONFIG_SNTP)")
        print(indent + "."+ name + " = {")
        print_string_elem(elem_value, 'server', 'server', "\t" + indent)
        print_int_elem(elem_value, 'timeout', 'timeout', "\t" + indent)
        print_bind_to(elem_value, 'bind-to', 'bind_to', "\t" + indent)
        print(indent + "},")
        print("#endif /* CONFIG_SNTP */")
        print(indent + "." + "is_sntp" + " = " + str(True).lower() + ",")
    else:
        print(indent + "." + "is_sntp" + " = " + str(False).lower() + ",")

print("#if defined(NET_CONFIG_ENABLE_DATA)")
print("static const struct net_init_config net_init_config_data = {")
print("\t.iface = {")
for i in range(netif_count):
    dict = netif[i]
    print("\t\t[" + str(i) + "] = {")
    print_string_elem(dict, 'name', 'name', "\t\t\t")
    print_string_elem(dict, 'device', 'dev', "\t\t\t")
    print_string_elem(dict, 'set-name', 'new_name', "\t\t\t")
    print_bool_elem(dict, 'set-default', 'is_default', "\t\t\t")
    print_enabled_elem(dict, 'IPv4-autoconf', 'is_ipv4_autoconf', "\t\t\t")
    print_flags(dict, 'flags', "\t\t\t")
    print_ip_elems(dict, 'IPv6', 'ipv6', socket.AF_INET6, "\t\t\t")
    print_ip_elems(dict, 'IPv4', 'ipv4', socket.AF_INET, "\t\t\t")
    print_vlan_elems(dict, 'VLAN', 'vlan', "\t\t\t")
    print_bind_to(dict, 'bind-to', 'bind_to', "\t\t\t")
    print("\t\t},")
print("\t},")

print_ieee802154_elems(net, 'IEEE-802.15.4', 'ieee802154', "\t")
print_sntp_elems(net, 'SNTP', 'sntp', "\t")
print("};")
print("#endif /* NET_CONFIG_ENABLE_DATA */")
print()
print("#ifndef ZEPHYR_INCLUDE_NET_CONFIG_AUTO_GENERATED_H_")
print("#define ZEPHYR_INCLUDE_NET_CONFIG_AUTO_GENERATED_H_")
print("#define NET_CONFIG_IFACE_COUNT " + str(netif_count))
print("#define NET_CONFIG_DHCPV4_SERVER_INSTANCE_COUNT " + str(dhcpv4_server_count))
print("#endif")
