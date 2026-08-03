#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Tests for scripts/ci/set_assignees.py.

Heavy third-party dependencies (PyGithub, west, get_maintainer) are stubbed
out at module level so the test suite runs without a full Zephyr environment
or a live GitHub token.
"""

import datetime
import logging
import sys
from types import SimpleNamespace
from unittest.mock import MagicMock, patch

import pytest

# ---------------------------------------------------------------------------
# Stub heavy third-party and project-local modules before the SUT is imported.
# ---------------------------------------------------------------------------


class _GithubException(Exception):
    pass


class _UnknownObjectException(_GithubException):
    pass


_github_mod = MagicMock()
_github_mod.GithubException = _GithubException
_github_mod.Auth = MagicMock()
_github_mod.Github = MagicMock()

_github_exc_mod = MagicMock()
_github_exc_mod.UnknownObjectException = _UnknownObjectException

for _name, _mod in [
    ("github", _github_mod),
    ("github.GithubException", _github_exc_mod),
    ("west", MagicMock()),
    ("west.manifest", MagicMock()),
    ("get_maintainer", MagicMock()),
    ("yaml", __import__("yaml")),  # real yaml is available
]:
    sys.modules.setdefault(_name, _mod)

import set_assignees as sut  # noqa: E402, I001  (import after sys.modules manipulation)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


class _Area:
    """Lightweight hashable area stub accepted by _pick_assignees and process_pr."""

    def __init__(self, name, maintainers=None, labels=None, collaborators=None, meta=False):
        self.name = name
        self.maintainers = list(maintainers or [])
        self.labels = list(labels or [])
        self.collaborators = list(collaborators or [])
        self.meta = meta

    def is_deferred_for_path(self, path):
        return False

    def get_collaborators_for_path(self, path):
        return self.collaborators


class _DeferredArea(_Area):
    """Area stub where is_deferred_for_path always returns True."""

    def is_deferred_for_path(self, path):
        return True


def _make_area(name, maintainers=None, labels=None, collaborators=None, meta=False):
    return _Area(
        name, maintainers=maintainers, labels=labels, collaborators=collaborators, meta=meta
    )


def _make_pr(commits=1, additions=0, deletions=0, labels=None, user_login="contributor"):
    """Return a mock GitHub PR object for update_size_xs_label / _pick_assignees."""
    pr = MagicMock()
    pr.commits = commits
    pr.additions = additions
    pr.deletions = deletions
    pr.number = 42
    pr.labels = [SimpleNamespace(name=lbl) for lbl in (labels or [])]
    pr.user = SimpleNamespace(login=user_login)
    pr.assignee = None
    return pr


def _make_file(filename):
    return SimpleNamespace(filename=filename)


def _make_args(dry_run=False):
    return SimpleNamespace(dry_run=dry_run)


# ---------------------------------------------------------------------------
# load_areas
# ---------------------------------------------------------------------------


class TestLoadAreas:
    def test_includes_area_with_files(self, tmp_path):
        content = """
Area A:
  files:
    - drivers/foo/
  maintainers:
    - alice
"""
        f = tmp_path / "M.yml"
        f.write_text(content)
        result = sut.load_areas(str(f))
        assert "Area A" in result

    def test_includes_area_with_files_regex(self, tmp_path):
        content = """
Area B:
  files-regex:
    - ^include/foo/
  maintainers:
    - bob
"""
        f = tmp_path / "M.yml"
        f.write_text(content)
        result = sut.load_areas(str(f))
        assert "Area B" in result

    def test_excludes_area_without_files_or_regex(self, tmp_path):
        content = """
Meta Area:
  labels:
    - area: meta
  maintainers:
    - charlie
"""
        f = tmp_path / "M.yml"
        f.write_text(content)
        result = sut.load_areas(str(f))
        assert "Meta Area" not in result

    def test_excludes_scalar_top_level_entries(self, tmp_path):
        content = """
version: 1
Area C:
  files:
    - lib/bar/
  maintainers:
    - dave
"""
        f = tmp_path / "M.yml"
        f.write_text(content)
        result = sut.load_areas(str(f))
        assert "version" not in result
        assert "Area C" in result

    def test_both_files_and_regex_included(self, tmp_path):
        content = """
Area D:
  files:
    - subsys/foo/
  files-regex:
    - ^include/foo/
  maintainers:
    - eve
