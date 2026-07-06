# Architectural layering analysis (standalone)

Reproduces the *method* of the ECLAIR `B.INDEPENDENCE` architectural-constraint
service shown in the Zephyr Safety WG talk "Verification of Software
Architectural Constraints for Zephyr" — but depends on **nothing beyond the
Zephyr SDK binutils** (`objdump`, `nm`) already used to build the image. No
commercial static analyzer is required.

It answers: *does the built image respect a declared component layering, and
where are the violations?* — and draws the same green/red dependency graph.

## How it works

1. Walk a Zephyr build directory for object files (`*.obj` / `*.o`).
2. Recover each object's source path from `compile_commands.json` and map it to
   a **component** — its area in the top-level `MAINTAINERS.yml`, parsed with the
   in-tree `scripts/get_maintainer.py`. Components are the project's own
   maintained areas (e.g. *Kernel*, *ARM arch*, *Heap Management*, *Drivers:
   System timer*), not a hand-written list.
3. `nm` builds a global map: defined symbol → component.
4. `objdump -dr` reads each object's relocations — the calls and data
   references the linker resolves — and attributes each to an inter-component
   edge (from = object's component, to = component defining the target symbol).
5. Each cross-component edge is checked against the scope model: the independence
   rule is *in-scope code must not call or read out-of-scope code*. Anything that
   breaks it is a violation.
6. Outputs: `graph.dot` / `graph.png` (green = allowed, red = violation),
   `report.txt`, and `report.json`.

## Usage

```sh
west build -p always -b frdm_mcxn947/mcxn947/cpu0 samples/hello_world -d build
python3 scripts/arch_analysis/arch_layers.py \
    --build build \
    --config scripts/arch_analysis/zephyr_scope.yaml \
    --out-dir build/arch_analysis
```

`objdump`/`nm` are auto-detected from the build's `CMakeCache.txt`; override
with `--objdump` / `--nm` if needed.

## Interactive web app

For exploration, `serve.py` runs a local web app on top of a real build. The
expensive extraction runs once server-side; the scope/violation decision is
client-side, so you can experiment interactively:

```sh
python3 scripts/arch_analysis/serve.py \
    --build build --config scripts/arch_analysis/zephyr_scope.yaml --port 8080
# open http://localhost:8080  (stdlib http server, no framework / CDN)
```

Features:

* **Interactive scope** — toggle any MAINTAINERS area in/out of the qualification
  scope from the left rail (or the detail panel). Violations recompute instantly.
  Each area shows a **Δ** = how many violation references would appear/disappear
  if you flipped it, so you can directly answer the talk's slide-26 questions
  ("can the kernel be qualified without X?"). The answer is quantified: e.g. the
  areas that most reduce violations for a Kernel-only scope are *Drivers: System
  timer* and *Heap Management*, while pulling in a leaf area can *add* violations
  by dragging in its own downward dependencies (libgcc, HAL).
* **Three views** of the same live data:
  * *Graph* — the force-directed area dependency graph (green allowed / red
    violation), draggable, click to inspect.
  * *Matrix* — a from×to interaction matrix, in-scope areas first, so a
    self-contained scope shows a clean green top-left block.
  * *Scope* — a boundary view: in-scope vs out-of-scope columns with the
    forbidden-direction dependencies drawn crossing the line.
* **Drill-down** — click any area for its files, top directories and in/out
  edges; click any edge for the concrete target symbols behind it.
* **Reload build** re-runs the analysis. **Copy scope as YAML** exports your
  chosen `in_scope` list back into the config.

The independence rule used everywhere (CLI and web app) is the talk's core one:
*in-scope code must not call or read out-of-scope code.*

## The model

The component partition is **not** maintained here — it is the set of areas in
the top-level `MAINTAINERS.yml`. Each source file is assigned to its most
specific matching area (longest literal `files` prefix wins when several match).
Files outside the Zephyr repo (HAL modules) become `module: <name>`; in-repo
files matching no area become `UNMAINTAINED`; unresolved toolchain symbols are
`EXTERNAL`.

`zephyr_scope.yaml` therefore only declares the **qualification scope** — the
list of areas that are in-scope (default: `Kernel`) — plus an optional override
for the maintainers file. Editing that list (or toggling in the web app) is how
you explore the layering policy; no code changes.

## Validated against the talk

On `samples/hello_world` + `frdm_mcxn947/mcxn947/cpu0` (the talk's exact
target), with only *Kernel* in-scope, the tool reproduces the qualitative
findings — now attributed to precise maintained areas:

| Talk (slide 24–25)                              | This tool (MAINTAINERS area)        |
|-------------------------------------------------|-------------------------------------|
| `IN_SCOPE → ARCH`                               | `Kernel → ARM arch` (`arch_*`)      |
| `IN_SCOPE → DRIVERS_TIMER`                      | `Kernel → Drivers: System timer`    |
| `IN_SCOPE → OUT_OF_SCOPE`: `sys_heap`           | `Kernel → Heap Management`          |
| `IN_SCOPE → OUT_OF_SCOPE`: data structures      | `Kernel → Utilities` (`ring_buf`)   |
| `OUT_OF_SCOPE → HAL`: `board_early_init_hook`   | `Kernel → NXP Platforms (MCU)`      |

## Known limitations

* **Inlining under-counts.** Object-level relocations miss calls the optimizer
  inlined away. Header `static inline` primitives (dlist/slist/rbtree,
  `arch_irq_lock`, ...) therefore show far lower counts than the talk's
  source-level ECLAIR run (e.g. `Kernel → Utilities` shows a handful here vs. the
  hundreds a source-level pass would see). Cross-component API calls that cross a
  *link* boundary are fully preserved — the layering *shape* is correct; some
  inlined-primitive *magnitudes* are not. A source-level backend (libclang over
  `compile_commands.json`) would close this gap and is the intended v2.
* **`ref` conflates read and write.** The talk's integrity story (write
  forbidden) needs per-instruction load/store decoding, not yet implemented.
  Layering (`call`/`ref`) does not need it.
* Analyzes one build (one board/config) at a time — run per target of interest.
