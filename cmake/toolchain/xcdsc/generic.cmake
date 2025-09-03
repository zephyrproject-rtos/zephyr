zephyr_get(XCDSC_TOOLCHAIN_PATH)

if(WIN32)
  # Detect invoking shell.
  set(_CURRENT_SHELL "cmd")
  if(DEFINED ENV{PSModulePath})
    set(_CURRENT_SHELL "powershell")
  endif()
  message(STATUS "Detected Windows shell: ${_CURRENT_SHELL}")

  # Buffer toolchain path from environment variable
  set(_XCDSC_REAL "${XCDSC_TOOLCHAIN_PATH}")
  if(NOT _XCDSC_REAL)
    message(FATAL_ERROR "XCDSC_TOOLCHAIN_PATH is not set in the environment.")
  endif()

  # Fixed junction path under %TEMP%
  set(_XCDSC_LINK "$ENV{TEMP}\\xcdsc")
  # Native path for cmd.exe commands
  file(TO_NATIVE_PATH "${_XCDSC_LINK}" _XCDSC_LINK_WIN)
  file(TO_NATIVE_PATH "${_XCDSC_REAL}" _XCDSC_REAL_WIN)

  # Ensures %TEMP%\xcdsc is a symlink/junction pointing to XCDSC_TOOLCHAIN_PATH, 
  # creating or replacing it as needed (shell chosen via _CURRENT_SHELL).
  if(_CURRENT_SHELL STREQUAL "powershell")
    message(STATUS "Ensuring junction via PowerShell")
    execute_process(
      COMMAND powershell -NoProfile -ExecutionPolicy Bypass -Command
        "\$ErrorActionPreference='Stop'; \$ConfirmPreference='None';"
        "\$p = Join-Path \$env:TEMP 'xcdsc';"
        "\$t = '${_XCDSC_REAL}';"
        "if (Test-Path -LiteralPath \$p) {"
        "  \$it = Get-Item -LiteralPath \$p -Force;"
        "  if (-not (\$it.Attributes -band [IO.FileAttributes]::ReparsePoint)) {"
        "    if (\$it.PSIsContainer) {"
        "      Remove-Item -LiteralPath \$p -Recurse -Force -Confirm:\$false"
        "    } else {"
        "      Remove-Item -LiteralPath \$p -Force -Confirm:\$false"
        "    }"
        "    New-Item -ItemType Junction -Path \$p -Target \$t | Out-Null"
        "  }"
        "} else {"
        "  New-Item -ItemType Junction -Path \$p -Target \$t | Out-Null"
        "}"
      RESULT_VARIABLE _ps_rv
      OUTPUT_VARIABLE _ps_out
      ERROR_VARIABLE  _ps_err
    )
    if(NOT _ps_rv EQUAL 0)
      message(FATAL_ERROR "Failed to ensure %TEMP%\\xcdsc junction (PowerShell).\n${_ps_err}")
    endif()
  else()
    message(STATUS "Ensuring junction via Command Prompt")
    # Ensures %TEMP%\xcdsc is a symlink/junction to XCDSC_TOOLCHAIN_PATH via cmd, 
    # creating or replacing it as needed.
    execute_process(
      COMMAND cmd.exe /C
        "if exist \"${_XCDSC_LINK_WIN}\" ( "
        "  fsutil reparsepoint query \"${_XCDSC_LINK_WIN}\" >nul 2>&1 && ( ver >nul ) "
        "  || ( "
        "       if exist \"${_XCDSC_LINK_WIN}\\*\" ( "
        "         rmdir /S /Q \"${_XCDSC_LINK_WIN}\" "
        "       ) else ( "
        "         del /Q /F \"${_XCDSC_LINK_WIN}\" 2>nul "
        "       ) & "
        "       mklink /J \"${_XCDSC_LINK_WIN}\" \"${_XCDSC_REAL_WIN}\" "
        "     ) "
        ") else ( "
        "  mklink /J \"${_XCDSC_LINK_WIN}\" \"${_XCDSC_REAL_WIN}\" "
        ")"
      RESULT_VARIABLE _cmd_rv
      OUTPUT_VARIABLE _cmd_out
      ERROR_VARIABLE  _cmd_err
    )
    if(NOT _cmd_rv EQUAL 0)
      message(FATAL_ERROR "Failed to ensure %TEMP%\\xcdsc junction (cmd).\n${_cmd_err}")
    endif()
  endif()

  # Use the junction for the toolchain
  set(XCDSC_TOOLCHAIN_PATH "${_XCDSC_LINK}")
  message(STATUS "XCDSC toolchain (real): ${_XCDSC_REAL}")
  message(STATUS "XCDSC toolchain (via junction): ${XCDSC_TOOLCHAIN_PATH}")
endif()

# Set toolchain module name to xcdsc
set(COMPILER xcdsc)
set(LINKER xcdsc)
set(BINTOOLS xcdsc)
# Set base directory where binaries are available
if(XCDSC_TOOLCHAIN_PATH)
    set(TOOLCHAIN_HOME ${XCDSC_TOOLCHAIN_PATH}/bin/)
    set(XCDSC_BIN_PREFIX xc-dsc-)
    set(CROSS_COMPILE ${XCDSC_TOOLCHAIN_PATH}/bin/xc-dsc-)
endif()
if(NOT EXISTS ${XCDSC_TOOLCHAIN_PATH})
    message(FATAL_ERROR "Nothing found at XCDSC_TOOLCHAIN_PATH: '${XCDSC_TOOLCHAIN_PATH}'")
else() # Support for picolibc is indicated by the presence of 'picolibc.h' in the toolchain path.
    file(GLOB_RECURSE picolibc_header ${XCDSC_TOOLCHAIN_PATH}/include/picolibc/picolibc.h)
    if(picolibc_header)
        set(TOOLCHAIN_HAS_PICOLIBC ON CACHE BOOL "True if toolchain supports picolibc")
        endif()
endif()
 