#!/usr/bin/env python3
#
# Copyright (c) 2022,2024 NXP
#
# SPDX-License-Identifier: Apache-2.0

"""
Implements a configuration file parser for LPC and RT6xx/5xx MCUs,
which can generate pinctrl definitions for Zephyr
"""

import collections
import logging
import os
import pathlib
import re
import xml.etree.ElementTree as ET

from shared import pinctrl_layout

import __main__

NAMESPACES = {'mex': 'http://mcuxpresso.nxp.com/XSD/mex_configuration_14'}


class MUXOption:
    """
    Internal class representing a mux option on the SOC
    """

    def __init__(self, connection, imx_rt=''):
        """
        Initializes a mux option
        @param connection XML connection option from signal_configuration.xml
        """
        self._name = connection.attrib.get('name_part')
        if self._name is None:
            self._name = ''
            return

        self._name = re.sub(r'[^a-zA-Z0-9_]', '', self._name)
        logging.debug("\t\t %s", self._name)
        # Get MUX settings
        self._offset = -1
        # Get default instance index
        self._index = 0
        for periph in connection.iter('peripheral_signal_ref'):
            self._periph = periph.attrib.get('peripheral')
            self._signal = periph.attrib.get('signal')
            self._channel = periph.attrib.get('channel')
        self._mux_overrides = {}
        if imx_rt == 'MIMXRT7XX':
            # RT 7xx series has different function register and instance number
            func_name = 'FSEL'
            pio_regex = re.compile(r'IOPCTL(\d+)_PIO(\d+)_(\d+)')
        elif imx_rt == 'MIMXRT5/6XX':
            # RT 6xx/5xx series has different function register
            func_name = 'FSEL'
            pio_regex = re.compile(r'IOPCTL_PIO(\d+)_(\d+)')
        else:
            func_name = 'FUNC'
            pio_regex = re.compile(r'IOCON_PIO(\d)_*(\d+)')
        for assign in connection.iter('assign'):
            reg = assign.attrib.get('register')
            field = assign.attrib.get('bit_field')
            val = assign.attrib.get('bit_field_value')
            logging.debug('\t\t\t [ASSIGN] %s %s', reg, val)
            # Only process PIO register FUNC setting
            match = pio_regex.match(reg)
            if match and (field == func_name):
                if self._channel:
                    # For mux options with channels, format pin name as:
                    # {Peripheral}_{Signal}{Channel}_{Pin}
                    self._name = f"{self._periph}_{self._signal}{self._channel}"
                # Append name of pin
                if imx_rt == 'MIMXRT7XX':
                    self._name += f"_PIO{match.group(2)}_{match.group(3)}"
                    self._index = int(match.group(1))
                    port = int(match.group(2))
                    pin = int(match.group(3))
                    if port < 4:
                        self._offset = (port * 32) + pin
                    elif port < 8:
                        self._offset = ((port - 4) * 32) + pin
                    else:
                        self._offset = ((port - 8) * 32) + pin
                else:
                    self._name += f"_PIO{match.group(1)}_{match.group(2)}"
                    port = int(match.group(1))
                    pin = int(match.group(2))
                    self._offset = (port * 32) + pin
                self._mux = int(val, 16)
            elif match and field == 'MODE':
                # MUX overrides pullup/pulldown mode
                if val == '0':
                    self._mux_overrides['mode'] = 'inactive'
                elif val == '1':
                    self._mux_overrides['mode'] = 'pullDown'
                elif val == '2':
                    self._mux_overrides['mode'] = 'pullUp'
                elif val == '3':
                    self._mux_overrides['mode'] = 'repeater'
            elif match and field == 'ASW' and not imx_rt:
                # MUX override analog switch setting
                if val == '0x1':
                    self._mux_overrides['asw'] = 'enabled'
                    self._mux_overrides['digimode'] = 'disabled'
            elif match and field == 'ASW0' and not imx_rt:
                # LPC553x has two ASW bits
                if val == '0x1':
                    self._mux_overrides['asw0'] = 'enabled'
                    self._mux_overrides['digimode'] = 'disabled'
            elif match and field == 'ASW1' and not imx_rt:
                # LPC553x has two ASW bits
                if val == '0x1':
                    self._mux_overrides['asw1'] = 'enabled'
                    self._mux_overrides['digimode'] = 'disabled'
            elif match and field == 'AMENA' and imx_rt:
                # MUX override analog switch setting
                if val == '0x1':
                    self._mux_overrides['amena'] = 'enabled'

        if self._name == 'PMIC_I2C_SCL' and imx_rt == "MIMXRT5/6XX":
            # RT600/500 have special pmic I2C pins
            self._offset = 0x100
            self._mux = 0
        elif self._name == 'PMIC_I2C_SDA' and imx_rt == "MIMXRT5/6XX":
            self._offset = 0x101
            self._mux = 0
        elif self._name == 'PMIC_I2C_SCL' and imx_rt == "MIMXRT7XX":
            self._index = 1
            self._offset = 0x61
            self._mux = 0
        elif self._name == 'PMIC_I2C_SDA' and imx_rt == "MIMXRT7XX":
            self._index = 1
            self._offset = 0x60
            self._mux = 0
        if re.match(r'^\d', self._name):
            # If string starts with a digit, it will not be a valid C name
            self._name = f"PIN_{self._name}"
        if self._offset == -1:
            # Not a valid port mapping. Clear name
            self._name = ''

    def __repr__(self):
        """
        String representation of object
        """
        return f"MUXOption({self._name})"

    def get_name(self):
        """
        Get mux option name
        """
        return self._name

    def get_mux_name(self):
        """
        Get name of the mux option, without pin name
        """
        if self._channel:
            return f"{self._periph}_{self._signal}, {self._channel}"
        return f"{self._periph}_{self._signal}"

    def get_mux_overrides(self):
        """
        Some MUX options define specific pin property overrides. Get them here
        if they exist
        """
        return self._mux_overrides

    def get_port(self):
        """
        Get mux port
        """
        return self._port

    def get_signal(self):
        """
        Get mux signal name
        """
        return self._signal

    def get_offset(self):
        """
        Get mux register offset
        """
        return self._offset

    def get_pin(self):
        """
        Get mux pin
        """
        return self._pin

    def get_index(self):
        """
        Get mux instance index
        """
        return self._index

    def get_mux(self):
        """
        Get mux register write value
        """
        return self._mux

    def get_periph(self):
        """
        Get peripheral name
        """
        return self._periph

    def get_channel(self):
        """
        Get channel number
        """
        return self._channel

    def __hash__(self):
        """
        Override hash method to return pin name as hash
        """
        return hash(self._name)

    def __eq__(self, obj):
        """
        Like the hash method, we override the eq method to return true if two
        objects have the same pin name
        """
        return isinstance(obj, MUXOption) and self._name == obj._name

    def __lt__(self, obj):
        """
        Compare objects based on name
        """
        if not isinstance(obj, MUXOption):
            return True
        return self._name < obj._name


