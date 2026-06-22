"""
BICR Generation Tool
--------------------

This tool is used to generate a BICR (Board Information Configuration Register)
file from a JSON file that contains the BICR configuration. It can also be used
to do the reverse operation, i.e., to extract the BICR configuration from a BICR
hex file.

::

         JSON     ┌────────────┐     JSON
           │      │            │       ▲
           └─────►│            ├───────┘
                  │ bicrgen.py │
           ┌─────►│            ├───────┐
           │      │            │       ▼
          HEX     └────────────┘      HEX

Usage::

    python genbicr.py \
        --input <input_file.{hex,json}> \
        [--svd <svd_file>] \
        [--output <output_file.{hex,json}>] \
        [--list]

Copyright (c) 2024 Nordic Semiconductor ASA
SPDX-License-Identifier: Apache-2.0
"""

import argparse
import json
import struct
import sys
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from enum import Enum
from pathlib import Path
from pprint import pprint

from intelhex import IntelHex


class Register:
    def __init__(self, regs: ET.Element, name: str, data: bytes | bytearray | None = None) -> None:
        cluster_name, reg_name = name.split(".")

        cluster = regs.find(f".//registers/cluster[name='{cluster_name}']")
        self._offset = int(cluster.find("addressOffset").text, 0)

        self._reg = cluster.find(f".//register[name='{reg_name}']")
        self._offset += int(self._reg.find("addressOffset").text, 0)
        self._size = int(self._reg.find("size").text, 0) // 8

        self._data = data

    @property
    def offset(self) -> int:
        return self._offset

    @property
    def size(self) -> int:
        return self._size

    def _msk_pos(self, name: str) -> tuple[int, int]:
        field = self._reg.find(f".//fields/field[name='{name}']")
        field_lsb = int(field.find("lsb").text, 0)
        field_msb = int(field.find("msb").text, 0)

        mask = (0xFFFFFFFF - (1 << field_lsb) + 1) & (0xFFFFFFFF >> (31 - field_msb))

        return mask, field_lsb

    def _enums(self, field: str) -> list[ET.Element]:
        return self._reg.findall(
            f".//fields/field[name='{field}']/enumeratedValues/enumeratedValue"
        )

    def __getitem__(self, field: str) -> int:
        if not self._data:
            raise TypeError("Empty register")

        msk, pos = self._msk_pos(field)
        raw = struct.unpack("<I", self._data[self._offset : self._offset + 4])[0]
        return (raw & msk) >> pos

    def __setitem__(self, field: str, value: int) -> None:
        if not isinstance(self._data, bytearray):
            raise TypeError("Register is read-only")

        msk, pos = self._msk_pos(field)
        raw = raw = struct.unpack("<I", self._data[self._offset : self._offset + 4])[0]
        raw &= ~msk
        raw |= (value << pos) & msk
        self._data[self._offset : self._offset + 4] = struct.pack("<I", raw)

    def enum_get(self, field: str) -> str:
        value = self[field]
        for enum in self._enums(field):
            if value == int(enum.find("value").text, 0):
                return enum.find("name").text

        raise ValueError(f"Invalid enum value for {field}: {value}")

    def enum_set(self, field: str, value: str) -> None:
        for enum in self._enums(field):
            if value == enum.find("name").text:
                self[field] = int(enum.find("value").text, 0)
                return

        raise ValueError(f"Invalid enum value for {field}: {value}")


class PowerSupplyScheme(Enum):
    UNCONFIGURED = "Unconfigured"
    VDD_VDDH_1V8 = "VDD_VDDH_1V8"
    VDDH_2V1_5V5 = "VDDH_2V1_5V5"


