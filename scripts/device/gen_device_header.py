#!/usr/bin/env python3
#
# Copyright (c) 2022 Trackunit A/S
#
# SPDX-License-Identifier: Apache-2.0

# Generates extern declarations and macros for for struct devices using
# properties pickle and the edt pickle.


import argparse
import os
import pickle
import sys


# Add absolute path to Python devicetree module to Python path
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(__file__)),
                                'dts', 'python-devicetree', 'src'))

# Import Python devicetree module
import devicetree.edtlib


def parse_args():
    parser = argparse.ArgumentParser()

    parser.add_argument("--properties-pickle", required=True,
                        help="Path to pickle file with properties")

    parser.add_argument("--edt-pickle", required=True,
                        help="Path to pickled edtlib.EDT devicetree object")

    parser.add_argument("--header-out", required=True,
                        help="Path to write generated header to")

    return parser.parse_args()


def load_edt(path: str) -> devicetree.edtlib.EDT:
    with open(path, 'rb') as fd:
        return pickle.load(fd)


def parse_edt(path: str) -> list:
    return list(filter(lambda n: 0 < len(n.compats) and n.status == "okay", load_edt(path).nodes))


def load_properties_pickle(path: str) -> dict:
    with open(path, 'rb') as fd:
        return pickle.load(fd)


def filter_properties_by_compats(properties: list, compats: list) -> list:
    associated_properties = []
    for binding in properties:
        if binding["compatible"] in compats:
            associated_properties.append(binding)
    return associated_properties


def combine_nodes_and_properties(nodes: list, properties: list) -> list:
    devices = []

    for node in nodes:
        devices.append({
            "node": node,
            "properties": filter_properties_by_compats(properties, node.compats)
        })

    return devices


def write_file_description(fd):
    fd.writelines([
        "/*\n",
        " * Generated from zephyr/scripts/build/gen_device_header.py\n",
        " *\n",
        " * Uses devicetree and properties files in\n",
        " * zephyr/driver/properties to generate macros, definitions\n",
        " * declarations included by zephyr/include/zephyr/device.h\n",
        " */\n",
        "\n",
    ])


def write_newline(fd):
    fd.write("\n")


def find_apis_in_properties(properties: list):
    associated_apis = []
    for binding in properties:
        if "api" not in binding:
            continue

        for api in binding["api"]:
            if api not in associated_apis:
                associated_apis.append(api)

    return associated_apis


def write_device_name(node: devicetree.edtlib.Node, fd):
    fd.write(f"/* Device name: {node.name} */\n")


def write_device_dt_supports_api_description(fd):
    fd.writelines([
        "/*\n",
        " * Macro used to test if an API is supported by a specific node.\n",
        " */\n",
        "\n",
    ])


def write_device_dt_supports_api(devices: list, fd):
    for device in devices:
        node = device["node"]
        properties = device["properties"]

        if not properties:
            continue

        apis = find_apis_in_properties(properties)

        write_device_name(node, fd)

        for api in apis:
            fd.write(f"#define DEVICE_DT_{node.z_path_id}_API_{api}_EXISTS 1\n")
        fd.write("\n")


def write_device_dt_foreach_api_description(fd):
    fd.writelines([
        "/*\n",
        " * Macros which invoke the specified fn macro for each node which\n",
        " * supports the specified API. The fn macro is passed the node\n",
        " * identifier as the first argument. Use VARGS variant to overload\n",
        " * the fn macro.\n",
        " */\n",
        "\n",
    ])


def write_device_dt_foreach_api_collect_nodes(devices: list) -> dict:
    nodes_collected = {}

    for device in devices:
        node = device["node"]
        properties = device["properties"]

        if not properties:
            continue

        apis = find_apis_in_properties(properties)

        for api in apis:
            if api in nodes_collected:
                nodes_collected[api].append(node)
            else:
                nodes_collected[api] = [node]

    return nodes_collected


def write_device_dt_foreach_api(devices: list, fd):
    nodes_collected = write_device_dt_foreach_api_collect_nodes(devices)

    for api in nodes_collected:
        fd.write(f"/* Device API: {api} */\n")
        fd.write(f"#define DEVICE_DT_API_{api}_FOREACH_EXISTS 1\n")
        fd.write(f"#define DEVICE_DT_API_{api}_FOREACH(fn) ")
        fd.write(" ".join(f"fn(DT_{node.z_path_id})" for node in nodes_collected[api]))
        fd.write("\n")
        fd.write(f"#define DEVICE_DT_API_{api}_FOREACH_VARGS(fn, ...) ")
        fd.write(" ".join(f"fn(DT_{node.z_path_id}, __VA_ARGS__)" for node in nodes_collected[api]))
        fd.write("\n\n")


def write_device_dt_properties_description(fd):
    fd.writelines([
        "/*\n",
        " * Macros which fetch the properties defined for a node in the\n",
        " * properties files.\n",
        " */\n",
        "\n",
    ])


def write_device_dt_properties(devices: list, fd):
    for device in devices:
        node = device["node"]

        if not device["properties"]:
            continue

        for properties in device["properties"]:
            if "properties" not in properties:
                continue

            write_device_name(node, fd)

            properties_dict = properties["properties"]

            for pd_key in properties_dict:
                pd_value = properties_dict[pd_key]
                pd_value = pd_value if isinstance(pd_value, int) else f"\"{pd_value}\""

                fd.write(f"#define DEVICE_DT_{node.z_path_id}_PROPERTY_{pd_key} {pd_value}\n")

            fd.write("\n")


def write_device_externs_description(fd):
    fd.writelines([
        "/*\n",
        " * API device structs need external visibility to be referenced\n",
        " * from outside the source files which define them.\n",
        " *\n",
        " * External declarations are generated from the properties files in\n",
        " * zephyr/properties. For each enabled node in the devicetree, one\n",
        " * API device struct is declared for each API declared with the\n",
        " * api: key in the properties file.\n",
        " *\n",
        " * The properties files are associated with devicetree nodes and\n",
        " * properties files using the compatible key\n",
        " */\n",
        "\n",
    ])


def write_device_externs(devices: list, fd):
    for device in devices:
        node = device["node"]
        properties = device["properties"]

        apis = find_apis_in_properties(properties)

        if not apis:
            continue

        write_device_name(node, fd)

        for api in apis:
            fd.write(f"extern const struct device __api_device_dts_ord_{node.dep_ordinal}_{api};\n")
        fd.write("\n")


def write_file_end(fd):
    fd.writelines([
        "/*\n",
        " * End of file\n",
        " */\n",
    ])


def main():
    # Load and parse dependencies
    args = parse_args()
    nodes = parse_edt(args.edt_pickle)
    properties = load_properties_pickle(args.properties_pickle)
    devices = combine_nodes_and_properties(nodes, properties)


    # Generate output header file
    with open(args.header_out, "w", encoding="utf-8") as fd:
        write_file_description(fd)
        write_newline(fd)

        write_device_dt_supports_api_description(fd)
        write_device_dt_supports_api(devices, fd)
        write_newline(fd)

        write_device_dt_foreach_api_description(fd)
        write_device_dt_foreach_api(devices, fd)
        write_newline(fd)

        write_device_dt_properties_description(fd)
        write_device_dt_properties(devices, fd)
        write_newline(fd)

        write_device_externs_description(fd)
        write_device_externs(devices, fd)
        write_newline(fd)

        write_file_end(fd)


if __name__ == "__main__":
    main()