class SignalPin:
    """
    Internal class representing a signal on the SOC
    """

    def __init__(self, pin, imx_rt=''):
        """
        Initializes a SignalPin object
        @param pin: pin XML object from signal_configuration.xml
        """
        # lpc pin names are formatted as PIOx_y
        pin_regex = re.search(r'PIO(\d+)_(\d+)', pin.attrib['name'])
        if imx_rt and (
            pin.attrib['name'] == 'PMIC_I2C_SCL' or pin.attrib['name'] == 'PMIC_I2C_SDA'
        ):
            # iMX RT has special pins without a mux setting
            self._name = pin.attrib['name']
            self._port = 0
            self._pin = 0
        elif pin_regex is None:
            logging.debug('Could not match pin name %s', pin.attrib['name'])
            self._name = ''
            return
        else:
            self._name = pin.attrib['name']
            self._port = int(pin_regex.group(1))
            self._pin = int(pin_regex.group(2))
        self._properties = self._get_pin_properties(pin.find('functional_properties'))
        self._mux_options = {}
        for connections in pin.findall('connections'):
            mux_opt = MUXOption(connections, imx_rt=imx_rt)
            # Only append mux options with a valid name
            if mux_opt.get_name() != '':
                self._mux_options[mux_opt.get_mux_name()] = mux_opt

    def __repr__(self):
        """
        String representation of object
        """
        return f"SignalPin({self._name})"

    def __hash__(self):
        """
        Override hash method to return pin name as hash
        """
        return hash(self._name)

    def __eq__(self, obj):
        """
        Like the hash method, we override the eq method to return true if two
        objects have the same pin and port
        """
        return isinstance(obj, SignalPin) and self._name == obj._name

    def __lt__(self, obj):
        """
        Compare objects based on port and pin
        """
        if not isinstance(obj, SignalPin):
            return True
        if self._port == obj._port:
            return self._pin < obj._pin
        return self._port < obj._port

    def get_name(self):
        """
        Get name of pin
        """
        return self._name

    def get_port(self):
        """
        Get PORT this signal is defined for
        """
        return self._port

    def get_pin(self):
        """
        Get pin this signal is defined for
        """
        return self._pin

    def get_mux_connection(self, signal):
        """
        Gets an MUXOption object for the relevant signal name
        @param signal: Signal name on pin to get mux option for
        """
        if signal in self._mux_options:
            return self._mux_options[signal]
        return None

    def get_mux_options(self):
        """
        Gets all unique settings for IOMUX on the specific pin
        """
        return set(self._mux_options.values())

    def get_pin_properties(self):
        """
        Gets array of pin property names
        """
        return self._properties.keys()

    def get_pin_property_default(self, prop):
        """
        Gets name of default pin property
        @param prop: name of pin property
        """
        return self._properties[prop]['default']

    def get_pin_defaults(self):
        """
        Gets mapping of all pin property names to default value names
        """
        pin_defaults = {}
        for prop in self.get_pin_properties():
            pin_default = self.get_pin_property_default(prop)
            pin_defaults[prop] = pin_default
        return pin_defaults

    def get_pin_property_value(self, prop, selection):
        """
        Gets bit value for pin property
        @param prop: name of pin property
        @param selection: name of option selected for property
        """
        return self._properties[prop][selection]

    def _get_pin_properties(self, props):
        """
        Builds dictionary with all pin properties
        @param props: pin function_properties XML object in signal_configuration.xml
        """
        prop_mapping = {}
        for prop in props.findall('functional_property'):
            prop_id = prop.attrib['id']
            if 'default' not in prop.attrib:
                # No default property. Skip
                continue
            prop_mapping[prop_id] = {}
            prop_mapping[prop_id]['default'] = prop.attrib['default']
            for state in prop.findall('state'):
                reg_assign = state.find('configuration/assign')
                if reg_assign:
                    bit_value = int(reg_assign.attrib['bit_field_value'], 0)
                else:
                    # Assume writing zero to register will select default
                    bit_value = 0
                prop_mapping[prop_id][state.attrib['id']] = bit_value
        return prop_mapping


