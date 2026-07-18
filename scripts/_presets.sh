#!/usr/bin/env bash

sf_project_root() {
  local script_dir
  script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
  cd -- "${script_dir}/.." && pwd
}

sf_platform() {
  local uname_value
  uname_value="$(uname -s)"

  case "${uname_value}" in
    MINGW*|MSYS*|CYGWIN*)
      echo "windows"
      ;;
    Linux*)
      echo "linux"
      ;;
    *)
      echo "unsupported"
      ;;
  esac
}

sf_configure_preset() {
  local config="$1"
  local platform
  platform="$(sf_platform)"

  case "${platform}:${config}" in
    windows:debug|windows:release)
      echo "windows-vs2022"
      ;;
    linux:debug)
      echo "linux-ninja-debug"
      ;;
    linux:release)
      echo "linux-ninja-release"
      ;;
    *)
      return 1
      ;;
  esac
}

sf_build_preset() {
  local config="$1"
  local platform
  platform="$(sf_platform)"

  case "${platform}:${config}" in
    windows:debug)
      echo "windows-vs2022-debug"
      ;;
    windows:release)
      echo "windows-vs2022-release"
      ;;
    linux:debug)
      echo "linux-ninja-debug"
      ;;
    linux:release)
      echo "linux-ninja-release"
      ;;
    *)
      return 1
      ;;
  esac
}

sf_test_preset() {
  sf_build_preset "$1"
}

sf_enter_project_root() {
  local root
  root="$(sf_project_root)" || return 1
  cd -- "${root}" || return 1
}

sf_require_supported_platform() {
  local platform
  platform="$(sf_platform)"

  if [[ "${platform}" == "unsupported" ]]; then
    echo "[ERROR] Unsupported platform: $(uname -s)"
    echo "Supported: Windows Git Bash/MSYS/Cygwin, Linux."
    return 1
  fi
}
