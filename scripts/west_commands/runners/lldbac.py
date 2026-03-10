# Copyright (c) 2025 Synopsys Inc.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for Synopsys ARC MetaWare Debugger (lldbac).'''

import argparse
import shutil
from os import path

from runners.core import RunnerCaps, ZephyrBinaryRunner


class LldbacBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for run-lldbac tool for simulator and hardware.'''

    def __init__(
        self,
        cfg,
        props=None,
        tcf=None,
        simulator=False,
        jtag='jtag-digilent',
        jtag_device=None,
        jtag_frequency='500KHz',
        gui=False,
        postconnect_cmds=None,
        postconnect_file=None,
        board_json=None,
    ):
        super().__init__(cfg)

        # Check for run-lldbac tool upfront
        # run-lldbac is used for all commands (flash, debug)
        self.run_lldbac_cmd = shutil.which('run-lldbac')

        if not self.run_lldbac_cmd:
            raise FileNotFoundError(
                "run-lldbac not found in PATH. "
                "Please ensure Synopsys MetaWare tools are installed and available."
            )

        self.tcf = tcf
        self.props = props
        self.simulator = simulator
        self.jtag = jtag
        self.jtag_device = jtag_device
        self.jtag_frequency = jtag_frequency
        self.gui = gui
        self.postconnect_cmds = postconnect_cmds or []
        self.postconnect_file = postconnect_file
        self.board_json = board_json

    @classmethod
    def name(cls):
        return 'lldbac'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash', 'debug'})

    @classmethod
    def do_add_parser(cls, parser):
        # Integrated mode - Simulator options
        parser.add_argument(
            '--simulator',
            action=argparse.BooleanOptionalAction,
            help='Use simulator instead of physical hardware (integrated mode)',
        )
        parser.add_argument(
            '--tcf', help='TCF (Tool Configuration File) for simulator (integrated mode)'
        )
        parser.add_argument(
            '--nsim-props', help='nsim props filename for simulator (integrated mode)'
        )

        # Integrated mode - Hardware options
        parser.add_argument(
            '--jtag',
            default='jtag-digilent',
            help='JTAG adapter type for hardware (default: jtag-digilent)',
        )
        parser.add_argument('--jtag-device', help='JTAG device name for hardware (e.g., JtagHs2)')
        parser.add_argument(
            '--jtag-frequency',
            default='500KHz',
            help='JTAG frequency for hardware (default: 500KHz)',
        )

        # Common options
        parser.add_argument(
            '--gui', action=argparse.BooleanOptionalAction, help='Launch VS Code GUI for debugging'
        )

        # Postconnect options
        parser.add_argument(
            '--postconnect-cmd',
            action='append',
            help='Command to execute after connection (can be used multiple times)',
        )
        parser.add_argument(
            '--postconnect-file',
            help='File containing commands to execute after connection',
        )

        # Board configuration
        parser.add_argument(
            '--board-json',
            help='Path to board.json file for board configuration',
        )

    @classmethod
    def do_create(cls, cfg, args):
        return LldbacBinaryRunner(
            cfg,
            props=args.nsim_props,
            tcf=args.tcf,
            simulator=args.simulator,
            jtag=args.jtag,
            jtag_device=args.jtag_device,
            jtag_frequency=args.jtag_frequency,
            gui=args.gui,
            postconnect_cmds=args.postconnect_cmd,
            postconnect_file=args.postconnect_file,
            board_json=args.board_json,
        )

    def do_run(self, command, **kwargs):
        if command == 'flash':
            self.do_flash(**kwargs)
        elif command == 'debug':
            # Integrated mode - runner manages server
            self.do_debug_integrated(**kwargs)
        else:
            raise ValueError(f'command {command} not supported by {self.name()}')

    def do_flash(self, **kwargs):
        '''Flash and run on physical hardware using run-lldbac.'''
        if self.simulator:
            raise ValueError(
                "Flash command is only available for hardware. "
                "For simulator execution, use arc-nsim runner: "
                "'west flash --runner arc-nsim'"
            )

        # Require run-lldbac for flash
        self.require(self.run_lldbac_cmd)

        # Create run-lldbac command for hardware flash
        lldbac_cmd = [
            self.run_lldbac_cmd,
            '--batch',  # Exit after executing commands
        ]

        # Use board.json if provided, otherwise build core properties
        if self.board_json:
            lldbac_cmd.extend(['--cores-json', self.board_json])
            self.logger.info(f"Using board configuration from: {self.board_json}")
        else:
            # Build core properties for hardware connection
            core_props = self._build_core_properties_for_hardware()
            lldbac_cmd.extend(['--core', core_props])

        # Add postconnect commands
        lldbac_cmd.extend(self._build_postconnect_args())

        lldbac_cmd.extend(['-o', 'c'])

        if self.gui:
            lldbac_cmd.append('--gui')

        lldbac_cmd.append(self.cfg.elf_file)

        self.logger.info(f"Flashing to hardware (JTAG: {self.jtag})")
        self.check_call(lldbac_cmd)

    def do_debug_integrated(self, **kwargs):
        '''Integrated debug mode - runner manages server via run-lldbac.'''
        # Require run-lldbac for integrated debug
        self.require(self.run_lldbac_cmd)

        lldbac_cmd = [self.run_lldbac_cmd]

        # Use board.json if provided (works for both simulator and hardware)
        if self.board_json:
            lldbac_cmd.extend(['--cores-json', self.board_json])
            self.logger.info(f"Using board configuration from: {self.board_json}")
        elif self.simulator:
            # Simulator mode: use --nsim or --tcf
            if self.tcf:
                lldbac_cmd.extend(['--tcf', self.tcf])
                self.logger.info(f"Starting integrated simulator debug (TCF: {self.tcf})")
            else:
                nsim_props_file = self._get_nsim_props()
                nsim_props_string = self._read_nsim_props(nsim_props_file)
                lldbac_cmd.extend(['--nsim', nsim_props_string])
                self.logger.info(f"Starting integrated simulator debug (props: {nsim_props_file})")
        else:
            # Hardware mode: use --core with connection properties
            core_props = self._build_core_properties_for_hardware()
            lldbac_cmd.extend(['--core', core_props])
            self.logger.info(f"Starting integrated hardware debug (JTAG: {self.jtag})")

        # Add postconnect commands
        lldbac_cmd.extend(self._build_postconnect_args())

        # Load ELF in debug mode
        lldbac_cmd.extend(['-o', 'load'])

        if self.gui:
            lldbac_cmd.append('--gui')

        lldbac_cmd.append(self.cfg.elf_file)

        # Run interactively
        self.run_client(lldbac_cmd)

    def _build_core_properties_for_hardware(self):
        '''Build core properties string for run-lldbac --core argument.

        Format: "connect=<type> connect-props=key1=val1 connect-props=key2=val2"
        '''
        props = [f'connect={self.jtag}']

        if self.jtag_device:
            props.append(f'connect-props=jtag_device={self.jtag_device}')

        if self.jtag_frequency:
            props.append(f'connect-props=jtag_frequency={self.jtag_frequency}')

        return ' '.join(props)

    def _build_postconnect_args(self):
        '''Build postconnect arguments for run-lldbac.

        Returns a list of arguments to add to the run-lldbac command.
        '''
        args = []

        # Add postconnect commands from --postconnect-cmd
        if self.postconnect_cmds:
            for cmd in self.postconnect_cmds:
                args.extend(['--postconnect', cmd])

        # Add postconnect file from --postconnect-file
        if self.postconnect_file:
            args.extend(['--postconnect', f'command source {self.postconnect_file}'])

        return args

    def _get_nsim_props(self):
        '''Get nSIM properties file path: board_dir/support/<props>.'''
        if not self.props:
            raise ValueError(
                "nsim-props filename is required but not provided. "
                "Either configure it in board.cmake or use --tcf for TCF-based configuration"
            )

        # Check if absolute path provided
        if path.isabs(self.props):
            return self.props

        # Look in board support directory
        if hasattr(self.cfg, 'board_dir') and self.cfg.board_dir:
            board_props = path.join(self.cfg.board_dir, 'support', self.props)
            if path.exists(board_props):
                return board_props

        # Fallback to current directory
        if path.exists(self.props):
            return self.props

        raise ValueError(f"Could not find nsim props file: {self.props}")

    def _read_nsim_props(self, props_file):
        '''Read nSIM properties file and convert to string format for run-lldbac.

        Reads a props file with lines like:
            nsim_isa_family=rv32
            nsim_isa_ext=All

        And converts to a space-separated string:
            "nsim_isa_family=rv32 nsim_isa_ext=All"
        '''
        properties = []
        with open(props_file, encoding='utf-8') as f:
            for line in f:
                # Strip whitespace and comments
                line = line.strip()
                if line and not line.startswith('#'):
                    properties.append(line)

        return ' '.join(properties)
