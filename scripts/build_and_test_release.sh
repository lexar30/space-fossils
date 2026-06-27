#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

"${script_dir}/configure_release.sh"
"${script_dir}/build_release.sh"
"${script_dir}/test_release.sh"

echo
echo "[OK] Release build and tests passed."