class PinGroup:
    """
    Internal class representing pin group
    """

    def __init__(self, function, signal_map, imx_rt=''):
        """
        Creates a pin group
        @param function: function xml structure from MEX configuration file
        @param signal_map: Signal mapping, maps signal names to signal pins
        """
        self._name = function.attrib.get('name')
        pins = function.find('mex:pins', NAMESPACES)
        description = function.find('mex:description', NAMESPACES)
        if description is not None and description.text is not None:
            # Replace <br> html tag with newline
            self._description = description.text.replace("&lt;br/&gt;", "\n")
        else:
            self._description = ""
        # Build dictionary mapping pin properties to pins. This allows us to
        # group pins based on shared configuration
        self._pin_groups = collections.defaultdict(lambda: [])
        for pin in pins:
            # find signal definition for this pin
            signal_name = pin.attrib.get('pin_signal')
            if signal_name not in signal_map:
                logging.warning('Signal name %s not present in mapping', signal_name)
                # No way to find mux option
                continue
            # Get mux option for this signal
            signal = signal_map[signal_name]
            mux_option = f"{pin.attrib.get('peripheral')}_{pin.attrib.get('signal')}"
            mux = signal.get_mux_connection(mux_option)
            if mux is None:
                logging.warning('Signal name %s has no mux', mux_option)
                # Do not add pinmux option to group
                continue
            # Get pin defaults for this pin
            defaults = signal.get_pin_defaults()
            # Get pin overrides
            features = pin.find('mex:pin_features', NAMESPACES)
            pin_overrides = {}
            if features is not None:
                for feature in pin.find('mex:pin_features', NAMESPACES):
                    pin_overrides[feature.attrib.get('name')] = feature.attrib.get('value')
            # Get pin mux option overrides
            for override, value in mux.get_mux_overrides().items():
                pin_overrides[override] = value
            if imx_rt:
                pin_props = self._imx_rt_props_to_dts(pin_overrides, defaults)
            else:
                pin_props = self._lpc_props_to_dts(pin_overrides, defaults)
            self._pin_groups[pin_props].append(mux)

    def __repr__(self):
        """
        Get string representation of the object
        """
        return f"PinGroup({self._name})"

    def __eq__(self, obj):
        """
        return true if two objects have the same pin group name
        """
        return isinstance(obj, PinGroup) and self._name == obj._name

    def __lt__(self, obj):
        """
        Compare objects based on name
        """
        if not isinstance(obj, PinGroup):
            return True
        return self._name < obj._name

    def get_pin_props(self):
        """
        Get all unique pin properties
        """
        return self._pin_groups.keys()

    def get_pins(self, props):
        """
        Get all pins with a provided set of properties
        @param props: property set
        """
        return self._pin_groups[props]

    def get_description(self):
        """
        Get description of the pin group, if present. If no description present,
        description will be ""
        """
        return self._description

    def get_name(self):
        """
        Get pin group name
        """
        return self._name

    def _imx_rt_props_to_dts(self, props, defaults):
        """
        Remap dictionary of property names from NXP defined values to
        Zephyr ones (applies to RT600/RT500 properties)
        @param props: Dictionary of NXP property names and values
        @param defaults: Dictionary of NXP property names and default pin values
        @return array of strings suitable for writing to DTS
        """
        zephyr_props = []
        prop_mapping = {
            # Slew rate property mappings
            'normal': 'normal',
            'slow': 'slow',
            # Drive strength property mappings
            'full': 'high',
        }

        def sanitize(x):
            return "\"" + prop_mapping[x] + "\"" if (x in prop_mapping) else ""

        def prop_val(x):
            return props[x] if x in props else defaults[x]

        # For each property, append the provided override or the default
        # Check pull settings
        pull_enable = prop_val('pupdena') == 'enabled'
        if pull_enable:
            if prop_val('pupdsel') == 'pullDown':
                zephyr_props.append('bias-pull-down')
            else:
                zephyr_props.append('bias-pull-up')
        # input buffer
        if prop_val('ibena') == 'enabled':
            zephyr_props.append('input-enable')
        # Slew rate settings
        zephyr_props.append(f"slew-rate = {sanitize(prop_val('slew_rate'))}")
        # Drive strength
        zephyr_props.append(f"drive-strength = {sanitize(prop_val('drive'))}")
        # analog switch
        if prop_val('amena') == 'enabled':
            zephyr_props.append('nxp,analog-mode')
        # open drain
        if prop_val('odena') == 'enabled':
            zephyr_props.append('drive-open-drain')
        # Pin invert settings
        if prop_val('iiena') == 'enabled':
            zephyr_props.append('nxp,invert')

        return tuple(zephyr_props)

    def _lpc_props_to_dts(self, props, defaults):
        """
        Remap dictionary of property names from NXP defined values to
        Zephyr ones (applies to LPC properties only)
        @param props: Dictionary of NXP property names and values
        @param defaults: Dictionary of NXP property names and default pin values
        @return array of strings suitable for writing to DTS
        """
        zephyr_props = []
        prop_mapping = {
            # Slew rate property mappings
            'standard': 'standard',
            'fast': 'fast',
            # power source property mappings
            'signal3v3': '3v3',
            'signal1v8': '1v8',
            # i2cfilter property mappings
            'nonhighspeedmode': 'slow',
            'highspeedmode': 'fast',
        }

        def sanitize(x):
            return "\"" + prop_mapping[x] + "\"" if (x in prop_mapping) else ""

        def prop_val(x):
            return props[x] if x in props else defaults.get(x, "")

        # For each property, append the provided override or the default
        # Check pull settings
        if prop_val('mode') == 'pullUp':
            zephyr_props.append('bias-pull-up')
        elif prop_val('mode') == 'pullDown':
            zephyr_props.append('bias-pull-down')
        elif prop_val('mode') == 'repeater':
            # Repeater latches the pin to the last input, to keep it from floating
            zephyr_props.append('drive-push-pull')
        # Slew rate settings
        if 'slew_rate' in defaults:
            zephyr_props.append(f"slew-rate = {sanitize(prop_val('slew_rate'))}")
        # Pin invert settings
        if prop_val('invert') == 'enabled':
            zephyr_props.append('nxp,invert')
        # open drain settings
        if prop_val('open_drain') == 'enabled':
            zephyr_props.append('drive-open-drain')
        # Note we only respect the ASW setting if digimode is false.
        if (
            'asw' in defaults
            and prop_val('asw') == 'enabled'
            and prop_val('digimode') == 'disabled'
        ):
            zephyr_props.append('nxp,analog-mode')
        if prop_val('asw0') == 'enabled' and prop_val('digimode') == 'disabled':
            # analog switch setting 0 (LPC553x has two ASW bits)
            zephyr_props.append('nxp,analog-mode')
        if prop_val('asw1') == 'enabled' and prop_val('digimode') == 'disabled':
            # analog switch setting 0 (LPC553x has two ASW bits)
            zephyr_props.append('nxp,analog-alt-mode')
        if 'ssel' in defaults:
            # i2c power source select (SSEL bit for type I pins)
            zephyr_props.append(f"power-source = {sanitize(prop_val('ssel'))}")
            # i2c filter (FILTEROFF bit for type I pins)
            # Note that when filter_off == 'enabled', the filter is actually on
            if prop_val('filter_off') == 'enabled':
                # Check i2c filter speed bit (I2CFILTER bit for type I pins)
                zephyr_props.append(f"nxp,i2c-filter = {sanitize(prop_val('i2cfilter'))}")
            # i2c pullup (ECS bit for type I pins)
            if prop_val('ecs') == 'enabled':
                zephyr_props.append('nxp,i2c-pullup')
            # i2c mode (EGP bit for type I pins)
            if prop_val('egp') == 'i2c':
                zephyr_props.append('nxp,i2c-mode')

        return tuple(zephyr_props)


