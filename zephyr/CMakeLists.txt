# SPDX-License-Identifier: Apache-2.0

# The Zephyr repository must use the repository-root CMakeLists.txt
# root to hook the build system into the application-provided CMakeLists.txt.
#
# The module-interface CMakeLists.txt (this file) is located in
# the same directory as `module.yml` to help indicate scope.
# Because the Zephyr directory tree is located above the location
# of the module-interface CMakeLists.txt all the `add_subdirectory()`
# calls must provide an path to the binary output directory.

zephyr_include_directories(../include)

# Unfortunately, the order in which CMakeLists.txt code is processed
# matters so we need to be careful about how we order the processing
# of subdirectories. One example is "Compiler flags added late in the
# build are not exported to external build systems #5605"; when we
# integrate with an external build system we read out all compiler
# flags when the external project is created. So an external project
# defined in subsys or ext will not get global flags added by drivers/
# or tests/ as the subdirectories are ordered now.
#
# Another example of when the order matters is the reading and writing
# of global properties such as ZEPHYR_LIBS or
# GENERATED_KERNEL_OBJECT_FILES.
#
# Arch is placed early because it defines important compiler flags
# that must be exported to external build systems defined in
# e.g. subsys/.
add_subdirectory(../arch ${__build_dir}/arch)
add_subdirectory(../lib ${__build_dir}/lib)
# We use include instead of add_subdirectory to avoid creating a new directory scope.
# This is because source file properties are directory scoped, including the GENERATED
# property which is set implicitly for custom command outputs
include(../misc/generated/CMakeLists.txt)

if(EXISTS ${SOC_DIR}/${ARCH}/CMakeLists.txt)
  add_subdirectory(${SOC_DIR}/${ARCH} ${__build_dir}/soc/${ARCH})
else()
  add_subdirectory(${SOC_DIR}/${ARCH}/${SOC_PATH} ${__build_dir}/soc/${ARCH}/${SOC_PATH})
endif()

add_subdirectory(../boards ${__build_dir}/boards)
add_subdirectory(../drivers ${__build_dir}/drivers)
add_subdirectory(../subsys ${__build_dir}/subsys)
