# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import logging

from pathlib import Path
from twister_harness.helpers.babblesim import BabbleSim


logger = logging.getLogger(__name__)


def test_adv_chain(bsim_env: dict, build_dir: Path):
    new_app_name = 'bs_nrf52_bsim_tests_bsim_bluetooth_host_adv_chain_prj_conf'
    script_name = 'adv_chain.sh'
    script_path = Path(__file__).resolve().parent / script_name
    timeout = 10.0  # [s]

    bsim = BabbleSim()
    bsim.copy_app(bsim_env, build_dir, new_app_name)
    bsim.run_script(script_path, timeout)

    assert getattr(bsim.process, 'returncode', None) == 0
