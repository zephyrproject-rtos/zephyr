# Copyright 2023 NXP
# SPDX-License-Identifier: Apache-2.0
"""
Runner for NXP P&E Micro Multilink Debug Probe.
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

from runners.core import BuildConfiguration, RunnerCaps, RunnerConfig, ZephyrBinaryRunner

NXP_PEDBG_USB_CLASS = 'usb'
NXP_PEDBG_USB_VID = 0x1357
NXP_PEDBG_USB_PID = 0x0089


@dataclass
class NXPPEDebugProbeConfig:
    """NXP P&E Micro Multilink Debug Probe configuration parameters."""

    dev_id: str = 'pedbg'
    server_port: int = 7224
    speed: int = 5000


class NXPPEDebugProbeRunner(ZephyrBinaryRunner):
    """Runner front-end for NXP P&E Micro Multilink Debug Probe."""

    def __init__(
        self,
        runner_cfg: RunnerConfig,
        probe_cfg: NXPPEDebugProbeConfig,
        soc_name: str,
        nxpide_path: str | None = None,
        pemicro_plugin_path: str | None = None,
        tool_opt: list[str] | None = None,
    ) -> None:
        super().__init__(runner_cfg)
        self.elf_file: str = runner_cfg.elf_file or ''
        self.probe_cfg: NXPPEDebugProbeConfig = probe_cfg
        self.soc_name: str = soc_name
        self.nxpide_path_override: str | None = nxpide_path
        self.pemicro_plugin_path: str | None = pemicro_plugin_path

        self.tool_opt: list[str] = []
        if tool_opt:
            for opt in tool_opt:
                self.tool_opt.extend(shlex.split(opt))

        build_cfg = BuildConfiguration(runner_cfg.build_dir)
        self.arch = build_cfg.get('CONFIG_ARCH').replace('"', '')

    @classmethod
    def name(cls) -> str:
        return 'nxp_pedbg'

    @classmethod
    def capabilities(cls) -> RunnerCaps:
        return RunnerCaps(commands={'debug', 'debugserver', 'attach'}, dev_id=True, tool_opt=True)

    @classmethod
    def dev_id_help(cls) -> str:
        return '''Debug probe connection string as in "pedbg[:<address>]"
                  where <address> can be the serial ID of the probe.'''

    @classmethod
    def tool_opt_help(cls) -> str:
        return '''Additional options for GDB client when used with "debug" or "attach" commands
                  or for P&E GDB server when used with "debugserver" command.'''

    @classmethod
    def do_add_parser(cls, parser: argparse.ArgumentParser) -> None:
        parser.add_argument(
            '--soc-name',
            required=True,
            help='SoC name as supported by the debug probe (e.g. "S32K148")',
        )
        parser.add_argument(
            '--nxpide-path',
            help='Override the path to NXP IDE installation '
            '(e.g. NXP S32 Design Studio). '
            'By default, this runner will try to obtain it from the system '
            'path, if available.',
        )
        parser.add_argument('--pemicro-plugin-path', help='Path to P&E Micro plugin.')
        parser.add_argument(
            '--server-port',
            default=NXPPEDebugProbeConfig.server_port,
            type=int,
            help='P&E GDB server port',
        )
        parser.add_argument(
            '--speed',
            default=NXPPEDebugProbeConfig.speed,
            type=int,
            help='Shift frequency is n KHz',
        )

    @classmethod
    def do_create(cls, cfg: RunnerConfig, args: argparse.Namespace) -> 'NXPPEDebugProbeRunner':
        probe_cfg = NXPPEDebugProbeConfig(
            args.dev_id, server_port=args.server_port, speed=args.speed
        )

        return NXPPEDebugProbeRunner(
            cfg,
            probe_cfg,
            args.soc_name,
            nxpide_path=args.nxpide_path,
            pemicro_plugin_path=args.pemicro_plugin_path,
            tool_opt=args.tool_opt,
        )

    def find_pemicro_plugin(self) -> list[str]:
        """Return a list of paths to installed P&E Micro plugins."""
        base_path = f'{self.nxpide_path}/eclipse/plugins'
        regex = re.compile(r'com\.pemicro\.debug\.gdbjtag\.pne_.*')

        return [os.path.join(base_path, f) for f in os.listdir(base_path) if regex.match(f)]

    def select_pemicro_plugin(self) -> str:
        """Find P&E Micro plugin and return the path."""
        plugins = self.find_pemicro_plugin()
        if not plugins:
            raise RuntimeError('there are no P&E Micro plugins installed')
        elif len(plugins) == 1:
            return plugins[0]
        else:
            print('There are multiple P&E Micro plugins installed')
            for i, plugin in enumerate(plugins, 1):
                print(f'{i}. {plugin}')

            prompt = f'Please select one (1-{len(plugins)}): '
            while True:
                try:
                    value: int = int(input(prompt))
                except EOFError:
                    sys.exit(0)
                except ValueError:
                    continue
                if 1 <= value <= len(plugins):
                    break
            return plugins[value - 1]

    @staticmethod
    def find_usb_probes() -> list[str]:
        """Return a list of debug probe serial numbers connected via USB to this host."""
        # use system's native commands to enumerate and retrieve the USB serial ID
        # to avoid bloating this runner with third-party dependencies that often
        # require priviledged permissions to access the device info
        serialid_pattern = r'sdafd[0-9a-f]{6}'
        if platform.system() == 'Windows':
            coding = 'windows-1252'
            cmd = f'pnputil /enum-devices /connected /class "{NXP_PEDBG_USB_CLASS}"'
            pattern = (
                rf'instance id:\s+usb\\vid_{NXP_PEDBG_USB_VID:04x}.*'
                'pid_{NXP_PEDBG_USB_PID:04x}\\{serialid_pattern}'
            )
        else:
            coding = 'utf-8'
            serialid_pattern = r'sdafd[0-9a-f]{6}'
            cmd = f'lsusb -v -d {NXP_PEDBG_USB_VID:04x}:{NXP_PEDBG_USB_PID:04x}'
            pattern = f'iserial +[0-255] +{serialid_pattern}'

        try:
            outb = subprocess.check_output(shlex.split(cmd), stderr=subprocess.DEVNULL)
            out = outb.decode(coding).strip().lower()
        except subprocess.CalledProcessError as err:
            raise RuntimeError('error while looking for debug probes connected') from err

        devices: list[str] = []
        if out and 'no devices were found' not in out:
            devices = re.findall(pattern, out)

        return sorted(devices)

    @classmethod
    def get_probe(cls) -> str:
        """
        Find debugger probes connected and return the serial number.

        If there are multiple debugger probes connected print an information to disconnect unneeded
        probe(s).

        :raises RuntimeError: if no probes or multiple probes connected
        """
        probes_snr = cls.find_usb_probes()
        if not probes_snr:
            raise RuntimeError('there are no debug probes connected')
        elif len(probes_snr) == 1:
            return probes_snr[0]
        else:
            print('There are multiple debug probes connected')
            for i, probe in enumerate(probes_snr, 1):
                print(f'{i}. {probe}')

            print('Please disconnect any unnecessary probes and leave only one connected.')
            raise RuntimeError('multiple debug probes connected')

    @property
    def runtime_environment(self) -> dict[str, str] | None:
        """Execution environment used for the client process."""
        if platform.system() == 'Windows':
            python_lib = (
                self.nxpide_path
                / 'S32DS'
                / 'build_tools'
                / 'msys32'
                / 'mingw32'
                / 'lib'
                / 'python2.7'
            )
            return {
                **os.environ,
                'PYTHONPATH': f'{python_lib}{os.pathsep}{python_lib / "site-packages"}',
            }

        return None

    def server_commands(self) -> list[str]:
        """Get launch commands to start the P&E GDB server."""
        path = Path(self.pemicro_plugin_path) if self.pemicro_plugin_path else Path()
        if platform.system() == 'Linux':
            app = path / 'lin' / 'pegdbserver_console'
        else:
            app = path / 'win32' / 'pegdbserver_console'

        cmd = [str(app)]
        if self.soc_name == 'S32K148':
            cmd += ['-device=NXP_S32K1xx_S32K148F2M0M11']
        cmd += [
            '-startserver',
            '-singlesession',
            '-serverport=' + str(self.probe_cfg.server_port),
            '-gdbmiport=6224',
            '-interface=OPENSDA',
            '-speed=' + str(self.probe_cfg.speed),
            '-porD',
        ]
        return cmd

    def client_commands(self) -> list[str]:
        """Get launch commands to start the GDB client."""
        if self.arch == 'arm':
            client_exec_name = 'arm-none-eabi-gdb-py'
        elif self.arch == 'arm64':
            client_exec_name = 'aarch64-none-elf-gdb-py'
        else:
            raise RuntimeError(f'architecture {self.arch} not supported')

        client_exec = str(
            self.nxpide_path
            / 'S32DS'
            / 'tools'
            / 'gdb-arm'
            / 'arm32-eabi'
            / 'bin'
            / client_exec_name
        )
        cmd = [client_exec]
        return cmd

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
        self.nxpide_path = Path(self.require(app_name, path=self.nxpide_path_override)).parent

        if not self.pemicro_plugin_path:
            self.pemicro_plugin_path = f'{self.select_pemicro_plugin()}'

        if not self.probe_cfg.dev_id:
            self.probe_cfg.dev_id = f'{self.get_probe()}'
            if self.probe_cfg.dev_id:
                self.logger.info(f'using debug probe "pedbg:{self.probe_cfg.dev_id}"')

        if command in ('attach', 'debug'):
            self.ensure_output('elf')
            self.do_attach_debug(command, **kwargs)
        else:
            self.do_debugserver(**kwargs)

    def do_attach_debug(self, command: str, **kwargs) -> None:
        """
        Launch the P&E GDB server and GDB client to start a debugging session.

        :param command: command name to execute
        """
        gdb_script: list[str] = []

        gdb_script.append('target remote localhost:' + str(self.probe_cfg.server_port))
        gdb_script.append('monitor reset')

        gdb_script.append(f'file {Path(self.elf_file).as_posix()}')
        if command == 'debug':
            gdb_script.append('load')

        with tempfile.TemporaryDirectory(suffix='nxp_pedbg') as tmpdir:
            gdb_cmds = Path(tmpdir) / 'runner.nxp_pedbg'
            gdb_cmds.write_text('\n'.join(gdb_script), encoding='utf-8')
            self.logger.debug(gdb_cmds.read_text(encoding='utf-8'))

            server_cmd = self.server_commands()
            print(server_cmd)
            client_cmd = self.client_commands()
            client_cmd.extend(['-x', gdb_cmds.as_posix()])
            client_cmd.extend(self.tool_opt)
            print(client_cmd)

            self.run_server_and_client(server_cmd, client_cmd, env=self.runtime_environment)

    def do_debugserver(self, **kwargs) -> None:
        """Start the P&E GDB server on a given port with the given extra parameters from cli."""
        server_cmd = self.server_commands()
        server_cmd.extend(self.tool_opt)
        self.check_call(server_cmd)
