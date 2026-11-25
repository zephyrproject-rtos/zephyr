# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Tests for scripts/ci/check_issue.py, run against the in-tree issue templates."""

import sys
from pathlib import Path

import pytest

ZEPHYR_BASE = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ZEPHYR_BASE / "scripts" / "ci"))

import check_issue  # noqa: E402

IMPACT = "Annoyance – Minor irritation; no significant impact on usability or functionality."

BUG_BODY = f"""### Describe the bug

The sample hangs at boot.

### Regression

- [ ] This is a regression.

### Steps to reproduce

_No response_

### Relevant log output

```shell
# Not a heading
### Impact
```

### Impact

{IMPACT}

### Environment

_No response_

### Additional Context

_No response_
"""


@pytest.fixture(scope="module")
def templates():
    return check_issue.load_templates(check_issue.TEMPLATE_DIR)


def make_issue(body, issue_type=None, labels=()):
    return {
        "number": 1,
        "body": body,
        "type": {"name": issue_type} if issue_type else None,
        "labels": [{"name": label} for label in labels],
    }


def test_templates_load(templates):
    assert {t.type for t in templates} >= {"Bug", "Enhancement", "Feature", "RFC"}
    for template in templates:
        assert template.fields, template.path


def test_bug_form_passes(templates):
    template, problems = check_issue.check_issue(make_issue(BUG_BODY, "Bug"), templates)
    assert template.type == "Bug"
    assert problems == []


def test_missing_type(templates):
    template, problems = check_issue.check_issue(make_issue(BUG_BODY), templates)
    assert template.type == "Bug"
    assert len(problems) == 1
    assert "no type" in problems[0]


def test_unknown_type(templates):
    _, problems = check_issue.check_issue(make_issue(BUG_BODY, "Question"), templates)
    assert len(problems) == 1
    assert "not a valid issue type" in problems[0]


def test_retyped_issue_keeps_its_template(templates):
    template, problems = check_issue.check_issue(make_issue(BUG_BODY, "Enhancement"), templates)
    assert template.type == "Bug"
    assert problems == []


def test_empty_required_field(templates):
    body = BUG_BODY.replace("The sample hangs at boot.", "_No response_")
    _, problems = check_issue.check_issue(make_issue(body, "Bug"), templates)
    assert problems == ['The "Describe the bug" section is empty.']


def test_heading_in_code_block_is_ignored(templates):
    body = BUG_BODY.replace(f"### Impact\n\n{IMPACT}\n", "")
    _, problems = check_issue.check_issue(make_issue(body, "Bug"), templates)
    assert problems == ['The "Impact" section is missing.']


def test_invalid_dropdown_value(templates):
    body = BUG_BODY.replace(IMPACT, "It is bad")
    _, problems = check_issue.check_issue(make_issue(body, "Bug"), templates)
    assert len(problems) == 1
    assert problems[0].startswith('The "Impact" section must start with one of:')


@pytest.mark.parametrize("impact", ["Major", "Annoyance - it is only cosmetic.\n\nMore text."])
def test_shortened_dropdown_value(templates, impact):
    body = BUG_BODY.replace(IMPACT, impact)
    _, problems = check_issue.check_issue(make_issue(body, "Bug"), templates)
    assert problems == []


def test_free_form_description(templates):
    issue = make_issue("It does not work, please fix.", "Bug")
    template, problems = check_issue.check_issue(issue, templates)
    assert template.type == "Bug"
    assert 'The "Describe the bug" section is missing.' in problems
    assert 'The "Impact" section is missing.' in problems


def test_free_form_description_without_type(templates):
    template, problems = check_issue.check_issue(make_issue(None), templates)
    assert template is None
    assert len(problems) == 2


def test_hand_written_headings(templates):
    body = f"**Describe the bug:** It hangs.\n\n**Environment**\nLinux\n\n## impact\n{IMPACT}\n"
    _, problems = check_issue.check_issue(make_issue(body, "Bug"), templates)
    assert problems == []


def test_type_without_generic_template(templates):
    issue = make_issue("Track the release work.", "Task")
    template, problems = check_issue.check_issue(issue, templates)
    assert template is None
    assert problems == []


def test_template_without_type(templates):
    untyped = [t for t in templates if t.type is None]
    if not untyped:
        pytest.skip("every template declares a type")

    body = "".join(f"### {f.label}\n\nSomething\n\n" for f in untyped[0].fields)
    template, problems = check_issue.check_issue(make_issue(body), templates)
    assert template is untyped[0]
    assert problems == []


def test_report(templates):
    issue = make_issue("It does not work.", "Bug")
    template, problems = check_issue.check_issue(issue, templates)
    report = check_issue.format_report(issue, template, problems, "org", "repo")
    assert "https://github.com/org/repo/issues/new/choose" in report
    assert '- The "Impact" section is missing.' in report


@pytest.mark.parametrize(
    "user, start",
    [
        ({"login": "reporter", "type": "User"}, "@reporter this issue"),
        ({"login": "github-actions[bot]", "type": "Bot"}, "This issue"),
        (None, "This issue"),
    ],
)
def test_report_mentions_reporter(templates, user, start):
    issue = make_issue("It does not work.", "Bug")
    issue["user"] = user
    template, problems = check_issue.check_issue(issue, templates)
    report = check_issue.format_report(issue, template, problems, "org", "repo")
    assert report.startswith(start)
