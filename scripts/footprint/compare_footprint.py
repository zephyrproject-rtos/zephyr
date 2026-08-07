#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""
This script help you to compare footprint results with previous commits in git.
It leverages twister to output size reports and then compares the results.

COMMIT is the commit to compare against BASE_COMMIT.
 Default
 current working directory if we have changes in git tree.
 HEAD in any other case.
BASE_COMMIT is the commit used as base to compare results.
 Default:
  HEAD is we have changes in the working tree.
  HEAD~1 if we don't have changes and we have default COMMIT.
  COMMIT~1 if we have a valid COMMIT.

To compare only specific tests, use -F to supply a testplan for twister.
You can generate testplans using twister's --save-tests option.
"""

import argparse
import hashlib
import json
import logging
import os
import shutil
import subprocess
import tempfile

if "ZEPHYR_BASE" not in os.environ:
    logging.error("$ZEPHYR_BASE environment variable undefined.\n")
    exit(1)

ZEPHYR_BASE = os.environ['ZEPHYR_BASE']
DEFAULT_TESTPLAN = os.path.join(ZEPHYR_BASE, 'scripts', 'footprint', 'testplan.json')
PLAN_STAMP_FILE = 'testplan.sha256'

logger = None


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


def plan_fingerprint(testplan_file):
    """Return a short digest of a twister testplan, or None if there is no plan.

    The digest is stored next to the twister reports so that a cached run can be
    detected as stale when the testplan it was produced with has changed.
    """
    if not testplan_file:
        return None

    with open(testplan_file, 'rb') as fp:
        return hashlib.sha256(fp.read()).hexdigest()


def write_plan_fingerprint(directory, fingerprint):
    """Record the testplan digest used for a set of twister reports."""
    if not fingerprint:
        return

    stamp_file = os.path.join(directory, PLAN_STAMP_FILE)
    with open(stamp_file, 'w') as fp:
        fp.write(fingerprint + '\n')


def read_plan_fingerprint(directory):
    """Return the testplan digest recorded for a set of twister reports."""
    stamp_file = os.path.join(directory, PLAN_STAMP_FILE)
    try:
        with open(stamp_file) as fp:
            return fp.read().strip()
    except FileNotFoundError:
        return None


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
        help="Commit ID to use as base for footprint compare. Default is parent current commit.",
    )
    parser.add_argument(
        '-c',
        '--commit',
        default=None,
        help="Commit ID to use compare footprint against base. Default is HEAD or working tree.",
    )
    parser.add_argument(
        "-F",
        "--load-tests",
        default=DEFAULT_TESTPLAN,
        help="Path to a twister testplan. "
        "Default: scripts/footprint/testplan.json in $ZEPHYR_BASE.",
    )
    parser.add_argument(
        '-o',
        '--output-dir',
        default=None,
        help="Directory to place twister reports and, with --keep-artifacts, "
        "build output for each commit. Results are stored in "
        "<output_dir>/<SHA_BASE> and <output_dir>/<SHA_COMMIT>. If not "
        "provided, output goes under a 'zephyr-footprint-compare' "
        "directory in the system temporary directory (e.g. /tmp).",
    )
    parser.add_argument(
        '--keep-artifacts',
        action='store_true',
        help="Preserve twister's per-test build trees (object files, "
        "assembly, ELF, maps, etc.) under "
        "<results_dir>/<SHA>/twister-out, where <results_dir> is "
        "either --output-dir or a 'zephyr-footprint-compare' "
        "directory in the system temporary directory. Without "
        "this flag, build trees go to a per-commit directory that is "
        "removed after the reports have been read.",
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


def twister_results_dir(commit=None, output_base=None):
    if not commit:
        dir_name = "tmp"
    else:
        dir_name = commit

    if output_base:
        return os.path.join(output_base, dir_name)

    return os.path.join(tempfile.gettempdir(), 'zephyr-footprint-compare', dir_name)


def run_twister_footprint(
    commit=None,
    cwd=None,
    output_dir=None,
    testplan_file=None,
    output_base=None,
    keep_artifacts=False,
):
    if cwd is None:
        cwd = ZEPHYR_BASE
    if not output_dir:
        output_dir = twister_results_dir(commit, output_base=output_base)

    # -o / --report-dir is where twister writes its JSON/XML reports.
    # -O / --outdir is where twister writes per-test build trees (the
    # 'twister-out' directory). It is always set explicitly: otherwise twister
    # writes to 'twister-out' in the current working directory, which for the
    # working-tree build is $ZEPHYR_BASE itself. Build trees are kept next to
    # the reports only when --keep-artifacts is given.
    build_outdir = os.path.join(output_dir, 'twister-out')
    cmd = [
        'west',
        'twister',
        '-b',
        '--enable-size-report',
        '--clobber-output',
        '-o',
        output_dir,
        '-O',
        build_outdir,
    ]
    if testplan_file:
        cmd += ['--load-tests', testplan_file]
    logger.debug(f'Twister ({commit})   {" ".join(cmd)}')

    new_env = os.environ.copy()
    new_env["ZEPHYR_BASE"] = cwd
    proc = None
    try:
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, cwd=cwd, env=new_env, text=True)
        output, _ = proc.communicate()
    finally:
        if not keep_artifacts:
            shutil.rmtree(build_outdir, ignore_errors=True)

    if proc is not None and proc.returncode == 0:
        logger.debug(output)
        return True

    logger.error(f"Couldn't build footprint apps in commit {commit}")
    logger.error(output)
    raise Exception(f"Couldn't build footprint apps in commit {commit}")


def run_footprint_build(commit=None, testplan_file=None, output_base=None, keep_artifacts=False):
    logging.debug(f"footprint build for {commit}")
    if not commit:
        run_twister_footprint(
            testplan_file=testplan_file, output_base=output_base, keep_artifacts=keep_artifacts
        )
        return True

    # Use `git worktree` instead of a full clone:
    # We place the worktree as a sibling of $ZEPHYR_BASE so it stays inside
    # the same west workspace.
    zephyr_base = os.environ.get('ZEPHYR_BASE')
    worktree_path = os.path.join(os.path.dirname(zephyr_base), f'zephyr-footprint-{commit}')

    # Drop any previous worktree, but don't touch normal directories
    if os.path.exists(worktree_path):
        r = subprocess.run(
            ['git', 'worktree', 'remove', '--force', worktree_path],
            cwd=zephyr_base,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
        )
        if r.returncode != 0:
            # `git worktree remove` may refuse (e.g. worktree unknown to git)
            raise Exception(f"Couldn't remove a worktree unknown to git: {worktree_path}")

    logging.debug(f"creating worktree at {worktree_path}")
    proc = subprocess.Popen(
        ['git', 'worktree', 'add', '--detach', worktree_path, commit],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        cwd=zephyr_base,
        text=True,
    )
    output, _ = proc.communicate()
    if proc.wait() != 0:
        logger.error(output)
        raise Exception(f"Couldn't create worktree for commit {commit}")

    try:
        run_twister_footprint(
            commit,
            worktree_path,
            testplan_file=testplan_file,
            output_base=output_base,
            keep_artifacts=keep_artifacts,
        )
    finally:
        subprocess.run(
            ['git', 'worktree', 'remove', '--force', worktree_path],
            cwd=zephyr_base,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
        )
        shutil.rmtree(worktree_path, ignore_errors=True)
    return True


def read_twister_report(directory):
    """Read test results from twister.json in the output directory."""
    twister_json = os.path.join(directory, 'twister.json')
    data = []
    with open(twister_json) as fp:
        report = json.load(fp)
        for ts in report.get('testsuites', []):
            # Only include testsuites that have footprint data
            if ts.get('used_ram') is not None or ts.get('used_rom') is not None:
                data.append(
                    {
                        'test': ts['name'],
                        'platform': ts['platform'],
                        'used_ram': str(ts.get('used_ram', '')),
                        'used_rom': str(ts.get('used_rom', '')),
                    }
                )
    return data


def get_footprint_results(commit=None, testplan_file=None, output_base=None, keep_artifacts=False):
    results_dir = twister_results_dir(commit, output_base=output_base)
    twister_json = os.path.join(results_dir, 'twister.json')
    fingerprint = plan_fingerprint(testplan_file)

    rebuild = not os.path.exists(twister_json) or not commit
    if not rebuild:
        cached_fingerprint = read_plan_fingerprint(results_dir)
        if cached_fingerprint != fingerprint:
            logger.info(
                f"{results_dir}: cached results for {commit} were produced "
                f"with a different testplan, rebuilding"
            )
            rebuild = True

    if rebuild:
        run_footprint_build(
            commit,
            testplan_file=testplan_file,
            output_base=output_base,
            keep_artifacts=keep_artifacts,
        )
        write_plan_fingerprint(results_dir, fingerprint)

    return read_twister_report(results_dir)


def tree_changes():
    # --quiet alone only reports unstaged changes, so a staged-only edit would
    # look like a clean tree and be compared against HEAD~1 instead of being
    # built from the working tree.
    proc = subprocess.Popen(['git', 'diff', '--quiet', 'HEAD'], cwd=os.environ.get('ZEPHYR_BASE'))
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


def build_history(b_commit=None, c_commit=None, plan=None, output_base=None, keep_artifacts=False):
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

    if plan:
        # If we don't provide an absolute path,
        # twister will look for the testplan relative
        # to ZEPHYR_BASE. Not what we want.
        plan = os.path.abspath(plan)
        logger.info(f"Using testplan: {plan}")

    if output_base:
        output_base = os.path.abspath(output_base)
        os.makedirs(output_base, exist_ok=True)
        logger.info(f"Output dir: {output_base}")

    if keep_artifacts:
        logger.info("Keeping per-test build artifacts under <results_dir>/<SHA>/twister-out")

    current_results = get_footprint_results(
        current_commit, testplan_file=plan, output_base=output_base, keep_artifacts=keep_artifacts
    )
    base_results = get_footprint_results(
        base_commit, testplan_file=plan, output_base=output_base, keep_artifacts=keep_artifacts
    )
    deltas = compare_results(base_results, current_results)
    print_deltas(deltas)


def compare_results(base_results, current_results):
    interesting_metrics = [("used_ram", int), ("used_rom", int)]
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
                base_size = test_data[metric] - delta
                # A missing baseline (e.g. the test did not build for the base
                # commit) or an empty baseline has no meaningful percentage.
                percentage = float(delta) / float(base_size) if base_size else None
                tmp[metric] = {
                    'delta': delta,
                    'current': test_data[metric],
                    'percentage': percentage,
                }

            if tmp:
                tests[platform] = tmp

        if tests:
            results[test] = tests

    return results


def print_deltas(deltas):
    changed = {}
    for test in sorted(deltas):
        changed_platforms = {}
        for platform, data in deltas[test].items():
            metrics = {m: v for m, v in data.items() if v['delta'] != 0}
            if metrics:
                changed_platforms[platform] = metrics

        if not changed_platforms:
            continue

        changed[test] = changed_platforms

        print(f"\n{test:<25}")
        for platform, data in changed_platforms.items():
            print(f" {platform:<25}")
            for metric, value in data.items():
                print(
                    f"  {'RAM' if metric == 'used_ram' else 'ROM'} "
                    f"({format_percentage(value['percentage'])}) {value['delta']:+6}   "
                    f"current size {value['current']:>7} bytes"
                )

    if not changed:
        print("There are no changes in RAM neither in ROM of footprint apps.")
        return

    # Summarise only the entries that actually changed, otherwise the reported
    # range is pinned to 0%/+0 bytes by the unchanged tests.
    print("\nSummary of footprint changes:")
    for metric, label in (("used_rom", "ROM"), ("used_ram", "RAM")):
        entries = [
            t[metric] for platforms in changed.values() for t in platforms.values() if metric in t
        ]
        if not entries:
            continue

        deltas_ = [e['delta'] for e in entries]
        percentages_ = [e['percentage'] for e in entries if e['percentage'] is not None]
        if percentages_:
            lo, hi = format_percentage(min(percentages_)), format_percentage(max(percentages_))
            print(f"\t{label}: [{lo}, {hi}] [{min(deltas_):+6}, {max(deltas_):+6}] bytes")
        else:
            print(f"\t{label}: [{min(deltas_):+6}, {max(deltas_):+6}] bytes")


def format_percentage(percentage):
    """Format a footprint change percentage, or mark it as not applicable."""
    if percentage is None:
        return "n/a"
    return f"{percentage:+.2%}"


def main():
    args = parse_args()
    output_base = os.path.abspath(args.output_dir) if args.output_dir else None
    build_history(args.base_commit, args.commit, args.load_tests, output_base, args.keep_artifacts)


if __name__ == "__main__":
    init_logs()
    main()
