#!/usr/bin/env python3

# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Common fixtures and helpers for the DT Doctor test suite."""

import os
import pickle
import sys
from pathlib import Path

import pytest

ZEPHYR_BASE = Path(os.environ.get("ZEPHYR_BASE", Path(__file__).parents[4]))
sys.path.insert(0, str(ZEPHYR_BASE / "scripts" / "dts"))

# Importing the analyzer first also puts python-devicetree/src on sys.path, so the
# edtlib imported below is the same module the analyzer unpickles with.
import dtdoctor_analyzer  # noqa: E402
from devicetree import edtlib  # noqa: E402

FIXTURE_DIR = Path(__file__).parent / "fixture"
BINDINGS_DIR = FIXTURE_DIR / "bindings"

# Disabled node with every kind of reference the disabled-node diagnosis can report:
# a dependent node ('friend' is a phandle-typed property in the vnd,consumer binding,
# which is what creates the edt dependency edge), a /chosen entry (which must use the
# bare &label path form, not <&label>, to show up in edt.chosen_nodes), and an alias.
DTS_DISABLED_FULL = """
/dts-v1/;

/ {
	chosen {
		zephyr,console = &foo_dev;
	};

	aliases {
		my-foo = &foo_dev;
	};

	foo_dev: foo-device {
		compatible = "vnd,foo-device";
		status = "disabled";
	};

	consumer_a: consumer-a {
		compatible = "vnd,consumer";
		friend = <&foo_dev>;
	};
};
"""

# Disabled node nothing refers to: the diagnosis must not claim dependents or references
DTS_DISABLED_UNREFERENCED = """
/dts-v1/;

/ {
	foo_dev: foo-device {
		compatible = "vnd,foo-device";
		status = "disabled";
	};
};
"""

# Enabled node whose compatible is gated by fixture/kconfig_basic (DTD_UART
# depends on DTD_SERIAL && DT_HAS_VND_FOO_DEVICE_ENABLED), for the "enabled but no
# driver" diagnosis and its Kconfig suggestions
DTS_ENABLED = """
/dts-v1/;

/ {
	foo_dev: foo-device {
		compatible = "vnd,foo-device";
		status = "okay";
	};
};
"""

# Two compatibles: suggestions must be the union over both DT_HAS_* symbols
DTS_ENABLED_MULTI = """
/dts-v1/;

/ {
	foo_dev: foo-device {
		compatible = "vnd,foo-special", "vnd,foo-device";
		status = "okay";
	};
};
"""

# Enabled node without a compatible: triggers the "could not determine compatible"
# fallback, no Kconfig analysis
DTS_ENABLED_NO_COMPAT = """
/dts-v1/;

/ {
	bare_dev: bare-device {
		status = "okay";
	};
};
"""


def ord_symbol(edt: edtlib.EDT, label: str) -> str:
    """Return the __device_dts_ord_N symbol for the node with the given label."""
    return f"__device_dts_ord_{edt.label2node[label].dep_ordinal}"


def dts_line_of(dts_path: Path, needle: str) -> int:
    """Return the 1-based line number of the first line containing 'needle'."""
    for lineno, line in enumerate(dts_path.read_text(encoding="utf-8").splitlines(), start=1):
        if needle in line:
            return lineno
    raise ValueError(f"'{needle}' not found in {dts_path}")


@pytest.fixture
def make_edt(tmp_path):
    """Build an EDT (and its .dts file) from an inline DTS string."""

    def _make(dts_text: str) -> tuple[edtlib.EDT, Path]:
        dts_path = tmp_path / "test.dts"
        dts_path.write_text(dts_text, encoding="utf-8")
        return edtlib.EDT(str(dts_path), [str(BINDINGS_DIR)]), dts_path

    return _make


@pytest.fixture
def make_pickle(tmp_path):
    """Pickle an EDT the same way gen_edt.py does."""

    def _make(edt: edtlib.EDT) -> Path:
        pickle_path = tmp_path / "edt.pickle"
        with open(pickle_path, "wb") as f:
            pickle.dump(edt, f, protocol=4)
        return pickle_path

    return _make


@pytest.fixture
def run_analyzer(monkeypatch, capsys):
    """Run dtdoctor_analyzer.main() in-process and capture its output."""

    def _run(edt_pickle, symbol: str) -> tuple[int, str, str]:
        monkeypatch.setattr(
            sys,
            "argv",
            ["dtdoctor_analyzer.py", "--edt-pickle", str(edt_pickle), "--symbol", symbol],
        )
        rc = dtdoctor_analyzer.main()
        out, err = capsys.readouterr()
        return rc, out, err

    return _run


@pytest.fixture
def kconfig_env(monkeypatch):
    """Point setup_kconfig() at a fixture Kconfig tree, scrubbing kconfiglib env vars."""

    def _use(scenario: str) -> None:
        for var in list(os.environ):
            if var.startswith("KCONFIG_"):
                monkeypatch.delenv(var)
        for var in ("srctree", "CONFIG_", "EDT_PICKLE"):
            monkeypatch.delenv(var, raising=False)
        monkeypatch.setenv("ZEPHYR_BASE", str(FIXTURE_DIR / scenario))

    return _use
