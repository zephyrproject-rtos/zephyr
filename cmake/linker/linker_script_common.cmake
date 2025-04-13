find_package(Zephyr REQUIRED COMPONENTS extensions HINTS ${CMAKE_CURRENT_LIST_DIR}/../../)

#
# Create functions - start
#
function(create_system)
  cmake_parse_arguments(OBJECT "" "ENTRY;FORMAT;NAME;OBJECT" "" ${ARGN})

  set_property(GLOBAL PROPERTY SYSTEM_${OBJECT_NAME}          TRUE)
  set_property(GLOBAL PROPERTY SYSTEM_${OBJECT_NAME}_OBJ_TYPE SYSTEM)
  set_property(GLOBAL PROPERTY SYSTEM_${OBJECT_NAME}_NAME     ${OBJECT_NAME})
  set_property(GLOBAL PROPERTY SYSTEM_${OBJECT_NAME}_FORMAT   ${OBJECT_FORMAT})
  set_property(GLOBAL PROPERTY SYSTEM_${OBJECT_NAME}_ENTRY    ${OBJECT_ENTRY})

  set(${OBJECT_OBJECT} SYSTEM_${OBJECT_NAME} PARENT_SCOPE)
endfunction()

function(create_region)
  cmake_parse_arguments(OBJECT "" "NAME;OBJECT;SIZE;START;FLAGS" "" ${ARGN})

  if(DEFINED OBJECT_SIZE)
    if(${OBJECT_SIZE} MATCHES "^([0-9]*)[kK]$")
      math(EXPR OBJECT_SIZE "1024 * ${CMAKE_MATCH_1}" OUTPUT_FORMAT HEXADECIMAL)
    elseif(${OBJECT_SIZE} MATCHES "^([0-9]*)[mM]$")
      math(EXPR OBJECT_SIZE "1024 * 1024 * ${CMAKE_MATCH_1}" OUTPUT_FORMAT HEXADECIMAL)
    elseif(NOT (${OBJECT_SIZE} MATCHES "^([0-9]*)$" OR ${OBJECT_SIZE} MATCHES "^0x([0-9a-fA-F]*)$"))
      message(FATAL_ERROR "SIZE format is unknown.")
    endif()
  endif()

  set_property(GLOBAL PROPERTY REGION_${OBJECT_NAME}          TRUE)
  set_property(GLOBAL PROPERTY REGION_${OBJECT_NAME}_OBJ_TYPE REGION)
  set_property(GLOBAL PROPERTY REGION_${OBJECT_NAME}_NAME     ${OBJECT_NAME})
  set_property(GLOBAL PROPERTY REGION_${OBJECT_NAME}_ADDRESS  ${OBJECT_START})
  set_property(GLOBAL PROPERTY REGION_${OBJECT_NAME}_FLAGS    ${OBJECT_FLAGS})
  set_property(GLOBAL PROPERTY REGION_${OBJECT_NAME}_SIZE     ${OBJECT_SIZE})

  set(${OBJECT_OBJECT} REGION_${OBJECT_NAME} PARENT_SCOPE)
endfunction()

function(get_parent)
  cmake_parse_arguments(GET_PARENT "" "OBJECT;PARENT;TYPE" "" ${ARGN})

  get_property(type GLOBAL PROPERTY ${GET_PARENT_OBJECT}_OBJ_TYPE)
  if(${type} STREQUAL ${GET_PARENT_TYPE})
    # Already the right type, so just set and return.
    set(${GET_PARENT_PARENT} ${GET_PARENT_OBJECT} PARENT_SCOPE)
    return()
  endif()

  # Follow parent pointers until a GET_PARENT_TYPE is reached
  get_property(parent GLOBAL PROPERTY ${GET_PARENT_OBJECT}_PARENT)
  get_property(type   GLOBAL PROPERTY ${parent}_OBJ_TYPE)
  while(NOT ${type} STREQUAL ${GET_PARENT_TYPE})
    get_property(parent GLOBAL PROPERTY ${parent}_PARENT)
    get_property(type   GLOBAL PROPERTY ${parent}_OBJ_TYPE)
  endwhile()

  set(${GET_PARENT_PARENT} ${parent} PARENT_SCOPE)
endfunction()

