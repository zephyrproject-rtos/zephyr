add_compile_options(
  -Wall
  -Wmissing-prototypes
  -Wstrict-prototypes
  -O2
  -fomit-frame-pointer
  -std=gnu89
  -Wno-format-security
)

# TODO: fix these warnings
add_compile_options(
  -Wno-pointer-to-int-cast
  -Wno-int-to-pointer-cast
)

add_definitions(
  -DKERNEL_VERSION=0
  -DCURSES_LOC=<curses.h>
  -DNCURSES_WIDECHAR=1
  -DLOCALE
  -DKBUILD_NO_NL
)

if(MINGW)
  get_filename_component(MINGW_BIN_DIR ${CMAKE_C_COMPILER} DIRECTORY)
  set(CMAKE_INCLUDE_PATH ${MINGW_BIN_DIR}/../include)
  set(CMAKE_LIBRARY_PATH ${MINGW_BIN_DIR}/../lib)
endif()

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(Curses REQUIRED)
find_package(Intl REQUIRED)
find_package(Regex REQUIRED)

include_directories(
  ${CURSES_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)
if(CURSES_HAVE_NCURSES_CURSES_H)
  include_directories(${CURSES_INCLUDE_DIRS}/ncurses)
endif()


configure_file(zconf.tab.c_shipped zconf.tab.c @ONLY)
configure_file(zconf.lex.c_shipped zconf.lex.c @ONLY)
configure_file(zconf.hash.c_shipped zconf.hash.c @ONLY)

add_executable(conf
  conf.c
  zconf.tab.c
)
target_link_libraries(conf ${Regex_LIBRARIES} ${Intl_LIBRARIES})

add_executable(mconf
  mconf.c
  zconf.tab.c
  lxdialog/checklist.c
  lxdialog/inputbox.c
  lxdialog/menubox.c
  lxdialog/textbox.c
  lxdialog/util.c
  lxdialog/yesno.c
)
target_link_libraries(mconf ${CURSES_LIBRARIES} ${Regex_LIBRARIES} ${Intl_LIBRARIES})
