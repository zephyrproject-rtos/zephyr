#!/usr/bin/env python3

# Copyright (c) 2024 Google Inc
# SPDX-License-Identifier: BDS-3-Clause
import io
import yaml
import re
import sys
from collections.abc import Sequence

import pw_sensor.constants_generator as pw_gen


def main() -> None:
    definition = yaml.safe_load(sys.stdin)
    sensors: list[Sensor] = []
    units: dict[str, pw_gen.Units] = {}
    for unit_id, spec in definition["units"].items():
        units[unit_id] = pw_gen.Units(
            unit_id=unit_id,
            definition=pw_gen.create_dataclass_from_dict(pw_gen.UnitsSpec, spec),
        )

    for sensor_id, sensor in definition["sensors"].items():
        sensors.append(Sensor(item_id=sensor_id, spec=sensor, trigger_map=definition["triggers"]))

    print(
        ZephyrHeader(
            channels=[
                Channel(identifier=identifier, definition=value, units=units)
                for identifier, value in definition["channels"].items()
            ],
            attributes=[
                Attribute(identifier, value)
                for identifier, value in definition["attributes"].items()
            ],
            triggers=[
                Trigger(identifier, value)
                for identifier, value in definition["triggers"].items()
            ],
            sensors=sensors,
        )
    )


class Attribute(pw_gen.Attribute):
    """
    A single attribute definition for Zephyr
    """

    def __init__(self, identifier: str, definition: dict) -> None:
        super().__init__(
            attr_id=identifier,
            definition=pw_gen.create_dataclass_from_dict(
                pw_gen.AttributeSpec, definition
            ),
        )

    def print(self, writer: io.TextIOWrapper) -> None:
        writer.write(
            f"""
  /**
   * {self.description}
   */
  SENSOR_ATTR_{self.id.upper()},
"""
        )


class Channel(pw_gen.Channel):
    def __init__(
        self, identifier: str, definition: dict, units: dict[str, pw_gen.Units]
    ) -> None:
        super().__init__(
            channel_id=identifier.upper(),
            definition=pw_gen.create_dataclass_from_dict(
                pw_gen.ChannelSpec, definition
            ),
            units=units,
        )

    def print(self, writer: io.TextIOWrapper) -> None:
        writer.write(
            f"""
  /**
   * {self.description}
   *
   * Units: {self.units.symbol}
   */
  SENSOR_CHAN_{self.id},
"""
        )


class Trigger(pw_gen.Trigger):
    def __init__(self, identifier: str, definition: dict) -> None:
        super().__init__(
            trigger_id=identifier.upper(),
            definition=pw_gen.create_dataclass_from_dict(
                pw_gen.TriggerSpec, definition
            ),
        )

    def print(self, writer: io.TextIOWrapper) -> None:
        writer.write(
            f"""
  /**
   * {self.description}
   */
  SENSOR_TRIG_{self.id},
"""
        )


class Sensor(pw_gen.Sensor):
    def __init__(self, item_id: str, spec: dict, trigger_map: dict) -> None:
        super().__init__(
            item_id=item_id,
            definition=pw_gen.create_dataclass_from_dict(pw_gen.SensorSpec, spec),
        )
        self._spec = spec
        self._trigger_map = trigger_map

    @property
    def compatible_str(self) -> str:
        return self.compatible_org + "," + self.compatible_part

    @property
    def compatible_c_str(self) -> str:
        return re.sub(r"[\-\,]", "_", self.compatible_str)

    @property
    def def_prefix(self) -> str:
        return f"SENSOR_SPEC_{self.compatible_c_str}"

    def _print_channel_spec(self, writer: io.TextIOWrapper) -> None:
        for chan_id, chans in self._spec["channels"].items():
            prefix = f"{self.def_prefix}_CH_{str(chan_id).lower()}"
            writer.write(f"\n#define {prefix}_COUNT {len(chans)}")
            for i, chan in enumerate(chans):
                description = chan['description'].strip().replace('\n', '\\n')
                writer.write(
                    f"""
#define {prefix}_{i}_EXISTS 1
#define {prefix}_{i}_NAME "{chan['name']}"
#define {prefix}_{i}_DESC "{description}"
#define {prefix}_{i}_SPEC {{ \\
    .chan_type = SENSOR_CHAN_{str(chan_id).upper()}, \\
    .chan_idx = {i}, \\
}}"""
                )

    def _print_attribute_spec(self, writer: io.TextIOWrapper) -> None:
        for attr in self._spec["attributes"]:
            prefix = f"{self.def_prefix}_AT_{str(attr['attribute']).lower()}_CH_{str(attr['channel']).lower()}"
            writer.write(f"\n#define {prefix}_EXISTS 1")

    def _print_trigger_spec(self, writer: io.TextIOWrapper) -> None:
        for trig in self._spec["triggers"]:
            prefix = f"{self.def_prefix}_TR{str(trig).lower()}"
            description = self._trigger_map[trig]['description'].strip().replace('\n', '\\n')
            writer.write(
                f"""
#define {prefix}_EXISTS 1
#define {prefix}_NAME "{self._trigger_map[trig]['name']}"
#define {prefix}_DESC "{description}"""
                + '"'
            )

    def __str__(self) -> str:
        writer = io.StringIO()
        writer.write(
            f"""

/*
 * Definitions for {self.compatible_str}
 */
#define {self.def_prefix}_EXISTS 1
#define {self.def_prefix}_CHAN_COUNT {self.chan_count}
#define {self.def_prefix}_ATTR_COUNT {self.attr_count}
#define {self.def_prefix}_TRIG_COUNT {self.trig_count}"""
        )
        self._print_channel_spec(writer=writer)
        self._print_attribute_spec(writer=writer)
        self._print_trigger_spec(writer=writer)
        return writer.getvalue()


