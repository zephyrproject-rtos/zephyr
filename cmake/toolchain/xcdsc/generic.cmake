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

  # Ensure %TEMP%\xcdsc points to ${_XCDSC_REAL}; recreate only if different.
  if(_CURRENT_SHELL STREQUAL "powershell")
    execute_process(
      COMMAND powershell -NoProfile -ExecutionPolicy Bypass -Command
        "\$ErrorActionPreference='Stop'; \$ConfirmPreference='None';"
        "\$p = Join-Path \$env:TEMP 'xcdsc';"
        "\$t = '${_XCDSC_REAL}';"
        "function Normalize([string]\$path){ if(-not \$path){return \$null}; [IO.Path]::GetFullPath(\$path).TrimEnd([IO.Path]::DirectorySeparatorChar,[IO.Path]::AltDirectorySeparatorChar) }"
        "if (Test-Path -LiteralPath \$p) {"
        "  \$it = Get-Item -LiteralPath \$p -Force;"
        "  if (\$it.Attributes -band [IO.FileAttributes]::ReparsePoint) {"
        "    \$curr = Normalize ((\$it.Target | Select-Object -First 1));"
        "    \$want = Normalize \$t;"
        "    if (-not \$curr -or -not [String]::Equals(\$curr,\$want,[StringComparison]::OrdinalIgnoreCase)) {"
        "      cmd /d /c \"rmdir `\"\$p`\"\" | Out-Null;"   # <-- changed: delete junction via cmd (no prompt)
        "      New-Item -ItemType Junction -Path \$p -Target \$t | Out-Null"
        "    }"
        "  } else {"
        "    if (\$it.PSIsContainer) { Remove-Item -LiteralPath \$p -Recurse -Force -Confirm:\$false } else { Remove-Item -LiteralPath \$p -Force -Confirm:\$false }"
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
    # Use PowerShell (invoked from cmd) to compare target; recreate only if different.
    execute_process(
      COMMAND cmd.exe /C
        "powershell -NoProfile -ExecutionPolicy Bypass -Command "
        "\"\$ErrorActionPreference='Stop'; \$ConfirmPreference='None'; "
        "\$p = Join-Path \$env:TEMP 'xcdsc'; "
        "\$t = '${_XCDSC_REAL}'; "
        "function N([string]\$x){ if(-not \$x){return \$null}; [IO.Path]::GetFullPath(\$x).TrimEnd([IO.Path]::DirectorySeparatorChar,[IO.Path]::AltDirectorySeparatorChar) } "
        "if (Test-Path -LiteralPath \$p) { "
        "  \$it = Get-Item -LiteralPath \$p -Force; "
        "  if (\$it.Attributes -band [IO.FileAttributes]::ReparsePoint) { "
        "    \$curr = N ((\$it.Target | Select-Object -First 1)); "
        "    \$want = N \$t; "
        "    if (-not \$curr -or -not [String]::Equals(\$curr,\$want,[StringComparison]::OrdinalIgnoreCase)) { "
        "      cmd /d /c \"rmdir `\"\$p`\"\" | Out-Null; "  # <-- changed: delete junction via cmd (no prompt)
        "      New-Item -ItemType Junction -Path \$p -Target \$t | Out-Null "
        "    } "
        "  } else { "
        "    if (\$it.PSIsContainer) { Remove-Item -LiteralPath \$p -Recurse -Force -Confirm:\$false } else { Remove-Item -LiteralPath \$p -Force -Confirm:\$false } "
        "    New-Item -ItemType Junction -Path \$p -Target \$t | Out-Null "
        "  } "
        "} else { "
        "  New-Item -ItemType Junction -Path \$p -Target \$t | Out-Null "
        "}\""
      RESULT_VARIABLE _cmd_rv
      OUTPUT_VARIABLE _cmd_out
      ERROR_VARIABLE  _cmd_err
    )
    if(NOT _cmd_rv EQUAL 0)
      message(FATAL_ERROR "Failed to ensure %TEMP%\\xcdsc junction (cmd/PowerShell).\n${_cmd_err}")
    endif()
  endif()

  # Use the (possibly newly created) junction for the toolchain
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
