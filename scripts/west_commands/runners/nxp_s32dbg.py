# Copyright 2023 NXP
# SPDX-License-Identifier: Apache-2.0
"""
Runner for NXP S32 Debug Probe.
"""

import argparse
import os
import platform
import re
import shlex
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional, Union

from runners.core import (BuildConfiguration, RunnerCaps, RunnerConfig,
                          ZephyrBinaryRunner)

NXP_S32DBG_USB_CLASS = 'NXP Probes'
NXP_S32DBG_USB_VID = 0x15a2
NXP_S32DBG_USB_PID = 0x0067


@dataclass
class NXPS32DebugProbeConfig:
    """NXP S32 Debug Probe configuration parameters."""
    conn_str: str = 's32dbg'
    server_port: int = 45000
    speed: int = 16000
    remote_timeout: int = 30
    reset_type: str | None = 'default'
    reset_delay: int = 0


class NXPS32DebugProbeRunner(ZephyrBinaryRunner):
    """Runner front-end for NXP S32 Debug Probe."""

    def __init__(self,
                 runner_cfg: RunnerConfig,
                 probe_cfg: NXPS32DebugProbeConfig,
                 core_name: str,
                 soc_name: str,
                 soc_family_name: str,
                 start_all_cores: bool,
                 s32ds_path: str | None = None,
                 tool_opt: list[str] | None = None) -> None:
        super().__init__(runner_cfg)
        self.elf_file: str = runner_cfg.elf_file or ''
        self.probe_cfg: NXPS32DebugProbeConfig = probe_cfg
        self.core_name: str = core_name
        self.soc_name: str = soc_name
        self.soc_family_name: str = soc_family_name
        self.start_all_cores: bool = start_all_cores
        self.s32ds_path_override: str | None = s32ds_path

        self.tool_opt: list[str] = []
        if tool_opt:
            for opt in tool_opt:
                self.tool_opt.extend(shlex.split(opt))

        build_cfg = BuildConfiguration(runner_cfg.build_dir)
        self.arch = build_cfg.get('CONFIG_ARCH').replace('"', '')

    @classmethod
    def name(cls) -> str:
        return 'nxp_s32dbg'

    @classmethod
    def capabilities(cls) -> RunnerCaps:
        return RunnerCaps(commands={'debug', 'debugserver', 'attach'},
                          dev_id=True, tool_opt=True)

    @classmethod
    def dev_id_help(cls) -> str:
        return '''Debug probe connection string as in "s32dbg[:<address>]"
                  where <address> can be the IP address if TAP is available via Ethernet,
                  the serial ID of the probe or empty if TAP is available via USB.'''

    @classmethod
    def tool_opt_help(cls) -> str:
        return '''Additional options for GDB client when used with "debug" or "attach" commands
                  or for GTA server when used with "debugserver" command.'''

    @classmethod
    def do_add_parser(cls, parser: argparse.ArgumentParser) -> None:
        parser.add_argument('--core-name',
                            required=True,
                            help='Core name as supported by the debug probe (e.g. "R52_0_0")')
        parser.add_argument('--soc-name',
                            required=True,
                            help='SoC name as supported by the debug probe (e.g. "S32Z270")')
        parser.add_argument('--soc-family-name',
                            required=True,
                            help='SoC family name as supported by the debug probe (e.g. "s32z2e2")')
        parser.add_argument('--start-all-cores',
                            action='store_true',
                            help='Start all SoC cores and not just the one being debugged. '
                                 'Use together with "debug" command.')
        parser.add_argument('--s32ds-path',
                            help='Override the path to NXP S32 Design Studio installation. '
                                 'By default, this runner will try to obtain it from the system '
                                 'path, if available.')
        parser.add_argument('--server-port',
                            default=NXPS32DebugProbeConfig.server_port,
                            type=int,
                            help='GTA server port')
        parser.add_argument('--speed',
                            default=NXPS32DebugProbeConfig.speed,
                            type=int,
                            help='JTAG interface speed')
        parser.add_argument('--remote-timeout',
                            default=NXPS32DebugProbeConfig.remote_timeout,
                            type=int,
                            help='Number of seconds to wait for the remote target responses')

    @classmethod
    def do_create(cls, cfg: RunnerConfig, args: argparse.Namespace) -> 'NXPS32DebugProbeRunner':
        probe_cfg = NXPS32DebugProbeConfig(args.dev_id,
                                           server_port=args.server_port,
                                           speed=args.speed,
                                           remote_timeout=args.remote_timeout)

        return NXPS32DebugProbeRunner(cfg, probe_cfg, args.core_name, args.soc_name,
                                      args.soc_family_name, args.start_all_cores,
                                      s32ds_path=args.s32ds_path, tool_opt=args.tool_opt)

    @staticmethod
    def find_usb_probes() -> list[str]:
        """Return a list of debug probe serial numbers connected via USB to this host."""
        # use system's native commands to enumerate and retrieve the USB serial ID
        # to avoid bloating this runner with third-party dependencies that often
        # require priviledged permissions to access the device info
        macaddr_pattern = r'(?:[0-9a-f]{2}[:]){5}[0-9a-f]{2}'
        if platform.system() == 'Windows':
            cmd = f'pnputil /enum-devices /connected /class "{NXP_S32DBG_USB_CLASS}"'
            serialid_pattern = f'instance id: +usb\\\\.*\\\\({macaddr_pattern})'
        else:
            cmd = f'lsusb -v -d {NXP_S32DBG_USB_VID:x}:{NXP_S32DBG_USB_PID:x}'
            serialid_pattern = f'iserial +.*({macaddr_pattern})'

        try:
            outb = subprocess.check_output(shlex.split(cmd), stderr=subprocess.DEVNULL)
            out = outb.decode('utf-8').strip().lower()
        except subprocess.CalledProcessError as err:
            raise RuntimeError('error while looking for debug probes connected') from err

        devices: list[str] = []
        if out and 'no devices were found' not in out:
            devices = re.findall(serialid_pattern, out)

        return sorted(devices)

    @classmethod
    def select_probe(cls) -> str:
        """
        Find debugger probes connected and return the serial number of the one selected.

        If there are multiple debugger probes connected and this runner is being executed
        in a interactive prompt, ask the user to select one of the probes.
        """
        probes_snr = cls.find_usb_probes()
        if not probes_snr:
            raise RuntimeError('there are no debug probes connected')
        elif len(probes_snr) == 1:
            return probes_snr[0]
        else:
            if not sys.stdin.isatty():
                raise RuntimeError(
                    f'refusing to guess which of {len(probes_snr)} connected probes to use '
                    '(Interactive prompts disabled since standard input is not a terminal). '
                    'Please specify a device ID on the command line.')

            print('There are multiple debug probes connected')
            for i, probe in enumerate(probes_snr, 1):
                print(f'{i}. {probe}')

            prompt = f'Please select one with desired serial number (1-{len(probes_snr)}): '
            while True:
                try:
                    value: int = int(input(prompt))
                except EOFError:
                    sys.exit(0)
                except ValueError:
                    continue
                if 1 <= value <= len(probes_snr):
                    break
            return probes_snr[value - 1]

    @property
    def runtime_environment(self) -> dict[str, str] | None:
        """Execution environment used for the client process."""
        if platform.system() == 'Windows':
            python_lib = (self.s32ds_path / 'S32DS' / 'build_tools' / 'msys32'
                        / 'mingw32' / 'lib' / 'python2.7')
            return {
                **os.environ,
                'PYTHONPATH': f'{python_lib}{os.pathsep}{python_lib / "site-packages"}'
            }

        return None

    @property
    def script_globals(self) -> dict[str, str | int | None]:
        """Global variables required by the debugger scripts."""
        return {
            '_PROBE_IP': self.probe_cfg.conn_str,
            '_JTAG_SPEED': self.probe_cfg.speed,
            '_GDB_SERVER_PORT': self.probe_cfg.server_port,
            '_RESET_TYPE': self.probe_cfg.reset_type,
            '_RESET_DELAY': self.probe_cfg.reset_delay,
            '_REMOTE_TIMEOUT': self.probe_cfg.remote_timeout,
            '_CORE_NAME': f'{self.soc_name}_{self.core_name}',
            '_SOC_NAME': self.soc_name,
            '_IS_LOGGING_ENABLED': False,
            '_FLASH_NAME': None,    # not supported
            '_SECURE_TYPE': None,   # not supported
            '_SECURE_KEY': None,    # not supported
        }

    def server_commands(self) -> list[str]:
        """Get launch commands to start the GTA server."""
        server_exec = str(self.s32ds_path / 'S32DS' / 'tools' / 'S32Debugger'
                          / 'Debugger' / 'Server' / 'gta' / 'gta')
        cmd = [server_exec, '-p', str(self.probe_cfg.server_port)]
        return cmd

    def client_commands(self) -> list[str]:
        """Get launch commands to start the GDB client."""
        if self.arch == 'arm':
            client_exec_name = 'arm-none-eabi-gdb-py'
        elif self.arch == 'arm64':
            client_exec_name = 'aarch64-none-elf-gdb-py'
        else:
            raise RuntimeError(f'architecture {self.arch} not supported')

        client_exec = str(self.s32ds_path / 'S32DS' / 'tools' / 'gdb-arm'
                          / 'arm32-eabi' / 'bin' / client_exec_name)
        cmd = [client_exec]
        return cmd

    def get_script(self, name: str) -> Path:
        """
        Get the file path of a debugger script with the given name.

        :param name: name of the script, without the SoC family name prefix
        :returns: path to the script
        :raises RuntimeError: if file does not exist
        """
        script = (self.s32ds_path / 'S32DS' / 'tools' / 'S32Debugger' / 'Debugger' / 'scripts'
                  / self.soc_family_name / f'{self.soc_family_name}_{name}.py')
        if not script.exists():
            raise RuntimeError(f'script not found: {script}')
        return script

    def do_run(self, command: str, **kwargs) -> None:
        """
        Execute the given command.

        :param command: command name to execute
        :raises RuntimeError: if target architecture or host OS is not supported
        :raises MissingProgram: if required tools are not found in the host
        """
        if platform.system() not in ('Windows', 'Linux'):
            raise RuntimeError(f'runner not supported on {platform.system()} systems')

        if self.arch not in ('arm', 'arm64'):
            raise RuntimeError(f'architecture {self.arch} not supported')

        app_name = 's32ds' if platform.system() == 'Windows' else 's32ds.sh'
        self.s32ds_path = Path(self.require(app_name, path=self.s32ds_path_override)).parent

        if not self.probe_cfg.conn_str:
            self.probe_cfg.conn_str = f's32dbg:{self.select_probe()}'
            self.logger.info(f'using debug probe {self.probe_cfg.conn_str}')

        if command in ('attach', 'debug'):
            self.ensure_output('elf')
            self.do_attach_debug(command, **kwargs)
        else:
            self.do_debugserver(**kwargs)

    def do_attach_debug(self, command: str, **kwargs) -> None:
        """
        Launch the GTA server and GDB client to start a debugging session.

        :param command: command name to execute
        """
        gdb_script: list[str] = []

        # setup global variables required for the scripts before sourcing them
        for name, val in self.script_globals.items():
            gdb_script.append(f'py {name} = {repr(val)}')

        # load platform-specific debugger script
        if command == 'debug':
            if self.start_all_cores:
                startup_script = self.get_script('generic_bareboard_all_cores')
            else:
                startup_script = self.get_script('generic_bareboard')
        else:
            startup_script = self.get_script('attach')
        gdb_script.append(f'source {startup_script}')

        # executes the SoC and board initialization sequence
        if command == 'debug':
            gdb_script.append('py board_init()')

        # initializes the debugger connection to the core specified
        gdb_script.append('py core_init()')

        gdb_script.append(f'file {Path(self.elf_file).as_posix()}')
        if command == 'debug':
            gdb_script.append('load')

        with tempfile.TemporaryDirectory(suffix='nxp_s32dbg') as tmpdir:
            gdb_cmds = Path(tmpdir) / 'runner.nxp_s32dbg'
            gdb_cmds.write_text('\n'.join(gdb_script), encoding='utf-8')
            self.logger.debug(gdb_cmds.read_text(encoding='utf-8'))

            server_cmd = self.server_commands()
            client_cmd = self.client_commands()
            client_cmd.extend(['-x', gdb_cmds.as_posix()])
            client_cmd.extend(self.tool_opt)

            self.run_server_and_client(server_cmd, client_cmd, env=self.runtime_environment)

    def do_debugserver(self, **kwargs) -> None:
        """Start the GTA server on a given port with the given extra parameters from cli."""
        server_cmd = self.server_commands()
        server_cmd.extend(self.tool_opt)
        self.check_call(server_cmd)
