# Copyright (c) 2025 Synopsys Inc.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for Synopsys ARC MetaWare Debugger (lldbac).'''

import shutil
from os import path

from runners.core import RunnerCaps, ZephyrBinaryRunner

DEFAULT_LLDBAC_GDB_PORT = 3333


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
        gdb_host=None,
        gdb_port=DEFAULT_LLDBAC_GDB_PORT,
    ):
        super().__init__(cfg)

        # Check for run-lldbac tool upfront
        # run-lldbac is used for all commands (flash, debug, debugserver)
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
        self.gdb_host = gdb_host
        self.gdb_port = gdb_port

    @classmethod
    def name(cls):
        return 'lldbac'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash', 'debug', 'debugserver'})

    @classmethod
    def do_add_parser(cls, parser):
        # Integrated mode - Simulator options
        parser.add_argument(
            '--simulator',
            action='store_true',
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

        # Client-only mode options (debugserver command)
        # Note: 'debugserver' connects lldbac client to external GDB server
        # (non-standard: most runners use 'debugserver' to start a server)
        parser.add_argument(
            '--gdb-host', help='GDB server host for debugserver command (default: localhost)'
        )
        parser.add_argument(
            '--gdb-port',
            type=int,
            default=DEFAULT_LLDBAC_GDB_PORT,
            help=f'GDB server port for debugserver command (default: {DEFAULT_LLDBAC_GDB_PORT})',
        )

        # Common options
        parser.add_argument('--gui', action='store_true', help='Launch VS Code GUI for debugging')

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
            gdb_host=args.gdb_host,
            gdb_port=args.gdb_port,
        )

    def do_run(self, command, **kwargs):
        if command == 'flash':
            self.do_flash(**kwargs)
        elif command == 'debug':
            # Integrated mode - runner manages server
            self.do_debug_integrated(**kwargs)
        elif command == 'debugserver':
            # Client-only mode - connect to existing server
            self.do_debugserver_client(**kwargs)
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

        if not self.jtag_device:
            raise ValueError(
                "Hardware flash requires --jtag-device. Example:\n"
                "  west flash --runner lldbac --jtag-device=JtagHs2"
            )

        # Require run-lldbac for flash
        self.require(self.run_lldbac_cmd)

        # Build platform connect command
        connect_cmd = self._build_platform_connect_cmd()

        # Create run-lldbac script for hardware flash
        lldbac_cmd = [
            self.run_lldbac_cmd,
            '--batch',  # Exit after executing commands
            '-o',
            connect_cmd,
            '-o',
            'run',
        ]

        if self.gui:
            lldbac_cmd.append('--gui')

        lldbac_cmd.append(self.cfg.elf_file)

        self.logger.info(f"Flashing to hardware (JTAG: {self.jtag}, device: {self.jtag_device})")
        self.check_call(lldbac_cmd)

    def do_debug_integrated(self, **kwargs):
        '''Integrated debug mode - runner manages server via run-lldbac.'''
        # Require run-lldbac for integrated debug
        self.require(self.run_lldbac_cmd)

        lldbac_cmd = [self.run_lldbac_cmd]

        if self.simulator:
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
            # Hardware mode: use platform connect
            if not self.jtag_device:
                raise ValueError(
                    "Hardware debug requires --jtag-device. Example:\n"
                    "  west debug --runner lldbac --jtag-device=JtagHs2"
                )

            connect_cmd = self._build_platform_connect_cmd()
            lldbac_cmd.extend(['-o', connect_cmd])
            self.logger.info(
                f"Starting integrated hardware debug "
                f"(JTAG: {self.jtag}, device: {self.jtag_device})"
            )

        # Load ELF in debug mode
        lldbac_cmd.extend(['-o', 'load'])

        if self.gui:
            lldbac_cmd.append('--gui')

        lldbac_cmd.append(self.cfg.elf_file)

        # Run interactively
        self.run_client(lldbac_cmd)

    def do_debugserver_client(self, **kwargs):
        '''Client-only mode - connect to externally managed GDB server.

        User must start the GDB server manually before running this command.
        For simulator: nsimdrv -gdb -port=3333 -propsfile board.props
        For hardware: lldbac gdbserver --jtag :3333
        '''
        gdb_host = self.gdb_host or 'localhost'

        self.logger.info(f"Connecting lldbac client to GDB server at {gdb_host}:{self.gdb_port}")
        self.logger.info("Ensure your GDB server is already running!")
        self.logger.info(f"Example: nsimdrv -gdb -port={self.gdb_port} -propsfile board.props")

        # Always use run-lldbac with gdb-server connection
        self.require(self.run_lldbac_cmd)

        lldbac_cmd = [
            self.run_lldbac_cmd,
            '--core',
            f'connect=gdb-server gdb-server=connect://{gdb_host}:{self.gdb_port}',
        ]

        if self.gui:
            lldbac_cmd.append('--gui')

        lldbac_cmd.append(self.cfg.elf_file)

        self.logger.debug(f"Running command: {' '.join(lldbac_cmd)}")
        self.check_call(lldbac_cmd)

    def _build_platform_connect_cmd(self):
        '''Build platform connect command from JTAG flags.'''
        parts = [f'platform connect --connect={self.jtag}']

        if self.jtag_device:
            parts.append(f'--jtag-device={self.jtag_device}')

        if self.jtag_frequency:
            parts.append(f'--jtag-frequency={self.jtag_frequency}')

        return ' '.join(parts)

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
