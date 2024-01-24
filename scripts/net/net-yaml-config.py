#!/usr/bin/env python3
# Copyright(c) 2024 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

import yaml, sys, os, re

data = {}
schema = {}

def read_yaml_file(input_data):
    return yaml.safe_load(input_data)

# Allow both yaml file or .config file input
input_data = sys.stdin.read()

lines = input_data.splitlines()
for line in lines:
    if not line.strip(): # skip empty lines
        continue

    if line.startswith("#"):
        continue
    if line.startswith("CONFIG_"):
        # Construct string with two places to avoid triggering warning
        # from compliance checker.
        if not line.startswith("CONFIG_" + "NET_"):
            continue

        (var, val) = line.split("=", 1)
        if val == 'y':
            val = "true"

        os.environ[var] = val

        #print("env " + var + " = " + os.getenv(var, ""))
        continue
    else:
        data = read_yaml_file(input_data)
        break

# yaml env variable handler
def constructor_env_variables(loader, node):
    value = loader.construct_scalar(node)
    match = pattern.findall(value)  # to find all env variables in line
    dt = ''.join(type_tag_pattern.findall(value)) or ''
    value = value.replace(dt, '')
    if match:
        full_value = value
        for g in match:
            curr_default_value = 'None'
            env_var_name = g
            env_var_name_with_default = g
            if default_sep and isinstance(g, tuple) and len(g) > 1:
                env_var_name = g[0]
                env_var_name_with_default = ''.join(g)
                found = False
                for each in g:
                    if default_sep in each:
                        _, curr_default_value = each.split(default_sep, 1)
                        found = True
                        break
                if not found:
                    raise ValueError(
                        f'Could not find default value for {env_var_name}'
                    )
            full_value = full_value.replace(
                f'${{{env_var_name_with_default}}}',
                os.environ.get(env_var_name, curr_default_value).strip('"')
            )
            if dt:
                # do one more roundtrip with the dt constructor:
                node.value = full_value
                node.tag = dt.strip()
                return loader.yaml_constructors[node.tag](loader, node)
        return full_value
    return value

# Create a yaml file from a .config variables. Treat config options as environment
# variables and substitute the variables in yaml template.
def create_yaml_file():
    if "CONFIG_NET_CONFIG_MY_IPV4_NETMASK" in os.environ and os.environ["CONFIG_NET_CONFIG_MY_IPV4_NETMASK"] != "":
        masklen = sum(bin(int(x)).count('1') for x in os.environ["CONFIG_NET_CONFIG_MY_IPV4_NETMASK"].strip('"').split('.'))
        os.environ["IPV4_NETMASK_LEN"] = str(masklen)

    if "CONFIG_NET_CONFIG_SNTP_INIT_SERVER" in os.environ and os.environ["CONFIG_NET_CONFIG_SNTP_INIT_SERVER"] != "":
        os.environ["SNTP_ENABLED"] = "true"

    if "CONFIG_NET_L2_IEEE802154" in os.environ:
        if "CONFIG_NET_CONFIG_IEEE802154_SECURITY_KEY" in os.environ and os.environ["CONFIG_NET_CONFIG_IEEE802154_SECURITY_KEY"] != "":
            IEEE802154_SECURITY_KEY = str([int(ord(l[0])) for l in list(os.environ["CONFIG_NET_CONFIG_IEEE802154_SECURITY_KEY"].strip('"'))])
        else:
            IEEE802154_SECURITY_KEY = str([])

        ieee802154 = """
  ieee_802_15_4:
    status: !ENV tag:yaml.org,2002:bool ${CONFIG_NET_L2_IEEE802154:false}
    pan_id: !ENV tag:yaml.org,2002:int ${CONFIG_NET_CONFIG_IEEE802154_PAN_ID:0xabcd}
    channel: !ENV tag:yaml.org,2002:int ${CONFIG_NET_CONFIG_IEEE802154_CHANNEL:0}
    tx_power: !ENV tag:yaml.org,2002:int ${CONFIG_NET_CONFIG_IEEE802154_RADIO_TX_POWER:0}
    security_key: """ + IEEE802154_SECURITY_KEY + """
    security_key_mode: !ENV tag:yaml.org,2002:int ${CONFIG_NET_CONFIG_IEEE802154_SECURITY_KEY_MODE:0}
    security_level: !ENV tag:yaml.org,2002:int ${CONFIG_NET_CONFIG_IEEE802154_SECURITY_LEVEL:0}
    ack_required: !ENV tag:yaml.org,2002:bool ${CONFIG_NET_CONFIG_IEEE802154_ACK_REQUIRED:False}
  """
    else:
        ieee802154 = ""

    if "SNTP_ENABLED" in os.environ:
        sntp = """
  sntp:
    status: !ENV tag:yaml.org,2002:bool ${SNTP_ENABLED:false}
    server: !ENV ${CONFIG_NET_CONFIG_SNTP_INIT_SERVER:""}
    timeout: !ENV tag:yaml.org,2002:int ${CONFIG_NET_CONFIG_SNTP_INIT_TIMEOUT:0}
        """
    else:
        sntp = ""

    # There is only a limited number of options to set via .config file
    yaml_file = """
net_init_config:
  network_interfaces:
    - ipv6:
        status: !ENV tag:yaml.org,2002:bool ${CONFIG_NET_CONFIG_NEED_IPV6:false}
        ipv6_addresses:
          - !ENV ${CONFIG_NET_CONFIG_MY_IPV6_ADDR:""}
        dhcpv6:
          status: !ENV tag:yaml.org,2002:bool ${CONFIG_NET_DHCPV6:false}
          do_request_address: !ENV tag:yaml.org,2002:bool ${CONFIG_NET_CONFIG_DHCPV6_REQUEST_ADDR:true}
          do_request_prefix: !ENV tag:yaml.org,2002:bool ${CONFIG_NET_CONFIG_DHCPV6_REQUEST_PREFIX:false}
      ipv4:
        status: !ENV tag:yaml.org,2002:bool ${CONFIG_NET_CONFIG_NEED_IPV4:false}
        ipv4_addresses:
          - !ENV ${CONFIG_NET_CONFIG_MY_IPV4_ADDR:""}/${IPV4_NETMASK_LEN:32}
        gateway: !ENV ${CONFIG_NET_CONFIG_MY_IPV4_GW:""}
        dhcpv4_enabled: !ENV tag:yaml.org,2002:bool ${CONFIG_NET_DHCPV4:false}

  """ + ieee802154 + sntp

    return yaml_file

