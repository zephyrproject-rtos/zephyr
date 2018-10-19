#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from .directive import DTDirective

##
# @brief Manage dma directives.
#
# Handles:
# - dma-channels
# - dma-requests
# - dmas
# - dma-names
#
# Generates in EDTS:
#
# DMA controller
# --------------
# - dma-channels : Number of DMA channels supported by the controller.
# - dma-requests : Number of DMA request signals supported by the controller
#
# DMA client
# ----------
# - TBD
#
class DTDMA(DTDirective):

    def __init__(self, extractors):
        super().__init__(extractors)
        # Ignore properties that are covered by the extraction of
        extractors.ignore_properties.extend([ \
            # - 'dmas' for dma clients
            'dma-names',
            # '- #dma-cells' for dma controllers
            'dma-channels', 'dma-requests'])

    ##
    # @brief Insert dma cells into EDTS
    #
    # @param dma_client
    # @param dma_controller
    # @param dma_cells
    # @param property_path_templ "xxx/yyy/{}"
    #
    def edts_insert_dma_cells(self, dma_client_node_address,
                               dma_controller_node_address, dma_cells,
                               property_path_templ):
        dts = self.dts()
        bindings = self.bindings()
        edts = self.edts()

        if len(dma_cells) == 0:
            return
        dma_client_device_id = self.edts_device_id(dma_client_node_address)
        dma_controller_device_id = self.edts_device_id(dma_controller_node_address)
        dma_controller = dts.reduced[dma_controller_node_address]
        dma_controller_compat = dts.get_compat(dma_controller_node_address)
        dma_controller_bindings = bindings[dma_controller_compat]
        dma_nr_cells = int(dma_controller['props'].get('#dma-cells', 0))
        dma_cells_names = dma_controller_bindings.get(
                    '#dma-cells', ['ID', 'CELL1',  "CELL2", "CELL3"])
        for i, cell in enumerate(dma_cells):
            if i >= len(dma_cells_names):
                dma_cell_name = 'CELL{}'.format(i).lower()
            else:
                dma_cell_name = dma_cells_names[i].lower()
            edts.insert_device_property(dma_client_device_id,
                property_path_templ.format(dma_cell_name), cell)

    def _extract_client(self, node_address, dmas):
        dts = self.dts()
        edts = self.edts()

        dma_client_device_id = self.edts_device_id(node_address)
        dma_client = dts.reduced[node_address]
        dma_client_compat = dts.get_compat(node_address)
        dma_client_dma_names = dma_client['props'].get('dma-names', None)

        dma_index = 0
        dma_cell_index = 0
        dma_nr_cells = 0
        dma_controller_node_address = ''
        dma_controller = {}
        for cell in dmas:
            if dma_cell_index == 0:
                if cell not in dts.phandles:
                    raise Exception(
                        ("Could not find the dma controller node {} for dmas"
                         " = {} in dma client node {}. Did you activate"
                         " the dma node?. Last dma controller: {}.")
                            .format(str(cell), str(dmas), node_address,
                                    str(dma_controller)))
                dma_controller_node_address = dts.phandles[cell]
                dma_controller = dts.reduced[dma_controller_node_address]
                dma_nr_cells = int(dma_controller['props'].get('#dma-cells', 1))
                dma_cells = []
            else:
                dma_cells.append(cell)
            dma_cell_index += 1
            if dma_cell_index > dma_nr_cells:
                if len(dma_client_dma_names) > dma_index:
                    dma_name = dma_client_dma_names[dma_index]
                else:
                    dma_name = str(dma_index)
                 # generate EDTS
                edts.insert_device_property(dma_client_device_id,
                    'dmas/{}/controller'.format(dma_name),
                    self.edts_device_id(dma_controller_node_address))
                self.edts_insert_dma_cells(node_address,
                                           dma_controller_node_address,
                                           dma_cells,
                                           "dmas/{}/{{}}".format(dma_name))

                dma_cell_index = 0
                dma_index += 1

    def _extract_controller(self, node_address, prop_list):
        dts = self.dts()
        edts = self.edts()

        dma_controller = dts.reduced[node_address]

        # generate EDTS
        dma_controller_device_id = self.edts_device_id(node_address)
        edts.insert_device_property(dma_controller_device_id,
            'dma-channels', dma_controller['props']['dma-channels'])
        edts.insert_device_property(dma_controller_device_id,
            'dma-requests', dma_controller['props']['dma-requests'])

    ##
    # @brief Extract dma related directives
    #
    # @param node_address Address of node owning the dmaxxx definition.
    # @param prop dmaxxx property name
    #
    def extract(self, node_address, prop):

        properties = self.dts().reduced[node_address]['props'][prop]

        prop_list = []
        if not isinstance(properties, list):
            prop_list.append(properties)
        else:
            prop_list = list(properties)

        if prop == 'dmas':
            # indicator for dma clients
            self._extract_client(node_address, prop_list)
        elif prop == '#dma-cells':
            # indicator for dma controllers
            self._extract_controller(node_address, prop_list)
        else:
            raise Exception(
                "DTDMA.extract called with unexpected directive ({})."
                    .format(prop))
