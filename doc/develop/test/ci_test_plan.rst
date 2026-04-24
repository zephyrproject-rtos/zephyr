.. _ci_test_plan:

CI Test Plan Selector (test_plan_v2)
#####################################

The CI test plan selector is a modular Python script located at
``scripts/ci/test_plan_v2.py``.  Its purpose is to analyse the set of files
changed in a Pull Request and emit a targeted twister test plan — exercising
only the tests that are plausibly affected by the change while avoiding a
full tree-wide run on every commit.

The script writes two artefacts:

* ``testplan.json`` — passed to twister via ``twister --load-tests``.
* ``.testplan`` — a plain ``KEY=value`` environment file consumed by CI
  orchestration scripts, containing ``TWISTER_TESTS``, ``TWISTER_NODES``, and
  ``TWISTER_FULL``.

Architecture
************

The selector is built around a *strategy pipeline*.  Each strategy is an
independent analyser that:

1. Receives the list of changed files (or the subset not yet consumed by
   earlier strategies).
2. Inspects those files according to its own logic.
3. Returns a list of :class:`TwisterCall` descriptors and the set of files it
   has *handled*.

The :class:`Orchestrator` drives the pipeline, merges all results into a
:class:`PlanAccumulator`, deduplicates test suites, and writes the output
files.

Strategy ordering
*****************

Strategies run in a fixed order that reflects two principles:

**Specificity over generality.**
The most precise strategies run first.  A file that lives inside
``tests/kernel/sched/`` is handled definitively by :class:`DirectTestStrategy`
(run exactly those tests) before the catch-all :class:`MaintainerAreaStrategy`
can add an entire Kernel area sweep.

**Consume-before-additive.**
Consuming strategies run before additive ones.  Once a consuming strategy
claims a file, downstream strategies never see it.  This prevents a change
to a board file from also triggering a driver compat scan and a Kconfig sweep
for the same path.

The current order is:

.. list-table::
   :header-rows: 1
   :widths: 5 20 10 65

   * - #
     - Strategy
     - Consumes
     - Rationale
   * - 0
     - :class:`ComplexityStrategy`
     - No
     - Scores the patchset using pydriller and lizard.  Must run first so the
       score is available to :class:`RiskClassifierStrategy`.  Emits no twister
       calls.
   * - 1
     - :class:`IgnoreStrategy`
     - Yes
     - Silently drops files that can never affect tests (documentation, CI
       workflows, tooling).  Runs early so later strategies never waste effort
       on ignored paths.
   * - 2
     - :class:`DirectTestStrategy`
     - Yes
     - A changed test or sample file must trigger *only* that test, not an
       entire area-wide sweep.  Consuming this file prevents
       :class:`MaintainerAreaStrategy` from also adding every test in the
       Kernel area when ``tests/kernel/sched/main.c`` is modified.
   * - 3
     - :class:`SnippetStrategy`
     - Yes
     - Snippet changes are self-contained: only tests that declare the snippet
       in ``required_snippets`` need to run.  Consuming prevents downstream
       strategies from treating a snippet YAML as an unknown configuration
       file.
   * - 4
     - :class:`BoardStrategy`
     - Yes
     - Board-specific changes require a targeted integration test on every
       board variant.  Consuming prevents a board ``.yaml`` file from also
       matching Kconfig and Header strategies.
   * - 5
     - :class:`SoCStrategy`
     - Yes
     - SoC-level changes affect every board built on that SoC family.
       Consuming prevents the same ``soc/`` path from reaching the Kconfig
       sweep.
   * - 6
     - :class:`ManifestStrategy`
     - Yes
     - A changed ``west.yml`` requires module-tagged integration tests, not a
       generic maintainer-area run.  Consuming the manifest file prevents the
       catch-all from adding unrelated tests.
   * - 7
     - :class:`DriverCompatStrategy`
     - No
     - Additive: driver files may also be covered by area patterns, so the
       file is not consumed and :class:`KconfigImpactStrategy` and
       :class:`MaintainerAreaStrategy` can add further coverage.
   * - 8
     - :class:`DtsBindingStrategy`
     - No
     - Additive: binding changes combine overlay-based test selection with
       board-targeted area calls.
   * - 9
     - :class:`KconfigImpactStrategy`
     - No
     - Additive: Kconfig changes may affect many unrelated files; only consumes
       when a non-widespread symbol is found, leaving widespread-symbol files
       for the catch-all.
   * - 10
     - :class:`HeaderImpactStrategy`
     - No
     - Additive: header changes trace include users back to maintainer areas.
       Widespread headers (included in more than the configured threshold) are
       skipped.
   * - 11
     - :class:`MaintainerAreaStrategy`
     - No
     - Catch-all: matches any remaining file against ``MAINTAINERS.yml`` area
       patterns and emits ``--test-pattern`` calls for each matching area that
       has a non-empty ``tests:`` list.

