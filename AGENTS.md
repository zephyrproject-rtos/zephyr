<!--
SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
SPDX-License-Identifier: Apache-2.0
-->

# Zephyr RTOS: instructions for AI coding agents

This file is for coding agents, and for the humans driving them, working in this repository.
It is a digest of `doc/contribute/` plus the things CI and maintainers reject most often. Where
it is silent or disagrees with the documentation, the documentation wins:
`doc/contribute/guidelines.rst` (process, DCO, AI-assistant policy),
`doc/contribute/contributor_expectations.rst` (PR shape), `doc/contribute/style/` (C, Kconfig,
CMake, Doxygen and devicetree style) and `doc/contribute/coding_guidelines/index.rst` (the
MISRA-C subset). Read the relevant page before working in an area you have not touched before;
do not rely on this digest or on training data alone.

## Rules for agents

- Never add a `Signed-off-by:` line: only the human submitter may sign off (DCO). Never add
  `Co-authored-by:`. Do not put "Generated with ...", session links or any other mention of AI
  tools in commit messages, PR bodies, issues or comments. The `Assisted-by:` trailer below is
  the only place AI involvement is recorded.
- Add exactly one `Assisted-by: <Agent>:<model-version> [tool ...]` trailer, for example
  `Assisted-by: Claude:claude-opus-4.6 coccinelle`, naming the tool actually used. Replace it
  rather than stacking when a different model amends the commit. `checkpatch.pl` validates the
  format; basic tools (git, gcc, cmake, editors) are not listed.
- The human reviews and tests every change before it is submitted. State what you did not do
  (not built, not run, not run on hardware, no reproducer) instead of implying it was done.
- Review comments written with AI help are verified by the human before posting; never post raw
  model output (`doc/contribute/reviewer_expectations.rst`).
- Commit messages, code comments, Kconfig help texts and docs describe the tree as it is: no
  references to prompts, plans, sessions, "the rework" or "as requested". Write tersely and
  concretely; maintainers reject verbose, hedging or self-congratulatory prose. No emoji.
- Change what the task needs, which can mean refactoring the code a fix touches, but keep
  unrelated reformatting, renames and cleanups out of the change; style is enforced on new or
  modified lines only. A wider cleanup is a separate PR.

## Build and test

- Zephyr builds only inside a west workspace: from the parent directory of the clone,
  `west init -l <zephyr-dir> && west update` (most boards need HAL and module repositories from
  `west.yml`). Build with `west build -b <board> <app-dir>`; `native_sim` runs on the host.
  See `doc/develop/west/workspaces.rst`.
- Tests and samples are run by twister: `west twister -p native_sim -T <test-dir>` or
  `-s <test-dir>/<scenario-id>`; `--build-only` skips execution; `-i` prints failing logs.
  Suites whose `tests.yaml` lists `unit_testing` as platform need `-p unit_testing`. See
  `doc/develop/twister/index.rst`.
- In a `git worktree`, export `ZEPHYR_BASE=<worktree>` for `west build` and twister; otherwise
  the workspace's registered checkout is built silently instead of your tree.
- Every commit in a series must build and pass its tests on its own (bisectability).

## Checks to run before pushing

CI runs the same checks (`.github/workflows/compliance.yml`); each failure costs a full round
trip. Fix the cause, never work around a check.

- `pip install -r scripts/requirements-compliance.txt`, then
  `./scripts/ci/check_compliance.py --parallel -c upstream/main..HEAD` (`upstream` being the
  zephyrproject-rtos remote, `origin` in a plain clone; the same below). `-l` lists the checks
  (Checkpatch, Gitlint, KconfigBasic, CMakeStyle, DevicetreeBindings, Ruff, Pylint, YAMLLint,
  SphinxLint, KeepSorted, ...); `-m <Check>` runs one.
- checkpatch on one commit: `git format-patch -1 --stdout <sha> | ./scripts/checkpatch.pl -`.
  Piping `git show` instead produces bogus `BAD_SIGN_OFF` errors.
- ClangFormat is advisory (`.clang-format`). Apply it to your own hunks only and skip hunks
  where it reflows surrounding macro tables; never run it on whole files.
- CMake style (`doc/contribute/style/cmake.rst`) is checked on touched lines only and old files
  are grandfathered: copying an old `CMakeLists.txt` as a template can fail CI although the
  original passes. The same holds for any in-tree file you copy: check it against the current
  style page, not only against its neighbors.

