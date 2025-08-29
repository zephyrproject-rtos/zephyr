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

This command generates SPDX 2.2 or 2.3 tag-value documents, creating relationships
from source files to the corresponding generated build files.
``SPDX-License-Identifier`` comments in source files are scanned and filled
into the SPDX documents.

To use this command:

#. Pre-populate a build directory :file:`BUILD_DIR` like this:

   .. code-block:: bash

      west spdx --init -d BUILD_DIR

   This step ensures the build directory contains CMake metadata required for
   SPDX document generation.

#. Enable :file:`CONFIG_BUILD_OUTPUT_META` in your project.

#. Build your application using this pre-created build directory, like so:

   .. code-block:: bash

      west build -d BUILD_DIR [...]

#. Generate SPDX documents using this build directory:

   .. code-block:: bash

      west spdx -d BUILD_DIR

   By default, this generates SPDX 2.3 documents. To generate SPDX 2.2 documents instead:

   .. code-block:: bash

      west spdx -d BUILD_DIR --spdx-version 2.2

.. note::

   When building with :ref:`sysbuild`, make sure you target the actual application
   which you want to generate the SBOM for. For example, if the application is
   named ``hello_world``:

   .. code-block:: bash

     west spdx --init  -d BUILD_DIR/hello_world
     west build -d BUILD_DIR/hello_world
     west spdx -d BUILD_DIR/hello_world

This generates the following SPDX bill-of-materials (BOM) documents in
:file:`BUILD_DIR/spdx/`:

- :file:`app.spdx`: BOM for the application source files used for the build
- :file:`zephyr.spdx`: BOM for the specific Zephyr source code files used for the build
- :file:`build.spdx`: BOM for the built output files
- :file:`modules-deps.spdx`: BOM for modules dependencies. Check
  :ref:`modules <modules-vulnerability-monitoring>` for more details.

Each file in the bill-of-materials is scanned, so that its hashes (SHA256 and
SHA1) can be recorded, along with any detected licenses if an
``SPDX-License-Identifier`` comment appears in the file.

Copyright notices are extracted using the third-party :command:`reuse` tool from the REUSE group.
When found, these notices are added to SPDX documents as ``FileCopyrightText`` fields.

.. note::
   Copyright extraction uses heuristics that may not capture complete notice text, so
   ``FileCopyrightText`` content is best-effort. This aligns with SPDX specification recommendations.

SPDX Relationships are created to indicate dependencies between
CMake build targets, build targets that are linked together, and
source files that are compiled to generate the built library files.

``west spdx`` accepts these additional options:

- ``-n PREFIX``: a prefix for the Document Namespaces that will be included in
  the generated SPDX documents. See `SPDX specification clause 6`_ for
  details. If ``-n`` is omitted, a default namespace will be generated
  according to the default format described in section 2.5 using a random UUID.

- ``-s SPDX_DIR``: specifies an alternate directory where the SPDX documents
  should be written instead of :file:`BUILD_DIR/spdx/`.

- ``--spdx-version {2.2,2.3}``: specifies which SPDX specification version to use.
  Defaults to ``2.3``. SPDX 2.3 includes additional fields like ``PrimaryPackagePurpose``
  that are not available in SPDX 2.2.

- ``--analyze-includes``: in addition to recording the compiled source code
  files (e.g. ``.c``, ``.S``) in the bills-of-materials, also attempt to
  determine the specific header files that are included for each ``.c`` file.

  This takes longer, as it performs a dry run using the C compiler for each
  ``.c`` file using the same arguments that were passed to it for the actual
  build.

- ``--include-sdk``: with ``--analyze-includes``, also create a fourth SPDX
  document, :file:`sdk.spdx`, which lists header files included from the SDK.

.. warning::

   The generation of SBOM documents for the ``native_sim`` platform is currently not supported.

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
