#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright The Zephyr Project Contributors

"""Sync test identifiers in MAINTAINERS.yml based on actual test coverage.

For each area in MAINTAINERS.yml that has tests/ or samples/ directory paths
in its ``files`` section, this script runs a single ``twister --list-tests``
call (passing all paths via repeated ``-T``) and extracts covering test
identifiers.  IDs are grouped by their first two dot-separated components and
the longest common prefix within each group becomes one candidate identifier.
Candidates already present (or subsumed by an existing entry) are skipped.

Typical usage
-------------
Run from the Zephyr root (ZEPHYR_BASE is inferred automatically):

    python3 scripts/ci/sync_maintainer_tests.py

Preview changes without writing them:

    python3 scripts/ci/sync_maintainer_tests.py --dry-run

Restrict to one area:

    python3 scripts/ci/sync_maintainer_tests.py --area "Drivers: PWM"

Verify existing test identifiers without modifying the file (fast, using a
pre-generated list from twister):

    ./scripts/twister --list-tests > /tmp/tests.txt
    python3 scripts/ci/sync_maintainer_tests.py --verify --tests-file /tmp/tests.txt

Verify using a live global twister run (slow — scans the full tree):

    python3 scripts/ci/sync_maintainer_tests.py --verify
"""

import argparse
import logging
import os
import re
import subprocess
import sys
from pathlib import Path

try:
    from ruamel.yaml import YAML
    from ruamel.yaml.comments import CommentedSeq
except ImportError:
    sys.exit("ruamel.yaml is required. Install with: pip install ruamel.yaml")


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

_GLOB_CHARS = re.compile(r"[*?\[]")
# Namespaces where 2-component grouping is too broad; a third component
# (the driver class, subsystem name, etc.) is required for a useful identifier.
_DEEP_GROUP_PREFIXES: frozenset[str] = frozenset(
    {
        "sample.drivers",
    }
)

# Meta-areas that intentionally cover large swaths of tests/ or samples/.
# Adding auto-generated identifiers to these would be too broad and unhelpful.
_META_AREAS: frozenset[str] = frozenset(
    {
        "Benchmarks",
        "Boards",
        "Samples",
        "Tests",
    }
)


def _is_plain_dir_path(path: str) -> bool:
    """Return True when *path* is a plain (non-glob) directory path."""
    return path.endswith("/") and not _GLOB_CHARS.search(path)


def collect_candidate_paths(area_dict: dict) -> list[str]:
    """Return ``tests/`` and ``samples/`` directory paths from *area_dict*.

    Only plain (no glob characters) directory paths (ending with ``/``) that
    exist under ``tests/`` or ``samples/`` are returned.  The trailing slash
    is stripped so the paths can be passed directly to twister.
    """
    result = []
    for f in area_dict.get("files", []):
        if (f.startswith("tests/") or f.startswith("samples/")) and _is_plain_dir_path(f):
            result.append(f.rstrip("/"))
    return result


def _run_twister_list_tests(zephyr_base: Path, paths: list[str] | None = None) -> list[str]:
    """Run ``twister --list-tests`` and return the discovered test IDs.

    When *paths* is provided, each path is passed as a ``-T`` argument.
    Returns an empty list when twister fails.
    """
    cmd = [str(zephyr_base / "scripts" / "twister"), "--list-tests"]
    if paths:
        for p in paths:
            cmd += ["-T", p]

    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            cwd=str(zephyr_base),
        )
    except OSError as exc:
        logging.warning("Failed to execute twister: %s", exc)
        return []

    ids = []
    for line in result.stdout.splitlines():
        stripped = line.strip()
        if stripped.startswith("- "):
            ids.append(stripped[2:])

    return ids


def get_test_ids(zephyr_base: Path, paths: list[str]) -> list[str]:
    """Run one ``twister --list-tests`` call for all *paths* and return test IDs.

    Each existing path is passed as a separate ``-T`` argument so that twister
    discovers tests across all of them in a single invocation.  Returns an
    empty list when no paths exist or twister fails.
    """
    valid = [p for p in paths if (zephyr_base / p).is_dir()]
    if not valid:
        return []

    ids = _run_twister_list_tests(zephyr_base, valid)
    logging.debug("  %d path(s) -> %d test IDs", len(valid), len(ids))
    return ids


def load_all_test_ids(zephyr_base: Path, tests_file: Path | None) -> set[str]:
    """Return the complete set of test IDs for use during verification.

    When *tests_file* is given its contents are parsed (same ``- <id>`` format
    that ``twister --list-tests`` produces).  Otherwise a global
    ``twister --list-tests`` run (no ``-T`` restriction) is executed, which
    discovers every test in the tree.  This can be slow; prefer supplying a
    pre-generated *tests_file*.
    """
    if tests_file is not None:
        ids: set[str] = set()
        try:
            with open(tests_file) as fh:
                for line in fh:
                    stripped = line.strip()
                    if stripped.startswith("- "):
                        ids.add(stripped[2:])
        except OSError as exc:
            logging.error("Cannot read tests file %s: %s", tests_file, exc)
        return ids

    logging.info("Running global 'twister --list-tests' (this may be slow) …")
    test_list = _run_twister_list_tests(zephyr_base)

    if not test_list:
        logging.error("Failed to execute twister")
        return set()

    ids = set(test_list)
    logging.info("  %d test IDs collected.", len(ids))
    return ids


