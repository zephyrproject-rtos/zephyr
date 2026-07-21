#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Tests for scripts/ci/set_assignees.py.

Heavy third-party dependencies (PyGithub, west, get_maintainer) are stubbed
out at module level so the test suite runs without a full Zephyr environment
or a live GitHub token.
"""

import logging
import sys
from types import SimpleNamespace
from unittest.mock import MagicMock, patch

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

    def __init__(self, name, maintainers=None, labels=None, collaborators=None):
        self.name = name
        self.maintainers = list(maintainers or [])
        self.labels = list(labels or [])
        self.collaborators = list(collaborators or [])

    def is_deferred_for_path(self, path):
        return False

    def get_collaborators_for_path(self, path):
        return self.collaborators


class _DeferredArea(_Area):
    """Area stub where is_deferred_for_path always returns True."""

    def is_deferred_for_path(self, path):
        return True


def _make_area(name, maintainers=None, labels=None, collaborators=None):
    return _Area(name, maintainers=maintainers, labels=labels, collaborators=collaborators)


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
        area = _make_area("Documentation", maintainers=["eve"])
        result = sut._pick_assignees(self._pr(), {area: 2}, {"eve": 2}, num_files=2)
        assert result == ["eve"]

    def test_meta_area_not_sole_area_skips_meta_logic(self):
        meta = _make_area("Documentation", maintainers=["eve"])
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
    pr.get_files.return_value = [SimpleNamespace(filename=fn) for fn in areas_per_file]
    pr.get_reviews.return_value = []
    pr.get_review_requests.return_value = ([], [])
    pr.get_issue_events.return_value = []

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
    gh_repo.get_pull.return_value = pr
    gh_repo.has_in_collaborators.return_value = True

    args = SimpleNamespace(
        org="testorg",
        repo="testrepo",
        dry_run=False,
        updated_manifest=None,
        updated_maintainer_file=None,
        size_labels=False,
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
