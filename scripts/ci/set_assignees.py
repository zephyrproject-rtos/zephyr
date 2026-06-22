#!/usr/bin/env python3

# Copyright (c) 2022 Intel Corp.
# SPDX-License-Identifier: Apache-2.0

"""Assign reviewers, maintainers, and labels to new PRs based on MAINTAINERS.yml.

Overview
--------
This script automates three housekeeping tasks on newly opened GitHub pull
requests (and issues):

  1. Apply area labels derived from MAINTAINERS.yml.
  2. Request reviews from the relevant maintainers and collaborators.
  3. Assign a primary maintainer as the PR assignee.

It reads MAINTAINERS.yml (or a custom file via -M) to map file paths to named
areas.  Each area carries a list of maintainers, collaborators, labels, and
file/path patterns.  The script uses the PyGithub library to interact with the
GitHub API and requires a personal access token in the GITHUB_TOKEN environment
variable.

Labeling strategy
-----------------
For every file changed in the PR, MAINTAINERS.yml is consulted to find the
matching areas.  The union of all area labels across all matched areas is
collected and applied to the PR, subject to a cap of MAX_LABELS (10).  If more
than MAX_LABELS distinct labels would be applied, the entire label step is
skipped to avoid polluting PRs that touch very broad cross-cutting areas.

A special lightweight "size: XS" label is managed by default:
  - Added when: the PR has exactly one commit AND at most one line added AND
    at most one line deleted AND does not touch any manifest file
    (west.yml, submanifests/optional.yaml).
  - Removed when: the PR already carries "size: XS" but no longer qualifies
    (e.g. after a rebase that added more commits or lines).

When --size-labels is given, the full size label suite is managed instead
of only "size: XS".  The suite maps total changed lines (additions +
deletions) to one of five mutually exclusive labels:

  size: XS  -- 1 commit, ≤1 line changed, no manifest file touched
  size: S   -- ≤9 lines changed
  size: M   -- ≤49 lines changed
  size: L   -- ≤499 lines changed
  size: XL  -- >499 lines changed

Any existing size label that does not match the computed bucket is removed
before the new label is applied, keeping exactly one size label on the PR
at all times.

Area weighting
--------------
Each changed file contributes a weight of 1 to the areas it belongs to, with
two exceptions that contribute 0:

  - CMakeLists.txt files: build-system boilerplate present in nearly every
    directory, not a reliable signal for area ownership.
  - Meta-areas (Documentation, Samples, Tests, Release Notes, Release): used
    only for assignee selection when they are the *sole* area touched (see
    below); not weighted for mixed PRs.

Platform (driver/board) areas receive a weight of 1 for the first file that
maps to them.  Subsequent files that also map to the *same* Platform area
score 0 to avoid double-counting.  This is controlled by the is_instance flag:
once a Platform area has been seen for a given file's sorted_areas list, all
further areas for that file score 0.

The resulting per-area weights drive both reviewer selection (ordered by
weight) and assignee selection.

Reviewer selection
------------------
The ordered reviewer candidate list is built as follows:

  1. For each area in descending weight order: add the area's maintainers,
     then its collaborators.
  2. Append any path-specific collaborators returned by
     get_collaborators_for_path() for each matched file.
  3. Append any extra reviewers identified from MAINTAINERS.yml diff (see
     Manifest / MAINTAINERS.yml change handling below).
  4. Deduplicate while preserving insertion order.

Candidates are then filtered:
  - The PR author is skipped.
  - Users who already submitted a review or are already on the review-request
    list are skipped.
  - Users who are not repository collaborators are skipped (GitHub requires
    collaborator status to receive a review request).
  - Users who previously self-removed themselves from the review request list
    are skipped.

If the PR already has MAX_REVIEWERS (15) or more reviewers, the normal
candidate list is discarded.  Only the maintainers of the *primary*
(highest-weight) area are used as fallback candidates, plus any
``additional_reviews`` users identified from manifest or MAINTAINERS.yml
diff.  This keeps the fallback small and high-signal, and avoids the
original behaviour of requesting reviews from all area maintainers across
all touched files (which could push a broad PR even further above the
reviewer cap).  The same author / existing-reviewer / self-removed /
collaborator-status filters are applied to the fallback candidates.

Otherwise the candidate list is capped at the remaining vacancy
(MAX_REVIEWERS - existing reviewer count) before the review request is created.

Assignee selection
------------------
Assignees are selected from area_counter (areas sorted by descending file
weight) using the following priority rules:

  1. If exactly one area is matched and it is a meta-area (Documentation,
     Samples, Tests, Release Notes, Release), that area's maintainers are
     assigned directly.  Meta-areas are not considered in mixed PRs.
  2. Areas with a weight of 0 or no maintainers are skipped.
  3. If the PR author is the area's only maintainer, the area is skipped
     (to avoid self-assignment).  If the author is one of several maintainers,
     they are excluded from the candidate list and the remaining maintainers
     are used.
  4. Non-platform areas (no "Platform" in the area name) take highest
     priority: the first such area's maintainers are inserted at the front
     of the ranked list, and the search stops.
  5. Platform areas (drivers, boards) are appended as lower-priority
     fallbacks.

After iterating all areas, the first entry in the ranked list is chosen.
If the ranking process yielded nothing (e.g. all areas had the author as sole
maintainer), the maintainer with the highest cumulative file-weight score is
used as a last resort.

Assignees are only set when the PR currently has no assignee.

Manifest / MAINTAINERS.yml change handling
-------------------------------------------
west.yml and submanifests/optional.yaml:
  If --updated-manifest is provided, the old and new manifest files are
  compared to find added, removed, and updated west projects.  Each changed
  project is looked up in MAINTAINERS.yml under "West project: <name>" and
  the corresponding collaborators are added to the reviewer candidate list.

MAINTAINERS.yml:
  If --updated-maintainer-file is provided, the old and new versions of
  MAINTAINERS.yml are diffed field-by-field (maintainers, collaborators,
  labels, files, files-regex, status).  Maintainers of every area that
  changed are appended to the reviewer candidate list so that the people
  responsible for each changed area are automatically notified.

Issue assignment
----------------
When run with -I/--issue, the script matches the issue's GitHub labels against
the area labels in MAINTAINERS.yml and assigns the maintainers of the matching
area.  A single unambiguous label is sufficient for a match; multi-label areas
require all their labels to be present.  If no match is found, or the issue
already has assignees, the script exits without making changes.

Module PR assignment
--------------------
When run with -m/--modules, the script reads the active west manifest,
locates every active non-manifest project that has a "West project: <name>"
entry in MAINTAINERS.yml with at least one maintainer, and searches GitHub for
open, non-draft, unassigned PRs across all those repos.  Each PR is assigned
to the project maintainers and review requests are created for both maintainers
and collaborators.
"""

