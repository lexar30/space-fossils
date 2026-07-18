# space-fossils

`space-fossils` is a C++20 disk usage analyzer. It scans a filesystem tree into a compact in-memory representation and provides an interactive CLI for inspecting directories, navigating the captured tree, and saving or loading snapshots.

## Features

- Scan a directory tree and calculate logical sizes.
- Navigate the captured tree without accessing the filesystem again.
- Show the current entry, its children, the subtree, and entries sorted by size.
- Rescan the current directory.
- Switch between binary and decimal size units.
- Save the current tree to a snapshot and load it later.
- Preserve the active tree when snapshot loading fails.
- Report scan and snapshot timing metrics.

## Requirements

- CMake 3.24+
- Bash for the repository helper scripts

Tested toolchains:

- Windows x64: Visual Studio Community 2022 17.14 (MSVC)
- Linux x64: GCC 16.1.1 and Ninja

Other platforms and compilers are not currently tested or guaranteed to work.

## First setup

Run once after cloning:

```bash
./bootstrap.sh
```

This configures the repository-local Git hooks and makes the helper scripts executable.

## Build and test

Configure, build, and run all tests in Debug mode:

```bash
./scripts/build_and_test_debug.sh
```

For Release mode:

```bash
./scripts/build_and_test_release.sh
```

The stages can also be run separately:

```bash
./scripts/configure_debug.sh
./scripts/build_debug.sh
./scripts/test_debug.sh

./scripts/configure_release.sh
./scripts/build_release.sh
./scripts/test_release.sh
```

Generated files are placed under `builds/`.

## Run

Release executable locations:

```text
Windows: builds/windows-vs2022/app/Release/space_fossils_app.exe
Linux:   builds/linux-ninja-release/app/space_fossils_app
```

## CLI commands

| Command | Alias | Description |
| --- | --- | --- |
| `help` | `h` | Show available commands. |
| `quit` | `q` | Quit the application. |
| `units <binary\|decimal>` | | Select the file size unit system. |
| `scan <path>` | | Scan a directory into an empty tree. Use `reset` before replacing an existing tree. |
| `rescan` | | Rescan the current directory from its original filesystem location. |
| `save <path>` | | Save the current tree to a snapshot file. |
| `load <path>` | | Load a snapshot and replace the current tree after a successful read. |
| `tree` | `t` | Print the subtree rooted at the current entry. |
| `children` | `ls` | List direct children of the current directory. |
| `info` | `i` | Show information about the current entry. |
| `top` | | List all direct children sorted by logical size. |
| `cd <path>` | | Change the current directory inside the captured tree. |
| `pwd` | | Print the current path inside the captured tree. |
| `reset` | `rst` | Clear the current tree and navigation state. |

## Project structure

- `core/` — scanning, storage, navigation, reporting, snapshots, and memory pools.
- `cli/` — command parsing, dispatching, rendering, and CLI application state.
- `app/` — application executable entry point.
- `tests/` — test suites and the micro test framework.
- `benchmarks/` — benchmark notes and baselines.
- `scripts/` — cross-platform configure, build, and test helpers.
- `.githooks/` — repository-local pre-commit and pre-push checks.

## Git hooks

After running `bootstrap.sh`:

- `pre-commit` runs the Debug build and tests.
- `pre-push` runs both Debug and Release builds and tests.
