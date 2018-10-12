# Copyright (c) 2018, Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

'''West project commands'''

import argparse
import collections
import os
import shutil
import subprocess
import textwrap

import pykwalify.core
import yaml

import log
import util
from commands import WestCommand


# Branch that points to the revision specified in the manifest (which might be
# an SHA). Local branches created with 'west branch' are set to track this
# branch.
_MANIFEST_REV_BRANCH = 'manifest-rev'


class ListProjects(WestCommand):
    def __init__(self):
        super().__init__(
            'list-projects',
            _wrap('''
            List projects.

            Prints the path to the manifest file and lists all projects along
            with their clone paths and manifest revisions. Also includes
            information on which projects are currently cloned.
            '''))

    def do_add_parser(self, parser_adder):
        return _add_parser(parser_adder, self)

    def do_run(self, args, user_args):
        log.inf("Manifest path: {}\n".format(_manifest_path(args)))

        for project in _all_projects(args):
            log.inf('{:15} {:30} {:15} {}'.format(
                project.name,
                os.path.join(project.path, ''),  # Add final '/' if missing
                project.revision,
                "(cloned)" if _cloned(project) else "(not cloned)"))


class Fetch(WestCommand):
    def __init__(self):
        super().__init__(
            'fetch',
            _wrap('''
            Clone/fetch projects.

            Fetches upstream changes in each of the specified projects
            (default: all projects). Repositories that do not already exist are
            cloned.

            Unless --no-update is passed, the manifest and West source code
            repositories are updated prior to fetching. See the 'update'
            command.

            ''' + _MANIFEST_REV_HELP))

    def do_add_parser(self, parser_adder):
        return _add_parser(parser_adder, self, _no_update_arg,
                           _project_list_arg)

    def do_run(self, args, user_args):
        if args.update:
            _update(True, True)

        for project in _projects(args, listed_must_be_cloned=False):
            log.dbg('fetching:', project, level=log.VERBOSE_VERY)
            _fetch(project)


class Pull(WestCommand):
    def __init__(self):
        super().__init__(
            'pull',
            _wrap('''
            Clone/fetch and rebase projects.

            Fetches upstream changes in each of the specified projects
            (default: all projects) and rebases the checked-out branch (or
            detached HEAD state) on top of '{}', effectively bringing the
            branch up to date. Repositories that do not already exist are
            cloned.

            Unless --no-update is passed, the manifest and West source code
            repositories are updated prior to pulling. See the 'update'
            command.

            '''.format(_MANIFEST_REV_BRANCH) + _MANIFEST_REV_HELP))

    def do_add_parser(self, parser_adder):
        return _add_parser(parser_adder, self, _no_update_arg,
                           _project_list_arg)

    def do_run(self, args, user_args):
        if args.update:
            _update(True, True)

        for project in _projects(args, listed_must_be_cloned=False):
            if _fetch(project):
                _rebase(project)


class Rebase(WestCommand):
    def __init__(self):
        super().__init__(
            'rebase',
            _wrap('''
            Rebase projects.

            Rebases the checked-out branch (or detached HEAD) on top of '{}' in
            each of the specified projects (default: all cloned projects),
            effectively bringing the branch up to date.

            '''.format(_MANIFEST_REV_BRANCH) + _MANIFEST_REV_HELP))

    def do_add_parser(self, parser_adder):
        return _add_parser(parser_adder, self, _project_list_arg)

    def do_run(self, args, user_args):
        for project in _cloned_projects(args):
            _rebase(project)


class Branch(WestCommand):
    def __init__(self):
        super().__init__(
            'branch',
            _wrap('''
            Create a branch or list branches, in multiple projects.

            Creates a branch in each of the specified projects (default: all
            cloned projects). The new branches are set to track '{}'.

            With no arguments, lists all local branches along with the
            repositories they appear in.

            '''.format(_MANIFEST_REV_BRANCH) + _MANIFEST_REV_HELP))

    def do_add_parser(self, parser_adder):
        return _add_parser(
            parser_adder, self,
            _arg('branch', nargs='?', metavar='BRANCH_NAME'),
            _project_list_arg)

    def do_run(self, args, user_args):
        if args.branch:
            # Create a branch in the specified projects
            for project in _cloned_projects(args):
                _create_branch(project, args.branch)
        else:
            # No arguments. List local branches from all cloned projects along
            # with the projects they appear in.

            branch2projs = collections.defaultdict(list)
            for project in _cloned_projects(args):
                for branch in _branches(project):
                    branch2projs[branch].append(project.name)

            for branch, projs in sorted(branch2projs.items()):
                log.inf('{:18} {}'.format(branch, ", ".join(projs)))


