#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from extract.globals import *
from extract.directive import DTDirective

from extract.default import default
from extract.reg import reg

flash_dev_label_prefix = {
    "soc-nv-flash":"soc_flash",
    "serial-flash":"spi_flash"
}

##
# @brief Manage flash directives.
#
class DTFlash(DTDirective):

    def __init__(self):
        # Node of the flash
        self._flash_node = None
        self._flash_dev = {}
        self._flash_area = {}

    def _extract_flash_dev(self, node_address, yaml, prop, def_label):
        prop_def = {}
        prop_alias = {}

        if node_address != 'dummy-flash':
            dev_type = get_compat(node_address)

        if 'flash-controller@' in node_address:
            controller_address = ''
            for comp in node_address.split('/')[1:-1]:
                controller_address += '/' + comp
                if 'flash-controller@' in controller_address:
                    node_address = controller_address
                    break

        if node_address != 'dummy-flash':
            node = reduced[node_address]

            if 'label' in node['props']:
                name = "\"{}\"".format(node['props']['label'])
            else:
                name = "\"{}\"".format(node['label'])

            if name in self._flash_dev:
                index = self._flash_dev[name]['id']
                type_index = self._flash_dev[name]['type_index']
            else:
                index = len(self._flash_dev)
                type_index = 0

                for dev in self._flash_dev:
                    if self._flash_dev[dev]['type'] == dev_type:
                        type_index = type_index + 1

                self._flash_dev[name] = {'id': index,
                                         'node': node,
                                         'type': dev_type,
                                         'type_index':type_index}

            label = self.get_label_string([def_label, str(index), 'name'])
            prop_def[label] = name

            if dev_type in flash_dev_label_prefix:
                dev_label = self.get_label_string(
                    [flash_dev_label_prefix[dev_type], str(type_index), 'id'])
            else:
                dev_label = self.get_label_string(
                    ['flash', str(type_index), 'id'])

            prop_def[dev_label] = index

        label = self.get_label_string([def_label, 'num'])
        prop_def[label] = len(self._flash_dev)

        insert_defs(def_label, prop_def, prop_alias)

        if node_address != 'dummy-flash':
            prop_def = {}
            prop_alias = {}

            for area in self._flash_area.keys():
                if node_address in area:
                    label = self.get_label_string(
                        ["FLASH_AREA", str(self._flash_area[area]["id"]), "DEV_ID"])
                    prop_alias[label] = dev_label

            insert_defs("FLASH_AREA", prop_def, prop_alias)

    def extract_partition(self, node_address):
        prop_def = {}
        prop_alias = {}
        node = reduced[node_address]

        partition_name = node['props']['label']
        partition_sectors = node['props']['reg']

        label_prefix = ["FLASH_AREA", partition_name]
        label = self.get_label_string(label_prefix + ["LABEL",])
        prop_def[label] = '"{}"'.format(partition_name)

        label = self.get_label_string(label_prefix + ["READ_ONLY",])
        if node['props'].get('read-only'):
            prop_def[label] = 1
        else:
            prop_def[label] = 0

        index = 0
        while index < len(partition_sectors):
            sector_index = int(index/2)
            sector_start_offset = partition_sectors[index]
            sector_size = partition_sectors[index + 1]
            label = self.get_label_string(
                label_prefix + ["OFFSET", str(sector_index)])
            prop_def[label] = "{}".format(sector_start_offset)
            label = self.get_label_string(
                label_prefix + ["SIZE", str(sector_index)])
            prop_def[label] = "{}".format(sector_size)
            index += 2

        if node_address in self._flash_area:
            area_id = self._flash_area[node_address]["id"]
        else:
            area_id = len(self._flash_area)
            self._flash_area[node_address] = {'id': area_id }

        label = self.get_label_string(label_prefix + ["ID",])
        prop_def[label] = area_id

        label = self.get_label_string(label_prefix + ["OFFSET",])
        prop_alias[label] = self.get_label_string(
            label_prefix + ["OFFSET", '0'])
        label = self.get_label_string(label_prefix + ["SIZE",])
        prop_alias[label] = self.get_label_string(
            label_prefix + ["SIZE", '0'])

        insert_defs(node_address, prop_def, prop_alias)

        prop_def = {}
        prop_alias = {}


        label = self.get_label_string(["FLASH_AREA", str(area_id)])
        prop_alias[label] = self.get_label_string([partition_name,])
        label = self.get_label_string(["FLASH_AREA", 'num'])
        prop_def[label] = len(self._flash_area)

        insert_defs("FLASH_AREA", prop_def, prop_alias)

    def _extract_flash(self, node_address, yaml, prop, def_label):
        load_defs = {}

        if node_address == 'dummy-flash':
            # We will add addr/size of 0 for systems with no flash controller
            # This is what they already do in the Kconfig options anyway
            load_defs = {
                'CONFIG_FLASH_BASE_ADDRESS': 0,
                'CONFIG_FLASH_SIZE': 0
            }
            insert_defs(node_address, load_defs, {})
            self._flash_base_address = 0
            return

        self._flash_node = reduced[node_address]

        flash_props = ["label", "write-block-size", "erase-block-size"]
        for prop in flash_props:
            if prop in self._flash_node['props']:
                default.extract(node_address, None, prop, None, def_label)
        insert_defs(node_address, load_defs, {})

        #for address in reduced:
        #    if address.startswith(node_address) and 'partition@' in address:
        #        self._extract_partition(address, yaml, 'partition', None, def_label)

    def _extract_code_partition(self, node_address, yaml, prop, def_label):
        load_defs = {}

        if node_address == 'dummy-flash':
            node = None
        else:
            node = reduced[node_address]
            if self._flash_node is None:
                # No flash node scanned before-
                raise Exception(
                    "Code partition '{}' {} without flash definition."
                        .format(prop, node_address))

        if node and node is not self._flash_node:
            # only compute the load offset if the code partition
            # is not the same as the flash base address
            load_offset = node['props']['reg'][0]
            load_defs['CONFIG_FLASH_LOAD_OFFSET'] = load_offset
            load_size = node['props']['reg'][1]
            load_defs['CONFIG_FLASH_LOAD_SIZE'] = load_size
        else:
            load_defs['CONFIG_FLASH_LOAD_OFFSET'] = 0
            load_defs['CONFIG_FLASH_LOAD_SIZE'] = 0

        insert_defs(node_address, load_defs, {})

    ##
    # @brief Extract flash
    #
    # @param node_address Address of node owning the
    #                     flash definition.
    # @param yaml YAML definition for the owning node.
    # @param prop compatible property name
    # @param def_label Define label string of node owning the
    #                  compatible definition.
    #
    def extract(self, node_address, yaml, prop, def_label):

        if prop == 'zephyr,flash':
            # indicator for flash
            self._extract_flash(node_address, yaml, prop, def_label)
        elif prop == 'zephyr,code-partition':
            # indicator for code_partition
            self._extract_code_partition(node_address, yaml, prop, def_label)
        elif prop == 'zephyr,flash_map':
            # indicator for flash_map
            self._extract_flash_dev(node_address, yaml, prop, def_label)
        else:
            raise Exception(
                "DTFlash.extract called with unexpected directive ({})."
                    .format(prop))
##
# @brief Management information for flash.
flash = DTFlash()
