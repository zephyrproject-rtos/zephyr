cmake_minimum_required(VERSION 3.28.0)

set(SORT_TYPE_NAME Lexical)
set(ARMLINK_MIN_REGION_ALIGNMENT 4)

# Return the execution region used for an individual input setting.
function(armlink_setting_region section idx output)
  get_property(name GLOBAL PROPERTY ${section}_NAME_CLEAN)
  foreach(field OFFSET SORT ALIGN MIN_SIZE MAX_SIZE INPUT ANY SYMBOLS)
    get_property(setting_${field} GLOBAL PROPERTY ${section}_SETTING_${idx}_${field})
  endforeach()
  if(DEFINED setting_OFFSET AND NOT "${setting_OFFSET}" STREQUAL "")
    math(EXPR suffix "${setting_OFFSET}")
    set(name ${name}_${suffix}_${idx})
  elseif(setting_SORT OR setting_ALIGN OR NOT "${setting_MIN_SIZE}" STREQUAL ""
         OR NOT "${setting_MAX_SIZE}" STREQUAL ""
         OR (setting_SYMBOLS AND NOT setting_INPUT AND NOT setting_ANY))
    set(name ${name}_${idx})
  endif()
  set(${output} ${name} PARENT_SCOPE)
endfunction()

# Apply armlink's minimum alignment to an execution region.
function(armlink_region_alignment requested output)
  if(NOT requested OR requested LESS ARMLINK_MIN_REGION_ALIGNMENT)
    set(requested ${ARMLINK_MIN_REGION_ALIGNMENT})
  endif()
  set(${output} ${requested} PARENT_SCOPE)
endfunction()

# Render input selectors and collect keep options from the unmodified names.
function(armlink_input_selectors inputs first keep output)
  set(selectors "")
  foreach(input ${inputs})
    if(keep)
      set_property(GLOBAL APPEND PROPERTY ARMLINK_KEEP_OPTIONS "--keep=\"*.o(${input})\"")
    endif()
    set(suffix "")
    if(first)
      set(suffix ", +First")
      set(first FALSE)
    endif()
    string(APPEND selectors "\n    *.o(${input}${suffix})")
  endforeach()
  set(${output} "${selectors}" PARENT_SCOPE)
endfunction()

# Only ISR metadata and pre0 typed payload may use an independent discovery region.
function(armlink_discovery_sections system output_sections output_alignment)
  get_property(sections GLOBAL PROPERTY ${system}_SECTIONS)
  get_property(fixed_sections GLOBAL PROPERTY ${system}_SECTIONS_FIXED)
  list(APPEND sections ${fixed_sections})
  set(discovery_sections)
  # MOVT exposes a 64-KiB target page, so separate whole discovery pages too.
  set(discovery_alignment 65536)
  foreach(section ${sections})
    get_property(type GLOBAL PROPERTY ${section}_TYPE)
    if(NOT type MATCHES "^(INFO|BUILD_ONLY)$")
      continue()
    endif()
    get_property(name GLOBAL PROPERTY ${section}_NAME_CLEAN)
    if(type STREQUAL INFO AND NOT name STREQUAL intList)
      message(FATAL_ERROR "armlink INFO requires the ISR metadata finalizer")
    endif()
    list(APPEND discovery_sections ${section})
    get_property(alignment GLOBAL PROPERTY ${section}_ALIGN)
    if(alignment GREATER discovery_alignment)
      set(discovery_alignment ${alignment})
    endif()
  endforeach()
  set(${output_sections} ${discovery_sections} PARENT_SCOPE)
  set(${output_alignment} ${discovery_alignment} PARENT_SCOPE)
endfunction()

