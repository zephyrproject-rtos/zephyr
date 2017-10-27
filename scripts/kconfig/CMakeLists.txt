add_compile_options(
  -Wall
  -Wmissing-prototypes
  -Wstrict-prototypes
  -O2
  -fomit-frame-pointer
  -std=gnu89
  -Wno-format-security
  -DKBUILD_NO_NLS
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
target_link_libraries(conf ${Regex_LIBRARIES})

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
target_link_libraries(mconf ${CURSES_LIBRARIES} ${Regex_LIBRARIES})

# Build a GTK-based frontend to Kconfig if the necessary dependencies
# are already installed.
find_package (PkgConfig)
if(PkgConfig_FOUND)
  pkg_check_modules (GTK2 gtk+-2.0 libglade-2.0)
  if(GTK2_FOUND)
    message(STATUS "Found GTK+-2.0 and libglade-2.0, will build gconf")
    add_executable(gconf
      gconf.c
      zconf.tab.c
      )
    target_include_directories( gconf PRIVATE ${GTK2_INCLUDE_DIRS})
    target_link_libraries(      gconf PRIVATE ${GTK2_LIBRARY_DIRS})
    target_compile_options(     gconf PRIVATE ${GTK2_CFLAGS_OTHER})
    target_link_libraries(      gconf PRIVATE ${GTK2_LIBRARIES})
    target_link_libraries(      gconf PRIVATE -Wl,--export-dynamic)
    file(COPY gconf.glade DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
  endif()
endif()
if(NOT GTK2_FOUND)
    message(STATUS "Skipped building gconf since GTK dependencies were not met.")
endif()

# Build a QT-based frontend to Kconfig if the necessary dependencies
# are already installed.
# FIXME: Support other QT versions
find_package(Qt4 COMPONENTS QtGui QtNetwork QtSql QtCore Qt3Support)
if(Qt4_FOUND)
  set(CMAKE_INCLUDE_CURRENT_DIR ON)
  set(CMAKE_AUTOMOC ON)

  add_executable(qconf
    qconf.cc
    zconf.tab.c
    )
  target_compile_options(qconf PRIVATE -Wfatal-errors)

  target_link_libraries(qconf
    Qt4::QtGui
    Qt4::QtNetwork
    Qt4::QtSql
    Qt4::QtCore
    Qt4::Qt3Support
    )
else()
  message(STATUS "Skipped building qconf since QT dependencies were not found.")
endif()
