#!/usr/bin/env python3
# Copyright (c) 2024 Intel Corp.
# SPDX-License-Identifier: Apache-2.0

# Script that operates on a merged PR and sends data to elasticsearch for
# further insepctions using the PR dashboard at
# https://kibana.zephyrproject.io/

import sys
import os
from github import Github
import argparse
from elasticsearch import Elasticsearch
from elasticsearch.helpers import bulk
from datetime import timedelta
import pprint


date_format = '%Y-%m-%d %H:%M:%S'

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter, allow_abbrev=False)

    parser.add_argument('--pull-request', help='pull request number', type=int)
    parser.add_argument('--range', help='execute based on a date range, for example 2023-01-01..2023-01-05')
    parser.add_argument('--repo', help='github repo', default='zephyrproject-rtos/zephyr')
    parser.add_argument('--es-index', help='Elasticsearch index')
    parser.add_argument('-y','--dry-run', action="store_true", help='dry run, do not upload data')

    return parser.parse_args()

def gendata(data, index):
    for t in data:
        yield {
                "_index": index,
                "_source": t
                }

def process_pr(pr):
    reviews = pr.get_reviews()
    print(f'#{pr.number}: {pr.title} - {pr.comments} Comments, reviews: {reviews.totalCount}, {len(pr.assignees)} Assignees (Updated {pr.updated_at})')
    assignee_reviews = 0
    prj = {}

    assignees = []
    labels = []
    for label in pr.labels:
        labels.append(label.name)

    reviewers = set()
    for review in reviews:
        # get list of all approved reviews
        if review.user and review.state == 'APPROVED':
            reviewers.add(review.user.login)

    for assignee in pr.assignees:
        # list assignees for later checks
        assignees.append(assignee.login)
        if assignee.login in reviewers:
            assignee_reviews += 1

    if assignee_reviews > 0 or pr.merged_by.login in assignees:
        # in case of assignee reviews or if PR was merged by an assignee
        prj['review_rule'] = "yes"
    elif not pr.assignees or \
            (pr.user.login in assignees and len(assignees) == 1) or \
            ('Trivial' in labels or 'Hotfix' in labels):
        # in case where no assignees set or if submitter is the only assignee
        # or in case of trivial or hotfixes
        prj['review_rule'] = "na"
    else:
        # everything else
        prj['review_rule'] = "no"


    created = pr.created_at
    # if a PR was made ready for review from draft, calculate based on when it
    # was moved out of draft.
    for event in pr.get_issue_events():
        if event.event == 'ready_for_review':
            created = event.created_at

    # calculate time the PR was in review, hours and business days.
    delta = pr.closed_at - created
    deltah = delta.total_seconds() / 3600
    prj['hours_open'] = deltah

    dates = (created + timedelta(idx + 1) for idx in range((pr.closed_at - created).days))

    # Get number of business days per the guidelines, we need at least 2.
    business_days = sum(1 for day in dates if day.weekday() < 5)
    prj['business_days_open'] = business_days

    release_notes = 'Release Notes' in labels and len(labels) == 1
    trivial = 'Trivial' in labels
    hotfix = 'Hotfix' in labels
    min_review_time_rule = "no"

    if hotfix or ((release_notes or trivial) and deltah >= 4) or business_days >= 2:
        min_review_time_rule = "yes"

    prj['time_rule'] = min_review_time_rule

    # This is all data we get easily though the Github API and serves as the basis
    # for displaying some trends and metrics.
    # Data can be extended in the future if we find more information that
    # is useful through the API

    prj['nr'] = pr.number
    prj['url'] = pr.url
    prj['title'] = pr.title
    prj['comments'] = pr.comments
    prj['reviews'] = reviews.totalCount
    prj['assignees'] = assignees
    prj['updated'] = pr.updated_at.strftime("%Y-%m-%d %H:%M:%S")
    prj['created'] = pr.created_at.strftime("%Y-%m-%d %H:%M:%S")
    prj['closed'] = pr.closed_at.strftime("%Y-%m-%d %H:%M:%S")
    prj['merged_by'] = pr.merged_by.login
    prj['submitted_by'] = pr.user.login
    prj['changed_files'] = pr.changed_files
    prj['additions'] = pr.additions
    prj['deletions'] = pr.deletions
    prj['commits'] = pr.commits
    # The branch we are targeting. main vs release branches.
    prj['base'] = pr.base.ref

    # list all reviewers
    prj['reviewers'] = list(reviewers)
    prj['labels'] = labels

    return prj

def main():
    args = parse_args()
    token = os.environ.get('GITHUB_TOKEN')
    if not token:
        sys.exit('Github token not set in environment, please set the '
                 'GITHUB_TOKEN environment variable and retry.')

    gh = Github(token)
    json_list = []
    gh_repo = gh.get_repo(args.repo)

    if args.pull_request:
        pr = gh_repo.get_pull(args.pull_request)
        prj = process_pr(pr)
        json_list.append(prj)
    elif args.range:
        query = f'repo:{args.repo} merged:{args.range} is:pr is:closed sort:updated-desc base:main'
        prs = gh.search_issues(query=f'{query}')
        for _pr in prs:
            pr = gh_repo.get_pull(_pr.number)
            prj = process_pr(pr)
            json_list.append(prj)

    if json_list and not args.dry_run:
        # Send data over to elasticsearch.
        es = Elasticsearch(
                [os.environ['ELASTICSEARCH_SERVER']],
                api_key=os.environ['ELASTICSEARCH_KEY'],
                verify_certs=False
                )

        try:
            if args.es_index:
                index = args.es_index
            else:
                index = os.environ['PR_STAT_ES_INDEX']
            bulk(es, gendata(json_list, index))
        except KeyError as e:
            print(f"Error: {e} not set.")
            print(json_list)
    if args.dry_run:
        pprint.pprint(json_list)

if __name__ == "__main__":
    main()