# Discovery inputs retain real relocations, but never consume target ROM or RAM.
function(armlink_discovery_to_string regions sections discovery_alignment output)
  set(text "")
  if(NOT sections)
    set(${output} "" PARENT_SCOPE)
    return()
  endif()
  linker_discovery_region("${regions}" ${discovery_alignment} address capacity)
  math(EXPR discovery_limit "${address} + ${capacity}")
  # Scatter expressions cannot represent the terminal address 2^32.
  if(discovery_limit EQUAL 4294967296)
    math(EXPR discovery_limit "${discovery_limit} - ${discovery_alignment}")
  endif()
  math(EXPR discovery_limit
    "(${discovery_limit} / ${discovery_alignment}) * ${discovery_alignment}")
  if(NOT discovery_limit GREATER address)
    message(FATAL_ERROR "No independent ISR discovery address page is available")
  endif()

  set(previous_discovery)
  foreach(section ${sections})
    get_property(name GLOBAL PROPERTY ${section}_NAME_CLEAN)
    get_property(alignment GLOBAL PROPERTY ${section}_ALIGN)
    armlink_region_alignment("${alignment}" alignment)
    if(previous_discovery)
      set(address "AlignExpr(ImageLimit(${previous_discovery}), ${alignment})")
    endif()
    string(APPEND text
      "\n${name}_discovery ${address} OVERLAY NOCOMPRESS\n{\n"
      "  ${name} ${address} OVERLAY ALIGN ${alignment}\n  {")
    get_property(indices GLOBAL PROPERTY ${section}_SETTINGS_INDICIES)
    get_property(minimum GLOBAL PROPERTY ${section}_MIN_SIZE)
    if(NOT minimum)
      set(minimum 0)
    endif()
    set(maximum)
    foreach(index ${indices})
      get_property(setting_minimum GLOBAL PROPERTY ${section}_SETTING_${index}_MIN_SIZE)
      get_property(setting_maximum GLOBAL PROPERTY ${section}_SETTING_${index}_MAX_SIZE)
      if(setting_minimum GREATER minimum)
        set(minimum ${setting_minimum})
      endif()
      if(NOT "${setting_maximum}" STREQUAL "" AND
         ("${maximum}" STREQUAL "" OR setting_maximum LESS maximum))
        set(maximum ${setting_maximum})
      endif()
      get_property(inputs GLOBAL PROPERTY ${section}_SETTING_${index}_INPUT)
      get_property(first GLOBAL PROPERTY ${section}_SETTING_${index}_FIRST)
      armlink_input_selectors("${inputs}" "${first}" TRUE selectors)
      string(APPEND text "${selectors}")
    endforeach()
    string(APPEND text "\n  }\n")
    set(previous_discovery ${name})
    if(minimum GREATER 0)
      set(previous_discovery ${name}_discovery_end)
      string(APPEND text
        "  ${previous_discovery} ((ImageLength(${name}) < ${minimum}) ? "
        "(ImageBase(${name}) + ${minimum}) : ImageLimit(${name})) EMPTY 0x0\n  {\n  }\n")
    endif()
    if(NOT "${maximum}" STREQUAL "")
      string(APPEND text "  ScatterAssert(ImageLength(${name}) <= ${maximum})\n")
    endif()
    string(APPEND text
      "  ScatterAssert(ImageLimit(${previous_discovery}) <= ${discovery_limit})\n}\n")
  endforeach()
  set(${output} "${text}" PARENT_SCOPE)
endfunction()