@dataclass
class PowerConfig:
    scheme: PowerSupplyScheme

    @classmethod
    def from_raw(cls: "PowerConfig", bicr_spec: ET.Element, data: bytes) -> "PowerConfig":
        power_config = Register(bicr_spec, "POWER.CONFIG", data)

        if (
            power_config.enum_get("VDDAO5V0") == "Shorted"
            and power_config.enum_get("VDDAO1V8") == "External"
        ):
            scheme = PowerSupplyScheme.VDD_VDDH_1V8
        elif (
            power_config.enum_get("VDDAO5V0") == "External"
            and power_config.enum_get("VDDAO1V8") == "Internal"
        ):
            scheme = PowerSupplyScheme.VDDH_2V1_5V5
        else:
            scheme = PowerSupplyScheme.UNCONFIGURED

        return cls(scheme=scheme)

    @classmethod
    def from_json(cls: "PowerConfig", data: dict) -> "PowerConfig":
        power = data["power"]

        return cls(scheme=PowerSupplyScheme[power["scheme"]])

    def to_raw(self, bicr_spec: ET.Element, buf: bytearray):
        power_config = Register(bicr_spec, "POWER.CONFIG", buf)

        if self.scheme == PowerSupplyScheme.VDD_VDDH_1V8:
            power_config.enum_set("VDDAO5V0", "Shorted")
            power_config.enum_set("VDDAO1V8", "External")
            power_config.enum_set("VDD1V0", "Internal")
            power_config.enum_set("VDDRF1V0", "Shorted")
            power_config.enum_set("VDDAO0V8", "Internal")
            power_config.enum_set("VDDVS0V8", "Internal")
            power_config.enum_set("INDUCTOR", "Present")
        elif self.scheme == PowerSupplyScheme.VDDH_2V1_5V5:
            power_config.enum_set("VDDAO5V0", "External")
            power_config.enum_set("VDDAO1V8", "Internal")
            power_config.enum_set("VDD1V0", "Internal")
            power_config.enum_set("VDDRF1V0", "Shorted")
            power_config.enum_set("VDDAO0V8", "Internal")
            power_config.enum_set("VDDVS0V8", "Internal")
            power_config.enum_set("INDUCTOR", "Present")
        else:
            power_config.enum_set("VDDAO5V0", "Unconfigured")
            power_config.enum_set("VDDAO1V8", "Unconfigured")
            power_config.enum_set("VDD1V0", "Unconfigured")
            power_config.enum_set("VDDRF1V0", "Unconfigured")
            power_config.enum_set("VDDAO0V8", "Unconfigured")
            power_config.enum_set("VDDVS0V8", "Unconfigured")
            power_config.enum_set("INDUCTOR", "Unconfigured")

    def to_json(self, buf: dict):
        buf["power"] = {"scheme": self.scheme.name}


class IoPortPower(Enum):
    DISCONNECTED = "Disconnected"
    SHORTED = "Shorted"
    EXTERNAL_1V8 = "External1V8"


class IoPortPowerExtended(Enum):
    DISCONNECTED = "Disconnected"
    SHORTED = "Shorted"
    EXTERNAL_1V8 = "External1V8"
    EXTERNAL_FULL = "ExternalFull"


@dataclass
class IoPortPowerConfig:
    p1_supply: IoPortPower
    p2_supply: IoPortPower
    p6_supply: IoPortPower
    p7_supply: IoPortPower
    p9_supply: IoPortPowerExtended

    @classmethod
    def from_raw(
        cls: "IoPortPowerConfig", bicr_spec: ET.Element, data: bytes
    ) -> "IoPortPowerConfig":
        ioport_power0 = Register(bicr_spec, "IOPORT.POWER0", data)
        ioport_power1 = Register(bicr_spec, "IOPORT.POWER1", data)

        return cls(
            p1_supply=IoPortPower(ioport_power0.enum_get("P1")),
            p2_supply=IoPortPower(ioport_power0.enum_get("P2")),
            p6_supply=IoPortPower(ioport_power0.enum_get("P6")),
            p7_supply=IoPortPower(ioport_power0.enum_get("P7")),
            p9_supply=IoPortPowerExtended(ioport_power1.enum_get("P9")),
        )

    @classmethod
    def from_json(cls: "IoPortPowerConfig", data: dict) -> "IoPortPowerConfig":
        ioport_power = data["ioPortPower"]

        return cls(
            p1_supply=IoPortPower[ioport_power["p1Supply"]],
            p2_supply=IoPortPower[ioport_power["p2Supply"]],
            p6_supply=IoPortPower[ioport_power["p6Supply"]],
            p7_supply=IoPortPower[ioport_power["p7Supply"]],
            p9_supply=IoPortPowerExtended[ioport_power["p9Supply"]],
        )

    def to_raw(self, bicr_spec: ET.Element, buf: bytearray):
        ioport_power0 = Register(bicr_spec, "IOPORT.POWER0", buf)
        ioport_power1 = Register(bicr_spec, "IOPORT.POWER1", buf)

        ioport_power0.enum_set("P1", self.p1_supply.value)
        ioport_power0.enum_set("P2", self.p2_supply.value)
        ioport_power0.enum_set("P6", self.p6_supply.value)
        ioport_power0.enum_set("P7", self.p7_supply.value)
        ioport_power1.enum_set("P9", self.p9_supply.value)

    def to_json(self, buf: dict):
        buf["ioPortPower"] = {
            "p1Supply": self.p1_supply.name,
            "p2Supply": self.p2_supply.name,
            "p6Supply": self.p6_supply.name,
            "p7Supply": self.p7_supply.name,
            "p9Supply": self.p9_supply.name,
        }


