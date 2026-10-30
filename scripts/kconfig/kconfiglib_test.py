# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for dependency loops through 'select' in kconfiglib.py."""

import kconfiglib
import pytest

# A GPIO expander on SPI and a SPI controller that uses GPIOs for chip select
BUS_LOOP = """
config DT_HAS_EXPANDER
	def_bool y

menuconfig GPIO
	bool "GPIO"

if GPIO

config GPIO_EXPANDER
	bool "Expander"
	default y
	depends on DT_HAS_EXPANDER
	select SPI

endif

menuconfig SPI
	bool "SPI"

if SPI

config SPI_CTRL
	bool "Controller"
	default y
	select GPIO

endif
"""

BUS_LOOP_SYMS = ("GPIO", "GPIO_EXPANDER", "SPI", "SPI_CTRL")


def _kconfig(tmp_path, text):
    path = tmp_path / "Kconfig"
    path.write_text(text)
    return kconfiglib.Kconfig(str(path), warn_to_stderr=False)


def _values(kconf, names):
    return {name: kconf.syms[name].str_value for name in names}


def test_select_loop_no_user_values(tmp_path):
    kconf = _kconfig(tmp_path, BUS_LOOP)
    assert set(_values(kconf, BUS_LOOP_SYMS).values()) == {"n"}
    assert kconf.warnings == []


@pytest.mark.parametrize("name", ["GPIO", "SPI"])
def test_select_loop_enabled(tmp_path, name):
    kconf = _kconfig(tmp_path, BUS_LOOP)
    kconf.syms[name].set_value("y")
    assert set(_values(kconf, BUS_LOOP_SYMS).values()) == {"y"}
    assert kconf.warnings == []


def test_select_loop_disabled(tmp_path):
    kconf = _kconfig(tmp_path, BUS_LOOP)
    kconf.syms["GPIO"].set_value("n")
    kconf.syms["SPI"].set_value("n")
    assert set(_values(kconf, BUS_LOOP_SYMS).values()) == {"n"}


def test_select_loop_reevaluated(tmp_path):
    kconf = _kconfig(tmp_path, BUS_LOOP)
    kconf.syms["GPIO"].set_value("y")
    assert kconf.syms["SPI_CTRL"].str_value == "y"
    kconf.syms["GPIO"].set_value("n")
    assert set(_values(kconf, BUS_LOOP_SYMS).values()) == {"n"}
    kconf.syms["GPIO_EXPANDER"].set_value("n")
    kconf.syms["SPI"].set_value("y")
    assert _values(kconf, BUS_LOOP_SYMS) == {
        "GPIO": "y",
        "GPIO_EXPANDER": "n",
        "SPI": "y",
        "SPI_CTRL": "y",
    }


def test_select_loop_started_by_default(tmp_path):
    kconf = _kconfig(tmp_path, BUS_LOOP.replace('bool "GPIO"', 'bool "GPIO"\n\tdefault y'))
    assert set(_values(kconf, BUS_LOOP_SYMS).values()) == {"y"}


def test_select_loop_write_config(tmp_path):
    kconf = _kconfig(tmp_path, BUS_LOOP)
    kconf.syms["GPIO"].set_value("y")
    kconf.write_config(str(tmp_path / ".config"), header="")
    config = (tmp_path / ".config").read_text()
    for name in BUS_LOOP_SYMS:
        assert f"CONFIG_{name}=y\n" in config


def test_select_loop_with_imply(tmp_path):
    kconf = _kconfig(
        tmp_path,
        """
config A
	bool "A"
	imply B

config B
	bool "B"
	depends on A
""",
    )
    assert kconf.syms["B"].str_value == "n"
    kconf.syms["A"].set_value("y")
    assert kconf.syms["B"].str_value == "y"


def test_depends_on_loop_is_error(tmp_path):
    with pytest.raises(kconfiglib.KconfigError, match="Dependency loop"):
        _kconfig(
            tmp_path,
            """
config A
	bool "A"
	depends on B

config B
	bool "B"
	default y if A
""",
        )


def test_select_loop_negated_is_error(tmp_path):
    with pytest.raises(kconfiglib.KconfigError, match="B is negated or compared"):
        _kconfig(
            tmp_path,
            """
config A
	bool "A"
	select B

config B
	bool "B"

config C
	bool "C"
	depends on !B
	select A
""",
        )


def test_select_loop_relation_is_error(tmp_path):
    with pytest.raises(kconfiglib.KconfigError, match="B is negated or compared"):
        _kconfig(
            tmp_path,
            """
config A
	bool "A"
	select B

config B
	bool "B"

config C
	bool "C"
	depends on B = y
	select A
""",
        )


def test_select_loop_choice_is_error(tmp_path):
    with pytest.raises(kconfiglib.KconfigError, match="is part of a choice"):
        _kconfig(
            tmp_path,
            """
choice
	prompt "Choice"

config A
	bool "A"
	select B

config C
	bool "C"
	depends on B

endchoice

config B
	bool "B"
""",
        )


def test_select_loop_non_monotonic_is_error(tmp_path):
    # D's prompt condition differs from its default condition, so a user value
    # of n makes D drop from y to n once A becomes y
    kconf = _kconfig(
        tmp_path,
        """
config A
	bool

config D
	bool "D" if A
	default y
	select A
""",
    )
    kconf.syms["D"].set_value("n")
    with pytest.raises(kconfiglib.KconfigError, match="changes from y to n"):
        _ = kconf.syms["A"].str_value
