#!/usr/bin/env python3
#
# Copyright (c) 2017 Intel Corporation
#

import argparse
import devicetree
import pprint
import sys


def find_phandle(root, label):
    def recurse(root, label):
        if not root or not 'label' in root:
            return False

        if label == root['label']:
            return '0x%x' % root['props']['phandle']

        for k, v in root['children'].items():
            phandle = recurse(v, label)
            if phandle:
                return phandle

    return recurse(root, label)


def is_number(v):
    if isinstance(v, int):
        return True
    if isinstance(v, str):
        return v.isdigit() or v.startswith('0x')
    if isinstance(v, dict):
        return 'ref' in v
    return False


def to_dts_value(root, value):
    if isinstance(value, list):
        formatted = (to_dts_value(root, item) for item in value)
        if all(is_number(v) for v in value):
            return '<%s>' % ' '.join(formatted)

        return ', '.join(formatted)

    if isinstance(value, int):
        return '0x%x' % value

    if isinstance(value, dict) and 'ref' in value:
        return find_phandle(root, value['ref'])

    return '"%s"' % value


def dump_tree(root, tree, output, indent=0):
    indent_str = '    ' * indent

    for k, v in tree.items():
        if not v or not 'label' in v:
            continue

        prop_indent = '    ' * (indent + 1)

        output.write(indent_str)

        if v['label'] and v['label'] != k:
            output.write('%s: %s {\n' % (v['label'], k))
        else:
            output.write('%s {\n' % k)

        for prop, value in v['props'].items():
            if isinstance(value, dict) and value.get('empty', False):
                output.write('%s%s;\n' % (prop_indent, prop))
            else:
                value = to_dts_value(root, value)
                output.write('%s%s = %s;\n' % (prop_indent, prop, value))

        if v['children']:
            dump_tree(root, v['children'], output, indent + 1)

        output.write('%s};\n\n' % indent_str)


def fixup_tree(tree):
    state = { 'phandle': 1 }

    def fixup_tree_item(root, label, value, state):
        if not root:
            return
        if not 'children' or not 'label' in root:
            return

        if label[0] == '&':
            label = label[1:]

        if root['label'] == label:
            root['children'] = devicetree.update_node(root['children'],
                                                      value['children'])
            root['props'] = devicetree.update_node(root['props'],
                                                   value['props'])
            return

        root['props']['phandle'] = state['phandle']
        state['phandle'] += 1

        for k, v in root['children'].items():
            fixup_tree_item(v, label, value, state)


    output = {'/': tree['/']}

    del tree['/']

    for k, v in tree.items():
        fixup_tree_item(output['/'], k, v, state)

    return output


def resolve_refs(tree):
    def build_path_traverse(root, ref):
        if not root or not 'label' in root:
            return False

        if ref == root['label']:
            return True

        for k, v in root['children'].items():
            path = build_path_traverse(v, ref)
            if path:
                if path == True:
                    return [k]

                return [k, path]


    def flatten(lst):
        for item in lst:
            if isinstance(item, str):
                yield item
            else:
                yield from flatten(item)


    def build_path(root, ref):
        built = build_path_traverse(root, ref)
        if not built:
            return '&%s' % ref

        return '/' + '/'.join(flatten(built))


    def resolve_refs_items(root, branch):
        if not branch:
            return

        for k, v in branch.get('props', {}).items():
            if isinstance(v, dict) and 'ref' in v:
                branch['props'][k] = build_path(root, v['ref'])
        for k, v in branch.get('children', {}).items():
            if isinstance(v, dict):
                resolve_refs_items(root, v)

    resolve_refs_items(tree['/'], tree['/'])

    return tree


def flatten(input, output, boot_cpu, include_path):
    output.write('/dts-v1/;\n')

    tree = devicetree.parse_file(input, include_path=include_path)

    tree = fixup_tree(tree)

    tree = resolve_refs(tree)

    dump_tree(tree['/'], tree, output)


def main():
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument('-b', '--boot-cpu',
                        required=True, help="Boot CPU number")
    parser.add_argument('-I', '--include-path',
                        required=True, action='append',
                        help="Add directory to include path")
    parser.add_argument('-i', '--input-dts',
                        required=True, help="Input DTS path")
    parser.add_argument('-o', '--output-dts',
                        required=True, help="Output DTS path")

    args = parser.parse_args()

    with open(args.input_dts, 'r') as input_file:
        with open(args.output_dts, 'w') as output_file:
            return flatten(input_file, output_file,
                           args.boot_cpu, args.include_path)

if __name__ == '__main__':
    sys.exit(main())
