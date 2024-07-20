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
        if node.path == '/chosen':
            yield from generate_chosen_node_struct(node)
        elif node.path == '/aliases':
            yield from generate_aliases_node_struct(node)
        else:
            yield from generate_normal_node_struct(node)

    for node in edt.nodes:
        if node.path == '/chosen':
            yield from generate_chosen_node_instance(node)
        elif node.path == '/aliases':
            yield from generate_aliases_node_instance(node)
        else:
            yield from generate_normal_node_instance(node)


def generate_chosen_node_struct(node):
    yield f'#[allow(dead_code)]\n'
    yield f'pub struct DtNode{node.dep_ordinal} {{\n'

    for chosen_name, chosen_node in node.edt.chosen_nodes.items():
        yield f'    pub {replace_chars(chosen_name)}: &\'static DtNode{chosen_node.dep_ordinal},\n'

    yield '}\n\n'


def generate_aliases_node_struct(node):
    yield f'#[allow(dead_code)]\n'
    yield f'pub struct DtNode{node.dep_ordinal} {{\n'

    alias2node = {}
    for node in node.edt.nodes:
        for name in node.aliases:
            alias2node[name] = node

    for alias_name, alias_node in alias2node.items():
        yield f'    pub {replace_chars(alias_name)}: &\'static DtNode{alias_node.dep_ordinal},\n'

    yield '}\n\n'


def generate_normal_node_struct(node):
    phandle_arrays = {}

    yield f'#[allow(dead_code)]\n'
    yield f'pub struct DtNode{node.dep_ordinal} {{\n'

    for child_name, child_node in node.children.items():
        yield f'    pub {replace_chars(child_name)}: &\'static DtNode{child_node.dep_ordinal},\n'

    for property_name, property_object in node.props.items():
        rust_type = None
        property_type = property_object.spec.type

        if property_name == 'status':
            continue
        if property_type == 'boolean':
            rust_type = 'bool'
        if property_type == 'int':
            rust_type = 'u32'
        if property_type == 'string':
            rust_type = '&\'static str'
        if property_type == 'uint8-array':
            rust_type = f'[u8; {len(property_object.val)}]'
        if property_type == 'array':
            rust_type = f'[u32; {len(property_object.val)}]'
        if property_type == 'string-array':
            rust_type = f'[&\'static str; {len(property_object.val)}]'
        if property_type in ('phandle', 'path'):
            rust_type = f'&\'static DtNode{property_object.val.dep_ordinal}'
        if property_type == 'phandles':
            rust_type = '(' + ', '.join(f'&\'static DtNode{n.dep_ordinal}' for n in property_object.val) + ',)'
        if property_type == 'phandle-array':
            pha_types = []
            for pha_entry in property_object.val:
                pha_type = f'DtNode{node.dep_ordinal}Pha{len(phandle_arrays)}'
                pha_types.append(pha_type)
                phandle_arrays[pha_type] = pha_entry
            rust_type = '(' + ', '.join('&\'static ' + pha_id for pha_id in pha_types) + ',)'

        if rust_type:
            yield f'    pub {replace_chars(property_name)}: {rust_type},\n'
        else:
            yield f'    // {replace_chars(property_name)} ({property_type})\n'

    yield '    pub status: &\'static str,\n'
    yield '}\n\n'

    for pha_type, pha_entry in phandle_arrays.items():
        yield f'#[allow(dead_code)]\n'
        yield f'pub struct {pha_type} {{\n'
        yield f'    pub phandle: &\'static DtNode{pha_entry.controller.dep_ordinal},\n'
        for cell_name in pha_entry.data.keys():
            yield f'    pub cell_{replace_chars(cell_name)}: u32,\n'
        yield '}\n\n'


def generate_chosen_node_instance(node):
    yield f'// path("{node.path}")\n'
    yield f'#[allow(dead_code)]\n'
    yield f'const DT_NODE_{node.dep_ordinal}: DtNode{node.dep_ordinal} = DtNode{node.dep_ordinal} {{\n'

    for chosen_name, chosen_node in node.edt.chosen_nodes.items():
        yield f'    {replace_chars(chosen_name)}: &DT_NODE_{chosen_node.dep_ordinal},\n'

    yield '};\n\n'