CHANNEL_PRIORITY_ORDER = {
    "ACCEL_X": 0,
    "ACCEL_Y": 1,
    "ACCEL_Z": 2,
    "ACCEL_XYZ": 3,
    "GYRO_X": 4,
    "GYRO_Y": 5,
    "GYRO_Z": 6,
    "GYRO_XYZ": 7,
    "MAGN_X": 8,
    "MAGN_Y": 9,
    "MAGN_Z": 10,
    "MAGN_XYZ": 11,
}


class ZephyrHeader:
    def __init__(
        self,
        channels: Sequence[Channel],
        attributes: Sequence[Attribute],
        triggers: Sequence[Trigger],
        sensors: Sequence[Sensor],
    ) -> None:
        self._sensors = sensors
        self._channels = list(channels)
        self._attributes = attributes
        self._triggers = triggers
        assert len(self._channels) == len(set(self._channels))
        assert len(self._attributes) == len(set(self._attributes))
        assert len(self._triggers) == len(set(self._triggers))

        def custom_sort_key(channel):
            if channel.id in CHANNEL_PRIORITY_ORDER:
                return (0, CHANNEL_PRIORITY_ORDER[channel.id])
            else:
                return (1, channel.id)

        self._channels = sorted(channels, key=custom_sort_key)

    def _print_attributes(self, writer: io.TextIOWrapper) -> None:
        writer.write(
            """
/**
 * @brief Sensor attribute types.
 */
enum sensor_attribute {
"""
        )
        for attribute in self._attributes:
            attribute.print(writer)
        writer.write(
            """
  /** Number of all common sensor attributes. */
  SENSOR_ATTR_COMMON_COUNT,

  /**
   * This and higher values are sensor specific.
   * Refer to the sensor header file.
   */
  SENSOR_ATTR_PRIV_START = SENSOR_ATTR_COMMON_COUNT,

  /**
   * Maximum value describing a sensor attribute type.
   */
  SENSOR_ATTR_MAX = INT16_MAX,
};"""
        )

    def _print_channels(self, writer: io.TextIOWrapper) -> None:
        writer.write(
            """
/**
 * @brief Sensor channels.
 */
enum sensor_channel {"""
        )

        for channel in self._channels:
            channel.print(writer=writer)

        writer.write(
            """
  /** All channels */
  SENSOR_CHAN_ALL,

  /** Number of all common sensor channels. */
  SENSOR_CHAN_COMMON_COUNT = (SENSOR_CHAN_ALL + 1),

  /**
   * This and higher values are sensor specific.
   * Refer to the sensor header file.
   */
  SENSOR_CHAN_PRIV_START = SENSOR_CHAN_COMMON_COUNT,

  /**
   * Maximum value describing a sensor channel type.
   */
  SENSOR_CHAN_MAX = INT16_MAX,
};"""
        )

    def _print_triggers(self, writer: io.TextIOWrapper) -> None:
        writer.write(
            """
/**
 * @brief Sensor trigger types.
 */
enum sensor_trigger_type {
"""
        )
        for trigger in self._triggers:
            trigger.print(writer)

        writer.write(
            """
  /** Number of all common sensor triggers. */
  SENSOR_TRIG_COMMON_COUNT,

  /**
   * This and higher values are sensor specific.
   * Refer to the sensor header file.
   */
  SENSOR_TRIG_PRIV_START = SENSOR_TRIG_COMMON_COUNT,

  /**
   * Maximum value describing a sensor trigger type.
   */
  SENSOR_TRIG_MAX = INT16_MAX,
};"""
        )

    def _print_sensor_compatible_foreach(self, writer: io.TextIOWrapper) -> None:
        if not self._sensors:
            return
        writer.write("\n#define SENSOR_SPEC_FOREACH_COMPAT(fn)")
        for sensor in self._sensors:
            writer.write(f" \\\n    fn({sensor.compatible_c_str})")

    def __str__(self) -> str:
        writer = io.StringIO()
        writer.write(
            """/* Auto-generated file, do not edit */
#ifndef _INCLUDE_ZEPHYR_GENERATED_SENSOR_CONSTANTS_H_
#define _INCLUDE_ZEPHYR_GENERATED_SENSOR_CONSTANTS_H_

/**
 * @addtogroup sensor_interface
 * @{{
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
"""
        )
        self._print_channels(writer=writer)
        self._print_attributes(writer=writer)
        self._print_triggers(writer=writer)

        writer.write(
            """

/**
 * @brief Sensor Channel Specification
 *
 * A sensor channel specification is a unique identifier per sensor device describing
 * a measurement channel.
 *
 * @note Typically passed by value as the size of a sensor_chan_spec is a single word.
 */
struct sensor_chan_spec {
	uint16_t chan_type; /**< A sensor channel type */
	uint16_t chan_idx;  /**< A sensor channel index */
};
"""
        )
        self._print_sensor_compatible_foreach(writer=writer)
        for sensor in self._sensors:
            writer.write(str(sensor))
        writer.write(
            """
#ifdef __cplusplus
}
#endif

/**
 * @}}
 */
#endif /* _INCLUDE_ZEPHYR_GENERATED_SENSOR_CONSTANTS_H_ */
"""
        )
        return writer.getvalue()


if __name__ == "__main__":
    main()
