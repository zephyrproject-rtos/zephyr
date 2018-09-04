# Copyright (c) 2018 Linaro Limited
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0

import pprint
import re
import codegen

from string import Template

_device_and_api_init_tmpl = \
    'DEVICE_AND_API_INIT( \\\n' + \
    '\t${device-name}, \\\n' + \
    '\t"${driver-name}", \\\n' + \
    '\t${device-init}, \\\n' + \
    '\t&${device-data}, \\\n' + \
    '\t&${device-config-info}, \\\n' + \
    '\t${device-level}, \\\n' + \
    '\t${device-prio}, \\\n' + \
    '\t&${device-api});'

##
# Aliases for EDTS property paths.
_property_path_aliases = [
    ('reg/0/address/0', 'reg/address'),
    ('reg/0/size/0', 'reg/size'),
]

##
# @brief Get device name
#
# Device name is generated from
# - device compatible
# - bus master address if the device is connected to a bus master
# - device address
# - parent device address if the device does not have a device address
def _device_name(device_id):
    device = codegen.edts().get_device_by_device_id(device_id)
    device_name = device.get_property('compatible/0', None)
    if device_name is None:
        codegen.error("No compatible property for device id '{}'."
                      .format(device_id))

    bus_master_device_id = device.get_property('bus/master', None)
    if bus_master_device_id is not None:
        bus_master_device = codegen.edts().get_device_by_device_id(bus_master_device_id)
        reg = bus_master_device.get_property('reg')
        try:
            # use always the first key to get first address inserted into dict
            # because reg_index may be number or name
            # reg/<reg_index>/address/<address_index> : address
            for reg_index in reg:
                for address_index in reg[reg_index]['address']:
                    bus = reg[reg_index]['address'][address_index]
                    device_name += '_' + hex(bus)[2:].zfill(8)
                    break
                break
        except:
            # this device is missing the register directive
            codegen.error("No bus master register address property for device id '{}'."
                          .format(bus_master_device_id))

    reg = device.get_property('reg', None)
    if reg is None:
        # no reg property - take the reg property of the parent device
        parent_device_id = device.get_property('parent-device', None)
        if parent_device_id:
            parent_device = codegen.edts().get_device_by_device_id(parent_device_id)
            reg = parent_device.get_property(parent_device_id, 'reg', None)
    device_address = None
    if reg is not None:
        try:
            # use always the first key to get first address inserted into dict
            # because reg_index may be number or name
            # reg/<reg_index>/address/<address_index> : address
            for reg_index in reg:
                for address_index in reg[reg_index]['address']:
                    address = reg[reg_index]['address'][address_index]
                    device_address = hex(address)[2:].zfill(8)
                    break
                break
        except:
            # this device is missing the register directive
            pass
    if device_address is None:
        # use label instead of address
        device_address = device.get_property('label','<unknown>')
        # Warn about missing reg property
        codegen.log("No register address property for device id '{}'."
                    .format(device_id), "warning", "\n")
    device_name += '_' + device_address

    device_name = device_name.replace("-", "_"). \
                              replace(",", "_"). \
                              replace(";", "_"). \
                              replace("@", "_"). \
                              replace("#", "_"). \
                              replace("&", "_"). \
                              replace("/", "_"). \
                              lower()
    return device_name

class _DeviceLocalTemplate(Template):
    # pattern is ${<property_path>}
    # never starts with /
    # extend default pattern by '-' '/' ','
    idpattern = r'[_a-z][_a-z0-9\-/,]*'


class _DeviceGlobalTemplate(Template):
    # pattern is ${<device-id>:<property_path>}
    # device ID is the same as node address
    # always starts with /
    # extend default pattern by '-', '@', '/', ':'
    idpattern = r'/[_a-z0-9\-/,@:]*'