# This function post process the region for easier use.
#
# Tasks:
# - Symbol translation using a steering file is configured.
function(process_region)
  cmake_parse_arguments(REGION "" "OBJECT" "" ${ARGN})

  process_region_common(${ARGN})

  get_property(empty GLOBAL PROPERTY ${REGION_OBJECT}_EMPTY)
  if(NOT empty)
    # For scatter files we move any system symbols into FLASH.
    get_parent(OBJECT ${REGION_OBJECT} PARENT parent TYPE SYSTEM)
    get_property(symbols GLOBAL PROPERTY ${parent}_SYMBOLS)

    get_property(flash GLOBAL PROPERTY ${parent}_REGION_BY_NAME_FLASH)
    set_property(GLOBAL APPEND PROPERTY ${flash}_SYMBOLS ${symbols})
    set_property(GLOBAL PROPERTY ${parent}_SYMBOLS)
  endif()

  get_property(sections GLOBAL PROPERTY ${REGION_OBJECT}_SECTION_LIST_ORDERED)
  foreach(section ${sections})

    get_property(name_clean GLOBAL PROPERTY ${section}_NAME_CLEAN)
    get_property(noinput    GLOBAL PROPERTY ${section}_NOINPUT)
    get_property(nosymbols  GLOBAL PROPERTY ${section}_NOSYMBOLS)
    get_property(type       GLOBAL PROPERTY ${section}_TYPE)

    get_property(indicies GLOBAL PROPERTY ${section}_SETTINGS_INDICIES)
    list(LENGTH indicies length)
    foreach(idx ${indicies})
      set(steering_postfixes Base Limit)
      get_property(symbols GLOBAL PROPERTY ${section}_SETTING_${idx}_SYMBOLS)
      get_property(sort    GLOBAL PROPERTY ${section}_SETTING_${idx}_SORT)
      get_property(offset  GLOBAL PROPERTY ${section}_SETTING_${idx}_OFFSET)
      armlink_setting_region(${section} ${idx} entry_region)
      if(NOT "${entry_region}" STREQUAL "${name_clean}")
        set_property(GLOBAL PROPERTY SYMBOL_TABLE___${name_clean}_end ${name_clean}_end)
        foreach(symbol ${symbols})
          list(POP_FRONT steering_postfixes postfix)
          set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_C
            "Image$$${entry_region}$$${postfix}"
          )
          set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_FILE
            "RESOLVE ${symbol} AS Image$$${entry_region}$$${postfix}\n"
          )
        endforeach()
      elseif(DEFINED symbols AND ${length} EQUAL 1 AND noinput)
        set(steering_postfixes Base Limit)
        foreach(symbol ${symbols})
          list(POP_FRONT steering_postfixes postfix)
          set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_C
            "Image$$${name_clean}$$${postfix}"
          )
          set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_FILE
            "RESOLVE ${symbol} AS Image$$${name_clean}$$${postfix}\n"
          )
        endforeach()
      endif()
    endforeach()

    if("${type}" STREQUAL BSS)
      set(ZI "$$ZI")
    endif()

    if(NOT nosymbols)
      string(TOLOWER "${name_clean}" name_clean_lower)

      # Symbols translation here.
      set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_C "Image$$${name_clean}${ZI}$$Base")
      set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_C "Image$$${name_clean}${ZI}$$Length")
      set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_C "Load$$${name_clean}${ZI}$$Base")

      set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_FILE
        "RESOLVE __${name_clean}_start AS Image$$${name_clean}${ZI}$$Base\n"
        "RESOLVE __${name_clean}_load_start AS Load$$${name_clean}${ZI}$$Base\n"
        "EXPORT  __${name_clean}_start AS __${name_clean}_start\n"
        "EXPORT  __${name_clean}_load_start AS __${name_clean}_load_start\n"
      )

      if(NOT "${name_clean_lower}" STREQUAL "${name_clean}")
        set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_FILE
          "RESOLVE __${name_clean_lower}_start AS Image$$${name_clean}${ZI}$$Base\n"
          "RESOLVE __${name_clean_lower}_load_start AS Load$$${name_clean}${ZI}$$Base\n"
          "EXPORT  __${name_clean_lower}_start AS __${name_clean_lower}_start\n"
          "EXPORT  __${name_clean_lower}_load_start AS __${name_clean_lower}_load_start\n"
        )
      endif()

      get_property(symbol_val GLOBAL PROPERTY SYMBOL_TABLE___${name_clean}_end)
      set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_C "Image$$${symbol_val}${ZI}$$Limit")
      set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_FILE
          "RESOLVE __${name_clean}_end AS Image$$${symbol_val}${ZI}$$Limit\n"
        )

      if("${symbol_val}" STREQUAL "${name_clean}")
        set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_FILE
          "RESOLVE __${name_clean}_size AS Image$$${name_clean}${ZI}$$Length\n"
          "EXPORT  __${name_clean}_size AS __${name_clean}_size\n"
        )
        if(NOT "${name_clean_lower}" STREQUAL "${name_clean}")
          set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_FILE
            "RESOLVE __${name_clean_lower}_size AS Image$$${name_clean}${ZI}$$Length\n"
            "EXPORT  __${name_clean_lower}_size AS __${name_clean_lower}_size\n"
          )
        endif()
      else()
        create_symbol(OBJECT ${REGION_OBJECT} SYMBOL __${name_clean}_size
          EXPR "(ImageLimit(${symbol_val}) - ImageBase(${name_clean}))"
        )
        if(NOT "${name_clean_lower}" STREQUAL "${name_clean}")
          create_symbol(OBJECT ${REGION_OBJECT} SYMBOL __${name_clean_lower}_size
            EXPR "(ImageLimit(${symbol_val}) - ImageBase(${name_clean}))"
          )
        endif()
      endif()
    endif()
    set(ZI)

  endforeach()

  get_property(groups GLOBAL PROPERTY ${REGION_OBJECT}_GROUP_LIST_ORDERED)
  foreach(group ${groups})
    get_property(name GLOBAL PROPERTY ${group}_NAME)
    string(TOLOWER ${name} name)

    get_objects(LIST sections OBJECT ${group} TYPE SECTION)
    list(GET sections 0 section)
    get_property(first_section_name GLOBAL PROPERTY ${section}_NAME_CLEAN)
    list(POP_BACK sections section)
    get_property(last_section_name GLOBAL PROPERTY ${section}_NAME_CLEAN)

    # Symbols translation here.
    set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_C "Image$$${first_section_name}$$Base")
    set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_C "Load$$${first_section_name}$$Base")
    set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_C "Image$$${last_section_name}$$Limit")

    set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_FILE
      "RESOLVE __${name}_start AS Image$$${first_section_name}$$Base\n"
      "EXPORT  __${name}_start AS __${name}_start\n"
      "RESOLVE __${name}_load_start AS Load$$${first_section_name}$$Base\n"
      "EXPORT  __${name}_load_start AS __${name}_load_start\n"
    )

    set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_FILE
        "RESOLVE __${name}_end AS Image$$${last_section_name}$$Limit\n"
      )

    create_symbol(OBJECT ${REGION_OBJECT} SYMBOL __${name}_size
      EXPR "(ImageLimit(${last_section_name}) - ImageBase(${first_section_name}))"
    )
  endforeach()

  get_property(symbols GLOBAL PROPERTY ${REGION_OBJECT}_SYMBOLS)
  foreach(symbol ${symbols})
    get_property(name GLOBAL PROPERTY ${symbol}_NAME)
    set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_C "Image$$${name}$$Base")

    set_property(GLOBAL APPEND PROPERTY SYMBOL_STEERING_FILE
      "RESOLVE ${name} AS Image$$${name}$$Base\n"
    )
  endforeach()

