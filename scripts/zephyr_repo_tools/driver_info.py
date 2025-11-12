#!/usr/bin/env python3
#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

import logging
import re
import sys
from pathlib import Path

logger = logging.getLogger(__name__)


def find_matching_brace(text, open_pos):
    """
    Find the position of the matching closing brace for an opening brace.

    Args:
        text (str): The text to search in
        open_pos (int): Position of the opening brace

    Returns:
        int: Position of the matching closing brace, or -1 if not found
    """
    if text[open_pos] != '{':
        return -1

    brace_level = 1
    pos = open_pos + 1

    while pos < len(text) and brace_level > 0:
        if text[pos] == '{':
            brace_level += 1
        elif text[pos] == '}':
            brace_level -= 1
            if brace_level == 0:
                return pos
        pos += 1

    return -1  # No matching brace found


def extract_api_functions(init_body):
    """
    Extract API function names from initialization body, including fields with sub-structures

    Args:
        init_body (str): The initialization body content

    Returns:
        list: List of API function names
    """
    api_functions = []
    brace_level = 0
    current_field = None

    # Process line by line to handle nested structures
    for line in init_body.split('\n'):
        line = line.strip()
        if not line or line.startswith('/*') or line.startswith('//'):
            continue

        # If we're inside a nested structure and this line doesn't end it, skip
        if brace_level > 0:
            # Count opening and closing braces to track nesting level
            brace_level += line.count('{') - line.count('}')
            continue

        # Look for field assignments
        match = re.search(r'\.(\w+)\s*=', line)
        if match:
            current_field = match.group(1)

            # Check if this line starts a nested structure
            if '{' in line:
                brace_level += line.count('{') - line.count('}')
                # If the braces are balanced, it's a single-line nested structure
                if brace_level == 0:
                    api_functions.append(current_field)
                    current_field = None
                else:
                    # It's a multi-line nested structure, add the field name
                    api_functions.append(current_field)
            else:
                # Simple field assignment
                api_functions.append(current_field)
                current_field = None

    return api_functions


def find_api_structures(content):
    """
    Find API structure declarations in file content

    Args:
        content (str): The file content to search in

    Returns:
        list: List of API structure declarations found
    """
    api_declarations = []

    # Pattern 1: Direct structure declarations (start pattern)
    direct_start_pattern = re.compile(
        r'(?:static\s+)?(?:const\s+)?struct\s+(\w+_api)\s+(\w+)\s*=\s*{', re.MULTILINE
    )

    # Pattern 2: DEVICE_API macro-based declarations (start pattern)
    macro_start_pattern = re.compile(
        r'\s*static\s+DEVICE_API\s*\(\s*([^\),]+)\s*,\s*([^\)]+)\)\s*=\s*{', re.MULTILINE
    )

    content = re.sub(r'\\\n', '\n', content)
    # Find all direct declarations
    for match in direct_start_pattern.finditer(content):
        struct_type = match.group(1)
        var_name = match.group(2)
        start_pos = match.start()

        # Extract driver class by removing '_api' from struct_type
        driver_class = re.sub(r'(?:_driver)?_api$', '', struct_type)

        # Find the ending brace with proper nesting support
        end_pos = find_matching_brace(content, match.end() - 1)
        if end_pos == -1:
            continue  # No matching brace found

        # Extract the full declaration including balanced braces
        full_text = content[start_pos : end_pos + 1]
        init_body = content[match.end() : end_pos]

        # Extract API functions
        api_functions = extract_api_functions(init_body)

        # logging.info(f"decl: {struct_type}")

        # Create declaration info
        decl_info = {
            'struct_type': struct_type,
            'var_name': var_name,
            'driver_class': driver_class,
            'declaration_type': 'direct',
            'full_text': full_text,
            'api_functions': api_functions,
        }

        api_declarations.append(decl_info)

    # Find all macro-based declarations
    for match in macro_start_pattern.finditer(content):
        driver_class = match.group(1)
        var_name = match.group(2)
        start_pos = match.start()
        logging.info(f"driver_class: {driver_class}, var_name: {var_name}")

        # Find the ending brace with proper nesting support
        end_pos = find_matching_brace(content, match.end() - 1)
        if end_pos == -1:
            continue  # No matching brace found

        # Extract the full declaration including balanced braces
        full_text = content[start_pos : end_pos + 1]
        init_body = content[match.end() : end_pos]

        # Construct the full struct type
        struct_type = f"{driver_class}_driver_api"
        logging.info(struct_type)

        # Extract API functions
        api_functions = extract_api_functions(init_body)
        logging.info(f"\n{api_functions}\n")

        logging.info(f"macro: {struct_type}")
        # Create declaration info
        decl_info = {
            'struct_type': struct_type,
            'var_name': var_name,
            'driver_class': driver_class,
            'declaration_type': 'macro',
            'full_text': full_text,
            'api_functions': api_functions,
        }

        api_declarations.append(decl_info)

    return api_declarations if api_declarations else None


