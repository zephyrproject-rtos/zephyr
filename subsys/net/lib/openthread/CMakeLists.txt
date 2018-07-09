# Invoke OpenThread's external autoconf-based build system.
include(ExternalProject)

set(ep_base ${PROJECT_BINARY_DIR}/ext_proj)
set_property(DIRECTORY PROPERTY "EP_BASE" ${ep_base})

# Construct a list of commands to give to ExternalProject_Add()
#
# See https://cmake.org/cmake/help/latest/module/ExternalProject.html
# for documentation on ExternalProject_Add
set(cmd "")

set(ot_name ot)
list(APPEND cmd
  ${ot_name}
  )

set(ot_STAMP_DIR   ${ep_base}/Stamp/${ot_name})
set(ot_INSTALL_DIR ${ep_base}/Install/${ot_name})

#--Download step--------------
if(NOT EXTERNAL_PROJECT_PATH_OPENTHREAD)
  # TODO: Point to a Zephyr fork
  # Nov. 7
  set_ifndef(ot_GIT_REPOSITORY "https://github.com/openthread/openthread.git")
  set_ifndef(ot_GIT_TAG db4759cc41257d0572ddedbeead1e02c52033620)
  set_ifndef(ot_GIT_PROGRESS 1)

  list(APPEND cmd
    GIT_REPOSITORY ${ot_GIT_REPOSITORY}
    GIT_TAG        ${ot_GIT_TAG}
    GIT_PROGRESS   ${ot_GIT_PROGRESS}
    )
endif()

#--Update/Patch step-------------

# An update step is not necessary because we are using a commit hash
# as a tag, and the code behind a hash cannot change.

# UPDATE_DISCONNECTED should be used when updates aren't needed, but
# for some reason we were not able to get this to work, so we use a
# dummy step to fake updating. This ensures that the git repo is not
# downloaded on every 'make' invocation.
list(APPEND cmd
  UPDATE_COMMAND
  ${CMAKE_COMMAND} -E touch ${ot_STAMP_DIR}/${ot_name}-update
  )

#--Configure step-------------
if(EXTERNAL_PROJECT_PATH_OPENTHREAD)
  set(ot_SOURCE_DIR ${EXTERNAL_PROJECT_PATH_OPENTHREAD})

  list(APPEND cmd
    SOURCE_DIR ${ot_SOURCE_DIR}
    )

else()
  set(ot_SOURCE_DIR ${ep_base}/Source/${ot_name}) # TODO: Download dir?

  # "If SOURCE_DIR is explicitly set to an existing directory the
  # project will be built from it. Otherwise a download step must be
  # specified using one of the DOWNLOAD_COMMAND, CVS_*, SVN_*, or URL
  # options." -- CMake docs
endif()

# TODO: Can we omit this, does OpenThread need to use install?
# TODO: Move this to host-tools?
find_program(INSTALL install)

# It looks like OpenThread requires a *nix system to build for
# embedded.
#
# https://github.com/openthread/openthread/blob/master/examples/drivers/windows/README.md

# TODO: Use different includes for C and CXX
zephyr_get_include_directories_for_lang_as_string(       C includes)
zephyr_get_system_include_directories_for_lang_as_string(C system_includes)
zephyr_get_compile_definitions_for_lang_as_string(       C definitions)

zephyr_get_compile_options_for_lang_as_string(C   c_options)
zephyr_get_compile_options_for_lang_as_string(CXX cxx_options)

# TODO: What happens if a CFLAG is added after this build script has
# been run?

# TODO: Does OpenThread need all flags, or just some? Should we
# whitelist, or blacklist flags?


# TODO: What is all this? $(dir $(realpath $(firstword $(MAKEFILE_LIST))))

set(exec_prefix zephyr)

set(commoncflags "-DOPENTHREAD_CONFIG_LOG_LEVEL=${CONFIG_OPENTHREAD_LOG_LEVEL}")
set(commoncflags "${commoncflags} -DOPENTHREAD_PROJECT_CORE_CONFIG_FILE=\\\"openthread-core-zephyr-config.h\\\"")
set(commoncflags "${commoncflags} -I${CMAKE_CURRENT_LIST_DIR}/platform")

get_property(RULE_LAUNCH_COMPILE GLOBAL PROPERTY RULE_LAUNCH_COMPILE)

set(configure_flags
  "INSTALL=${INSTALL} -p"
  "CPP=${CMAKE_C_COMPILER} -E"                 # TODO: Find CPP in toolchain-gcc.cmake and use that instead?
  "CC=${RULE_LAUNCH_COMPILE} ${CMAKE_C_COMPILER}"
  "CXX=${RULE_LAUNCH_COMPILE} ${CMAKE_CXX_COMPILER}"
  OBJC=""                      # TODO: Omit this?
  "OBJCXX=${OBJCXX}"           # TODO: Omit this?
  "AR=${CMAKE_AR}"
  "RANLIB=${CMAKE_RANLILB}"
  "NM=${CROSS_COMPILE}nm"      # TODO: Find NM in toolchain-gcc.cmake and use that instead?
  "STRIP=${CMAKE_STRIP}"
  "CPPFLAGS=${definitions} ${commoncflags} ${includes} ${system_includes}"
  "CFLAGS=${c_options}     ${commoncflags} ${includes} ${system_includes}"
  "CXXFLAGS=${cxx_options} ${commoncflags} ${includes} ${system_includes}" # TODO: Do we need includes here?
  LDFLAGS="" # TODO: What does a networking stack need to use the linker for?

  --host=arm-none-eabi
  --prefix=/
  --exec-prefix=/${exec_prefix}
  --target=arm-none-eabi # TODO: Is Kconfig expressing that OT is ARM-only?
  --enable-no-executables-hack
  --disable-docs
  --with-platform-info=zephyr
)

