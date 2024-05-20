#!/usr/bin/env python3

# Copyright (c) 2024 Google Inc
# SPDX-License-Identifier: BDS-3-Clause
import io
import yaml
import sys
from collections.abc import Sequence


_CHANNEL_BACKWARDS_COMPATIBILITY_MAP: list[str] = [
    "ACCEL_X",
    "ACCEL_Y",
    "ACCEL_Z",
    "ACCEL_XYZ",
    "GYRO_X",
    "GYRO_Y",
    "GYRO_Z",
    "GYRO_XYZ",
    "MAGN_X",
    "MAGN_Y",
    "MAGN_Z",
    "MAGN_XYZ",
    "DIE_TEMP",
    "AMBIENT_TEMP",
    "PRESS",
    "PROX",
    "HUMIDITY",
    "LIGHT",
    "IR",
    "RED",
    "GREEN",
    "BLUE",
    "ALTITUDE",
    "PM_1_0",
    "PM_2_5",
    "PM_10",
    "DISTANCE",
    "CO2",
    "O2",
    "VOC",
    "GAS_RES",
    "VOLTAGE",
    "VSHUNT",
    "CURRENT",
    "POWER",
    "RESISTANCE",
    "ROTATION",
    "POS_DX",
    "POS_DY",
    "POS_DZ",
    "RPM",
    "GAUGE_VOLTAGE",
    "GAUGE_AVG_CURRENT",
    "GAUGE_STDBY_CURRENT",
    "GAUGE_MAX_LOAD_CURRENT",
    "GAUGE_TEMP",
    "GAUGE_STATE_OF_CHARGE",
    "GAUGE_FULL_CHARGE_CAPACITY",
    "GAUGE_REMAINING_CHARGE_CAPACITY",
    "GAUGE_NOM_AVAIL_CAPACITY",
    "GAUGE_FULL_AVAIL_CAPACITY",
    "GAUGE_AVG_POWER",
    "GAUGE_STATE_OF_HEALTH",
    "GAUGE_TIME_TO_EMPTY",
    "GAUGE_TIME_TO_FULL",
    "GAUGE_CYCLE_COUNT",
    "GAUGE_DESIGN_VOLTAGE",
    "GAUGE_DESIRED_VOLTAGE",
    "GAUGE_DESIRED_CHARGING_CURRENT",
]

_ATTRIBUTE_BACKWARDS_COMPATIBILITY_MAP: list[str] = [
    "SAMPLING_FREQUENCY",
    "LOWER_THRESH",
    "UPPER_THRESH",
    "SLOPE_TH",
    "SLOPE_DUR",
    "HYSTERESIS",
    "OVERSAMPLING",
    "FULL_SCALE",
    "OFFSET",
    "CALIB_TARGET",
    "CONFIGURATION",
    "CALIBRATION",
    "FEATURE_MASK",
    "ALERT",
    "FF_DUR",
    "BATCH_DURATION",
]

_TRIGGER_BACKWARDS_COMPATIBILITY_MAP:  list[str] = [
    "TIMER",
    "DATA_READY",
    "DELTA",
    "NEAR_FAR",
    "THRESHOLD",
    "TAP",
    "DOUBLE_TAP",
    "FREEFALL",
    "MOTION",
    "STATIONARY",
    "FIFO_WATERMARK",
    "FIFO_FULL",
    "COMMON_COUNT",
]


def main() -> None:
    definition = yaml.safe_load(sys.stdin)
    sensors: list[Sensor] = []

    for _, sensor in definition["sensors"].items():
        sensors.append(Sensor(spec=sensor))

    print(
        ZephyrHeader(
            channels=[Channel(identifier, value) for identifier, value in definition["channels"].items()],
            attributes=[Attribute(identifier, value) for identifier, value in definition["attributes"].items()],
            triggers=[Trigger(identifier, value) for identifier, value in definition["triggers"].items()],
            sensors=sensors,
        )
    )


