collect (PROJECT_LIB_HEADERS sys.h)

collect (PROJECT_LIB_SOURCES sys.c)

add_subdirectory(../xlnx_common ${CMAKE_CURRENT_BINARY_DIR}/../xlnx_common)
# vim: expandtab:ts=2:sw=2:smartindent