function(create_group)
  cmake_parse_arguments(OBJECT "" "GROUP;LMA;NAME;OBJECT;SYMBOL;VMA" "" ${ARGN})

  set_property(GLOBAL PROPERTY GROUP_${OBJECT_NAME}          TRUE)
  set_property(GLOBAL PROPERTY GROUP_${OBJECT_NAME}_OBJ_TYPE GROUP)
  set_property(GLOBAL PROPERTY GROUP_${OBJECT_NAME}_NAME     ${OBJECT_NAME})
  set_property(GLOBAL PROPERTY GROUP_${OBJECT_NAME}_SYMBOL   ${OBJECT_SYMBOL})

  if(DEFINED OBJECT_GROUP)
    find_object(OBJECT parent NAME ${OBJECT_GROUP})
  else()
    if(DEFINED OBJECT_VMA)
      find_object(OBJECT obj NAME ${OBJECT_VMA})
      get_parent(OBJECT ${obj} PARENT parent TYPE REGION)

      get_property(vma GLOBAL PROPERTY ${parent}_NAME)
      set_property(GLOBAL PROPERTY GROUP_${OBJECT_NAME}_VMA ${vma})
    endif()

    if(DEFINED OBJECT_LMA)
      find_object(OBJECT obj NAME ${OBJECT_LMA})
      get_parent(OBJECT ${obj} PARENT parent TYPE REGION)

      get_property(lma GLOBAL PROPERTY ${parent}_NAME)
      set_property(GLOBAL PROPERTY GROUP_${OBJECT_NAME}_LMA ${lma})
    endif()
  endif()

  get_property(GROUP_FLAGS_INHERITED GLOBAL PROPERTY ${parent}_FLAGS)
  set_property(GLOBAL PROPERTY GROUP_${OBJECT_NAME}_FLAGS  ${GROUP_FLAGS_INHERITED})
  set_property(GLOBAL PROPERTY GROUP_${OBJECT_NAME}_PARENT ${parent})

  add_group(OBJECT ${parent} GROUP GROUP_${OBJECT_NAME})

  set(${OBJECT_OBJECT} GROUP_${OBJECT_NAME} PARENT_SCOPE)
endfunction()

function(is_active_in_pass ret_ptr current_pass pass_rules)
  # by validation in zephyr_linker_* we know that if there is a NOT,
  # it is the first, and the other entries are pass names
  if(NOT pass_rules)
    set(result 1)
  elseif("NOT" IN_LIST pass_rules)
    set(result 1)
    if(current_pass IN_LIST pass_rules)
      set(result 0)
    endif()
  else()
    set(result 0)
    if(current_pass IN_LIST pass_rules)
      set(result 1)
    endif()
  endif()
  set(${ret_ptr} ${result} PARENT_SCOPE)
endfunction()

