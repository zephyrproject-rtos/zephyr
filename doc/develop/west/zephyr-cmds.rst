.. _west-zephyr-ext-cmds:

Additional Zephyr extension commands
####################################

This page documents miscellaneous :ref:`west-zephyr-extensions`.

.. _west-boards:

Listing boards: ``west boards``
*******************************

The ``boards`` command can be used to list the boards that are supported by
Zephyr without having to resort to additional sources of information.

It can be run by typing::

  west boards

This command lists all supported boards in a default format. If you prefer to
specify the display format yourself you can use the ``--format`` (or ``-f``)
flag::

  west boards -f "{arch}:{name}"

Additional help about the formatting options can be found by running::

  west boards -h

.. _west-completion:

Shell completion scripts: ``west completion``
*********************************************

The ``completion`` extension command outputs shell completion scripts that can
then be used directly to enable shell completion for the supported shells.

It currently supports the following shells:

- bash
- zsh
- fish
- powershell (board qualifiers only)

Additional instructions are available in the command's help::

  west help completion

.. _west-zephyr-export:

Installing CMake packages: ``west zephyr-export``
*************************************************

This command registers the current Zephyr installation as a CMake
config package in the CMake user package registry.

In Windows, the CMake user package registry is found in
``HKEY_CURRENT_USER\Software\Kitware\CMake\Packages``.

In Linux and MacOS, the CMake user package registry is found in.
:file:`~/.cmake/packages`.

You may run this command when setting up a Zephyr workspace. If you do,
application CMakeLists.txt files that are outside of your workspace will be
able to find the Zephyr repository with the following:

.. code-block:: cmake

   find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

See :zephyr_file:`share/zephyr-package/cmake` for details.

.. _west-spdx:

Software bill of materials: ``west spdx``
*****************************************

This command generates a Software Bill of Materials (SBOM) for a Zephyr build as a set of `SPDX`_
documents. It records the source files that went into the build, the build artifacts they produced,
and the relationships between them. ``SPDX-License-Identifier`` comments found in source files are
scanned and filled into the documents, together with file hashes and (best-effort) copyright
notices.

.. _west-spdx-versions:

Choosing an SPDX version
------------------------

``west spdx`` can emit either of the two major SPDX specification families. SPDX 2.3 is the default;
select another version with the ``--spdx-version`` option.

SPDX 2.3 is a superset of 2.2 and adds fields such as ``PrimaryPackagePurpose``. Pick SPDX 2.x for
compatibility with tooling that does not yet understand SPDX 3.0; pick SPDX 3.0 for the richer,
machine-readable build provenance described in :ref:`west-spdx-build-profile`.

Generating SPDX documents
-------------------------

#. Pre-populate a build directory :file:`BUILD_DIR` like this:

   .. code-block:: bash

      west spdx --init -d BUILD_DIR

   This step ensures the build directory contains the CMake metadata (a CMake file-API query)
   required for SPDX document generation.

#. Enable :kconfig:option:`CONFIG_BUILD_OUTPUT_META` in your project.

#. Build your application using this pre-created build directory, like so:

   .. code-block:: bash

      west build -d BUILD_DIR [...]

#. Generate SPDX documents using this build directory:

   .. code-block:: bash

      west spdx -d BUILD_DIR

   This generates SPDX 2.3 tag-value documents by default. To generate SPDX 2.2
   or 3.0 documents instead, pass ``--spdx-version``:

   .. code-block:: bash

      west spdx -d BUILD_DIR --spdx-version 3.0

.. note::

   When building with :ref:`sysbuild`, make sure you target the actual application
   which you want to generate the SBOM for. For example, if the application is
   named ``hello_world``:

   .. code-block:: bash

     west spdx --init  -d BUILD_DIR/hello_world
     west build -d BUILD_DIR/hello_world
     west spdx -d BUILD_DIR/hello_world

Output documents
----------------

The documents are written to :file:`BUILD_DIR/spdx/` (override with ``-s``). The same set of
bill-of-materials (BOM) documents is produced regardless of the SPDX version; only the file
extension differs: ``.spdx`` for the SPDX 2.x tag-value format and ``.jsonld`` for the SPDX 3.0
JSON-LD format:

- ``app``: BOM for the application source files used for the build
- ``zephyr``: BOM for the specific Zephyr source code files used for the build
- ``build``: BOM for the built output files
- ``modules-deps``: BOM for modules dependencies. Check :ref:`modules
  <modules-vulnerability-monitoring>` for more details.

For SPDX 3.0, every document declares conformance to the Core, Software and Simple Licensing
profiles, and :file:`build.jsonld` additionally declares the :ref:`Build profile
<west-spdx-build-profile>` that captures how the artifacts were produced.

