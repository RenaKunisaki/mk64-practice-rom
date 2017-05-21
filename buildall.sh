#!/bin/bash
set -e
./build.sh bootstrap
./build.sh hooks
./build.sh lib
./build.sh crash-handler
./build.sh debug
./build.sh new-menu
