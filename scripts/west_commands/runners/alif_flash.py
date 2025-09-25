# Copyright 2025 AlifSemiconductor.
# SPDX-License-Identifier: Apache-2.0
"""
Runner for Alif binary image burner.
"""
import os
import sys
import json
import shutil
import re
import fdt

from pathlib import Path
from runners.core import ZephyrBinaryRunner, RunnerCaps, FileType

DEFAULT_JLINK_GDB_PORT = 2331
DEFAULT_JLINK_GDB_SERVER = 'JLinkGDBServerCL' if sys.platform == 'win32' else 'JLinkGDBServer'

class AlifImageBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for Alif Image Flasher.'''

    #Location.
    zephyr_repo = str(Path.cwd())

    exe_dir = os.getenv("ALIF_SE_TOOLS_DIR")

    mram_base_addr = int('0x80000000', 0)

    cfg_ip_file = '/build/config/app-cpu-stubs.json'
    cfg_op_file = '/build/config/tmp-cpu-stubs.json'
    glbl_cfg_file = '/utils/global-cfg.db'
    isp_cfg_file = '/isp_config_data.cfg'

    def __init__(self, cfg, device,
                 erase=False, reset=True,
                 iface='swd', speed='auto', gdb_host='',
                 gdb_port=DEFAULT_JLINK_GDB_PORT, tui=False, com_port='',
                 toc_create='app-gen-toc', toc_write='app-write-mram'):
        super().__init__(cfg)

        self.com_port = com_port
        self.gen_toc = toc_create
        self.write_toc = toc_write
        self.gdbserver = DEFAULT_JLINK_GDB_SERVER
        self.gdb_host = gdb_host
        self.gdb_port = gdb_port
        self.device = device
        self.iface = iface
        self.speed = speed
        self.erase = bool(erase)
        self.reset = bool(reset)
        self.gdb_cmd = [cfg.gdb] if cfg.gdb else None
        self.file = cfg.file
        self.file_type = cfg.file_type
        self.hex_name = cfg.hex_file
        self.elf_name = cfg.elf_file
        self.tui_arg = ['-tui'] if tui else []

    @classmethod
    def name(cls):
        return 'alif_flash'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash', 'debug', 'attach'}, erase=False, reset=True)

    @classmethod
    def do_add_parser(cls, parser):
        # Required:
        parser.add_argument('--device', required=True, help='device name')

        # Optional:
        parser.add_argument('--com-port', default='',
                            help='SE Port to write ToC image')
        parser.add_argument('--iface', default='swd',
                            help='interface to use, default is swd')
        parser.add_argument('--speed', default='auto',
                            help='interface speed, default is autodetect')
        parser.add_argument('--tui', default=False, action='store_true',
                            help='if given, GDB uses -tui')
        parser.add_argument('--gdb-host', default='',
                            help='custom gdb host, defaults to the empty string '
                            'and runs a gdb server')
        parser.add_argument('--gdb-port', default=DEFAULT_JLINK_GDB_PORT,
                            help='JLink gdb port, defaults to {}'.format(
                                DEFAULT_JLINK_GDB_PORT))
        parser.set_defaults(reset=True)

    @classmethod
    def do_create(cls, cfg, args):
        return AlifImageBinaryRunner(cfg, args.device,
                                    erase=args.erase,
                                    reset=args.reset,
                                    iface=args.iface,
                                    speed=args.speed,
                                    gdb_host=args.gdb_host,
                                    gdb_port=args.gdb_port,
                                    tui=args.tui,
                                    com_port=args.com_port)

    def flash(self, **kwargs):

        #check tools availability
        self.require(self.gen_toc, self.exe_dir)

        fls_addr = self.flash_address_from_build_conf(self.build_conf)
        fls_size = self.build_conf.get('CONFIG_FLASH_SIZE')

        self.logger.info("binary address %s and size %s KB", hex(fls_addr), fls_size)

        if self.build_conf.getboolean('CONFIG_RTSS_HP'):
            self.logger.info("..build for HighPerformance Core")
            build_core = "hp"
        else:
            self.logger.info("..build for HighEfficency Core")
            build_core = "he"

        shutil.copy(
            os.path.join(self.cfg.build_dir, 'zephyr', 'zephyr.bin'),
            os.path.join(self.exe_dir, 'build/images/')
        )

        # change dir just for the JSON and TOC
        old_cwd = os.getcwd()
        try:
            os.chdir(self.exe_dir)
            self.logger.info(f"Changed working directory to {self.exe_dir}")

            #Prepare json to create AToC.
            self.prepare_json(self.logger, build_core, fls_addr, fls_size)

            # Generate ToC.
            self.check_call(['./' + self.gen_toc, '-f',
                             self.exe_dir + self.cfg_op_file],
                            **kwargs)

            # Write ToC.
            self.check_call(['./' + self.write_toc, '-p'], **kwargs)

        finally:
            os.chdir(old_cwd)
            self.logger.info(f"Returned to working directory {old_cwd}")

    def debug(self, attach=False, **kwargs):
        '''Launch a debug session with J-Link.'''

        #check tool availability
        self.require(self.gdbserver)

        #server command
        server_cmd = ([self.gdbserver] +
                    ['-select', 'usb',
                    '-port', str(self.gdb_port),
                    '-if', self.iface,
                    '-speed', self.speed,
                    '-device', self.device,
                    '-silent',
                    '-singlerun', '-nogui'])

        if self.gdb_cmd is None:
            raise ValueError('Cannot debug; gdb is missing')
        if self.file is not None:
            if self.file_type != FileType.ELF:
                raise ValueError('Cannot debug; elf file required')
            elf_name = self.file
        elif self.elf_name is None:
            raise ValueError('Cannot debug; elf is missing')
        else:
            elf_name = self.elf_name

        #client command
        client_cmd = (self.gdb_cmd +
                [elf_name] +
                self.tui_arg +
                ['-ex', 'target remote {}:{}'.format(self.gdb_host, self.gdb_port)] +
                ['-ex', 'monitor halt'])

        #don't reset for 'attach'
        if not attach:
            client_cmd += ['-ex', 'monitor reset']

        #launch Server and Client subprocess
        self.run_server_and_client(server_cmd, client_cmd)

    def do_run(self, command, **kwargs):

        #check env
        if self.exe_dir is None:
            raise RuntimeError("ALIF_SE_TOOLS_DIR Environment unset")

        #Attach : Launch debug session to the running process.
        if command == 'attach':
            self.debug(attach=True, **kwargs)
            return

        #Verification: SE Configuration with Target
        if not self.verify_tool_config(self.logger, self.device):
            return

        #Update SE communication port.
        if not self.verify_se_port(self.logger, self.com_port):
            self.logger.warning("Failed to update COMPORT")

        #Flash : Write to MRAM with Alif SE Tools.
        if command == 'flash':
            self.flash(**kwargs)
            return

        #Debug : Flash and launch JLink debug session.
        if command == 'debug':
            self.flash(**kwargs)
            self.debug(**kwargs)
            return

    @classmethod
    def verify_se_port(cls, logger, com_port):
        'Verify and Update Secure Enclave communication port'

        if not com_port:
            return True

        try:
            with open(cls.exe_dir + cls.isp_cfg_file, "r") as f:
                lines = f.readlines()

            # update comport
            lines[0] = f"comport /dev/tty{com_port}\n"

            with open(cls.exe_dir + cls.isp_cfg_file, "w", newline="\n") as f:
                f.writelines(lines)

        except IOError as err:
            logger.error(f"Can't open file to read {cls.exe_dir + cls.isp_cfg_file} : {err}")
            return False

        return True

    @classmethod
    def verify_tool_config(cls, logger, device):
        """check SE Tool configuration and device argument"""
        try:
            with open(cls.exe_dir + cls.glbl_cfg_file, 'r', encoding="utf-8") as conf_file:
                json_data = json.load(conf_file)
                val = json_data.get("DEVICE", {})
                value = val.get("Part#")

                if not value:
                    logger.error("Failed to fetch Part config")
                    return False

                dev_val = re.search(r"\(([^)]+)\)", value)
                if dev_val:
                    cfg_dev = dev_val.group(1)
                else:
                    logger.error("Failed to fetch Device config")
                    return False

                logger.info(f"SE Tool configured to {cfg_dev} and target {device}")
                if (cfg_dev[:5] == device[:5]):
                    return True
                else:
                    logger.error('Target and configuration Mismatch !!! ' \
                                  'Please re-check the Alif tools configuration !')
                    return False

        except json.JSONDecodeError as err:
            logger.error(f"Invalid JSON format: {err}")
            return False

        except IOError as err:
            logger.error(f"Can't open file to read {cls.exe_dir + cls.glbl_cfg_file} : {err}")
            return False

    @classmethod
    def get_itcm_address(cls, logger):
        """Function retrieves itcm address."""
        try:
            with open(os.path.join(cls.zephyr_repo,
                                   'build', 'zephyr', 'zephyr.dts'), "r", encoding="utf-8") as f:
                dtext = f.read()
        except Exception as err:
            logger.error(f"dts parsing error {err}")
            return 0

        try:
            dt2 = fdt.parse_dts(dtext)
            addr = dt2.get_node('soc').get_subnode('itcm@0').get_property('global_base')
        except Exception as err:
            logger.error(f"Parsing itcm address {err}")
            return 0
        return addr.value

    @classmethod
    def prepare_json(cls, logger, build_core, fls_addr, fls_size):
        """prepares json"""
        try:
            with open(cls.exe_dir + cls.cfg_ip_file, 'r', encoding="utf-8") as conf_file:
                json_data = json.load(conf_file)
        except IOError as err:
            logger.error(f"Can't open file to read {cls.exe_dir + cls.cfg_ip_file} : {err}")
            return

        if build_core == "hp":
            cpu_node = json_data["HP_APP"]
        else:
            cpu_node = json_data["HE_APP"]

        #update binary name
        cpu_node["binary"] = "zephyr.bin"

        #verify flash address
        if fls_addr == 0:
            itcm_addr = cls.get_itcm_address(logger)
            if itcm_addr == 0:
                logger.error(f"err addr 0x{itcm_addr:x}")
                return
            logger.info(f"itcm global address 0x{itcm_addr:x}")
            cpu_node["loadAddress"] = hex(itcm_addr)
            cpu_node["flags"] = ["load","boot"]

        elif fls_addr >= cls.mram_base_addr and fls_addr <= cls.mram_base_addr + (fls_size * 1024):
            del cpu_node['loadAddress']
            cpu_node['mramAddress'] = hex(fls_addr)
            cpu_node['flags'] = ["boot"]

        else:
            raise NotImplementedError(f'Unsupported address base 0x{fls_addr:x} to write')

        try:
            with open(cls.exe_dir + cls.cfg_op_file, 'w', encoding="utf-8") as file:
                json.dump(json_data, file, indent=4)
        except IOError as err:
            logger.error(f"Can't open file to write {cls.exe_dir + cls.cfg_op_file}: {err}")
