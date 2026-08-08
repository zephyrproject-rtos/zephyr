#!/usr/bin/env python3

# Copyright 2025 Google LLC
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import sys
import time

import github
import urllib3

DNM_LABELS = ["DNM", "DNM (manifest)", "TSC", "Architecture Review", "dev-review"]

# Workflows that label the pull request, and so have to complete before the
# labels can be trusted. Keyed by workflow file name rather than display name,
# which is stable across renames.
WAIT_FOR_WORKFLOWS = ["manifest.yml"]

# Events that can be followed by a labelling workflow. A label event carries the
# final label state already, so there is nothing to wait for.
WAIT_FOR_ACTIONS = {"opened", "reopened", "synchronize"}

WAIT_POLL_S = 30
WAIT_TIMEOUT_S = 600


def parse_args(argv):
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )

    parser.add_argument("-p", "--pull-request", required=True, type=int, help="The PR number")
    parser.add_argument("-o", "--org", default="zephyrproject-rtos", help="Github organization")
    parser.add_argument("-r", "--repo", default="zephyr", help="Github repository")
    parser.add_argument("-a", "--action", default="", help="The pull_request event action")
    parser.add_argument(
        "-t",
        "--wait-timeout",
        type=int,
        default=WAIT_TIMEOUT_S,
        help="Seconds to wait for the labelling workflows before giving up",
    )

    return parser.parse_args(argv)


def workflow_delay(repo, pr, action, timeout):
    """Wait for the workflows that label the PR to complete.

    Returns once they are done, or once the timeout expires. Not completing is
    not fatal: the workflow reruns on 'labeled' and 'unlabeled', so a label
    applied after this point re-evaluates the check.
    """
    if action and action not in WAIT_FOR_ACTIONS:
        print(f"event action is {action!r}, labels are already settled, not waiting")
        return

    sha = pr.head.sha
    print(f"PR is at {sha}")

    pending = {}
    for name in WAIT_FOR_WORKFLOWS:
        try:
            pending[name] = repo.get_workflow(name)
        except github.GithubException as e:
            # The workflow does not exist on this base branch, so it will never
            # run and there is nothing to wait for.
            print(f"{name}: not available on this branch ({e.status}), not waiting")

    deadline = time.monotonic() + timeout

    while pending:
        for name, workflow in list(pending.items()):
            runs = workflow.get_runs(head_sha=sha)

            if runs.totalCount == 0:
                # No run has been created. This is normal for a few seconds
                # after a push, but permanent for PRs opened by GITHUB_TOKEN
                # (backport bot), which do not trigger pull_request_target
                # workflows at all. The timeout below is what bounds this.
                print(f"{name}: no run for {sha} yet")
                continue

            run = runs[0]
            print(f"{name}: {run.status} {run.conclusion} {run.html_url}")
            if run.status == "completed":
                del pending[name]

        if not pending:
            return

        remaining = deadline - time.monotonic()
        if remaining <= 0:
            print(
                f"timed out after {timeout}s waiting for {sorted(pending)}, "
                "checking the labels as they are now"
            )
            return

        # Re-read the PR: a new push supersedes this run, and the concurrency
        # group cancels it, but do not keep polling a stale sha in the meantime.
        pr.update()
        if pr.head.sha != sha:
            print(f"PR moved to {pr.head.sha}, this run is superseded")
            return

        time.sleep(min(WAIT_POLL_S, remaining))


def main(argv):
    args = parse_args(argv)

    auth = github.Auth.Token(os.environ.get('GITHUB_TOKEN', None))
    # Retry on the transient 5xx and on secondary rate limits rather than
    # failing the check, which is a required check and would block the PR.
    retry = urllib3.Retry(total=5, backoff_factor=2, status_forcelist=[403, 500, 502, 503, 504])
    gh = github.Github(auth=auth, retry=retry)

    repo = gh.get_repo(f"{args.org}/{args.repo}")
    pr = repo.get_pull(args.pull_request)

    workflow_delay(repo, pr, args.action, args.wait_timeout)

    print(f"pr: {pr.html_url}")

    fail = False

    for label in pr.get_labels():
        print(f"label: {label.name}")

        if label.name in DNM_LABELS or label.name.startswith("block:"):
            print(f"Pull request is labeled as \"{label.name}\".")
            fail = True

    if not pr.body:
        print("Pull request is description is empty.")
        fail = True

    if fail:
        print("This workflow fails so that the pull request cannot be merged.")
        sys.exit(1)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
