#!/usr/bin/env python3

# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

# A convenience script provided for running tests on the runners
# package. Runs mypy and pytest. Any extra arguments in sys.argv are
# passed along to pytest.
#
# Using tox was considered, but rejected as overkill for now.
#
# We would have to configure tox to create the test virtualenv with
# all of zephyr's scripts/requirements.txt, which seems like too much
# effort for now. We just run in the same Python environment as the
# user for developer testing and trust CI to set that environment up
# properly for integration testing.
#
# If this file starts to reimplement too many features that are
# already available in tox, we can revisit this decision.

import os
import shlex
import subprocess
import sys

here = os.path.abspath(os.path.dirname(__file__))

mypy = [sys.executable, '-m', 'mypy', f'--config-file={here}/mypy.ini',
        '--package', 'runners']
pytest = [sys.executable, '-m', 'pytest'] + sys.argv[1:]

print(f'Running mypy from {here}:\n\t' +
      ' '.join(shlex.quote(s) for s in mypy),
      flush=True)
subprocess.run(mypy, check=True, cwd=here)
print(f'Running pytest from {here}:\n\t' +
      ' '.join(shlex.quote(s) for s in pytest),
      flush=True)
subprocess.run(pytest, check=True, cwd=here)
