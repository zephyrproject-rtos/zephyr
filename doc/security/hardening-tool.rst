.. _hardening:

Hardening Tool
##############

Before launching a product, it's crucial to ensure that your software is as secure as possible. This
process, known as "hardening", involves strengthening the security of a system to protect it from
potential threats and vulnerabilities.

At a high-level, hardening a Zephyr application can be seen as a two-fold process:

#. Disabling features and compilation flags that might lead to security vulnerabilities (ex. making
   sure that no "experimental" features are being used, disabling features typically used for
   debugging purposes such as assertions, shell, etc.).
#. Enabling optional features that can lead to improve security (ex. stack sentinel, hardware stack
   protection, etc.). Some of these features might be hardware-dependent.

To simplify this process, Zephyr offers a **hardening tool** designed to analyze an application's
configuration against the **hardening database**, a set of recommendations curated by the Zephyr
**Security Working Group**. The tool looks at the Kconfig options in the build target and provides
tailored suggestions and recommendations to adjust security-related options, along with the
rationale behind each recommendation.

Usage
*****

.. zephyr-app-commands::
    :tool: all
    :zephyr-app: samples/hello_world
    :board: reel_board
    :goals: hardenconfig

The output should be similar to the table below. For each configuration option set to a value that
could lead to a security vulnerability, the table will propose a recommended value that should be
used instead, together with the reason the option matters.

.. code-block:: console

   Hardening report for profile: strict
   +------------------------------+-----------+---------------+----------------+--------------------------------------------------+
   | Name                         | Current   | Recommended   | Check result   | Rationale                                        |
   +==============================+===========+===============+================+==================================================+
   | CONFIG_BUILD_OUTPUT_STRIPPED | n         | y             | FAIL           | Produces a stripped copy of the ELF next to the  |
   |                              |           |               |                | unstripped one, so a product that distributes or |
   |                              |           |               |                | flashes the ELF itself ships no symbol names or  |
   |                              |           |               |                | debug information for an attacker to work from.  |
   |                              |           |               |                | Both files are built; deploying the stripped one |
   |                              |           |               |                | remains the user's responsibility.               |
   +------------------------------+-----------+---------------+----------------+--------------------------------------------------+
   | CONFIG_STACK_SENTINEL        | n         | y             | FAIL           | Places a software sentinel value at the end of   |
   |                              |           |               |                | each thread stack and checks it at context       |
   |                              |           |               |                | switches, catching overflows on hardware without |
   |                              |           |               |                | MPU/MMU stack protection.                        |
   +------------------------------+-----------+---------------+----------------+--------------------------------------------------+

Options that are not applicable to the current target (for example, MPU-based protections on
hardware without an MPU, or options with no visible prompt in the current configuration) are not
reported.

In addition to the database-driven checks, the tool flags any enabled option that is marked in
Kconfig itself as experimental, deprecated or not secure (i.e. options selecting
:kconfig:option:`CONFIG_EXPERIMENTAL`, :kconfig:option:`CONFIG_DEPRECATED` or
:kconfig:option:`CONFIG_NOT_SECURE`).

Profiles
********

Recommendations are grouped into **profiles**. Two profiles are provided in-tree:

``base``
   Baseline hardening every production build should satisfy: enable available memory-protection
   and exploit-mitigation features, and disable inherently insecure options.

``strict`` (default)
   Everything in ``base``, plus removal of debugging, tracing and observability features (logging,
   shell, assertions, etc.) that enlarge the attack surface or disclose internal state. Some
   products legitimately keep a subset of these enabled — if that is your case, check against the
   ``base`` profile instead:

.. code-block:: shell

   west build -t hardenconfig -- -DHARDENCONFIG_PROFILE=base

Configuration options
*********************

The tool is controlled through CMake cache variables (passed after ``--`` on the ``west build``
command line, as above) or environment variables of the same name:

.. list-table::
   :header-rows: 1

   * - Option
     - Effect
   * - ``HARDENCONFIG_PROFILE``
     - Hardening profile to check against. Default: ``strict``.
   * - ``HARDENCONFIG_SHOW_ALL``
     - When set, also list passing and non-applicable options instead of only failures.
   * - ``HARDENCONFIG_STRICT``
     - When set, exit with a non-zero code if any check fails. Useful to gate CI pipelines or
       release processes on a clean hardening report.
   * - ``HARDENCONFIG_JSON``
     - Path to a file where results are additionally written as JSON, for consumption by scripts
       and dashboards.
   * - ``HARDENCONFIG_EXTRA_SOURCES``
     - Semicolon-separated list of additional hardening database files (see below).

The hardening database
**********************

