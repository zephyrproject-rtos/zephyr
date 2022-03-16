#!/usr/bin/env python3

# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

"""
Pinctrl Migration Utility Script for nRF Boards
###############################################

This script can be used to automatically migrate the Devicetree files of
nRF-based boards using the old <signal>-pin properties to select peripheral
pins. The script will parse a board Devicetree file and will first adjust that
file by removing old pin-related properties replacing them with pinctrl states.
A board-pinctrl.dtsi file will be generated containing the configuration for
all pinctrl states. Note that script will also work on files that have been
partially ported.

.. warning::
    This script uses a basic line based parser, therefore not all valid
    Devicetree files will be converted correctly. **ADJUSTED/GENERATED FILES
    MUST BE MANUALLY REVIEWED**.

Known limitations: All SPI nodes will be assumed to be a master device.

Usage::

    python3 pinctrl_nrf_migrate.py
        -i path/to/board.dts
        [--no-backup]
        [--skip-nrf-check]
        [--header ""]

Example:

.. code-block:: devicetree

    /* Old board.dts */
    ...
    &uart0 {
        ...
        tx-pin = <5>;
        rx-pin = <33>;
        rx-pull-up;
        ...
    };

    /* Adjusted board.dts */
    ...
    #include "board-pinctrl.dtsi"
    ...
    &uart0 {
        ...
        pinctrl-0 = <&uart0_default>;
        pinctrl-1 = <&uart0_sleep>;
        pinctrl-names = "default", "sleep";
        ...
    };

    /* Generated board-pinctrl.dtsi */
    &pinctrl {
        uart0_default: uart0_default {
            group1 {
                psels = <NRF_PSEL(UART_TX, 0, 5);
            };
            group2 {
                psels = <NRF_PSEL(UART_RX, 1, 1)>;
                bias-pull-up;
            };
        };

        uart0_sleep: uart0_sleep {
            group1 {
                psels = <NRF_PSEL(UART_TX, 0, 5)>,
                        <NRF_PSEL(UART_RX, 1, 1)>;
                low-power-enable;
            };
        };
    };
"""

import argparse
import enum
from pathlib import Path
import re
import shutil
from typing import Callable, Optional, Dict, List


#
# Data types and containers
#


class PIN_CONFIG(enum.Enum):
    """Pin configuration attributes"""

    PULL_UP = "bias-pull-up"
    PULL_DOWN = "bias-pull-down"
    LOW_POWER = "low-power-enable"
    NORDIC_INVERT = "nordic,invert"


class Device(object):
    """Device configuration class"""

    def __init__(
        self,
        pattern: str,
        callback: Callable,
        signals: Dict[str, str],
        needs_sleep: bool,
    ) -> None:
        self.pattern = pattern
        self.callback = callback
        self.signals = signals
        self.needs_sleep = needs_sleep
        self.attrs = {}


class SignalMapping(object):
    """Signal mapping (signal<>pin)"""

    def __init__(self, signal: str, pin: int) -> None:
        self.signal = signal
        self.pin = pin


class PinGroup(object):
    """Pin group"""

    def __init__(self, pins: List[SignalMapping], config: List[PIN_CONFIG]) -> None:
        self.pins = pins
        self.config = config


class PinConfiguration(object):
    """Pin configuration (mapping and configuration)"""

    def __init__(self, mapping: SignalMapping, config: List[PIN_CONFIG]) -> None:
        self.mapping = mapping
        self.config = config


class DeviceConfiguration(object):
    """Device configuration"""

    def __init__(self, name: str, pins: List[PinConfiguration]) -> None:
        self.name = name
        self.pins = pins

    def add_signal_config(self, signal: str, config: PIN_CONFIG) -> None:
        """Add configuration to signal"""
        for pin in self.pins:
            if signal == pin.mapping.signal:
                pin.config.append(config)
                return

        self.pins.append(PinConfiguration(SignalMapping(signal, -1), [config]))

    def set_signal_pin(self, signal: str, pin: int) -> None:
        """Set signal pin"""
        for pin_ in self.pins:
            if signal == pin_.mapping.signal:
                pin_.mapping.pin = pin
                return

        self.pins.append(PinConfiguration(SignalMapping(signal, pin), []))


#
# Content formatters and writers
#


