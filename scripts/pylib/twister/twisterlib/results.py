# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""
Results document for hardware test results ("zephyr-results").

convert() builds the document from a twister.json report plus a CI context,
validate() checks a document against results-schema.yaml and the rules that
a schema cannot express (ids, digests, roll-up, counts).
"""

from __future__ import annotations

import collections
import copy
import dataclasses
import glob
import hashlib
import json
import logging
import os
import re
import shlex
import subprocess
import sys
from datetime import UTC, datetime

import jsonschema
import yaml

logger = logging.getLogger('twister')

FORMAT = 'zephyr-results'
VERSION = {'major': 0, 'minor': 1}
MAX_DOC_BYTES = 16 * 1024 * 1024
TEXT_MAX = 1024
EXCERPT_MAX = 16384
PRIO = ['FAIL', 'ERROR', 'MISS', 'PASS', 'SKIP']
TRIGGERS = ['pr', 'merge_queue', 'push', 'nightly', 'rc', 'release', 'manual']
SCHEMA_FILE = os.path.join(
    os.path.dirname(os.path.abspath(__file__)),
    '..',
    '..',
    '..',
    'schemas',
    'twister',
    'results-schema.yaml',
)
FLASH_ORDER = ['zephyr/zephyr.elf', 'zephyr/zephyr.hex', 'zephyr/zephyr.bin', 'zephyr/zephyr.exe']
MISS_REASONS = [
    'reservation',
    'flash',
    'console',
    'power',
    'probe',
    'timeout',
    'lab_offline',
    'artifact',
    'harness',
    'aborted',
    'other',
]

_BAD_CHAR = re.compile(r'[^A-Za-z0-9_-]')
_BAD_ID = re.compile(r'[^A-Za-z0-9._-]')

# (twister status, reason) -> (status, failure_class.reason); reasons are set in handlers.py
_SUITE_MAP = [
    (
        'error',
        r'Device issue \((Flash error\?|Flash error|Flash timeout|Timeout)\)',
        'MISS',
        'flash',
    ),
    ('failed', r'Timeout during flashing', 'MISS', 'flash'),
    ('failed', r'Serial Device Error', 'MISS', 'console'),
    ('failed', r'Timeout', 'FAIL', 'timeout'),
    ('failed', r'Fault detected while running test', 'FAIL', 'fault'),
    ('failed', r'.*', 'FAIL', 'other'),
    ('error', r'.*', 'ERROR', 'other'),
]
_SKIP_MAP = {'skipped': 'harness_skip', 'not run': 'built_only'}

LAB_EVENTS_SCHEMA = {
    'type': 'object',
    'additionalProperties': False,
    'properties': {
        'tool': {
            'type': 'object',
            'required': ['name'],
            'additionalProperties': False,
            'properties': {
                'name': {'type': 'string', 'minLength': 1},
                'version': {'type': 'string'},
            },
        },
        'suites': {
            'type': 'array',
            'items': {
                'type': 'object',
                'required': ['platform', 'scenario'],
                'additionalProperties': False,
                'properties': {
                    'platform': {'type': 'string', 'minLength': 1},
                    'scenario': {'type': 'string', 'minLength': 1},
                    'flashed_sha256': {'type': 'string', 'pattern': '^[0-9a-f]{64}$'},
                    'event': {
                        'type': 'object',
                        'required': ['reason', 'id'],
                        'additionalProperties': False,
                        'properties': {
                            'reason': {'enum': MISS_REASONS},
                            'id': {'type': 'string', 'pattern': r'^\S{1,256}$'},
                            'url': {'type': 'string', 'pattern': r'^https://\S+$'},
                            'detail': {'type': 'string', 'maxLength': TEXT_MAX},
                        },
                    },
                },
            },
        },
    },
}


@dataclasses.dataclass
class Context:
    """Where and why the tests ran; most of it comes from the CI environment."""

    origin: str
    run: str
    job: str
    commit: str
    repository: str
    trigger: str = 'manual'
    lab: str | None = None
    pr: int | None = None
    ref: str | None = None
    tag: bool = False
    run_url: str | None = None
    start_time: str | None = None
    submitter: str | None = None
    command: str = 'twister'


def _safe_id(text: str) -> str:
    return _BAD_ID.sub('_', text) or '_'


def _git(args: list[str], cwd: str) -> str | None:
    try:
        out = subprocess.run(['git', *args], cwd=cwd, capture_output=True, text=True, check=True)
    except (OSError, subprocess.CalledProcessError):
        return None
    return out.stdout.strip() or None


def _https_url(url: str | None) -> str | None:
    if not url:
        return None
    m = re.fullmatch(r'(?:ssh://)?git@([^:/]+)[:/](.+)', url)
    if m:
        url = f'https://{m.group(1)}/{m.group(2)}'
    return url if url.startswith('https://') else None


def context_from_env(
    origin: str,
    lab: str | None = None,
    trigger: str | None = None,
    environ: dict | None = None,
    zephyr_base: str | None = None,
) -> Context:
    """Build the context from GitHub Actions or GitLab CI variables, else from git."""
    e = os.environ if environ is None else environ
    if e.get('GITHUB_ACTIONS') == 'true':
        server = e.get('GITHUB_SERVER_URL', 'https://github.com')
        repo = f"{server}/{e.get('GITHUB_REPOSITORY', '')}"
        event = e.get('GITHUB_EVENT_NAME', '')
        m = re.fullmatch(r'refs/pull/(\d+)/merge', e.get('GITHUB_REF', ''))
        ctx = Context(
            origin=origin,
            run=f"gha-{e.get('GITHUB_RUN_ID', '0')}-{e.get('GITHUB_RUN_ATTEMPT', '1')}",
            job=_safe_id(e.get('GITHUB_JOB', 'twister')),
            commit=e.get('GITHUB_SHA', ''),
            repository=repo + '.git',
            trigger={
                'pull_request': 'pr',
                'pull_request_target': 'pr',
                'merge_group': 'merge_queue',
                'push': 'push',
                'schedule': 'nightly',
            }.get(event, 'manual'),
            pr=int(m.group(1)) if m else None,
            ref=e.get('GITHUB_HEAD_REF') or e.get('GITHUB_REF_NAME'),
            tag=e.get('GITHUB_REF_TYPE') == 'tag',
            run_url=f"{repo}/actions/runs/{e.get('GITHUB_RUN_ID', '0')}",
        )
    elif e.get('GITLAB_CI') == 'true':
        source = e.get('CI_PIPELINE_SOURCE', '')
        ctx = Context(
            origin=origin,
            run=e.get('CI_PIPELINE_ID', '0'),
            job=_safe_id(e.get('CI_JOB_NAME', 'twister')),
            commit=e.get('CI_COMMIT_SHA', ''),
            repository=e.get('CI_PROJECT_URL', '') + '.git',
            trigger={'merge_request_event': 'pr', 'push': 'push', 'schedule': 'nightly'}.get(
                source, 'manual'
            ),
            pr=int(e['CI_MERGE_REQUEST_IID']) if source == 'merge_request_event' else None,
            ref=e.get('CI_COMMIT_REF_NAME'),
            tag=bool(e.get('CI_COMMIT_TAG')),
            run_url=e.get('CI_PIPELINE_URL'),
            start_time=e.get('CI_PIPELINE_CREATED_AT'),
        )
    else:
        base = zephyr_base or os.environ.get('ZEPHYR_BASE', '.')
        now = datetime.now(UTC)
        ctx = Context(
            origin=origin,
            run='local-' + now.strftime('%Y%m%dT%H%M%S'),
            job='twister',
            commit=_git(['rev-parse', 'HEAD'], base) or '',
            repository=_https_url(_git(['remote', 'get-url', 'origin'], base)) or '',
            ref=_git(['rev-parse', '--abbrev-ref', 'HEAD'], base),
        )
    ctx.lab = lab
    if trigger:
        ctx.trigger = trigger
    if ctx.trigger != 'pr':
        ctx.pr = None
    return ctx


def _is_path(value: str) -> bool:
    return (
        os.path.isabs(value)
        or value.startswith(('./', '../', '~'))
        or (os.sep in value and os.path.exists(value))
    )


def _strip_paths(argv: list[str]) -> list[str]:
    out = [os.path.basename(argv[0])] if argv else []
    for arg in argv[1:]:
        opt, eq, value = arg.partition('=')
        if eq and opt.startswith('-'):
            out.append(f'{opt}={os.path.basename(value) if _is_path(value) else value}')
        elif not arg.startswith('-') and _is_path(arg):
            out.append(os.path.basename(arg.rstrip('/')))
        else:
            out.append(arg)
    return out


def invocation(argv: list[str] | None = None) -> str:
    """The twister command line of this run, with path values cut to their basename."""
    argv = _strip_paths(list(sys.argv if argv is None else argv))
    if argv[:1] == ['twister'] or argv[:2] == ['west', 'twister']:
        return shlex.join(argv)
    return 'twister'


def load_lab_events(path: str) -> dict:
    """Read and check a lab events file (see doc/develop/twister/test_results.rst)."""
    with open(path, encoding='utf-8') as f:
        data = json.load(f)
    jsonschema.Draft202012Validator(LAB_EVENTS_SCHEMA).validate(data)
    return data


def sanitize(text: str) -> str:
    """Twister id -> KCIDB path: split at '.' outside [], replace other characters by '_'."""
    segs, cur, depth = [], '', 0
    for ch in text:
        if ch == '[':
            depth += 1
        elif ch == ']':
            depth = max(0, depth - 1)
        if ch == '.' and depth == 0:
            segs.append(cur)
            cur = ''
        else:
            cur += ch
    segs.append(cur)
    return '.'.join(_BAD_CHAR.sub('_', s) or '_' for s in segs)


def _suffix(original_id: str) -> str:
    return '_' + hashlib.sha256(original_id.encode()).hexdigest()[:8]


def _resolve_collisions(entries: list[list[str]]) -> None:
    by_path = collections.defaultdict(list)
    for e in entries:
        by_path[e[1]].append(e)
    for group in by_path.values():
        if len({e[0] for e in group}) < 2:
            continue
        for e in group:
            if sanitize(e[0]) != e[0]:
                e[1] += _suffix(e[0])


def highest(statuses) -> str:
    return min(statuses, key=PRIO.index)


def _clip(text, n=TEXT_MAX):
    text = (text or '').replace('\0', '')
    return text[:n] or None


def _excerpt(text):
    text = (text or '').replace('\0', '')
    return text[-EXCERPT_MAX:] or None


def _drop_none(d: dict) -> dict:
    return {k: v for k, v in d.items() if v is not None and v != {} and v != []}


def _datetime(text):
    pattern = r'\d{4}-\d\d-\d\dT\d\d:\d\d:\d\d(\.\d+)?(Z|[+-]\d\d:\d\d)'
    return text if isinstance(text, str) and re.fullmatch(pattern, text) else None


def _map_suite(status, reason):
    if status == 'passed':
        return 'PASS', None, None
    if status in _SKIP_MAP:
        skip = {'reason': _SKIP_MAP[status], 'detail': _clip(reason), 'source': 'twister'}
        return 'SKIP', None, _drop_none(skip)
    if status is None:
        return 'MISS', {'reason': 'aborted', 'source': 'twister'}, None
    for st, rx, out, code in _SUITE_MAP:
        if st == status and re.fullmatch(rx, reason or ''):
            fc = {'reason': code, 'detail': _clip(reason), 'source': 'twister'}
            return out, _drop_none(fc), None
    fc = {'reason': 'other', 'detail': _clip(f'{status}: {reason}'), 'source': 'twister'}
    return 'ERROR', fc, None


def _map_case(status, reason, suite_status):
    if status == 'passed':
        return 'PASS', None, None
    if status in _SKIP_MAP:
        skip = {'reason': _SKIP_MAP[status], 'detail': _clip(reason), 'source': 'twister'}
        return 'SKIP', None, _drop_none(skip)
    if status == 'failed':
        return 'FAIL', None, None
    if status == 'error':
        return 'ERROR', None, None
    if status == 'blocked':
        st = 'MISS' if suite_status == 'MISS' else 'ERROR'
        return st, {'reason': 'blocked', 'source': 'twister'}, None
    if status == 'started':
        return 'ERROR', {'reason': 'incomplete', 'source': 'twister'}, None
    if status is None:
        return 'MISS', {'reason': 'aborted', 'source': 'twister'}, None
    return 'ERROR', {'reason': 'other', 'detail': _clip(status), 'source': 'twister'}, None


def _target(platform: str, arch: str) -> dict:
    board, _, quals = platform.partition('/')
    board, _, rev = board.partition('@')
    q = quals.split('/') if quals else []
    return _drop_none(
        {
            'id': platform,
            'board': board,
            'revision': rev or None,
            'soc': q[0] if len(q) > 0 else None,
            'cpucluster': q[1] if len(q) > 1 else None,
            'variant': q[2] if len(q) > 2 else None,
            'arch': arch,
        }
    )


def _read(path):
    try:
        with open(path, encoding='utf-8', errors='replace') as f:
            return f.read()
    except OSError:
        return None


def _build_facts(build_dir: str | None, zephyr_base: str | None) -> dict:
    """Compiler, toolchain, SDK, config files and DT root compatible from the build dir."""
    facts = {}
    if not build_dir or not os.path.isdir(build_dir):
        return facts
    for path in glob.glob(os.path.join(build_dir, 'CMakeFiles', '*', 'CMakeCCompiler.cmake')):
        text = _read(path) or ''
        cid = re.search(r'set\(CMAKE_C_COMPILER_ID "([^"]+)"\)', text)
        ver = re.search(r'set\(CMAKE_C_COMPILER_VERSION "([^"]+)"\)', text)
        if cid and ver:
            facts['compiler'] = f'{cid.group(1)} {ver.group(1)}'
            break
    if 'compiler' not in facts:
        m = re.search(
            r'The C compiler identification is (.+)',
            _read(os.path.join(build_dir, 'build.log')) or '',
        )
        if m:
            facts['compiler'] = m.group(1).strip()
    cache = _read(os.path.join(build_dir, 'CMakeCache.txt')) or ''
    m = re.search(r'^Zephyr-sdk_DIR:PATH=(.+)$', cache, re.M)
    if m:
        sdk = _read(os.path.join(m.group(1).strip(), '..', 'sdk_version'))
        if sdk and sdk.strip():
            facts['sdk_version'] = sdk.strip()
    info = _read(os.path.join(build_dir, 'build_info.yml'))
    if info:
        try:
            bi = yaml.safe_load(info) or {}
        except yaml.YAMLError:
            bi = {}
        cmake = bi.get('cmake', {}) if isinstance(bi, dict) else {}
        facts['toolchain'] = (cmake.get('toolchain') or {}).get('name')
        base = zephyr_base or (cmake.get('zephyr') or {}).get('zephyr-base')
        files = (cmake.get('kconfig') or {}).get('user-files') or []
        base = os.path.realpath(base) if base else None
        files = [
            os.path.relpath(p, base)
            if base and os.path.realpath(p).startswith(base + os.sep)
            else os.path.basename(p)
            for p in files
        ]
        facts['config_files'] = files or None
    facts['compatible'] = _root_compatible(_read(os.path.join(build_dir, 'zephyr', 'zephyr.dts')))
    return _drop_none(facts)


def _root_compatible(dts: str | None) -> list[str] | None:
    in_root = False
    for line in (dts or '').splitlines():
        s = line.strip()
        if s == '/ {':
            in_root = True
        elif in_root and s.endswith('{'):
            return None
        elif in_root and s.startswith('compatible = '):
            return re.findall(r'"([^"]*)"', s)
    return None


def _artifacts(suite: dict, flash_file: str | None):
    files = [a['file'] for a in suite.get('artifacts') or []]
    flash = None
    if flash_file:
        flash = next((f for f in files if os.path.basename(f) == flash_file), None)
    else:
        flash = next((f for f in FLASH_ORDER if f in files), files[0] if files else None)
    arts = []
    for a in suite.get('artifacts') or []:
        item = {
            'name': a['file'].removeprefix('zephyr/').replace('/', '_'),
            'sha256': a['sha256'],
            'size': a['size'],
        }
        if a['file'] == flash:
            item['flash'] = True
        arts.append(item)
    return arts, next((a for a in arts if a.get('flash')), None)


def _apply_lab(st, fc, art, info, fail):
    """Lab facts for one suite: flashed digest, a recorded lab event."""
    if not info:
        return st, fc
    if 'flashed_sha256' in info and art:
        art['flashed_sha256'] = info['flashed_sha256']
        if info['flashed_sha256'] != art['sha256']:
            fc = {
                'reason': 'artifact_mismatch',
                'detail': 'flashed image differs from the build',
                'source': 'lab',
            }
            if info.get('event'):
                fc['event'] = _drop_none(
                    {'id': info['event']['id'], 'url': info['event'].get('url')}
                )
            return 'MISS', fc
    event = info.get('event')
    if event and fail:
        fc = _drop_none(
            {
                'reason': event['reason'],
                'detail': _clip(event.get('detail')),
                'source': 'lab',
                'event': _drop_none({'id': event['id'], 'url': event.get('url')}),
            }
        )
        return 'MISS', fc
    return st, fc


def convert(
    twister_json: dict,
    ctx: Context,
    build_dirs: dict | None = None,
    lab_events: dict | None = None,
    flash_file: str | None = None,
    zephyr_base: str | None = None,
    created: str | None = None,
) -> dict:
    """
    Build a results document from a twister.json report.

    build_dirs maps (platform, scenario) to the build directory; it is used
    for the compiler, toolchain and DT compatible. Suites without build
    artifacts (filtered, build errors, cmake only) are not reported.
    """
    env = twister_json.get('environment', {})
    o, lab = ctx.origin, ctx.lab
    at = f'@{lab}' if lab else ''
    prefix = f'{o}:{ctx.run}:{ctx.job}'
    version = env.get('zephyr_version') or 'unknown'
    events = {(s['platform'], s['scenario']): s for s in (lab_events or {}).get('suites', [])}
    lab_tool = (lab_events or {}).get('tool')
    created = created or datetime.now(UTC).strftime('%Y-%m-%dT%H:%M:%SZ')

    checkout = _drop_none(
        {
            'id': f'{o}:{ctx.run}',
            'origin': o,
            'tree_name': ctx.repository.rstrip('/').rsplit('/', 1)[-1].removesuffix('.git'),
            'git_repository_url': ctx.repository,
            'git_commit_hash': ctx.commit,
            'git_commit_name': env.get('zephyr_version'),
            'git_repository_branch': ctx.ref if ctx.ref and not ctx.tag else None,
            'git_commit_tags': [ctx.ref] if ctx.ref and ctx.tag else None,
            'start_time': _datetime(ctx.start_time),
            'misc': {
                'zephyr': _drop_none(
                    {
                        'commit_date': _datetime(env.get('commit_date')),
                        'run': _drop_none({'id': ctx.run, 'url': ctx.run_url}),
                    }
                )
            },
        }
    )
    doc = {
        'format': FORMAT,
        'version': dict(VERSION),
        'submission': _drop_none(
            {
                'id': prefix,
                'origin': o,
                'lab': lab,
                'submitter': ctx.submitter or o,
                'generated_by': {'tool': 'twister', 'version': version},
                'created': created,
                'context': _drop_none(
                    {
                        'trigger': ctx.trigger,
                        'pr': ctx.pr,
                        'commit': ctx.commit,
                        'repository': ctx.repository,
                        'ref': ctx.ref,
                        'run_url': ctx.run_url,
                    }
                ),
            }
        ),
        'checkouts': [checkout],
        'builds': [],
        'tests': [],
    }
    provenance = 'ci' if o == 'zephyr' else 'local'
    skipped = 0
    for s in sorted(twister_json.get('testsuites', []), key=lambda x: (x['platform'], x['name'])):
        plat, scen = s['platform'], s['name']
        arts, flash = _artifacts(s, flash_file)
        if s.get('status') == 'filtered' or not flash:
            skipped += 1
            continue
        facts = _build_facts((build_dirs or {}).get((plat, scen)), zephyr_base)
        bid = f'{prefix}:{plat}:{scen}'
        doc['builds'].append(
            _drop_none(
                {
                    'id': bid,
                    'origin': o,
                    'checkout_id': checkout['id'],
                    'duration': float(s['build_time']) if s.get('build_time') is not None else None,
                    'architecture': s.get('arch'),
                    'compiler': facts.get('compiler') or s.get('toolchain') or 'unknown',
                    'command': ctx.command,
                    'config_name': plat,
                    'status': 'PASS',
                    'misc': {
                        'zephyr': _drop_none(
                            {
                                'target': _target(plat, s.get('arch')),
                                'provenance': provenance,
                                'scenario': scen,
                                'artifacts': arts,
                                'toolchain': _drop_none(
                                    {
                                        'name': facts.get('toolchain')
                                        or s.get('toolchain')
                                        or 'unknown',
                                        'variant': s.get('toolchain'),
                                        'sdk_version': facts.get('sdk_version'),
                                    }
                                ),
                                'config_files': facts.get('config_files'),
                                'twister': _drop_none({'run_id': s.get('run_id')}),
                                'footprint': _drop_none(
                                    {
                                        'used_ram': s.get('used_ram') or None,
                                        'used_rom': s.get('used_rom') or None,
                                    }
                                ),
                            }
                        )
                    },
                }
            )
        )

        st, fc, skip = _map_suite(s.get('status'), s.get('reason'))
        art = {
            'name': flash['name'],
            'sha256': flash['sha256'],
            'size': flash['size'],
            'provenance': provenance,
        }
        st, fc = _apply_lab(st, fc, art, events.get((plat, scen)), st in ('FAIL', 'ERROR', 'MISS'))
        cases = s.get('testcases') or []
        if len(cases) == 1 and cases[0].get('identifier') == scen:
            cases = []
        spath = sanitize(scen)
        entries = []
        for c in cases:
            ident = c['identifier']
            rest = ident[len(scen) + 1 :] if ident.startswith(scen + '.') else ident
            entries.append([ident, spath + '.' + sanitize(rest)])
        _resolve_collisions(entries)
        mapped = [
            (c, e[0], e[1], *_map_case(c.get('status'), c.get('reason'), st))
            for c, e in zip(cases, entries, strict=True)
        ]
        if mapped:
            st = highest([st] + [m[3] for m in mapped])
        if st in ('PASS', 'SKIP'):
            fc = None
        if st != 'SKIP':
            skip = None
        sid = f'{prefix}:{plat}:{spath}{at}'
        fail = st in ('FAIL', 'ERROR', 'MISS')
        environment = None
        if lab:
            environment = _drop_none(
                {
                    'compatible': facts.get('compatible'),
                    'misc': {'zephyr': {'lab': lab, 'dut': {'board_tag': s.get('dut') or plat}}},
                }
            )
        doc['tests'].append(
            _drop_none(
                {
                    'id': sid,
                    'origin': o,
                    'build_id': bid,
                    'path': spath,
                    'status': st,
                    'comment': _clip(s.get('reason')),
                    'duration': float(s['execution_time'])
                    if s.get('execution_time') is not None
                    else None,
                    'log_excerpt': _excerpt(s.get('log')) if fail else None,
                    'environment': environment,
                    'misc': {
                        'zephyr': _drop_none(
                            {
                                'kind': 'suite',
                                'original_id': scen,
                                'failure_class': fc,
                                'skip_reason': skip,
                                'artifact': art,
                                'retries': s.get('retries'),
                                'tools': _drop_none(
                                    {
                                        'harness': {'name': 'twister', 'version': version},
                                        'lab_tool': lab_tool,
                                    }
                                ),
                            }
                        )
                    },
                }
            )
        )
        for c, ident, path, cst, cfc, cskip in mapped:
            doc['tests'].append(
                _drop_none(
                    {
                        'id': f'{prefix}:{plat}:{path}{at}',
                        'origin': o,
                        'build_id': bid,
                        'path': path,
                        'status': cst,
                        'comment': _clip(c.get('reason')),
                        'duration': (
                            float(c['execution_time'])
                            if c.get('execution_time') is not None
                            else None
                        ),
                        'misc': {
                            'zephyr': _drop_none(
                                {
                                    'kind': 'testcase',
                                    'original_id': ident,
                                    'suite_id': sid,
                                    'failure_class': cfc,
                                    'skip_reason': cskip,
                                }
                            )
                        },
                    }
                )
            )
    if skipped:
        logger.debug(
            f'{skipped} test suites without build artifacts are not in the results document'
        )
    _finish(doc)
    return doc


def _finish(doc: dict) -> None:
    tests = doc['tests']
    kids = collections.defaultdict(list)
    for t in tests:
        z = t['misc']['zephyr']
        if z['kind'] == 'testcase':
            kids[z['suite_id']].append(t['status'])
    for t in tests:
        z = t['misc']['zephyr']
        if z['kind'] == 'suite' and kids.get(t['id']):
            z['counts'] = dict(sorted(collections.Counter(kids[t['id']]).items()))
    if not tests:
        doc.pop('tests')
        return
    suites = collections.Counter(
        t['status'] for t in tests if t['misc']['zephyr']['kind'] == 'suite'
    )
    cases = collections.Counter(
        t['status'] for t in tests if t['misc']['zephyr']['kind'] == 'testcase'
    )
    summary = {
        'status': highest(list(suites) + list(cases)),
        'suites': dict(sorted(suites.items())),
    }
    if cases:
        summary['testcases'] = dict(sorted(cases.items()))
    doc['submission']['summary'] = summary


def write(doc: dict, path: str) -> None:
    with open(path, 'w', encoding='ascii', newline='\n') as f:
        f.write(json.dumps(doc, indent=2, ensure_ascii=True) + '\n')


_SCHEMA = None


def load_schema() -> dict:
    global _SCHEMA
    if _SCHEMA is None:
        with open(SCHEMA_FILE, encoding='utf-8') as f:
            _SCHEMA = yaml.safe_load(f)
    return _SCHEMA


def _jpath(parts) -> str:
    out = '$'
    for p in parts:
        out += f'[{p}]' if isinstance(p, int) else f'.{p}'
    return out


def _short(value, n=60) -> str:
    s = json.dumps(value)
    return s if len(s) <= n else s[:n] + '...'


def _resolve(root, ref):
    node = root
    for part in ref.lstrip('#/').split('/'):
        node = node[part]
    return node


def _rule_of(root, spath):
    node, rule = root, None
    try:
        for key in spath:
            while isinstance(node, dict) and key not in node and '$ref' in node:
                node = _resolve(root, node['$ref'])
            node = node[key]
            while isinstance(node, dict) and '$ref' in node and 'title' not in node:
                node = _resolve(root, node['$ref'])
            if isinstance(node, dict) and 'title' in node:
                rule = node['title']
    except (KeyError, IndexError, TypeError):
        pass
    return rule


def _schema_message(e) -> str:
    v, inst = e.validator, e.instance
    if v in ('maxLength', 'minLength'):
        return f'length {len(inst)}, {v} {e.validator_value}'
    if v == 'pattern':
        return f'{_short(inst)} does not match {e.validator_value}'
    if v == 'not':
        return f'must not match {json.dumps(e.validator_value)}'
    if v == 'anyOf':
        return 'matches none of: ' + ' | '.join(json.dumps(s) for s in e.validator_value)
    if v in ('enum', 'const'):
        return f'{_short(inst)} not in {json.dumps(e.validator_value)}'
    return e.message if len(e.message) <= 300 else e.message[:300] + '...'


class _Errors(list):
    def add(self, path, rule, msg):
        self.append(f'{path}: [{rule}] {msg}' if rule else f'{path}: {msg}')


def _check_schema(errs: _Errors, data: dict, schema: dict) -> bool:
    v = jsonschema.Draft202012Validator(schema)
    found = sorted(v.iter_errors(data), key=lambda e: [str(p) for p in e.absolute_path])
    for e in found:
        errs.add(
            _jpath(e.absolute_path), _rule_of(schema, e.absolute_schema_path), _schema_message(e)
        )
    return not found


def _path_ok(path, expected, original_id):
    return path in (expected, expected + _suffix(original_id))


def _check_digest(errs, t, art, ap):
    fc = t['misc']['zephyr'].get('failure_class', {})
    flashed = art.get('flashed_sha256') if art else None
    if t['status'] == 'MISS' and fc.get('reason') == 'artifact_mismatch':
        if flashed is None:
            errs.add(
                ap,
                'artifact-mismatch-needs-both',
                'MISS artifact_mismatch needs sha256 and flashed_sha256',
            )
        elif flashed == art['sha256']:
            errs.add(
                ap + '.flashed_sha256',
                'artifact-mismatch-equal-sha',
                'MISS artifact_mismatch but flashed_sha256 equals sha256',
            )
    elif flashed is not None and flashed != art['sha256']:
        errs.add(ap + '.flashed_sha256', 'flashed-sha-mismatch', 'differs from artifact.sha256')


def _lab_of(t):
    return t.get('environment', {}).get('misc', {}).get('zephyr', {}).get('lab')


def _at(lab):
    return f'@{lab}' if lab is not None else ''


def _check_ids(errs, d, origin, run_prefix, job_prefix):
    for kind in ('checkouts', 'builds', 'tests'):
        seen = {}
        for i, o in enumerate(d.get(kind, [])):
            p = _jpath([kind, i])
            if o['origin'] != origin:
                errs.add(
                    p + '.origin',
                    'origin-match',
                    f"{_short(o['origin'])} != submission.origin {_short(origin)}",
                )
            if not o['id'].startswith(origin + ':'):
                errs.add(p + '.id', 'id-origin-prefix', f'does not start with "{origin}:"')
            elif kind == 'checkouts' and o['id'] != run_prefix:
                errs.add(
                    p + '.id', 'id-run-job', f'checkout id != "{run_prefix}" (from submission.id)'
                )
            elif kind != 'checkouts' and not o['id'].startswith(job_prefix):
                errs.add(
                    p + '.id',
                    'id-run-job',
                    f'does not start with "{job_prefix}" (from submission.id)',
                )
            if o['id'] in seen:
                errs.add(p + '.id', 'duplicate-id', f'same id as {_jpath([kind, seen[o["id"]]])}')
            else:
                seen[o['id']] = i


def _check_suite(errs, t, p, lab, builds):
    z = t['misc']['zephyr']
    art = z.get('artifact')
    tlab = _lab_of(t)
    if tlab is not None and tlab != lab:
        errs.add(
            p + '.environment.misc.zephyr.lab',
            'lab-match',
            f'{_short(tlab)} != submission.lab {_short(lab)}',
        )
    exp = sanitize(z['original_id'])
    if not _path_ok(t['path'], exp, z['original_id']):
        errs.add(p + '.path', 'path-sanitize', f'expected {_short(exp)} from original_id')
    if z.get('failure_class', {}).get('reason') == 'blocked':
        errs.add(
            p + '.misc.zephyr.failure_class.reason',
            'blocked-status',
            'blocked applies to testcases only',
        )
    _check_digest(errs, t, art, p + '.misc.zephyr.artifact')
    b = builds.get(t['build_id'])
    if b is not None and art:
        bz = b['misc']['zephyr']
        fl = next(a for a in bz['artifacts'] if a.get('flash'))
        if (fl['name'], fl['sha256'], fl['size']) != (art['name'], art['sha256'], art['size']):
            errs.add(
                p + '.misc.zephyr.artifact',
                'artifact-vs-build',
                "name/sha256/size differ from the build's flash artifact",
            )
        if art['provenance'] != bz['provenance']:
            errs.add(
                p + '.misc.zephyr.artifact.provenance',
                'provenance-vs-build',
                'differs from build provenance',
            )
    if not t['id'].endswith(f":{t['path']}{_at(tlab)}"):
        errs.add(p + '.id', 'test-id-path-lab', f"does not end with \":{t['path']}{_at(tlab)}\"")


def _check_case(errs, tests, byid, i, children):
    t = tests[i]
    p = _jpath(['tests', i])
    z = t['misc']['zephyr']
    fc = z.get('failure_class', {})
    si = byid.get(z['suite_id'])
    if si is None:
        errs.add(
            p + '.misc.zephyr.suite_id', 'suite-id-dangling', 'no test with this id in the document'
        )
        return
    s = tests[si]
    sz = s['misc']['zephyr']
    if sz['kind'] != 'suite':
        errs.add(
            p + '.misc.zephyr.suite_id',
            'suite-id-not-suite',
            f"points to {_jpath(['tests', si])}, kind {sz['kind']}",
        )
        return
    children[si].append(i)
    if s['build_id'] != t['build_id']:
        errs.add(p + '.build_id', 'testcase-build-mismatch', "differs from the suite's build_id")
    slab = _lab_of(s)
    if not t['id'].endswith(f":{t['path']}{_at(slab)}"):
        errs.add(
            p + '.id',
            'test-id-path-lab',
            f"does not end with \":{t['path']}{_at(slab)}\" (lab from suite)",
        )
    oid, soid = z['original_id'], sz['original_id']
    rest = oid[len(soid) + 1 :] if oid.startswith(soid + '.') else oid
    exp = s['path'] + '.' + sanitize(rest)
    if not _path_ok(t['path'], exp, oid):
        errs.add(
            p + '.path', 'path-sanitize', f'expected {_short(exp)} from suite path + original_id'
        )
    if fc.get('reason') == 'artifact_mismatch':
        _check_digest(errs, t, sz['artifact'], _jpath(['tests', si, 'misc', 'zephyr', 'artifact']))
    if fc.get('reason') == 'blocked':
        want = 'MISS' if s['status'] == 'MISS' else 'ERROR'
        if t['status'] != want:
            errs.add(
                p + '.status',
                'blocked-status',
                f"blocked testcase must be {want} (suite is {s['status']})",
            )


def _check_semantics(errs: _Errors, d: dict) -> None:
    sub = d['submission']
    origin = sub['origin']
    sid = sub['id'].split(':')
    if sid[0] != origin:
        errs.add('$.submission.id', 'id-origin-prefix', f'does not start with "{origin}:"')
    _check_ids(errs, d, origin, f'{sid[0]}:{sid[1]}', f'{sid[0]}:{sid[1]}:{sid[2]}:')

    for i, c in enumerate(d.get('checkouts', [])):
        if c['git_commit_hash'] != sub['context']['commit']:
            errs.add(
                _jpath(['checkouts', i, 'git_commit_hash']),
                'checkout-commit-context',
                'differs from submission.context.commit',
            )

    builds = {}
    for i, b in enumerate(d.get('builds', [])):
        p = _jpath(['builds', i])
        builds.setdefault(b['id'], b)
        if b['config_name'] != b['misc']['zephyr']['target']['id']:
            errs.add(
                p + '.config_name', 'config-name-target-id', 'differs from misc.zephyr.target.id'
            )
        if not b['id'].startswith(b['checkout_id'] + ':'):
            errs.add(p + '.id', 'build-under-checkout', 'does not start with checkout_id + ":"')

    tests = d.get('tests', [])
    byid = {}
    for i, t in enumerate(tests):
        byid.setdefault(t['id'], i)
    children = collections.defaultdict(list)
    suites, cases = collections.Counter(), collections.Counter()
    for i, t in enumerate(tests):
        if t['misc']['zephyr']['kind'] == 'suite':
            suites[t['status']] += 1
            _check_suite(errs, t, _jpath(['tests', i]), sub.get('lab'), builds)
        else:
            cases[t['status']] += 1
            _check_case(errs, tests, byid, i, children)

    for si, kids in children.items():
        s = tests[si]
        top = highest([tests[k]['status'] for k in kids] + [s['status']])
        if top != s['status']:
            errs.add(
                _jpath(['tests', si]) + '.status',
                'rollup',
                f"suite {s['status']} but a testcase is {top}",
            )
    for si, s in enumerate(tests):
        cnt = s['misc']['zephyr'].get('counts')
        if s['misc']['zephyr']['kind'] != 'suite' or cnt is None:
            continue
        got = dict(collections.Counter(tests[k]['status'] for k in children.get(si, [])))
        want = {k: v for k, v in cnt.items() if v}
        if want != got:
            errs.add(
                _jpath(['tests', si, 'misc', 'zephyr', 'counts']),
                'suite-counts-mismatch',
                f'{_short(want)} != testcases in the document {_short(got)}',
            )

    summ = sub.get('summary')
    if summ is not None:
        for key, got in (('suites', suites), ('testcases', cases)):
            want = {k: v for k, v in summ.get(key, {}).items() if v}
            if want != dict(got):
                errs.add(
                    f'$.submission.summary.{key}',
                    'summary-mismatch',
                    f'{_short(want)} != counted {_short(dict(got))}',
                )
        allst = list(suites) + list(cases)
        if 'status' in summ and allst and summ['status'] != highest(allst):
            errs.add(
                '$.submission.summary.status',
                'summary-mismatch',
                f"{summ['status']} != roll-up {highest(allst)}",
            )


def _no_dup_keys(pairs):
    out = {}
    for k, v in pairs:
        if k in out:
            raise ValueError(f'duplicate key {k!r}')
        out[k] = v
    return out


def validate(raw: bytes, schema: dict | None = None) -> list[str]:
    """Check a serialized document; returns "path: [rule] message" lines, empty when valid."""
    errs = _Errors()
    if len(raw) > MAX_DOC_BYTES:
        errs.add('$', 'document-size', f'{len(raw)} bytes > {MAX_DOC_BYTES}')
        return errs
    if not raw.strip():
        errs.add('$', 'invalid-json', 'empty file')
        return errs
    try:
        data = json.loads(raw.decode('utf-8'), object_pairs_hook=_no_dup_keys)
    except UnicodeDecodeError as e:
        errs.add('$', 'invalid-json', f'not UTF-8: {e}')
        return errs
    except json.JSONDecodeError as e:
        errs.add('$', 'invalid-json', f'line {e.lineno} column {e.colno}: {e.msg}')
        return errs
    except ValueError as e:
        errs.add('$', 'invalid-json', str(e))
        return errs
    if _check_schema(errs, data, schema or load_schema()):
        _check_semantics(errs, copy.deepcopy(data))
    return errs


def validate_document(doc: dict) -> list[str]:
    return validate((json.dumps(doc, indent=2, ensure_ascii=True) + '\n').encode())
