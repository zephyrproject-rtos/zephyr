#
# Copyright (c) 2019 Song Qiang <songqiang1304521@gmail.com>
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
    def insert_one(self, node_path, dma_cell_name, index, dma_label, prop_def):
        prop_alias = {}

        add_compat_alias(node_path,
                self.get_label_string(["", dma_cell_name, index]),
                dma_label, prop_alias)
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

    def _extract_consumer(self, node_path, dma, def_label):
        dma_consumer_bindings = get_binding(node_path)
        dma_consumer_label = 'DT_' + node_label(node_path)

        dma_index = 0
        dma_cell_index = 0
        dma_provider_node_path = ''
        dma_provider = {}
        dma_count = int(len(dma) / 3)

        if len(dma) % 3 != 0:
            raise Exception("DTDMA.extract: dma's number of elements must"
                            "be times of 3.")

        for i in range(dma_count):
            dma_cells = []

            dma_controller = dma.pop(0)
            if dma_controller not in phandles:
                raise Exception(
                    ("Could not find the dma provider node {} for dma"
                    " = {} in dma consumer node {}. Did you activate"
                    " the dma node?.")
                        .format(str(dma_controller), str(dma), node_path))
            dma_provider_node_path = phandles[dma_controller]
            dma_provider = reduced[dma_provider_node_path]
            dma_provider_bindings = get_binding(
                                        dma_provider_node_path)

            dma_cells.append(dma.pop(0))
            dma_cells.append(dma.pop(0))

            prop_def = {}
            prop_alias = {}

            dma_provider_label_str = dma_provider['props'].get('label',
                                                                    None)
            if dma_provider_label_str is not None:
                index = str(i)
                dma_cell_name = 'DMA_CONTROLLER'
                dma_label = self.get_label_string([dma_consumer_label,
                                                        dma_cell_name,
                                                        index])
                prop_def[dma_label] = '"' + dma_provider_label_str + '"'
                self.insert_one(node_path, dma_cell_name, index, dma_label, prop_def)

                dma_cell_name = "DMA_CHANNEL"
                dma_label = self.get_label_string([dma_consumer_label,
                                                        dma_cell_name,
                                                        index])
                prop_def[dma_label] = dma_cells.pop(0)
                self.insert_one(node_path, dma_cell_name, index, dma_label, prop_def)

                dma_cell_name = "DMA_SLOT"
                dma_label = self.get_label_string([dma_consumer_label,
                                                        dma_cell_name,
                                                        index])
                prop_def[dma_label] = dma_cells.pop(0)
                self.insert_one(node_path, dma_cell_name, index, dma_label, prop_def)

    ##
    # @brief Extract dma related directives
    #
    # @param node_path Path to node owning the dma definition.
    # @param prop dma property name
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