endfunction()

#
# String functions - start
#

function(system_to_string)
  cmake_parse_arguments(STRING "" "OBJECT;STRING" "" ${ARGN})

  get_property(name    GLOBAL PROPERTY ${STRING_OBJECT}_NAME)
  get_property(regions GLOBAL PROPERTY ${STRING_OBJECT}_REGIONS)
  get_property(format  GLOBAL PROPERTY ${STRING_OBJECT}_FORMAT)

  foreach(region ${regions})
    get_property(empty GLOBAL PROPERTY ${region}_EMPTY)
    if(NOT empty)
      to_string(OBJECT ${region} STRING ${STRING_STRING})
    endif()
  endforeach()

  armlink_discovery_sections(${STRING_OBJECT} discovery_sections discovery_alignment)
  armlink_discovery_to_string("${regions}" "${discovery_sections}"
    ${discovery_alignment} discovery_text)
  string(APPEND ${STRING_STRING} "${discovery_text}")

  set(${STRING_STRING} ${${STRING_STRING}} PARENT_SCOPE)
endfunction()

function(group_to_string)
  cmake_parse_arguments(STRING "" "OBJECT;STRING" "" ${ARGN})

  get_property(type GLOBAL PROPERTY ${STRING_OBJECT}_OBJ_TYPE)
  if(${type} STREQUAL REGION)
    get_property(name GLOBAL PROPERTY ${STRING_OBJECT}_NAME)
    get_property(address GLOBAL PROPERTY ${STRING_OBJECT}_ADDRESS)
    get_property(size GLOBAL PROPERTY ${STRING_OBJECT}_SIZE)
    set(${STRING_STRING} "${${STRING_STRING}}\n${name} ${address} NOCOMPRESS ${size}\n{\n")
  endif()

  get_property(sections GLOBAL PROPERTY ${STRING_OBJECT}_SECTIONS_FIXED)
  foreach(section ${sections})
    to_string(OBJECT ${section} STRING ${STRING_STRING})
  endforeach()

  get_property(groups GLOBAL PROPERTY ${STRING_OBJECT}_GROUPS)
  foreach(group ${groups})
    to_string(OBJECT ${group} STRING ${STRING_STRING})
  endforeach()

  get_property(sections GLOBAL PROPERTY ${STRING_OBJECT}_SECTIONS)
  foreach(section ${sections})
    to_string(OBJECT ${section} STRING ${STRING_STRING})
  endforeach()

  get_parent(OBJECT ${STRING_OBJECT} PARENT parent TYPE SYSTEM)
  get_property(regions GLOBAL PROPERTY ${parent}_REGIONS)
  list(REMOVE_ITEM regions ${STRING_OBJECT})
  foreach(region ${regions})
    get_property(vma GLOBAL PROPERTY ${region}_NAME)
    get_property(sections GLOBAL PROPERTY ${STRING_OBJECT}_${vma}_SECTIONS_FIXED)
    foreach(section ${sections})
      to_string(OBJECT ${section} STRING ${STRING_STRING})
    endforeach()

    get_property(groups GLOBAL PROPERTY ${STRING_OBJECT}_${vma}_GROUPS)
    foreach(group ${groups})
      to_string(OBJECT ${group} STRING ${STRING_STRING})
    endforeach()

    get_property(sections GLOBAL PROPERTY ${STRING_OBJECT}_${vma}_SECTIONS)
    foreach(section ${sections})
      to_string(OBJECT ${section} STRING ${STRING_STRING})
    endforeach()
  endforeach()

  get_property(symbols GLOBAL PROPERTY ${STRING_OBJECT}_SYMBOLS)
  foreach(symbol ${symbols})
    to_string(OBJECT ${symbol} STRING ${STRING_STRING})
  endforeach()

  if(${type} STREQUAL REGION)
    set(${STRING_STRING} "${${STRING_STRING}}\n}\n")
  endif()
  set(${STRING_STRING} ${${STRING_STRING}} PARENT_SCOPE)
