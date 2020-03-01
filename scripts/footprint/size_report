#!/usr/bin/env python3
#
# Copyright (c) 2016, Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

# Based on a script by:
#       Chereau, Fabien <fabien.chereau@intel.com>

import os
import re
import argparse
import subprocess
import json
import operator
import platform
from pathlib import Path


# Return a dict containing {
#   symbol_name: {:,path/to/file}/symbol
# }
# for all symbols from the .elf file. Optionaly strips the path according
# to the passed sub-path
def load_symbols_and_paths(bin_nm, elf_file, path_to_strip=""):
    nm_out = subprocess.check_output(
        [bin_nm, elf_file, "-S", "-l", "--size-sort", "--radix=d"],
        universal_newlines=True
    )
    for line in nm_out.splitlines():
        if not line:
            # Get rid of trailing empty field
            continue

        symbol, path = parse_symbol_path_pair(line)

        if path:
            p_path          = Path(path)
            p_path_to_strip = Path(path_to_strip)
            try:
                processed_path = p_path.relative_to(p_path_to_strip)
            except ValueError:
                # path is valid, but is not prefixed by path_to_strip
                processed_path = p_path
        else:
            processed_path = Path(":")

        pathlike_string = processed_path / symbol

        yield symbol, pathlike_string

# Return a pair containing either
#
#   (symbol_name, "path/to/file")
#   or
#   (symbol_name, "")
#   or
#   ("", "")
#
#   depending on whether the symbol name and the file are found or not
# }
def parse_symbol_path_pair(line):
    # Line's output from nm might look like this:
    # '536871152 00000012 b gpio_e	/absolute/path/gpio.c:247'
    #
    # We are only trying to extract the symbol and the filename.
    #
    # In general lines look something like this:
    #
    # 'number number string[\t<symbol>][\t<absolute_path>:line]
    #
    # The symbol and file is optional, nm might not find out what a
    # symbol is named or where it came from.
    #
    # NB: <absolute_path> looks different on Windows and Linux

    # Replace tabs with spaces to easily split up the fields (NB:
    # Whitespace in paths is not supported)
    line_without_tabs = line.replace('\t', ' ')

    fields = line_without_tabs.split()

    assert len(fields) >= 3

    # When a symbol has been stripped, it's symbol name does not show
    # in the 'nm' output, but it is still listed as something that
    # takes up space. We use the empty string to denote these stripped
    # symbols.
    symbol_is_missing = len(fields) < 4
    if symbol_is_missing:
        symbol = ""
    else:
        symbol = fields[3]


    file_is_missing = len(fields) < 5
    if file_is_missing:
        path = ""
    else:
        path_with_line_number = fields[4]

        # Remove the trailing line number, e.g. 'C:\file.c:237'
        line_number_index = path_with_line_number.rfind(':')
        path = path_with_line_number[:line_number_index]

    return (symbol, path)


def get_section_size(f, section_name):
    decimal_size = 0
    re_res = re.search(r"(.*] " + section_name + ".*)", f, re.MULTILINE)
    if re_res is not None:
        # Replace multiple spaces with one space
        # Skip first characters to avoid having 1 extra random space
        res = ' '.join(re_res.group(1).split())[5:]
        decimal_size = int(res.split()[4], 16)
    return decimal_size


def get_footprint_from_bin_and_statfile(
        bin_file, stat_file, total_flash, total_ram):
    """Compute flash and RAM memory footprint from a .bin and .stat file"""
    f = open(stat_file).read()

    # Get kctext + text + ctors + rodata + kcrodata segment size
    total_used_flash = os.path.getsize(bin_file)

    # getting used ram on target
    total_used_ram = (get_section_size(f, "noinit") +
                      get_section_size(f, "bss") +
                      get_section_size(f, "initlevel") +
                      get_section_size(f, "datas") +
                      get_section_size(f, ".data") +
                      get_section_size(f, ".heap") +
                      get_section_size(f, ".stack") +
                      get_section_size(f, ".bss") +
                      get_section_size(f, ".panic_section"))

    total_percent_ram = 0
    total_percent_flash = 0
    if total_ram > 0:
        total_percent_ram = float(total_used_ram) / total_ram * 100
    if total_flash > 0:
        total_percent_flash = float(total_used_flash) / total_flash * 100

    res = {"total_flash": total_used_flash,
           "percent_flash": total_percent_flash,
           "total_ram": total_used_ram,
           "percent_ram": total_percent_ram}
    return res


