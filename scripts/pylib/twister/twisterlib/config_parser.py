# vim: set syntax=python ts=4 :
#
# Copyright (c) 2018-2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

import copy
import warnings

import scl
from twisterlib.error import ConfigurationError


def extract_fields_from_arg_list(target_fields: set, arg_list: str | list):
    """
    Given a list of "FIELD=VALUE" args, extract values of args with a
    given field name and return the remaining args separately.
    """
    extracted_fields = {f : list() for f in target_fields}
    other_fields = []

    if isinstance(arg_list, str):
        args = arg_list.strip().split()
    else:
        args = arg_list

    for field in args:
        try:
            name, val = field.split("=", 1)
        except ValueError:
            # Can't parse this. Just pass it through
            other_fields.append(field)
            continue

        if name in target_fields:
            extracted_fields[name].append(val.strip('\'"'))
        else:
            # Move to other_fields
            other_fields.append(field)

    return extracted_fields, other_fields

class TwisterConfigParser:
    """Class to read testsuite yaml files with semantic checking
    """

    testsuite_valid_keys = {"tags": {"type": "set", "required": False},
                       "type": {"type": "str", "default": "integration"},
                       "extra_args": {"type": "list"},
                       "extra_configs": {"type": "list"},
                       "extra_conf_files": {"type": "list", "default": []},
                       "extra_overlay_confs" : {"type": "list", "default": []},
                       "extra_dtc_overlay_files": {"type": "list", "default": []},
                       "required_snippets": {"type": "list"},
                       "build_only": {"type": "bool", "default": False},
                       "build_on_all": {"type": "bool", "default": False},
                       "skip": {"type": "bool", "default": False},
                       "slow": {"type": "bool", "default": False},
                       "timeout": {"type": "int", "default": 60},
                       "min_ram": {"type": "int", "default": 16},
                       "modules": {"type": "list", "default": []},
                       "depends_on": {"type": "set"},
                       "min_flash": {"type": "int", "default": 32},
                       "arch_allow": {"type": "set"},
                       "arch_exclude": {"type": "set"},
                       "vendor_allow": {"type": "set"},
                       "vendor_exclude": {"type": "set"},
                       "extra_sections": {"type": "list", "default": []},
                       "integration_platforms": {"type": "list", "default": []},
                       "integration_toolchains": {"type": "list", "default": []},
                       "ignore_faults": {"type": "bool", "default": False },
                       "ignore_qemu_crash": {"type": "bool", "default": False },
                       "testcases": {"type": "list", "default": []},
                       "platform_type": {"type": "list", "default": []},
                       "platform_exclude": {"type": "set"},
                       "platform_allow": {"type": "set"},
                       "platform_key": {"type": "list", "default": []},
                       "simulation_exclude": {"type": "list", "default": []},
                       "toolchain_exclude": {"type": "set"},
                       "toolchain_allow": {"type": "set"},
                       "filter": {"type": "str"},
                       "levels": {"type": "list", "default": []},
                       "harness": {"type": "str", "default": "test"},
                       "harness_config": {"type": "map", "default": {}},
                       "seed": {"type": "int", "default": 0},
                       "sysbuild": {"type": "bool", "default": False}
                       }

    def __init__(self, filename, schema):
        """Instantiate a new TwisterConfigParser object

        @param filename Source .yaml file to read
        """
        self.data = {}
        self.schema = schema
        self.filename = filename
        self.scenarios = {}
        self.common = {}

    def load(self):
        data = scl.yaml_load_verify(self.filename, self.schema)
        self.data = data

        if 'tests' in self.data:
            self.scenarios = self.data['tests']
        if 'common' in self.data:
            self.common = self.data['common']
        return data

    def _cast_value(self, value, typestr):
        if typestr == "str":
            return value.strip()

        elif typestr == "float":
            return float(value)

        elif typestr == "int":
            return int(value)

        elif typestr == "bool":
            return value

        elif typestr.startswith("list"):
            if isinstance(value, list):
                return value
            elif isinstance(value, str):
                value = value.strip()
                return [value] if value else list()
            else:
                raise ValueError

        elif typestr.startswith("set"):
            if isinstance(value, list):
                return set(value)
            elif isinstance(value, str):
                value = value.strip()
                return {value} if value else set()
            else:
                raise ValueError

        elif typestr.startswith("map"):
            return value
        else:
            raise ConfigurationError(self.filename, f"unknown type '{value}'")

    def get_scenario(self, name):
        """Get a dictionary representing the keys/values within a scenario

        @param name The scenario in the .yaml file to retrieve data from
        @return A dictionary containing the scenario key-value pairs with
            type conversion and default values filled in per valid_keys
        """

        # "CONF_FILE", "OVERLAY_CONFIG", and "DTC_OVERLAY_FILE" fields from each
        # of the extra_args lines
        extracted_common = {}
        extracted_testsuite = {}

        d = {}
        for k, v in self.common.items():
            if k == "extra_args":
                # Pull out these fields and leave the rest
                extracted_common, d[k] = extract_fields_from_arg_list(
                    {"CONF_FILE", "OVERLAY_CONFIG", "DTC_OVERLAY_FILE"}, v
                )
            else:
                # Copy common value to avoid mutating it with test specific values below
                d[k] = copy.copy(v)

        for k, v in self.scenarios[name].items():
            if k == "extra_args":
                # Pull out these fields and leave the rest
                extracted_testsuite, v = extract_fields_from_arg_list(
                    {"CONF_FILE", "OVERLAY_CONFIG", "DTC_OVERLAY_FILE"}, v
                )
            if k in d:
                if k == "filter":
                    d[k] = f"({d[k]}) and ({v})"
                elif k not in ("extra_conf_files", "extra_overlay_confs",
                               "extra_dtc_overlay_files"):
                    if isinstance(d[k], str) and isinstance(v, list):
                        d[k] = [d[k]] + v
                    elif isinstance(d[k], list) and isinstance(v, str):
                        d[k] += [v]
                    elif isinstance(d[k], list) and isinstance(v, list):
                        d[k] += v
                    elif isinstance(d[k], str) and isinstance(v, str):
                        # overwrite if type is string, otherwise merge into a list
                        type = self.testsuite_valid_keys[k]["type"]
                        if type == "str":
                            d[k] = v
                        elif type in ("list", "set"):
                            d[k] = [d[k], v]
                        else:
                            raise ValueError
                    else:
                        # replace value if not str/list (e.g. integer)
                        d[k] = v
            else:
                d[k] = v

        # Compile conf files in to a single list. The order to apply them is:
        #  (1) CONF_FILEs extracted from common['extra_args']
        #  (2) common['extra_conf_files']
        #  (3) CONF_FILES extracted from scenarios[name]['extra_args']
        #  (4) scenarios[name]['extra_conf_files']
        d["extra_conf_files"] = \
            extracted_common.get("CONF_FILE", []) + \
            self.common.get("extra_conf_files", []) + \
            extracted_testsuite.get("CONF_FILE", []) + \
            self.scenarios[name].get("extra_conf_files", [])

        # Repeat the above for overlay confs and DTC overlay files
        d["extra_overlay_confs"] = \
            extracted_common.get("OVERLAY_CONFIG", []) + \
            self.common.get("extra_overlay_confs", []) + \
            extracted_testsuite.get("OVERLAY_CONFIG", []) + \
            self.scenarios[name].get("extra_overlay_confs", [])

        d["extra_dtc_overlay_files"] = \
            extracted_common.get("DTC_OVERLAY_FILE", []) + \
            self.common.get("extra_dtc_overlay_files", []) + \
            extracted_testsuite.get("DTC_OVERLAY_FILE", []) + \
            self.scenarios[name].get("extra_dtc_overlay_files", [])

        if any({len(x) > 0 for x in extracted_common.values()}) or \
           any({len(x) > 0 for x in extracted_testsuite.values()}):
            warnings.warn(
                "Do not specify CONF_FILE, OVERLAY_CONFIG, or DTC_OVERLAY_FILE "
                "in extra_args. This feature is deprecated and will soon "
                "result in an error. Use extra_conf_files, extra_overlay_confs "
                "or extra_dtc_overlay_files YAML fields instead",
                DeprecationWarning,
                stacklevel=2
            )

        for k, kinfo in self.testsuite_valid_keys.items():
            if k not in d:
                required = kinfo.get("required", False)

                if required:
                    raise ConfigurationError(
                        self.filename,
                        f"missing required value for '{k}' in test '{name}'"
                    )
                else:
                    if "default" in kinfo:
                        default = kinfo["default"]
                    else:
                        default = self._cast_value("", kinfo["type"])
                    d[k] = default
            else:
                try:
                    d[k] = self._cast_value(d[k], kinfo["type"])
                except ValueError:
                    raise ConfigurationError(
                        self.filename,
                        f"bad {kinfo['type']} value '{d[k]}' for key '{k}' in name '{name}'"
                    ) from None

        return d