endfunction()


function(section_to_string)
  set(assertions)
  cmake_parse_arguments(STRING "" "SECTION;STRING" "" ${ARGN})

  get_property(name     GLOBAL PROPERTY ${STRING_SECTION}_NAME)
  get_property(address  GLOBAL PROPERTY ${STRING_SECTION}_ADDRESS)
  get_property(type     GLOBAL PROPERTY ${STRING_SECTION}_TYPE)
  if(type STREQUAL INFO)
    message(FATAL_ERROR "armlink does not support non-allocated INFO sections")
  endif()
  get_property(align    GLOBAL PROPERTY ${STRING_SECTION}_ALIGN)
  get_property(subalign GLOBAL PROPERTY ${STRING_SECTION}_SUBALIGN)
  get_property(endalign GLOBAL PROPERTY ${STRING_SECTION}_ENDALIGN)
  get_property(vma      GLOBAL PROPERTY ${STRING_SECTION}_VMA)
  get_property(lma      GLOBAL PROPERTY ${STRING_SECTION}_LMA)
  get_property(noinput  GLOBAL PROPERTY ${STRING_SECTION}_NOINPUT)
  get_property(noinit   GLOBAL PROPERTY ${STRING_SECTION}_NOINIT)

  string(REGEX REPLACE "^[\.]" "" name_clean "${name}")
  string(REPLACE "." "_" name_clean "${name_clean}")

  set(TEMP "  ${name_clean}")
  if(DEFINED address)
    set(TEMP "${TEMP} ${address}")
  else()
    set(TEMP "${TEMP} +0")
  endif()

  if(noinit)
    # Currently we simply uses offset +0, but we must support offset defined
    # externally.
    set(TEMP "${TEMP} UNINIT")
  endif()

  if(align)
    armlink_region_alignment("${align}" align)
    set(TEMP "${TEMP} ALIGN ${align}")
  elseif(subalign)
    # Currently we simply uses offset +0, but we must support offset defined
    # externally.
    armlink_region_alignment("${subalign}" subalign)
    set(TEMP "${TEMP} ALIGN ${subalign}")
  endif()

  if(NOT noinput)
    set(TEMP "${TEMP}\n  {")

    if("${type}" STREQUAL NOLOAD)
      set(TEMP "${TEMP}\n    *.o(${name}*)")
      set(TEMP "${TEMP}\n    *.o(${name}*.*)")
    elseif(VMA_FLAGS)
      # ToDo: Proper names as provided by armclang