class Checkout(WestCommand):
    def __init__(self):
        super().__init__(
            'checkout',
            _wrap('''
            Check out topic branch.

            Checks out the specified branch in each of the specified projects
            (default: all cloned projects). Projects that do not have the
            branch are left alone.
            '''))

    def do_add_parser(self, parser_adder):
        return _add_parser(
            parser_adder, self,
            _arg('-b',
                 dest='create_branch',
                 action='store_true',
                 help='create the branch before checking it out'),
            _arg('branch', metavar='BRANCH_NAME'),
            _project_list_arg)

    def do_run(self, args, user_args):
        branch_exists = False

        for project in _cloned_projects(args):
            if args.create_branch:
                _create_branch(project, args.branch)
                _checkout(project, args.branch)
                branch_exists = True
            elif _has_branch(project, args.branch):
                _checkout(project, args.branch)
                branch_exists = True

        if not branch_exists:
            msg = 'No branch {} exists in any '.format(args.branch)
            if args.projects:
                log.die(msg + 'of the listed projects')
            else:
                log.die(msg + 'cloned project')


class Diff(WestCommand):
    def __init__(self):
        super().__init__(
            'diff',
            _wrap('''
            'git diff' projects.

            Runs 'git diff' for each of the specified projects (default: all
            cloned projects).

            Extra arguments are passed as-is to 'git diff'.
            '''),
            accepts_unknown_args=True)

    def do_add_parser(self, parser_adder):
        return _add_parser(parser_adder, self, _project_list_arg)

    def do_run(self, args, user_args):
        for project in _cloned_projects(args):
            # Use paths that are relative to the base directory to make it
            # easier to see where the changes are
            _git(project, 'diff --src-prefix=(path)/ --dst-prefix=(path)/',
                 extra_args=user_args)


class Status(WestCommand):
    def __init__(self):
        super().__init__(
            'status',
            _wrap('''
            Runs 'git status' for each of the specified projects (default: all
            cloned projects). Extra arguments are passed as-is to 'git status'.
            '''),
            accepts_unknown_args=True)

    def do_add_parser(self, parser_adder):
        return _add_parser(parser_adder, self, _project_list_arg)

    def do_run(self, args, user_args):
        for project in _cloned_projects(args):
            _inf(project, 'status of (name-and-path)')
            _git(project, 'status', extra_args=user_args)


class Update(WestCommand):
    def __init__(self):
        super().__init__(
            'update',
            _wrap('''
            Updates the manifest repository and/or the West source code
            repository.

            There is normally no need to run this command manually, because
            'west fetch' and 'west pull' automatically update the West and
            manifest repositories to the latest version before doing anything
            else.

            Pass --update-west or --update-manifest to update just that
            repository. With no arguments, both are updated.
            '''))

    def do_add_parser(self, parser_adder):
        return _add_parser(
            parser_adder, self,
            _arg('--update-west',
                 dest='update_west',
                 action='store_true',
                 help='update the West source code repository'),
            _arg('--update-manifest',
                 dest='update_manifest',
                 action='store_true',
                 help='update the manifest repository'))

    def do_run(self, args, user_args):
        if not args.update_west and not args.update_manifest:
            _update(True, True)
        else:
            _update(args.update_west, args.update_manifest)


class ForAll(WestCommand):
    def __init__(self):
        super().__init__(
            'forall',
            _wrap('''
            Runs a shell (Linux) or batch (Windows) command within the
            repository of each of the specified projects (default: all cloned
            projects). Note that you have to quote the command if it consists
            of more than one word, to prevent the shell you use to run 'west'
            from splitting it up.

            Since the command is run through the shell, you can use wildcards
            and the like.

            For example, the following command will list the contents of
            proj-1's and proj-2's repositories on Linux, in long form:

              west forall -c 'ls -l' proj-1 proj-2
            '''))

    def do_add_parser(self, parser_adder):
        return _add_parser(
            parser_adder, self,
            _arg('-c',
                 dest='command',
                 metavar='COMMAND',
                 required=True),
            _project_list_arg)

    def do_run(self, args, user_args):
        for project in _cloned_projects(args):
            _inf(project, "Running '{}' in (name-and-path)"
                          .format(args.command))

            subprocess.Popen(args.command, shell=True, cwd=project.abspath) \
                .wait()


