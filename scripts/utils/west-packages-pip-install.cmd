:: SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
:: SPDX-License-Identifier: Apache-2.0

@echo off
rem Collect packages from west and install them with a single pip call.
setlocal enabledelayedexpansion

set "PACKAGES="

for /f "usebackq delims=" %%p in (`west packages pip`) do (
	if defined PACKAGES (
		set "PACKAGES=!PACKAGES! %%p"
	) else (
		set "PACKAGES=%%p"
	)
)

if not defined PACKAGES (
	echo west packages pip returned no packages to install.
	exit /b 0
)

echo Installing packages with: python.exe -m pip install %PACKAGES%
python.exe -m pip install %PACKAGES%
set "RESULT=%ERRORLEVEL%"

endlocal & exit /b %RESULT%
