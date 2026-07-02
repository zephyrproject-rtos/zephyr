# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import hashlib
import shutil
import subprocess
import sys
from collections import defaultdict
from pathlib import Path, PurePath, PurePosixPath

from west.commands import WestCommand

LINK_DESCRIPTION = '''\
west ln extension command.

west ln can be used to link files which must be kept in sync.
When one file is updated then 'west ln --sync' will update all linked files to the same content.

A git pre-commit hook can be installed to automatically sync linked files on commit.
'''


WEST_LN_FILE = PurePath('.westlinks')


class Ln(WestCommand):
    def __init__(self):
        super().__init__(
            'ln',
            # Keep this in sync with the string in west-commands.yml.
            'west link handling',
            LINK_DESCRIPTION,
            accepts_unknown_args=False,
        )

    def do_add_parser(self, parser_adder):
        usage = (
            'west ln ['
            '--status [<commit>[...<commit>]] | '
            '--sync | '
            '--duplicates TARGET | '
            '[--force] TARGET LINK_NAME]'
        )
        parser = parser_adder.add_parser(
            self.name,
            help=self.help,
            usage=usage,
            formatter_class=argparse.RawDescriptionHelpFormatter,
            description=self.description,
        )

        group = parser.add_mutually_exclusive_group()
        group.add_argument(
            '-d', '--delete', action='store_true', help='''Remove link association for LINK_NAME.'''
        )
        group.add_argument(
            '--duplicates', action='store_true', help='''Find duplicate files in current repo.'''
        )
        group.add_argument(
            '--status',
            nargs='?',
            default=False,
            const=True,
            dest='status',
            metavar='<commit>[...<commit>]',
            help='''Show current status of links out of sync.''',
        )
        group.add_argument(
            '--sync',
            action='store_true',
            help='''Sync links for targets in git stage area, if TARGET is given only links shared
                    with TARGET will be synced.''',
        )
        group.add_argument(
            '--hook',
            action='store_true',
            help='''Show git pre-commit hook for automatic link syncing.
                    The hook can easily be installed by redirecting the output to
                    '.git/hooks/pre-commmit', for example like:
                      'west ln --hook >> <repo>/.git/hooks/pre-commit'
                    ''',
        )

        group = parser.add_argument_group('west ln options')
        group.add_argument(
            '-f',
            '--force',
            action="store_true",
            help='''force creating link to target even if link name already exists and points to
                    other target''',
        )

        parser.add_argument(
            'target',
            metavar='TARGET',
            nargs='?',
            help='''link target''',
        )

        parser.add_argument(
            'linkname',
            metavar='LINK_NAME',
            nargs='?',
            help='''link name''',
        )

        return parser

    def _git_run(self, repo, command, args):
        cp = subprocess.run(
            ['git', command, *args], capture_output=True, text=True, check=True, cwd=repo
        )
        return cp.stdout.splitlines()

    def repo_path(self, file, repo):
        try:
            rel_path = file.resolve().relative_to(repo)
        except ValueError:
            sys.exit(f"{file} is not inside repository {repo}")

        return PurePosixPath(rel_path)

    def delete(self, args, repo, links):
        if not args.target:
            sys.exit("west ln: error: the following arguments is required: LINKNAME")

        # We are working with the link name, but because first position argument to `west ln` is
        # target, then that's the argument name we fetch in this case.
        linkname_path = Path(args.target)
        linkname = str(self.repo_path(linkname_path, repo))

        if linkname not in links:
            sys.exit(f"Link '{linkname}' not found.")
        else:
            del links[linkname]

        with open(repo / WEST_LN_FILE, 'w', encoding="utf-8") as fp:
            for name, md5 in links.items():
                fp.write(f"{name} {md5}\n")

    def duplicates(self, args, repo, links):
        if not args.target:
            sys.exit("west ln: error: the following arguments is required: TARGET")

        target_path = Path(args.target)
        target = str(self.repo_path(target_path, repo))

        if not target_path.is_file():
            sys.exit(f"File {args.target} not found.")

        links_inv = defaultdict(list)
        for k, v in links.items():
            links_inv[v].append(k)
        linked = links_inv[links.get(target)]

        duplicates = []
        target_id = self._git_run(repo, 'hash-object', [target]).pop()
        tree_objects = self._git_run(repo, 'ls-tree', ['-r', 'HEAD'])

        for line in tree_objects:
            meta, filename = line.split("\t", 1)
            _, _, object_id = meta.split()

            if object_id == target_id:
                duplicates.append(filename)

        print("Duplicate files, *=linked, -=not linked")
        for file in duplicates:
            if file in linked:
                print(f"* {file}")
            else:
                print(f"- {file}")

    def hook(self, args, repo, links):
        if args.target:
            sys.exit(f"west ln: error: unexpected arguments: ['{args.target}']")

        file_path = Path(__file__).parent / "west_ln.pre-commit-hook"

        with open(file_path) as file:
            print(file.read())

    def link(self, args, repo, links):
        if not args.target or not args.linkname:
            sys.exit("west ln: error: the following arguments are required: TARGET, LINK_NAME")

        target_path = Path(args.target)
        linkname_path = Path(args.linkname)
        target = str(self.repo_path(target_path, repo))
        linkname = str(self.repo_path(linkname_path, repo))

        if target in links and linkname not in links:
            links[linkname] = links.get(target)
            md5 = links.get(linkname)
        elif linkname in links and not args.force:
            if target not in links or links.get(target) != links.get(linkname):
                sys.exit(f"Link '{linkname}' already exists. Use --force to update target.")
        elif target in links:
            links[linkname] = links.get(target)
            md5 = links.get(linkname)
        else:
            md5 = hashlib.md5(target.encode("utf-8")).hexdigest()
            links[linkname] = md5
            links[target] = md5

        with open(repo / WEST_LN_FILE, 'w', encoding="utf-8") as fp:
            for name, md5 in links.items():
                fp.write(f"{name} {md5}\n")

        if not linkname_path.is_file():
            shutil.copyfile(target_path, linkname_path)

        print(f"Link {linkname} --> {target} created, remember to run 'git add .westlinks'")

    def get_staged_links(self, repo, links):
        files = self._git_run(repo, 'diff', ['--cached', '--name-only'])
        return list(set(files) & set(links.keys()))

    def get_committed_links(self, repo, commit_range, links):
        files = self._git_run(repo, 'diff', [commit_range, '--name-only'])
        return list(set(files) & set(links.keys()))

    def status(self, args, repo, links):
        if args.target:
            sys.exit(f"west ln: error: unexpected arguments: ['{args.target}']")

        if args.status is True:
            files = self.get_staged_links(repo, links)
        else:
            commit_range = args.status
            files = self.get_committed_links(repo, commit_range, links)

        links_inv = defaultdict(list)
        for k, v in links.items():
            links_inv[v].append(k)

        state = {}
        for file in files:
            if file in state:
                continue

            linked = links_inv[links[file]]
            registered_linked_files = list(set(files) & set(linked))
            other_linked_files = list(set(files) ^ set(linked))

            result = set(self._git_run(repo, 'hash-object', registered_linked_files))
            if len(result) > 1:
                for registered_file in registered_linked_files:
                    state[registered_file] = '!'
                for other_file in other_linked_files:
                    state[other_file] = '?'
            else:
                for registered_file in registered_linked_files:
                    state[registered_file] = '*'

                ref_sha = next(iter(result))
                result = self._git_run(repo, 'hash-object', other_linked_files)
                for other_file, sha in zip(other_linked_files, result, strict=True):
                    state[other_file] = '+' if sha == ref_sha else '-'

        print(
            f"Linked files status, *=synced ({'staged' if args.status is True else 'committed'}), "
            "+=synced, -=differ, !=conflicts, ?=undefined"
        )
        for file, s in state.items():
            print(f"{s}: {file}")

    def sync(self, args, repo, links):
        if args.target:
            # Not yet supported.
            sys.exit(f"west ln: error: unexpected arguments: ['{args.target}']")

        if args.linkname:
            sys.exit(f"west ln: error: unexpected arguments: ['{args.linkname}']")

        files = self.get_staged_links(repo, links)

        links_inv = defaultdict(list)
        for k, v in links.items():
            links_inv[v].append(k)

        synced = []
        for file in files:
            if file in synced:
                continue

            linked = links_inv[links[file]]
            staged_linked_files = list(set(files) & set(linked))
            other_linked_files = list(set(files) ^ set(linked))

            result = set(self._git_run(repo, 'hash-object', staged_linked_files))
            if len(result) > 1:
                sys.exit(f"Sync failed, conflicting files staged: {staged_linked_files}")
            else:
                ref_sha = next(iter(result))
                result = self._git_run(repo, 'hash-object', other_linked_files)
                for other_file, sha in zip(other_linked_files, result, strict=True):
                    if sha != ref_sha:
                        shutil.copyfile(repo / file, repo / other_file)

                self._git_run(repo, 'add', other_linked_files)
            synced.extend(linked)

    def do_run(self, args, unknown_args):
        try:
            cp = subprocess.run(
                ['git', 'rev-parse', '--show-toplevel'],
                capture_output=True,
                text=True,
                check=True,
            )

        except subprocess.CalledProcessError as e:
            print('west ln failed, cannot determine git top-level folder')
            print("exit code:", e.returncode)
            print("stderr:", e.stderr)
        toplevel_path = PurePath(cp.stdout.rstrip())

        links = {}
        try:
            linkfile = toplevel_path / WEST_LN_FILE
            with open(linkfile, encoding="utf-8") as fp:
                for lineno, fp_line in enumerate(fp, start=1):
                    line = fp_line.rstrip("\n")

                    if line.lstrip().startswith("#"):
                        continue
                    if line == "":
                        continue

                    try:
                        name, md5 = line.split()
                    except ValueError:
                        sys.exit(f"west ln: error: invalid line {lineno} in {linkfile}, aborting")
                    links[name] = md5
        except FileNotFoundError:
            # Missing file is ok, simply means there are no links created.
            pass

        if args.delete:
            self.delete(args, toplevel_path, links)
        elif args.duplicates:
            self.duplicates(args, toplevel_path, links)
        elif args.hook:
            self.hook(args, toplevel_path, links)
        elif args.status is not False:
            self.status(args, toplevel_path, links)
        elif args.sync:
            self.sync(args, toplevel_path, links)
        else:
            self.link(args, toplevel_path, links)
