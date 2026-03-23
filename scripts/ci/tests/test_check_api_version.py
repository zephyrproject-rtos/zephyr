#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for scripts/ci/check_api_version.py

Tests use real Doxygen XML files generated on disk so the full
``doxmlparser`` parse path is exercised end-to-end.
"""

import subprocess
import sys
import tempfile
import textwrap
import unittest
from pathlib import Path

# Allow import from parent directory
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import check_api_version as cav  # noqa: E402

# Helpers

_DOXYFILE_TEMPLATE = textwrap.dedent(
    """\
    PROJECT_NAME   = "Test"
    INPUT          = {src_dir}
    GENERATE_HTML  = NO
    GENERATE_LATEX = NO
    GENERATE_XML   = YES
    XML_OUTPUT     = {xml_dir}
    QUIET          = YES
    WARNINGS       = NO
    WARN_IF_UNDOCUMENTED = NO
    EXTRACT_ALL    = YES
    """
)


def _build_xml(header_text: str) -> tempfile.TemporaryDirectory:
    """Write *header_text* to a temp header, run Doxygen, return the tmpdir.

    The returned :class:`~tempfile.TemporaryDirectory` must be kept alive by
    the caller for as long as the XML directory is needed.
    The Doxygen XML output lives at ``<tmpdir>/xml/``.
    """
    tmp = tempfile.TemporaryDirectory()
    root = Path(tmp.name)
    src_dir = root / "src"
    xml_dir = root / "xml"
    src_dir.mkdir()
    xml_dir.mkdir()

    (src_dir / "api.h").write_text(header_text)
    doxyfile = root / "Doxyfile"
    doxyfile.write_text(_DOXYFILE_TEMPLATE.format(src_dir=src_dir, xml_dir=xml_dir))

    result = subprocess.run(
        ["doxygen", str(doxyfile)],
        capture_output=True,
        cwd=root,
    )
    if result.returncode != 0:
        tmp.cleanup()
        raise RuntimeError(f"Doxygen failed:\n{result.stderr.decode()}")
    return tmp


# Sample headers

_HDR_V010 = textwrap.dedent(
    """\
    /**
     * @defgroup foo_api Foo API
     * @version 0.1.0
     * @{
     */
    void foo_init(void);
    /** @} */
    """
)

_HDR_V011 = textwrap.dedent(
    """\
    /**
     * @defgroup foo_api Foo API
     * @version 0.1.1
     * @{
     */
    void foo_init(void);
    void foo_reset(void);
    /** @} */
    """
)

_HDR_V020 = textwrap.dedent(
    """\
    /**
     * @defgroup foo_api Foo API
     * @version 0.2.0
     * @{
     */
    void foo_init(void);
    void foo_new(void);
    /** @} */
    """
)

_HDR_V023_BAD_MINOR = textwrap.dedent(
    """\
    /**
     * @defgroup foo_api Foo API
     * @version 0.2.3
     * @{
     */
    void foo_init(void);
    /** @} */
    """
)

_HDR_V100 = textwrap.dedent(
    """\
    /**
     * @defgroup foo_api Foo API
     * @version 1.0.0
     * @{
     */
    void foo_v2_init(void);
    /** @} */
    """
)

_HDR_V103_BAD_MAJOR = textwrap.dedent(
    """\
    /**
     * @defgroup foo_api Foo API
     * @version 1.0.3
     * @{
     */
    void foo_v2_init(void);
    /** @} */
    """
)

_HDR_V013_SKIP = textwrap.dedent(
    """\
    /**
     * @defgroup foo_api Foo API
     * @version 0.1.3
     * @{
     */
    void foo_init(void);
    /** @} */
    """
)

_HDR_NO_VERSION = textwrap.dedent(
    """\
    /**
     * @defgroup bar_api Bar internal
     * @{
     */
    void bar_helper(void);
    /** @} */
    """
)

_HDR_ZERO_VERSION = textwrap.dedent(
    """\
    /**
     * @defgroup new_api New API
     * @version 0.0.0
     * @{
     */
    void new_thing(void);
    /** @} */
    """
)

_HDR_MULTI_OLD = textwrap.dedent(
    """\
    /**
     * @defgroup alpha_api Alpha API
     * @version 1.0.0
     * @{
     */
    void alpha_init(void);
    /** @} */

    /**
     * @defgroup beta_api Beta API
     * @version 0.3.0
     * @{
     */
    void beta_init(void);
    /** @} */
    """
)

_HDR_MULTI_BOTH_BUMPED = textwrap.dedent(
    """\
    /**
     * @defgroup alpha_api Alpha API
     * @version 1.1.0
     * @{
     */
    void alpha_init(void);
    void alpha_extra(void);
    /** @} */

    /**
     * @defgroup beta_api Beta API
     * @version 0.4.0
     * @{
     */
    void beta_init(void);
    void beta_extra(void);
    /** @} */
    """
)

_HDR_MULTI_ONE_STALE = textwrap.dedent(
    """\
    /**
     * @defgroup alpha_api Alpha API
     * @version 1.1.0
     * @{
     */
    void alpha_init(void);
    void alpha_extra(void);
    /** @} */

    /**
     * @defgroup beta_api Beta API
     * @version 0.3.0
     * @{
     */
    void beta_init(void);
    /** @} */
    """
)

# Tests: parse_api_versions


class TestParseApiVersions(unittest.TestCase):
    def test_single_group_with_version(self):
        with _build_xml(_HDR_V010) as d:
            result = cav.parse_api_versions(Path(d) / "xml")
        self.assertEqual(result, {"foo_api": "0.1.0"})

    def test_group_without_version_omitted(self):
        with _build_xml(_HDR_NO_VERSION) as d:
            result = cav.parse_api_versions(Path(d) / "xml")
        self.assertEqual(result, {})

    def test_multiple_groups(self):
        with _build_xml(_HDR_MULTI_OLD) as d:
            result = cav.parse_api_versions(Path(d) / "xml")
        self.assertEqual(result, {"alpha_api": "1.0.0", "beta_api": "0.3.0"})

    def test_missing_index_exits(self):
        with tempfile.TemporaryDirectory() as d, self.assertRaises(SystemExit):
            cav.parse_api_versions(Path(d))


# Tests: _parse_semver


class TestParseSemver(unittest.TestCase):
    def test_valid(self):
        self.assertEqual(cav._parse_semver("1.2.3"), (1, 2, 3))
        self.assertEqual(cav._parse_semver("0.0.0"), (0, 0, 0))
        self.assertEqual(cav._parse_semver("10.20.30"), (10, 20, 30))

    def test_invalid_returns_none(self):
        self.assertIsNone(cav._parse_semver("1.2"))
        self.assertIsNone(cav._parse_semver("abc"))
        self.assertIsNone(cav._parse_semver("1.2.3.4"))


# Tests: check_api_versions (dict-level, no XML needed)
class TestCheckApiVersions(unittest.TestCase):
    def _errors(self, problems):
        return [p for p in problems if p.level == cav.Problem.ERROR]

    def test_empty_both(self):
        self.assertEqual(cav.check_api_versions({}, {}), [])

    def test_new_api_proper_version_ok(self):
        probs = cav.check_api_versions({}, {"new_api": "0.1.0"})
        self.assertEqual(self._errors(probs), [])

    def test_new_api_zero_version_error(self):
        probs = cav.check_api_versions({}, {"new_api": "0.0.0"})
        errs = self._errors(probs)
        self.assertEqual(len(errs), 1)
        self.assertIn("0.0.0", str(errs[0]))

    def test_unchanged_version_is_warning(self):
        probs = cav.check_api_versions({"foo_api": "0.1.0"}, {"foo_api": "0.1.0"})
        warns = [p for p in probs if p.level == cav.Problem.WARNING]
        self.assertEqual(len(warns), 1)
        self.assertIn("not incremented", str(warns[0]))
        self.assertEqual(self._errors(probs), [])

    def test_version_regression_is_error(self):
        probs = cav.check_api_versions({"foo_api": "0.2.0"}, {"foo_api": "0.1.0"})
        errs = self._errors(probs)
        self.assertEqual(len(errs), 1)
        self.assertIn("regressed", str(errs[0]))

    def test_valid_patch_bump(self):
        self.assertEqual(
            self._errors(cav.check_api_versions({"foo_api": "0.1.0"}, {"foo_api": "0.1.1"})),
            [],
        )

    def test_valid_minor_bump(self):
        self.assertEqual(
            self._errors(cav.check_api_versions({"foo_api": "0.1.5"}, {"foo_api": "0.2.0"})),
            [],
        )

    def test_valid_major_bump(self):
        self.assertEqual(
            self._errors(cav.check_api_versions({"foo_api": "0.9.0"}, {"foo_api": "1.0.0"})),
            [],
        )

    def test_minor_bump_patch_not_reset(self):
        errs = self._errors(cav.check_api_versions({"foo_api": "0.1.0"}, {"foo_api": "0.2.3"}))
        self.assertTrue(any("patch must be reset" in str(p) for p in errs))

    def test_major_bump_minor_patch_not_reset(self):
        errs = self._errors(cav.check_api_versions({"foo_api": "0.1.0"}, {"foo_api": "1.0.3"}))
        self.assertTrue(any("minor and patch must be reset" in str(p) for p in errs))

    def test_patch_skip_by_two(self):
        errs = self._errors(cav.check_api_versions({"foo_api": "0.1.0"}, {"foo_api": "0.1.3"}))
        self.assertTrue(any("at most 1" in str(p) for p in errs))

    def test_minor_skip_by_two(self):
        errs = self._errors(cav.check_api_versions({"foo_api": "0.1.0"}, {"foo_api": "0.3.0"}))
        self.assertTrue(any("at most 1" in str(p) for p in errs))

    def test_major_skip_by_two(self):
        errs = self._errors(cav.check_api_versions({"foo_api": "1.0.0"}, {"foo_api": "3.0.0"}))
        self.assertTrue(any("at most 1" in str(p) for p in errs))

    def test_removed_group_not_reported(self):
        probs = cav.check_api_versions({"old_api": "1.0.0"}, {})
        self.assertEqual(probs, [])

    def test_invalid_head_version_error(self):
        errs = self._errors(cav.check_api_versions({"foo_api": "0.1.0"}, {"foo_api": "bad"}))
        self.assertTrue(any("not a valid semantic version" in str(p) for p in errs))

    def test_invalid_base_version_warning(self):
        warns = [
            p
            for p in cav.check_api_versions({"foo_api": "bad"}, {"foo_api": "0.2.0"})
            if p.level == cav.Problem.WARNING
        ]
        self.assertTrue(any("not a valid semantic version" in str(p) for p in warns))

    def test_multiple_groups_all_ok(self):
        base = {"alpha_api": "1.0.0", "beta_api": "0.3.0"}
        head = {"alpha_api": "1.1.0", "beta_api": "0.4.0"}
        self.assertEqual(self._errors(cav.check_api_versions(base, head)), [])

    def test_multiple_groups_one_stale(self):
        base = {"alpha_api": "1.0.0", "beta_api": "0.3.0"}
        head = {"alpha_api": "1.1.0", "beta_api": "0.3.0"}
        probs = cav.check_api_versions(base, head)
        warns = [p for p in probs if p.level == cav.Problem.WARNING]
        self.assertEqual(len(warns), 1)
        self.assertIn("beta_api", str(warns[0]))
        self.assertIn("not incremented", str(warns[0]))
        self.assertEqual(self._errors(probs), [])


# End-to-end tests: real Doxygen XML → parse → check


class TestEndToEnd(unittest.TestCase):
    """Parse real Doxygen XML on disk then run check_api_versions."""

    def _errs(self, base_hdr, head_hdr):
        base_tmp = _build_xml(base_hdr)
        head_tmp = _build_xml(head_hdr)
        base_v = cav.parse_api_versions(Path(base_tmp.name) / "xml")
        head_v = cav.parse_api_versions(Path(head_tmp.name) / "xml")
        return [p for p in cav.check_api_versions(base_v, head_v) if p.level == cav.Problem.ERROR]

    def test_valid_patch_bump_no_errors(self):
        self.assertEqual(self._errs(_HDR_V010, _HDR_V011), [])

    def test_valid_minor_bump_no_errors(self):
        self.assertEqual(self._errs(_HDR_V010, _HDR_V020), [])

    def test_valid_major_bump_no_errors(self):
        self.assertEqual(self._errs(_HDR_V010, _HDR_V100), [])

    def test_unchanged_version_warning(self):
        base_tmp = _build_xml(_HDR_V010)
        head_tmp = _build_xml(_HDR_V010)
        base_v = cav.parse_api_versions(Path(base_tmp.name) / "xml")
        head_v = cav.parse_api_versions(Path(head_tmp.name) / "xml")
        probs = cav.check_api_versions(base_v, head_v)
        warns = [p for p in probs if p.level == cav.Problem.WARNING]
        self.assertEqual(len(warns), 1)
        self.assertEqual([p for p in probs if p.level == cav.Problem.ERROR], [])

    def test_minor_bump_patch_not_reset(self):
        errs = self._errs(_HDR_V010, _HDR_V023_BAD_MINOR)
        self.assertTrue(any("patch must be reset" in str(p) for p in errs))

    def test_major_bump_non_zero_patch(self):
        errs = self._errs(_HDR_V010, _HDR_V103_BAD_MAJOR)
        self.assertTrue(any("minor and patch must be reset" in str(p) for p in errs))

    def test_skip_by_two_error(self):
        errs = self._errs(_HDR_V010, _HDR_V013_SKIP)
        self.assertTrue(any("at most 1" in str(p) for p in errs))

    def test_new_api_no_version_not_reported(self):
        # Group exists in head only, has no @version → omitted from dict → no error
        base_tmp = _build_xml(_HDR_NO_VERSION)
        head_tmp = _build_xml(_HDR_NO_VERSION)
        base_v = cav.parse_api_versions(Path(base_tmp.name) / "xml")
        head_v = cav.parse_api_versions(Path(head_tmp.name) / "xml")
        self.assertEqual(cav.check_api_versions(base_v, head_v), [])

    def test_new_api_zero_version_error(self):
        base_tmp = _build_xml(_HDR_NO_VERSION)
        head_tmp = _build_xml(_HDR_ZERO_VERSION)
        base_v = cav.parse_api_versions(Path(base_tmp.name) / "xml")
        head_v = cav.parse_api_versions(Path(head_tmp.name) / "xml")
        errs = [p for p in cav.check_api_versions(base_v, head_v) if p.level == cav.Problem.ERROR]
        self.assertEqual(len(errs), 1)
        self.assertIn("0.0.0", str(errs[0]))

    def test_multi_group_both_bumped_ok(self):
        self.assertEqual(self._errs(_HDR_MULTI_OLD, _HDR_MULTI_BOTH_BUMPED), [])

    def test_multi_group_one_stale(self):
        base_tmp = _build_xml(_HDR_MULTI_OLD)
        head_tmp = _build_xml(_HDR_MULTI_ONE_STALE)
        base_v = cav.parse_api_versions(Path(base_tmp.name) / "xml")
        head_v = cav.parse_api_versions(Path(head_tmp.name) / "xml")
        probs = cav.check_api_versions(base_v, head_v)
        warns = [p for p in probs if p.level == cav.Problem.WARNING]
        self.assertEqual(len(warns), 1)
        self.assertIn("beta_api", str(warns[0]))
        self.assertIn("not incremented", str(warns[0]))
        self.assertEqual([p for p in probs if p.level == cav.Problem.ERROR], [])


if __name__ == "__main__":
    unittest.main()
