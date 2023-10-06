# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
from __future__ import annotations

import logging
import shutil
import subprocess
from pathlib import Path

logger = logging.getLogger(__name__)


class BabbleSim:
    def __init__(self):
        self.process: subprocess.Popen | None = None

    @staticmethod
    def copy_app(bsim_env: dict, build_dir: Path, new_app_name: str):
        new_app_path = Path(bsim_env['BSIM_OUT_PATH']) / 'bin' / new_app_name
        original_app_path = build_dir / 'zephyr' / 'zephyr.exe'
        logger.debug(f'Copy application {original_app_path} to {new_app_path}')
        shutil.copy(original_app_path, new_app_path)

    def run_script(self, script_path: Path | str, timeout: float = 30.0):
        script_path = str(script_path)
        logger.info(f'Run bsim test script: {script_path}')
        self.process = subprocess.Popen([script_path], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = self.process.communicate(timeout=timeout)
        stdout_decoded, stderr_decoded = stdout.decode(), stderr.decode()
        if self.process.returncode == 0:
            logger.debug(f'Script stdout:\n{stdout_decoded}')
        else:
            logger.info(f'Script stdout:\n{stdout_decoded}')
            logger.error(f'Script stderr:\n{stderr_decoded}')
