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
than MAX_LABELS distinct labels would be applied, they are ranked by how much of
the PR the areas carrying them cover (their file weight, falling back to their
plain file count for areas weighted 0 -- see Area weighting below) and only the
MAX_LABELS highest-ranked ones are applied.  That keeps a PR touching very
broad cross-cutting areas from being papered with labels, while still labelling
it by whatever it predominantly changes.  Size labels are managed separately:
they neither count towards the cap nor get dropped by it.

The cap governs what a run applies; labels already on the PR are never removed,
so it cannot pull an over-labelled PR back under the cap (use --reset for that).

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
  - Meta-areas, i.e. areas carrying ``meta: true`` in MAINTAINERS.yml
    (Documentation, Samples, Tests, ...): used only for assignee selection
    when they are the *sole* area touched (see below); not weighted for
    mixed PRs.

Platform (driver/board) areas receive a weight of 1 for the first file that
maps to them.  Subsequent files that also map to the *same* Platform area
score 0 to avoid double-counting.  This is controlled by the is_instance flag:
once a Platform area has been seen for a given file's sorted_areas list, all
further areas for that file score 0.

The resulting per-area weights drive both reviewer selection (ordered by
weight) and assignee selection.

Reviewer selection
------------------
The ordered reviewer candidate list is built in two tiers, maintainers before
collaborators, so that a PR touching many areas does not spend its limited
review slots on the collaborators of the highest-weight areas while dropping
the maintainers of every other touched area:

  1. Maintainer tier:
     a. For each area in descending weight order: the area's maintainers.
     b. Any extra reviewers identified from the manifest or MAINTAINERS.yml
        diff (see Manifest / MAINTAINERS.yml change handling below).
     c. Maintainers of deferred file-group areas (see Deferred file-groups).
  2. Collaborator tier:
     a. For each area in descending weight order: the area's collaborators.
     b. Any path-specific collaborators returned by
        get_collaborators_for_path() for each matched file.
  3. Deduplicate while preserving insertion order.
  4. Heuristic tier, used for changed files MAINTAINERS.yml cannot staff:

       - files in an under-covered area, i.e. one that names no maintainers or
         at most THIN_AREA_COLLABORATORS (2) collaborators, and so cannot
         supply enough reviewers on its own.  Meta-areas are excluded: they
         cover large parts of the tree by design, and the people they name are
         the right reviewers for them, and
       - orphaned files, which match no area at all.  A PR consisting only of
         these would otherwise get no reviewer whatsoever, since every tier
         above is derived from matched areas.

     This tier is skipped entirely once the tiers above have already produced
     MIN_TOTAL_REVIEWERS (3) candidates other than the author: the PR then has
     reviewers who own it by declaration, and lower-confidence names added on
     top would only dilute the request (and cost API calls).

     Two heuristics are tried in order, and the result is appended after the
     tiers above (the author and anyone already listed are excluded).  These
     are lower-confidence picks, so they fill only leftover review slots.

     a. GitHub's own ``suggestedReviewers`` for the PR, which GitHub derives
        from commit history *and* past review comments.  This is the better
        signal and costs one query, but it is only available over GraphQL and
        is empty for most PRs, so it cannot be relied on alone.
     b. When GitHub suggests nobody, the recent contributors to the unstaffed
        files, read from the commit history.  Unlike (a) this is scoped to the
        files that are actually short of reviewers, rather than to the PR as a
        whole.  Only the last HISTORY_MAX_AGE_DAYS (365) days are looked at, so
        that people who stopped working on the file long ago are not asked.
        Further bounded by HISTORY_COMMITS_PER_FILE, MAX_HISTORY_FILES, and
        MAX_HISTORY_REVIEWERS.

Candidates are then filtered:
  - The PR author is skipped.
  - Users who already submitted a review or are already on the review-request
    list are skipped.
  - Users whose login no longer resolves (deleted accounts) are skipped.
  - Users who previously self-removed themselves from the review request list
    are skipped.  This is checked before the collaborator test below, so that
    someone who opted out is never routed to the mention fallback instead.
  - Users who are not repository collaborators are set aside: GitHub requires
    collaborator status to receive a review request, so they are mentioned in a
    comment instead (see below) rather than dropped.

Every candidate that survives those filters is then requested: no cap is
applied.  GitHub documents no limit on how many reviewers a pull request may
carry, and PRs holding more than 15 are observable in the wild, so imposing one
here only meant dropping people who were legitimately responsible for the
change.  What keeps the request meaningful is the tier ordering above, not a
count: if a PR is broad enough to need thirty reviewers, all thirty own part of
it.

Should GitHub refuse the full set anyway (some undocumented limit, or a
candidate who lost collaborator status between the check and the request), the
call is retried once with the first REVIEWER_RETRY_BATCH (15) candidates.
Because the list is ordered highest-signal first, that retry keeps the area
maintainers and drops only the tail.