class Attribute:
    _attribute_count: int = len(_ATTRIBUTE_BACKWARDS_COMPATIBILITY_MAP)
    def __init__(self, identifier: str, definition: dict) -> None:
        self._identifier: str = identifier.upper()
        self._name: str = definition["name"]
        self._description: str = definition["description"]
        self._units_name: str = definition["units"]["name"]
        self._units_symbol: str = definition["units"]["symbol"]
        try:
            self.value = _ATTRIBUTE_BACKWARDS_COMPATIBILITY_MAP.index(self._identifier)
        except ValueError:
            self.value = Attribute._attribute_count
            Attribute._attribute_count += 1

    @staticmethod
    def tail_str() -> str:
        writer = io.StringIO()
        writer.write(f"""
/** Number of all common sensor attributes. */
#define SENSOR_ATTR_COMMON_COUNT {Attribute._attribute_count}

/**
 * This and higher values are sensor specific.
 * Refer to the sensor header file.
 */
#define SENSOR_ATTR_PRIV_START SENSOR_ATTR_COMMON_COUNT

/**
 * Maximum value describing a sensor attribute type.
 */
#define SENSOR_ATTR_MAX INT16_MAX
""")
        return writer.getvalue()

    def __str__(self) -> str:
        writer = io.StringIO()
        writer.write(f"""
/**
 * {self._description}
 *
 * Units: {self._units_name} ({self._units_symbol})
 */
#define SENSOR_ATTR_{self._identifier} {self.value}
""")
        return writer.getvalue()

    def __hash__(self) -> int:
        return hash((self._identifier, self._name, self._description, self._units_name, self._units_symbol))


class Channel:
    _channel_count: int = len(_CHANNEL_BACKWARDS_COMPATIBILITY_MAP)
    def __init__(self, identifier: str, definition: dict) -> None:
        self._identifier: str = identifier.upper()
        self._name: str = definition["name"]
        self._description: str = definition["description"]
        self._units_name: str = definition["units"]["name"]
        self._units_symbol: str = definition["units"]["symbol"]
        try:
            self.value = _CHANNEL_BACKWARDS_COMPATIBILITY_MAP.index(self._identifier)
        except ValueError:
            self.value = Channel._channel_count
            Channel._channel_count += 1

    @staticmethod
    def tail_str() -> str:
        writer = io.StringIO()
        writer.write(f"""
/** All channels */
#define SENSOR_CHAN_ALL {Channel._channel_count}

/** Number of all common sensor channels. */
#define SENSOR_CHAN_COMMON_COUNT (SENSOR_CHAN_ALL + 1)

/**
 * This and higher values are sensor specific.
 * Refer to the sensor header file.
 */
#define SENSOR_CHAN_PRIV_START SENSOR_CHAN_COMMON_COUNT

/**
 * Maximum value describing a sensor channel type.
 */
#define SENSOR_CHAN_MAX INT16_MAX
""")
        return writer.getvalue()

    def __str__(self) -> str:
        writer = io.StringIO()
        writer.write(f"""
/**
 * {self._description}
 *
 * Units: {self._units_name} ({self._units_symbol})
 */
#define SENSOR_CHAN_{self._identifier} {self.value}
""")
        return writer.getvalue()

    def __hash__(self) -> int:
        return hash((self._identifier, self._name, self._description, self._units_name, self._units_symbol))


class Trigger:
    _trigger_count: int = len(_TRIGGER_BACKWARDS_COMPATIBILITY_MAP)
    def __init__(self, identifier: str, definition: dict) -> None:
        self._identifier: str = identifier.upper()
        self._name: str = definition["name"]
        self._description: str = definition["description"]
        self.value: int
        try:
            self.value = _TRIGGER_BACKWARDS_COMPATIBILITY_MAP.index(self._identifier)
        except ValueError:
            self.value = Trigger._trigger_count
            Trigger._trigger_count += 1

    @staticmethod
    def tail_str() -> str:
        writer = io.StringIO()
        writer.write(f"""
/** Number of all common sensor triggers. */
#define SENSOR_TRIG_COMMON_COUNT {Trigger._trigger_count}

/**
 * This and higher values are sensor specific.
 * Refer to the sensor header file.
 */
#define SENSOR_TRIG_PRIV_START SENSOR_TRIG_COMMON_COUNT

/**
 * Maximum value describing a sensor trigger type.
 */
#define SENSOR_TRIG_MAX INT16_MAX
""")
        return writer.getvalue()

    def __str__(self) -> str:
        writer = io.StringIO()
        writer.write(f"""
/**
 * {self._description}
 */
#define SENSOR_TRIG_{self._identifier} {self.value}
""")
        return writer.getvalue()

    def __hash__(self) -> int:
        return hash((self._identifier, self._name, self._description))

