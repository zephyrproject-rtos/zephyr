#!/usr/bin/env python3

# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""
Check that a GitHub issue has a valid issue type and provides the information
required by the issue template it was filed with.

The issue forms in .github/ISSUE_TEMPLATE/ are the reference: the valid types
are the ones the forms declare, and every form field marked as required must
show up in the issue description as a heading followed by an answer. An issue
written by hand passes as long as it uses the same headings.

The issue is read from GitHub (-I, needs GITHUB_TOKEN) or from a JSON file
holding either an issue as returned by the REST API or an "issues" webhook
payload (-f), for example:

    gh api repos/zephyrproject-rtos/zephyr/issues/1234 > issue.json
    ./scripts/ci/check_issue.py -f issue.json
"""

import argparse
import json
import os
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

import yaml

ZEPHYR_BASE = Path(__file__).resolve().parents[2]
TEMPLATE_DIR = ZEPHYR_BASE / ".github" / "ISSUE_TEMPLATE"

# Identifies the comment owned by this script among the comments of an issue.
COMMENT_MARKER = "<!-- zephyr-issue-check -->"

# What GitHub renders for a form field that was left empty.
NO_ANSWER = {"", "_no response_", "none"}

HEADING_RE = re.compile(r"^\s{0,3}#{1,6}\s+(.*?)\s*#*\s*$")
BOLD_RE = re.compile(r"^\s*(?:\*\*|__)(.+?)(?:\*\*|__)\s*:?\s*(.*)$")
FENCE_RE = re.compile(r"^\s{0,3}(`{3,}|~{3,})")
HTML_COMMENT_RE = re.compile(r"<!--.*?-->", re.DOTALL)
OPTION_NAME_RE = re.compile(r"\s+[–—-]\s+")
CHECKED_RE = re.compile(r"^\s*[-*]\s*\[[xX]\]\s*(.*?)\s*$", re.MULTILINE)


@dataclass
class Field:
    """A form element that collects input."""

    label: str
    kind: str
    required: bool = False
    options: list[str] = field(default_factory=list)
    required_options: list[str] = field(default_factory=list)


@dataclass
class Template:
    """An issue form."""

    name: str
    path: Path
    type: str | None
    labels: set[str]
    fields: list[Field]


def normalize(text):
    """Reduce a heading or a form label to a form that survives small edits."""
    text = re.sub(r"\s+", " ", text).strip().lower()
    return text.rstrip(" :")


def load_template(path):
    with open(path, encoding="utf-8") as f:
        data = yaml.safe_load(f)

    fields = []
    for element in data.get("body", []):
        kind = element.get("type")
        attributes = element.get("attributes", {})
        if kind == "markdown" or "label" not in attributes:
            continue

        required = bool(element.get("validations", {}).get("required", False))
        options = []
        required_options = []
        for option in attributes.get("options", []):
            if isinstance(option, dict):
                options.append(str(option.get("label", "")))
                if option.get("required", False):
                    required_options.append(options[-1])
            else:
                options.append(str(option))

        fields.append(
            Field(
                label=attributes["label"],
                kind=kind,
                required=required or bool(required_options),
                options=options,
                required_options=required_options,
            )
        )

    labels = data.get("labels") or []
    if isinstance(labels, str):
        labels = [label.strip() for label in labels.split(",")]

    return Template(
        name=data.get("name", path.stem),
        path=path,
        type=data.get("type"),
        labels=set(labels),
        fields=fields,
    )


def load_templates(template_dir):
    templates = []
    for path in sorted(Path(template_dir).glob("*.y*ml")):
        if path.stem == "config":
            continue
        templates.append(load_template(path))
    return templates


def parse_sections(body, known_labels):
    """
    Split an issue description into the answers given to form fields.

    Only headings matching a form label start a section, so headings used
    inside an answer stay part of that answer. Fenced code blocks are skipped
    as shell comments in logs look like headings.
    """
    sections = {}
    current = None
    fence = None

    for line in (body or "").splitlines():
        match = FENCE_RE.match(line)
        if match:
            marker = match.group(1)
            if fence is None:
                fence = marker[0]
            elif marker[0] == fence:
                fence = None
        elif fence is None:
            match = HEADING_RE.match(line) or BOLD_RE.match(line)
            if match and normalize(match.group(1)) in known_labels:
                current = normalize(match.group(1))
                # A bold label may be followed by its answer on the same line.
                inline = match.group(2) if match.re is BOLD_RE else ""
                sections.setdefault(current, []).append(inline)
                continue

        if current is not None:
            sections[current].append(line)

    return {label: "\n".join(lines) for label, lines in sections.items()}


def answer(text):
    """Return what is left of a section once comments and padding are gone."""
    return HTML_COMMENT_RE.sub("", text).strip()


def is_option(text, options):
    """
    Tell whether an answer picks one of the options of a dropdown.

    Options of the form "Name – explanation" are commonly shortened to their
    name or given another explanation, which still answers the question.
    """
    text = normalize(text)
    for option in options:
        name = normalize(OPTION_NAME_RE.split(option, maxsplit=1)[0])
        if text.startswith(name):
            return True
    return False


def select_template(templates, issue_type, issue_labels, sections):
    """
    Find the template an issue was filed with.

    The template sharing the most headings with the description wins, the
    issue type breaks ties. A description sharing no heading with any template
    is held against the template of its type, unless that template is tied to
    labels the issue does not carry.
    """
    best = None
    best_key = (0, False)
    for template in templates:
        score = sum(1 for f in template.fields if normalize(f.label) in sections)
        key = (score, issue_type is not None and template.type == issue_type)
        if score > 0 and key > best_key:
            best, best_key = template, key

    if best is not None or issue_type is None:
        return best

    for template in templates:
        if template.type == issue_type and template.labels <= issue_labels:
            return template

    return None


def check_field(form_field, sections):
    """Return the problem with the answer to a required field, if any."""
    label = normalize(form_field.label)
    if label not in sections:
        return f'The "{form_field.label}" section is missing.'

    text = answer(sections[label])
    if text.lower() in NO_ANSWER:
        return f'The "{form_field.label}" section is empty.'

    if form_field.kind == "dropdown" and not is_option(text, form_field.options):
        choices = "\n".join(f"  - {option}" for option in form_field.options)
        return f'The "{form_field.label}" section must start with one of:\n{choices}'

    if form_field.kind == "checkboxes":
        checked = {normalize(item) for item in CHECKED_RE.findall(text)}
        for option in form_field.required_options:
            if normalize(option) not in checked:
                return f'"{option}" must be checked in the "{form_field.label}" section.'

    return None


def check_issue(issue, templates):
    """
    Check an issue, given as the dictionary the REST API returns for it.

    Returns the template the issue was checked against, or None when no
    template applies, and the list of problems found.
    """
    problems = []
    valid_types = sorted({t.type for t in templates if t.type})
    issue_type = (issue.get("type") or {}).get("name")
    issue_labels = {label["name"] for label in issue.get("labels", [])}

    known_labels = {normalize(f.label) for t in templates for f in t.fields}
    sections = parse_sections(issue.get("body"), known_labels)
    template = select_template(templates, issue_type, issue_labels, sections)

    if issue_type is None:
        # A template without a type leaves the choice to whoever triages.
        if template is None or template.type is not None:
            problems.append(f"The issue has no type. Valid types: {', '.join(valid_types)}.")
    elif issue_type not in valid_types:
        problems.append(
            f'"{issue_type}" is not a valid issue type. Valid types: {", ".join(valid_types)}.'
        )

    if template is None:
        if issue_type is None:
            problems.append("The description does not follow any of the issue templates.")
        return None, problems

    for form_field in template.fields:
        if form_field.required:
            problem = check_field(form_field, sections)
            if problem is not None:
                problems.append(problem)

    return template, problems


def format_report(issue, template, problems, org, repo):
    """Describe the outcome in Markdown, for the comment and the job summary."""
    if not problems:
        name = f'the "{template.name}" template' if template else "no template applies"
        return f"Issue #{issue.get('number')} passes the issue checks ({name})."

    url = f"https://github.com/{org}/{repo}/issues/new/choose"
    # Mention the reporter so that the comment notifies them; bots do not read it.
    user = issue.get("user") or {}
    mention = f"@{user['login']} this" if user.get("type") == "User" else "This"
    lines = [f"{mention} issue does not provide what the [issue templates]({url}) ask for:", ""]
    lines += [f"- {problem}" for problem in problems]
    lines.append("")
    if template is not None:
        lines.append(f'The description was checked against the "{template.name}" template.')
    lines.append(
        "Please edit the issue description to add the missing information; "
        "this comment goes away once the issue passes the checks."
    )
    return "\n".join(lines)


def update_comment(gh_issue, report, failed, create):
    """Keep at most one comment from this script on the issue."""
    existing = None
    for comment in gh_issue.get_comments():
        if COMMENT_MARKER in comment.body:
            existing = comment
            break

    body = f"{COMMENT_MARKER}\n{report}"
    if not failed:
        if existing is not None:
            existing.delete()
    elif existing is not None:
        if existing.body != body:
            existing.edit(body)
    elif create:
        gh_issue.create_comment(body)


def get_github_issue(args, number):
    # Deferred so that checking a file needs neither PyGithub nor a token.
    import github

    token = os.environ.get("GITHUB_TOKEN")
    if not token:
        sys.exit("GITHUB_TOKEN is not set")

    gh = github.Github(auth=github.Auth.Token(token))
    return gh.get_repo(f"{args.org}/{args.repo}").get_issue(number)


def parse_args(argv):
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )

    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("-I", "--issue", type=int, help="Number of the issue to fetch and check")
    source.add_argument(
        "-f", "--file", type=Path, help="JSON file with an issue or an issues event payload"
    )
    parser.add_argument("-o", "--org", default="zephyrproject-rtos", help="GitHub organization")
    parser.add_argument("-r", "--repo", default="zephyr", help="GitHub repository")
    parser.add_argument(
        "-t",
        "--templates",
        type=Path,
        default=TEMPLATE_DIR,
        help="Directory holding the issue templates",
    )
    parser.add_argument(
        "-c",
        "--comment",
        choices=["create", "update"],
        help="Report problems in a comment on the issue and remove that comment once the "
        "issue passes. 'update' only touches a comment that already exists.",
    )

    return parser.parse_args(argv)


def main(argv):
    args = parse_args(argv)

    templates = load_templates(args.templates)
    if not templates:
        sys.exit(f"No issue templates found in {args.templates}")

    gh_issue = None
    if args.file is not None:
        with open(args.file, encoding="utf-8") as f:
            issue = json.load(f)
        issue = issue.get("issue", issue)
    else:
        gh_issue = get_github_issue(args, args.issue)
        issue = gh_issue.raw_data

    if "pull_request" in issue:
        print(f"#{issue.get('number')} is a pull request, nothing to check")
        return 0

    template, problems = check_issue(issue, templates)
    report = format_report(issue, template, problems, args.org, args.repo)
    print(report)

    summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary:
        with open(summary, "a", encoding="utf-8") as f:
            f.write(report + "\n")

    if args.comment is not None:
        if gh_issue is None:
            gh_issue = get_github_issue(args, issue["number"])
        update_comment(gh_issue, report, bool(problems), args.comment == "create")

    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