Consume vs. additive behaviour
*******************************

Every strategy subclass carries a class attribute ``consumes: bool``.

When ``consumes = True``:
  Files returned in the *handled* set are removed from the ``remaining`` pool
  before the next strategy runs.  Use this when the strategy is *authoritative*
  for its file type — i.e. when seeing the file in a later strategy would
  produce redundant or incorrect results.

When ``consumes = False`` (default):
  Files remain in the pool regardless of what the strategy returns.  All
  downstream strategies receive the same file list.  Use this for additive
  strategies that contribute *additional* test coverage without claiming
  exclusive ownership.

.. note::

   The :class:`Orchestrator` skips a strategy entirely when the ``remaining``
   pool is empty.  Because non-consuming strategies see all files, "remaining"
   only shrinks through consuming strategies.  As soon as the pool is empty
   the orchestrator stops calling strategies.

Full-run and fallback conditions
**********************************

The orchestrator signals ``TWISTER_FULL=True`` in ``.testplan`` when either of
the following is true:

1. **Unresolved files.** After all strategies have run, at least one changed
   file was not *handled* by any strategy.  Rather than silently skipping
   coverage for an unknown path, the script falls back to requesting a full
   run so no regression slips through.

2. **Explicit full-run signal.** A strategy returns a :class:`TwisterCall`
   with ``full_run=True``.  When the orchestrator encounters such a call, it
   immediately sets ``TWISTER_FULL=True``, empties the remaining pool, and
   stops executing further calls.  This mechanism allows a strategy to opt out
   of targeted selection (e.g. when a core subsystem header used by the entire
   tree is modified).

When ``TWISTER_FULL=True`` the CI script is expected to discard
``testplan.json`` and run twister without ``--load-tests``.

Node count calculation
**********************

``TWISTER_NODES`` in ``.testplan`` is computed as follows:

* ``0`` — no tests were selected.
* ``1`` — fewer than ``--tests-per-builder`` tests were selected (fits in one
  builder).
* ``ceil(total / tests_per_builder)`` — otherwise.  The ceiling ensures that
  no builder is over-capacity when the division is not exact.

The default value of ``--tests-per-builder`` is ``900``.

Adding a new strategy
*********************

1. **Subclass** :class:`SelectionStrategy` and implement the two abstract
   members:

   .. code-block:: python

      class MyStrategy(SelectionStrategy):

          consumes: bool = False  # or True if authoritative

          @property
          def name(self):
              return "MyStrategy"

          def analyze(self, changed_files):
              # inspect changed_files
              calls = [...]       # list of TwisterCall
              handled = {...}     # subset of changed_files this strategy owns
              return calls, handled

2. **Decide consume vs. additive.**  Ask: "If a downstream strategy also sees
   this file, will it produce useful additional coverage, or redundant/wrong
   results?"  If redundant/wrong: ``consumes = True``.

3. **Insert in the correct position** in :func:`build_strategies`.  As a rule
   of thumb:

   * Consuming strategies belong before all additive strategies.
   * More-specific strategies belong before less-specific ones.
   * Strategies with no side-effects on the file pool (additive) can be
     ordered by cost — cheapest first.

