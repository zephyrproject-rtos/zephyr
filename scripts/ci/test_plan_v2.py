#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Intel Corporation

"""Modular CI test selector for Zephyr.

Analyses the set of files changed in a Pull Request and selects a targeted
test plan to run in CI, avoiding tests that are wholly unrelated to the
change while maximising coverage of what was touched.

Architecture
============
Each *strategy* is an independent analyser.  A strategy receives the list of
changed files and returns a list of :class:`TwisterCall` descriptors – each
descriptor maps to exactly one ``twister --save-tests`` invocation.  The
orchestrator executes all calls, merges the resulting test-suite lists, removes
duplicates and writes the consolidated ``testplan.json``.

To add a new strategy:
  1. Subclass :class:`SelectionStrategy` and implement ``name`` / ``analyze``.
  2. Append an instance to the list in :func:`build_strategies`.

Current strategies (execution order)
=====================================
0. :class:`ComplexityStrategy` – analyses patchset complexity using
   **pydriller** (commit-level metrics: churn, DMM unit-complexity,
   changed-method CCN) and **lizard** (full-file average CCN delta).
   Emits a composite score into a shared :class:`PipelineContext`.  The
   :class:`RiskClassifierStrategy` reads this score to optionally escalate
   NORMAL-risk files to WIDE coverage.  Non-consuming; emits no twister calls.
1. :class:`IgnoreStrategy` – reads ``scripts/ci/twister_ignore.txt`` and
   silently drops any changed file that matches a glob pattern in that file
   (documentation, tooling, CI workflows, etc.).  Consumes matched files so
   downstream strategies never see them.
2. :class:`DirectTestStrategy` – for changed files that live directly
   inside a ``tests/`` or ``samples/`` tree, walks up to find the
   ``testcase.yaml`` / ``tests.yaml`` / ``sample.yaml`` root and runs
   so the exact test suite is exercised end-to-end.
   Consumes matched files so downstream strategies do not inflate the plan.
3. :class:`SnippetStrategy` – for changed files under ``snippets/``, reads
   the snippet name from ``snippet.yml`` then finds and runs only the tests
   that declare that snippet as a ``required_snippets`` dependency.
   Consumes matched files.
4. :class:`BoardStrategy` – for changed files under ``boards/``, uses
   ``scripts/list_boards.py`` to enumerate all board variants (SoC /
   CPU-cluster combinations) and runs ``tests/integration/kernel`` on each.
   Consumes matched files so downstream strategies do not see board changes.
5. :class:`SoCStrategy` – for changed files under ``soc/``, resolves the
   SoC names declared in the nearest ``soc.yml``, finds every board whose
   ``board.yml`` references those SoCs, and runs ``tests/integration/kernel``
   on each.  Consumes matched files.
6. :class:`ManifestStrategy` – for changed ``west.yml`` or
   ``submanifests/*.yaml`` files, diffs the manifest against its previous
   revision (requires ``--commits``), identifies added/removed/updated west
   projects, and runs ``--tag <module> --integration`` so only tests that
   declare that module as a tag are selected.  Consumes matched files.
7. :class:`DriverCompatStrategy` – for changed files under ``drivers/``,
   extracts ``DT_DRV_COMPAT`` from the source, converts to a DTS compatible
   string, then searches ``tests/`` and ``samples/`` for overlay/DTS files
   that instantiate that compatible.  The containing test directories are
   added as ``-T`` roots so twister exercises the actual driver code.
8. :class:`DtsBindingStrategy` – for changed ``dts/bindings/**/*.yaml``
   files, reads the ``compatible:`` field and feeds it through the same
   compat-resolution chain used by :class:`DriverCompatStrategy` (overlay
   scan + board-targeted area calls).  Additive, not consuming.
9. :class:`KconfigImpactStrategy` – for changed Kconfig definition files,
   config fragments, and board defconfigs, extracts the affected symbol
   names and greps ``tests/`` and ``samples/`` for ``.conf`` / ``.yaml``
   files that enable those symbols.  Per-symbol and per-test-root thresholds
   prevent core-symbol changes from triggering a near-full run.
10. :class:`HeaderImpactStrategy` – for changed headers under
    ``include/zephyr/``, greps the source tree for files that ``#include``
    the header, then maps those files to MAINTAINERS areas with tests.
    Headers included in more than ``_MAX_INCLUDE_REFS`` files are treated as
    "widespread" and skipped to prevent a near-full run from a single common
    header change (e.g. ``kernel.h``).
11. :class:`MaintainerAreaStrategy` – catch-all: matches any remaining
    changed files against ``MAINTAINERS.yml`` areas and uses the ``tests:``
    list from each matching area to build ``--test-pattern`` arguments.

Output
======
* ``<output_file>`` (default ``testplan.json``) – passed to twister via
  ``twister --load-tests``.
* ``.testplan`` – plain env-var file (``TWISTER_TESTS=``, ``TWISTER_NODES=``,
  ``TWISTER_FULL=``) consumed by CI orchestration scripts.
"""

from __future__ import annotations

import abc
import argparse
import enum
import fnmatch
import json
import logging
import os
import re
import subprocess
import sys
import tempfile
from dataclasses import dataclass, field
from pathlib import Path

import yaml
from git import Repo

try:
    from yaml import CSafeLoader as SafeLoader
except ImportError:
    from yaml import SafeLoader

# ---------------------------------------------------------------------------
# Bootstrap
# ---------------------------------------------------------------------------

if "ZEPHYR_BASE" not in os.environ:
    sys.exit("$ZEPHYR_BASE environment variable undefined.")

ZEPHYR_BASE = Path(os.environ["ZEPHYR_BASE"])
sys.path.insert(0, str(ZEPHYR_BASE / "scripts"))

logging.basicConfig(format="%(levelname)s: %(message)s", level=logging.INFO)
log = logging.getLogger(__name__)


# ---------------------------------------------------------------------------
# Pipeline context – shared state across strategies
# ---------------------------------------------------------------------------


@dataclass
class PipelineContext:
    """Mutable context object threaded through all strategies.

    Strategies may read and write fields here to share derived information
    without coupling strategy classes to each other directly.

    Attributes
    ----------
    complexity_score:
        Aggregate patchset complexity score produced by
        :class:`ComplexityStrategy` (``0.0`` if not yet computed or if the
        strategy was not enabled).  Ranges from ``0.0`` (trivial) upward,
        with no fixed upper bound.  The :class:`RiskClassifierStrategy` reads
        this to optionally escalate NORMAL-risk files to WIDE-risk.
    file_metrics:
        Per-file mapping of ``{relative_path: ComplexityMetrics}``.  Provides
        method-level detail for logging and downstream decision-making.
    """

    complexity_score: float = 0.0
    file_metrics: dict = field(default_factory=dict)  # path → ComplexityMetrics


@dataclass
class ComplexityMetrics:
    """Complexity measurements for a single modified source file.

    Attributes
    ----------
    filename:
        Workspace-relative path.
    churn:
        Total lines added + deleted by the patch.
    max_method_ccn:
        Highest cyclomatic complexity (CCN) among methods touched by the diff.
        ``0`` when no methods were identified.
    avg_ccn_delta:
        Change in average CCN across all methods in the file
        (new ``average_cyclomatic_complexity`` minus old).  Positive means
        the patch made the file more complex.
    nloc_delta:
        Change in non-comment lines-of-code (new minus old).
    dmm_complexity:
        pydriller's DMM unit-complexity metric for the commit
        (fraction of methods with CCN > 5).  ``None`` when unavailable.
    """

    filename: str
    churn: int = 0
    max_method_ccn: int = 0
    avg_ccn_delta: float = 0.0
    nloc_delta: int = 0
    dmm_complexity: float | None = None


# ---------------------------------------------------------------------------
# Core data types
# ---------------------------------------------------------------------------


@dataclass
class TwisterCall:
    """Describes a single ``twister -c --save-tests …`` invocation.

    Attributes
    ----------
    description:
        Human-readable label shown in log output.
    test_patterns:
        List of ``--test-pattern`` regex arguments.  Each pattern is matched
        against ``suite.id`` via ``re.search``.  An empty list means "no
        pattern filter" (all suites).
    platforms:
        ``-p <platform>`` restrictions.  Empty means no platform restriction.
    testsuite_roots:
        Extra ``-T <dir>`` roots.  Empty means the twister defaults
        (``tests/`` and ``samples/``).
    integration:
        Pass ``--integration`` to twister when *True*.
    extra_args:
        Any additional raw twister arguments appended verbatim.
    """

    description: str
    test_patterns: list = field(default_factory=list)
    platforms: list = field(default_factory=list)
    testsuite_roots: list = field(default_factory=list)
    integration: bool = False
    extra_args: list = field(default_factory=list)
    full_run: bool = False  # when True, signal TWISTER_FULL and skip test plan


# ---------------------------------------------------------------------------
# Plan accumulator
# ---------------------------------------------------------------------------


class PlanAccumulator:
    """Collects test suites produced by multiple twister calls.

    Each call appends its results; :meth:`testsuites` returns the deduplicated
    union.  The dedup key is ``(name, arch, platform, toolchain)`` which
    matches the logic used in the original ``test_plan.py``.
    """

    def __init__(self):
        self._suites = []
        self._seen = set()

    def merge(self, suites):
        """Merge *suites* into the accumulator, skipping duplicates.

        Returns the number of *new* suites added.
        """
        added = 0
        for ts in suites:
            key = (
                ts.get("name"),
                ts.get("arch"),
                ts.get("platform"),
                ts.get("toolchain"),
            )
            if key not in self._seen:
                self._suites.append(ts)
                self._seen.add(key)
                added += 1
        return added

    @property
    def testsuites(self):
        return list(self._suites)

    def __len__(self):
        return len(self._suites)

    def write(self, path):
        """Write the accumulated test suites to *path* as twister JSON."""
        with open(path, "w", newline="", encoding="utf-8") as fh:
            json.dump({"testsuites": self._suites}, fh, indent=4, separators=(",", ":"))
        log.info("Test plan written to %s (%d suites)", path, len(self._suites))


# ---------------------------------------------------------------------------
# Strategy base class
# ---------------------------------------------------------------------------


class SelectionStrategy(abc.ABC):
    """Base class for test-selection strategies.

    Implement :attr:`name` and :meth:`analyze`.  ``analyze`` inspects the
    changed-file list and returns the twister calls needed to cover those
    changes.  It must also report which files it has *handled* so the
    orchestrator can detect unresolved files and fall back to a full run.

    Class attributes
    ----------------
    consumes : bool
        When *True*, files returned as *handled* by this strategy are
        **removed** from the pool before the next strategy runs.  Only
        set this on strategies that are *authoritative* for their file
        type — i.e. strategies where seeing the file in a later strategy
        would produce unhelpful or redundant results.

        The default is *False*: strategies are purely additive and every
        remaining file is passed to all downstream strategies unchanged.
        :class:`DirectTestStrategy` and :class:`RiskClassifierStrategy`
        are the canonical consumers: a changed test file must not trigger
        the area-wide test sweep, and a SKIP/FULL/WIDE classification
        must prevent downstream strategies from overriding that decision.
    """

    consumes: bool = False

    @property
    @abc.abstractmethod
    def name(self):
        """Short identifier used in log messages."""

    @abc.abstractmethod
    def analyze(self, changed_files):
        """Analyse *changed_files* and return ``(calls, handled_files)``.

        ``calls`` is a list of :class:`TwisterCall`.
        ``handled_files`` is the subset of *changed_files* that this strategy
        has produced coverage for.  Files not handled by any strategy cause
        the orchestrator to log a warning about unresolved files.
        """


# ---------------------------------------------------------------------------
# Helper: file-pattern matching (shared across strategies)
# ---------------------------------------------------------------------------


def _matches_area(filepath, area):
    """Return *True* if *filepath* belongs to the MAINTAINERS area *area*."""
    files = area.get("files", [])
    files_regex = area.get("files-regex", [])
    files_exclude = area.get("files-exclude", [])
    files_regex_exclude = area.get("files-regex-exclude", [])

    matched = False

    for pattern in files:
        if pattern.endswith("/"):
            if filepath.startswith(pattern):
                matched = True
                break
        elif fnmatch.fnmatch(filepath, pattern):
            matched = True
            break

    if not matched:
        for regex in files_regex:
            if re.search(regex, filepath):
                matched = True
                break

    if not matched:
        return False

    for pattern in files_exclude:
        if pattern.endswith("/"):
            if filepath.startswith(pattern):
                return False
        elif fnmatch.fnmatch(filepath, pattern):
            return False

    return all(not re.search(regex, filepath) for regex in files_regex_exclude)


def _test_pattern(test_name):
    """Convert a MAINTAINERS ``tests:`` entry to a ``--test-pattern`` regex.

    The entry is treated as a dot-separated test-name prefix.  The resulting
    regex matches the entry itself and any sub-identifiers separated by a dot
    (e.g. ``arch.arm`` matches ``arch.arm`` and ``arch.arm.irq``, but NOT
    ``arch.arm64``).
    """
    escaped = re.escape(test_name)  # e.g. arch\.arm  /  acpi  /  sip_svc
    return r"^" + escaped + r"(\.|$)"  # ^arch\.arm(\.|$)


# ---------------------------------------------------------------------------
# Strategy 1 – Maintainer area
# ---------------------------------------------------------------------------


class MaintainerAreaStrategy(SelectionStrategy):
    """Match changed files against MAINTAINERS.yml and use the ``tests:`` list.

    For each MAINTAINERS area whose file patterns overlap with the changed
    files *and* that has a non-empty ``tests:`` list, this strategy emits one
    :class:`TwisterCall` carrying ``--test-pattern`` arguments built from
    those test names.

    All patterns for a single area are batched into one twister call to avoid
    spawning many processes.  A separate call is made per area so that the
    log clearly shows which area drove which test selection.
    """

    @property
    def name(self):
        return "MaintainerArea"

    def __init__(self, maintainers_file, platform_filter=None):
        self._maintainers_file = maintainers_file
        self._platform_filter = platform_filter or []

    def analyze(self, changed_files):
        areas = self._load_areas()
        calls = []
        handled = set()

        for area_name, area_data in areas.items():
            test_names = area_data.get("tests", [])
            if not test_names:
                continue

            matched_files = {f for f in changed_files if _matches_area(f, area_data)}
            if not matched_files:
                continue

            handled.update(matched_files)

            patterns = [_test_pattern(t) for t in test_names]
            log.info(
                "[%s] Area '%s': %d file(s) matched → patterns: %s",
                self.name,
                area_name,
                len(matched_files),
                ", ".join(repr(p) for p in patterns),
            )

            calls.append(
                TwisterCall(
                    description=f"MaintainerArea: {area_name}",
                    test_patterns=patterns,
                    platforms=list(self._platform_filter),
                )
            )

        if not calls:
            log.info("[%s] No areas with tests matched the changed files.", self.name)

        return calls, handled

    def _load_areas(self):
        with open(self._maintainers_file, encoding="utf-8") as fh:
            data = yaml.load(fh, Loader=SafeLoader)
        return data if isinstance(data, dict) else {}


# ---------------------------------------------------------------------------
# Twister executor
# ---------------------------------------------------------------------------


