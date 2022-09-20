#!/usr/bin/env python3
#
# Copyright (c) 2022 Trackunit A/S
#
# SPDX-License-Identifier: Apache-2.0


# This script searches for, parses and validates properties files to generate a
# list of dictionaries containing these properties.
# The list is formatted using the binary pickle format and written to a file
# which is used by downstream scripts.


import argparse
import yaml
import os
import pickle
import copy


# Top level keys which are allowed are placed within this list and the immutable
# reserved_keys list. An error will be raised if an unallowed key is found within
# any properties YAML file.
allowed_keys = [
    "description",
    "api",
    "properties",
]


# Allowed keys are not automatically included in this script's generated output.
# Key values must be processed using key value processors, which parse, validate
# and merge key values into a single value from one to multiple raw properties
# files, to be included in the output.
#
# A key value processor takes the following form:
#
#     def process_<key>_key_value(properties_file_chain: list):
#
# The key value processor is passed a list of properties files which have been
# chained together using the reserved "include" key.
#
# A properties file is a dictionary which contains the raw key values found in
# the source YAML file. The keys "filename" and "filepath", which describe the
# source YAML file, are added to the properties files before they are passed to
# the key value processor. These keys can be used when creating error messages.
#
# An example properties file dictionary is presented here:
#
#     properties_file = {
#         "key1": "value1",
#         "key2": 2,
#         "key3": [value3, value4],
#         "filename": "dummy,uart.yaml",
#         "filepath": "/home/dummy/git/zephyr_project/zephyr/drivers/properties/uart/dummy,uart.yaml"
#     }
#
# An example include chain is presented here:
#
#     device-base.yaml <-+- includes <- dummy,sensor-base.yaml <- includes <- dummy,sensor.yaml
#     sensor-base.yaml <-+
#
# The following properties file chain will be created from the example include chain:
#
#     properties_file_chain = [
#         {"filename": "device-base.yaml", ...},
#         {"filename": "sensor-base.yaml", ...},
#         {"filename": "dummy,sensor-base", ...},
#         {"filename": "dummy,sensor.yaml", ...}
#     ]
#
# The key value processor must use the properties file chain to create the single key value
# which will be created for the properties file at the end of the chain:
#
#     {"filename": "dummy,sensor.yaml", ...}.
#
# The key value processor must be added to the key_processors dictionary to be used when
# processing the properties file chains.
#
#     key_processors = {
#         process_<key1>_key_value,
#         process_<key2>_key_value,
#     }
#
# A key value processor may be used solely for validating key values. In this case, the key
# value processor must return nothing or None.


def process_description_key_value(properties_file_chain: list):
    for pfc in properties_file_chain:
        if "description" not in pfc:
            continue

        path = pfc["filepath"]
        desc = pfc["description"]

        if not isinstance(desc, str):
            raise AssertionError(f"malformed property 'description: {desc}' in '{path}' - "
                "must be a string")


def process_api_key_value(properties_file_chain: list):
    collected_apis = []

    for pfc in properties_file_chain:
        if "api" not in pfc:
            continue

        path = pfc["filepath"]
        api = pfc["api"]

        apis = api if isinstance(api, list) else [api]

        if not all((isinstance(val, str) and val.find(" ") == -1) for val in apis):
            raise AssertionError(f"malformed property 'api: {api}' in '{path}' - "
                "must be string or list of strings without whitespace")

        if len(apis) != len(set(apis)):
            raise AssertionError(f"malformed property 'api: {api}' in '{path}' - "
                "must not contain duplicate entries")

        for ca in collected_apis:
            if ca in apis:
                raise AssertionError(f"The api '{ca}' in '{path}' has already "
                    "been added within included files")

        collected_apis += apis

    if not collected_apis:
        path = properties_file_chain[-1]["filepath"]
        raise AssertionError(f"'{path}' is missing the required 'api' key")

    return collected_apis


def process_compatible_key_value(properties_file_chain: list):
    last_pf = properties_file_chain[-1]
    last_compat = last_pf["compatible"]
    last_path = last_pf["filepath"]
    included_pfs = properties_file_chain[:-1]

    if not isinstance(last_compat, str):
        raise AssertionError(f"malformed property 'compatible: {last_compat}' in '{last_path}'"
            " - must be string without whitespace")

    if -1 < last_compat.find(" "):
        raise AssertionError(f"malformed property 'compatible: {last_compat}' in '{last_path}'"
            " - must be string without whitespace")

    for pf in included_pfs:
        if "compatible" in pf:
            compat = pf["compatible"]
            path = pf["filepath"]

            raise AssertionError(f"'{last_path}' must not include '{path}' "
                f"which contains the property 'compatible: {compat}' - "
                "files which contain the property 'compatible' can not "
                "be included by other files")

    return last_compat


