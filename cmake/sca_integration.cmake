# SPDX-License-Identifier: Apache-2.0

function(write_sca_export_jsons SCA_IN_TREE_INTEGRATION_DIR SCA_OUT_OF_TREE_INTEGRATION_DIR)
  # Write current selected toolchain/compiler to export file
  message(STATUS "Reading '${SCA_IN_TREE_INTEGRATION_DIR}/sca_export.json.in'")
  # Read the json
  file(READ ${SCA_IN_TREE_INTEGRATION_DIR}/sca_export.json.in SCA_EXPORT_JSON)
  #message(STATUS "read result: ${SCA_EXPORT_JSON}")
  # Modify the json
  string(TIMESTAMP DATE_TIME "%Y-%m-%d %H:%M:%S")
  string(JSON SCA_EXPORT_JSON ERROR_VARIABLE ERR_VAR SET ${SCA_EXPORT_JSON} "Build" "TIMESTAMP" \"${DATE_TIME}\")
  string(JSON SCA_EXPORT_JSON ERROR_VARIABLE ERR_VAR SET ${SCA_EXPORT_JSON} "Build" "C_COMPILER" \"${CMAKE_C_COMPILER}\")
  string(JSON SCA_EXPORT_JSON ERROR_VARIABLE ERR_VAR SET ${SCA_EXPORT_JSON} "Build" "CXX_COMPILER" \"${CMAKE_CXX_COMPILER}\")
  string(JSON SCA_EXPORT_JSON ERROR_VARIABLE ERR_VAR SET ${SCA_EXPORT_JSON} "Build" "ASM_COMPILER" \"${CMAKE_ASM_COMPILER}\")
  string(JSON SCA_EXPORT_JSON ERROR_VARIABLE ERR_VAR SET ${SCA_EXPORT_JSON} "Build" "LINKER" \"${CMAKE_LINKER}\")
  #message(STATUS "err-var: ${ERR_VAR}")
  #message(STATUS "json: ${SCA_EXPORT_JSON}")

  # Write the in tree json
  file(WRITE ${SCA_IN_TREE_INTEGRATION_DIR}/sca_export.json ${SCA_EXPORT_JSON})
  message(STATUS "Writing '${SCA_IN_TREE_INTEGRATION_DIR}/sca_export.json'")

  # Write the out of tree json
  if(NOT ${SCA_OUT_OF_TREE_INTEGRATION_DIR} MATCHES ".*-NOTFOUND")
    file(WRITE ${SCA_OUT_OF_TREE_INTEGRATION_DIR}/sca_export.json ${SCA_EXPORT_JSON})
    message(STATUS "Writing '${SCA_OUT_OF_TREE_INTEGRATION_DIR}/sca_export.json'")
  endif()
endfunction()

function(exec_sca_hook_scripts SCA_INTEGRATION_DIR)
  if(${SCA_INTEGRATION_DIR} MATCHES ".*-NOTFOUND")
    message(STATUS "SCA_INTEGRATION_DIR: ${SCA_INTEGRATION_DIR} - bailing out!")
    return()
  endif()
  string(TOLOWER "${SCA_VARIANT}" LC_SCA_VARIANT)
  set(SCA_VARIANT_HOOKS_DIR  "${SCA_INTEGRATION_DIR}/hooks/${LC_SCA_VARIANT}")
# Update the SCA configuration if required, run any matching script!
  set(SCA_UPDATE_SCRIPT_EXTS sh bat py rb pl js ps1)
  foreach(SCA_UPDATE_SCRIPT_EXT ${SCA_UPDATE_SCRIPT_EXTS})
    # unset SCA_UPDATE_SCRIPT everytime before find_program to force new search!
    unset(SCA_UPDATE_SCRIPT CACHE)
    string(TOLOWER "update.${SCA_UPDATE_SCRIPT_EXT}" SCA_UPDATE_SCRIPT_NAME)
    find_program(SCA_UPDATE_SCRIPT ${SCA_UPDATE_SCRIPT_NAME} PATHS ${SCA_VARIANT_HOOKS_DIR} NO_CACHE)
    if(NOT ${SCA_UPDATE_SCRIPT} MATCHES ".*-NOTFOUND")
      execute_process(COMMAND ${SCA_UPDATE_SCRIPT} ${SCA_INTEGRATION_DIR}/sca_export.json
      WORKING_DIRECTORY ${SCA_INTEGRATION_DIR}
      RESULT_VARIABLE SCA_UPDATE_SCRIPT_RETVAL)
      # In accordance with https://tldp.org/LDP/abs/html/exitcodes.html for the user-defined exit code range
      if(${SCA_UPDATE_SCRIPT_RETVAL} EQUAL 0)
        # The script was successful. No further scripts will be executed.
        break()
      elseif(${SCA_UPDATE_SCRIPT_RETVAL} EQUAL 111)
        # The script wants more scripts to be executed.
      else()
        # The script was unsuccessful. The build will be terminated.
        message(FATAL_ERROR "The script '${SCA_UPDATE_SCRIPT_NAME}' returned '${SCA_UPDATE_SCRIPT_RETVAL}'")
      endif()
    else()
        message(STATUS "'${SCA_UPDATE_SCRIPT_NAME}' in '${SCA_VARIANT_HOOKS_DIR}' not found or not an executable!")
    endif()
  endforeach()
endfunction()

function(read_sca_import_json SCA_INTEGRATION_DIR)
  if(${SCA_INTEGRATION_DIR} MATCHES ".*-NOTFOUND")
    message(STATUS "SCA_INTEGRATION_DIR: ${SCA_INTEGRATION_DIR} - bailing out!")
    return()
  endif()
# read SCA integration import file
  message(STATUS "Reading '${SCA_INTEGRATION_DIR}/sca_import.json'")
  file(READ ${SCA_INTEGRATION_DIR}/sca_import.json SCA_IMPORT_JSON)
  #message(STATUS ${SCA_IMPORT_JSON})

  # read the variables from the json
  string(JSON SCA_CONFIG ERROR_VARIABLE ERR_VAR GET ${SCA_IMPORT_JSON} "SCAs" ${SCA_VARIANT})
  #message(STATUS "SCA_CONFIG: ${SCA_CONFIG}")

  if(${SCA_CONFIG} MATCHES ".*-NOTFOUND")
    message(STATUS "Config not found!")
  else()
    message(STATUS "Config found!")
    set(SCA_CONFIG_FOUND TRUE PARENT_SCOPE)
    string(JSON SCA_C_COMPILER ERROR_VARIABLE ERR_VAR GET ${SCA_IMPORT_JSON} "SCAs" ${SCA_VARIANT} "C_COMPILER")
    message(STATUS "SCA_C_COMPILER: ${SCA_C_COMPILER}")

    string(JSON SCA_CXX_COMPILER ERROR_VARIABLE ERR_VAR GET ${SCA_IMPORT_JSON} "SCAs" ${SCA_VARIANT} "CXX_COMPILER")
    message(STATUS "SCA_CXX_COMPILER: ${SCA_CXX_COMPILER}")

    string(JSON SCA_ASM_COMPILER ERROR_VARIABLE ERR_VAR GET ${SCA_IMPORT_JSON} "SCAs" ${SCA_VARIANT} "ASM_COMPILER")
    message(STATUS "SCA_ASM_COMPILER: ${SCA_ASM_COMPILER}")

    string(JSON SCA_LINKER ERROR_VARIABLE ERR_VAR GET ${SCA_IMPORT_JSON} "SCAs" ${SCA_VARIANT} "LINKER")
    message(STATUS "SCA_LINKER: ${SCA_LINKER}")

    # replace the current selected toolchain/compiler by SCA executables
    if(${SCA_C_COMPILER})
      #set(CMAKE_C_COMPILER ${SCA_C_COMPILER})
    endif()
    if(${SCA_CXX_COMPILER})
      #set(CMAKE_CXX_COMPILER ${SCA_CXX_COMPILER})
    endif()
    if(${SCA_ASM_COMPILER})
      #set(CMAKE_ASM_COMPILER ${SCA_ASM_COMPILER})
    endif()
    if(${SCA_LINKER})
      #set(CMAKE_LINKER ${SCA_LINKER})
    endif()
  endif()
endfunction()

if(DEFINED SCA_VARIANT)
  message(STATUS "*** Start SCA integration for '${SCA_VARIANT}' ***")
  set(SCA_IN_TREE_INTEGRATION_DIR "${ZEPHYR_BASE}/scripts/sca_integration")
  if(DEFINED ENV{SCA_INTEGRATION_DIR})
  set(SCA_OUT_OF_TREE_INTEGRATION_DIR $ENV{SCA_INTEGRATION_DIR})
  else()
    set(SCA_OUT_OF_TREE_INTEGRATION_DIR "-NOTFOUND")
    message(STATUS "No SCA integration folder defined!")
  endif()

  write_sca_export_jsons(${SCA_IN_TREE_INTEGRATION_DIR} ${SCA_OUT_OF_TREE_INTEGRATION_DIR})

  message(STATUS "Calling 'read_sca_import_json' (out of tree): ${SCA_OUT_OF_TREE_INTEGRATION_DIR}")
  read_sca_import_json(${SCA_OUT_OF_TREE_INTEGRATION_DIR})
  if(${SCA_CONFIG_FOUND})
    message(STATUS "Calling 'exec_sca_hook_scripts' ${SCA_OUT_OF_TREE_INTEGRATION_DIR}")
    exec_sca_hook_scripts(${SCA_OUT_OF_TREE_INTEGRATION_DIR})
  else()
    message(STATUS "Calling 'read_sca_import_json' (in tree): ${SCA_IN_TREE_INTEGRATION_DIR}")
    read_sca_import_json(${SCA_IN_TREE_INTEGRATION_DIR})
    if(${SCA_CONFIG_FOUND})
      message(STATUS "Calling 'exec_sca_hook_scripts' ${SCA_IN_TREE_INTEGRATION_DIR}")
      exec_sca_hook_scripts(${SCA_IN_TREE_INTEGRATION_DIR})
    endif()
  endif()
  message(STATUS "*** SCA integration for: '${SCA_VARIANT}' done! ***")
else()
  message(STATUS "No SCA integration (SCA_VARIANT undefined)")
endif()



