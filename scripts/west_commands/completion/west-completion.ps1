# Copyright © 2025, Tais Hjortshøj <tbh@mjolner.dk> / Mjølner Informatics A/S
# SPDX-License-Identifier: Apache-2.0

# This script is used to provide command completion for the west command line tool.
# It is designed to be used with PowerShell and provides completion for the 'west' command
# Currently, it provides completion for the 'west boards' command.

# Usage: path/to/zephyr/scripts/west_commands/completion/west-completion.ps1

# region West board finder initialize
$s = {
    param($wordToComplete, $commandAst, $cursorPosition)
    $arg_decider = (($commandAst -split ' ') | Select-Object -Last 2)

    if (($arg_decider[0] -eq '-b') -or ($arg_decider[1] -eq '-b')) {
        $boards_found = west boards | Out-String | ForEach-Object { $_ -split '\r?\n' | ForEach-Object { if ($_ -match $wordToComplete) { $_ } } }
        $output = $boards_found
    }
    else {
        $output = (Get-NexusRepository).Name
    }

    $output
}
Register-ArgumentCompleter -Native -CommandName west -ScriptBlock $s
echo "West board finder loaded"
# endregion
