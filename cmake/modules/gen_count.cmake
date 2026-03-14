# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# gen_count.cmake, reusable two-pass section-count code generation
#
# Provides two functions:
#
#   gen_section_count(
#       TARGET           <name>          # unique name; drives all internal target names
#       SECTION          <section-name>  # linker section, e.g. net_if_area
#       ITEM_TYPE        <c-type>        # C type for sizeof(), e.g. "struct net_if"
#       CONSUMERS        <file> ...      # .c files in the subsystem that #include
#                                        # the generated header (compiled under
#                                        # the net_if_consumers OBJECT library)
#
#       # --- Pass-1 source: choose ONE of the two options below ---
#
#       SOURCE_DIR       <dir>           # Option A: glob *.c under this dir (app use)
#
#       PRODUCER_TARGET  <cmake-target>  # Option B: reuse objects from an existing
#                                        # CMake target (subsystem / zephyr_library)
#   )
#
#   gen_section_count_add_consumer(
#       TARGET   <cmake-target>   # CMake target that owns the source files
#       HEADER   <name>           # TARGET name passed to gen_section_count()
#       FILES    <file> ...       # source files in TARGET that use the macro
#   )
#
#   Call gen_section_count_add_consumer() from any CMakeLists.txt where
#   source files use the generated macro but are compiled under a different
#   CMake target (e.g. the application's 'app' target).  This ensures ninja
#   recompiles those files when the header changes.
#
# Build flow (transparent to the user, single west build invocation):
#
#   Build 1 (automatic, internal):
#     configure  → header written with count = 1U sentinel (avoids zero-length arrays)
#     compile    → consumers built with count = 1
#     link       → zephyr.elf produced
#     POST_BUILD → script reads zephyr.elf, count changed (1 → real value)
#                  → header updated, cmake --build re-invoked automatically
#
#   Build 2 (automatic, triggered by POST_BUILD above):
#     compile    → all registered consumers older than header → recompiled
#     link       → correct zephyr.elf
#     POST_BUILD → count unchanged → no further re-invocation
#
#   Subsequent west build runs:
#     POST_BUILD → count unchanged → nothing recompiled
#
# The generated header is written to:
#   <build>/zephyr/include/generated/zephyr/<TARGET>_count.h

include_guard(GLOBAL)

# ---------------------------------------------------------------------------
# Internal helper which is called deferred with no arguments so cmake_language(DEFER)
# has nothing to expand in the deferred context (it does NOT expand ${} before
# storing argument strings). All baked values are read from a global property.
# ---------------------------------------------------------------------------
function(_gen_section_count_apply_hooks)
    get_property(_runners GLOBAL PROPERTY _GEN_SECTION_COUNT_HOOKS)
    foreach(_runner IN LISTS _runners)
        add_custom_command(
            TARGET  zephyr_pre0
            POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -P "${_runner}"
            COMMENT "gen_section_count: checking section count, rebuilding if changed"
            VERBATIM
        )
    endforeach()
endfunction()

