# Copyright © 2025, Tais Hjortshøj <tbh@mjolner.dk> / Mjølner Informatics A/S
# SPDX-License-Identifier: Apache-2.0

# region custom west board finder initialize
$s = {
    param($wordToComplete, $commandAst, $cursorPosition)

    function Get-MatchingBoards {
        param($wordToComplete)
        west boards | Out-String | ForEach-Object {
             $_ -split '\r?\n' | Where-Object { $_ -CMatch "^$wordToComplete.*" } | Sort-Object
        }
    }

    $commandDecider = (($commandAst -split ' ') | Select-Object -First 2 -ExpandProperty $_) -join ' '
    if ($commandDecider -eq 'west build') {

        $argDecider = (($commandAst -split ' ') | Select-Object -Last 2)

        if ($argDecider -contains '-b' -or $argDecider -contains '--board') {
            $boardsFound = Get-MatchingBoards -wordToComplete $wordToComplete
            $output = $boardsFound
        } else {
            # Fallback to default behavior of suggesting files in the current directory
            $output = (Get-NexusRepository).Name
        }
    } else {
        # Fallback to default behavior of suggesting files in the current directory
        $output = (Get-NexusRepository).Name
    }

    # Uncomment the following lines to log the output for debugging purposes
    # @("wordToComplete:  $wordToComplete",
    # "commandAst:        $commandAst",
    # "cursorPosition:    $cursorPosition",
    # "commandDecider:    $commandDecider",
    # "argDecider:        $argDecider",
    # "",
    # "boardsFound:",
    # ($boardsFound | ForEach-Object { $_ -split ' '})
    # ) | Set-Content log.txt

    $output
}
Register-ArgumentCompleter -Native -CommandName west -ScriptBlock $s
echo "West completion tool loaded"
# endregion
