#!/bin/sh
#
# Build all apps found in passed directory
#
# Usage build_app_in_dir.sh <path_to_dir_with_apps>

if [ ! -d "$1" ]; then
    echo "The passed directory doesn't exist. Exit."
    return
fi

find "$1" -iname "CMakeLists.txt" | while read line; do
    echo "Building ${line%/*} app ..."
    west build -p -b tlsr9518adk80d "${line%/*}"
done