## Commits

    area: subarea: imperative summary (75 columns max, no trailing period)

    Body wrapped at 75 columns: what the change does, why this approach,
    which assumptions were made and how it was tested. Never empty.

    Fixes #12345

    Assisted-by: Claude:claude-opus-4.6
    Signed-off-by: Full Name <email@example.com>

- `area:` is the prefix the file's recent history uses: `git log --format=%s -20 -- <path>`.
  Examples: `Bluetooth: Host:`, `drivers: i2c: nrfx:`, `dts: arm: st:`, `boards: nordic:`,
  `kernel:`, `doc:`, `.github:`. Never `subsys:` or `treewide:`, never `WIP`.
- Trailers (`Assisted-by`, `Signed-off-by`, `Link:`, `(cherry picked from commit ...)`) form one
  block as the last paragraph. `Fixes #N` goes in its own paragraph directly before that
  block. Reference other commits as `commit <12-char sha> ("subject")`.
- One `Signed-off-by:` line must match the commit `Author:` name and e-mail; never remove an
  existing sign-off. Verify with `git log -1 --format='%an <%ae>%n%b'`.
- One logical change per commit: a fix, its tests and its docs may be separate commits, but one
  commit never mixes unrelated changes. No fixup, squash or merge commits in a PR: fold review
  fixes into the commit that owns the lines. Without an interactive terminal:
  `git commit --fixup=<sha>`, then
  `GIT_SEQUENCE_EDITOR=true GIT_EDITOR=true git rebase -i --autosquash <base>`.
- Update a branch with `git rebase upstream/main`, never by merging `main` into it.
- Stage explicit paths (`git add <file>`); never `git add -A`, `git add .` or `commit -a`, which
  pick up build directories, notes and reports. Before pushing, review
  `git show --stat upstream/main..HEAD`.

## Pull requests

- A PR is one self-contained change. Reviewers are assigned from the `MAINTAINERS.yml` areas of
  the touched files (`./scripts/get_maintainer.py path <files>` lists them), so when a change
  spans several areas and splits cleanly, split it so each part gets the right reviewers.
  Small PRs get reviewed; large changes start as an RFC issue
  (`doc/contribute/proposals_and_rfcs.rst`).
- The PR description summarizes the change and its rationale and carries the `Fixes #N` line
  when the change addresses an issue.
  It is rendered as Markdown: unwrap paragraphs instead of pasting hard-wrapped commit text.
  Keep title and description in sync with the commits after every force-push.
- Breaking changes to stable APIs are described in the migration guide; new, deprecated and
  removed APIs are listed in the release notes. Both go in the same PR as the
  change: `doc/releases/migration-guide-X.Y.rst` and `doc/releases/release-notes-X.Y.rst`, where
  `X.Y` is the next release (`VERSION` shows `X.(Y-1).99` during development). Stable APIs
  follow `doc/develop/api/api_lifecycle.rst`.
- Backports to `v*-branch` are opened by a bot when a maintainer adds `backport vX.Y-branch`
  labels after merge. Do not open manual backport PRs unless the bot failed. Every backport PR
  body must contain a `Fixes #N` line resolving to a real issue
  (`.github/workflows/backport_issue_check.yml`), so bug fixes that may be backported need a
  public issue.
- Issues use a GitHub issue type (Bug, Feature, Enhancement, RFC, Task) and the templates in
  `.github/ISSUE_TEMPLATE/`, not a `bug` label: `gh issue create --type Bug ...`.
- Security: never file a public issue or PR describing an undisclosed vulnerability, and never
  reference an unpublished advisory (GHSA/CVE) in a commit, PR or issue. See
  `.github/SECURITY.md`.
- When reviewing, remember that CI tests the PR rebased onto current `main`. Reproduce the same
  way before reporting a failure or claiming what `main` does or does not contain:
  `git fetch upstream main && git fetch upstream pull/<N>/head && git checkout --detach
  FETCH_HEAD && git rebase upstream/main` (two fetches: after a multi-ref fetch `FETCH_HEAD` is
  the first ref, `main`).

## Code rules agents get wrong most often

Complete lists: `doc/contribute/style/code.rst`, `doc/contribute/coding_guidelines/index.rst`.

- Linux kernel style: 8-column tabs, 100-column lines, braces on every `if`/`else`/loop body,
  `/* */` comments only (no `//`), `snake_case`, no binary literals; non-ASCII symbols only
  where they significantly improve clarity, emoji never.
