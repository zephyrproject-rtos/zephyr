DT Doctor tests
###############

Test suite for the DT Doctor static analysis tool
(``scripts/dts/dtdoctor_sca_wrapper.py`` and ``scripts/dts/dtdoctor_analyzer.py``,
documented in ``doc/develop/sca/dtdoctor.rst``).

Twister builds this application like any other, with
``ZEPHYR_SCA_VARIANT=dtdoctor``, and then runs two ctest entries on the host
(``harness: ctest``) — the firmware itself never executes:

* ``unit/`` is a unit-test pytest suite for the scripts. EDTs are built from
  inline DTS snippets against the fixture bindings and Kconfig trees under
  ``unit/fixture/``, so it needs no toolchain and no application build.

* ``e2e/`` is an integration pytest suite run against this application's build
  directory. It compiles deliberately-broken sources that use the real
  devicetree macros on the fixture nodes from ``app.overlay``, replaying the
  application's own compile commands (from ``compile_commands.json``) through
  the real SCA wrapper, and checks the resulting diagnoses. This exercises the
  whole chain — generated macros, ``<devicetree.h>`` expansion, the
  toolchain's actual error messages, wrapper, analyzer — with the same C and
  C++ compilers the application was built with.

The application is only a build vehicle: ``app.overlay`` declares fake
``vnd,dtdoctor-*`` devices that deliberately have no driver, and a C++ source
file is included so a real C++ compile command is exported for the e2e suite.

To run everything locally::

   west twister -T tests/misc/dtdoctor

or, against an existing build::

   west build -b qemu_cortex_m3 tests/misc/dtdoctor -- -DZEPHYR_SCA_VARIANT=dtdoctor
   ctest --test-dir build --output-on-failure