"""
        f = tmp_path / "M.yml"
        f.write_text(content)
        result = sut.load_areas(str(f))
        assert "Area D" in result


# ---------------------------------------------------------------------------
# set_or_empty
# ---------------------------------------------------------------------------


class TestSetOrEmpty:
    def test_key_present_with_list(self):
        assert sut.set_or_empty({"a": ["x", "y"]}, "a") == {"x", "y"}

    def test_key_present_with_none(self):
        assert sut.set_or_empty({"a": None}, "a") == set()

    def test_key_absent(self):
        assert sut.set_or_empty({}, "a") == set()

    def test_key_present_with_empty_list(self):
        assert sut.set_or_empty({"a": []}, "a") == set()


# ---------------------------------------------------------------------------
# _diff_area_entry
# ---------------------------------------------------------------------------


class TestDiffAreaEntry:
    def test_identical_entries_produce_no_changes(self):
        entry = {"maintainers": ["alice"], "collaborators": [], "status": "maintained"}
        assert sut._diff_area_entry(entry, entry) == []

    def test_maintainer_added(self):
        old = {"maintainers": ["alice"]}
        new = {"maintainers": ["alice", "bob"]}
        changes = sut._diff_area_entry(old, new)
        assert any("bob" in c and "added" in c for c in changes)

    def test_maintainer_removed(self):
        old = {"maintainers": ["alice", "bob"]}
        new = {"maintainers": ["alice"]}
        changes = sut._diff_area_entry(old, new)
        assert any("bob" in c and "removed" in c for c in changes)

    def test_collaborator_added(self):
        old = {"collaborators": []}
        new = {"collaborators": ["carol"]}
        changes = sut._diff_area_entry(old, new)
        assert any("carol" in c and "added" in c for c in changes)

    def test_status_changed(self):
        old = {"status": "maintained"}
        new = {"status": "odd fixes"}
        changes = sut._diff_area_entry(old, new)
        assert any("Status changed" in c for c in changes)

    def test_status_unchanged_no_entry(self):
        changes = sut._diff_area_entry({}, {})
        assert not any("Status" in c for c in changes)

    def test_label_added(self):
        old = {"labels": ["area: kernel"]}
        new = {"labels": ["area: kernel", "area: drivers"]}
        changes = sut._diff_area_entry(old, new)
        assert any("area: drivers" in c and "added" in c for c in changes)

    def test_files_regex_removed(self):
        old = {"files-regex": ["^include/foo/"]}
        new = {"files-regex": []}
        changes = sut._diff_area_entry(old, new)
        assert any("removed" in c for c in changes)

    def test_multiple_changes_all_reported(self):
        old = {"maintainers": ["alice"], "status": "maintained"}
        new = {"maintainers": ["bob"], "status": "odd fixes"}
        changes = sut._diff_area_entry(old, new)
        # At least two change entries (maintainer add + remove, status change)
        assert len(changes) >= 2


# ---------------------------------------------------------------------------
# compare_areas
# ---------------------------------------------------------------------------


class TestCompareAreas:
    def test_added_area_returned(self):
        old = {}
        new = {"New Area": {"files": ["foo/"], "maintainers": ["alice"]}}
        result = sut.compare_areas(old, new)
        assert "New Area" in result

    def test_removed_area_returned(self):
        old = {"Old Area": {"files": ["bar/"], "maintainers": ["bob"]}}
        new = {}
        result = sut.compare_areas(old, new)
        assert "Old Area" in result

    def test_changed_area_returned(self):
        area = {"files": ["baz/"], "maintainers": ["alice"]}
        old = {"My Area": dict(area)}
        new = {"My Area": {**area, "maintainers": ["alice", "carol"]}}
        result = sut.compare_areas(old, new)
        assert "My Area" in result

    def test_unchanged_area_not_returned(self):
        area = {"files": ["qux/"], "maintainers": ["dave"]}
        result = sut.compare_areas({"Stable": area}, {"Stable": area})
        assert "Stable" not in result

    def test_combination_of_all_three(self):
        old = {
            "Removed": {"files": ["r/"]},
            "Common": {"files": ["c/"], "maintainers": ["alice"]},
        }
        new = {
            "Added": {"files": ["a/"]},
            "Common": {"files": ["c/"], "maintainers": ["alice", "bob"]},
        }
        result = sut.compare_areas(old, new)
        assert "Removed" in result
        assert "Added" in result
        assert "Common" in result


# ---------------------------------------------------------------------------
# update_size_xs_label
# ---------------------------------------------------------------------------


class TestUpdateSizeXsLabel:
    def test_qualifies_adds_label(self):
        pr = _make_pr(commits=1, additions=1, deletions=1)
        labels = set()
        sut.update_size_xs_label(pr, _make_args(), [_make_file("drivers/foo/bar.c")], labels)
        assert "size: XS" in labels

    def test_zero_changes_qualifies(self):
        pr = _make_pr(commits=1, additions=0, deletions=0)
        labels = set()
        sut.update_size_xs_label(pr, _make_args(), [_make_file("lib/foo.c")], labels)
        assert "size: XS" in labels

    def test_too_many_additions_does_not_qualify(self):
        pr = _make_pr(commits=1, additions=5, deletions=0)
        labels = set()
        sut.update_size_xs_label(pr, _make_args(), [_make_file("lib/foo.c")], labels)
        assert "size: XS" not in labels

    def test_too_many_deletions_does_not_qualify(self):
        pr = _make_pr(commits=1, additions=0, deletions=3)
        labels = set()
        sut.update_size_xs_label(pr, _make_args(), [_make_file("lib/foo.c")], labels)
        assert "size: XS" not in labels

    def test_multiple_commits_does_not_qualify(self):
        pr = _make_pr(commits=2, additions=1, deletions=1)
        labels = set()
        sut.update_size_xs_label(pr, _make_args(), [_make_file("lib/foo.c")], labels)
        assert "size: XS" not in labels

    def test_west_yml_touch_disqualifies(self):
        pr = _make_pr(commits=1, additions=1, deletions=1)
        labels = set()
        sut.update_size_xs_label(pr, _make_args(), [_make_file("west.yml")], labels)
        assert "size: XS" not in labels

    def test_submanifest_touch_disqualifies(self):
        pr = _make_pr(commits=1, additions=1, deletions=1)
        labels = set()
        sut.update_size_xs_label(
            pr, _make_args(), [_make_file("submanifests/optional.yaml")], labels
        )
        assert "size: XS" not in labels

    def test_manifest_among_multiple_files_disqualifies(self):
        pr = _make_pr(commits=1, additions=1, deletions=1)
        labels = set()
        files = [_make_file("lib/foo.c"), _make_file("west.yml")]
        sut.update_size_xs_label(pr, _make_args(), files, labels)
        assert "size: XS" not in labels

    def test_removes_stale_label_when_not_qualifying(self):
        pr = _make_pr(commits=3, additions=10, deletions=5, labels=["size: XS"])
        sut.update_size_xs_label(pr, _make_args(dry_run=False), [_make_file("lib/foo.c")], set())
        pr.remove_from_labels.assert_called_once_with("size: XS")

    def test_does_not_remove_label_on_dry_run(self):
        pr = _make_pr(commits=3, additions=10, deletions=5, labels=["size: XS"])
        sut.update_size_xs_label(pr, _make_args(dry_run=True), [_make_file("lib/foo.c")], set())
        pr.remove_from_labels.assert_not_called()

    def test_no_action_when_no_label_and_not_qualifying(self):
        pr = _make_pr(commits=2, additions=5, deletions=0, labels=[])
        sut.update_size_xs_label(pr, _make_args(), [_make_file("lib/foo.c")], set())
        pr.remove_from_labels.assert_not_called()

    def test_existing_labels_preserved(self):
        pr = _make_pr(commits=1, additions=1, deletions=1)
        labels = {"area: kernel"}
        sut.update_size_xs_label(pr, _make_args(), [_make_file("lib/foo.c")], labels)
        assert "area: kernel" in labels
        assert "size: XS" in labels


# ---------------------------------------------------------------------------
# _pick_assignees
# ---------------------------------------------------------------------------


class TestPickAssignees:
    def _pr(self, login="contributor"):
        return _make_pr(user_login=login)

    def test_single_non_platform_area(self):
        area = _make_area("Networking", maintainers=["alice", "bob"])
        pr = self._pr()
        result = sut._pick_assignees(pr, {area: 3}, {"alice": 3, "bob": 1}, num_files=3)
        assert result == ["alice", "bob"]

    def test_author_is_sole_maintainer_falls_back_to_all_maintainers(self):
        area = _make_area("Networking", maintainers=["contributor"])
        pr = self._pr("contributor")
        all_m = {"contributor": 2}
        result = sut._pick_assignees(pr, {area: 2}, all_m, num_files=2)
        # ranked_assignees stays empty; fallback picks first of all_maintainers
        assert result == ["contributor"]

    def test_author_is_one_of_multiple_maintainers(self):
        area = _make_area("Networking", maintainers=["contributor", "alice"])
        pr = self._pr("contributor")
        result = sut._pick_assignees(pr, {area: 2}, {"contributor": 2, "alice": 1}, num_files=2)
        assert "contributor" not in result
        assert "alice" in result

    def test_platform_area_only_is_used_as_fallback(self):
        area = _make_area("Platform: nrf52", maintainers=["charlie"])
        result = sut._pick_assignees(self._pr(), {area: 2}, {"charlie": 2}, num_files=2)
        assert result == ["charlie"]

    def test_non_platform_beats_platform(self):
        platform = _make_area("Platform: nrf52", maintainers=["charlie"])
        subsys = _make_area("Bluetooth", maintainers=["dave"])
        # area_counter ordered: platform first (higher count), subsys second
        pr = self._pr()
        result = sut._pick_assignees(
            pr, {platform: 5, subsys: 3}, {"charlie": 5, "dave": 3}, num_files=5
        )
        # non-platform (Bluetooth) should be inserted at front and returned
        assert result == ["dave"]

    def test_meta_area_only_assigns_meta_maintainer(self):
        area = _make_area("Documentation", maintainers=["eve"], meta=True)
        result = sut._pick_assignees(self._pr(), {area: 2}, {"eve": 2}, num_files=2)
        assert result == ["eve"]

    def test_meta_area_not_sole_area_skips_meta_logic(self):
        meta = _make_area("Documentation", maintainers=["eve"], meta=True)
        other = _make_area("Kernel", maintainers=["frank"])
        # area_counter is always sorted descending by count before _pick_assignees
        # is called; put the higher-count non-meta area first so it is visited first.
        result = sut._pick_assignees(
            self._pr(), {other: 2, meta: 1}, {"frank": 2, "eve": 1}, num_files=3
        )
        # Kernel (non-platform, non-meta, higher count) wins
        assert result == ["frank"]

    def test_zero_count_area_is_skipped(self):
        area = _make_area("Networking", maintainers=["alice"])
        all_m = {"alice": 0}
        result = sut._pick_assignees(self._pr(), {area: 0}, all_m, num_files=1)
        # count==0 → continue; all_maintainers fallback picks alice
        assert result == ["alice"]

    def test_area_without_maintainers_is_skipped(self):
        area = _make_area("Empty Area", maintainers=[])
        result = sut._pick_assignees(self._pr(), {area: 3}, {}, num_files=3)
        assert result is None

    def test_empty_inputs_returns_none(self):
        result = sut._pick_assignees(self._pr(), {}, {}, num_files=0)
        assert result is None

    def test_zero_num_files_coverage_does_not_divide_by_zero(self):
        area = _make_area("Networking", maintainers=["alice"])
        result = sut._pick_assignees(self._pr(), {area: 1}, {"alice": 1}, num_files=0)
        assert result == ["alice"]


# ---------------------------------------------------------------------------
# setup_logging
# ---------------------------------------------------------------------------


class TestSetupLogging:
    def test_verbose_0_sets_warning(self):
        with patch("logging.basicConfig") as mock_cfg:
            sut.setup_logging(0)
            mock_cfg.assert_called_once()
            assert mock_cfg.call_args.kwargs["level"] == logging.WARNING

    def test_verbose_1_sets_info(self):
        with patch("logging.basicConfig") as mock_cfg:
            sut.setup_logging(1)
            assert mock_cfg.call_args.kwargs["level"] == logging.INFO

    def test_verbose_2_sets_debug(self):
        with patch("logging.basicConfig") as mock_cfg:
            sut.setup_logging(2)
            assert mock_cfg.call_args.kwargs["level"] == logging.DEBUG

    def test_verbose_3_still_sets_debug(self):
        with patch("logging.basicConfig") as mock_cfg:
            sut.setup_logging(3)
            assert mock_cfg.call_args.kwargs["level"] == logging.DEBUG


# ---------------------------------------------------------------------------
# Deferred file-groups  (process_pr integration)
# ---------------------------------------------------------------------------


def _suggested_payload(suggestions):
    """Wrap *suggestions* in the GraphQL envelope suggestedReviewers returns.

    Each entry is (login, is_author); the reviewer sub-object mirrors the real
    schema so the SUT's unwrapping is exercised for real.
    """
    return {
        "data": {
            "repository": {
                "pullRequest": {
                    "suggestedReviewers": [
                        {
                            "isAuthor": is_author,
                            "isCommenter": not is_author,
                            "reviewer": {"login": login},
                        }
                        for login, is_author in suggestions
                    ]
                }
            }
        }
    }


def _make_process_pr_harness(areas_per_file, pr_user="someone"):
    """Return (gh, args, maintainer_file, pr) ready for sut.process_pr().

    *areas_per_file* maps filename strings to lists of _Area (or _DeferredArea)
    instances.  All GitHub API calls are stubbed; dry_run is False so that
    create_review_request and add_to_assignees are actually invoked.
    """
    pr = MagicMock()
    pr.draft = False
    pr.state = "open"
    pr.commits = 1
    pr.additions = 5
    pr.deletions = 0
    pr.number = 99
    pr.labels = []
    pr.user = SimpleNamespace(login=pr_user)
    pr.assignee = None
    pr.assignees = []
    pr.get_files.return_value = [SimpleNamespace(filename=fn) for fn in areas_per_file]
    pr.get_reviews.return_value = []
    pr.get_review_requests.return_value = ([], [])
    pr.get_issue_events.return_value = []
    pr.get_issue_comments.return_value = []
    pr.get_review_comments.return_value = []

    mf = MagicMock()
    mf.path2areas.side_effect = lambda p: areas_per_file.get(p, [])
    mf.name2areas.return_value = []
    mf.areas = {}
    for fname_areas in areas_per_file.values():
        for area in fname_areas:
            mf.areas[area.name] = area

    # Use MagicMock for user objects so they are hashable (required by
    # _add_reviewers which stores them in a set).
    user_cache = {}

    def _get_user(login):
        if login not in user_cache:
            u = MagicMock()
            u.login = login
            user_cache[login] = u
        return user_cache[login]

    gh = MagicMock()
    gh_repo = MagicMock()
    gh.get_repo.return_value = gh_repo
    gh.get_user.side_effect = _get_user
    # No GitHub-suggested reviewers by default; tests override as needed.
    gh.requester.graphql_query.return_value = ({}, _suggested_payload([]))
    gh_repo.get_pull.return_value = pr
    gh_repo.has_in_collaborators.return_value = True
    # No Git history by default; individual tests override per path as needed.
    gh_repo.get_commits.return_value = []

    args = SimpleNamespace(
        org="testorg",
        repo="testrepo",
        dry_run=False,
        updated_manifest=None,
        updated_maintainer_file=None,
        size_labels=False,
        reset=False,
    )
    return gh, args, mf, pr


def _reviewers_requested(pr):
    """Collect all logins passed to pr.create_review_request across all calls."""
    result = []
    for call in pr.create_review_request.call_args_list:
        reviewers = call.kwargs.get("reviewers", call.args[0] if call.args else [])
        result.extend(reviewers)
    return result


def _assignees_set(pr):
    """Collect all logins passed to pr.add_to_assignees across all calls."""
    result = []
    for call in pr.add_to_assignees.call_args_list:
        if call.args:
            result.append(call.args[0].login)
    return result


class TestDeferredFileGroups:
    """Verify defer-to-other-areas file-group behaviour in process_pr."""

    def test_deferred_maintainer_added_as_reviewer(self):
        """Deferred area maintainer must appear in the review request."""
        clock_area = _DeferredArea("Clock Control", maintainers=["clock_m"])
        platform_area = _make_area("Platform: nrf52", maintainers=["platform_m"])
        areas = {"drivers/clk/nrf.c": [clock_area, platform_area]}

        gh, args, mf, pr = _make_process_pr_harness(areas)
        sut.process_pr(gh, args, mf, 99)

        assert "clock_m" in _reviewers_requested(pr)

    def test_deferred_maintainer_not_set_as_assignee(self):
        """When a non-deferred area covers the same file, the deferred area's
        maintainer must not become the PR assignee."""
        clock_area = _DeferredArea("Clock Control", maintainers=["clock_m"])
        platform_area = _make_area("Platform: nrf52", maintainers=["platform_m"])
        areas = {"drivers/clk/nrf.c": [clock_area, platform_area]}

        gh, args, mf, pr = _make_process_pr_harness(areas)
        sut.process_pr(gh, args, mf, 99)

        assert "clock_m" not in _assignees_set(pr)

    def test_non_deferred_maintainer_becomes_assignee(self):
        """The non-deferred area's maintainer must be set as the assignee."""
        clock_area = _DeferredArea("Clock Control", maintainers=["clock_m"])
        platform_area = _make_area("Platform: nrf52", maintainers=["platform_m"])
        areas = {"drivers/clk/nrf.c": [clock_area, platform_area]}

        gh, args, mf, pr = _make_process_pr_harness(areas)
        sut.process_pr(gh, args, mf, 99)

        assert "platform_m" in _assignees_set(pr)

    def test_exclusively_deferred_area_used_normally(self):
        """When only a deferred area covers a file (no competing area),
        the deferral flag has no effect and the maintainer is assigned."""
        clock_area = _DeferredArea("Clock Control", maintainers=["clock_m"])
        areas = {"drivers/clk/nrf.c": [clock_area]}

        gh, args, mf, pr = _make_process_pr_harness(areas)
        sut.process_pr(gh, args, mf, 99)

        assert "clock_m" in _assignees_set(pr)

    def test_non_deferred_area_unaffected(self):
        """A normal (non-deferred) area without any competing deferred area
        works exactly as before."""
        subsys_area = _make_area("Networking", maintainers=["net_m"])
        areas = {"subsys/net/foo.c": [subsys_area]}

        gh, args, mf, pr = _make_process_pr_harness(areas)
        sut.process_pr(gh, args, mf, 99)

        assert "net_m" in _assignees_set(pr)


