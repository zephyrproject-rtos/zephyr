#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from .directive import DTDirective

##
# @brief Manage pwm directives.
#
# Handles:
# - pwms
# - pwm-names
#
# Generates in EDTS:
# - <device>-controller : True
class DTPWM(DTDirective):

    def __init__(self, extractors):
        super().__init__(extractors)
        # Ignore properties that are covered by the extraction of
        extractors.ignore_properties.extend([ \
            # - 'pwms' for pwm clients
            'pwm-names', ])

    ##
    # @brief Insert pwm cells into EDTS
    #
    # @param pwm_client
    # @param bindings
    # @param pwm_controller
    # @param pwm_cells
    # @param property_path_templ "xxx/yyy/{}"
    #
    def edts_insert_pwm_cells(self, pwm_client_node_address,
                              pwm_controller_node_address, pwm_cells,
                              property_path_templ):
        dts = self.dts()
        bindings = self.bindings()
        edts = self.edts()

        if len(pwm_cells) == 0:
            return
        pwm_client_device_id = self.edts_device_id(pwm_client_node_address)
        pwm_controller_device_id = self.edts_device_id(pwm_controller_node_address)
        pwm_controller = dts.reduced[pwm_controller_node_address]
        pwm_controller_compat = dts.get_compat(pwm_controller_node_address)
        pwm_controller_bindings = bindings[pwm_controller_compat]
        pwm_nr_cells = int(pwm_controller['props'].get('#pwm-cells', 0))
        pwm_cells_names = pwm_controller_bindings.get(
                    '#pwm-cells', ['ID', 'CELL1',  "CELL2", "CELL3"])
        for i, cell in enumerate(pwm_cells):
            if i >= len(pwm_cells_names):
                pwm_cell_name = 'CELL{}'.format(i).lower()
            else:
                pwm_cell_name = pwm_cells_names[i].lower()
            edts.insert_device_property(pwm_client_device_id,
                property_path_templ.format(pwm_cell_name), cell)

    def _extract_client(self, node_address, prop):
        dts = self.dts()
        edts = self.edts()

        pwms = dts.reduced[node_address]['props']['pwms']

        if not isinstance(pwms, list):
            pwms = [pwms, ]
        # Flatten the list in case
        if isinstance(pwms[0], list):
            pwms = [item for sublist in pwms for item in sublist]

        pwm_client_device_id = self.edts_device_id(node_address)
        pwm_client = dts.reduced[node_address]
        pwm_client_pwm_names = pwm_client['props'].get('pwm-names', None)

        pwm_index = 0
        pwm_cell_index = 0
        pwm_nr_cells = 0
        pwm_controller_node_address = ''
        pwm_controller = {}
        for cell in pwms:
            if pwm_cell_index == 0:
                if cell not in dts.phandles:
                    raise Exception(
                        ("Could not find the pwm controller node {} for pwms"
                         " = {} in pwm client node {}. Did you activate"
                         " the pwm node?. Last pwm controller: {}.")
                            .format(str(cell), str(pwms), node_address,
                                    str(pwm_controller)))
                pwm_controller_node_address = dts.phandles[cell]
                pwm_controller = dts.reduced[pwm_controller_node_address]
                pwm_nr_cells = int(pwm_controller['props'].get('#pwm-cells', 0))
                pwm_cells = []
            else:
                pwm_cells.append(cell)
            pwm_cell_index += 1
            if pwm_cell_index > pwm_nr_cells:
                if len(pwm_client_pwm_names) > pwm_index:
                    pwm_name = pwm_client_pwm_names[pwm_index]
                else:
                    pwm_name = str(pwm_index)
                 # generate EDTS
                edts.insert_device_property(pwm_client_device_id,
                    'pwms/{}/controller'.format(pwm_name),
                    self.edts_device_id(pwm_controller_node_address))
                self.edts_insert_pwm_cells(node_address,
                                           pwm_controller_node_address,                                        pwm_cells,
                                           "pwms/{}/{{}}".format(pwm_name))

                pwm_cell_index = 0
                pwm_index += 1

    def _extract_controller(self, node_address, prop):
        dts = self.dts()

        pwm_controller = dts.reduced[node_address]
        # generate EDTS
        pwm_controller_device_id = self.edts_device_id(node_address)
        # TODO

    ##
    # @brief Extract pwm related directives
    #
    # @param edts Extended device tree database
    # @param dts Device tree specification
    # @param bindings Device tree bindings
    # @param node_address Address of node issuing the directive.
    # @param prop Directive property name
    #
    def extract(self, node_address, prop):

        if prop == 'pwms':
            # indicator for pwm clients
            self._extract_client(node_address, prop)
        elif prop in ('pwm-names', ):
            # covered by _extract_client
            pass
        elif prop == '#pwm-cells':
            # indicator for pwm controllers
            self._extract_controller(node_address, prop)
        else:
            raise Exception(
                "DTPWM.extract called with unexpected directive ({})."
                    .format(prop))