Mention fallback
----------------
A formal review request is not always possible: non-collaborators cannot
receive one at all, and a request may be refused outright.  Rather than let
those people fall off the PR silently, they are @mentioned in a comment asking
for their review, which notifies them regardless of their permissions.

The comment carries a hidden MENTION_MARKER so that repeated runs over the same
PR find their own previous comment and edit it in place, instead of posting a
duplicate every time the PR is updated.  Users who removed themselves from the
review request are never mentioned, since a mention would route around an
explicit opt-out.

Assignee selection
------------------
Assignees are selected from area_counter (areas sorted by descending file
weight) using the following priority rules:

  1. Areas with a weight of 0 or no maintainers are skipped.
  2. If the PR author is the area's only maintainer, the area is skipped
     (to avoid self-assignment).  If the author is one of several maintainers,
     they are excluded from the candidate list and the remaining maintainers
     are used.
  3. Non-platform areas (no "Platform" in the area name) take highest
     priority: the first such area's maintainers are inserted at the front
     of the ranked list, and the search stops.
  4. Platform areas (drivers, boards) are appended as lower-priority
     fallbacks.
  5. Meta-areas (``meta: true`` in MAINTAINERS.yml) are skipped in mixed PRs.

After iterating all areas, the first entry in the ranked list is chosen.
If the ranking process yielded nothing (e.g. all areas had the author as sole
maintainer), the maintainer with the highest cumulative file-weight score is
used as a last resort.

Reset mode
----------
With --reset, the PR is staffed from scratch instead of being topped up.  The
new staffing is worked out first, and every decision an earlier run made is then
undone, just before the new one is applied:

  - review requests are withdrawn, except from people who reviewed or commented
    on the PR (they are involved by their own action) and people this run is
    about to request again (withdrawing only to re-send would notify them
    twice).  Team review requests are left alone: the script only ever requests
    individuals, so a team was requested by hand;
  - all assignees are removed;
  - all area labels defined in MAINTAINERS.yml are removed.  Size labels are
    left to update_size_labels, which recomputes them on every run, and labels
    the script does not own (``bug``, backport labels, ...) are never touched.

Labels, reviewers and the assignee are chosen exactly as they would be for a
newly opened PR -- including the rule that people who removed themselves from
the review request are never re-added, which --reset cannot route around.

A PR the run skips (draft, closed, more than MAX_FILES changed files) is not
reset either: the skip is decided before the reset runs, so such a PR keeps the
staffing it has rather than being stripped and left bare.

Use it after MAINTAINERS.yml changed, or when a previous run staffed a PR
badly.

Deferred file-groups
--------------------
A file-group in MAINTAINERS.yml may carry ``defer-to-other-areas: true``.
When a changed file is matched by both a deferred file-group area **and** a
non-deferred area, the deferred area's maintainers are added to the reviewer
request instead of being considered for the assignee role.  Only the
non-deferred area's maintainers are candidates for assignment.

If a file is matched *only* by deferred file-groups and no other area also
claims it, the deferral has no effect: the area is used normally for
both assignment and review.

Typical use-case: the Clock Control subsystem marks platform-specific files
(e.g. ``drivers/clock_control/nrf_*.c``) in a deferred file-group.  A Nordic
platform area also lists those files without the defer flag.  When a PR
touches those files the platform maintainer is assigned and the clock-control
maintainer is added as a reviewer.

