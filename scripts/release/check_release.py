#!/usr/bin/env python3
# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Verify that everything needed for a release milestone is in place.

The checks follow doc/project/release_process.rst and the per-release
checklist issue. Each milestone checks the VERSION file, the git tags and
branches, the release notes and migration guides, the list of supported
releases in the documentation and, unless --no-github is given, the GitHub
milestones, labels, releases and release blocker issues.

Milestones:

  rc            a release candidate, e.g. 4.6.0-rc1
  final         a release, e.g. 4.6.0, or a point release, e.g. 4.5.1
  post-release  main after a release: release branch, VERSION back to
                X.Y.99, drafts for the next release, milestones
  auto          infer the milestone from the VERSION file (default)

Use --pre-tag before tagging to check the release PR without requiring the
tag, and --ref to check a git ref instead of the working tree.

Usage:
    git fetch upstream --tags
    ./scripts/release/check_release.py --ref upstream/main
    ./scripts/release/check_release.py rc 4.6.0-rc2 --pre-tag
    ./scripts/release/check_release.py final 4.6.0
    GITHUB_TOKEN=<token> ./scripts/release/check_release.py post-release 4.6.0

The exit status is 1 if any check failed.
"""

import argparse
import datetime
import fnmatch
import json
import os
import re
import subprocess
import sys
import urllib.error
import urllib.parse
import urllib.request
from dataclasses import dataclass
from pathlib import Path

ZEPHYR_BASE = Path(__file__).resolve().parents[2]
DOCS_URL = "https://docs.zephyrproject.org"
RELEASES = "doc/releases"
NOTES_DRAFT = f"{RELEASES}/release-notes-draft.rst"
GUIDE_DRAFT = f"{RELEASES}/migration-guide-draft.rst"
BLOCKER_LABEL = "Release Blocker"


@dataclass(frozen=True)
class Version:
    major: int
    minor: int
    patch: int = 0
    rc: int | None = None

    @classmethod
    def parse(cls, text):
        m = re.fullmatch(r"v?(\d+)\.(\d+)(?:\.(\d+))?(?:-rc(\d+))?", text.strip())
        if m is None:
            raise ValueError(f"invalid version '{text}'")
        major, minor, patch, rc = m.groups()
        return cls(int(major), int(minor), int(patch or 0), int(rc) if rc else None)

    @property
    def series(self):
        return f"{self.major}.{self.minor}"

    @property
    def release(self):
        return Version(self.major, self.minor, self.patch)

    @property
    def tag(self):
        return f"v{self}"

    @property
    def branch(self):
        return f"v{self.series}-branch"

    def key(self):
        return (self.major, self.minor, self.patch, self.rc if self.rc is not None else 1 << 16)

    def __str__(self):
        rc = f"-rc{self.rc}" if self.rc is not None else ""
        return f"{self.major}.{self.minor}.{self.patch}{rc}"


class Report:
    COLORS = {"PASS": "32", "WARN": "33", "FAIL": "31", "SKIP": "36"}

    def __init__(self, verbose):
        self.verbose = verbose
        self.color = sys.stdout.isatty() and os.environ.get("NO_COLOR") is None
        self.counts = dict.fromkeys(self.COLORS, 0)

    def section(self, title):
        print(f"\n== {title}")

    def _emit(self, kind, msg):
        self.counts[kind] += 1
        if kind == "PASS" and not self.verbose:
            return
        label = f"\033[{self.COLORS[kind]}m{kind}\033[0m" if self.color else kind
        print(f"  [{label}] {msg}")

    def ok(self, msg):
        self._emit("PASS", msg)

    def warn(self, msg):
        self._emit("WARN", msg)

    def fail(self, msg):
        self._emit("FAIL", msg)

    def skip(self, msg):
        self._emit("SKIP", msg)

    def check(self, cond, msg, hint=None, warn_only=False):
        if cond:
            self.ok(msg)
        elif warn_only:
            self.warn(msg if hint is None else f"{msg}: {hint}")
        else:
            self.fail(msg if hint is None else f"{msg}: {hint}")
        return cond

    def summary(self):
        c = self.counts
        print(
            f"\n{c['PASS']} passed, {c['WARN']} warnings, {c['FAIL']} failed, {c['SKIP']} skipped"
        )
        return c["FAIL"] == 0


class Git:
    def __init__(self, base, remote, main_branch):
        self.base = base
        self.remote = remote
        self.main_branch = main_branch
        self._remote_refs = None

    def run(self, *args):
        res = subprocess.run(
            ["git", "-C", str(self.base), *args], capture_output=True, text=True, check=False
        )
        return res.stdout if res.returncode == 0 else None

    def ok(self, *args):
        return self.run(*args) is not None

    def remote_refs(self):
        """Tags and heads on the remote: name -> object sha (unpeeled)."""
        if self._remote_refs is None:
            self._remote_refs = {}
            out = self.run("ls-remote", "--tags", "--heads", self.remote) or ""
            for line in out.splitlines():
                sha, ref = line.split("\t")
                if not ref.endswith("^{}"):
                    self._remote_refs[ref] = sha
        return self._remote_refs

    def remote_tags(self):
        prefix = "refs/tags/"
        return {r[len(prefix) :]: s for r, s in self.remote_refs().items() if r.startswith(prefix)}

    def remote_head(self, branch):
        return self.remote_refs().get(f"refs/heads/{branch}")

    def has_commit(self, sha):
        return self.ok("cat-file", "-e", f"{sha}^{{commit}}")

    def is_ancestor(self, a, b):
        return self.ok("merge-base", "--is-ancestor", a, b)


class Tree:
    """Files of the working tree, or of a git ref when one is given."""

    def __init__(self, git, ref):
        self.git = git
        self.ref = ref

    def read(self, path):
        if self.ref is not None:
            return self.git.run("show", f"{self.ref}:{path}")
        try:
            return (self.git.base / path).read_text(encoding="utf-8")
        except OSError:
            return None

    def listdir(self, path):
        if self.ref is not None:
            out = self.git.run("ls-tree", "--name-only", f"{self.ref}:{path}") or ""
            return sorted(out.split())
        return sorted(p.name for p in (self.git.base / path).iterdir())


class GitHub:
    def __init__(self, repo, token):
        self.repo = repo
        self.token = token

    def get(self, path, params=None):
        """Return the decoded JSON, or None on 404."""
        url = f"https://api.github.com/{path.format(repo=self.repo)}"
        if params:
            url += "?" + urllib.parse.urlencode(params)
        req = urllib.request.Request(url, headers={"Accept": "application/vnd.github+json"})
        if self.token:
            req.add_header("Authorization", f"Bearer {self.token}")
        try:
            with urllib.request.urlopen(req, timeout=30) as resp:
                return json.load(resp)
        except urllib.error.HTTPError as e:
            if e.code == 404:
                return None
            raise

    def milestones(self):
        return self.get("repos/{repo}/milestones", {"state": "all", "per_page": 100}) or []


def parse_version_file(text):
    fields = dict(re.findall(r"^(\w+)[ \t]*=[ \t]*(.*?)[ \t]*$", text, re.MULTILINE))
    return fields


def norm(text):
    return " ".join(text.split())


def release_doc_series(name, kind):
    m = re.fullmatch(rf"{kind}-(\d+\.\d+)\.rst", name)
    return m.group(1) if m else None


def heading_present(text, title):
    return re.search(rf"^{re.escape(title)}\n#+$", text, re.MULTILINE) is not None


class Checker:
    def __init__(self, args):
        self.args = args
        self.report = Report(args.verbose)
        self.git = Git(args.zephyr_base, args.remote, args.main_branch)
        self.tree = Tree(self.git, args.ref)
        self.gh = None
        if not args.no_github:
            token = os.environ.get("GITHUB_TOKEN") or os.environ.get("GH_TOKEN")
            self.gh = GitHub(args.repo, token)

    # --- helpers -----------------------------------------------------------

    def released_versions(self):
        """Final X.Y.0 releases tagged on the remote (or locally as fallback)."""
        tags = self.git.remote_tags() or dict.fromkeys(
            (self.git.run("tag", "-l", "v*") or "").split()
        )
        out = []
        for t in tags:
            try:
                v = Version.parse(t)
            except ValueError:
                continue
            if v.rc is None and v.patch == 0:
                out.append(v)
        return sorted(out, key=Version.key)

    def previous_release(self, v):
        older = [r for r in self.released_versions() if (r.major, r.minor) < (v.major, v.minor)]
        return older[-1] if older else None

    def read_required(self, path):
        text = self.tree.read(path)
        if text is None:
            self.report.fail(f"{path} not found")
        return text

    # --- VERSION -----------------------------------------------------------

    def check_version_file(self, expected, dev=False):
        r = self.report
        where = self.args.ref or "working tree"
        r.section(f"VERSION file ({where})")
        text = self.read_required("VERSION")
        if text is None:
            return
        f = parse_version_file(text)
        patch = "99" if dev else str(expected.patch)
        extra = f"rc{expected.rc}" if expected.rc is not None else ""
        r.check(
            f.get("VERSION_MAJOR") == str(expected.major),
            f"VERSION_MAJOR = {expected.major}",
            f"found {f.get('VERSION_MAJOR')}",
        )
        r.check(
            f.get("VERSION_MINOR") == str(expected.minor),
            f"VERSION_MINOR = {expected.minor}",
            f"found {f.get('VERSION_MINOR')}",
        )
        r.check(
            f.get("PATCHLEVEL") == patch, f"PATCHLEVEL = {patch}", f"found {f.get('PATCHLEVEL')}"
        )
        r.check(
            f.get("VERSION_TWEAK") == "0", "VERSION_TWEAK = 0", f"found {f.get('VERSION_TWEAK')}"
        )
        hint = f"found '{f.get('EXTRAVERSION')}'"
        if extra == "":
            hint += " (must be empty, not 0)"
        r.check(f.get("EXTRAVERSION") == extra, f"EXTRAVERSION = '{extra}'", hint)

        subject = self.git.run("log", "-1", "--format=%s", self.args.ref or "HEAD", "--", "VERSION")
        subject = (subject or "").strip()
        if dev:
            want = "release:"
            good = subject.startswith(want)
        else:
            want = f"release: Zephyr {expected}"
            good = subject == want
        r.check(
            good, f"last VERSION commit subject is '{want}'", f"found '{subject}'", warn_only=True
        )

    # --- tags and branches -------------------------------------------------

    def check_tag(self, v, required=True):
        r = self.report
        tag = v.tag
        r.section(f"Tag {tag}")
        obj_type = (self.git.run("cat-file", "-t", f"refs/tags/{tag}") or "").strip()
        remote_sha = self.git.remote_tags().get(tag)
        if not obj_type:
            if remote_sha is not None:
                r.fail(
                    f"{tag} exists on {self.git.remote} but not locally: "
                    f"git fetch {self.git.remote} tag {tag}"
                )
            elif required:
                r.fail(f"{tag} does not exist (locally or on {self.git.remote})")
            else:
                r.skip(f"{tag} not created yet (--pre-tag)")
            return
        r.check(remote_sha is not None, f"{tag} pushed to {self.git.remote}")
        local_sha = (self.git.run("rev-parse", f"refs/tags/{tag}") or "").strip()
        if remote_sha is not None:
            r.check(
                local_sha == remote_sha,
                f"local {tag} matches {self.git.remote}",
                f"local {local_sha[:12]}, remote {remote_sha[:12]}",
            )
        if not r.check(
            obj_type == "tag",
            f"{tag} is an annotated tag",
            "lightweight tag, recreate with git tag -s",
        ):
            return
        body = self.git.run("cat-file", "-p", f"refs/tags/{tag}") or ""
        r.check(
            "-----BEGIN PGP SIGNATURE-----" in body or "-----BEGIN SSH SIGNATURE-----" in body,
            f"{tag} is signed",
        )
        msg = body.split("\n\n", 1)[1].splitlines()[0] if "\n\n" in body else ""
        r.check(
            msg == f"Zephyr {v}", f"tag message is 'Zephyr {v}'", f"found '{msg}'", warn_only=True
        )

        version = self.git.run("show", f"{tag}^{{commit}}:VERSION")
        if version is not None:
            f = parse_version_file(version)
            extra = f"rc{v.rc}" if v.rc is not None else ""
            got = (
                f.get("VERSION_MAJOR"),
                f.get("VERSION_MINOR"),
                f.get("PATCHLEVEL"),
                f.get("EXTRAVERSION"),
            )
            want = (str(v.major), str(v.minor), str(v.patch), extra)
            r.check(got == want, f"VERSION at {tag} is {v}", f"found {got}")

        base = self.git.main_branch if v.patch == 0 else v.branch
        head = self.git.remote_head(base)
        if head is None:
            r.fail(f"{self.git.remote}/{base} not found")
        elif not self.git.has_commit(head):
            r.warn(f"{self.git.remote}/{base} not fetched, cannot check that {tag} is on it")
        else:
            r.check(
                self.git.is_ancestor(f"{tag}^{{commit}}", head),
                f"{tag} is on {self.git.remote}/{base}",
            )

    def check_rc_sequence(self, v):
        r = self.report
        r.section(f"Release candidates of {v.release}")
        tags = self.git.remote_tags()
        end = (
            v.rc
            if v.rc is not None
            else 1
            + max(
                [
                    Version.parse(t).rc
                    for t in tags
                    if re.fullmatch(rf"v{re.escape(str(v.release))}-rc\d+", t)
                ],
                default=0,
            )
        )
        if v.rc is None and end == 1:
            r.warn(f"no release candidates tagged for {v.release}")
            return
        for n in range(1, end):
            r.check(f"v{v.release}-rc{n}" in tags, f"v{v.release}-rc{n} tagged")
        if v.rc is not None and v.release.tag in tags:
            r.warn(f"{v.release.tag} is already tagged")

    def check_release_branch(self, v):
        r = self.report
        branch = v.branch
        r.section(f"Release branch {branch}")
        head = self.git.remote_head(branch)
        if not r.check(
            head is not None, f"{branch} exists on {self.git.remote}", f"create it from {v.tag}"
        ):
            return
        if not self.git.ok("rev-parse", "-q", "--verify", f"refs/tags/{v.tag}"):
            r.skip(f"{v.tag} not available locally")
        elif not self.git.has_commit(head):
            r.warn(f"{self.git.remote}/{branch} not fetched, cannot check its base")
        else:
            r.check(self.git.is_ancestor(f"{v.tag}^{{commit}}", head), f"{branch} contains {v.tag}")
            main = self.git.remote_head(self.git.main_branch)
            if main is not None and self.git.has_commit(main):
                base = (self.git.run("merge-base", head, main) or "").strip()
                tag_commit = (self.git.run("rev-parse", f"{v.tag}^{{commit}}") or "").strip()
                r.check(
                    base == tag_commit,
                    f"{branch} branches off {self.git.main_branch} at {v.tag}",
                    f"merge base is {base[:12]}",
                    warn_only=True,
                )

    # --- documentation -----------------------------------------------------

    def parse_index(self):
        text = self.tree.read(f"{RELEASES}/index.rst")
        if text is None:
            return None
        info = {"text": text, "toctree": [], "rows": {}, "links": {}, "next": None}
        m = re.search(r"^\.\. toctree::\n((?:[ \t]+.*\n|\n)+)", text, re.MULTILINE)
        if m:
            for line in m.group(1).splitlines():
                line = line.strip()
                if line and not line.startswith(":"):
                    info["toctree"].append(line)
        rows = re.finditer(
            r"^\s*\* - `Zephyr (\d+\.\d+)\.(\d+)(?: \(([^)]*)\))?`_\n"
            r"\s*- (\S+)\n\s*- (\S+)\n\s*- (.+)\n",
            text,
            re.MULTILINE,
        )
        for row in rows:
            series, patch, tagline, date, eol, status = row.groups()
            info["rows"][series] = {
                "patch": patch,
                "tagline": tagline,
                "date": date,
                "eol": eol,
                "status": status.strip(),
            }
        for link in re.finditer(
            r"^\.\. _`Zephyr (\d+\.\d+)\.\d+[^`]*`: (\S+)$", text, re.MULTILINE
        ):
            info["links"][link.group(1)] = link.group(2)
        m = re.search(r"next planned release is \*\*Zephyr (\d+\.\d+)\*\*", text)
        if m:
            info["next"] = m.group(1)
        return info

    def in_toctree(self, info, stem):
        return any(fnmatch.fnmatchcase(stem, p) for p in info["toctree"])

    def conf_versions(self, conf):
        m = re.search(r'"versions":\s*\((.*?)\n\s*\),', conf, re.DOTALL)
        if m is None:
            return None
        return re.findall(r'\("([^"]*)",\s*"([^"]*)"\)', m.group(1))

    def conf_eol_patterns(self, conf):
        m = re.search(r"RELEASE_NOTES_GLOB_PATTERNS\s*=\s*\[(.*?)\n\]", conf, re.DOTALL)
        return re.findall(r'"([^"]+)"', m.group(1)) if m else []

    def check_supported_releases(self, latest):
        """Supported releases table, doc/conf.py version list and SECURITY.md."""
        r = self.report
        r.section("Supported releases")
        info = self.parse_index()
        conf = self.tree.read("doc/conf.py")
        security = self.tree.read(".github/SECURITY.md")
        if info is None or conf is None or security is None:
            r.fail("doc/releases/index.rst, doc/conf.py or .github/SECURITY.md not found")
            return
        rows = info["rows"]
        if not r.check(len(rows) > 0, "supported releases table parsed"):
            return
        stable = [s for s, row in rows.items() if row["status"] == "Latest stable"]
        r.check(
            stable == [latest.series],
            f"{latest.series} is the only 'Latest stable' release",
            f"found {stable}",
        )

        today = datetime.date.today().isoformat()
        conf_versions = self.conf_versions(conf)
        conf_series = {
            m.group(1) for name, _ in conf_versions or [] if (m := re.match(r"(\d+\.\d+)\.", name))
        }
        sec_series = set(re.findall(r"^\s*- v(\d+\.\d+):", security, re.MULTILINE))
        for series, row in rows.items():
            for kind in ("release-notes", "migration-guide"):
                stem = f"{kind}-{series}"
                r.check(self.tree.read(f"{RELEASES}/{stem}.rst") is not None, f"{stem}.rst exists")
                r.check(self.in_toctree(info, stem), f"{stem} matched by the index.rst toctree")
            url = f"{DOCS_URL}/{series}.{row['patch']}/"
            r.check(
                info["links"].get(series) == url,
                f"index.rst link target for {series}",
                f"expected {url}, found {info['links'].get(series)}",
            )
            if row["eol"] <= today:
                r.warn(f"{series} reached its EOL date {row['eol']}, move it to eol_releases.rst")
            r.check(series in conf_series, f"{series} in doc/conf.py html_context versions")
            r.check(series in sec_series, f"v{series} listed in .github/SECURITY.md")

        for series in sorted(conf_series - rows.keys()):
            r.fail(f"{series} in doc/conf.py html_context versions but not supported")
        for series in sorted(sec_series - rows.keys()):
            r.fail(f"v{series} in .github/SECURITY.md but not supported")
        if conf_versions is not None:
            r.check(
                conf_versions[:1] == [("latest", "/")], "'latest' is first in doc/conf.py versions"
            )
        m = re.search(r"latest release of v(\d+\.\d+)", security)
        r.check(
            m is not None and m.group(1) == latest.series,
            f".github/SECURITY.md names v{latest.series} as the latest release",
            f"found {m.group(1) if m else None}",
        )
        m = re.search(rf"^\s*- v{re.escape(latest.series)}: (.*)$", security, re.MULTILINE)
        r.check(
            m is not None and m.group(1).strip() == "Current release",
            f".github/SECURITY.md marks v{latest.series} as 'Current release'",
        )

        regular = [s for s, row in rows.items() if "LTS" not in (row["tagline"] or "")]
        r.check(
            len(regular) <= 2,
            "at most two non-LTS releases supported",
            f"found {regular}",
            warn_only=True,
        )

    def check_eol_releases(self):
        """Release docs not in the supported table are EOL and excluded from the build."""
        r = self.report
        r.section("End-of-life releases")
        info = self.parse_index()
        conf = self.tree.read("doc/conf.py")
        eol = self.tree.read(f"{RELEASES}/eol_releases.rst")
        if info is None or conf is None or eol is None:
            r.fail("doc/releases/index.rst, doc/conf.py or eol_releases.rst not found")
            return
        patterns = self.conf_eol_patterns(conf)
        supported = info["rows"].keys()
        stable = [
            Version.parse(s) for s, row in info["rows"].items() if row["status"] == "Latest stable"
        ]
        for name in self.tree.listdir(RELEASES):
            for kind in ("release-notes", "migration-guide"):
                series = release_doc_series(name, kind)
                if series is None:
                    continue
                excluded = any(fnmatch.fnmatchcase(f"releases/{name}", p) for p in patterns)
                listed = f"<doc/releases/{name}>" in eol
                if stable and Version.parse(series).key() > stable[0].key():
                    # Draft of a release in development (pre-4.6 layout)
                    continue
                if series in supported:
                    r.check(
                        not excluded and not listed,
                        f"{name} is not marked EOL",
                        "supported release excluded or listed in eol_releases.rst",
                    )
                else:
                    r.check(listed, f"{name} listed in eol_releases.rst")
                    r.check(
                        excluded,
                        f"{name} matched by RELEASE_NOTES_GLOB_PATTERNS",
                        "add it to doc/conf.py so it is no longer built",
                    )
                    r.check(
                        not self.in_toctree(info, name[:-4]), f"{name} not in the index.rst toctree"
                    )

    def check_draft(self, target, prev):
        """Working draft release notes and migration guide for the next release.

        The drafts are release-notes-draft.rst and migration-guide-draft.rst;
        trees before 4.6 used release-notes-X.Y.rst for the draft instead.
        Returns the release the drafts target.
        """
        r = self.report
        r.section("Working draft release notes and migration guide")
        stems = ("release-notes-draft", "migration-guide-draft")
        if self.tree.read(NOTES_DRAFT) is None and target is not None:
            legacy = (f"release-notes-{target.series}", f"migration-guide-{target.series}")
            if self.tree.read(f"{RELEASES}/{legacy[0]}.rst") is not None:
                r.warn(f"no {NOTES_DRAFT}, using the pre-4.6 layout ({legacy[0]}.rst)")
                stems = legacy
        notes_path, guide_path = (f"{RELEASES}/{stem}.rst" for stem in stems)
        notes = self.read_required(notes_path)
        guide = self.read_required(guide_path)
        if notes is None or guide is None:
            return None
        m = re.search(r"^\.\. _zephyr_(\d+\.\d+):$", notes, re.MULTILINE)
        if not r.check(m is not None, f"{notes_path} defines a zephyr_X.Y label"):
            return None
        found = Version.parse(m.group(1))
        if target is None:
            target = found
            r.ok(f"drafts target {target.series}")
        else:
            r.check(
                found == target, f"{notes_path} targets {target.series}", f"found {found.series}"
            )
        s = target.series
        r.check(
            heading_present(notes, f"Zephyr {s}.0 (Working Draft)"),
            f"release notes title is 'Zephyr {s}.0 (Working Draft)'",
        )
        r.check(
            re.search(rf"^\.\. _migration_{re.escape(s)}:$", guide, re.MULTILINE) is not None,
            f"{guide_path} defines migration_{s}",
        )
        r.check(
            heading_present(guide, f"Migration guide to Zephyr v{s}.0 (Working Draft)"),
            f"migration guide title is 'Migration guide to Zephyr v{s}.0 (Working Draft)'",
        )
        if prev is not None:
            span = f"from Zephyr v{prev.series}.0 to Zephyr v{s}.0"
            r.check(span in norm(guide), f"migration guide covers {prev.series} -> {s}")
            r.check(span in norm(notes), f"release notes overview covers {prev.series} -> {s}")

        info = self.parse_index()
        if info is not None:
            for stem in stems:
                r.check(self.in_toctree(info, stem), f"{stem} in the index.rst toctree")
            r.check(
                info["next"] == s,
                f"index.rst names {s} as the next planned release",
                f"found {info['next']}",
            )
        return target

    def check_release_docs(self, v):
        """Final release notes and migration guide of release v."""
        r = self.report
        s = v.series
        notes_path = f"{RELEASES}/release-notes-{s}.rst"
        guide_path = f"{RELEASES}/migration-guide-{s}.rst"
        r.section(f"Release notes and migration guide for {v}")
        notes = self.read_required(notes_path)
        if notes is not None:
            r.check(
                re.search(rf"^\.\. _zephyr_{re.escape(s)}:$", notes, re.MULTILINE) is not None,
                f"{notes_path} defines zephyr_{s}",
            )
            r.check(
                heading_present(notes, f"Zephyr {v}"), f"{notes_path} has a 'Zephyr {v}' section"
            )
            r.check("Working Draft" not in notes, f"{notes_path} is not marked 'Working Draft'")
            if v.patch == 0:
                m = re.search(r"Major enhancements with this release include:\n\n(\S.*)", notes)
                r.check(
                    m is not None and not m.group(1).startswith("An overview"),
                    "release notes list major enhancements",
                )
                r.check(
                    re.search(r"qualif", notes, re.IGNORECASE) is not None,
                    "release notes mention the Bluetooth qualification listing",
                    "check with the Bluetooth maintainers",
                    warn_only=True,
                )
        if v.patch != 0:
            return
        guide = self.read_required(guide_path)
        if guide is not None:
            r.check(
                re.search(rf"^\.\. _migration_{re.escape(s)}:$", guide, re.MULTILINE) is not None,
                f"{guide_path} defines migration_{s}",
            )
            r.check("Working Draft" not in guide, f"{guide_path} is not marked 'Working Draft'")
            prev = self.previous_release(v)
            if prev is not None:
                span = f"from Zephyr v{prev.series}.0 to Zephyr v{s}.0"
                r.check(span in norm(guide), f"migration guide covers {prev.series} -> {s}")
        draft = self.tree.read(NOTES_DRAFT)
        if draft is not None:
            r.check(
                f".. _zephyr_{s}:" not in draft,
                f"{NOTES_DRAFT} no longer targets {s}",
                f"duplicate zephyr_{s} label",
            )

    # --- GitHub ------------------------------------------------------------

    def github(self, title, func, *args):
        if self.gh is None:
            return
        self.report.section(f"GitHub: {title}")
        try:
            func(*args)
        except (urllib.error.URLError, OSError) as e:
            self.report.skip(f"GitHub API request failed: {e}")

    def gh_blockers(self, fatal):
        q = f'repo:{self.gh.repo} is:issue is:open label:"{BLOCKER_LABEL}"'
        res = self.gh.get("search/issues", {"q": q, "per_page": 50})
        items = res["items"] if res else []
        self.report.check(
            not items,
            f"no open '{BLOCKER_LABEL}' issues",
            f"{len(items)} open",
            warn_only=not fatal,
        )
        for i in items:
            print(f"      #{i['number']} {i['title']}")

    def gh_milestones(self, v, released):
        ms = {m["title"]: m for m in self.gh.milestones()}
        r = self.report
        cur = ms.get(v.release.tag)
        if not r.check(cur is not None, f"milestone {v.release.tag} exists"):
            return
        if released:
            r.check(cur["state"] == "closed", f"milestone {v.release.tag} closed", warn_only=True)
            r.check(
                cur["open_issues"] == 0,
                f"milestone {v.release.tag} has no open items",
                f"{cur['open_issues']} open, move them to the next milestone",
                warn_only=True,
            )
        newer = []
        for title, m in ms.items():
            try:
                mv = Version.parse(title)
            except ValueError:
                continue
            if mv.key() > v.release.key() and mv.patch == 0 and m["state"] == "open":
                newer.append(title)
        r.check(
            bool(newer),
            "milestone for the next release exists",
            "create it",
            warn_only=not released,
        )
        if newer:
            r.ok(f"next milestones: {', '.join(sorted(newer))}")

    def gh_backport_label(self, v, fatal):
        name = f"backport {v.branch}"
        label = self.gh.get(f"repos/{{repo}}/labels/{urllib.parse.quote(name)}")
        self.report.check(label is not None, f"label '{name}' exists", warn_only=not fatal)

    def gh_release(self, v):
        r = self.report
        rel = self.gh.get(f"repos/{{repo}}/releases/tags/{v.tag}")
        if rel is None:
            r.fail(
                f"no published GitHub release for {v.tag} (draft releases are only "
                "visible with a token with push access)"
            )
            return
        r.check(not rel["draft"], f"GitHub release {v.tag} is published")
        r.check(
            rel["prerelease"] == (v.rc is not None),
            f"GitHub release {v.tag} prerelease flag is {v.rc is not None}",
            warn_only=True,
        )
        if v.rc is None:
            body = rel.get("body") or ""
            r.check(
                f"{DOCS_URL}/" in body and v.series in body,
                "GitHub release links the release notes",
                warn_only=True,
            )

    # --- milestones --------------------------------------------------------

    def run_rc(self, v):
        prev = self.previous_release(v)
        self.check_version_file(v)
        self.check_tag(v, required=not self.args.pre_tag)
        self.check_rc_sequence(v)
        if prev is not None:
            self.check_supported_releases(prev)
        self.check_eol_releases()
        self.check_draft(v.release, prev)
        self.github("release blockers", self.gh_blockers, False)
        self.github("milestones", self.gh_milestones, v, False)
        if v.rc >= 3:
            self.github("labels", self.gh_backport_label, v, False)
        if not self.args.pre_tag:
            self.github("release", self.gh_release, v)

    def run_final(self, v):
        self.check_version_file(v)
        self.check_tag(v, required=not self.args.pre_tag)
        if v.patch == 0:
            self.check_rc_sequence(v)
        self.check_release_docs(v)
        if v.patch == 0:
            self.check_supported_releases(v)
            self.check_eol_releases()
        self.github("release blockers", self.gh_blockers, True)
        if v.patch == 0:
            self.github("labels", self.gh_backport_label, v, True)
        if not self.args.pre_tag:
            self.github("release", self.gh_release, v)

    def run_post_release(self, v):
        self.check_version_file(v, dev=True)
        self.check_tag(v)
        self.check_release_branch(v)
        target = None if self.args.next is None else Version.parse(self.args.next)
        found = self.check_draft(target, v)
        if found is not None:
            self.report.check(
                found.key() > v.key(), f"drafts target a release after {v}", f"found {found.series}"
            )
        self.check_supported_releases(v)
        self.check_eol_releases()
        self.github("release", self.gh_release, v)
        self.github("milestones", self.gh_milestones, v, True)
        self.github("labels", self.gh_backport_label, v, True)

    def infer(self):
        text = self.tree.read("VERSION")
        if text is None:
            sys.exit("VERSION not found")
        f = parse_version_file(text)
        major, minor = int(f["VERSION_MAJOR"]), int(f["VERSION_MINOR"])
        patch, extra = int(f["PATCHLEVEL"]), f.get("EXTRAVERSION", "")
        if patch == 99:
            return "post-release", Version(major, minor)
        m = re.fullmatch(r"rc(\d+)", extra)
        if m:
            return "rc", Version(major, minor, patch, int(m.group(1)))
        return "final", Version(major, minor, patch)

    def run(self):
        milestone, v = self.args.milestone, None
        if self.args.version is not None:
            v = Version.parse(self.args.version)
        if milestone == "auto" or v is None:
            inferred, iv = self.infer()
            if milestone == "auto":
                milestone = inferred
            v = v or iv
        if milestone == "rc" and v.rc is None:
            sys.exit("rc milestone needs a version such as 4.6.0-rc1")
        if milestone != "rc" and v.rc is not None:
            sys.exit(f"{milestone} milestone needs a version without -rc")
        if milestone == "post-release":
            v = Version(v.major, v.minor)

        print(
            f"Checking milestone '{milestone}' for Zephyr {v} "
            f"({self.args.ref or 'working tree'}, remote {self.args.remote})"
        )
        if self.gh is not None and not self.gh.token:
            print("No GITHUB_TOKEN set, GitHub API requests are rate limited")
        {"rc": self.run_rc, "final": self.run_final, "post-release": self.run_post_release}[
            milestone
        ](v)
        return self.report.summary()


def parse_args():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )
    parser.add_argument(
        "milestone",
        nargs="?",
        default="auto",
        choices=["auto", "rc", "final", "post-release"],
        help="release milestone to verify (default: %(default)s)",
    )
    parser.add_argument(
        "version",
        nargs="?",
        help="release version, e.g. 4.6.0-rc1 or 4.6.0 (default: from the VERSION file)",
    )
    parser.add_argument(
        "--ref", help="check files at this git ref (e.g. upstream/main) instead of the working tree"
    )
    parser.add_argument(
        "--remote",
        default="upstream",
        help="git remote of the Zephyr repository (default: %(default)s)",
    )
    parser.add_argument(
        "--main-branch",
        default="main",
        help="development branch on the remote (default: %(default)s)",
    )
    parser.add_argument(
        "--repo",
        default="zephyrproject-rtos/zephyr",
        help="GitHub repository (default: %(default)s)",
    )
    parser.add_argument(
        "--next", help="post-release: expected version of the next release, e.g. 5.0"
    )
    parser.add_argument(
        "--pre-tag",
        action="store_true",
        help="the tag is not created yet, check the release PR only",
    )
    parser.add_argument(
        "--no-github", action="store_true", help="skip checks that use the GitHub API"
    )
    parser.add_argument(
        "--zephyr-base",
        type=Path,
        default=ZEPHYR_BASE,
        help="Zephyr repository (default: %(default)s)",
    )
    parser.add_argument("-v", "--verbose", action="store_true", help="also print passed checks")
    return parser.parse_args()


def main():
    return 0 if Checker(parse_args()).run() else 1


if __name__ == "__main__":
    sys.exit(main())
