cmake_minimum_required(VERSION 3.6)
Project(Zehpyr-Host-Tools)

# fixdep is added because all files depend on autoconf.h,
# which would imply that changing the configuration triggers a complete rebuild.
# The build time is not that long though, and also fixdep would not work with Ninja
# as the build system generator because Ninja creates it's on header dependency cache.
# Suggestion is to stop using fixdep.
# add_executable(fixdep basic/fixdep.c)

add_subdirectory(kconfig)
