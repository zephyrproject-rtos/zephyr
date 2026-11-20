#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""
Tests for results.py: the results document (zephyr-results)
"""

import copy
import importlib.util
import json
import os
import re
from pathlib import Path
from unittest import mock

import jsonschema
import pytest
from twisterlib import results
from twisterlib.environment import add_parse_arguments, parse_arguments
from twisterlib.reports import Reporting

from . import ZEPHYR_BASE

DATA = Path(ZEPHYR_BASE) / 'scripts' / 'tests' / 'twister' / 'test_data' / 'results'
VALID = sorted((DATA / 'valid').glob('*.json'))
INVALID = sorted((DATA / 'invalid').glob('*.json'))


def apply_ops(doc, ops):
    """Patch a copy of doc: ["set", path, value], ["del", path], ["fill", path, char, count]."""
    doc = copy.deepcopy(doc)
    for op in ops:
        *parent, last = op[1]
        node = doc
        for key in parent:
            node = node[key]
        if op[0] == 'del':
            del node[last]
        elif op[0] == 'fill':
            node[last] = op[2] * op[3]
        else:
            node[last] = op[2]
    return doc


def invalid_case(path):
    case = json.loads(path.read_text())
    if 'raw' in case:
        return case['raw'].encode()
    base = json.loads((DATA / 'valid' / case['base']).read_text())
    return json.dumps(apply_ops(base, case['ops'])).encode()


def rules(errors):
    return {m for line in errors for m in re.findall(r': \[([a-z0-9.-]+)\]', line)}


def load_twister(name):
    return json.loads((DATA / 'twister' / name).read_text())


def context(**kw):
    args = {
        'origin': 'examplelab',
        'run': '1234',
        'job': 'hil',
        'commit': '0123456789abcdef0123456789abcdef01234567',
        'repository': 'https://git.example/examplelab/zephyr.git',
        'lab': 'lab1',
    }
    args.update(kw)
    return results.Context(**args)


@pytest.mark.parametrize('path', VALID, ids=[p.name for p in VALID])
def test_valid_documents(path):
    assert results.validate(path.read_bytes()) == []


@pytest.mark.parametrize('path', INVALID, ids=[p.stem for p in INVALID])
def test_invalid_documents(path):
    rule = path.stem.split('--')[0]
    errors = results.validate(invalid_case(path))
    assert rule in rules(errors), errors


def test_document_size():
    head = (DATA / 'valid' / 'hil-two-boards.json').read_text().rstrip().rstrip('}')
    raw = (head + ', "pad": "' + 'x' * results.MAX_DOC_BYTES + '"}\n').encode()
    assert rules(results.validate(raw)) == {'document-size'}


@pytest.mark.parametrize(
    'raw, path',
    [
        (
            'time_conversions[conversions/ms_to_cyc_floor32]',
            'time_conversions_conversions_ms_to_cyc_floor32_',
        ),
        ('test_os[main]', 'test_os_main_'),
        ('libraries.spsc_pbuf.nocache.', 'libraries.spsc_pbuf.nocache._'),
        (
            'debug.coredump.backends.in_memory.coredump_4***',
            'debug.coredump.backends.in_memory.coredump_4___',
        ),
        ('a[x.y].b', 'a_x_y_.b'),
        ('kernel.timer', 'kernel.timer'),
    ],
)
def test_sanitize(raw, path):
    assert results.sanitize(raw) == path


def test_convert_hil():
    doc = results.convert(load_twister('hil.json'), context(), created='2026-11-17T05:02:59Z')
    assert results.validate_document(doc) == []
    tests = {t['id']: t for t in doc['tests']}
    prefix = 'examplelab:1234:hil'

    ok = tests[f'{prefix}:board_a/soc_a/cpu0:kernel.timer@lab1']
    assert ok['status'] == 'PASS'
    assert ok['environment']['misc']['zephyr']['dut']['board_tag'] == 'board_a-0001'
    assert ok['misc']['zephyr']['artifact']['name'] == 'zephyr.elf'
    assert ok['misc']['zephyr']['counts'] == {'PASS': 2, 'SKIP': 1}
    case = tests[
        f'{prefix}:board_a/soc_a/cpu0:kernel.timer.time_conversions_conversions_ms_to_cyc_floor32_@lab1'
    ]
    assert (
        case['misc']['zephyr']['original_id']
        == 'kernel.timer.time_conversions[conversions/ms_to_cyc_floor32]'
    )

    flash = tests[f'{prefix}:board_b@2.0.0/soc_b/cpu0:kernel.timer@lab1']
    assert flash['status'] == 'MISS'
    assert flash['misc']['zephyr']['failure_class']['reason'] == 'flash'
    assert flash['misc']['zephyr']['counts'] == {'MISS': 2}

    serial = tests[f'{prefix}:board_a/soc_a/cpu0:sample.basic.helloworld@lab1']
    assert serial['status'] == 'MISS'
    assert serial['misc']['zephyr']['failure_class']['reason'] == 'console'

    other = tests[f'{prefix}:board_b@2.0.0/soc_b/cpu0:sample.basic.helloworld@lab1']
    assert other['status'] == 'FAIL'
    assert other['log_excerpt'].endswith('o World!\r\n')

    # filtered suites and suites without build artifacts are left out
    assert not [t for t in doc['tests'] if t['path'] in ('kernel.common', 'kernel.mem')]
    assert len(doc['builds']) == 4
    build = next(b for b in doc['builds'] if b['config_name'] == 'board_b@2.0.0/soc_b/cpu0')
    assert build['misc']['zephyr']['target'] == {
        'id': 'board_b@2.0.0/soc_b/cpu0',
        'board': 'board_b',
        'revision': '2.0.0',
        'soc': 'soc_b',
        'cpucluster': 'cpu0',
        'arch': 'arm',
    }
    assert doc['submission']['summary'] == {
        'status': 'FAIL',
        'suites': {'FAIL': 1, 'MISS': 2, 'PASS': 1},
        'testcases': {'MISS': 2, 'PASS': 2, 'SKIP': 1},
    }


def test_convert_flash_file():
    doc = results.convert(load_twister('hil.json'), context(), flash_file='zephyr.hex')
    assert results.validate_document(doc) == []
    for build in doc['builds']:
        flashed = [a['name'] for a in build['misc']['zephyr']['artifacts'] if a.get('flash')]
        assert flashed == ['zephyr.hex']


def test_convert_lab_events():
    tj = load_twister('hil.json')
    sha = tj['testsuites'][0]['artifacts'][0]['sha256']
    events = {
        'tool': {'name': 'labtool', 'version': '1.2.0'},
        'suites': [
            {'platform': 'board_a/soc_a/cpu0', 'scenario': 'kernel.timer', 'flashed_sha256': sha},
            {
                'platform': 'board_b@2.0.0/soc_b/cpu0',
                'scenario': 'sample.basic.helloworld',
                'event': {
                    'reason': 'console',
                    'id': 'job-209',
                    'url': 'https://lab.example/jobs/209',
                },
            },
            {
                'platform': 'board_a/soc_a/cpu0',
                'scenario': 'sample.basic.helloworld',
                'flashed_sha256': 'f' * 64,
            },
        ],
    }
    doc = results.convert(tj, context(), lab_events=events)
    assert results.validate_document(doc) == []
    tests = {(t['build_id'].split(':')[3], t['path']): t for t in doc['tests']}

    ok = tests[('board_a/soc_a/cpu0', 'kernel.timer')]['misc']['zephyr']
    assert ok['artifact']['flashed_sha256'] == sha
    assert ok['tools']['lab_tool'] == {'name': 'labtool', 'version': '1.2.0'}

    console = tests[('board_b@2.0.0/soc_b/cpu0', 'sample.basic.helloworld')]
    assert console['status'] == 'MISS'
    assert console['misc']['zephyr']['failure_class'] == {
        'reason': 'console',
        'source': 'lab',
        'event': {'id': 'job-209', 'url': 'https://lab.example/jobs/209'},
    }

    mismatch = tests[('board_a/soc_a/cpu0', 'sample.basic.helloworld')]
    assert mismatch['status'] == 'MISS'
    assert mismatch['misc']['zephyr']['failure_class']['reason'] == 'artifact_mismatch'
    assert mismatch['misc']['zephyr']['failure_class']['source'] == 'lab'


def test_lab_event_does_not_hide_a_pass():
    events = {
        'suites': [
            {
                'platform': 'board_a/soc_a/cpu0',
                'scenario': 'kernel.timer',
                'event': {'reason': 'power', 'id': 'job-1'},
            }
        ]
    }
    doc = results.convert(load_twister('hil.json'), context(), lab_events=events)
    suite = next(
        t for t in doc['tests'] if t['id'].endswith(':board_a/soc_a/cpu0:kernel.timer@lab1')
    )
    assert suite['status'] == 'PASS'


def test_load_lab_events_rejects_unknown_reason(tmp_path):
    path = tmp_path / 'events.json'
    path.write_text(
        json.dumps(
            {'suites': [{'platform': 'p', 'scenario': 's', 'event': {'reason': 'bad', 'id': 'x'}}]}
        )
    )
    with pytest.raises(jsonschema.ValidationError):
        results.load_lab_events(str(path))


def test_convert_build_only():
    doc = results.convert(load_twister('build_only.json'), context(lab=None, job='build'))
    assert results.validate_document(doc) == []
    assert 'lab' not in doc['submission']
    for t in doc['tests']:
        assert t['status'] == 'SKIP'
        assert t['misc']['zephyr']['skip_reason']['reason'] == 'built_only'
        assert 'environment' not in t
        assert '@' not in t['id']
    assert doc['builds'][0]['misc']['zephyr']['footprint'] == {
        'used_ram': 39122,
        'used_rom': 105632,
    }


def test_convert_path_collision():
    tj = load_twister('build_only.json')
    suite = tj['testsuites'][0]
    suite['testcases'] = [
        {'identifier': 'kernel.timer.a[x]', 'status': 'not run'},
        {'identifier': 'kernel.timer.a_x_', 'status': 'not run'},
    ]
    doc = results.convert(tj, context(lab=None))
    assert results.validate_document(doc) == []
    paths = sorted(t['path'] for t in doc['tests'] if t['misc']['zephyr']['kind'] == 'testcase')
    assert paths[0] == 'kernel.timer.a_x_'
    assert (
        paths[1].startswith('kernel.timer.a_x__') and len(paths[1]) == len('kernel.timer.a_x__') + 8
    )


def test_build_facts(tmp_path):
    sdk = tmp_path / 'sdk'
    (sdk / 'cmake').mkdir(parents=True)
    (sdk / 'sdk_version').write_text('1.0.1\n')
    build = tmp_path / 'build'
    (build / 'zephyr').mkdir(parents=True)
    (build / 'build.log').write_text('-- The C compiler identification is GNU 14.3.0\n')
    (build / 'CMakeCache.txt').write_text(f'Zephyr-sdk_DIR:PATH={sdk / "cmake"}\n')
    base = tmp_path / 'zephyr'
    (build / 'build_info.yml').write_text(
        'cmake:\n  toolchain:\n    name: zephyr\n  kconfig:\n    user-files:\n'
        f'      - {base}/samples/hello_world/prj.conf\n'
    )
    (build / 'zephyr' / 'zephyr.dts').write_text(
        '/dts-v1/;\n\n/ {\n\t#address-cells = <1>;\n\tcompatible = "vnd,board", "vnd,soc";\n'
        '\tchosen {\n\t};\n};\n'
    )
    base.mkdir()
    assert results._build_facts(str(build), str(base)) == {
        'compiler': 'GNU 14.3.0',
        'sdk_version': '1.0.1',
        'toolchain': 'zephyr',
        'config_files': ['samples/hello_world/prj.conf'],
        'compatible': ['vnd,board', 'vnd,soc'],
    }


def test_context_github():
    env = {
        'GITHUB_ACTIONS': 'true',
        'GITHUB_SERVER_URL': 'https://github.com',
        'GITHUB_REPOSITORY': 'example/zephyr',
        'GITHUB_EVENT_NAME': 'pull_request',
        'GITHUB_REF': 'refs/pull/42/merge',
        'GITHUB_HEAD_REF': 'topic',
        'GITHUB_SHA': 'a' * 40,
        'GITHUB_RUN_ID': '100',
        'GITHUB_RUN_ATTEMPT': '2',
        'GITHUB_JOB': 'hil test',
    }
    ctx = results.context_from_env('examplelab', 'lab1', environ=env)
    assert (ctx.trigger, ctx.pr, ctx.run, ctx.job) == ('pr', 42, 'gha-100-2', 'hil_test')
    assert ctx.repository == 'https://github.com/example/zephyr.git'
    assert ctx.run_url == 'https://github.com/example/zephyr/actions/runs/100'
    assert ctx.ref == 'topic'


def test_context_gitlab_and_trigger_override():
    env = {
        'GITLAB_CI': 'true',
        'CI_PIPELINE_ID': '77',
        'CI_JOB_NAME': 'hil: board',
        'CI_COMMIT_SHA': 'b' * 40,
        'CI_PROJECT_URL': 'https://gitlab.example/group/zephyr',
        'CI_PIPELINE_SOURCE': 'web',
        'CI_COMMIT_REF_NAME': 'v4.5.0',
        'CI_COMMIT_TAG': 'v4.5.0',
        'CI_PIPELINE_URL': 'https://gitlab.example/group/zephyr/-/pipelines/77',
    }
    ctx = results.context_from_env('examplelab', environ=env)
    assert (ctx.trigger, ctx.run, ctx.job, ctx.tag) == ('manual', '77', 'hil__board', True)
    ctx = results.context_from_env('examplelab', trigger='release', environ=env)
    assert ctx.trigger == 'release'
    doc = results.convert(load_twister('build_only.json'), ctx)
    assert doc['checkouts'][0]['git_commit_tags'] == ['v4.5.0']
    assert results.validate_document(doc) == []


@pytest.mark.parametrize(
    'args',
    [
        ['--report-ci'],
        ['--report-ci', '--report-ci-origin', 'Example'],
        ['--report-ci', '--report-ci-origin', 'examplelab', '--report-ci-lab', 'Lab_1'],
    ],
)
def test_report_ci_option_checks(args):
    with pytest.raises(SystemExit):
        parse_arguments(add_parse_arguments(), args)


def test_validate_cli(tmp_path, capsys):
    script = Path(ZEPHYR_BASE) / 'scripts' / 'ci' / 'test_results' / 'validate.py'
    spec = importlib.util.spec_from_file_location('validate_cli', script)
    cli = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(cli)
    bad = tmp_path / 'bad.json'
    bad.write_bytes(invalid_case(DATA / 'invalid' / 'trigger-enum.json'))
    assert cli.main([str(VALID[0])]) == 0
    assert cli.main([str(VALID[0]), str(bad)]) == 1
    out = capsys.readouterr().out
    assert f'{bad}: $.submission.context.trigger: [trigger-enum]' in out
    assert os.path.basename(str(VALID[0])) in out


@pytest.mark.parametrize(
    'env, trigger',
    [
        ({'GITHUB_ACTIONS': 'true', 'GITHUB_EVENT_NAME': 'push', 'GITHUB_SHA': 'c' * 40}, 'push'),
        ({'GITLAB_CI': 'true', 'CI_PIPELINE_SOURCE': 'push', 'CI_COMMIT_SHA': 'c' * 40}, 'push'),
    ],
    ids=['github', 'gitlab'],
)
def test_context_push_trigger(env, trigger):
    ctx = results.context_from_env('examplelab', environ=env)
    assert ctx.trigger == trigger
    ctx.repository = 'https://git.example/zephyr.git'
    doc = results.convert(load_twister('build_only.json'), ctx)
    assert doc['submission']['context']['trigger'] == 'push'
    assert results.validate_document(doc) == []


def test_invocation_strips_paths(tmp_path):
    hw_map = tmp_path / 'map.yaml'
    hw_map.write_text('[]\n')
    argv = [
        '/usr/local/bin/twister',
        '--hardware-map',
        str(hw_map),
        f'--outdir={tmp_path}/out',
        '-p',
        'board_a/soc_a/cpu0',
        '-T',
        './tests/kernel',
        '--report-ci',
    ]
    assert results.invocation(argv) == (
        'twister --hardware-map map.yaml --outdir=out -p board_a/soc_a/cpu0 -T kernel --report-ci'
    )
    assert results.invocation(['/usr/bin/python3', '-m', 'pytest']) == 'twister'


def test_no_absolute_path_in_document(tmp_path):
    tj = load_twister('build_only.json')
    suite = tj['testsuites'][0]
    build = tmp_path / 'build'
    build.mkdir()
    outside = tmp_path / 'app' / 'prj.conf'
    (build / 'build_info.yml').write_text(
        f'cmake:\n  kconfig:\n    user-files:\n      - {outside}\n'
        f'      - {ZEPHYR_BASE}/samples/hello_world/prj.conf\n'
    )
    ctx = context(lab=None, command=results.invocation(['twister', '-T', str(tmp_path)]))
    doc = results.convert(
        tj, ctx, {(suite['platform'], suite['name']): str(build)}, zephyr_base=ZEPHYR_BASE
    )
    assert results.validate_document(doc) == []
    files = doc['builds'][0]['misc']['zephyr']['config_files']
    assert files == ['prj.conf', 'samples/hello_world/prj.conf']
    text = json.dumps(doc)
    assert str(tmp_path) not in text
    assert ZEPHYR_BASE not in text


@pytest.mark.parametrize('content', [None, '{not json'], ids=['missing', 'not-json'])
def test_report_ci_lab_events_checked_at_parse(tmp_path, content):
    path = tmp_path / 'events.json'
    if content is not None:
        path.write_text(content)
    args = ['--report-ci', '--report-ci-origin', 'examplelab', '--report-ci-lab-events', str(path)]
    with pytest.raises(SystemExit) as e:
        parse_arguments(add_parse_arguments(), args)
    assert e.value.code == 1


def test_results_report_error_does_not_stop_save_reports(tmp_path, caplog):
    events = tmp_path / 'events.json'
    events.write_text('{"suites": [{"platform": "p"}]}')
    options = mock.Mock(
        report_ci=True,
        report_ci_origin='examplelab',
        report_ci_lab=None,
        report_ci_trigger=None,
        report_ci_lab_events=str(events),
        report_ci_flash_file=None,
        footprint_report=None,
    )
    report = Reporting.__new__(Reporting)
    report.env = mock.Mock(options=options, version='v4.5.0')
    report.instances = {'x': mock.Mock()}
    report.outdir = str(tmp_path)
    report.json_report = mock.Mock()
    report.json_report.create.side_effect = lambda f, **kw: Path(f).write_text(
        (DATA / 'twister' / 'build_only.json').read_text()
    )
    report.xunit_report = mock.Mock()
    with mock.patch.object(Reporting, 'target_report') as target:
        report.save_reports(None, None, None, False, True)
    target.assert_called_once()
    assert not (tmp_path / 'zephyr-results.json').exists()
    assert 'results document failed: ValidationError' in caplog.text