4. **Guard optional dependencies** with a late import inside ``analyze`` and
   return ``[], set()`` gracefully when the dependency is missing.  Use the
   ``# noqa: PLC0415`` comment to silence the late-import warning.

5. **Write unit tests** in ``scripts/tests/ci/test_test_plan_v2.py`` covering:

   * The happy path (files correctly identified and routed).
   * The no-op path (irrelevant files produce no calls and no handled set).
   * Edge cases (missing files on disk, malformed YAML, empty inputs).

Shared pipeline context
***********************

:class:`PipelineContext` is a dataclass threaded through all strategies via
:func:`build_strategies`.  It currently carries:

* ``complexity_score`` — aggregate patchset score written by
  :class:`ComplexityStrategy` and read by :class:`RiskClassifierStrategy`.
* ``file_metrics`` — per-file :class:`ComplexityMetrics` mappings for
  detailed logging and downstream decision-making.

New cross-strategy state should be added as typed fields on
:class:`PipelineContext` rather than as strategy-level instance variables.

Test strategy for validating selection behaviour
*************************************************

The test suite lives in ``scripts/tests/ci/test_test_plan_v2.py`` and is run
with pytest:

.. code-block:: bash

   pytest scripts/tests/ci/test_test_plan_v2.py -v

Tests are organised into one class per strategy or shared component.  The
following categories of tests are expected for each new strategy:

**Unit tests (no Zephyr tree required)**
  Use a ``tmp_path`` fixture to create the minimum filesystem structures
  (``board.yml``, ``snippet.yml``, ``testcase.yaml``, etc.) needed to exercise
  each code path.  These tests should be fast and hermetic.

**Helper method tests**
  Test internal methods (path-walking, YAML parsing, regex extraction) in
  isolation.  Pass synthetic content rather than relying on real files in the
  repository.

**Orchestrator integration tests**
  Use mock strategies and a mock :class:`TwisterExecutor` to test the
  orchestrator's consume-vs-additive logic, full-run signalling, and
  ``.testplan`` output without invoking twister.

**Real-tree integration tests** (optional, ``@pytest.mark.integration``)
  May be added and marked with ``@pytest.mark.integration`` to be excluded
  from the default run.  These are only meaningful on a complete Zephyr
  checkout.

Minimal test checklist for a new strategy
==========================================

* The strategy returns no calls and no handled files when no relevant files
  are in the input list.
* The strategy correctly identifies and returns the expected :class:`TwisterCall`
  for a known-good input.
* The strategy does not crash when required files are absent from the filesystem.
* For consuming strategies: the handled set equals the matched files and
  nothing else.
* For additive strategies: the handled set is empty or equals only the files
  the strategy is authoritative for.

CLI reference
*************

.. code-block:: text

   usage: test_plan_v2.py [-c A..B] [-m FILE] [-f PATH]
                          [-o FILE] [-p PLATFORM] [--maintainers-file FILE]
                          [-T DIR] [--quarantine-list FILE]
                          [--tests-per-builder N] [--disable-strategy NAME]
                          [--detailed-test-id]

   -c A..B              Git commit range (e.g. ``main..HEAD``).  Changed files
                        are derived from ``git diff --name-only A..B``.
   -m FILE              JSON file containing a list of changed file paths.
   -f PATH              Treat PATH as a changed file (repeatable).
   -o FILE              Output JSON file (default: ``testplan.json``).
   -p PLATFORM          Restrict all selections to this platform (repeatable).
   --maintainers-file   Path to ``MAINTAINERS.yml``.
   -T DIR               Extra testsuite root forwarded to every twister call.
   --quarantine-list    Quarantine YAML forwarded to twister.
   --tests-per-builder  Tests per CI builder node (default: 900).
   --disable-strategy   Skip a strategy by name (repeatable).
   --detailed-test-id   Pass ``--detailed-test-id`` to twister.
