#!/usr/bin/env python3

# Copyright (c) 2022 Intel Corp.
# SPDX-License-Identifier: Apache-2.0

import argparse
import sys
import os
import time
import datetime
from github import Github, GithubException
from github.GithubException import UnknownObjectException
from collections import defaultdict
from west.manifest import Manifest
from west.manifest import ManifestProject

TOP_DIR = os.path.join(os.path.dirname(__file__))
sys.path.insert(0, os.path.join(TOP_DIR, "scripts"))
from get_maintainer import Maintainers

def log(s):
    if args.verbose > 0:
        print(s, file=sys.stdout)

def parse_args():
    global args
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter, allow_abbrev=False)

    parser.add_argument("-M", "--maintainer-file", required=False, default="MAINTAINERS.yml",
                        help="Maintainer file to be used.")

    group = parser.add_mutually_exclusive_group()
    group.add_argument("-P", "--pull_request", required=False, default=None, type=int,
                       help="Operate on one pull-request only.")
    group.add_argument("-I", "--issue", required=False, default=None, type=int,
                       help="Operate on one issue only.")
    group.add_argument("-s", "--since", required=False,
                       help="Process pull-requests since date.")
    group.add_argument("-m", "--modules", action="store_true",
                       help="Process pull-requests from modules.")

    parser.add_argument("-y", "--dry-run", action="store_true", default=False,
                        help="Dry run only.")

    parser.add_argument("-o", "--org", default="zephyrproject-rtos",
                        help="Github organisation")

    parser.add_argument("-r", "--repo", default="zephyr",
                        help="Github repository")

    parser.add_argument("-v", "--verbose", action="count", default=0,
                        help="Verbose Output")

    args = parser.parse_args()

def process_pr(gh, maintainer_file, number):

    gh_repo = gh.get_repo(f"{args.org}/{args.repo}")
    pr = gh_repo.get_pull(number)

    log(f"working on https://github.com/{args.org}/{args.repo}/pull/{pr.number} : {pr.title}")

    labels = set()
    area_counter = defaultdict(int)
    found_maintainers = defaultdict(int)

    num_files = 0
    all_areas = set()
    fn = list(pr.get_files())

    manifest_change = False
    for changed_file in fn:
        if changed_file.filename in ['west.yml','submanifests/optional.yaml']:
            manifest_change = True
            break

    # one liner PRs should be trivial
    if pr.commits == 1 and (pr.additions <= 1 and pr.deletions <= 1) and not manifest_change:
        labels = {'Trivial'}

    if len(fn) > 500:
        log(f"Too many files changed ({len(fn)}), skipping....")
        return

    for changed_file in fn:
        num_files += 1
        log(f"file: {changed_file.filename}")
        areas = maintainer_file.path2areas(changed_file.filename)

        if not areas:
            continue

        all_areas.update(areas)
        is_instance = False
        sorted_areas = sorted(areas, key=lambda x: 'Platform' in x.name, reverse=True)
        for area in sorted_areas:
            c = 1 if not is_instance else 0

            area_counter[area] += c
            labels.update(area.labels)
            # FIXME: Here we count the same file multiple times if it exists in
            # multiple areas with same maintainer
            for area_maintainer in area.maintainers:
                found_maintainers[area_maintainer] += c

            if 'Platform' in area.name:
                is_instance = True

    area_counter = dict(sorted(area_counter.items(), key=lambda item: item[1], reverse=True))
    log(f"Area matches: {area_counter}")
    log(f"labels: {labels}")

    # Create a list of collaborators ordered by the area match
    collab = list()
    for area in area_counter:
        collab += maintainer_file.areas[area.name].maintainers
        collab += maintainer_file.areas[area.name].collaborators
    collab = list(dict.fromkeys(collab))
    log(f"collab: {collab}")

    _all_maintainers = dict(sorted(found_maintainers.items(), key=lambda item: item[1], reverse=True))

    log(f"Submitted by: {pr.user.login}")
    log(f"candidate maintainers: {_all_maintainers}")

    maintainers = list(_all_maintainers.keys())
    assignee = None

    # we start with areas with most files changed and pick the maintainer from the first one.
    # if the first area is an implementation, i.e. driver or platform, we
    # continue searching for any other areas
    for area, count in area_counter.items():
        if count == 0:
            continue
        if len(area.maintainers) > 0:
            assignee = area.maintainers[0]

            if 'Platform' not in area.name:
                break

    # if the submitter is the same as the maintainer, check if we have
    # multiple maintainers
    if len(maintainers) > 1 and pr.user.login == assignee:
        log("Submitter is same as Assignee, trying to find another assignee...")
        aff = list(area_counter.keys())[0]
        for area in all_areas:
            if area == aff:
                if len(area.maintainers) > 1:
                    assignee = area.maintainers[1]
                else:
                    log(f"This area has only one maintainer, keeping assignee as {assignee}")

    if assignee:
        prop = (found_maintainers[assignee] / num_files) * 100
        log(f"Picked assignee: {assignee} ({prop:.2f}% ownership)")
        log("+++++++++++++++++++++++++")

    # Set labels
    if labels:
        if len(labels) < 10:
            for l in labels:
                log(f"adding label {l}...")
                if not args.dry_run:
                    pr.add_to_labels(l)
        else:
            log(f"Too many labels to be applied")

    if collab:
        reviewers = []
        existing_reviewers = set()

        revs = pr.get_reviews()
        for review in revs:
            existing_reviewers.add(review.user)

        rl = pr.get_review_requests()
        page = 0
        for r in rl:
            existing_reviewers |= set(r.get_page(page))
            page += 1

        # check for reviewers that remove themselves from list of reviewer and
        # do not attempt to add them again based on MAINTAINERS file.
        self_removal = []
        for event in pr.get_issue_events():
            if event.event == 'review_request_removed' and event.actor == event.requested_reviewer:
                self_removal.append(event.actor)

        for collaborator in collab:
            try:
                gh_user = gh.get_user(collaborator)
                if pr.user != gh_user and gh_repo.has_in_collaborators(gh_user):
                    if gh_user not in existing_reviewers and gh_user not in self_removal:
                        reviewers.append(collaborator)
            except UnknownObjectException as e:
                log(f"Can't get user '{collaborator}', account does not exist anymore? ({e})")

        if len(existing_reviewers) < 15:
            reviewer_vacancy = 15 - len(existing_reviewers)
            reviewers = reviewers[:reviewer_vacancy]

            if reviewers:
                try:
                    log(f"adding reviewers {reviewers}...")
                    if not args.dry_run:
                        pr.create_review_request(reviewers=reviewers)
                except GithubException:
                    log("cant add reviewer")
        else:
            log("not adding reviewers because the existing reviewer count is greater than or "
                "equal to 15")

    ms = []
    # assignees
    if assignee and not pr.assignee:
        try:
            u = gh.get_user(assignee)
            ms.append(u)
        except GithubException:
            log(f"Error: Unknown user")

        for mm in ms:
            log(f"Adding assignee {mm}...")
            if not args.dry_run:
                pr.add_to_assignees(mm)
    else:
        log("not setting assignee")

    time.sleep(1)


