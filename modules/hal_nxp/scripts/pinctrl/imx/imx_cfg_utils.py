# SPDX-License-Identifier: Apache-2.0

"""
Implements a module to parse iMX.RT MEX configuration files to extract pin
configuration groups, and transform them into pinctrl groups suitable for use
in Zephyr.
"""

import collections
import logging
import os
import pathlib
import re
import xml.etree.ElementTree as ET

from shared import pinctrl_layout

import __main__

# MEX file has a default namespace, map it here
NAMESPACES = {'mex': 'http://mcuxpresso.nxp.com/XSD/mex_configuration_14'}

# Description formats used by the DAISY bit field values of an IOMUXC
# select-input register. Both name the pad and the ALT mode that a given daisy
# value selects; only the wording differs between SOC families.
DAISY_DESCRIPTIONS = (
    # i.MX RT266x: "PIO2_0---ALT11"
    re.compile(r'(?P<pad>PIO\d+_\d+)---ALT(?P<alt>\d+)$'),
    # i.MX RT 4-digit: "Selecting Pad: GPIO_AD_16 for Mode: ALT9"
    re.compile(r'Selecting Pad:\s*(?P<pad>\S+)\s+for Mode:\s*ALT(?P<alt>\d+)$'),
)


def _parse_daisy_description(description):
    """
    Extract the (pad, alt mode) a daisy value selects from its description.
    @param description: DAISY bit field value description
    @return (pad name, alt mode) tuple, or (None, None) if unrecognized
    """
    for pattern in DAISY_DESCRIPTIONS:
        match = pattern.fullmatch(description.strip())
        if match:
            return match.group('pad'), int(match.group('alt'))
    return None, None


# One entry of the daisy maps built by NXPSdkUtil.build_daisy_maps()
DaisyForward = collections.namedtuple('DaisyForward', 'reg value signal instance pad')
DaisyReverse = collections.namedtuple('DaisyReverse', 'mux_reg mux_mode pad instance signal')


class Peripheral:
    """
    Internal class used to represent a peripheral
    """

    def __init__(self, peripheral_xml, register_xml=None):
        """
        Initializes peripheral object using peripheral XML structure
        @param peripheral_xml: peripheral XML object - parsed from registers.xml
        @param register: path to XML file with register description for this peripheral
        """
        self._name = peripheral_xml.attrib['name']
        self._fullname = peripheral_xml.attrib['full_name']
        # Peripheral address space in bytes
        self._size = int(peripheral_xml.attrib['size'])
        self._base_addr = int(peripheral_xml.find('base_address').attrib['addr'], 0)
        # Do not load registers now, just record the filename. This
        # Lazy loading speeds up the script, since we only need registers from
        # some peripherals
        if register_xml is not None:
            self._registers = {}
            self._register_file = register_xml
        else:
            self._registers = None
            self._register_file = None

    def _load_registers(self):
        """
        Loads registers from register XML file
        """
        if (self._register_file is not None) and (self._registers == {}):
            # Parse registers
            try:
                if not pathlib.Path(self._register_file).exists():
                    raise RuntimeError(f"Register file {self._register_file} does not exist")
                register_xml = ET.parse(self._register_file)
                for register_def in register_xml.findall('register'):
                    reg = Register(register_def)
                    self._registers[reg.get_name()] = reg
                # Parse register template definitions to locate remaining
                # Register definitions
                reg_templates = {}
                for template in register_xml.findall('reg_template'):
                    reg_templates[template.get('rid')] = template
                for reg_instance in register_xml.findall('reg_instance'):
                    reg = TemplatedRegister(reg_templates[reg_instance.get('rid')], reg_instance)
                    self._registers[reg.get_name()] = reg

            except ET.ParseError as error:
                raise RuntimeError(
                    f"Register file {self._register_file} is not valid XML"
                ) from error
        elif self._register_file is None:
            raise RuntimeError("Cannot load registers, no xml file path provided")

    def __repr__(self):
        """
        Generate string representation of the object
        """
        if self._registers:
            return f"Peripheral({self._name}, 0x{self._base_addr:X}, {len(self._registers)} Regs)"
        return f"Peripheral({self._name}, 0x{self._base_addr:X})"

    def get_name(self):
        """
        Gets peripheral name
        """
        return self._name

    def get_size(self):
        """
        Gets size of peripheral address region in bytes
        """
        return self._size

    def get_base(self):
        """
        Gets base address of peripheral
        """
        return self._base_addr

    def get_register(self, reg_name):
        """
        Get register object within peripheral by name
        @param reg_name: name of register to get
        """
        self._load_registers()
        return self._registers[reg_name]

    def get_registers(self):
        """
        Get every register in this peripheral, keyed by name. Templated
        registers appear under their expanded name.
        """
        self._load_registers()
        return self._registers

    def get_reg_addr(self, reg_name):
        """
        Gets full address of register in peripheral
        @param reg_name: name of register to calculate address for
        """
        self._load_registers()
        return self._base_addr + self._registers[reg_name].get_offset()


class Register:
    """
    Internal class used to represent a register in a peripheral
    """

    def __init__(self, register_xml):
        """
        Constructs a register object from provided register xml data
        """
        self._name = register_xml.attrib['name']
        self._offset = int(register_xml.attrib['offset'], 0)
        # Build mapping of register field values to descriptions
        self._bit_field_map = {}
        for bit_field in register_xml.findall('bit_field'):
            bit_field_map = {}
            for bit_field_value in bit_field.findall('bit_field_value'):
                desc = bit_field_value.attrib['description']
                if desc not in ('Reserved', ''):
                    # Some iMX8 fields have a ?, remove that
                    bit_field_str = bit_field_value.attrib['value'].strip('?')
                    field_val = int(bit_field_str, 0)
                    bit_field_map[field_val] = bit_field_value.attrib
            # Save bit field mapping
            self._bit_field_map[bit_field.attrib['name']] = bit_field_map

    def __repr__(self):
        """
        Generate string representation of the object
        """
        return f"Register({self._name}, 0x{self._offset:X})"

    def get_name(self):
        """
        Get the name of the register
        """
        return self._name

    def get_offset(self):
        """
        Get the offset of this register from the base
        """
        return self._offset

    def get_bit_field_value_description(self, bit_field, value):
        """
        Get human-readable description of the value a bit field in the register
        represents
        @param bit_field: name of register bit field
        @param value: value assigned to bit field
        @return description of effect that value has on register
        """
        return self._bit_field_map[bit_field][value]['description']

    def get_bit_fields(self):
        """
        Get list of all bit fields present in register
        """
        return self._bit_field_map.keys()

    def get_bit_field_values(self, bit_field):
        """
        Get every value defined for a bit field in this register
        @param bit_field: name of register bit field
        """
        return sorted(self._bit_field_map[bit_field].keys())


