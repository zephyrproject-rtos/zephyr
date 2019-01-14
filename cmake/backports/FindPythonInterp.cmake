# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#.rst:
# FindPythonInterp
# ----------------
#
# Find python interpreter
#
# This module finds if Python interpreter is installed and determines
# where the executables are.  This code sets the following variables:
#
# ::
#
#   PYTHONINTERP_FOUND         - Was the Python executable found
#   PYTHON_EXECUTABLE          - path to the Python interpreter
#
#
#
# ::
#
#   PYTHON_VERSION_STRING      - Python version found e.g. 2.5.2
#   PYTHON_VERSION_MAJOR       - Python major version found e.g. 2
#   PYTHON_VERSION_MINOR       - Python minor version found e.g. 5
#   PYTHON_VERSION_PATCH       - Python patch version found e.g. 2
#
#
#
# The Python_ADDITIONAL_VERSIONS variable can be used to specify a list
# of version numbers that should be taken into account when searching
# for Python.  You need to set this variable before calling
# find_package(PythonInterp).
#
# If calling both ``find_package(PythonInterp)`` and
# ``find_package(PythonLibs)``, call ``find_package(PythonInterp)`` first to
# get the currently active Python version by default with a consistent version
# of PYTHON_LIBRARIES.

function(determine_python_version exe result)
    execute_process(COMMAND "${exe}" -c
                            "import sys; sys.stdout.write(';'.join([str(x) for x in sys.version_info[:3]]))"
                    OUTPUT_VARIABLE _VERSION
                    RESULT_VARIABLE _PYTHON_VERSION_RESULT
                    ERROR_QUIET)
    if(_PYTHON_VERSION_RESULT)
        # sys.version predates sys.version_info, so use that
        execute_process(COMMAND "${exe}" -c "import sys; sys.stdout.write(sys.version)"
                        OUTPUT_VARIABLE _VERSION
                        RESULT_VARIABLE _PYTHON_VERSION_RESULT
                        ERROR_QUIET)
        if(_PYTHON_VERSION_RESULT)
            # sys.version was first documented for Python 1.5, so assume
            # this is older.
            set(ver "1.4")
        else()
            string(REGEX REPLACE " .*" "" ver "${_VERSION}")
        endif()
    else()
        string(REPLACE ";" "." ver "${_VERSION}")
    endif()
    set(${result} ${ver} PARENT_SCOPE)
endfunction()

# Find out if the 'python' executable on path has the correct version,
# and choose it if it does. This gives this executable the highest
# priority, which is expected behaviour.
find_program(PYTHON_EXECUTABLE python)
if(NOT (${PYTHON_EXECUTABLE} STREQUAL PYTHON_EXECUTABLE-NOTFOUND))
  determine_python_version(${PYTHON_EXECUTABLE} ver)
  if(${ver} VERSION_LESS PythonInterp_FIND_VERSION)
    # We didn't find the correct version on path, so forget about it
    # and continue looking.
    unset(PYTHON_EXECUTABLE)
    unset(PYTHON_EXECUTABLE CACHE)
  endif()
endif()

unset(_Python_NAMES)

set(_PYTHON1_VERSIONS 1.6 1.5)
set(_PYTHON2_VERSIONS 2.7 2.6 2.5 2.4 2.3 2.2 2.1 2.0)
set(_PYTHON3_VERSIONS 3.7 3.6 3.5 3.4 3.3 3.2 3.1 3.0)

