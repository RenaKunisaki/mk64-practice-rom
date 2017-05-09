#!/bin/bash
set -e
./build.sh bootstrap
./build.sh lib
./build.sh crash-handler
