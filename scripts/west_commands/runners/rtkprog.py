# Copyright (c) 2026, A-Labs GmbH
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing Realtek devices with rtkprog.'''

from pathlib import Path

from runners.core import FileType, RunnerCaps, ZephyrBinaryRunner


class RtkProgBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for rtkprog.'''

    def __init__(self, cfg, port, build_dir, bin_address, chip_erase, reset):
        super().__init__(cfg)
        self.port = port
        self.build_dir = build_dir
        self.bin_address = bin_address
        self.chip_erase = chip_erase
        self.reset = reset
        if not reset:
            raise Exception('rtkprog always resets')
        self.app_bin_file = cfg.bin_file
        self.ext_file = cfg.file
        self.ext_file_type = cfg.file_type

    @classmethod
    def name(cls):
        return 'rtkprog'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'}, file=True, erase=True, reset=True)

    @classmethod
    def do_add_parser(cls, parser):
        argument_parser = parser

        argument_parser.add_argument(
            '--port',
            required=False,
            type=str,
            help='Serial communication port (e.g., COM3, /dev/ttyUSB0)',
        )
        argument_parser.add_argument(
            '--bin-address',
            type=str,
            help='Download address (hex format, e.g., 0x8000000)',
        )
        return parser

    @classmethod
    def do_create(cls, cfg, args):
        return RtkProgBinaryRunner(
            cfg,
            args.port,
            build_dir=cfg.build_dir,
            bin_address=args.bin_address,
            chip_erase=args.erase,
            reset=args.reset,
        )

    def execute_rtkprog(self, bin_file_path, download_address):
        """
        Execute rtkprog command with the given configuration file.
        """
        cmd_args = [
            'rtkprog',
        ]

        # global args need to come before command
        if self.port:
            cmd_args.extend(['--port', self.port])

        # command and command args
        cmd_args.extend([
            'write',
            '-w',
            download_address + ',0x0,' + str(bin_file_path),
        ])

        # TODO
        # if self.chip_erase:
        #     cmd_args.extend(["--erase"])

        self.check_call(cmd_args)

    def get_flash_parameters(self):
        """Get file path and download address for flashing."""
        if self.ext_file:
            # Use external binary file
            if self.ext_file_type != FileType.BIN:
                raise ValueError('Cannot flash; runner only supports bin type')

            bin_file_path = Path(self.ext_file)
        else:
            # Use build system generated file
            bin_file_path = Path(self.app_bin_file)

        download_address = self.bin_address or hex(
            self.flash_address_from_build_conf(self.build_conf)
        )

        return bin_file_path, download_address

    def do_run(self, command, **kwargs):
        self.require('rtkprog')

        bin_file_path, download_address = self.get_flash_parameters()

        self.execute_rtkprog(bin_file_path, download_address)
