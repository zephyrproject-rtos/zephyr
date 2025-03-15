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


def _sort_channel_keys(s: str) -> tuple[str, int]:
    """
    Sort channel keys alphabetically except:
        Items ending in _XYZ or _DXYZ should come after [_X, _Y, _Z] and
        [_DX, _DY, _DZ] respectively.
    Args:
        s: String to sort
    Returns:
        A tuple with the substring to use for further sorting and a sorting
        priority.
    """
    text = s.upper()
    if text.endswith("_XYZ"):
        return (s[:-3], 1)  # Sort XYZ after X, Y, Z
    elif text.endswith("_DXYZ"):
        return (s[:-4], 1)  # Sort DXYZ after DX, DY, DZ
    elif text.endswith(("_X", "_Y", "_Z")):
        return (s[:-1], 0)  # Sort single/double letter suffixes before XYZ
    elif text.endswith(("_DX", "_DY", "_DZ")):
        return (s[:-2], 0)  # Sort single/double letter suffixes before DXYZ
    else:
        return (s, 2)  # Keep other strings unchanged


def sort_sensor_channels(chan_list):
    """
    Jinja sort function for sensor channel IDs with a preference for _x, _y, _z, and _xyz suffixes.

    Args:
        chan_list: List of channel ID keys

    Returns:
        Sorted list of the channel ID keys
    """
    new_list = list(chan_list)
    new_list.sort(key=_sort_channel_keys)
    # Special handling for the ALL channel
    if "all" in new_list:
        new_list.remove("all")
        new_list.append("all")
    return new_list


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
    env.globals["sort_sensor_channels"] = sort_sensor_channels
    env.filters["enumerate"] = enumerate_filter

    template = env.get_template("sensor_constants.h.j2")
    print(template.render({"spec": spec}))


if __name__ == "__main__":
    main()
