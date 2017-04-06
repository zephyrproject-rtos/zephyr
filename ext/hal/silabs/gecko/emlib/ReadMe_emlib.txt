================ Silicon Labs Peripheral Library ============================

This directory, "emlib", contains the Silicon Labs Peripheral Support
library for the EFM32 and EZR32 series of microcontrollers and System-On-Chip
devices.

Some design guidelines for this library:

* Follow the guidelines established by ARM's and Silicon Labs's adaptation
  of the CMSIS (see below) standard.

* Be usable as a starting point for developing richer, more target specific
  functionality (i.e. copy and modify further).

* Ability to be used as a standalone software component, to be used by other
  drivers, that should cover "the most common cases".

* Readability of the code and usability preferred before optimization for speed
  and size or covering a particular "narrow" purpose.

* As little "cross-dependency" between modules as possible, to enable users to
  pick and choose what they want.

================ About CMSIS ================================================

These APIs are based on EM_CMSIS "Device" header file structure.

As a result of this, the library requires basic C99-support. You might have
to enable C99 support in your compiler. Comments are in doxygen compatible
format.

The EM_CMSIS library contains all peripheral module registers and bit field
descriptors.

To download EM_CMSIS, go to
    http://www.silabs.com/support/pages/document-library.aspx?p=MCUs--32-bit

For more information about CMSIS see
    http://www.onarm.com
    http://www.arm.com/products/CPUs/CMSIS.html

The requirements for using CMSIS also apply to this package.

================ File structure ==============================================

inc/ - header files
src/ - source files

================ Licenses ====================================================

See the top of each file for SW license. Basically you are free to use the
Silicon Labs code for any project using Silicon Labs devices. Parts of the
CMSIS library is copyrighted by ARM Inc. See "License.doc" for ARM's CMSIS
license.

================ Software updates ============================================

Silicon Labs continually works to provide updated and improved emlib, example
code and other software of use for Silicon Labs customers. Please check the
download section of Silicon Labs's web site at

        http://www.silabs.com

for the latest releases. If you download and install the
Simplicity Studio application, you will be notified about updates when
available.

(C) Copyright Silicon Labs, 2015