def gen_pinctrl(
    configs: List[DeviceConfiguration], input_file: Path, header: str
) -> None:
    """Generate board-pinctrl.dtsi file

    Args:
        configs: Board configs.
        input_file: Board DTS file.
    """

    last_line = 0

    pinctrl_file = input_file.parent / (input_file.stem + "-pinctrl.dtsi")
    # append content before last node closing
    if pinctrl_file.exists():
        content = open(pinctrl_file).readlines()
        for i, line in enumerate(content[::-1]):
            if re.match(r"\s*};.*", line):
                last_line = len(content) - (i + 1)
                break

    out = open(pinctrl_file, "w")

    if not last_line:
        out.write(header)
        out.write("&pinctrl {\n")
    else:
        for line in content[:last_line]:
            out.write(line)

    for config in configs:
        # create pin groups with common configuration (default state)
        default_groups: List[PinGroup] = []
        for pin in config.pins:
            merged = False
            for group in default_groups:
                if group.config == pin.config:
                    group.pins.append(pin.mapping)
                    merged = True
                    break
            if not merged:
                default_groups.append(PinGroup([pin.mapping], pin.config))

        # create pin group for low power state
        group = PinGroup([], [PIN_CONFIG.LOW_POWER])
        for pin in config.pins:
            group.pins.append(pin.mapping)
        sleep_groups = [group]

        # generate default and sleep state entries
        out.write(f"\t{config.name}_default: {config.name}_default {{\n")
        out.write(fmt_pinctrl_groups(default_groups))
        out.write("\t};\n\n")

        out.write(f"\t{config.name}_sleep: {config.name}_sleep {{\n")
        out.write(fmt_pinctrl_groups(sleep_groups))
        out.write("\t};\n\n")

    if not last_line:
        out.write("};\n")
    else:
        for line in content[last_line:]:
            out.write(line)

    out.close()


def board_is_nrf(content: List[str]) -> bool:
    """Check if board is nRF based.

    Args:
        content: DT file content as list of lines.

    Returns:
        True if board is nRF based, False otherwise.
    """

    for line in content:
        m = re.match(r'^#include\s+(?:"|<).*nrf.*(?:>|").*', line)
        if m:
            return True

    return False


def fmt_pinctrl_groups(groups: List[PinGroup]) -> str:
    """Format pinctrl groups.

    Example generated content::

        group1 {
            psels = <NRF_PSEL(UART_TX, 0, 5)>;
        };
        group2 {
            psels = <NRF_PSEL(UART_RX, 1, 1)>;
            bias-pull-up;
        };

    Returns:
        Generated groups.
    """

    content = ""

    for i, group in enumerate(groups):
        content += f"\t\tgroup{i + 1} {{\n"

        # write psels entries
        for i, mapping in enumerate(group.pins):
            prefix = "psels = " if i == 0 else "\t"
            suffix = ";" if i == len(group.pins) - 1 else ","
            pin = mapping.pin
            port = 0 if pin < 32 else 1
            if port == 1:
                pin -= 32
            content += (
                f"\t\t\t{prefix}<NRF_PSEL({mapping.signal}, {port}, {pin})>{suffix}\n"
            )

        # write all pin configuration (bias, low-power, etc.)
        for entry in group.config:
            content += f"\t\t\t{entry.value};\n"

        content += "\t\t};\n"

    return content


def fmt_states(device: str, indent: str, needs_sleep: bool) -> str:
    """Format state entries for the given device.

    Args:
        device: Device name.
        indent: Indentation.
        needs_sleep: If sleep entry is needed.

    Returns:
        State entries to be appended to the device.
    """

    if needs_sleep:
        return "\n".join(
            (
                f"{indent}pinctrl-0 = <&{device}_default>;",
                f"{indent}pinctrl-1 = <&{device}_sleep>;",
                f'{indent}pinctrl-names = "default", "sleep";\n',
            )
        )
    else:
        return "\n".join(
            (
                f"{indent}pinctrl-0 = <&{device}_default>;",
                f'{indent}pinctrl-names = "default";\n',
            )
        )