# ---------------------------------------------------------------------------
# _build_reviewer_candidates
# ---------------------------------------------------------------------------


class TestBuildReviewerCandidates:
    """Reviewer candidates must list every area's maintainers before any
    collaborator, so that truncation to the reviewer cap never drops a
    maintainer in favour of a higher-weight area's collaborator."""

    @staticmethod
    def _mf(*areas):
        mf = SimpleNamespace(areas={area.name: area for area in areas})
        return mf

    def test_all_maintainers_precede_all_collaborators(self):
        net = _make_area("Networking", maintainers=["net_m"], collaborators=["net_c"])
        usb = _make_area("USB", maintainers=["usb_m"], collaborators=["usb_c"])
        mf = self._mf(net, usb)

        result = sut._build_reviewer_candidates(mf, {net: 5, usb: 2}, set(), set(), set())

        assert result == ["net_m", "usb_m", "net_c", "usb_c"]

    def test_maintainer_order_follows_area_weight(self):
        net = _make_area("Networking", maintainers=["net_m"])
        usb = _make_area("USB", maintainers=["usb_m"])
        mf = self._mf(net, usb)

        # area_counter is pre-sorted by descending weight by the caller.
        result = sut._build_reviewer_candidates(mf, {usb: 9, net: 1}, set(), set(), set())

        assert result == ["usb_m", "net_m"]

    def test_extra_and_deferred_reviewers_join_maintainer_tier(self):
        net = _make_area("Networking", maintainers=["net_m"], collaborators=["net_c"])
        mf = self._mf(net)

        result = sut._build_reviewer_candidates(
            mf, {net: 1}, {"path_c"}, {"extra_m"}, {"deferred_m"}
        )

        assert result == ["net_m", "extra_m", "deferred_m", "net_c", "path_c"]

    def test_duplicates_are_removed_keeping_first_occurrence(self):
        net = _make_area("Networking", maintainers=["alice"], collaborators=["bob"])
        usb = _make_area("USB", maintainers=["bob"], collaborators=["alice"])
        mf = self._mf(net, usb)

        result = sut._build_reviewer_candidates(mf, {net: 3, usb: 1}, set(), set(), set())

        # 'bob' is a maintainer of USB, so he keeps his maintainer-tier slot
        # rather than the collaborator slot he also holds in Networking.
        assert result == ["alice", "bob"]

    def test_no_areas_returns_only_extra_reviewers(self):
        mf = self._mf()

        result = sut._build_reviewer_candidates(mf, {}, set(), {"extra_m"}, set())

        assert result == ["extra_m"]

    def test_broad_pr_requests_every_maintainer_then_collaborators(self):
        """A broad PR requests everyone eligible -- no cap -- with all area
        maintainers ranked ahead of any collaborator."""
        areas_per_file = {}
        for i in range(8):
            area = _make_area(
                f"Area{i}",
                maintainers=[f"m{i}"],
                collaborators=[f"c{i}a", f"c{i}b", f"c{i}c", f"c{i}d"],
            )
            areas_per_file[f"subsys/area{i}/foo.c"] = [area]

        gh, args, mf, pr = _make_process_pr_harness(areas_per_file)
        sut.process_pr(gh, args, mf, 99)

        requested = _reviewers_requested(pr)
        # 8 maintainers + 8*4 collaborators, none dropped.
        assert len(requested) == 40
        assert requested[:8] == [f"m{i}" for i in range(8)]


# ---------------------------------------------------------------------------
# _thin_areas
# ---------------------------------------------------------------------------


