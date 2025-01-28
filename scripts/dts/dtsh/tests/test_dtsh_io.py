# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for the dtsh.io module."""

# Relax pylint a bit for unit tests.
# pylint: disable=missing-function-docstring


import os

import pytest

from dtsh.config import DTShConfig
from dtsh.io import DTShRedirect

from .dtsh_uthelpers import DTShTests


_dtshconf: DTShConfig = DTShConfig.getinstance()


def test_dtsh_redirect() -> None:
    redir2 = DTShRedirect("> path")
    assert not redir2.append
    assert redir2.path == os.path.abspath("path")

    redir2 = DTShRedirect(">> path")
    # Won't append to non existing file.
    assert not redir2.append
    assert redir2.path == os.path.abspath("path")

    with pytest.raises(DTShRedirect.Error):
        # Redirection to empty path.
        redir2 = DTShRedirect("command string>")

    with pytest.raises(DTShRedirect.Error):
        # No spaces allowed.
        redir2 = DTShRedirect("command string > to/file with spaces")

    path = DTShTests.get_resource_path("README")
    with pytest.raises(DTShRedirect.Error):
        # File exists, won't override.
        redir2 = DTShRedirect(f"command string > {path}")