import argparse
import datetime
import logging
import os
import sys
import time
from collections import defaultdict
from pathlib import Path

import yaml
from github import Auth, Github, GithubException
from github.GithubException import UnknownObjectException
from west.manifest import Manifest, ManifestProject

TOP_DIR = os.path.join(os.path.dirname(__file__))
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from get_maintainer import Maintainers  # noqa: E402

ZEPHYR_BASE = os.environ.get('ZEPHYR_BASE')
if ZEPHYR_BASE:
    ZEPHYR_BASE = Path(ZEPHYR_BASE)
else:
    ZEPHYR_BASE = Path(__file__).resolve().parents[2]
    # Propagate this decision to child processes.
    os.environ['ZEPHYR_BASE'] = str(ZEPHYR_BASE)

logger = logging.getLogger(__name__)

# Maximum number of changed files to process; larger PRs are skipped.
MAX_FILES = 500

# Maximum number of reviewers on a PR before switching to maintainer-only strategy.
MAX_REVIEWERS = 15

# Maximum number of labels to apply; more than this is likely noise from over-broad matches.
MAX_LABELS = 10

# Courtesy sleep between consecutive GitHub API calls to avoid secondary rate limits.
API_SLEEP_SECONDS = 1

# Areas where assignee selection only fires when they are the sole area affected.
META_AREAS = frozenset(['Release Notes', 'Documentation', 'Samples', 'Tests', 'Release'])


