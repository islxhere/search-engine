#!/usr/bin/env bash
set -euo pipefail

root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
cd "$root"

cmake -S . -B build
cmake --build build --target build_dictionary

set -a
source .env
set +a

exec ./build_dictionary