class NXPSdkUtil:
    """
    Class for lpc configuration file parser
    """

    def __init__(self, cfg_root, copyright_header="", log_level=logging.ERROR):
        """
        Initialize SDK utilities.
        Providing a signal file will enable this class to parse MEX files,
        and generate output DTS
        @param cfg_root processor configuration folder root
        @param copyright_header: copyright string to add to any generated file header
        @param log_level: log level for SDK utility
        """
        # Load the signal XML data

        self._logger = logging.getLogger('')
        self._logger.setLevel(log_level)
        self._parse_signal_xml(pathlib.Path(cfg_root) / 'signal_configuration.xml')
        self._copyright = copyright_header
        logging.info("Loaded %d configurable pin defs", len(self._pins))

    def _parse_signal_xml(self, signal_fn):
        """
        Parses signal XML configuration file. Builds a list of pins, which can
        be used to generate soc level DTSI file.
        @param signal_fn: signal_configuration.xml file to parse
        """
        self._pins = {}
        try:
            signal_tree = ET.parse(signal_fn)
        except ET.ParseError:
            logging.error("Could not parse provided signal file: %s", signal_fn)
            return

        signal_root = signal_tree.getroot()

        self._part_num = signal_root.find("./part_information/part_number").get('id')
        if 'MIMXRT7' in self._part_num:
            # IMX RT600/500 series part. Different register layout and pin names
            self._imx_rt = 'MIMXRT7XX'
        elif 'MIMXRT' in self._part_num:
            # IMX RT600/500 series part. Different register layout and pin names
            self._imx_rt = 'MIMXRT5/6XX'
        else:
            self._imx_rt = ''

        logging.info("Loaded XML for %s", self._part_num)

        pins_node = signal_root.find("pins")
        for pin in pins_node:
            signal = SignalPin(pin, self._imx_rt)
            # Only add valid signal pins to list
            if signal.get_name() != '':
                self._pins[signal.get_name()] = signal

    def get_part_num(self):
        """
        Return the part number this class is instantiated for
        """
        return self._part_num

    def write_pinctrl_defs(self, outputfile):
        """
        Writes all pin mux options into pinctrl header file. Board level pin
        groups can include this pinctrl header file to access pin control
        definitions.
        @param outputfile: file to write output pinctrl defs to
        """
        file_header = (
            "/*\n"
            f" * NOTE: File generated by {os.path.basename(__main__.__file__)}\n"
            f" * from {self._part_num}/signal_configuration.xml\n"
            " *\n"
            f" * {self._copyright}\n"
            " */\n"
            "\n"
        )

        if self._imx_rt == 'MIMXRT7XX':
            # Notes on the below macro:
            # Due to IOPCTL instance number is nonunique, index variable is
            # introduced to represent the label of IOPCTL instance. We use
            # 4 bits to store index value.
            # We store the pin and port values as an offset, because some pins
            # do not follow a consistent offset. We use 12 bits to store this
            # offset.
            # Mux values range from 0-15, so we give 4 bits
            # shift the offset to the MSBs of the mux value, so they
            # don't conflict with pin configuration settings
            # Store the mux value at the offset it will actually be written to the
            # configuration register
            mux_macro = (
                "#define IOPCTL_MUX(index, offset, mux)\t\t\\\n"
                "\t((((index) & 0x7) << 15) |\t\t\\\n"
                "\t(((offset) & 0xFFF) << 20) |\t\t\\\n"
                "\t(((mux) & 0xF) << 0))\n\n"
            )
        elif self._imx_rt == 'MIMXRT5/6XX':
            # Notes on the below macro:
            # We store the pin and port values as an offset, because some pins
            # do not follow a consistent offset. We use 12 bits to store this
            # offset.
            # Mux values range from 0-15, so we give 4 bits
            # shift the offset to the MSBs of the mux value, so they
            # don't conflict with pin configuration settings
            # Store the mux value at the offset it will actually be written to the
            # configuration register
            mux_macro = (
                "#define IOPCTL_MUX(offset, mux)\t\t\\\n"
                "\t((((offset) & 0xFFF) << 20) |\t\t\\\n"
                "\t(((mux) & 0xF) << 0))\n\n"
            )
        else:
            # Notes on the below macro:
            # We store the pin and port values as an offset, because some pins
            # do not follow a consistent offset. We use 12 bits to store this
            # offset.
            # Mux values range from 0-15, so we give 4 bits
            # type values range from 0-2, so we give 3 bits
            # shift the offset to the MSBs of the mux value, so they
            # don't conflict with pin configuration settings
            # Store the mux value at the offset it will actually be written to the
            # configuration register
            mux_macro = (
                "#define IOCON_MUX(offset, type, mux)\t\t\\\n"
                "\t(((offset & 0xFFF) << 20) |\t\t\\\n"
                "\t(((type) & 0x3) << 18) |\t\t\\\n"
                "\t(((mux) & 0xF) << 0))\n\n"
                "#define IOCON_TYPE_D 0x0\n"
                "#define IOCON_TYPE_I 0x1\n"
                "#define IOCON_TYPE_A 0x2\n\n"
            )
        with open(outputfile, "w", encoding="utf8") as file:
            file.write(file_header)
            # ifdef guard
            file.write(f"#ifndef _ZEPHYR_DTS_BINDING_{self._part_num.upper()}_\n")
            file.write(f"#define _ZEPHYR_DTS_BINDING_{self._part_num.upper()}_\n\n")
            # Write macro to make port name
            file.write(mux_macro)
            # Write pins
            for pin in sorted(self._pins.values()):
                if not self._imx_rt:
                    # LPC IOCON has analog and I2C type pins, iMX RT does not
                    if 'asw' in pin.get_pin_defaults():
                        pin_type = 'IOCON_TYPE_A'  # Analog pin type
                    elif 'asw0' in pin.get_pin_defaults():
                        pin_type = 'IOCON_TYPE_A'  # LPC553x has ASW0 and ASW1 bits
                    elif 'ssel' in pin.get_pin_defaults():
                        pin_type = 'IOCON_TYPE_I'
                    else:
                        pin_type = 'IOCON_TYPE_D'
                sig_port = pin.get_port()
                sig_pin = pin.get_pin()
                for mux in sorted(pin.get_mux_options()):
                    index = mux.get_index()
                    offset = mux.get_offset()
                    label = mux.get_name()
                    mux = mux.get_mux()
                    if self._imx_rt == 'MIMXRT7XX':
                        file.write(
                            f"#define {label} IOPCTL_MUX({index}, {offset}, {mux}) "
                            f"/* PIO{sig_port}_{sig_pin} */\n"
                        )
                    elif self._imx_rt == 'MIMXRT5/6XX':
                        file.write(
                            f"#define {label} IOPCTL_MUX({offset}, {mux}) "
                            f"/* PIO{sig_port}_{sig_pin} */\n"
                        )
                    else:
                        file.write(
                            f"#define {label} IOCON_MUX({offset}, {pin_type}, {mux}) "
                            f"/* PIO{sig_port}_{sig_pin} */\n"
                        )

            file.write("\n#endif\n")

    def _parse_mex_cfg(self, mexfile):
        """
        Parses mex configuration into pin groups.
        @param mexfile: mex configuration file to parse
        @return parsed pin groups
        """
        pin_groups = {}
        try:
            mex_xml = ET.parse(mexfile)
            for function in mex_xml.findall(
                'mex:tools/mex:pins/mex:functions_list/mex:function', NAMESPACES
            ):
                group = PinGroup(function, self._pins, self._imx_rt)
                pin_groups[group.get_name()] = group
            return pin_groups
        except ET.ParseError:
            logging.error("Could not parse mex file %s", mex_xml)
            return None

    def write_pinctrl_groups(self, mexfile, outputfile):
        """
        Write pinctrl groups to disk as a parsed DTS file. Intended for use
        with the output of @ref write_pinctrl_defs
        @param mexfile: mex file to parse
        @param outputfile: DTS pinctrl file to write pin groups to
        """

        file_header = (
            "/*\n"
            f" * NOTE: File generated by {os.path.basename(__main__.__file__)}\n"
            f" * from {os.path.basename(mexfile)}\n"
            " *\n"
            f" * {self._copyright}\n"
            " */\n"
            "\n"
        )
        pin_groups = self._parse_mex_cfg(mexfile)
        with open(outputfile, "w", encoding="utf8") as file:
            file.write(file_header)
            include = pinctrl_layout.include_path(get_package_name(mexfile), ".h")
            file.write(f"\n#include <{include}>\n\n")
            file.write("&pinctrl {\n")
            # Write pin groups back out to disk
            for group in pin_groups.values():
                pin_props = group.get_pin_props()
                description = group.get_description()
                # if a description is present, write it
                if description != "":
                    description_lines = description.split("\n")
                    if len(description_lines) == 1:
                        file.write(f"\t/* {description} */\n")
                    else:
                        file.write("\t/*\n")
                        for line in description_lines:
                            file.write(f"\t * {line}\n")
                        file.write("\t */\n")
                logging.info("Writing pin group %s to disk", group.get_name())
                file.write(f"\t{group.get_name().lower()}: {group.get_name().lower()} {{\n")
                for idx, pin_prop in enumerate(sorted(pin_props)):
                    group_str = f"\t\tgroup{idx} {{\n"
                    # Write all pin names
                    group_str += "\t\t\tpinmux = "
                    for pin in group.get_pins(pin_prop):
                        group_str += f"<{pin.get_name()}>,\n\t\t\t\t"
                    # Strip out last 3 tabs and close pin name list
                    group_str = re.sub(r',\n\t\t\t\t$', ';\n', group_str)
                    # Write all pin props
                    if pin_prop is None:
                        logging.error("No pin properties present")
                    for prop in pin_prop:
                        group_str += f"\t\t\t{prop};\n"
                    group_str += "\t\t};\n"
                    file.write(group_str)
                file.write("\t};\n\n")
            file.write("};\n")


