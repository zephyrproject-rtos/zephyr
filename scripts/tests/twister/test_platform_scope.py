# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

import textwrap
from pathlib import Path

import pytest
from twisterlib.platform import Platform
from twisterlib.testplan import TestConfiguration, TestPlan


@pytest.fixture
def platform_scope_plan(class_env, tmp_path):
    """Create test suites with overlapping platform restrictions."""
    (tmp_path / 'tests.yaml').write_text(
        textwrap.dedent("""\
        tests:
          scope.foo:
            platform_allow: foo
            tags: selected
          scope.bar:
            platform_allow: bar
          scope.both:
            platform_allow: [foo, bar]
          scope.generic: {}
          scope.excluded:
            platform_exclude: foo
          scope.integration:
            platform_allow: bar
            integration_platforms: [bar]
        """)
    )
    class_env.test_roots = [str(tmp_path)]
    class_env.options.detailed_test_id = False
    plan = TestPlan(class_env)
    for name in ('foo', 'bar'):
        platform = Platform()
        platform.name = f'{name}/native'
        platform.aliases = [name, platform.name]
        platform.normalized_name = f'{name}_native'
        platform.arch = 'posix'
        platform.type = 'native'
        platform.supported_toolchains = ['host']
        plan.platforms.append(platform)
        plan.platform_names.extend(platform.aliases)
    return plan


@pytest.mark.parametrize(
    'platforms, excluded, expected',
    [
        (['foo'], [], {'scope.foo', 'scope.both', 'scope.generic'}),
        (['foo/native'], [], {'scope.foo', 'scope.both', 'scope.generic'}),
        (
            ['bar'],
            [],
            {'scope.bar', 'scope.both', 'scope.generic', 'scope.excluded', 'scope.integration'},
        ),
        ([], ['bar'], {'scope.foo', 'scope.both', 'scope.generic'}),
        (['foo'], ['foo/native'], set()),
    ],
)
def test_listing_platform_scope(platform_scope_plan, platforms, excluded, expected):
    plan = platform_scope_plan
    plan.options.platform = platforms
    plan.options.exclude_platform = excluded
    plan.add_testsuites()

    assert set(plan.get_tests_list()) == expected


@pytest.mark.parametrize('all_platforms, selected', [(False, []), (True, []), (True, ['foo'])])
def test_missing_platforms_are_rejected_without_target_scope(
    platform_scope_plan, all_platforms, selected
):
    plan = platform_scope_plan
    plan.options.all = all_platforms
    plan.options.platform = selected
    plan.platforms = plan.platforms[:1]
    plan.platform_names = plan.platforms[0].aliases

    with pytest.raises(SystemExit) as exc:
        plan.add_testsuites()

    assert exc.value.code == 2


@pytest.fixture
def partial_workspace_plan(platform_scope_plan):
    plan = platform_scope_plan
    plan.platforms = plan.platforms[:1]
    plan.platform_names = plan.platforms[0].aliases
    plan.options.platform = ['foo']
    plan.env.toolchain = 'host/gnu'
    plan.test_config = TestConfiguration(plan.env.test_config)
    return plan


@pytest.mark.parametrize('platform', ['foo', 'foo/native'])
def test_listing_with_an_unavailable_board(partial_workspace_plan, platform):
    plan = partial_workspace_plan
    plan.options.platform = [platform]
    plan.add_testsuites()

    assert set(plan.get_tests_list()) == {'scope.foo', 'scope.both', 'scope.generic'}


def test_execution_plan_with_an_unavailable_board(partial_workspace_plan):
    plan = partial_workspace_plan
    plan.add_testsuites()
    plan.apply_filters()

    active = [instance for instance in plan.instances.values() if not instance.filters]
    assert {instance.testsuite.id for instance in active} == {
        'scope.foo',
        'scope.both',
        'scope.generic',
    }
    assert {instance.platform.name for instance in active} == {'foo/native'}


