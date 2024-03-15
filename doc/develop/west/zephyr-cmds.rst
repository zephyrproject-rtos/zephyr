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

This command generates SPDX 2.2 tag-value documents, creating relationships
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

This generates the following SPDX bill-of-materials (BOM) documents in
:file:`BUILD_DIR/spdx/`:

- :file:`app.spdx`: BOM for the application source files used for the build
- :file:`zephyr.spdx`: BOM for the specific Zephyr source code files used for the build
- :file:`build.spdx`: BOM for the built output files

Each file in the bill-of-materials is scanned, so that its hashes (SHA256 and
SHA1) can be recorded, along with any detected licenses if an
``SPDX-License-Identifier`` comment appears in the file.

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

- ``--analyze-includes``: in addition to recording the compiled source code
  files (e.g. ``.c``, ``.S``) in the bills-of-materials, also attempt to
  determine the specific header files that are included for each ``.c`` file.

  This takes longer, as it performs a dry run using the C compiler for each
  ``.c`` file using the same arguments that were passed to it for the actual
  build.

- ``--include-sdk``: with ``--analyze-includes``, also create a fourth SPDX
  document, :file:`sdk.spdx`, which lists header files included from the SDK.

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