class TemplatedRegister(Register):
    """
    Subclass of standard register, that implements support for templated
    register definitions in a manner compatible with the standard register
    class instance.
    """

    def __init__(self, template_xml, instance_xml):
        """
        Constructs a register instance based off the register template XML
        and register instance XML
        """
        self._values = instance_xml.get('vals').split(' ')
        self._name = self._sub_template(template_xml.attrib['name'])
        self._offset = int(self._sub_template(template_xml.attrib['offset']), 0)
        # Build mapping of register field values to descriptions
        self._bit_field_map = {}
        for bit_field in template_xml.findall('bit_field'):
            bit_field_map = {}
            for bit_field_value in bit_field.findall('bit_field_value'):
                # Template parameters appear inside the bit field value
                # attributes as well, not just in the register name and offset.
                # The DAISY descriptions are the important case
                # ("PIO{1}_{2}---ALT{5}"), since they name the pad and ALT mode
                # each daisy value selects. Substitute the whole attribute dict
                # so callers never see a raw placeholder.
                attrib = {
                    key: self._sub_template(val) for key, val in bit_field_value.attrib.items()
                }
                desc = attrib['description']
                if desc not in ('Reserved', ''):
                    # Some iMX8 fields have a ?, remove that
                    bit_field_str = attrib['value'].strip('?')
                    field_val = int(bit_field_str, 0)
                    bit_field_map[field_val] = attrib
            # Save bit field mapping
            self._bit_field_map[self._sub_template(bit_field.attrib['name'])] = bit_field_map

    def _sub_template(self, string):
        """
        Uses string substitution to replace references to template parameter
        in string with value in a value array. For instance,
        SW_PAD_CTL_PAD_GPIO_EMC_{1} would become SW_PAD_CTL_PAD_GPIO_EMC_15
        if values[1] == 15
        """
        for i in range(len(self._values)):
            string = re.sub(r'\{' + re.escape(str(i)) + r'\}', self._values[i], string)
        return string


