#!/bin/bash

./scripts/twister --all --cmake-only -T tests -T samples
./scripts/build_sqlite.py --twister-out twister-out --sqlite-db build_metadata.db
