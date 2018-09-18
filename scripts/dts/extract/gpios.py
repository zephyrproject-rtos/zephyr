#
# Copyright (c) 2018 Linaro Limited
#
# SPDX-License-Identifier: Apache-2.0
#

import pprint

from extract.globals import *
from extract.directive import DTDirective
from extract.edts import *

##
# @brief Manage gpio-x directive.
# insert <gpio-prop-name>-_controller
# insert gpio cells based on gpio-controller definitions
#
class DTGpios(DTDirective):

    def __init__(self):
        pass

    ##
    # @brief Extract gpio information.
    #
    # @param node_address Address of node owning the pinctrl definition.
    # @param yaml YAML definition for the owning node.
    # @param prop gpio key
    # @param def_label Define label string of client node owning the gpio
    #                  definition.
    #
    def _extract_controller(self, node_address, yaml, prop, prop_values, def_label, index):

        prop_def = {}
        prop_alias = {}

        # get controller node (referenced via phandle)
        cell_parent = phandles[prop_values[0]]

        for k in reduced[cell_parent]['props'].keys():
            if k[0] == '#' and '-cells' in k:
                num_cells = reduced[cell_parent]['props'].get(k)

        try:
           l_cell = reduced[cell_parent]['props'].get('label')
        except KeyError:
            l_cell = None

        if l_cell is not None:

            l_base = def_label.split('/')

            # Check is defined should be indexed (_0, _1)
            if index == 0 and len(prop_values) < (num_cells + 2):
                # 0 or 1 element in prop_values
                # ( ie len < num_cells + phandle + 1 )
                l_idx = []
            else:
                l_idx = [str(index)]

            # Check node generation requirements
            try:
                generation = yaml[get_compat(node_address)]['properties'][prop][
                    'generation']
            except:
                generation = ''

            if 'use-prop-name' in generation:
                l_cellname = convert_string_to_label(prop + '_' + 'controller')
            else:
                l_cellname = convert_string_to_label('gpio' + '_' +
                                                                'controller')

            edts_insert_device_property(edts_device_id(node_address),
                    '{}/{}/controller'.format(prop, index),
                    cell_parent)

            edts_insert_device_parent_device_property(node_address)

            label = l_base + [l_cellname] + l_idx

            prop_def['_'.join(label)] = "\"" + l_cell + "\""

            #generate defs also if node is referenced as an alias in dts
            if node_address in aliases:
                for i in aliases[node_address]:
                    alias_label = \
                        convert_string_to_label(i)
                    alias = [alias_label] + label[1:]
                    prop_alias['_'.join(alias)] = '_'.join(label)

            insert_defs(node_address, prop_def, prop_alias)

        # prop off phandle + num_cells to get to next list item
        prop_values = prop_values[num_cells+1:]

        return cell_parent


    def _extract_cells(self, node_address, yaml, prop, prop_values, def_label, index, controller):

        cell_parent = phandles[prop_values.pop(0)]

        try:
            cell_yaml = yaml[get_compat(cell_parent)]
        except:
            raise Exception(
                "Could not find yaml description for " +
                    reduced[cell_parent]['name'])


        # Get number of cells per element of current property
        for k in reduced[cell_parent]['props'].keys():
            if k[0] == '#' and '-cells' in k:
                num_cells = reduced[cell_parent]['props'].get(k)

        l_cell = [convert_string_to_label(str(prop))]


        # Check node generation requirements
        try:
            generation = yaml[get_compat(node_address)]['properties'][prop][
                'generation']
        except:
            generation = ''

        if 'use-prop-name' in generation:
            l_cell = [convert_string_to_label(str(prop))]
        else:
            l_cell = [convert_string_to_label('gpio')]

        l_base = def_label.split('/')
        # Check if #define should be indexed (_0, _1, ...)
        if index == 0 and len(prop_values) < (num_cells + 2):
            # Less than 2 elements in prop_values
            # (ie len < num_cells + phandle + 1)
            # Indexing is not needed
            l_idx = []
        else:
            l_idx = [str(index)]

        prop_def = {}
        prop_alias = {}

        # Generate label for each field of the property element
        for i in range(num_cells):
            cell_name = cell_yaml['#cells'][i]
            l_cellname = [str(cell_name).upper()]
            if l_cell == l_cellname:
                label = l_base + l_cell + l_idx
            else:
                label = l_base + l_cell + l_cellname + l_idx
            # label_name = l_base + name + l_cellname
            label_name = l_base + l_cellname
            cell_value = prop_values.pop(0)
            prop_def['_'.join(label)] = cell_value
            # if len(name):
            #     prop_alias['_'.join(label_name)] = '_'.join(label)

            # generate defs for node aliases
            if node_address in aliases:
                for i in aliases[node_address]:
                    alias_label = convert_string_to_label(i)
                    alias = [alias_label] + label[1:]
                    prop_alias['_'.join(alias)] = '_'.join(label)

            edts_insert_device_property(edts_device_id(node_address),
                    '{}/{}/{}'.format(prop, index, cell_name), cell_value)

            if 'pin' in cell_name:
                edts_insert_device_controller(controller, node_address,
                                              cell_value)

            insert_defs(node_address, prop_def, prop_alias)

        return prop_values


    ##
    # @brief Extract gpio related directives
    #
    # @param node_address Address of node owning the gpio definition.
    # @param yaml YAML definition for the owning node.
    # @param prop gpio property name
    # @param def_label Define label string of node owning the directive.
    #
    def extract(self, node_address, yaml, prop, def_label):

        try:
            prop_values = list(reduced[node_address]['props'].get(prop))
        except:
            prop_values = reduced[node_address]['props'].get(prop)

        # Newer versions of dtc might have the property look like
        # cs-gpios = <0x05 0x0d 0x00>, < 0x06 0x00 0x00>;
        # So we need to flatten the list in that case
        if isinstance(prop_values[0], list):
            prop_values = [item for sublist in prop_values for item in sublist]

        if 'gpio' in prop:
            # indicator for clock consumers
            index = 0
            while len(prop_values) > 0:
                controller = self._extract_controller(node_address, yaml, prop, prop_values,
                                         def_label, index)
                prop_values = self._extract_cells(node_address, yaml, prop, prop_values,
                                    def_label, index, controller)
                index += 1
        else:
            raise Exception(
                "DTGpios.extract called with unexpected directive ({})."
                    .format(prop))

##
# @brief Management information for gpio
gpios = DTGpios()