def generate_target_memory_section(
        bin_objdump, bin_nm, out, kernel_name, source_dir, features_json):
    try:
        json.loads(open(features_json, 'r').read())
    except BaseException:
        pass

    bin_file_abs = os.path.join(out, kernel_name + '.bin')
    elf_file_abs = os.path.join(out, kernel_name + '.elf')

    # First deal with size on flash. These are the symbols flagged as LOAD in
    # objdump output
    size_out = subprocess.check_output(
        [bin_objdump, "-hw", elf_file_abs],
        universal_newlines=True
    )
    loaded_section_total = 0
    loaded_section_names = []
    loaded_section_names_sizes = {}
    ram_section_total = 0
    ram_section_names = []
    ram_section_names_sizes = {}
    for line in size_out.splitlines():
        if "LOAD" in line:
            loaded_section_total = loaded_section_total + \
                int(line.split()[2], 16)
            loaded_section_names.append(line.split()[1])
            loaded_section_names_sizes[line.split()[1]] = int(
                line.split()[2], 16)
        if "ALLOC" in line and "READONLY" not in line and "rodata" not in line and "CODE" not in line:
            ram_section_total = ram_section_total + int(line.split()[2], 16)
            ram_section_names.append(line.split()[1])
            ram_section_names_sizes[line.split()[1]] = int(line.split()[2], 16)

    # Actual .bin size, which doesn't not always match section sizes
    bin_size = os.stat(bin_file_abs).st_size

    # Get the path associated to each symbol
    symbols_paths = dict(load_symbols_and_paths(bin_nm, elf_file_abs, source_dir))

    # A set of helper function for building a simple tree with a path-like
    # hierarchy.
    def _insert_one_elem(tree, path, size):
        cur = None
        for p in path.parts:
            if cur is None:
                cur = p
            else:
                cur = cur + os.path.sep + p
            if cur in tree:
                tree[cur] += size
            else:
                tree[cur] = size

    def _parent_for_node(e):
        parent = "root" if len(os.path.sep) == 1 else e.rsplit(os.path.sep, 1)[0]
        if e == "root":
            parent = None
        return parent

    def _childs_for_node(tree, node):
        res = []
        for e in tree:
            if _parent_for_node(e) == node:
                res += [e]
        return res

    def _siblings_for_node(tree, node):
        return _childs_for_node(tree, _parent_for_node(node))

    def _max_sibling_size(tree, node):
        siblings = _siblings_for_node(tree, node)
        return max([tree[e] for e in siblings])

    # Extract the list of symbols a second time but this time using the objdump tool
    # which provides more info as nm

    symbols_out = subprocess.check_output(
        [bin_objdump, "-tw", elf_file_abs],
        universal_newlines=True
    )
    flash_symbols_total = 0
    data_nodes = {}
    data_nodes['root'] = 0

    ram_symbols_total = 0
    ram_nodes = {}
    ram_nodes['root'] = 0
    for l in symbols_out.splitlines():
        line = l[0:9] + "......." + l[16:]
        fields = line.replace('\t', ' ').split(' ')
        # Get rid of trailing empty field
        if len(fields) != 5:
            continue
        size = int(fields[3], 16)
        if fields[2] in loaded_section_names and size != 0:
            flash_symbols_total += size
            _insert_one_elem(data_nodes, symbols_paths[fields[4]], size)
        if fields[2] in ram_section_names and size != 0:
            ram_symbols_total += size
            _insert_one_elem(ram_nodes, symbols_paths[fields[4]], size)

    def _init_features_list_results(features_list):
        for feature in features_list:
            _init_feature_results(feature)

    def _init_feature_results(feature):
        feature["size"] = 0
        # recursive through children
        for child in feature["children"]:
            _init_feature_results(child)

    def _check_all_symbols(symbols_struct, features_list):
        out = ""
        sorted_nodes = sorted(symbols_struct.items(),
                              key=operator.itemgetter(0))
        named_symbol_filter = re.compile(r'.*\.[a-zA-Z]+/.*')
        out_symbols_filter = re.compile('^:/')
        for symbpath in sorted_nodes:
            matched = 0
            # The files and folders (not matching regex) are discarded
            # like: folder folder/file.ext
            is_symbol = named_symbol_filter.match(symbpath[0])
            is_generated = out_symbols_filter.match(symbpath[0])
            if is_symbol is None and is_generated is None:
                continue
            # The symbols inside a file are kept: folder/file.ext/symbol
            # and unrecognized paths too (":/")
            for feature in features_list:
                matched = matched + \
                    _does_symbol_matches_feature(
                        symbpath[0], symbpath[1], feature)
            if matched == 0:
                out += "UNCATEGORIZED: %s %d<br/>" % (symbpath[0], symbpath[1])
        return out

    def _does_symbol_matches_feature(symbol, size, feature):
        matched = 0
        # check each include-filter in feature
        for inc_path in feature["folders"]:
            # filter out if the include-filter is not in the symbol string
            if inc_path not in symbol:
                continue
            # if the symbol match the include-filter, check against
            # exclude-filter
            is_excluded = 0
            for exc_path in feature["excludes"]:
                if exc_path in symbol:
                    is_excluded = 1
                    break
            if is_excluded == 0:
                matched = 1
                feature["size"] = feature["size"] + size
                # it can only be matched once per feature (add size once)
                break
        # check children independently of this feature's result
        for child in feature["children"]:
            child_matched = _does_symbol_matches_feature(symbol, size, child)
            matched = matched + child_matched
        return matched

    # Create a simplified tree keeping only the most important contributors
    # This is used for the pie diagram summary
    min_parent_size = bin_size / 25
    min_sibling_size = bin_size / 35
    tmp = {}
    for e in data_nodes:
        if _parent_for_node(e) is None:
            continue
        if data_nodes[_parent_for_node(e)] < min_parent_size:
            continue
        if _max_sibling_size(data_nodes, e) < min_sibling_size:
            continue
        tmp[e] = data_nodes[e]

    # Keep only final nodes
    tmp2 = {}
    for e in tmp:
        if not _childs_for_node(tmp, e):
            tmp2[e] = tmp[e]

    # Group nodes too small in an "other" section
    filtered_data_nodes = {}
    for e in tmp2:
        if tmp[e] < min_sibling_size:
            k = _parent_for_node(e) + "/(other)"
            if k in filtered_data_nodes:
                filtered_data_nodes[k] += tmp[e]
            else:
                filtered_data_nodes[k] = tmp[e]
        else:
            filtered_data_nodes[e] = tmp[e]

    def _parent_level_3_at_most(node):
        e = _parent_for_node(node)
        while e.count('/') > 2:
            e = _parent_for_node(e)
        return e

    return ram_nodes, data_nodes


