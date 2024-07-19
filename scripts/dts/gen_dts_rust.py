#!/usr/bin/env python3

import re
import os
import sys
import pickle
import argparse

sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'python-devicetree',
                                'src'))


def parse_args():
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument("--rust-out", required=True,
                        help="path to write the Rust device tree file")
    parser.add_argument("--edt-pickle", required=True,
                        help="path to read the pickled edtlib.EDT object from")
    return parser.parse_args()


def main():
    args = parse_args()

    with open(args.edt_pickle, 'rb') as f:
        edt = pickle.load(f)

    with open(args.rust_out, 'w') as f:
        f.write(''.join(generate_dts_rust(edt)))


def generate_dts_rust(edt):
    for node in edt.nodes:
        yield from generate_node_struct(node)

    for node in edt.nodes:
        yield from generate_node_instance(node)


def generate_node_struct(node):
    yield f'struct DtNode{node.dep_ordinal} {{\n'

    for child_name, child_node in node.children.items():
        yield f'    {replace_chars(child_name)}: &DtNode{child_node.dep_ordinal},\n'

    for property_name, property_object in node.props.items():
        rust_type = None

        if property_object.spec.type == 'boolean':
            rust_type = 'bool'
        if property_object.spec.type == 'int':
            rust_type = 'u32'
        if property_object.spec.type == 'string':
            rust_type = 'str'
        if property_object.spec.type == 'array':
            rust_type = '[u32]'
        if property_object.spec.type == 'string-array':
            rust_type = '[str]'
        if property_object.spec.type == 'phandle' or property_object.spec.type == 'path':
            rust_type = f'&DtNode{property_object.val.dep_ordinal}'
        if property_object.spec.type == 'phandle-array':
            pass

        if rust_type:
            yield f'    {replace_chars(property_name)}: {rust_type},\n'
        else:
            yield f'    // {replace_chars(property_name)} ({property_object.spec.type})\n'


    yield '}\n\n'


def generate_node_instance(node):
    for label in node.labels:
        yield f'// label("{label}")\n'
    for alias in node.aliases:
        yield f'// alias("{alias}")\n'
    if node.matching_compat:
        yield f'// matching_compat("{node.matching_compat}")\n'

    yield f'const DT_NODE_{node.dep_ordinal}: DtNode{node.dep_ordinal} = DtNode{node.dep_ordinal} {{\n'

    for child_name, child_node in node.children.items():
        yield f'    {replace_chars(child_name)}: DT_NODE_{child_node.dep_ordinal},\n'

    for property_name, property_object in node.props.items():
        if property_object.spec.type == 'boolean':
            yield f'    {replace_chars(property_name)}: {"true" if property_object.val else "false"},\n'
        elif property_object.spec.type == 'int':
            yield f'    {replace_chars(property_name)}: {property_object.val},\n'
        elif property_object.spec.type == 'string':
            yield f'    {replace_chars(property_name)}: "{property_object.val}",\n'
        elif property_object.spec.type == 'array':
            yield f'    {replace_chars(property_name)}: ['
            yield ', '.join(str(val) for val in property_object.val)
            yield '],\n'
        elif property_object.spec.type == 'string-array':
            yield f'    {replace_chars(property_name)}: ['
            yield ', '.join(f'"{val}"' for val in property_object.val)
            yield '],\n'
        elif property_object.spec.type == 'phandle' or property_object.spec.type == 'path':
            yield f'    {replace_chars(property_name)}: "DT_NODE_{property_object.val.dep_ordinal}",\n'
        else:
            yield f'    // {replace_chars(property_name)} ({property_object.spec.type})\n'

    yield '};\n\n'


def replace_chars(text: str):
    return re.sub(r'\W+', '_', text)


if __name__ == "__main__":
    main()