def _arg(*args, **kwargs):
    # Helper for creating a new argument parser for a single argument,
    # later passed in parents= to add_parser()

    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument(*args, **kwargs)
    return parser


# Arguments shared between more than one command

_manifest_arg = _arg(
    '-m', '--manifest',
    help='path to manifest file (default: west/manifest/default.yml)')

# For 'fetch' and 'pull'
_no_update_arg = _arg(
    '--no-update',
    dest='update',
    action='store_false',
    help='do not update the manifest or West before fetching project data')

# List of projects
_project_list_arg = _arg('projects', metavar='PROJECT', nargs='*')


def _add_parser(parser_adder, cmd, *extra_args):
    # Adds and returns a subparser for the project-related WestCommand 'cmd'.
    # All of these commands (currently) take the manifest path flag, so it's
    # hardcoded here.

    return parser_adder.add_parser(
        cmd.name,
        description=cmd.description,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        parents=(_manifest_arg,) + extra_args)


def _wrap(s):
    # Wraps help texts for commands. Some of them have variable length (due to
    # _MANIFEST_REV_BRANCH), so just a textwrap.dedent() can look a bit wonky.

    # [1:] gets rid of the initial newline. It's turned into a space by
    # textwrap.fill() otherwise.
    paragraphs = textwrap.dedent(s[1:]).split("\n\n")

    return "\n\n".join(textwrap.fill(paragraph) for paragraph in paragraphs)


_MANIFEST_REV_HELP = """
The '{}' branch points to the revision that the manifest specified for the
project as of the most recent 'west fetch'/'west pull'.
""".format(_MANIFEST_REV_BRANCH)[1:].replace("\n", " ")


# Holds information about a project, taken from the manifest file (or
# constructed manually for "special" projects)
Project = collections.namedtuple(
    'Project',
    'name url revision path abspath clone_depth')


def _cloned_projects(args):
    # Returns _projects(args, listed_must_be_cloned=True) if a list of projects
    # was given by the user (i.e., listed projects are required to be cloned).
    # If no projects were listed, returns all cloned projects.

    # This approach avoids redundant _cloned() checks
    return _projects(args) if args.projects else \
        [project for project in _all_projects(args) if _cloned(project)]


def _projects(args, listed_must_be_cloned=True):
    # Returns a list of project instances for the projects requested in 'args'
    # (the command-line arguments), in the same order that they were listed by
    # the user. If args.projects is empty, no projects were listed, and all
    # projects will be returned. If a non-existent project was listed by the
    # user, an error is raised.
    #
    # Before the manifest is parsed, it is validated agains a pykwalify schema.
    # An error is raised on validation errors.
    #
    # listed_must_be_cloned (default: True):
    #   If True, an error is raised if an uncloned project was listed. This
    #   only applies to projects listed explicitly on the command line.

    projects = _all_projects(args)

    if not args.projects:
        # No projects specified. Return all projects.
        return projects

    # Got a list of projects on the command line. First, check that they exist
    # in the manifest.

    project_names = [project.name for project in projects]
    nonexistent = set(args.projects) - set(project_names)
    if nonexistent:
        log.die('Unknown project{} {} (available projects: {})'
                .format('s' if len(nonexistent) > 1 else '',
                        ', '.join(nonexistent),
                        ', '.join(project_names)))

    # Return the projects in the order they were listed
    res = []
    for name in args.projects:
        for project in projects:
            if project.name == name:
                res.append(project)
                break

    # Check that all listed repositories are cloned, if requested
    if listed_must_be_cloned:
        uncloned = [prj.name for prj in res if not _cloned(prj)]
        if uncloned:
            log.die('The following projects are not cloned: {}. Please clone '
                    "them first (with 'west fetch')."
                    .format(", ".join(uncloned)))

    return res