Manifest / MAINTAINERS.yml change handling
-------------------------------------------
west.yml and submanifests/optional.yaml:
  If --updated-manifest is provided, it (the PR's manifest) is compared against
  --base-manifest to find added, removed, and updated west projects.  Each
  changed project is looked up in MAINTAINERS.yml under "West project: <name>"
  and the corresponding collaborators are added to the reviewer candidate list.
  --base-manifest must be the manifest the PR forked from (the merge ref's
  first parent); comparing against the moving base-branch tip instead would
  also pick up every project that advanced on the base branch after the PR was
  created and add all their maintainers as reviewers (see #110422).  When
  --base-manifest is omitted it defaults to the checked-out west.yml.

MAINTAINERS.yml:
  If --updated-maintainer-file is provided, it (the PR's version) is diffed
  field-by-field against --base-maintainer-file (maintainers, collaborators,
  labels, files, files-regex, status).  Maintainers of every area that changed
  are appended to the reviewer candidate list so that the people responsible
  for each changed area are automatically notified.  As with the manifest, the
  base file must be the version the PR forked from, not the base-branch tip;
  it defaults to the checked-out MAINTAINERS.yml when omitted.

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

# GitHub does not document a cap on how many reviewers a pull request may have,
# and PRs carrying more than 15 are observable in the wild, so no cap is imposed
# here: every eligible candidate is requested.  Should GitHub reject the full
# set anyway, the request is retried with this many of the highest-ranked
# candidates so that a rejection cannot cost the review request entirely.
REVIEWER_RETRY_BATCH = 15

# Hidden marker identifying the comment used to mention reviewers who could not
# be added to the review request.  It lets a re-run find and update its own
# previous comment instead of posting a duplicate.
MENTION_MARKER = "<!-- set_assignees: reviewer-mention -->"

# Maximum number of labels to apply; beyond this the labels are noise from
# over-broad matches, so only the ones contributed by the heaviest areas are
# kept and the PR is still labelled by what it mostly changes.
MAX_LABELS = 10

# An area is considered under-covered when it names no maintainers, or names at
# most this many collaborators.  Such areas do not supply enough reviewers on
# their own, so recent Git contributors to the changed files are added as
# heuristic reviewer candidates (see _history_reviewers).
THIN_AREA_COLLABORATORS = 2

# Bounds on the Git-history heuristic, to keep its GitHub API cost predictable:
#   - sample at most this many recent commits per changed file,
#   - inspect at most this many under-covered files in total,
#   - add at most this many heuristic reviewers to the candidate list,
#   - ignore commits older than this, so that people who last touched the file
#     years ago and have long since moved on are not asked to review it.
HISTORY_COMMITS_PER_FILE = 20
MAX_HISTORY_FILES = 40
MAX_HISTORY_REVIEWERS = 3
HISTORY_MAX_AGE_DAYS = 365

# The heuristic reviewer tiers (GitHub suggestions, Git history) exist to keep a
# PR from going unreviewed when MAINTAINERS.yml cannot staff it.  They are not
# consulted once all the touched areas together have yielded this many
# candidates: at that point the PR has reviewers responsible for it by
# declaration, and adding lower-confidence names on top only dilutes the
# request.
MIN_TOTAL_REVIEWERS = 3

# GitHub's own reviewer suggestions ("based on commit history and past review
# comments") are exposed only through the GraphQL API, as a plain unpaginated
# list on the PullRequest object.
SUGGESTED_REVIEWERS_QUERY = """
query($owner: String!, $name: String!, $number: Int!) {
  repository(owner: $owner, name: $name) {
    pullRequest(number: $number) {
      suggestedReviewers {
        isAuthor
        isCommenter
        reviewer {
          login
        }
      }
    }
  }
}
"""

# Courtesy sleep between consecutive GitHub API calls to avoid secondary rate limits.
API_SLEEP_SECONDS = 1


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
        help="Manifest file as it appears in the pull request.",
    )

    parser.add_argument(
        "--base-manifest",
        default=None,
        help=(
            "Manifest file to compare --updated-manifest against. Should be the "
            "version the PR forked from (the merge ref's first parent), not the "
            "base-branch tip. Defaults to the checked-out west.yml when omitted."
        ),
    )

    parser.add_argument(
        "--updated-maintainer-file",
        default=None,
        help="Maintainer file as it appears in the pull request.",
    )

    parser.add_argument(
        "--base-maintainer-file",
        default=None,
        help=(
            "Maintainer file to compare --updated-maintainer-file against. Should "
            "be the version the PR forked from (the merge ref's first parent), not "
            "the base-branch tip. Defaults to the checked-out MAINTAINERS.yml when "
            "omitted."
        ),
    )

    parser.add_argument(
        "-v",
        "--verbose",
        action="count",
        default=0,
        help="Verbose output. Use -v for INFO, -vv for DEBUG.",
    )

    parser.add_argument(
        "--reset",
        action="store_true",
        default=False,
        help=(
            "Re-staff the pull request from scratch: drop the review requests of "
            "everyone who has not reviewed or commented, remove the assignees and "
            "the MAINTAINERS.yml area labels, then apply labels, reviewers and an "
            "assignee as if the pull request had just been opened. Reviewers who "
            "removed themselves are not re-added."
        ),
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


def process_manifest(pr_manifest_file: str, base_manifest_file: str = "west.yml") -> list:
    """Return area name strings for projects that changed between two west.yml files.

    *pr_manifest_file* is the manifest as it appears in the pull request, and
    *base_manifest_file* is the manifest it should be compared against.  The
    base must be the version the PR actually forked from (the merge ref's first
    parent), not the moving base-branch tip; otherwise projects that advanced on
    the base branch after the PR was created are wrongly reported as changed and
    their maintainers get spuriously added as reviewers (see #110422).
    """
    logger.info("Processing manifest changes")

    if not os.path.isfile(base_manifest_file):
        logger.warning(
            "Base manifest '%s' not found; skipping manifest processing", base_manifest_file
        )
        return []
    if not os.path.isfile(pr_manifest_file):
        logger.warning("PR manifest '%s' not found; skipping manifest processing", pr_manifest_file)
        return []

    old_manifest = Manifest.from_file(base_manifest_file)
    new_manifest = Manifest.from_file(pr_manifest_file)

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
        elif not area.meta:
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


def _thin_areas(maintainer_file, area_counter: dict) -> set:
    """Return the names of areas in *area_counter* that are under-covered.

    An area is under-covered when MAINTAINERS.yml names no maintainers for it,
    or names at most THIN_AREA_COLLABORATORS collaborators.  These areas cannot
    supply enough reviewers on their own, so the caller supplements them with
    recent Git contributors (see _history_reviewers).

    Meta-areas (``meta: true`` in MAINTAINERS.yml) are never reported as
    under-covered.  Their file patterns span large parts of the tree, so the
    small number of people they name is deliberate rather than a gap, and
    walking the history of every documentation or sample file they match would
    add reviewers on top of the maintainers who already own the area.
    """
    thin = set()
    for area in area_counter:
        entry = maintainer_file.areas[area.name]
        if entry.meta:
            continue
        if not entry.maintainers or len(entry.collaborators) <= THIN_AREA_COLLABORATORS:
            thin.add(area.name)
    return thin


def _suggested_reviewers(gh, args, pr, exclude: set) -> list:
    """Return GitHub's own reviewer suggestions for *pr* as ordered logins.

    GitHub computes these from commit history and past review comments, so
    they are a strictly better-informed version of the _history_reviewers
    heuristic when available.  They are only exposed through GraphQL, hence
    the raw query; PyGithub's requester reuses the same token and session, so
    this adds no dependency.

    Suggestions where GitHub flagged the person as an author of the changed
    code rank ahead of comment-only suggestions, and ties break by login for
    determinism.  Logins in *exclude* are dropped.

    The list is frequently empty (GitHub's heuristic is sparse, and it is not
    documented when it declines to suggest), and the field is best-effort, so
    every failure path degrades to an empty list and lets the caller fall back
    to _history_reviewers.
    """
    variables = {"owner": args.org, "name": args.repo, "number": pr.number}
    try:
        _headers, response = gh.requester.graphql_query(SUGGESTED_REVIEWERS_QUERY, variables)
    except GithubException as exc:
        logger.debug("suggestedReviewers query failed for PR #%d: %s", pr.number, exc)
        return []

    if response.get("errors"):
        logger.debug(
            "suggestedReviewers query returned errors for PR #%d: %s",
            pr.number,
            response["errors"],
        )

    try:
        suggestions = response["data"]["repository"]["pullRequest"]["suggestedReviewers"]
    except (KeyError, TypeError):
        logger.debug("Unexpected suggestedReviewers response for PR #%d", pr.number)
        return []

    ranked = []
    for suggestion in suggestions or []:
        login = (suggestion.get("reviewer") or {}).get("login")
        if not login or login in exclude:
            continue
        # Authorship of the changed code is a stronger signal than having
        # commented on it, so sort authors first.
        ranked.append((0 if suggestion.get("isAuthor") else 1, login))

    return [login for _rank, login in sorted(ranked)]


def _history_reviewers(gh_repo, filenames, exclude: set) -> list:
    """Return recent contributors to *filenames* as ordered GitHub logins.

    Walks each file's commit history (newest first) and tallies the commit
    authors GitHub resolved to a real account, ranking them by how many of the
    sampled commits they authored (ties broken by login for determinism).

    Commit history is read through the GitHub API rather than a local
    ``git log``/``git blame`` because the API yields actual GitHub logins
    (commit-author emails cannot be mapped to logins reliably) and does not
    depend on the CI checkout having the full history.

    Only commits from the last HISTORY_MAX_AGE_DAYS days are considered: a
    contributor who has not touched the file within a year is unlikely to still
    be the right person to ask, and on old files the unbounded history would
    otherwise fill every heuristic slot with people who have moved on.

    *exclude* holds logins already covered (PR author, existing candidates) so
    they are not re-proposed.  Sampling is bounded by HISTORY_COMMITS_PER_FILE,
    MAX_HISTORY_FILES, and MAX_HISTORY_REVIEWERS.
    """
    since = datetime.datetime.now(datetime.UTC) - datetime.timedelta(days=HISTORY_MAX_AGE_DAYS)

    sampled_files = sorted(filenames)
    if len(sampled_files) > MAX_HISTORY_FILES:
        logger.info(
            "Sampling Git history for %d of %d under-covered files (limit %d)",
            MAX_HISTORY_FILES,
            len(sampled_files),
            MAX_HISTORY_FILES,
        )
        sampled_files = sampled_files[:MAX_HISTORY_FILES]

    counts = defaultdict(int)
    for filename in sampled_files:
        try:
            commits = gh_repo.get_commits(path=filename, since=since)
        except GithubException as exc:
            logger.debug("No commit history for '%s': %s", filename, exc)
            continue

        sampled = 0
        for commit in commits:
            if sampled >= HISTORY_COMMITS_PER_FILE:
                break
            sampled += 1
            author = commit.author
            login = getattr(author, "login", None) if author else None
            if not login or login in exclude:
                continue
            counts[login] += 1

    ranked = sorted(counts, key=lambda login: (-counts[login], login))
    return ranked[:MAX_HISTORY_REVIEWERS]


def _build_reviewer_candidates(
    maintainer_file,
    area_counter: dict,
    collab_per_path: set,
    additional_reviews: set,
    deferred_reviewers: set,
) -> list:
    """Build the ordered reviewer candidate list for a PR.

    Maintainers of every touched area come first (in descending area-weight
    order), then collaborators.  The caller truncates the filtered result to
    the reviewer cap, so ordering maintainers ahead of collaborators ensures
    the highest-signal reviewers are never dropped in favour of lower-signal
    ones.  Lower-confidence Git-history reviewers (for under-covered areas) are
    appended after this list by the caller so they fill only leftover slots.
    """
    maintainers = []
    collaborators = []
    for area in area_counter:
        maintainers += maintainer_file.areas[area.name].maintainers
        collaborators += maintainer_file.areas[area.name].collaborators

    candidates = (
        maintainers
        + sorted(additional_reviews)
        + sorted(deferred_reviewers)
        + collaborators
        + sorted(collab_per_path)
    )

    # Deduplicate while preserving insertion order.
    return list(dict.fromkeys(candidates))


def _add_reviewers(gh, gh_repo, pr, args, collab: list):
    """Request reviews from eligible collaborators on *pr*.

    Skips the PR author, existing reviewers, non-collaborators, and users
    who previously removed themselves from the review request.

    Every remaining candidate is requested; no cap is applied.  *collab* is
    ordered highest-signal first (area maintainers, then collaborators, then
    heuristic picks -- see _build_reviewer_candidates), so the ordering, rather
    than an arbitrary limit, is what protects the review request from being
    dominated by low-signal reviewers.
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

    reviewers = []
    non_collaborators = []
    for collaborator in collab:
        try:
            gh_user = gh.get_user(collaborator)
        except UnknownObjectException:
            # The login does not resolve, so there is nobody to mention either.
            logger.warning("User '%s' not found; account may have been deleted", collaborator)
            continue

        if pr.user == gh_user:
            logger.debug("Skipping PR author '%s'", collaborator)
            continue
        if gh_user in existing_reviewers:
            logger.debug("Skipping existing reviewer '%s'", collaborator)
            continue
        if gh_user in self_removed:
            logger.info("Skipping '%s': previously self-removed from reviewers", collaborator)
            continue
        if not gh_repo.has_in_collaborators(gh_user):
            # GitHub refuses a review request for non-collaborators, so ask
            # them by mention instead of dropping them.
            logger.info("'%s' is not a repository collaborator; will mention", collaborator)
            non_collaborators.append(collaborator)
            continue

        reviewers.append(collaborator)

    if reviewers:
        logger.info("Requesting reviews from %d user(s): %s", len(reviewers), reviewers)
        unrequested = _request_reviews(pr, args, reviewers)
    else:
        unrequested = []

    # Anyone the review request could not accommodate still gets asked, by
    # name, in a comment.  Users who removed themselves are deliberately not
    # mentioned: they opted out, and a mention would route around that.
    _mention_reviewers(pr, args, non_collaborators + unrequested)


def _request_reviews(pr, args, reviewers: list) -> list:
    """Request reviews from *reviewers*; return those that could not be added.

    A rejected bulk request is retried once with the highest-ranked
    REVIEWER_RETRY_BATCH candidates, so that hitting an undocumented per-PR
    reviewer limit cannot leave the PR with no reviewers at all.  Whoever is
    left out is returned for the caller to mention instead.
    """
    if args.dry_run:
        return []

    try:
        pr.create_review_request(reviewers=reviewers)
        return []
    except GithubException as exc:
        logger.error("Failed to add reviewers %s: %s", reviewers, exc)

    if len(reviewers) > REVIEWER_RETRY_BATCH:
        retry = reviewers[:REVIEWER_RETRY_BATCH]
        logger.info("Retrying with the first %d: %s", len(retry), retry)
        try:
            pr.create_review_request(reviewers=retry)
            return reviewers[REVIEWER_RETRY_BATCH:]
        except GithubException as retry_exc:
            logger.error("Retry failed for reviewers %s: %s", retry, retry_exc)

    return list(reviewers)


def _mention_reviewers(pr, args, logins: list):
    """Ask *logins* for a review in a PR comment.

    Used for people who cannot receive a formal review request: those who are
    not repository collaborators (GitHub rejects a review request for them),
    and those the request itself could not accommodate.  An @mention notifies
    them regardless of their permissions, so the change still reaches the
    people responsible for it.

    The comment is maintained in place: one is created the first time and
    edited afterwards, keyed on MENTION_MARKER, so that re-running over the
    same PR does not spam it with duplicates.
    """
    logins = list(dict.fromkeys(logins))
    if not logins:
        return

    mentions = " ".join(f"@{login}" for login in logins)
    body = (
        f"{MENTION_MARKER}\n"
        f"{mentions}\n\n"
        "You have been identified as a likely reviewer for the code this pull "
        "request changes, but could not be added to its review request "
        "automatically. Please review it if you are able to."
    )

    logger.info("Mentioning %d user(s) unable to be requested: %s", len(logins), logins)
    if args.dry_run:
        return

    try:
        for comment in pr.get_issue_comments():
            if MENTION_MARKER in comment.body:
                # Already asked; refresh only when the set of people changed.
                if comment.body.strip() != body.strip():
                    comment.edit(body)
                    logger.info("Updated existing reviewer-mention comment")
                else:
                    logger.debug("Reviewer-mention comment already up to date")
                return

        pr.create_issue_comment(body)
    except GithubException as exc:
        logger.error("Failed to mention reviewers %s: %s", logins, exc)


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


def _rank_labels(labels: set, area_counter: dict, area_files: dict) -> list:
    """Return *labels* ordered by how much of the PR the areas carrying them cover.

    A label's rank is the sum of the file weights of every touched area that
    carries it, so a label backed by the bulk of the changed files outranks one
    coming from a single incidentally matched area.  Ties break by label name to
    keep the choice deterministic.

    Areas weighing 0 fall back to the number of files they matched.  Those
    weights are zeroed for assignee selection -- meta-areas, files that only
    matched CMakeLists.txt, repeated matches of one Platform area -- but the
    areas still describe what the PR changes, and a label is about exactly that.
    Without the fallback a documentation-only PR would rank "area: Documentation"
    last and drop it first.  Labels with no contributing area at all (labels of
    deferred file-groups) still score 0 and sort last.
    """
    weights = defaultdict(int)
    for area, weight in area_counter.items():
        weight = weight or len(area_files.get(area.name, ()))
        for label in area.labels:
            if label in labels:
                weights[label] += weight

    return sorted(labels, key=lambda label: (-weights[label], label))


def _select_labels(pr, labels: set, area_counter: dict, area_files: dict) -> set:
    """Return the labels to apply to *pr*, truncated to MAX_LABELS if needed.

    Up to MAX_LABELS area labels everything is applied.  Beyond that the PR
    touches so many areas that labelling it with all of them says nothing, so
    only the MAX_LABELS highest-ranked ones are applied (see _rank_labels).

    Only the area labels count towards the limit, and only they can be dropped.
    Size labels are computed for this PR alone and update_size_labels has
    already removed the stale ones, so dropping the new one would leave the PR
    with no size label at all -- and counting it would make MAX_LABELS area
    labels plus a size label look like an overflow.

    This caps what the run *applies*; it never takes a label off the PR.  A
    label already there stays, whether a human added it or an earlier run did
    (--reset is the way to clear those).
    """
    area_labels = labels - _SIZE_LABELS
    if len(area_labels) <= MAX_LABELS:
        return labels

    ranked = _rank_labels(area_labels, area_counter, area_files)
    kept = set(ranked[:MAX_LABELS])

    logger.warning(
        "PR #%d matched %d area labels (limit %d); applying only the %d highest-weight: "
        "%s (not applying: %s)",
        pr.number,
        len(area_labels),
        MAX_LABELS,
        len(kept),
        sorted(kept),
        sorted(area_labels - kept),
    )
    return kept | (labels & _SIZE_LABELS)


def _interacted_logins(pr) -> set:
    """Return the logins of everyone who has engaged with *pr*.

    That is anyone who submitted a review, or commented on the PR itself or on
    one of its review threads.  Such people are involved in the change by their
    own action, so a reset must not drop them.
    """
    logins = set()

    def _add(user):
        login = getattr(user, 'login', None)
        if login:
            logins.add(login)

    for review in pr.get_reviews():
        _add(review.user)
    for comment in pr.get_issue_comments():
        _add(comment.user)
    for comment in pr.get_review_comments():
        _add(comment.user)

    return logins


def _reset_pr(pr, args, maintainer_file, keep=()) -> set:
    """Undo this script's earlier decisions on *pr* so it can be staffed afresh.

    Removes the review request from everyone who has not engaged with the PR,
    drops every assignee, and strips the area labels that MAINTAINERS.yml
    defines.  The caller then applies the staffing it has just computed, so the
    PR ends up as if it had been opened now.

    Called late in process_pr, once that staffing is known, for two reasons:
    every early return (draft, closed, MAX_FILES) has already had its say, so a
    reset cannot strip a PR and then bail out without restaffing it; and *keep*
    can hold the logins about to be requested again.  Their request is left in
    place, since withdrawing it only to re-send it would notify them twice.

    Four things are deliberately left alone:

      - Reviewers who reviewed or commented (see _interacted_logins).  They are
        already part of the conversation, and dropping their request would
        either lose that or notify them again for nothing.
      - Team review requests.  This script only ever requests individuals, so a
        team request was added by hand and a reset has no basis to withdraw it.
      - Size labels: update_size_labels recomputes them on every run and
        removes the stale ones itself.
      - Labels the script does not own (``bug``, ``RFC``, backport labels, ...).
        They are not derived from MAINTAINERS.yml, so a reset has no basis for
        second-guessing whoever added them.

    People who removed themselves from the review request are not handled here:
    _add_reviewers already refuses to re-request them, so the reset cannot
    route around that opt-out.

    Returns the names of the labels removed, since pr.labels is a snapshot
    taken before the reset.
    """
    protected = _interacted_logins(pr) | set(keep)

    review_users, _review_teams = pr.get_review_requests()
    requested = {user.login for user in review_users}

    kept = sorted(requested & protected)
    if kept:
        logger.info(
            "Reset: keeping review request(s) for users who engaged or are still responsible: %s",
            kept,
        )

    stale = sorted(requested - protected)
    if stale:
        logger.info("Reset: removing %d review request(s): %s", len(stale), stale)
        if not args.dry_run:
            pr.delete_review_request(reviewers=stale)

    assignees = sorted({assignee.login for assignee in pr.assignees})
    if assignees:
        logger.info("Reset: removing assignee(s): %s", assignees)
        if not args.dry_run:
            pr.remove_from_assignees(*assignees)

    area_labels = {label for area in maintainer_file.areas.values() for label in area.labels}
    removed_labels = {label.name for label in pr.labels} & area_labels
    if removed_labels:
        logger.info("Reset: removing label(s): %s", sorted(removed_labels))
        for name in sorted(removed_labels):
            if not args.dry_run:
                pr.remove_from_labels(name)

    return removed_labels


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
    deferred_reviewers = set()
    # Files that mapped to each area, used to sample Git history for areas that
    # MAINTAINERS.yml under-covers (see _thin_areas / _history_reviewers).
    area_files = defaultdict(set)
    # Changed files that mapped to no area at all.  MAINTAINERS.yml offers no
    # reviewer for these, so they feed the same heuristics as thin areas.
    orphan_files = set()

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
            parsed_areas = process_manifest(
                pr_manifest_file=args.updated_manifest,
                base_manifest_file=args.base_manifest or "west.yml",
            )
            for area_name in parsed_areas:
                area_matches = maintainer_file.name2areas(area_name)
                if area_matches:
                    collab_per_path.update(area_matches[0].get_collaborators_for_path(filename))
                    areas.extend(area_matches)

        elif filename == 'MAINTAINERS.yml':
            areas = maintainer_file.path2areas(filename)
            if args.updated_maintainer_file:
                # Compare the PR's MAINTAINERS.yml against the version it forked
                # from (the merge ref's first parent), not the base-branch tip,
                # so areas changed on the base branch after the PR was created
                # are not wrongly attributed to this PR (see #110422).
                old_areas = load_areas(args.base_maintainer_file or 'MAINTAINERS.yml')
                new_areas = load_areas(args.updated_maintainer_file)
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
            # Orphaned: no area claims this file, so MAINTAINERS.yml yields no
            # reviewer for it.  Remember it for the heuristic pass below.
            logger.debug("  '%s' matches no area", filename)
            orphan_files.add(filename)
            continue

        # Handle deferred file-groups: when a file is matched by both a
        # deferred area (file-group with defer-to-other-areas: true) and a
        # non-deferred area, the deferred area's maintainers become reviewers
        # only and do not count toward assignee weighting for this file.
        _deferred = [a for a in areas if a.is_deferred_for_path(filename)]
        _non_deferred = [a for a in areas if not a.is_deferred_for_path(filename)]
        if _deferred and _non_deferred:
            for _area in _deferred:
                deferred_reviewers.update(_area.maintainers)
                labels.update(_area.labels)
                logger.debug(
                    "  area '%s' defers to other areas for '%s'; "
                    "maintainers %s added as reviewers only",
                    _area.name,
                    filename,
                    _area.maintainers,
                )
            areas = _non_deferred

        # Sort so that Platform (driver/board) areas are processed first.  This
        # sets is_instance=True early, preventing the same file from being
        # counted again for the corresponding subsystem area.
        sorted_areas = sorted(areas, key=lambda x: 'Platform' in x.name, reverse=True)
        is_instance = False

        for area in sorted_areas:
            # CMakeLists.txt changes and meta-area files do not count toward
            # the area weight used for assignee selection.
            if 'CMakeLists.txt' in filename or area.meta:
                count = 0
            else:
                # Once an instance (Platform) area has been seen, subsequent
                # areas for this file score 0 to avoid double-counting.
                count = 1 if not is_instance else 0

            area_counter[area] += count
            area_files[area.name].add(filename)
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

    # Build the ordered collaborator/reviewer list: maintainers of all touched
    # areas by area priority first, then collaborators.
    collab = _build_reviewer_candidates(
        maintainer_file, area_counter, collab_per_path, additional_reviews, deferred_reviewers
    )

    # Supplement files MAINTAINERS.yml cannot staff with heuristic reviewers:
    # those in under-covered areas (no maintainers, or few collaborators) and
    # those in no area at all.  Exclude the author and everyone already listed
    # so these last-resort slots are not wasted on reviewers we already have.
    thin = _thin_areas(maintainer_file, area_counter)
    uncovered_files = set(orphan_files)
    for name in thin:
        uncovered_files.update(area_files.get(name, ()))

    # The heuristics below are a stopgap for PRs the areas cannot staff, so they
    # only run while the area tiers are short of reviewers.  The author does not
    # count: they cannot review their own PR.
    total_reviewers = set(collab) - {pr.user.login}

    if uncovered_files and len(total_reviewers) >= MIN_TOTAL_REVIEWERS:
        logger.info(
            "Skipping heuristic reviewers: %d reviewer(s) already found from the "
            "touched areas (threshold %d)",
            len(total_reviewers),
            MIN_TOTAL_REVIEWERS,
        )
    elif uncovered_files:
        if orphan_files:
            logger.info(
                "%d changed file(s) match no area: %s",
                len(orphan_files),
                sorted(orphan_files),
            )

        exclude = set(collab) | {pr.user.login}

        # Prefer GitHub's own suggestions: they draw on both commit history and
        # past review comments, and cost a single query.  They are often empty,
        # in which case fall back to walking the history of the files that
        # MAINTAINERS.yml left unstaffed.
        heuristic = _suggested_reviewers(gh, args, pr, exclude)[:MAX_HISTORY_REVIEWERS]
        source = "GitHub-suggested"

        if not heuristic:
            heuristic = _history_reviewers(gh_repo, uncovered_files, exclude)
            source = "Git-history"

        if heuristic:
            logger.info(
                "Under-covered areas %s / %d orphaned file(s); adding %s reviewers: %s",
                sorted(thin),
                len(orphan_files),
                source,
                heuristic,
            )
            collab += heuristic
        else:
            logger.info("No heuristic reviewers found for under-covered files")

    logger.debug("Reviewer candidates: %s", collab)

    all_maintainers = dict(
        sorted(found_maintainers.items(), key=lambda item: item[1], reverse=True)
    )
    logger.info("PR submitted by: %s", pr.user.login)
    logger.info("Maintainer file-coverage scores: %s", all_maintainers)

    assignees = _pick_assignees(pr, area_counter, all_maintainers, num_files)

    # With --reset, wipe what an earlier run decided before applying the above,
    # so the PR ends up staffed as if it were new.  This happens here, not
    # earlier: every skip condition above has had its say, so a reset cannot
    # strip a PR the run then declines to restaff, and the reviewers about to be
    # requested again keep the request they already have.  What the reset
    # removed is tracked because pr.labels and pr.assignee still describe the
    # PR as it was before.
    removed_labels = _reset_pr(pr, args, maintainer_file, keep=collab) if args.reset else set()

    # Apply labels — skip any that are already present, then add the rest in
    # a single API call to avoid per-label timeline noise.
    if labels:
        labels = _select_labels(pr, labels, area_counter, area_files)
        current_label_names = {lbl.name for lbl in pr.labels} - removed_labels
        new_labels = sorted(labels - current_label_names)
        if new_labels:
            logger.info("Adding labels: %s", new_labels)
            if not args.dry_run:
                pr.add_to_labels(*new_labels)
        else:
            logger.info("All labels already present on PR #%d; skipping", pr.number)

    # Request reviews.
    # additional_reviews is already folded into collab by
    # _build_reviewer_candidates, so collab alone decides whether to ask.
    if collab:
        _add_reviewers(gh, gh_repo, pr, args, collab)

    # Set assignees (only when none are set yet, unless doing a dry run).
    # --reset just cleared them, so pr.assignee describes a state that is gone.
    has_assignee = pr.assignee and not args.reset
    if assignees and (not has_assignee or args.dry_run):
        _assign_maintainers(gh, pr, args, assignees)
    else:
        reason = "already has assignee" if has_assignee else "no assignees found"
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
