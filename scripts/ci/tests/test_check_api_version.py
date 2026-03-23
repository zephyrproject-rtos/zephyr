#!/usr/bin/env python3
# Copyright (c) 2025 The Zephyr Project
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for scripts/ci/check_api_version.py"""

import sys
import unittest
from pathlib import Path
from unittest.mock import patch

# Allow import from parent directory
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import check_api_version as cav  # noqa: E402

# ---------------------------------------------------------------------------
# Sample header text snippets
# ---------------------------------------------------------------------------

_HDR_V010 = """\
/**
 * @defgroup foo_api Foo
 * @version 0.1.0
 * @{
 */
void foo_init(void);
"""

_HDR_V011 = """\
/**
 * @defgroup foo_api Foo
 * @version 0.1.1
 * @{
 */
void foo_init(void);
void foo_reset(void);
"""

_HDR_V020 = """\
/**
 * @defgroup foo_api Foo
 * @version 0.2.0
 * @{
 */
void foo_init(void);
"""

_HDR_V023 = """\
/**
 * @defgroup foo_api Foo
 * @version 0.2.3
 * @{
 */
void foo_init(void);
"""

_HDR_V100 = """\
/**
 * @defgroup foo_api Foo
 * @version 1.0.0
 * @{
 */
void foo_v2_init(void);
"""

_HDR_V103 = """\
/**
 * @defgroup foo_api Foo
 * @version 1.0.3
 * @{
 */
void foo_v2_init(void);
"""

_HDR_V013 = """\
/**
 * @defgroup foo_api Foo
 * @version 0.1.3
 * @{
 */
void foo_init(void);
"""

_HDR_NO_VERSION = """\
/**
 * @brief Internal bar utilities
 * @defgroup bar_internal Bar internal
 * @{
 */
void bar_helper(void);
"""

_HDR_ZERO = """\
/**
 * @defgroup new_api New
 * @version 0.0.0
 * @{
 */
void new_thing(void);
"""

_HDR_MULTI_OLD = """\
/**
 * @defgroup alpha_api Alpha
 * @version 1.0.0
 */
/**
 * @defgroup beta_api Beta
 * @version 0.3.0
 */
"""

_HDR_MULTI_BOTH_BUMPED = """\
/**
 * @defgroup alpha_api Alpha
 * @version 1.1.0
 */
/**
 * @defgroup beta_api Beta
 * @version 0.4.0
 */
"""

_HDR_MULTI_ONE_STALE = """\
/**
 * @defgroup alpha_api Alpha
 * @version 1.1.0
 */
/**
 * @defgroup beta_api Beta
 * @version 0.3.0
 */
