include(ExternalProject)

ExternalProject_Add(
  open-amp
  SOURCE_DIR $ENV{ZEPHYR_BASE}/ext/lib/ipc/open-amp/open-amp/
  DEPENDS metal
  INSTALL_COMMAND ""      # This particular build system has no install command
  CMAKE_ARGS -DWITH_ZEPHYR=ON -DWITH_PROXY=OFF -DBOARD=${BOARD} -DLIBMETAL_INCLUDE_DIR=${ZEPHYR_BINARY_DIR}/ext/hal/libmetal/libmetal/lib/include -DLIBMETAL_LIB=${ZEPHYR_BINARY_DIR}/ext/hal/libmetal/libmetal/lib
)

ExternalProject_Get_property(open-amp SOURCE_DIR)
set(OPENAMP_INCLUDE_DIR  ${SOURCE_DIR}/lib/include CACHE PATH "Path to the OpenAMP header files")
ExternalProject_Get_property(open-amp BINARY_DIR)
set(OPENAMP_LIBRARY      ${BINARY_DIR}/lib/libopen_amp.a CACHE FILEPATH "Path to the OpenAMP library")