class TestThinAreas:
    @staticmethod
    def _mf(*areas):
        return SimpleNamespace(areas={area.name: area for area in areas})

    def test_area_without_maintainers_is_thin(self):
        area = _make_area("Orphan", maintainers=[], collaborators=["a", "b", "c", "d"])
        mf = self._mf(area)
        assert sut._thin_areas(mf, {area: 1}) == {"Orphan"}

    def test_area_with_few_collaborators_is_thin(self):
        area = _make_area("Small", maintainers=["m"], collaborators=["a", "b"])
        mf = self._mf(area)
        assert sut._thin_areas(mf, {area: 1}) == {"Small"}

    def test_well_covered_area_is_not_thin(self):
        area = _make_area("Big", maintainers=["m"], collaborators=["a", "b", "c"])
        mf = self._mf(area)
        assert sut._thin_areas(mf, {area: 1}) == set()

    def test_threshold_is_inclusive(self):
        area = _make_area(
            "AtLimit",
            maintainers=["m"],
            collaborators=["a"] * sut.THIN_AREA_COLLABORATORS,
        )
        mf = self._mf(area)
        assert sut._thin_areas(mf, {area: 1}) == {"AtLimit"}

    def test_mixed_areas_reports_only_thin_ones(self):
        thin = _make_area("Thin", maintainers=[], collaborators=[])
        full = _make_area("Full", maintainers=["m"], collaborators=["a", "b", "c"])
        mf = self._mf(thin, full)
        assert sut._thin_areas(mf, {thin: 2, full: 1}) == {"Thin"}

    def test_meta_area_is_never_thin(self):
        # Meta-areas name few people on purpose; they must not pull in
        # heuristic reviewers on top of their own maintainers.
        area = _make_area("Documentation", maintainers=["m"], collaborators=[], meta=True)
        mf = self._mf(area)
        assert sut._thin_areas(mf, {area: 1}) == set()

    def test_meta_area_excluded_but_thin_area_still_reported(self):
        meta = _make_area("Documentation", maintainers=[], collaborators=[], meta=True)
        thin = _make_area("Thin", maintainers=[], collaborators=[])
        mf = self._mf(meta, thin)
        assert sut._thin_areas(mf, {meta: 2, thin: 1}) == {"Thin"}


# ---------------------------------------------------------------------------
# _history_reviewers
# ---------------------------------------------------------------------------


def _commit(login):
    """A stub commit whose GitHub-resolved author has *login* (None => unmatched)."""
    author = SimpleNamespace(login=login) if login is not None else None
    return SimpleNamespace(author=author)


def _repo_with_history(history):
    """gh_repo stub whose get_commits(path=..., since=...) returns history[path] (or [])."""
    repo = MagicMock()
    repo.get_commits.side_effect = lambda path, since=None: list(history.get(path, []))
    return repo


class TestHistoryReviewers:
    def test_ranks_contributors_by_commit_count(self):
        history = {
            "f.c": [_commit("alice"), _commit("bob"), _commit("alice")],
        }
        repo = _repo_with_history(history)
        result = sut._history_reviewers(repo, {"f.c"}, exclude=set())
        assert result == ["alice", "bob"]

    def test_excluded_logins_are_dropped(self):
        history = {"f.c": [_commit("author"), _commit("alice")]}
        repo = _repo_with_history(history)
        result = sut._history_reviewers(repo, {"f.c"}, exclude={"author"})
        assert result == ["alice"]

    def test_unmatched_authors_are_ignored(self):
        history = {"f.c": [_commit(None), _commit("alice"), _commit(None)]}
        repo = _repo_with_history(history)
        result = sut._history_reviewers(repo, {"f.c"}, exclude=set())
        assert result == ["alice"]

    def test_result_is_capped(self):
        history = {
            "f.c": [_commit(name) for name in ["a", "a", "b", "b", "c", "c", "d", "d"]],
        }
        repo = _repo_with_history(history)
        result = sut._history_reviewers(repo, {"f.c"}, exclude=set())
        assert len(result) == sut.MAX_HISTORY_REVIEWERS

    def test_commits_sampled_per_file_are_bounded(self):
        # More commits than the per-file sample budget; only the first
        # HISTORY_COMMITS_PER_FILE are counted.
        commits = [_commit("early")] * sut.HISTORY_COMMITS_PER_FILE + [_commit("late")]
        repo = _repo_with_history({"f.c": commits})
        result = sut._history_reviewers(repo, {"f.c"}, exclude=set())
        assert result == ["early"]

    def test_github_exception_on_a_file_is_skipped(self):
        def _raise_or_return(path, since=None):
            if path == "bad.c":
                raise sut.GithubException("no history")
            return [_commit("alice")]

        repo = MagicMock()
        repo.get_commits.side_effect = _raise_or_return
        result = sut._history_reviewers(repo, {"bad.c", "good.c"}, exclude=set())
        assert result == ["alice"]

    def test_contributions_aggregate_across_files(self):
        history = {
            "a.c": [_commit("alice")],
            "b.c": [_commit("alice"), _commit("bob")],
        }
        repo = _repo_with_history(history)
        result = sut._history_reviewers(repo, {"a.c", "b.c"}, exclude=set())
        assert result == ["alice", "bob"]

    def test_history_is_limited_to_one_year(self):
        repo = _repo_with_history({"f.c": [_commit("alice")]})
        sut._history_reviewers(repo, {"f.c"}, exclude=set())

        since = repo.get_commits.call_args.kwargs["since"]
        age = datetime.datetime.now(datetime.UTC) - since
        assert sut.HISTORY_MAX_AGE_DAYS == 365
        # Allow a minute of slack for the time spent between the two calls.
        assert abs(age - datetime.timedelta(days=sut.HISTORY_MAX_AGE_DAYS)) < datetime.timedelta(
            minutes=1
        )


# ---------------------------------------------------------------------------
# _suggested_reviewers
# ---------------------------------------------------------------------------


def _gh_with_suggestions(payload):
    """Github stub whose requester.graphql_query returns *payload*."""
    gh = MagicMock()
    gh.requester.graphql_query.return_value = ({}, payload)
    return gh


def _suggest_args():
    return SimpleNamespace(org="zephyrproject-rtos", repo="zephyr")


def _suggest_pr(number=99):
    return SimpleNamespace(number=number)


class TestSuggestedReviewers:
    def test_returns_suggested_logins(self):
        gh = _gh_with_suggestions(_suggested_payload([("alice", True), ("bob", True)]))
        result = sut._suggested_reviewers(gh, _suggest_args(), _suggest_pr(), exclude=set())
        assert result == ["alice", "bob"]

    def test_authors_rank_ahead_of_commenters(self):
        gh = _gh_with_suggestions(_suggested_payload([("commenter", False), ("author", True)]))
        result = sut._suggested_reviewers(gh, _suggest_args(), _suggest_pr(), exclude=set())
        assert result == ["author", "commenter"]

    def test_excluded_logins_are_dropped(self):
        gh = _gh_with_suggestions(_suggested_payload([("alice", True), ("known", True)]))
        result = sut._suggested_reviewers(gh, _suggest_args(), _suggest_pr(), exclude={"known"})
        assert result == ["alice"]

    def test_empty_suggestion_list(self):
        gh = _gh_with_suggestions(_suggested_payload([]))
        assert sut._suggested_reviewers(gh, _suggest_args(), _suggest_pr(), set()) == []

    def test_null_suggestion_list_is_tolerated(self):
        payload = {"data": {"repository": {"pullRequest": {"suggestedReviewers": None}}}}
        gh = _gh_with_suggestions(payload)
        assert sut._suggested_reviewers(gh, _suggest_args(), _suggest_pr(), set()) == []

    def test_malformed_response_returns_empty(self):
        gh = _gh_with_suggestions({"data": {"repository": None}})
        assert sut._suggested_reviewers(gh, _suggest_args(), _suggest_pr(), set()) == []

    def test_graphql_exception_returns_empty(self):
        gh = MagicMock()
        gh.requester.graphql_query.side_effect = sut.GithubException("boom")
        assert sut._suggested_reviewers(gh, _suggest_args(), _suggest_pr(), set()) == []

    def test_query_is_parameterised_with_pr_coordinates(self):
        gh = _gh_with_suggestions(_suggested_payload([]))
        sut._suggested_reviewers(gh, _suggest_args(), _suggest_pr(1234), set())
        _query, variables = gh.requester.graphql_query.call_args.args
        assert variables == {
            "owner": "zephyrproject-rtos",
            "name": "zephyr",
            "number": 1234,
        }


# ---------------------------------------------------------------------------
# Heuristic reviewers  (process_pr integration)
# ---------------------------------------------------------------------------