#      set(TEMP "${TEMP}\n    *.o(${SEC_NAME}*, +${VMA_FLAGS})")
#      set(TEMP "${TEMP}\n    *.o(${SEC_NAME}*.*, +${VMA_FLAGS})")
      set(TEMP "${TEMP}\n    *.o(${name}*)")
      set(TEMP "${TEMP}\n    *.o(${name}*.*)")
    else()
      set(TEMP "${TEMP}\n    *.o(${name}*)")
      set(TEMP "${TEMP}\n    *.o(${name}*.*)")
    endif()
  else()
    set(empty TRUE)
  endif()

  get_property(indicies GLOBAL PROPERTY ${STRING_SECTION}_SETTINGS_INDICIES)
  foreach(idx ${indicies})
    get_property(align    GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_ALIGN)
    if(align)
      armlink_region_alignment("${align}" align)
    endif()
    get_property(any      GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_ANY)
    get_property(first    GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_FIRST)
    get_property(keep     GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_KEEP)
    get_property(sort     GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_SORT)
    get_property(flags    GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_FLAGS)
    get_property(input    GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_INPUT)
    get_property(offset   GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_OFFSET)
    get_property(min_size GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_MIN_SIZE)
    get_property(max_size GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_MAX_SIZE)
    armlink_setting_region(${STRING_SECTION} ${idx} entry_region)
    if(NOT "${entry_region}" STREQUAL "${name_clean}")
      set(section_close TRUE)
      if(empty)
        set(TEMP "${TEMP} EMPTY 0x0\n  {")
        set(empty FALSE)
      endif()
      set(TEMP "${TEMP}\n  }")
      set(entry_address "+0")
      if(NOT "${offset}" STREQUAL "")
        set(entry_address "(ImageBase(${name_clean}) + ${offset})")
      endif()
      set(entry_attributes)
      if(sort)
        string(APPEND entry_attributes " SORTTYPE ${SORT_TYPE_${sort}}")
      endif()
      if(align)
        string(APPEND entry_attributes " ALIGN ${align}")
      endif()
      set(TEMP "${TEMP}\n  ${entry_region} ${entry_address}${entry_attributes}\n  {")
      set(last_region ${entry_region})
    endif()

    if(empty)
      set(TEMP "${TEMP}\n  {")
      set(empty FALSE)
    endif()

    armlink_input_selectors("${input}" "${first}" "${keep}" selectors)
    string(APPEND TEMP "${selectors}")

    if(any)
      if(NOT flags)
        message(FATAL_ERROR ".ANY requires flags to be set.")
      endif()
      string(REPLACE ";" " " flags "${flags}")

      set(TEMP "${TEMP}\n    .ANY (${flags})")
    endif()
    if(NOT "${max_size}" STREQUAL "")
      string(APPEND assertions
        "\n  ScatterAssert(ImageLength(${entry_region}) <= ${max_size})"
      )
    endif()
    if(NOT "${min_size}" STREQUAL "")
      set(limit_region ${entry_region}_limit)
      set(limit_address "((ImageLength(${entry_region}) < ${min_size}) ? ")
      string(APPEND limit_address
        "(ImageBase(${entry_region}) + ${min_size}) : ImageLimit(${entry_region}))"
      )
      string(APPEND TEMP "\n  }\n  ${limit_region} ${limit_address} EMPTY 0x0\n  {")
      set(last_region ${limit_region})
    endif()
  endforeach()

  if(empty)
    set(TEMP "${TEMP} EMPTY 0x0\n  {")
    set(empty FALSE)
  endif()

  if(section_close OR DEFINED endalign)
    set(section_close)
    set(TEMP "${TEMP}\n  }")

    if(DEFINED endalign)
      if(DEFINED last_region)
        set(align_expr "AlignExpr(ImageLimit(${last_region}), ${endalign}) FIXED")
      else()
        set(align_expr "AlignExpr(ImageLimit(${name_clean}), ${endalign}) FIXED")
      endif()
    else()
      set(align_expr "+0")
    endif()

    set(TEMP "${TEMP}\n  ${name_clean}_end ${align_expr} EMPTY 0x0\n  {")
    unset(last_region)
  endif()

  set(TEMP "${TEMP}")
  # ToDo: add patterns here.

  if("${type}" STREQUAL BSS)
    set(ZI "$$ZI")
  endif()

  set(TEMP "${TEMP}\n  }")

  set(${STRING_STRING} "${${STRING_STRING}}\n${TEMP}\n${assertions}\n" PARENT_SCOPE)
