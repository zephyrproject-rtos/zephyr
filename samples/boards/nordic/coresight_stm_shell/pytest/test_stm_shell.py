# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

"""
Validate runtime log level filtering on the main core (cpuapp) shell.

Uses the sample ``ping`` command, which emits one log line per severity (err/wrn/inf/dbg).
"""

from __future__ import annotations

import logging

import pytest
from twister_harness import Shell

logger = logging.getLogger(__name__)

# Substrings from LOG_* calls in src/main.c
MARK_ERR = r".*<err> \w{3}\/app: error 100.*"
MARK_WRN = r".*<wrn> \w{3}\/app: warning 78187493520.*"
MARK_INF = r".*<inf> \w{3}\/app: info test.*"
MARK_DBG = r".*<dbg> \w{3}\/app: debug 1000 100.*"


def test_runtime_stm_shell(shell: Shell) -> None:
    """
    On cpuapp: set module ``app`` to DBG, run ``ping`` and expect all severities;
    set to INF, run ``ping`` again and expect no DBG line. The full sequence runs twice.
    """
    logger.info("testing cpuapp shell")

    for attempt in range(2):
        logger.info("test sequence iteration %d/2", attempt + 1)

        shell.exec_command("log enable dbg app")
        out_ping_all = shell.exec_command("ping", get_full_output=True, full_output_timeout=1.0)
        pytest.LineMatcher(out_ping_all).re_match_lines([MARK_ERR, MARK_WRN, MARK_INF, MARK_DBG])

        shell.exec_command("log enable inf app")
        out_ping_inf = shell.exec_command("ping", get_full_output=True, full_output_timeout=1.0)
        pytest.LineMatcher(out_ping_inf).re_match_lines([MARK_ERR, MARK_WRN, MARK_INF])
        pytest.LineMatcher(out_ping_inf).no_re_match_line(MARK_DBG)
