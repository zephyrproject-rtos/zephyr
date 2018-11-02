# Copyright (c) 2018..2020 Bobby Noelte.
# SPDX-License-Identifier: Apache-2.0

# utilities
function(cogeno_unique_target_name_from_filename filename target_name)
    get_filename_component(basename ${filename} NAME)
    string(REPLACE "." "_" x ${basename})
    string(REPLACE "@" "_" x ${x})

    string(MD5 unique_chars ${filename})

    set(${target_name} cogeno_${x}_${unique_chars} PARENT_SCOPE)
endfunction()

# 1st priority: cogeno as a Zephyr module
if(ZEPHYR_MODULES)
    foreach(module ${ZEPHYR_MODULES})
        set(full_path ${module}/cogeno/cogeno.py)
        if(EXISTS ${full_path})
            set(COGENO_BASE "${module}" CACHE INTERNAL "cogeno base directory")
            set(COGENO_PY ${full_path})
            break()
        endif()
    endforeach()
endif()

# 2nd priority: cogeno installed side by side to Zephyr
# (but not as a Zephyr module)
if(NOT EXISTS "${COGENO_PY}")
    FILE(GLOB modules LIST_DIRECTORIES true "${ZEPHYR_BASE}/../*")
    foreach(module ${modules})
        set(full_path ${module}/cogeno/cogeno.py)
        if(EXISTS ${full_path})
            set(COGENO_BASE "${module}" CACHE INTERNAL "cogeno base directory")
            set(COGENO_PY ${full_path})
            break()
        endif()
    endforeach()
endif()

# 3rd prioritity: get cogeno from the git repository
# (this is a work around as long as cogeno is not installed as one of above.)
if(NOT EXISTS "${COGENO_PY}")
    find_package(Git)
    if(NOT GIT_FOUND)
        message(FATAL_ERROR "git not found!")
    endif()
    execute_process(
        COMMAND             ${GIT_EXECUTABLE} clone https://gitlab.com/b0661/cogeno.git --recursive
        WORKING_DIRECTORY   "${ZEPHYR_BASE}/.."
        OUTPUT_VARIABLE     git_output)
    message(STATUS "${git_output}")
    set(module "${ZEPHYR_BASE}/../cogeno")
    if (EXISTS ${module})
        execute_process(
            COMMAND             ${GIT_EXECUTABLE} checkout master
            WORKING_DIRECTORY   "${module}"
            OUTPUT_VARIABLE     git_output)
        message(STATUS "${git_output}")
        set(full_path ${module}/cogeno/cogeno.py)
        if(EXISTS ${full_path})
            set(COGENO_BASE "${module}" CACHE INTERNAL "cogeno base directory")
            set(COGENO_PY ${full_path})
        endif()
    endif()
endif()

# We also need a Python interpreter for cogeno
if(EXISTS "${COGENO_PY}")
    if(${CMAKE_VERSION} VERSION_LESS "3.12")
        set(Python_ADDITIONAL_VERSIONS 3.7 3.6 3.5)
        find_package(PythonInterp)

        set(Python3_Interpreter_FOUND ${PYTHONINTERP_FOUND})
        set(Python3_EXECUTABLE ${PYTHON_EXECUTABLE})
        set(Python3_VERSION ${PYTHON_VERSION_STRING})
    else()
        # CMake >= 3.12
        find_package(Python3 COMPONENTS Interpreter)
    endif()

    if(NOT ${Python3_Interpreter_FOUND})
        message(FATAL_ERROR "Python 3 not found")
    endif()

    set(COGENO_EXECUTABLE "${Python3_EXECUTABLE}")
    set(COGENO_EXECUTABLE_OPTION "${COGENO_PY}")
endif()

# 4th priority: cogeno installed on host
if(NOT EXISTS "${COGENO_PY}")
    find_program(COGENO_PY cogeno)

    if(EXISTS "${COGENO_PY}")
        set(COGENO_EXECUTABLE "${COGENO_PY}")
        # We do not need the Python 3 interpreter
        set(COGENO_EXECUTABLE_OPTION)
    endif()
endif()

if(NOT EXISTS "${COGENO_PY}")
    message(FATAL_ERROR "Cogeno not found - '${COGENO_PY}'.")
endif()

message(STATUS "Found cogeno: '${COGENO_PY}'.")