function(create_section)
  set(single_args "NAME;ADDRESS;ALIGN_WITH_INPUT;TYPE;ALIGN;ENDALIGN;SUBALIGN;VMA;LMA;NOINPUT;NOINIT;NOSYMBOLS;GROUP;SYSTEM;MIN_SIZE;MAX_SIZE")
  set(multi_args  "PASS")

  cmake_parse_arguments(SECTION "" "${single_args}" "${multi_args}" ${ARGN})

  if(DEFINED SECTION_PASS)
    is_active_in_pass(active ${PASS} "${SECTION_PASS}")
    if(NOT active)
      return()
    endif()
  endif()

  set_property(GLOBAL PROPERTY SECTION_${SECTION_NAME} TRUE)
  set_property(GLOBAL PROPERTY SECTION_${SECTION_NAME}_OBJ_TYPE         SECTION)
  set_property(GLOBAL PROPERTY SECTION_${SECTION_NAME}_NAME             ${SECTION_NAME})
  set_property(GLOBAL PROPERTY SECTION_${SECTION_NAME}_ADDRESS          ${SECTION_ADDRESS})
  set_property(GLOBAL PROPERTY SECTION_${SECTION_NAME}_TYPE             ${SECTION_TYPE})
  set_property(GLOBAL PROPERTY SECTION_${SECTION_NAME}_ALIGN            ${SECTION_ALIGN})
  set_property(GLOBAL PROPERTY SECTION_${SECTION_NAME}_ALIGN_WITH_INPUT ${SECTION_ALIGN_WITH_INPUT})
  set_property(GLOBAL PROPERTY SECTION_${SECTION_NAME}_SUBALIGN         ${SECTION_SUBALIGN})
  set_property(GLOBAL PROPERTY SECTION_${SECTION_NAME}_ENDALIGN         ${SECTION_ENDALIGN})
  set_property(GLOBAL PROPERTY SECTION_${SECTION_NAME}_NOINPUT          ${SECTION_NOINPUT})
  set_property(GLOBAL PROPERTY SECTION_${SECTION_NAME}_NOINIT           ${SECTION_NOINIT})
  set_property(GLOBAL PROPERTY SECTION_${SECTION_NAME}_NOSYMBOLS        ${SECTION_NOSYMBOLS})
  set_property(GLOBAL PROPERTY SECTION_${SECTION_NAME}_MIN_SIZE         ${SECTION_MIN_SIZE})
  set_property(GLOBAL PROPERTY SECTION_${SECTION_NAME}_MAX_SIZE         ${SECTION_MAX_SIZE})

  string(REGEX REPLACE "^[\.]" "" name_clean "${SECTION_NAME}")
  string(REPLACE "." "_" name_clean "${name_clean}")
  set_property(GLOBAL PROPERTY SECTION_${SECTION_NAME}_NAME_CLEAN ${name_clean})

  set_property(GLOBAL PROPERTY SYMBOL_TABLE___${name_clean}_start      ${name_clean})
  set_property(GLOBAL PROPERTY SYMBOL_TABLE___${name_clean}_size       ${name_clean})
  set_property(GLOBAL PROPERTY SYMBOL_TABLE___${name_clean}_load_start ${name_clean})
  set_property(GLOBAL PROPERTY SYMBOL_TABLE___${name_clean}_end        ${name_clean})

  set(INDEX 100)
  set(settings_single "ALIGN;ANY;FIRST;KEEP;OFFSET;PRIO;SECTION;SORT;MIN_SIZE;MAX_SIZE")
  set(settings_multi  "FLAGS;INPUT;PASS;SYMBOLS")
  foreach(settings ${SECTION_SETTINGS} ${DEVICE_API_SECTION_SETTINGS})
    if("${settings}" MATCHES "^{(.*)}$")
      cmake_parse_arguments(SETTINGS "" "${settings_single}" "${settings_multi}" ${CMAKE_MATCH_1})

      if(NOT ("${SETTINGS_SECTION}" STREQUAL "${SECTION_NAME}"))
        continue()
      endif()

      if(DEFINED SETTINGS_PASS)
        is_active_in_pass(active ${PASS} "${SETTINGS_PASS}")
        if(NOT active)
          continue()
        endif()
      endif()

      if(DEFINED SETTINGS_PRIO)
        set(idx ${SETTINGS_PRIO})
      else()
        set(idx ${INDEX})
        math(EXPR INDEX "${INDEX} + 1")
      endif()

      foreach(setting ${settings_single} ${settings_multi})
        set_property(GLOBAL PROPERTY
          SECTION_${SECTION_NAME}_SETTING_${idx}_${setting}
          ${SETTINGS_${setting}}
        )
        if(DEFINED SETTINGS_SORT)
          set_property(GLOBAL PROPERTY SYMBOL_TABLE___${name_clean}_end ${name_clean}_end)
        endif()
      endforeach()

      set_property(GLOBAL APPEND PROPERTY SECTION_${SECTION_NAME}_SETTINGS_INDICIES ${idx})

    endif()
  endforeach()

  get_property(indicies GLOBAL PROPERTY SECTION_${SECTION_NAME}_SETTINGS_INDICIES)
  if(DEFINED indicies)
    list(SORT indicies COMPARE NATURAL)
    set_property(GLOBAL PROPERTY SECTION_${SECTION_NAME}_SETTINGS_INDICIES ${indicies})
  endif()

  if(DEFINED SECTION_GROUP)
    find_object(OBJECT parent NAME ${SECTION_GROUP})
  elseif(DEFINED SECTION_VMA OR DEFINED SECTION_LMA)
    if(DEFINED SECTION_VMA)
      find_object(OBJECT object NAME ${SECTION_VMA})
      get_parent(OBJECT ${object} PARENT parent TYPE REGION)

      get_property(vma GLOBAL PROPERTY ${parent}_NAME)
      set_property(GLOBAL PROPERTY SECTION_${SECTION_NAME}_VMA ${vma})
      set(SECTION_VMA ${vma})
    endif()

    if(DEFINED SECTION_LMA)
      find_object(OBJECT object NAME ${SECTION_LMA})
      get_parent(OBJECT ${object} PARENT parent TYPE REGION)

      get_property(lma GLOBAL PROPERTY ${parent}_NAME)
      set_property(GLOBAL PROPERTY SECTION_${SECTION_NAME}_LMA ${lma})
      set(SECTION_LMA ${lma})
    endif()
  else()
    set(parent ${SECTION_SYSTEM})
  endif()

  set_property(GLOBAL PROPERTY SECTION_${SECTION_NAME}_PARENT ${parent})
  add_section(OBJECT ${parent} SECTION ${SECTION_NAME} ADDRESS ${SECTION_ADDRESS} VMA ${SECTION_VMA})
endfunction()

function(create_symbol)
  cmake_parse_arguments(SYM "" "OBJECT;EXPR;SIZE;SUBALIGN;SYMBOL" "" ${ARGN})

  set_property(GLOBAL PROPERTY SYMBOL_${SYM_SYMBOL} TRUE)
  set_property(GLOBAL PROPERTY SYMBOL_${SYM_SYMBOL}_OBJ_TYPE SYMBOL)
  set_property(GLOBAL PROPERTY SYMBOL_${SYM_SYMBOL}_NAME     ${SYM_SYMBOL})
  set_property(GLOBAL PROPERTY SYMBOL_${SYM_SYMBOL}_EXPR     ${SYM_EXPR})
  set_property(GLOBAL PROPERTY SYMBOL_${SYM_SYMBOL}_SIZE     ${SYM_SIZE})
  set_property(GLOBAL PROPERTY SYMBOL_${SYM_SYMBOL}_SYMBOL   ${SYM_SYMBOL})

  set_property(GLOBAL PROPERTY SYMBOL_TABLE_${SYM_SYMBOL} ${SYM_SYMBOL})

  add_symbol(OBJECT ${SYM_OBJECT} SYMBOL SYMBOL_${SYM_SYMBOL})