"""
Utility functions used to get details about board/processor from MEX file
"""


def get_board_name(mexfile):
    """
    Extracts board name from a mex file
    @param mexfile: mex file to parse for board name
    """
    try:
        config_tree = ET.parse(mexfile)
        if config_tree.getroot().find('mex:common/mex:board', NAMESPACES) is None:
            return get_processor_name(mexfile) + '-board'
        return config_tree.getroot().find('mex:common/mex:board', NAMESPACES).text
    except ET.ParseError:
        print(f"Malformed XML tree {mexfile}")
        return None
    except OSError:
        print(f"File {mexfile} could not be opened")
        return None


def get_processor_name(mexfile):
    """
    Extracts processor name from a mex file
    @param mexfile: mex file to parse for processor name
    """
    try:
        config_tree = ET.parse(mexfile)
        processor = config_tree.getroot().find('mex:common/mex:processor', NAMESPACES)
        if processor is None:
            raise RuntimeError(
                "Cannot locate processor name in MEX file. "
                "Are you using v12 of the MCUXpresso configuration tools?"
            )
        return processor.text
    except ET.ParseError:
        print(f"Malformed XML tree {mexfile}")
        return None
    except OSError:
        print(f"File {mexfile} could not be opened")
        return None


def get_package_name(mexfile):
    """
    Extracts package name from a mex file
    @param mexfile: mex file to parse for package name
    """
    try:
        config_tree = ET.parse(mexfile)
        return config_tree.getroot().find('mex:common/mex:package', NAMESPACES).text
    except ET.ParseError:
        print(f"Malformed XML tree {mexfile}")
        return None
    except OSError:
        print(f"File {mexfile} could not be opened")
        return None