def parse_args():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )

    parser.add_argument(
        "-M",
        "--maintainer-file",
        required=False,
        default="MAINTAINERS.yml",
        help="Maintainer file to be used.",
    )

    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        "-P",
        "--pull_request",
        required=False,
        default=None,
        type=int,
        help="Operate on one pull-request only.",
    )
    group.add_argument(
        "-I", "--issue", required=False, default=None, type=int, help="Operate on one issue only."
    )
    group.add_argument("-s", "--since", required=False, help="Process pull-requests since date.")
    group.add_argument(
        "-m", "--modules", action="store_true", help="Process pull-requests from modules."
    )

    parser.add_argument("-y", "--dry-run", action="store_true", default=False, help="Dry run only.")

    parser.add_argument("-o", "--org", default="zephyrproject-rtos", help="Github organisation")

    parser.add_argument("-r", "--repo", default="zephyr", help="Github repository")

    parser.add_argument(
        "--updated-manifest",
        default=None,
        help="Updated manifest file to compare against current west.yml",
    )

    parser.add_argument(
        "--updated-maintainer-file",
        default=None,
        help="Updated maintainer file to compare against current MAINTAINERS.yml",
    )

    parser.add_argument(
        "-v",
        "--verbose",
        action="count",
        default=0,
        help="Verbose output. Use -v for INFO, -vv for DEBUG.",
    )

    parser.add_argument(
        "--size-labels",
        action="store_true",
        default=False,
        help=(
            "Manage the full size label suite (size: XS/S/M/L/XL) instead of "
            "only the default size: XS label."
        ),
    )

    return parser.parse_args()


def setup_logging(verbose: int):
    """Configure logging verbosity.

    -v   -> INFO level (operational progress)
    -vv  -> DEBUG level (per-file and per-area detail)
    """
    if verbose >= 2:
        level = logging.DEBUG
    elif verbose == 1:
        level = logging.INFO
    else:
        level = logging.WARNING

    logging.basicConfig(
        format="%(levelname)s: %(message)s",
        level=level,
        stream=sys.stdout,
    )


def load_areas(filename: str) -> dict:
    """Load MAINTAINERS YAML and return only areas that define file-matching patterns."""
    with open(filename) as f:
        doc = yaml.safe_load(f)
    return {
        k: v for k, v in doc.items() if isinstance(v, dict) and ("files" in v or "files-regex" in v)
    }


def process_manifest(old_manifest_file: str) -> list:
    """Return area name strings for projects that changed between two west.yml files."""
    logger.info("Processing manifest changes")

    if not os.path.isfile("west.yml"):
        logger.warning("west.yml not found; skipping manifest processing")
        return []
    if not os.path.isfile(old_manifest_file):
        logger.warning(
            "Old manifest '%s' not found; skipping manifest processing", old_manifest_file
        )
        return []

    old_manifest = Manifest.from_file(old_manifest_file)
    new_manifest = Manifest.from_file("west.yml")

    old_projs = {(p.name, p.revision) for p in old_manifest.projects}
    new_projs = {(p.name, p.revision) for p in new_manifest.projects}

    old_names = {name for name, _ in old_projs}
    new_names = {name for name, _ in new_projs}

    # Projects whose name no longer appears in the new manifest.
    removed = {(n, r) for n, r in old_projs if n not in new_names}
    # Projects that exist in both manifests but with a different revision.
    updated = {(n, r) for n, r in new_projs if n in old_names and (n, r) not in old_projs}
    # Entirely new projects.
    added = {(n, r) for n, r in new_projs if n not in old_names}

    changed_names = sorted({n for n, _ in removed | updated | added})
    logger.info("Modified west projects: %s", changed_names)

    areas = [f"West project: {name}" for name in changed_names]
    logger.debug("Manifest areas: %s", areas)
    return areas


def set_or_empty(d: dict, key: str) -> set:
    return set(d.get(key, []) or [])


def _diff_area_entry(old_entry: dict, new_entry: dict) -> list:
    """Return human-readable change strings between two MAINTAINERS area entries."""
    changes = []
    fields = [
        ("maintainers", "Maintainers"),
        ("collaborators", "Collaborators"),
        ("labels", "Labels"),
        ("files", "Files"),
        ("files-regex", "files-regex"),
    ]
    for key, label in fields:
        added = set_or_empty(new_entry, key) - set_or_empty(old_entry, key)
        removed = set_or_empty(old_entry, key) - set_or_empty(new_entry, key)
        if added:
            changes.append(f"{label} added: {', '.join(sorted(added))}")
        if removed:
            changes.append(f"{label} removed: {', '.join(sorted(removed))}")

    old_status = old_entry.get("status")
    new_status = new_entry.get("status")
    if old_status != new_status:
        changes.append(f"Status changed: {old_status} -> {new_status}")

    return changes