class TwisterExecutor:
    """Executes :class:`TwisterCall` descriptors and collects results.

    Each call saves its results to a temporary JSON file (``--save-tests``).
    The file is read and its ``testsuites`` list is returned for accumulation.
    """

    def __init__(
        self,
        zephyr_base,
        extra_testsuite_roots=None,
        detailed_test_id=False,
        quarantine_list=None,
    ):
        self._zephyr_base = Path(zephyr_base)
        self._extra_roots = extra_testsuite_roots or []
        self._detailed_test_id = detailed_test_id
        self._quarantine_list = quarantine_list or []

    def execute(self, call):
        """Run twister for *call* and return the list of test suites."""
        with tempfile.NamedTemporaryFile(
            suffix=".json", prefix="twister_partial_", delete=False
        ) as tf:
            partial_path = tf.name

        try:
            cmd = self._build_cmd(call, partial_path)
            log.info("Running: %s", " ".join(cmd))
            ret = subprocess.call(cmd)  # noqa: S603
            if ret != 0:
                log.warning("twister exited with code %d for call: %s", ret, call.description)

            if not os.path.exists(partial_path):
                log.warning("twister did not produce %s", partial_path)
                return []

            with open(partial_path, encoding="utf-8") as fh:
                data = json.load(fh)
            return data.get("testsuites", [])
        finally:
            if os.path.exists(partial_path):
                os.remove(partial_path)

    def _build_cmd(self, call, save_path):
        cmd = [str(self._zephyr_base / "scripts" / "twister"), "-c"]

        for pattern in call.test_patterns:
            cmd += ["--test-pattern", pattern]

        for platform in call.platforms:
            cmd += ["-p", platform]

        for root in call.testsuite_roots + self._extra_roots:
            cmd += ["-T", root]

        if call.integration:
            cmd.append("--integration")

        if self._detailed_test_id:
            cmd.append("--detailed-test-id")

        for q in self._quarantine_list:
            cmd += ["--quarantine-list", q]

        cmd += call.extra_args
        cmd += ["--save-tests", save_path]

        return cmd


# ---------------------------------------------------------------------------
# Orchestrator
# ---------------------------------------------------------------------------


class Orchestrator:
    """Runs all strategies, executes the twister calls, merges the plan.

    Parameters
    ----------
    strategies:
        Ordered list of :class:`SelectionStrategy` instances.
    executor:
        A :class:`TwisterExecutor` used to materialise :class:`TwisterCall`s.
    tests_per_builder:
        Node count divisor for the ``.testplan`` env-var file.
    """

    def __init__(self, strategies, executor, tests_per_builder=900):
        self._strategies = strategies
        self._executor = executor
        self._tests_per_builder = tests_per_builder

    def run(self, changed_files, output_file):
        """Run all strategies for *changed_files* and write *output_file*.

        Each strategy receives only the files not yet consumed by an earlier
        strategy (see :attr:`SelectionStrategy.consumes`).  This ensures that
        a test-only change, for example, never causes downstream strategies to
        add entire maintainer-area test suites.

        Returns the number of error-status test suites encountered.
        """
        if not changed_files:
            log.info("No changed files – nothing to do.")
            self._write_dotplan(0, full=False)
            return 0

        accumulator = PlanAccumulator()
        remaining = list(changed_files)  # shrinks as strategies consume files
        all_handled = set()
        full_run = False

        for strategy in self._strategies:
            if not remaining:
                log.info(
                    "=== Strategy: %s === (skipped – no remaining files)",
                    strategy.name,
                )
                continue

            log.info("=== Strategy: %s ===", strategy.name)
            calls, handled = strategy.analyze(remaining)
            calls = self._batch_calls(calls)
            all_handled.update(handled)

            if strategy.consumes and handled:
                before = len(remaining)
                remaining = [f for f in remaining if f not in handled]
                log.info(
                    "  [%s] consumed %d file(s), %d remaining.",
                    strategy.name,
                    before - len(remaining),
                    len(remaining),
                )

            force_full_run = False
            for call in calls:
                if call.full_run:
                    log.warning(
                        "  Full-run signaled by '%s' – skipping targeted plan.",
                        call.description,
                    )
                    full_run = True
                    force_full_run = True
                    remaining = []
                    break
                log.info("  Executing call: %s", call.description)
                suites = self._executor.execute(call)
                added = accumulator.merge(suites)
                log.info(
                    "  → %d suites from twister, %d new (total so far: %d)",
                    len(suites),
                    added,
                    len(accumulator),
                )
            if force_full_run:
                break

        unresolved = set(remaining) - all_handled  # files no strategy handled at all
        if unresolved:
            log.warning(
                "Files not covered by any strategy: %s",
                ", ".join(sorted(unresolved)),
            )
            full_run = True

        errors = self._count_errors(accumulator.testsuites)

        total = len(accumulator)
        log.info("=== Summary: %d unique test suites selected ===", total)

        if total > 0:
            accumulator.write(output_file)
        else:
            log.info("No test suites selected – %s not written.", output_file)

        self._write_dotplan(total, full=full_run)
        return errors

    @staticmethod
    def _batch_calls(calls):
        """Merge compatible :class:`TwisterCall` objects to reduce subprocess count.

        Two calls can be merged when they share the same ``integration`` flag,
        ``platforms`` list, and ``extra_args``, **and** both use the same
        selection mode:

        * *roots-only* – ``testsuite_roots`` non-empty, ``test_patterns`` empty:
          all ``-T`` roots are combined into one call.
        * *patterns-only* – ``test_patterns`` non-empty, ``testsuite_roots`` empty:
          all ``--test-pattern`` args are combined into one call.
        * *mixed* – both set: not merged (unusual; kept as-is).

        The resulting call carries a consolidated description that lists all
        merged descriptions.
        """
        if len(calls) <= 1:
            return calls

        # Group by merge key
        groups: dict = {}
        order = []  # preserve insertion order
        for call in calls:
            has_roots = bool(call.testsuite_roots)
            has_patterns = bool(call.test_patterns)
            if has_roots and not has_patterns:
                mode = "roots"
            elif has_patterns and not has_roots:
                mode = "patterns"
            else:
                mode = "mixed"
            key = (
                call.integration,
                tuple(sorted(call.platforms)),
                tuple(call.extra_args),
                mode,
            )
            if key not in groups:
                groups[key] = []
                order.append(key)
            groups[key].append(call)

        batched = []
        for key in order:
            group = groups[key]
            if len(group) == 1 or key[3] == "mixed":
                batched.extend(group)
                continue

            integration, platforms, extra_args, mode = key
            merged_roots: list = []
            merged_patterns: list = []
            desc_parts: list = []
            for call in group:
                merged_roots.extend(r for r in call.testsuite_roots if r not in merged_roots)
                merged_patterns.extend(p for p in call.test_patterns if p not in merged_patterns)
                desc_parts.append(call.description)

            if len(group) > 1:
                log.debug(
                    "  Batched %d calls into 1 (%s mode): %s",
                    len(group),
                    mode,
                    ", ".join(desc_parts),
                )

            batched.append(
                TwisterCall(
                    description=desc_parts[0]
                    + (f" [+{len(group) - 1} more]" if len(group) > 1 else ""),
                    test_patterns=merged_patterns,
                    testsuite_roots=merged_roots,
                    platforms=list(platforms),
                    integration=integration,
                    extra_args=list(extra_args),
                )
            )

        return batched

    @staticmethod
    def _count_errors(testsuites):
        errors = 0
        try:
            from pylib.twister.twisterlib.statuses import TwisterStatus  # noqa: PLC0415
        except ImportError:
            return 0
        for ts in testsuites:
            if TwisterStatus(ts.get("status")) == TwisterStatus.ERROR:
                log.warning(
                    "Error: %s on %s – %s",
                    ts.get("name"),
                    ts.get("platform"),
                    ts.get("reason"),
                )
                errors += 1
        return errors

    def _write_dotplan(self, total, full):
        if not total:
            nodes = 0
        elif total < self._tests_per_builder:
            nodes = 1
        else:
            import math  # noqa: PLC0415

            nodes = math.ceil(total / self._tests_per_builder)
        with open(".testplan", "w", encoding="utf-8") as fh:
            fh.write(f"TWISTER_TESTS={total}\n")
            fh.write(f"TWISTER_NODES={nodes}\n")
            fh.write(f"TWISTER_FULL={full}\n")
        log.info(".testplan written (tests=%d, nodes=%d, full=%s)", total, nodes, full)


# ---------------------------------------------------------------------------
# Strategy 2 – Snippet changes → tests that require the snippet
# ---------------------------------------------------------------------------


class SnippetStrategy(SelectionStrategy):
    """For changed files under ``snippets/``, run tests that declare that
    snippet as a ``required_snippets`` dependency.

    Resolution chain
    ----------------
    1. For each changed file under ``snippets/``, walk up the directory tree
       (stopping at ``snippets/``) to find the nearest ``snippet.yml``.

    2. Read the ``name:`` field from ``snippet.yml`` to get the canonical
       snippet identifier (e.g. ``nordic-log-stm``).  Snippets without a
       ``snippet.yml`` ancestor (vendor group directories) are skipped.

    3. Grep ``tests/`` and ``samples/`` for ``testcase.yaml`` and
       ``sample.yaml`` files that contain the snippet name string, then
       parse each found manifest to confirm the snippet name actually appears
       in a ``required_snippets:`` list of at least one test entry.

    4. Walk up from the confirmed YAML file to the test root (the directory
       that contains the ``testcase.yaml`` / ``sample.yaml``) and emit a
       ``-T`` root call so twister exercises those tests.

    This strategy **consumes** all matched snippet files so downstream
    strategies do not produce redundant Build-system-area runs for the same
    change.
    """

    consumes: bool = True

    @property
    def name(self):
        return "SnippetStrategy"

    def __init__(self, zephyr_base, platform_filter=None):
        self._zephyr_base = Path(zephyr_base)
        self._platform_filter = platform_filter or []

    # ------------------------------------------------------------------
    # SelectionStrategy interface
    # ------------------------------------------------------------------

    def analyze(self, changed_files):
        snippet_files = [f for f in changed_files if f.startswith("snippets/")]
        if not snippet_files:
            return [], set()

        # Map snippet_name → set of changed files that belong to it
        name_to_files: dict = {}
        for f in snippet_files:
            snippet_name = self._find_snippet_name(self._zephyr_base / f)
            if snippet_name is None:
                log.debug(
                    "[%s] '%s' has no ancestor snippet.yml – skipping.",
                    self.name,
                    f,
                )
                continue
            name_to_files.setdefault(snippet_name, set()).add(f)

        if not name_to_files:
            return [], set()

        handled = set()
        calls = []
        for snippet_name, files in sorted(name_to_files.items()):
            test_roots = self._find_test_roots_for_snippet(snippet_name)
            if not test_roots:
                log.info(
                    "[%s] snippet '%s': no tests with required_snippets found.",
                    self.name,
                    snippet_name,
                )
                continue
            handled.update(files)
            log.info(
                "[%s] snippet '%s': %d test root(s) → %s",
                self.name,
                snippet_name,
                len(test_roots),
                sorted(test_roots),
            )
            for root in sorted(test_roots):
                calls.append(
                    TwisterCall(
                        description=f"SnippetStrategy: {snippet_name} → {root}",
                        testsuite_roots=[root],
                        platforms=list(self._platform_filter),
                    )
                )

        return calls, handled

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _find_snippet_name(self, abs_path):
        """Walk up from *abs_path* to find the nearest ``snippet.yml``.

        Returns the ``name:`` value from that file, or ``None`` if not found.
        Stops searching when it reaches the ``snippets/`` root.
        """
        snippets_root = self._zephyr_base / "snippets"
        current = abs_path if abs_path.is_dir() else abs_path.parent
        for _ in range(6):
            candidate = current / "snippet.yml"
            if candidate.exists():
                try:
                    data = yaml.safe_load(candidate.read_text(encoding="utf-8"))
                    if isinstance(data, dict) and "name" in data:
                        return str(data["name"])
                except Exception:  # noqa: BLE001
                    pass
                return None  # snippet.yml found but no name field
            if current in (snippets_root, self._zephyr_base):
                break
            current = current.parent
        return None

    def _find_test_roots_for_snippet(self, snippet_name):
        """Return the set of test-root directories that require *snippet_name*.

        Grepping for the snippet name string in ``testcase.yaml`` /
        ``sample.yaml`` files is fast.  Each hit is then parsed to confirm
        the name actually appears in a ``required_snippets:`` list.
        The confirmed YAML's directory is the ``-T`` root.
        """
        test_roots: set = set()
        search_roots = [
            self._zephyr_base / "tests",
            self._zephyr_base / "samples",
        ]
        for root in search_roots:
            if not root.is_dir():
                continue
            for manifest in root.rglob("testcase.yaml"):
                self._check_manifest(manifest, snippet_name, test_roots)
            for manifest in root.rglob("sample.yaml"):
                self._check_manifest(manifest, snippet_name, test_roots)
        return test_roots

    def _check_manifest(self, manifest_path, snippet_name, acc):
        """Parse *manifest_path* and add its directory to *acc* if any test
        entry lists *snippet_name* under ``required_snippets``."""
        try:
            content = manifest_path.read_text(encoding="utf-8", errors="replace")
            # Fast pre-filter: skip files that don't even mention the name
            if snippet_name not in content:
                return
            data = yaml.safe_load(content)
        except Exception:  # noqa: BLE001
            return
        if not isinstance(data, dict):
            return
        # Check top-level common section
        common = data.get("common", {}) or {}
        if snippet_name in (common.get("required_snippets") or []):
            rel = os.path.relpath(manifest_path.parent, self._zephyr_base)
            acc.add(rel)
            return
        # Check per-test entries
        for section_key in ("tests", "samples"):
            for test_data in (data.get(section_key) or {}).values():
                if not isinstance(test_data, dict):
                    continue
                if snippet_name in (test_data.get("required_snippets") or []):
                    rel = os.path.relpath(manifest_path.parent, self._zephyr_base)
                    acc.add(rel)
                    return


# ---------------------------------------------------------------------------
# Strategy 3 – Board directory changes → integration kernel test
# ---------------------------------------------------------------------------


