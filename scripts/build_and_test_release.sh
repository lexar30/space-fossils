#!/usr/bin/env bash
set -euo pipefail

source "$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)/_presets.sh"

sf_require_supported_platform
sf_enter_project_root

configure_preset="$(sf_configure_preset release)"
build_preset="$(sf_build_preset release)"
test_preset="$(sf_test_preset release)"

echo "[CONFIGURE] ${configure_preset}"
cmake --preset "${configure_preset}"

echo
echo "[BUILD] ${build_preset}"
cmake --build --preset "${build_preset}"

echo
echo "[TEST] ${test_preset}"
ctest --preset "${test_preset}"

echo
echo "[OK] Release build and tests passed."