def process_properties_key_value_combine_properties_dicts(properties_file_chain: list) -> dict:
    merged_properties_dicts = {}

    for pfc in properties_file_chain:
        if "properties" not in pfc:
            continue

        path = pfc["filepath"]
        properties_dicts = pfc["properties"]

        if not isinstance(properties_dicts, dict):
            raise AssertionError(f"Malformed 'properties: {properties_dicts}' property in '{path}'"
                " - must be a dictionary")

        for pds_key in properties_dicts:
            pd = properties_dicts[pds_key]

            if not isinstance(pd, dict):
                raise AssertionError(f"Malformed 'properties: {pd}' property in '{path}' - "
                    "must contain dictionaries which may contain the keys 'type', 'required', "
                    "'default' and 'value'")

        for pds_key in properties_dicts:
            pd = properties_dicts[pds_key]
            ppd = merged_properties_dicts[pds_key] if pds_key in merged_properties_dicts else {}

            for pd_key in pd:
                pdv = pd[pd_key]

                if pd_key in ppd:
                    raise AssertionError(f"Malformed 'properties: {pds_key}:' property in '{path}' - "
                        f"The key value '{pd_key}: {pdv}' is overwriting previous definition")

                ppd[pd_key] = pdv

            merged_properties_dicts[pds_key] = ppd

    return merged_properties_dicts


def process_properties_key_value_validate_properties_dicts(path: str, properties_dicts: dict):
    for pd_key in properties_dicts:
        pd = properties_dicts[pd_key]

        if "type" not in pd:
            raise AssertionError(f"Malformed 'properties: {pd_key}:' in '{path}' - "
                "must contain the 'type:' property which must be assigned 'int' or 'string'")

        if pd["type"] != "int" and pd["type"] != "string":
            raise AssertionError(f"Malformed 'properties: {pd_key}: type:' in '{path}' - "
                "must be assigned 'int' or 'string'")

        if "required" in pd:
            if not isinstance(pd["required"], bool):
                raise AssertionError(f"Malformed 'properties: {pd_key}: required:' in '{path}' - "
                    "must be assigned 'true' or 'false'")
        else:
            pd["required"] = False

        if "value" not in pd and "default" not in pd and pd["required"]:
            raise AssertionError(f"Malformed 'properties: {pd_key}:' in '{path}' - "
                f"{pd_key} must be assigned a value through the 'value:' or 'default:'"
                "properties since it is marked as 'required'")

        if "value" not in pd and "default" in pd:
            pd["value"] = pd["default"]

        if "value" not in pd:
            continue

        if pd["type"] == "int" and not isinstance(pd["value"], int):
            raise AssertionError(f"Malformed 'properties: {pd_key}:' in '{path}' - "
                "must be an integer which is specified by the 'type:' property")

        if pd["type"] == "string" and not isinstance(pd["value"], str):
            raise AssertionError(f"Malformed 'properties: {pd_key}:' in '{path}' - "
                "must be a string which is specified by the 'type:' property")


def process_properties_key_value_merge_properties_dicts(properties_dicts: list) -> list:
    merged_properties = {}

    for pd_key in properties_dicts:
        pd = properties_dicts[pd_key]

        if "value" not in pd:
            continue

        merged_properties[pd_key] = pd["value"]

    return merged_properties


def process_properties_key_value(properties_file_chain: list):
    properties_dicts = process_properties_key_value_combine_properties_dicts(properties_file_chain)

    filepath = properties_file_chain[-1]["filepath"]
    process_properties_key_value_validate_properties_dicts(filepath, properties_dicts)

    return process_properties_key_value_merge_properties_dicts(properties_dicts)


key_processors = {
    "description": process_description_key_value,
    "api": process_api_key_value,
    "compatible": process_compatible_key_value,
    "properties": process_properties_key_value,
}


# The rest of the script concerns pre and post processing of the properties
# files.


reserved_keys = [
    "include",
    "compatible",
    "filename",
    "filepath"
]


def parse_args():
    parser = argparse.ArgumentParser()

    parser.add_argument("--properties-dirs", nargs="+", required=True,
                        help="Directories which may contain a properties "
                             "folder with device properties files in YAML "
                             "format")

    parser.add_argument("--properties-pickle-out", required=True,
                        help="Path to write generated bidnings pickle dict to")

    return parser.parse_args()


def list_dirs_in_dir(path: str) -> filter:
    return filter(lambda filename: os.path.isdir(os.path.join(path, filename)),
                  os.listdir(path))


def load_properties_file(path: str) -> dict:
    with open(path, "r") as fp:
        properties_file = yaml.safe_load(fp)

        if not properties_file:
            raise AssertionError(f"Properties file '{path}' is empty")

        return properties_file


def load_properties_files(paths: list) -> list:
    properties_files = []

    for path in paths:
        if "drivers" not in list_dirs_in_dir(path):
            continue

        if "properties" not in list_dirs_in_dir(os.path.join(path, "drivers")):
            continue

        for root, _, filenames in os.walk(os.path.join(path, "drivers", "properties")):
            for filename in filenames:
                filepath = os.path.join(root, filename)

                if not filename.endswith(".yaml"):
                    continue

                if os.path.isdir(filepath):
                    continue

                properties_file = load_properties_file(filepath)
                properties_file["filename"] = filename
                properties_file["filepath"] = filepath

                properties_files.append(properties_file)

    return properties_files


