.. _twister_test_results:

Hardware Test Results Document
##############################

Twister can write a results document, :file:`zephyr-results.json`, next to
:file:`twister.json`. It describes one test run on real boards in a fixed,
versioned format, so that results from different labs can be stored, compared
and queried in the same way.

Each result names the exact image that was flashed by its SHA-256, the board
target, the lab and the commit. Lab and infrastructure failures get their own
status, so they are kept apart from test failures.

The format follows the KCIDB object model
(checkouts, builds, tests) and is defined by
:zephyr_file:`scripts/schemas/twister/results-schema.yaml`. Fields specific to
Zephyr live in closed ``misc.zephyr`` objects. The current version is 0.1, a
draft: a later 0.x version may change the format.

Making a document
*****************

Run twister with ``--report-ci`` and an origin, the organisation that submits
the results:

.. code-block:: console

   west twister --device-testing --hardware-map map.yaml -p <board> -T <tests> \
       --report-ci --report-ci-origin examplelab --report-ci-lab lab1

The options are:

``--report-ci``
    Write :file:`zephyr-results.json` next to :file:`twister.json`. The file name
    takes the ``--report-suffix`` if one is given.

``--report-ci-origin ORIGIN``
    Required. Lower case letters, digits and ``_``.

``--report-ci-lab LAB``
    The lab of the origin where the tests ran. Leave it out for build-only runs.

``--report-ci-trigger``
    One of ``pr``, ``merge_queue``, ``push``, ``nightly``, ``rc``, ``release``,
    ``manual``. By default it is taken from the CI event, else ``manual``.

``--report-ci-lab-events FILE``
    Lab facts per test suite, see `Lab events`_. Twister stops at start-up if the
    file is missing or is not JSON.

``--report-ci-flash-file NAME``
    The image file the lab flashes, for example ``zephyr.hex``. By default the
    first of :file:`zephyr.elf`, :file:`zephyr.hex`, :file:`zephyr.bin` and
    :file:`zephyr.exe` that the build produced.

The CI context (repository, commit, branch or tag, run id and URL, PR number) is
read from the GitHub Actions or GitLab CI environment. Outside CI it is taken
from the git checkout of ``ZEPHYR_BASE``.

Path values in the recorded twister command line, and configuration files outside
``ZEPHYR_BASE``, are reduced to their file name.

The build ``provenance`` is ``ci`` only for the origin ``zephyr`` (Zephyr CI),
and ``local`` for every other origin.

The image digests come from the ``artifacts`` that twister records in
:file:`twister.json`. Filtered test suites and suites without build artifacts
(for example build errors) are not in the document.

Checking a document
*******************

Twister checks the document after writing it and logs the first 20 errors. An
invalid document is still written. Neither an invalid document nor a failure to
write it changes the exit code of twister. Labs that do not run twister can check
a document with:

.. code-block:: console

   ./scripts/ci/test_results/validate.py zephyr-results.json

Each error is printed as ``path: [rule] message``. The exit status is 0 when all
files are valid.

Document content
****************

Submission header, one per document:

.. code-block:: json

   {
     "format": "zephyr-results",
     "version": {"major": 0, "minor": 1},
     "submission": {
       "id": "examplelab:1234:hil",
       "origin": "examplelab",
       "lab": "lab1",
       "created": "2026-11-17T05:02:59+00:00",
       "context": {
         "trigger": "manual",
         "commit": "5dd96bc95cb1db3a6e0b17ebe3e894ed1c42acd7",
         "repository": "https://git.example/zephyr.git",
         "run_url": "https://ci.example/runs/1234"
       },
       "summary": {"status": "FAIL", "suites": {"FAIL": 1, "MISS": 2, "PASS": 9}}
     }
   }

The validator checks that the summary matches the tests.

A build is one image, a test scenario on one board target. Exactly one artifact
is marked as the flashed file:

.. code-block:: json

   {
     "id": "examplelab:1234:hil:kit_t2g_b_h_evk/cyt4bfbche/m7_0:kernel.timer",
     "config_name": "kit_t2g_b_h_evk/cyt4bfbche/m7_0",
     "status": "PASS",
     "misc": {"zephyr": {
       "target": {"board": "kit_t2g_b_h_evk", "soc": "cyt4bfbche", "cpucluster": "m7_0"},
       "scenario": "kernel.timer",
       "artifacts": [
         {"name": "zephyr.elf", "flash": true, "size": 2277936,
          "sha256": "1636d3311c1deb1654e3bec0bb9958da66543a52f5abf06453bbadeca8d2e154"},
         {"name": "zephyr.hex", "size": 371545,
          "sha256": "3e27fd18dcf9355dbb0a39195782993c4934ca8b6a94caaa22523421682113a1"}
       ]
     }}
   }

A test suite names the lab, the board and the image it flashed. The sha256 must
equal the flashed artifact of its build:

.. code-block:: json

   {
     "id": "examplelab:1234:hil:kit_t2g_b_h_lite/cyt4bf8cds/m7_0:kernel.timer@lab1",
     "build_id": "examplelab:1234:hil:kit_t2g_b_h_lite/cyt4bf8cds/m7_0:kernel.timer",
     "path": "kernel.timer",
     "status": "PASS",
     "environment": {"misc": {"zephyr": {"lab": "lab1",
       "dut": {"board_tag": "kit_t2g_b_h_lite-0001"}}}},
     "misc": {"zephyr": {"kind": "suite",
       "artifact": {"name": "zephyr.elf", "size": 2253424,
         "sha256": "c104ec1b9a59488e49bd34be1f5a2072d9f055b4a011bd67c6d2a3d002ad8d19"},
       "counts": {"PASS": 104}}}
   }