class Sensor:
    def __init__(self, spec: dict) -> None:
        self._spec = spec

    @property
    def name(self) -> str:
        return self._spec["compatible"]["org"] + "_" + self._spec["compatible"]["part"]

    @property
    def def_prefix(self) -> str:
        return f"SENSOR_SPEC_{self.name}"

    @property
    def chan_count(self) -> int:
        return len(self._spec["channels"])

    @property
    def attr_count(self) -> int:
        return len(self._spec["attributes"])

    @property
    def trig_count(self) -> int:
        return len(self._spec["triggers"])

    def _print_channel_spec(self, writer: io.TextIOWrapper) -> None:
        for chan_id, chan in self._spec["channels"].items():
            prefix = f"{self.def_prefix}_CHAN_SENSOR_CHAN_{str(chan_id).upper()}"
            writer.write(
                f"#define {prefix}_EXISTS 1\n"
                f"#define {prefix}_NAME \"{chan['name']}\"\n"
                f"#define {prefix}_DESC \"{chan['description']}\"\n"
                f"#define {prefix}_UNITS_NAME \"{chan['units']['name']}\"\n"
                f"#define {prefix}_UNITS_SYMBOL \"{chan['units']['symbol']}\"\n"
            )

    def _print_attribute_spec(self, writer: io.TextIOWrapper) -> None:
        for attr_id, attr in self._spec["attributes"].items():
            prefix = f"{self.def_prefix}_ATTR_SENSOR_ATTR_{str(attr_id).upper()}"
            writer.write(
                f"#define {prefix}_EXISTS 1\n"
                f"#define {prefix}_NAME \"{attr['name']}\"\n"
                f"#define {prefix}_DESC \"{attr['description']}\"\n"
                f"#define {prefix}_UNITS_NAME \"{attr['units']['name']}\"\n"
                f"#define {prefix}_UNITS_SYMBOL \"{attr['units']['symbol']}\"\n"
            )

    def _print_trigger_spec(self, writer: io.TextIOWrapper) -> None:
        for trig_id, trig in self._spec["triggers"].items():
            prefix = f"{self.def_prefix}_TRIG_SENSOR_TRIG_{str(trig_id).upper()}"
            writer.write(
                f"#define {prefix}_EXISTS 1\n"
                f"#define {prefix}_NAME \"{trig['name']}\"\n"
                f"#define {prefix}_DESC \"{trig['description']}\"\n"
            )

    def __str__(self) -> str:
        writer = io.StringIO()
        writer.write(f"""/*
 * Definitions for {self.name}
 */
#define {self.def_prefix}_EXISTS 1
#define {self.def_prefix}_CHAN_COUNT {self.chan_count}
#define {self.def_prefix}_ATTR_COUNT {self.attr_count}
#define {self.def_prefix}_TRIG_COUNT {self.trig_count}
""")
        self._print_channel_spec(writer=writer)
        self._print_attribute_spec(writer=writer)
        self._print_trigger_spec(writer=writer)
        return writer.getvalue()


class ZephyrHeader:
    def __init__(self, channels: Sequence[Channel], attributes: Sequence[Attribute], triggers: Sequence[Trigger], sensors: Sequence[Sensor]) -> None:
        self._sensors = sensors
        self._channels = list(channels)
        self._channels.sort(key=lambda chan: chan.value)
        self._attributes = list(attributes)
        self._attributes.sort(key=lambda attr: attr.value)
        self._triggers = list(triggers)
        self._triggers.sort(key=lambda attr: attr.value)
        assert len(self._channels) == len(set(self._channels))
        assert len(self._attributes) == len(set(self._attributes))
        assert len(self._triggers) == len(set(self._triggers))

    def __str__(self) -> str:
        writer = io.StringIO()
        writer.write("/* Auto-generated file, do not edit */\n")
        writer.write("#ifndef _INCLUDE_ZEPHYR_DT_BINDINGS_SENSOR_H_\n")
        writer.write("#define _INCLUDE_ZEPHYR_DT_BINDINGS_SENSOR_H_\n")
        writer.write("\n")
        for channel in self._channels:
            writer.write(str(channel))
        writer.write("\n")
        writer.write(Channel.tail_str())
        writer.write("\n")
        for attribute in self._attributes:
            writer.write(str(attribute))
        writer.write("\n")
        writer.write(Attribute.tail_str())
        writer.write("\n")
        for trigger in self._triggers:
            writer.write(str(trigger))
        writer.write("\n")
        writer.write(Trigger.tail_str())
        writer.write("\n")
        for sensor in self._sensors:
            writer.write(str(sensor))
        writer.write("#endif /* _INCLUDE_ZEPHYR_DT_BINDINGS_SENSOR_H_ */\n")
        return writer.getvalue()

if __name__ == "__main__":
    main()
