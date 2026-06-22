# MCUX SDK and Zephyr Integration

This document describes how NXP MCUX SDK code integrates with Zephyr.
Currently, MCUX SDK NG (Next Generation) is primarily used.
Some SOCs supported by Zephyr are not supported by MCUX SDK NG,
these SOCs still use MCUX SDK NG drivers. MCUX SDK provides source code,
and this glue layer provides Kconfig and CMake files to select and
configure MCUX source code.

## Folder Structure

```
mcux
├── CMakeLists.txt       # CMake entry point for MCUX SDK glue layer
├── Kconfig.mcux         # Kconfig options for configuring MCUX SDK
├── mcux-sdk
│   └── CMakeLists.txt   # CMake entry for SOCs not supported by SDK NG
└── mcux-sdk-ng
    ├── basic.cmake      # Basic settings and functions for MCUX SDK NG
    ├── fixup.cmake      # Fixes to ensure MCUX SDK NG works with Zephyr
    ├── CMakeLists.txt   # CMake entry for MCUX SDK NG supported SOCs
    ├── components       # Files for MCUX SDK NG components
    ├── device           # Files for MCUX SDK NG device
    ├── drivers          # Files for MCUX SDK NG drivers
    └── middleware       # Files for MCUX SDK NG middleware
```

## Architecture

### MCUX SDK NG CMake Architecture

MCUX SDK NG code is located in the folder:
*<zephyr_workspace>/modules/hal/nxp/mcux/mcux-sdk-ng*.

#### Adding Contents to Project

MCUX SDK's code is modularized and controlled by separate CMake files.
A typical MCUX SDK NG driver CMake file looks like this:

```cmake
if(CONFIG_MCUX_COMPONENT_driver.acmp)

    mcux_add_source(SOURCES fsl_acmp.c fsl_acmp.h)

    mcux_add_include(INCLUDES .)

endif()
```

When the variable `CONFIG_MCUX_COMPONENT_driver.acmp` is enabled, the driver source
files `fsl_acmp.c` and `fsl_acmp.h` are added to the project, along with
their include path.

The functions `mcux_add_source` and `mcux_add_include` are defined in MCUX SDK NG
CMake extensions, located in the folder
*<zephyr_workspace>/modules/hal/nxp/mcux/mcux-sdk-ng/cmake*.

These functions provide additional features, such as adding contents
based on specific conditions.

Here's an example:

```cmake
if (CONFIG_MCUX_COMPONENT_driver.fro_calib)
    mcux_add_source( SOURCES fsl_fro_calib.h )
    mcux_add_library(
        LIBS ../iar/iar_lib_fro_calib_cm33_core0.a
        TOOLCHAINS iar)
    mcux_add_library(
        LIBS ../arm/keil_lib_fro_calib_cm33_core0.lib
        TOOLCHAINS mdk)
    mcux_add_library(
        LIBS ../gcc/libfro_calib_hardabi.a
        TOOLCHAINS armgcc)
    mcux_add_include( INCLUDES . )
endif()
```

In this example, different libraries are added to the project based on
the selected toolchain.

#### Device Level CMake Files

Here are example folder structures for two SOCs: MIMXRT633S (single-core) and
MIMXRT685S (dual-core with 'cm33' and 'hifi4' cores).

```
RT600/
├── MIMXRT633S
│   ├── CMakeLists.txt         # CMake entry for MIMXRT633S
│   ├── variable.cmake
│   ├── drivers/               # SOC specific drivers
│   ├── arm/                   # Files for ARM toolchains (MDK)
│   ├── gcc/                   # Files for armgcc
│   ├── iar/                   # Files for IAR
│   ├── MIMXRT633S.h           # SOC header
│   ├── system_MIMXRT633S.c
│   └── system_MIMXRT633S.h
└── MIMXRT685S
    ├── CMakeLists.txt
    ├── variable.cmake
    ├── drivers/
    ├── arm/
    ├── gcc/
    ├── iar/
    ├── xtensa/
    ├── cm33                   # CMake files for cm33 core
    │   ├── CMakeLists.txt
    │   └── variable.cmake
    ├── hifi4                  # CMake files for hifi4 core
    │   ├── CMakeLists.txt
    │   └── variable.cmake
    ├── MIMXRT685S_cm33.h
    ├── MIMXRT685S_dsp.h
    ├── system_MIMXRT685S_cm33.c
    ├── system_MIMXRT685S_cm33.h
    ├── system_MIMXRT685S_dsp.c
    └── system_MIMXRT685S_dsp.h
```