endfunction()

#
# Create functions - end
#

#
# Add functions - start
#
function(add_region)
  cmake_parse_arguments(ADD_REGION "" "OBJECT;REGION" "" ${ARGN})

  get_property(exists GLOBAL PROPERTY ${ADD_REGION_OBJECT})
  if(NOT exists)
    message(FATAL_ERROR
      "Adding region ${ADD_REGION_REGION} to none-existing object: "
      "${ADD_REGION_OBJECT}"
    )
  endif()

  set_property(GLOBAL PROPERTY ${ADD_REGION_REGION}_PARENT ${ADD_REGION_OBJECT})
  set_property(GLOBAL APPEND PROPERTY ${ADD_REGION_OBJECT}_REGIONS ${ADD_REGION_REGION})
endfunction()

# add_group OBJECT o GROUP g adds group g to object o
function(add_group)
  cmake_parse_arguments(ADD_GROUP "" "OBJECT;GROUP" "" ${ARGN})

  get_property(exists GLOBAL PROPERTY ${ADD_GROUP_OBJECT})
  if(NOT exists)
    message(FATAL_ERROR
      "Adding group ${ADD_GROUP_GROUP} to none-existing object: "
      "${ADD_GROUP_OBJECT}"
    )
  endif()

  get_property(vma GLOBAL PROPERTY ${ADD_GROUP_GROUP}_VMA)
  get_property(object_name GLOBAL PROPERTY ${ADD_GROUP_OBJECT}_NAME)

  if((NOT DEFINED vma) OR ("${vma}" STREQUAL ${object_name}))
    set_property(GLOBAL APPEND PROPERTY ${ADD_GROUP_OBJECT}_GROUPS ${ADD_GROUP_GROUP})
  else()
    set_property(GLOBAL APPEND PROPERTY ${ADD_GROUP_OBJECT}_${vma}_GROUPS ${ADD_GROUP_GROUP})
  endif()
endfunction()

function(add_section)
  cmake_parse_arguments(ADD_SECTION "" "OBJECT;SECTION;ADDRESS;VMA" "" ${ARGN})

  if(DEFINED ADD_SECTION_OBJECT)
    get_property(type GLOBAL PROPERTY ${ADD_SECTION_OBJECT}_OBJ_TYPE)
    get_property(object_name GLOBAL PROPERTY ${ADD_SECTION_OBJECT}_NAME)

    if(NOT DEFINED type)
      message(FATAL_ERROR
              "Adding section ${ADD_SECTION_SECTION} to "
              "none-existing object: ${ADD_SECTION_OBJECT}"
      )
    endif()
  else()
    set(ADD_SECTION_OBJECT RELOCATEABLE)
  endif()

  if("${ADD_SECTION_VMA}" STREQUAL "${object_name}" AND DEFINED ADD_SECTION_ADDRESS)
    set_property(GLOBAL APPEND PROPERTY
      ${ADD_SECTION_OBJECT}_SECTIONS_FIXED
      SECTION_${ADD_SECTION_SECTION}
    )
  elseif(NOT DEFINED ADD_SECTION_VMA AND DEFINED SECTION_ADDRESS)
    set_property(GLOBAL APPEND PROPERTY
      ${ADD_SECTION_OBJECT}_SECTIONS_FIXED
      SECTION_${ADD_SECTION_SECTION}
    )
  elseif("${ADD_SECTION_VMA}" STREQUAL "${object_name}")
    set_property(GLOBAL APPEND PROPERTY
      ${ADD_SECTION_OBJECT}_SECTIONS
      SECTION_${ADD_SECTION_SECTION}
    )
  elseif(NOT DEFINED ADD_SECTION_VMA)
    set_property(GLOBAL APPEND PROPERTY
      ${ADD_SECTION_OBJECT}_SECTIONS
      SECTION_${ADD_SECTION_SECTION}
    )
  elseif(DEFINED SECTION_ADDRESS)
    set_property(GLOBAL APPEND PROPERTY
      ${ADD_SECTION_OBJECT}_${ADD_SECTION_VMA}_SECTIONS_FIXED
      SECTION_${ADD_SECTION_SECTION}
    )
  else()
    set_property(GLOBAL APPEND PROPERTY
      ${ADD_SECTION_OBJECT}_${ADD_SECTION_VMA}_SECTIONS
      SECTION_${ADD_SECTION_SECTION}
    )
  endif()