def insert_pinctrl_include(content: List[str], board: str) -> None:
    """Insert board pinctrl include if not present.

    Args:
        content: DT file content as list of lines.
        board: Board name
    """

    already = False
    include_last_line = -1
    root_line = -1

    for i, line in enumerate(content):
        # check if file already includes a board pinctrl file
        m = re.match(r'^#include\s+".*-pinctrl\.dtsi".*', line)
        if m:
            already = True
            continue

        # check if including
        m = re.match(r'^#include\s+(?:"|<)(.*)(?:>|").*', line)
        if m:
            include_last_line = i
            continue

        # check for root entry
        m = re.match(r"^\s*/\s*{.*", line)
        if m:
            root_line = i
            break

    if include_last_line < 0 and root_line < 0:
        raise ValueError("Unexpected DT file content")

    if not already:
        if include_last_line >= 0:
            line = include_last_line + 1
        else:
            line = max(0, root_line - 1)

        content.insert(line, f'#include "{board}-pinctrl.dtsi"\n')


def adjust_content(content: List[str], board: str) -> List[DeviceConfiguration]:
    """Adjust content

    Args:
        content: File content to be adjusted.
        board: Board name.
    """

    configs: List[DeviceConfiguration] = []
    level = 0
    in_device = False
    states_written = False

    new_content = []

    for line in content:
        # look for a device reference node (e.g. &uart0)
        if not in_device:
            m = re.match(r"^[^&]*&([a-z0-9]+)\s*{[^}]*$", line)
            if m:
                # check if device requires processing
                current_device = None
                for device in DEVICES:
                    if re.match(device.pattern, m.group(1)):
                        current_device = device
                        indent = ""
                        config = DeviceConfiguration(m.group(1), [])
                        configs.append(config)
                        break

                # we are now inside a device node
                level = 1
                in_device = True
                states_written = False
        else:
            # entering subnode (must come after all properties)
            if re.match(r"[^\/\*]*{.*", line):
                level += 1
            # exiting subnode (or device node)
            elif re.match(r"[^\/\*]*}.*", line):
                level -= 1
                in_device = level > 0
            elif current_device:
                # device already ported, drop
                if re.match(r"[^\/\*]*pinctrl-\d+.*", line):
                    current_device = None
                    configs.pop()
                # determine indentation
                elif not indent:
                    m = re.match(r"(\s+).*", line)
                    if m:
                        indent = m.group(1)

            # process each device line, append states at the end
            if current_device:
                if level == 1:
                    line = current_device.callback(config, current_device.signals, line)
                if (level == 2 or not in_device) and not states_written:
                    line = (
                        fmt_states(config.name, indent, current_device.needs_sleep)
                        + line
                    )
                    states_written = True
                    current_device = None

        if line:
            new_content.append(line)

    if configs:
        insert_pinctrl_include(new_content, board)

    content[:] = new_content

    return configs


#
# Processing utilities
#


def match_and_store_pin(
    config: DeviceConfiguration, signals: Dict[str, str], line: str
) -> Optional[str]:
    """Match and store a pin mapping.

    Args:
        config: Device configuration.
        signals: Signals name mapping.
        line: Line containing potential pin mapping.

    Returns:
        Line if found a pin mapping, None otherwise.
    """

    # handle qspi special case for io-pins (array case)
    m = re.match(r"\s*io-pins\s*=\s*([\s<>,0-9]+).*", line)
    if m:
        pins = re.sub(r"[<>,]", "", m.group(1)).split()
        for i, pin in enumerate(pins):
            config.set_signal_pin(signals[f"io{i}"], int(pin))
        return

    m = re.match(r"\s*([a-z]+\d?)-pins?\s*=\s*<(\d+)>.*", line)
    if m:
        config.set_signal_pin(signals[m.group(1)], int(m.group(2)))
        return

    return line


#
# Device processing callbacks
#


def process_uart(config: DeviceConfiguration, signals, line: str) -> Optional[str]:
    """Process UART/UARTE devices."""

    # check if line specifies a pin
    if not match_and_store_pin(config, signals, line):
        return

    # check if pull-up is specified
    m = re.match(r"\s*([a-z]+)-pull-up.*", line)
    if m:
        config.add_signal_config(signals[m.group(1)], PIN_CONFIG.PULL_UP)
        return

    return line


def process_spi(config: DeviceConfiguration, signals, line: str) -> Optional[str]:
    """Process SPI devices."""

    # check if line specifies a pin
    if not match_and_store_pin(config, signals, line):
        return

    # check if pull-up is specified
    m = re.match(r"\s*miso-pull-up.*", line)
    if m:
        config.add_signal_config(signals["miso"], PIN_CONFIG.PULL_UP)
        return

    # check if pull-down is specified
    m = re.match(r"\s*miso-pull-down.*", line)
    if m:
        config.add_signal_config(signals["miso"], PIN_CONFIG.PULL_DOWN)
        return

    return line


