#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from extract.globals import *
from extract.directive import DTDirective

##
# @brief Manage reg directive.
#
class DTReg(DTDirective):

    def __init__(self):
        pass

    ##
    # @brief Extract reg directive info
    #
    # @param node_address Address of node owning the
    #                     reg definition.
    # @param names (unused)
    # @param def_label Define label string of node owning the
    #                  compatible definition.
    #
    def extract(self, node_address, names, def_label, div):

        node = reduced[node_address]
        node_compat = get_compat(node_address)
        binding = get_binding(node_address)

        reg = reduced[node_address]['props']['reg']
        if type(reg) is not list: reg = [ reg, ]

        (nr_address_cells, nr_size_cells) = get_addr_size_cells(node_address)

        if 'parent' in binding:
            bus = binding['parent']['bus']
            if bus == 'spi':
                cs_gpios = None

                try:
                    cs_gpios = deepcopy(find_parent_prop(node_address, 'cs-gpios'))
                except:
                    pass

                if cs_gpios:
                    extract_controller(node_address, "cs-gpios", cs_gpios, reg[0], def_label, "cs-gpio", True)
                    extract_cells(node_address, "cs-gpios", cs_gpios, None, reg[0], def_label, "cs-gpio", True)

        # generate defines
        l_base = def_label.split('/')
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
            if index == 0 and len(props) < 3:
                # 1 element (len 2) or no element (len 0) in props
                l_idx = []
            else:
                l_idx = [str(index)]

            try:
                name = [names.pop(0).upper()]
            except:
                name = []

            for x in range(nr_address_cells):
                addr += props.pop(0) << (32 * (nr_address_cells - x - 1))
            for x in range(nr_size_cells):
                size += props.pop(0) << (32 * (nr_size_cells - x - 1))

            addr += translate_addr(addr, node_address,
                    nr_address_cells, nr_size_cells)

            l_addr_fqn = '_'.join(l_base + l_addr + l_idx)
            l_size_fqn = '_'.join(l_base + l_size + l_idx)
            if nr_address_cells:
                prop_def[l_addr_fqn] = hex(addr)
                add_compat_alias(node_address, '_'.join(l_addr + l_idx), l_addr_fqn, prop_alias)
            if nr_size_cells:
                prop_def[l_size_fqn] = int(size / div)
                add_compat_alias(node_address, '_'.join(l_size + l_idx), l_size_fqn, prop_alias)
            if len(name):
                if nr_address_cells:
                    prop_alias['_'.join(l_base + name + l_addr)] = l_addr_fqn
                    add_compat_alias(node_address, '_'.join(name + l_addr), l_addr_fqn, prop_alias)
                if nr_size_cells:
                    prop_alias['_'.join(l_base + name + l_size)] = l_size_fqn
                    add_compat_alias(node_address, '_'.join(name + l_size), l_size_fqn, prop_alias)

            # generate defs for node aliases
            if node_address in aliases:
                add_prop_aliases(
                    node_address,
                    lambda alias:
                        '_'.join([str_to_label(alias)] + l_addr + l_idx),
                    l_addr_fqn,
                    prop_alias)
                if nr_size_cells:
                    add_prop_aliases(
                        node_address,
                        lambda alias:
                            '_'.join([str_to_label(alias)] + l_size + l_idx),
                        l_size_fqn,
                        prop_alias)

            insert_defs(node_address, prop_def, prop_alias)

            # increment index for definition creation
            index += 1

##
# @brief Management information for registers.
reg = DTReg()
