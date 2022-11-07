.. _coccinelle:

..
   Copyright 2010 Nicolas Palix <npalix@diku.dk>
   Copyright 2010 Julia Lawall <julia.lawall@lip6.fr>
   Copyright 2010 Gilles Muller <Gilles.Muller@lip6.fr>

Coccinelle
##########

Coccinelle is a tool for pattern matching and text transformation that has
many uses in kernel development, including the application of complex,
tree-wide patches and detection of problematic programming patterns.

.. note::
   Linux and macOS development environments are supported, but not Windows.

Getting Coccinelle
******************

The semantic patches included in the kernel use features and options
which are provided by Coccinelle version 1.0.0-rc11 and above.
Using earlier versions will fail as the option names used by
the Coccinelle files and ``coccicheck`` have been updated.

Coccinelle is available through the package manager
of many distributions, e.g. :

.. rst-class:: rst-columns

   * Debian
   * Fedora
   * Ubuntu
   * OpenSUSE
   * Arch Linux
   * NetBSD
   * FreeBSD

Some distribution packages are obsolete and it is recommended
to use the latest version released from the Coccinelle homepage at
http://coccinelle.lip6.fr/

Or from Github at:

https://github.com/coccinelle/coccinelle

Once you have it, run the following commands:

.. code-block:: console

   ./autogen
   ./configure
   make

as a regular user, and install it with:

.. code-block:: console

   sudo make install

More detailed installation instructions to build from source can be
found at:

https://github.com/coccinelle/coccinelle/blob/master/install.txt

Supplemental documentation
**************************

For Semantic Patch Language(SmPL) grammar documentation refer to:

https://coccinelle.gitlabpages.inria.fr/website/documentation.html

Using Coccinelle on Zephyr
**************************

``coccicheck`` checker is the front-end to the Coccinelle infrastructure
and has various modes:

Four basic modes are defined: ``patch``, ``report``, ``context``, and
``org``. The mode to use is specified by setting ``--mode=<mode>`` or
``-m=<mode>``.

* ``patch`` proposes a fix, when possible.

* ``report`` generates a list in the following format:
  file:line:column-column: message

* ``context`` highlights lines of interest and their context in a
  diff-like style.Lines of interest are indicated with ``-``.

* ``org`` generates a report in the Org mode format of Emacs.

Note that not all semantic patches implement all modes. For easy use
of Coccinelle, the default mode is ``report``.

Two other modes provide some common combinations of these modes.

- ``chain`` tries the previous modes in the order above until one succeeds.

- ``rep+ctxt`` runs successively the report mode and the context mode.
  It should be used with the C option (described later)
  which checks the code on a file basis.

Examples
********

To make a report for every semantic patch, run the following command:

.. code-block:: console

   ./scripts/coccicheck --mode=report

To produce patches, run:

.. code-block:: console

   ./scripts/coccicheck --mode=patch

The ``coccicheck`` target applies every semantic patch available in the
sub-directories of ``scripts/coccinelle`` to the entire source code tree.

For each semantic patch, a commit message is proposed.  It gives a
description of the problem being checked by the semantic patch, and
includes a reference to Coccinelle.

As any static code analyzer, Coccinelle produces false
positives. Thus, reports must be carefully checked, and patches reviewed.

To enable verbose messages set ``--verbose=1`` option, for example:

.. code-block:: console

   ./scripts/coccicheck --mode=report --verbose=1

Coccinelle parallelization
**************************

By default, ``coccicheck`` tries to run as parallel as possible. To change
the parallelism, set the ``--jobs=<number>`` option. For example, to run
across 4 CPUs:

.. code-block:: console

   ./scripts/coccicheck --mode=report --jobs=4

As of Coccinelle 1.0.2 Coccinelle uses Ocaml parmap for parallelization,
if support for this is detected you will benefit from parmap parallelization.