- Explicit comparisons: `if (err != 0)`, `if (ptr == NULL)`, `if (len > 0)`. Controlling
  expressions must be essentially Boolean (coding guideline rule 85); `if (!err)` on an
  integer is not allowed.
- Fixed-width types (`uint8_t`, `int32_t`) for numeric data and `size_t` for sizes instead of
  bare `int`, `short` or `long` (Dir 4.6; `char` stays for text); `U` suffix on unsigned
  constants; no octal constants.
- No commented-out code; check every return value that carries an error; include guards in
  every header.
- Dynamic allocation: the coding guidelines list MISRA Dir 4.12 (no dynamic allocation) and
  Rule 21.3 (no `<stdlib.h>` allocators) as required, while Rule A.4 allows the libc allocators
  in the kernel and subsystems use `k_malloc()`, `k_heap`, slabs and `net_buf` pools. Follow the
  surrounding subsystem: do not "fix" existing allocation, and do not add a heap where a static
  pool fits.
- Prefer `if (IS_ENABLED(CONFIG_FOO)) { ... }` to `#ifdef CONFIG_FOO` for code paths. Both
  branches must compile, so declarations in headers stay unconditional (guideline Rule A.1).
  Use the preprocessor only for real memory or ABI differences, and follow the surrounding
  file's pattern for small edits.
- Public symbols carry their subsystem prefix (`k_`, `sys_`, `net_`, `bt_`, `i2c_`, ...). Public
  functions have Doxygen with a brief description, `@param` and `@retval <value> <text>` for
  each discrete return value, success first and a value repeated when different conditions
  produce it: `@retval -EINVAL Invalid argument`, not "if the argument is invalid"
  (`doc/contribute/style/doxygen.rst`).
- Every `CONFIG_` symbol referenced in code or Kconfig must be defined in the tree (KconfigBasic
  check). Symbol naming and `menuconfig`/`if` structure: `doc/contribute/style/kconfig.rst`;
  `select` versus `depends on`: `doc/build/kconfig/tips.rst`.
- New original files start with the two SPDX header lines in the file's comment syntax: the
  copyright text (`Copyright The Zephyr Project Contributors`, or the actual holder) and the
  `Apache-2.0` license identifier; the top of this file shows the Markdown form. Code imported
  under another license keeps its own headers, needs the approval described in
  `doc/contribute/guidelines.rst` (Components using other Licenses) and an annotation in
  `REUSE.toml`; never rewrite existing license metadata.
- Test and sample metadata lives in `tests.yaml`; `sample.yaml` and `testcase.yaml` are legacy
  names CI rejects for new files. Scripts that twister executes directly, such as bsim
  `tests_scripts/*.sh`, must be committed with mode `100755`.
- Blocks delimited by `zephyr-keep-sorted` start/stop marker comments stay sorted (KeepSorted
  check). No binary files except images under `doc/`, `boards/` and `samples/` within the
  size limits (the BinaryFiles check holds the exact allow-list).
- Devicetree: a compatible for a vendor's device is `<vendor>,<device>` with the vendor listed
  in `dts/bindings/vendor-prefixes.txt`; generic hardware such as `gpio-leds` gets no invented
  prefix. Every new compatible needs a binding under `dts/bindings/`. See
  `doc/contribute/style/devicetree.rst` and `doc/build/dts/bindings-upstream.rst`.
- Documentation under `doc/` is reStructuredText in American English, 100 columns (URLs
  excepted): `doc/contribute/documentation/guidelines.rst`.

## Finding things

- Subsystems: `subsys/`; drivers: `drivers/<class>/`; public API headers: `include/zephyr/` and
  the APIs defined under `lib/` (a subsystem's own `include/zephyr/`, such as ztest's, is not
  public API); kernel: `kernel/`; boards: `boards/<vendor>/`; SoCs: `soc/`; devicetree: `dts/`;
  tests: `tests/`; samples: `samples/`; tooling: `scripts/`.
- `MAINTAINERS.yml` maps paths to areas and people. The recent history of a path is the best
  guide to its local conventions: `git log --oneline -20 -- <path>`.
- The documentation source is `doc/`, published at https://docs.zephyrproject.org/latest/. The
  published docs, the source tree and the last six months of issues and PRs are also searchable
  through the project's MCP server, `https://zephyrproject.mcp.kapa.ai`
  (`doc/develop/tools/kapa_ai.rst`); its answers are generated and must be verified against the
  tree.