class TestHistoryReviewersIntegration:
    def test_orphan_area_gets_history_reviewer(self):
        """An area with no maintainers pulls a recent contributor as reviewer."""
        area = _make_area("Orphan", maintainers=[], collaborators=[])
        areas = {"subsys/orphan/foo.c": [area]}

        gh, args, mf, pr = _make_process_pr_harness(areas)
        gh.get_repo.return_value.get_commits.side_effect = lambda path, since=None: [
            SimpleNamespace(author=SimpleNamespace(login="frequent_contributor"))
        ]
        sut.process_pr(gh, args, mf, 99)

        assert "frequent_contributor" in _reviewers_requested(pr)

    def test_history_reviewer_excludes_pr_author(self):
        area = _make_area("Orphan", maintainers=[], collaborators=[])
        areas = {"subsys/orphan/foo.c": [area]}

        gh, args, mf, pr = _make_process_pr_harness(areas, pr_user="self")
        gh.get_repo.return_value.get_commits.side_effect = lambda path, since=None: [
            SimpleNamespace(author=SimpleNamespace(login="self"))
        ]
        sut.process_pr(gh, args, mf, 99)

        assert "self" not in _reviewers_requested(pr)

    def test_well_covered_area_skips_history_lookup(self):
        """A fully-staffed area must not trigger any commit-history query."""
        area = _make_area("Full", maintainers=["m"], collaborators=["c1", "c2", "c3"])
        areas = {"subsys/full/foo.c": [area]}

        gh, args, mf, _ = _make_process_pr_harness(areas)
        sut.process_pr(gh, args, mf, 99)

        gh.get_repo.return_value.get_commits.assert_not_called()

    def test_well_covered_area_skips_suggestion_query(self):
        """A fully-staffed area must not trigger the GraphQL query either."""
        area = _make_area("Full", maintainers=["m"], collaborators=["c1", "c2", "c3"])
        areas = {"subsys/full/foo.c": [area]}

        gh, args, mf, _ = _make_process_pr_harness(areas)
        sut.process_pr(gh, args, mf, 99)

        gh.requester.graphql_query.assert_not_called()

    def test_github_suggestion_is_preferred_over_history(self):
        """When GitHub suggests reviewers, the commit walk is skipped."""
        area = _make_area("Orphan", maintainers=[], collaborators=[])
        areas = {"subsys/orphan/foo.c": [area]}

        gh, args, mf, pr = _make_process_pr_harness(areas)
        gh.requester.graphql_query.return_value = (
            {},
            _suggested_payload([("suggested_by_github", True)]),
        )
        gh.get_repo.return_value.get_commits.side_effect = lambda path, since=None: [
            SimpleNamespace(author=SimpleNamespace(login="from_history"))
        ]
        sut.process_pr(gh, args, mf, 99)

        requested = _reviewers_requested(pr)
        assert "suggested_by_github" in requested
        assert "from_history" not in requested
        gh.get_repo.return_value.get_commits.assert_not_called()

    def test_falls_back_to_history_when_github_suggests_nobody(self):
        """GitHub's suggestions are empty for most PRs; the walk must cover that."""
        area = _make_area("Orphan", maintainers=[], collaborators=[])
        areas = {"subsys/orphan/foo.c": [area]}

        gh, args, mf, pr = _make_process_pr_harness(areas)
        gh.requester.graphql_query.return_value = ({}, _suggested_payload([]))
        gh.get_repo.return_value.get_commits.side_effect = lambda path, since=None: [
            SimpleNamespace(author=SimpleNamespace(login="from_history"))
        ]
        sut.process_pr(gh, args, mf, 99)

        assert "from_history" in _reviewers_requested(pr)

    def test_github_suggestions_are_capped(self):
        area = _make_area("Orphan", maintainers=[], collaborators=[])
        areas = {"subsys/orphan/foo.c": [area]}

        gh, args, mf, pr = _make_process_pr_harness(areas)
        gh.requester.graphql_query.return_value = (
            {},
            _suggested_payload([(f"s{i}", True) for i in range(10)]),
        )
        sut.process_pr(gh, args, mf, 99)

        requested = _reviewers_requested(pr)
        assert len([r for r in requested if r.startswith("s")]) == sut.MAX_HISTORY_REVIEWERS

    def test_github_suggestion_excludes_pr_author(self):
        area = _make_area("Orphan", maintainers=[], collaborators=[])
        areas = {"subsys/orphan/foo.c": [area]}

        gh, args, mf, pr = _make_process_pr_harness(areas, pr_user="self")
        gh.requester.graphql_query.return_value = (
            {},
            _suggested_payload([("self", True)]),
        )
        sut.process_pr(gh, args, mf, 99)

        assert "self" not in _reviewers_requested(pr)


class TestUnmatchedFiles:
    """A file matching no area at all yields no reviewer from MAINTAINERS.yml,
    so it must still reach the heuristics (see PR #112563, a lone doc change
    to an orphaned path)."""

    def test_pr_with_no_matching_area_uses_github_suggestion(self):
        gh, args, mf, pr = _make_process_pr_harness({"doc/orphaned.rst": []})
        gh.requester.graphql_query.return_value = (
            {},
            _suggested_payload([("foouser", False)]),
        )
        sut.process_pr(gh, args, mf, 99)

        assert "foouser" in _reviewers_requested(pr)

    def test_pr_with_no_matching_area_falls_back_to_history(self):
        gh, args, mf, pr = _make_process_pr_harness({"doc/orphaned.rst": []})
        gh.requester.graphql_query.return_value = ({}, _suggested_payload([]))
        gh.get_repo.return_value.get_commits.side_effect = lambda path, since=None: [
            SimpleNamespace(author=SimpleNamespace(login="baruser"))
        ]
        sut.process_pr(gh, args, mf, 99)

        assert "baruser" in _reviewers_requested(pr)

    def test_history_is_scoped_to_the_orphaned_files(self):
        """Only the unmatched file is walked, not the well-covered one."""
        covered = _make_area("Full", maintainers=["m"], collaborators=["c1", "c2", "c3"])
        areas = {"doc/orphaned.rst": [], "subsys/full/foo.c": [covered]}

        gh, args, mf, _ = _make_process_pr_harness(areas)
        gh.requester.graphql_query.return_value = ({}, _suggested_payload([]))
        gh.get_repo.return_value.get_commits.side_effect = lambda path, since=None: [
            SimpleNamespace(author=SimpleNamespace(login="doc_author"))
        ]
        # The covered area alone would satisfy MIN_TOTAL_REVIEWERS; disable that
        # gate so this test covers only which files get walked.
        with patch.object(sut, "MIN_TOTAL_REVIEWERS", 99):
            sut.process_pr(gh, args, mf, 99)

        walked = [
            call.kwargs.get("path") for call in gh.get_repo.return_value.get_commits.call_args_list
        ]
        assert walked == ["doc/orphaned.rst"]

    def test_maintainers_still_rank_ahead_of_heuristics(self):
        """A mixed PR keeps real maintainers first; heuristics only top up."""
        covered = _make_area("Full", maintainers=["real_m"], collaborators=["c1", "c2", "c3"])
        areas = {"doc/orphaned.rst": [], "subsys/full/foo.c": [covered]}

        gh, args, mf, pr = _make_process_pr_harness(areas)
        gh.requester.graphql_query.return_value = (
            {},
            _suggested_payload([("heuristic_r", True)]),
        )
        with patch.object(sut, "MIN_TOTAL_REVIEWERS", 99):
            sut.process_pr(gh, args, mf, 99)

        requested = _reviewers_requested(pr)
        assert requested.index("real_m") < requested.index("heuristic_r")

    def test_fully_matched_pr_triggers_no_heuristic(self):
        covered = _make_area("Full", maintainers=["m"], collaborators=["c1", "c2", "c3"])
        gh, args, mf, _ = _make_process_pr_harness({"subsys/full/foo.c": [covered]})
        sut.process_pr(gh, args, mf, 99)

        gh.requester.graphql_query.assert_not_called()
        gh.get_repo.return_value.get_commits.assert_not_called()

    def test_no_reviewer_found_at_all_does_not_crash(self):
        """Orphaned file with no suggestions and no history: nothing requested."""
        gh, args, mf, pr = _make_process_pr_harness({"doc/orphaned.rst": []})
        gh.requester.graphql_query.return_value = ({}, _suggested_payload([]))
        gh.get_repo.return_value.get_commits.side_effect = lambda path, since=None: []
        sut.process_pr(gh, args, mf, 99)

        assert _reviewers_requested(pr) == []


class TestHeuristicThreshold:
    """The heuristics only top up PRs that the areas left short of reviewers."""

    def test_enough_area_reviewers_skips_heuristics(self):
        """An orphaned file does not trigger a walk when the areas staffed the PR."""
        covered = _make_area("Full", maintainers=["m"], collaborators=["c1", "c2", "c3"])
        areas = {"doc/orphaned.rst": [], "subsys/full/foo.c": [covered]}

        gh, args, mf, pr = _make_process_pr_harness(areas)
        sut.process_pr(gh, args, mf, 99)

        gh.requester.graphql_query.assert_not_called()
        gh.get_repo.return_value.get_commits.assert_not_called()
        assert _reviewers_requested(pr) == ["m", "c1", "c2", "c3"]

    def test_too_few_area_reviewers_runs_heuristics(self):
        thin = _make_area("Thin", maintainers=["m"], collaborators=[])
        areas = {"subsys/thin/foo.c": [thin]}

        gh, args, mf, pr = _make_process_pr_harness(areas)
        gh.requester.graphql_query.return_value = (
            {},
            _suggested_payload([("heuristic_r", True)]),
        )
        sut.process_pr(gh, args, mf, 99)

        assert "heuristic_r" in _reviewers_requested(pr)

    def test_author_does_not_count_towards_the_threshold(self):
        """The author cannot review their own PR, so they leave the PR short."""
        thin = _make_area("Thin", maintainers=["self"], collaborators=["c1", "c2"])
        areas = {"subsys/thin/foo.c": [thin]}

        gh, args, mf, pr = _make_process_pr_harness(areas, pr_user="self")
        gh.requester.graphql_query.return_value = (
            {},
            _suggested_payload([("heuristic_r", True)]),
        )
        sut.process_pr(gh, args, mf, 99)

        # Only c1 and c2 can review: one short of MIN_TOTAL_REVIEWERS.
        assert "heuristic_r" in _reviewers_requested(pr)

    def test_threshold_counts_reviewers_across_areas(self):
        """Two thin areas can jointly staff a PR that neither could alone."""
        thin_a = _make_area("ThinA", maintainers=["ma"], collaborators=["ca"])
        thin_b = _make_area("ThinB", maintainers=["mb"], collaborators=["cb"])
        areas = {"subsys/a/foo.c": [thin_a], "subsys/b/bar.c": [thin_b]}

        gh, args, mf, _ = _make_process_pr_harness(areas)
        sut.process_pr(gh, args, mf, 99)

        gh.requester.graphql_query.assert_not_called()
        gh.get_repo.return_value.get_commits.assert_not_called()