When parmap is enabled ``coccicheck`` will enable dynamic load balancing by using
``--chunksize 1`` argument, this ensures we keep feeding threads with work
one by one, so that we avoid the situation where most work gets done by only
a few threads. With dynamic load balancing, if a thread finishes early we keep
feeding it more work.

When parmap is enabled, if an error occurs in Coccinelle, this error
value is propagated back, the return value of the ``coccicheck``
command captures this return value.

Using Coccinelle with a single semantic patch
*********************************************

The option ``--cocci`` can be used to check a single
semantic patch. In that case, the variable must be initialized with
the name of the semantic patch to apply.

For instance:

.. code-block:: console

   ./scripts/coccicheck --mode=report --cocci=<example.cocci>

or:

.. code-block:: console

   ./scripts/coccicheck --mode=report --cocci=./path/to/<example.cocci>


Controlling which files are processed by Coccinelle
***************************************************

By default the entire source tree is checked.

To apply Coccinelle to a specific directory, pass the path of specific
directory as an argument.

For example, to check ``drivers/usb/`` one may write:

.. code-block:: console

   ./scripts/coccicheck --mode=patch drivers/usb/

The ``report`` mode is the default. You can select another one with the
``--mode=<mode>`` option explained above.

Debugging Coccinelle SmPL patches
*********************************

Using ``coccicheck`` is best as it provides in the spatch command line
include options matching the options used when we compile the kernel.
You can learn what these options are by using verbose option, you could
then manually run Coccinelle with debug options added.

Alternatively you can debug running Coccinelle against SmPL patches
by asking for stderr to be redirected to stderr, by default stderr
is redirected to /dev/null, if you'd like to capture stderr you
can specify the ``--debug=file.err`` option to ``coccicheck``. For
instance:

.. code-block:: console

   rm -f cocci.err
   ./scripts/coccicheck --mode=patch --debug=cocci.err
   cat cocci.err

Debugging support is only supported when using Coccinelle >= 1.0.2.

Additional Flags
****************

Additional flags can be passed to spatch through the SPFLAGS
variable. This works as Coccinelle respects the last flags
given to it when options are in conflict.

.. code-block:: console

   ./scripts/coccicheck --sp-flag="--use-glimpse"

Coccinelle supports idutils as well but requires coccinelle >= 1.0.6.
When no ID file is specified coccinelle assumes your ID database file
is in the file .id-utils.index on the top level of the kernel, coccinelle
carries a script scripts/idutils_index.sh which creates the database with:

.. code-block:: console

   mkid -i C --output .id-utils.index

If you have another database filename you can also just symlink with this
name.

.. code-block:: console

   ./scripts/coccicheck --sp-flag="--use-idutils"

Alternatively you can specify the database filename explicitly, for
instance:

.. code-block:: console

   ./scripts/coccicheck --sp-flag="--use-idutils /full-path/to/ID"

Sometimes coccinelle doesn't recognize or parse complex macro variables
due to insufficient definition. Therefore, to make it parsable we
explicitly provide the prototype of the complex macro using the
``---macro-file-builtins <headerfile.h>`` flag.

The ``<headerfile.h>`` should contain the complete prototype of
the complex macro from which spatch engine can extract the type
information required during transformation.

For example:

``Z_SYSCALL_HANDLER`` is not recognized by coccinelle. Therefore, we
put its prototype in a header file, say for example ``mymacros.h``.

.. code-block:: console

   $ cat mymacros.h
   #define Z_SYSCALL_HANDLER int xxx

Now we pass the header file ``mymacros.h`` during transformation:

.. code-block:: console

   ./scripts/coccicheck --sp-flag="---macro-file-builtins mymacros.h"

See ``spatch --help`` to learn more about spatch options.

Note that the ``--use-glimpse`` and ``--use-idutils`` options
require external tools for indexing the code. None of them is
thus active by default. However, by indexing the code with
one of these tools, and according to the cocci file used,
spatch could proceed the entire code base more quickly.


SmPL patch specific options
***************************

SmPL patches can have their own requirements for options passed
to Coccinelle. SmPL patch specific options can be provided by
providing them at the top of the SmPL patch, for instance:

.. code-block:: console

   // Options: --no-includes --include-headers

Proposing new semantic patches
******************************

New semantic patches can be proposed and submitted by kernel
developers. For sake of clarity, they should be organized in the
sub-directories of ``scripts/coccinelle/``.

The cocci script should have the following properties:

* The script **must** have ``report`` mode.

* The first few lines should state the purpose of the script
  using ``///`` comments . Usually, this message would be used as the
  commit log when proposing a patch based on the script.

Example
=======

.. code-block:: console

   /// Use ARRAY_SIZE instead of dividing sizeof array with sizeof an element

* A more detailed information about the script with exceptional cases
  or false positives (if any) can be listed using ``//#`` comments.

Example
=======

.. code-block:: console

   //# This makes an effort to find cases where ARRAY_SIZE can be used such as
   //# where there is a division of sizeof the array by the sizeof its first
   //# element or by any indexed element or the element type. It replaces the
   //# division of the two sizeofs by ARRAY_SIZE.

* Confidence: It is a property defined to specify the accuracy level of
  the script. It can be either ``High``, ``Moderate`` or ``Low`` depending
  upon the number of false positives observed.

Example
=======

.. code-block:: console

   // Confidence: High

* Virtual rules: These are required to support the various modes framed
  in the script. The virtual rule specified in the script should have
  the corresponding mode handling rule.

Example
=======

.. code-block:: console

   virtual context

   @depends on context@
   type T;
   T[] E;
   @@
   (
   * (sizeof(E)/sizeof(*E))
   |
   * (sizeof(E)/sizeof(E[...]))
   |
   * (sizeof(E)/sizeof(T))
   )

Detailed description of the ``report`` mode
*******************************************

``report`` generates a list in the following format:

.. code-block:: console

   file:line:column-column: message

Example
=======

Running:

.. code-block:: console

   ./scripts/coccicheck --mode=report --cocci=scripts/coccinelle/array_size.cocci

will execute the following part of the SmPL script:

.. code-block:: console

   <smpl>

   @r depends on (org || report)@
   type T;
   T[] E;
   position p;
   @@
   (
   (sizeof(E)@p /sizeof(*E))
   |
   (sizeof(E)@p /sizeof(E[...]))
   |
   (sizeof(E)@p /sizeof(T))
   )

   @script:python depends on report@
   p << r.p;
   @@

   msg="WARNING: Use ARRAY_SIZE"
   coccilib.report.print_report(p[0], msg)

   </smpl>

This SmPL excerpt generates entries on the standard output, as
illustrated below:

.. code-block:: console

   ext/hal/nxp/mcux/drivers/lpc/fsl_wwdt.c:66:49-50: WARNING: Use ARRAY_SIZE
   ext/hal/nxp/mcux/drivers/lpc/fsl_ctimer.c:74:53-54: WARNING: Use ARRAY_SIZE
   ext/hal/nxp/mcux/drivers/imx/fsl_dcp.c:944:45-46: WARNING: Use ARRAY_SIZE


Detailed description of the ``patch`` mode
******************************************

When the ``patch`` mode is available, it proposes a fix for each problem
identified.

Example
=======

Running:

.. code-block:: console

   ./scripts/coccicheck --mode=patch --cocci=scripts/coccinelle/misc/array_size.cocci

will execute the following part of the SmPL script:

.. code-block:: console

   <smpl>

   @depends on patch@
   type T;
   T[] E;
   @@
   (
   - (sizeof(E)/sizeof(*E))
   + ARRAY_SIZE(E)
   |
   - (sizeof(E)/sizeof(E[...]))
   + ARRAY_SIZE(E)
   |
   - (sizeof(E)/sizeof(T))
   + ARRAY_SIZE(E)
   )

   </smpl>

This SmPL excerpt generates patch hunks on the standard output, as
illustrated below:

.. code-block:: console

   diff -u -p a/ext/lib/encoding/tinycbor/src/cborvalidation.c b/ext/lib/encoding/tinycbor/src/cborvalidation.c
   --- a/ext/lib/encoding/tinycbor/src/cborvalidation.c
   +++ b/ext/lib/encoding/tinycbor/src/cborvalidation.c
   @@ -325,7 +325,7 @@ static inline CborError validate_number(
   static inline CborError validate_tag(CborValue *it, CborTag tag, int flags, int recursionLeft)
   {
     CborType type = cbor_value_get_type(it);
   -    const size_t knownTagCount = sizeof(knownTagData) / sizeof(knownTagData[0]);
   +    const size_t knownTagCount = ARRAY_SIZE(knownTagData);
      const struct KnownTagData *tagData = knownTagData;
      const struct KnownTagData * const knownTagDataEnd = knownTagData + knownTagCount;

Detailed description of the ``context`` mode
********************************************

``context`` highlights lines of interest and their context
in a diff-like style.

.. note::
 The diff-like output generated is NOT an applicable patch. The
 intent of the ``context`` mode is to highlight the important lines
 (annotated with minus, ``-``) and gives some surrounding context
 lines around. This output can be used with the diff mode of
 Emacs to review the code.

Example
=======

Running:

.. code-block:: console

   ./scripts/coccicheck --mode=context --cocci=scripts/coccinelle/array_size.cocci

will execute the following part of the SmPL script:

.. code-block:: console

   <smpl>

   @depends on context@
   type T;
   T[] E;
   @@
   (
   * (sizeof(E)/sizeof(*E))
   |
   * (sizeof(E)/sizeof(E[...]))
   |
   * (sizeof(E)/sizeof(T))
   )

   </smpl>

This SmPL excerpt generates diff hunks on the standard output, as
illustrated below:

.. code-block:: console

   diff -u -p ext/lib/encoding/tinycbor/src/cborvalidation.c /tmp/nothing/ext/lib/encoding/tinycbor/src/cborvalidation.c
   --- ext/lib/encoding/tinycbor/src/cborvalidation.c
   +++ /tmp/nothing/ext/lib/encoding/tinycbor/src/cborvalidation.c
   @@ -325,7 +325,6 @@ static inline CborError validate_number(
   static inline CborError validate_tag(CborValue *it, CborTag tag, int flags, int recursionLeft)
   {
     CborType type = cbor_value_get_type(it);
   -    const size_t knownTagCount = sizeof(knownTagData) / sizeof(knownTagData[0]);
      const struct KnownTagData *tagData = knownTagData;
      const struct KnownTagData * const knownTagDataEnd = knownTagData + knownTagCount;

Detailed description of the ``org`` mode
****************************************

``org`` generates a report in the Org mode format of Emacs.

Example
=======

Running:

.. code-block:: console

   ./scripts/coccicheck --mode=org --cocci=scripts/coccinelle/misc/array_size.cocci

will execute the following part of the SmPL script:

.. code-block:: console

   <smpl>

   @r depends on (org || report)@
   type T;
   T[] E;
   position p;
   @@
   (
   (sizeof(E)@p /sizeof(*E))
   |
   (sizeof(E)@p /sizeof(E[...]))
   |
   (sizeof(E)@p /sizeof(T))
   )

   @script:python depends on org@
   p << r.p;
   @@
   coccilib.org.print_todo(p[0], "WARNING should use ARRAY_SIZE")

   </smpl>

This SmPL excerpt generates Org entries on the standard output, as
illustrated below:

.. code-block:: console

   * TODO [[view:ext/lib/encoding/tinycbor/src/cborvalidation.c::face=ovl-face1::linb=328::colb=52::cole=53][WARNING should use ARRAY_SIZE]]

Coccinelle Mailing List
***********************

Subscribe to the coccinelle mailing list:

* https://systeme.lip6.fr/mailman/listinfo/cocci

Archives:

* https://lore.kernel.org/cocci/
* https://systeme.lip6.fr/pipermail/cocci/