def compare_areas(old: dict, new: dict) -> set:
    """Compare two MAINTAINERS area dicts; return the set of added, removed, or changed names."""
    old_areas = set(old.keys())
    new_areas = set(new.keys())

    added_areas = new_areas - old_areas
    removed_areas = old_areas - new_areas
    common_areas = old_areas & new_areas

    if added_areas:
        logger.info("Areas added (%d):", len(added_areas))
        for area in sorted(added_areas):
            logger.info("  + %s", area)

    if removed_areas:
        logger.info("Areas removed (%d):", len(removed_areas))
        for area in sorted(removed_areas):
            logger.info("  - %s", area)

    changed_areas = set()
    for area in sorted(common_areas):
        changes = _diff_area_entry(old[area], new[area])
        if changes:
            changed_areas.add(area)
            logger.info("Area changed: %s", area)
            for change in changes:
                logger.debug("  %s", change)

    return added_areas | removed_areas | changed_areas


def _pick_assignees(pr, area_counter: dict, all_maintainers: dict, num_files: int):
    """Select the best assignee list for *pr* from area and file-coverage data.

    Iterates areas in descending file-change count.  Non-platform areas take
    priority; platform (driver/board) areas are only used as a fallback.
    Returns a list of login strings, or None if no suitable assignee is found.
    """
    ranked_assignees = []
    assignees = None

    for area, count in area_counter.items():
        if count == 0 or not area.maintainers:
            continue

        if pr.user.login in area.maintainers:
            # Author is a maintainer; try to assign someone else from the same area.
            if len(area.maintainers) > 1:
                candidates = area.maintainers.copy()
                candidates.remove(pr.user.login)
                assignees = candidates
            else:
                # Author is the sole maintainer; skip this area.
                continue
        else:
            assignees = area.maintainers

        # FIXME: identify platform areas and other areas using some flag in the
        # MAINTAINERS.yml data instead of relying on the name containing "Platform".
        if 'platform' in area.name.lower():
            logger.debug(
                "Platform area '%s' with maintainers %s added as fallback for assignment",
                area.name,
                assignees,
            )
            ranked_assignees.append(assignees)
        elif area.name not in META_AREAS:
            logger.debug(
                "Non-platform area '%s' with maintainers %s takes priority for assignment",
                area.name,
                assignees,
            )
            # Non-platform area: highest priority — insert at front and stop.
            ranked_assignees.insert(0, assignees)
            break

    if ranked_assignees:
        assignees = ranked_assignees[0]

    if assignees:
        coverage = (all_maintainers.get(assignees[0], 0) / num_files) * 100 if num_files else 0
        logger.info("Picked assignees: %s (%.2f%% file coverage)", assignees, coverage)
    elif all_maintainers:
        # No area-based pick succeeded; fall back to the maintainer with most file changes.
        assignees = [next(iter(all_maintainers))]
        logger.info("Fallback assignee (highest file count): %s", assignees)

    return assignees