class SignalPin:
    """
    Internal class representing a signal on the SOC
    """

    def __init__(self, pin, peripheral_map, imx_rt, imx_rt2=False):
        """
        Initializes a SignalPin object
        @param pin: pin XML object from signal_configuration.xml
        @param peripheral_map mapping of peripheral names to peripheral objects
        @param imx_rt: is this signal configuration for an IMX RT part
        @param imx_rt2: is this signal configuration for an i.MX RT266x part.
            RT266x stores the pad configuration in the same PIO<port>_<pin>
            register that holds MUX_MODE (no SW_PAD_CTL_PAD_* register), so the
            pad-config discovery below is gated for this family.
        """
        self._name = pin.attrib['name']
        self._properties = self._get_pin_properties(pin.find('functional_properties'))
        self._iomuxc_options = {}

        cfg_addr = 0x0
        pad_name = self._name
        for prop in pin.findall('functional_properties/functional_property'):
            cfg_assign_xml = prop.find('state/configuration/assign')
            if cfg_assign_xml is None:
                # Not a configurable register. Skip.
                return
            match = re.match(r'init_([\w_]+)', cfg_assign_xml.attrib['configuration_step'])
            periph_name = match.group(1)
            # See if this property will have the pad configuration address
            prop_id = prop.attrib.get('id')
            if (prop_id != 'software_input_on') and (prop_id != 'SION'):
                match = re.match(
                    re.escape(periph_name) + r'_(\w+)', cfg_assign_xml.attrib['register']
                )
                reg_name = match.group(1)
                if imx_rt2:
                    # i.MX RT266x: pad config lives in the PIO<port>_<pin>
                    # register (same register as MUX_MODE). The reg_name is
                    # already the pad name, e.g. "PIO2_2". There is no
                    # SW_PAD_CTL_PAD_* prefix to strip.
                    pad_name = reg_name
                else:
                    match = re.match(r'SW_PAD_CTL_PAD_(\w+)', reg_name)
                    pad_name = match.group(1)
                cfg_reg = peripheral_map[periph_name].get_register(reg_name)
                cfg_addr = peripheral_map[periph_name].get_base() + cfg_reg.get_offset()
                # We have found the pad configuration address. Break.
                break
        for connections in pin.findall('connections'):
            name_part = connections.attrib.get('name_part')
            connection = connections.find('connection')
            signal = connection.find('peripheral_signal_ref').attrib['signal']
            if imx_rt:
                name = f"{periph_name}_{pad_name}_{name_part}"
            else:
                name = f"{periph_name}_{pad_name}_{signal.upper()}_{name_part}"
            # Determine the configuration register type. This is needed for
            # iMX RT11xx series devices
            cfg_fields = cfg_reg.get_bit_fields()
            if 'PDRV' in cfg_fields:
                pin_type = 'pdrv'
            elif 'ODE_LPSR' in cfg_fields:
                pin_type = 'lpsr'
            elif 'ODE_SNVS' in cfg_fields:
                pin_type = 'snvs'
            elif 'PUE' in cfg_fields:
                pin_type = 'pue'
            else:
                pin_type = 'unknown'
            iomux_opt = IOMUXOption(connection, peripheral_map, cfg_addr, name, pin_type, imx_rt2)
            peripheral = connection.find('peripheral_signal_ref').attrib['peripheral']
            channel = connection.find('peripheral_signal_ref').attrib.get('channel')
            if channel is not None:
                mux_name = f"{peripheral}_{signal}, {channel}"
            else:
                mux_name = f"{peripheral}_{signal}"
            self._iomuxc_options[mux_name] = iomux_opt

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
        objects have the same pin name
        """
        return isinstance(obj, SignalPin) and self._name == obj._name

    def __lt__(self, obj):
        """
        Compare objects based on name
        """
        if not isinstance(obj, SignalPin):
            return True
        return self._name < obj._name

    def get_name(self):
        """
        Get name of pin
        """
        return self._name

    def get_iomux_connection(self, signal):
        """
        Gets an IOMUXOption object for the relevant signal name
        @param signal: Signal name on pin to get mux option for
        """
        if signal in self._iomuxc_options:
            return self._iomuxc_options[signal]
        return None

    def get_mux_options(self):
        """
        Gets all unique settings for IOMUX on the specific pin
        """
        diff = len(self._iomuxc_options.values()) - len(set(self._iomuxc_options.values()))
        if diff:
            logging.warning("Warning: %d mux options dropped", diff)
        return set(self._iomuxc_options.values())

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
            if len(prop.findall('state/configuration/assign')) == 0:
                # Not configurable property. Skip
                continue
            prop_id = prop.attrib['id']
            prop_mapping[prop_id] = {}
            prop_mapping[prop_id]['default'] = prop.attrib['default']
            for state in prop.findall('state'):
                reg_assign = state.find('configuration/assign')
                bit_value = int(reg_assign.attrib['bit_field_value'], 0)
                prop_mapping[prop_id][state.attrib['id']] = bit_value
        return prop_mapping


# named tuple for GPIO port/pin
GPIO = collections.namedtuple('GPIO', 'port pin')


class IOMUXOption:
    """
    Internal class representing an IOMUXC option
    """

    def __init__(self, connection, peripheral_map, cfg_reg, name, pin_type, imx_rt2=False):
        """
        Initializes an IOMUXC option object
        @param connection: connection XML object from signal_configuration.xml
        @param peripheral_map: mapping of peripheral names to peripheral objects
        @param cfg_reg: configuration register for this IOMUXC option
        @param name: allows caller to override iomuxc name, if it is known
        @param pin_type: sets pin type value for config register (for RT11xx)
        @param imx_rt2: i.MX RT266x family. RT266x names GPIO peripherals
            "HSP__GPIO_<n>" / "VBAT__GPIO" rather than "GPIO<n>", so GPIO port
            extraction is gated for this family.
        """
        self._mux = 0
        self._mux_val = 0
        self._mux_periph = None
        self._daisy = 0
        self._daisy_val = 0
        self._cfg_reg = cfg_reg
        self._has_extended_config = False
        self._has_gpr = False
        self._extended_config = []
        self._name = name
        self._type = pin_type
        # Check if this connection controls a GPIO
        peripheral = connection.find('peripheral_signal_ref').attrib.get('peripheral')
        channel = connection.find('peripheral_signal_ref').attrib.get('channel')
        if 'GPIO' in peripheral and channel is not None:
            if imx_rt2:
                # RT266x GPIO peripherals are "HSP__GPIO_<n>" (numbered) or
                # "VBAT__GPIO"/"WAKE__GPIO" (unnumbered, treated as port 0).
                match = re.search(r'GPIO_(\d+)', peripheral)
                gpio_port = match.group(1) if match else '0'
            else:
                match = re.search(r'GPIO(\d+)', peripheral)
                gpio_port = match.group(1)
            self._is_gpio = True
            self._gpio = GPIO(int(gpio_port), int(channel))
        else:
            self._is_gpio = False
            self._gpio = (0, 0)
        # Process all configurations for this connection
        configurations = connection.findall('configuration')
        # handle 'assign' if only one configuration node, and it has no id
        if len(configurations) == 1 and 'id' not in configurations[0].attrib:
            for assignment in configurations[0].findall('assign'):
                self._process_assignment(assignment, peripheral_map)
        # handle multiple configurations
        else:
            for config in configurations:
                # only process configurations with an id containing 'alt'
                config_id = config.attrib.get('id')
                if config_id and 'alt' in config_id:
                    for assignment in config.findall('assign'):
                        self._process_assignment(assignment, peripheral_map)
                # ignore configurations without an id containing 'alt'
                else:
                    continue

    def _process_assignment(self, assignment, peripheral_map):
        """
        Process a single register assignment XML object
        """
        match = re.match(r'init_([\w_]+)', assignment.attrib['configuration_step'])
        periph_name = match.group(1)
        match = re.match(re.escape(periph_name) + r'_(\w+)', assignment.attrib['register'])
        reg_name = match.group(1)
        full_name = f"{periph_name}_{reg_name}"
        periph = peripheral_map[periph_name]
        addr = periph.get_reg_addr(reg_name)
        value = int(assignment.attrib['bit_field_value'], 0)

        if assignment.attrib.get('bit_field') == 'DAISY':
            self._daisy = addr
            self._daisy_val = value
        elif assignment.attrib.get('bit_field') == 'MUX_MODE':
            self._mux = addr
            self._mux_val = value
            # Record which IOMUXC peripheral instance owns the mux register.
            # RT266x exposes several instances (MAIN__IOMUXC, WAKE__IOMUXC,
            # VBAT__IOMUXC) in distinct power domains; single-instance parts
            # (RT10xx/RT11xx) simply report "IOMUXC" here.
            self._mux_periph = periph_name
        elif periph_name == 'IOMUXC_GPR':
            self._has_gpr = True
            self._gpr_reg = addr
            gpr_mask = int(assignment.attrib.get('bit_field_mask'), 0)
            self._gpr_shift = ((gpr_mask) & -(gpr_mask)).bit_length() - 1
            self._gpr_val = int(assignment.attrib.get('bit_field_value'), 0)
        else:
            self._has_extended_config = True
            config = {full_name: assignment.attrib['bit_field_value']}
            self._extended_config.append(config)

    def __repr__(self):
        """
        String representation of object
        """
        if self._has_extended_config:
            return f"IOMUXOpt({self._name}, 0x{self._mux:X} = {self._mux_val}, ExtCfg)"
        elif self._has_gpr:
            return f"IOMUXOpt({self._name}, 0x{self._mux:X} = {self._mux_val}, GPR)"
        return f"IOMUXOpt({self._name}, 0x{self._mux:X} = {self._mux_val})"

    def __hash__(self):
        """
        Override hash method to return the same hash if iomuxc name is the same.
        This means an object with extended configuration has the same hash as
        one without it if they have the same MUX, DAISY, and CFG registers.
        This behavior is desirable because it allows a set of iomuxc registers
        to be generated with unique iomuxc names
        """
        return hash(self._name)

    def __eq__(self, obj):
        """
        Like the hash method, we override the eq method to return true if two
        objects have the same iomuxc name
        """
        return isinstance(obj, IOMUXOption) and self._name == obj._name

    def __lt__(self, obj):
        """
        Compare objects based on name
        """
        if not isinstance(obj, IOMUXOption):
            return True
        return self._name < obj._name

    def get_name(self):
        """
        Get IOMUXC name
        """
        return self._name

    def get_mux_reg(self):
        """
        Get the mux reg for this iomux option
        """
        return self._mux

    def get_mux_periph(self):
        """
        Get the name of the IOMUXC peripheral instance that owns the mux
        register (e.g. "MAIN__IOMUXC", "WAKE__IOMUXC", "VBAT__IOMUXC" on
        RT266x, or "IOMUXC" on single-instance parts).
        """
        return self._mux_periph

    def is_gpio(self):
        """
        return True if this iomux option is for a GPIO
        """
        return self._is_gpio

    def gpio(self):
        """
        Get iomux gpio port and pin as a tuple of (port,pin)
        only valid if is_gpio is True
        """
        return self._gpio

    def get_mux_val(self):
        """
        Get the mux value for this iomux option
        """
        return self._mux_val

    def get_daisy_reg(self):
        """
        Get the daisy reg for this iomux option
        """
        return self._daisy

    def get_daisy_val(self):
        """
        Get the daisy value for this iomux option
        """
        return self._daisy_val

    def get_cfg_reg(self):
        """
        Get the configuration reg for this iomux option
        """
        return self._cfg_reg

    def has_gpr(self):
        """
        Return true if iomux option has associated GPR configuration requirement
        """
        return self._has_gpr

    def gpr_reg(self):
        """
        If has_gpr() is true, return GPR register address
        """
        return self._gpr_reg

    def gpr_shift(self):
        """
        If has_gpr() is true, return shift on GPR register value
        """
        return self._gpr_shift

    def gpr_val(self):
        """
        If has_gpr() is true, return GPR register value
        """
        return self._gpr_val

    def has_extended_config(self):
        """
        Return true if the iomux option requires extended register configuration
        """
        return self._has_extended_config

    def get_extended_config(self):
        """
        Get any extended configuration for this option
        """
        return self._extended_config

    def get_cfg_type(self):
        """
        Get the configuration type for this option. Currently only relevant
        for RT11xx SOCs.
        """
        return self._type


class PinGroup:
    """
    Internal class representing pin group
    """

    def __init__(self, function, signal_map, rt):
        """
        Creates a pin group
        @param function: function xml structure from MEX configuration file
        @param signal_map: signal mapping, maps signal names to signal file
        """
        self._name = function.attrib.get('name')
        description = function.find('mex:description', NAMESPACES)
        pins = function.find('mex:pins', NAMESPACES)
        if description is not None and description.text is not None:
            # Replace <br> html tag with newline
            self._description = description.text.replace("&lt;br/&gt;", "\n")
        else:
            self._description = ""
        # Build dictionary mapping pin properties to pins. This allows us to
        # group pins based on shared configuration
        self._pin_groups = collections.defaultdict(lambda: [])
        for pin in pins:
            signal_name = pin.attrib.get('pin_signal')
            if signal_name not in signal_map:
                logging.warning("Warning: Signal name %s not present in mapping", signal_name)
                # No way to find mux option
                continue
            signal = signal_map[signal_name]
            mux_name = f"{pin.attrib.get('peripheral')}_{pin.attrib.get('signal')}"
            iomuxc_conn = signal.get_iomux_connection(mux_name)
            if iomuxc_conn is None:
                logging.warning("Warning: Signal name %s has no mux", signal_name)
            # determine functional properties for each pin
            defaults = signal.get_pin_defaults()
            # Get pin overrides
            features = pin.find('mex:pin_features', NAMESPACES)
            pin_overrides = {}
            if features is not None:
                for feature in pin.find('mex:pin_features', NAMESPACES):
                    pin_overrides[feature.attrib.get('name')] = feature.attrib.get('value')
            if rt:
                pin_props = self._props_to_dts(pin_overrides, defaults)
            else:
                pin_props = self._imx_props_to_dts(pin_overrides, defaults)
            self._pin_groups[pin_props].append(iomuxc_conn)

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

    def _imx_props_to_dts(self, props, defaults):
        """
        Remap dictionary of property names from NXP defined values to
        Zephyr ones. Valid for iMX8 series parts.
        """
        zephyr_props = []
        prop_mapping = {
            # DSE field name mappings
            'HIZ': 'disabled',
            'OHM_255': '255-ohm',
            'OHM_105': '105-ohm',
            'OHM_75': '75-ohm',
            'OHM_85': '85-ohm',
            'OHM_65': '65-ohm',
            'OHM_45': '45-ohm',
            'OHM_40': '40-ohm',
            # SRE field name mappings
            'SLOW': 'slow',
            'MEDIUM': 'medium',
            'FAST': 'fast',
            'MAX': 'max',
            # FSEL field name mappings
            'Slow': 'slow',
            'Fast': 'fast',
            # Alternative FSEL field name mappings for IMX8MM
            'SLOW0': 'slow',
            'SLOW1': 'slow',
            'FAST0': 'fast',
            'FAST1': 'fast',
            # Alternative DSE field name mappings for IMX8MP
            'X1': 'x1',
            'X2': 'x2',
            'X4': 'x4',
            'X6': 'x6',
            # Alternative DSE field name mappings for IMX8MM
            'X1_0': '255-ohm',
            'X1_1': '255-ohm',
            'X4_0': '105-ohm',
            'X4_1': '105-ohm',
            'X2_0': '85-ohm',
            'X2_1': '85-ohm',
            'X6_0': '40-ohm',
            'X6_1': '40-ohm',
        }

        def sanitize(x):
            return "\"" + prop_mapping[x] + "\"" if (x in prop_mapping) else ""

        # Note if property is not in either dict we fall back to empty string
        def prop_val(x):
            return props[x] if x in props else (defaults.get(x, ""))

        # For each property, append the provided override or the default names
        if prop_val('SION') == 'ENABLED':
            zephyr_props.append('input-enable')
        if prop_val('LVTTL') == 'Enabled':
            zephyr_props.append('nxp,lvttl')
        if prop_val('HYS') == 'Enable' or prop_val('HYS') == 'Schmitt':
            zephyr_props.append('input-schmitt-enable')
        if prop_val('PE') == 'Enabled':
            # If PE is present, pull down resistor will be available.
            if prop_val('PUE') == 'Weak_Pull_Up':
                zephyr_props.append('bias-pull-up')
            elif prop_val('PUE') == 'Weak_Pull_Down':
                zephyr_props.append('bias-pull-down')
        else:
            # Without PE bit, only pull up resistor is available
            if prop_val('PUE') == 'Enabled':
                zephyr_props.append('bias-pull-up')
        if prop_val('ODE') == 'Enabled' or prop_val('ODE') == 'Open_Drain_Enable':
            zephyr_props.append('drive-open-drain')
        if 'SRE' in defaults:
            zephyr_props.append(f"slew-rate = {sanitize(prop_val('SRE'))}")
        if 'FSEL' in defaults:
            zephyr_props.append(f"slew-rate = {sanitize(prop_val('FSEL'))}")
        if 'DSE' in defaults:
            zephyr_props.append(f"drive-strength = {sanitize(prop_val('DSE'))}")

        return tuple(zephyr_props)

    def _props_to_dts(self, props, defaults):
        """
        Remap dictionary of property names from NXP defined values to
        Zephyr ones. Valid for iMX.RT 1xxx series parts.
        @param props: Dictionary of NXP property names and values
        @param defaults: Dictionary of NXP property names and default pin values
        @return array of strings suitable for writing to DTS
        """
        zephyr_props = []
        prop_mapping = {
            'MHZ_50': '50-mhz',
            'MHZ_100_01': '100-mhz',
            # On some iMX RT10xx SOCs, 150 MHz is mapped to this value. However,
            # this is not consistent for all iMX RT10xx supported by
            # config tools. Therefore, we just force both MHZ_100_01 and
            # MHZ_100 to 100-mhz.
            'MHZ_100': '100-mhz',
            'MHZ_200': '200-mhz',
            'R0': 'r0',
            'R0_2': 'r0-2',
            'R0_3': 'r0-3',
            'R0_4': 'r0-4',
            'R0_5': 'r0-5',
            'R0_6': 'r0-6',
            'R0_7': 'r0-7',
            'Pull_Down_100K_Ohm': '100k',
            'Pull_Up_47K_Ohm': '47k',
            'Pull_Up_100K_Ohm': '100k',
            'Pull_Up_22K_Ohm': '22k',
            'Fast': 'fast',
            'Slow': 'slow',
            'Normal': 'normal',
            'High': 'high',
        }

        def sanitize(x):
            return "\"" + prop_mapping[x] + "\"" if (x in prop_mapping) else ""

        # Note if property is not in either dict we fall back to empty string
        def prop_val(x):
            return props[x] if x in props else (defaults.get(x, ""))

        # Check pin defaults and overrides to see if the pin will have a pull or keeper
        if 'pull_keeper_enable' in defaults:
            pull_keeper = prop_val('pull_keeper_enable') == 'Enable'
        else:
            # RT11xx series has no PKE field, pull/keeper is always enabled
            pull_keeper = True
            # config utils maps slow slew rate and fast slew rate incorrectly
            # (slow slew rate should set 0b1 to SRE field). Switch mapping here.
            prop_mapping['Slow'] = 'fast'
            prop_mapping['Fast'] = 'slow'
        if pull_keeper:
            # Determine if pull or keeper is selected
            keeper = prop_val('pull_keeper_select') == 'Keeper'
        else:
            zephyr_props.append('bias-disable')
            keeper = False
        # For each property, append the provided override or the default names
        if prop_val('drive_strength') != '':
            zephyr_props.append(f"drive-strength = {sanitize(prop_val('drive_strength'))}")
        if prop_val('hysteresis_enable') == 'Enable':
            zephyr_props.append('input-schmitt-enable')
        if prop_val('open_drain') == 'Enable':
            zephyr_props.append('drive-open-drain')
        if pull_keeper and not keeper:
            if 'pull_keeper_enable' in defaults:
                if prop_val('pull_up_down_config') == 'Pull_Down_100K_Ohm':
                    # Pull down the pin
                    zephyr_props.append('bias-pull-down')
                    zephyr_props.append(
                        f"bias-pull-down-value = {sanitize(prop_val('pull_up_down_config'))}"
                    )
                else:
                    # Pull up the pin
                    zephyr_props.append('bias-pull-up')
                    zephyr_props.append(
                        f"bias-pull-up-value = {sanitize(prop_val('pull_up_down_config'))}"
                    )
            elif 'pull_up_down_config' in defaults:
                # RT11xx series has no pullup/pulldown value selection,
                # only pull up/ pull down
                if prop_val('pull_up_down_config') == 'Pull_Down':
                    zephyr_props.append('bias-pull-down')
                else:
                    zephyr_props.append('bias-pull-up')
            else:
                # RT1xx series has a second pin register layout, which
                # uses a different property value for pull up and pull down
                if prop_val('pull_down_pull_up_config') == 'Pull_Down':
                    zephyr_props.append('bias-pull-down')
                elif prop_val('pull_down_pull_up_config') == 'Pull_Up':
                    zephyr_props.append('bias-pull-up')
                else:
                    zephyr_props.append('bias-disable')
        if 'slew_rate' in defaults:
            zephyr_props.append(f"slew-rate = {sanitize(prop_val('slew_rate'))}")
        if 'speed' in defaults:
            zephyr_props.append(f"nxp,speed = {sanitize(prop_val('speed'))}")
        if prop_val('software_input_on') == 'Enable':
            zephyr_props.append('input-enable')
        return tuple(zephyr_props)


class NXPSdkUtil:
    """
    Class for iMX.RT configuration file parser for Zephyr
    """

    def __init__(self, cfg_root, copyright_header="", log_level=logging.DEBUG):
        """
        Initialize SDK utilities.
        Providing a signal_configuration.xml file as well as an iomuxc.h file will enable
        the class to parse MEX files and generate output DTS
        @param cfg_root processor configuration folder root
        @param copyright_header: copyright string to add to any generated file header
        """
        # Validate configuration path
        cfg_path = pathlib.Path(cfg_root)
        self._logger = logging.getLogger('')
        self._logger.setLevel(log_level)
        if not cfg_path.is_dir():
            raise RuntimeError("Provided configuration path must be directory")
        # Find all required register and signal definitions
        signal_path = cfg_path / 'signal_configuration.xml'
        register_path = cfg_path / 'registers/registers.xml'
        register_dir = cfg_path / 'registers'
        if not (signal_path.exists() and register_path.exists() and register_dir.is_dir()):
            raise RuntimeError("Required processor configuration files not present")
        try:
            # Load the register xml definition
            register_xml = ET.parse(str(register_path))
            # Load the peripheral definitions
            self._peripheral_map = self._load_peripheral_map(register_xml, register_dir)
        except ET.ParseError as error:
            raise RuntimeError(f"Malformed XML tree in {register_xml}") from error
        try:
            # Try to parse the signal XML file
            signal_file = str(signal_path)
            signal_xml = ET.parse(signal_file)
            # Set SOC name and SKU
            self._soc_sku = signal_xml.find('part_information/part_number').attrib['id']
            self._soc = re.match(r'MIMXR?T?[0-9]+(M\w\d)*', self._soc_sku).group(0)
            reference = signal_xml.find('reference')
            if reference is not None:
                # Signal configuration is stored in reference file, open that
                file_name = reference.get('file')
                file_path = cfg_path.parent / file_name
                if not file_path.exists():
                    raise RuntimeError(
                        "Signal configuration file references "
                        "unknown signal configuration file path",
                        file_path,
                    )
                # Load and parse this signal configuration file
                signal_xml = ET.parse(str(file_path))
            # Load the signal file definition
            self._signal_map = self._load_signal_map(signal_xml)
        except ET.ParseError:
            logging.error("Malformed XML tree %s", signal_file)
            self._signal_map = None
        except OSError:
            logging.error("File %s could not be opened", signal_file)
            self._signal_map = None
        self._copyright = copyright_header

    def get_soc(self):
        """
        Get SOC this class is initialized for
        """
        return self._soc

    def get_part_num(self):
        """
        Get part number this class is initialized for
        """
        return self._soc_sku

    def write_gpio_mux(self, outputfile):
        """
        Write pinctrl definitions for GPIO mux. These definitions map GPIO port
        and pin combinations to iomuxc options. Note that these definitions are
        not indended to be used directly, and will likely need to be hand edited.
        @param outputfile file to write gpio dtsi file to
        """
        # Layered dictionary of gpio mux options. The root keys
        # are the port names, and those port names map to
        # dictionaries of pin->iomux option mappings
        gpio_map = collections.defaultdict(lambda: {})
        # regex to get pin number from gpio mux option
        pin_re = re.compile(r'gpio\d+_io(\d+)|\d+_gpiomux_io(\d\d)|_mux\d_io(\d\d)')
        # Newline pinned to LF; see write_pinctrl_defs.
        with open(outputfile, "w", encoding='utf8', newline='\n') as gpio_dsti:
            # Write header
            gpio_dsti.write(
                f"/*\n"
                f" * File created by {os.path.basename(__main__.__file__)}\n"
                " * not intended for direct usage. Hand edit these DTS\n"
                " * nodes as needed to integrate them into Zephyr.\n"
                " */\n\n"
            )
            for pin in sorted(self._signal_map.values()):
                for iomux_opt in sorted(pin.get_mux_options()):
                    if iomux_opt.is_gpio():
                        gpio = iomux_opt.gpio()
                        if 'CM7' in iomux_opt.get_name():
                            gpio_map[f"{gpio.port}_CM7"][gpio.pin] = iomux_opt
                        else:
                            gpio_map[str(gpio.port)][gpio.pin] = iomux_opt
            # Now write SOC level GPIO pinmux definitions. These are required
            # so that gpio driver is capable of selecting pinmux options when
            # a gpio pin is configured.
            gpio_dsti.write(
                "/*\n"
                " * GPIO pinmux options. These options define the pinmux settings\n"
                " * for GPIO ports on the package, so that the GPIO driver can\n"
                " * select GPIO mux options during GPIO configuration.\n"
                " */\n\n"
            )
            for port in sorted(gpio_map):
                dts_node = f"&gpio{port}{{\n\tpinmux = "
                gpio_gaps = []
                last_pin_num = -1
                for pin in sorted(gpio_map[port]):
                    iomux_opt = gpio_map[port][pin]
                    opt_name = iomux_opt.get_name().lower()
                    opt_match = pin_re.search(opt_name)
                    if opt_match is None:
                        logging.warning("Unmatched gpio pin num %s", opt_name)
                        pin = 0
                    else:
                        # Several different pinmux patterns exist, so choose
                        # the group with a valid string in it
                        if opt_match.group(1):
                            pin = int(opt_match.group(1))
                        elif opt_match.group(2):
                            pin = int(opt_match.group(2))
                        elif opt_match.group(3):
                            pin = int(opt_match.group(3))

                    if (pin - last_pin_num) != 1:
                        # gap in gpio pin number present. Account for this.
                        gpio_gaps.append((last_pin_num + 1, ((pin - last_pin_num) - 1)))
                    dts_node += f"<&{opt_name}>,\n\t\t"
                    last_pin_num = pin
                dts_node = re.sub(r',\n\t\t$', ";\n", dts_node)
                if len(gpio_gaps) != 0:
                    dts_node += "\tgpio-reserved-ranges = "
                    for pair in gpio_gaps:
                        dts_node += f"<{pair[0]} {pair[1]}>, "
                    dts_node = re.sub(r', $', ";\n", dts_node)
                # end group
                dts_node += "};\n\n"

                gpio_dsti.write(dts_node)
            gpio_dsti.close()

    def build_daisy_maps(self):
        """
        Build the IOMUXC select-input (daisy) maps from the register data in the
        configuration data pack.

        A select-input register is a many-pads-to-one-signal multiplexer, so its
        own definition has to enumerate the candidate pads: every DAISY bit field
        value carries a description naming the pad and its ALT mode. That makes
        the register data a second, independent source for the daisy column of a
        pinmux entry - the signal configuration file is the only source for the
        mux mode itself, but not for the daisy value.

        @return dict with:
            'forward': (mux_reg, mux_mode) -> DaisyForward
            'reverse': (daisy_reg, daisy_val) -> DaisyReverse
            'unparsed': list of DAISY descriptions that did not match a known
                        pattern. Callers must treat a non-empty list as fatal:
                        a description format this parser does not understand
                        means the map is silently incomplete.
        """
        forward = {}
        reverse = {}
        unparsed = []
        for periph_name, periph in self._peripheral_map.items():
            if not re.fullmatch(r'(\w+__)?IOMUXC', periph_name):
                continue
            # Pad mux registers: PIO<port>_<pin> on RT266x, SW_MUX_CTL_PAD_<pad>
            # on the RT 4-digit parts.
            mux_addr = {}
            daisy_regs = []
            for reg_name, reg in periph.get_registers().items():
                match = re.fullmatch(r'PIO\d+_\d+|SW_MUX_CTL_PAD_(\w+)', reg_name)
                if match:
                    pad = match.group(1) if match.group(1) else reg_name
                    mux_addr[pad] = periph.get_base() + reg.get_offset()
                if 'DAISY' in reg.get_bit_fields():
                    daisy_regs.append(reg)
            for reg in daisy_regs:
                daisy_addr = periph.get_base() + reg.get_offset()
                for value in reg.get_bit_field_values('DAISY'):
                    desc = reg.get_bit_field_value_description('DAISY', value)
                    pad, alt = _parse_daisy_description(desc)
                    if pad is None:
                        unparsed.append((periph_name, reg.get_name(), desc))
                        continue
                    if pad not in mux_addr:
                        # Pad is not bonded out on this package
                        continue
                    key = (mux_addr[pad], alt)
                    forward[key] = DaisyForward(daisy_addr, value, reg.get_name(), periph_name, pad)
                    reverse[(daisy_addr, value)] = DaisyReverse(
                        mux_addr[pad], alt, pad, periph_name, reg.get_name()
                    )
        return {'forward': forward, 'reverse': reverse, 'unparsed': unparsed}

    def write_pinctrl_defs(self, outputfile, daisy_maps=None):
        """
        Writes a pinctrl dtsi file that defines all pinmux options. The board
        level pin groups will include the pinctrl definitions here, and define
        the properties to be set on each pin.
        @param outputfile file to write pinctrl dtsi file to
        @param daisy_maps result of build_daisy_maps(). When given, mux options
            that the signal configuration file does not list but that a
            select-input candidate list names are emitted as well. Only used for
            RT266x, where the pad configuration register is the mux register, so
            a complete pinmux entry can be derived without the signal file.
        @return list of node names recovered from daisy_maps
        """
        # Newline is pinned to LF: these files are checked in with LF endings,
        # and the platform default would rewrite every line on Windows.
        with open(outputfile, "w", encoding='utf8', newline='\n') as soc_dtsi:
            # Start by writing header
            header = (
                f"/*\n"
                f" * {self._copyright}\n"
                f" *\n"
                f" * Note: File generated by {os.path.basename(__main__.__file__)}\n"
                f" * from configuration data for {self._soc_sku}\n"
                " */\n\n"
            )
            soc_dtsi.write(header)
            # Write documentation block
            soc_dtsi.write(
                "/*\n"
                " * SOC level pinctrl definitions\n"
                " * These definitions define SOC level defaults for each pin,\n"
                " * and select the pinmux for the pin. Pinmux entries are a tuple of:\n"
                " * <mux_register mux_mode input_register input_daisy config_register>\n"
                " * the mux_register and input_daisy reside in the IOMUXC peripheral, and\n"
                " * the pinctrl driver will write the mux_mode and input_daisy values into\n"
                " * each register, respectively. The config_register is used to configure\n"
                " * the pin based on the devicetree properties set\n"
                " */\n\n"
            )
            # RT11xx has multiple types of pin registers, with a variety
            # of register layouts. Define types here.
            soc_rt11xx = re.match(r'MIMXRT11\d+', self._soc) is not None
            # Collect the DTS node text per anchor so each anchor block is
            # emitted once, in a stable order. A part with several IOMUXC
            # instances (RT266x: MAIN__IOMUXC, WAKE__IOMUXC, VBAT__IOMUXC in
            # distinct power domains) needs one devicetree node per instance,
            # so the pinmux definitions are grouped by the IOMUXC peripheral
            # that owns each pin's mux register. Single-instance parts report
            # "IOMUXC" for every pin and collapse onto the legacy &iomuxc.
            anchor_nodes = collections.defaultdict(list)
            emitted = set()
            for pin in sorted(self._signal_map.values()):
                for iomux_opt in sorted(pin.get_mux_options()):
                    # Get iomuxc constant values
                    iomuxc_name = re.sub(r'\[(\d+)\]', r'_\1', iomux_opt.get_name())
                    register = iomux_opt.get_mux_reg()
                    mode = iomux_opt.get_mux_val()
                    input_reg = iomux_opt.get_daisy_reg()
                    input_daisy = iomux_opt.get_daisy_val()
                    config_reg = iomux_opt.get_cfg_reg()
                    emitted.add((register, mode))
                    # build DTS node
                    dts_node = f"\t/omit-if-no-ref/ {iomuxc_name.lower()}: {iomuxc_name} {{\n"
                    dts_node += (
                        f"\t\tpinmux = <0x{register:x} {mode:d} 0x{input_reg:x} "
                        f"{input_daisy:d} 0x{config_reg:x}>;\n"
                    )
                    if soc_rt11xx:
                        # RT11xx pins can have multiple register layouts, so we need to
                        # record the type of pin here
                        reg_type = f"pin-{iomux_opt.get_cfg_type()}"
                        dts_node += f"\t\t{reg_type};\n"
                    if iomux_opt.has_gpr():
                        gpr_reg = iomux_opt.gpr_reg()
                        gpr_shift = iomux_opt.gpr_shift()
                        gpr_val = iomux_opt.gpr_val()
                        # Add GPR configuration
                        dts_node += f"\t\tgpr = <0x{gpr_reg:x} 0x{gpr_shift:x} 0x{gpr_val:x}>;\n"
                    dts_node += "\t};\n"
                    anchor_nodes[self._iomuxc_anchor(iomux_opt.get_mux_periph())].append(dts_node)
            # A pad/ALT pair can be named by a select-input candidate list
            # without appearing in the signal file's <connections> data. The
            # candidate list carries everything a pinmux entry needs (pad, ALT
            # mode, daisy register and value), and on RT266x the pad
            # configuration register is the mux register, so the entry can be
            # completed from the register data alone. Recover those.
            recovered = []
            if daisy_maps and re.match(r'MIMXRT2\d+', self._soc):
                for key, entry in sorted(daisy_maps['reverse'].items()):
                    mux_key = (entry.mux_reg, entry.mux_mode)
                    if mux_key in emitted:
                        continue
                    name = f"{entry.instance}_{entry.pad}_{entry.signal}"
                    node = (
                        f"\t/omit-if-no-ref/ {name.lower()}: {name} {{\n"
                        f"\t\tpinmux = <0x{entry.mux_reg:x} "
                        f"{entry.mux_mode:d} 0x{key[0]:x} {key[1]:d} "
                        f"0x{entry.mux_reg:x}>;\n"
                        "\t};\n"
                    )
                    anchor_nodes[self._iomuxc_anchor(entry.instance)].append(node)
                    emitted.add(mux_key)
                    recovered.append(name)
            # Emit one block per anchor. "&iomuxc" first (legacy / MAIN), then the
            # remaining domain anchors in a stable order so single-instance
            # parts produce byte-identical output to the previous generator.
            for anchor in ["&iomuxc"] + sorted(a for a in anchor_nodes if a != "&iomuxc"):
                if anchor not in anchor_nodes:
                    continue
                soc_dtsi.write(anchor + " {\n")
                for dts_node in anchor_nodes[anchor]:
                    soc_dtsi.write(dts_node)
                soc_dtsi.write("};\n\n")
        return recovered

    @staticmethod
    def _iomuxc_anchor(mux_periph):
        """
        Derive the devicetree anchor for a pin from the name of the IOMUXC
        peripheral instance that owns its mux register.

        The instance name carries the power-domain prefix straight from the
        configuration data (e.g. "MAIN__IOMUXC", "WAKE__IOMUXC",
        "VBAT__IOMUXC" on RT266x). MAIN keeps the legacy "&iomuxc" anchor so
        single-instance parts (whose instance is just "IOMUXC") are unchanged;
        every other domain maps to "&iomuxc_<domain>". No fixed instance list
        or base-address table is baked in, so a new domain is routed
        automatically without a script change.
        """
        if not mux_periph:
            return "&iomuxc"
        match = re.match(r'([A-Z]+)__IOMUXC', mux_periph)
        if not match or match.group(1) == "MAIN":
            return "&iomuxc"
        return "&iomuxc_" + match.group(1).lower()

    def write_pinctrl_groups(self, mexfile, outputfile):
        """
        Write pinctrl groups to disk as a parsed DTS file. Intended for use
        with the output of @ref write_pinctrl_header
        """
        if self._signal_map is None:
            logging.error("Cannot write pinctrl groups without a signal map")
            return
        # Parse the mex file
        pin_groups = self._parse_mex_cfg(mexfile)
        # Start by writing header
        header = (
            f"/*\n"
            f" * {self._copyright}\n"
            f" *\n"
            f" * Note: File generated by {os.path.basename(__main__.__file__)}\n"
            f" * from {os.path.basename(mexfile)}\n"
            " */\n\n"
        )
        # Newline pinned to LF; see write_pinctrl_defs.
        with open(outputfile, "w", encoding="utf8", newline='\n') as dts_file:
            dts_file.write(header)
            include = pinctrl_layout.include_path(self.get_part_num(), ".dtsi")
            dts_file.write(f"#include <{include}>\n\n")
            dts_file.write("&pinctrl {\n")
            for pin_group in pin_groups.values():
                pin_props = pin_group.get_pin_props()
                description = pin_group.get_description()
                # if a description is present, write it
                if description != "":
                    description_lines = description.split("\n")
                    if len(description_lines) == 1:
                        dts_file.write(f"\t/* {description} */\n")
                    else:
                        dts_file.write("\t/*\n")
                        for line in description_lines:
                            dts_file.write(f"\t * {line}\n")
                        dts_file.write("\t */\n")
                # Write pin group name
                name = pin_group.get_name().lower()
                dts_file.write(f"\t{name}: {name} {{\n")
                for idx, pin_prop in enumerate(sorted(pin_props)):
                    group_str = f"\t\tgroup{idx} {{\n"
                    # Write all pin names
                    group_str += "\t\t\tpinmux = "
                    for pin in pin_group.get_pins(pin_prop):
                        group_str += f"<&{pin.get_name().lower()}>,\n\t\t\t\t"
                    # Strip out last 3 tabs and close pin name list
                    group_str = re.sub(r',\n\t\t\t\t$', ';\n', group_str)
                    # Write all pin props
                    for prop in pin_prop:
                        group_str += f"\t\t\t{prop};\n"
                    group_str += "\t\t};\n"
                    dts_file.write(group_str)
                # Write closing brace of pin group
                dts_file.write("\t};\n\n")
            # Write closing brace of pinctrl node
            dts_file.write("};\n\n")

    """
    Private class methods
    """

    def _load_peripheral_map(self, reg_xml, reg_dir):
        """
        Generates a mapping of peripheral names to peripheral objects
        @param reg_xml: XML tree for register file
        @param reg_dir: directory where register definition files are stored
        @return dict mapping peripheral names to base addresses
        """
        periph_map = {}
        for peripheral in reg_xml.findall('peripherals/peripheral'):
            periph_path = reg_dir / peripheral.attrib.get('link')
            if periph_path.exists():
                try:
                    # Build register map for this peripheral
                    periph_map[peripheral.attrib['name']] = Peripheral(peripheral, str(periph_path))
                except ET.ParseError:
                    logging.error("Malformed XML tree in %s, skipping...", periph_path)
                    periph_map[peripheral.attrib['name']] = Peripheral(peripheral)

        return periph_map

    def _generate_pin_overrides(self, pin):
        """
        Create pinctrl dict using the SOC pad and pin XML definition
        populates any selected pin features for the pinctrl dict
        @param signal_pin signal pin object for this connection
        @param pin: SOC pin XML structure
        @return dictionary with pinctrl feature settings
        """
        overrides = {}
        features = pin.find('mex:pin_features', NAMESPACES)
        if features is not None:
            for feature in features:
                # Get bit field value of feature
                name = feature.attrib['name']
                value = feature.attrib['value']
                overrides[name] = value
        return overrides

    def _parse_mex_cfg(self, mexfile):
        """
        Parses mex configuration into pin groups.
        @param mexfile: mex configuration file to parse
        @return parsed pin groups
        """
        pin_groups = {}
        try:
            mex_xml = ET.parse(mexfile)
            is_rt = 'RT' in get_processor_name(mexfile)
            for function in mex_xml.findall(
                'mex:tools/mex:pins/mex:functions_list/mex:function', NAMESPACES
            ):
                group = PinGroup(function, self._signal_map, is_rt)
                pin_groups[group.get_name()] = group
            return pin_groups
        except ET.ParseError:
            logging.error("Error: Could not parse mex file %s", mexfile)
            return None

    def _load_signal_map(self, xml):
        """
        Generates a dictionary with pin names as keys, mapping to
         dictionary with peripheral signal refs. The values in this dictionary
         are IOMUXC options with their default values assigned
        @param xml signal xml object
        """
        # Open signal XML file
        signal_root = xml.getroot()
        # Get the pins element
        pads = signal_root.find('pins')
        # Now, build a mapping in memory between the name of each pin, and
        # the peripheral signal refs
        iomuxc_options = {}
        imx_rt = 'RT' in self._soc
        # i.MX RT266x (RT2660 family) uses a different IOMUXC register layout
        # (PIO<port>_<pin> registers, no SW_PAD_CTL_PAD_* registers). Detect it
        # here so SignalPin can gate the pad-config discovery accordingly.
        imx_rt2 = re.match(r'MIMXRT2\d+', self._soc) is not None
        for pad in pads:
            pad_name = pad.attrib['name']
            # Verify signal pad is configurable
            if len(pad.findall('functional_properties/functional_property')) != 0:
                iomuxc_options[pad_name] = SignalPin(pad, self._peripheral_map, imx_rt, imx_rt2)
        return iomuxc_options


"""
Convenience methods, exposed here because they may be required before
the files required to instantiate an NXPSDKUtil class have been located
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
        logging.error("Malformed XML tree %s", mexfile)
        return None
    except OSError:
        logging.error("File %s could not be opened", mexfile)
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
            raise RuntimeError("Cannot locate processor name in MEX file.")
        return processor.text
    except ET.ParseError:
        logging.error("Malformed XML tree %s", mexfile)
        return None
    except OSError:
        logging.error("File %s could not be opened", mexfile)
        return None


def get_package_name(mexfile):
    """
    Extracts package name from a mex file
    @param mexfile: mex file to parse for package name
    """
    try:
        config_tree = ET.parse(mexfile)
        package = config_tree.getroot().find('mex:common/mex:package', NAMESPACES)
        if package is None:
            raise RuntimeError(
                "Cannot locate package name in MEX file. "
                "Are you using v11 of the MCUXpresso configuration tools?"
            )
        return package.text
    except ET.ParseError:
        logging.error("Malformed XML tree %s", mexfile)
        return None
    except OSError:
        logging.error("File %s could not be opened", mexfile)
        return None