# ---------------------------------------------------------------------------
# Reviewer cap removal  (_add_reviewers)
# ---------------------------------------------------------------------------


def _make_add_reviewers_stubs(author="author"):
    """Return (gh, gh_repo, pr, args) for a direct _add_reviewers() call."""
    pr = MagicMock()
    pr.number = 7
    pr.get_reviews.return_value = []
    pr.get_review_requests.return_value = ([], [])
    pr.get_issue_events.return_value = []
    pr.get_issue_comments.return_value = []

    cache = {}

    def _get_user(login):
        if login not in cache:
            user = MagicMock()
            user.login = login
            cache[login] = user
        return cache[login]

    gh = MagicMock()
    gh.get_user.side_effect = _get_user
    pr.user = _get_user(author)

    gh_repo = MagicMock()
    gh_repo.has_in_collaborators.return_value = True

    return gh, gh_repo, pr, SimpleNamespace(dry_run=False)


class TestNoReviewerCap:
    """The old MAX_REVIEWERS cap is gone: every eligible candidate is
    requested, and GitHub decides what it will accept."""

    def test_all_candidates_are_requested(self):
        gh, gh_repo, pr, args = _make_add_reviewers_stubs()
        candidates = [f"u{i}" for i in range(40)]

        sut._add_reviewers(gh, gh_repo, pr, args, candidates)

        assert _reviewers_requested(pr) == candidates

    def test_many_existing_reviewers_no_longer_restricts_candidates(self):
        """Previously, a PR at or over the cap discarded the candidate list and
        fell back to primary-area maintainers only."""
        gh, gh_repo, pr, args = _make_add_reviewers_stubs()
        pr.get_reviews.return_value = [
            SimpleNamespace(user=gh.get_user(f"old{i}")) for i in range(20)
        ]

        sut._add_reviewers(gh, gh_repo, pr, args, ["m1", "m2", "c1"])

        assert _reviewers_requested(pr) == ["m1", "m2", "c1"]

    def test_existing_reviewers_are_still_skipped(self):
        gh, gh_repo, pr, args = _make_add_reviewers_stubs()
        pr.get_review_requests.return_value = ([gh.get_user("already")], [])

        sut._add_reviewers(gh, gh_repo, pr, args, ["already", "fresh"])

        assert _reviewers_requested(pr) == ["fresh"]

    def test_author_is_still_skipped(self):
        gh, gh_repo, pr, args = _make_add_reviewers_stubs(author="me")

        sut._add_reviewers(gh, gh_repo, pr, args, ["me", "someone"])

        assert _reviewers_requested(pr) == ["someone"]

    def test_dry_run_requests_nobody(self):
        gh, gh_repo, pr, args = _make_add_reviewers_stubs()
        args.dry_run = True

        sut._add_reviewers(gh, gh_repo, pr, args, ["a", "b"])

        pr.create_review_request.assert_not_called()


class TestReviewRequestRetry:
    """A rejected bulk request must not cost the PR all of its reviewers."""

    def test_rejection_retries_with_highest_ranked_candidates(self):
        gh, gh_repo, pr, args = _make_add_reviewers_stubs()
        candidates = [f"u{i}" for i in range(25)]
        pr.create_review_request.side_effect = [sut.GithubException("too many"), None]

        sut._add_reviewers(gh, gh_repo, pr, args, candidates)

        assert pr.create_review_request.call_count == 2
        retried = pr.create_review_request.call_args_list[1].kwargs["reviewers"]
        assert retried == candidates[: sut.REVIEWER_RETRY_BATCH]

    def test_small_request_is_not_retried(self):
        gh, gh_repo, pr, args = _make_add_reviewers_stubs()
        pr.create_review_request.side_effect = sut.GithubException("nope")

        sut._add_reviewers(gh, gh_repo, pr, args, ["a", "b"])

        # Nothing to trim, so a retry would just repeat the failing call.
        assert pr.create_review_request.call_count == 1

    def test_failing_retry_does_not_raise(self):
        gh, gh_repo, pr, args = _make_add_reviewers_stubs()
        pr.create_review_request.side_effect = sut.GithubException("nope")

        sut._add_reviewers(gh, gh_repo, pr, args, [f"u{i}" for i in range(25)])

        assert pr.create_review_request.call_count == 2


# ---------------------------------------------------------------------------
# Reset mode  (--reset / _reset_pr)
# ---------------------------------------------------------------------------


def _user(login):
    return SimpleNamespace(login=login)


def _label(name):
    return SimpleNamespace(name=name)


def _requests_deleted(pr):
    """Logins passed to delete_review_request() across all calls."""
    return [
        login
        for call in pr.delete_review_request.call_args_list
        for login in call.kwargs.get("reviewers", call.args[0] if call.args else [])
    ]


def _labels_removed(pr):
    """Label names passed to remove_from_labels() across all calls."""
    return [name for call in pr.remove_from_labels.call_args_list for name in call.args]


class TestResetPr:
    @staticmethod
    def _mf(*areas):
        mf = MagicMock()
        mf.areas = {area.name: area for area in areas}
        return mf

    def _pr(self, requested=(), reviewed=(), commented=(), assignees=(), labels=()):
        pr = MagicMock()
        pr.number = 99
        pr.labels = [_label(name) for name in labels]
        pr.assignees = [_user(login) for login in assignees]
        pr.get_review_requests.return_value = ([_user(u) for u in requested], [])
        pr.get_reviews.return_value = [SimpleNamespace(user=_user(u)) for u in reviewed]
        pr.get_issue_comments.return_value = [SimpleNamespace(user=_user(u)) for u in commented]
        pr.get_review_comments.return_value = []
        return pr

    def test_uninvolved_reviewers_are_removed(self):
        pr = self._pr(requested=["idle_a", "idle_b"])
        sut._reset_pr(pr, _make_args(), self._mf())
        assert sorted(_requests_deleted(pr)) == ["idle_a", "idle_b"]

    def test_reviewer_who_reviewed_is_kept(self):
        pr = self._pr(requested=["reviewer", "idle"], reviewed=["reviewer"])
        sut._reset_pr(pr, _make_args(), self._mf())
        assert _requests_deleted(pr) == ["idle"]

    @pytest.mark.parametrize("state", ["APPROVED", "CHANGES_REQUESTED", "COMMENTED", "DISMISSED"])
    def test_submitted_review_is_never_removed(self, state):
        """Whatever the verdict, having reviewed protects the review request.

        GitHub normally clears the pending request when a review is submitted,
        so such a reviewer is not in get_review_requests() at all; this covers
        the case where someone re-requested a review afterwards.
        """
        pr = self._pr(requested=["reviewer", "idle"])
        pr.get_reviews.return_value = [SimpleNamespace(user=_user("reviewer"), state=state)]

        sut._reset_pr(pr, _make_args(), self._mf())

        assert "reviewer" not in _requests_deleted(pr)
        assert _requests_deleted(pr) == ["idle"]

    def test_reviewer_who_commented_is_kept(self):
        pr = self._pr(requested=["talker", "idle"], commented=["talker"])
        sut._reset_pr(pr, _make_args(), self._mf())
        assert _requests_deleted(pr) == ["idle"]

    def test_review_thread_comment_counts_as_interaction(self):
        pr = self._pr(requested=["nitpicker", "idle"])
        pr.get_review_comments.return_value = [SimpleNamespace(user=_user("nitpicker"))]
        sut._reset_pr(pr, _make_args(), self._mf())
        assert _requests_deleted(pr) == ["idle"]

    def test_reviewer_about_to_be_requested_again_is_kept(self):
        """Withdrawing a request the run is about to re-send would notify twice."""
        pr = self._pr(requested=["still_owns_it", "idle"])
        sut._reset_pr(pr, _make_args(), self._mf(), keep=["still_owns_it"])
        assert _requests_deleted(pr) == ["idle"]

    def test_team_review_requests_are_left_alone(self):
        pr = self._pr(requested=["idle"])
        pr.get_review_requests.return_value = ([_user("idle")], [SimpleNamespace(slug="a-team")])

        sut._reset_pr(pr, _make_args(), self._mf())

        assert _requests_deleted(pr) == ["idle"]
        assert pr.delete_review_request.call_args.kwargs.get("team_reviewers") is None

    def test_assignees_are_removed(self):
        pr = self._pr(assignees=["assignee_a", "assignee_b"])
        sut._reset_pr(pr, _make_args(), self._mf())
        pr.remove_from_assignees.assert_called_once_with("assignee_a", "assignee_b")

    def test_only_maintainer_file_labels_are_removed(self):
        area = _make_area("Kernel", labels=["area: Kernel"])
        pr = self._pr(labels=["area: Kernel", "bug", "size: M"])
        removed = sut._reset_pr(pr, _make_args(), self._mf(area))
        assert _labels_removed(pr) == ["area: Kernel"]
        assert removed == {"area: Kernel"}

    def test_dry_run_changes_nothing(self):
        area = _make_area("Kernel", labels=["area: Kernel"])
        pr = self._pr(requested=["idle"], assignees=["a"], labels=["area: Kernel"])
        removed = sut._reset_pr(pr, _make_args(dry_run=True), self._mf(area))

        pr.delete_review_request.assert_not_called()
        pr.remove_from_assignees.assert_not_called()
        pr.remove_from_labels.assert_not_called()
        # Still reported, so the caller can log what a real run would do.
        assert removed == {"area: Kernel"}


