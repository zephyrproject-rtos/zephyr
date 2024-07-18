# SPDX-License-Identifier: Apache-2.0

# Rust make support

# Zephyr targets are defined through Kconfig.  We need to map these to
# an appropriate llbm target triple.  This sets `RUST_TARGET` in the
# parent scope, or an error if the target is not yet supported by
# Rust.
function(_rust_map_target)
  # Map Zephyr targets to LLVM targets.
  if(CONFIG_CPU_CORTEX_M)
    if(CONFIG_CPU_CORTEX_M0 OR CONFIG_CPU_CORTEX_M0PLUS OR CONFIG_CPU_CORTEX_M1)
      set(RUST_TARGET "thumbv6m-none-eabi" PARENT_SCOPE)
    elseif(CONFIG_CPU_CORTEX_M3)
      set(RUST_TARGET "thumbv7m-none-eabi" PARENT_SCOPE)
    elseif(CONFIG_CPU_CORTEX_M4 OR CONFIG_CPU_CORTEX_M7)
      if(CONFIG_FP_HARDABI OR FORCE_FP_HARDABI)
        set(RUST_TARGET "thumbv7em-none-eabihf" PARENT_SCOPE)
      else()
        set(RUST_TARGET "thumbv7em-none-eabi" PARENT_SCOPE)
      endif()
    elseif(CONFIG_CPU_CORTEX_M23)
      set(RUST_TARGET "thumbv8m.base-none-eabi" PARENT_SCOPE)
    elseif(CONFIG_CPU_CORTEX_M33 OR CONFIG_CPU_CORTEX_M55)
      # Not a typo, Zephyr, uses ARMV7_M_ARMV8_M_FP to select the FP even on v8m.
      if(CONFIG_FP_HARDABI OR FORCE_FP_HARDABI)
        set(RUST_TARGET "thumbv8m.main-none-eabihf" PARENT_SCOPE)
      else()
        set(RUST_TARGET "thumbv8m.main-none-eabi" PARENT_SCOPE)
      endif()

      # Todo: The M55 is thumbv8.1m.main-none-eabi, which can be added when Rust
      # gain support for this target.
    else()
      message(FATAL_ERROR "Unknown Cortex-M target.")
    endif()
  else()
    message(FATAL_ERROR "Rust: Add support for other target")
  endif()
endfunction()

function(rust_cargo_application)
  # For now, hard-code the Zephyr crate directly here.  Once we have
  # more than one crate, these should be added by the modules
  # themselves.
  set(LIB_RUST_CRATES zephyr zephyr-build zephyr-sys)

  _rust_map_target()
  message(STATUS "Building Rust llvm target ${RUST_TARGET}")

  # TODO: Make sure RUSTFLAGS is not set.

  # TODO: Let this be configurable, or based on Kconfig debug?
  set(RUST_BUILD_TYPE debug)
  set(BUILD_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${RUST_TARGET}/${RUST_BUILD_TYPE}")

  set(CARGO_TARGET_DIR "${CMAKE_CURRENT_BINARY_DIR}/rust/target")
  set(RUST_LIBRARY "${CARGO_TARGET_DIR}/${RUST_TARGET}/${RUST_BUILD_TYPE}/librustapp.a")
  set(SAMPLE_CARGO_CONFIG "${CMAKE_CURRENT_BINARY_DIR}/rust/sample-cargo-config.toml")

  # To get cmake to always invoke Cargo requires a bit of a trick.  We make the output of the
  # command a file that never gets created.  This will cause cmake to always rerun cargo.  We
  # add the actual library as a BYPRODUCTS list of this command, otherwise, the first time the
  # link will fail because it doesn't think it knows how to build the library.  This will also
  # cause the relink when the cargo command actually does rebuild the rust code.
  set(DUMMY_FILE "${CMAKE_BINARY_DIR}/always-run-cargo.dummy")

  # For each module in zephyr-rs, add entry both to the .cargo/config template and for the
  # command line, since either invocation will need to see these.
  set(command_paths)
  set(config_paths "")
  message(STATUS "Processing crates: ${ZEPHYR_RS_MODULES}")
  foreach(module IN LISTS LIB_RUST_CRATES)
    message(STATUS "module: ${module}")
    set(config_paths
      "${config_paths}\
${module}.path = \"${ZEPHYR_BASE}/lib/rust/${module}\"
")
    list(APPEND command_paths
      "--config"
      "patch.crates-io.${module}.path=\\\"${ZEPHYR_BASE}/lib/rust/${module}\\\""
      )
  endforeach()

  # Write out a cargo config file that can be copied into `.cargo/config.toml` (or made a
  # symlink) in the source directory to allow various IDE tools and such to work.  The build we
  # invoke will override these settings, in case they are out of date.  Everything set here
  # should match the arguments given to the cargo build command below.
  file(WRITE ${SAMPLE_CARGO_CONFIG} "
# This is a generated sample .cargo/config.toml file from the Zephyr build.
# At the time of generation, this represented the settings needed to allow
# a `cargo build` command to compile the rust code using the current Zephyr build.
# If any settings in the Zephyr build change, this could become out of date.
[build]
target = \"${RUST_TARGET}\"
target-dir = \"${CARGO_TARGET_DIR}\"

[env]
BUILD_DIR = \"${CMAKE_CURRENT_BINARY_DIR}\"
DOTCONFIG = \"${DOTCONFIG}\"
ZEPHYR_DTS = \"${ZEPHYR_DTS}\"

[patch.crates-io]
${config_paths}
")

  # The library is built by invoking Cargo.
  add_custom_command(
    OUTPUT ${DUMMY_FILE}
    BYPRODUCTS ${RUST_LIBRARY}
    COMMAND
      ${CMAKE_EXECUTABLE}
      env BUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}
      DOTCONFIG=${DOTCONFIG}
      ZEPHYR_DTS=${ZEPHYR_DTS}
      cargo build
      # TODO: release flag if release build
      # --release

      # Override the features according to the shield given. For a general case,
      # this will need to come from a variable or argument.
      # TODO: This needs to be passed in.
      # --no-default-features
      # --features ${SHIELD_FEATURE}

      # Set a replacement so that packages can just use `zephyr-sys` as a package
      # name to find it.
      ${command_paths}
      --target ${RUST_TARGET}
      --target-dir ${CARGO_TARGET_DIR}
    COMMENT "Building Rust application"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )

  add_custom_target(librustapp ALL
    DEPENDS ${DUMMY_FILE}
  )

  target_link_libraries(app PUBLIC -Wl,--allow-multiple-definition ${RUST_LIBRARY})
  add_dependencies(app librustapp)

  # Presumably, Rust applications will have no C source files, but cmake will require them.
  # Add an empty file so that this will build.  The main will come from the rust library.
  target_sources(app PRIVATE ${ZEPHYR_BASE}/lib/rust/main.c)

  # TODO: Make safer and nicer
  target_sources(app PRIVATE /tmp/bindgen/extern.c)
  target_include_directories(app PRIVATE ${ZEPHYR_BASE}/lib/rust/zephyr-sys)
endfunction()
