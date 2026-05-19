# Zephyr Project — Copilot Instructions

These instructions are derived from `doc/contribute/` and apply to all AI-assisted
contributions to the Zephyr RTOS repository.

## Prerequisites

- All contributions target the `main` branch via GitHub Pull Requests.
- Familiarity with CMake, Git, and GitHub is assumed.

---

## Licensing & File Headers

- All new files must use **Apache 2.0** and include SPDX headers at the very top using
  the file's native comment syntax:

  ```
  SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
  SPDX-License-Identifier: Apache-2.0
  ```

  The copyright statement may refer to the individual or organization who authored the contribution
  but "Copyright The Zephyr Project Contributors" is always the preferred form.

- External code under a license other than Apache 2.0 requires TSC + governing board
  approval before integration.

---

## Developer Certificate of Origin (DCO)

- Every commit requires a `Signed-off-by` line — **only humans** may add this; AI agents
  should not add `Signed-off-by` tags unless explicitly requested by humans.
- The human submitter is responsible for reviewing all AI-generated code, ensuring license
  compliance, and signing off.
- Use a real legal name and real email address (no pseudonyms, no noreply addresses).

Do not add a `Co-authored-by` tag to commit messages.

When AI tools are used, add an attribution tag in the commit message:

```
Assisted-by: [Agent Name]:[Model Version] [Tool1] [Tool2]
```

Example:

```
Assisted-by: Claude:claude-opus-4.6 coccinelle
```

Basic tools (git, gcc, make, editors) should not be listed.

---

## C Code Style