endfunction()

function(add_symbol)
  cmake_parse_arguments(ADD_SYMBOL "" "OBJECT;SYMBOL" "" ${ARGN})

  # Section can be fixed address or not, VMA == LMA, .
  #
  get_property(exists GLOBAL PROPERTY ${ADD_SYMBOL_OBJECT})
  if(NOT exists)
    message(FATAL_ERROR
      "Adding symbol ${ADD_SYMBOL_SYMBOL} to none-existing object: "
      "${ADD_SYMBOL_OBJECT}"
    )
  endif()

  set_property(GLOBAL APPEND PROPERTY ${ADD_SYMBOL_OBJECT}_SYMBOLS ${ADD_SYMBOL_SYMBOL})
endfunction()

#
# Add functions - end
#

#
# Retrieval functions - start
#
function(find_object)
  cmake_parse_arguments(FIND "" "OBJECT;NAME" "" ${ARGN})

  get_property(REGION  GLOBAL PROPERTY REGION_${FIND_NAME})
  get_property(GROUP   GLOBAL PROPERTY GROUP_${FIND_NAME})
  get_property(SECTION GLOBAL PROPERTY SECTION_${FIND_NAME})

  if(REGION)
    set(${FIND_OBJECT} REGION_${FIND_NAME} PARENT_SCOPE)
  elseif(GROUP)
    set(${FIND_OBJECT} GROUP_${FIND_NAME} PARENT_SCOPE)
  elseif(SECTION)
    set(${FIND_OBJECT} SECTION_${FIND_NAME} PARENT_SCOPE)
  else()
    message(WARNING "No object with name ${FIND_NAME} could be found.")
  endif()
endfunction()

# get_object(LIST l OBJECT o TYPE t)
# sets l to a list of objects of type t that are children of o
function(get_objects)
  cmake_parse_arguments(GET "" "LIST;OBJECT;TYPE" "" ${ARGN})

  # Get what type of object we are starting from
  get_property(type GLOBAL PROPERTY ${GET_OBJECT}_OBJ_TYPE)

  if(${type} STREQUAL SECTION)
    # A section doesn't have sub-items.
    return()
  endif()

  if(NOT (${GET_TYPE} STREQUAL SECTION
     OR   ${GET_TYPE} STREQUAL GROUP)
  )
    message(WARNING "Only retrieval of SECTION GROUP objects are supported.")
    return()
  endif()

  set(out)

  # Find (other) regions in our system
  get_parent(OBJECT ${GET_OBJECT} PARENT parent TYPE SYSTEM)
  get_property(regions GLOBAL PROPERTY ${parent}_REGIONS)
  list(REMOVE_ITEM regions ${GET_OBJECT})

  if(${GET_TYPE} STREQUAL SECTION)
    # If we are retrieving sections, then we need to get _SECTIONS_FIXED,
    # sections from sub-groups, and immediate setion children
    get_property(sections GLOBAL PROPERTY ${GET_OBJECT}_SECTIONS_FIXED)
    list(APPEND out ${sections})

    get_property(groups GLOBAL PROPERTY ${GET_OBJECT}_GROUPS)
    foreach(group ${groups})
      get_objects(LIST sections OBJECT ${group} TYPE ${GET_TYPE})
      list(APPEND out ${sections})
    endforeach()

    get_property(sections GLOBAL PROPERTY ${GET_OBJECT}_SECTIONS)
    list(APPEND out ${sections})

    # Now pick up sections from each region via the _vma_ properties.
    foreach(region ${regions})
      get_property(vma GLOBAL PROPERTY ${region}_NAME)

      get_property(sections GLOBAL PROPERTY ${GET_OBJECT}_${vma}_SECTIONS_FIXED)
      list(APPEND out ${sections})

      get_property(groups GLOBAL PROPERTY ${GET_OBJECT}_${vma}_GROUPS)
      foreach(group ${groups})
        get_objects(LIST sections OBJECT ${group} TYPE ${GET_TYPE})
        list(APPEND out ${sections})
      endforeach()

      get_property(sections GLOBAL PROPERTY ${GET_OBJECT}_${vma}_SECTIONS)
      list(APPEND out ${sections})
    endforeach()
  endif()

  if(${GET_TYPE} STREQUAL GROUP)
    # For groups we add immediate sub-groups, and all their descendant groups,
    # and all the _vma_ groups and descendants
    get_property(groups GLOBAL PROPERTY ${GET_OBJECT}_GROUPS)
    list(APPEND out ${groups})

    foreach(group ${groups})
      get_objects(LIST subgroups OBJECT ${group} TYPE ${GET_TYPE})
      list(APPEND out ${subgroups})
    endforeach()

    foreach(region ${regions})
      get_property(vma GLOBAL PROPERTY ${region}_NAME)

      get_property(groups GLOBAL PROPERTY ${GET_OBJECT}_${vma}_GROUPS)
      list(APPEND out ${groups})

      foreach(group ${groups})
        get_objects(LIST subgroups OBJECT ${group} TYPE ${GET_TYPE})
        list(APPEND out ${subgroups})
      endforeach()
    endforeach()
  endif()

  set(${GET_LIST} ${out} PARENT_SCOPE)
