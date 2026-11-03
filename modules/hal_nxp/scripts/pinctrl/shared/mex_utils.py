# SPDX-FileCopyrightText: (C) 2026 NXP

# SPDX-License-Identifier: Apache-2.0

"""
Shared accessors for MEX metadata and pin configuration XML.
"""

import logging
import xml.etree.ElementTree as ET

# MEX file has a default namespace, map it here
NAMESPACES = {'mex': 'http://mcuxpresso.nxp.com/XSD/mex_configuration_14'}


class NxpSignalPin:
    """Common metadata for PORT and IOCON pins; families retain their ordering."""

    _port: int | str
    _pin: int | str

    def __init__(self, pin):
        self._name = pin.attrib['name']
        self._properties = self._get_pin_properties(pin.find('functional_properties'))
        self._mux_options = {}

    def get_name(self):
        return self._name

    def get_port(self):
        return self._port

    def get_pin(self):
        return self._pin

    def get_mux_connection(self, signal):
        return self._mux_options.get(signal)

    def get_mux_options(self):
        return set(self._mux_options.values())

    def get_pin_properties(self):
        return self._properties.keys()

    def get_pin_property_default(self, prop):
        return self._properties[prop]['default']

    def get_pin_defaults(self):
        return {prop: self.get_pin_property_default(prop) for prop in self.get_pin_properties()}

    def get_pin_property_value(self, prop, selection):
        return self._properties[prop][selection]

    def _get_pin_properties(self, props):
        prop_mapping = {}
        for prop in props.findall('functional_property'):
            if 'default' not in prop.attrib:
                continue
            prop_id = prop.attrib['id']
            prop_mapping[prop_id] = {'default': prop.attrib['default']}
            for state in prop.findall('state'):
                reg_assign = state.find('configuration/assign')
                if reg_assign is not None and len(reg_assign):
                    bit_value = int(reg_assign.attrib['bit_field_value'], 0)
                else:
                    bit_value = 0
                prop_mapping[prop_id][state.attrib['id']] = bit_value
        return prop_mapping


class NxpMexUtil:
    def __init__(self, mexfile):
        """
        Initialize instance of this utility class
        @param mexfile: mex file to parse for board name
        """
        self.mexfile = mexfile

    def get_pins_version(self):
        """
        Gets version of pins tool from MEX file.
        @param self: instance of this class
        @ret version as integer
        """
        try:
            config_tree = ET.parse(self.mexfile)
            pins_version = config_tree.getroot().find('mex:tools/mex:pins', NAMESPACES)
            if pins_version is None:
                return 0.0
            return float(pins_version.get('version'))
        except ET.ParseError:
            logging.error("Malformed XML tree %s", self.mexfile)
            return None
        except OSError:
            logging.error("File %s could not be opened", self.mexfile)
            return None

    def get_board_name(self):
        """
        Extracts board name from a mex file
        @param self: instance of this class
        """
        try:
            config_tree = ET.parse(self.mexfile)
            if config_tree.getroot().find('mex:common/mex:board', NAMESPACES) is None:
                return self.get_processor_name() + '-board'
            return config_tree.getroot().find('mex:common/mex:board', NAMESPACES).text
        except ET.ParseError:
            logging.error("Malformed XML tree %s", self.mexfile)
            return None
        except OSError:
            logging.error("File %s could not be opened", self.mexfile)
            return None

    def get_processor_name(self):
        """
        Extracts processor name from a mex file
        @param self: instance of this class
        """
        try:
            config_tree = ET.parse(self.mexfile)
            processor = config_tree.getroot().find('mex:common/mex:processor', NAMESPACES)
            if processor is None:
                raise RuntimeError("Cannot locate processor name in MEX file")
            return processor.text
        except ET.ParseError:
            logging.error("Malformed XML tree %s", self.mexfile)
            return None
        except OSError:
            logging.error("File %s could not be opened", self.mexfile)
            return None

    def get_package_name(self):
        """
        Extracts package name from a mex file
        @param self.mexfile: mex file to parse for package name
        @param self: instance of this class
        """
        try:
            config_tree = ET.parse(self.mexfile)
            package = config_tree.getroot().find('mex:common/mex:package', NAMESPACES)
            if package is None:
                raise RuntimeError("Cannot locate package name in MEX file")
            return package.text
        except ET.ParseError:
            logging.error("Malformed XML tree %s", self.mexfile)
            return None
        except OSError:
            logging.error("File %s could not be opened", self.mexfile)
            return None