class BoardStrategy(SelectionStrategy):
    """For changed files under ``boards/``, run a basic integration test on
    every affected board variant.

    Resolution chain
    ----------------
    1. For each changed file under ``boards/``, walk up the directory tree
       until a ``board.yml`` is found (stopping at ``boards/``).  Files
       inside ``boards/shields/`` or other areas without ``board.yml`` are
       silently ignored.

    2. De-duplicate: multiple changed files in the same board directory are
       mapped to a single board.

    3. For each board directory, use :func:`list_boards.find_v2_boards` and
       :func:`list_boards.board_v2_qualifiers` (from ``scripts/list_boards.py``)
       to enumerate all twister-compatible identifiers, e.g.::

           nrf52840dk/nrf52840
           nrf52840dk/nrf52811

    4. Emit one :class:`TwisterCall` per affected board, running
       ``tests/integration/kernel`` restricted to those board targets
       (``-p`` flags).  This verifies that every variant of the board still
       builds and boots correctly with a minimal kernel.

    This strategy **consumes** all matched board files so downstream
    strategies (DriverCompat, KconfigImpact, MaintainerArea …) do not
    produce redundant or misleading runs for the same change.
    """

    consumes: bool = True  # board files must not trigger area-level sweeps

    # Generic test root used to verify board buildability
    _INTEGRATION_TEST_ROOT: str = "tests/integration/kernel"

    # Safety cap: if a board has more variants than this, truncate the list.
    # Prevents an explosion for boards with many CPU/SoC/cluster combinations.
    _MAX_TARGETS_PER_BOARD: int = 20

    @property
    def name(self):
        return "BoardStrategy"

    def __init__(self, zephyr_base, platform_filter=None):
        self._zephyr_base = Path(zephyr_base)
        self._platform_filter = platform_filter or []

    # ------------------------------------------------------------------
    # SelectionStrategy interface
    # ------------------------------------------------------------------

    def analyze(self, changed_files):
        board_dirs = self._find_board_dirs(changed_files)

        if not board_dirs:
            return [], set()

        handled = set()
        calls = []

        for board_dir in sorted(board_dirs):
            files = board_dirs[board_dir]
            targets = self._get_board_targets(board_dir)
            if not targets:
                log.info(
                    "[%s] No targets found for '%s' – skipping.",
                    self.name,
                    board_dir,
                )
                continue

            handled.update(files)
            log.info(
                "[%s] Board '%s': %d target(s) → %s",
                self.name,
                board_dir.name,
                len(targets),
                targets,
            )
            calls.append(
                TwisterCall(
                    description=f"BoardStrategy: {board_dir.name}",
                    testsuite_roots=[self._INTEGRATION_TEST_ROOT],
                    platforms=targets,
                )
            )

        return calls, handled

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _find_board_dirs(self, changed_files):
        """Return a mapping ``{board_dir_path: set_of_changed_files}``.

        Only files whose ancestor directory (within ``boards/``) contains a
        ``board.yml`` are included.  Files under ``boards/shields/`` or other
        sub-trees that lack a ``board.yml`` are silently excluded.
        """
        board_dirs: dict = {}
        for f in changed_files:
            if not f.startswith("boards/"):
                continue
            board_dir = self._find_board_yml_dir(self._zephyr_base / f)
            if board_dir is None:
                log.debug("[%s] '%s' has no ancestor board.yml – skipping.", self.name, f)
                continue
            board_dirs.setdefault(board_dir, set()).add(f)
        return board_dirs

    def _find_board_yml_dir(self, abs_path):
        """Walk up *abs_path* until ``board.yml`` is found.

        Returns the directory containing ``board.yml``, or ``None`` if no
        such file exists between *abs_path* and the ``boards/`` root.
        """
        current = abs_path.parent
        boards_root = self._zephyr_base / "boards"
        for _ in range(8):
            if (current / "board.yml").exists():
                return current
            if current in (boards_root, self._zephyr_base):
                break
            current = current.parent
        return None

    def _find_twister_yaml(self, board_dir):
        """Walk up from *board_dir* to find ``twister.yaml``.

        Returns the path to the first ``twister.yaml`` found, or ``None``.
        Stops at the ``boards/`` root to avoid picking up unrelated files.
        """
        boards_root = self._zephyr_base / "boards"
        current = board_dir
        for _ in range(8):
            candidate = current / "twister.yaml"
            if candidate.exists():
                return candidate
            if current in (boards_root, self._zephyr_base):
                break
            current = current.parent
        return None

    def _get_board_targets(self, board_dir):
        """Return twister-compatible board identifiers for *board_dir*.

        Two sources are consulted:

        1. Per-variant ``<board_name>*.yaml`` files in *board_dir*, each
           containing an ``identifier:`` key (the legacy approach used by
           most boards and mirrored from ``scripts/ci/test_plan.py``).

        2. ``twister.yaml`` – a single file that consolidates all variants for
           a board (used by e.g. ``intel_adsp``, ``mediatek``).  The file may
           live in *board_dir* itself or in a parent directory.  Identifiers
           are the keys of the ``variants:`` mapping, filtered to those that
           start with ``<board_name>/``.

        Returns at most ``_MAX_TARGETS_PER_BOARD`` identifiers, sorted for
        determinism.  Returns an empty list on any error so the caller can
        skip gracefully.
        """
        scripts_dir = str(self._zephyr_base / "scripts")
        if scripts_dir not in sys.path:
            sys.path.insert(0, scripts_dir)

        try:
            import glob as _glob  # noqa: PLC0415

            import list_boards  # noqa: PLC0415  (local late import)

            args = argparse.Namespace(
                arch_roots=[self._zephyr_base],
                board_roots=[self._zephyr_base],
                soc_roots=[self._zephyr_base],
                board=None,
                board_dir=[board_dir],
                fuzzy_match=None,
                cmakeformat=None,
            )
            boards = list_boards.find_v2_boards(args)

            # Collect identifiers from <board_name>*.yaml files exactly as
            # test_plan.py does: open each, read b['identifier'].
            targets: list = []
            for bname in boards:
                for yaml_file in sorted(_glob.glob(os.path.join(str(board_dir), f"{bname}*.yaml"))):
                    try:
                        with open(yaml_file, encoding="utf-8") as fh:
                            data = yaml.load(fh, Loader=SafeLoader)
                        if isinstance(data, dict) and "identifier" in data:
                            identifier = data["identifier"]
                            if identifier not in targets:
                                targets.append(identifier)
                    except Exception:  # noqa: BLE001
                        pass
                    if len(targets) >= self._MAX_TARGETS_PER_BOARD:
                        break
                if len(targets) >= self._MAX_TARGETS_PER_BOARD:
                    break

            # Also collect identifiers from twister.yaml (a single file that
            # consolidates all variants for a board, used by e.g. intel_adsp).
            # twister.yaml may live in board_dir itself or in a parent directory.
            twister_yaml_path = self._find_twister_yaml(board_dir)
            if twister_yaml_path:
                try:
                    with open(twister_yaml_path, encoding="utf-8") as fh:
                        ty_data = yaml.load(fh, Loader=SafeLoader)
                    variants_map = ty_data.get("variants") or {}
                    for bname in boards:
                        prefix = f"{bname}/"
                        for variant_key, variant_data in variants_map.items():
                            if not (variant_key == bname or variant_key.startswith(prefix)):
                                continue
                            # Skip variants explicitly disabled for twister
                            if (
                                isinstance(variant_data, dict)
                                and variant_data.get("twister") is False
                            ):
                                log.debug(
                                    "[%s] Skipping '%s': twister: false in %s.",
                                    self.name,
                                    variant_key,
                                    twister_yaml_path,
                                )
                                continue
                            if variant_key not in targets:
                                targets.append(variant_key)
                            if len(targets) >= self._MAX_TARGETS_PER_BOARD:
                                break
                        if len(targets) >= self._MAX_TARGETS_PER_BOARD:
                            break
                    log.debug(
                        "[%s] Found %d identifier(s) via twister.yaml '%s'.",
                        self.name,
                        len(targets),
                        twister_yaml_path,
                    )
                except Exception:  # noqa: BLE001
                    pass

            if not targets:
                log.debug(
                    "[%s] No .yaml board files with 'identifier' found in '%s'.",
                    self.name,
                    board_dir,
                )

            return sorted(targets)[: self._MAX_TARGETS_PER_BOARD]

        except Exception as exc:  # noqa: BLE001
            log.warning(
                "[%s] Could not enumerate targets for '%s': %s",
                self.name,
                board_dir,
                exc,
            )
            return []


# ---------------------------------------------------------------------------
# Strategy 4 – SoC directory changes → integration kernel test on boards
# ---------------------------------------------------------------------------


class SoCStrategy(SelectionStrategy):
    """For changed files under ``soc/``, run a basic integration test on
    every board that uses an affected SoC.

    Resolution chain
    ----------------
    1. For each changed file under ``soc/``, walk up the directory tree
       (stopping at ``soc/``) to find the nearest ``soc.yml``.

    2. Determine the *group prefix*: the first path component of the changed
       file's directory relative to the ``soc.yml`` directory.  For example,
       a change to ``soc/nordic/nrf52/soc.c`` finds ``soc/nordic/soc.yml``
       and derives the prefix ``nrf52``.

    3. Parse ``soc.yml`` recursively (``family`` → ``series`` → ``socs``)
       to collect SoC names.  Filter to those that start with the group
       prefix.  This resolves ``nrf52`` → ``{nrf52805, nrf52810, …, nrf52840}``.

    4. Scan all ``board.yml`` files under ``boards/``.  For each board whose
       ``socs:`` list intersects the resolved SoC names, collect twister
       ``-p`` identifiers from the board's ``<name>*.yaml`` files (same
       logic as :class:`BoardStrategy`).

    5. Emit one :class:`TwisterCall` per affected SoC group, running
       ``tests/integration/kernel`` restricted to the discovered board
       targets.

    This strategy **consumes** all matched SoC files so downstream strategies
    do not produce maintainer-area runs for the same change (those areas lack
    ``tests:`` entries for ``soc/`` anyway).
    """

    consumes: bool = True

    _INTEGRATION_TEST_ROOT: str = "tests/integration/kernel"
    _MAX_TARGETS_PER_SOC: int = 30

    @property
    def name(self):
        return "SoCStrategy"

    def __init__(self, zephyr_base, platform_filter=None):
        self._zephyr_base = Path(zephyr_base)
        self._platform_filter = platform_filter or []

    # ------------------------------------------------------------------
    # SelectionStrategy interface
    # ------------------------------------------------------------------

    def analyze(self, changed_files):
        soc_files = [f for f in changed_files if f.startswith("soc/")]
        if not soc_files:
            return [], set()

        # Group changed files by (soc_yml_path, group_prefix)
        group_to_files: dict = {}
        for f in soc_files:
            abs_path = self._zephyr_base / f
            soc_yml = self._find_soc_yml(abs_path)
            if soc_yml is None:
                log.debug("[%s] '%s' has no ancestor soc.yml – skipping.", self.name, f)
                continue
            soc_yml_dir = soc_yml.parent
            try:
                rel = abs_path.parent.relative_to(soc_yml_dir)
                prefix = rel.parts[0] if rel.parts else None
            except ValueError:
                prefix = None
            key = (soc_yml, prefix)
            group_to_files.setdefault(key, set()).add(f)

        if not group_to_files:
            return [], set()

        handled = set()
        calls = []
        for (soc_yml, prefix), files in sorted(
            group_to_files.items(), key=lambda kv: (str(kv[0][0]), kv[0][1] or "")
        ):
            soc_names = self._collect_soc_names(soc_yml, group_prefix=prefix)
            if not soc_names:
                log.debug(
                    "[%s] No SoC names found for prefix '%s' in '%s' – skipping.",
                    self.name,
                    prefix,
                    soc_yml,
                )
                continue

            targets = self._find_board_targets_for_socs(soc_names)
            if not targets:
                log.info(
                    "[%s] SoC group '%s': no board targets found – skipping.",
                    self.name,
                    prefix or soc_yml.parent.name,
                )
                continue

            handled.update(files)
            label = prefix or soc_yml.parent.name
            log.info(
                "[%s] SoC group '%s' (%d SoC(s)) → %d board target(s)",
                self.name,
                label,
                len(soc_names),
                len(targets),
            )
            calls.append(
                TwisterCall(
                    description=f"SoCStrategy: {label}",
                    testsuite_roots=[self._INTEGRATION_TEST_ROOT],
                    platforms=targets,
                )
            )

        return calls, handled

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _find_soc_yml(self, abs_path):
        """Walk up from *abs_path* until ``soc.yml`` is found.

        Returns the path to ``soc.yml``, or ``None`` if not found before
        reaching the ``soc/`` root.
        """
        soc_root = self._zephyr_base / "soc"
        current = abs_path.parent
        for _ in range(6):
            if (current / "soc.yml").exists():
                return current / "soc.yml"
            if current in (soc_root, self._zephyr_base):
                break
            current = current.parent
        return None

    def _collect_soc_names(self, soc_yml_path, group_prefix=None):
        """Return all SoC names declared in *soc_yml_path*.

        If *group_prefix* is given, only names that start with that prefix are
        returned.  The prefix corresponds to the sub-directory name that
        contains the changed file (e.g. ``nrf52`` for ``soc/nordic/nrf52/``).
        """
        try:
            data = yaml.safe_load(soc_yml_path.read_text(encoding="utf-8"))
        except Exception:  # noqa: BLE001
            return []

        all_names: list = []
        self._collect_soc_names_recursive(data, all_names)

        if group_prefix:
            all_names = [s for s in all_names if s.startswith(group_prefix)]
        return all_names

    def _collect_soc_names_recursive(self, node, acc):
        """Recursively walk *node* and append every ``socs[].name`` to *acc*."""
        if isinstance(node, list):
            for item in node:
                self._collect_soc_names_recursive(item, acc)
        elif isinstance(node, dict):
            if "socs" in node:
                for soc in node["socs"]:
                    if isinstance(soc, dict) and "name" in soc:
                        acc.append(soc["name"])
            for k, v in node.items():
                if k in ("family", "series"):
                    self._collect_soc_names_recursive(v, acc)

    def _find_board_targets_for_socs(self, soc_names):
        """Return twister ``-p`` identifiers for boards that use *soc_names*.

        Scans all ``board.yml`` files under ``boards/``, checks whether the
        board's ``socs:`` list intersects *soc_names*, and collects
        ``identifier`` values from the matching ``<board_name>*.yaml`` files.

        Returns at most ``_MAX_TARGETS_PER_SOC`` identifiers, sorted for
        determinism.
        """
        import glob as _glob  # noqa: PLC0415

        soc_name_set = set(soc_names)
        boards_root = self._zephyr_base / "boards"
        targets: list = []
        seen_dirs: set = set()

        for board_yml in sorted(boards_root.rglob("board.yml")):
            try:
                data = yaml.safe_load(board_yml.read_text(encoding="utf-8"))
            except Exception:  # noqa: BLE001
                continue

            board_socs = {s.get("name", "") for s in data.get("board", {}).get("socs", [])}
            if not board_socs.intersection(soc_name_set):
                continue

            board_dir = board_yml.parent
            if board_dir in seen_dirs:
                continue
            seen_dirs.add(board_dir)

            board_name = data.get("board", {}).get("name", board_dir.name)
            for yaml_file in sorted(
                _glob.glob(os.path.join(str(board_dir), f"{board_name}*.yaml"))
            ):
                try:
                    with open(yaml_file, encoding="utf-8") as fh:
                        yd = yaml.load(fh, Loader=SafeLoader)
                    if isinstance(yd, dict) and "identifier" in yd:
                        ident = yd["identifier"]
                        if ident not in targets:
                            targets.append(ident)
                except Exception:  # noqa: BLE001
                    pass
                if len(targets) >= self._MAX_TARGETS_PER_SOC:
                    break
            if len(targets) >= self._MAX_TARGETS_PER_SOC:
                break

        return sorted(targets)[: self._MAX_TARGETS_PER_SOC]