# Get all the files that make up cogeno for dependency reasons.
file(GLOB_RECURSE COGENO_SOURCES LIST_DIRECTORIES false
      ${COGENO_BASE}/cogeno/*.py
      ${COGENO_BASE}/cogeno/*.yaml
      ${COGENO_BASE}/cogeno/*.c
      ${COGENO_BASE}/cogeno/*.jinja2)

function(target_sources_cogeno
    target          # The CMake target that depends on the generated file
    )

    # defines
    if(NOT COGENO_CONFIG)
        set(COGENO_DEFINES
            "\"BOARD=${BOARD}\""
            "\"GENERATED_DTS_BOARD_H=${GENERATED_DTS_BOARD_H}\""
            "\"GENERATED_DTS_BOARD_CONF=${GENERATED_DTS_BOARD_CONF}\"")
    endif()
    set(options)
    set(oneValueArgs)
    set(multiValueArgs COGENO_DEFINES DEPENDS)
    cmake_parse_arguments(COGENO "${options}" "${oneValueArgs}"
                          "${multiValueArgs}" ${ARGN})
    # prepend -D to all defines
    string(REGEX REPLACE "([^;]+)" "-D;\\1"
           cogeno_opt_defines "${COGENO_COGENO_DEFINES}")

    # --config
    if(COGENO_CMAKECACHE)
        set(cogeno_opt_cmakecache "--cmakecache" "${COGENO_CMAKECACHE}")
    else()
        set(cogeno_opt_cmakecache "--cmakecache" "${CMAKE_BINARY_DIR}/CMakeCache.txt")
    endif()

    # --config
    if(COGENO_CONFIG)
        set(cogeno_opt_config "--config" "${COGENO_CONFIG}")
    else()
        set(cogeno_opt_config "--config" "${PROJECT_BINARY_DIR}/.config")
    endif()

    # --dts
    # --bindings
    # --edts
    if(COGENO_DTS)
        set(cogeno_opt_dts "--dts" "${COGENO_DTS}")
    else()
        set(cogeno_opt_dts "--dts" "${PROJECT_BINARY_DIR}/${BOARD}.dts.pre.tmp")
    endif()
    if(COGENO_BINDINGS)
        set(cogeno_opt_bindings "--bindings" ${COGENO_BINDINGS})
    else()
        string (REPLACE "?" ";" cogeno_opt_bindings "${DTS_ROOT_BINDINGS}")
        set(cogeno_opt_bindings "--bindings"  ${cogeno_opt_bindings})
    endif()
    if(COGENO_EDTS)
        set(cogeno_opt_edts "--edts" "${COGENO_EDTS}")
    else()
        set(cogeno_opt_edts "--edts" "${CMAKE_BINARY_DIR}/edts.json")
    endif()

    # --modules
    if(COGENO_MODULES)
        set(cogeno_opt_modules "--modules" ${COGENO_MODULES})
    else()
        if(EXISTS "${APPLICATION_SOURCE_DIR}/templates")
           list(APPEND COGENO_MODULES "${APPLICATION_SOURCE_DIR}/templates")
        endif()
        if(EXISTS "${PROJECT_SOURCE_DIR}/templates")
            list(APPEND COGENO_MODULES "${PROJECT_SOURCE_DIR}/templates")
        endif()
        if(COGENO_MODULES)
            set(cogeno_opt_modules "--modules" ${COGENO_MODULES})
        else()
            set(cogeno_opt_modules)
        endif()
    endif()

    # --templates
    if(COGENO_TEMPLATES)
        set(cogeno_opt_templates "--templates" ${COGENO_TEMPLATES})
    else()
        if(EXISTS "${APPLICATION_SOURCE_DIR}/templates")
            list(APPEND COGENO_TEMPLATES "${APPLICATION_SOURCE_DIR}/templates")
        endif()
        if(EXISTS "${PROJECT_SOURCE_DIR}/templates")
            list(APPEND COGENO_TEMPLATES "${PROJECT_SOURCE_DIR}/templates")
        endif()
        if(COGENO_TEMPLATES)
            set(cogeno_opt_templates "--templates" ${COGENO_TEMPLATES})
        else()
            set(cogeno_opt_templates)
        endif()
    endif()

    message(STATUS "Will generate for target ${target}")
    # Generated file must be generated to the current binary directory.
    # Otherwise this would trigger CMake issue #14633:
    # https://gitlab.kitware.com/cmake/cmake/issues/14633
    foreach(arg ${COGENO_UNPARSED_ARGUMENTS})
        if(IS_ABSOLUTE ${arg})
            set(template_file ${arg})
            get_filename_component(generated_file_name ${arg} NAME)
            set(generated_file ${CMAKE_CURRENT_BINARY_DIR}/${generated_file_name})
        else()
            set(template_file ${CMAKE_CURRENT_SOURCE_DIR}/${arg})
            set(generated_file ${CMAKE_CURRENT_BINARY_DIR}/${arg})
        endif()
        get_filename_component(template_dir ${template_file} DIRECTORY)
        get_filename_component(generated_dir ${generated_file} DIRECTORY)

        if(IS_DIRECTORY ${template_file})
            message(FATAL_ERROR "target_sources_cogeno() was called on a directory")
        endif()

        # Generate file from template
        message(STATUS " from '${template_file}'")
        message(STATUS " to   '${generated_file}'")
        add_custom_command(
            COMMENT "cogeno ${generated_file}"
            OUTPUT ${generated_file}
            MAIN_DEPENDENCY ${template_file}
            DEPENDS
            DEPENDS
            ${COGENO_DEPENDS}
            ${COGENO_SOURCES}
            COMMAND
            ${COGENO_EXECUTABLE}
            ${COGENO_EXECUTABLE_OPTION}
            --input "${template_file}"
            --output "${generated_file}"
            --log "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/cogeno.log"
            --lock "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/cogeno.lock"
            ${cogeno_opt_defines}
            -D "\"APPLICATION_SOURCE_DIR=${APPLICATION_SOURCE_DIR}\""
            -D "\"APPLICATION_BINARY_DIR=${APPLICATION_BINARY_DIR}\""
            -D "\"PROJECT_NAME=${PROJECT_NAME}\""
            -D "\"PROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}\""
            -D "\"PROJECT_BINARY_DIR=${PROJECT_BINARY_DIR}\""
            -D "\"CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR}\""
            -D "\"CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}\""
            -D "\"CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}\""
            -D "\"CMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}\""
            -D "\"CMAKE_CURRENT_LIST_DIR=${CMAKE_CURRENT_LIST_DIR}\""
            -D "\"CMAKE_FILES_DIRECTORY=${CMAKE_FILES_DIRECTORY}\""
            -D "\"CMAKE_PROJECT_NAME=${CMAKE_PROJECT_NAME}\""
            -D "\"CMAKE_SYSTEM=${CMAKE_SYSTEM}\""
            -D "\"CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}\""
            -D "\"CMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION}\""
            -D "\"CMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}\""
            -D "\"CMAKE_C_COMPILER=${CMAKE_C_COMPILER}\""
            -D "\"CMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}\""
            -D "\"CMAKE_COMPILER_IS_GNUCC=${CMAKE_COMPILER_IS_GNUCC}\""
            -D "\"CMAKE_COMPILER_IS_GNUCXX=${CMAKE_COMPILER_IS_GNUCXX}\""
            ${cogeno_opt_cmakecache}
            ${cogeno_opt_config}
            ${cogeno_opt_dts}
            ${cogeno_opt_bindings}
            ${cogeno_opt_edts}
            ${cogeno_opt_modules}
            ${cogeno_opt_templates}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )

        set_source_files_properties(${generated_file} PROPERTIES GENERATED 1)
        if("${target}" STREQUAL "zephyr")
          # Zephyr is special
          cogeno_unique_target_name_from_filename(${generated_file} generated_target_name)
          add_custom_target(${generated_target_name} DEPENDS ${generated_file})
          add_dependencies(zephyr ${generated_target_name})
          # Remember all the files that are generated for zephyr.
          # target_sources(zephyr PRIVATE ${ZEPHYR_GENERATED_SOURCE_FILES})
          # is executed in the top level Zephyr CMakeFile.txt context.
          set_property(GLOBAL APPEND PROPERTY ZEPHYR_GENERATED_SOURCE_FILES ${generated_file})
          # Add template directory to include path to allow includes with
          # relative path in generated file to work
          zephyr_include_directories(${template_dir})
          # Add directory of generated file to include path to allow includes
          # of generated header file with relative path.
          zephyr_include_directories(${generated_dir})
        else()
          target_sources(${target} PRIVATE ${generated_file})
          # Add template directory to include path to allow includes with
          # relative path in generated file to work
          target_include_directories(${target} PRIVATE ${template_dir})
          # Add directory of generated file to include path to allow includes
          # of generated header file with relative path.
          target_include_directories(${target} PRIVATE ${generated_dir})
        endif()
    endforeach()
endfunction()

function(zephyr_sources_cogeno)
    target_sources_cogeno(zephyr ${ARGN})
endfunction()

function(zephyr_sources_cogeno_ifdef feature_toggle)
    if(${${feature_toggle}})
        zephyr_sources_cogeno(${ARGN})
    endif()
endfunction()

function(zephyr_library_sources_cogeno)
    target_sources_cogeno(${ZEPHYR_CURRENT_LIBRARY} ${ARGN})
endfunction()

function(zephyr_library_sources_cogeno_ifdef feature_toggle)
    if(${${feature_toggle}})
        zephyr_library_sources_cogeno(${ARGN})
    endif()
endfunction()