endfunction()

#
# Retrieval functions - end
#

function(is_empty)
  cmake_parse_arguments(IS_EMPTY "" "OBJECT" "" ${ARGN})

  get_property(sections GLOBAL PROPERTY ${IS_EMPTY_OBJECT}_SECTIONS_FIXED)
  if(DEFINED sections)
    set_property(GLOBAL PROPERTY ${IS_EMPTY_OBJECT}_EMPTY FALSE)
    return()
  endif()

  get_property(groups GLOBAL PROPERTY ${IS_EMPTY_OBJECT}_GROUPS)
  if(DEFINED groups)
    set_property(GLOBAL PROPERTY ${IS_EMPTY_OBJECT}_EMPTY FALSE)
    return()
  endif()


  get_property(sections GLOBAL PROPERTY ${IS_EMPTY_OBJECT}_SECTIONS)
  if(DEFINED sections)
    set_property(GLOBAL PROPERTY ${IS_EMPTY_OBJECT}_EMPTY FALSE)
    return()
  endif()

  get_parent(OBJECT ${IS_EMPTY_OBJECT} PARENT parent TYPE SYSTEM)
  get_property(regions GLOBAL PROPERTY ${parent}_REGIONS)
  list(REMOVE_ITEM regions ${IS_EMPTY_OBJECT})
  foreach(region ${regions})
    get_property(vma GLOBAL PROPERTY ${region}_NAME)
    get_property(sections GLOBAL PROPERTY ${IS_EMPTY_OBJECT}_${vma}_SECTIONS_FIXED)
    if(DEFINED sections)
      set_property(GLOBAL PROPERTY ${IS_EMPTY_OBJECT}_EMPTY FALSE)
      return()
    endif()

    get_property(groups GLOBAL PROPERTY ${IS_EMPTY_OBJECT}_${vma}_GROUPS)
    if(DEFINED groups)
      set_property(GLOBAL PROPERTY ${IS_EMPTY_OBJECT}_EMPTY FALSE)
      return()
    endif()

    get_property(sections GLOBAL PROPERTY ${IS_EMPTY_OBJECT}_${vma}_SECTIONS)
    if(DEFINED sections)
      set_property(GLOBAL PROPERTY ${IS_EMPTY_OBJECT}_EMPTY FALSE)
      return()
    endif()
  endforeach()
  set_property(GLOBAL PROPERTY ${IS_EMPTY_OBJECT}_EMPTY TRUE)
endfunction()

# This function post process the region for easier use.
#
# This is common post processing.
# If the calling <linker>_script.cmake generator implements its own
# process_region(), then the process_region_common() must be called explicitly
# from the process_region() from the <linker>_script.cmake generator.
#
# This allows a custom <linker>_script.cmake generator to completely disable
# the common post processing of regions.
#
# Tasks:
# - Apply missing settings, such as initial address for first section in a region.
# - Symbol names on sections
# - Ordered list of all sections for easier retrieval on printing and configuration.
function(process_region_common)
  cmake_parse_arguments(REGION_COMMON "" "OBJECT" "" ${ARGN})

  is_empty(OBJECT ${REGION_COMMON_OBJECT})

  set(sections)
  get_objects(LIST sections OBJECT ${REGION_COMMON_OBJECT} TYPE SECTION)
  set_property(GLOBAL PROPERTY ${REGION_COMMON_OBJECT}_SECTION_LIST_ORDERED ${sections})

  set(groups)
  get_objects(LIST groups OBJECT ${REGION_COMMON_OBJECT} TYPE GROUP)
  set_property(GLOBAL PROPERTY ${REGION_COMMON_OBJECT}_GROUP_LIST_ORDERED ${groups})

  list(LENGTH sections section_count)
  if(section_count GREATER 0)
    list(GET sections 0 section)
    get_property(address GLOBAL PROPERTY ${section}_ADDRESS)
    if(NOT DEFINED address)
      get_parent(OBJECT ${REGION_COMMON_OBJECT} PARENT parent TYPE REGION)
      get_property(address GLOBAL PROPERTY ${parent}_ADDRESS)
      set_property(GLOBAL PROPERTY ${section}_ADDRESS ${address})
    endif()
  endif()

  # Loop over other regions with the same parent
  get_parent(OBJECT ${REGION_COMMON_OBJECT} PARENT parent TYPE SYSTEM)
  get_property(regions GLOBAL PROPERTY ${parent}_REGIONS)
  list(REMOVE_ITEM regions ${REGION_COMMON_OBJECT})
  foreach(region ${regions})
    get_property(vma GLOBAL PROPERTY ${region}_NAME)
    set(sections_${vma})
    get_property(sections GLOBAL PROPERTY ${REGION_COMMON_OBJECT}_${vma}_SECTIONS_FIXED)
    list(APPEND sections_${vma} ${sections})

    get_property(groups GLOBAL PROPERTY ${REGION_COMMON_OBJECT}_${vma}_GROUPS)
    foreach(group ${groups})
      get_objects(LIST sections OBJECT ${group} TYPE SECTION)
      list(APPEND sections_${vma} ${sections})
    endforeach()

    get_property(sections GLOBAL PROPERTY ${REGION_COMMON_OBJECT}_${vma}_SECTIONS)
    list(APPEND sections_${vma} ${sections})

    list(LENGTH sections_${vma} section_count)
    if(section_count GREATER 0)
      list(GET sections_${vma} 0 section)
      get_property(address GLOBAL PROPERTY ${section}_ADDRESS)
      if(NOT DEFINED address)
        get_property(address GLOBAL PROPERTY ${region}_ADDRESS)
        set_property(GLOBAL PROPERTY ${section}_ADDRESS ${address})
      endif()
    endif()
  endforeach()
