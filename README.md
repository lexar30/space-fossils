# space-fossils

The goal is a lightweight disk usage visualizer: scan a directory tree, calculate file/folder sizes, and later visualize the result.

Current structure:

* `core/` — core library code
* `app/` — application executable
* `tests/` — test executable and micro test framework
* `scripts/` — build/test helper scripts
* `.githooks/` — Git hooks for local checks

## Requirements

Windows:

* CMake 3.24+
* Visual Studio 2022
* Git Bash / MSYS2 / WSL for shell scripts

Linux/Debian:

* CMake 3.24+
* Ninja
* C++20-capable compiler

## First setup

Run once after cloning:

```bash
./bootstrap.sh
```

This configures repository-local Git hooks and makes scripts executable.

## Build and test

Debug:

```bash
./scripts/build_and_test_debug.sh
```

Release:

```bash
./scripts/build_and_test_release.sh
```

Separate commands are also available:

```bash
./scripts/configure_debug.sh
./scripts/build_debug.sh
./scripts/test_debug.sh

./scripts/configure_release.sh
./scripts/build_release.sh
./scripts/test_release.sh
```

## Git hooks

After running `bootstrap.sh`:

* `pre-commit` runs Debug build and tests
* `pre-push` runs Debug and Release build/tests

## Build output

Generated build files are placed under:

```text
builds/
```

