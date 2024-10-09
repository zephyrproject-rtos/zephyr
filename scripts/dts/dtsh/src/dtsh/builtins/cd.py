# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Devicetree shell built-in "cd".

Change the current working branch.

Unit tests and examples: tests/test_dtsh_builtin_cd.py
"""


from typing import Sequence

from dtsh.io import DTShOutput
from dtsh.shell import DTSh, DTShCommand, DTPathNotFoundError, DTShCommandError
from dtsh.shellutils import DTShParamDTPath


class DTShBuiltinCd(DTShCommand):
    """Devicetree shell built-in "cd"."""

    def __init__(self) -> None:
        super().__init__(
            "cd",
            "change the current working branch",
            [],
            DTShParamDTPath(),
        )

    def execute(self, argv: Sequence[str], sh: DTSh, out: DTShOutput) -> None:
        """Overrides DTShCommand.execute()."""
        super().execute(argv, sh, out)

        param_path = self.with_param(DTShParamDTPath).path
        try:
            sh.cd(param_path)
        except DTPathNotFoundError as e:
            raise DTShCommandError(self, e.msg) from e
