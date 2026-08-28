<!--
SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
SPDX-License-Identifier: Apache-2.0
-->

# Zephyr Project — Copilot Instructions

The instructions for AI coding agents working on this repository are in `AGENTS.md` at the
repository root. Copilot cloud agent, Copilot code review and the IDE agents load that file
directly; this file only covers the Copilot features that do not, and code review.

## All Copilot features

- Follow `AGENTS.md`, `doc/contribute/guidelines.rst` and `doc/contribute/style/`.
- Never add `Signed-off-by:` or `Co-authored-by:`; add one `Assisted-by: Copilot:<model>`
  trailer to commits you help write.

## Code review

- Report only concrete, verifiable problems: a rule violation with the rule, a bug with the
  input that triggers it, a missing test with the untested behavior. Skip praise and summaries.
- Do not request changes for anything CI catches (checkpatch, gitlint, compliance); reviewers
  are asked not to (`doc/contribute/reviewer_expectations.rst`).
- Do not flag style in lines the PR does not touch, and do not ask for tree-wide cleanups.
- Check commit messages: `area:` prefix, non-empty body, `Signed-off-by` present, exactly one
  `Assisted-by:` trailer when AI tools were used, no `Co-authored-by`. A change that addresses
  an issue carries `Fixes #N` in the PR description or a commit.
