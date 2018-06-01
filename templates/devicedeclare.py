# Copyright (c) 2018 Linaro Limited
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0

import pprint
import re
import cogeno

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
    device = cogeno.edts().get_device_by_device_id(device_id)
    # substitute device local placeholders ${<property_path>}, config, ...
    mapping = {}
    # add preset mapping
    mapping.update(preset)
    # add device properties from device tree
    mapping.update(device.get_properties_flattened())
    # add specific device declaration vars/ constants
    mapping['device-name'] = mapping.get('device-name', device.get_name())
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
    for device_id in cogeno.edts()['devices']:
        path_prefix = device_id + ':'
        device = cogeno.edts().get_device_by_device_id(device_id)
        mapping.update(device.get_properties_flattened(path_prefix))
        # add specific device declaration vars/ constants
        try:
            mapping[path_prefix + 'device-name'] = device.get_name()
            mapping[path_prefix + 'driver-name'] = \
                device.get_property('label').strip('"')
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
    device_configured = cogeno.config_property(device_config, '<not-set>')
    if device_configured == '<not-set>' or device_configured[-1] == '0':
        # Not configured - do not generate
        #
        # The generation decision must be taken by cogen here
        # (vs. #define CONFIG_xxx) to cope with the following situation:
        #
        # If config is not set the device may also be not activated in the
        # device tree. No device tree info is available in this case.
        # An attempt to generate code without the DTS info
        # will lead to an exception for a valid situation.
        cogeno.outl("/* !!! '{}' not configured !!! */".format(driver_name))
        return False

    device_id = cogeno.edts().get_device_id_by_label(driver_name)
    if device_id is None:
        # this should not happen
        raise cogeno.Error("Did not find driver name '{}'.".format(driver_name))

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
        cogeno.outl(device_info)
    #
    # device init
    cogeno.outl(_device_template_substitute(_device_and_api_init_tmpl,
                                             device_id, preset))
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
        cogeno.log(err)
        raise cogeno.Error(err)


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
    for irq_name in irq_names:
        irq_num = '${{interrupts/{}/irq}}'.format(irq_name)
        irq_prio = '${{interrupts/{}/priority}}'.format(irq_name)
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

    for device_id in cogeno.edts().get_device_ids_by_compatible(compatibles):
        device = cogeno.edts().get_device_by_device_id(device_id)
        driver_name = device.get_property('label')
        device_config = "CONFIG_{}".format(driver_name.strip('"'))
        interrupts = device.get_property('interrupts', None)
        if interrupts is not None:
            irq_names = list(interrupts.keys())
        else:
            irq_names = None

        device_info = ""
        if irq_func is not None:
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