endfunction()

if(NOT COMMAND process_region)
  function(process_region)
    process_region_common(${ARGN})
  endfunction()
endif()


#
# String functions - start
#
# Each linker must implement their own <type>_to_string() functions to
# generate a correct linker script.
#
if(NOT COMMAND system_to_string)
  function(system_to_string)
    message(WARNING "No linker defined function found. Please implement a "
                    "system_to_string() function for this linker."
    )
  endfunction()
endif()

if(NOT COMMAND group_to_string)
  function(group_to_string)
    message(WARNING "No linker defined function found. Please implement a "
                    "group_to_string() function for this linker."
    )
  endfunction()
endif()

if(NOT COMMAND section_to_string)
  function(section_to_string)
    message(WARNING "No linker defined function found. Please implement a "
                    "section_to_string() function for this linker."
    )
  endfunction()
endif()

if(NOT COMMAND symbol_to_string)
  function(symbol_to_string)
    message(WARNING "No linker defined function found. Please implement a "
                    "symbol_to_string() function for this linker."
    )
  endfunction()
endif()

function(to_string)
  cmake_parse_arguments(STRING "" "OBJECT;STRING" "" ${ARGN})

  get_property(type GLOBAL PROPERTY ${STRING_OBJECT}_OBJ_TYPE)

  if("${type}" STREQUAL SYSTEM)
    system_to_string(OBJECT ${STRING_OBJECT} STRING ${STRING_STRING})
  elseif(("${type}" STREQUAL REGION) OR ("${type}" STREQUAL GROUP))
    group_to_string(OBJECT ${STRING_OBJECT} STRING ${STRING_STRING})
  elseif("${type}" STREQUAL SECTION)
    section_to_string(SECTION ${STRING_OBJECT} STRING ${STRING_STRING})
  elseif("${type}" STREQUAL SYMBOL)
    symbol_to_string(SYMBOL ${STRING_OBJECT} STRING ${STRING_STRING})
  endif()

  set(${STRING_STRING} ${${STRING_STRING}} PARENT_SCOPE)
endfunction()

#
# String functions - end
#

## Preprocess and gather input
foreach(VAR ${VARIABLES})
  if("${VAR}" MATCHES "^{(.*)}$")
    cmake_parse_arguments(VAR "" "VAR;VALUE" "PASS" ${CMAKE_MATCH_1})

    if(DEFINED VAR_PASS)
      is_active_in_pass(active ${PASS} "${VAR_PASS}")
      if(NOT active)
        continue()
      endif()
    endif()

    set(${VAR_VAR} "${VAR_VALUE}" CACHE INTERNAL "")
  endif()
endforeach()

# By the input we have INCLUDES defined that contains the files that we need
# to include depending on linker pass.
foreach(file ${INCLUDES})
  if("${file}" MATCHES "^{(.*)}$")
    cmake_parse_arguments(INC "" "KCONFIG;HEADER;CMAKE" "PASS" ${CMAKE_MATCH_1})

    if(DEFINED INC_PASS)
      is_active_in_pass(active ${PASS} "${INC_PASS}")
      if(NOT active)
        continue()
      endif()
    endif()

    if(CMAKE_VERBOSE_MAKEFILE)
      message("Reading file ${INC_KCONFIG}${INC_HEADER}${INC_CMAKE}")
    endif()

    if(INC_KCONFIG)
      import_kconfig(CONFIG ${INC_KCONFIG})
    elseif(INC_CMAKE)
      include(${INC_CMAKE})
    elseif(INC_HEADER)
      list(APPEND PREPROCESSOR_FILES ${INC_HEADER})
    endif()
  endif()