def generate_aliases_node_instance(node):
    yield f'// path("{node.path}")\n'
    yield f'#[allow(dead_code)]\n'
    yield f'const DT_NODE_{node.dep_ordinal}: DtNode{node.dep_ordinal} = DtNode{node.dep_ordinal} {{\n'

    alias2node = {}
    for node in node.edt.nodes:
        for name in node.aliases:
            alias2node[name] = node

    for alias_name, alias_node in alias2node.items():
        yield f'    {replace_chars(alias_name)}: &DT_NODE_{alias_node.dep_ordinal},\n'

    yield '};\n\n'


def generate_normal_node_instance(node):
    phandle_arrays = {}

    yield f'// path("{node.path}")\n'
    for label in node.labels:
        yield f'// nodelabel("{label}")\n'
    for alias in node.aliases:
        yield f'// alias("{alias}")\n'
    if node.matching_compat:
        yield f'// matching_compat("{node.matching_compat}")\n'

    yield f'#[allow(dead_code)]\n'
    yield f'const DT_NODE_{node.dep_ordinal}: DtNode{node.dep_ordinal} = DtNode{node.dep_ordinal} {{\n'

    for child_name, child_node in node.children.items():
        yield f'    {replace_chars(child_name)}: &DT_NODE_{child_node.dep_ordinal},\n'

    for property_name, property_object in node.props.items():
        property_type = property_object.spec.type
        rust_value = None

        if property_name == 'status':
            continue
        if property_type == 'boolean':
            rust_value = "true" if property_object.val else "false"
        elif property_type == 'int':
            rust_value = str(property_object.val)
        elif property_type == 'string':
            rust_value = f'"{property_object.val}"'
        elif property_type in ('uint8-array', 'array'):
            rust_value = '[' + ', '.join(str(val) for val in property_object.val) + ']'
        elif property_type == 'string-array':
            rust_value = '[' + ', '.join(f'"{val}"' for val in property_object.val) + ']'
        elif property_type in ('phandle', 'path'):
            rust_value = f'&DT_NODE_{property_object.val.dep_ordinal}'
        elif property_type == 'phandles':
            rust_value = '(' + ', '.join(f'&DT_NODE_{n.dep_ordinal}' for n in property_object.val) + ',)'
        elif property_type == 'phandle-array':
            pha_insts = []
            for pha_entry in property_object.val:
                pha_inst = f'DT_NODE_{node.dep_ordinal}_PHA_{len(phandle_arrays)}'
                pha_type = f'DtNode{node.dep_ordinal}Pha{len(phandle_arrays)}'
                pha_insts.append('&' + pha_inst)
                phandle_arrays[pha_inst] = (pha_type, pha_entry)
            rust_value = '(' + ', '.join(pha_insts) + ',)'

        if rust_value:
            yield f'    {replace_chars(property_name)}: {rust_value},\n'
        else:
            yield f'    // {replace_chars(property_name)} ({property_type})\n'

    yield f'    status: "{node.status}",\n'
    yield '};\n\n'

    for pha_inst, (pha_type, pha_entry) in phandle_arrays.items():
        if pha_entry.name:
            yield f'// {pha_entry.name}\n'
        yield f'#[allow(dead_code)]\n'
        yield f'const {pha_inst}: {pha_type} = {pha_type} {{\n'
        yield f'    phandle: &DT_NODE_{pha_entry.controller.dep_ordinal},\n'
        for cell_name, cell_value in pha_entry.data.items():
            yield f'    cell_{replace_chars(cell_name)}: {cell_value},\n'
        yield '};\n\n'

    if node.status == 'okay':
        yield f'#[allow(dead_code)]\n'
        yield f'impl DtNode{node.dep_ordinal} {{\n'
        yield '    pub fn device(&self) -> *const zephyr_sys::device {\n'
        yield f'        unsafe {{ &zephyr_sys::__device_dts_ord_{node.dep_ordinal} }}\n'
        yield '    }\n'
        yield '}\n\n'


def replace_chars(text: str):
    return re.sub(r'\W+', '_', text)


if __name__ == "__main__":
    main()