# This branch handles the variables read from .config, it generates a yaml
# file and then validates (if enabled) that it is correct.
if not bool(data):
    # Create a yaml using the env variables
    loader = yaml.SafeLoader
    tag = '!ENV'
    default_sep = ':'
    default_value = ''
    default_sep_pattern = r'(' + default_sep + '[^}]+)?'
    pattern = re.compile(
        r'.*?\$\{([^}{' + default_sep + r']+)' + default_sep_pattern + r'\}.*?')
    type_tag = 'tag:yaml.org,2002:'
    type_tag_pattern = re.compile(rf'({type_tag}\w+\s)')

    loader.add_implicit_resolver(tag, pattern, first=[tag])
    loader.add_constructor(tag, constructor_env_variables)

    data = read_yaml_file(create_yaml_file())

# If user has supplied an argument, treat it as a schema file
if bool(data) and len(sys.argv[1:]) > 0:
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

    yaml_validate(data, sys.argv[1])

with open(sys.argv[1], 'r') as schema_file:
    schema = yaml.safe_load(schema_file)

if not bool(schema):
    sys.stderr.write("Schema file needs to be supplied\n")
    exit(1)

netif = data['net_init_config']['network_interfaces']
netif_count = len(netif)

# Note that all the bind-to fields are added +1 so that we can catch
# the case where the value is not set (0). When taken into use in C code,
# then -1 is added to the value.
def get_bind_to(dict):
    for i in range(netif_count):
        if dict == netif[i]:
            return i + 1

def print_bind_to(dict, indent):
    print(indent + ".bind_to = " + str(get_bind_to(dict)) + ",")

def walk_dict(map, indent):
    for key, value in map.items():
        if isinstance(value, list):
            print(indent + "." + key + " = {")
            walk_list(value, "\t" + indent)
            print(indent + "},")
        elif isinstance(value, dict):
            if key == "bind_to":
                print_bind_to(value, indent)
            else:
                print(indent + "." + key + " = {")
                walk_dict(value, "\t" + indent)
                print(indent + "},")
        elif isinstance(value, str):
            print(indent + "." + key + " = \"" + value + "\",")
        elif isinstance(value, bool):
            print(indent + "." + key + " = " + str(value).lower() + ",")
        elif isinstance(value, int):
            print(indent + "." + key + " = " + str(value) + ",")