endforeach()

# For now, lets start with @FOO@ and store each #define FOO value as a global
# property AT_VAR_FOO (prefix to avoid name clashes).
# We will do all the replacements in the input lists before giving them to the
# generator functions.
set(VAR_DEF_REGEX "#define ([A-Za-z0-9_]+)[ \t\r\n]+(.+)")
foreach(file IN LISTS PREPROCESSOR_FILES )
  if(EXISTS ${file})
    file(STRINGS ${file} defs REGEX ${VAR_DEF_REGEX})
    foreach(def ${defs})
      if(${def} MATCHES ${VAR_DEF_REGEX})
        set("AT_VAR_${CMAKE_MATCH_1}" "${CMAKE_MATCH_2}")
      endif()
    endforeach()
  else()
    message("Missing file ${file}")
  endif()
endforeach()

# To pickup information gathered by the scripts from the previous pass, we use
# the syntax @FOO@ where FOO is a cmake variable name
function(do_var_replace_in res_ptr src)
  string(REGEX MATCHALL "@([^@]*)@" match_res "${src}")
  foreach(match IN LISTS match_res)
    string(REPLACE "@" "" expr ${match})
    # the variable expression is as follows:
    # @NAME[,undef:VALUE]@ Where the VALUE gets picked if we dont find NAME
    string(REPLACE "," ";" expr ${expr})
    list(GET expr 0 var)
    if(DEFINED "AT_VAR_${var}")
      set(value "${AT_VAR_${var}}")
    elseif(DEFINED ${var}) # set by zephyr_linker_include_generated files
      set(value "${${var}}")
    elseif("${expr}" MATCHES ";undef:([^,]*)")
      set(value "${CMAKE_MATCH_1}")
    else()
      set(value "${match}")
      # can't warn here because we can't check for what is relevant in this pass
      # message(WARNING "Missing definition for ${match}")
    endif()

    if(CMAKE_VERBOSE_MAKEFILE)
      message("Using variable ${match} with value ${value}")
    endif()
    string(REPLACE "${match}" "${value}" src "${src}")
  endforeach()
  set(${res_ptr} "${src}" PARENT_SCOPE)
endfunction()

foreach(input IN ITEMS "MEMORY_REGIONS" "GROUPS" "SECTIONS" "SECTION_SETTINGS")
  do_var_replace_in(${input} "${${input}}")
endforeach()


create_system(OBJECT new_system NAME ZEPHYR_LINKER_v1 FORMAT ${FORMAT} ENTRY ${ENTRY})

# Sorting the memory sections in ascending order.
foreach(region ${MEMORY_REGIONS})
  if("${region}" MATCHES "^{(.*)}$")
    cmake_parse_arguments(REGION "" "NAME;START" "" ${CMAKE_MATCH_1})
    math(EXPR start_dec "${REGION_START}" OUTPUT_FORMAT DECIMAL)
    set(region_id ${start_dec}_${REGION_NAME})
    set(region_${region_id} ${region})
    string(REPLACE ";" "\;" region_${region_id} "${region_${region_id}}")
    list(APPEND region_sort ${region_id})
  endif()
endforeach()

list(SORT region_sort COMPARE NATURAL)
set(MEMORY_REGIONS_SORTED)
foreach(region_start ${region_sort})
  list(APPEND MEMORY_REGIONS_SORTED "${region_${region_start}}")
endforeach()
# sorting complete.

foreach(region ${MEMORY_REGIONS_SORTED})
  if("${region}" MATCHES "^{(.*)}$")
    create_region(OBJECT new_region ${CMAKE_MATCH_1})
    add_region(OBJECT ${new_system} REGION ${new_region})
  endif()
endforeach()

foreach(group ${GROUPS})
  if("${group}" MATCHES "^{(.*)}$")
    create_group(OBJECT new_group ${CMAKE_MATCH_1})
  endif()
endforeach()

foreach(section ${SECTIONS} ${DEVICE_API_SECTIONS})
  if("${section}" MATCHES "^{(.*)}$")
    create_section(${CMAKE_MATCH_1} SYSTEM ${new_system})
  endif()
endforeach()

foreach(symbol ${SYMBOLS})
  if("${symbol}" MATCHES "^{(.*)}$")
    create_symbol(OBJECT ${new_system} ${CMAKE_MATCH_1})
  endif()
endforeach()

get_property(regions GLOBAL PROPERTY ${new_system}_REGIONS)
foreach(region ${regions})
  process_region(OBJECT ${region})
endforeach()

set(OUT)
to_string(OBJECT ${new_system} STRING OUT)

if(OUT_FILE)
  file(WRITE ${OUT_FILE} "${OUT}")
endif()