def process_pwm(config: DeviceConfiguration, signals, line: str) -> Optional[str]:
    """Process PWM devices."""

    # check if line specifies a pin
    if not match_and_store_pin(config, signals, line):
        return

    # check if channel inversion is specified
    m = re.match(r"\s*([a-z0-9]+)-inverted.*", line)
    if m:
        config.add_signal_config(signals[m.group(1)], PIN_CONFIG.NORDIC_INVERT)
        return

    return line


DEVICES = [
    Device(
        r"uart\d",
        process_uart,
        {
            "tx": "UART_TX",
            "rx": "UART_RX",
            "rts": "UART_RTS",
            "cts": "UART_CTS",
        },
        needs_sleep=True,
    ),
    Device(
        r"i2c\d",
        match_and_store_pin,
        {
            "sda": "TWIM_SDA",
            "scl": "TWIM_SCL",
        },
        needs_sleep=True,
    ),
    Device(
        r"spi\d",
        process_spi,
        {
            "sck": "SPIM_SCK",
            "miso": "SPIM_MISO",
            "mosi": "SPIM_MOSI",
        },
        needs_sleep=True,
    ),
    Device(
        r"pdm\d",
        match_and_store_pin,
        {
            "clk": "PDM_CLK",
            "din": "PDM_DIN",
        },
        needs_sleep=False,
    ),
    Device(
        r"qdec",
        match_and_store_pin,
        {
            "a": "QDEC_A",
            "b": "QDEC_B",
            "led": "QDEC_LED",
        },
        needs_sleep=True,
    ),
    Device(
        r"qspi",
        match_and_store_pin,
        {
            "sck": "QSPI_SCK",
            "io0": "QSPI_IO0",
            "io1": "QSPI_IO1",
            "io2": "QSPI_IO2",
            "io3": "QSPI_IO3",
            "csn": "QSPI_CSN",
        },
        needs_sleep=True,
    ),
    Device(
        r"pwm\d",
        process_pwm,
        {
            "ch0": "PWM_OUT0",
            "ch1": "PWM_OUT1",
            "ch2": "PWM_OUT2",
            "ch3": "PWM_OUT3",
        },
        needs_sleep=True,
    ),
    Device(
        r"i2s\d",
        match_and_store_pin,
        {
            "sck": "I2S_SCK_M",
            "lrck": "I2S_LRCK_M",
            "sdout": "I2S_SDOUT",
            "sdin": "I2S_SDIN",
            "mck": "I2S_MCK",
        },
        needs_sleep=False,
    ),
]
"""Supported devices and associated configuration"""


def main(input_file: Path, no_backup: bool, skip_nrf_check: bool, header: str) -> None:
    """Entry point

    Args:
        input_file: Input DTS file.
        no_backup: Do not create backup files.
    """

    board_name = input_file.stem
    content = open(input_file).readlines()

    if not skip_nrf_check and not board_is_nrf(content):
        print(f"Board {board_name} is not nRF based, terminating")
        return

    if not no_backup:
        backup_file = input_file.parent / (board_name + ".bck" + input_file.suffix)
        shutil.copy(input_file, backup_file)

    configs = adjust_content(content, board_name)

    if configs:
        with open(input_file, "w") as f:
            f.writelines(content)

        gen_pinctrl(configs, input_file, header)

        print(f"Board {board_name} Devicetree file has been converted")
    else:
        print(f"Nothing to be converted for {board_name}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser("pinctrl migration utility for nRF")
    parser.add_argument(
        "-i", "--input", type=Path, required=True, help="Board DTS file"
    )
    parser.add_argument(
        "--no-backup", action="store_true", help="Do not create backup files"
    )
    parser.add_argument(
        "--skip-nrf-check",
        action="store_true",
        help="Skip checking if board is nRF-based",
    )
    parser.add_argument(
        "--header", default="", type=str, help="Header to be prepended to pinctrl files"
    )
    args = parser.parse_args()

    main(args.input, args.no_backup, args.skip_nrf_check, args.header)
