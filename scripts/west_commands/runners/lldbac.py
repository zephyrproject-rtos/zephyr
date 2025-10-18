# Copyright (c) 2025 Synopsys Inc.
#
# SPDX-License-Identifier: Apache-2.0

'''lldbac runner using run-lldbac tool.

This runner provides `debug` for simulator and `flash`/`debug` for hardware.

Usage Examples:
  Simulator mode:
    west debug --runner lldbac                    # Debug on nsim

  Hardware mode:
    west flash --runner lldbac --hardware --board-json=myboard.json
    west debug --runner lldbac --hardware --board-json=myboard.json

Note on simulator flash:
  Flash is intentionally not supported for simulator mode. Use the arc-nsim
  runner for simulator execution without debugging: 'west flash --runner arc-nsim'
  The lldbac runner is specifically designed for interactive debugging workflows.

Configuration:
  --hardware: Use physical hardware instead of simulator
  --tcf: Tool Configuration File for target configuration (optional)
         For simulator: mutually exclusive with --nsim-props
         For hardware: can be used alongside --board-json
  --nsim-props: nsim properties filename (board-specific, no default)
                Only used for simulator mode when --tcf is not specified
  --board-json: board.json file for hardware connection (required for hardware mode)

Hardware Mode:
  Hardware mode requires a board.json file containing JTAG configuration.
  The file path must be explicitly specified with --board-json argument.

  Expected board.json format:
  [
    {
      "connect": "jtag-digilent",
      "jtag_device": "JtagHs2",
      "jtag_frequency": "500KHz",
      "postconnect": ["command source preload.cmd"]
    }
  ]

  Note: The postconnect field is optional. You can add any run-lldbac
  compatible parameters to the board.json configuration.

Board Configuration:
  Each board should specify its nsim properties file in board.cmake:
    board_runner_args(lldbac "--nsim-props=board_specific.props")

  For hardware support, users must explicitly specify the board.json file:
    west debug --runner lldbac --hardware --board-json=path/to/board.json
'''

from os import path

from runners.core import RunnerCaps, ZephyrBinaryRunner


class LldbacBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for run-lldbac tool for simulator and hardware.'''

    def __init__(self, cfg, props, tcf=None, hardware=False, board_json=None, gui=False):
        super().__init__(cfg)
        self.lldbac_cmd = ['run-lldbac']
        self.tcf = tcf
        self.props = props
        self.hardware = hardware
        self.board_json = board_json
        self.gui = gui

    @classmethod
    def name(cls):
        return 'lldbac'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash', 'debug'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument(
            '--hardware', action='store_true', help='Use physical hardware instead of simulator'
        )
        parser.add_argument('--tcf', help='TCF (Tool Configuration File) for target configuration')
        parser.add_argument(
            '--nsim-props', help='nsim props filename (looked up under board support)'
        )
        parser.add_argument(
            '--board-json',
            help='board.json file for hardware connection (required for hardware mode)',
        )
        parser.add_argument('--gui', action='store_true', help='Launch VS Code GUI for debugging')

    @classmethod
    def do_create(cls, cfg, args):
        chosen_props = args.nsim_props

        # Get board.json for hardware mode
        board_json = None
        if args.hardware:
            board_json = cls._get_board_json(cfg, args)

        return LldbacBinaryRunner(
            cfg,
            props=chosen_props,
            tcf=args.tcf,
            hardware=args.hardware,
            board_json=board_json,
            gui=args.gui,
        )

    @classmethod
    def _get_board_json(cls, cfg, args):
        '''Get board.json file path for hardware mode'''
        if not args.board_json:
            raise ValueError(
                "Hardware mode requires --board-json argument. "
                "Specify the board.json file path with --board-json"
            )

        # Handle relative or absolute path
        if path.isabs(args.board_json):
            board_json_path = args.board_json
        else:
            board_json_path = path.join(cfg.board_dir, args.board_json)

        # Verify the file exists
        if not path.exists(board_json_path):
            raise ValueError(f"Board JSON file not found: {board_json_path}")

        return board_json_path

    def do_run(self, command, **kwargs):
        # Log mode selection to help diagnose environment issues
        config = f"tcf={self.tcf}" if self.tcf else f"props={self.props}"
        self.logger.info(f"lldbac: mode={'hardware' if self.hardware else 'sim'} {config}")
        if command == 'flash':
            self.do_flash(**kwargs)
        elif command == 'debug':
            self.require(self.lldbac_cmd[0])
            self.do_debug(**kwargs)
        else:
            raise ValueError(f'command {command} not supported by {self.name()}')

    def do_flash(self, **kwargs):
        '''Flash and run on physical hardware using run-lldbac.

        Note: Flash is intentionally not supported for simulator mode.
        The arc-nsim runner already provides efficient simulator execution
        without debugging overhead. Use 'west flash --runner arc-nsim' for
        simulator runs.
        '''
        if not self.hardware:
            raise ValueError(
                "Flash command is only available for hardware. "
                "For simulator execution, use arc-nsim runner: "
                "'west flash --runner arc-nsim'"
            )

        self.require(self.lldbac_cmd[0])

        # Create run-lldbac script for hardware flash
        lldbac_cmds = [
            self.lldbac_cmd[0],
            '--batch',  # Exit after executing commands
            '-o',
            f'platform connect {self.board_json}',
            '-o',
            'run',
        ]

        if self.tcf:
            lldbac_cmds.extend(['--tcf', self.tcf])

        if self.gui:
            lldbac_cmds.append('--gui')

        lldbac_cmds.append(self.cfg.elf_file)

        self.logger.info(f"Flashing to hardware using board.json: {self.board_json}")
        self.check_call(lldbac_cmds)

    def do_debug(self, **kwargs):
        '''Start interactive debugging session'''
        if self.hardware:
            self._do_debug_hardware(**kwargs)
        else:
            self._do_debug_simulator(**kwargs)

    def _do_debug_simulator(self, **kwargs):
        '''Debug on nsim simulator'''
        # run-lldbac with --nsim manages nSIM internally
        lldbac_cmd = self.lldbac_cmd.copy()

        if self.tcf:
            # TCF mode: use TCF file for target configuration
            lldbac_cmd.extend(['--tcf', self.tcf])
        else:
            # Props mode: use nsim properties for target configuration
            nsim_props_file = self._get_nsim_props()
            self.logger.info(f"lldbac: using nsim props: {nsim_props_file}")
            nsim_props_string = self._read_nsim_props(nsim_props_file)
            lldbac_cmd.extend(['--nsim', nsim_props_string])

        lldbac_cmd.extend(['-o', 'load'])

        if self.gui:
            lldbac_cmd.append('--gui')

        lldbac_cmd.append(self.cfg.elf_file)

        # Run run-lldbac interactively
        self.run_client(lldbac_cmd)

    def _do_debug_hardware(self, **kwargs):
        '''Debug on physical hardware'''
        self.require(self.lldbac_cmd[0])

        # Interactive debug session on hardware
        lldbac_cmd = self.lldbac_cmd + ['-o', f'platform connect {self.board_json}']

        if self.tcf:
            lldbac_cmd.extend(['--tcf', self.tcf])

        if self.gui:
            lldbac_cmd.append('--gui')

        lldbac_cmd.append(self.cfg.elf_file)

        self.logger.info(f"Starting debug session on hardware using board.json: {self.board_json}")
        # Run interactively
        self.run_client(lldbac_cmd)

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