def _all_projects(args):
    # Parses the manifest file, returning a list of Project instances.
    #
    # Before the manifest is parsed, it is validated against a pykwalify
    # schema. An error is raised on validation errors.

    manifest_path = _manifest_path(args)

    _validate_manifest(manifest_path)

    with open(manifest_path) as f:
        manifest = yaml.safe_load(f)['manifest']

    projects = []
    # Manifest "defaults" keys whose values get copied to each project
    # that doesn't specify its own value.
    project_defaults = ('remote', 'revision')

    # mp = manifest project (dictionary with values parsed from the manifest)
    for mp in manifest['projects']:
        # Fill in any missing fields in 'mp' with values from the 'defaults'
        # dictionary
        if 'defaults' in manifest:
            for key, val in manifest['defaults'].items():
                if key in project_defaults:
                    mp.setdefault(key, val)

        # Add the repository URL to 'mp'
        for remote in manifest['remotes']:
            if remote['name'] == mp['remote']:
                mp['url'] = remote['url'] + '/' + mp['name']
                break
        else:
            log.die('Remote {} not defined in {}'
                    .format(mp['remote'], manifest_path))

        # If no clone path is specified, the project's name is used
        clone_path = mp.get('path', mp['name'])

        # Use named tuples to store project information. That gives nicer
        # syntax compared to a dict (project.name instead of project['name'],
        # etc.)
        projects.append(Project(
            mp['name'],
            mp['url'],
            # If no revision is specified, 'master' is used
            mp.get('revision', 'master'),
            clone_path,
            # Absolute clone path
            os.path.join(util.west_topdir(), clone_path),
            # If no clone depth is specified, we fetch the entire history
            mp.get('clone-depth', None)))

    return projects


def _validate_manifest(manifest_path):
    # Validates the manifest with pykwalify. schema.yml holds the schema.

    schema_path = os.path.join(os.path.dirname(__file__), "schema.yml")

    try:
        pykwalify.core.Core(
            source_file=manifest_path,
            schema_files=[schema_path]
        ).validate()
    except pykwalify.errors.SchemaError as e:
        log.die('{} malformed (schema: {}):\n{}'
                .format(manifest_path, schema_path, e))


def _manifest_path(args):
    # Returns the path to the manifest file. Defaults to
    # .west/manifest/default.yml if the user didn't specify a manifest.

    return args.manifest or os.path.join(util.west_dir(), 'manifest',
                                         'default.yml')


def _fetch(project):
    # Fetches upstream changes for 'project' and updates the 'manifest-rev'
    # branch to point to the revision specified in the manifest. If the
    # project's repository does not already exist, it is created first.
    #
    # Returns True if the project's repository already existed.

    exists = _cloned(project)

    if not exists:
        _inf(project, 'Creating repository for (name-and-path)')
        _git_base(project, 'init (abspath)')
        _git(project, 'remote add origin (url)')

    if project.clone_depth:
        _inf(project,
             'Fetching changes for (name-and-path) with --depth (clone-depth)')

        # If 'clone-depth' is specified, fetch just the specified revision
        # (probably a branch). That will download the minimum amount of data,
        # which is probably what's wanted whenever 'clone-depth is used. The
        # default 'git fetch' behavior is to do a shallow clone of all branches
        # on the remote.
        #
        # Note: Many servers won't allow fetching arbitrary commits by SHA.
        # Combining --depth with an SHA will break for those.

        # Qualify branch names with refs/heads/, just to be safe. Just the
        # branch name is likely to work as well though.
        _git(project,
             'fetch --depth=(clone-depth) origin ' +
                 (project.revision if _is_sha(project.revision) else \
                     'refs/heads/' + project.revision))

    else:
        _inf(project, 'Fetching changes for (name-and-path)')

        # If 'clone-depth' is not specified, fetch all branches on the
        # remote. This gives a more usable repository.
        _git(project, 'fetch origin')

    # Create/update the 'manifest-rev' branch
    _git(project,
         'update-ref refs/heads/(manifest-rev-branch) ' +
             (project.revision if _is_sha(project.revision) else
                 'remotes/origin/' + project.revision))

    if not exists:
        # If we just initialized the repository, check out 'manifest-rev' in a
        # detached HEAD state.
        #
        # Otherwise, the initial state would have nothing checked out, and HEAD
        # would point to a non-existent refs/heads/master branch (that would
        # get created if the user makes an initial commit). That state causes
        # e.g. 'west rebase' to fail, and might look confusing.
        #
        # (The --detach flag is strictly redundant here, because the
        # refs/heads/<branch> form already detaches HEAD, but it avoids a
        # spammy detached HEAD warning from Git.)
        _git(project, 'checkout --detach refs/heads/(manifest-rev-branch)')

    return exists


def _rebase(project):
    _inf(project, 'Rebasing (name-and-path) to (manifest-rev-branch)')
    _git(project, 'rebase (manifest-rev-branch)')