def print_tree(data, total, depth):
    base = os.environ['ZEPHYR_BASE']
    totp = 0

    bcolors_ansi = {
        "HEADER"    : '\033[95m',
        "OKBLUE"    : '\033[94m',
        "OKGREEN"   : '\033[92m',
        "WARNING"   : '\033[93m',
        "FAIL"      : '\033[91m',
        "ENDC"      : '\033[0m',
        "BOLD"      : '\033[1m',
        "UNDERLINE" : '\033[4m'
    }
    if platform.system() == "Windows":
        # Set all color codes to empty string on Windows
        #
        # TODO: Use an approach like the pip package 'colorama' to
        # support colors on Windows
        bcolors = dict.fromkeys(bcolors_ansi, '')
    else:
        bcolors = bcolors_ansi

    print('{:92s} {:10s} {:8s}'.format(
        bcolors["FAIL"] + "Path", "Size", "%" + bcolors["ENDC"]))
    print('=' * 110)
    for i in sorted(data):
        p = i.split(os.path.sep)
        if depth and len(p) > depth:
            continue

        percent = 100 * float(data[i]) / float(total)
        percent_c = percent
        if len(p) < 2:
            totp += percent

        if len(p) > 1:
            if not os.path.exists(os.path.join(base, i)):
                s = bcolors["WARNING"] + p[-1] + bcolors["ENDC"]
            else:
                s = bcolors["OKBLUE"] + p[-1] + bcolors["ENDC"]
            print('{:80s} {:20d} {:8.2f}%'.format(
                "  " * (len(p) - 1) + s, data[i], percent_c))
        else:
            print('{:80s} {:20d} {:8.2f}%'.format(
                bcolors["OKBLUE"] + i + bcolors["ENDC"], data[i], percent_c))

    print('=' * 110)
    print('{:92d}'.format(total))
    return totp


def main():

    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument("-d", "--depth", dest="depth", type=int,
                      help="How deep should we go into the tree", metavar="DEPTH")
    parser.add_argument("-o", "--outdir", dest="outdir", required=True,
                      help="read files from directory OUT", metavar="OUT")
    parser.add_argument("-k", "--kernel-name", dest="binary", default="zephyr",
                      help="kernel binary name")
    parser.add_argument("-r", "--ram",
                      action="store_true", dest="ram", default=False,
                      help="print RAM statistics")
    parser.add_argument("-F", "--rom",
                      action="store_true", dest="rom", default=False,
                      help="print ROM statistics")
    parser.add_argument("-s", "--objdump", dest="bin_objdump", required=True,
                      help="Path to the GNU binary utility objdump")
    parser.add_argument("-c", "--objcopy", dest="bin_objcopy",
                      help="Path to the GNU binary utility objcopy")
    parser.add_argument("-n", "--nm", dest="bin_nm", required=True,
                      help="Path to the GNU binary utility nm")

    args = parser.parse_args()

    bin_file = os.path.join(args.outdir, args.binary + ".bin")
    stat_file = os.path.join(args.outdir, args.binary + ".stat")
    elf_file = os.path.join(args.outdir, args.binary + ".elf")

    if not os.path.exists(elf_file):
        print("%s does not exist." % (elf_file))
        return

    if not os.path.exists(bin_file):
        FNULL = open(os.devnull, 'w')
        subprocess.call([args.bin_objcopy,"-S", "-Obinary", "-R", ".comment", "-R",
                       "COMMON", "-R", ".eh_frame", elf_file, bin_file],
                       stdout=FNULL, stderr=subprocess.STDOUT)

    fp = get_footprint_from_bin_and_statfile(bin_file, stat_file, 0, 0)
    base = os.environ['ZEPHYR_BASE']
    ram, data = generate_target_memory_section(
        args.bin_objdump, args.bin_nm, args.outdir, args.binary,
        base + '/', None)
    if args.rom:
        print_tree(data, fp['total_flash'], args.depth)
    if args.ram:
        print_tree(ram, fp['total_ram'], args.depth)


if __name__ == "__main__":
    main()
