# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for the dtsh.dts module."""

# Relax pylint a bit for unit tests.
# pylint: disable=protected-access
# pylint: disable=missing-function-docstring


from typing import Optional

import os

import pytest

from dtsh.dts import DTS, CMakeCache, YAMLFilesystem, YAMLFile

from .dtsh_uthelpers import DTShTests


def test_cmakecache_init() -> None:
    cache: Optional[CMakeCache]

    with DTShTests.from_res():
        assert 4 == len(CMakeCache("CMakeCache.txt"))

    # Opening a non existing file should not fault.
    assert CMakeCache.open("notafile") is None

    # Opening an invalid CMakeCache file will not fault,
    # and answer an empty cache instead (lines do not match expected format).
    with DTShTests.from_res():
        cache = CMakeCache.open("zephyr.dts")
        assert cache is not None
        assert 0 == len(cache)


def test_cmakecache_getstr() -> None:
    with DTShTests.from_res():
        cache = CMakeCache("CMakeCache.txt")
    assert "foobar" == cache.getstr("DTSH_TEST_STRING")
    assert cache.getstr("NOT_AN_ENTRY") is None


def test_cmakecache_getstrs() -> None:
    with DTShTests.from_res():
        cache = CMakeCache("CMakeCache.txt")
    assert ["foo", "bar"] == cache.getstrs("DTSH_TEST_STRING_LIST")
    assert [] == cache.getstrs("NOT_AN_ENTRY")


def test_cmakecache_getbool() -> None:
    with DTShTests.from_res():
        cmake_cache = CMakeCache("CMakeCache.txt")
    assert cmake_cache.getbool("DTSH_TEST_BOOL_TRUE")
    assert not cmake_cache.getbool("DTSH_TEST_BOOL_FALSE")
    assert not cmake_cache.getbool("NOT_AN_ENTRY")


def test_dts_init() -> None:
    # Use case:
    # - CMake cache: not available
    # - OS environment: reset
    with DTShTests.mock_env(
        {
            "ZEPHYR_BASE": None,
            "ZEPHYR_SDK_INSTALL_DIR": None,
            "ZEPHYR_TOOLCHAIN_VARIANT": None,
        }
    ):
        with DTShTests.from_res():
            dts = DTS("zephyr.dts")

            # App. binary directory (aka build) is always set to
            # the parent of the directory that contains the DTS file
            # (aka build/zephyr/zephyr.dts file layout).
            app_bin_dir = os.path.dirname(os.path.dirname(dts.path))
            assert app_bin_dir == dts.app_binary_dir
            # Derived from the app. bin. directory.
            app_src_dir = os.path.dirname(app_bin_dir)
            assert app_src_dir == dts.app_source_dir

            # ZEPHYR_BASE derived from expected directories layout.
            assert DTShTests.ZEPHYR_BASE == dts.zephyr_base

            # Unset in mock environment.
            assert dts.zephyr_sdk_dir is None
            assert dts.toolchain_variant is None
            assert dts.toolchain_dir is None

            # Possibly set only when a CMake cache is available.
            assert dts.board_dir is None
            assert dts.board is None
            assert [] == dts.shield_dirs
            assert dts.fw_name is None
            assert dts.fw_version is None

            # Default minimal bindings search path.
            assert [
                os.path.join(app_src_dir, "dts", "bindings"),
                os.path.join(DTShTests.ZEPHYR_BASE, "dts", "bindings"),
            ] == dts.bindings_search_path


