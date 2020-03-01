# SPDX-License-Identifier: Apache-2.0

"""
The classes below are examples of user-defined CommitRules. Commit rules are gitlint rules that
act on the entire commit at once. Once the rules are discovered, gitlint will automatically take care of applying them
to the entire commit. This happens exactly once per commit.

A CommitRule contrasts with a LineRule (see examples/my_line_rules.py) in that a commit rule is only applied once on
an entire commit. This allows commit rules to implement more complex checks that span multiple lines and/or checks
that should only be done once per gitlint run.

While every LineRule can be implemented as a CommitRule, it's usually easier and more concise to go with a LineRule if
that fits your needs.
"""

from gitlint.rules import CommitRule, RuleViolation, CommitMessageTitle, LineRule, CommitMessageBody
from gitlint.options import IntOption, StrOption
import re

class BodyMinLineCount(CommitRule):
    # A rule MUST have a human friendly name
    name = "body-min-line-count"

    # A rule MUST have an *unique* id, we recommend starting with UC (for User-defined Commit-rule).
    id = "UC6"

    # A rule MAY have an option_spec if its behavior should be configurable.
    options_spec = [IntOption('min-line-count', 2, "Minimum body line count excluding Signed-off-by")]

    def validate(self, commit):
        filtered = [x for x in commit.message.body if not x.lower().startswith("signed-off-by") and x != '']
        line_count = len(filtered)
        min_line_count = self.options['min-line-count'].value
        if line_count < min_line_count:
            message = "Body has no content, should at least have {} line.".format(min_line_count)
            return [RuleViolation(self.id, message, line_nr=1)]

class BodyMaxLineCount(CommitRule):
    # A rule MUST have a human friendly name
    name = "body-max-line-count"

    # A rule MUST have an *unique* id, we recommend starting with UC (for User-defined Commit-rule).
    id = "UC1"

    # A rule MAY have an option_spec if its behavior should be configurable.
    options_spec = [IntOption('max-line-count', 3, "Maximum body line count")]

    def validate(self, commit):
        line_count = len(commit.message.body)
        max_line_count = self.options['max-line-count'].value
        if line_count > max_line_count:
            message = "Body contains too many lines ({0} > {1})".format(line_count, max_line_count)
            return [RuleViolation(self.id, message, line_nr=1)]

class SignedOffBy(CommitRule):
    """ This rule will enforce that each commit contains a "Signed-off-by" line.
    We keep things simple here and just check whether the commit body contains a line that starts with "Signed-off-by".
    """

    # A rule MUST have a human friendly name
    name = "body-requires-signed-off-by"

    # A rule MUST have an *unique* id, we recommend starting with UC (for User-defined Commit-rule).
    id = "UC2"

    def validate(self, commit):
        flags = re.UNICODE
        flags |= re.IGNORECASE
        for line in commit.message.body:
            if line.lower().startswith("signed-off-by"):
                if not re.search(r"(^)Signed-off-by: ([-'\w.]+) ([-'\w.]+) (.*)", line, flags=flags):
                    return [RuleViolation(self.id, "Signed-off-by: must have a full name", line_nr=1)]
                else:
                    return
        return [RuleViolation(self.id, "Body does not contain a 'Signed-off-by:' line", line_nr=1)]

class TitleMaxLengthRevert(LineRule):
    name = "title-max-length-no-revert"
    id = "UC5"
    target = CommitMessageTitle
    options_spec = [IntOption('line-length', 72, "Max line length")]
    violation_message = "Title exceeds max length ({0}>{1})"

    def validate(self, line, _commit):
        max_length = self.options['line-length'].value
        if len(line) > max_length and not line.startswith("Revert"):
            return [RuleViolation(self.id, self.violation_message.format(len(line), max_length), line)]

class TitleStartsWithSubsystem(LineRule):
    name = "title-starts-with-subsystem"
    id = "UC3"
    target = CommitMessageTitle
    options_spec = [StrOption('regex', ".*", "Regex the title should match")]

    def validate(self, title, _commit):
        regex = self.options['regex'].value
        pattern = re.compile(regex, re.UNICODE)
        violation_message = "Title does not follow [subsystem]: [subject] (and should not start with literal subsys:)"
        if not pattern.search(title):
            return [RuleViolation(self.id, violation_message, title)]

class MaxLineLengthExceptions(LineRule):
    name = "max-line-length-with-exceptions"
    id = "UC4"
    target = CommitMessageBody
    options_spec = [IntOption('line-length', 80, "Max line length")]
    violation_message = "Line exceeds max length ({0}>{1})"

    def validate(self, line, _commit):
        max_length = self.options['line-length'].value
        urls = re.findall(r'http[s]?://(?:[a-zA-Z]|[0-9]|[$-_@.&+]|[!*\(\),]|(?:%[0-9a-fA-F][0-9a-fA-F]))+', line)
        if line.startswith('Signed-off-by'):
            return

        if urls:
            return

        if len(line) > max_length:
            return [RuleViolation(self.id, self.violation_message.format(len(line), max_length), line)]