@pytest.mark.parametrize(
    'restriction, integration, active',
    [
        ('platform_allow: [foo, bar]', False, True),
        ('platform_allow: bar', False, False),
        ('platform_exclude: bar', False, True),
        ('integration_platforms: [bar]', False, True),
        ('integration_platforms: [bar]', True, False),
    ],
)
def test_unavailable_board_restrictions(partial_workspace_plan, restriction, integration, active):
    plan = partial_workspace_plan
    (Path(plan.env.test_roots[0]) / 'tests.yaml').write_text(
        f'tests:\n  scope.restricted:\n    {restriction}\n'
    )
    plan.options.integration = integration
    plan.add_testsuites()
    assert not plan.load_errors
    assert set(plan.testsuites) == {'scope.restricted'}
    plan.apply_filters()

    active_suites = {
        instance.testsuite.id for instance in plan.instances.values() if not instance.filters
    }
    assert active_suites == ({'scope.restricted'} if active else set())


@pytest.mark.parametrize('platforms', [['bar'], ['missing'], ['foo', 'bar']])
def test_requesting_an_unavailable_board_is_rejected(partial_workspace_plan, platforms):
    plan = partial_workspace_plan
    plan.options.platform = platforms

    with pytest.raises(SystemExit) as exc:
        plan.add_testsuites()

    assert exc.value.code == 2
    assert not plan.testsuites


def test_explicit_missing_platform_is_rejected(platform_scope_plan):
    plan = platform_scope_plan
    plan.options.platform = ['missing']

    with pytest.raises(SystemExit) as exc:
        plan.add_testsuites()

    assert exc.value.code == 2


def test_missing_target_is_rejected_with_no_tests(partial_workspace_plan):
    plan = partial_workspace_plan
    plan.env.test_roots = []
    plan.options.platform = ['missing']

    with pytest.raises(SystemExit) as exc:
        plan.add_testsuites()

    assert exc.value.code == 2


@pytest.mark.parametrize(
    'dependency, field',
    [
        (
            'required_applications:\n  - application: scope.provider\n    platform: bar\n',
            'required_applications.platform',
        ),
        (
            'harness_config:\n  required_devices:\n    - platform: bar\n',
            'required_devices.platform',
        ),
    ],
)
def test_unavailable_dependency_platform_is_rejected(
    partial_workspace_plan, dependency, field, caplog
):
    plan = partial_workspace_plan
    (Path(plan.env.test_roots[0]) / 'tests.yaml').write_text(
        'tests:\n  scope.dependent:\n    platform_allow: [foo, bar]\n'
        + textwrap.indent(dependency, '    ')
    )

    with pytest.raises(SystemExit) as exc:
        plan.add_testsuites()

    assert exc.value.code == 2
    assert f'{field} in scope.dependent - unrecognized platform - bar' in caplog.text


def test_listing_ignores_unknown_platform_exclusion(platform_scope_plan):
    plan = platform_scope_plan
    plan.options.platform = ['foo']
    plan.options.exclude_platform = ['missing']
    plan.add_testsuites()

    assert set(plan.get_tests_list()) == {'scope.foo', 'scope.both', 'scope.generic'}


def test_listing_preserves_tag_filters(platform_scope_plan):
    plan = platform_scope_plan
    plan.options.platform = ['foo']
    plan.options.tag = ['selected']
    plan.add_testsuites()

    assert plan.get_tests_list() == ['scope.foo']
    plan.options.exclude_tag = ['selected']
    assert plan.get_tests_list() == []


def test_listing_force_platform(platform_scope_plan):
    plan = platform_scope_plan
    plan.options.platform = ['foo']
    plan.options.force_platform = True
    plan.add_testsuites()

    assert set(plan.get_tests_list()) == set(plan.testsuites)


@pytest.mark.parametrize('excluded', [['foo'], ['foo/native'], ['missing']])
def test_listing_force_platform_overrides_command_line_exclusions(platform_scope_plan, excluded):
    plan = platform_scope_plan
    plan.options.platform = ['foo']
    plan.options.exclude_platform = excluded
    plan.options.force_platform = True
    plan.add_testsuites()

    assert set(plan.get_tests_list()) == set(plan.testsuites)


def test_listing_without_platform_filter(platform_scope_plan):
    plan = platform_scope_plan
    plan.add_testsuites()

    assert set(plan.get_tests_list()) == set(plan.testsuites)


@pytest.mark.parametrize('platform', ['foo', 'missing'])
def test_listing_all_overrides_platform_filter(platform_scope_plan, platform):
    plan = platform_scope_plan
    plan.options.all = True
    plan.options.platform = [platform]
    plan.add_testsuites()

    assert set(plan.get_tests_list()) == set(plan.testsuites)