@dataclass
class IoPortImpedanceConfig:
    p6_impedance_ohms: int
    p7_impedance_ohms: int

    @classmethod
    def from_raw(
        cls: "IoPortImpedanceConfig", bicr_spec: ET.Element, data: bytes
    ) -> "IoPortImpedanceConfig":
        drivectl0 = Register(bicr_spec, "IOPORT.DRIVECTRL0", data)

        return cls(
            p6_impedance_ohms=int(drivectl0.enum_get("P6")[4:]),
            p7_impedance_ohms=int(drivectl0.enum_get("P7")[4:]),
        )

    @classmethod
    def from_json(cls: "IoPortImpedanceConfig", data: dict) -> "IoPortImpedanceConfig":
        ioport_impedance = data["ioPortImpedance"]

        return cls(
            p6_impedance_ohms=ioport_impedance["p6ImpedanceOhms"],
            p7_impedance_ohms=ioport_impedance["p7ImpedanceOhms"],
        )

    def to_raw(self, bicr_spec: ET.Element, buf: bytearray):
        drivectl0 = Register(bicr_spec, "IOPORT.DRIVECTRL0", buf)

        drivectl0.enum_set("P6", f"Ohms{self.p6_impedance_ohms}")
        drivectl0.enum_set("P7", f"Ohms{self.p7_impedance_ohms}")

    def to_json(self, buf: dict):
        buf["ioPortImpedance"] = {
            "p6ImpedanceOhms": self.p6_impedance_ohms,
            "p7ImpedanceOhms": self.p7_impedance_ohms,
        }


class LFXOMode(Enum):
    CRYSTAL = "Crystal"
    EXT_SINE = "ExtSine"
    EXT_SQUARE = "ExtSquare"


@dataclass
class LFXOConfig:
    accuracy_ppm: int
    mode: LFXOMode
    builtin_load_capacitors: bool
    builtin_load_capacitance_pf: int | None
    startup_time_ms: int

    @classmethod
    def from_raw(cls: "LFXOConfig", bicr_spec: ET.Element, data: bytes) -> "LFXOConfig":
        lfosc_lfxoconfig = Register(bicr_spec, "LFOSC.LFXOCONFIG", data)

        try:
            loadcap = lfosc_lfxoconfig.enum_get("LOADCAP")
        except ValueError:
            builtin_load_capacitors = True
            builtin_load_capacitance_pf = lfosc_lfxoconfig["LOADCAP"]
        else:
            if loadcap == "Unconfigured":
                raise ValueError("Invalid LFXO load capacitors configuration")

            builtin_load_capacitors = False
            builtin_load_capacitance_pf = None

        startup_time_ms = 0
        try:
            lfosc_lfxoconfig.enum_get("TIME")
        except ValueError:
            startup_time_ms = lfosc_lfxoconfig["TIME"]
        else:
            raise ValueError("Invalid LFXO startup time (not configured)")

        return cls(
            accuracy_ppm=int(lfosc_lfxoconfig.enum_get("ACCURACY")[:-3]),
            mode=LFXOMode(lfosc_lfxoconfig.enum_get("MODE")),
            builtin_load_capacitors=builtin_load_capacitors,
            builtin_load_capacitance_pf=builtin_load_capacitance_pf,
            startup_time_ms=startup_time_ms,
        )

    @classmethod
    def from_json(cls: "LFXOConfig", data: dict) -> "LFXOConfig":
        lfxo = data["lfosc"]["lfxo"]

        builtin_load_capacitors = lfxo["builtInLoadCapacitors"]
        if builtin_load_capacitors:
            builtin_load_capacitance_pf = lfxo["builtInLoadCapacitancePf"]
        else:
            builtin_load_capacitance_pf = None

        return cls(
            accuracy_ppm=lfxo["accuracyPPM"],
            mode=LFXOMode[lfxo["mode"]],
            builtin_load_capacitors=builtin_load_capacitors,
            builtin_load_capacitance_pf=builtin_load_capacitance_pf,
            startup_time_ms=lfxo["startupTimeMs"],
        )

    def to_raw(self, bicr_spec: ET.Element, buf: bytearray):
        lfosc_lfxoconfig = Register(bicr_spec, "LFOSC.LFXOCONFIG", buf)

        lfosc_lfxoconfig.enum_set("ACCURACY", f"{self.accuracy_ppm}ppm")
        lfosc_lfxoconfig.enum_set("MODE", self.mode.value)
        lfosc_lfxoconfig["TIME"] = self.startup_time_ms

        if self.builtin_load_capacitors:
            lfosc_lfxoconfig["LOADCAP"] = self.builtin_load_capacitance_pf
        else:
            lfosc_lfxoconfig.enum_set("LOADCAP", "External")

    def to_json(self, buf: dict):
        lfosc = buf["lfosc"]
        lfosc["lfxo"] = {
            "accuracyPPM": self.accuracy_ppm,
            "mode": self.mode.name,
            "builtInLoadCapacitors": self.builtin_load_capacitors,
            "startupTimeMs": self.startup_time_ms,
        }

        if self.builtin_load_capacitors:
            lfosc["lfxo"]["builtInLoadCapacitancePf"] = self.builtin_load_capacitance_pf


