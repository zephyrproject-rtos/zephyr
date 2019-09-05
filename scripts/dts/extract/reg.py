#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

# NOTE: This file is part of the old device tree scripts, which will be removed
# later. They are kept to generate some legacy #defines via the
# --deprecated-only flag.
#
# The new scripts are gen_defines.py, edtlib.py, and dtlib.py.

from copy import deepcopy

from extract.globals import *
from extract.directive import DTDirective

##
# @brief Manage reg directive.
#
class DTReg(DTDirective):
    ##
    # @brief Extract reg directive info
    #
    # @param node_path Path to node owning the
    #                  reg definition.
    # @param names (unused)
    # @param def_label Define label string of node owning the
    #                  compatible definition.
    #
    def extract(self, node_path, names, def_label, div):
        binding = get_binding(node_path)

        reg = reduced[node_path]['props']['reg']
        if not isinstance(reg, list):
            reg = [reg]

        (nr_address_cells, nr_size_cells) = get_addr_size_cells(node_path)

        if 'parent' in binding:
            bus = binding['parent']['bus']
            if bus == 'spi':
                cs_gpios = None

                try:
                    cs_gpios = deepcopy(find_parent_prop(node_path, 'cs-gpios'))
                except Exception:
                    pass

                if cs_gpios:
                    extract_controller(node_path, "cs-gpios", cs_gpios, reg[0], def_label, "cs-gpio", True, True)
                    extract_controller(node_path, "cs-gpios", cs_gpios, reg[0], def_label, "cs-gpios", True)
                    extract_cells(node_path, "cs-gpios", cs_gpios, None, reg[0], def_label, "cs-gpio", True, True)
                    extract_cells(node_path, "cs-gpios", cs_gpios, None, reg[0], def_label, "cs-gpios", True)

        # generate defines
        l_base = [def_label]
        l_addr = [str_to_label("BASE_ADDRESS")]
        l_size = ["SIZE"]

        index = 0
        props = list(reg)

        while props:
            prop_def = {}
            prop_alias = {}
            addr = 0
            size = 0
            # Check is defined should be indexed (_0, _1)
            if index == 0 and len(props) <= (nr_address_cells + nr_size_cells):
                # 1 element (len 2) or no element (len 0) in props
                l_idx = []
            else:
                l_idx = [str(index)]

            try:
                name = [names.pop(0).upper()]
            except Exception:
                name = []

            for x in range(nr_address_cells):
                addr += props.pop(0) << (32 * (nr_address_cells - x - 1))
            for x in range(nr_size_cells):
                size += props.pop(0) << (32 * (nr_size_cells - x - 1))

            addr += translate_addr(addr, node_path,
                    nr_address_cells, nr_size_cells)

            l_addr_fqn = '_'.join(l_base + l_addr + l_idx)
            l_size_fqn = '_'.join(l_base + l_size + l_idx)
            if nr_address_cells:
                prop_def[l_addr_fqn] = hex(addr)
                add_compat_alias(node_path, '_'.join(l_addr + l_idx), l_addr_fqn, prop_alias)
            if nr_size_cells:
                prop_def[l_size_fqn] = int(size / div)
                add_compat_alias(node_path, '_'.join(l_size + l_idx), l_size_fqn, prop_alias)
            if name:
                if nr_address_cells:
                    prop_alias['_'.join(l_base + name + l_addr)] = l_addr_fqn
                    add_compat_alias(node_path, '_'.join(name + l_addr), l_addr_fqn, prop_alias)
                if nr_size_cells:
                    prop_alias['_'.join(l_base + name + l_size)] = l_size_fqn
                    add_compat_alias(node_path, '_'.join(name + l_size), l_size_fqn, prop_alias)

            # generate defs for node aliases
            if node_path in aliases:
                add_prop_aliases(
                    node_path,
                    lambda alias:
                        '_'.join([str_to_label(alias)] + l_addr + l_idx),
                    l_addr_fqn,
                    prop_alias)
                if nr_size_cells:
                    add_prop_aliases(
                        node_path,
                        lambda alias:
                            '_'.join([str_to_label(alias)] + l_size + l_idx),
                        l_size_fqn,
                        prop_alias)

            insert_defs(node_path, prop_def, prop_alias)

            # increment index for definition creation
            index += 1

##
# @brief Management information for registers.
reg = DTReg()
