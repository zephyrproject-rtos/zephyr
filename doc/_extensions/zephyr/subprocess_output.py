# SPDX-FileCopyrightText: The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""
Script Output
##############

Runs a Python script from the Zephyr tree and includes its output as a
literal block, similar to ``sphinxcontrib.programoutput``'s
``command-output`` directive.

Unlike ``command-output``, this does not depend on a shell to resolve the
script's shebang line or expand ``$ZEPHYR_BASE``: the script is invoked
directly with the Python interpreter that is running Sphinx
(``sys.executable``), and ``$ZEPHYR_BASE`` is resolved from this file's own
location in the tree, the same way other ``zephyr.*`` extensions do. This
builds identically on Windows and on POSIX systems, and regardless of
whether Sphinx is run from a staged copy of ``doc/``.

Usage::

    .. zephyr-script-output:: $ZEPHYR_BASE/scripts/twister --help
"""

import shlex
import subprocess
import sys
from pathlib import Path
from typing import Any

from docutils import nodes
from sphinx.application import Sphinx
from sphinx.util import logging
from sphinx.util.docutils import SphinxDirective

logger = logging.getLogger(__name__)

ZEPHYR_BASE = Path(__file__).parents[3]

_ZEPHYR_BASE_PREFIX = "$ZEPHYR_BASE/"


class ScriptOutputDirective(SphinxDirective):
    """Run a Zephyr-tree script and show its output as a literal block."""

    has_content = False
    required_arguments = 1
    final_argument_whitespace = True

    def run(self) -> list[nodes.Element]:
        command_str = self.arguments[0].strip()
        if not command_str.startswith(_ZEPHYR_BASE_PREFIX):
            raise self.error(
                f"zephyr-script-output: command must start with '{_ZEPHYR_BASE_PREFIX}'"
            )

        script, *args = shlex.split(command_str[len(_ZEPHYR_BASE_PREFIX) :])
        script_path = ZEPHYR_BASE / script

        result = subprocess.run(
            [sys.executable, str(script_path), *args],
            cwd=ZEPHYR_BASE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            check=False,
        )
        if result.returncode != 0:
            logger.warning(
                "zephyr-script-output: %r exited with %d", command_str, result.returncode
            )

        text = f"$ {command_str}\n{result.stdout}".rstrip()
        node = nodes.literal_block(text, text)
        node["language"] = "text"
        return [node]


def setup(app: Sphinx) -> dict[str, Any]:
    app.add_directive("zephyr-script-output", ScriptOutputDirective)

    return {
        "version": "0.1.0",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