def validate_keys_in_properties_files(properties_files: list):
    valid_keys = reserved_keys + allowed_keys

    for pf in properties_files:
        filepath = pf["filepath"]

        for key in pf:
            if key not in valid_keys:
                raise AssertionError(f"Invalid key '{key}' in '{filepath}'")


def parse_include_property(properties_files: list):
    def include_exists(include: str, properties_files: list) -> bool:
        for pf in properties_files:
            if pf["filename"] == include:
                return True

        return False

    for pf in properties_files:
        if "include" not in pf:
            continue

        includes = pf["include"] if isinstance(pf["include"], list) else [pf["include"]]
        filepath = pf["filepath"]

        for i in includes:
            if not isinstance(i, str):
                raise AssertionError(f"malformed include property 'include: {includes}' "
                    f" in '{filepath}' - must be filename or list of filenames "
                    f"with .yaml extension")

        for i in includes:
            if 0 <= i.find("/"):
                raise AssertionError(f"malformed include property 'include: {includes}' "
                    f" in '{filepath}' - must be filename or list of filenames "
                    f"with .yaml extension")

        for i in includes:
            if i == pf["filename"]:
                raise AssertionError(f"malformed include property 'include: {includes}' "
                    f" in '{filepath}' - must not include self")

        for i in includes:
            if not include_exists(i, properties_files):
                raise Exception(f"File '{i}' included in '{filepath}' does not exist")

        pf["include"] = includes


def chain_properties_files_by_include_property(properties_files: list) -> list:
    def init(properties_files: list):
        chains = []
        remaining = []

        for pf in properties_files:
            if "include" not in pf:
                chains.append([pf])
            else:
                remaining.append([pf])

        return chains, remaining

    def iterate(chains, remaining):
        chains_new = copy.copy(chains)
        remaining_new = []

        for r in remaining:
            satisfied_dependencies = []

            for i in r[-1]["include"]:
                dependencies = []
                for c in chains:
                    if c[-1]["filename"] == i:
                        dependencies.append(c)

                if not dependencies:
                    continue

                satisfied_dependencies += dependencies

            if not satisfied_dependencies:
                remaining_new.append(r)
                continue

            for sd in satisfied_dependencies:
                r[-1]["include"].remove(sd[-1]["filename"])

            chain_new = []
            for sd in satisfied_dependencies:
                chain_new += sd
            chain_new += r

            chains_new.append(chain_new)

            if r[-1]["include"]:
                remaining_new.append(r)

        return chains_new, remaining_new

    def on_error():
        raise AssertionError("Circular dependency detected within properties files")

    def merge(chains: list) -> list:
        chains_merged = []
        work = chains
        remaining = []

        while work:
            end = work[0][-1]
            cm = work[0][:-1]

            for w in work[1:]:
                if w[-1] == end:
                    cm += w[:-1]
                else:
                    remaining.append(w)

            chains_merged.append(cm + [end])

            work = copy.copy(remaining)
            remaining.clear()

        return chains_merged

    # Split properties files into chains (no unmet
    # includes) and remaining (has unmet includes)
    chains, remaining = init(properties_files)

    iterations = 0
    prev_remaining_cnt = len(remaining)

    # Iterate over remaining, adding them to chains until
    # no properties files with unmet includes remain
    while remaining:
        chains, remaining = iterate(chains, remaining)

        remaining_cnt = len(remaining)
        iterations += 1

        # Catch circular dependencies and limit dependency depth
        if prev_remaining_cnt < remaining_cnt  or 10 < iterations:
            on_error()

        prev_remaining_cnt = remaining_cnt

    # Merge chains which end in the same properties file
    return merge(chains)


def filter_properties_file_chains(properties_file_chains: list) -> list:
    complete_properties_file_chains = []

    for chain in properties_file_chains:
        if "compatible" in chain[-1]:
            complete_properties_file_chains.append(chain)

    return complete_properties_file_chains


def process_properties_file_chains(properties_file_chains: list) -> list:
    properties_files = []

    for pfc in properties_file_chains:
        properties_file = {}

        for key in key_processors:
            value = key_processors[key](pfc)

            if not value:
                continue

            properties_file[key] = value

        properties_files.append(properties_file)

    return properties_files


def export_properties_to_pickle_file(path: str, properties: list):
    with open(path, "wb") as fp:
        pickle.dump(properties, fp)


def main():
    args = parse_args()

    properties_files = load_properties_files(args.properties_dirs)

    validate_keys_in_properties_files(properties_files)

    parse_include_property(properties_files)

    properties_file_chains = chain_properties_files_by_include_property(properties_files)

    properties_file_chains = filter_properties_file_chains(properties_file_chains)

    properties_files = process_properties_file_chains(properties_file_chains)

    export_properties_to_pickle_file(args.properties_pickle_out, properties_files)


if __name__ == "__main__":
    main()