def _add_reviewers(
    gh,
    gh_repo,
    pr,
    args,
    collab: list,
    primary_maintainers: list,
    extra_reviewers: frozenset = frozenset(),
):
    """Request reviews from eligible collaborators on *pr*.

    Skips the PR author, existing reviewers, non-collaborators, and users
    who previously removed themselves from the review request.

    When the PR already has MAX_REVIEWERS or more reviewers, only
    *primary_maintainers* (the maintainers of the highest-weight area) plus
    *extra_reviewers* (e.g. maintainers of MAINTAINERS.yml-changed areas) are
    used as fallback candidates.  This keeps the fallback small and
    high-signal while ensuring that people specifically responsible for
    changed areas are never silently dropped.
    The same filters apply in both the normal and overflow paths.
    """
    existing_reviewers = set()
    for review in pr.get_reviews():
        existing_reviewers.add(review.user)

    # get_review_requests() returns (PaginatedList[NamedUser], PaginatedList[Team]).
    review_users, _review_teams = pr.get_review_requests()
    existing_reviewers.update(review_users)

    logger.debug(
        "Existing reviewers: %s", [u.login for u in existing_reviewers if hasattr(u, 'login')]
    )

    # Track users who removed themselves; do not re-add them.
    self_removed = set()
    for event in pr.get_issue_events():
        if event.event == 'review_request_removed' and event.actor == event.requested_reviewer:
            self_removed.add(event.actor)

    if len(existing_reviewers) >= MAX_REVIEWERS:
        logger.info(
            "PR #%d already has %d reviewer(s) (limit %d); "
            "restricting candidates to primary area maintainers",
            pr.number,
            len(existing_reviewers),
            MAX_REVIEWERS,
        )
        # Preserve extra_reviewers (e.g. MAINTAINERS.yml-change reviewers)
        # even in the overflow path so they are never silently dropped.
        candidates = list(dict.fromkeys(primary_maintainers + sorted(extra_reviewers)))
        vacancy = None
    else:
        candidates = collab  # extra_reviewers already included via process_pr
        vacancy = MAX_REVIEWERS - len(existing_reviewers)

    reviewers = []
    for collaborator in candidates:
        try:
            gh_user = gh.get_user(collaborator)
        except UnknownObjectException:
            logger.warning("User '%s' not found; account may have been deleted", collaborator)
            continue

        if pr.user == gh_user:
            logger.debug("Skipping PR author '%s'", collaborator)
            continue
        if gh_user in existing_reviewers:
            logger.debug("Skipping existing reviewer '%s'", collaborator)
            continue
        if not gh_repo.has_in_collaborators(gh_user):
            logger.info("Skipping '%s': not a repository collaborator", collaborator)
            continue
        if gh_user in self_removed:
            logger.info("Skipping '%s': previously self-removed from reviewers", collaborator)
            continue

        reviewers.append(collaborator)

    if vacancy is not None:
        reviewers = reviewers[:vacancy]

    if reviewers:
        logger.info("Requesting reviews from: %s", reviewers)
        if not args.dry_run:
            try:
                pr.create_review_request(reviewers=reviewers)
            except GithubException as exc:
                logger.error("Failed to add reviewers %s: %s", reviewers, exc)


def _assign_maintainers(gh, pr, args, assignees: list):
    """Add *assignees* (login strings) to *pr*, logging any unknown-user errors."""
    users = []
    for login in assignees:
        try:
            users.append(gh.get_user(login))
        except GithubException as exc:
            logger.error("Unknown user '%s': %s", login, exc)

    for user in users:
        logger.info("Adding assignee: %s", user.login)
        if not args.dry_run:
            pr.add_to_assignees(user)


# Manifest files are never considered trivial regardless of line count.
_MANIFEST_FILES = frozenset({'west.yml', 'submanifests/optional.yaml'})

# All size labels managed by this script; used to remove stale buckets.
_SIZE_LABELS = frozenset({'size: XS', 'size: S', 'size: M', 'size: L', 'size: XL'})

# (max total lines changed inclusive, label name) in ascending order.
# XS is handled separately by update_size_xs_label / update_size_labels.
_SIZE_THRESHOLDS = (
    (9, 'size: S'),
    (49, 'size: M'),
    (499, 'size: L'),
)


def update_size_xs_label(pr, args, changed_files: list, labels: set):
    """Add or remove the 'size: XS' label based on PR size and changed files.

    A PR qualifies for 'size: XS' when all of the following are true:
      - Exactly one commit.
      - At most one line added and one line deleted.
      - No manifest file (west.yml, submanifests/optional.yaml) is changed.
    """
    touches_manifest = any(f.filename in _MANIFEST_FILES for f in changed_files)
    current_label_names = {label.name for label in pr.labels}
    qualifies_for_xs = (
        pr.commits == 1 and pr.additions <= 1 and pr.deletions <= 1 and not touches_manifest
    )

    if qualifies_for_xs:
        labels.add('size: XS')
    elif 'size: XS' in current_label_names:
        logger.info(
            "Removing 'size: XS' label from PR #%d (commits=%d, +%d/-%d)",
            pr.number,
            pr.commits,
            pr.additions,
            pr.deletions,
        )
        if not args.dry_run:
            pr.remove_from_labels('size: XS')