@dataclass
class LFRCCalibrationConfig:
    calibration_enabled: bool
    temp_meas_interval_seconds: float | None
    temp_delta_calibration_trigger_celsius: float | None
    max_meas_interval_between_calibrations: int | None

    @classmethod
    def from_raw(
        cls: "LFRCCalibrationConfig", bicr_spec: ET.Element, data: bytes
    ) -> "LFRCCalibrationConfig":
        lfosc_lfrcautocalconfig = Register(bicr_spec, "LFOSC.LFRCAUTOCALCONFIG", data)

        calibration_enabled = lfosc_lfrcautocalconfig.enum_get("ENABLE") == "Enabled"
        if calibration_enabled:
            return cls(
                calibration_enabled=calibration_enabled,
                temp_meas_interval_seconds=lfosc_lfrcautocalconfig["TEMPINTERVAL"] * 0.25,
                temp_delta_calibration_trigger_celsius=lfosc_lfrcautocalconfig["TEMPDELTA"] * 0.25,
                max_meas_interval_between_calibrations=lfosc_lfrcautocalconfig["INTERVALMAXNO"],
            )
        else:
            return cls(
                calibration_enabled=calibration_enabled,
                temp_meas_interval_seconds=None,
                temp_delta_calibration_trigger_celsius=None,
                max_meas_interval_between_calibrations=None,
            )

    @classmethod
    def from_json(cls: "LFRCCalibrationConfig", data: dict) -> "LFRCCalibrationConfig":
        lfrccal = data["lfosc"]["lfrccal"]

        calibration_enabled = lfrccal["calibrationEnabled"]
        if calibration_enabled:
            temp_meas_interval_seconds = lfrccal["tempMeasIntervalSeconds"]
            temp_delta_calibration_trigger_celsius = lfrccal["tempDeltaCalibrationTriggerCelsius"]
            max_meas_interval_between_calibrations = lfrccal["maxMeasIntervalBetweenCalibrations"]
        else:
            temp_meas_interval_seconds = None
            temp_delta_calibration_trigger_celsius = None
            max_meas_interval_between_calibrations = None

        return cls(
            calibration_enabled=calibration_enabled,
            temp_meas_interval_seconds=temp_meas_interval_seconds,
            temp_delta_calibration_trigger_celsius=temp_delta_calibration_trigger_celsius,
            max_meas_interval_between_calibrations=max_meas_interval_between_calibrations,
        )

    def to_raw(self, bicr_spec: ET.Element, buf: bytearray):
        lfosc_lfrcautocalconfig = Register(bicr_spec, "LFOSC.LFRCAUTOCALCONFIG", buf)

        lfosc_lfrcautocalconfig.enum_set(
            "ENABLE", "Enabled" if self.calibration_enabled else "Disabled"
        )
        if self.calibration_enabled:
            lfosc_lfrcautocalconfig["TEMPINTERVAL"] = int(self.temp_meas_interval_seconds / 0.25)
            lfosc_lfrcautocalconfig["TEMPDELTA"] = int(
                self.temp_delta_calibration_trigger_celsius / 0.25
            )
            lfosc_lfrcautocalconfig["INTERVALMAXNO"] = self.max_meas_interval_between_calibrations

    def to_json(self, buf: dict):
        lfosc = buf["lfosc"]
        lfosc["lfrccal"] = {
            "calibrationEnabled": self.calibration_enabled,
        }

        if self.calibration_enabled:
            lfosc["lfrccal"]["tempMeasIntervalSeconds"] = self.temp_meas_interval_seconds
            lfosc["lfrccal"]["tempDeltaCalibrationTriggerCelsius"] = (
                self.temp_delta_calibration_trigger_celsius
            )
            lfosc["lfrccal"]["maxMeasIntervalBetweenCalibrations"] = (
                self.max_meas_interval_between_calibrations
            )


class LFOSCSource(Enum):
    LFXO = "LFXO"
    LFRC = "LFRC"


