cmake_minimum_required(VERSION 3.17)

set(SORT_TYPE_NAME Lexical)

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
  cmake_parse_arguments(STRING "" "SECTION;STRING" "" ${ARGN})

  get_property(name     GLOBAL PROPERTY ${STRING_SECTION}_NAME)
  get_property(address  GLOBAL PROPERTY ${STRING_SECTION}_ADDRESS)
  get_property(type     GLOBAL PROPERTY ${STRING_SECTION}_TYPE)
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

  if(subalign)
    # Currently we simply uses offset +0, but we must support offset defined
    # externally.
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
    get_property(any      GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_ANY)
    get_property(first    GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_FIRST)
    get_property(keep     GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_KEEP)
    get_property(sort     GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_SORT)
    get_property(flags    GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_FLAGS)
    get_property(input    GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_INPUT)
    get_property(offset   GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_OFFSET)
    if(DEFINED offset)
      set(section_close TRUE)
      math(EXPR offset_dec "${offset} + 0")
      if(empty)
        set(TEMP "${TEMP} EMPTY 0x0\n  {")
        set(empty FALSE)
      endif()
      set(last_index ${offset_dec})
      if(sort)
        set(sorttype "SORTTYPE ${SORT_TYPE_${sort}}")
      endif()
      set(TEMP "${TEMP}\n  }")
      set(TEMP "${TEMP}\n  ${name_clean}_${offset_dec} (ImageBase(${name_clean}) + ${offset}) ${sorttype}\n {")
    elseif(sort)
      set(section_close TRUE)
      if(empty)
        set(TEMP "${TEMP} EMPTY 0x0\n  {")
        set(empty FALSE)
      endif()
      set(last_index ${idx})
      set(TEMP "${TEMP}\n  }")
      set(TEMP "${TEMP}\n  ${name_clean}_${idx} +0 SORTTYPE ${SORT_TYPE_${sort}}\n  {")
    endif()

    if(empty)
      set(TEMP "${TEMP}\n  {")
      set(empty FALSE)
    endif()

    foreach(setting ${input})
      #set(SETTINGS ${SETTINGS_INPUT})

#      # ToDo: The code below had en error in original implementation, causing
#      #       settings not to be applied
#      #       Verify behaviour and activate if working as intended.
#      if(align)
#        set(setting "${setting}, OVERALIGN ${align}")
#      endif()

      #if(SETTINGS_KEEP)
      # armlink has --keep=<section_id>, but is there an scatter equivalant ?
      #endif()

      if(first)
        set(setting "${setting}, +First")
        set(first "")
      endif()

      set(TEMP "${TEMP}\n    *.o(${setting})")
    endforeach()

    if(any)
      if(NOT flags)
        message(FATAL_ERROR ".ANY requires flags to be set.")
      endif()
      string(REPLACE ";" " " flags "${flags}")

      set(TEMP "${TEMP}\n    .ANY (${flags})")
    endif()
  endforeach()

  if(section_close OR DEFINED endalign)
    set(section_close)
    set(TEMP "${TEMP}\n  }")

    if(DEFINED endalign)
      if(DEFINED last_index)
        set(align_expr "AlignExpr(ImageLimit(${name_clean}_${last_index}), ${endalign}) FIXED")
      else()
        set(align_expr "AlignExpr(ImageLimit(${name_clean}), ${endalign}) FIXED")
      endif()
    else()
      set(align_expr "+0")
    endif()

    set(TEMP "${TEMP}\n  ${name_clean}_end ${align_expr} EMPTY 0x0\n  {")
    set(last_index)
  endif()

  set(TEMP "${TEMP}")
  # ToDo: add patterns here.

  set(TEMP "${TEMP}\n  }")

  set(${STRING_STRING} "${${STRING_STRING}}\n${TEMP}\n" PARENT_SCOPE)
endfunction()

function(symbol_to_string)
  cmake_parse_arguments(STRING "" "SYMBOL;STRING" "" ${ARGN})

  get_property(name     GLOBAL PROPERTY ${STRING_SYMBOL}_NAME)
  get_property(expr     GLOBAL PROPERTY ${STRING_SYMBOL}_EXPR)
  get_property(size     GLOBAL PROPERTY ${STRING_SYMBOL}_SIZE)
  get_property(symbol   GLOBAL PROPERTY ${STRING_SYMBOL}_SYMBOL)
  get_property(subalign GLOBAL PROPERTY ${STRING_SYMBOL}_SUBALIGN)

  string(REPLACE "\\" "" expr "${expr}")
  string(REGEX MATCHALL "%([^%]*)%" match_res ${expr})

  foreach(match ${match_res})
    string(REPLACE "%" "" match ${match})
    get_property(symbol_val GLOBAL PROPERTY SYMBOL_TABLE_${match})
    string(REPLACE "%${match}%" "ImageBase(${symbol_val})" expr ${expr})
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