# ---------------------------------------------------------------------------
# Strategy 5 – Driver DT_DRV_COMPAT → overlay → test directory
# ---------------------------------------------------------------------------


class DriverCompatStrategy(SelectionStrategy):
    """For changed driver sources, find tests that build/run the driver.

    Resolution chain
    ----------------
    1. For each changed file under ``drivers/`` (C or header):

       a. Collect the set of source files in that driver's directory
          (the driver may be split across multiple ``.c`` / ``.h`` files).
       b. Grep them for ``#define DT_DRV_COMPAT <value>``.

    2. Convert each compat macro value to a DTS compatible string::

           adi_max14906_gpio  →  first underscore → comma,
                                 remaining underscores → hyphens
                              →  adi,max14906-gpio

    3. Search ``tests/`` and ``samples/`` for ``.overlay``, ``.dts`` and
       ``.dtsi`` files that reference that compat string.  Generated build
       artifacts (any path containing ``twister-out``) are skipped.

    4. From each found overlay/DTS, walk up the directory tree until a
       ``testcase.yaml`` or ``sample.yaml`` is found.  That directory
       becomes a ``-T`` root.

    5. Emit one :class:`TwisterCall` per discovered test directory (no
       ``--test-pattern`` filter — all tests in that directory are relevant
       because they share the overlay that enables the driver).

    6. **Board-targeted path** (when *maintainers_file* is supplied):

       a. Search ``dts/`` for ``.dtsi`` files (excluding ``dts/bindings/``)
          that define a hardware node whose ``compatible`` matches the compat
          string.  These are the SoC/board peripheral descriptor files.
       b. For each such ``.dtsi``, grep ``boards/`` for ``.dts`` and
          ``.dtsi`` files that ``#include`` it.  Extract the board
          identifier from the ``.yaml`` file in the same directory.
       c. Look up the MAINTAINERS area for the changed driver file and
          collect its ``tests:`` entries.  Emit a :class:`TwisterCall`
          restricted to those boards (``-p`` flag) carrying the area
          ``--test-pattern`` arguments.

       This ensures that e.g. a GPIO driver change also runs the generic
       GPIO test suite on every board that actually ships the peripheral,
       without requiring a DTS overlay in the test directory.

       At most ``_MAX_BOARDS`` board identifiers are used per compat to
       prevent an explosion when a widely-used peripheral is changed.
    """

    # C/H extensions worth scanning for DT_DRV_COMPAT
    _SOURCE_EXTS: frozenset = frozenset({".c", ".h"})

    # Regex to extract the compat value from a #define line
    _COMPAT_RE = re.compile(r"^\s*#define\s+DT_DRV_COMPAT\s+(\w+)", re.MULTILINE)

    # Path fragments that indicate a generated build artifact
    _ARTIFACT_FRAGMENTS: tuple = ("twister-out",)

    # Vendor prefixes that indicate test/mock devices – skip them
    _MOCK_VENDORS: frozenset = frozenset({"vnd", "test", "zephyr"})

    # Maximum number of board identifiers to collect per compat string.
    # Prevents an explosion when a widely-used peripheral (e.g. nrf-gpio)
    # is present on hundreds of boards.
    _MAX_BOARDS: int = 10

    @property
    def name(self):
        return "DriverCompat"

    def __init__(self, zephyr_base, platform_filter=None, maintainers_file=None):
        self._zephyr_base = Path(zephyr_base)
        self._platform_filter = platform_filter or []
        self._maintainers_file = maintainers_file

    # ------------------------------------------------------------------
    # SelectionStrategy interface
    # ------------------------------------------------------------------

    def analyze(self, changed_files):
        driver_files = [
            f
            for f in changed_files
            if f.startswith("drivers/") and Path(f).suffix in self._SOURCE_EXTS
        ]

        if not driver_files:
            return [], set()

        # Collect DTS compat strings for all changed driver directories
        compat_to_driver_files = self._extract_compats(driver_files)

        if not compat_to_driver_files:
            log.info("[%s] No DT_DRV_COMPAT found in changed driver files.", self.name)
            return [], set()

        # Map each compat to the test dirs that exercise it, deduplicating
        # both the per-compat dir set and the global set of dirs-to-call-twister.
        handled = set()
        # test_dir → set of compat strings that map to it (for log clarity)
        dir_to_compats: dict = {}

        for compat_str, src_files in compat_to_driver_files.items():
            # Skip mock / test-only compat strings (vnd,gpio  zephyr,… etc.)
            vendor = compat_str.split(",")[0]
            if vendor in self._MOCK_VENDORS:
                log.debug("[%s] Skipping mock compat '%s'.", self.name, compat_str)
                continue

            test_dirs = self._find_test_dirs_for_compat(compat_str)
            if not test_dirs:
                log.info(
                    "[%s] compat '%s': no test overlays found.",
                    self.name,
                    compat_str,
                )
                continue

            handled.update(src_files)
            for test_dir in test_dirs:
                rel_dir = os.path.relpath(test_dir, self._zephyr_base)
                dir_to_compats.setdefault(rel_dir, set()).add(compat_str)

        # Emit one TwisterCall per unique test directory
        calls = []
        for rel_dir, compats in sorted(dir_to_compats.items()):
            log.info(
                "[%s] test dir '%s' ← compats: %s",
                self.name,
                rel_dir,
                ", ".join(sorted(compats)),
            )
            calls.append(
                TwisterCall(
                    description=f"DriverCompat → {rel_dir}",
                    testsuite_roots=[rel_dir],
                    platforms=list(self._platform_filter),
                )
            )

        # Board-targeted path: for each compat, discover real hardware boards
        # via the dts/ dtsi chain and emit area-pattern calls restricted to
        # those platforms.
        if self._maintainers_file:
            areas = self._load_areas()
            for compat_str, src_files in sorted(compat_to_driver_files.items()):
                vendor = compat_str.split(",")[0]
                if vendor in self._MOCK_VENDORS:
                    continue

                boards = self._find_boards_for_compat(compat_str)
                if not boards:
                    log.debug(
                        "[%s] compat '%s': no dtsi-backed boards found.",
                        self.name,
                        compat_str,
                    )
                    continue

                patterns = self._area_patterns_for_driver_files(list(src_files), areas)
                if not patterns:
                    log.info(
                        "[%s] compat '%s': boards found but no MAINTAINERS area"
                        " with tests for the driver files.",
                        self.name,
                        compat_str,
                    )
                    continue

                handled.update(src_files)
                log.info(
                    "[%s] compat '%s': boards %s → patterns %s",
                    self.name,
                    compat_str,
                    boards,
                    [p for p in patterns],
                )
                calls.append(
                    TwisterCall(
                        description=f"DriverCompat/Board: {compat_str}",
                        test_patterns=patterns,
                        platforms=boards,
                    )
                )

        return calls, handled

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _load_areas(self):
        """Load MAINTAINERS.yml areas (same logic as MaintainerAreaStrategy)."""
        with open(self._maintainers_file, encoding="utf-8") as fh:
            data = yaml.load(fh, Loader=SafeLoader)
        return data if isinstance(data, dict) else {}

    def _area_patterns_for_driver_files(self, driver_files, areas):
        """Return ``--test-pattern`` strings from MAINTAINERS areas that cover
        *driver_files*.

        Parameters
        ----------
        driver_files:
            Relative paths of the changed driver source files.
        areas:
            Dict of area_name → area_data loaded from MAINTAINERS.yml.

        Returns
        -------
        Sorted list of unique ``--test-pattern`` regex strings derived from
        the ``tests:`` entries of all matching areas.
        """
        patterns = []
        seen: set = set()
        for area_data in areas.values():
            if not area_data.get("tests"):
                continue
            if any(_matches_area(f, area_data) for f in driver_files):
                for t in area_data["tests"]:
                    pat = _test_pattern(t)
                    if pat not in seen:
                        seen.add(pat)
                        patterns.append(pat)
        return sorted(patterns)

    def _find_boards_for_compat(self, compat_str):
        """Return board identifiers whose DTS tree contains *compat_str*.

        Resolution chain
        ----------------
        1. ``grep -rl`` inside ``dts/`` (excluding ``dts/bindings/``) for
           ``.dtsi`` files that contain ``compatible = "<compat_str>"``.
           These are the SoC/board peripheral descriptor files shipped with
           Zephyr.
        2. For each found ``.dtsi``, ``grep -rEl`` inside ``boards/`` for
           ``.dts`` / ``.dtsi`` files that ``#include`` it.
        3. Walk to the directory of each found board file and read every
           ``.yaml`` in that directory; keep entries that have an
           ``identifier`` key (the twister ``-p`` value).
        4. Return at most ``_MAX_BOARDS`` identifiers, sorted for
           determinism.

        Returns an empty list when the compat does not correspond to an
        on-SoC peripheral (e.g. external components like ``adi,max14906-gpio``
        that are instantiated only in board overlays, not in vendor dtsi files).
        """
        # Step 1 – find vendor dtsi files that define the peripheral
        dts_root = self._zephyr_base / "dts"
        compat_literal = f'compatible = "{compat_str}"'
        try:
            result = subprocess.run(
                ["grep", "-rl", "-F", compat_literal, str(dts_root)],
                capture_output=True,
                text=True,
                timeout=30,
            )
        except (subprocess.TimeoutExpired, OSError):
            return []

        dtsi_files = [
            ln
            for ln in result.stdout.splitlines()
            if ln.endswith(".dtsi")
            and (os.sep + "bindings" + os.sep) not in ln
            and "/bindings/" not in ln
        ]
        if not dtsi_files:
            return []

        # Step 2 – find board files that #include any of those dtsi files
        boards_root = self._zephyr_base / "boards"
        board_yaml_dirs: set = set()

        for dtsi_file in dtsi_files:
            dtsi_name = Path(dtsi_file).name  # e.g. nrf54l_05_10_15.dtsi
            # Match: #include <nordic/nrf54l_05_10_15.dtsi>
            #     or #include "nrf54l_05_10_15.dtsi"
            inc_pattern = r'#\s*include\s*[<"](.*/)?' + re.escape(dtsi_name) + r'[>"]'
            try:
                result2 = subprocess.run(
                    [
                        "grep",
                        "-rl",
                        "-E",
                        inc_pattern,
                        "--include=*.dts",
                        "--include=*.dtsi",
                        str(boards_root),
                    ],
                    capture_output=True,
                    text=True,
                    timeout=30,
                )
            except (subprocess.TimeoutExpired, OSError):
                continue

            for board_file in result2.stdout.splitlines():
                board_yaml_dirs.add(Path(board_file).parent)

        if not board_yaml_dirs:
            return []

        # Step 3 – extract board identifiers from .yaml files
        board_ids: set = set()
        for board_dir in sorted(board_yaml_dirs):
            for yaml_file in sorted(board_dir.glob("*.yaml")):
                try:
                    data = yaml.safe_load(yaml_file.read_text(encoding="utf-8"))
                    if isinstance(data, dict) and "identifier" in data:
                        board_ids.add(data["identifier"])
                except Exception:  # noqa: BLE001
                    pass
                if len(board_ids) >= self._MAX_BOARDS:
                    break
            if len(board_ids) >= self._MAX_BOARDS:
                break

        return sorted(board_ids)[: self._MAX_BOARDS]

    def _extract_compats(self, driver_files):
        """Return a mapping of DTS compat string → set of source files.

        Scoping rules
        -------------
        * Always scan the changed file itself.
        * If the changed file lives in a *dedicated* driver subdirectory
          (determined by the directory containing few source files, i.e.
          the directory is named after the driver rather than being a flat
          driver-class directory such as ``drivers/gpio/``), also scan every
          sibling source file in that directory.  This handles multi-file
          drivers where ``DT_DRV_COMPAT`` is defined in a shared header.
        * Flat class directories (``drivers/gpio/``, ``drivers/sensor/st/``,
          …) contain many drivers; in those cases only the changed file is
          scanned to avoid pulling in unrelated compat strings.

        The threshold is ``_DEDICATED_DIR_MAX_SOURCES``.
        """
        compat_map = {}  # compat_str → set of changed files that triggered it

        for rel_path in driver_files:
            abs_path = self._zephyr_base / rel_path
            driver_dir = abs_path.parent

            # Decide which files to scan
            candidates = self._driver_scan_candidates(abs_path, driver_dir)

            for src_file in candidates:
                try:
                    text = src_file.read_text(encoding="utf-8", errors="replace")
                except OSError:
                    continue
                for match in self._COMPAT_RE.finditer(text):
                    macro_val = match.group(1)
                    compat_str = self._macro_to_compat(macro_val)
                    if compat_str not in compat_map:
                        compat_map[compat_str] = set()
                    compat_map[compat_str].add(rel_path)

        return compat_map

    # Files in a directory at or below this count → treat as a dedicated
    # per-driver directory and scan all sibling sources.
    _DEDICATED_DIR_MAX_SOURCES = 10

    def _driver_scan_candidates(self, changed_abs, driver_dir):
        """Return the list of source files to scan for DT_DRV_COMPAT.

        If *driver_dir* is a dedicated per-driver directory (few sources),
        return all source files in that directory.  Otherwise return only
        the changed file.
        """
        try:
            all_sources = [
                f for f in driver_dir.iterdir() if f.suffix in self._SOURCE_EXTS and f.is_file()
            ]
        except OSError:
            return [changed_abs]

        if len(all_sources) <= self._DEDICATED_DIR_MAX_SOURCES:
            # Small directory → dedicated driver subdir; scan all siblings
            return all_sources

        # Large flat class directory (e.g. drivers/gpio/) → only the changed file
        return [changed_abs]

    @staticmethod
    def _macro_to_compat(macro_value):
        """Convert a ``DT_DRV_COMPAT`` macro value to a DTS compat string.

        The Zephyr DTS tooling encodes the compat string by replacing the
        vendor/device separator comma with ``_`` and all hyphens with ``_``.
        To reverse::

            adi_max14906_gpio
              first  _ → ,   →  adi,max14906_gpio
              rest   _ → -   →  adi,max14906-gpio
        """
        first_sep = macro_value.index("_")
        vendor = macro_value[:first_sep]
        device = macro_value[first_sep + 1 :].replace("_", "-")
        return f"{vendor},{device}"

    def _find_test_dirs_for_compat(self, compat_str):
        """Return the set of test/sample directory paths that use *compat_str*.

        Scans ``tests/`` and ``samples/`` for overlay and DTS files that
        reference the compat.  Skips generated build artifacts.  For each
        hit, walks up to find the enclosing ``testcase.yaml`` or
        ``sample.yaml``.
        """
        search_roots = [
            self._zephyr_base / "tests",
            self._zephyr_base / "samples",
        ]
        # Pattern to match the compat inside a DTS value string
        compat_pattern = re.compile(
            r'compatible\s*=\s*[^;]*"' + re.escape(compat_str) + r'"',
            re.DOTALL,
        )

        test_dirs = set()
        for root in search_roots:
            if not root.is_dir():
                continue
            for dts_file in root.rglob("*"):
                if dts_file.suffix not in (".overlay", ".dts", ".dtsi"):
                    continue
                # Skip build artifacts
                if any(frag in str(dts_file) for frag in self._ARTIFACT_FRAGMENTS):
                    continue
                try:
                    content = dts_file.read_text(encoding="utf-8", errors="replace")
                except OSError:
                    continue
                if compat_pattern.search(content):
                    yaml_dir = self._find_yaml_dir(dts_file.parent)
                    if yaml_dir:
                        test_dirs.add(yaml_dir)

        return test_dirs

    @staticmethod
    def _find_yaml_dir(start_dir):
        """Walk up *start_dir* until a ``testcase.yaml`` or ``sample.yaml`` is found.

        Returns the directory containing the yaml file, or ``None`` if not found.
        """
        current = Path(start_dir)
        for _ in range(6):  # cap at 6 levels to avoid runaway traversal
            if (current / "testcase.yaml").exists() or (current / "sample.yaml").exists():
                return str(current)
            parent = current.parent
            if parent == current:
                break
            current = parent
        return None


