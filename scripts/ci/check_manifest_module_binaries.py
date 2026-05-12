#!/usr/bin/env python3

# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

"""Detect binary files added by west manifest module revision updates."""

import argparse
import subprocess
import sys
import tempfile
from pathlib import Path

import yaml


def git(repo, *args, check=True):
    result = subprocess.run(("git", *args), cwd=repo, capture_output=True, check=False)
    if check and result.returncode:
        print(result.stderr.decode("utf-8", errors="replace"), file=sys.stderr, end="")
        sys.exit(result.returncode)

    return result


def git_stdout(repo, *args):
    return git(repo, *args).stdout.decode("utf-8-sig", errors="replace").rstrip()


def parse_allow_rule(rule):
    if ":" not in rule:
        raise argparse.ArgumentTypeError(
            f"invalid allow rule '{rule}', expected PROJECT:PATH_PREFIX:EXT[,EXT...]"
        )

    project, rest = rule.split(":", 1)
    if ":" not in rest:
        raise argparse.ArgumentTypeError(
            f"invalid allow rule '{rule}', expected PROJECT:PATH_PREFIX:EXT[,EXT...]"
        )

    prefix, extensions = rest.split(":", 1)
    extensions = tuple(ext.strip() for ext in extensions.split(",") if ext.strip())
    if not project or not prefix or not extensions:
        raise argparse.ArgumentTypeError(
            f"invalid allow rule '{rule}', expected PROJECT:PATH_PREFIX:EXT[,EXT...]"
        )

    return project, prefix, extensions


def is_allowed(project, path, allow_rules):
    return any(
        rule_project in ("*", project) and path.startswith(prefix) and path.endswith(extensions)
        for rule_project, prefix, extensions in allow_rules
    )


def manifest_at_revision(repo, revision, manifest_path):
    content = git_stdout(repo, "show", f"{revision}:{manifest_path}").lstrip("\ufeff")
    return yaml.safe_load(content)


def manifest_projects(manifest):
    manifest = manifest.get("manifest", manifest.get("\ufeffmanifest", {}))
    default_remote = manifest.get("defaults", {}).get("remote")
    remotes = {remote["name"]: remote["url-base"] for remote in manifest.get("remotes", [])}
    projects = {}

    for project in manifest.get("projects", []):
        name = project.get("name")
        revision = project.get("revision")
        if not name or not revision:
            continue

        url = project.get("url")
        if not url:
            remote = project.get("remote", default_remote)
            url_base = remotes.get(remote)
            if not url_base:
                continue
            url = f"{url_base.rstrip('/')}/{project.get('repo-path', name)}"

        projects[name] = {"revision": revision, "url": url}

    return projects


def changed_projects(old_manifest, new_manifest, project_filter):
    old_projects = manifest_projects(old_manifest)
    new_projects = manifest_projects(new_manifest)
    changed = []

    for name, new_project in sorted(new_projects.items()):
        if project_filter and name not in project_filter:
            continue

        old_project = old_projects.get(name)
        if old_project is None:
            continue
        if old_project["revision"] == new_project["revision"]:
            continue

        changed.append((name, old_project["url"], old_project["revision"], new_project["revision"]))

    return changed


def fetch_ref(repo, url, revision, refname, depth):
    depth_arg = (f"--depth={depth}",) if depth else ()
    result = git(repo, "fetch", "--no-tags", *depth_arg, url, revision, check=False)
    if result.returncode:
        print(f"warning: failed to fetch {url} {revision}:", file=sys.stderr)
        print(result.stderr.decode("utf-8", errors="replace"), file=sys.stderr, end="")
        return False

    git(repo, "update-ref", refname, "FETCH_HEAD")
    return True


def added_binary_files(url, old_revision, new_revision, depth, allow_rules, project):
    with tempfile.TemporaryDirectory() as temp_dir:
        repo = Path(temp_dir)
        git(repo, "init", "-q")

        old_ref = "refs/module-binary-check/old"
        new_ref = "refs/module-binary-check/new"
        if not fetch_ref(repo, url, old_revision, old_ref, depth):
            return None
        if not fetch_ref(repo, url, new_revision, new_ref, depth):
            return None

        output = git_stdout(
            repo, "diff", "--numstat", "--no-renames", "--diff-filter=A", old_ref, new_ref
        )
        binaries = []

        for line in output.splitlines():
            try:
                added, deleted, path = line.split("\t", 2)
            except ValueError:
                print(f"warning: ignoring unexpected numstat line: {line}", file=sys.stderr)
                continue

            if added == "-" and deleted == "-" and not is_allowed(project, path, allow_rules):
                binaries.append(path)

        return binaries


def parse_args(argv):
    parser = argparse.ArgumentParser(
        description="Check changed west manifest modules for newly added binary files.",
        allow_abbrev=False,
    )
    parser.add_argument(
        "--repo",
        default=".",
        help="Zephyr repository path, default is the current directory",
    )
    parser.add_argument(
        "--manifest-path",
        default="west.yml",
        help="manifest path inside the Zephyr repository, default is west.yml",
    )
    parser.add_argument(
        "--old-rev", required=True, help="old Zephyr revision to read manifest from"
    )
    parser.add_argument(
        "--new-rev", required=True, help="new Zephyr revision to read manifest from"
    )
    parser.add_argument(
        "--project",
        action="append",
        default=[],
        help="manifest project name to inspect; may be given multiple times",
    )
    parser.add_argument(
        "--allow",
        action="append",
        default=[],
        type=parse_allow_rule,
        metavar="PROJECT:PATH_PREFIX:EXT[,EXT...]",
        help="allow added binaries matching a project, path prefix, and extension list",
    )
    parser.add_argument(
        "--fetch-depth",
        type=int,
        default=1,
        help="depth used when fetching module revisions, default is 1; use 0 for full fetch",
    )

    return parser.parse_args(argv)


def main(argv=None):
    args = parse_args(argv)
    repo = Path(args.repo).resolve()
    old_manifest = manifest_at_revision(repo, args.old_rev, args.manifest_path)
    new_manifest = manifest_at_revision(repo, args.new_rev, args.manifest_path)
    projects = changed_projects(old_manifest, new_manifest, set(args.project))
    failures = []

    for name, url, old_revision, new_revision in projects:
        binaries = added_binary_files(
            url, old_revision, new_revision, args.fetch_depth, args.allow, name
        )
        if binaries is None:
            failures.append(f"{name}: unable to inspect {old_revision}..{new_revision}")
            continue
        for binary in binaries:
            failures.append(f"{name}: added binary file: {binary}")

    if failures:
        print("Disallowed binary files were added by manifest module updates:", file=sys.stderr)
        for failure in failures:
            print(f"  {failure}", file=sys.stderr)
        return 1

    print("No disallowed added binary files found in changed manifest modules")
    return 0


if __name__ == "__main__":
    sys.exit(main())
