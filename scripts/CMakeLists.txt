cmake_minimum_required(VERSION 3.7)
Project(Zehpyr-Host-Tools)

# fixdep is added because all files depend on autoconf.h,
# which would imply that changing the configuration triggers a complete rebuild.
# The build time is not that long though, and also fixdep would not work with Ninja
# as the build system generator because Ninja creates it's on header dependency cache.
# Suggestion is to stop using fixdep.
# add_executable(fixdep basic/fixdep.c)

add_executable(gen_idt gen_idt/gen_idt.c gen_idt/version.c)
target_include_directories(gen_idt PRIVATE gen_idt ../include/arch/x86 ../include)
target_compile_definitions(gen_idt PRIVATE -DKERNEL_VERSION=0)
target_compile_options(gen_idt PRIVATE
  -Wall
  -Werror
  -g
  -Wno-unused-result
)

add_subdirectory(kconfig)