def _cloned(project):
    # Returns True if the project's path is a directory that looks
    # like the top-level directory of a Git repository, and False
    # otherwise.

    def handle(result):
        log.dbg('project', project.name,
                'is {}cloned'.format('' if result else 'not '),
                level=log.VERBOSE_EXTREME)
        return result

    if not os.path.isdir(project.abspath):
        return handle(False)

    # --is-inside-work-tree doesn't require that the directory is the top-level
    # directory of a Git repository. Use --show-cdup instead, which prints an
    # empty string (i.e., just a newline, which we strip) for the top-level
    # directory.
    res = _git(project, 'rev-parse --show-cdup', capture_stdout=True,
               check=False)

    return handle(not (res.returncode or res.stdout))


def _branches(project):
    # Returns a sorted list of all local branches in 'project'

    # refname:lstrip=-1 isn't available before Git 2.8 (introduced by commit
    # 'tag: do not show ambiguous tag names as "tags/foo"'). Strip
    # 'refs/heads/' manually instead.
    return [ref[len('refs/heads/'):] for ref in
            _git(project,
                 'for-each-ref --sort=refname --format=%(refname) refs/heads',
                 capture_stdout=True).stdout.split('\n')]


def _create_branch(project, branch):
    if _has_branch(project, branch):
        _inf(project, "Branch '{}' already exists in (name-and-path)"
                      .format(branch))
    else:
        _inf(project, "Creating branch '{}' in (name-and-path)"
                      .format(branch))
        _git(project, 'branch --quiet --track {} (manifest-rev-branch)'
                      .format(branch))


def _has_branch(project, branch):
    return _git(project, 'show-ref --quiet --verify refs/heads/' + branch,
                check=False).returncode == 0


def _checkout(project, branch):
    _inf(project, "Checking out branch '{}' in (name-and-path)".format(branch))
    _git(project, 'checkout ' + branch)


def _special_project(name):
    # Returns a Project instance for one of the special repositories in west/,
    # so that we can reuse the project-related functions for them

    return Project(
        name,
        'dummy URL for {} repository'.format(name),
        'master',
        os.path.join('west', name.lower()),  # Path
        os.path.join(util.west_dir(), name.lower()),  # Absolute path
        None  # Clone depth
    )


def _update(update_west, update_manifest):
    # 'try' is a keyword
    def attempt(project, cmd):
        res = _git(project, cmd, capture_stdout=True, check=False)
        if res.returncode:
            # The Git command's stderr isn't redirected and will also be
            # available
            _die(project, _FAILED_UPDATE_MSG.format(cmd))
        return res.stdout

    projects = []
    if update_west:
        projects.append(_special_project('West'))
    if update_manifest:
        projects.append(_special_project('manifest'))

    for project in projects:
        _dbg(project, 'Updating (name-and-path)', level=log.VERBOSE_NORMAL)

        # Fetch changes from upstream
        attempt(project, 'fetch')

        # Get the SHA of the last commit in common with the upstream branch
        merge_base = attempt(project, 'merge-base HEAD remotes/origin/master')

        # Get the current SHA of the upstream branch
        head_sha = attempt(project, 'show-ref --hash remotes/origin/master')

        # If they differ, we need to rebase
        if merge_base != head_sha:
            attempt(project, 'rebase remotes/origin/master')

            _inf(project, 'Updated (rebased) (name-and-path) to the '
                          'latest version')

            if project.name == 'west':
                # Signal self-update, which will cause a restart. This is a bit
                # nicer than doing the restart here, as callers will have a
                # chance to flush file buffers, etc.
                raise WestUpdated()


_FAILED_UPDATE_MSG = """
Failed to update (name-and-path), while running command '{}'. Please fix the
state of the repository, or pass --no-update to 'west fetch/pull' to skip
updating the manifest and West for the duration of the command."""[1:]


class WestUpdated(Exception):
    '''Raised after West has updated its own source code'''


def _is_sha(s):
    try:
        int(s, 16)
    except ValueError:
        return False

    return len(s) == 40


def _git_base(project, cmd, *, extra_args=(), capture_stdout=False,
              check=True):
    # Runs a git command in the West top directory. See _git_helper() for
    # parameter documentation.
    #
    # Returns a CompletedProcess instance (see below).

    return _git_helper(project, cmd, extra_args, util.west_topdir(),
                       capture_stdout, check)


