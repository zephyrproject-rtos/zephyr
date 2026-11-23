# Copyright (c) 2017 Linaro Limited.
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing bee devices with mpcli.'''

import json
from pathlib import Path
from textwrap import dedent

from runners.core import FileType, RunnerCaps, ZephyrBinaryRunner


class MPCLIBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for mpcli.'''

    def __init__(self, cfg, port, build_dir, bin_address, chip_erase, mp_json, reset):
        super().__init__(cfg)
        self.port = port
        self.build_dir = build_dir
        self.bin_address = bin_address
        self.chip_erase = chip_erase
        self.mp_json = mp_json
        self.baud = "1000000"
        self.reset = reset
        self.app_bin_file = cfg.bin_file
        self.ext_file = cfg.file
        self.ext_file_type = cfg.file_type
        self.files: list[dict[str, any]] = []

    @classmethod
    def name(cls):
        return 'mpcli'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'}, file=True, erase=True, reset=True)

    @classmethod
    def do_add_parser(cls, parser):
        mpcli_parser = parser

        mpcli_parser.add_argument(
            '--port',
            required=False,
            type=str,
            help='Serial communication port (e.g., COM3, /dev/ttyUSB0)',
        )
        group = mpcli_parser.add_mutually_exclusive_group()
        group.add_argument(
            '--bin-address',
            type=str,
            help='Download address(hex format, e.g., 0x8000000)',
        )
        group.add_argument(
            '--mp-json',
            type=str,
            help=dedent('''
                        Configuration json file containing binary path and download address.
                        When not provided, it will be generated on-the-fly from the other arguments.
                        Note: The port setting can be omitted from the JSON configuration if
                        it is provided via the --port option.
                        Example format:
                        {
                            "mptoolconfig": {
                                "port": "/dev/ttyX",
                                "baud": "1000000",
                                "appimage": {
                                    "relativepath": "directory path for `file` objects, relative to
                                                     this configuration file",
                                    "file": [
                                        {
                                            "id": 0,
                                            "address": "0x00801000",
                                            "name": "fw1.bin",
                                            "enable": "1"
                                        },
                                        {
                                            "id": 1,
                                            "address": "0x00802000",
                                            "name": "fw2.bin",
                                            "enable": "1"
                                        },
                                        ...
                                    ]
                                }
                            }
                        }
                        '''),
        )
        return parser

    @classmethod
    def do_create(cls, cfg, args):
        return MPCLIBinaryRunner(
            cfg,
            args.port,
            build_dir=cfg.build_dir,
            bin_address=args.bin_address,
            chip_erase=args.erase,
            mp_json=args.mp_json,
            reset=args.reset,
        )

    def create_mptool_config(self, config_filename: str) -> None:
        """
        Create MP tool configuration to a JSON file.
        """
        if not self.port:
            raise ValueError('Cannot flash; --port is required')
        mptool_config = {
            "mptoolconfig": {
                "port": self.port,  # serial port
                "baud": self.baud,  # baud rate
                "appimage": {"relativepath": "", "file": self.files},
            }
        }
        with open(config_filename, 'w', encoding='utf-8') as f:
            json.dump(mptool_config, f, indent=4, ensure_ascii=False)

    def add_file(self, address: str, name: str, id: int = 0, enable: str = "1") -> None:
        file_item = {"id": id, "address": address, "name": name, "enable": enable}
        self.files.append(file_item)

    def execute_mpcli(self, mptoolconfig_path):
        """
        Execute mpcli command with the given configuration file.
        """
        cmd_args = [
            'mpcli',
            '-f',
            mptoolconfig_path,
            '-a',
        ]

        if self.port:
            cmd_args.extend(['-c', self.port])

        if self.reset:
            cmd_args.extend(['-r'])

        if self.chip_erase:
            cmd_args.extend(["-E"])

        self.check_call(cmd_args)

    def get_flash_parameters(self):
        """Get file path and download address for flashing."""
        if self.ext_file:
            # Use external binary file
            if self.ext_file_type != FileType.BIN:
                raise ValueError('Cannot flash; mpcli runner only supports bin type')

            bin_file_path = Path(self.ext_file)
        else:
            # Use build system generated file
            bin_file_path = Path(self.app_bin_file)

        download_address = self.bin_address or hex(
            self.flash_address_from_build_conf(self.build_conf)
        )

        return bin_file_path, download_address

    def do_run(self, command, **kwargs):
        self.require('mpcli')

        if self.mp_json:
            # Execute mpcli using the provided configuration file
            mptoolconfig_path = self.mp_json
        else:
            # Create the configuration file and execute mpcli using it
            bin_file_path, download_address = self.get_flash_parameters()

            self.add_file(download_address, bin_file_path.name)
            mptoolconfig_path = str(bin_file_path.parent / "mptoolconfig.json")
            self.create_mptool_config(mptoolconfig_path)

        self.execute_mpcli(mptoolconfig_path)