def find_device_definitions(content):
    """
    Find DEVICE_DEFINE and related macro usages in file content

    Args:
        content (str): The file content to search in

    Returns:
        list: List of device definitions found
    """
    device_definitions = []

    # Pattern for DEVICE_DEFINE and similar macros
    device_pattern = re.compile(
        r'(?:DEVICE|SPI|I2C|UART)_(?:DT_)?(?:DEFINE|INST)_\d*\s*\(\s*([^,]+)\s*,', re.MULTILINE
    )

    for match in device_pattern.finditer(content):
        device_name = match.group(1).strip()

        device_info = {'device_name': device_name, 'match_text': match.group(0)}

        device_definitions.append(device_info)

    return device_definitions


def find_dt_drv_compat(content):
    """
    Find all DT_DRV_COMPAT definitions in file content

    some unfortunate special cases where the DT_COMPATS are hidden or the driver doesn't
    use DT_DRV_COMPAT but instead direct mapping to a named node or chosen node. But for
    DT_DRV_COMPAT, we would like to attempt to find them.

    Some will have them in a local include file it pulls in.

    Some drivers appear to hardcode the dt node label mapping By using DT_NODELABEL(<label>)
    directly. Counter_ace_v1x_art.c, counter_ace_v1x_rtc.c, and counter_gecko_stimer.c map directly
    to the named node
      - DT_NODELABEL(ace_art_counter)
      - DT_NODELABEL(ace_rtc_counter)
      - DT_CHOSEN(silabs_sleeptimer)


    net/canbus.c uses zephyr_canbus from:

        static const struct net_canbus_config net_canbus_cfg = {
            .can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus))
        };

    counter_ace_v1x_art.c, counter_ace_v1x_rtc.c, and counter_gecko_stimer.c map directly
    to the named node
      - DT_NODELABEL(ace_art_counter)
      - DT_NODELABEL(ace_rtc_counter)
      - DT_CHOSEN(silabs_sleeptimer)

    crypto_mtls_shim.c:
      - Doesn't appear to even have a devicetree binding

    Args:
        content (str): The file content to search in

    Returns:
        list: List of information about each DT_DRV_COMPAT found, empty list if none
    """

    #
    # Most common pattern is just a define of DT_DRV_COMPAT
    #
    compat_pattern = re.compile(r'#define\s+DT_DRV_COMPAT\s+(\w+)', re.MULTILINE)

    compat_matches = []

    for match in compat_pattern.finditer(content):
        compat_value = match.group(1)

        compat_info = {'value': compat_value, 'full_text': match.group(0)}

        compat_matches.append(compat_info)

    if compat_matches == []:
        # Some drivers use DT_FOREACH_STATUS_OKAY(<compat>,<dt_node_path>) directly
        # with a hard coded name
        pattern = re.compile(r'DT_FOREACH_STATUS_OKAY\(\s*([A-Za-z0-9_]+)\s*,', re.S)
        for match in pattern.finditer(content):
            compat_value = match.group(1)

            compat_info = {'value': compat_value, 'full_text': match.group(0)}

            compat_matches.append(compat_info)

    if compat_matches == []:
        # Some drivers use DT_FOREACH_STATUS_OKAY(<compat>,<dt_node_path>) directly with a
        # hard coded name
        pattern = re.compile(r'DT_FOREACH_STATUS_OKAY\(\s*([A-Za-z0-9_]+)\s*,', re.S)
        for match in pattern.finditer(content):
            compat_value = match.group(1)

            compat_info = {'value': compat_value, 'full_text': match.group(0)}

            compat_matches.append(compat_info)

    if compat_matches == []:
        # Special case MCP drivers: adc/adc_mcp320x.c
        pattern = re.compile(r'INST_DT_MCP320X_FOREACH\(\s*([A-Za-z0-9_]+)\s*,', re.S)
        for match in pattern.finditer(content):
            compat_value = "microchip_mcp" + match.group(1)

            compat_info = {'value': compat_value, 'full_text': match.group(0)}

            compat_matches.append(compat_info)

    if compat_matches == []:
        # Special case some DAC drivers:
        # Regex for INST_DT_DACXxxx_FOREACH(...)
        #  - INST_DT_DACX0508_FOREACH(60508, DAC60508_DEVICE)
        #  - INST_DT_DACX3608_FOREACH(43608, DAC43608_DEVICE)
        pattern = re.compile(r'INST_DT_DACX[A-Za-z0-9]+_FOREACH\(\s*([^\s,()]+)\s*,', re.S)

        for match in pattern.finditer(content):
            compat_value = "ti_dac" + match.group(1)

            compat_info = {'value': compat_value, 'full_text': match.group(0)}

            compat_matches.append(compat_info)

    if compat_matches == []:
        # dac_ltc166x.c: INST_DT_LTC166X_FOREACH(1660, LTC1660_DEVICE)
        pattern = re.compile(r'INST_DT_LTC[A-Za-z0-9]+_FOREACH\(\s*([^\s,()]+)\s*,', re.S)

        for match in pattern.finditer(content):
            compat_value = "lltc_ltc" + match.group(1)

            compat_info = {'value': compat_value, 'full_text': match.group(0)}

            compat_matches.append(compat_info)

    return compat_matches


