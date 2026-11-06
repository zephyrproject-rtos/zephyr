---
name: zephyr-upgrade
description: >
  Guide for upgrading a Zephyr RTOS-based project from one Zephyr version to
  another. Use this skill when asked to upgrade, migrate, or port a Zephyr
  project (application, out-of-tree boards/drivers, or a downstream fork) to a
  newer Zephyr version, or when assessing what such an upgrade would take.
---

# Zephyr RTOS Version Upgrade Guide

This skill guides AI-assisted (and human-assisted) upgrades of projects built on
[Zephyr RTOS](https://zephyrproject.org). It is deliberately **version- and
vendor-agnostic**: it teaches the *process* of a Zephyr upgrade and the
*failure modes* that recur across projects, and it defers all version-specific
API/DTS/Kconfig details to the authoritative source — Zephyr's own release
notes and migration guides.

> **Golden rule: the migration guide is the source of truth.**
> For every concrete "what changed in version X" question, read
> `doc/releases/migration-guide-<X>.rst` and `doc/releases/release-notes-<X>.rst`
> **in the target Zephyr tree**. This skill does *not* reproduce those lists;
> it tells you how to apply them and warns you about the things they don't say.
> If you find a real breaking change that the migration guide fails to mention,
> that is a bug in the guide — consider proposing a fix upstream.

---

## What this skill adds beyond the migration guide

The migration guides are excellent at "symbol A became symbol B". They are
mostly silent about the *engineering reality* of running an upgrade. The
sections below capture that reality — the recurring, project-independent
hazards observed when repeatedly porting unrelated projects (keyboards,
wearables, embedded controllers) across Zephyr versions:

- Host-tool and SDK version gates that fail *before* any of your code compiles.
- How structural changes (the board model, Kconfig/CMake variable renames)
  ripple into *every* board and into any project that copied Zephyr's own
  build snippets.
- Why a clean per-version, per-target, one-change-at-a-time workflow is the
  difference between a tractable upgrade and an undebuggable one.
- How downstream customizations (Kconfig overrides, forks, patches) break
  *silently* when upstream removes the thing they were customizing.

---

## Phase 0 — Initial Information Gathering

Before doing anything, establish the facts below. Ask the user only for what you
cannot determine yourself from the repository. Ask one question at a time and
wait for the answer.

1. **Current Zephyr version.** Find the pinned revision/tag in `west.yml` (or,
   for very old or non-west setups, wherever the Zephyr revision is recorded).
   Note the exact commit, not just "roughly 3.x".

2. **Target Zephyr version.** If the user has no firm target, propose one (see
   Phase 3) but let the user decide.

3. **How, if at all, is Zephyr customized?** Projects fall on a spectrum:
   - **No customization** — plain upstream Zephyr pinned in `west.yml`. This is
     common; do not assume a fork or patches exist.
   - **A downstream fork** with commits on top of an upstream tag (possibly its
     own `+project-fixes` branch).
   - **Patches** applied at build time, or ad-hoc edits to the Zephyr checkout.
   - **Out-of-tree** boards/SoCs/drivers/modules that track Zephyr APIs.
   The customization model determines how much Phase 4 work is "carry my changes
   forward" versus "just bump the revision".

4. **Validation method** — the success criterion for every milestone. A
   "builds all supported targets" script is the minimum; hardware or CI tests
   are better. **If none exists, help create one first** — an upgrade without a
   repeatable pass/fail signal is not verifiable.

5. **Targets** — every board/SoC/shield/variant the project supports. Structural
   migrations (board model, DTS, Kconfig) act per target, so the target list is
   your migration checklist.

---

## Phase 1 — Establish and Verify the Baseline

**Before changing anything, get a known-good baseline on the current version.**

- Build and run the validation method on the *current* Zephyr version.
- If it does not pass today, fix that first and record it. Never let
  pre-existing breakage masquerade as a migration regression.
- **Trust the validation method only after verifying it actually fails on
  failure.** A script that swallows exit codes, continues past a failed build,
  or reports success when an artifact is missing produces a *false green* — the
  most dangerous state in a migration. Deliberately break one target once and
  confirm the validation goes red before you rely on it.
- **Verify the baseline actually contains the customizations you think it does.**
  If the project claims to carry patches or a fork, confirm they are *applied*
  in the checkout you are baselining (diff against the pristine upstream tag).
  Carrying "patches" forward that were never applied yields a build that is
  green for the wrong reason.
- Capture the baseline output (which targets pass, size/warnings) so you can
  diff against it after each step.
- Note warnings, especially deprecation warnings — they are the preview of the
  *next* version's errors.

### The environment is part of the baseline

An upgrade can fail before a single line of your code is touched, because the
*build environment* is wrong. Verify these explicitly:

- **Toolchain / Zephyr SDK version.** Each Zephyr tree records the SDK it was
  validated against (e.g. in a `SDK_VERSION` file). Treat that as the
  recommended default and prefer it unless you deliberately use another
  supported toolchain (`ZEPHYR_TOOLCHAIN_VARIANT`); confirm against the tree's
  toolchain documentation. A toolchain the tree did not expect typically
  surfaces as cryptic C-library/ABI errors deep in the build (for example,
  `conflicting types` for libc lock/retargetable-locking symbols) rather than an
  honest "wrong SDK" message — so when the failure is in libc rather than your
  code, suspect the toolchain version first. Point the build at the intended SDK
  (`ZEPHYR_SDK_INSTALL_DIR`) for the version you are currently on.
- **Host tooling (CMake, Python, dtc, west) and Python dependencies.** Zephyr's
  *own* build scripts and Python requirements are versioned with the tree. Two
  things follow:
  - Use an **isolated environment per hop** that satisfies *that* tree's
    declared requirements — install its `scripts/requirements*.txt` (and west
    plugins) into a per-version virtualenv rather than sharing one across
    versions. Different trees pin incompatible Python/west expectations.
  - A host toolchain much **newer** than the checked-out tree can break Zephyr's
    CMake/Python before your project is even parsed (e.g. stricter CMake
    argument handling tripping over an older module's unquoted variable
    expansion). Satisfy each tree's declared host-tool minimums, and when a very
    old tree fights a very new host, apply the minimal workaround (for instance,
    setting a variable the old script assumed would be exported) rather than
    editing many Zephyr internals. This is also a reason **not** to linger on
    ancient versions: the farther your host runs ahead of the tree, the more of
    these you hit, so moving forward is often *easier* than faithfully
    reproducing a very old build.

Record the exact toolchain, host-tool, and Python-environment versions that
produce a green baseline. They change as you move between version steps.

---

## Phase 2 — Optional Pre-Migration Cleanup

Some projects benefit from cleanup *before* a version bump — but only do it if
it genuinely reduces migration risk, and keep it strictly separate.

- **Never mix cleanup with a version bump.** Structural refactors and Zephyr
  version changes must be different commits so that a regression can be bisected
  to one or the other.
- If custom Zephyr changes are currently untracked (ad-hoc edits or a flat
  patch blob), convert them to **reviewable commits on a branch of a Zephyr
  checkout** first. Git history then tells you exactly what is customized, makes
  each change portable (`git format-patch`), and turns "move my changes to the
  new version" into a `git rebase --onto <new-tag> <old-tag>` or a series of
  cherry-picks.
- Re-run the baseline validation after any cleanup.

Beyond that, resist scope creep. Directory reshuffles, build-system
modernization, and similar improvements are legitimate projects in their own
right, but they are *not* part of a version upgrade and should not ride along
with one.

---

## Phase 3 — Plan the Version Path

### Go one release at a time

Each Zephyr migration guide covers exactly **one hop** — from the immediately
preceding release to that release. There is no single "3.x → latest" guide.
Therefore:

- **Do not jump multiple releases in one step.** Walk the versions in order,
  applying each release's migration guide as you pass through it, even for the
  intermediate releases you don't intend to ship on.
- Treat each step as its own mini-project: bump one version, apply that
  version's guide, get back to green, commit, then move on.
- **LTS releases make good resting points** (they are supported longest and are
  where the surrounding ecosystem/HAL modules are most settled), so they are
  natural milestones to pause, ship, or hand off on — but the *guide-application*
  discipline is per release, not per LTS.

Present the proposed path (the ordered list of intermediate versions) to the
user and let them confirm the endpoints.

### Version-specific gates are looked up, not memorized

SDK requirements, newly-required modules, and toolchain minimums change per
release and go stale the moment they are written down. Do not rely on a static
table. For each hop, derive them from the target tree itself:

- SDK: the tree's declared SDK version (see Phase 1).
- Required/renamed modules: the target tree's `west.yml` (see Phase 4).
- Everything else: that release's migration guide and release notes.

---

## Phase 4 — Execute One Version Step

Repeat this loop for each version in the planned path.

1. **Move the Zephyr revision forward one step** in `west.yml` (or advance your
   fork branch to the new upstream tag). If you maintain custom Zephyr commits,
   re-base or cherry-pick them onto the new tag now:
   - A cherry-pick that comes out **empty** is a strong hint the change was
     absorbed upstream — but confirm by inspecting upstream history/content
     before dropping it, since a commit can also become empty because of
     unrelated prior changes.
   - A **clean** apply — carry it forward.
   - A **conflict** — resolve it, deciding whether upstream now supersedes or
     merely coexists with your change. The long-term goal is to *shrink* the
     custom set, not grow it.

   A manifest revision must be something west can *resolve* — a tag, branch, or
   commit reachable from a configured remote. If you carry **local** Zephyr
   commits that exist only on disk (no upstream fork branch at the new version),
   west.yml cannot point at them directly; publish the branch, add a local
   `file://`/path remote, or use a path override so the workspace is
   reproducible rather than depending on an unrecorded local checkout.
2. **`west update`, then diff the module set** (`west list`). HAL and library
   modules are pinned per Zephyr release; an old module revision against a new
   Zephyr produces subtle struct/API mismatches. Notes:
   - If the project **patches west modules at build time** (or you edited a
     module by hand), those modules are *dirty* and `west update` will refuse to
     move them or will silently skip them. Reset/clean the affected modules
     first, then re-apply patches after updating.
   - A revision the manifest itself pins can be wrong or unresolvable for the new
     hop (a module may have moved a branch/tag). Be prepared to correct a module
     revision, not just run a plain update.
   - If the project keeps a manifest `import` blocklist/allowlist, re-sync it
     against the new tree's `west.yml` — modules get added, removed, or renamed
     between releases.
3. **Read the migration guide and release notes for this hop** and apply them to
   your application, boards, DTS, and Kconfig. This is where the version-specific
   work lives; the guide is your checklist.
4. **Build clean, one target first.** After changing the Zephyr tree, any
   module, or the toolchain, always build in a **fresh/pristine build
   directory** (e.g. `west build -p always` or a new `-d` dir). Stale
   CMake/Kconfig/devicetree cache is a leading cause of *false* migration
   failures and of *phantom* successes. Build a single target first to surface
   configuration-time errors (Kconfig/DTS) before spending time compiling, then
   expand to all targets.
5. **Get back to green** on the validation method for every supported target.
6. **Commit** the step as one clean logical change (or a small set of logically
   grouped commits). Keep the version bump and any manifest/module re-sync
   together and clearly described. Use `git commit --fixup` + `git rebase -i
   --autosquash` while iterating, and **squash the fixups before the step is
   declared done** — the recorded history should read as an intentional
   migration, not a debugging diary.
7. **Tag/record the milestone**, then proceed to the next version.

---

## Phase 5 — Recurring Breaking-Change *Patterns* (not a change list)

The specific renames belong to the migration guides. What follows are the
**categories** of breakage that recur across projects and versions, so you know
what to look for and how to reason about each. For any specific symbol, confirm
the replacement in the guide for the relevant hop.

### 5.1 The board model ("hardware model")

Zephyr overhauled how boards and SoCs are defined. The change is structural and
touches **every** board, in-tree or out-of-tree. Symptoms when an old-style
board meets a newer Zephyr include "board does not support board qualifiers" and
an empty board directory causing a `Kconfig`/`defconfig` include to resolve to
nothing. The shape of the migration (consult the board-porting guide for exact
files and syntax):

- Add the board-description metadata file (identity: name, vendor, SoC list, and
  a human-readable full name — a missing full name can fail with a confusing
  CMake error).
- Move SoC/series *selection* into the board's Kconfig, and **remove the symbols
  that are now auto-generated** from the board `defconfig` (selecting them by
  hand duplicates and conflicts with the generated ones).
- Boards may now be addressed with **qualifiers** identifying the SoC (and, for
  multi-core/variant parts, the cluster/variant); a single-core single-SoC board
  can still be named on its own. Derive the canonical target from `west boards`.
  Board-specific overlay/config files are matched by the **board target name**,
  so renaming a board ripples into every shield/overlay that names it.
- Migrate and validate **one board at a time**. On a project with a large board
  set this is the bulk of the work; do it incrementally rather than all at once.

### 5.2 Kconfig/CMake variable renames ripple into copied snippets

Build-system variables get renamed across releases (a board-directory variable,
for instance). This bites hardest in projects that **copied a snippet out of
Zephyr's own `Kconfig.zephyr`/CMake** into their application: the copy keeps the
old variable name, which now expands to empty and fails an include. When a build
variable "went missing", check whether Zephyr renamed it and whether your app is
carrying a stale copy of upstream glue. Prefer optional includes (`osource`)
where upstream does.

### 5.3 Deprecations become errors — and warnings-as-errors makes it abrupt

A symbol is typically *deprecated* one release and *removed* a few releases
later. Two implications:

- Sweep deprecation warnings at **every** step, not just when something finally
  breaks. The deprecation warning tells you the replacement while the old symbol
  still works — the cheapest possible time to migrate.
- Many projects build with warnings treated as errors (including Kconfig
  warnings). Under that setting a *deprecation warning* aborts the build
  outright. Expect a formerly-warning to become a hard stop after an upgrade,
  and migrate rather than silence it.

Whole subsystems get replaced this way (an old subsystem gaining a compatibility
shim, then the shim and its headers being deleted a couple of releases later).
When you see a subsystem-wide deprecation, plan the real migration early — the
shim will not be there forever, and it usually spans DTS + Kconfig + C together.
And note the direction of dependency: once upstream *deletes* a subsystem (its
headers, bindings, and Kconfig), a project that still relies on it must either
finish migrating to the replacement or **carry its own local compatibility
shim** (header/binding/Kconfig/library). "The shim was removed" is not just an
API rename — it can mean *you* now own the compatibility layer.

### 5.4 Downstream overrides and patches of upstream break *silently*

A project that sets a **default** for an *upstream* Kconfig symbol (to tune a
library or subsystem) depends on that symbol continuing to exist. When upstream
renames or removes it, the override becomes a definition with no type
(`"<SYMBOL>" defined without a type`) or simply stops having any effect. These
are easy to miss because they are *your* lines failing, not Zephyr's. After a
bump, reconcile every downstream override against the current upstream symbol —
find the new name, or drop the override if the knob is gone.

The same hazard applies to any downstream customization that hooks into
upstream **build glue**, not just Kconfig defaults: patches to Zephyr's CMake
(custom board/shield hooks, module `CMakeLists` injection), to linker fragments,
or to west extension commands. When upstream restructures the glue you patched,
your patch conflicts or silently no-ops. Inventory *everything* you override or
patch in upstream — Kconfig, CMake, DTS, linker — and re-check each one every
hop.

A close cousin is build glue you don't *patch* but merely *depend on*: a
project-local `CMakeLists`/`.cmake` that `include()`s an upstream file **by
path**, or that attaches a hook to a named upstream **extension point** (a
pre/post board or shield step, a devicetree pre-step, etc.). An upgrade can
relocate the file or rename/re-sequence the extension point. The path case fails
loudly (`include could not find ...`) and is easy to chase. The extension-point
case is nastier: the build still *succeeds*, but your hook silently never fires,
so its behavior quietly disappears with no error to follow. So don't only audit
what you patch — also audit what you *reference* and what you *hook*: confirm
each referenced upstream path still exists, and assert that each hook actually
runs (check its side effect or the build log) rather than assuming it did.

### 5.5 DTS strictness increases over time

Devicetree tooling gets stricter across releases: property-name normalization is
removed, checks start applying to *all* nodes (including `disabled` ones), and
bindings are enforced more literally. A single build often surfaces DTS errors
for nodes your board doesn't even enable — including nodes in shared SoC `.dtsi`
files. Build one target early specifically to flush these out, and be prepared
to fix (or, for a fork, patch) shared DTSI that older, laxer tooling tolerated.

The tightening extends to *your own* out-of-tree **bindings** and their headers,
not just DTS content. Newer tooling validates custom binding schemas more
literally (property names, types, `compatible` strings) and requires any
binding-companion header that gets pulled into C to be **C-safe** — a header the
generator once tolerated can start failing a C parse. The signal is a
configuration-time failure in the devicetree/bindings step, or a parse error
whose origin is a *generated or included binding header* rather than application
code. Fix the binding or header to the current schema rather than loosening the
tooling to accept the old form.

### 5.6 C API and header churn

Function/macro renames, header moves, and driver-API-registration changes are
routine. The migration guide lists them per hop; the compiler finds the rest.
Keep the fixes mechanical and per-symbol, and lean on "build one target, read
the first fatal error, fix, repeat". Watch for the inverse of a rename too: a
new upstream release can **add** a symbol (a macro, type, or function) whose
name *collides* with one your project already defines, turning previously-fine
code into a redefinition error. The fix is usually to rename or namespace your
own definition, or adopt the upstream one.

### 5.7 Toolchain/host and standard-library shifts

Independently of your code, the C library and toolchain shift (e.g.
retargetable-locking and libc details). These manifest as ABI/`conflicting
types` errors and are almost always an **SDK/tree version mismatch** (Phase 1),
not a bug in your project — fix the environment, not the symptom.

### 5.8 Downstream distributions add their own layer

Some projects do not build directly on upstream Zephyr but on a **downstream
distribution** that repackages it (a vendor SDK or similar). Such distributions
add their own machinery on top of Zephyr — additional build orchestration,
their own board/partition/image configuration, and vendor-specific modules and
manifests — all of which must be migrated *alongside* the underlying Zephyr
version. Two rules: keep that machinery per target, and read the
**distribution's own** release notes and migration documentation *in addition
to* Zephyr's. Do not assume upstream Zephyr instructions map one-to-one onto a
downstream distribution, or vice versa.

---

## Phase 6 — Per-Step Checklist

After each version step, before calling it done:

- [ ] Every supported target builds.
- [ ] Every target passes the project's validation method.
- [ ] No new warnings (or each is documented and understood); remaining
      deprecations are noted for the next step.
- [ ] `west.yml` reviewed: Zephyr revision and every module revision are
      consistent with this target Zephyr; any import blocklist/allowlist
      re-synced.
- [ ] History is clean: version bump separate from cleanup, fixups squashed, the
      commit message explains *what changed and why*.
- [ ] Milestone tagged/recorded.
- [ ] Deferred (non-regression) issues written down explicitly rather than
      quietly carried.

---

## Guiding Principles

1. **The migration guide is the source of truth; this skill is the method.**
   Never hand-maintain a parallel list of version-specific changes here — read
   the guide for the hop. If the guide is wrong or incomplete, fix the guide.
2. **One version at a time.** Never chase two releases' worth of breaking
   changes at once.
3. **One target at a time** when a change is structural. Per-board issues are
   usually a single DTS/Kconfig mismatch specific to that SoC or overlay.
4. **The environment is a first-class variable.** SDK and host-tool versions
   move with the tree; verify them at every step and suspect them first when the
   failure is deep in libc/CMake rather than in your code.
5. **Keep customizations minimal and shrinking.** At each step, check whether a
   custom Zephyr change was absorbed upstream (empty cherry-pick) and drop it.
6. **Reconcile downstream overrides and patches.** Anything you override or
   patch in upstream (Kconfig defaults, CMake/build glue, DTS, linker) is a
   liability across an upgrade — inventory each one and re-check it every hop.
7. **Clean history over minimal history.** A migration that is easy to bisect,
   review, and roll back is worth more than the fewest commits. Separate
   concerns; squash noise.
8. **Document deferrals, don't hide them.** A pre-existing, non-regression issue
   should be recorded and moved past — not conflated with migration work.

---

## Useful Commands Reference

```bash
# What Zephyr revision is pinned?
grep -n revision west.yml            # or inspect your fork branch

# What SDK does this tree expect? (compare to what you have installed)
cat zephyr/SDK_VERSION 2>/dev/null

# Point the build at a specific SDK for the current version step
ZEPHYR_SDK_INSTALL_DIR=/path/to/matching/sdk west build -b <board> -p always

# See the module set and diff it after a bump
west list

# Update modules to match the new manifest
west update            # or: west update <module>

# Triage a carried Zephyr patch (empty apply == absorbed upstream)
git cherry-pick --allow-empty <sha> && git show --stat HEAD

# Keep the step's history clean, then squash before finishing
git commit --fixup <sha>
git rebase -i --autosquash <base>

# Read the authoritative change lists for a hop, in the target tree
#   doc/releases/migration-guide-<version>.rst
#   doc/releases/release-notes-<version>.rst
#   plus the board-porting / hardware-model documentation for board work
```

---

## Maintaining and extending this skill

*This section is for an agent or human asked to **update this skill** — not for a
migration in progress. New Zephyr releases will surface new breakages; resist the
urge to paste them in raw. This skill stays useful only if every edit preserves
the properties below.*

1. **Stay evergreen.** Do not add version/SDK tables or per-release change lists.
   Anything specific to one hop belongs in Zephyr's own migration and release
   notes — link to the method of finding them, never mirror their contents here.
2. **Stay project- and vendor-neutral.** No board, SoC, product, vendor, or
   symbol names, and no downstream-distribution specifics. Describe the *shape*
   of a problem, not the instance you happened to hit. Before committing, run a
   leakage self-check (grep for vendor/board/project names and `X.Y.Z` version
   numbers) and confirm only section numbers match.
3. **Clear the recurrence bar before adding a pattern.** A breakage earns a place
   in Phase 5 only if it recurs across **two or more unrelated projects**. A
   single-project observation is a candidate to watch, not a rule to encode —
   leaving it out is the correct default.
4. **Validate edits empirically, not from one anecdote.** The credible way to
   evolve this skill is to re-run real upgrades on several unrelated projects
   (including a held-out project the skill was *not* tuned on), capture where the
   skill helped versus stayed silent, and fold in only what proves generic across
   them. A change justified by a single port is probably a leak.
5. **Anonymize on the way in.** Rewrite each new lesson as *shape → symptom →
   signal → fix* with all identifiers stripped, so it reads as general
   documentation a human could learn from.
6. **Preserve the framing.** Phase 5 is a catalog of recurring *patterns*, not a
   change list. Keep the prose readable as standalone documentation, and keep the
   division of labor explicit: the migration guide is the source of truth, this
   skill is the method.
