set(SUPPORTED_EMU_PLATFORMS renode)
set(RENODE_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/support/it8xxx2_evb.resc)
include(${ZEPHYR_BASE}/boards/common/misc-flasher.board.cmake)