class TestProcessPrReset:
    @staticmethod
    def _harness(areas_per_file, **kwargs):
        gh, args, mf, pr = _make_process_pr_harness(areas_per_file, **kwargs)
        args.reset = True
        return gh, args, mf, pr

    def test_labels_are_reapplied_after_removal(self):
        area = _make_area("Kernel", maintainers=["m"], labels=["area: Kernel"])
        gh, args, mf, pr = self._harness({"kernel/sched.c": [area]})
        pr.labels = [_label("area: Kernel")]

        sut.process_pr(gh, args, mf, 99)

        # Removed by the reset, then re-added by the normal flow rather than
        # being considered already present.
        assert _labels_removed(pr) == ["area: Kernel"]
        assert "area: Kernel" in _labels_added(pr)

    def test_assignee_is_set_again_after_removal(self):
        area = _make_area("Kernel", maintainers=["m"], labels=["area: Kernel"])
        gh, args, mf, pr = self._harness({"kernel/sched.c": [area]})
        pr.assignee = _user("old_assignee")
        pr.assignees = [_user("old_assignee")]

        sut.process_pr(gh, args, mf, 99)

        pr.remove_from_assignees.assert_called_once_with("old_assignee")
        assert [call.args[0].login for call in pr.add_to_assignees.call_args_list] == ["m"]

    def test_self_removed_reviewer_is_not_re_added(self):
        area = _make_area("Kernel", maintainers=["m"], collaborators=["quitter"])
        gh, args, mf, pr = self._harness({"kernel/sched.c": [area]})
        quitter = gh.get_user("quitter")
        pr.get_issue_events.return_value = [
            SimpleNamespace(
                event="review_request_removed", actor=quitter, requested_reviewer=quitter
            )
        ]

        sut.process_pr(gh, args, mf, 99)

        assert "quitter" not in _reviewers_requested(pr)
        assert "m" in _reviewers_requested(pr)

    def test_reviewers_are_requested_as_for_a_new_pr(self):
        area = _make_area("Kernel", maintainers=["m"], collaborators=["c1"])
        gh, args, mf, pr = self._harness({"kernel/sched.c": [area]})
        # An earlier run left a request for someone the areas no longer name.
        pr.get_review_requests.return_value = ([gh.get_user("stale_reviewer")], [])

        sut.process_pr(gh, args, mf, 99)

        assert _requests_deleted(pr) == ["stale_reviewer"]
        assert sorted(_reviewers_requested(pr)) == ["c1", "m"]

    def test_still_valid_reviewer_is_not_withdrawn_and_re_requested(self):
        """Churning the request would notify a maintainer who never left."""
        area = _make_area("Kernel", maintainers=["m"], collaborators=["c1"])
        gh, args, mf, pr = self._harness({"kernel/sched.c": [area]})
        pr.get_review_requests.return_value = ([gh.get_user("m")], [])

        sut.process_pr(gh, args, mf, 99)

        assert _requests_deleted(pr) == []
        # Already requested, so _add_reviewers leaves them be and asks the rest.
        assert _reviewers_requested(pr) == ["c1"]

    def test_skipped_pr_is_not_stripped(self):
        """A PR the run declines to staff must not be left bare (MAX_FILES)."""
        area = _make_area("Kernel", maintainers=["m"], labels=["area: Kernel"])
        areas_per_file = {f"kernel/f{i}.c": [area] for i in range(sut.MAX_FILES + 1)}
        gh, args, mf, pr = self._harness(areas_per_file)
        pr.labels = [_label("area: Kernel")]
        pr.assignees = [_user("old_assignee")]
        pr.get_review_requests.return_value = ([gh.get_user("stale_reviewer")], [])

        sut.process_pr(gh, args, mf, 99)

        pr.delete_review_request.assert_not_called()
        pr.remove_from_assignees.assert_not_called()
        pr.remove_from_labels.assert_not_called()

    def test_without_reset_nothing_is_removed(self):
        area = _make_area("Kernel", maintainers=["m"], labels=["area: Kernel"])
        gh, args, mf, pr = _make_process_pr_harness({"kernel/sched.c": [area]})
        pr.labels = [_label("area: Kernel")]
        pr.assignees = [_user("old_assignee")]
        pr.get_review_requests.return_value = ([gh.get_user("stale_reviewer")], [])

        sut.process_pr(gh, args, mf, 99)

        pr.delete_review_request.assert_not_called()
        pr.remove_from_assignees.assert_not_called()
        assert _labels_removed(pr) == []


# ---------------------------------------------------------------------------
# Label selection  (_rank_labels / _select_labels)
# ---------------------------------------------------------------------------


def _labels_added(pr):
    """Label names passed to add_to_labels() on *pr*."""
    return [name for call in pr.add_to_labels.call_args_list for name in call.args]


class TestRankLabels:
    def test_orders_by_area_weight(self):
        heavy = _make_area("Heavy", labels=["area: heavy"])
        light = _make_area("Light", labels=["area: light"])
        ranked = sut._rank_labels({"area: heavy", "area: light"}, {heavy: 9, light: 1}, {})
        assert ranked == ["area: heavy", "area: light"]

    def test_weights_of_areas_sharing_a_label_add_up(self):
        a = _make_area("A", labels=["shared"])
        b = _make_area("B", labels=["shared"])
        solo = _make_area("Solo", labels=["solo"])
        ranked = sut._rank_labels({"shared", "solo"}, {solo: 3, a: 2, b: 2}, {})
        assert ranked == ["shared", "solo"]

    def test_labels_from_no_contributing_area_sort_last(self):
        area = _make_area("A", labels=["area: a"])
        ranked = sut._rank_labels({"area: a", "deferred"}, {area: 1}, {})
        assert ranked == ["area: a", "deferred"]

    def test_ties_break_by_name(self):
        a = _make_area("A", labels=["area: b"])
        b = _make_area("B", labels=["area: a"])
        assert sut._rank_labels({"area: a", "area: b"}, {a: 2, b: 2}, {}) == ["area: a", "area: b"]

    def test_unweighted_area_falls_back_to_its_file_count(self):
        """A meta-area weighs 0 for assignment but still describes the PR."""
        meta = _make_area("Documentation", labels=["area: Documentation"], meta=True)
        other = _make_area("Kernel", labels=["area: Kernel"])
        area_files = {"Documentation": {f"doc/{i}.rst" for i in range(9)}, "Kernel": {"k.c"}}

        ranked = sut._rank_labels(
            {"area: Documentation", "area: Kernel"}, {meta: 0, other: 1}, area_files
        )
        assert ranked == ["area: Documentation", "area: Kernel"]


class TestSelectLabels:
    @staticmethod
    def _areas(count, weight_of=lambda i: i):
        """count areas, each with one label, area i weighing weight_of(i)."""
        return {_make_area(f"A{i}", labels=[f"area: {i:02d}"]): weight_of(i) for i in range(count)}

    def test_under_the_limit_keeps_everything(self):
        area_counter = self._areas(sut.MAX_LABELS)
        labels = {next(iter(a.labels)) for a in area_counter}
        assert sut._select_labels(_make_pr(), labels, area_counter, {}) == labels

    def test_over_the_limit_keeps_the_heaviest(self):
        area_counter = self._areas(sut.MAX_LABELS + 1)
        labels = {next(iter(a.labels)) for a in area_counter}
        kept = sut._select_labels(_make_pr(), labels, area_counter, {})
        # Weights are 0..MAX_LABELS, so the highest-numbered labels win.
        assert kept == {f"area: {i:02d}" for i in range(1, sut.MAX_LABELS + 1)}

    def test_size_label_survives_truncation(self):
        area_counter = self._areas(sut.MAX_LABELS + 1)
        labels = {next(iter(a.labels)) for a in area_counter} | {"size: L"}
        kept = sut._select_labels(_make_pr(), labels, area_counter, {})
        assert "size: L" in kept
        # The size label is exempt rather than taking one of the MAX_LABELS slots.
        assert len(kept) == sut.MAX_LABELS + 1

    def test_size_label_does_not_count_towards_the_limit(self, caplog):
        """MAX_LABELS area labels plus a size label is not an overflow."""
        area_counter = self._areas(sut.MAX_LABELS)
        labels = {next(iter(a.labels)) for a in area_counter} | {"size: L"}

        with caplog.at_level(logging.WARNING, logger=sut.logger.name):
            kept = sut._select_labels(_make_pr(), labels, area_counter, {})

        assert kept == labels
        assert caplog.records == []


