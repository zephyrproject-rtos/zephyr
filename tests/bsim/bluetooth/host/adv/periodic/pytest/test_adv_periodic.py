# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import logging
import pytest

from pathlib import Path
from twister_harness.helpers.babblesim import BabbleSim


logger = logging.getLogger(__name__)
TIMEOUT = 10.0  # [s]


class TestAdvPeriodic:

    @pytest.fixture(scope="class", autouse=True)
    def copy_app(self, bsim_env: dict, build_dir: Path):
        new_app_name = 'bs_nrf52_bsim_tests_bsim_bluetooth_host_adv_periodic_prj_conf'
        BabbleSim.copy_app(bsim_env, build_dir, new_app_name)

    def test_per_adv(self):
        script_name = 'per_adv.sh'
        script_path = Path(__file__).resolve().parent / script_name

        bsim = BabbleSim()
        bsim.run_script(script_path, TIMEOUT)

        assert getattr(bsim.process, 'returncode', None) == 0

    def test_per_adv_conn(self):
        script_name = 'per_adv_conn.sh'
        script_path = Path(__file__).resolve().parent / script_name

        bsim = BabbleSim()
        bsim.run_script(script_path, TIMEOUT)

        assert getattr(bsim.process, 'returncode', None) == 0

    def test_per_adv_conn_privacy(self):
        script_name = 'per_adv_conn_privacy.sh'
        script_path = Path(__file__).resolve().parent / script_name

        bsim = BabbleSim()
        bsim.run_script(script_path, TIMEOUT)

        assert getattr(bsim.process, 'returncode', None) == 0