# TODO: Simplify
set(ZEPHYR_MBEDTLS_CPPFLAGS "${ZEPHYR_MBEDTLS_CPPFLAGS} ")
set(ZEPHYR_MBEDTLS_CPPFLAGS "-DMBEDTLS_CONFIG_FILE='\"mbedtls-config.h\"'")
set(ZEPHYR_MBEDTLS_CPPFLAGS "${ZEPHYR_MBEDTLS_CPPFLAGS} -DMBEDTLS_USER_CONFIG_FILE='\"${CMAKE_CURRENT_SOURCE_DIR}/zephyr-mbedtls-config.h\"'")
set(ZEPHYR_MBEDTLS_CPPFLAGS "${ZEPHYR_MBEDTLS_CPPFLAGS} -I${ot_SOURCE_DIR}/third_party/mbedtls")
set(ZEPHYR_MBEDTLS_CPPFLAGS "${ZEPHYR_MBEDTLS_CPPFLAGS} -I${ot_SOURCE_DIR}/third_party/mbedtls/repo.patched/include")
set(ZEPHYR_MBEDTLS_CPPFLAGS "${ZEPHYR_MBEDTLS_CPPFLAGS} -I${ot_SOURCE_DIR}/third_party/mbedtls/repo.patched/include/mbedtls")

list(APPEND configure_flags
  "MBEDTLS_CPPFLAGS=${ZEPHYR_MBEDTLS_CPPFLAGS}"
  )

if(CONFIG_OPENTHREAD_COMMISSIONER)
  list(APPEND configure_flags
    --enable-commissioner
    )
endif()

if(CONFIG_OPENTHREAD_JAM_DETECTION)
  list(APPEND configure_flags
    --enable-jam-detection
    )
endif()

if(CONFIG_OPENTHREAD_JOINER)
  list(APPEND configure_flags
    --enable-joiner
    )
endif()

if(CONFIG_OPENTHREAD_SHELL)
  list(APPEND configure_flags
    --enable-cli-app=all
    )
endif()

if(CONFIG_OPENTHREAD_DIAG)
  list(APPEND configure_flags
    --enable-diag
    )
endif()

list(APPEND cmd
  CONFIGURE_COMMAND ./configure ${configure_flags}
)

#--Build step-----------------

# Invoke OpenThread's build system from the root of it's source
# directory
# TODO: Support out-of-source builds
set(ot_BINARY_DIR  ${ot_SOURCE_DIR})
list(APPEND cmd
  BINARY_DIR  ${ot_BINARY_DIR}
  INSTALL_DIR ${ot_INSTALL_DIR}
  )

set(make_flags
  -j99 # TODO: Why 99?
  --no-print-directory
  )

list(APPEND cmd
  BUILD_COMMAND   make ${make_flags}                           all
  INSTALL_COMMAND make ${make_flags} DESTDIR=${ot_INSTALL_DIR} install
  )

# TODO: Find out how to make this work.
set(ot_include_dir ${ot_SOURCE_DIR}/include)
# For platform.h
set(ot_platforms_dir ${ot_SOURCE_DIR}/examples/platforms)

# TODO: Is this only needed by alarm.c?
zephyr_system_include_directories(${ot_include_dir})
zephyr_system_include_directories(${ot_platforms_dir})

# TODO: Why doesn't app get this path from the above function call?
target_include_directories(app SYSTEM PRIVATE ${ot_include_dir})
target_include_directories(app SYSTEM PRIVATE ${ot_platforms_dir})

#set_target_properties(ot_lib PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${ot_include_dir})
zephyr_include_directories(${ot_include_dir})
zephyr_include_directories(${ot_platforms_dir})

# Determine which libs should be linked in
set(ot_libs
  openthread-platform-utils
  mbedcrypto
  )
if(CONFIG_OPENTHREAD_FTD)
  list(APPEND ot_libs openthread-ftd)
  set(cli_lib         openthread-cli-ftd)
elseif(CONFIG_OPENTHREAD_MTD)
  list(APPEND ot_libs openthread-mtd)
  set(cli_lib         openthread-cli-mtd)
endif()

if(CONFIG_OPENTHREAD_SHELL)
  list(APPEND ot_libs ${cli_lib})
endif()

foreach(ot_lib ${ot_libs})
  set(build_byproducts ${build_byproducts} ${ot_INSTALL_DIR}/${exec_prefix}/lib/lib${ot_lib}.a)
endforeach()

list(APPEND cmd
  BUILD_BYPRODUCTS ${build_byproducts}
  )

ExternalProject_Add(${cmd})

ExternalProject_Add_Step(
  ${ot_name} bootstrap # Names of project and custom step
  COMMAND ./bootstrap     # Command line invoked by this step
  COMMENT "bootstrapping..." # Text printed when step executes
  DEPENDEES download        # Steps on which this step depends
  DEPENDERS configure     # Steps that depend on this step
  WORKING_DIRECTORY ${ot_SOURCE_DIR}
  )

# Create wrapper CMake libraries
foreach(ot_lib ${ot_libs})
  add_library(${ot_lib} STATIC IMPORTED GLOBAL)
  add_dependencies(   # TODO: Necessary?
    ${ot_lib}
    ${ot_name}
    )
  set_target_properties(${ot_lib} PROPERTIES IMPORTED_LOCATION
    ${ot_INSTALL_DIR}/${exec_prefix}/lib/lib${ot_lib}.a
    )
  zephyr_append_cmake_library(${ot_lib})
endforeach()

add_subdirectory(platform)
