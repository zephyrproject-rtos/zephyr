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
from collections import deque
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

    labels = deque()
    area_counter = defaultdict(int)
    found_maintainers = defaultdict(int)

    num_files = 0
    all_areas = set()
    fn = list(pr.get_files())

    for changed_file in fn:
        if changed_file.filename in ['west.yml','submanifests/optional.yaml']:
            break

    if pr.commits == 1 and (pr.additions <= 1 and pr.deletions <= 1):
        labels.append('size: XS')

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
        #
        # DJL: Don't understand what "is_instance" is trying to record?
        # It mucks with the weighting though.
        #
        #is_instance = False
        sorted_areas = sorted(areas, key=lambda x: 'Platform' in x.name, reverse=True)
        for area in sorted_areas:
            #c = 1 if not is_instance else 0

            area_counter[area] += 1

            # FIXME: Here we count the same file multiple times if it exists in
            # multiple areas with same maintainer
            for area_maintainer in area.maintainers:
                found_maintainers[area_maintainer] += 1

            #if 'Platform' in area.name:
            #    is_instance = True

    area_counter = dict(sorted(area_counter.items(), key=lambda item: item[1], reverse=True))

    # selectively pick some of the most likely labels
    for item in area_counter:
        #
        # Prioritize 'Release Notes' label to ensure it is automatically
        # added since touching the release notes can potentially be a
        # low percentage item in a large PR.
        #
        # I know this is hard coded but not sure how else to handle this.
        #
        if 'Release Notes' in item.labels:
            labels.appendleft('Release Notes')

        #
        # Not sure what the right percentage is. When a PR has touched
        # a lot of files it will create a giant list. So try to find
        # just the top areas touched and hopefully it will get the right
        # labels.
        #
        prop = (area_counter[item] / num_files) * 100
        # log(f":::: label {item.labels} : count {area_counter[item]} : {prop:.2f}%")

        if prop > 15:
            for l in item.labels:
                if l not in labels:
                    labels.append(l)

    log(f"Area matches: {area_counter}")
    log(f"labels: {labels}")

    # Create a list of collaborators ordered by the area match
    #
    # There is a limit to how many reviewers can be assigned
    # to the PR. The following allocation strategy attempts
    # to more fairly distribute the assigned reviewers across
    # the impacted areas.
    #
    # Bias walking through each touched area to pull in the
    # maintainers from that area into the collab list
    # first.
    #
    # Then process the collaborators by using a round-robin
    # merging strategy by picking one collaborator at a time
    # from each area impacted.
    #
    # Note the order of the names in each collaborator list
    # will matter if there is a large array of areas being
    # touched as it will be more likely to select someone
    # from the top of the list. As such, it may be more
    # prudent to put higher priority people at the top of
    # the list... sort of a "promotion ladder".
    #
	# Exammple:
    # Areas (by priority/impact order): A, B, C, D
    #
    # collab list would look like:
    #
    # maintainers [A, B, C, D] followed by
    # collaborators [A1, B1, C1, D1, A2, B2, C2, D2, A3, ... ]
    #
	# If there is only 1 collaborator in area B then the array is:
	#
	# collaborators [A1, B1, C1, D1, A2, C2, D2, A3, ... ]
    #
    # An alternative is to just walk the collabs list taking them
    # in groups by just doing the loop again:
    #
    #     for area in area_counter:
    #         collab += maintainer_file.areas[area.name].maintainers
    #
    # collaborators [A1, A2, A3, B1, B2, C1, C2, D1, D2, ... ]
    #
    # Down side seem to be a strong bias to the top list, which
    # may be better.
    #
    collab = list()
    for area in area_counter:
        collab += maintainer_file.areas[area.name].maintainers

    # Perform a round-robin merging of the maintainers from each area represented in the
    # PR. Create an array of areas where each element is a deque collection of collaborators
    # from each area.
    area_collab_queues = [deque(maintainer_file.areas[area.name].collaborators) for area in area_counter]

    while any(area_collab_queues):
        for queue in area_collab_queues:
            if queue:
                collab.append(queue.popleft())

    # remove duplicates from collab list while ensuring order
    collab = list(dict.fromkeys(collab))
    log(f"collab: {collab}")

    _all_maintainers = dict(sorted(found_maintainers.items(), key=lambda item: item[1], reverse=True))

    log(f"Submitted by: {pr.user.login}")
    log(f"candidate maintainers: {_all_maintainers}")

    assignees = []
    tmp_assignees = []

    # we start with areas with most files changed and pick the maintainer from the first one.
    # if the first area is an implementation, i.e. driver or platform, we
    # continue searching for any other areas involved
    for area, count in area_counter.items():
        log(f"area: {area}, count: {count}, maintainers: {area.maintainers}")
        if count == 0:
            continue
        if len(area.maintainers) > 0:
            tmp_assignees = area.maintainers
            if pr.user.login in area.maintainers:
                # submitter = assignee, try to pick next area and
                # assign someone else other than the submitter
                continue
            else:
                assignees = area.maintainers

            if 'Platform' not in area.name:
                break

    if tmp_assignees and not assignees:
        assignees = tmp_assignees

    if assignees:
        prop = (found_maintainers[assignees[0]] / num_files) * 100
        log(f"Picked assignees: {assignees} ({prop:.2f}% ownership)")
        log("+++++++++++++++++++++++++")

    # Set labels by applying the most important ones first
    if labels:
        for _ in range(10):
            try:
                tmp_label = labels.popleft()
                log(f"adding label {tmp_label}")
                if not args.dry_run:
                    pr.add_to_labels(tmp_label)
            except IndexError:
                break

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
                if pr.user == gh_user or gh_user in existing_reviewers:
                    continue
                if not gh_repo.has_in_collaborators(gh_user):
                    log(f"Skip '{collaborator}': not in collaborators")
                    continue
                if gh_user in self_removal:
                    log(f"Skip '{collaborator}': self removed")
                    continue
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
    if assignees and not pr.assignee:
        try:
            for assignee in assignees:
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