Key differences between single-core and dual-core SOC folders:
1. Dual-core SOCs have separate folders for each core (cm33, hifi4)
2. Dual-core SOCs have separate header and system files for
   each core with distinct suffixes (MIMXRT685S_cm33.h, MIMXRT685S_dsp.h)

Each SOC provides a "variable.cmake" file allowing upper layer CMake files
to determine appropriate names for different SOCs or cores. Upper layer
CMake files only need to provide `DEVICE` (SOC name, e.g., `MIMXRT685S`)
and `core_id` (subfolder name like `cm33`, `hifi4`; empty for single-core SOCs).

**NOTE**:
In MCUX SDK NG, `core_id` and `core_id_suffix_name` are distinct concepts.
Take RT685S as an example:
- `core_id`: matches subfolder names (`cm33`, `hifi4`)
- `core_id_suffix_name`: matches header file suffixes (`cm33`, `dsp`)

While these are typically identical, some exceptions exist due to
historical reasons (e.g., RT685 HIFI4). Future SOCs will maintain
consistency between these names.

### Integration with Zephyr

#### MCUX SDK NG Glue Layer

- [basic.cmake](./mcux-sdk-ng/basic.cmake): Provides basic settings and functions for MCUX SDK NG,
  including necessary CMake extensions and additional function definitions.

- CMake Files for MCUX SDK NG code selection:
  - [device.cmake](./mcux-sdk-ng/device/device.cmake)
  - [components.cmake](./mcux-sdk-ng/components/components.cmake)
  - [drivers.cmake](./mcux-sdk-ng/drivers/drivers.cmake)
  - [middleware.cmake](./mcux-sdk-ng/middleware/middleware.cmake)

  Each file manages its corresponding MCUX SDK NG code. Example content:

  ```cmake
  set_variable_ifdef(CONFIG_SENSOR_MCUX_ACMP      CONFIG_MCUX_COMPONENT_driver.acmp)
  ```
  When `CONFIG_SENSOR_MCUX_ACMP` is enabled, the ACMP driver is added to the project.

- [fixup.cmake](./mcux-sdk-ng/fixup.cmake): Contains fixes for MCUX SDK NG
  compatibility with Zephyr.

To use MCUX SDK NG code, include these files in the top-level CMakeLists.txt:

```cmake
include(basic.cmake)

include(middleware/middleware.cmake)
include(components/components.cmake)
include(drivers/drivers.cmake)
include(device/device.cmake)

include(fixup.cmake)
```

#### MCUX SDK NG Device Level Code

MCUX SDK NG device CMake files require `DEVICE` and `core_id`.
Zephyr's `CONFIG_SOC` corresponds to MCUX SDK's `DEVICE`, while
`CONFIG_MCUX_CORE_SUFFIX` corresponds to MCUX SDK's `core_id_suffix_name`.
Since these are typically identical, the glue layer converts
`CONFIG_MCUX_CORE_SUFFIX` to `core_id` before passing it to
MCUX SDK NG CMake. Exceptions are handled through hard-coding
in the glue layer.

This functionality is implemented in
[device/device.cmake](./mcux-sdk-ng/device/device.cmake).

#### MCUX SDK NG Unsupported SOCs

These SOCs utilize MCUX SDK NG code for latest features and bug fixes,
along with device-specific code like header files.

Their [CMakeLists.txt](./mcux-sdk/CMakeLists.txt) typically follows this pattern:

```cmake
include(basic.cmake)

include(middleware/middleware.cmake)
include(components/components.cmake)
include(drivers/drivers.cmake)

# ...
# Device specific settings
# ...

include(fixup.cmake)
```

For these SOCs, `basic.cmake` and `fixup.cmake` are required,
while other CMake files (`middleware.cmake`, `components.cmake`, `drivers.cmake`)
are optional.