def walk_list(lst, indent):
    for i,v in enumerate(lst):
        print(indent + "[" + str(i) + "] = {")
        if isinstance(v, list):
            walk_list(v, "\t" + indent)
        elif isinstance(v, dict):
            walk_dict(v, "\t" + indent)
        elif isinstance(v, str):
            print(indent + "\t" + "\"" + v + "\",")
        elif isinstance(v, bool):
            print(indent + "\t" + str(v).lower() + ",")
        elif isinstance(v, int):
            print(indent + "\t" + str(v) + ",")
        print(indent + "},")

def walk_dict_union(cdict, prev_key, map):
    for key, value in map.items():
        if isinstance(value, list):
            walk_list_union(cdict, key, value)
        elif isinstance(value, dict):
            walk_dict_union(cdict, key, value)

def walk_list_union(cdict, key, lst):
    for _,v in enumerate(lst):
        if isinstance(v, list):
            walk_list_union(cdict, key, v)
        elif isinstance(v, dict):
            walk_dict_union(cdict, key, v)
        # Store the length of the array so that we can fetch that
        # when walking the schema file.
        if key in cdict:
            if cdict[key] < len(lst):
                cdict[key] = len(lst)
        else:
            cdict[key] = len(lst)

def walk_dict_schema(level, top_level_name, cdict, key_upper, map, indent):
    for key, value in map.items():
        if key == "type":
            if value == "str":
                print(indent + "const char *" + key_upper + ";")
            elif value == "bool":
                print(indent + "bool " + key_upper + ";")
            elif value == "int":
                print(indent + "int " + key_upper + ";")
            elif value == "seq":
                print(indent + "struct " + top_level_name + "_" + key_upper + " {")
            elif value == "map":
                if key_upper != "value":
                    if level == 1:
                        print(indent + "struct " + key_upper + " {")
                    else:
                        print(indent + "struct " + top_level_name + "_" + key_upper + " {")
            elif value == "any" and key_upper == "bind_to":
                print(indent + "int bind_to;")
            continue
        elif key == "mapping":
            walk_dict_schema(level + 1, top_level_name, cdict, key, value, "\t" + indent)
            if key_upper != "value":
                # Avoid creating a variable at the top level because we have
                # a separate variable created in the header file that is used in C file.
                if level == 1:
                    print(indent + "};")
                else:
                    print(indent + "} " + key_upper + ";")
            continue
        elif key == "sequence":
            walk_list_schema(level + 1, top_level_name, cdict, "value", value, "\t" + indent)
            if key_upper in combined_data:
                print(indent + "} " + key_upper + "[" + str(combined_data[key_upper]) + "];")
            else:
                print(indent + "} " + key_upper + "[1];")
            continue

        if isinstance(value, dict):
            walk_dict_schema(level + 1, top_level_name, cdict, key, value, indent)

def walk_list_schema(level, top_level_name, cdict, key, lst, indent):
    for _,v in enumerate(lst):
        if isinstance(v, list):
            walk_list_schema(level, top_level_name, cdict, key, v, indent)
        elif isinstance(v, dict):
            walk_dict_schema(level, top_level_name, cdict, key, v, indent)

combined_data = {}
schema_data = {}

print("#ifndef ZEPHYR_INCLUDE_NET_CONFIG_AUTO_GENERATED_H_")
print("#define ZEPHYR_INCLUDE_NET_CONFIG_AUTO_GENERATED_H_")

# Figure out the array sizes
for key, value in data.items():
    if isinstance(value, dict):
        walk_dict_union(combined_data, key, value)

# Create C struct definition. Prefix some of the generated C struct fields with
# by the name of the struct to make them unambiguous.
for key, value in schema.items():
    if isinstance(value, dict):
        walk_dict_schema(0, list(data.keys())[0], schema_data, key, value, "")

print()
print("const struct " + list(data.keys())[0] + "* net_config_get_init_config(void);")
print()
print("#define NET_CONFIG_NETWORK_INTERFACE_COUNT " + str(netif_count))
print("#endif /* ZEPHYR_INCLUDE_NET_CONFIG_AUTO_GENERATED_H_ */")
print()

# Create C struct values
for key, value in data.items():
    if isinstance(value, dict):
        print("#if defined(" + key.upper() + "_ENABLE_DATA)")
        print("#define " + key.upper() + "_DATA " + key + "_data")
        print("static const struct " + key + " " + key + "_data = {")
        walk_dict(value, "\t")
        print("};")
        print("#endif /* " + key.upper() + "_ENABLE_DATA */")