@dataclass
class LFOSCConfig:
    source: LFOSCSource
    lfxo: LFXOConfig | None
    lfrccal: LFRCCalibrationConfig | None

    @classmethod
    def from_raw(cls: "LFOSCConfig", bicr_spec: ET.Element, data: bytes) -> "LFOSCConfig":
        lfosc_lfxoconfig = Register(bicr_spec, "LFOSC.LFXOCONFIG", data)

        mode = lfosc_lfxoconfig.enum_get("MODE")
        if mode == "Disabled":
            source = LFOSCSource.LFRC
            lfxo = None
            lfrccal = LFRCCalibrationConfig.from_raw(bicr_spec, data)
        elif mode == "Unconfigured":
            raise ValueError("Invalid LFOSC configuration")
        else:
            source = LFOSCSource.LFXO
            lfxo = LFXOConfig.from_raw(bicr_spec, data)
            lfrccal = None

        return cls(
            source=source,
            lfxo=lfxo,
            lfrccal=lfrccal,
        )

    @classmethod
    def from_json(cls: "LFOSCConfig", data: dict) -> "LFOSCConfig":
        lfosc = data["lfosc"]

        source = LFOSCSource[lfosc["source"]]
        if source == LFOSCSource.LFXO:
            source = source
            lfxo = LFXOConfig.from_json(data)
            lfrccal = None
        else:
            source = source
            lfxo = None
            lfrccal = LFRCCalibrationConfig.from_json(data)

        return cls(
            source=source,
            lfxo=lfxo,
            lfrccal=lfrccal,
        )

    def to_raw(self, bicr_spec: ET.Element, buf: bytearray):
        lfosc_lfxoconfig = Register(bicr_spec, "LFOSC.LFXOCONFIG", buf)

        if self.source == LFOSCSource.LFRC:
            lfosc_lfxoconfig.enum_set("MODE", "Disabled")
            self.lfrccal.to_raw(bicr_spec, buf)
        elif self.source == LFOSCSource.LFXO:
            self.lfxo.to_raw(bicr_spec, buf)

    def to_json(self, buf: dict):
        buf["lfosc"] = {
            "source": self.source.name,
        }

        if self.source == LFOSCSource.LFXO:
            self.lfxo.to_json(buf)
        else:
            self.lfrccal.to_json(buf)


class HFXOMode(Enum):
    CRYSTAL = "Crystal"
    EXT_SQUARE = "ExtSquare"


@dataclass
class HFXOConfig:
    mode: HFXOMode
    builtin_load_capacitors: bool
    builtin_load_capacitance_pf: float | None
    startup_time_us: int

    @classmethod
    def from_raw(cls: "HFXOConfig", bicr_spec: ET.Element, data: bytes) -> "HFXOConfig":
        hfxo_config = Register(bicr_spec, "HFXO.CONFIG", data)
        hfxo_startuptime = Register(bicr_spec, "HFXO.STARTUPTIME", data)

        mode = HFXOMode(hfxo_config.enum_get("MODE"))

        try:
            loadcap = hfxo_config.enum_get("LOADCAP")
        except ValueError:
            builtin_load_capacitors = True
            builtin_load_capacitance_pf = hfxo_config["LOADCAP"] * 0.25
        else:
            if loadcap == "Unconfigured":
                raise ValueError("Invalid HFXO load capacitors configuration")

            builtin_load_capacitors = False
            builtin_load_capacitance_pf = None

        startup_time_us = 0
        try:
            hfxo_startuptime.enum_get("TIME")
        except ValueError:
            startup_time_us = hfxo_startuptime["TIME"]
        else:
            raise ValueError("Invalid LFXO startup time (not configured)")

        return cls(
            mode=mode,
            builtin_load_capacitors=builtin_load_capacitors,
            builtin_load_capacitance_pf=builtin_load_capacitance_pf,
            startup_time_us=startup_time_us,
        )

    @classmethod
    def from_json(cls: "HFXOConfig", data: dict) -> "HFXOConfig":
        hfxo = data["hfxo"]

        builtin_load_capacitors = hfxo["builtInLoadCapacitors"]
        if builtin_load_capacitors:
            builtin_load_capacitance_pf = hfxo["builtInLoadCapacitancePf"]
        else:
            builtin_load_capacitance_pf = None

        return cls(
            mode=HFXOMode[hfxo["mode"]],
            builtin_load_capacitors=builtin_load_capacitors,
            builtin_load_capacitance_pf=builtin_load_capacitance_pf,
            startup_time_us=hfxo["startupTimeUs"],
        )

    def to_raw(self, bicr_spec: ET.Element, buf: bytearray):
        hfxo_config = Register(bicr_spec, "HFXO.CONFIG", buf)
        hfxo_startuptime = Register(bicr_spec, "HFXO.STARTUPTIME", buf)

        hfxo_config.enum_set("MODE", self.mode.value)
        hfxo_startuptime["TIME"] = self.startup_time_us

        if self.builtin_load_capacitors:
            hfxo_config["LOADCAP"] = int(self.builtin_load_capacitance_pf / 0.25)
        else:
            hfxo_config.enum_set("LOADCAP", "External")

    def to_json(self, buf: dict):
        buf["hfxo"] = {
            "mode": self.mode.name,
            "builtInLoadCapacitors": self.builtin_load_capacitors,
            "startupTimeUs": self.startup_time_us,
        }

        if self.builtin_load_capacitors:
            buf["hfxo"]["builtInLoadCapacitancePf"] = self.builtin_load_capacitance_pf