The board tag is the hardware map id of the board (``--hardware-map``).

A suite the lab could not run:

.. code-block:: json

   {
     "id": "examplelab:1234:hil:kit_t2g_b_h_evk/cyt4bfbche/m7_0:kernel.timer@lab1",
     "path": "kernel.timer",
     "status": "MISS",
     "comment": "Timeout during flashing",
     "misc": {"zephyr": {"kind": "suite",
       "failure_class": {"reason": "flash", "source": "twister",
                         "detail": "Timeout during flashing"},
       "counts": {"MISS": 20}}}
   }

A test case keeps its original twister id:

.. code-block:: json

   {
     "id": "examplelab:1234:hil:cytvii_b_e_1m_100_cpu/cyt2b75cae/m4:kernel.timer.time_conversions_conversions_ms_to_cyc_floor32_@lab1",
     "path": "kernel.timer.time_conversions_conversions_ms_to_cyc_floor32_",
     "status": "PASS",
     "misc": {"zephyr": {"kind": "testcase",
       "original_id": "kernel.timer.time_conversions[conversions/ms_to_cyc_floor32]",
       "suite_id": "examplelab:1234:hil:cytvii_b_e_1m_100_cpu/cyt2b75cae/m4:kernel.timer@lab1"}}
   }

Statuses and reasons
********************

.. list-table::
   :header-rows: 1

   * - Status
     - Meaning
     - ``failure_class.reason`` or ``skip_reason.reason``
   * - PASS
     - the test passed
     - none
   * - FAIL
     - the test failed
     - optional: assertion, fault, timeout, unexpected_output, other
   * - ERROR
     - the test could not complete
     - optional: blocked, incomplete, harness, timeout, no_output, other
   * - MISS
     - the lab or infrastructure failed
     - required: reservation, flash, console, power, probe, timeout,
       lab_offline, artifact, artifact_mismatch, harness, aborted, blocked,
       other
   * - SKIP
     - the test did not run
     - required: built_only, filtered, excluded, harness_skip, not_applicable

Twister statuses map as follows:

- ``passed`` is PASS, ``skipped`` is SKIP harness_skip, ``not run`` is SKIP
  built_only.
- ``failed`` is FAIL. The reasons ``Timeout during flashing`` and
  ``Serial Device Error`` give MISS flash and MISS console.
- ``error`` is ERROR. ``Device issue (...)`` flash errors give MISS flash.
- A ``blocked`` test case is MISS when its suite is MISS, else ERROR.

MISS ``console`` and ``timeout`` need a recorded lab event (``source`` lab),
except MISS console from twister's ``Serial Device Error``. A suite status is
the highest of its own and its test cases' statuses, in the order FAIL, ERROR,
MISS, PASS, SKIP.

Lab events
==========

Twister alone cannot tell a lab failure from a test failure in all cases. A lab
can pass what it recorded with ``--report-ci-lab-events``. The file is kept
small on purpose: it carries only what the rules above need, the flashed image
digest and the id of a recorded lab event.

.. code-block:: json

   {
     "tool": {"name": "labtool", "version": "1.2.0"},
     "suites": [
       {"platform": "kit_t2g_b_h_lite/cyt4bf8cds/m7_0", "scenario": "kernel.timer",
        "flashed_sha256": "c104ec1b9a59488e49bd34be1f5a2072d9f055b4a011bd67c6d2a3d002ad8d19"},
       {"platform": "cytvii_b_e_1m_100_cpu/cyt2b75cae/m4", "scenario": "sample.basic.helloworld",
        "event": {"reason": "console", "id": "job-209", "url": "https://lab.example/jobs/209"}}
     ]
   }

- ``flashed_sha256`` is the digest of the file the lab flashed. When it differs
  from the build, the suite becomes MISS artifact_mismatch with source ``lab``.
  The two digests are the evidence, so no event id is needed.
- ``event`` turns a failed suite into MISS with that reason. It never changes a
  suite that passed.
- ``tool`` names the lab tool in every suite.

Ids and paths
*************

Ids are built only from the origin, the run, the job, the board target, the test
path and the lab, so sending the same results again gives the same ids:

- checkout: ``<origin>:<run>``
- build: ``<origin>:<run>:<job>:<board target>:<scenario>``
- test: ``<origin>:<run>:<job>:<board target>:<path>@<lab>``; without a lab
  (build only) there is no ``@<lab>``

The test path is the twister id made safe for KCIDB: it is split at ``.``
outside of ``[]``, and every other character outside ``[A-Za-z0-9_-]`` becomes
``_``. A test case path is the suite path, ``.``, and the sanitized rest of the
test case id. When two different ids in one build give the same path, the
changed ones get ``_`` and the first 8 hex digits of the SHA-256 of the original
id appended. The original id is kept in ``misc.zephyr.original_id``.

A document is at most 16 MiB. ``log_excerpt`` holds at most 16384 characters,
and only on FAIL, ERROR and MISS.
