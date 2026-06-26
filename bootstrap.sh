#!/usr/bin/env bash
set -euo pipefail

repo_root="$(git rev-parse --show-toplevel)"
cd "${repo_root}"

echo "[SETUP] Configure Git hooks path"
git config core.hooksPath .githooks

echo "[SETUP] Make scripts executable"
chmod +x scripts/*.sh
chmod +x .githooks/pre-commit
chmod +x .githooks/pre-push

echo
echo "[OK] Developer setup completed."
echo "Git hooks path: $(git config --get core.hooksPath)"