##
# @brief Substitude values in device template
#
def _device_template_substitute(template, device_id, preset={}):
    # substitute device local placeholders ${<property_path>}, config, ...
    mapping = {}
    # add preset mapping
    mapping.update(preset)
    # add device properties from device tree
    device = codegen.edts().get_device_by_device_id(device_id)
    mapping.update(device.properties_flattened())
    # add specific device declaration vars/ constants
    mapping['device-name'] = mapping.get('device-name',
                                         _device_name(device_id))
    mapping['driver-name'] = mapping.get('driver-name',
        device.get_property('label').strip('"'))
    mapping['device-data'] = mapping.get('device-data',
        "{}_data".format(mapping['device-name']).lower())
    mapping['device-config-info'] = mapping.get('device-config-info',
        "{}_config".format(mapping['device-name']).lower())
    mapping['device-config-irq'] = mapping.get('device-config-irq',
        "{}_config_irq".format(mapping['device-name']).lower())
    substituted = _DeviceLocalTemplate(template).safe_substitute(mapping)

    # substitute device global placeholders ${<device-id>:<property_path>}
    #
    # we need a second substitude to allow for device indirections
    # ${${<property_path for device id>}:<property_path>}
    mapping = {}
    for device_id in codegen.edts()['devices']:
        path_prefix = device_id + ':'
        device = codegen.edts().get_device_by_device_id(device_id)
        mapping.update(device.properties_flattened(path_prefix))
        # add specific device declaration vars/ constants
        try:
            mapping[path_prefix + 'device-name'] = _device_name(device_id)
            mapping[path_prefix + 'driver-name'] = \
                codegen.edts().device_property(device_id, 'label').strip('"')
        except:
            # will be obvious if any of this is needed, just skip here
            pass

    # add aliases to mapping
    aliases_mapping = {}
    for property_path, property_value in mapping.items():
        for alias_property_path, alias in _property_path_aliases:
            if property_path.endswith(alias_property_path):
                property_path = property_path[:-len(alias_property_path)] \
                                + alias
                aliases_mapping[property_path] = property_value
    mapping.update(aliases_mapping)

    substituted = _DeviceGlobalTemplate(substituted).safe_substitute(mapping)

    return substituted


#
# @return True if device is declared, False otherwise
def device_declare_single(device_config,
                          driver_name,
                          device_init,
                          device_level,
                          device_prio,
                          device_api,
                          device_info,
                          device_defaults = {}):
    device_configured = codegen.config_property(device_config, '<not-set>')

    codegen.outl('\n#ifdef {}\n'.format(device_config))

    device_id = codegen.edts().get_device_id_by_label(driver_name)
    if device_id is None:
        # this should not happen
        raise codegen.Error("Did not find driver name '{}'.".format(driver_name))

    # Presets for mapping this device data to template
    preset = device_defaults
    preset['device-init'] = device_init
    preset['device-level'] = device_level
    preset['device-prio'] = device_prio
    preset['device-api'] = device_api
    preset['device-config'] = device_config
    preset['driver-name'] = driver_name.strip('"')

    #
    # device info
    if device_info:
        device_info = _device_template_substitute(device_info, device_id,
                                                  preset)
        codegen.outl(device_info)
    #
    # device init
    codegen.outl(_device_template_substitute(_device_and_api_init_tmpl,
                                             device_id, preset))

    codegen.outl('')
    codegen.outl('#endif /* {} */\n'.format(device_config))

    return True

##
# @param device_configs
#   A list of configuration variables for device instantiation.
#   (e.g. ['CONFIG_SPI_0', 'CONFIG_SPI_1'])
# @param driver_names
#   A list of driver names for device instantiation. The list shall be ordered
#   as the list of device configs.
#   (e.g. ['SPI_0', 'SPI_1'])
# @param device_inits
#   A list of device initialisation functions or a one single function. The
#   list shall be ordered as the list of device configs.
#   (e.g. 'spi_stm32_init')
# @param device_levels
#   A list of driver initialisation levels or one single level definition. The
#   list shall be ordered as the list of device configs.
#   (e.g. 'PRE_KERNEL_1')
# @param device_prios
#   A list of driver initialisation priorities or one single priority
#   definition. The list shall be ordered as the list of device configs.
#   (e.g. 32)
# @param device_api
#   Identifier of the device api.
#   (e.g. 'spi_stm32_driver_api')
# @param device_info
#   Device info template for device driver config, data and interrupt
#   initialisation.
# @param device_defaults
#   Device default property values. `device_defaults` is a dictionary of
#   property path : property value.
#
def device_declare_multi(device_configs,
                         driver_names,
                         device_inits,
                         device_levels,
                         device_prios,
                         device_api,
                         device_info,
                         device_defaults = {}):
    devices_declared = []
    for i, device_config in enumerate(device_configs):
        driver_name = driver_names[i]
        if isinstance(device_inits, str):
            device_init = device_inits
        else:
            try:
                device_init = device_inits[i]
            except:
                device_init = device_inits
        if isinstance(device_levels, str):
            device_level = device_levels
        else:
            try:
                device_level = device_levels[i]
            except:
                device_level = device_levels
        if isinstance(device_prios, str):
            device_prio = device_prios
        else:
            try:
                device_prio = device_prios[i]
            except:
                device_prio = device_prios

        device_declared = device_declare_single(device_config,
                                                driver_name,
                                                device_init,
                                                device_level,
                                                device_prio,
                                                device_api,
                                                device_info,
                                                device_defaults)
        devices_declared.append(str(device_declared))

    if 'True' not in devices_declared:
        err = "No active device found for {} = {} and {}.".format(
            ', '.join(device_configs), ', '.join(devices_declared),
            ', '.join(driver_names))
        codegen.log(err)
        raise codegen.Error(err)


