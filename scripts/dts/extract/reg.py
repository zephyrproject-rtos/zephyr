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
    # @param yaml YAML definition for the owning node.
    # @param prop reg property name
    # @param names (unused)
    # @param def_label Define label string of node owning the
    #                  compatible definition.
    #
    def extract(self, node_address, yaml, prop, names, def_label):

        node = reduced[node_address]
        node_compat = get_compat(node_address)

        reg = reduced[node_address]['props']['reg']
        if type(reg) is not list: reg = [ reg ]
        props = list(reg)

        address_cells = reduced['/']['props'].get('#address-cells')
        size_cells = reduced['/']['props'].get('#size-cells')
        address = ''
        for comp in node_address.split('/')[1:-1]:
            address += '/' + comp
            address_cells = reduced[address]['props'].get(
                '#address-cells', address_cells)
            size_cells = reduced[address]['props'].get('#size-cells', size_cells)

        post_label = "BASE_ADDRESS"
        if yaml[node_compat].get('use-property-label', False):
            label = node['props'].get('label', None)
            if label:
                post_label = label

        index = 0
        l_base = def_label.split('/')
        l_addr = [convert_string_to_label(post_label)]
        l_size = ["SIZE"]

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

            for x in range(address_cells):
                addr += props.pop(0) << (32 * x)
            for x in range(size_cells):
                size += props.pop(0) << (32 * x)

            l_addr_fqn = '_'.join(l_base + l_addr + l_idx)
            l_size_fqn = '_'.join(l_base + l_size + l_idx)
            if address_cells:
                prop_def[l_addr_fqn] = hex(addr)
            if size_cells:
                prop_def[l_size_fqn] = int(size)
            if len(name):
                if address_cells:
                    prop_alias['_'.join(l_base + name + l_addr)] = l_addr_fqn
                if size_cells:
                    prop_alias['_'.join(l_base + name + l_size)] = l_size_fqn

            # generate defs for node aliases
            if node_address in aliases:
                for i in aliases[node_address]:
                    alias_label = convert_string_to_label(i)
                    alias_addr = [alias_label] + l_addr
                    alias_size = [alias_label] + l_size
                    prop_alias['_'.join(alias_addr)] = '_'.join(l_base + l_addr)
                    prop_alias['_'.join(alias_size)] = '_'.join(l_base + l_size)

            insert_defs(node_address, prop_def, prop_alias)

            # increment index for definition creation
            index += 1

##
# @brief Management information for registers.
reg = DTReg()