def find_power_management(content):
    """
    Find power management support in file content

    Args:
        content (str): The file content to search in

    Returns:
        dict: Information about power management support if found, None otherwise
    """
    result = {'supported': False, 'macro': None, 'configs': []}

    # Check for PM support - use first match in priority order
    pm_patterns = ["PM_DEVICE_DT_INST_DEFINE", "PM_DEVICE_DT_DEFINE", "PM_DEVICE_DEFINE"]

    for pattern in pm_patterns:
        match = re.search(pattern, content)
        if match:
            # Get the context (the line containing the match)
            line_start = content.rfind('\n', 0, match.start()) + 1
            line_end = content.find('\n', match.end())
            if line_end == -1:  # Handle case where match is on the last line
                line_end = len(content)
            context = content[line_start:line_end].strip()

            result['supported'] = True
            result['macro'] = pattern
            result['context'] = context
            break  # Use first match only

    # Find all CONFIG_PM_xxx references
    pm_config_pattern = re.compile(r'CONFIG_(PM_\w+)')

    # Find all unique PM configurations
    pm_configs = set()
    for match in pm_config_pattern.finditer(content):
        pm_configs.add(match.group(1))

    if pm_configs:
        result['configs'] = sorted(list(pm_configs))

    # Only return the result if we found any PM support
    if result['supported'] or result['configs']:
        return result

    return None


