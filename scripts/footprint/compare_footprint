#!/usr/bin/env python3

# SPDX-License-Identifier: Apache-2.0
"""
This script help you to compare footprint results with previous commits in git.
If you don't have a git repository, it will compare your current tree
against the last release results.
To run it you need to set up the same environment as twister.
The scripts take 2 optional args COMMIT and BASE_COMMIT, which tell the scripts
which commit to use as current commit and as base for comparing, respectively.
The script can take any SHA commit recognized for git.

COMMIT is the commit to compare against BASE_COMMIT.
 Default
 current working directory if we have changes in git tree or we don't have git.
 HEAD in any other case.
BASE_COMMIT is the commit used as base to compare results.
 Default:
  twister_last_release.csv if we don't have git tree.
  HEAD is we have changes in the working tree.
  HEAD~1 if we don't have changes and we have default COMMIT.
  COMMIT~1 if we have a valid COMMIT.

"""

import argparse
import csv
import logging
import os
import shutil
import subprocess
import tempfile

if "ZEPHYR_BASE" not in os.environ:
    logging.error("$ZEPHYR_BASE environment variable undefined.\n")
    exit(1)

logger = None
GIT_ENABLED = False
RELEASE_DATA = 'twister_last_release.csv'


def is_git_enabled():
    global GIT_ENABLED
    proc = subprocess.Popen(
        'git rev-parse --is-inside-work-tree',
        stdout=subprocess.PIPE,
        cwd=os.environ.get('ZEPHYR_BASE'),
        shell=True,
    )
    if proc.wait() != 0:
        GIT_ENABLED = False

    GIT_ENABLED = True


def init_logs():
    global logger
    log_lev = os.environ.get('LOG_LEVEL', None)
    level = logging.INFO
    if log_lev == "DEBUG":
        level = logging.DEBUG
    elif log_lev == "ERROR":
        level = logging.ERROR

    console = logging.StreamHandler()
    format = logging.Formatter('%(levelname)-8s: %(message)s')
    console.setFormatter(format)
    logger = logging.getLogger('')
    logger.addHandler(console)
    logger.setLevel(level)

    logging.debug("Log init completed")


def parse_args():
    parser = argparse.ArgumentParser(
        description="Compare footprint apps RAM and ROM sizes. Note: "
        "To run it you need to set up the same environment as twister.",
        allow_abbrev=False,
    )
    parser.add_argument(
        '-b',
        '--base-commit',
        default=None,
        help="Commit ID to use as base for footprint "
        "compare. Default is parent current commit."
        " or twister_last_release.csv if we don't have git.",
    )
    parser.add_argument(
        '-c',
        '--commit',
        default=None,
        help="Commit ID to use compare footprint against base. Default is HEAD or working tree.",
    )
    return parser.parse_args()


def get_git_commit(commit):
    commit_id = None
    proc = subprocess.Popen(
        f'git rev-parse {commit}',
        stdout=subprocess.PIPE,
        cwd=os.environ.get('ZEPHYR_BASE'),
        shell=True,
    )
    if proc.wait() == 0:
        commit_id = proc.stdout.read().decode("utf-8").strip()
    return commit_id


def sanity_results_filename(commit=None, cwd=None):
    if cwd is None:
        cwd = os.environ.get('ZEPHYR_BASE')

    if not commit:
        file_name = "tmp.csv"
    else:
        if commit == RELEASE_DATA:
            file_name = RELEASE_DATA
        else:
            file_name = f"{commit}.csv"

    return os.path.join(cwd, 'scripts', 'sanity_chk', file_name)


def git_reset(commit, cwd=None):
    if cwd is None:
        cwd = os.environ.get('ZEPHYR_BASE')

    proc = subprocess.Popen(
        'git diff --quiet', stdout=subprocess.PIPE, stderr=subprocess.STDOUT, cwd=cwd, shell=True
    )
    if proc.wait() != 0:
        raise Exception("Cannot continue, you have unstaged changes in your working tree")

    proc = subprocess.Popen(
        f'git reset {commit} --hard',
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        cwd=cwd,
        shell=True,
    )
    if proc.wait() == 0:
        return True
    else:
        logger.error(proc.stdout.read())
    return False


def run_sanity_footprint(commit=None, cwd=None, output_file=None):
    if cwd is None:
        cwd = os.environ.get('ZEPHYR_BASE')

    if not output_file:
        output_file = sanity_results_filename(commit)
    cmd = '/bin/bash -c "source ./zephyr-env.sh && twister'
    cmd += f' +scripts/sanity_chk/sanity_compare.args -o {output_file}"'
    logger.debug(f'Sanity ({commit})   {cmd}')

    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, cwd=cwd, shell=True)
    output, _ = proc.communicate()
    if proc.wait() == 0:
        logger.debug(output)
        return True

    logger.error(f"Couldn't build footprint apps in commit {commit}")
    logger.error(output)
    raise Exception(f"Couldn't build footprint apps in commit {commit}")


