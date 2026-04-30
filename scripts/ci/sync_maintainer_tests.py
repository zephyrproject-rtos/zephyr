#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright The Zephyr Project Contributors

"""Sync test identifiers in MAINTAINERS.yml based on actual test coverage.

For each area in MAINTAINERS.yml that has tests/ or samples/ directory paths
in its ``files`` section, this script runs ``twister --list-tests`` and
extracts a common test identifier (the longest dot-separated prefix shared by
all test IDs returned for that path).  Any identifier that is not yet listed
in the area's ``tests`` section is appended automatically.

Typical usage
-------------
Run from the Zephyr root (ZEPHYR_BASE is inferred automatically):

    python3 scripts/ci/sync_maintainer_tests.py

Preview changes without writing them:

    python3 scripts/ci/sync_maintainer_tests.py --dry-run

Restrict to one area:

    python3 scripts/ci/sync_maintainer_tests.py --area "PWM Drivers"

Speed up processing with more parallel workers:

    python3 scripts/ci/sync_maintainer_tests.py --jobs 8
"""

import argparse
import concurrent.futures
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


def get_test_ids(zephyr_base: Path, rel_path: str) -> list[str]:
    """Run ``twister --list-tests -T <rel_path>`` and return test IDs.

    Returns an empty list when the path does not exist or twister fails.
    """
    full_path = zephyr_base / rel_path
    if not full_path.is_dir():
        logging.debug("Path does not exist, skipping: %s", rel_path)
        return []

    cmd = [
        str(zephyr_base / "scripts" / "twister"),
        "--list-tests",
        "-T",
        rel_path,
    ]
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            cwd=str(zephyr_base),
        )
    except OSError as exc:
        logging.warning("Failed to execute twister for %s: %s", rel_path, exc)
        return []

    ids = []
    for line in result.stdout.splitlines():
        stripped = line.strip()
        if stripped.startswith("- "):
            ids.append(stripped[2:])

    logging.debug("  %s -> %d test IDs", rel_path, len(ids))
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


def prefix_for_path(zephyr_base: Path, rel_path: str) -> str | None:
    """Return the common test-ID prefix for *rel_path*, or ``None``."""
    test_ids = get_test_ids(zephyr_base, rel_path)
    return common_prefix(test_ids)


# ---------------------------------------------------------------------------
# Main logic
# ---------------------------------------------------------------------------


def process_area(
    area_name: str,
    area_dict: dict,
    zephyr_base: Path,
    jobs: int,
) -> list[str]:
    """Return a sorted list of new test prefixes to add to *area_name*.

    Runs twister for all candidate paths (possibly in parallel) and collects
    prefixes that are not yet in the area's ``tests`` list.
    """
    paths = collect_candidate_paths(area_dict)
    if not paths:
        return []

    existing = set(area_dict.get("tests", []))
    new_prefixes = []

    if jobs > 1 and len(paths) > 1:
        with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
            futures = {pool.submit(prefix_for_path, zephyr_base, p): p for p in paths}
            for future in concurrent.futures.as_completed(futures):
                path = futures[future]
                try:
                    prefix = future.result()
                except Exception as exc:  # noqa: BLE001
                    logging.warning("Error processing %s: %s", path, exc)
                    continue
                if prefix and prefix not in existing and prefix not in new_prefixes:
                    logging.debug("  %s -> new prefix: %s", path, prefix)
                    new_prefixes.append(prefix)
    else:
        for path in paths:
            prefix = prefix_for_path(zephyr_base, path)
            if prefix and prefix not in existing and prefix not in new_prefixes:
                logging.debug("  %s -> new prefix: %s", path, prefix)
                new_prefixes.append(prefix)

    return sorted(new_prefixes)


def _ensure_tests_key(area_dict: dict, new_prefixes: list[str]) -> None:
    """Append *new_prefixes* to area_dict['tests'], creating the key if absent."""
    if "tests" not in area_dict:
        seq = CommentedSeq(new_prefixes)
        area_dict["tests"] = seq
    else:
        area_dict["tests"].extend(new_prefixes)


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
        "--jobs",
        type=int,
        default=4,
        metavar="N",
        help="Number of parallel twister invocations (default: 4).",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="Enable debug logging.",
    )
    args = parser.parse_args()

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

    total_areas = 0
    updated_areas = 0

    for area_name, area_dict in data.items():
        if not isinstance(area_dict, dict):
            continue
        if args.area and area_name != args.area:
            continue

        total_areas += 1
        logging.info("Processing: %s", area_name)

        new_prefixes = process_area(area_name, area_dict, zephyr_base, args.jobs)

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
