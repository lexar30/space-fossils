#!/usr/bin/env bash
set -euo pipefail

source "$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)/_presets.sh"

sf_require_supported_platform
sf_enter_project_root

preset="$(sf_configure_preset release)"

echo "[CONFIGURE] ${preset}"
cmake --preset "${preset}"

echo
echo "[OK] Release configure completed."