# ---------------------------------------------------------------------------
# Strategy 6 – DTS binding changes → compat-matched tests + board targets
# ---------------------------------------------------------------------------


class DtsBindingStrategy(DriverCompatStrategy):
    """For changed ``dts/bindings/**/*.yaml`` files, run tests that use the
    compatible string declared in the binding.

    This strategy reuses the compat-resolution infrastructure of
    :class:`DriverCompatStrategy`:

    * ``_find_test_dirs_for_compat()`` — scans ``tests/`` and ``samples/``
      for overlay/DTS files that instantiate the compat and walks up to
      the test root.
    * ``_find_boards_for_compat()`` — traces the ``dts/`` → ``boards/`` dtsi
      include chain to find board identifiers that ship the peripheral.
    * ``_area_patterns_for_driver_files()`` — looks up MAINTAINERS areas for
      the changed binding path and converts ``tests:`` entries to
      ``--test-pattern`` regexes.

    Resolution chain
    ----------------
    1. For each changed ``.yaml`` file under ``dts/bindings/``, read the
       top-level ``compatible:`` field.  Bindings without a ``compatible:``
       field (base / include fragments) are skipped.

    2. Feed the compat string to ``_find_test_dirs_for_compat()`` to collect
       ``-T`` test-root calls.

    3. If a *maintainers_file* is configured, additionally call
       ``_find_boards_for_compat()`` and emit a board-targeted area-pattern
       call (same as the board-targeted path in :class:`DriverCompatStrategy`).

    This strategy is **additive** (``consumes = False``) so that
    :class:`MaintainerAreaStrategy` still runs as a backstop for binding
    changes that cannot be resolved via compat.
    """

    # Inherit consumes = False from SelectionStrategy (DCS sets it False too)

    @property
    def name(self):
        return "DtsBinding"

    def analyze(self, changed_files):
        binding_files = [
            f for f in changed_files if f.startswith("dts/bindings/") and f.endswith(".yaml")
        ]
        if not binding_files:
            return [], set()

        # Map compat_str → set of binding file paths
        compat_to_files: dict = {}
        for f in binding_files:
            compat_str = self._read_binding_compat(self._zephyr_base / f)
            if compat_str is None:
                log.debug("[%s] '%s' has no top-level compatible – skipping.", self.name, f)
                continue
            vendor = compat_str.split(",")[0]
            if vendor in self._MOCK_VENDORS:
                log.debug("[%s] Skipping mock compat '%s'.", self.name, compat_str)
                continue
            compat_to_files.setdefault(compat_str, set()).add(f)

        if not compat_to_files:
            return [], set()

        handled = set()
        dir_to_compats: dict = {}

        for compat_str, src_files in compat_to_files.items():
            test_dirs = self._find_test_dirs_for_compat(compat_str)
            if not test_dirs:
                log.info(
                    "[%s] compat '%s': no test overlays found.",
                    self.name,
                    compat_str,
                )
                continue
            handled.update(src_files)
            for test_dir in test_dirs:
                rel_dir = os.path.relpath(test_dir, self._zephyr_base)
                dir_to_compats.setdefault(rel_dir, set()).add(compat_str)

        calls = []
        for rel_dir, compats in sorted(dir_to_compats.items()):
            log.info(
                "[%s] test dir '%s' ← compats: %s",
                self.name,
                rel_dir,
                ", ".join(sorted(compats)),
            )
            calls.append(
                TwisterCall(
                    description=f"DtsBinding → {rel_dir}",
                    testsuite_roots=[rel_dir],
                    platforms=list(self._platform_filter),
                )
            )

        # Board-targeted path (when maintainers_file is configured)
        if self._maintainers_file:
            areas = self._load_areas()
            for compat_str, src_files in sorted(compat_to_files.items()):
                vendor = compat_str.split(",")[0]
                if vendor in self._MOCK_VENDORS:
                    continue
                boards = self._find_boards_for_compat(compat_str)
                if not boards:
                    continue
                patterns = self._area_patterns_for_driver_files(list(src_files), areas)
                if not patterns:
                    continue
                handled.update(src_files)
                log.info(
                    "[%s] compat '%s': board-targeted call on %d board(s).",
                    self.name,
                    compat_str,
                    len(boards),
                )
                calls.append(
                    TwisterCall(
                        description=f"DtsBinding (boards): {compat_str}",
                        test_patterns=patterns,
                        platforms=boards,
                    )
                )

        return calls, handled

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    @staticmethod
    def _read_binding_compat(abs_path):
        """Return the ``compatible:`` value from a binding YAML, or ``None``.

        Only top-level ``compatible:`` fields are read; bindings that use
        ``include:`` without their own ``compatible:`` are base fragments and
        are skipped.
        """
        try:
            data = yaml.safe_load(abs_path.read_text(encoding="utf-8"))
            if isinstance(data, dict):
                compat = data.get("compatible")
                if isinstance(compat, str) and compat:
                    return compat
        except Exception:  # noqa: BLE001
            pass
        return None


# ---------------------------------------------------------------------------
# Strategy 3 – Header include impact
# ---------------------------------------------------------------------------


class HeaderImpactStrategy(SelectionStrategy):
    """For changed headers under ``include/zephyr/``, trace indirect impact.

    Resolution chain
    ----------------
    1. For each changed ``include/zephyr/**/*.h`` file, derive the canonical
       ``#include`` path by stripping the leading ``include/`` prefix
       (e.g. ``include/zephyr/drivers/gpio.h`` → ``zephyr/drivers/gpio.h``).

    2. Run ``grep -rl`` across the source tree to find all files that
       ``#include`` that path (both angle-bracket and quote styles).

    3. If the result set exceeds ``_MAX_INCLUDE_REFS`` the header is
       considered **widespread** (like ``kernel.h`` or ``device.h``) and is
       skipped with a warning.  This prevents a single core-header change
       from triggering a near-full test run.

    4. Map the found source files to MAINTAINERS areas that carry a
       ``tests:`` list, using the same ``_matches_area`` logic as
       :class:`MaintainerAreaStrategy`.

    5. Emit one :class:`TwisterCall` per unique area, carrying
       ``--test-pattern`` arguments built from the area's ``tests:`` list.
       Multiple headers that resolve to the same area are batched into one
       call (no redundant twister invocations).
    """

    # Source file extensions passed to grep --include=
    _GREP_INCLUDE_ARGS: tuple = ("--include=*.c", "--include=*.cpp", "--include=*.h")

    # Directories under ZEPHYR_BASE to grep for #include references.
    # Deliberately excludes build/, twister-out*, modules/ (third-party),
    # tests/, and samples/ – we want *source* files that consume the header.
    _SCAN_DIRS: tuple = (
        "arch",
        "drivers",
        "include",
        "kernel",
        "lib",
        "subsys",
    )

    # If a header is included in more files than this, treat it as widespread
    # and skip it to avoid an explosion of test coverage.
    _MAX_INCLUDE_REFS: int = 50

    # Path fragments that indicate a generated build artifact – exclude them
    # from the grep results.
    _ARTIFACT_FRAGMENTS: tuple = ("twister-out", "build/")

    @property
    def name(self):
        return "HeaderImpact"

    def __init__(self, maintainers_file, zephyr_base, platform_filter=None):
        self._maintainers_file = maintainers_file
        self._zephyr_base = Path(zephyr_base)
        self._platform_filter = platform_filter or []

    # ------------------------------------------------------------------
    # SelectionStrategy interface
    # ------------------------------------------------------------------

    def analyze(self, changed_files):
        headers = [f for f in changed_files if f.startswith("include/zephyr/") and f.endswith(".h")]

        if not headers:
            return [], set()

        areas = self._load_areas()
        # area_name → (area_data, set-of-headers-that-triggered-it)
        area_hits: dict = {}
        handled = set()

        for header in headers:
            # include/zephyr/drivers/gpio.h  →  zephyr/drivers/gpio.h
            include_path = header[len("include/") :]
            refs = self._grep_includes(include_path)

            if refs is None:
                log.warning("[%s] Could not scan refs for '%s'.", self.name, header)
                continue

            if len(refs) > self._MAX_INCLUDE_REFS:
                log.info(
                    "[%s] '%s' has %d refs (> %d) – widespread header, skipping.",
                    self.name,
                    header,
                    len(refs),
                    self._MAX_INCLUDE_REFS,
                )
                continue

            log.info("[%s] '%s': %d ref(s) found.", self.name, header, len(refs))

            matched_any = False
            for area_name, area_data in areas.items():
                if not area_data.get("tests"):
                    continue
                if any(_matches_area(ref, area_data) for ref in refs):
                    entry = area_hits.setdefault(area_name, (area_data, set()))
                    entry[1].add(header)
                    matched_any = True

            if matched_any:
                handled.add(header)

        calls = []
        for area_name, (area_data, triggering_headers) in sorted(area_hits.items()):
            patterns = [_test_pattern(t) for t in area_data["tests"]]
            log.info(
                "[%s] area '%s' ← headers: %s",
                self.name,
                area_name,
                ", ".join(sorted(triggering_headers)),
            )
            calls.append(
                TwisterCall(
                    description=f"HeaderImpact → {area_name}",
                    test_patterns=patterns,
                    platforms=list(self._platform_filter),
                )
            )

        return calls, handled

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _grep_includes(self, include_path):
        """Return workspace-relative paths of files that #include *include_path*.

        Returns ``None`` on error, or a (possibly empty) list on success.
        """
        # Match both <zephyr/...> and "zephyr/..." styles
        pattern = rf'#\s*include\s*[<"]{re.escape(include_path)}[>"]'

        scan_dirs = [
            str(self._zephyr_base / d) for d in self._SCAN_DIRS if (self._zephyr_base / d).is_dir()
        ]
        if not scan_dirs:
            return []

        cmd = ["grep", "-rl", "-E", pattern] + list(self._GREP_INCLUDE_ARGS) + scan_dirs

        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=30,
                cwd=str(self._zephyr_base),
            )  # noqa: S603
        except subprocess.TimeoutExpired:
            log.warning("[%s] grep timed out scanning for '%s'.", self.name, include_path)
            return None
        except OSError as exc:
            log.warning("[%s] grep failed: %s", self.name, exc)
            return None

        # grep returns 1 when no matches found – that is not an error
        if result.returncode not in (0, 1):
            log.warning("[%s] grep error: %s", self.name, result.stderr.strip())
            return None

        rel_files = []
        for line in result.stdout.splitlines():
            f = line.strip()
            if not f:
                continue
            try:
                rel = str(Path(f).relative_to(self._zephyr_base))
            except ValueError:
                rel = f
            if not any(frag in rel for frag in self._ARTIFACT_FRAGMENTS):
                rel_files.append(rel)

        return rel_files

    def _load_areas(self):
        with open(self._maintainers_file, encoding="utf-8") as fh:
            data = yaml.load(fh, Loader=SafeLoader)
        return data if isinstance(data, dict) else {}


# ---------------------------------------------------------------------------
# Strategy 3 – Kconfig symbol impact
# ---------------------------------------------------------------------------