class ZephyrDriver:
    def __init__(self, filepath):
        self.file_path = Path(filepath)
        self.api_structures = []
        self.apis = {}
        self.device_defs = None
        self.dt_drv_compats = None
        self.power_mgmt = None
        try:
            with self.file_path.open('r', errors='replace') as file:
                content = file.read()

                # Find API structures
                self.api_structures = find_api_structures(content)

                if self.api_structures:
                    self.apis = {item: True for item in self.api_structures[0]['api_functions']}

                # Find device definitions
                self.device_defs = find_device_definitions(content)

                # Find power management support
                self.power_mgmt = find_power_management(content)

                # Find DT_DRV_COMPAT definitions
                self.dt_drv_compats = find_dt_drv_compat(content)
                if self.dt_drv_compats == []:
                    # search header filess included by this file to see if
                    # compat is defined there
                    parent_dir = Path(filepath).parent
                    # Regex to capture filename inside quotes
                    pattern = re.compile(r'#include\s+"([^"]+)"')
                    for match in pattern.finditer(content):
                        include_file = parent_dir / match.group(1)
                        # logging.info(f"include: {include_file}")
                        try:
                            with open(include_file) as f:
                                include_data = f.read()
                                self.dt_drv_compats = find_dt_drv_compat(include_data)
                        except FileNotFoundError:
                            logging.info(f"FileNotFoundError: {include_file}")
                        if self.dt_drv_compats != []:
                            logging.info(f"dt_compats: {self.dt_drv_compats}")
                            break

        except Exception as e:
            logging.info(
                f"Error processing file {self.file_path}: {e}",
                file=sys.stderr,
            )

    def __str__(self):
        return (
            f"ZephyrDriver(file: {self.filename}, class: {self.driver_class}, "
            f"DT compat: {self.dt_compats}, PM Status: {self.pm_status})"
        )

    def __repr__(self):
        return self.__str__()

    def load_zephyr_def(self, api_def):
        if len(api_def):
            try:
                self.zephyr_api_def = api_def[0]

                logging.info("----------------------------------------")
                logging.info(f"\napi_def: {self.zephyr_api_def}\n")
                logging.info(f"\ninitial apis: {self.apis}\n")
                logging.info(f"mandatory: {self.zephyr_api_def['mandatory_apis']}")

                self.apis = {
                    item: (item in self.api_structures[0]['api_functions'])
                    for item in self.zephyr_api_def['mandatory_apis']
                }

                # the optional array is actually an array of ["api", "KCONFIG"].
                # Lets extract just the API from the optional and test
                # accordingly
                logging.info(f"optional_apis: {self.zephyr_api_def['optional_apis']}")
                if len(self.zephyr_api_def['optional_apis']):
                    temp_optional = [koption for koption, _ in self.zephyr_api_def['optional_apis']]
                else:
                    temp_optional = []
                self.apis.update(
                    {
                        item: (item in self.api_structures[0]['api_functions'])
                        for item in temp_optional
                    }
                )
                logging.info(
                    f"runtime_optional_apis: {self.zephyr_api_def['runtime_optional_apis']}"
                )
                self.apis.update(
                    {
                        item: (item in self.api_structures[0]['api_functions'])
                        for item in self.zephyr_api_def['runtime_optional_apis']
                    }
                )
            except Exception as e:
                logging.error(f"\nError load_zephyr_def {self.filename}: {e}")
                logging.error(f"\napi_def: {api_def}")
                logging.error(f"\nself.apis: {self.apis}")
                logging.error(f"\nself.api_structure: {self.api_structures}")

    def simple_list(self):
        return {
            "driver_class": self.driver_class,
            "filename": self.filename,
            "dt_compat": (self.dt_compats[0] if self.dt_compats is not None else ''),
            "PM": self.pm_status,
            "api_structure": self.api_struct,
        }

    def api_list(self):
        return {
            "filename": self.filename,
            "driver_class": self.driver_class,
            "dt_compat": (self.dt_compats[0] if self.dt_compats is not None else ''),
            "PM": self.pm_status,
            "api_structure": self.api_struct,
        } | self.api_status

    @property
    def filename(self):
        return self.file_path.name

    @property
    def filepath(self):
        """str | pathlib.Path: Full path to the analyzed file."""
        return self.file_path

    @property
    def driver_class(self):
        """str | None: Driver class extracted from the API structure.

        Notes
        -----
        Returns the ``"driver_class"`` field of the first parsed API structure,
        or ``None`` if unavailable.

        N.B. Not sure if we have any drivers in tree that can be different kinds
             of class?
        """
        return self.api_structures[0]["driver_class"] if self.api_structures else None

    @property
    def api_struct(self):
        """str | None: Struct type for the driver API.

        Notes
        -----
        Returns the ``"struct_type"`` field of the first parsed API structure,
        or ``None`` if unavailable.

        N.B. Not sure if we have any drivers in tree that can be different kinds
             of class?
        """
        return self.api_structures[0]["struct_type"] if self.api_structures else None

    @property
    def api_status(self):
        return self.apis

    @property
    def pm_status(self):
        return self.power_mgmt['supported'] if self.power_mgmt is not None else False

    @property
    def dt_compats(self):
        """list[str] | None: List of DT compat strings or ``None`` if absent."""
        if self.dt_drv_compats:
            return [item['value'] for item in self.dt_drv_compats]
        else:
            return None
