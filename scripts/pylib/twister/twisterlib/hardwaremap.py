#!/usr/bin/env python3
# vim: set syntax=python ts=4 :
#
# Copyright (c) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

import logging
import os
import platform
import re
from multiprocessing import Lock, Value
from pathlib import Path

import scl
import yaml
from natsort import natsorted
from twisterlib.environment import ZEPHYR_BASE

try:
    # Use the C LibYAML parser if available, rather than the Python parser.
    # It's much faster.
    from yaml import CDumper as Dumper
    from yaml import CSafeLoader as SafeLoader
except ImportError:
    from yaml import Dumper, SafeLoader

try:
    from tabulate import tabulate
except ImportError:
    print("Install tabulate python module with pip to use --device-testing option.")

logger = logging.getLogger('twister')


class DUT:
    def __init__(self,
                 id=None,
                 serial=None,
                 serial_baud=None,
                 platform=None,
                 product=None,
                 serial_pty=None,
                 connected=False,
                 runner_params=None,
                 pre_script=None,
                 post_script=None,
                 post_flash_script=None,
                 script_param=None,
                 runner=None,
                 flash_timeout=60,
                 flash_with_test=False,
                 flash_before=False):

        self.serial = serial
        self.baud = serial_baud or 115200
        self.platform = platform
        self.serial_pty = serial_pty
        self._counter = Value("i", 0)
        self._available = Value("i", 1)
        self._failures = Value("i", 0)
        self.connected = connected
        self.pre_script = pre_script
        self.id = id
        self.product = product
        self.runner = runner
        self.runner_params = runner_params
        self.flash_before = flash_before
        self.fixtures = []
        self.post_flash_script = post_flash_script
        self.post_script = post_script
        self.pre_script = pre_script
        self.script_param = script_param
        self.probe_id = None
        self.notes = None
        self.lock = Lock()
        self.match = False
        self.flash_timeout = flash_timeout
        self.flash_with_test = flash_with_test

    @property
    def available(self):
        with self._available.get_lock():
            return self._available.value

    @available.setter
    def available(self, value):
        with self._available.get_lock():
            self._available.value = value

    @property
    def counter(self):
        with self._counter.get_lock():
            return self._counter.value

    @counter.setter
    def counter(self, value):
        with self._counter.get_lock():
            self._counter.value = value

    def counter_increment(self, value=1):
        with self._counter.get_lock():
            self._counter.value += value

    @property
    def failures(self):
        with self._failures.get_lock():
            return self._failures.value

    @failures.setter
    def failures(self, value):
        with self._failures.get_lock():
            self._failures.value = value

    def failures_increment(self, value=1):
        with self._failures.get_lock():
            self._failures.value += value

    def to_dict(self):
        d = {}
        exclude = ['_available', '_counter', '_failures', 'match']
        v = vars(self)
        for k in v:
            if k not in exclude and v[k]:
                d[k] = v[k]
        return d


    def __repr__(self):
        return f"<{self.platform} ({self.product}) on {self.serial}>"