class KconfigImpactStrategy(SelectionStrategy):
    """For changed Kconfig files or config fragments, find tests that enable the affected symbols.

    Resolution chain
    ----------------
    1. Accept changed files that are Kconfig definition files (``Kconfig``,
       ``Kconfig.*``), board/SoC defconfigs (``*_defconfig``), or ``.conf``
       fragments **outside** ``tests/`` and ``samples/`` (those are already
       handled by :class:`DirectTestStrategy`).

    2. Extract symbol names from each file:

       * Kconfig files: ``config SYMBOL`` / ``menuconfig SYMBOL`` lines.
       * ``*.conf`` / ``*_defconfig`` files: ``CONFIG_SYMBOL=…`` assignments.

    3. Run a single ``grep -rl`` across ``tests/`` and ``samples/`` for
       ``.conf`` and ``.yaml`` files (which carry ``extra_configs``,
       ``filter``, and ``prj.conf`` content) that reference any of the
       extracted symbols.

    4. For each matched file, determine which symbols it mentions and walk
       up to the nearest ``testcase.yaml`` / ``tests.yaml`` / ``sample.yaml``
       to find the test root.

    5. Apply a per-symbol threshold (``_MAX_SYMBOL_ROOTS``): if a symbol
       appears in more test roots than the threshold it is considered
       **widespread** (like ``CONFIG_ZTEST``) and is discarded.  A fixed
       skip-list (``_SKIP_SYMBOLS``) covers the most common universal
       symbols without paying the grep cost.

    6. Emit one :class:`TwisterCall` per surviving test root.  Only consume
       the input files if at least one targeted test root was found – this
       way, if all symbols are widespread the files fall through to
       :class:`MaintainerAreaStrategy` for broad coverage.
    """

    # Extract symbol names from Kconfig definition files
    _KCONFIG_SYMBOL_RE = re.compile(r'^(?:config|menuconfig)\s+(\w+)', re.MULTILINE)

    # Extract symbol names from .conf / _defconfig assignment files
    _CONF_SYMBOL_RE = re.compile(r'^CONFIG_(\w+)\s*=', re.MULTILINE)

    # YAML markers that identify a test/sample root directory
    _YAML_MARKERS: tuple = ("testcase.yaml", "tests.yaml", "sample.yaml")

    # Per-symbol threshold: if a symbol appears in more test roots than
    # this it is widespread and is skipped.
    _MAX_SYMBOL_ROOTS: int = 20

    # Total-roots cap: if the combined surviving test roots across all symbols
    # exceeds this, the Kconfig file is too broad to handle in a targeted way
    # and the file is released back to MaintainerAreaStrategy.
    _MAX_TOTAL_ROOTS: int = 30

    # Symbols present in virtually every test – skip without even counting.
    _SKIP_SYMBOLS: frozenset = frozenset(
        {
            "ZTEST",
            "ZTEST_NEW_API",
            "TEST",
            "TEST_RANDOM_SEED",
            "ZTEST_SHUFFLE",
            "ZTEST_STACK_SIZE",
            "ZTEST_FATAL_HOOK",
            "ZTEST_THREAD_PRIO",
        }
    )

    @property
    def name(self):
        return "KconfigImpact"

    def __init__(self, zephyr_base, platform_filter=None):
        self._zephyr_base = Path(zephyr_base)
        self._platform_filter = platform_filter or []

    # ------------------------------------------------------------------
    # SelectionStrategy interface
    # ------------------------------------------------------------------

    def analyze(self, changed_files):
        kconfig_files = [f for f in changed_files if self._is_kconfig_file(f)]
        if not kconfig_files:
            return [], set()

        # Extract symbols from all changed Kconfig/conf files
        file_to_symbols: dict = {}
        for f in kconfig_files:
            syms = self._extract_symbols(f) - self._SKIP_SYMBOLS
            if syms:
                file_to_symbols[f] = syms

        all_symbols = set().union(*file_to_symbols.values()) if file_to_symbols else set()
        if not all_symbols:
            log.info("[%s] No relevant symbols found in changed files.", self.name)
            return [], set()

        log.info(
            "[%s] Scanning for %d symbol(s): %s",
            self.name,
            len(all_symbols),
            ", ".join(sorted(all_symbols)[:10]) + (" …" if len(all_symbols) > 10 else ""),
        )

        # Single grep across tests/ and samples/ for any of the symbols
        match_files = self._grep_for_symbols(all_symbols)
        if match_files is None:
            return [], set()
        if not match_files:
            log.info("[%s] No test files reference any of the changed symbols.", self.name)
            return [], set()

        # Pre-compile combined regex to identify which symbols each file mentions
        sym_re = re.compile(
            r'\bCONFIG_(' + '|'.join(re.escape(s) for s in sorted(all_symbols)) + r')\b'
        )

        # Build: test_root → set of symbols that reference it
        root_to_symbols: dict = {}
        for match_file in match_files:
            root = self._find_test_root(Path(match_file))
            if not root:
                continue
            rel_root = str(root.relative_to(self._zephyr_base))
            for sym in self._symbols_in_file(match_file, sym_re):
                root_to_symbols.setdefault(rel_root, set()).add(sym)

        if not root_to_symbols:
            return [], set()

        # Count per-symbol how many unique test roots it hits
        symbol_root_count: dict = {}
        for syms in root_to_symbols.values():
            for sym in syms:
                symbol_root_count[sym] = symbol_root_count.get(sym, 0) + 1

        widespread = {s for s, c in symbol_root_count.items() if c > self._MAX_SYMBOL_ROOTS}
        if widespread:
            log.info(
                "[%s] Widespread symbols (skipped): %s",
                self.name,
                ", ".join(sorted(widespread)),
            )

        surviving_symbols = all_symbols - widespread

        # Collect test roots that have at least one surviving symbol
        surviving_roots = {
            rel_root: (syms & surviving_symbols)
            for rel_root, syms in root_to_symbols.items()
            if syms & surviving_symbols
        }

        if not surviving_roots:
            log.info(
                "[%s] All symbols widespread – file(s) not consumed, "
                "falling through to MaintainerArea.",
                self.name,
            )
            return [], set()

        # Second guard: if the union of surviving roots is still too large,
        # this Kconfig file touches too many unrelated areas to be targeted.
        # Release it to MaintainerAreaStrategy instead.
        if len(surviving_roots) > self._MAX_TOTAL_ROOTS:
            log.info(
                "[%s] %d surviving roots (> %d) – file too broad, "
                "falling through to MaintainerArea.",
                self.name,
                len(surviving_roots),
                self._MAX_TOTAL_ROOTS,
            )
            return [], set()

        # Only consume files whose symbols contributed to surviving test roots
        surviving_contributing = set().union(*surviving_roots.values())
        handled = {f for f, syms in file_to_symbols.items() if syms & surviving_contributing}

        calls = []
        for rel_root, syms in sorted(surviving_roots.items()):
            log.info(
                "[%s] '%s' ← symbols: %s",
                self.name,
                rel_root,
                ", ".join(sorted(syms)),
            )
            calls.append(
                TwisterCall(
                    description=f"KconfigImpact → {rel_root}",
                    testsuite_roots=[rel_root],
                    platforms=list(self._platform_filter),
                )
            )

        return calls, handled

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _is_kconfig_file(self, filepath):
        """True for Kconfig definition files, defconfigs, and non-test .conf fragments."""
        name = Path(filepath).name
        if name == "Kconfig" or name.startswith("Kconfig."):
            return True
        if name.endswith("_defconfig") or name.endswith(".defconfig"):
            return True
        # .conf fragments outside tests/ and samples/ (those belong to DirectTest)
        if name.endswith(".conf"):
            return not (filepath.startswith("tests/") or filepath.startswith("samples/"))
        return False

    def _extract_symbols(self, filepath):
        """Return the set of Kconfig symbol names from *filepath*."""
        full = self._zephyr_base / filepath
        try:
            content = full.read_text(encoding="utf-8", errors="replace")
        except OSError:
            log.debug("[%s] Cannot read '%s'.", self.name, filepath)
            return set()
        name = Path(filepath).name
        if name == "Kconfig" or name.startswith("Kconfig."):
            return set(self._KCONFIG_SYMBOL_RE.findall(content))
        # .conf / _defconfig: extract CONFIG_SYMBOL names
        return set(self._CONF_SYMBOL_RE.findall(content))

    def _grep_for_symbols(self, symbols):
        """Grep tests/ and samples/ for .conf/.yaml files referencing any symbol.

        Returns a list of absolute file paths, or *None* on error.
        """
        sym_alternation = "|".join(re.escape(s) for s in sorted(symbols))
        pattern = rf'\bCONFIG_({sym_alternation})\b'

        scan_dirs = [
            str(self._zephyr_base / d)
            for d in ("tests", "samples")
            if (self._zephyr_base / d).is_dir()
        ]
        if not scan_dirs:
            return []

        cmd = ["grep", "-rl", "-E", pattern, "--include=*.conf", "--include=*.yaml"] + scan_dirs
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=60,
                cwd=str(self._zephyr_base),
            )  # noqa: S603
        except subprocess.TimeoutExpired:
            log.warning("[%s] grep timed out.", self.name)
            return None
        except OSError as exc:
            log.warning("[%s] grep error: %s", self.name, exc)
            return None

        if result.returncode not in (0, 1):
            log.warning("[%s] grep stderr: %s", self.name, result.stderr.strip())
            return None

        return [ln.strip() for ln in result.stdout.splitlines() if ln.strip()]

    @staticmethod
    def _symbols_in_file(filepath, sym_re):
        """Return set of symbol names matched by *sym_re* in *filepath*."""
        try:
            content = Path(filepath).read_text(encoding="utf-8", errors="replace")
        except OSError:
            return set()
        return set(sym_re.findall(content))

    def _find_test_root(self, start_path):
        """Walk up from *start_path* to the directory containing a YAML marker."""
        current = start_path if start_path.is_dir() else start_path.parent
        for _ in range(8):
            if any((current / m).exists() for m in self._YAML_MARKERS):
                return current
            parent = current.parent
            if parent == current:
                break
            current = parent
        return None


# ---------------------------------------------------------------------------
# Strategy 0 – Risk-based file classifier
# ---------------------------------------------------------------------------


class RiskClassifierStrategy(SelectionStrategy):
    """Classify changed files by risk level and adjust selection breadth.

    This strategy runs first.  It inspects every changed file and assigns it
    a risk level.  Higher-risk files require broader test coverage than the
    downstream targeted strategies would select on their own.

    Risk levels
    -----------
    SKIP
        No runtime tests needed.  Documentation, READMEs, licence files,
        comment-only changes, MAINTAINERS.yml updates, etc.  Files at this
        level are consumed so no other strategy wastes time on them.
    NARROW
        Board/SoC metadata, DTS files.  The normal pipeline (MaintainerArea
        or DirectTest) already gives appropriately targeted coverage.  The
        risk classifier takes no special action.
    NORMAL
        Default.  All downstream strategies apply normally.
    WIDE
        Core code whose defects propagate across the whole OS: ``arch/**``,
        ``kernel/**``, ``include/zephyr/**`` public headers, ``lib/os/**``,
        ``lib/libc/**``.  The classifier consumes these files and emits broad
        ``-T <test-root>`` calls covering the relevant test sub-trees, which
        is more comprehensive than the pattern-based coverage from
        MaintainerAreaStrategy.
    FULL
        Changes to the build system (cmake, top-level CMakeLists.txt),
        linker scripts, or the west.yml manifest.  A defect here could break
        any target; the classifier signals ``TWISTER_FULL=true`` so CI can
        dispatch a complete run.
    """

    # RiskClassifier is authoritative: SKIP/FULL/WIDE files must not be seen
    # by downstream strategies, so they are removed from the pool.
    consumes: bool = True

    class _Risk(enum.IntEnum):
        SKIP = 0
        NARROW = 1
        NORMAL = 2
        WIDE = 3
        FULL = 4

    # File/directory prefixes → risk level (evaluated in order, first match wins)
    _PREFIX_RULES: tuple = (
        # SKIP prefixes
        ("doc/", _Risk.SKIP),
        ("LICENSES/", _Risk.SKIP),
        # FULL prefixes
        ("cmake/", _Risk.FULL),
        ("scripts/build/", _Risk.FULL),
        # WIDE prefixes
        ("arch/", _Risk.WIDE),
        ("kernel/", _Risk.WIDE),
        ("include/zephyr/", _Risk.WIDE),
        ("lib/os/", _Risk.WIDE),
        ("lib/libc/", _Risk.WIDE),
        # NARROW prefixes
        ("boards/", _Risk.NARROW),
        ("soc/", _Risk.NARROW),
        ("dts/", _Risk.NARROW),
    )

    # Exact filenames that get SKIP regardless of location
    _SKIP_NAMES: frozenset = frozenset(
        {
            "MAINTAINERS.yml",
            "CODEOWNERS",
            "CODE_OF_CONDUCT.md",
            "CONTRIBUTING.rst",
            "README.rst",
            "REUSE.toml",
        }
    )

    # File suffixes that always get SKIP (plain text / markup / images)
    _SKIP_SUFFIXES: frozenset = frozenset(
        {
            ".rst",
            ".md",
            ".png",
            ".jpg",
            ".svg",
            ".gif",
        }
    )

    # Root-level files (no parent directory) that trigger a FULL run
    _FULL_ROOT_FILES: frozenset = frozenset(
        {
            "CMakeLists.txt",
            "Kconfig",
            "Kconfig.zephyr",
            "west.yml",
        }
    )

    # File suffixes that trigger a FULL run (linker scripts)
    _FULL_SUFFIXES: frozenset = frozenset({".ld", ".ld.in"})

    # WIDE zone → test directory roots to cover
    _WIDE_TEST_ROOTS: tuple = (
        ("arch/", ["tests/arch", "tests/kernel"]),
        ("kernel/", ["tests/kernel"]),
        ("include/zephyr/arch/", ["tests/arch"]),
        ("include/zephyr/", ["tests/kernel", "tests/lib"]),
        ("lib/os/", ["tests/kernel"]),
        ("lib/libc/", ["tests/lib/c_lib", "tests/lib/cpp"]),
    )

    # Complexity score threshold above which NORMAL-risk files are escalated to WIDE
    _COMPLEXITY_ESCALATION_THRESHOLD: float = 10.0

    @property
    def name(self):
        return "RiskClassifier"

    def __init__(self, zephyr_base, platform_filter=None, context=None):
        self._zephyr_base = Path(zephyr_base)
        self._platform_filter = platform_filter or []
        self._context = context  # PipelineContext or None

    # ------------------------------------------------------------------
    # SelectionStrategy interface
    # ------------------------------------------------------------------

    def analyze(self, changed_files):
        Risk = self._Risk
        by_risk: dict = {r: [] for r in Risk}
        for f in changed_files:
            by_risk[self._classify(f)].append(f)

        # Complexity escalation: if ComplexityStrategy scored the patchset
        # above the threshold, promote NORMAL-risk files to WIDE so they
        # receive broader test coverage.
        complexity_score = self._context.complexity_score if self._context else 0.0
        if complexity_score >= self._COMPLEXITY_ESCALATION_THRESHOLD:
            escalated = by_risk[Risk.NORMAL]
            if escalated:
                log.info(
                    "[%s] Complexity score %.2f ≥ %.1f – escalating %d "
                    "NORMAL-risk file(s) to WIDE: %s",
                    self.name,
                    complexity_score,
                    self._COMPLEXITY_ESCALATION_THRESHOLD,
                    len(escalated),
                    ", ".join(sorted(escalated)),
                )
                by_risk[Risk.WIDE].extend(escalated)
                by_risk[Risk.NORMAL] = []

        skip_files = by_risk[Risk.SKIP]
        full_files = by_risk[Risk.FULL]
        wide_files = by_risk[Risk.WIDE]

        handled = set(skip_files)
        calls = []

        if skip_files:
            log.info(
                "[%s] SKIP – %d test-free file(s): %s",
                self.name,
                len(skip_files),
                ", ".join(sorted(skip_files)),
            )

        # A single FULL-risk file overrides everything: signal a full run.
        if full_files:
            log.warning(
                "[%s] FULL-risk change – %d file(s) affect the build system / linker: %s",
                self.name,
                len(full_files),
                ", ".join(sorted(full_files)),
            )
            handled = set(changed_files)  # consume everything
            calls.append(
                TwisterCall(
                    description="RiskClassifier: FULL (build-system / linker change)",
                    full_run=True,
                )
            )
            return calls, handled

        if wide_files:
            # Collect unique test roots across all wide-zone files.
            # Use an ordered dict to maintain deterministic order.
            root_to_files: dict = {}
            for f in wide_files:
                for root in self._wide_roots_for(f):
                    abs_root = self._zephyr_base / root
                    if abs_root.is_dir():
                        root_to_files.setdefault(root, []).append(f)

            if root_to_files:
                for root, files in sorted(root_to_files.items()):
                    log.info(
                        "[%s] WIDE – '%s' ← %s",
                        self.name,
                        root,
                        ", ".join(sorted(set(files))),
                    )
                handled.update(wide_files)
                calls.append(
                    TwisterCall(
                        description="RiskClassifier: WIDE coverage",
                        testsuite_roots=sorted(root_to_files.keys()),
                        platforms=list(self._platform_filter),
                    )
                )
            else:
                log.info(
                    "[%s] WIDE-zone files found but no test roots exist; falling through.",
                    self.name,
                )

        return calls, handled

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _classify(self, filepath):
        """Return the :class:`_Risk` level for *filepath*."""
        Risk = self._Risk
        p = Path(filepath)
        name = p.name

        # Exact skip names
        if name in self._SKIP_NAMES:
            return Risk.SKIP

        # Skip by suffix
        if p.suffix in self._SKIP_SUFFIXES:
            return Risk.SKIP

        # Linker scripts → FULL
        if name.endswith(".ld") or name.endswith(".ld.in"):
            return Risk.FULL

        # Root-level build-system files → FULL
        if str(p.parent) == "." and name in self._FULL_ROOT_FILES:
            return Risk.FULL

        # Prefix rules
        for prefix, risk in self._PREFIX_RULES:
            if filepath.startswith(prefix):
                return risk

        return Risk.NORMAL

    def _wide_roots_for(self, filepath):
        """Return the list of test root directories for a WIDE-zone *filepath*."""
        for prefix, roots in self._WIDE_TEST_ROOTS:
            if filepath.startswith(prefix):
                return roots
        return []


