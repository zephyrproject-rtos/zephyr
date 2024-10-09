# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for the dtsh_uthelpers module."""

# Relax pylint a bit for unit tests.
# pylint: disable=missing-function-docstring


import os

from .dtsh_uthelpers import DTShTests


def test_dtsh_uthelpers_mockenv() -> None:
    # An existing variable we'll change.
    prev_pwd = os.environ["PWD"]
    assert prev_pwd
    tmp_pwd = "TMP1"
    # An existing variable we'll unset.
    prev_user = os.environ["USER"]
    assert prev_user
    # A new OS environment variable, that should not exist.
    tmp_kandiarok = "TMP2"
    assert not os.environ.get("KONDIARONK")

    with DTShTests.mock_env(
        {"PWD": tmp_pwd, "KONDIARONK": tmp_kandiarok, "USER": None}
    ):
        assert tmp_pwd == os.environ["PWD"]
        assert not os.environ.get("USER")
        assert tmp_kandiarok == os.environ["KONDIARONK"]

    assert prev_pwd == os.environ["PWD"]
    assert prev_user == os.environ["USER"]
    assert not os.environ.get("KONDIARONK")


def test_dtsh_uthelpers_zephyr_base() -> None:
    assert os.path.isfile(os.path.join(DTShTests.ZEPHYR_BASE, "zephyr-env.sh"))


def test_dtsh_uthelpers_get_resource_path() -> None:
    assert os.path.join(
        DTShTests.RES_BASE, "README"
    ) == DTShTests.get_resource_path("README")


def test_dtsh_uthelpers_from_res() -> None:
    with DTShTests.from_res():
        assert os.path.isfile("README")


def test_get_sample_edt() -> None:
    model = DTShTests.get_sample_edt()
    assert model
    assert model is DTShTests.get_sample_edt()
    assert model is not DTShTests.get_sample_edt(force_reload=True)