def common_prefix(test_ids: list[str]) -> str | None:
    """Return the longest common dot-separated prefix across all *test_ids*.

    Returns ``None`` when *test_ids* is empty or there is no common prefix.
    """
    if not test_ids:
        return None

    split = [tid.split(".") for tid in test_ids]
    candidate = split[0]
    for parts in split[1:]:
        # Truncate candidate to the length of the shared leading components.
        new_len = 0
        for a, b in zip(candidate, parts, strict=False):
            if a == b:
                new_len += 1
            else:
                break
        candidate = candidate[:new_len]
        if not candidate:
            return None

    return ".".join(candidate) or None


def get_prefixes(test_ids: list[str]) -> list[str]:
    """Derive a minimal covering set of test-ID prefixes from *test_ids*.

    IDs are grouped by their first two dot-separated components (e.g.
    ``drivers.pwm`` or ``sample.basic``).  Within each group the longest
    common prefix is computed, yielding at most one prefix per group.  This
    preserves the granularity of per-path prefixes while requiring only a
    single combined twister invocation.

    Namespaces listed in ``_DEEP_GROUP_PREFIXES`` (e.g. ``sample.drivers``)
    require a third component for a useful identifier and are therefore
    grouped by their first three components instead.
    """
    groups: dict[str, list[str]] = {}
    for tid in test_ids:
        parts = tid.split(".", 3)
        two = ".".join(parts[:2]) if len(parts) >= 2 else parts[0]
        if two in _DEEP_GROUP_PREFIXES:
            key = ".".join(parts[:3]) if len(parts) >= 3 else two
        else:
            key = two
        groups.setdefault(key, []).append(tid)

    prefixes = []
    for ids in groups.values():
        prefix = common_prefix(ids)
        if prefix:
            prefixes.append(prefix)

    return sorted(prefixes)


# ---------------------------------------------------------------------------
# Main logic
# ---------------------------------------------------------------------------


def process_area(
    area_name: str,
    area_dict: dict,
    zephyr_base: Path,
) -> list[str]:
    """Return a sorted list of new test prefixes to add to *area_name*.

    Issues a single twister invocation for all candidate paths and collects
    prefixes that are not yet in the area's ``tests`` list.
    """
    paths = collect_candidate_paths(area_dict)
    if not paths:
        return []

    existing = set(area_dict.get("tests", []))

    def _is_new(prefix: str, pending: list[str]) -> bool:
        """Return True if *prefix* is not already covered by an existing or
        pending test identifier.  A candidate is covered when an existing entry
        equals it or is a dot-separated prefix of it (e.g. ``sample.foo``
        covers ``sample.foo.bar``).
        """
        return all(not (prefix == e or prefix.startswith(e + ".")) for e in existing | set(pending))

    test_ids = get_test_ids(zephyr_base, paths)
    candidates = get_prefixes(test_ids)

    new_prefixes: list[str] = []
    for prefix in candidates:
        if _is_new(prefix, new_prefixes):
            logging.debug("  new prefix: %s", prefix)
            new_prefixes.append(prefix)

    return new_prefixes


def verify_area(
    area_name: str,
    area_dict: dict,
    all_test_ids: set[str],
) -> list[str]:
    """Return the test identifiers in *area_dict* that match no known test ID.

    An identifier is considered valid when at least one element of
    *all_test_ids* either equals it exactly or starts with it followed by a
    dot (i.e. the identifier is a proper dot-separated prefix of a real test).
    Identifiers that match nothing are returned as-is.
    """
    invalid = []
    for identifier in area_dict.get("tests", []):
        prefix_dot = identifier + "."
        matched = any(tid == identifier or tid.startswith(prefix_dot) for tid in all_test_ids)
        if not matched:
            invalid.append(identifier)
    return invalid


