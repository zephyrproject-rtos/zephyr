#!/usr/bin/env python3

# Copyright (c) 2024 Google Inc
# SPDX-License-Identifier: Apache-2.0
import os
import re
import sys

import pw_sensor.constants_generator as pw_sensor
import yaml
from jinja2 import Environment, FileSystemLoader


def getSensorCompatibleString(sensor: pw_sensor.SensorSpec) -> str:
    """
    Concatenates the compatible org and part, replacing ',' and '-' with '_'.

    Args:
        sensor: The sensor object with compatible.org and compatible.part attributes.

    Returns:
        A string with the formatted compatible string.
    """
    return re.sub(r"[\-\,]", "_", f"{sensor.compatible.org}_{sensor.compatible.part}")


def formatStringForC(string: None | str) -> str:
    """
    Format a string (or None) to work inside a C string literal.

    Args:
        string: The string to format (may be None)

    Returns:
        A string that can be placed inside a C string.
    """
    if string:
        return string.strip().replace("\n", "\\n")
    return ""


def enumerate_filter(iterable):
    """
    Jinja filter used to enumerate an iterable

    Args:
        iterable: Any iterable object

    Returns:
        An enumeration of the iterable
    """
    return enumerate(iterable)


def main() -> None:
    definition = yaml.safe_load(sys.stdin)
    spec: pw_sensor.InputSpec = pw_sensor.create_dataclass_from_dict(
        cls=pw_sensor.InputSpec,
        data=definition,
    )
    script_dir = os.path.dirname(os.path.abspath(__file__))
    env = Environment(loader=FileSystemLoader(script_dir))
    env.globals["getSensorCompatibleString"] = getSensorCompatibleString
    env.globals["formatStringForC"] = formatStringForC
    env.filters["enumerate"] = enumerate_filter

    template = env.get_template("sensor_constants.h.j2")
    print(template.render({"spec": spec}))


if __name__ == "__main__":
    main()
