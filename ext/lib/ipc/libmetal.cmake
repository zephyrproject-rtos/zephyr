include(ExternalProject)

ExternalProject_Add(
  libmetal                 # Name for custom target
  SOURCE_DIR $ENV{ZEPHYR_BASE}/ext/lib/ipc/libmetal/
  INSTALL_COMMAND ""      # This particular build system has no install command
  CMAKE_ARGS -DWITH_ZEPHYR=ON -DBOARD=${BOARD} -DWITH_DEFAULT_LOGGER=OFF -DWITH_DOC=OFF
  )

ExternalProject_Get_property(libmetal BINARY_DIR)
set(LIBMETAL_INCLUDE_DIR ${BINARY_DIR}/lib/include)
set(LIBMETAL_LIBRARY     ${BINARY_DIR}/lib/libmetal.a)
