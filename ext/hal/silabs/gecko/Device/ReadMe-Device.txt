============= Device CMSIS Compliant Peripheral Support Library ==============

This archive contains header file and peripheral support libraries for
Silicon Labs EFM32 and EZR32 series of microcontrollers and System-On-Chip
devices.

================ About Device ================================================

The library and header files are based on CMSIS, the Cortex Microcontroller
Software Interface Standard, as defined by ARM Ltd.

For more information about the CMSIS standard in itself see
    http://www.onarm.com/cmsis
    http://www.arm.com/cmsis

In short, CMSIS tries to provide a common interface for programming devices
having one of the Cortex-M core architectures, making code sharing and reuse
easier.

================ Development Environments ====================================

You will need a development environment which supports Silicon Labs EFM32/EZR32
devices. Currently this can be one of either

Atollic TrueStudio for ARM
  http://www.atollic.com

CooCox CoIDE
  http://www.coocox.com

Hitex HiTop IDE
  http://www.hitex.com

IAR Embedded Workbench for ARM
  http://www.iar.com

Keil uVision "MDK-ARM"
  http://www.keil.com

Mentor Graphics CodeBench (previously Codesourcery Sourcery G++)
  http://www.mentor.com

Rowley CrossWorks for ARM
  http://www.rowley.co.uk

The version required depends on the family of products you are developing
for, check with the vendor for compliance for a particular device.

It is possible to develop with other tools, but these vendors provide good,
integrated development environments for Silicon Labs EFM32/EZR32 devices.

This library uses C99-types, requires the presence of <stdint.h> and can use
other functionality standardized in C99. If your compiler has a C99 compliance
toggle, you should enable it for your projects.

================ File structure ==============================================

Short getting started guide:

The quickest way to start is to base your work on one of the simple example
projects for the Silicon Labs EFM32/EZR32 Development or Starter Kits. These
should be easy to port and change to adopt to your needs.

The board support packages for the various Silicon Labs EFM32/EZR32 kits comes
with a "blink" example, that serves as a good starting point for new projects.

Please note that you _will_ need to change the "Debugger" and "Flash/Download"
configuration settings to fit your environment. See your IDE's manual for
details. You might also need to change the limits of the linker file to adopt
to your part's SRAM and flash size limits.

Support for Silicon Labs EFM32/EZR32 devices family is located in the directory
  Device/SiliconLabs/<FAMILY>,
where family could be one of EFM32G, EFM32TG, EFM32GG, EFM32LG, EZR32WG or
others.

The most convenient way to start a project, is to define the device target
in your compiler options, e.g. add a -DEFM32G890F128 to your compile options
if you are targetting an EFM32G890F128 part.

Include the file "em_device.h" wherever you need register and bit field
definitions for the peripherals or the MCUs/RF transceivers, after setting
the correct include path to point to your "family".

Gecko peripheral registers follow the CMSIS convention of defining a
structure which hold "volatile" peripheral registers. Again, take a look
at the examples for usage.

================ Licenses ====================================================

See the top of each file for software license.

================ Software updates ============================================

Silicon Labs continually works to provide updated and improved example code,
header files and other software of use for our customers. Please check

http://www.silabs.com/support/pages/document-library.aspx?p=MCUs--32-bit

for the latest releases.

If you download and install the "Simplicity Studio" package, you can easily
keep up to date with the latest Silicon Labs EFM32/EZR32 software deliveries,
datasheets, app.notes, erratas and more.

(C) Copyright Silicon Laboratories, Inc. 2016.

