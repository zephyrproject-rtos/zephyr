#!/usr/bin/env python3
#
# Copyright (c) 2022 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

import itertools
import argparse
import os

DDC_HEADER = "/* THIS FILE IS AUTO GENERATED. PLEASE DO NOT EDIT. */\n\n\
#ifndef GEN_DDC_TYPES_H\n\
#define GEN_DDC_TYPES_H\n\n\
#ifdef __cplusplus\n\
extern \"C\" {\n\
#endif\n\n\
"

DDC_FOOTER = "#ifdef __cplusplus\n\
}\n\n\
#endif\n\n\
#endif /* GEN_DDC_TYPES_H */\n"

ddc_prefix = "ddc"

#
# An attribute is a dict made of : type, variable name,
# and optional init template
#    { "type" : "",
#      "var"  : "",
#      "init" : "" },


# List of dicts of attributes to combine
ddc_attributes = [
]

def parse_args():
    global args

    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument("-o", "--output-header", required=True,
                        help="Output header file")

    args = parser.parse_args()


def ddc_struct_name(prefix, combination):
    struct_name = prefix
    for attribute in combination:
        struct_name += "_" + attribute["var"]

    return struct_name

def start_ifdef(prefix, combination):
    ifdef = "#if defined(" + prefix.upper()

    for attribute in combination:
        ifdef += "_" + attribute["var"].upper()

    ifdef += ")\n\n"

    return ifdef

def put_elif_def(prefix, combination):
    elseifdef = "#elif defined(" + prefix.upper()

    for attribute in combination:
        elseifdef += "_" + attribute["var"].upper()

    elseifdef += ")\n\n"

    return elseifdef

def end_ifdef():
    return "#endif\n\n"

def generate_structure(prefix, combination):
    structure = "struct " + ddc_struct_name(prefix, combination)
    structure += " {\n"

    for attribute in combination:
        structure += "\t" + attribute["type"] + " " + attribute["var"] + ";\n"

    structure += "};\n\n"

    return structure

def generate_ddc_macro(prefix, combination):
    return "#define " + prefix.upper() + " struct " + \
        ddc_struct_name(prefix, combination) + " ctx\n\n"

def generate_ddc_cfg_enum(prefix, combinations):
    cfg_enum = "enum " + prefix + "_cfg_types {\n"
    n = 0
    for c in combinations:
        cfg_enum += "\t" + prefix.upper() + "_CFG_TYPE"
        for attribute in c:
            cfg_enum += "_" + attribute["var"].upper()

        cfg_enum += " = " + str(n) + ",\n"
        n += 1

    cfg_enum += "\t" + prefix.upper() + "_CFG_TYPE_MAX = " + str(n) + "\n"

    cfg_enum += "};\n\n"

    return cfg_enum

def generate_ddc_cfg(prefix):
    return "struct " + prefix + "_config {\n\tenum " + prefix + \
        "_cfg_types type;\n};\n\n"

def generate_ddc_get(attribute, prefix, combinations):
    get = "inline " + attribute["type"] + " *" + prefix + "_get_" + \
        attribute["var"]
    get += "(const struct device *dev)\n{\n\tstruct " + prefix
    get += "_config *cfg = (struct " + prefix + "_config *)dev->config;\n\n"
    get += "\tif (cfg == NULL) {\n\t\treturn NULL;\n\t}\n\n"
    get += "\tswitch(cfg->type) {\n"

    for c in combinations:
        get += "\tcase " + prefix.upper() + "_CFG_TYPE"
        for attr in c:
            get += "_" + attr["var"].upper()
        get += ":\n"

        def presence(attr_name, combination):
            for attr in combination:
                if attr_name == attr["var"]:
                    return True
            return False

        if not presence(attribute["var"], c):
            get += "\t\tbreak;\n"
        else:
            get += "\t\treturn &((struct " + ddc_struct_name(prefix, c) + \
                " *)dev->data)->" + attribute["var"] + ";\n"

    get += "\tdefault:\n\t\tbreak;\n\t};\n\n\treturn NULL;\n}\n\n"

    return get

def generate_ddc_init_macro(prefix, combination):
    ddc_init = "#define " + prefix.upper() + "_INIT(_data) \\\n"
    ddc_init += "\t.ctx = { \\\n"
    for c in combination:
        if len(c["init"]) > 0:
            ddc_init += "\t\t" + c["init"] + " \\\n"

    ddc_init += "\t}\n\n"

    return ddc_init

def generate_ddc_cfg_init_macro(prefix, combination):
    cfg_init = "#define " + prefix.upper() + "_CFG_INIT() \\\n\t"
    cfg_init += ".ddc_cfg.type = " + prefix.upper() + "_CFG_TYPE"
    for attribute in combination:
        cfg_init += "_" + attribute["var"].upper()

    cfg_init += "\n\n"

    return cfg_init

def generate_ddc_get_macro(prefix, combination):
    get = ""
    for attribute in combination:
        get += "#define " + prefix.upper() + "_GET_" + \
            attribute["var"].upper() + "(dev) \\\n"
        get += "\t((" + ddc_struct_name(prefix, combination) + " *)dev->data)->"
        get += attribute["var"] + "\n\n"

    return get

def generate_attribute_combinations(attributes):
    list_combination = list()
    for n in range(len(attributes)):
        list_combination += list(list(itertools.combinations(attributes, n+1)))

    return list_combination

def generate_macros(prefix, combination):
    macros = generate_ddc_macro(prefix, combination)
    macros += generate_ddc_init_macro(prefix, combination)
    macros += generate_ddc_cfg_init_macro(prefix, combination)
    macros += generate_ddc_get_macro(prefix, combination)

    return macros

def generate_ddc_header_file(prefix, attributes, destination):
    with open(destination, "w") as dst:
        lines = DDC_HEADER

        combinations = generate_attribute_combinations(attributes)
        for c in combinations:
            lines += generate_structure(prefix, c)

        lines += generate_ddc_cfg_enum(prefix, combinations)
        lines += generate_ddc_cfg(prefix)

        for a in attributes:
            lines += generate_ddc_get(a, prefix, combinations)

        n = 0
        for c in combinations:
            if n == 0:
                lines += start_ifdef(prefix, c)
            else:
                lines += put_elif_def(prefix, c)

            lines += generate_macros(prefix, c)
            n += 1

        lines += end_ifdef()
        lines += DDC_FOOTER

        dst.write(lines)

if __name__ == "__main__":
    parse_args()

    generate_ddc_header_file(ddc_prefix, ddc_attributes, args.output_header)
