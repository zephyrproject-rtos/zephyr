# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

'''Tests for "west patch apply --all" and "west patch reverse --all".

Each test uses a small west workspace with real git repositories.
'''

import argparse
import subprocess
import textwrap

import pytest
import yaml
import zephyr_module
from west.manifest import Manifest

from patch import Patch

ORIGINAL = 'line 1\nline 2\n'


class FakeConfig:
    '''Fake west configuration.'''

    def __init__(self, values):
        self._values = values

    def get(self, option, default=None):
        return self._values.get(option, default)

    def getboolean(self, option, default=False):
        # No colors, so the output can be checked
        return False


def git(cwd, *args):
    subprocess.run(['git', *args], cwd=cwd, check=True, capture_output=True)


def init_repo(path, content=ORIGINAL):
    path.mkdir(parents=True)
    git(path, 'init', '-q')
    git(path, 'config', 'user.name', 'Test')
    git(path, 'config', 'user.email', 'test@example.com')
    (path / 'file.txt').write_text(content)
    git(path, 'add', '.')
    git(path, 'commit', '-q', '-m', 'initial')


def make_patch(before, after):
    '''Return a patch that changes file.txt from before to after.'''
    old = before.splitlines(keepends=True)
    new = after.splitlines(keepends=True)
    hunk = [f'-{line}' for line in old] + [f'+{line}' for line in new]
    return (
        'diff --git a/file.txt b/file.txt\n'
        '--- a/file.txt\n'
        '+++ b/file.txt\n'
        f'@@ -1,{len(old)} +1,{len(new)} @@\n' + ''.join(hunk)
    )


def add_patches(project_path, patches):
    '''Write zephyr/patches.yml and the patch files.

    patches is a list of (module, patch) tuples.
    '''
    patch_dir = project_path / 'zephyr' / 'patches'
    patch_dir.mkdir(parents=True, exist_ok=True)

    entries = []
    for i, (module, content) in enumerate(patches):
        name = f'{i}.patch'
        (patch_dir / name).write_text(content)
        entries.append(
            {
                'path': name,
                'sha256sum': Patch.get_file_sha256sum(patch_dir / name),
                'module': module,
                'author': 'Test',
                'email': 'test@example.com',
                'date': '2026-01-01',
            }
        )

    with open(project_path / 'zephyr' / 'patches.yml', 'w') as f:
        yaml.dump({'patches': entries}, f)


@pytest.fixture
def workspace(tmp_path, monkeypatch):
    '''A west workspace with the manifest repository "app" and the projects "lib" and "mod".'''
    topdir = tmp_path / 'ws'
    for name in ['app', 'lib', 'mod']:
        init_repo(topdir / name)

    (topdir / 'app' / 'west.yml').write_text(
        textwrap.dedent('''\
        manifest:
          projects:
            - name: lib
              url: https://example.com/lib
            - name: mod
              url: https://example.com/mod
            - name: inactive
              url: https://example.com/inactive
              groups:
                - inactive
          group-filter: [-inactive]
        ''')
    )
    (topdir / '.west').mkdir()
    (topdir / '.west' / 'config').write_text('[manifest]\npath = app\nfile = west.yml\n')

    # The tests use paths, not module names
    monkeypatch.setattr(zephyr_module, 'parse_modules', lambda *args, **kwargs: [])

    return topdir


def run_patch(topdir, *argv):
    cmd = Patch()
    cmd.topdir = str(topdir)
    cmd.manifest = Manifest.from_topdir(topdir=topdir)
    cmd.config = FakeConfig({'manifest.path': 'app'})

    parser = argparse.ArgumentParser(allow_abbrev=False)
    cmd.do_add_parser(parser.add_subparsers())
    args = parser.parse_args(['patch', *argv])

    cmd.do_run(args, [])


def content(topdir, project):
    return (topdir / project / 'file.txt').read_text()


def test_apply_all_applies_manifest_repository_last(workspace):
    patched_by_lib = 'line 1\nline 2 from lib\n'
    patched_by_app = 'line 1\nline 2 from app\n'

    # The patch of "app" only works on top of the one of "lib"
    add_patches(workspace / 'lib', [('mod', make_patch(ORIGINAL, patched_by_lib))])
    add_patches(workspace / 'app', [('mod', make_patch(patched_by_lib, patched_by_app))])

    run_patch(workspace, 'apply', '--all')
    assert content(workspace, 'mod') == patched_by_app

    run_patch(workspace, 'reverse', '--all')
    assert content(workspace, 'mod') == ORIGINAL


def test_apply_all_roll_back_cleans_every_west_project(workspace):
    init_repo(workspace / 'mod2')
    add_patches(workspace / 'lib', [('mod', make_patch(ORIGINAL, 'patched\n'))])
    add_patches(workspace / 'app', [('mod2', make_patch('does not match\n', 'patched\n'))])

    with pytest.raises(SystemExit):
        run_patch(workspace, 'apply', '--all', '--roll-back')

    assert content(workspace, 'mod') == ORIGINAL
    assert content(workspace, 'mod2') == ORIGINAL


def test_apply_all_without_roll_back_keeps_applied_patches(workspace):
    init_repo(workspace / 'mod2')
    add_patches(workspace / 'lib', [('mod', make_patch(ORIGINAL, 'patched\n'))])
    add_patches(workspace / 'app', [('mod2', make_patch('does not match\n', 'patched\n'))])

    with pytest.raises(SystemExit):
        run_patch(workspace, 'apply', '--all')

    assert content(workspace, 'mod') == 'patched\n'


@pytest.mark.parametrize('module', ['app', 'app/sub'])
def test_apply_all_skips_west_projects_patching_the_manifest_repository(workspace, capsys, module):
    (workspace / 'app' / 'sub').mkdir()
    add_patches(workspace / 'lib', [(module, make_patch(ORIGINAL, 'patched\n'))])

    run_patch(workspace, 'apply', '--all')

    assert content(workspace, 'app') == ORIGINAL
    assert 'skipping 1 patches for the manifest repository' in capsys.readouterr().err


def test_apply_all_skips_inactive_west_projects(workspace):
    init_repo(workspace / 'inactive')
    add_patches(workspace / 'inactive', [('mod', make_patch(ORIGINAL, 'patched\n'))])

    run_patch(workspace, 'apply', '--all')

    assert content(workspace, 'mod') == ORIGINAL


def test_apply_all_allows_manifest_repository_to_patch_itself(workspace):
    add_patches(workspace / 'app', [('app', make_patch(ORIGINAL, 'patched\n'))])

    run_patch(workspace, 'apply', '--all')

    assert content(workspace, 'app') == 'patched\n'


def test_apply_all_reports_modules_not_found(workspace, capsys):
    add_patches(
        workspace / 'lib',
        [
            ('mod', make_patch(ORIGINAL, 'patched\n')),
            ('missing', make_patch(ORIGINAL, 'patched\n')),
        ],
    )

    run_patch(workspace, 'apply', '--all')

    assert content(workspace, 'mod') == 'patched\n'
    assert 'skipping 1 patches, module not found: missing' in capsys.readouterr().err


@pytest.mark.parametrize('subcommand', ['apply', 'reverse'])
def test_all_cannot_be_combined_with_src_module(workspace, capsys, subcommand):
    with pytest.raises(SystemExit):
        run_patch(workspace, '-sm', 'lib', subcommand, '--all')

    assert f"cannot be used with '{subcommand} --all'" in capsys.readouterr().err


def test_apply_all_without_patches(workspace):
    run_patch(workspace, 'apply', '--all')

    assert all(content(workspace, p) == ORIGINAL for p in ['app', 'lib', 'mod'])