class TestLabelApplication:
    def test_labels_are_applied(self):
        area = _make_area("Kernel", maintainers=["m"], labels=["area: Kernel"])
        gh, args, mf, pr = _make_process_pr_harness({"kernel/sched.c": [area]})
        sut.process_pr(gh, args, mf, 99)

        assert "area: Kernel" in _labels_added(pr)

    def test_broad_pr_keeps_top_labels_instead_of_none(self):
        """Exceeding MAX_LABELS used to drop every label; now the heaviest stay."""
        areas_per_file = {}
        for i in range(sut.MAX_LABELS + 1):
            area = _make_area(f"A{i}", maintainers=[f"m{i}"], labels=[f"area: {i:02d}"])
            # Area i matches i + 1 files, so later areas weigh more.
            for f in range(i + 1):
                areas_per_file.setdefault(f"subsys/a{i}/f{f}.c", []).append(area)

        gh, args, mf, pr = _make_process_pr_harness(areas_per_file)
        sut.process_pr(gh, args, mf, 99)

        added = [label for label in _labels_added(pr) if label.startswith("area: ")]
        # The lightest area (a single file) is the one dropped.
        assert added == [f"area: {i:02d}" for i in range(1, sut.MAX_LABELS + 1)]

    def test_truncation_does_not_remove_labels_already_on_the_pr(self):
        """The cap governs what a run applies, not what the PR already carries."""
        areas_per_file = {}
        for i in range(sut.MAX_LABELS + 1):
            area = _make_area(f"A{i}", maintainers=[f"m{i}"], labels=[f"area: {i:02d}"])
            for f in range(i + 1):
                areas_per_file.setdefault(f"subsys/a{i}/f{f}.c", []).append(area)

        gh, args, mf, pr = _make_process_pr_harness(areas_per_file)
        # The label the truncation drops is already on the PR.
        pr.labels = [SimpleNamespace(name="area: 00")]

        sut.process_pr(gh, args, mf, 99)

        assert "area: 00" not in _labels_added(pr)
        pr.remove_from_labels.assert_not_called()


# ---------------------------------------------------------------------------
# Mention fallback
# ---------------------------------------------------------------------------


def _posted_comments(pr):
    """Bodies of every comment created on *pr*."""
    return [call.args[0] for call in pr.create_issue_comment.call_args_list]


def _mentioned(pr):
    """Logins @mentioned in the single comment posted on *pr*."""
    bodies = _posted_comments(pr)
    assert len(bodies) == 1, f"expected exactly one comment, got {len(bodies)}"
    return [word[1:] for word in bodies[0].split() if word.startswith("@")]


def _existing_comment(body):
    comment = MagicMock()
    comment.body = body
    return comment


class TestMentionFallback:
    """People who cannot receive a formal review request are asked by name in
    a comment instead of being dropped."""

    def test_non_collaborator_is_mentioned_not_requested(self):
        gh, gh_repo, pr, args = _make_add_reviewers_stubs()
        gh_repo.has_in_collaborators.side_effect = lambda user: user.login != "outsider"

        sut._add_reviewers(gh, gh_repo, pr, args, ["insider", "outsider"])

        assert _reviewers_requested(pr) == ["insider"]
        assert _mentioned(pr) == ["outsider"]

    def test_candidates_dropped_by_retry_are_mentioned(self):
        gh, gh_repo, pr, args = _make_add_reviewers_stubs()
        candidates = [f"u{i}" for i in range(25)]
        pr.create_review_request.side_effect = [sut.GithubException("too many"), None]

        sut._add_reviewers(gh, gh_repo, pr, args, candidates)

        assert _mentioned(pr) == candidates[sut.REVIEWER_RETRY_BATCH :]

    def test_everyone_is_mentioned_when_the_request_fails_outright(self):
        gh, gh_repo, pr, args = _make_add_reviewers_stubs()
        pr.create_review_request.side_effect = sut.GithubException("nope")

        sut._add_reviewers(gh, gh_repo, pr, args, ["a", "b"])

        assert _mentioned(pr) == ["a", "b"]

    def test_self_removed_user_is_never_mentioned(self):
        """Opting out must not be routed around by a mention."""
        gh, gh_repo, pr, args = _make_add_reviewers_stubs()
        quitter = gh.get_user("quitter")
        pr.get_issue_events.return_value = [
            SimpleNamespace(
                event="review_request_removed", actor=quitter, requested_reviewer=quitter
            )
        ]
        # Also a non-collaborator, so only the opt-out can keep them out.
        gh_repo.has_in_collaborators.side_effect = lambda user: user.login != "quitter"

        sut._add_reviewers(gh, gh_repo, pr, args, ["quitter", "keeper"])

        assert _reviewers_requested(pr) == ["keeper"]
        pr.create_issue_comment.assert_not_called()

    def test_author_and_existing_reviewers_are_not_mentioned(self):
        gh, gh_repo, pr, args = _make_add_reviewers_stubs(author="me")
        pr.get_review_requests.return_value = ([gh.get_user("already")], [])
        gh_repo.has_in_collaborators.return_value = False

        sut._add_reviewers(gh, gh_repo, pr, args, ["me", "already", "outsider"])

        assert _mentioned(pr) == ["outsider"]

    def test_nobody_to_mention_posts_no_comment(self):
        gh, gh_repo, pr, args = _make_add_reviewers_stubs()

        sut._add_reviewers(gh, gh_repo, pr, args, ["a", "b"])

        pr.create_issue_comment.assert_not_called()

    def test_comment_carries_the_marker(self):
        gh, gh_repo, pr, args = _make_add_reviewers_stubs()
        gh_repo.has_in_collaborators.return_value = False

        sut._add_reviewers(gh, gh_repo, pr, args, ["outsider"])

        assert sut.MENTION_MARKER in _posted_comments(pr)[0]

    def test_dry_run_posts_no_comment(self):
        gh, gh_repo, pr, args = _make_add_reviewers_stubs()
        args.dry_run = True
        gh_repo.has_in_collaborators.return_value = False

        sut._add_reviewers(gh, gh_repo, pr, args, ["outsider"])

        pr.create_issue_comment.assert_not_called()

    def test_comment_creation_failure_does_not_raise(self):
        gh, gh_repo, pr, args = _make_add_reviewers_stubs()
        gh_repo.has_in_collaborators.return_value = False
        pr.create_issue_comment.side_effect = sut.GithubException("no write access")

        sut._add_reviewers(gh, gh_repo, pr, args, ["outsider"])


class TestMentionCommentIsIdempotent:
    """Re-running over the same PR must update the existing comment rather
    than posting another one."""

    def _run(self, existing_body, candidates=("outsider",)):
        gh, gh_repo, pr, args = _make_add_reviewers_stubs()
        gh_repo.has_in_collaborators.return_value = False
        existing = _existing_comment(existing_body)
        pr.get_issue_comments.return_value = [existing]
        sut._add_reviewers(gh, gh_repo, pr, args, list(candidates))
        return pr, existing

    def test_existing_comment_is_edited_when_the_set_changes(self):
        stale = f"{sut.MENTION_MARKER}\n@someone_else\n\nold text"
        pr, existing = self._run(stale)

        pr.create_issue_comment.assert_not_called()
        existing.edit.assert_called_once()
        assert "@outsider" in existing.edit.call_args.args[0]

    def test_unchanged_comment_is_left_alone(self):
        """A second identical run must not edit, to avoid timeline noise."""
        gh, gh_repo, pr, args = _make_add_reviewers_stubs()
        gh_repo.has_in_collaborators.return_value = False
        sut._add_reviewers(gh, gh_repo, pr, args, ["outsider"])
        first_body = _posted_comments(pr)[0]

        pr2, existing = self._run(first_body)

        pr2.create_issue_comment.assert_not_called()
        existing.edit.assert_not_called()

    def test_unrelated_comments_are_ignored(self):
        pr, _ = self._run("just a normal review comment, no marker")

        assert len(_posted_comments(pr)) == 1

    def test_maintainer_outside_the_org_reaches_the_comment(self):
        """End-to-end: an area maintainer without push access is still asked."""
        area = _make_area("Net", maintainers=["outside_m"], collaborators=["inside_c"])
        gh, args, mf, pr = _make_process_pr_harness({"subsys/net/foo.c": [area]})
        gh.requester.graphql_query.return_value = ({}, _suggested_payload([]))
        gh.get_repo.return_value.get_commits.side_effect = lambda path, since=None: []
        gh.get_repo.return_value.has_in_collaborators.side_effect = (
            lambda user: user.login != "outside_m"
        )

        sut.process_pr(gh, args, mf, 99)

        assert _reviewers_requested(pr) == ["inside_c"]
        assert _mentioned(pr) == ["outside_m"]
