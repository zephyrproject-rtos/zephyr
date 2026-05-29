:orphan:

.. _cmake-style:

CMake Style Guidelines
######################

General Formatting
******************

- **Indentation**: Use **2 spaces** for indentation. Avoid tabs to ensure
  consistency across different environments.
- **Line Length**: Limit line length to **100 characters** where possible.
- **Empty Lines**: Use empty lines to separate logically distinct sections
  within a CMake file.
- **No Space Before Opening Brackets**: Do not add a space between a command
  and the opening parenthesis.
  Use ``if(...)`` instead of ``if (...)``.

  .. code-block:: cmake

     # Good:
     if(ENABLE_TESTS)
       add_subdirectory(tests)
     endif()

     # Bad:
     if (ENABLE_TESTS)
       add_subdirectory(tests)
     endif()

Commands and Syntax
*******************

- **Lowercase Commands**: Always use **lowercase** CMake commands (e.g.,
  ``add_executable``, ``find_package``). This improves readability and
  consistency.

  .. code-block:: cmake

     # Good:
     add_library(my_lib STATIC src/my_lib.cpp)

     # Bad:
     ADD_LIBRARY(my_lib STATIC src/my_lib.cpp)

- **One File Argument per Line**: Break the file arguments across multiple
  lines to make it easier to scan and identify each source file or item.

  .. code-block:: cmake

     # Good:
     target_sources(my_target PRIVATE
       src/file1.cpp
       src/file2.cpp
     )

      # Bad:
     target_sources(my_target PRIVATE src/file1.cpp src/file2.cpp)

Variable Naming
***************

- **Use Uppercase for Cache Variables or variables shared across CMake files**:
  When defining cache variables using ``option`` or ``set(... CACHE ...)``, use
  **UPPERCASE names**.

  .. code-block:: cmake

     option(ENABLE_TESTS "Enable test suite" ON)
     set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard version")

- **Use Lowercase for Local Variables**: For local variables within CMake
  files, use **lowercase** or **snake_case**.

  .. code-block:: cmake

     set(output_dir "${CMAKE_BINARY_DIR}/output")

- **Consistent Prefixing**: Use consistent prefixes for variables to avoid name
  conflicts, especially in large projects.

  .. code-block:: cmake

     set(MYPROJECT_SRC_DIR "${CMAKE_SOURCE_DIR}/src")

Quoting
*******

- **Quote Strings and Variables**: Always quote string literals and variables
  to prevent unintended behavior, especially when dealing with paths or
  arguments that may contain spaces.

  .. code-block:: cmake

     # Good:
     set(my_path "${CMAKE_SOURCE_DIR}/include")

     # Bad:
     set(my_path ${CMAKE_SOURCE_DIR}/include)

- **Do Not Quote Boolean Values**: For boolean values (``ON``, ``OFF``,
  ``TRUE``, ``FALSE``), avoid quoting them.

  .. code-block:: cmake

     option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

Avoid Hardcoding Paths
**********************

- Use CMake variables (``CMAKE_SOURCE_DIR``, ``CMAKE_BINARY_DIR``,
  ``CMAKE_CURRENT_SOURCE_DIR``) instead of hardcoding paths.

  .. code-block:: cmake

     set(output_dir "${CMAKE_BINARY_DIR}/bin")

Conditional Logic
*****************

- Use ``if``, ``elseif``, and ``else`` with proper indentation, and close with
  ``endif()``.

  .. code-block:: cmake

     if(ENABLE_TESTS)
       add_subdirectory(tests)
     endif()

Documentation
*************

- Use comments to document complex logic in the CMake files.

  .. code-block:: cmake

     # Find LlvmLld components required for building with llvm
     find_package(LlvmLld 14.0.0 REQUIRED)