@dataclass
class BICR:
    power: PowerConfig
    ioport_power: IoPortPowerConfig
    ioport_impedance: IoPortImpedanceConfig
    lfosc: LFOSCConfig
    hfxo: HFXOConfig

    @classmethod
    def from_raw(cls: "BICR", bicr_spec: ET.Element, data: bytes) -> "BICR":
        return cls(
            power=PowerConfig.from_raw(bicr_spec, data),
            ioport_power=IoPortPowerConfig.from_raw(bicr_spec, data),
            ioport_impedance=IoPortImpedanceConfig.from_raw(bicr_spec, data),
            lfosc=LFOSCConfig.from_raw(bicr_spec, data),
            hfxo=HFXOConfig.from_raw(bicr_spec, data),
        )

    @classmethod
    def from_json(cls: "BICR", data: dict) -> "BICR":
        return cls(
            power=PowerConfig.from_json(data),
            ioport_power=IoPortPowerConfig.from_json(data),
            ioport_impedance=IoPortImpedanceConfig.from_json(data),
            lfosc=LFOSCConfig.from_json(data),
            hfxo=HFXOConfig.from_json(data),
        )

    def to_raw(self, bicr_spec: ET.Element, buf: bytearray):
        self.power.to_raw(bicr_spec, buf)
        self.ioport_power.to_raw(bicr_spec, buf)
        self.ioport_impedance.to_raw(bicr_spec, buf)
        self.lfosc.to_raw(bicr_spec, buf)
        self.hfxo.to_raw(bicr_spec, buf)

    def to_json(self, buf: dict):
        self.power.to_json(buf)
        self.ioport_power.to_json(buf)
        self.ioport_impedance.to_json(buf)
        self.lfosc.to_json(buf)
        self.hfxo.to_json(buf)


