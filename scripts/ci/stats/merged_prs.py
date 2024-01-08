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


date_format = '%Y-%m-%d %H:%M:%S'

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter, allow_abbrev=False)

    parser.add_argument('--pull-request', required=True, help='pull request number', type=int)
    parser.add_argument('--repo', required=True, help='github repo')

    return parser.parse_args()

def gendata(data, index):
    for t in data:
        yield {
                "_index": index,
                "_source": t
                }

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

        reviews = pr.get_reviews()
        print(f'#{pr.number}: {pr.title} - {pr.comments} Comments, reviews: {reviews.totalCount}, {len(pr.assignees)} Assignees (Updated {pr.updated_at})')
        assignee_reviews = 0
        reviewers = set()
        prj = {}
        for r in reviews:
            if r.user and r.state == 'APPROVED':
                reviewers.add(r.user.login)
            if pr.assignees and r.user:
                for assignee in pr.assignees:
                    if r.user.login == assignee.login:
                        assignee_reviews = assignee_reviews + 1
                        # was reviewed at least by one assignee
                        prj['reviewed_by_assignee'] = "yes"

        # This is all data we get easily though the Github API and serves as the basis
        # for displaying some trends and metrics.
        # Data can be extended in the future if we find more information that
        # is useful through the API

        prj['nr'] = pr.number
        prj['url'] = pr.url
        prj['title'] = pr.title
        prj['comments'] = pr.comments
        prj['reviews'] = reviews.totalCount
        prj['assignees'] = len(pr.assignees)
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

        ll = []
        for l in pr.labels:
            ll.append(l.name)
        prj['labels'] = ll

        # take first assignee, otherwise we have no assignees and this rule is not applicable
        if pr.assignee:
            prj['assignee'] = pr.assignee.login
        else:
            prj['assignee'] = "none"
            prj['reviewed_by_assignee'] = "na"
            prj['review_rule'] = "na"

        # go through all assignees and check if anyone has approved and reset assignee to the one who approved
        for assignee in pr.assignees:
            if assignee.login in reviewers:
                prj['assignee'] = assignee.login
            elif assignee.login == pr.user.login:
                prj['reviewed_by_assignee'] = "yes"


        # list assignees for later checks
        assignees = [a.login for a in pr.assignees]

        # Deal with exceptions when assignee approval is not needed.
        if 'Trivial' in ll or 'Hotfix' in ll:
            prj['review_rule'] = "yes"
        elif pr.merged_by.login in assignees:
            prj['review_rule'] = "yes"
        else:
            prj['review_rule'] = "no"

        prj['assignee_reviews'] = assignee_reviews

        delta = pr.closed_at - pr.created_at
        deltah = delta.total_seconds() / 3600
        prj['hours_open'] = deltah

        dates = (pr.created_at + timedelta(idx + 1) for idx in range((pr.closed_at - pr.created_at).days))

        # Get number of business days per the guidelines, we need at least 2.
        res = sum(1 for day in dates if day.weekday() < 5)

        if res < 2 and not ('Trivial' in ll or 'Hotfix' in ll):
            prj['time_rule'] = False
        elif deltah < 4 and 'Trivial' in ll:
            prj['time_rule'] = False
        else:
            prj['time_rule'] = True
        prj['reviewers'] = list(reviewers)

        json_list.append(prj)


    # Send data over to elasticsearch.
    es = Elasticsearch(
            [os.environ['ELASTICSEARCH_SERVER']],
            api_key=os.environ['ELASTICSEARCH_KEY'],
            verify_certs=False
            )

    try:
        index = os.environ['PR_STAT_INDEX']
        bulk(es, gendata(json_list, index))
    except KeyError as e:
        print(f"Error: {e} not set.")
        print(json_list)

if __name__ == "__main__":
    main()
