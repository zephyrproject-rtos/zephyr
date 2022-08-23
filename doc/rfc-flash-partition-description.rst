.. _blob:

RFC  Workflow for Flash memory partitions (and other system definitions)
########################################################################

Overview
********

Flash memory devices are usually "partitiioned" into logical segments, each with a specific
purpose, e.g. storage, boot loader, application executables.

Partitions are sometimes specified in the board device tree (partitions are not board specific
but "system" specific_ or in a device tree overlay to meet the requirements of a test
(e.g. tests/subsys/fs/littlefs).

It should be recognized partitions are either application specific or system specific (e.g. MCUboot
+ application)

Overall goal of this RFC is to start exploring how we can adjust the Zephyr workflow to:

 - define flash memory partitions consistently
   - for an application
   - for a test
   - for a system (from a "core" prospective)
   - for manufacturing

Observations
************

As noted above, the definition of Flash device partitions is somewhat "inconsistent" (overlay for
tests or in board device tree).

Some SoC's have on-silicon flash memory (like NXP's lpc family). Having the partitions defined in
the board device tree is as inappropriate as in the SoC dtsi....

Manufacturing wise, images have to be prepared prior to "flashing". The partition information is needed
to write the binary blobs (e.g. executables, data, etc) to the proper offsets.
The same will apply for eMMC devices.
**In other words, the Zephyr workflow should enable the customer's/end user's tools for manufacturing.
the end user's/silicon vendors could write west extensions to generate the file(s) needed for their
respective tools. This assumes the "system" data is easily accessible, perhaps in a 'system.yaml' file.**

System definitions
******************

System definitions cover:

- for each core of the system:
 - raw flash memory partitions
  - location of the device tree overlay
 - eMMC flash memory partitions
  - location of the description file
    - this information does not belong to the device tree, but to the disk image to be generated

Other system wide definitions:

- signing information
 - location of keys, etc

Workflow changes
****************

Generating images can be achieved in different ways:
 - using standard tools (such as dd and mtdtools on Linux)
 - using silicon vendor provided tools

The best approach to supporting these tools is to rely on west extensions, written by silicon vendors/end users
to generate the files needed by their respective tools. It is most useful if the system information needed is
located in a single file, i.e. system.yaml. It makes writing extensions "simpler". The details of the CLI for that
extension is left to the Si-vendor/end user.

When building an application for a specific core+board, the partition overlay can be applied when the device tree is
generated

Questions
*********

- Impact on sysbuild?