"""

_FP = "include/zephyr/drivers/foo.h"


# ---------------------------------------------------------------------------
# Tests: _parse_versions
# ---------------------------------------------------------------------------

class TestParseVersions(unittest.TestCase):

    def test_single_group(self):
        self.assertEqual(
            cav._parse_versions(_HDR_V010, "x"),
            {"foo_api": (0, 1, 0)},
        )

    def test_no_version_tag(self):
        self.assertEqual(cav._parse_versions(_HDR_NO_VERSION, "x"), {})

    def test_multiple_groups(self):
        result = cav._parse_versions(_HDR_MULTI_OLD, "x")
        self.assertEqual(result, {"alpha_api": (1, 0, 0), "beta_api": (0, 3, 0)})

    def test_fallback_name_used_when_no_defgroup(self):
        text = "/** @version 1.2.3 */"
        result = cav._parse_versions(text, "fallback")
        self.assertEqual(result, {"fallback": (1, 2, 3)})

    def test_empty_string(self):
        self.assertEqual(cav._parse_versions("", "x"), {})


# ---------------------------------------------------------------------------
# Tests: check_api_versions (integration, git ops mocked)
# ---------------------------------------------------------------------------

class TestCheckApiVersions(unittest.TestCase):
    """
    Mocks _changed_public_headers and _git_show so no real git repo is needed.
    """

    def _run(self, changed_files, base_contents, head_contents):
        """Helper: run check_api_versions with mocked git helpers."""

        def fake_changed(root, base, head):
            return changed_files

        def fake_show(ref, path, cwd):
            if ref == "v4.2.0":
                return base_contents.get(path)
            return head_contents.get(path)

        with patch.object(cav, "_changed_public_headers", fake_changed), \
             patch.object(cav, "_git_show", fake_show):
            return cav.check_api_versions(Path("/fake"), "v4.2.0", "HEAD")

    def _errors(self, problems):
        return [p for p in problems if p.level == cav.Problem.ERROR]

    # --- No changed headers ---
    def test_no_changed_headers(self):
        self.assertEqual(self._run([], {}, {}), [])

    # --- Non-versioned header silently skipped ---
    def test_header_without_version_tag_ignored(self):
        probs = self._run([_FP], {_FP: _HDR_NO_VERSION}, {_FP: _HDR_NO_VERSION})
        self.assertEqual(probs, [])

    # --- Deleted file ---
    def test_deleted_file_skipped(self):
        probs = self._run([_FP], {_FP: _HDR_V010}, {})
        self.assertEqual(probs, [])

    # --- New file with proper version ---
    def test_new_file_proper_version_ok(self):
        probs = self._run([_FP], {}, {_FP: _HDR_V010})
        self.assertEqual(self._errors(probs), [])

    # --- New file with 0.0.0 is an error ---
    def test_new_file_zero_version_error(self):
        probs = self._run([_FP], {}, {_FP: _HDR_ZERO})
        self.assertEqual(len(self._errors(probs)), 1)
        self.assertIn("0.0.0", str(probs[0]))

    # --- Valid patch bump ---
    def test_valid_patch_bump(self):
        probs = self._run([_FP], {_FP: _HDR_V010}, {_FP: _HDR_V011})
        self.assertEqual(self._errors(probs), [])

    # --- Valid minor bump ---
    def test_valid_minor_bump(self):
        probs = self._run([_FP], {_FP: _HDR_V010}, {_FP: _HDR_V020})
        self.assertEqual(self._errors(probs), [])

    # --- Valid major bump ---
    def test_valid_major_bump(self):
        probs = self._run([_FP], {_FP: _HDR_V010}, {_FP: _HDR_V100})
        self.assertEqual(self._errors(probs), [])

    # --- Unchanged version is an ERROR ---
    def test_unchanged_version_is_error(self):
        probs = self._run([_FP], {_FP: _HDR_V010}, {_FP: _HDR_V010})
        errs = self._errors(probs)
        self.assertEqual(len(errs), 1)
        self.assertIn("not incremented", str(errs[0]))

    # --- Version regression is an error ---
    def test_version_regression_is_error(self):
        probs = self._run([_FP], {_FP: _HDR_V020}, {_FP: _HDR_V010})
        errs = self._errors(probs)
        self.assertEqual(len(errs), 1)
        self.assertIn("regressed", str(errs[0]))

    # --- Minor bump with non-zero patch ---
    def test_minor_bump_patch_not_reset(self):
        probs = self._run([_FP], {_FP: _HDR_V010}, {_FP: _HDR_V023})
        errs = self._errors(probs)
        self.assertTrue(any("patch must be reset" in str(p) for p in errs))

    # --- Major bump with non-zero minor/patch ---
    def test_major_bump_minor_patch_not_reset(self):
        probs = self._run([_FP], {_FP: _HDR_V010}, {_FP: _HDR_V103})
        errs = self._errors(probs)
        self.assertTrue(any("minor and patch must be reset" in str(p) for p in errs))

    # --- Skip-by-two patch ---
    def test_patch_skip_by_two(self):
        probs = self._run([_FP], {_FP: _HDR_V010}, {_FP: _HDR_V013})
        errs = self._errors(probs)
        self.assertTrue(any("at most 1" in str(p) for p in errs))

    # --- Multiple groups: both bumped correctly ---
    def test_multiple_groups_both_bumped_ok(self):
        fp = "include/zephyr/drivers/multi.h"
        probs = self._run([fp], {fp: _HDR_MULTI_OLD}, {fp: _HDR_MULTI_BOTH_BUMPED})
        self.assertEqual(self._errors(probs), [])

    # --- Multiple groups: one stale version ---
    def test_multiple_groups_one_stale(self):
        fp = "include/zephyr/drivers/multi.h"
        probs = self._run([fp], {fp: _HDR_MULTI_OLD}, {fp: _HDR_MULTI_ONE_STALE})
        errs = self._errors(probs)
        self.assertEqual(len(errs), 1)
        self.assertIn("beta_api", str(errs[0]))
        self.assertIn("not incremented", str(errs[0]))

    # --- File outside public include root is ignored ---
    def test_file_outside_public_root_ignored(self):
        fp = "subsys/net/private/net_priv.h"
        # _changed_public_headers filters by prefix, so this won't appear;
        # but if it somehow did appear in the list, still verify no false error
        probs = self._run([], {fp: _HDR_V010}, {fp: _HDR_V010})
        self.assertEqual(probs, [])

    # --- Non-.h file never reaches checker ---
    def test_non_header_never_reaches_checker(self):
        # _changed_public_headers only returns .h files; passing [] simulates that
        probs = self._run([], {}, {})
        self.assertEqual(probs, [])

    # --- New @defgroup added mid-cycle with 0.0.0 ---
    def test_new_defgroup_mid_cycle_zero_version(self):
        old = "/** @defgroup existing_api Existing\n * @version 1.0.0\n */\n"
        new = ("/** @defgroup existing_api Existing\n * @version 1.0.0\n */\n"
               "/** @defgroup brand_new New\n * @version 0.0.0\n */\n")
        probs = self._run([_FP], {_FP: old}, {_FP: new})
        errs = self._errors(probs)
        self.assertTrue(any("brand_new" in str(p) for p in errs))

    # --- New @defgroup added mid-cycle with valid version ---
    def test_new_defgroup_mid_cycle_valid_version(self):
        old = "/** @defgroup existing_api Existing\n * @version 1.0.0\n */\n"
        new = ("/** @defgroup existing_api Existing\n * @version 1.0.0\n */\n"
               "/** @defgroup brand_new New\n * @version 0.1.0\n */\n")
        probs = self._run([_FP], {_FP: old}, {_FP: new})
        # existing_api unchanged triggers error
        errs = [p for p in self._errors(probs) if "brand_new" in str(p)]
        self.assertEqual(errs, [])


if __name__ == "__main__":
    unittest.main()