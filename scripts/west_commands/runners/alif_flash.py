# Copyright 2025 AlifSemiconductor.
# SPDX-License-Identifier: Apache-2.0
"""
Runner for Alif binary image burner.
"""
import os
import json
import shutil

from pathlib import Path
from runners.core import ZephyrBinaryRunner, RunnerCaps

import fdt

class AlifImageBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for Alif Image Flasher.'''

    #Location.
    zephyr_repo = str(Path.cwd())

    exe_dir = os.getenv("ALIF_SE_TOOLS_DIR")

    mram_base_addr = int('0x80000000', 0)

    cfg_ip_file = '/build/config/app-cpu-stubs.json'
    cfg_op_file = '/build/config/tmp-cpu-stubs.json'

    def __init__(self, cfg, toc_create='app-gen-toc', toc_write='app-write-mram',
                 erase=False, reset=True):
        super().__init__(cfg)
        self.bin_ = cfg.bin_file

        self.gen_toc = toc_create
        self.write_toc = toc_write
        self.erase = bool(erase)
        self.reset = bool(reset)

    @classmethod
    def name(cls):
        return 'alif_flash'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'}, erase=False, reset=True)

    @classmethod
    def do_add_parser(cls, parser):
        parser.set_defaults(reset=True)

    @classmethod
    def do_create(cls, cfg, args):
        return AlifImageBinaryRunner(cfg)


    def do_run(self, command, **kwargs):
        #check env
        if self.exe_dir is None:
            raise RuntimeError("ALIF_SE_TOOLS_DIR Environment unset")

        #check tools availability
        self.require(self.gen_toc, self.exe_dir)

        #set Execution directory
        os.chdir(self.exe_dir)

        fls_addr = self.flash_address_from_build_conf(self.build_conf)
        fls_size = self.build_conf.get('CONFIG_FLASH_SIZE')

        self.logger.info("binary address %s and size %s KB", hex(fls_addr), fls_size)

        if self.build_conf.getboolean('CONFIG_RTSS_HP'):
            self.logger.info("..build for HighPerformance Core")
            build_core = "hp"
        else:
            self.logger.info("..build for HighEfficency Core")
            build_core = "he"

        shutil.copy(self.zephyr_repo + '/build/zephyr/zephyr.bin',
                          self.exe_dir + '/build/images/')

        #Prepare json to create AToC.
        self.prepare_json(self.logger, build_core, fls_addr, fls_size)

        #Generte ToC.
        self.check_call(['./' + self.gen_toc, '-f',
                        self.exe_dir + self.cfg_op_file],
                        **kwargs)

        #Write ToC.
        self.check_call(['./' + self.write_toc, '-p'], **kwargs)

    @classmethod
    def get_itcm_address(cls, logger) :
        """Function retrieves itcm address."""
        try:
            with open( os.path.join(cls.zephyr_repo,
                            'build', 'zephyr', 'zephyr.dts'), "r", encoding="utf-8") as f:
                dtext = f.read()
        except Exception as err:
            logger.error(f"dts parsing error {err}")
            return 0

        try:
            dt2 = fdt.parse_dts(dtext)
            addr = dt2.get_node('soc').get_subnode('itcm@0').get_property('itcm_global_base')
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

        if build_core == "hp" :
            cpu_node = json_data["HP_APP"]
        else :
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
