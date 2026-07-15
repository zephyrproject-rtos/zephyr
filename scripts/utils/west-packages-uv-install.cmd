:: SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
:: SPDX-License-Identifier: Apache-2.0

@echo off
rem Collect packages from west and install them with a single uv call.
setlocal enabledelayedexpansion

set "PACKAGES="

for /f "usebackq delims=" %%p in (`west packages uv`) do (
	if defined PACKAGES (
		set "PACKAGES=!PACKAGES! %%p"
	) else (
		set "PACKAGES=%%p"
	)
)

if not defined PACKAGES (
	echo west packages uv returned no packages to install.
	exit /b 0
)

echo Installing packages with: uv.exe pip install %PACKAGES%
uv.exe pip install %PACKAGES%
set "RESULT=%ERRORLEVEL%"

endlocal & exit /b %RESULT%
