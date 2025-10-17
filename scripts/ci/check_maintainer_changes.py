#!/usr/bin/env python3

# SPDX-License-Identifier: Apache-2.0
# Copyright The Zephyr Project Contributors

import argparse
import os
import sys

import yaml
from github import Auth, Github


def load_areas(filename: str):
    with open(filename) as f:
        doc = yaml.safe_load(f)
    return {
        k: v for k, v in doc.items() if isinstance(v, dict) and ("files" in v or "files-regex" in v)
    }


def set_or_empty(d, key):
    return set(d.get(key, []) or [])


def check_github_access(usernames, repo_fullname, token):
    """Check if each username has at least Triage access to the repo."""
    gh = Github(auth=Auth.Token(token))
    repo = gh.get_repo(repo_fullname)
    missing_access = set()
    for username in usernames:
        try:
            collab = repo.get_collaborator_role_name(username)
            # Roles: admin, maintain, write, triage, read
            if collab not in ("admin", "maintain", "write", "triage"):
                missing_access.add(username)
        except Exception:
            missing_access.add(username)
    return missing_access


def compare_areas(old, new, repo_fullname=None, token=None):
    old_areas = set(old.keys())
    new_areas = set(new.keys())

    added_areas = new_areas - old_areas
    removed_areas = old_areas - new_areas
    common_areas = old_areas & new_areas

    all_added_maintainers = set()
    all_added_collaborators = set()

    print("=== Areas Added ===")
    for area in sorted(added_areas):
        print(f"+ {area}")
        entry = new[area]
        all_added_maintainers.update(set_or_empty(entry, "maintainers"))
        all_added_collaborators.update(set_or_empty(entry, "collaborators"))

    print("\n=== Areas Removed ===")
    for area in sorted(removed_areas):
        print(f"- {area}")

    print("\n=== Area Changes ===")
    for area in sorted(common_areas):
        changes = []
        old_entry = old[area]
        new_entry = new[area]

        # Compare maintainers
        old_maint = set_or_empty(old_entry, "maintainers")
        new_maint = set_or_empty(new_entry, "maintainers")
        added_maint = new_maint - old_maint
        removed_maint = old_maint - new_maint
        if added_maint:
            changes.append(f"  Maintainers added: {', '.join(sorted(added_maint))}")
            all_added_maintainers.update(added_maint)
        if removed_maint:
            changes.append(f"  Maintainers removed: {', '.join(sorted(removed_maint))}")

        # Compare collaborators
        old_collab = set_or_empty(old_entry, "collaborators")
        new_collab = set_or_empty(new_entry, "collaborators")
        added_collab = new_collab - old_collab
        removed_collab = old_collab - new_collab
        if added_collab:
            changes.append(f"  Collaborators added: {', '.join(sorted(added_collab))}")
            all_added_collaborators.update(added_collab)
        if removed_collab:
            changes.append(f"  Collaborators removed: {', '.join(sorted(removed_collab))}")

        # Compare status
        old_status = old_entry.get("status")
        new_status = new_entry.get("status")
        if old_status != new_status:
            changes.append(f"  Status changed: {old_status} -> {new_status}")

        # Compare labels
        old_labels = set_or_empty(old_entry, "labels")
        new_labels = set_or_empty(new_entry, "labels")
        added_labels = new_labels - old_labels
        removed_labels = old_labels - new_labels
        if added_labels:
            changes.append(f"  Labels added: {', '.join(sorted(added_labels))}")
        if removed_labels:
            changes.append(f"  Labels removed: {', '.join(sorted(removed_labels))}")

        # Compare files
        old_files = set_or_empty(old_entry, "files")
        new_files = set_or_empty(new_entry, "files")
        added_files = new_files - old_files
        removed_files = old_files - new_files
        if added_files:
            changes.append(f"  Files added: {', '.join(sorted(added_files))}")
        if removed_files:
            changes.append(f"  Files removed: {', '.join(sorted(removed_files))}")

        # Compare files-regex
        old_regex = set_or_empty(old_entry, "files-regex")
        new_regex = set_or_empty(new_entry, "files-regex")
        added_regex = new_regex - old_regex
        removed_regex = old_regex - new_regex
        if added_regex:
            changes.append(f"  files-regex added: {', '.join(sorted(added_regex))}")
        if removed_regex:
            changes.append(f"  files-regex removed: {', '.join(sorted(removed_regex))}")

        if changes:
            print(f"* {area}")
            for c in changes:
                print(c)

    print("\n=== Summary ===")
    print(f"Total areas added: {len(added_areas)}")
    print(f"Total maintainers added: {len(all_added_maintainers)}")
    if all_added_maintainers:
        print("  Added maintainers: " + ", ".join(sorted(all_added_maintainers)))
    print(f"Total collaborators added: {len(all_added_collaborators)}")
    if all_added_collaborators:
        print("  Added collaborators: " + ", ".join(sorted(all_added_collaborators)))

    # Check GitHub access if repo and token are provided

    print("\n=== GitHub Access Check ===")
    missing_maint = check_github_access(all_added_maintainers, repo_fullname, token)
    missing_collab = check_github_access(all_added_collaborators, repo_fullname, token)
    if missing_maint:
        print("Maintainers without at least triage access:")
        for u in sorted(missing_maint):
            print(f"  - {u}")
    if missing_collab:
        print("Collaborators without at least triage access:")
        for u in sorted(missing_collab):
            print(f"  - {u}")
    if not missing_maint and not missing_collab:
        print("All added maintainers and collaborators have required access.")
    else:
        print("Some added maintainers or collaborators do not have sufficient access.")

        # --- GitHub Actions inline annotation ---
        # Try to find the line number in the new file for each missing user
        def find_line_for_user(yaml_file, user_set):
            """Return a dict of user -> line number in yaml_file for missing users."""
            user_lines = {}
            try:
                with open(yaml_file) as f:
                    lines = f.readlines()
                for idx, line in enumerate(lines, 1):
                    for user in user_set:
                        if user in line:
                            user_lines[user] = idx
                return user_lines
            except Exception:
                return {}

        all_missing_users = missing_maint | missing_collab
        user_lines = find_line_for_user(args.new, all_missing_users)

        for user, line in user_lines.items():
            print(
                f"::error file={args.new},line={line},title=User lacks access::"
                f"{user} does not have needed access level to {repo_fullname}"
            )

        # For any missing users not found in the file, print a general error
        for user in sorted(all_missing_users - set(user_lines)):
            print(
                f"::error title=User lacks access::{user} does not have needed "
                f"access level to {repo_fullname}"
            )

        sys.exit(1)


def main():
    parser = argparse.ArgumentParser(
        description="Compare two MAINTAINERS.yml files and show changes in areas, "
        "maintainers, collaborators, etc.",
        allow_abbrev=False,
    )
    parser.add_argument("old", help="Old MAINTAINERS.yml file")
    parser.add_argument("new", help="New MAINTAINERS.yml file")
    parser.add_argument("--repo", help="GitHub repository in org/repo format for access check")
    parser.add_argument("--token", help="GitHub token for API access (required for access check)")
    global args
    args = parser.parse_args()

    old_areas = load_areas(args.old)
    new_areas = load_areas(args.new)
    token = os.environ.get("GITHUB_TOKEN") or args.token

    if not token or not args.repo:
        print("GitHub token and repository are required for access check.")
        sys.exit(1)

    compare_areas(old_areas, new_areas, repo_fullname=args.repo, token=token)


if __name__ == "__main__":
    main()