Follow the
[Linux kernel coding style](https://kernel.org/doc/html/latest/process/coding-style.html)
with these Zephyr-specific rules:

- **Tabs** are 8 characters; **line length** ≤ 100 columns.
- Use `snake_case` for all code and variable names.
- **Always add braces** to `if`, `else`, `do`, `while`, `for`, and `switch` bodies — even
  single-line blocks.
- Use `/* */` for comments; `//` is not allowed.
- Use `/** */` for Doxygen API documentation.
- No binary literals (`0b...`).
- No non-ASCII symbols in code (no emojis under any circumstances).
- Capitalize correctly in comments: `UART` not `uart`, `CMake` not `cmake`.
- Use spaces (not tabs) to align inline comments after declarations.
- Style is enforced on new or modified code only — do not reformat unrelated existing code.

---

## Naming Conventions (C)

- All public APIs must be prefixed by subsystem: `k_` (kernel), `sys_`, `net_`, `bt_`,
  `i2c_`, etc.
- Identifiers must be unique across all scopes; no shadowing of outer-scope identifiers.
- `typedef` names and tag names must be globally unique identifiers.
- Macro identifiers must be distinct from all other identifiers.

---

## MISRA-C Coding Guidelines

All new code must comply with the Zephyr MISRA-C subset. Key required rules:

- **No dynamic memory allocation** (`malloc`, `calloc`, etc.).
- Document all implementation-defined behavior on which program output depends.
- All source files must compile without errors.
- No unreachable code and no dead code.
- No commented-out code sections.
- Check return values of all functions that return error information.
- Use fixed-width typedefs (`uint8_t`, `int32_t`, etc.) instead of bare `int`, `char`.
- Prefer `inline` or `static` functions over function-like macros.
- All header files must have include guards.
- Octal constants are not allowed.
- Apply a `u` or `U` suffix to all unsigned integer constants.
- Do not use the lowercase `l` suffix in literals.
- Identifiers in the same namespace with overlapping visibility must be typographically
  distinct.

---

## Kconfig Style

- **Indentation**: tabs; `help` text: one tab + two extra spaces.
- **Line length**: ≤ 100 columns.
- One blank line between symbol declarations.
- Comments: `# Comment` (space after `#`).
- One blank line before and after top-level `if` / `endif` blocks.

### Symbol Naming

| Subtree | Symbol format | Prompt format |
|---|---|---|
| Drivers | `{DRIVER_TYPE}_{DRIVER_NAME}` | `{Driver Name} {Driver Type} driver` |
| Sensors | `SENSOR_{SENSOR_NAME}` | `{Sensor Name} sensor driver` |
| Samples | `SAMPLE_...` | — |
| Tests | `TEST_...` | — |
| Boards | `BOARD_...` | — |
| SoCs | `SOC_FAMILY_...`, `SOC_SERIES_...`, or `SOC_...` | — |

- Use `menuconfig` (not `config`) for enabling symbols that have sub-options.
- Encapsulate sub-symbols in an `if` block keyed to the enabling symbol.
- Prefix sub-symbols with the enabling symbol's name.

---

## CMake Style

- **Indentation**: 2 spaces (no tabs).
- **Line length**: ≤ 100 characters.
- Always use **lowercase** CMake commands: `add_library`, not `ADD_LIBRARY`.
- No space between a command name and its opening parenthesis: `if(...)` not `if (...)`.
- One source file argument per line in `target_sources()` and similar calls.
- **UPPERCASE** for cache variables and variables shared across files
  (`option`, `set(... CACHE ...)`).
- **lowercase / snake_case** for local variables.
- Always quote strings and path variables; do not quote boolean values (`ON`, `OFF`).
- Use CMake variables (`CMAKE_SOURCE_DIR`, `CMAKE_BINARY_DIR`) — never hardcode paths.
- Use comments to document complex logic.

---

## Documentation Style

- Written in reStructuredText (`.rst`).
- Public API headers must use Doxygen `/** */` block comments.
- Follow `doc/contribute/documentation/guidelines.rst` for formatting details.
- URL references are the only allowed exception to the 100-column line limit in docs.

---

## Commit Message Guidelines

Every commit message must follow this format:

```
[area]: [summary of change]

[Commit message body]

Signed-off-by: Your Full Name <your.email@example.com>
```

### Title (`[area]: [summary]`)

- **One line**, **<= 75 characters**, followed by a **completely blank line**
- `[area]` identifies the subsystem or context being changed. Examples:
  - `doc:` — documentation changes
  - `drivers: foo:` — changes to the `foo` driver
  - `Bluetooth: shell:` — Bluetooth shell changes
  - `net: ethernet:` — Ethernet networking changes
  - `dts:` — treewide devicetree changes
  - `.github:` — CI/tooling configuration changes
- If unsure, run `git log FILE` on a file you're changing and follow existing conventions

### Body

- **Must not be empty** — even trivial changes require a descriptive body (CI will fail otherwise)
- Explain **what** the change does, **why** you chose this approach, **what** assumptions
  were made, and **how** you know it works (e.g. which tests were run)
- Lines should be ≤ **75 characters**; exceptions for long URLs or email addresses

### Signed-off-by

- Required on every commit for DCO compliance — use `git commit -s` to add automatically
- Must use your **legal name** and **real email** matching the commit `Author:` field
- **AI agents must not add `Signed-off-by`** — only the human submitter may sign off
- Do not add a `Co-authored-by` tag to commit messages

### Linking Issues

To close a GitHub issue automatically on merge, add to the commit or PR description:

```
Fixes #[issue number]
```

Or cross-repo:

```
Fixes zephyrproject-rtos/zephyr#[issue number]
```

---

## Pull Request Guidelines

- Search existing issues and PRs before starting work.

### Modifying Others' Contributions

- Cherry-picking from a stale PR: explain the reason in your PR and invite the original
  author to review.
- Force-pushing to another contributor's branch: state the reason explicitly in the PR.
- If patches are substantially modified, either obtain the original author's acknowledgment
  or resubmit as your own work with a reference to the original PR.

---

## Source Tree Quick Reference

| Directory | Contents |
|---|---|
| `arch/` | Architecture-specific kernel and SoC code |
| `boards/` | Board configurations |
| `doc/` | Documentation sources |
| `drivers/` | Device drivers |
| `dts/` | Devicetree source files |
| `include/` | Public API headers |
| `kernel/` | Architecture-independent kernel code |
| `lib/` | Library code (minimal libc, etc.) |
| `samples/` | Sample applications |
| `scripts/` | Build and test tooling |
| `subsys/` | Subsystems (networking, BT, USB, FS, …) |
| `tests/` | Tests and benchmarks |
| `soc/` | SoC-specific code and configuration |
