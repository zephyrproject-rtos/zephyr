# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import logging

from pathlib import Path
from twister_harness.helpers.babblesim import BabbleSim


logger = logging.getLogger(__name__)


def test_test_read_fill_buf(bsim_env: dict, build_dir: Path):
    new_client_app_name = 'bs_nrf52_bsim_tests_bsim_bluetooth_host_att_read_fill_buf_client_prj_conf'
    original_client_app_dir = build_dir / 'read_fill_buf'
    new_server_app_name = 'bs_nrf52_bsim_tests_bsim_bluetooth_host_att_read_fill_buf_server_prj_conf'
    original_server_app_dir = build_dir / 'server'

    script_name = 'run_tests.sh'
    script_path = Path(__file__).resolve().parent / script_name

    bsim = BabbleSim()
    bsim.copy_app(bsim_env, original_client_app_dir, new_client_app_name)
    bsim.copy_app(bsim_env, original_server_app_dir, new_server_app_name)
    bsim.run_script(script_path)

    assert getattr(bsim.process, 'returncode', None) == 0