def update_size_labels(pr, args, changed_files: list, labels: set):
    """Manage the full size label suite (XS/S/M/L/XL) on *pr*.

    Computes the correct size bucket, removes any existing size labels that
    belong to a different bucket, and queues the correct label for addition.
    The XS bucket inherits the same stricter qualification rules as
    update_size_xs_label (single commit, ≤1 line, no manifest files).
    """
    total_lines = pr.additions + pr.deletions
    touches_manifest = any(f.filename in _MANIFEST_FILES for f in changed_files)

    qualifies_for_xs = (
        pr.commits == 1 and pr.additions <= 1 and pr.deletions <= 1 and not touches_manifest
    )

    if qualifies_for_xs:
        correct_label = 'size: XS'
    else:
        correct_label = 'size: XL'
        for max_lines, label in _SIZE_THRESHOLDS:
            if total_lines <= max_lines:
                correct_label = label
                break

    current_label_names = {lbl.name for lbl in pr.labels}
    stale = _SIZE_LABELS & current_label_names - {correct_label}

    for label in sorted(stale):
        logger.info(
            "Removing stale size label '%s' from PR #%d",
            label,
            pr.number,
        )
        if not args.dry_run:
            pr.remove_from_labels(label)

    labels.add(correct_label)
    logger.info(
        "PR #%d size: %s (+%d/-%d, %d total line(s), %d commit(s))",
        pr.number,
        correct_label,
        pr.additions,
        pr.deletions,
        total_lines,
        pr.commits,
    )