# ---------------------------------------------------------------------------
# Strategy N – west manifest changes → module-tagged integration tests
# ---------------------------------------------------------------------------
# Strategy – patchset complexity scoring (pydriller + lizard)
# ---------------------------------------------------------------------------


class ComplexityStrategy(SelectionStrategy):
    """Analyse patchset complexity using pydriller and lizard, then emit a
    composite score into :class:`PipelineContext`.

    This strategy is **non-consuming** and emits **no** :class:`TwisterCall`
    objects.  Its sole purpose is to populate
    :attr:`PipelineContext.complexity_score` and
    :attr:`PipelineContext.file_metrics` so that downstream strategies (in
    particular :class:`RiskClassifierStrategy`) can use the data to make
    risk-escalation decisions.

    Score model
    -----------
    For every modified C / C++ / assembly source file the following raw
    metrics are collected and weighted into a per-file sub-score:

    * **churn** – ``added_lines + deleted_lines`` from pydriller's commit
      model.  High churn correlates with merge-conflict risk and review
      fatigue.  Weight: ``0.01`` (1 point per 100 lines changed).

    * **max_method_ccn** – highest cyclomatic complexity (McCabe CCN) among
      the methods *actually touched by the diff*, taken from pydriller's
      ``changed_methods`` list.  A CCN above 10 indicates highly branching
      logic.  Weight: ``0.5`` (0.5 points per CCN unit of the worst method).

    * **avg_ccn_delta** – change in the file's average CCN between the old
      and new versions, computed by running lizard on the full source text
      of both versions.  A positive delta means the patch made the file
      harder to understand.  Weight: ``2.0`` (2 points per unit of average
      CCN increase).

    * **dmm_unit_complexity** – pydriller's Delta Maintainability Model
      fraction: the proportion of methods in the commit with CCN > 5.
      Ranges 0–1; higher is riskier.  Weight: ``3.0``.

    The aggregate score is the sum of per-file sub-scores:

    .. code-block:: none

        sub_score(f) = 0.01 * churn
                     + 0.5  * max_method_ccn
                     + 2.0  * max(0, avg_ccn_delta)
                     + 3.0  * (dmm_unit_complexity or 0)

        total_score = Σ sub_score(f)  for all modified source files

    The weights are deliberately conservative so that trivial patches
    (one-liner bug fixes in simple functions) score near zero while
    wide-ranging refactors of high-CCN code score several tens of points.
    A score above :attr:`_ESCALATION_THRESHOLD` causes the strategy to mark
    itself as "high complexity" in the context, which
    :class:`RiskClassifierStrategy` can use to escalate NORMAL-risk files to
    WIDE.

    Requirements
    ------------
    Both ``pydriller`` and ``lizard`` must be importable.  If either is
    missing the strategy logs a warning and exits cleanly (score stays 0).
    A git commit range must be supplied via ``--commits``; without it the
    strategy is a no-op.

    The strategy is non-consuming so **all** changed files remain visible to
    downstream strategies regardless of their complexity score.
    """

    consumes: bool = False

    # Source file extensions analysed by lizard
    _SOURCE_EXTS: frozenset = frozenset(
        {
            ".c",
            ".h",
            ".cpp",
            ".cc",
            ".cxx",
            ".hpp",
            ".S",
            ".s",
        }
    )

    # Scoring weights
    _W_CHURN = 0.01  # per added+deleted line
    _W_MAX_CCN = 0.5  # per CCN unit of the worst changed method
    _W_CCN_DELTA = 2.0  # per unit of avg-CCN increase
    _W_DMM = 3.0  # DMM unit-complexity fraction (0–1)

    # Score above this value is treated as "high complexity" by the context
    _ESCALATION_THRESHOLD: float = 10.0

    @property
    def name(self):
        return "ComplexityScorer"

    def __init__(self, context, repo_path, commits=None):
        """
        Parameters
        ----------
        context:
            Shared :class:`PipelineContext` instance.  The strategy writes
            ``complexity_score`` and ``file_metrics`` into it.
        repo_path:
            Filesystem path to the git repository root.
        commits:
            Commit range string (e.g. ``"main..HEAD"``).  Required; without
            it the strategy is a no-op.
        """
        self._context = context
        self._repo_path = str(repo_path)
        self._commits = commits

    # ------------------------------------------------------------------
    # SelectionStrategy interface
    # ------------------------------------------------------------------

    def analyze(self, changed_files):
        if not self._commits:
            log.debug("[%s] No --commits range – skipping complexity analysis.", self.name)
            return [], set()

        try:
            import lizard as _lizard  # noqa: PLC0415
            from pydriller import Repository as PydrillerRepository  # noqa: PLC0415
        except ImportError as exc:
            log.warning("[%s] Optional dependency missing (%s) – skipping.", self.name, exc)
            return [], set()

        parts = self._commits.split("..")
        base_ref = parts[0]
        to_commit = parts[1] if len(parts) > 1 else "HEAD"

        # pydriller's from_commit is *inclusive*, but git's A..B notation
        # excludes A.  Resolve A's full hash and skip it inside the loop.
        try:
            _repo = Repo(self._repo_path)
            base_hash = _repo.git.rev_parse(base_ref)
        except Exception:  # noqa: BLE001
            base_hash = None  # can't resolve; will process all commits

        total_score = 0.0
        file_metrics = {}

        try:
            repo_iter = PydrillerRepository(
                self._repo_path,
                from_commit=base_ref,
                to_commit=to_commit,
            ).traverse_commits()
        except Exception as exc:  # noqa: BLE001
            log.warning("[%s] pydriller failed to open repository: %s", self.name, exc)
            return [], set()

        for commit in repo_iter:
            # Skip the base (start) commit – git's A..B excludes A but
            # pydriller's from_commit is inclusive.
            if base_hash and commit.hash == base_hash:
                continue
            # DMM metrics are per-commit (shared across all files in the commit)
            dmm = commit.dmm_unit_complexity  # float 0-1 or None

            for mf in commit.modified_files:
                rel_path = mf.new_path or mf.old_path or mf.filename
                if not rel_path:
                    continue
                if Path(rel_path).suffix not in self._SOURCE_EXTS:
                    continue

                # --- churn ---
                churn = (mf.added_lines or 0) + (mf.deleted_lines or 0)

                # --- max CCN among changed methods (pydriller) ---
                max_ccn = 0
                try:
                    for method in mf.changed_methods or []:
                        if method.complexity is not None:
                            max_ccn = max(max_ccn, method.complexity)
                except Exception:  # noqa: BLE001
                    pass

                # --- avg CCN delta (lizard on full source text) ---
                avg_ccn_delta = 0.0
                nloc_delta = 0
                try:
                    old_ccn = new_ccn = None
                    old_nloc = new_nloc = 0
                    if mf.source_code_before:
                        r = _lizard.analyze_file.analyze_source_code(
                            mf.filename, mf.source_code_before
                        )
                        old_ccn = r.average_cyclomatic_complexity
                        old_nloc = r.nloc or 0
                    if mf.source_code:
                        r = _lizard.analyze_file.analyze_source_code(mf.filename, mf.source_code)
                        new_ccn = r.average_cyclomatic_complexity
                        new_nloc = r.nloc or 0
                    if old_ccn is not None and new_ccn is not None:
                        avg_ccn_delta = new_ccn - old_ccn
                    nloc_delta = new_nloc - old_nloc
                except Exception:  # noqa: BLE001
                    pass

                metrics = ComplexityMetrics(
                    filename=rel_path,
                    churn=churn,
                    max_method_ccn=max_ccn,
                    avg_ccn_delta=avg_ccn_delta,
                    nloc_delta=nloc_delta,
                    dmm_complexity=dmm,
                )
                file_metrics[rel_path] = metrics

                sub = (
                    self._W_CHURN * churn
                    + self._W_MAX_CCN * max_ccn
                    + self._W_CCN_DELTA * max(0.0, avg_ccn_delta)
                    + self._W_DMM * (dmm or 0.0)
                )
                total_score += sub

                log.debug(
                    "[%s] %s: churn=%d max_ccn=%d ccn_delta=%+.2f dmm=%.2f → sub=%.2f",
                    self.name,
                    rel_path,
                    churn,
                    max_ccn,
                    avg_ccn_delta,
                    dmm or 0.0,
                    sub,
                )

        self._context.complexity_score = total_score
        self._context.file_metrics = file_metrics

        log.info(
            "[%s] Patchset complexity score: %.2f  "
            "(%d source file(s) analysed, threshold=%.1f, escalate=%s)",
            self.name,
            total_score,
            len(file_metrics),
            self._ESCALATION_THRESHOLD,
            total_score >= self._ESCALATION_THRESHOLD,
        )

        if file_metrics:
            # Log top-3 contributors by sub-score for debugging
            scored = sorted(
                file_metrics.items(),
                key=lambda kv: (
                    self._W_CHURN * kv[1].churn
                    + self._W_MAX_CCN * kv[1].max_method_ccn
                    + self._W_CCN_DELTA * max(0.0, kv[1].avg_ccn_delta)
                    + self._W_DMM * (kv[1].dmm_complexity or 0.0)
                ),
                reverse=True,
            )
            for path, m in scored[:3]:
                log.info(
                    "[%s]   %-50s  churn=%4d  max_ccn=%2d  ccn_delta=%+.2f  dmm=%s",
                    self.name,
                    path,
                    m.churn,
                    m.max_method_ccn,
                    m.avg_ccn_delta,
                    f"{m.dmm_complexity:.2f}" if m.dmm_complexity is not None else "N/A",
                )

        return [], set()


# ---------------------------------------------------------------------------
# Strategy N – west manifest changes → module-tagged integration tests
# ---------------------------------------------------------------------------


class ManifestStrategy(SelectionStrategy):
    """For changed ``west.yml`` or ``submanifests/*.yaml`` files, diff the
    manifest against its previous revision and run integration tests tagged
    with the names of added, removed, or updated projects.

    This mirrors the ``find_modules`` logic from ``scripts/ci/test_plan.py``:

    Resolution chain
    ----------------
    1. Detect whether ``west.yml`` or any ``submanifests/`` file appears in
       the changed-file list.  If not, this strategy is a no-op.

    2. Require a git commit range (``--commits A..B``).  Without one the old
       manifest content cannot be retrieved and the strategy logs a warning
       and falls through so :class:`MaintainerAreaStrategy` can catch the
       file.

    3. Load the *old* manifest via ``git show <base>:west.yml`` (written to a
       temp file) and the *new* manifest from the working tree.  Both are
       parsed with :class:`west.manifest.Manifest`.

    4. Compute three project sets:

       * **removed** – in old, not in new (by name).
       * **updated** – in both, but revision changed.
       * **added**   – in new, not in old (by name).

    5. For each changed project name, emit one :class:`TwisterCall` using
       ``--tag <name> --integration``.  This lets twister select only tests
       whose ``tags:`` field lists the module (e.g. ``tags: mbedtls``).

    This strategy **consumes** all matched manifest files so they do not
    reach :class:`MaintainerAreaStrategy`.

    Note: ``submanifests/`` changes are treated as a proxy for ``west.yml``
    changes because the sub-manifest imports are resolved into the top-level
    manifest.  The entire diff is computed from ``west.yml`` regardless of
    which manifest file was edited.
    """

    consumes: bool = True

    # Manifest files that trigger this strategy
    _MANIFEST_FILES: frozenset = frozenset({"west.yml"})
    _SUBMANIFEST_PREFIX: str = "submanifests/"

    @property
    def name(self):
        return "ManifestStrategy"

    def __init__(self, zephyr_base, repo=None, commits=None, platform_filter=None):
        """
        Parameters
        ----------
        repo:
            A :class:`git.Repo` instance for the Zephyr repository.  Required
            to retrieve the old manifest content via ``git show``.  When
            ``None`` the strategy will warn and fall through.
        commits:
            The commit range string used on the CLI (e.g. ``"main..HEAD"``).
            The base commit is derived as ``commits.split("..")[0]``.
        """
        self._zephyr_base = Path(zephyr_base)
        self._repo = repo
        self._commits = commits
        self._platform_filter = platform_filter or []

    # ------------------------------------------------------------------
    # SelectionStrategy interface
    # ------------------------------------------------------------------

    def analyze(self, changed_files):
        manifest_files = [
            f for f in changed_files if f == "west.yml" or f.startswith(self._SUBMANIFEST_PREFIX)
        ]
        if not manifest_files:
            return [], set()

        if self._repo is None or not self._commits:
            log.warning(
                "[%s] west.yml changed but no --commits range supplied; "
                "cannot diff manifest – falling through to MaintainerArea.",
                self.name,
            )
            return [], set()

        parts = self._commits.split("..")
        base_commit = parts[0]
        head_commit = parts[1] if len(parts) > 1 else "HEAD"
        changed_projects = self._diff_manifests(base_commit, head_commit)

        if changed_projects is None:
            # Diff failed – do not consume so MaintainerArea can catch it
            return [], set()

        if not changed_projects:
            log.info(
                "[%s] Manifest changed but no project revisions differ "
                "(e.g. comment / formatting edit) – no module tests needed.",
                self.name,
            )
            return set(manifest_files), set(manifest_files)

        log.info(
            "[%s] Changed west.yml modules: %s",
            self.name,
            ", ".join(sorted(changed_projects)),
        )

        calls = []
        for proj_name in sorted(changed_projects):
            extra = []
            for p in self._platform_filter:
                extra += ["-p", p]
            calls.append(
                TwisterCall(
                    description=f"ManifestStrategy: module '{proj_name}'",
                    extra_args=["-t", proj_name] + extra,
                    integration=True,
                )
            )

        return calls, set(manifest_files)

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _diff_manifests(self, base_commit, head_commit="HEAD"):
        """Return the set of project names whose revision changed between
        *base_commit* and *head_commit* in ``west.yml``.

        Both sides are read via ``git show`` so the comparison is always
        against the exact commits in the range, not the working tree.

        Returns ``None`` on error, ``set()`` when manifest changed but no
        project revisions differ.
        """
        try:
            old_content = self._repo.git.show(f"{base_commit}:west.yml")
        except Exception as exc:  # noqa: BLE001
            log.warning(
                "[%s] Could not retrieve old west.yml at '%s': %s",
                self.name,
                base_commit,
                exc,
            )
            return None

        import importlib.util

        if importlib.util.find_spec("west.manifest") is None:
            log.warning("[%s] west.manifest not importable – skipping.", self.name)
            return None

        try:
            old_manifest = self._load_manifest_data(old_content)
            new_content = self._repo.git.show(f"{head_commit}:west.yml")
            new_manifest = self._load_manifest_data(new_content)
        except Exception as exc:  # noqa: BLE001
            log.warning("[%s] Failed to parse manifest(s): %s", self.name, exc)
            return None

        if old_manifest is None or new_manifest is None:
            return None

        old_projs = {p.name: p.revision for p in old_manifest.projects}
        new_projs = {p.name: p.revision for p in new_manifest.projects}

        removed = {n for n in old_projs if n not in new_projs}
        added = {n for n in new_projs if n not in old_projs}
        updated = {n for n in old_projs if n in new_projs and old_projs[n] != new_projs[n]}

        log.debug("[%s] removed=%s added=%s updated=%s", self.name, removed, added, updated)
        return removed | added | updated

    @staticmethod
    def _load_manifest_data(content):
        """Parse a manifest YAML *content* string without requiring a west
        workspace on disk.

        Uses :meth:`west.manifest.Manifest.from_data` with
        ``ImportFlag.IGNORE`` so that ``self: import: submanifests`` entries
        are silently skipped.  Only top-level project entries are compared,
        which is sufficient for detecting bumped module revisions.

        Returns a :class:`west.manifest.Manifest` instance, or ``None`` on
        failure.
        """
        try:
            from west.manifest import ImportFlag, Manifest  # noqa: PLC0415

            data = yaml.load(content, Loader=SafeLoader)
            return Manifest.from_data(data, import_flags=ImportFlag.IGNORE)
        except Exception as exc:  # noqa: BLE001
            log.debug("ManifestStrategy: could not parse manifest data: %s", exc)
            return None


