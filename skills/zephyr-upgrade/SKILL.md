---
name: zephyr-upgrade
description: >
  Guide for upgrading a Zephyr RTOS-based project from one Zephyr version to another.
  Use this skill when asked to upgrade, migrate, or port a Zephyr project to a newer
  Zephyr version, or when assessing what it would take to do so.
---

# Zephyr RTOS Version Upgrade Guide

This skill guides AI-assisted upgrades of embedded firmware projects built on
[Zephyr RTOS](https://zephyrproject.org). It covers assessment, structural
preparation, migration planning, and known breaking changes across Zephyr
versions.

---

## Phase 0 — Initial Information Gathering

Before doing anything else, ask the user the following questions (one at a time,
do not batch them into a list). Wait for answers before proceeding to the next
phase.

1. **What is the current Zephyr version the project uses?** Check `west.yml`
   (or `zephyr/CMakeLists.txt` for older setups) for the pinned revision or tag.

2. **What is the target Zephyr version?** If the user doesn't know, suggest a
   path (see Phase 3 — Migration Planning). Do not assume; present options.

3. **How are custom Zephyr patches managed?**
   - As a monolithic `.patch` file applied at build time?
   - As commits on a local git branch/fork?
   - Inline modifications to the Zephyr checkout (no tracking)?
   - Via `west` overrides in `west.yml`?

4. **What is the build and test validation method for this project?**
   For example: a `build_all_boards.sh` script, `west build` for a specific
   board, a CI pipeline, a hardware-in-the-loop test suite, or unit tests.
   This will be used as the success criterion at each milestone.
   **If no automated validation exists, help the user define one before
   proceeding — even a simple "builds all supported boards" script is valuable.**

5. **Which boards/targets does the project support?** List all boards, as each
   may need individual attention, especially during DTS and Kconfig migrations.

---

## Phase 1 — Baseline Validation

**Before making any changes**, verify the project builds and passes its defined
validation method on the current Zephyr version.

- If the build already fails on the current version, **fix those issues first**
  and document them. Do not conflate pre-existing issues with migration regressions.
- Run the validation method and record the baseline output.
- Note any warnings that might become errors in newer Zephyr versions
  (e.g., `-Werror=unknown-pragmas`, deprecated DTS properties, deprecated APIs).

---

## Phase 2 — Pre-Migration Structural Improvements

These improvements reduce risk during migration and should be done on the
**current stable version** before any version bump. Do not mix structural
changes with version bumps — keep them as separate, clean commits.

### 2.1 — Patch Management: Flat Patch → Git Branch

If patches are managed as a monolithic `.patch` file or as untracked edits to
the Zephyr checkout, migrate to a **local git fork with named branches**:

```
# In your Zephyr checkout:
git checkout -b ecfw/v<current_version> v<current_version_tag>
# Apply existing patches as individual commits
git am <patches/*.patch>   # or cherry-pick / recreate manually
```

Benefits:
- `git log` shows exactly what is customized
- `git format-patch` exports portable patches
- Cherry-picking to new version branches is clean and reviewable
- Merging upstream changes is `git rebase --onto v<new_tag> v<old_tag>`

Name branches predictably: `project/v3.7`, `project/v4.0`, `project/v4.4`.
Tag the base state of each: `project/v3.7.0-base`.

Update `west.yml` to point to your fork:
```yaml
- name: zephyr
  url: file:///path/to/your/zephyr-fork
  revision: project/v<version>
```

### 2.2 — Directory Structure

Zephyr projects should separate concerns clearly. Common structural issues:

- **`drivers/` naming**: If the directory contains project-specific hardware
  wrappers (not upstream driver class implementations), rename to `lib/` or
  `hal/` to avoid confusion with Zephyr's own `drivers/` directory.
- **Flat CMakeLists.txt**: A single top-level `CMakeLists.txt` collecting all
  sources is hard to maintain. Convert to `add_subdirectory()` per module so
  each module owns its `CMakeLists.txt`.

  > **Critical Zephyr CMake constraint**: `zephyr_library()` only works for
  > code processed before `find_package(Zephyr)` returns (i.e., Zephyr modules).
  > For application code added after `find_package()`, use
  > `target_sources(app PRIVATE ...)` — otherwise, the linker excludes your
  > libraries from the `--start-group`/`--end-group` kernel linker groups,
  > causing undefined references to kernel syscalls and thread symbols.

- **`boards/` directory**: Separate hardware-specific overlays (`.overlay`,
  `.conf` per-board customization) from board init C code. Keep DTS overlays
  and Kconfig fragments alongside the build scripts that use them.

- **Out-of-tree boards**: Keep out-of-tree board definitions under a dedicated
  `out_of_tree_boards/` or `boards/` directory, distinct from board-specific
  overlay files used at build time.

### 2.3 — Validate Again

After structural changes, re-run baseline validation to confirm nothing broke.

---

## Phase 3 — Migration Planning

### Suggested Version Path

**Do not jump across multiple LTS releases without intermediate stops.**
For every major gap, plan intermediate milestones:

| Gap | Recommended stops |
|-----|------------------|
| 3.x → 4.4 | Stop at 3.7 LTS, then 4.0, then 4.4 |
| 3.x → latest | Stop at each LTS: 3.7, 4.4, future LTS |
| 4.x → latest | Stop at 4.4 LTS minimum |

**Strongly recommend stopping at each LTS release.** LTS releases are the
lowest-risk checkpoints because:
- They receive backported bug fixes
- The HAL/driver ecosystem stabilizes around them
- Migration guides from the Zephyr project are most complete for LTS→LTS jumps
- If a regression is found later, you know exactly which version introduced it

Present the suggested path to the user. **Leave the final decision to the user.**
Confirm before proceeding.

### SDK Compatibility (Hard Gates)

The Zephyr SDK version is a hard requirement — an incompatible SDK will produce
cryptic compile errors (often picolibc ABI mismatches or missing toolchain files).

| Zephyr Version | Minimum SDK | Notes |
|----------------|-------------|-------|
| 3.6 | 0.16.x | |
| 3.7 | 0.16.x | 0.16.4 works |
| 4.0 | 0.17.x | 0.16.x causes picolibc `conflicting types` errors |
| 4.1 | 0.17.x | |
| 4.2 | 0.17.x | |
| 4.3 | 0.17.x | |
| 4.4 | 1.0.0 | SDK 0.17.x will fail; SDK 1.0.0 required |

**Before starting each migration step**, verify the correct SDK version is
installed and set `ZEPHYR_SDK_INSTALL_DIR` accordingly.

---

## Phase 4 — Per-Version Migration

For each version step:

1. Create a new patch branch in your Zephyr fork from the target tag:
   ```
   git checkout -b project/v<target> v<target_tag>
   ```
2. **Triage existing patches**: cherry-pick each patch from the previous branch.
   - If a cherry-pick is an empty commit: the change was absorbed upstream → drop it.
   - If it applies cleanly: carry it forward.
   - If it conflicts: resolve the conflict, understanding whether the upstream
     change supersedes or complements your patch.
3. Update `west.yml` to point to the new branch.
4. Run `west update` to pull the new Zephyr and all updated HAL modules.
5. Fix any application-level build failures (see Known Breaking Changes below).
6. Run the project's validation method against all supported boards.
7. Commit all application changes as a single clean commit (or logically grouped
   commits — avoid "fix" follow-up commits; use `git commit --fixup` and
   `git rebase -i --autosquash` to keep history clean).
8. Tag the milestone: `git tag project/v<target>.0-base`.
9. Merge to `master`/`main` only after all boards pass.

---

## Phase 5 — Known Breaking Changes by Version

### Zephyr 3.7 (LTS)

**Board model (hw_model_v2)** — All boards must be updated:
- Add `board.yml` with `name`, `vendor`, `socs: [{name: <soc>}]`
- Rename `Kconfig.board` → `Kconfig.<boardname>` (exact board name)
- In `Kconfig.<boardname>`: use `select SOC_<VARIANT>` (not `depends on`)
- Remove from `*_defconfig`: `config BOARD default "..."`,
  `CONFIG_SOC_SERIES_*=y`, `CONFIG_BOARD_*=y`, `CONFIG_SOC_<variant>=y`
  (these are now auto-generated)

**API renames:**
- `z_device_is_ready()` → `device_is_ready()` (the `z_` prefix was internal)
- `CONFIG_ESPI_SAF` → `CONFIG_ESPI_TAF` (corrected acronym — same functionality)
- Microchip MEC SoC: `SOC_FAMILY_MEC` → `SOC_FAMILY_MICROCHIP_MEC`,
  `SOC_SERIES_MEC1501X` → `SOC_SERIES_MEC15XX`

**KSCAN → Input subsystem (partial migration):**
- XEC KSCAN driver renamed/moved to `drivers/input/input_xec_kbd.c`
- DT node label `kscan0` → `kbd0`
- `CONFIG_KSCAN_XEC_COLUMN_SIZE` / `ROW_SIZE` removed from Kconfig → DT
  properties `col-size` / `row-size` on the `kbd0` node
- `CONFIG_KSCAN_XEC_DEBOUNCE_UP/DOWN` removed from Kconfig → DT properties
- `CONFIG_INPUT=y` must be explicitly set for boards using keyboard
- In 3.7, a compatibility shim (`zephyr,kscan-input`) bridges old KSCAN API
  calls to the new input subsystem; use it as an intermediate step if needed

**Other:**
- `CONFIG_RTOS_TIMER` → `CONFIG_MCHP_XEC_RTOS_TIMER` (Microchip MEC)

---

### Zephyr 4.0

**No application-level breaking changes beyond 3.7** in most projects. The main
item is:
- **SDK**: must upgrade to 0.17.x (0.16.x is incompatible)
- Update `west.yml` revisions for HAL modules (e.g., `hal_microchip`, `cmsis`)

Run `west update` after changing `west.yml`. All HAL module revisions must be
compatible with the new Zephyr version; check `zephyr/west.yml` for the
expected revisions of each module.

---

### Zephyr 4.1

- `CONFIG_CPP_DEMANGLE` removed
- CMake `build type` feature removed (was deprecated in 3.x)
- `DEVICE_API()` macro introduced for driver API tables — check if your
  out-of-tree drivers use the old struct-literal pattern

---

### Zephyr 4.4 (LTS)

This is the largest breaking change release in recent history for projects with
Microchip/KSCAN/eSPI usage. Expect significant work.

**SDK**: Must use SDK 1.0.0. SDK 0.17.x will fail.

**New required west module — `cmsis_6`:**
Zephyr 4.4 uses a separate CMSIS_6 GitHub module for Cortex-M CMSIS headers.
Without it, builds will fail with missing `cmsis_core.h` or similar.
Add to `west.yml`:
```yaml
- name: cmsis_6
  remote: upstream
  revision: <commit-or-tag>  # check zephyr/west.yml for the exact revision
  path: modules/hal/cmsis_6
```
Run `west update cmsis_6` after adding.

**DTS include path restructuring (Microchip MEC):**
SoC DTS files moved from `dts/arm/microchip/` → `dts/arm/microchip/mec/`.
Update any `#include` in your board DTS files accordingly.

**DTS property name strictness — underscore vs hyphen:**
Zephyr 4.4's `edtlib.py` no longer normalizes `_` ↔ `-` in property names.
The check now runs on **all nodes, including `status = "disabled"` nodes**.
Every property name in your DTS must exactly match the binding declaration.
Common renames that will break builds:
- `port_sel` → `port-sel` (I2C, `microchip,xec-i2c-v2`)
- `io_girq` → `io-girq`, `vw_girqs` → `vw-girqs`, `pc_girq` → `pc-girq` (eSPI)
- `chip_select` → `chip-select` (SPI)
- Check all your custom bindings and all nodes referencing vendor bindings

> **Tip**: Run `west build` on one board first to surface DTS errors. They
> appear for all nodes, not just the ones your board enables. If a base SoC
> DTSI in the Zephyr fork has disabled nodes with old property names, you may
> need to patch the DTSI in your Zephyr fork.

**`drive-strength` type change (Microchip MEC pinctrl):**
Changed from string enum (`"1x"`, `"2x"`) to integer (`<1>`, `<2>`).

**SPI node rename (Microchip MEC172x):**
`spi0` → `qspi0` (the LDMA-based QMSPI controller). Check all DTS references,
C code using `DT_NODELABEL(spi0)`, and aliases.

**I2C `sda-gpios`/`scl-gpios` required:**
For enabled I2C nodes using `microchip,xec-i2c-v2`, these properties are now
required. Add them to your board DTS.

**`lines` and `chip-select` removed from ldma binding:**
`microchip,xec-qmspi-ldma` no longer declares these properties. Remove from
your DTS; they are now implicit or derived differently.

**`board.yml` `full_name` required:**
All boards must now include `full_name:` in `board.yml`, or the build will
fail with a confusing CMake error.

**KSCAN subsystem fully removed:**
The compatibility shim (`zephyr,kscan-input`) from 3.7 is gone. The KSCAN
header (`zephyr/drivers/kscan.h`) is deleted. Full migration to the input
subsystem is required:
- Remove `kscan_input: kscan-input` DTS nodes
- Change aliases: `kscan0 = &kscan_input` → `kscan0 = &kbd0`
- Remove `CONFIG_KSCAN=y` (symbol no longer exists)
- In C code:
  - Remove `#include <zephyr/drivers/kscan.h>`
  - Add `#include <zephyr/input/input.h>`
  - Replace `kscan_config(dev, callback)` with `INPUT_CALLBACK_DEFINE(dev, callback_fn)`
  - New callback signature: `static void cb(struct input_event *evt, void *user_data)`
  - Events arrive as a sequence: `INPUT_EV_ABS` with `INPUT_ABS_X` (column),
    then `INPUT_EV_ABS` with `INPUT_ABS_Y` (row), then a sync event
    (`evt->sync == true`, `evt->value` = pressed/released)
  - Use `atomic_t` for any enable/disable flags accessed from the callback
    (it runs in the input thread context)
- Update any `Kconfig` dependency: `default KSCAN` → `default INPUT_XEC_KBD`
  (or equivalent for your platform's input driver)

**`pinctrl_soc_pin_t` struct change (Microchip MEC):**
Changed from `uint32_t` to a `struct mec_pinctrl` bitfield struct. C99 integer
initializers like `MCHP_XEC_PINMUX(...)` must become
`{ .pinmux = MCHP_XEC_PINMUX(...) }`.

**`#pragma error` not valid:**
`#pragma error "message"` is an MSVC extension, not standard GCC.
With `-Werror=unknown-pragmas` this becomes a build error.
Replace with `#error "message"`.

---

## Phase 6 — Post-Migration Checklist

After each milestone:

- [ ] All supported boards build successfully
- [ ] All boards pass the project's defined validation method
- [ ] No new warnings introduced (or they are documented and understood)
- [ ] `west.yml` reviewed: all module revisions are consistent with the target Zephyr
- [ ] Migration commit is clean: no "fix" follow-up commits; squash with
      `git rebase -i --autosquash` before merging to `master`
- [ ] Tag the milestone in the Zephyr fork: `git tag project/v<version>.0-base`
- [ ] Document any issues deferred to a future cleanup commit

---

## General Principles for This Migration

1. **Never mix structural changes with version bumps.** Keep them as separate
   commits to make bisection and rollback straightforward.

2. **One version step at a time.** Build and validate before moving to the next
   target. Chasing two sets of breaking changes simultaneously is extremely
   difficult to debug.

3. **When a board fails but others pass**, investigate that board's DTS/Kconfig
   in isolation. Per-board issues are usually DTS property mismatches specific
   to that SoC or overlay.

4. **Check the Zephyr migration guide** before each step:
   `doc/releases/migration-guide-<version>.rst` in the Zephyr tree.
   Also check `doc/releases/release-notes-<version>.rst`.

5. **Check HAL module revisions** in `zephyr/west.yml` after each bump.
   Out-of-tree boards often depend on HAL modules (e.g., `hal_microchip`,
   `hal_nxp`, `hal_st`) — using old HAL revision with new Zephyr generates
   subtle struct/API mismatches.

6. **Keep custom Zephyr patches minimal and focused.** At each migration step,
   check whether your patch has been absorbed upstream (empty cherry-pick = it has).
   The goal is to reduce the custom patch set over time, not grow it.

7. **Document deferred issues explicitly.** Not every problem needs to be fixed
   during migration. If something is pre-existing and not a regression, document
   it clearly and move on. This prevents scope creep that stalls migrations.

---

## Useful Commands Reference

```bash
# Check current Zephyr version
cat west.yml | grep revision

# Triage a patch (empty = absorbed upstream)
git cherry-pick --allow-empty <sha>
git diff HEAD~1 HEAD --stat   # empty diff = safe to drop

# Clean up fix commits before merging
git rebase -i --autosquash HEAD~<n>

# Build a single board with specific SDK
ZEPHYR_SDK_INSTALL_DIR=/path/to/sdk \
  west build -b <board_name> -p always

# Check west module revisions
west list

# Update a specific west module
west update <module_name>
```