Each file in the bill-of-materials is scanned, so that its hashes (SHA256, SHA1, and MD5)
can be recorded, along with any detected licenses if an
``SPDX-License-Identifier`` comment appears in the file.

Copyright notices are extracted using the third-party :command:`reuse` tool from the REUSE group.
When found, these notices are added to SPDX documents as ``FileCopyrightText`` fields (SPDX 2.x)
or copyright properties (SPDX 3.0).

.. note::
   Copyright extraction uses heuristics that may not capture complete notice text, so
   ``FileCopyrightText`` content is best-effort. This aligns with SPDX specification recommendations.

Relationships
-------------

SPDX relationships are created to indicate dependencies between CMake build targets, build targets
that are linked together, and source files that are compiled to generate the built library files.

The two specification families express build provenance differently:

- In **SPDX 2.x**, each generated artifact carries file-level ``GENERATED_FROM`` relationships
  pointing back at the source (and, with ``--analyze-includes``, header) files it was compiled from.

- In **SPDX 3.0**, that provenance is carried by the :ref:`Build profile <west-spdx-build-profile>`
  instead, using ``hasInput``/``hasOutput``/``usesTool`` relationships scoped to the ``build``
  lifecycle.

.. _west-spdx-build-profile:

Build profile (SPDX 3.0)
------------------------

When generating SPDX 3.0 documents, ``west spdx`` populates the `SPDX 3.0 Build profile`_ so the
SBOM records not just *what* was built but *how* it was built. The information is collected
automatically from the CMake file-API, and lives in :file:`build.jsonld`.

The profile adds a ``build_Build`` element describing the overall build: its build type, the CMake
generator and build configuration, and selected environment variables such as ``BOARD`` and
``ARCH``. The toolchain (CMake, the compilers, assembler, linker and archiver) is recorded as
``Tool`` elements, each with its path and version. Build-scoped relationships then link the build to
its inputs (the source packages and compiled files), its outputs (the final image) and the tools it
used.

Each intermediate target, such as a static library, also gets its own sub-build capturing the exact
sources, tools and compile flags that produced its artifact, so any output can be traced back to how
it was built.

Command-line options
--------------------

``west spdx`` accepts these additional options:

- ``-n PREFIX``: a prefix for the Document Namespaces that will be included in
  the generated SPDX documents. See `SPDX specification clause 6`_ for
  details. If ``-n`` is omitted, a default namespace will be generated
  according to the default format described in section 2.5 using a random UUID.

- ``-s SPDX_DIR``: specifies an alternate directory where the SPDX documents
  should be written instead of :file:`BUILD_DIR/spdx/`.

- ``--spdx-version {2.2,2.3,3.0}``: specifies which SPDX specification version to use.
  Defaults to ``2.3``. See :ref:`west-spdx-versions` for the differences between
  the versions.

- ``--analyze-includes``: in addition to recording the compiled source code
  files (e.g. ``.c``, ``.S``) in the bills-of-materials, also attempt to
  determine the specific header files that are included for each ``.c`` file.

  This takes longer, as it performs a dry run using the C compiler for each
  ``.c`` file using the same arguments that were passed to it for the actual
  build.

- ``--include-sdk``: with ``--analyze-includes``, also create a fourth SPDX
  document, :file:`sdk.spdx` (or :file:`sdk.jsonld`), which lists header files
  included from the SDK.

.. warning::

   The generation of SBOM documents for the ``native_sim`` platform is currently not supported.

.. _SPDX: https://spdx.dev/

.. _SPDX 3.0 Build profile:
   https://spdx.github.io/spdx-spec/v3.0.1/model/Build/Build/

.. _SPDX specification clause 6:
   https://spdx.github.io/spdx-spec/v2.2.2/document-creation-information/

.. _west-blobs:

Working with binary blobs: ``west blobs``
*****************************************

The ``blobs`` command allows users to interact with :ref:`binary blobs
<bin-blobs>` declared in one or more :ref:`modules <modules>` via their
:ref:`module.yml <module-yml>` file.

The ``blobs`` command has three sub-commands, used to list, fetch or clean (i.e.
delete) the binary blobs themselves.

You can list binary blobs while specifying the format of the output::

  west blobs list -f '{module}: {type} {path}'

For the full set of variables available in ``-f/--format`` run
``west blobs -h``.

Fetching blobs works in a similar manner::

  west blobs fetch

Note that, as described in :ref:`the modules section <modules-bin-blobs>`,
fetched blobs are stored in a :file:`zephyr/blobs/` folder relative to the root
of the corresponding module repository.

As does deleting them::

  west blobs clean

Additionally the tool allows you to specify the modules you want to list,
fetch or clean blobs for by typing the module names as a command-line
parameter.