def process_pr(gh, args, maintainer_file, number: int):
    gh_repo = gh.get_repo(f"{args.org}/{args.repo}")
    pr = gh_repo.get_pull(number)
    pr_url = f"https://github.com/{args.org}/{args.repo}/pull/{pr.number}"

    logger.info("Processing PR #%d: %s  (%s)", pr.number, pr.title, pr_url)

    if pr.draft:
        logger.info("PR #%d is a draft; skipping", pr.number)
        return

    if pr.state != 'open':
        logger.info("PR #%d is %s; skipping", pr.number, pr.state)
        return

    labels = set()
    area_counter = defaultdict(int)
    found_maintainers = defaultdict(int)
    collab_per_path = set()
    additional_reviews = set()

    changed_files = list(pr.get_files())
    num_files = len(changed_files)

    if num_files > MAX_FILES:
        logger.warning(
            "PR #%d has %d changed files (limit %d); skipping",
            pr.number,
            num_files,
            MAX_FILES,
        )
        return

    if args.size_labels:
        update_size_labels(pr, args, changed_files, labels)
    else:
        update_size_xs_label(pr, args, changed_files, labels)

    for changed_file in changed_files:
        filename = changed_file.filename
        logger.debug("Processing file: %s", filename)

        areas = []
        if filename in ('west.yml', 'submanifests/optional.yaml'):
            if not args.updated_manifest:
                logger.debug(
                    "No --updated-manifest provided; skipping manifest diff for %s", filename
                )
                continue
            parsed_areas = process_manifest(old_manifest_file=args.updated_manifest)
            for area_name in parsed_areas:
                area_matches = maintainer_file.name2areas(area_name)
                if area_matches:
                    collab_per_path.update(area_matches[0].get_collaborators_for_path(filename))
                    areas.extend(area_matches)

        elif filename == 'MAINTAINERS.yml':
            areas = maintainer_file.path2areas(filename)
            if args.updated_maintainer_file:
                old_areas = load_areas(args.updated_maintainer_file)
                new_areas = load_areas('MAINTAINERS.yml')
                changed_area_names = compare_areas(old_areas, new_areas)
                for area_name in changed_area_names:
                    area_matches = maintainer_file.name2areas(area_name)
                    if area_matches:
                        additional_reviews.update(maintainer_file.areas[area_name].maintainers)
                logger.info(
                    "MAINTAINERS.yml changed; adding extra reviewers: %s",
                    sorted(additional_reviews),
                )

        else:
            areas = maintainer_file.path2areas(filename)
            for area in areas:
                collab_per_path.update(area.get_collaborators_for_path(filename))

        logger.debug("  areas for %s: %s", filename, [a.name for a in areas])

        if not areas:
            continue

        # Sort so that Platform (driver/board) areas are processed first.  This
        # sets is_instance=True early, preventing the same file from being
        # counted again for the corresponding subsystem area.
        sorted_areas = sorted(areas, key=lambda x: 'Platform' in x.name, reverse=True)
        is_instance = False

        for area in sorted_areas:
            # CMakeLists.txt changes and meta-area files do not count toward
            # the area weight used for assignee selection.
            if 'CMakeLists.txt' in filename or area.name in META_AREAS:
                count = 0
            else:
                # Once an instance (Platform) area has been seen, subsequent
                # areas for this file score 0 to avoid double-counting.
                count = 1 if not is_instance else 0

            area_counter[area] += count
            logger.debug("  area weight update: %s += %d", area.name, count)
            labels.update(area.labels)

            # NOTE: a file appearing in multiple areas with the same maintainer
            # will over-count that maintainer's file coverage score.
            for maintainer in area.maintainers:
                found_maintainers[maintainer] += count

            if 'Platform' in area.name:
                is_instance = True

        # Collect path-specific collaborators for all areas that matched this file.
        for area in sorted_areas:
            collab_per_path.update(area.get_collaborators_for_path(filename))

    area_counter = dict(sorted(area_counter.items(), key=lambda item: item[1], reverse=True))
    logger.info("Area weights: %s", {a.name: c for a, c in area_counter.items()})
    logger.debug("Collected labels: %s", labels)

    # Build the ordered collaborator/reviewer list by area priority.
    collab = []
    for area in area_counter:
        collab += maintainer_file.areas[area.name].maintainers
        collab += maintainer_file.areas[area.name].collaborators
    collab += list(collab_per_path)
    collab += list(additional_reviews)
    # Deduplicate while preserving insertion order.
    collab = list(dict.fromkeys(collab))
    logger.debug("Reviewer candidates: %s", collab)

    all_maintainers = dict(
        sorted(found_maintainers.items(), key=lambda item: item[1], reverse=True)
    )
    logger.info("PR submitted by: %s", pr.user.login)
    logger.info("Maintainer file-coverage scores: %s", all_maintainers)

    assignees = _pick_assignees(pr, area_counter, all_maintainers, num_files)

    # Apply labels — skip any that are already present, then add the rest in
    # a single API call to avoid per-label timeline noise.
    if labels:
        if len(labels) <= MAX_LABELS:
            current_label_names = {lbl.name for lbl in pr.labels}
            new_labels = sorted(labels - current_label_names)
            if new_labels:
                logger.info("Adding labels: %s", new_labels)
                if not args.dry_run:
                    pr.add_to_labels(*new_labels)
            else:
                logger.info("All labels already present on PR #%d; skipping", pr.number)
        else:
            logger.warning(
                "Too many labels (%d) for PR #%d; skipping label assignment",
                len(labels),
                pr.number,
            )

    # Request reviews.
    if collab or additional_reviews:
        primary_area = next(iter(area_counter), None)
        primary_maintainers = (
            maintainer_file.areas[primary_area.name].maintainers if primary_area else []
        )
        _add_reviewers(
            gh,
            gh_repo,
            pr,
            args,
            collab,
            primary_maintainers,
            frozenset(additional_reviews),
        )

    # Set assignees (only when none are set yet, unless doing a dry run).
    if assignees and (not pr.assignee or args.dry_run):
        _assign_maintainers(gh, pr, args, assignees)
    else:
        reason = "already has assignee" if pr.assignee else "no assignees found"
        logger.info("Not setting assignee for PR #%d: %s", pr.number, reason)

    time.sleep(API_SLEEP_SECONDS)


