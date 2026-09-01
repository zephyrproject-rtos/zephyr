# vim: set syntax=python ts=4 :
#
# Copyright (c) 2018-2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

import copy
import warnings
from typing import Any

import scl
from twisterlib.error import ConfigurationError


def extract_fields_from_arg_list(
    target_fields: set, arg_list: str | list
 ) -> tuple[dict[str, list[str]], list[str]]:
    """
    Given a list of "FIELD=VALUE" args, extract values of args with a
    given field name and return the remaining args separately.
    """
    extracted_fields: dict[str, list[str]] = {f: list() for f in target_fields}
    other_fields: list[str] = []

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

    testsuite_valid_keys: dict[str, dict[str, Any]] = {
        "tags": {"type": "set", "required": False},
        "type": {"type": "str", "default": "integration"},
        "extra_args": {"type": "list"},
        "extra_configs": {"type": "list"},
        "conf_files": {"type": "list", "default": []},
        "extra_conf_files": {"type": "list", "default": []},
        "extra_overlay_confs": {"type": "list", "default": []},
        "extra_dtc_overlay_files": {"type": "list", "default": []},
        "required_applications": {"type": "list"},
        "required_snippets": {"type": "list"},
        "build": {"type": "bool", "default": True},
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
        "ignore_faults": {"type": "bool", "default": False},
        "ignore_qemu_crash": {"type": "bool", "default": False},
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
        "sidecar": {"type": "str", "default": None},
        "seed": {"type": "int", "default": 0},
        "sysbuild": {"type": "bool", "default": False}
    }

    # Keys whose list/set members each name a single identifier: a tag, a
    # platform, a board feature, a toolchain. Whitespace inside one of these is
    # always a mistake. Either it is the space-separated list syntax dropped in
    # commit 714e1933401, which is no longer split and so collapses into one
    # unusable value, or it is a multi-word value that can never be matched.
    #
    # extra_args and extra_configs are deliberately absent: they carry Kconfig
    # and CMake values that legitimately contain spaces. The conf file and
    # overlay keys are absent because a path may contain a space.
    keys_without_whitespace: set[str] = {
        "arch_allow",
        "arch_exclude",
        "depends_on",
        "extra_sections",
        "integration_platforms",
        "integration_toolchains",
        "levels",
        "modules",
        "platform_allow",
        "platform_exclude",
        "platform_key",
        "platform_type",
        "required_snippets",
        "simulation_exclude",
        "tags",
        "testcases",
        "toolchain_allow",
        "toolchain_exclude",
        "vendor_allow",
        "vendor_exclude",
    }

    def __init__(self, filename: str, schema: dict[str, Any]) -> None:
        """Instantiate a new TwisterConfigParser object

        @param filename Source .yaml file to read
        """
        self.schema = schema
        self.filename = filename
        self.data: dict[str, Any] = {}
        self.scenarios: dict[str, Any] = {}
        self.common: dict[str, Any] = {}

    def load(self) -> dict[str, Any]:
        data = scl.yaml_load_verify(self.filename, self.schema)
        self.data = data

        if 'tests' in self.data:
            self.scenarios = self.data['tests']
        if 'common' in self.data:
            self.common = self.data['common']
        return data

    def _check_no_whitespace(self, key: str, value: Any, name: str) -> None:
        """Reject whitespace inside a value that has to name a single item.

        Twister no longer splits space-separated lists, so "a b" is taken
        verbatim and silently matches nothing. Fail loudly instead.
        """
        if key not in self.keys_without_whitespace:
            return

        items = [value] if isinstance(value, str) else value
        if not isinstance(items, list):
            return

        for item in items:
            if isinstance(item, str) and len(item.split()) > 1:
                raise ConfigurationError(
                    self.filename,
                    f"whitespace in '{key}' value '{item}' in test '{name}'. "
                    "Space-separated lists are not supported, write the value "
                    "as a YAML list instead"
                )

    def _cast_value(self, value: Any, typestr: str) -> Any:
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

    def get_scenario(self, name: str) -> dict[str, Any]:
        """Get a dictionary representing the keys/values within a scenario

        @param name The scenario in the .yaml file to retrieve data from
        @return A dictionary containing the scenario key-value pairs with
            type conversion and default values filled in per valid_keys
        """

        # "CONF_FILE", "OVERLAY_CONFIG", and "DTC_OVERLAY_FILE" fields from each
        # of the extra_args lines
        extracted_common: dict = {}
        extracted_testsuite: dict = {}

        d: dict[str, Any] = {}
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

        # Compile extra conf files intended for EXTRA_CONF_FILE into a single list.
        # The order to apply them is:
        #  (1) common['extra_conf_files']
        #  (2) scenarios[name]['extra_conf_files']
        d["extra_conf_files"] = \
            self.common.get("extra_conf_files", []) + \
            self.scenarios[name].get("extra_conf_files", [])

        # Compile conf files intended for CONF_FILE into a single list.
        # The order to apply them is:
        #  (1) CONF_FILEs extracted from common['extra_args']
        #  (2) CONF_FILES extracted from scenarios[name]['extra_args']
        d["conf_files"] = \
            extracted_common.get("CONF_FILE", []) + \
            extracted_testsuite.get("CONF_FILE", [])

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
                self._check_no_whitespace(k, d[k], name)
                try:
                    d[k] = self._cast_value(d[k], kinfo["type"])
                except ValueError:
                    raise ConfigurationError(
                        self.filename,
                        f"bad {kinfo['type']} value '{d[k]}' for key '{k}' in name '{name}'"
                    ) from None

        return d