# ---------------------------------------------------------------------------
# gen_section_count, main entry point (call once per section, in the
# CMakeLists.txt that owns the section, e.g. subsys/net/ip/CMakeLists.txt)
# ---------------------------------------------------------------------------
function(gen_section_count)
    cmake_parse_arguments(GEN
        ""
        "TARGET;SECTION;ITEM_TYPE;SOURCE_DIR;PRODUCER_TARGET"
        "CONSUMERS"
        ${ARGN}
    )

    foreach(_req TARGET SECTION ITEM_TYPE)
        if(NOT DEFINED GEN_${_req} OR GEN_${_req} STREQUAL "")
            message(FATAL_ERROR "gen_section_count: ${_req} is required")
        endif()
    endforeach()

    if(NOT DEFINED GEN_SOURCE_DIR AND NOT DEFINED GEN_PRODUCER_TARGET)
        message(FATAL_ERROR
            "gen_section_count: provide either SOURCE_DIR or PRODUCER_TARGET")
    endif()

    if(DEFINED GEN_SOURCE_DIR AND DEFINED GEN_PRODUCER_TARGET)
        message(FATAL_ERROR
            "gen_section_count: SOURCE_DIR and PRODUCER_TARGET are mutually exclusive")
    endif()

    # CONSUMERS is optional. A section may have no direct consumers
    # in the subsystem if all consuming files are registered via
    # gen_section_count_add_consumer() instead.
    if(NOT GEN_CONSUMERS AND NOT DEFINED GEN_PRODUCER_TARGET)
        message(FATAL_ERROR
            "gen_section_count: CONSUMERS must list at least one file "
            "when SOURCE_DIR is used")
    endif()

    # Derived names
    string(TOUPPER "${GEN_TARGET}" _target_upper)
    set(_header_name  "${GEN_TARGET}_count")
    set(_macro_name   "${_target_upper}_COUNT")
    set(_guard        "${_target_upper}_COUNT_H")

    set(_gen_header
        ${ZEPHYR_BINARY_DIR}/include/generated/zephyr/${_header_name}.h)
    set(_gen_header_tmp
        ${CMAKE_CURRENT_BINARY_DIR}/${_header_name}.h.tmp)
    set(_zephyr_elf
        ${ZEPHYR_BINARY_DIR}/zephyr.elf)

    set(_consumer_obj_target  ${GEN_TARGET}_consumers)

    set(_script ${ZEPHYR_BASE}/scripts/gen_section_count.py)
    if(NOT EXISTS "${_script}")
        message(FATAL_ERROR
            "gen_section_count: script not found at ${_script}")
    endif()

    # Initial header
    # Written at configure time so the consumer compiles cleanly on the first
    # build before zephyr.elf exists.
    #
    # Uses literal 1U (not 0U) to avoid zero-length arrays in structs on the
    # first build pass, zero-length arrays are a non-standard GCC extension.
    # This value is discarded after the re-invoked build compiles consumers
    # with the real count from zephyr.elf.
    #
    # Guard: if the file already exists it contains the real count from a
    # previous build's post-build step, do NOT overwrite it or the count
    # resets to 1 on every reconfigure.
    get_filename_component(_gen_header_dir "${_gen_header}" DIRECTORY)
    file(MAKE_DIRECTORY "${_gen_header_dir}")
    if(NOT EXISTS "${_gen_header}")
        file(WRITE "${_gen_header}"
            "/* Initial value, overridden after first successful build.\n"
            " * Do not edit: auto-generated by gen_count.cmake */\n"
            "#ifndef ${_guard}\n"
            "#define ${_guard}\n"
            "\n"
            "/** Number of items in the '${GEN_SECTION}' linker section. */\n"
            "#define ${_macro_name}  1U\n"
            "\n"
            "#endif /* ${_guard} */\n"
        )
    endif()

    # Pass-1 objects
    # Option A — SOURCE_DIR: create a fresh OBJECT library from globbed sources
    if(DEFINED GEN_SOURCE_DIR)
        file(GLOB_RECURSE _all_sources
            CONFIGURE_DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/${GEN_SOURCE_DIR}/*.c
        )
        set(_producer_sources ${_all_sources})
        foreach(_f IN LISTS GEN_CONSUMERS)
            if(IS_ABSOLUTE "${_f}")
                list(REMOVE_ITEM _producer_sources "${_f}")
            else()
                list(REMOVE_ITEM _producer_sources
                    "${CMAKE_CURRENT_SOURCE_DIR}/${_f}")
            endif()
        endforeach()

        if(NOT _producer_sources)
            message(WARNING
                "gen_section_count [${GEN_TARGET}]: no producer sources found "
                "under ${GEN_SOURCE_DIR}")
        endif()

        set(_producer_obj_target ${GEN_TARGET}_producers)
        add_library(${_producer_obj_target} OBJECT ${_producer_sources})
        target_link_libraries(${_producer_obj_target} PRIVATE zephyr_interface)
        target_include_directories(${_producer_obj_target} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/${GEN_SOURCE_DIR}
        )

    # Option B — PRODUCER_TARGET: reuse objects from a target already compiled
    # by zephyr_library() in the subsystem CMakeLists.txt.
    # No re-compilation, no duplicate symbols.
    else()
        if(NOT TARGET ${GEN_PRODUCER_TARGET})
            message(FATAL_ERROR
                "gen_section_count: PRODUCER_TARGET '${GEN_PRODUCER_TARGET}' "
                "is not a known CMake target")
        endif()
    endif()

    # Post-build runner script
    # Written at configure time while all variables are in scope.
    # cmake_language(DEFER) does NOT expand ${} before storing arguments so
    # we bake every value as a string literal into a helper .cmake script
    # that is invoked with cmake -P at build time.
    #
    # Logic:
    #   1. Run the Python script → write new header content to a .tmp file
    #   2. Compare .tmp against the current header
    #   3. If different: overwrite the header, then re-invoke cmake --build
    #      so that all registered consumers are recompiled with the correct
    #      count in the same west build invocation the user ran.
    #   4. If same: do nothing — no recompilation, no infinite loop.
    set(_runner
        ${CMAKE_CURRENT_BINARY_DIR}/${GEN_TARGET}_gen_count_runner.cmake)

    file(WRITE "${_runner}"
        "# Auto-generated by gen_count.cmake — do not edit\n"
        "\n"
        "# Step 1: generate new header content into a temp file\n"
        "execute_process(\n"
        "    COMMAND\n"
        "        \"${PYTHON_EXECUTABLE}\"\n"
        "        \"${_script}\"\n"
        "        --section \"${GEN_SECTION}\"\n"
        "        --type    \"${GEN_ITEM_TYPE}\"\n"
        "        --output  \"${_gen_header_tmp}\"\n"
        "        \"${_zephyr_elf}\"\n"
        "    COMMAND_ERROR_IS_FATAL ANY\n"
        ")\n"
        "\n"
        "# Step 2: compare against the current header\n"
        "file(READ \"${_gen_header}\"     _current)\n"
        "file(READ \"${_gen_header_tmp}\" _new)\n"
        "\n"
        "if(NOT _current STREQUAL _new)\n"
        "    # Step 3: count changed — update header and rebuild so all\n"
        "    # registered consumers are recompiled with the correct count\n"
        "    # in this same west build invocation.\n"
        "    message(STATUS\n"
        "        \"gen_section_count [${GEN_TARGET}]: \"\n"
        "        \"count changed, updating header and re-invoking build\")\n"
        "    file(COPY_FILE \"${_gen_header_tmp}\" \"${_gen_header}\")\n"
        "    execute_process(\n"
        "        COMMAND \"${CMAKE_COMMAND}\" --build \"${CMAKE_BINARY_DIR}\"\n"
        "        COMMAND_ERROR_IS_FATAL ANY\n"
        "    )\n"
        "else()\n"
        "    # Step 4: count unchanged — nothing to do.\n"
        "    message(STATUS\n"
        "        \"gen_section_count [${GEN_TARGET}]: count unchanged\")\n"
        "endif()\n"
    )

    # Register runner in a global property — survives function scope exit
    set_property(GLOBAL APPEND PROPERTY _GEN_SECTION_COUNT_HOOKS "${_runner}")

    # Defer the add_custom_command(TARGET zephyr_pre0 ...) to the directory
    # where the zephyr_pre0 executable target is defined (ZEPHYR_BASE).
    # Only one deferred call is needed regardless of how many times
    # gen_section_count() is called — _gen_section_count_apply_hooks iterates
    # over all registered runners.
    get_property(_already_deferred GLOBAL PROPERTY _GEN_SECTION_COUNT_DEFERRED)
    if(NOT _already_deferred)
        set_property(GLOBAL PROPERTY _GEN_SECTION_COUNT_DEFERRED TRUE)
        cmake_language(DEFER DIRECTORY "${ZEPHYR_BASE}" CALL
            _gen_section_count_apply_hooks)
    endif()

    # Pass-2 consumers
    # On the first build compiled with the sentinel count = 1.
    # The POST_BUILD runner updates the header and re-invokes cmake --build,
    # at which point ninja sees these .obj files are older than the updated
    # header (via OBJECT_DEPENDS) and recompiles them automatically.
    if(GEN_CONSUMERS)
        add_library(${_consumer_obj_target} OBJECT ${GEN_CONSUMERS})
	target_link_libraries(${_consumer_obj_target} PRIVATE zephyr_interface)
	target_include_directories(${_consumer_obj_target} PRIVATE
            ${ZEPHYR_BINARY_DIR}/include/generated/zephyr/..  # real header
            ${CMAKE_CURRENT_SOURCE_DIR}                       # local headers
	  )

	  # Explicit ninja dependency: recompile consumer .obj when header changes.
	  # Required because CMakeFiles/ for OBJECT libraries may not generate .d
	  # depfiles on all platforms/generators.
	  set_source_files_properties(${GEN_CONSUMERS}
            TARGET_DIRECTORY ${_consumer_obj_target}
            PROPERTIES OBJECT_DEPENDS "${_gen_header}"
	  )

	  # Attach consumer objects to the top-level zephyr target.
	  # Using zephyr (not ZEPHYR_CURRENT_LIBRARY) avoids a dependency cycle:
	  # zephyr sits above the partial-link chain and never feeds back into it.
	  target_link_libraries(zephyr PRIVATE ${_consumer_obj_target})
    endif()

    message(STATUS
        "gen_section_count [${GEN_TARGET}]: "
        "section='${GEN_SECTION}'  "
        "type='${GEN_ITEM_TYPE}'  "
        "macro='${_macro_name}'  "
        "header='${_gen_header}'"
    )
endfunction()

# ---------------------------------------------------------------------------
# gen_section_count_add_consumer, register additional source files from any
# CMake target as dependents of a generated header.
#
# Call this from application or subsystem CMakeLists.txt files where source
# files use the generated macro but are compiled under a different CMake
# target than the one created by gen_section_count() (e.g. 'app').
# ---------------------------------------------------------------------------
function(gen_section_count_add_consumer)
    cmake_parse_arguments(ACC
        ""
        "TARGET;HEADER"
        "FILES"
        ${ARGN}
    )

    foreach(_req TARGET HEADER)
        if(NOT DEFINED ACC_${_req} OR ACC_${_req} STREQUAL "")
            message(FATAL_ERROR
                "gen_section_count_add_consumer: ${_req} is required")
        endif()
    endforeach()

    if(NOT ACC_FILES)
        message(FATAL_ERROR
            "gen_section_count_add_consumer: FILES must list at least one file")
    endif()

    set(_gen_header
        ${ZEPHYR_BINARY_DIR}/include/generated/zephyr/${ACC_HEADER}_count.h)

    if(NOT EXISTS "${_gen_header}")
        message(FATAL_ERROR
            "gen_section_count_add_consumer: header not found: ${_gen_header}\n"
            "Has gen_section_count(TARGET ${ACC_HEADER} ...) been called?")
    endif()

    # Inject the header as an explicit ninja dependency for each listed file.
    # ninja will recompile the .obj whenever the header timestamp advances
    # (i.e. after the POST_BUILD runner updates the count).
    set_source_files_properties(${ACC_FILES}
        TARGET_DIRECTORY ${ACC_TARGET}
        PROPERTIES OBJECT_DEPENDS "${_gen_header}"
    )

    message(STATUS
        "gen_section_count_add_consumer [${ACC_HEADER}]: "
        "registered ${ACC_FILES} under target '${ACC_TARGET}'"
    )
endfunction()
