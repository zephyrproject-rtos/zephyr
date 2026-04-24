#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Tests for scripts/ci/test_plan_v2.py.

Each test class targets one well-defined unit.  Tests that require the
real Zephyr source tree are marked with ``@pytest.mark.integration`` and
are skipped by default; run them explicitly with ``-m integration``.

Run from the zephyr root::

    pytest scripts/tests/ci/test_test_plan_v2.py -v
"""

from __future__ import annotations

import json
import math
import os
import sys
import textwrap
from pathlib import Path
from unittest import mock

import pytest

ZEPHYR_BASE = os.environ.get("ZEPHYR_BASE", str(Path(__file__).parents[3]))
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts", "ci"))

import test_plan_v2 as tp  # noqa: E402, I001


# ---------------------------------------------------------------------------
# Helpers / fixtures
# ---------------------------------------------------------------------------


@pytest.fixture()
def tmp_zephyr(tmp_path):
    """Minimal fake Zephyr tree used by unit tests that need the filesystem."""
    (tmp_path / "tests").mkdir()
    (tmp_path / "samples").mkdir()
    (tmp_path / "snippets").mkdir()
    (tmp_path / "boards").mkdir()
    (tmp_path / "soc").mkdir()
    (tmp_path / "drivers").mkdir()
    (tmp_path / "include").mkdir()
    (tmp_path / "dts").mkdir()
    return tmp_path


# ---------------------------------------------------------------------------
# _matches_area
# ---------------------------------------------------------------------------


class TestMatchesArea:
    """Unit tests for the helper that matches a file path to a MAINTAINERS area."""

    def _area(self, files=(), files_regex=(), files_exclude=(), files_regex_exclude=()):
        return {
            "files": list(files),
            "files-regex": list(files_regex),
            "files-exclude": list(files_exclude),
            "files-regex-exclude": list(files_regex_exclude),
        }

    def test_exact_glob_match(self):
        area = self._area(files=["drivers/gpio/*.c"])
        assert tp._matches_area("drivers/gpio/gpio_nrfx.c", area) is True

    def test_directory_prefix_match(self):
        area = self._area(files=["drivers/sensor/"])
        assert tp._matches_area("drivers/sensor/bmp280/bmp280.c", area) is True

    def test_directory_prefix_no_match_for_sibling(self):
        area = self._area(files=["drivers/sensor/"])
        assert tp._matches_area("drivers/gpio/gpio_nrfx.c", area) is False

    def test_files_exclude_wins_over_files(self):
        area = self._area(files=["drivers/gpio/"], files_exclude=["drivers/gpio/Kconfig"])
        assert tp._matches_area("drivers/gpio/Kconfig", area) is False
        assert tp._matches_area("drivers/gpio/gpio_nrfx.c", area) is True

    def test_files_regex_match(self):
        area = self._area(files_regex=[r"drivers/.*\.c$"])
        assert tp._matches_area("drivers/foo/bar.c", area) is True
        assert tp._matches_area("drivers/foo/bar.h", area) is False

    def test_files_regex_exclude_wins(self):
        area = self._area(
            files=["drivers/"],
            files_regex_exclude=[r"drivers/.*/test_.*\.c$"],
        )
        assert tp._matches_area("drivers/foo/test_foo.c", area) is False
        assert tp._matches_area("drivers/foo/foo.c", area) is True

    def test_no_patterns_no_match(self):
        area = self._area()
        assert tp._matches_area("drivers/foo/bar.c", area) is False


# ---------------------------------------------------------------------------
# _test_pattern
# ---------------------------------------------------------------------------


class TestTestPattern:
    """Unit tests for the test-name → regex converter."""

    def _matches(self, pattern, name):
        import re

        return bool(re.search(pattern, name))

    def test_exact_match(self):
        p = tp._test_pattern("arch.arm")
        assert self._matches(p, "arch.arm")

    def test_sub_id_match(self):
        p = tp._test_pattern("arch.arm")
        assert self._matches(p, "arch.arm.irq")
        assert self._matches(p, "arch.arm.mpu.stack_guard")

    def test_no_partial_match(self):
        p = tp._test_pattern("arch.arm")
        assert not self._matches(p, "arch.arm64")
        assert not self._matches(p, "notarch.arm")

    def test_simple_name(self):
        p = tp._test_pattern("acpi")
        assert self._matches(p, "acpi")
        assert self._matches(p, "acpi.something")
        assert not self._matches(p, "not_acpi")

    def test_special_chars_escaped(self):
        # Dots in the name must match literally, not as wildcards
        p = tp._test_pattern("net.ip")
        import re

        assert not re.search(p, "netXip")


# ---------------------------------------------------------------------------
# PlanAccumulator
# ---------------------------------------------------------------------------


class TestPlanAccumulator:
    """Unit tests for PlanAccumulator."""

    def _suite(self, name, arch="arm", platform="qemu_x86", toolchain="zephyr"):
        return {"name": name, "arch": arch, "platform": platform, "toolchain": toolchain}

    def test_empty_accumulator(self):
        acc = tp.PlanAccumulator()
        assert len(acc) == 0
        assert acc.testsuites == []

    def test_merge_adds_suites(self):
        acc = tp.PlanAccumulator()
        added = acc.merge([self._suite("test.foo"), self._suite("test.bar")])
        assert added == 2
        assert len(acc) == 2

    def test_dedup_by_key(self):
        acc = tp.PlanAccumulator()
        acc.merge([self._suite("test.foo")])
        added = acc.merge([self._suite("test.foo")])  # exact duplicate
        assert added == 0
        assert len(acc) == 1

    def test_dedup_different_platform(self):
        acc = tp.PlanAccumulator()
        acc.merge([self._suite("test.foo", platform="native_sim")])
        added = acc.merge([self._suite("test.foo", platform="qemu_x86")])
        assert added == 1
        assert len(acc) == 2

    def test_write_json(self, tmp_path):
        acc = tp.PlanAccumulator()
        acc.merge([self._suite("test.foo"), self._suite("test.bar")])
        out = tmp_path / "testplan.json"
        acc.write(str(out))
        data = json.loads(out.read_text())
        assert "testsuites" in data
        assert len(data["testsuites"]) == 2


# ---------------------------------------------------------------------------
# Orchestrator._batch_calls
# ---------------------------------------------------------------------------


class TestBatchCalls:
    """Unit tests for the call-batching logic in Orchestrator."""

    def _call(self, desc, patterns=(), roots=(), platforms=(), integration=False, extra=()):
        return tp.TwisterCall(
            description=desc,
            test_patterns=list(patterns),
            testsuite_roots=list(roots),
            platforms=list(platforms),
            integration=integration,
            extra_args=list(extra),
        )

    def test_single_call_unchanged(self):
        call = self._call("a", roots=["tests/foo"])
        result = tp.Orchestrator._batch_calls([call])
        assert len(result) == 1
        assert result[0] is call

    def test_two_roots_only_merged(self):
        c1 = self._call("a", roots=["tests/foo"])
        c2 = self._call("b", roots=["tests/bar"])
        result = tp.Orchestrator._batch_calls([c1, c2])
        assert len(result) == 1
        assert set(result[0].testsuite_roots) == {"tests/foo", "tests/bar"}

    def test_two_patterns_only_merged(self):
        c1 = self._call("a", patterns=["^arch.arm"])
        c2 = self._call("b", patterns=["^net."])
        result = tp.Orchestrator._batch_calls([c1, c2])
        assert len(result) == 1
        assert "^arch.arm" in result[0].test_patterns
        assert "^net." in result[0].test_patterns

    def test_different_integration_not_merged(self):
        c1 = self._call("a", roots=["tests/foo"], integration=False)
        c2 = self._call("b", roots=["tests/bar"], integration=True)
        result = tp.Orchestrator._batch_calls([c1, c2])
        assert len(result) == 2

    def test_different_platforms_not_merged(self):
        c1 = self._call("a", roots=["tests/foo"], platforms=["native_sim"])
        c2 = self._call("b", roots=["tests/bar"], platforms=["qemu_x86"])
        result = tp.Orchestrator._batch_calls([c1, c2])
        assert len(result) == 2

    def test_mixed_mode_not_merged(self):
        # A call with both patterns AND roots is 'mixed' and must not merge.
        c1 = self._call("a", patterns=["^net."], roots=["tests/net"])
        c2 = self._call("b", patterns=["^arch."], roots=["tests/arch"])
        result = tp.Orchestrator._batch_calls([c1, c2])
        assert len(result) == 2

    def test_empty_list(self):
        assert tp.Orchestrator._batch_calls([]) == []

    def test_dedup_roots_across_merge(self):
        c1 = self._call("a", roots=["tests/foo"])
        c2 = self._call("b", roots=["tests/foo"])  # same root in both
        result = tp.Orchestrator._batch_calls([c1, c2])
        assert len(result) == 1
        assert result[0].testsuite_roots.count("tests/foo") == 1


# ---------------------------------------------------------------------------
# Orchestrator._write_dotplan
# ---------------------------------------------------------------------------


class TestWriteDotplan:
    """Unit tests for node-count calculation in _write_dotplan."""

    def _write(self, total, tests_per_builder=900, tmp_path=None):
        orch = tp.Orchestrator(strategies=[], executor=None, tests_per_builder=tests_per_builder)
        original_cwd = os.getcwd()
        try:
            os.chdir(tmp_path or original_cwd)
            orch._write_dotplan(total, full=False)
            return Path(".testplan").read_text()
        finally:
            os.chdir(original_cwd)

    def test_zero_tests_zero_nodes(self, tmp_path):
        out = self._write(0, tmp_path=tmp_path)
        assert "TWISTER_NODES=0" in out
        assert "TWISTER_TESTS=0" in out

    def test_less_than_one_builder_gives_one_node(self, tmp_path):
        out = self._write(500, tests_per_builder=900, tmp_path=tmp_path)
        assert "TWISTER_NODES=1" in out

    def test_exactly_one_builder(self, tmp_path):
        out = self._write(900, tests_per_builder=900, tmp_path=tmp_path)
        assert "TWISTER_NODES=1" in out

    def test_rounds_up_not_down(self, tmp_path):
        # 901 / 900 = 1.001... which floor gives 1 but ceil gives 2
        out = self._write(901, tests_per_builder=900, tmp_path=tmp_path)
        assert "TWISTER_NODES=2" in out

    def test_large_count_ceil(self, tmp_path):
        # 2701 / 900 = 3.001... -> ceil = 4
        out = self._write(2701, tests_per_builder=900, tmp_path=tmp_path)
        expected_nodes = math.ceil(2701 / 900)
        assert f"TWISTER_NODES={expected_nodes}" in out

    def test_full_flag_written(self, tmp_path):
        orch = tp.Orchestrator(strategies=[], executor=None)
        os.chdir(tmp_path)
        orch._write_dotplan(10, full=True)
        out = Path(".testplan").read_text()
        assert "TWISTER_FULL=True" in out

    def test_not_full_flag_written(self, tmp_path):
        orch = tp.Orchestrator(strategies=[], executor=None)
        os.chdir(tmp_path)
        orch._write_dotplan(10, full=False)
        out = Path(".testplan").read_text()
        assert "TWISTER_FULL=False" in out


# ---------------------------------------------------------------------------
# MaintainerAreaStrategy
# ---------------------------------------------------------------------------


class TestMaintainerAreaStrategy:
    """Unit tests for MaintainerAreaStrategy using a synthetic MAINTAINERS.yml."""

    _MAINTAINERS_YAML = textwrap.dedent("""\
        Bluetooth:
          files:
            - subsys/bluetooth/
          tests:
            - bluetooth.host
            - bluetooth.audio

        GPIO Drivers:
          files:
            - drivers/gpio/
          tests:
            - drivers.gpio

        No Tests Area:
          files:
            - doc/
    """)

    @pytest.fixture()
    def mf(self, tmp_path):
        f = tmp_path / "MAINTAINERS.yml"
        f.write_text(self._MAINTAINERS_YAML)
        return str(f)

    def test_matching_area_emits_call(self, mf):
        import re

        strategy = tp.MaintainerAreaStrategy(mf)
        calls, handled = strategy.analyze(["subsys/bluetooth/host/hci_core.c"])
        assert len(calls) == 1
        # Patterns are regexes produced by _test_pattern(); verify they match the test names.
        patterns = calls[0].test_patterns
        assert any(re.search(p, "bluetooth.host") for p in patterns)
        assert "subsys/bluetooth/host/hci_core.c" in handled

    def test_area_without_tests_skipped(self, mf):
        strategy = tp.MaintainerAreaStrategy(mf)
        calls, _ = strategy.analyze(["doc/README.rst"])
        assert calls == []

    def test_multiple_areas_multiple_calls(self, mf):
        strategy = tp.MaintainerAreaStrategy(mf)
        calls, _ = strategy.analyze(
            [
                "subsys/bluetooth/host/hci_core.c",
                "drivers/gpio/gpio_nrfx.c",
            ]
        )
        assert len(calls) == 2
        descs = {c.description for c in calls}
        assert any("Bluetooth" in d for d in descs)
        assert any("GPIO" in d for d in descs)

    def test_no_matching_files_returns_empty(self, mf):
        strategy = tp.MaintainerAreaStrategy(mf)
        calls, _ = strategy.analyze(["kernel/sched.c"])
        assert calls == []

    def test_platform_filter_propagated(self, mf):
        strategy = tp.MaintainerAreaStrategy(mf, platform_filter=["native_sim"])
        calls, _ = strategy.analyze(["subsys/bluetooth/host/hci_core.c"])
        assert len(calls) == 1
        assert calls[0].platforms == ["native_sim"]


# ---------------------------------------------------------------------------
# IgnoreStrategy
# ---------------------------------------------------------------------------


class TestIgnoreStrategy:
    """Unit tests for IgnoreStrategy."""

    def _make_ignore(self, tmp_path, patterns):
        f = tmp_path / "twister_ignore.txt"
        f.write_text("\n".join(patterns) + "\n")
        return tp.IgnoreStrategy(ignore_file=str(f))

    def test_ignored_file_consumed(self, tmp_path):
        strategy = self._make_ignore(tmp_path, ["doc/**", "*.rst"])
        calls, handled = strategy.analyze(["doc/index.rst", "doc/release-notes.rst"])
        assert calls == []
        assert handled == {"doc/index.rst", "doc/release-notes.rst"}

    def test_non_matching_file_not_consumed(self, tmp_path):
        strategy = self._make_ignore(tmp_path, ["doc/**"])
        _, handled = strategy.analyze(["drivers/gpio/gpio.c"])
        assert "drivers/gpio/gpio.c" not in handled

    def test_comment_lines_ignored(self, tmp_path):
        strategy = self._make_ignore(tmp_path, ["# this is a comment", "doc/**"])
        _, handled = strategy.analyze(["doc/index.rst"])
        assert "doc/index.rst" in handled

    def test_blank_lines_ignored(self, tmp_path):
        strategy = self._make_ignore(tmp_path, ["", "  ", "doc/**"])
        _, handled = strategy.analyze(["doc/index.rst"])
        assert "doc/index.rst" in handled

    def test_missing_ignore_file_no_crash(self, tmp_path):
        strategy = tp.IgnoreStrategy(ignore_file=str(tmp_path / "nonexistent.txt"))
        _, handled = strategy.analyze(["doc/index.rst"])
        assert _ == []
        assert handled == set()

    def test_consumes_is_true(self):
        assert tp.IgnoreStrategy.consumes is True


# ---------------------------------------------------------------------------
# DirectTestStrategy
# ---------------------------------------------------------------------------


class TestDirectTestStrategy:
    """Unit tests for DirectTestStrategy."""

    def _make_tree(self, base):
        test_dir = base / "tests" / "kernel" / "sched"
        test_dir.mkdir(parents=True)
        (test_dir / "testcase.yaml").write_text("tests:\n  kernel.sched: {}\n")
        (test_dir / "src" / "main.c").parent.mkdir()
        (test_dir / "src" / "main.c").write_text("// test\n")
        return test_dir

    def test_finds_test_root(self, tmp_path):
        self._make_tree(tmp_path)
        strategy = tp.DirectTestStrategy(zephyr_base=str(tmp_path))
        calls, handled = strategy.analyze(["tests/kernel/sched/src/main.c"])
        assert len(calls) == 1
        assert "tests/kernel/sched" in calls[0].testsuite_roots[0]
        assert "tests/kernel/sched/src/main.c" in handled

    def test_non_test_file_ignored(self, tmp_path):
        strategy = tp.DirectTestStrategy(zephyr_base=str(tmp_path))
        calls, _ = strategy.analyze(["drivers/gpio/gpio.c"])
        assert calls == []
        assert _ == set()

    def test_deduplication_of_same_root(self, tmp_path):
        self._make_tree(tmp_path)
        strategy = tp.DirectTestStrategy(zephyr_base=str(tmp_path))
        calls, _ = strategy.analyze(
            [
                "tests/kernel/sched/src/main.c",
                "tests/kernel/sched/testcase.yaml",
            ]
        )
        assert len(calls) == 1  # same root -> merged

    def test_no_yaml_marker_returns_no_call(self, tmp_path):
        lost_dir = tmp_path / "tests" / "no_yaml"
        lost_dir.mkdir(parents=True)
        (lost_dir / "src" / "main.c").parent.mkdir()
        (lost_dir / "src" / "main.c").write_text("")
        strategy = tp.DirectTestStrategy(zephyr_base=str(tmp_path))
        calls, _ = strategy.analyze(["tests/no_yaml/src/main.c"])
        assert calls == []

    def test_sample_yaml_as_marker(self, tmp_path):
        sample_dir = tmp_path / "samples" / "hello_world"
        sample_dir.mkdir(parents=True)
        (sample_dir / "sample.yaml").write_text("sample:\n  name: hello_world\n")
        strategy = tp.DirectTestStrategy(zephyr_base=str(tmp_path))
        calls, _ = strategy.analyze(["samples/hello_world/src/main.c"])
        assert len(calls) == 1


# ---------------------------------------------------------------------------
# SnippetStrategy helpers
# ---------------------------------------------------------------------------


class TestSnippetStrategy:
    """Unit tests for SnippetStrategy helper methods."""

    def _strategy(self, tmp_path):
        return tp.SnippetStrategy(zephyr_base=str(tmp_path))

    def test_find_snippet_name_direct(self, tmp_path):
        snip_dir = tmp_path / "snippets" / "my-snippet"
        snip_dir.mkdir(parents=True)
        (snip_dir / "snippet.yml").write_text("name: my-snippet\n")
        s = self._strategy(tmp_path)
        assert s._find_snippet_name(snip_dir / "some-file.conf") == "my-snippet"

    def test_find_snippet_name_no_yml(self, tmp_path):
        snip_dir = tmp_path / "snippets" / "vendor" / "sub"
        snip_dir.mkdir(parents=True)
        s = self._strategy(tmp_path)
        assert s._find_snippet_name(snip_dir / "some-file.conf") is None

    def test_find_snippet_name_no_name_field(self, tmp_path):
        snip_dir = tmp_path / "snippets" / "bad-snippet"
        snip_dir.mkdir(parents=True)
        (snip_dir / "snippet.yml").write_text("description: no name here\n")
        s = self._strategy(tmp_path)
        assert s._find_snippet_name(snip_dir / "file.conf") is None

    def test_check_manifest_required_snippets(self, tmp_path):
        manifest = tmp_path / "testcase.yaml"
        manifest.write_text(
            textwrap.dedent("""\
            tests:
              my.test:
                required_snippets:
                  - my-snippet
        """)
        )
        s = self._strategy(tmp_path)
        acc: set = set()
        s._check_manifest(manifest, "my-snippet", acc)
        assert len(acc) == 1

    def test_check_manifest_no_match(self, tmp_path):
        manifest = tmp_path / "testcase.yaml"
        manifest.write_text(
            textwrap.dedent("""\
            tests:
              my.test:
                required_snippets:
                  - other-snippet
        """)
        )
        s = self._strategy(tmp_path)
        acc: set = set()
        s._check_manifest(manifest, "my-snippet", acc)
        assert len(acc) == 0

    def test_check_manifest_common_section(self, tmp_path):
        manifest = tmp_path / "testcase.yaml"
        manifest.write_text(
            textwrap.dedent("""\
            common:
              required_snippets:
                - my-snippet
            tests:
              my.test: {}
        """)
        )
        s = self._strategy(tmp_path)
        acc: set = set()
        s._check_manifest(manifest, "my-snippet", acc)
        assert len(acc) == 1

    def test_analyze_non_snippet_files_ignored(self, tmp_path):
        s = self._strategy(tmp_path)
        calls, handled = s.analyze(["drivers/gpio/gpio.c", "kernel/sched.c"])
        assert calls == []
        assert handled == set()


# ---------------------------------------------------------------------------
# SoCStrategy helpers
# ---------------------------------------------------------------------------


class TestSoCStrategy:
    """Unit tests for SoCStrategy._collect_soc_names."""

    def _strategy(self, tmp_path):
        return tp.SoCStrategy(zephyr_base=str(tmp_path))

    def _write_soc_yml(self, path, content):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content)

    def test_collect_flat_soc_names(self, tmp_path):
        soc_yml = tmp_path / "soc" / "nordic" / "soc.yml"
        self._write_soc_yml(
            soc_yml,
            textwrap.dedent("""\
            family:
              - name: nordic_nrf
                series:
                  - name: nrf52
                    socs:
                      - name: nrf52840
                      - name: nrf52833
                      - name: nrf52811
        """),
        )
        s = self._strategy(tmp_path)
        names = s._collect_soc_names(soc_yml)
        assert set(names) == {"nrf52840", "nrf52833", "nrf52811"}

    def test_collect_with_prefix_filter(self, tmp_path):
        soc_yml = tmp_path / "soc" / "nordic" / "soc.yml"
        self._write_soc_yml(
            soc_yml,
            textwrap.dedent("""\
            family:
              - name: nordic_nrf
                series:
                  - name: nrf52
                    socs:
                      - name: nrf52840
                      - name: nrf52833
                  - name: nrf91
                    socs:
                      - name: nrf9160
        """),
        )
        s = self._strategy(tmp_path)
        names = s._collect_soc_names(soc_yml, group_prefix="nrf52")
        assert set(names) == {"nrf52840", "nrf52833"}
        assert "nrf9160" not in names

    def test_collect_empty_on_bad_file(self, tmp_path):
        soc_yml = tmp_path / "soc" / "bad" / "soc.yml"
        self._write_soc_yml(soc_yml, ": invalid: yaml: [")
        s = self._strategy(tmp_path)
        assert s._collect_soc_names(soc_yml) == []

    def test_find_soc_yml_walks_up(self, tmp_path):
        soc_dir = tmp_path / "soc" / "nordic" / "nrf52"
        soc_dir.mkdir(parents=True)
        (tmp_path / "soc" / "nordic" / "soc.yml").write_text("family: []\n")
        s = self._strategy(tmp_path)
        result = s._find_soc_yml(soc_dir / "nrf52840" / "soc.h")
        assert result is not None
        assert result.name == "soc.yml"

    def test_find_soc_yml_returns_none_when_missing(self, tmp_path):
        soc_dir = tmp_path / "soc" / "novendor" / "nochip"
        soc_dir.mkdir(parents=True)
        s = self._strategy(tmp_path)
        assert s._find_soc_yml(soc_dir / "soc.h") is None


# ---------------------------------------------------------------------------
# BoardStrategy helpers
# ---------------------------------------------------------------------------


class TestBoardStrategy:
    """Unit tests for BoardStrategy filesystem helpers."""

    def _strategy(self, tmp_path):
        return tp.BoardStrategy(zephyr_base=str(tmp_path))

    def _make_board(self, base, vendor, board):
        bd = base / "boards" / vendor / board
        bd.mkdir(parents=True)
        (bd / "board.yml").write_text(f"board:\n  name: {board}\n")
        return bd

    def test_find_board_yml_dir_direct(self, tmp_path):
        bd = self._make_board(tmp_path, "nordic", "nrf52840dk")
        s = self._strategy(tmp_path)
        result = s._find_board_yml_dir(bd / "nrf52840dk.yaml")
        assert result == bd

    def test_find_board_yml_dir_walks_up(self, tmp_path):
        bd = self._make_board(tmp_path, "nordic", "nrf52840dk")
        sub = bd / "sub" / "dir"
        sub.mkdir(parents=True)
        s = self._strategy(tmp_path)
        result = s._find_board_yml_dir(sub / "some_file.c")
        assert result == bd

    def test_find_board_yml_dir_shield_returns_none(self, tmp_path):
        shield_dir = tmp_path / "boards" / "shields" / "my_shield"
        shield_dir.mkdir(parents=True)
        s = self._strategy(tmp_path)
        result = s._find_board_yml_dir(shield_dir / "Kconfig.shield")
        assert result is None

    def test_find_twister_yaml(self, tmp_path):
        bd = self._make_board(tmp_path, "intel", "intel_adsp")
        (bd / "twister.yaml").write_text("variants:\n  intel_adsp/ace15:\n    twister: true\n")
        s = self._strategy(tmp_path)
        result = s._find_twister_yaml(bd)
        assert result is not None
        assert result.name == "twister.yaml"

    def test_find_twister_yaml_walks_up(self, tmp_path):
        parent = tmp_path / "boards" / "intel"
        parent.mkdir(parents=True)
        (parent / "twister.yaml").write_text("variants: {}\n")
        bd = parent / "intel_adsp"
        bd.mkdir()
        (bd / "board.yml").write_text("board:\n  name: intel_adsp\n")
        s = self._strategy(tmp_path)
        result = s._find_twister_yaml(bd)
        assert result is not None

    def test_find_twister_yaml_missing_returns_none(self, tmp_path):
        bd = self._make_board(tmp_path, "nordic", "nrf52840dk")
        s = self._strategy(tmp_path)
        assert s._find_twister_yaml(bd) is None

    def test_analyze_non_board_files_ignored(self, tmp_path):
        s = self._strategy(tmp_path)
        calls, handled = s.analyze(["drivers/gpio/gpio.c", "kernel/sched.c"])
        assert calls == []
        assert handled == set()

    def test_analyze_shield_file_ignored(self, tmp_path):
        shield_dir = tmp_path / "boards" / "shields" / "my_shield"
        shield_dir.mkdir(parents=True)
        s = self._strategy(tmp_path)
        calls, handled = s.analyze(["boards/shields/my_shield/Kconfig.shield"])
        assert calls == []
        assert handled == set()


# ---------------------------------------------------------------------------
# KconfigImpactStrategy symbol extraction
# ---------------------------------------------------------------------------


class TestKconfigImpactStrategy:
    """Unit tests for the Kconfig symbol extraction regexes."""

    def test_kconfig_symbol_re_config(self):
        src = "config GPIO_NRFX\n\tbool\n"
        matches = tp.KconfigImpactStrategy._KCONFIG_SYMBOL_RE.findall(src)
        assert "GPIO_NRFX" in matches

    def test_kconfig_symbol_re_menuconfig(self):
        src = "menuconfig SERIAL\n\tbool\n"
        matches = tp.KconfigImpactStrategy._KCONFIG_SYMBOL_RE.findall(src)
        assert "SERIAL" in matches

    def test_conf_symbol_re(self):
        src = "CONFIG_SERIAL=y\nCONFIG_UART_NRFX=y\n# CONFIG_LOG=n\n"
        matches = tp.KconfigImpactStrategy._CONF_SYMBOL_RE.findall(src)
        assert "SERIAL" in matches
        assert "UART_NRFX" in matches

    def test_conf_symbol_re_skips_comments(self):
        src = "# CONFIG_LOG=n\n"
        matches = tp.KconfigImpactStrategy._CONF_SYMBOL_RE.findall(src)
        assert matches == []


# ---------------------------------------------------------------------------
# Orchestrator.run with mocked executor
# ---------------------------------------------------------------------------


class TestOrchestratorRun:
    """Integration-style tests for the Orchestrator with a mock executor."""

    def _make_executor(self, suites=None):
        executor = mock.MagicMock()
        executor.execute.return_value = suites or []
        return executor

    def test_empty_changed_files(self, tmp_path):
        os.chdir(tmp_path)
        orch = tp.Orchestrator(strategies=[], executor=self._make_executor())
        errors = orch.run([], output_file=str(tmp_path / "testplan.json"))
        assert errors == 0
        plan_text = (tmp_path / ".testplan").read_text()
        assert "TWISTER_TESTS=0" in plan_text
        assert "TWISTER_NODES=0" in plan_text

    def test_unresolved_files_trigger_full_run(self, tmp_path):
        os.chdir(tmp_path)
        orch = tp.Orchestrator(strategies=[], executor=self._make_executor())
        orch.run(["some/unknown/file.c"], output_file=str(tmp_path / "testplan.json"))
        plan_text = (tmp_path / ".testplan").read_text()
        assert "TWISTER_FULL=True" in plan_text

    def test_strategy_consumes_files(self, tmp_path):
        """When a consuming strategy handles all files, subsequent strategies
        are skipped (remaining is empty) by the Orchestrator."""
        os.chdir(tmp_path)

        class ConsumeAll(tp.SelectionStrategy):
            consumes = True

            @property
            def name(self):
                return "ConsumeAll"

            def analyze(self, changed_files):
                return [], set(changed_files)

        class ShouldBeSkipped(tp.SelectionStrategy):
            consumes = False
            reached = False

            @property
            def name(self):
                return "ShouldBeSkipped"

            def analyze(self, changed_files):
                ShouldBeSkipped.reached = True
                return [], set()

        consumer = ConsumeAll()
        downstream = ShouldBeSkipped()
        orch = tp.Orchestrator(
            strategies=[consumer, downstream],
            executor=self._make_executor(),
        )
        orch.run(["drivers/gpio/gpio.c"], output_file=str(tmp_path / "testplan.json"))
        # After ConsumeAll consumes all files, 'remaining' is empty,
        # so Orchestrator skips ShouldBeSkipped entirely.
        assert ShouldBeSkipped.reached is False

    def test_full_run_signal_short_circuits(self, tmp_path):
        os.chdir(tmp_path)
        executor = self._make_executor()

        class FullRunStrategy(tp.SelectionStrategy):
            consumes = False

            @property
            def name(self):
                return "FullRun"

            def analyze(self, changed_files):
                return [tp.TwisterCall(description="full", full_run=True)], set()

        class DownstreamStrategy(tp.SelectionStrategy):
            reached = False
            consumes = False

            @property
            def name(self):
                return "Downstream"

            def analyze(self, changed_files):
                DownstreamStrategy.reached = True
                return [], set()

        orch = tp.Orchestrator(
            strategies=[FullRunStrategy(), DownstreamStrategy()],
            executor=executor,
        )
        orch.run(["something.c"], output_file=str(tmp_path / "testplan.json"))
        plan_text = (tmp_path / ".testplan").read_text()
        assert "TWISTER_FULL=True" in plan_text

    def test_suites_written_to_output(self, tmp_path):
        os.chdir(tmp_path)
        suites = [{"name": "foo.test", "arch": "arm", "platform": "qemu", "toolchain": "zephyr"}]
        executor = self._make_executor(suites=suites)

        class AddSuites(tp.SelectionStrategy):
            consumes = False

            @property
            def name(self):
                return "AddSuites"

            def analyze(self, changed_files):
                return [tp.TwisterCall(description="test", full_run=False)], set(changed_files)

        orch = tp.Orchestrator(
            strategies=[AddSuites()],
            executor=executor,
        )
        out = str(tmp_path / "testplan.json")
        orch.run(["some/file.c"], output_file=out)
        data = json.loads(Path(out).read_text())
        assert len(data["testsuites"]) == 1