if(PythonInterp_FIND_VERSION)
    if(PythonInterp_FIND_VERSION_COUNT GREATER 1)
        set(_PYTHON_FIND_MAJ_MIN "${PythonInterp_FIND_VERSION_MAJOR}.${PythonInterp_FIND_VERSION_MINOR}")
        list(APPEND _Python_NAMES
             python${_PYTHON_FIND_MAJ_MIN}
             python${PythonInterp_FIND_VERSION_MAJOR})
        unset(_PYTHON_FIND_OTHER_VERSIONS)
        if(NOT PythonInterp_FIND_VERSION_EXACT)
            foreach(_PYTHON_V ${_PYTHON${PythonInterp_FIND_VERSION_MAJOR}_VERSIONS})
                if(NOT _PYTHON_V VERSION_LESS _PYTHON_FIND_MAJ_MIN)
                    list(APPEND _PYTHON_FIND_OTHER_VERSIONS ${_PYTHON_V})
                endif()
             endforeach()
        endif()
        unset(_PYTHON_FIND_MAJ_MIN)
    else()
        list(APPEND _Python_NAMES python${PythonInterp_FIND_VERSION_MAJOR})
        set(_PYTHON_FIND_OTHER_VERSIONS ${_PYTHON${PythonInterp_FIND_VERSION_MAJOR}_VERSIONS})
    endif()
else()
    set(_PYTHON_FIND_OTHER_VERSIONS ${_PYTHON3_VERSIONS} ${_PYTHON2_VERSIONS} ${_PYTHON1_VERSIONS})
endif()
find_program(PYTHON_EXECUTABLE NAMES ${_Python_NAMES})

# Set up the versions we know about, in the order we will search. Always add
# the user supplied additional versions to the front.
set(_Python_VERSIONS ${Python_ADDITIONAL_VERSIONS})
# If FindPythonInterp has already found the major and minor version,
# insert that version next to get consistent versions of the interpreter and
# library.
if(DEFINED PYTHONLIBS_VERSION_STRING)
  string(REPLACE "." ";" _PYTHONLIBS_VERSION "${PYTHONLIBS_VERSION_STRING}")
  list(GET _PYTHONLIBS_VERSION 0 _PYTHONLIBS_VERSION_MAJOR)
  list(GET _PYTHONLIBS_VERSION 1 _PYTHONLIBS_VERSION_MINOR)
  list(APPEND _Python_VERSIONS ${_PYTHONLIBS_VERSION_MAJOR}.${_PYTHONLIBS_VERSION_MINOR})
endif()

# Search for the current active python version first on Linux, and last on Windows
if(NOT WIN32)
  list(APPEND _Python_VERSIONS ";")
endif()

list(APPEND _Python_VERSIONS ${_PYTHON_FIND_OTHER_VERSIONS})

if(WIN32)
  list(APPEND _Python_VERSIONS ";")
endif()

unset(_PYTHON_FIND_OTHER_VERSIONS)
unset(_PYTHON1_VERSIONS)
unset(_PYTHON2_VERSIONS)
unset(_PYTHON3_VERSIONS)

# Search for newest python version if python executable isn't found
if(NOT PYTHON_EXECUTABLE)
    foreach(_CURRENT_VERSION IN LISTS _Python_VERSIONS)
      set(_Python_NAMES python${_CURRENT_VERSION})
      if(WIN32)
        list(APPEND _Python_NAMES python)
      endif()

      if(WIN32)
          find_program(PYTHON_EXECUTABLE
            NAMES ${_Python_NAMES}
            HINTS [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\${_CURRENT_VERSION}\\InstallPath]
            )
      endif()

      find_program(PYTHON_EXECUTABLE
        NAMES ${_Python_NAMES}
        PATHS [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\${_CURRENT_VERSION}\\InstallPath]
        )
    endforeach()
endif()

# determine python version string
if(PYTHON_EXECUTABLE)
    determine_python_version(${PYTHON_EXECUTABLE} res)
    set(PYTHON_VERSION_STRING ${res})

    if(PYTHON_VERSION_STRING MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)")
      set(PYTHON_VERSION_PATCH "${CMAKE_MATCH_3}")
    else()
      set(PYTHON_VERSION_PATCH "0")
    endif()
    set(PYTHON_VERSION_MAJOR "${CMAKE_MATCH_1}")
    set(PYTHON_VERSION_MINOR "${CMAKE_MATCH_2}")
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PythonInterp REQUIRED_VARS PYTHON_EXECUTABLE VERSION_VAR PYTHON_VERSION_STRING)

mark_as_advanced(PYTHON_EXECUTABLE)
