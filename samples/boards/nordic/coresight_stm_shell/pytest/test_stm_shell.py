# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

"""
Validate runtime log level filtering on cpuapp and on each remote shell (radio, ppr, flpr).

Uses the sample ``ping`` command, which emits one log line per severity (err/wrn/inf/dbg).
"""

from __future__ import annotations

import logging

import pytest
from twister_harness import Shell

logger = logging.getLogger(__name__)

# Substrings from LOG_* calls in src/main.c (core tag: app, rad, ppr, flpr, …)
MARK_ERR = r".*<err> \w+\/app: error 100.*"
MARK_WRN = r".*<wrn> \w+\/app: warning 78187493520.*"
MARK_INF = r".*<inf> \w+\/app: info test.*"
MARK_DBG = r".*<dbg> \w+\/app: debug 1000 100.*"


@pytest.mark.parametrize(
    "cmd_prefix",
    [
        "",
        "remote_shell radio ",
        "remote_shell ppr ",
        "remote_shell flpr ",
    ],
)
def test_runtime_stm_shell(shell: Shell, cmd_prefix: str) -> None:
    """
    For each core (local or via ``remote_shell``): set module ``app`` to DBG, run ``ping`` and
    expect all severities; set to INF, run ``ping`` again and expect no DBG line.
    The full sequence runs twice per core.
    """
    label = "cpuapp" if not cmd_prefix else cmd_prefix.strip()
    logger.info("testing %s", label)

    log_dbg = f"{cmd_prefix}log enable dbg app".rstrip()
    log_inf = f"{cmd_prefix}log enable inf app".rstrip()
    ping_cmd = f"{cmd_prefix}ping".rstrip()

    for attempt in range(2):
        logger.info("%s: test sequence iteration %d/2", label, attempt + 1)

        shell.exec_command(log_dbg)
        out_ping_all = shell.exec_command(ping_cmd, get_full_output=True, full_output_timeout=1.0)
        pytest.LineMatcher(out_ping_all).re_match_lines([MARK_ERR, MARK_WRN, MARK_INF, MARK_DBG])

        shell.exec_command(log_inf)
        out_ping_inf = shell.exec_command(ping_cmd, get_full_output=True, full_output_timeout=1.0)
        pytest.LineMatcher(out_ping_inf).re_match_lines([MARK_ERR, MARK_WRN, MARK_INF])
        pytest.LineMatcher(out_ping_inf).no_re_match_line(MARK_DBG)