def test_dts_init_from_os_env() -> None:
    # Use case:
    # - CMake cache: not available
    # - OS environment: ZEPHYR_BASE, ZEPHYR_TOOLCHAIN_VARIANT
    tmpenv = {
        "ZEPHYR_BASE": "dummy",
        "ZEPHYR_SDK_INSTALL_DIR": None,
        "ZEPHYR_TOOLCHAIN_VARIANT": "dummy",
    }
    with DTShTests.mock_env(tmpenv):
        with DTShTests.from_res():
            dts = DTS("zephyr.dts")

            # Parent of the directory that contains the DTS file.
            app_bin_dir = os.path.dirname(os.path.dirname(dts.path))
            assert app_bin_dir == dts.app_binary_dir
            # Derived from the app. bin. directory.
            app_src_dir = os.path.dirname(app_bin_dir)
            assert app_src_dir == dts.app_source_dir

            # Possibly set only when a CMake cache is available.
            assert dts.board_dir is None
            assert dts.board is None
            assert [] == dts.shield_dirs
            assert dts.fw_name is None
            assert dts.fw_version is None

            # Retrieved from mock environment.
            assert dts.zephyr_base
            assert tmpenv["ZEPHYR_BASE"] == dts.zephyr_base
            assert (
                os.path.join(
                    dts.zephyr_base, "dts", "bindings", "vendor-prefixes.txt"
                )
                == dts.vendors_file
            )
            assert tmpenv["ZEPHYR_TOOLCHAIN_VARIANT"] == dts.toolchain_variant
            assert dts.zephyr_sdk_dir is None
            # Environment variable dummy_TOOLCHAIN_PATH does not exist.
            assert dts.toolchain_dir is None

            # Default minimal bindings search path.
            expect_dirs = [
                os.path.join(app_src_dir, "dts", "bindings"),
                os.path.join(dts.zephyr_base, "dts", "bindings"),
            ]
            assert expect_dirs == dts.bindings_search_path


def test_dts_init_from_cmake_zephyr() -> None:
    # Use-case:
    # - CMake cache: res/Build_zephyr/CMakeCache.txt
    # - Environment: reset
    with DTShTests.mock_env(
        {
            "ZEPHYR_BASE": None,
            "ZEPHYR_SDK_INSTALL_DIR": None,
            "ZEPHYR_TOOLCHAIN_VARIANT": None,
        }
    ):
        dts = DTS(
            DTShTests.get_resource_path("Build_zephyr", "zephyr", "zephyr.dts")
        )

        assert dts.zephyr_base
        assert (
            os.path.join(
                dts.zephyr_base, "dts", "bindings", "vendor-prefixes.txt"
            )
            == dts.vendors_file
        )

        # Parent of the directory that contains the DTS file.
        app_bin_dir = os.path.dirname(os.path.dirname(dts.path))
        assert app_bin_dir == dts.app_binary_dir

        # Retrieved from the CMake cache.
        assert (
            os.path.join(
                DTShTests.ANON_ZEPHYR_BASE, "samples", "sensor", "bme680"
            )
            == dts.app_source_dir
        )
        assert (
            os.path.join(
                DTShTests.ANON_ZEPHYR_BASE,
                "boards",
                "arm",
                "nrf52840dk_nrf52840",
            )
            == dts.board_dir
        )
        assert DTShTests.BOARD == dts.board
        assert [] == dts.shield_dirs
        assert "bme680" == dts.fw_name
        assert DTShTests.ZEPHYR_VERSION == dts.fw_version
        assert os.path.join(DTShTests.ANON_ZEPHYR_BASE) == dts.zephyr_base
        assert DTShTests.ANON_ZEPHYR_SDK == dts.zephyr_sdk_dir
        assert "zephyr" == dts.toolchain_variant
        assert DTShTests.ANON_ZEPHYR_SDK == dts.toolchain_dir


