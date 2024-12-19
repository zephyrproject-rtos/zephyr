# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Devicetree shell built-in "pwd".

Print current working branch.

Unit tests and examples: tests/test_dtsh_builtin_pwd.py
"""


from typing import Sequence

from dtsh.io import DTShOutput
from dtsh.shell import DTSh, DTShCommand


class DTShBuiltinPwd(DTShCommand):
    """Devicetree shell built-in "pwd"."""

    def __init__(self) -> None:
        """Command definition."""
        super().__init__(
            "pwd", "print path of current working branch", [], None
        )

    def execute(self, argv: Sequence[str], sh: DTSh, out: DTShOutput) -> None:
        """Overrides DTShCommand.execute()."""
        super().execute(argv, sh, out)
        out.write(sh.pwd)
