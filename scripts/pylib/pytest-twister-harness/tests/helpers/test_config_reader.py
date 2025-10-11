# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import textwrap

import pytest
from twister_harness.helpers.config_reader import ConfigReader

CONFIG: str = textwrap.dedent("""
    # comment
    X_PM_MCUBOOT_OFFSET=0x0
    X_PM_MCUBOOT_END_ADDRESS=0xd800

    X_PM_MCUBOOT_NAME=mcuboot
    X_PM_MCUBOOT_ID=0
    X_CONFIG_BOOL_TRUE=y
    X_CONFIG_BOOL_FALSE=n
""")


@pytest.fixture
def config_reader(tmp_path) -> ConfigReader:
    config_file = tmp_path / "config"
    config_file.write_text(CONFIG)
    reader = ConfigReader(config_file)
    return reader


def test_if_raises_exception_path_is_directory(tmp_path):
    build_dir = tmp_path / "build"
    build_dir.mkdir()
    with pytest.raises(AssertionError, match=f"It is not a file: {build_dir}"):
        ConfigReader(build_dir)


def test_if_raises_exception_when_path_does_not_exist(tmp_path):
    build_dir = tmp_path / "build"
    build_dir.mkdir()
    config_file = build_dir / "file_does_not_exist"
    with pytest.raises(AssertionError, match=f"Path does not exist: {config_file}"):
        ConfigReader(config_file)


def test_if_can_read_values_from_config_file(config_reader):
    assert config_reader.config, "Config is empty"
    assert config_reader.read("X_PM_MCUBOOT_NAME") == "mcuboot"
    assert config_reader.read("X_CONFIG_BOOL_TRUE") == "y"
    assert config_reader.read_bool("X_CONFIG_BOOL_TRUE") is True
    assert config_reader.read_bool("X_CONFIG_BOOL_FALSE") is False
    assert config_reader.read_hex("X_PM_MCUBOOT_END_ADDRESS") == "0xd800"
    assert config_reader.read_int("X_PM_MCUBOOT_END_ADDRESS") == 0xD800


def test_if_raises_value_error_when_key_does_not_exist(config_reader):
    with pytest.raises(ValueError, match="Could not find key: DO_NOT_EXIST"):
        config_reader.read("DO_NOT_EXIST")

    with pytest.raises(ValueError, match="Could not find key: DO_NOT_EXIST"):
        config_reader.read_int("DO_NOT_EXIST")


def test_if_raises_value_error_when_default_value_is_not_proper(config_reader):
    with pytest.raises(TypeError, match="default value must be type of int, but was .*"):
        config_reader.read_hex("X_PM_MCUBOOT_OFFSET", "0x10")
    with pytest.raises(TypeError, match="default value must be type of int, but was .*"):
        config_reader.read_int("X_PM_MCUBOOT_OFFSET", "0x10")


def test_if_returns_default_value_when_key_does_not_exist(config_reader):
    assert config_reader.read("DO_NOT_EXIST", "default") == "default"
    assert config_reader.read_int("DO_NOT_EXIST", 10) == 10
    assert config_reader.read_hex("DO_NOT_EXIST", 0x20) == "0x20"
    assert config_reader.read_bool("DO_NOT_EXIST", True) is True
    assert config_reader.read_bool("DO_NOT_EXIST", False) is False
    assert config_reader.read_bool("DO_NOT_EXIST", 1) is True
    assert config_reader.read_bool("DO_NOT_EXIST", "1") is True


def test_if_does_not_raise_exception_in_silent_mode_for_key_not_found(config_reader):
    assert config_reader.read("DO_NOT_EXIST", silent=True) is None