def run_footprint_build(commit=None):
    logging.debug(f"footprint build for {commit}")
    if not commit:
        run_sanity_footprint()
    else:
        cmd = f"git clone --no-hardlinks {os.environ.get('ZEPHYR_BASE')}"
        tmp_location = os.path.join(
            tempfile.gettempdir(), os.path.basename(os.environ.get('ZEPHYR_BASE'))
        )
        if os.path.exists(tmp_location):
            shutil.rmtree(tmp_location)
        logging.debug(f"cloning into {tmp_location}")
        proc = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            cwd=tempfile.gettempdir(),
            shell=True,
        )
        if proc.wait() == 0:
            if git_reset(commit, tmp_location):
                run_sanity_footprint(commit, tmp_location)
        else:
            logger.error(proc.stdout.read())
        shutil.rmtree(tmp_location, ignore_errors=True)
    return True


def read_sanity_report(filename):
    data = []
    with open(filename) as fp:
        tmp = csv.DictReader(fp)
        for row in tmp:
            data.append(row)
    return data


def get_footprint_results(commit=None):
    sanity_file = sanity_results_filename(commit)
    if (not os.path.exists(sanity_file) or not commit) and commit != RELEASE_DATA:
        run_footprint_build(commit)

    return read_sanity_report(sanity_file)


def tree_changes():
    proc = subprocess.Popen(
        'git diff --quiet', stdout=subprocess.PIPE, cwd=os.environ.get('ZEPHYR_BASE'), shell=True
    )
    return proc.wait() != 0


def get_default_current_commit():
    if tree_changes():
        return None
    else:
        return get_git_commit('HEAD')


def get_default_base_commit(current_commit):
    if not current_commit:
        if tree_changes():
            return get_git_commit('HEAD')
        else:
            return get_git_commit('HEAD~1')
    else:
        return get_git_commit(f'{current_commit}~1')


def build_history(b_commit=None, c_commit=None):
    if not GIT_ENABLED:
        logger.info('Working on current tree, not git enabled.')
        current_commit = None
        base_commit = RELEASE_DATA
    else:
        if not c_commit:
            current_commit = get_default_current_commit()
        else:
            current_commit = get_git_commit(c_commit)

        if not b_commit:
            base_commit = get_default_base_commit(current_commit)
        else:
            base_commit = get_git_commit(b_commit)

    if not base_commit:
        logger.error("Cannot resolve base commit")
        return

    logger.info(f"Base:    {base_commit}")
    logger.info(f"Current: {current_commit if current_commit else 'working space'}")

    current_results = get_footprint_results(current_commit)
    base_results = get_footprint_results(base_commit)
    deltas = compare_results(base_results, current_results)
    print_deltas(deltas)


def compare_results(base_results, current_results):
    interesting_metrics = [("ram_size", int), ("rom_size", int)]
    results = {}
    metrics = {}

    for type, data in {'base': base_results, 'current': current_results}.items():
        metrics[type] = {}
        for row in data:
            d = {}
            for m, mtype in interesting_metrics:
                if row[m]:
                    d[m] = mtype(row[m])
            if row["test"] not in metrics[type]:
                metrics[type][row["test"]] = {}
            metrics[type][row["test"]][row["platform"]] = d

    for test, platforms in metrics['current'].items():
        if test not in metrics['base']:
            continue
        tests = {}

        for platform, test_data in platforms.items():
            if platform not in metrics['base'][test]:
                continue
            golden_metric = metrics['base'][test][platform]
            tmp = {}
            for metric, _ in interesting_metrics:
                if metric not in golden_metric or metric not in test_data:
                    continue
                if test_data[metric] == "":
                    continue
                delta = test_data[metric] - golden_metric[metric]
                if delta == 0:
                    continue
                tmp[metric] = {
                    'delta': delta,
                    'current': test_data[metric],
                }

            if tmp:
                tests[platform] = tmp

        if tests:
            results[test] = tests

    return results


def print_deltas(deltas):
    error_count = 0
    for test in sorted(deltas):
        print(f"\n{test:<25}")
        for platform, data in deltas[test].items():
            print(f" {platform:<25}")
            for metric, value in data.items():
                percentage = float(value['delta']) / float(value['current'] - value['delta'])
                print(
                    "  {} ({:+.2%}) {:+6}   current size {:>7} bytes".format(
                        "RAM" if metric == "ram_size" else "ROM",
                        percentage,
                        value['delta'],
                        value['current'],
                    )
                )
                error_count = error_count + 1
    if error_count == 0:
        print("There are no changes in RAM neither in ROM of footprint apps.")
    return error_count


def main():
    args = parse_args()
    build_history(args.base_commit, args.commit)


if __name__ == "__main__":
    init_logs()
    is_git_enabled()
    main()
