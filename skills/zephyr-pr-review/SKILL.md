---
name: zephyr-pr-review
description: Use when reviewing Zephyr RTOS pull requests or patches. Spawns parallel subagents that scrutinize code under different lenses: documented conventions, unwritten conventions, CI compliance checks, and (for ATMEL code) maintainer impersonation. Trigger on commands like "review this PR", "review patch", "check my Zephyr code", or when a PR URL or diff is provided.
---

# Zephyr PR Review Skill

You are a PR review orchestrator for Zephyr RTOS. You review the code under several specialized lenses (each defined in a sub-skill file in this same folder), then synthesize the findings into a unified review.

## Workflow

### Step 1: Gather the diff

Get the diff to review. Accept:
- A PR URL (fetch with `gh pr diff <number>` or `git diff origin/main...`)
- A branch name (use `git diff origin/main...<branch>`)
- Pasted diff text
- A set of changed files (use `git diff` against HEAD)

If no specific input is given, default to `git diff origin/main...HEAD`.

### Step 2: Determine scope

Read the diff and determine:
1. **Which subsystems are touched?** (drivers, subsys, arch, boards, dts, etc.)
2. **Is this ATMEL/Microchip SAM code?** Check for:
   - Files matching `drivers/*/*sam*`, `boards/atmel/`, `dts/arm/atmel/`, `soc/atmel/`
   - Devicetree nodes with `atmel,` compatible strings
   - Kconfig symbols starting with `SOC_ATMEL` or referencing SAM chips
   - Any file in the `hal_atmel` module

### Step 3: Apply the review lenses

The lens instructions live in this same folder. Read each lens file and review the diff against it.

**Always apply these three:**
1. `zephyr-conventions.md` — documented coding conventions
2. `zephyr-unwritten.md` — unwritten codebase patterns
3. `compliance-check.md` — CI compliance script (check_compliance.py) results

**Conditionally apply (ATMEL only):**
4. `nandojve-impersonator.md` — only if the diff touches ATMEL/Microchip SAM code (boards/atmel, drivers/*/*sam*, dts/arm/atmel, soc/atmel, or atmel-related Kconfig/DT)

For each lens, review the full diff, the file paths changed, the subsystem context, and any PR metadata (title, description, author).

### Step 4: Collect and synthesize

Combine the findings from every lens into a single review with:

1. **Critical Issues** — blocking problems from any agent (must fix)
2. **Compliance Failures** — CI check failures that will block merge (from `compliance-check`)
3. **Convention Issues** — documented and unwritten convention violations (should fix)
4. **Maintainer Notes** — if ATMEL, include nandojve's perspective (advisory)
5. **Positive Notes** — things done well
6. **Summary** — overall assessment and recommendation

Deduplicate findings across agents. If multiple agents flag the same issue, mention it once with the strongest framing.

### Step 5: Format the review

Present the review in this format:

```
## Zephyr PR Review

### Critical Issues
> [!CAUTION]
> Issues that will likely cause CI failure or functional bugs.

- **[file:line]** Description (`conventions` / `unwritten` / `maintainer` / `compliance`)

### Compliance Failures
> [!CAUTION]
> Automated check failures from `check_compliance.py` that will block CI.

- **[file:line]** `CheckName`: Description (`compliance`)

### Convention Violations
> [!WARNING]
> Style and pattern issues that should be addressed.

- **[file:line]** Description (`conventions` / `unwritten`)

### Maintainer Notes
> [!NOTE]
> Feedback from the ATMEL/Microchip SAM maintainer perspective.

- [comment in nandojve's style]

### Positive Notes
- Things done well

### Summary
[Overall assessment with recommendation: approve / request changes / needs discussion]
```

## Review lenses

The review lenses live next to this skill, in the same folder (peer sub-skills, not registered agents):
- `zephyr-conventions.md` — documented conventions lens
- `zephyr-unwritten.md` — unwritten patterns lens
- `compliance-check.md` — CI compliance script lens (always applied)
- `nandojve-impersonator.md` — ATMEL maintainer lens (SAM/Atmel only)
