#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

"${script_dir}/configure_debug.sh"
"${script_dir}/build_debug.sh"
"${script_dir}/test_debug.sh"

echo
echo "[OK] Debug build and tests passed."
