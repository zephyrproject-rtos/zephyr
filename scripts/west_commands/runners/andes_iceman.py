# Copyright (c) 2026 Andes Technology Corporation
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing and debugging with Andes ICEman.'''

import os
import pathlib
import re
import shlex
import socket
import subprocess
import threading
from dataclasses import dataclass

from runners.core import BuildConfiguration, RunnerCaps, ZephyrBinaryRunner

DEFAULT_ICEMAN = 'ICEman'
DEFAULT_SPI_BURN = 'SPI_burn'
ICEMAN_READY_MSG = 'ICEman is ready to use.'
ICEMAN_READY_TIMEOUT = 30
TELNET_TIMEOUT = 30


@dataclass
class IcemanServer:
    '''Host and ports of ICEman. A None host starts ICEman on the local
    host, and a None port selects the port reported by the started ICEman.'''

    host: str | None = None
    gdb_port: int | None = None
    burner_port: int | None = None
    telnet_port: int | None = None


@dataclass
class TargetConfig:
    '''Target properties from the build configuration.'''

    kernel_entry: str | None = None
    xip: bool = False
    smp: bool = False
    riscv64: bool = False
    boot_hart: int = 0


@dataclass
class FlashLayout:
    '''Flash addresses of the image from the build configuration.'''

    rom_start_offset: int = 0
    flash_load_offset: int = 0
    flash_base_address: int = 0


class AndesIcemanBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for Andes ICEman.'''

    def __init__(
        self,
        cfg,
        target_conf,
        flash_layout,
        iceman_server=None,
        iceman=DEFAULT_ICEMAN,
        spi_burn=DEFAULT_SPI_BURN,
        reset_mtime=False,
        dev_id=None,
        erase=False,
        load=True,
        gdb_init=None,
        tool_opt=None,
    ):
        super().__init__(cfg)

        self.target_conf = target_conf
        self.flash_layout = flash_layout
        self.iceman_server = iceman_server or IcemanServer()
        self.iceman = iceman
        self.spi_burn = spi_burn
        self.reset_mtime = reset_mtime
        self.dev_id = dev_id
        self.erase = bool(erase)
        self.load = load
        self.gdb_init = gdb_init or []
        self.iceman_proc = None

        self.tool_opt = []
        if tool_opt is not None:
            for opts in [shlex.split(opt) for opt in tool_opt]:
                self.tool_opt += opts

    @classmethod
    def name(cls):
        return 'andes-iceman'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(
            commands={'flash', 'debug', 'attach'},
            dev_id=True,
            erase=True,
            tool_opt=True,
            gdb_init=True,
            skip_load=True,
        )

    @classmethod
    def dev_id_help(cls) -> str:
        return 'ICE adapter ID for the started ICEman, see ICEman --list-device'

    @classmethod
    def tool_opt_help(cls) -> str:
        return "Additional options for the started ICEman, e.g. '--detect-2wire'"

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument(
            '--iceman',
            default=DEFAULT_ICEMAN,
            help=f'path to ICEman, defaults to {DEFAULT_ICEMAN}',
        )
        parser.add_argument(
            '--iceman-host',
            default=None,
            help='host name or IP of a running ICEman to connect to, '
            'instead of starting ICEman on this host',
        )
        parser.add_argument(
            '--gdb-port',
            type=int,
            default=None,
            help='ICEman GDB server port of core #0, required with --iceman-host, '
            'defaults to the port reported by the started ICEman',
        )
        parser.add_argument(
            '--burner-port',
            type=int,
            default=None,
            help='ICEman burner port for SPI_burn, required with --iceman-host, '
            'defaults to the port reported by the started ICEman',
        )
        parser.add_argument(
            '--telnet-port',
            type=int,
            default=None,
            help='ICEman telnet port, required with --iceman-host, to reset and halt '
            'the target before GDB connects',
        )
        parser.add_argument(
            '--spi-burn',
            default=DEFAULT_SPI_BURN,
            help=f'path to SPI_burn, defaults to {DEFAULT_SPI_BURN}',
        )
        parser.add_argument(
            '--reset-mtime',
            action='store_true',
            help='reset the mtime counter to 0 before running the image, '
            'for targets whose mtime is not reset with the target',
        )

    @classmethod
    def do_create(cls, cfg, args):
        if cfg.gdb is None:
            raise ValueError('--gdb not provided at command line')

        build_conf = BuildConfiguration(cfg.build_dir)
        target_conf = TargetConfig(
            kernel_entry=build_conf.get('CONFIG_KERNEL_ENTRY', None),
            xip=build_conf.getboolean('CONFIG_XIP'),
            smp=build_conf.getboolean('CONFIG_SMP'),
            riscv64=build_conf.getboolean('CONFIG_RISCV_ISA_RV64I'),
            boot_hart=build_conf.get('CONFIG_RV_BOOT_HART', 0),
        )
        flash_layout = FlashLayout(
            rom_start_offset=build_conf.get('CONFIG_ROM_START_OFFSET', 0),
            flash_load_offset=build_conf.get('CONFIG_FLASH_LOAD_OFFSET', 0),
            flash_base_address=build_conf.get('CONFIG_FLASH_BASE_ADDRESS', 0),
        )
        iceman_server = IcemanServer(
            host=args.iceman_host,
            gdb_port=args.gdb_port,
            burner_port=args.burner_port,
            telnet_port=args.telnet_port,
        )

        return AndesIcemanBinaryRunner(
            cfg,
            target_conf,
            flash_layout,
            iceman_server=iceman_server,
            iceman=args.iceman,
            spi_burn=args.spi_burn,
            reset_mtime=args.reset_mtime,
            dev_id=args.dev_id,
            erase=args.erase,
            load=args.load,
            gdb_init=args.gdb_init,
            tool_opt=args.tool_opt,
        )

    def get_mtime_address(self):
        # mtime address of the RISC-V machine timer, or None without one
        nodes = self.build_conf.edt.compat2nodes.get('riscv,machine-timer')
        if not nodes:
            return None

        for reg in nodes[0].regs:
            if reg.name == 'mtime' and reg.addr is not None:
                return reg.addr

        raise ValueError(f"Could not find the 'mtime' register address in {nodes[0].path}")

    def get_sram_address(self):
        node = self.build_conf.edt.chosen_node('zephyr,sram')
        if node is None or not node.regs or node.regs[0].addr is None:
            raise ValueError("Could not find the 'zephyr,sram' chosen node address")

        return node.regs[0].addr

    def get_flash_base_address(self):
        node = self.get_chosen_flash_node()

        # Same ROM base address as include/zephyr/arch/riscv/common/linker.ld
        if self.node_has_compat(node, "soc-nv-flash"):
            if not node.regs or node.regs[0].addr is None:
                raise ValueError(f"Could not find the reg address of {node.path}")
            flash_addr = node.regs[0].addr

        elif self.node_has_compat(node, "jedec,spi-nor"):
            spi_node = node.parent
            if not spi_node:
                raise ValueError(f"Could not find the parent SPI node of {node.path}")

            reg_idx = 1
            for idx, reg in enumerate(spi_node.regs):
                if reg.name == "flash_mmap":
                    reg_idx = idx
                    break

            if len(spi_node.regs) <= reg_idx or spi_node.regs[reg_idx].addr is None:
                raise ValueError(
                    f"Could not find the 'flash_mmap' reg or reg[1] of {spi_node.path}"
                )

            flash_addr = spi_node.regs[reg_idx].addr

        else:
            flash_addr = self.flash_layout.flash_base_address

        return flash_addr

    def get_image_offset(self):
        # Offset of the image in the flash, from the code partition when it is
        # memory-mapped, as in the linker script
        if self.build_conf.getboolean('CONFIG_FLASH_USES_MAPPED_PARTITION'):
            node = self.build_conf.edt.chosen_node('zephyr,code-partition')
            if node is None:
                raise ValueError("Could not find the 'zephyr,code-partition' chosen node")

            return node.regs[0].addr - self.get_flash_base_address()

        return self.flash_layout.flash_load_offset

    def get_chosen_flash_node(self):
        # Get the EDT Node corresponding to the zephyr,flash chosen DT node
        node = self.build_conf.edt.chosen_node('zephyr,flash')
        if node is None:
            raise ValueError("Could not find the 'zephyr,flash' chosen node")

        return node

    def node_has_compat(self, node, compat):
        return compat in node.compats

    def xip_flash(self):
        # XIP from the SPI flash rather than the ILM, programmed by SPI_burn
        node = self.get_chosen_flash_node()
        return self.target_conf.xip and not self.node_has_compat(node, "andestech,ilm")

    def smp_load_entry(self, command):
        '''Return the GDB command that starts all cores at the kernel entry.'''

        if not self.target_conf.kernel_entry:
            raise ValueError(f'Cannot {command}; CONFIG_KERNEL_ENTRY is missing')

        return f'thread apply all set $pc=&{self.target_conf.kernel_entry}'

    def run_gdb(self, command, load, SPI_burn):
        self.require(self.cfg.gdb)

        # Connect to the ICEman GDB server
        host = self.iceman_server.host or 'localhost'
        gdb_cmds = [f'target remote {host}:{self.iceman_server.gdb_port}']

        ilm_dump = None

        if command in ('flash', 'debug'):
            node = self.get_chosen_flash_node()

            # Reset and halt the target, unless SPI_burn holds it for debugging
            if not (command == 'debug' and SPI_burn):
                gdb_cmds.append('monitor reset halt')

            # Enable the ILM (Andes Instruction Local Memory) before loading
            if load and self.node_has_compat(node, "andestech,ilm"):
                start = node.regs[0].addr
                end = start + node.regs[0].size
                ilm_dump = pathlib.Path(self.cfg.build_dir) / 'zephyr' / 'ilm_dump.bin'

                gdb_cmds.append(f'thread {self.target_conf.boot_hart + 1}')
                gdb_cmds.append('set $milmb=(unsigned long)$milmb | 0x1')

                # If ECC is enabled at reset, initialize the ECC codes by
                # rewriting the ILM with ECC disabled
                gdb_cmds.append('set $ori_milmb = (unsigned long)$milmb')
                gdb_cmds.append('if ($ori_milmb & 0x6UL) != 0')
                gdb_cmds.append('set $milmb = $ori_milmb & ~0x6UL')
                gdb_cmds.append(f'dump binary memory {ilm_dump.as_posix()} {start} {end}')
                gdb_cmds.append(f'restore {ilm_dump.as_posix()} binary {start}')
                gdb_cmds.append('set $milmb = $ori_milmb')
                gdb_cmds.append('end')

            # Load the image and start all cores at the kernel entry
            if load:
                gdb_cmds.append('load')
                gdb_cmds.append(self.smp_load_entry(command))

            # The target reset does not clear mtime on FPGA boards, while kernel
            # tests assume the system clock starts from 0
            mtime_addr = self.get_mtime_address() if self.reset_mtime else None
            if mtime_addr is not None:
                addr_hex = f"0x{mtime_addr:x}"
                addr_high_hex = f"0x{mtime_addr + 4:x}"

                if self.target_conf.riscv64:
                    gdb_cmds.append(f'set *(unsigned long*){addr_hex} = 0x0UL')
                else:
                    gdb_cmds.append(f'set *(unsigned long*){addr_high_hex} = 0x0UL')
                    gdb_cmds.append(f'set *(unsigned long*){addr_hex} = 0x0UL')
                    gdb_cmds.append(f'set *(unsigned long*){addr_high_hex} = 0x0UL')

            # Park all cores in a '1: j 1b' loop at the start of the SRAM until
            # SPI_burn programs the flash and resets the target, so that the
            # previous image in the flash does not run once GDB detaches.
            if command == 'flash' and SPI_burn:
                sram_addr = self.get_sram_address()
                gdb_cmds.append(f'set *(unsigned int*)0x{sram_addr:x} = 0x0000006f')
                gdb_cmds.append(f'thread apply all set $pc=0x{sram_addr:x}')

        # Exit GDB after flashing
        if command == 'flash':
            gdb_cmds.append('quit')

        # Run the commands from a script, which stops at the first error and
        # makes GDB in batch mode exit with an error.
        gdb_script = pathlib.Path(self.cfg.build_dir) / 'zephyr' / 'andes_iceman.gdb'
        gdb_script.write_text('\n'.join(gdb_cmds) + '\n')
        self.logger.debug(f'GDB script {gdb_script}:\n' + '\n'.join(gdb_cmds))

        gdb_cmd = [self.cfg.gdb, self.cfg.elf_file, '-q']
        if command == 'flash':
            gdb_cmd += ['--batch']
        gdb_cmd += ['-x', gdb_script.as_posix()]
        if command != 'flash':
            gdb_cmd += self.gdb_ex_args(self.gdb_init)

        try:
            if command == 'flash':
                self.check_call(gdb_cmd)

            elif command in ('debug', 'attach'):
                self.check_call_ignore_sigint(gdb_cmd)
        finally:
            gdb_script.unlink(missing_ok=True)
            if ilm_dump is not None:
                ilm_dump.unlink(missing_ok=True)

    def reset_halt_remote_target(self):
        # GDB cannot connect to a target that does not halt
        host = self.iceman_server.host
        port = self.iceman_server.telnet_port
        if host is None:
            return

        self.logger.info(f'Reset and halt the target via {host}:{port}')
        try:
            with socket.create_connection((host, port), timeout=TELNET_TIMEOUT) as sock:
                self.read_telnet_prompt(sock)
                sock.sendall(b'reset halt\n')
                self.logger.debug(self.read_telnet_prompt(sock))
        except OSError as e:
            raise RuntimeError(f'ICEman telnet server {host}:{port} failed: {e}') from e

    @staticmethod
    def read_telnet_prompt(sock):
        data = b''
        while not data.rstrip().endswith(b'>'):
            chunk = sock.recv(4096)
            if not chunk:
                raise RuntimeError('ICEman telnet connection closed')
            data += chunk

        return data.decode(errors='replace')

    def run_SPI_burn(self, command):
        # Connect SPI_burn as a client to the burner port of ICEman
        iceman_ip = socket.gethostbyname(self.iceman_server.host or 'localhost')
        port = self.iceman_server.burner_port
        burner = [self.spi_burn, '--host', f'{iceman_ip}', '--port', f'{port}']
        burn_cmd = list(burner)

        # Flash offset of the image
        image_offset = self.get_image_offset()
        burn_cmd += ['--addr', f'{hex(image_offset)}']

        # Reset vector used after the target reset
        reset_vector = (
            self.get_flash_base_address() + image_offset + self.flash_layout.rom_start_offset
        )
        burn_cmd += ['--reset-vector', f'{hex(reset_vector)}']

        if self.erase:
            burn_cmd += ['--erase-all']

        burn_cmd += ['--unlock', '--verify', f'--image={self.cfg.bin_file}']

        self.check_call(burn_cmd)

        # Keep the target halted at the reset vector for debugging. SPI_burn
        # only resets the target in this mode but still requires an image.
        if command == 'debug':
            self.check_call(burner + ['--reset-and-hold', f'--image={self.cfg.bin_file}'])

    def start_iceman(self, reset_hold):
        # Start ICEman from its directory, where it finds its configuration
        # files, and wait until it accepts connections.
        iceman = self.require(self.iceman)
        cmd = [iceman]

        if self.iceman_server.gdb_port is not None:
            cmd += ['--port', str(self.iceman_server.gdb_port)]

        if self.iceman_server.burner_port is not None:
            cmd += ['--bport', str(self.iceman_server.burner_port)]

        if reset_hold:
            cmd += ['--reset-hold']

        if self.target_conf.smp:
            cmd += ['--smp']

        if self.dev_id is not None:
            cmd += ['--device', self.dev_id]

        cmd += self.tool_opt

        self.iceman_proc = self.popen_ignore_int(
            cmd,
            cwd=os.path.dirname(iceman),
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )

        ready = threading.Event()
        output = []

        # Keep reading the output so that ICEman never blocks on a full pipe
        def read_output():
            for line in self.iceman_proc.stdout:
                self.logger.debug(line.rstrip())
                if not ready.is_set():
                    output.append(line)
                    if ICEMAN_READY_MSG in line:
                        ready.set()
            ready.set()

        threading.Thread(target=read_output, daemon=True).start()

        if not ready.wait(ICEMAN_READY_TIMEOUT) or self.iceman_proc.poll() is not None:
            self.stop_iceman()
            self.logger.error(''.join(output))
            raise RuntimeError('ICEman failed to start')

        # Use the ports reported by ICEman unless given
        if self.iceman_server.gdb_port is None:
            self.iceman_server.gdb_port = self.find_iceman_port(
                r'The core #0 listens on (\d+)', output
            )

        if self.iceman_server.burner_port is None:
            self.iceman_server.burner_port = self.find_iceman_port(
                r'Burner listens on (\d+)', output
            )

    def find_iceman_port(self, pattern, output):
        match = re.search(pattern, ''.join(output))
        if match is None:
            self.stop_iceman()
            self.logger.error(''.join(output))
            raise RuntimeError(f'ICEman output does not match {pattern!r}')

        return int(match.group(1))

    def stop_iceman(self):
        if self.iceman_proc is None:
            return

        self.iceman_proc.terminate()
        self.iceman_proc.wait()

        self.iceman_proc = None

    def do_run(self, command, **kwargs):
        # Load the image on flash, and on debug unless --no-load is given
        load = command == 'flash' or (command == 'debug' and self.load)

        # SPI_burn programs the image instead of GDB on XIP from the SPI flash
        SPI_burn = load and self.xip_flash()

        if self.erase and not SPI_burn:
            self.logger.warning('--erase only applies when SPI_burn programs the flash')

        if self.iceman_server.host is not None:
            missing = [
                option
                for option, port in (
                    ('--gdb-port', self.iceman_server.gdb_port),
                    ('--burner-port', self.iceman_server.burner_port),
                    ('--telnet-port', self.iceman_server.telnet_port),
                )
                if port is None
            ]
            if missing:
                raise ValueError(f'--iceman-host requires {", ".join(missing)}')
        elif self.iceman_server.telnet_port is not None:
            self.logger.warning('--telnet-port only applies with --iceman-host')

        self.ensure_output('elf')
        if SPI_burn:
            self.ensure_output('bin')
            self.spi_burn = self.require(self.spi_burn)

        # Start ICEman unless a running ICEman is given
        if self.iceman_server.host is None:
            # Hold the target at reset so that the previous image in the flash
            # does not run, except on attach and when SPI_burn resets the target
            self.start_iceman(reset_hold=command != 'attach' and not SPI_burn)

        try:
            self.run_commands(command, load, SPI_burn)
        finally:
            self.stop_iceman()

    def run_commands(self, command, load, SPI_burn):
        if SPI_burn and command == 'flash':
            # SPI_burn programs the flash and resets the target. GDB only
            # resets mtime before.
            if self.reset_mtime:
                self.reset_halt_remote_target()
                self.run_gdb(command, load=False, SPI_burn=True)

            self.run_SPI_burn(command)

        elif SPI_burn and command == 'debug':
            # SPI_burn programs the flash and holds the target for GDB
            self.run_SPI_burn(command)
            self.run_gdb(command, load=False, SPI_burn=True)

        else:
            # Attach must not reset the target
            if command != 'attach':
                self.reset_halt_remote_target()

            self.run_gdb(command, load, SPI_burn=False)