# ---------------------------------------------------------------------------
# Strategy 4 – Ignore list: files that never trigger tests
# ---------------------------------------------------------------------------


class IgnoreStrategy(SelectionStrategy):
    """Consume changed files that are known to never affect test outcomes.

    The ignore list is read from ``scripts/ci/twister_ignore.txt`` (or a
    custom path supplied at construction time).  Each non-blank, non-comment
    line is treated as an :func:`fnmatch.fnmatch` glob pattern matched
    against each changed file's *relative* path.

    Files that match are consumed and produce **no** :class:`TwisterCall`.
    This prevents documentation, tooling, and workflow changes from
    propagating to downstream strategies and generating spurious test runs.
    """

    consumes: bool = True

    @property
    def name(self):
        return "IgnoreList"

    def __init__(self, ignore_file):
        self._ignore_file = Path(ignore_file)

    def analyze(self, changed_files):
        patterns = self._load_patterns()
        if not patterns:
            return [], set()

        ignored = set()
        for f in changed_files:
            if any(fnmatch.fnmatch(f, pat) for pat in patterns):
                log.info("[%s] Ignoring '%s' (matched ignore list).", self.name, f)
                ignored.add(f)

        if ignored:
            log.info(
                "[%s] %d file(s) suppressed by ignore list.",
                self.name,
                len(ignored),
            )

        return [], ignored

    def _load_patterns(self):
        """Return the list of active glob patterns from the ignore file.

        Lines that are blank or start with ``#`` are skipped.
        """
        try:
            lines = self._ignore_file.read_text(encoding="utf-8").splitlines()
        except OSError:
            log.warning("[%s] Ignore file not found: %s", self.name, self._ignore_file)
            return []
        return [ln.strip() for ln in lines if ln.strip() and not ln.strip().startswith("#")]


# ---------------------------------------------------------------------------
# Strategy 5 – Direct test/sample changes
# ---------------------------------------------------------------------------


class DirectTestStrategy(SelectionStrategy):
    """Run any test or sample that was directly changed.

    Resolution chain
    ----------------
    1. Accept any changed file that lives under ``tests/`` or ``samples/``.

    2. For each such file, walk up the directory tree looking for
       ``testcase.yaml``, ``tests.yaml``, or ``sample.yaml``.  The first
       directory that contains one of those files becomes the test root.

    3. Deduplicate: multiple changed files that resolve to the same root
       are merged into a single :class:`TwisterCall`.

    4. Emit one :class:`TwisterCall` per unique root so that twister builds and
       runs the suite end-to-end rather than just collecting it.

    This strategy is a *consumer* (``consumes = True``): once a test/sample
    file is claimed here, downstream strategies will not see it.  This
    prevents a test-only change from triggering the full area test sweep in
    :class:`MaintainerAreaStrategy`.
    """

    # DirectTest is authoritative for files under tests/ and samples/:
    # a changed test file must not also trigger the area-wide sweep.
    consumes: bool = True

    # YAML files that mark the root of a test or sample directory
    _YAML_MARKERS: tuple = ("testcase.yaml", "tests.yaml", "sample.yaml")

    # Top-level trees to consider
    _TEST_PREFIXES: tuple = ("tests/", "samples/")

    # Cap on directory-tree walk depth
    _MAX_WALK_DEPTH: int = 8

    @property
    def name(self):
        return "DirectTest"

    def __init__(self, zephyr_base, platform_filter=None):
        self._zephyr_base = Path(zephyr_base)
        self._platform_filter = platform_filter or []

    def analyze(self, changed_files):
        test_files = [
            f for f in changed_files if any(f.startswith(pfx) for pfx in self._TEST_PREFIXES)
        ]

        if not test_files:
            return [], set()

        # file → its test root (rel path) or None
        root_to_files: dict = {}
        unrooted = []

        for f in test_files:
            root = self._find_test_root(f)
            if root:
                root_to_files.setdefault(root, []).append(f)
            else:
                unrooted.append(f)

        if unrooted:
            log.debug(
                "[%s] No testcase/sample yaml found for: %s",
                self.name,
                ", ".join(unrooted),
            )

        calls = []
        handled = set()

        for rel_root, files in sorted(root_to_files.items()):
            log.info(
                "[%s] '%s' ← %d changed file(s): %s",
                self.name,
                rel_root,
                len(files),
                ", ".join(sorted(files)),
            )
            handled.update(files)
            calls.append(
                TwisterCall(
                    description=f"DirectTest → {rel_root}",
                    testsuite_roots=[rel_root],
                    platforms=list(self._platform_filter),
                    integration=False,
                )
            )

        return calls, handled

    def _find_test_root(self, filepath):
        """Walk up from *filepath* to find the directory with a YAML marker.

        Returns the workspace-relative path string, or ``None``.
        """
        current = (self._zephyr_base / filepath).parent
        for _ in range(self._MAX_WALK_DEPTH):
            if any((current / m).exists() for m in self._YAML_MARKERS):
                try:
                    return str(current.relative_to(self._zephyr_base))
                except ValueError:
                    return None
            parent = current.parent
            if parent == current:
                break
            current = parent
        return None


# ---------------------------------------------------------------------------
# Strategy registry
# ---------------------------------------------------------------------------


def build_strategies(
    maintainers_file,
    platform_filter=None,
    zephyr_base=None,
    disabled=None,
    repo=None,
    commits=None,
    context=None,
):
    """Return the ordered list of strategies to run.

    Parameters
    ----------
    disabled:
        Optional collection of strategy *names* (case-insensitive) to exclude
        from the returned list.  Useful for testing or temporarily bypassing a
        strategy on the CLI via ``--disable-strategy``.
    repo:
        A :class:`git.Repo` instance.  Required for :class:`ManifestStrategy`
        to retrieve the old manifest content via ``git show``.
    commits:
        Commit range string (e.g. ``"main..HEAD"``).  Passed to
        :class:`ManifestStrategy` and :class:`ComplexityStrategy` so they
        can identify the commit range.
    context:
        Shared :class:`PipelineContext` instance.  When ``None`` a fresh
        context is created.  Pass an explicit instance to share state across
        calls (e.g. in tests).
    """
    base = Path(zephyr_base) if zephyr_base else ZEPHYR_BASE
    disabled_set = {n.lower() for n in (disabled or [])}
    ctx = context if context is not None else PipelineContext()

    all_strategies = [
        # 0. Complexity scorer: analyses patchset using pydriller + lizard
        #    and stores a score in the shared context.  Must run before
        #    RiskClassifierStrategy so the score is available for escalation.
        #    Non-consuming; emits no TwisterCalls.
        ComplexityStrategy(
            context=ctx,
            repo_path=base,
            commits=commits,
        ),
        # 0b. Risk classifier: disabled pending rework – needs better
        #    classification rules and blast-radius definition before it
        #    can be re-enabled in production.
        # RiskClassifierStrategy(
        #     zephyr_base=base,
        #     platform_filter=platform_filter,
        #     context=ctx,
        # ),
        # 1. Ignore list: consume files that never affect tests (docs, tooling,
        #    CI workflows …) so they don't reach downstream strategies.
        IgnoreStrategy(
            ignore_file=base / "scripts" / "ci" / "twister_ignore.txt",
        ),
        # 2. Test/sample changes: run exactly those tests, nothing more.
        DirectTestStrategy(
            zephyr_base=base,
            platform_filter=platform_filter,
        ),
        # 3. Snippet changes: run only the tests that require the snippet.
        SnippetStrategy(
            zephyr_base=base,
            platform_filter=platform_filter,
        ),
        # 4. Board directory changes: run integration/kernel on every variant.
        BoardStrategy(
            zephyr_base=base,
            platform_filter=platform_filter,
        ),
        # 5. SoC directory changes: run integration/kernel on all boards that
        #    use any affected SoC.
        SoCStrategy(
            zephyr_base=base,
            platform_filter=platform_filter,
        ),
        # 6. west.yml / submanifests changes: diff the manifest, find changed
        #    projects and run integration tests tagged with those module names.
        ManifestStrategy(
            zephyr_base=base,
            repo=repo,
            commits=commits,
            platform_filter=platform_filter,
        ),
        # 7. Driver source changes: find tests via DT overlay compat matching
        #    and board-targeted area-pattern calls via the dts/ dtsi chain.
        DriverCompatStrategy(
            zephyr_base=base,
            platform_filter=platform_filter,
            maintainers_file=maintainers_file,
        ),
        # 8. DTS binding changes: resolve compatible string and run
        #    compat-matched tests + board-targeted area calls.
        DtsBindingStrategy(
            zephyr_base=base,
            platform_filter=platform_filter,
            maintainers_file=maintainers_file,
        ),
        # 9. Kconfig / config-fragment changes: tests that enable the symbols.
        KconfigImpactStrategy(
            zephyr_base=base,
            platform_filter=platform_filter,
        ),
        # 10. Header changes: trace #include users back to maintainer areas.
        HeaderImpactStrategy(
            maintainers_file=maintainers_file,
            zephyr_base=base,
            platform_filter=platform_filter,
        ),
        # 11. Catch-all: remaining files matched against MAINTAINERS.yml areas.
        MaintainerAreaStrategy(
            maintainers_file=maintainers_file,
            platform_filter=platform_filter,
        ),
    ]

    if not disabled_set:
        return all_strategies
    kept = [s for s in all_strategies if s.name.lower() not in disabled_set]
    skipped = [s.name for s in all_strategies if s.name.lower() in disabled_set]
    if skipped:
        log.info("Disabled strategies: %s", ", ".join(skipped))
    return kept


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


def parse_args():
    parser = argparse.ArgumentParser(
        description=(
            "Modular CI test selector: build a targeted twister test plan "
            "from the set of files changed in a Pull Request."
        ),
        allow_abbrev=False,
    )

    input_group = parser.add_mutually_exclusive_group()
    input_group.add_argument(
        "-c",
        "--commits",
        metavar="A..B",
        default=None,
        help="Git commit range (e.g. 'main..HEAD').  "
        "Changed files are derived from 'git diff --name-only A..B'.",
    )
    input_group.add_argument(
        "-m",
        "--modified-files",
        metavar="FILE",
        default=None,
        help="JSON file containing a list of changed file paths.",
    )
    input_group.add_argument(
        "-f",
        "--file",
        action="append",
        dest="files",
        default=[],
        metavar="PATH",
        help="Treat PATH as a changed file (repeatable). Useful for quick local testing.",
    )

    parser.add_argument(
        "-o",
        "--output-file",
        metavar="FILE",
        default="testplan.json",
        help="Output JSON file passed to 'twister --load-tests' (default: testplan.json).",
    )
    parser.add_argument(
        "-p",
        "--platform",
        action="append",
        dest="platforms",
        default=[],
        metavar="PLATFORM",
        help="Restrict every test selection to this platform (repeatable).",
    )
    parser.add_argument(
        "--maintainers-file",
        default=str(ZEPHYR_BASE / "MAINTAINERS.yml"),
        metavar="FILE",
        help="Path to MAINTAINERS.yml (default: $ZEPHYR_BASE/MAINTAINERS.yml).",
    )
    parser.add_argument(
        "-T",
        "--testsuite-root",
        action="append",
        default=[],
        dest="testsuite_roots",
        metavar="DIR",
        help="Extra test-suite root forwarded to every twister call (repeatable).",
    )
    parser.add_argument(
        "--quarantine-list",
        action="append",
        default=[],
        metavar="FILE",
        help="Quarantine YAML file forwarded to twister (repeatable).",
    )
    parser.add_argument(
        "--detailed-test-id",
        action="store_true",
        default=False,
        help="Pass --detailed-test-id to twister.",
    )
    parser.add_argument(
        "--tests-per-builder",
        default=900,
        type=int,
        metavar="N",
        help="Tests per CI builder node – used to compute TWISTER_NODES (default: 900).",
    )
    parser.add_argument(
        "-r",
        "--repo",
        default=str(ZEPHYR_BASE),
        metavar="DIR",
        help="Git repository used when --commits is given (default: ZEPHYR_BASE).",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="Enable DEBUG-level logging.",
    )
    parser.add_argument(
        "--disable-strategy",
        action="append",
        default=[],
        dest="disabled_strategies",
        metavar="NAME",
        help=(
            "Disable the named strategy (repeatable, case-insensitive). "
            "Available names: RiskClassifier, DirectTest, DriverCompat, "
            "KconfigImpact, HeaderImpact, MaintainerArea."
        ),
    )

    return parser.parse_args()


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------


def main():
    args = parse_args()

    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)

    changed_files = []
    repo = None

    if args.commits:
        repo = Repo(args.repo)
        raw = repo.git.diff("--name-only", args.commits)
        changed_files = [f for f in raw.split("\n") if f]
    elif args.modified_files:
        with open(args.modified_files, encoding="utf-8") as fh:
            changed_files = json.load(fh)
    elif args.files:
        changed_files = args.files

    # Normalise paths: strip leading ./ so all strategies see bare relative
    # paths (e.g. "soc/nordic/nrf51/soc.h" not "./soc/nordic/nrf51/soc.h").
    changed_files = [
        str(Path(f).as_posix()).lstrip("/")
        if not Path(f).is_absolute()
        else str(Path(f).relative_to(ZEPHYR_BASE))
        for f in changed_files
    ]

    if changed_files:
        log.info("Changed files (%d):\n  %s", len(changed_files), "\n  ".join(changed_files))
    else:
        log.info("No changed files provided.")

    context = PipelineContext()

    strategies = build_strategies(
        maintainers_file=args.maintainers_file,
        platform_filter=args.platforms,
        zephyr_base=ZEPHYR_BASE,
        disabled=args.disabled_strategies,
        repo=repo,
        commits=args.commits,
        context=context,
    )

    executor = TwisterExecutor(
        zephyr_base=ZEPHYR_BASE,
        extra_testsuite_roots=args.testsuite_roots,
        detailed_test_id=args.detailed_test_id,
        quarantine_list=args.quarantine_list,
    )

    orchestrator = Orchestrator(
        strategies=strategies,
        executor=executor,
        tests_per_builder=args.tests_per_builder,
    )

    return orchestrator.run(changed_files, args.output_file)


if __name__ == "__main__":
    sys.exit(main())
