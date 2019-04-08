#
# Copyright (c) 2018 Bobby Noelte
# Copyright (c) 2018 Song Qiang <songqiang1304521@gmail.com>
#
# Derived from clocks.py
#
# SPDX-License-Identifier: Apache-2.0
#

from extract.globals import *
from extract.directive import DTDirective

##
# @brief Manage dma related directives.
#
# Handles:
# - dma
# directives.
#
class DTDMA(DTDirective):
    def _extract_consumer(self, node_path, dma, def_label):
        dma_consumer_bindings = get_binding(node_path)
        dma_consumer_label = 'DT_' + node_label(node_path)

        dma_index = 0
        dma_cell_index = 0
        nr_dma_cells = 0
        dma_provider_node_path = ''
        dma_provider = {}
        for cell in dma:
            if dma_cell_index == 0:
                if cell not in phandles:
                    raise Exception(
                        ("Could not find the dma provider node {} for dma"
                         " = {} in dma consumer node {}. Did you activate"
                         " the dma node?. Last dma provider: {}.")
                            .format(str(cell), str(dma), node_path,
                                    str(dma_provider)))
                dma_provider_node_path = phandles[cell]
                dma_provider = reduced[dma_provider_node_path]
                dma_provider_bindings = get_binding(
                                            dma_provider_node_path)
                nr_dma_cells = int(dma_provider['props'].get(
                                     '#dma-cells', 0))
                dma_cells_string = dma_provider_bindings.get(
                    'cell_string', 'DMA')
                dma_cells_names = dma_provider_bindings.get(
                    '#cells', ['ID', 'CELL1',  "CELL2", "CELL3"])
                dma_cells = []
            else:
                dma_cells.append(cell)
            dma_cell_index += 1
            if dma_cell_index > nr_dma_cells:
                # dma consumer device - dma info
                #####################################
                prop_def = {}
                prop_alias = {}

                # Legacy dma definitions by extract_cells
                for i, cell in enumerate(dma_cells):
                    if i >= len(dma_cells_names):
                        dma_cell_name = 'CELL{}'.format(i)
                    else:
                        dma_cell_name = dma_cells_names[i]
                    if dma_cells_string == dma_cell_name:
                        dma_label = self.get_label_string([
                            dma_consumer_label, dma_cells_string,
                            str(dma_index)])
                        add_compat_alias(node_path,
                                self.get_label_string(["",
                                    dma_cells_string, str(dma_index)]),
                                dma_label, prop_alias)
                    else:
                        dma_label = self.get_label_string([
                            dma_consumer_label, dma_cells_string,
                            dma_cell_name, str(dma_index)])
                        add_compat_alias(node_path,
                                self.get_label_string(["",
                                    dma_cells_string, dma_cell_name,
                                    str(dma_index)]),
                                dma_label, prop_alias)
                    prop_def[dma_label] = str(cell)
                    if dma_index == 0 and \
                        len(dma) == (len(dma_cells) + 1):
                        index = ''
                    else:
                        index = str(dma_index)
                    if node_path in aliases:
                        if dma_cells_string == dma_cell_name:
                            add_prop_aliases(
                                node_path,
                                lambda alias:
                                    self.get_label_string([
                                        alias,
                                        dma_cells_string,
                                        index]),
                                dma_label,
                                prop_alias)
                        else:
                            add_prop_aliases(
                                node_path,
                                lambda alias:
                                    self.get_label_string([
                                        alias,
                                        dma_cells_string,
                                        dma_cell_name,
                                        index]),
                                dma_label,
                                prop_alias)
                    # alias
                    if i < nr_dma_cells:
                        # dma info for first dma
                        dma_alias_label = self.get_label_string([
                            dma_consumer_label, dma_cells_string,
                            dma_cell_name])
                        prop_alias[dma_alias_label] = dma_label
                        add_compat_alias(node_path,
                                self.get_label_string(["",
                                    dma_cells_string, dma_cell_name]),
                                dma_label, prop_alias)


                # Legacy dma definitions by extract_controller
                dma_provider_label_str = dma_provider['props'].get('label',
                                                                       None)
                if dma_provider_label_str is not None:
                    try:
                        generation = dma_consumer_bindings['properties'][
                            'dma']['generation']
                    except:
                        generation = ''
                    dma_cell_name = 'DMA_CONTROLLER'
                    if dma_index == 0 and \
                        len(dma) == (len(dma_cells) + 1):
                        index = ''
                    else:
                        index = str(dma_index)
                    dma_label = self.get_label_string([dma_consumer_label,
                                                         dma_cell_name,
                                                         index])
                    add_compat_alias(node_path,
                            self.get_label_string(["", dma_cell_name, index]),
                            dma_label, prop_alias)
                    prop_def[dma_label] = '"' + dma_provider_label_str + '"'
                    if node_path in aliases:
                        add_prop_aliases(
                            node_path,
                            lambda alias:
                                self.get_label_string([
                                    alias,
                                    dma_cell_name,
                                    index]),
                            dma_label,
                            prop_alias)

                insert_defs(node_path, prop_def, prop_alias)

                dma_cell_index = 0
                dma_index += 1

    ##
    # @brief Extract dma related directives
    #
    # @param node_path Path to node owning the dmaxxx definition.
    # @param prop dmaxxx property name
    # @param def_label Define label string of node owning the directive.
    #
    def extract(self, node_path, prop, def_label):

        properties = reduced[node_path]['props'][prop]

        prop_list = []
        if not isinstance(properties, list):
            prop_list.append(properties)
        else:
            prop_list = list(properties)

        if prop == 'dma':
            # indicator for dma consumers
            self._extract_consumer(node_path, prop_list, def_label)
        else:
            raise Exception(
                "DTDMA.extract called with unexpected directive ({})."
                    .format(prop))

##
# @brief Management information for dma.
dma = DTDMA()
