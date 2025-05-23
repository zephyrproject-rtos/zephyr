# Copyright 2025 AlifSemiconductor.
# SPDX-License-Identifier: Apache-2.0
"""
Runner for Alif binary image burner.
"""

import json
import os
import re
import shlex
import shutil
import sys
from pathlib import Path

from runners.core import MissingProgram, RunnerCaps, ZephyrBinaryRunner


class AlifImageBinaryRunner(ZephyrBinaryRunner):
    """Runner front-end for Alif Image Flasher."""

    mram_base_addr = int('0x80000000', 0)

    cfg_ip_file = Path('build/config/app-cpu-stubs.json')
    cfg_op_file = Path('build/config/tmp-cpu-stubs.json')
    glbl_cfg_file = Path('utils/global-cfg.db')
    isp_cfg_file = Path('isp_config_data.cfg')
    images_dir = Path('build/images')

    def __init__(
        self,
        cfg,
        device,
        tools_dir=None,
        port=None,
        tool_opt=None,
    ):
        super().__init__(cfg)

        if tools_dir is None or not tools_dir.is_dir():
            raise ValueError('Alif SE tools not found, set ALIF_SE_TOOLS_DIR or --tools-dir')

        self.tools_dir = tools_dir
        self.port = port
        self.gen_toc = 'app-gen-toc'
        self.write_toc = 'app-write-mram'
        self.device = device

        self.tool_opt = []
        if tool_opt is not None:
            for opts in [shlex.split(opt) for opt in tool_opt]:
                self.tool_opt += opts

    @classmethod
    def name(cls):
        return 'alif_flash'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'}, tool_opt=True)

    @classmethod
    def tool_opt_help(cls) -> str:
        return "Additional options for app-write-mram, e.g. '-b 55000'"

    @classmethod
    def do_add_parser(cls, parser):
        # Required:
        parser.add_argument('--device', required=True, help='device name')

        # Optional:
        parser.add_argument(
            '--tools-dir',
            type=Path,
            default=os.environ.get('ALIF_SE_TOOLS_DIR') or None,
            help='Alif SE tools directory, default: $ALIF_SE_TOOLS_DIR',
        )
        parser.add_argument(
            '--port',
            help='SE-UART serial port, e.g. /dev/ttyACM0; saved to the SETOOLS ISP '
            'configuration, so it can be omitted on later runs',
        )

    @classmethod
    def do_create(cls, cfg, args):
        return AlifImageBinaryRunner(
            cfg,
            device=args.device,
            tools_dir=args.tools_dir,
            port=args.port,
            tool_opt=args.tool_opt,
        )

    def flash(self, **kwargs):
        """Flash the binary using Alif SE tools."""

        self.ensure_output('bin')

        # Resolve tool filenames (binary preferred over .py)
        gen_toc = self._require_tool(self.gen_toc)
        write_toc = self._require_tool(self.write_toc)

        fls_addr = self.flash_address_from_build_conf(self.build_conf)
        fls_size = self.build_conf.get('CONFIG_FLASH_SIZE')

        self.logger.info("Binary address %s and size %s KB", hex(fls_addr), fls_size)

        if self.build_conf.getboolean('CONFIG_SOC_FAMILY_ENSEMBLE_RTSS_HP'):
            self.logger.info("..build for HighPerformance Core")
            toc_entry = "HP_APP"
        else:
            self.logger.info("..build for HighEfficency Core")
            toc_entry = "HE_APP"

        shutil.copy(self.cfg.bin_file, self.tools_dir / self.images_dir / 'zephyr.bin')

        # Prepare json to create AToC.
        self.prepare_json(toc_entry, fls_addr, fls_size)

        # Generate ToC. The SE tools resolve their files relative to the
        # working directory.
        self.check_call(
            [str(gen_toc), '-f', str(self.tools_dir / self.cfg_op_file)],
            cwd=self.tools_dir,
            **kwargs,
        )

        # Write ToC.
        self.check_call([str(write_toc), '-p'] + self.tool_opt, cwd=self.tools_dir, **kwargs)

    def do_run(self, command, **kwargs):
        """Entry point for west runner commands."""
        # Verification: SE Configuration with Target
        self.verify_tool_config()

        # Update SE communication port.
        if self.port:
            self.update_se_port()

        # Flash : Write to MRAM with Alif SE Tools.
        self.flash(**kwargs)

    def update_se_port(self):
        """Update the Secure Enclave communication port in the configuration file."""
        isp_cfg_path = self.tools_dir / self.isp_cfg_file
        lines = isp_cfg_path.read_text(encoding="utf-8").splitlines(keepends=True)

        # update comport
        if sys.platform.startswith("win") or self.port.startswith("/dev/"):
            lines[0] = f"comport {self.port}\n"
        else:
            lines[0] = f"comport /dev/tty{self.port}\n"

        with isp_cfg_path.open("w", encoding="utf-8", newline="\n") as f:
            f.writelines(lines)

    def verify_tool_config(self):
        """Check SE Tool configuration against the device argument."""
        glbl_cfg_path = self.tools_dir / self.glbl_cfg_file
        json_data = json.loads(glbl_cfg_path.read_text(encoding="utf-8"))

        value = json_data.get("DEVICE", {}).get("Part#")
        if not value:
            raise ValueError(f"No Part# in {glbl_cfg_path}")

        dev_val = re.search(r"\(([^)]+)\)", value)
        if not dev_val:
            raise ValueError(f"No device in Part# '{value}' in {glbl_cfg_path}")

        cfg_dev = dev_val.group(1)
        self.logger.info("SE Tool configured to %s and target %s", cfg_dev, self.device)
        if cfg_dev[:5] != self.device[:5]:
            raise ValueError(
                f"SE tools configured for {cfg_dev}, target is {self.device}; "
                "re-run tools-config in the Alif SE tools directory"
            )

    def prepare_json(self, toc_entry, fls_addr, fls_size):
        """Prepare JSON for ToC generation."""
        cfg_ip_path = self.tools_dir / self.cfg_ip_file
        cfg_op_path = self.tools_dir / self.cfg_op_file

        json_data = json.loads(cfg_ip_path.read_text(encoding="utf-8"))
        cpu_node = json_data[toc_entry]

        # change binary name
        cpu_node["binary"] = "zephyr.bin"

        # verify flash address
        if self.mram_base_addr <= fls_addr <= self.mram_base_addr + (fls_size * 1024):
            cpu_node.pop('loadAddress', None)
            cpu_node['mramAddress'] = hex(fls_addr)
            cpu_node['flags'] = ["boot"]

        else:
            raise NotImplementedError(f'Unsupported address base 0x{fls_addr:x} to write')

        cfg_op_path.write_text(json.dumps(json_data, indent=4), encoding="utf-8")

    def _require_tool(self, tool_name: str) -> Path:
        """
        Find a tool in the SE tools directory, either as a native executable
        or as a <tool_name>.py script.
        """
        try:
            tool = self.require(tool_name, path=str(self.tools_dir))
        except MissingProgram:
            tool = self.require(f'{tool_name}.py', path=str(self.tools_dir))

        self.logger.info("Using tool: %s", Path(tool).name)
        return Path(tool)