def _git(project, cmd, *, extra_args=(), capture_stdout=False, check=True):
    # Runs a git command within a particular project. See _git_helper() for
    # parameter documentation.
    #
    # Returns a CompletedProcess instance (see below).

    return _git_helper(project, cmd, extra_args, project.abspath,
                       capture_stdout, check)


def _git_helper(project, cmd, extra_args, cwd, capture_stdout, check):
    # Runs a git command.
    #
    # project:
    #   The Project instance for the project, derived from the manifest file.
    #
    # cmd:
    #   String with git arguments. Supports some "(foo)" shorthands. See below.
    #
    # extra_args:
    #   List of additional arguments to pass to the git command (e.g. from the
    #   user).
    #
    # cwd:
    #   Directory to switch to first (None = current directory)
    #
    # capture_stdout:
    #   True if stdout should be captured into the returned
    #   subprocess.CompletedProcess instance instead of being printed.
    #
    #   We never capture stderr, to prevent error messages from being eaten.
    #
    # check:
    #   True if an error should be raised if the git command finishes with a
    #   non-zero return code.
    #
    # Returns a subprocess.CompletedProcess instance.

    # TODO: Run once somewhere?
    if shutil.which('git') is None:
        log.die('Git is not installed or cannot be found')

    args = (('git',) +
            tuple(_expand_shorthands(project, arg) for arg in cmd.split()) +
            tuple(extra_args))
    cmd_str = util.quote_sh_list(args)

    log.dbg("running '{}'".format(cmd_str), 'in', cwd, level=log.VERBOSE_VERY)
    popen = subprocess.Popen(
        args, stdout=subprocess.PIPE if capture_stdout else None, cwd=cwd)

    stdout, _ = popen.communicate()

    dbg_msg = "'{}' in {} finished with exit status {}" \
              .format(cmd_str, cwd, popen.returncode)
    if capture_stdout:
        dbg_msg += " and wrote {} to stdout".format(stdout)
    log.dbg(dbg_msg, level=log.VERBOSE_VERY)

    if check and popen.returncode:
        _die(project, "Command '{}' failed for (name-and-path)"
                      .format(cmd_str))

    if capture_stdout:
        # Manual UTF-8 decoding and universal newlines. Before Python 3.6,
        # Popen doesn't seem to allow using universal newlines mode (which
        # enables decoding) with a specific encoding (because the encoding=
        # parameter is missing).
        #
        # Also strip all trailing newlines as convenience. The splitlines()
        # already means we lose a final '\n' anyway.
        stdout = "\n".join(stdout.decode('utf-8').splitlines()).rstrip("\n")

    return CompletedProcess(popen.args, popen.returncode, stdout)


def _expand_shorthands(project, s):
    # Expands project-related shorthands in 's' to their values,
    # returning the expanded string

    return s.replace('(name)', project.name) \
            .replace('(name-and-path)',
                     '{} ({})'.format(
                         project.name, os.path.join(project.path, ""))) \
            .replace('(url)', project.url) \
            .replace('(path)', project.path) \
            .replace('(abspath)', project.abspath) \
            .replace('(revision)', project.revision) \
            .replace('(manifest-rev-branch)', _MANIFEST_REV_BRANCH) \
            .replace('(clone-depth)', str(project.clone_depth))


def _inf(project, msg):
    # Print '=== msg' (to clearly separate it from Git output). Supports the
    # same (foo) shorthands as the git commands.
    #
    # Prints the message in green if stdout is a terminal, to clearly separate
    # it from command (usually Git) output.

    log.inf('=== ' + _expand_shorthands(project, msg), colorize=True)


def _wrn(project, msg):
    # Warn with 'msg'. Supports the same (foo) shorthands as the git commands.

    log.wrn(_expand_shorthands(project, msg))


def _dbg(project, msg, level):
    # Like _wrn(), for debug messages

    log.dbg(_expand_shorthands(project, msg), level=level)


def _die(project, msg):
    # Like _wrn(), for dying

    log.die(_expand_shorthands(project, msg))


# subprocess.CompletedProcess-alike, used instead of the real deal for Python
# 3.4 compatibility, and with two small differences:
#
# - Trailing newlines are stripped from stdout
#
# - The 'stderr' attribute is omitted, because we never capture stderr
CompletedProcess = collections.namedtuple(
    'CompletedProcess', 'args returncode stdout')