def _ensure_tests_key(area_dict: dict, new_prefixes: list[str]) -> None:
    """Append *new_prefixes* to area_dict['tests'], creating the key if absent."""
    if "tests" not in area_dict:
        seq = CommentedSeq()
        for p in new_prefixes:
            seq.append(p)
        area_dict["tests"] = seq
    else:
        seq = area_dict["tests"]
        last_idx = len(seq) - 1

        # The trailing comment on the last item (typically a blank line that
        # separates areas) must be moved to the new last item; otherwise
        # ruamel.yaml emits it between the old last item and the first new one.
        trailing = None
        if last_idx in seq.ca.items and seq.ca.items[last_idx][0] is not None:
            trailing = seq.ca.items[last_idx][0]
            seq.ca.items[last_idx][0] = None

        for p in new_prefixes:
            seq.append(p)

        if trailing is not None:
            new_last = len(seq) - 1
            if new_last not in seq.ca.items:
                seq.ca.items[new_last] = [None, None, None, None]
            seq.ca.items[new_last][0] = trailing


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print proposed changes without modifying MAINTAINERS.yml.",
    )
    parser.add_argument(
        "--area",
        metavar="AREA",
        help="Restrict processing to a single area name.",
    )
    parser.add_argument(
        "--maintainers",
        default="MAINTAINERS.yml",
        metavar="FILE",
        help="Path to MAINTAINERS.yml relative to ZEPHYR_BASE (default: MAINTAINERS.yml).",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="Enable debug logging.",
    )
    parser.add_argument(
        "--verify",
        action="store_true",
        help=(
            "Verify that every test identifier already recorded in "
            "MAINTAINERS.yml matches at least one real test ID.  "
            "Exits with a non-zero status when invalid identifiers are found.  "
            "Mutually exclusive with --dry-run."
        ),
    )
    parser.add_argument(
        "--tests-file",
        metavar="FILE",
        type=Path,
        help=(
            "Path to a file containing the output of "
            "'./scripts/twister --list-tests'.  "
            "Used by --verify to avoid a slow global twister run.  "
            "Generate with: ./scripts/twister --list-tests > tests.txt"
        ),
    )
    args = parser.parse_args()

    if args.verify and args.dry_run:
        parser.error("--verify and --dry-run are mutually exclusive")

    logging.basicConfig(
        format="%(levelname)s: %(message)s",
        level=logging.DEBUG if args.verbose else logging.INFO,
    )

    zephyr_base = Path(os.environ.get("ZEPHYR_BASE", Path(__file__).parents[2]))
    maintainers_path = zephyr_base / args.maintainers

    if not maintainers_path.exists():
        logging.error("MAINTAINERS file not found: %s", maintainers_path)
        return 1

    yaml = YAML()
    yaml.preserve_quotes = True
    yaml.indent(mapping=2, sequence=4, offset=2)
    yaml.width = 4096  # Prevent line wrapping when writing back

    with open(maintainers_path) as fh:
        data = yaml.load(fh)

    # ------------------------------------------------------------------
    # Verification mode: check that existing test identifiers are valid.
    # ------------------------------------------------------------------
    if args.verify:
        all_test_ids = load_all_test_ids(zephyr_base, args.tests_file)
        if not all_test_ids:
            logging.error("No test IDs available for verification; aborting.")
            return 1

        total_areas = 0
        invalid_count = 0

        for area_name, area_dict in data.items():
            if not isinstance(area_dict, dict):
                continue
            if args.area and area_name != args.area:
                continue
            if not area_dict.get("tests"):
                continue

            total_areas += 1
            invalid = verify_area(area_name, area_dict, all_test_ids)
            if invalid:
                for ident in invalid:
                    logging.warning(
                        "[%s] invalid test identifier (no matching test): %s",
                        area_name,
                        ident,
                    )
                invalid_count += len(invalid)
            else:
                logging.debug("  [%s] all identifiers valid", area_name)

        if invalid_count:
            logging.error(
                "Verification failed: %d invalid identifier(s) across %d area(s).",
                invalid_count,
                total_areas,
            )
            return 1

        logging.info(
            "Verification passed: all test identifiers in %d area(s) are valid.",
            total_areas,
        )
        return 0

    # ------------------------------------------------------------------
    # Normal sync mode.
    # ------------------------------------------------------------------
    total_areas = 0
    updated_areas = 0

    for area_name, area_dict in data.items():
        if not isinstance(area_dict, dict):
            continue
        if args.area and area_name != args.area:
            continue

        total_areas += 1
        logging.info("Processing: %s", area_name)

        if area_name in _META_AREAS:
            logging.debug("  [%s] skipping meta-area", area_name)
            continue

        new_prefixes = process_area(area_name, area_dict, zephyr_base)

        if new_prefixes:
            updated_areas += 1
            logging.info("  [%s] adding: %s", area_name, new_prefixes)
            if not args.dry_run:
                _ensure_tests_key(area_dict, new_prefixes)
        else:
            logging.debug("  [%s] nothing new to add", area_name)

    logging.info(
        "Scanned %d area(s); %d area(s) %s new test identifier(s).",
        total_areas,
        updated_areas,
        "would receive" if args.dry_run else "received",
    )

    if not args.dry_run and updated_areas:
        with open(maintainers_path, "w") as fh:
            yaml.dump(data, fh)
        logging.info("Wrote updated %s", maintainers_path)

    return 0


if __name__ == "__main__":
    sys.exit(main())