def process_issue(gh, args, maintainer_file, number: int):
    gh_repo = gh.get_repo(f"{args.org}/{args.repo}")
    issue = gh_repo.get_issue(number)

    logger.info("Processing issue #%d: %s  (%s)", issue.number, issue.title, issue.html_url)

    if issue.assignees:
        logger.warning(
            "Issue #%d already has assignees (%s); skipping",
            issue.number,
            [a.login for a in issue.assignees],
        )
        return

    # Build a mapping from sorted label-name tuples to maintainer sets.
    label_to_maintainer = defaultdict(set)
    for _, area in maintainer_file.areas.items():
        if not area.labels:
            continue
        label_tuple = tuple(sorted(label.lower() for label in area.labels))
        for maintainer in area.maintainers:
            label_to_maintainer[label_tuple].add(maintainer)

    # Also allow matching on a single label when it is unambiguous enough.
    for label_tuple, maintainers in dict(label_to_maintainer).items():
        for label in label_tuple:
            single = (label,)
            if single not in label_to_maintainer:
                label_to_maintainer[single] = maintainers

    valid_labels = []
    for label in issue.labels:
        label_name = label.name.lower()
        if (label_name,) not in label_to_maintainer:
            logger.debug("Ignoring label '%s': no area match", label.name)
        else:
            valid_labels.append(label_name)
    issue_labels = tuple(sorted(valid_labels))

    logger.info("Matched labels: %s", issue_labels)

    if not issue_labels or issue_labels not in label_to_maintainer:
        logger.warning("No matching label set found for issue #%d; not assigning", issue.number)
        return

    for maintainer in label_to_maintainer[issue_labels]:
        logger.info("Assigning '%s' to issue #%d (%s)", maintainer, issue.number, issue.html_url)
        if not args.dry_run:
            issue.add_to_assignees(maintainer)


def process_modules(gh, args, maintainers_file):
    manifest = Manifest.from_file()

    repos = {}
    for project in manifest.get_projects([]):
        if not manifest.is_active(project) or isinstance(project, ManifestProject):
            continue

        area_name = f"West project: {project.name}"
        if area_name not in maintainers_file.areas:
            logger.debug("No area defined for project '%s'; skipping", project.name)
            continue

        area = maintainers_file.areas[area_name]
        if not area.maintainers:
            logger.info("No maintainers for project '%s'; skipping", project.name)
            continue

        logger.debug(
            "Project '%s': maintainers=%s, collaborators=%s",
            project.name,
            area.maintainers,
            area.collaborators,
        )
        repos[f"{args.org}/{project.name}"] = area

    if not repos:
        logger.warning("No active module repos with maintainers found in manifest")
        return

    query = "is:open is:pr no:assignee " + " ".join(f"repo:{repo}" for repo in repos)
    logger.info("Searching for unassigned module PRs with query: %s", query)

    for issue in gh.search_issues(query=query):
        pull = issue.as_pull_request()

        if pull.draft:
            logger.debug("Skipping draft PR: %s", pull.html_url)
            continue

        if pull.assignees:
            logger.error(
                "PR %s unexpectedly has assignees %s despite no:assignee filter",
                pull.html_url,
                pull.assignees,
            )
            continue

        area = repos[f"{args.org}/{issue.repository.name}"]

        for maintainer in area.maintainers:
            logger.info("Assigning '%s' to %s", maintainer, pull.html_url)
            if not args.dry_run:
                pull.add_to_assignees(maintainer)
                pull.create_review_request(maintainer)

        for collaborator in area.collaborators:
            logger.info("Adding reviewer '%s' to %s", collaborator, pull.html_url)
            if not args.dry_run:
                pull.create_review_request(collaborator)


def main():
    args = parse_args()
    setup_logging(args.verbose)

    token = os.environ.get('GITHUB_TOKEN')
    if not token:
        sys.exit('GITHUB_TOKEN environment variable is not set. Please set it and retry.')

    gh = Github(auth=Auth.Token(token))
    maintainer_file = Maintainers(args.maintainer_file)

    if args.pull_request:
        process_pr(gh, args, maintainer_file, args.pull_request)
    elif args.issue:
        process_issue(gh, args, maintainer_file, args.issue)
    elif args.modules:
        process_modules(gh, args, maintainer_file)
    else:
        if args.since:
            since = args.since
        else:
            since = datetime.date.today() - datetime.timedelta(days=1)

        query = (
            f'repo:{args.org}/{args.repo} is:open is:pr base:main '
            f'-is:draft no:assignee created:>{since}'
        )
        logger.info("Searching for unassigned PRs with query: %s", query)
        for issue in gh.search_issues(query=query):
            process_pr(gh, args, maintainer_file, issue.number)


if __name__ == "__main__":
    main()