The argument ``--allow-regex`` can be passed ``west blobs fetch`` to restrict
the specific blobs that are fetched, by passing a regular expression::

  # For example, only download esp32 blobs, skip the other variants
  west blobs fetch hal_espressif --allow-regex 'lib/esp32/.*'

An auto-cache directory can be provided via the ``--auto-cache`` cli argument
or via the ``blobs.auto-cache`` config option. When enabled, the auto-cache
directory is automatically populated whenever a blob is missing and downloaded.

One or more additional cache directories (separated by ``;``) can be provided
in ``--cache-dirs`` cli argument or ``blobs.cache-dirs`` config option.

``west blobs fetch`` searches all configured cache directories (including the
auto-cache) for a matching blob filename. Cached files may be stored either
under their original filename or with a SHA-256 suffix (``<filename>.<sha>``).
If found, the blob is copied from the cache to the blob path; otherwise
it is downloaded from its URL(s) to the blob path.

.. _west-twister:

Twister wrapper: ``west twister``
*********************************
This command is a wrapper for :ref:`twister <twister_script>`.

Twister can then be invoked via west as follows::

  west twister -help
  west twister -T tests/ztest/base

.. _west-bindesc:

Working with binary descriptors: ``west bindesc``
*************************************************

The ``bindesc`` command allows users to read :ref:`binary descriptors<binary_descriptors>`
of executable files. It currently supports ``.bin``, ``.hex``, ``.elf`` and ``.uf2`` files
as input.

You can search for a specific descriptor in an image, for example::

   west bindesc search KERNEL_VERSION_STRING build/zephyr/zephyr.bin

You can search for a custom descriptor by type and ID, for example::

   west bindesc custom_search STR 0x200 build/zephyr/zephyr.bin

You can dump all of the descriptors in an image using::

   west bindesc dump build/zephyr/zephyr.bin

You can extract the descriptor data area of the image to a file using::

   west bindesc extract

You can list all known standard descriptor names using::

   west bindesc list

You can print the offset of the descriptors inside the image using::

   west bindesc get_offset

Indexing the sources with GNU Global: ``west gtags``
****************************************************

.. important:: You must install the ``gtags`` and ``global`` programs provided
               by `GNU Global`_ to use this command.

The ``west gtags`` command lets you create a GNU Global tags file for the entire
west workspace::

  west gtags

.. _GNU Global: https://www.gnu.org/software/global/

This will create a tags file named ``GTAGS`` in the workspace :ref:`topdir
<west-workspace>` (it will also create other Global-related metadata files
named ``GPATH`` and ``GRTAGS`` in the same place).

You can then run the ``global`` command anywhere inside the
workspace to search for symbol locations using this tags file.

For example, to search for definitions of the ``arch_system_halt()`` function,
starting from the ``zephyr/drivers`` directory::

  $ cd zephyr/drivers
  $ global -x arch_system_halt
  arch_system_halt   65 ../arch/arc/core/fatal.c FUNC_NORETURN void arch_system_halt(unsigned int reason)
  arch_system_halt  455 ../arch/arm64/core/fatal.c FUNC_NORETURN void arch_system_halt(unsigned int reason)
  arch_system_halt  137 ../arch/nios2/core/fatal.c FUNC_NORETURN void arch_system_halt(unsigned int reason)
  arch_system_halt   18 ../arch/posix/core/fatal.c FUNC_NORETURN void arch_system_halt(unsigned int reason)
  arch_system_halt   17 ../arch/x86/core/fatal.c FUNC_NORETURN void arch_system_halt(unsigned int reason)
  arch_system_halt  126 ../arch/xtensa/core/fatal.c FUNC_NORETURN void arch_system_halt(unsigned int reason)
  arch_system_halt   21 ../kernel/fatal.c FUNC_NORETURN __weak void arch_system_halt(unsigned int reason)

This prints the search symbol, the line it is defined on, a relative path to
the file it is defined in, and the line itself, for all places where the symbol
is defined.

Additional tips:

- This can also be useful to search for vendor HAL function definitions.

- See the ``global`` command's manual page for more information on how to use
  this tool.

- You should run ``global``, **not** ``west global``. There is no need for a
  separate ``west global`` command since ``global`` already searches for the
  ``GTAGS`` file starting from your current working directory. This is why you
  need to run ``global`` from inside the workspace.

.. _west-patch:

Working with patches: ``west patch``
************************************

The ``patch`` command allows users to apply patches to Zephyr or Zephyr modules
in a controlled manner that makes automation and tracking easier for external applications that
use the :ref:`T2 star topology <west-t2>`. The :ref:`patches.yml <patches-yml>` file stores
metadata about patch files and fills-in the gaps between official Zephyr releases, so that users
can easily see the status of any upstreaming efforts, and determine which patches to drop before
upgrading to the next Zephyr release.

There are several sub-commands available to manage patches for Zephyr or other modules in the
workspace:

* ``apply``: apply patches listed in ``patches.yml``
* ``clean``: remove all patches that have been applied, and reset to the manifest checkout state
* ``list``: list all patches in ``patches.yml``
* ``gh-fetch``: fetch patches from a GitHub pull request

.. code-block:: none

    west-workspace/
    └── application/
       ...
       ├── west.yml
       └── zephyr
           ├── module.yml
           ├── patches
           │   ├── bootloader
           │   │   └── mcuboot
           │   │       └── my-tweak-for-mcuboot.patch
           │   └── zephyr
           │       └── my-zephyr-change.patch
           └── patches.yml

In this example, the :ref:`west manifest <west-manifests>` file, ``west.yml``, would pin to a
specific Zephyr revision (e.g. ``v4.1.0``) and apply patches against that revision of Zephyr and
the specific revisions of other modules used in the application. However, this application needs
two changes in order to meet requirements; one for Zephyr and another for MCUBoot.

.. _patches-yml:

.. code-block:: yaml

    patches:
      - path: zephyr/my-zephyr-change.patch
        sha256sum: c676cd376a4d19dc95ac4e44e179c253853d422b758688a583bb55c3c9137035
        module: zephyr
        author: Obi-Wan Kenobi
        email: obiwan@jedi.org
        date: 2025-05-04
        upstreamable: false
        comments: |
          An application-specific change we need for Zephyr.
      - path: bootloader/mcuboot/my-tweak-for-mcuboot.patch
        sha256sum: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
        module: mcuboot
        author: Darth Sidious
        email: sidious@sith.org
        date: 2025-05-04
        merge-pr: https://github.com/zephyrproject-rtos/zephyr/pull/<pr-number>
        issue: https://github.com/zephyrproject-rtos/zephyr/issues/<issue-number>
        merge-status: true
        merge-commit: 1234567890abcdef1234567890abcdef12345678
        merge-date: 2025-05-06
        apply-command: git apply
        comments: |
          A change to mcuboot that has been merged already. We can remove this
          patch when we are ready to upgrade to the next Zephyr release.

Patches can easily be applied in an automated manner. For example:

.. code-block:: bash

    west init -m <manifest repo> <workspace>
    cd <workspace>
    west update
    west patch apply

When it is time to update to a newer version of Zephyr, the ``west.yml`` file can be updated to
point at the next Zephyr release, e.g. ``v4.2.0``. Patches that are no longer needed, like
``my-tweak-for-mcuboot.patch`` in the example above, can be removed from ``patches.yml`` and from
the external application repository, and then the following commands can be run.

.. code-block:: bash

    west patch clean
    west update
    west patch apply --roll-back # roll-back all patches if one does not apply cleanly

If a patch needs to be reworked, remember to update the ``patches.yml`` file with the new SHA256
checksum.

.. code-block:: bash

    sha256sum zephyr/patches/zephyr/my-zephyr-change.patch
    7d57ca78d5214f422172cc47fed9d0faa6d97a0796c02485bff0bf29455765e9

It is also possible to use ``west patch gh-fetch`` to fetch patches from a GitHub pull request and
automatically create or update the ``patches.yml`` file. This can be useful when the author already
has a number of changes captured in existing upstream pull requests.

.. code-block:: bash

    west patch gh-fetch --owner zephyrproject-rtos --repo zephyr --pull-request <pr-number> \
      --module zephyr --split-commits

The above command will create the directory and file structure below, which includes patches for
each individual commit associated with the given pull request.

.. code-block:: none

    zephyr
    ├── patches
    │   ├── first-commit-from-pr.patch
    │   ├── second-commit-from-pr.patch
    │   └── third-commit-from-pr.patch
    └── patches.yml

Working with the Zephyr SDK: ``west sdk``
*****************************************

The ``west sdk`` command is a Zephyr-specific west command used to list
and install the Zephyr SDK and its toolchains.

Listing SDKs and toolchains
---------------------------

To list installed Zephyr SDKs as well as available SDK releases and
toolchains, run:

.. code-block:: console

   west sdk list

This command shows:

- Installed SDK versions
- Available SDK releases
- Toolchains included in each SDK

Installing the Zephyr SDK
-------------------------

To install the Zephyr SDK, run:

.. code-block:: console

   west sdk install

This command may run in interactive mode and prompt for SDK or
toolchain selection. When specific toolchains are provided via
``--toolchains``, the command runs non-interactively, which is
recommended for automation.

To install specific toolchains only, use the ``--toolchains`` option:

.. code-block:: console

   west sdk install --toolchains arm-zephyr-eabi riscv64-zephyr-elf

If you are unsure which toolchains you need, run ``west sdk list`` first
to see the available options and avoid downloading unnecessary
toolchains, which can save gigabytes of disk space and download time.
