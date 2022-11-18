#!/bin/bash

./scripts/twister --all --cmake-only -T samples/hello_world
find twister-out -name "edt.pickle" | xargs -t -I % sh -c './scripts/dts/add_board_dts_sqlite.py --board-name `echo % | sed "s/\//\n/g" | head -n2 | tail -n1` --edt-pickle % --sqlite-db boards.sqlite'