INIT_VALUE = 0xFFFFFFFF
BYTE_MASK = 0xFF
FINAL_XOR = 0xFFFFFFFF
CRC32_TABLE = [
    0x00000000,
    0x04C11DB7,
    0x09823B6E,
    0x0D4326D9,
    0x130476DC,
    0x17C56B6B,
    0x1A864DB2,
    0x1E475005,
    0x2608EDB8,
    0x22C9F00F,
    0x2F8AD6D6,
    0x2B4BCB61,
    0x350C9B64,
    0x31CD86D3,
    0x3C8EA00A,
    0x384FBDBD,
    0x4C11DB70,
    0x48D0C6C7,
    0x4593E01E,
    0x4152FDA9,
    0x5F15ADAC,
    0x5BD4B01B,
    0x569796C2,
    0x52568B75,
    0x6A1936C8,
    0x6ED82B7F,
    0x639B0DA6,
    0x675A1011,
    0x791D4014,
    0x7DDC5DA3,
    0x709F7B7A,
    0x745E66CD,
    0x9823B6E0,
    0x9CE2AB57,
    0x91A18D8E,
    0x95609039,
    0x8B27C03C,
    0x8FE6DD8B,
    0x82A5FB52,
    0x8664E6E5,
    0xBE2B5B58,
    0xBAEA46EF,
    0xB7A96036,
    0xB3687D81,
    0xAD2F2D84,
    0xA9EE3033,
    0xA4AD16EA,
    0xA06C0B5D,
    0xD4326D90,
    0xD0F37027,
    0xDDB056FE,
    0xD9714B49,
    0xC7361B4C,
    0xC3F706FB,
    0xCEB42022,
    0xCA753D95,
    0xF23A8028,
    0xF6FB9D9F,
    0xFBB8BB46,
    0xFF79A6F1,
    0xE13EF6F4,
    0xE5FFEB43,
    0xE8BCCD9A,
    0xEC7DD02D,
    0x34867077,
    0x30476DC0,
    0x3D044B19,
    0x39C556AE,
    0x278206AB,
    0x23431B1C,
    0x2E003DC5,
    0x2AC12072,
    0x128E9DCF,
    0x164F8078,
    0x1B0CA6A1,
    0x1FCDBB16,
    0x018AEB13,
    0x054BF6A4,
    0x0808D07D,
    0x0CC9CDCA,
    0x7897AB07,
    0x7C56B6B0,
    0x71159069,
    0x75D48DDE,
    0x6B93DDDB,
    0x6F52C06C,
    0x6211E6B5,
    0x66D0FB02,
    0x5E9F46BF,
    0x5A5E5B08,
    0x571D7DD1,
    0x53DC6066,
    0x4D9B3063,
    0x495A2DD4,
    0x44190B0D,
    0x40D816BA,
    0xACA5C697,
    0xA864DB20,
    0xA527FDF9,
    0xA1E6E04E,
    0xBFA1B04B,
    0xBB60ADFC,
    0xB6238B25,
    0xB2E29692,
    0x8AAD2B2F,
    0x8E6C3698,
    0x832F1041,
    0x87EE0DF6,
    0x99A95DF3,
    0x9D684044,
    0x902B669D,
    0x94EA7B2A,
    0xE0B41DE7,
    0xE4750050,
    0xE9362689,
    0xEDF73B3E,
    0xF3B06B3B,
    0xF771768C,
    0xFA325055,
    0xFEF34DE2,
    0xC6BCF05F,
    0xC27DEDE8,
    0xCF3ECB31,
    0xCBFFD686,
    0xD5B88683,
    0xD1799B34,
    0xDC3ABDED,
    0xD8FBA05A,
    0x690CE0EE,
    0x6DCDFD59,
    0x608EDB80,
    0x644FC637,
    0x7A089632,
    0x7EC98B85,
    0x738AAD5C,
    0x774BB0EB,
    0x4F040D56,
    0x4BC510E1,
    0x46863638,
    0x42472B8F,
    0x5C007B8A,
    0x58C1663D,
    0x558240E4,
    0x51435D53,
    0x251D3B9E,
    0x21DC2629,
    0x2C9F00F0,
    0x285E1D47,
    0x36194D42,
    0x32D850F5,
    0x3F9B762C,
    0x3B5A6B9B,
    0x0315D626,
    0x07D4CB91,
    0x0A97ED48,
    0x0E56F0FF,
    0x1011A0FA,
    0x14D0BD4D,
    0x19939B94,
    0x1D528623,
    0xF12F560E,
    0xF5EE4BB9,
    0xF8AD6D60,
    0xFC6C70D7,
    0xE22B20D2,
    0xE6EA3D65,
    0xEBA91BBC,
    0xEF68060B,
    0xD727BBB6,
    0xD3E6A601,
    0xDEA580D8,
    0xDA649D6F,
    0xC423CD6A,
    0xC0E2D0DD,
    0xCDA1F604,
    0xC960EBB3,
    0xBD3E8D7E,
    0xB9FF90C9,
    0xB4BCB610,
    0xB07DABA7,
    0xAE3AFBA2,
    0xAAFBE615,
    0xA7B8C0CC,
    0xA379DD7B,
    0x9B3660C6,
    0x9FF77D71,
    0x92B45BA8,
    0x9675461F,
    0x8832161A,
    0x8CF30BAD,
    0x81B02D74,
    0x857130C3,
    0x5D8A9099,
    0x594B8D2E,
    0x5408ABF7,
    0x50C9B640,
    0x4E8EE645,
    0x4A4FFBF2,
    0x470CDD2B,
    0x43CDC09C,
    0x7B827D21,
    0x7F436096,
    0x7200464F,
    0x76C15BF8,
    0x68860BFD,
    0x6C47164A,
    0x61043093,
    0x65C52D24,
    0x119B4BE9,
    0x155A565E,
    0x18197087,
    0x1CD86D30,
    0x029F3D35,
    0x065E2082,
    0x0B1D065B,
    0x0FDC1BEC,
    0x3793A651,
    0x3352BBE6,
    0x3E119D3F,
    0x3AD08088,
    0x2497D08D,
    0x2056CD3A,
    0x2D15EBE3,
    0x29D4F654,
    0xC5A92679,
    0xC1683BCE,
    0xCC2B1D17,
    0xC8EA00A0,
    0xD6AD50A5,
    0xD26C4D12,
    0xDF2F6BCB,
    0xDBEE767C,
    0xE3A1CBC1,
    0xE760D676,
    0xEA23F0AF,
    0xEEE2ED18,
    0xF0A5BD1D,
    0xF464A0AA,
    0xF9278673,
    0xFDE69BC4,
    0x89B8FD09,
    0x8D79E0BE,
    0x803AC667,
    0x84FBDBD0,
    0x9ABC8BD5,
    0x9E7D9662,
    0x933EB0BB,
    0x97FFAD0C,
    0xAFB010B1,
    0xAB710D06,
    0xA6322BDF,
    0xA2F33668,
    0xBCB4666D,
    0xB8757BDA,
    0xB5365D03,
    0xB1F740B4,
]