def test_dts_init_from_cmake_gnuarm() -> None:
    # Use-case:
    # - CMake cache: res/Build_gnuarm/CMakeCache.txt
    # - Environment: reset
    with DTShTests.mock_env(
        {
            "ZEPHYR_BASE": None,
            "ZEPHYR_SDK_INSTALL_DIR": None,
            "ZEPHYR_TOOLCHAIN_VARIANT": None,
        }
    ):
        dts = DTS(
            DTShTests.get_resource_path("Build_gnuarm", "zephyr", "zephyr.dts")
        )

        assert dts.zephyr_base
        assert (
            os.path.join(
                dts.zephyr_base, "dts", "bindings", "vendor-prefixes.txt"
            )
            == dts.vendors_file
        )

        # Parent of the directory that contains the DTS file.
        app_bin_dir = os.path.dirname(os.path.dirname(dts.path))
        assert app_bin_dir == dts.app_binary_dir

        # Retrieved from the CMake cache.
        assert (
            os.path.join(
                DTShTests.ANON_ZEPHYR_BASE, "samples", "sensor", "bme680"
            )
            == dts.app_source_dir
        )
        assert (
            os.path.join(
                DTShTests.ANON_ZEPHYR_BASE,
                "boards",
                "arm",
                "nrf52840dk_nrf52840",
            )
            == dts.board_dir
        )
        assert DTShTests.BOARD == dts.board
        assert [] == dts.shield_dirs
        assert "bme680" == dts.fw_name
        assert DTShTests.ZEPHYR_VERSION == dts.fw_version
        assert DTShTests.ANON_ZEPHYR_BASE == dts.zephyr_base
        assert dts.zephyr_sdk_dir is None
        assert "gnuarmemb" == dts.toolchain_variant
        assert DTShTests.ANON_GNUARMEMB == dts.toolchain_dir


def test_yamlfs_find_path() -> None:
    with DTShTests.from_res():
        yamlfs = YAMLFilesystem(["yaml"])

    assert yamlfs.find_path("notafile") is None

    for name, path in [
        ("power.yaml", DTShTests.get_resource_path("yaml", "power.yaml")),
        (
            "i2c-device.yaml",
            DTShTests.get_resource_path("yaml", "i2c-device.yaml"),
        ),
        (
            "sensor-device.yaml",
            DTShTests.get_resource_path("yaml", "sensor-device.yaml"),
        ),
    ]:
        assert path == yamlfs.find_path(name)


def test_yamlfs_find_file() -> None:
    with DTShTests.from_res():
        yamlfs = YAMLFilesystem(["yaml"])

    fyaml = yamlfs.find_file("i2c-device.yaml")
    assert fyaml
    assert DTShTests.get_resource_path("yaml", "i2c-device.yaml") == fyaml.path

    # Opening a non existing file should not fault.
    assert yamlfs.find_file("notafile") is None


def test_yamlfs_name2path() -> None:
    with DTShTests.from_res():
        yamlfs = YAMLFilesystem(["yaml"])

    assert {
        "i2c-device.yaml": DTShTests.get_resource_path(
            "yaml", "i2c-device.yaml"
        ),
        "power.yaml": DTShTests.get_resource_path("yaml", "power.yaml"),
        "sensor-device.yaml": DTShTests.get_resource_path(
            "yaml", "sensor-device.yaml"
        ),
    } == yamlfs.name2path

    with pytest.raises(KeyError):
        _ = yamlfs.name2path["notafile"]


def test_dtshdts_yamlfs_find_path() -> None:
    with DTShTests.from_res():
        dts = DTS("zephyr.dts", ["yaml"])

    assert DTShTests.get_resource_path(
        "yaml", "sensor-device.yaml"
    ) == dts.yamlfs.find_path("sensor-device.yaml")


def test_dtshdts_yamlfs_find_file() -> None:
    with DTShTests.from_res():
        dts = DTS("zephyr.dts", ["yaml"])

    yaml = dts.yamlfs.find_file("sensor-device.yaml")
    assert yaml
    assert (
        DTShTests.get_resource_path("yaml", "sensor-device.yaml") == yaml.path
    )


def test_yamlfile() -> None:
    yaml = YAMLFile(DTShTests.get_resource_path("yaml", "i2c-device.yaml"))

    # Lazy initialzation.
    assert yaml._content is None
    assert yaml._raw is None
    assert yaml._includes is None
    assert yaml.content.startswith("# Copyright (c) 2017, Linaro Limited")
    assert yaml.content.endswith("i2c bus")
    assert "i2c" == yaml.raw["on-bus"]
    assert ["base.yaml", "power.yaml"] == yaml.includes

    # Fail-safe
    yaml = YAMLFile("notafile")
    assert "" == yaml.content
    assert {} == yaml.raw
    assert [] == yaml.includes