def _device_generate_struct(type_of_struct, _struct):
    if _struct is None and type_of_struct == 'config':
        return 'static const int ${device-config-info}[] = {};\n'
    elif _struct is None and type_of_struct == 'data':
        return 'static int ${device-data}[] = {};\n'

    struct = ""
    # convert _struct into a list. Struct might have only one element
    if type(_struct) is str:
        _struct = [_struct]
    else:
        _struct = list(_struct)

    if type_of_struct == 'config':
        struct += 'static const struct {} ${{device-config-info}} = {{\n'.format(_struct[0])
    elif type_of_struct == 'data':
        struct += 'static struct {} ${{device-data}} = {{\n'.format(_struct[0])
    else:
        msg("Not expected")

    if len(_struct) > 1:
        struct += _struct[1]

    struct += '};\n\n'
    return struct


def _device_generate_irq_bootstrap(irq_names, irq_flag, irq_func):
    irq_bootstrap_info = \
        '#ifdef {}\n'.format(irq_flag) + \
        'DEVICE_DECLARE(${device-name});\n' + \
        'static void ${device-config-irq}(struct device *dev)\n' + \
        '{\n'
    for index, irq_name in enumerate(irq_names):
        irq_name = irq_names[index]
        irq_num = '${{get_property(\'interrupts/{}/irq\')}}'.format(index)
        irq_prio = '${{get_property(\'interrupts/{}/priority\')}}'.format(index)
        irq_bootstrap_info += \
            '\tIRQ_CONNECT({},\n'.format(irq_num) + \
            '\t\t{},\n'.format(irq_prio)
        if len(irq_names) == 1 and irq_name == '0':
            # Only one irq and no name associated. Keep it simple name
            irq_bootstrap_info += '\t\t{},\n'.format(irq_func)
        else:
            irq_bootstrap_info += '\t\t{}_{},\n'.format(irq_func, irq_name)
        irq_bootstrap_info += \
            '\t\tDEVICE_GET(${device-name}),\n' + \
            '\t\t0);\n' + \
            '\tirq_enable({});\n\n'.format(irq_num)
    irq_bootstrap_info += \
        '}\n' + \
        '#endif /* {} */\n\n'.format(irq_flag)
    return irq_bootstrap_info


def device_declare(compatibles, init_prio_flag, kernel_level, irq_func,
                   init_func, api, data_struct, config_struct):

    config_struct = _device_generate_struct('config', config_struct)
    data_struct = _device_generate_struct('data', data_struct)
    if api is None:
        api = "(*(const int *)0)"

    for device_id in codegen.edts().get_device_ids_by_compatible(compatibles):
        device_info = ""
        device = codegen.edts().get_device_by_device_id(device_id)
        driver_name = device.get_property('label')
        device_config = "CONFIG_{}".format(driver_name.strip('"'))
        interrupts = device.get_property('interrupts', None)
        if interrupts is not None:
            try:
                irq_names = list(interrupts[dev]['name'] for dev in interrupts)
            except:
                irq_names = list(interrupts)

            device_info += _device_generate_irq_bootstrap(
                        irq_names, irq_func['irq_flag'], irq_func['irq_func'])
        device_info += config_struct
        device_info += data_struct

        device_declare_single(device_config,
                              driver_name,
                              init_func,
                              kernel_level,
                              init_prio_flag,
                              api,
                              device_info)
