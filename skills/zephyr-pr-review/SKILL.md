---
name: zephyr-pr-review
description: Use when reviewing Zephyr RTOS pull requests or patches. Spawns parallel subagents that scrutinize code under different lenses: documented conventions, unwritten conventions, CI compliance checks, upstream reference leaks in commit messages and PR metadata, and maintainer impersonation for the areas the diff touches (ATMEL, STM32, tracing, audio). Trigger on commands like "review this PR", "review patch", "check my Zephyr code", or when a PR URL or diff is provided.
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
2. **Which vendor or subsystem personas apply?** Check for:
   - **ATMEL/Microchip SAM**: `drivers/*/*sam*`, `boards/atmel/`, `dts/arm/atmel/`, `soc/atmel/`, `atmel,` compatibles, `SOC_ATMEL*` Kconfig, the `hal_atmel` module
   - **STM32**: `boards/st/`, `dts/arm/st/`, `soc/st/stm32/`, `drivers/*/*stm32*`, `st,stm32` compatibles
   - **Tracing**: `subsys/tracing/`, `include/zephyr/tracing/`, `scripts/tracing/`, `doc/services/tracing/`
   - **Audio**: `drivers/audio/`, `include/zephyr/audio/`, `dts/bindings/audio/`, `samples/drivers/audio/`, `tests/drivers/audio/`, `tests/drivers/build_all/audio/`

### Step 3: Apply the review lenses

The lens instructions live in this same folder. Read each lens file and review the diff against it.

**Always apply these four:**
1. `zephyr-conventions.md` — documented coding conventions
2. `zephyr-unwritten.md` — unwritten codebase patterns
3. `compliance-check.md` — CI compliance script (check_compliance.py) results
4. `upstream-reference-check.md` — `#NNNNN` autolinks in commit messages and PR metadata that post an irreversible cross-reference into an upstream thread

**Conditionally apply (ATMEL only):**
5. `nandojve-impersonator.md` — only if the diff touches ATMEL/Microchip SAM code (boards/atmel, drivers/*/*sam*, dts/arm/atmel, soc/atmel, or atmel-related Kconfig/DT)

**Conditionally apply (STM32 only):**
6. `erwango-impersonator.md` — only if the diff touches STM32 platform code (boards/st/, dts/arm/st/, soc/st/stm32/, or stm32-related Kconfig/DT)
7. `FRASTM-impersonator.md` — only if the diff touches STM32 drivers (drivers/*/*stm32*, drivers/clock_control/*stm32*, drivers/pinctrl/*stm32*)
8. `gautierg-st-impersonator.md` — only if the diff touches STM32 SoC Kconfig, DT bindings, or clock/pinctrl drivers
9. `djiatsaf-st-impersonator.md` — only if the diff touches STM32 board DTS, test overlays, or board defconfigs
10. `mathieuchopstm-impersonator.md` — only if the diff touches STM32 SoC-level DTSI, SoC Kconfig, or HAL integration
11. `GeorgeCGV-impersonator.md` — only if the diff touches STM32 drivers (USB, flash, SDHC, SPI, I2C, video, Ethernet)

**Conditionally apply (tracing only):**
12. `nashif-impersonator.md` — only if the diff touches tracing code (scripts/tracing, subsys/tracing, include/zephyr/tracing, samples/subsys/tracing, tests/subsys/tracing, doc/services/tracing, or the CI wiring for them)
13. `teburd-impersonator.md` — only if the diff touches tracing code or tracing tests/scripts (same paths as above plus scripts/tests/tracing)

**Conditionally apply (audio only):**
14. `rriveramcrus-impersonator.md` — only if the diff touches audio code (drivers/audio/, include/zephyr/audio/, dts/bindings/audio/, samples/drivers/audio/, tests/drivers/audio/, tests/drivers/build_all/audio/)
15. `rgallaispou-impersonator.md` — only if the diff touches audio codec or DMIC drivers, their bindings, or the audio API headers
16. `TomasBarakNXP-impersonator.md` — only if the diff touches audio code, or any NXP board/driver that carries an audio codec node
17. `kartben-impersonator.md` — advisory, whenever the diff touches DT bindings, Kconfig help text, doc/, samples/, boards/, or any new driver; he reviews across the whole tree

For each lens, review the full diff, the file paths changed, the subsystem context, and any PR metadata (title, description, author).

### Step 4: Collect and synthesize

Combine the findings from every lens into a single review with:

1. **Critical Issues** — blocking problems from any agent (must fix)
2. **Compliance Failures** — CI check failures that will block merge (from `compliance-check`)
3. **Upstream Reference Leaks** — `#NNNNN` autolinks that will notify an unrelated upstream thread (from `upstream-reference-check`); blocking before push, unfixable after
4. **Convention Issues** — documented and unwritten convention violations (should fix)
5. **Maintainer Notes** — include the perspective of every impersonator lens that fired: nandojve for ATMEL, the ST reviewers for STM32, nashif and teburd for tracing, the audio reviewers for drivers/audio, kartben where he applies (advisory)
6. **Positive Notes** — things done well
7. **Summary** — overall assessment and recommendation

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

### Upstream Reference Leaks
> [!CAUTION]
> `#NNNNN` references that post a permanent cross-reference into an upstream thread. Fix before pushing — editing does not retract them.

- **[commit <sha> / PR title / PR body]** Description + suggested rewording (`upstream-reference`)

### Convention Violations
> [!WARNING]
> Style and pattern issues that should be addressed.

- **[file:line]** Description (`conventions` / `unwritten`)

### Maintainer Notes
> [!NOTE]
> Feedback from the reviewer personas that fired for this diff. Name each one.

- **@<reviewer>** [comment in that reviewer's style]

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
- `upstream-reference-check.md` — commit message / PR metadata lens for upstream `#NNNNN` autolinks (always applied)
- `nandojve-impersonator.md` — ATMEL maintainer lens (SAM/Atmel only)
- `nashif-impersonator.md` — Tracing maintainer lens (tracing/twister/CI, tracing code only)
- `teburd-impersonator.md` — Tracing collaborator lens (tracing code and tests)
- `erwango-impersonator.md` — STM32 platform reviewer lens (STM32 platform/DTS/SoC)
- `FRASTM-impersonator.md` — STM32 driver reviewer lens (drivers/*stm32*)
- `gautierg-st-impersonator.md` — STM32 SoC/clock reviewer lens (Kconfig, bindings, clock/pinctrl)
- `djiatsaf-st-impersonator.md` — STM32 board/test reviewer lens (board DTS, test overlays)
- `mathieuchopstm-impersonator.md` — STM32 SoC-level reviewer lens (DTSI, SoC Kconfig, HAL)
- `GeorgeCGV-impersonator.md` — STM32 driver correctness lens (USB, flash, SDHC, SPI, I2C)
- `rriveramcrus-impersonator.md` — audio collaborator lens (drivers/audio)
- `rgallaispou-impersonator.md` — audio codec/DMIC reviewer lens (drivers/audio)
- `TomasBarakNXP-impersonator.md` — NXP audio reviewer lens (drivers/audio, NXP boards)
- `kartben-impersonator.md` — tree-wide style/DT/doc reviewer lens (advisory)