class HardwareMap:
    schema_path = os.path.join(ZEPHYR_BASE, "scripts", "schemas", "twister", "hwmap-schema.yaml")

    manufacturer = [
        'ARM',
        'SEGGER',
        'MBED',
        'STMicroelectronics',
        'Atmel Corp.',
        'Texas Instruments',
        'Silicon Labs',
        'NXP',
        'NXP Semiconductors',
        'Microchip Technology Inc.',
        'FTDI',
        'Digilent',
        'Microsoft',
        'Nuvoton',
        'Espressif',
    ]

    runner_mapping = {
        'pyocd': [
            'DAPLink CMSIS-DAP',
            'MBED CMSIS-DAP'
        ],
        'jlink': [
            'J-Link',
            'J-Link OB'
        ],
        'openocd': [
            'STM32 STLink', '^XDS110.*', 'STLINK-V3'
        ],
        'dediprog': [
            'TTL232R-3V3',
            'MCP2200 USB Serial Port Emulator'
        ]
    }

    def __init__(self, env=None):
        self.detected = []
        self.duts = []
        self.options = env.options

    def discover(self):

        if self.options.generate_hardware_map:
            self.scan(persistent=self.options.persistent_hardware_map)
            self.save(self.options.generate_hardware_map)
            return 0

        if not self.options.device_testing and self.options.hardware_map:
            self.load(self.options.hardware_map)
            logger.info("Available devices:")
            self.dump(connected_only=True)
            return 0

        if self.options.device_testing:
            if self.options.hardware_map:
                self.load(self.options.hardware_map)
                if not self.options.platform:
                    self.options.platform = []
                    for d in self.duts:
                        if d.connected and d.platform != 'unknown':
                            self.options.platform.append(d.platform)

            elif self.options.device_serial:
                self.add_device(self.options.device_serial,
                                self.options.platform[0],
                                self.options.pre_script,
                                False,
                                baud=self.options.device_serial_baud,
                                flash_timeout=self.options.device_flash_timeout,
                                flash_with_test=self.options.device_flash_with_test,
                                flash_before=self.options.flash_before,
                                )

            elif self.options.device_serial_pty:
                self.add_device(self.options.device_serial_pty,
                                self.options.platform[0],
                                self.options.pre_script,
                                True,
                                flash_timeout=self.options.device_flash_timeout,
                                flash_with_test=self.options.device_flash_with_test,
                                flash_before=False,
                                )

            # the fixtures given by twister command explicitly should be assigned to each DUT
            if self.options.fixture:
                for d in self.duts:
                    d.fixtures.extend(self.options.fixture)
        return 1


    def summary(self, selected_platforms):
        print("\nHardware distribution summary:\n")
        table = []
        header = ['Board', 'ID', 'Counter', 'Failures']
        for d in self.duts:
            if d.connected and d.platform in selected_platforms:
                row = [d.platform, d.id, d.counter, d.failures]
                table.append(row)
        print(tabulate(table, headers=header, tablefmt="github"))


    def add_device(
        self,
        serial,
        platform,
        pre_script,
        is_pty,
        baud=None,
        flash_timeout=60,
        flash_with_test=False,
        flash_before=False
    ):
        device = DUT(
            platform=platform,
            connected=True,
            pre_script=pre_script,
            serial_baud=baud,
            flash_timeout=flash_timeout,
            flash_with_test=flash_with_test,
            flash_before=flash_before
        )
        if is_pty:
            device.serial_pty = serial
        else:
            device.serial = serial

        self.duts.append(device)

    def load(self, map_file):
        hwm_schema = scl.yaml_load(self.schema_path)
        duts = scl.yaml_load_verify(map_file, hwm_schema)
        for dut in duts:
            pre_script = dut.get('pre_script')
            script_param = dut.get('script_param')
            post_script = dut.get('post_script')
            post_flash_script = dut.get('post_flash_script')
            flash_timeout = dut.get('flash_timeout') or self.options.device_flash_timeout
            flash_with_test = dut.get('flash_with_test')
            if flash_with_test is None:
                flash_with_test = self.options.device_flash_with_test
            serial_pty = dut.get('serial_pty')
            flash_before = dut.get('flash_before')
            if flash_before is None:
                flash_before = self.options.flash_before and (not (flash_with_test or serial_pty))
            platform = dut.get('platform')
            if isinstance(platform, str):
                platforms = platform.split()
            elif isinstance(platform, list):
                platforms = platform
            else:
                raise ValueError(f"Invalid platform value: {platform}")
            id = dut.get('id')
            runner = dut.get('runner')
            runner_params = dut.get('runner_params')
            serial = dut.get('serial')
            baud = dut.get('baud', None)
            product = dut.get('product')
            fixtures = dut.get('fixtures', [])
            connected = dut.get('connected') and ((serial or serial_pty) is not None)
            if not connected:
                continue
            for plat in platforms:
                new_dut = DUT(platform=plat,
                              product=product,
                              runner=runner,
                              runner_params=runner_params,
                              id=id,
                              serial_pty=serial_pty,
                              serial=serial,
                              serial_baud=baud,
                              connected=connected,
                              pre_script=pre_script,
                              flash_before=flash_before,
                              post_script=post_script,
                              post_flash_script=post_flash_script,
                              script_param=script_param,
                              flash_timeout=flash_timeout,
                              flash_with_test=flash_with_test)
                new_dut.fixtures = fixtures
                new_dut.counter = 0
                self.duts.append(new_dut)

    def scan(self, persistent=False):
        from serial.tools import list_ports

        if persistent and platform.system() == 'Linux':
            # On Linux, /dev/serial/by-id provides symlinks to
            # '/dev/ttyACMx' nodes using names which are unique as
            # long as manufacturers fill out USB metadata nicely.
            #
            # This creates a map from '/dev/ttyACMx' device nodes
            # to '/dev/serial/by-id/usb-...' symlinks. The symlinks
            # go into the hardware map because they stay the same
            # even when the user unplugs / replugs the device.
            #
            # Some inexpensive USB/serial adapters don't result
            # in unique names here, though, so use of this feature
            # requires explicitly setting persistent=True.
            by_id = Path('/dev/serial/by-id')
            def readlink(link):
                return str((by_id / link).resolve())

            if by_id.exists():
                persistent_map = {readlink(link): str(link)
                                  for link in by_id.iterdir()}
            else:
                persistent_map = {}
        else:
            persistent_map = {}

        serial_devices = list_ports.comports()
        logger.info("Scanning connected hardware...")
        for d in serial_devices:
            if (
                d.manufacturer
                and d.manufacturer.casefold() in [m.casefold() for m in self.manufacturer]
            ):

                # TI XDS110 can have multiple serial devices for a single board
                # assume endpoint 0 is the serial, skip all others
                if d.manufacturer == 'Texas Instruments' and not d.location.endswith('0'):
                    continue

                if d.product is None:
                    d.product = 'unknown'

                s_dev = DUT(platform="unknown",
                                        id=d.serial_number,
                                        serial=persistent_map.get(d.device, d.device),
                                        product=d.product,
                                        runner='unknown',
                                        connected=True)

                for runner, _ in self.runner_mapping.items():
                    products = self.runner_mapping.get(runner)
                    if d.product in products:
                        s_dev.runner = runner
                        continue
                    # Try regex matching
                    for p in products:
                        if re.match(p, d.product):
                            s_dev.runner = runner

                s_dev.connected = True
                s_dev.lock = None
                self.detected.append(s_dev)
            else:
                logger.warning(f"Unsupported device ({d.manufacturer}): {d}")

    def save(self, hwm_file):
        # use existing map
        self.detected = natsorted(self.detected, key=lambda x: x.serial or '')
        if os.path.exists(hwm_file):
            with open(hwm_file) as yaml_file:
                hwm = yaml.load(yaml_file, Loader=SafeLoader)
                if hwm:
                    hwm.sort(key=lambda x: x.get('id', ''))

                    # disconnect everything
                    for h in hwm:
                        h['connected'] = False
                        h['serial'] = None

                    for _detected in self.detected:
                        for h in hwm:
                            if all([
                                _detected.id == h['id'],
                                _detected.product == h['product'],
                                _detected.match is False,
                                h['connected'] is False
                            ]):
                                h['connected'] = True
                                h['serial'] = _detected.serial
                                _detected.match = True
                                break

                new_duts = list(filter(lambda d: not d.match, self.detected))
                new = []
                for d in new_duts:
                    new.append(d.to_dict())

                if hwm:
                    hwm = hwm + new
                else:
                    hwm = new

            with open(hwm_file, 'w') as yaml_file:
                yaml.dump(hwm, yaml_file, Dumper=Dumper, default_flow_style=False)

            self.load(hwm_file)
            logger.info("Registered devices:")
            self.dump()

        else:
            # create new file
            dl = []
            for _connected in self.detected:
                platform  = _connected.platform
                id = _connected.id
                runner = _connected.runner
                serial = _connected.serial
                product = _connected.product
                d = {
                    'platform': platform,
                    'id': id,
                    'runner': runner,
                    'serial': serial,
                    'product': product,
                    'connected': _connected.connected
                }
                dl.append(d)
            with open(hwm_file, 'w') as yaml_file:
                yaml.dump(dl, yaml_file, Dumper=Dumper, default_flow_style=False)
            logger.info("Detected devices:")
            self.dump(detected=True)

    def dump(self, filtered=None, header=None, connected_only=False, detected=False):
        if filtered is None:
            filtered = []
        if header is None:
            header = []
        print("")
        table = []
        if detected:
            to_show = self.detected
        else:
            to_show = self.duts

        if not header:
            header = ["Platform", "ID", "Serial device"]
        for p in to_show:
            platform = p.platform
            connected = p.connected
            if filtered and platform not in filtered:
                continue

            if not connected_only or connected:
                table.append([platform, p.id, p.serial])

        print(tabulate(table, headers=header, tablefmt="github"))
