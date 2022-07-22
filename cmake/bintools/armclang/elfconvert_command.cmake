# For armclang the elfconvert command is made into a script.
# Reason for that is because not a single command covers all use cases,
# and it must therefore be possible to call individual commands, depending
# on the arguments used.
cmake_minimum_required(VERSION 3.20.0)

# Handle stripping
if (STRIP_DEBUG OR STRIP_ALL)
  set(obj_copy_target_output "--elf")
  if(STRIP_ALL)
    set(obj_copy_strip "--strip=all")
  elseif(STRIP_DEBUG)
    set(obj_copy_strip "--strip=debug")
  endif()
endif()

# Unknown support of --srec-len in arm-ds

# Handle Input and Output target types
if(DEFINED OUTTARGET)
  if(${OUTTARGET} STREQUAL "srec")
    set(obj_copy_target_output "--m32")
  elseif(${OUTTARGET} STREQUAL "ihex")
    set(obj_copy_target_output "--i32combined")
  elseif(${OUTTARGET} STREQUAL "binary")
    set(obj_copy_target_output "--bincombined")
    if(GAP_FILL)
      set(obj_copy_gap_fill "--bincombined_padding=1,${GAP_FILL}")
    endif()
  endif()
endif()

if(DEFINED ONLY_SECTION AND "${OUTTARGET}" STREQUAL "binary")
  set(obj_copy_target_output "--bin")
  set(outfile_dir .dir)
  string(REGEX REPLACE "^[\.]" "" only_section_clean "${ONLY_SECTION}")
endif()

# Note: fromelf is a little special regarding bin output, as each section gets
#       its own file. This means that when only a specific section is required
#       then that section must be moved to correct location.
execute_process(
  COMMAND ${FROMELF}
    ${obj_copy_strip}
    ${obj_copy_gap_fill} ${obj_copy_target_output}
    --output ${OUTFILE}${outfile_dir} ${INFILE}
)

if(DEFINED ONLY_SECTION AND "${OUTTARGET}" STREQUAL "binary")
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy
      ${OUTFILE}${outfile_dir}/${only_section_clean} ${OUTFILE}
  )
endif()
