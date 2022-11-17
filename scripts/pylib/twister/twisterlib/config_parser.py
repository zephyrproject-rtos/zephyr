# vim: set syntax=python ts=4 :
#
# Copyright (c) 2018-2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

import scl
from twisterlib.error import ConfigurationError

class TwisterConfigParser:
    """Class to read testsuite yaml files with semantic checking
    """

    testsuite_valid_keys = {"tags": {"type": "set", "required": False},
                       "type": {"type": "str", "default": "integration"},
                       "extra_args": {"type": "list"},
                       "extra_configs": {"type": "list"},
                       "build_only": {"type": "bool", "default": False},
                       "build_on_all": {"type": "bool", "default": False},
                       "skip": {"type": "bool", "default": False},
                       "slow": {"type": "bool", "default": False},
                       "timeout": {"type": "int", "default": 60},
                       "min_ram": {"type": "int", "default": 8},
                       "modules": {"type": "list", "default": []},
                       "depends_on": {"type": "set"},
                       "min_flash": {"type": "int", "default": 32},
                       "arch_allow": {"type": "set"},
                       "arch_exclude": {"type": "set"},
                       "extra_sections": {"type": "list", "default": []},
                       "integration_platforms": {"type": "list", "default": []},
                       "testcases": {"type": "list", "default": []},
                       "platform_type": {"type": "list", "default": []},
                       "platform_exclude": {"type": "set"},
                       "platform_allow": {"type": "set"},
                       "toolchain_exclude": {"type": "set"},
                       "toolchain_allow": {"type": "set"},
                       "filter": {"type": "str"},
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
        self.data = scl.yaml_load_verify(self.filename, self.schema)

        if 'tests' in self.data:
            self.scenarios = self.data['tests']
        if 'common' in self.data:
            self.common = self.data['common']

    def _cast_value(self, value, typestr):
        if isinstance(value, str):
            v = value.strip()
        if typestr == "str":
            return v

        elif typestr == "float":
            return float(value)

        elif typestr == "int":
            return int(value)

        elif typestr == "bool":
            return value

        elif typestr.startswith("list") and isinstance(value, list):
            return value
        elif typestr.startswith("list") and isinstance(value, str):
            vs = v.split()
            if len(typestr) > 4 and typestr[4] == ":":
                return [self._cast_value(vsi, typestr[5:]) for vsi in vs]
            else:
                return vs

        elif typestr.startswith("set"):
            vs = v.split()
            if len(typestr) > 3 and typestr[3] == ":":
                return {self._cast_value(vsi, typestr[4:]) for vsi in vs}
            else:
                return set(vs)

        elif typestr.startswith("map"):
            return value
        else:
            raise ConfigurationError(
                self.filename, "unknown type '%s'" % value)

    def get_scenario(self, name):
        """Get a dictionary representing the keys/values within a scenario

        @param name The scenario in the .yaml file to retrieve data from
        @return A dictionary containing the scenario key-value pairs with
            type conversion and default values filled in per valid_keys
        """

        d = {}
        for k, v in self.common.items():
            d[k] = v

        for k, v in self.scenarios[name].items():
            if k in d:
                if isinstance(d[k], str):
                    # By default, we just concatenate string values of keys
                    # which appear both in "common" and per-test sections,
                    # but some keys are handled in adhoc way based on their
                    # semantics.
                    if k == "filter":
                        d[k] = "(%s) and (%s)" % (d[k], v)
                    else:
                        d[k] += " " + v
            else:
                d[k] = v

        for k, kinfo in self.testsuite_valid_keys.items():
            if k not in d:
                if "required" in kinfo:
                    required = kinfo["required"]
                else:
                    required = False

                if required:
                    raise ConfigurationError(
                        self.filename,
                        "missing required value for '%s' in test '%s'" %
                        (k, name))
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
                        self.filename, "bad %s value '%s' for key '%s' in name '%s'" %
                                       (kinfo["type"], d[k], k, name))

        return d