def crc32_bzip2_input_reversed(buf: bytes) -> int:
    # Process bytes from last to first (default for ICR)
    buf = buf[::-1]

    crc = INIT_VALUE

    for b in buf:
        crc = ((crc << 8) ^ CRC32_TABLE[((crc >> 24) ^ b) & BYTE_MASK]) & 0xFFFFFFFF

    return crc ^ FINAL_XOR


def encode_crc32(crc: int) -> bytes:
    return crc.to_bytes(4, "little", signed=False)


def decode_crc32(buf: bytes) -> int:
    assert len(buf) == 4
    return int.from_bytes(buf, "little", signed=False)


# Golden vector including CRC in 4-byte little endian at end.
TEST_BICR = bytes(
    [
        0x81,
        0xF0,
        0xFF,
        0x7F,
        0x2F,
        0xF2,
        0xFF,
        0x22,
        0x4F,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0x22,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0x87,
        0x0F,
        0x58,
        0xF2,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0x88,
        0xF3,
        0xFF,
        0x52,
        0x03,
        0x00,
        0x00,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0x56,
        0x50,
        0x9D,
        0x67,
    ]
)

# Golden vector CRC as integer, also at end of TEST_BICR in 4-byte little endian.
TEST_BICR_CRC = 0x679D5056

# Size of BICR in bytes, hardcoded here since the MDK is incomplete, missing reserved fields and
# the BICR CRC field.
BICR_SIZE = 0x50

# Size of BICR CRC field in bytes.
BICR_CRC_SIZE = 0x4

# Debug tests validating CRC computation and helpers
#
#   assert len(TEST_BICR) == BICR_SIZE
#   assert crc32_bzip2_input_reversed(TEST_BICR[:-BICR_CRC_SIZE]) == TEST_BICR_CRC
#   assert encode_crc32(TEST_BICR_CRC) == TEST_BICR[-BICR_CRC_SIZE:]
#   assert decode_crc32(TEST_BICR[-BICR_CRC_SIZE:]) == TEST_BICR_CRC

if __name__ == "__main__":
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument("-i", "--input", type=Path, required=True, help="Input file")
    parser.add_argument("-s", "--svd", type=Path, help="SVD file")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("-o", "--output", type=Path, help="Output file")
    group.add_argument("-l", "--list", action="store_true", help="List BICR options")
    args = parser.parse_args()

    if args.input.suffix == ".hex" or (args.output and args.output.suffix == ".hex"):
        if not args.svd:
            sys.exit("SVD file is required for hex files")

        bicr_spec = ET.parse(args.svd).getroot().find(".//peripheral[name='BICR_NS']")

    if args.input.suffix == ".hex":
        ih = IntelHex()
        ih.loadhex(args.input)
        buf = ih.tobinstr()
        bicr_size = len(buf)
        if bicr_size == BICR_SIZE:
            actual_crc = decode_crc32(buf[BICR_SIZE - BICR_CRC_SIZE :])
            buf = buf[: BICR_SIZE - BICR_CRC_SIZE]
            expected_crc = crc32_bzip2_input_reversed(buf)
            if actual_crc != expected_crc:
                sys.exit(f"Expected BICR CRC: {expected_crc}, actual BICR CRC: {actual_crc}")
        else:
            print("Unexpected BICR size, skipping CRC check")

        bicr = BICR.from_raw(bicr_spec, buf)
    elif args.input.suffix == ".json":
        with open(args.input) as f:
            data = json.load(f)
        bicr = BICR.from_json(data)
    else:
        sys.exit("Unsupported input file format")

    if args.output:
        if args.output.suffix == ".hex":
            bicr_address = int(bicr_spec.find("baseAddress").text, 0)

            # Initialize the BICR area up until the BICR CRC field, then set the BICR values.
            buf = bytearray([0xFF] * (BICR_SIZE - BICR_CRC_SIZE))
            bicr.to_raw(bicr_spec, buf)

            # Compute the CRC value of the BICR area up until the BICR CRC field, then append it to
            # the BICR area.
            crc = crc32_bzip2_input_reversed(buf)
            buf += encode_crc32(crc)

            ih = IntelHex()
            ih.frombytes(buf, offset=bicr_address)
            ih.tofile(args.output, format="hex")
        elif args.output.suffix == ".json":
            buf = dict()
            bicr.to_json(buf)

            with open(args.output, "w") as f:
                json.dump(buf, f, indent=4)
        else:
            sys.exit("Unsupported output file format")
    elif args.list:
        pprint(bicr)
