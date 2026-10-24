---
name: zephyr-pull-request
description: Commit, check, push and open or update a Zephyr pull request. Use when committing changes to the Zephyr tree, preparing a branch for review, folding review fixes, force-pushing, or opening or editing a pull request against zephyrproject-rtos/zephyr.
---

<!--
SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
SPDX-License-Identifier: Apache-2.0
-->

# Zephyr commits and pull requests

The rules are in `AGENTS.md`; where this skill and `AGENTS.md` differ, `AGENTS.md` wins.

## Commit

1. **Branch.** Never commit on `main`. Find the zephyrproject-rtos remote with
   `git remote -v` (`upstream` in a west workspace with a fork, otherwise `origin`) and
   branch from its `main`.
2. **Review the change.** Run `git status` and `git diff --staged`. Stage explicit paths
   only; if nothing is staged, ask what to stage. Split the work into one logical change per
   commit; each commit must build and pass its tests on its own. When
   `./scripts/get_maintainer.py path <files>` shows several areas that split cleanly, plan
   one PR per area.
3. **Write the message** to a file and commit with `-F <file>`, so wrapping and trailers stay
   as written:
   - Prefix from `git log --format=%s -20 -- <path>`, the one the file's history uses most.
     Never `subsys:`, `treewide:` or `WIP`.
   - Title and body lines at most 75 columns. The body says what changed, why, and how it
     was tested.
   - Refer to other commits as `commit <12-char sha> ("subject")`.
   - `Fixes #N` in its own paragraph, then one trailer block as the last paragraph.
   - Exactly one `Assisted-by: <Agent>:<model-version>` line naming the model actually used;
     ask the human if you cannot tell rather than invent a version. No `Co-authored-by:`, no
     "Generated with" line.
4. **Sign-off.** The human certifies the DCO. Commit without `-s` and tell the human to run
   `git commit --amend -s --no-edit` once they have reviewed the commit, unless they have
   seen the diff and explicitly asked you to run `git commit -s`. Never type, edit or copy a
   `Signed-off-by:` line. Check that one sign-off matches the author:
   `git log -1 --format='%an <%ae>%n%b'`.

## Check

5. **Compliance.** From the checkout root, with the compliance requirements installed:

   ```bash
   ./scripts/ci/check_compliance.py --parallel -n -c <remote>/main..HEAD -o <tmpdir>/compliance.xml
   ```

   `-n` keeps it from writing a `<Check>.txt` file per failure into the tree. For a commit
   message error, checkpatch the commit itself:
   `git format-patch -1 --stdout <sha> | ./scripts/checkpatch.pl -`. Fix the cause; never
   silence a check.
6. **Build and test** the narrowest scenario the change can reach, for example
   `west twister -p native_sim -s <test-dir>/<scenario>`, with `ZEPHYR_BASE` exported when
   working in a git worktree. Skip it when the diff reaches no build (documentation,
   `MAINTAINERS.yml`). Whatever was not built or run gets one line in the PR description.
7. **Release documents.** A breaking change to a stable API needs an entry in
   `doc/releases/migration-guide-X.Y.rst`. New, deprecated and removed APIs go in
   `doc/releases/release-notes-X.Y.rst`. `X.Y` is the next release (`VERSION`). Both go in
   the same PR.

## Push and open

8. **Push** to the human's fork, never to the zephyrproject-rtos remote. When rewriting a
   pushed branch, pin the lease to the commit you last pushed:
   `git push --force-with-lease=<branch>:<last-pushed-sha> <fork> <branch>`. A rejected
   lease means someone else pushed. Fetch and inspect their commits instead of retrying.
9. **Open** with `gh pr create --draft --head <fork-owner>:<branch>` and let the human mark
   it ready.
   - Title: the commit subject for a single commit, otherwise `area: summary` covering the
     series.
   - Body: unwrapped Markdown paragraphs. Say what the change does and why, put the
     `Fixes #N` line in when there is an issue, and add one line on what was not built or
     tested. Do not list changed files, paste the diff or restate the title. No AI footer:
     the `Assisted-by:` trailer is the only disclosure.
10. **Return** the PR URL.

## Update after review

11. **Fold fixes** into the commit that owns the lines. Do not add fixup or merge commits:

    ```bash
    git commit --fixup=<sha>
    GIT_SEQUENCE_EDITOR=true git rebase -i --autosquash <remote>/main
    ```

    To reword an older commit, add an `exec git commit --amend -F <file>` line after its
    `pick` instead of using `reword`. With `GIT_EDITOR` set to a no-op, `reword` silently
    keeps the old message. Update the base with `git rebase <remote>/main`, never by
    merging.
12. **Keep the description in sync.** After a force-push, read the current title and body
    first (`gh pr view <N> --json title,body`). The human may have edited them. Change only
    the sentences the push made stale, and say what you changed.