endfunction()

function(symbol_to_string)
  cmake_parse_arguments(STRING "" "SYMBOL;STRING" "" ${ARGN})

  get_property(name     GLOBAL PROPERTY ${STRING_SYMBOL}_NAME)
  get_property(expr     GLOBAL PROPERTY ${STRING_SYMBOL}_EXPR)
  get_property(size     GLOBAL PROPERTY ${STRING_SYMBOL}_SIZE)
  get_property(symbol   GLOBAL PROPERTY ${STRING_SYMBOL}_SYMBOL)
  get_property(subalign GLOBAL PROPERTY ${STRING_SYMBOL}_SUBALIGN)

  string(REPLACE "\\" "" expr "${expr}")
  string(REGEX MATCHALL "@([^@]*)@" match_res ${expr})

  foreach(match ${match_res})
    string(REPLACE "@" "" match ${match})
    get_property(symbol_val GLOBAL PROPERTY SYMBOL_TABLE_${match})
    string(REPLACE "@${match}@" "ImageBase(${symbol_val})" expr ${expr})
  endforeach()

  if(DEFINED subalign)
    set(subalign "ALIGN ${subalign}")
  endif()

  if(NOT DEFINED size)
    set(size "0x0")
  endif()

  set(${STRING_STRING}
    "${${STRING_STRING}}\n  ${symbol} ${expr} ${subalign} ${size} { }\n"
    PARENT_SCOPE
  )
endfunction()

include(${CMAKE_CURRENT_LIST_DIR}/../linker_script_common.cmake)

if(DEFINED STEERING_C)
  get_property(symbols_c GLOBAL PROPERTY SYMBOL_STEERING_C)
  file(WRITE ${STEERING_C}  "/* AUTO-GENERATED - Do not modify\n")
  file(APPEND ${STEERING_C} " * AUTO-GENERATED - All changes will be lost\n")
  file(APPEND ${STEERING_C} " */\n")
  # Undefined assembly declarations make armlink generate its region symbols,
  # including under LTO, without retaining a runtime reference function.
  list(REMOVE_DUPLICATES symbols_c)
  foreach(symbol ${symbols_c})
    file(APPEND ${STEERING_C} "__asm__(\".global ${symbol}\\n\");\n")
  endforeach()
endif()

if(DEFINED STEERING_FILE)
  get_property(steering_content GLOBAL PROPERTY SYMBOL_STEERING_FILE)
  file(WRITE ${STEERING_FILE}  "; AUTO-GENERATED - Do not modify\n")
  file(APPEND ${STEERING_FILE} "; AUTO-GENERATED - All changes will be lost\n")
  file(APPEND ${STEERING_FILE} ${steering_content})
endif()

if(DEFINED KEEP_FILE)
  get_property(keep_options GLOBAL PROPERTY ARMLINK_KEEP_OPTIONS)
  list(REMOVE_DUPLICATES keep_options)
  string(REPLACE ";" "\n" keep_options "${keep_options}")
  file(WRITE ${KEEP_FILE} "${keep_options}\n")
endif()