The database is distributed so that each rule lives with the code it hardens and is reviewed by
that code's maintainers:

* :zephyr_file:`scripts/kconfig/hardening.yaml` — the central file, owned by the Security Working
  Group. It defines the hardening **profiles**, plus rules for symbols defined in the top-level
  Kconfig files.
* ``hardening.yaml`` **fragments** next to the Kconfig files they relate to (for example
  :zephyr_file:`subsys/bluetooth/hardening.yaml` or :zephyr_file:`arch/x86/hardening.yaml`),
  containing rules only. Fragments are discovered automatically under ``arch/``, ``boards/``,
  ``drivers/``, ``kernel/``, ``lib/``, ``modules/``, ``share/``, ``soc/`` and ``subsys/``.
  Defining the same symbol in two files is an error, as is defining profiles in a fragment.

All files use the same JSON schema, :zephyr_file:`scripts/schemas/hardening-schema.yaml`. Each
rule is keyed by the Kconfig symbol it applies to and recommends either an exact value or an
integer constraint:

.. code-block:: yaml

   rules:
     BOOT_BANNER:
       value: n
       rationale: |
         The boot banner prints the exact Zephyr version to the console,
         letting anyone with console access fingerprint the firmware and
         match it against known vulnerabilities for that release.
       references: [CWE-200]

     STACK_POINTER_RANDOM:
       min: 100
       rationale: |
         Randomizing each thread's initial stack pointer makes stack
         addresses unpredictable. The offset is taken out of the thread's
         own stack, which keeps the recommended value small.
       references: [CWE-121]

Contributing a new rule means adding it to the ``hardening.yaml`` fragment next to the relevant
Kconfig file (creating the fragment if the subsystem does not have one yet) and requires a
``rationale`` — that requirement is enforced by the schema. Continuous integration additionally
verifies that every file parses and matches the schema, that every rule references an existing
Kconfig symbol with a value coherent with the symbol's type, that no rule is defined twice, and
that no file looks like a misnamed fragment — so entries cannot silently go stale or silently
fail to load.

The ``rationale`` is what the report shows the reader, wrapped into a narrow column, and is usually
all they have to go on. State the security consequence of the option in one or two sentences,
present tense, with the feature as the subject:

``value: n``
   The exposure enabling it creates and, where it matters, who can reach it: "Verbose modem logging
   dumps all data exchanged with the modem, which can include credentials such as APN settings,
   PINs and server addresses."

``value: y``
   The attack it defeats, or what leaving it off permits, whichever reads better.

``value: 0``, ``min`` and ``max``
   What the other values emit or fail to prevent, and what the boundary value itself does.

Do not:

* Say where the option belongs. The profile is what decides whether a product should accept the
  consequence; write "…discloses internal transfer state and addresses", not "…and belongs only
  in development builds".
* Claim an effect that is not complete without further action. For example, an option may enable
  the generation of a stripped binary, but making sure that is the one deployed is up to the user.
* Restate a column the report already shows. In "0 disables randomization entirely; at least 100
  bytes of random offset is recommended", the second half is the Recommended column.
* Repeat another rule's wording. Two rules that can fail in the same report should read differently
  enough for the reader to tell them apart.

Do not add an ``n`` rule for an option that can only be enabled while another ``n`` rule covering
at least the same profiles is already violated; for example, a rule for a shell command module is
redundant once :kconfig:option:`CONFIG_SHELL` itself is flagged, and CI rejects it. A rule for an
option that merely ``select``\ s a flagged one is not redundant: it names what the user must
actually change.

Rules do not carry applicability conditions: whether a recommendation applies to a given target
is already encoded in Kconfig itself (dependencies, hardware support), and the tool reports
options that are not applicable to the current target as not applicable rather than as failures.

Extending the database out of tree
**********************************

Product teams can layer their own recommendations on top of the in-tree database with
``HARDENCONFIG_EXTRA_SOURCES``. Each additional file uses the same schema and may define new
profiles (optionally extending in-tree ones) and new rules, as well as override in-tree rules.
Redefining an existing profile is rejected, so an in-tree profile keeps the meaning documented
here whatever a product layers on top of it:

.. code-block:: yaml

   profiles:
     acme-production:
       extends: strict
       description: ACME product security policy.

   rules:
     MY_VENDOR_DEBUG_INTERFACE:
       value: n
       profiles: [acme-production]
       rationale: |
         The vendor debug interface bypasses the product's authentication.

.. code-block:: shell

   west build -t hardenconfig -- \
     -DHARDENCONFIG_EXTRA_SOURCES=/path/to/acme-hardening.yaml \
     -DHARDENCONFIG_PROFILE=acme-production