def process_issue(gh, maintainer_file, number):
    gh_repo = gh.get_repo(f"{args.org}/{args.repo}")
    issue = gh_repo.get_issue(number)

    log(f"Working on {issue.url}: {issue.title}")

    if issue.assignees:
        print(f"Already assigned {issue.assignees}, bailing out")
        return

    label_to_maintainer = defaultdict(set)
    for _, area in maintainer_file.areas.items():
        if not area.labels:
            continue

        labels = set()
        for label in area.labels:
            labels.add(label.lower())
        labels = tuple(sorted(labels))

        for maintainer in area.maintainers:
            label_to_maintainer[labels].add(maintainer)

    # Add extra entries for areas with multiple labels so they match with just
    # one label if it's specific enough.
    for areas, maintainers in dict(label_to_maintainer).items():
        for area in areas:
            if tuple([area]) not in label_to_maintainer:
                label_to_maintainer[tuple([area])] = maintainers

    issue_labels = set()
    for label in issue.labels:
        label_name = label.name.lower()
        if tuple([label_name]) not in label_to_maintainer:
            print(f"Ignoring label: {label}")
            continue
        issue_labels.add(label_name)
    issue_labels = tuple(sorted(issue_labels))

    print(f"Using labels: {issue_labels}")

    if issue_labels not in label_to_maintainer:
        print(f"no match for the label set, not assigning")
        return

    for maintainer in label_to_maintainer[issue_labels]:
        log(f"Adding {maintainer} to {issue.html_url}")
        if not args.dry_run:
            issue.add_to_assignees(maintainer)


def process_modules(gh, maintainers_file):
    manifest = Manifest.from_file()

    repos = {}
    for project in manifest.get_projects([]):
        if not manifest.is_active(project):
            continue

        if isinstance(project, ManifestProject):
            continue

        area = f"West project: {project.name}"
        if area not in maintainers_file.areas:
            log(f"No area for: {area}")
            continue

        maintainers = maintainers_file.areas[area].maintainers
        if not maintainers:
            log(f"No maintainers for: {area}")
            continue

        collaborators = maintainers_file.areas[area].collaborators

        log(f"Found {area}, maintainers={maintainers}, collaborators={collaborators}")

        repo_name = f"{args.org}/{project.name}"
        repos[repo_name] = maintainers_file.areas[area]

    query = f"is:open is:pr no:assignee"
    for repo in repos:
        query += f" repo:{repo}"

    issues = gh.search_issues(query=query)
    for issue in issues:
        pull = issue.as_pull_request()

        if pull.draft:
            continue

        if pull.assignees:
            log(f"ERROR: {pull.html_url} should have no assignees, found {pull.assignees}")
            continue

        repo_name = f"{args.org}/{issue.repository.name}"
        area = repos[repo_name]

        for maintainer in area.maintainers:
            log(f"Assigning {maintainer} to {pull.html_url}")
            if not args.dry_run:
                pull.add_to_assignees(maintainer)
                pull.create_review_request(maintainer)

        for collaborator in area.collaborators:
            log(f"Adding {collaborator} to {pull.html_url}")
            if not args.dry_run:
                pull.create_review_request(collaborator)


def main():
    parse_args()

    token = os.environ.get('GITHUB_TOKEN', None)
    if not token:
        sys.exit('Github token not set in environment, please set the '
                 'GITHUB_TOKEN environment variable and retry.')

    gh = Github(token)
    maintainer_file = Maintainers(args.maintainer_file)

    if args.pull_request:
        process_pr(gh, maintainer_file, args.pull_request)
    elif args.issue:
        process_issue(gh, maintainer_file, args.issue)
    elif args.modules:
        process_modules(gh, maintainer_file)
    else:
        if args.since:
            since = args.since
        else:
            today = datetime.date.today()
            since = today - datetime.timedelta(days=1)

        common_prs = f'repo:{args.org}/{args.repo} is:open is:pr base:main -is:draft no:assignee created:>{since}'
        pulls = gh.search_issues(query=f'{common_prs}')

        for issue in pulls:
            process_pr(gh, maintainer_file, issue.number)


if __name__ == "__main__":
    main()
