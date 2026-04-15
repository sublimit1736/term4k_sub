# term4k

[Chinese documentation / 中文文档](documents/README_zh_CN.md)

<p align="center">
  <strong>Project Meta</strong><br>
  <a href="https://github.com/TheBadRoger/term4k/blob/main/LICENSE"><img alt="License" src="https://img.shields.io/github/license/TheBadRoger/term4k?style=flat-square&labelColor=0f172a&color=64748b"></a>
  <a href="https://en.cppreference.com/w/cpp/20"><img alt="C++20" src="https://img.shields.io/badge/C%2B%2B-20-1d4ed8?logo=c%2B%2B&logoColor=white&style=flat-square&labelColor=0f172a"></a>
  <a href="https://github.com/TheBadRoger/term4k/commits/main"><img alt="Last Commit" src="https://img.shields.io/github/last-commit/TheBadRoger/term4k?style=flat-square&labelColor=0f172a&color=0ea5e9"></a>
  <a href="https://github.com/TheBadRoger/term4k"><img alt="Repo Size" src="https://img.shields.io/github/repo-size/TheBadRoger/term4k?style=flat-square&labelColor=0f172a&color=14b8a6"></a>
  <br><br>
  <strong>Code Stats</strong><br>
  <a href="https://github.com/TheBadRoger/term4k#live-code-statistics"><img alt="LOC" src="https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/TheBadRoger/term4k/main/.github/badges/loc.json&style=flat-square&labelColor=0f172a&color=22c55e"></a>
  <a href="https://github.com/TheBadRoger/term4k#live-code-statistics"><img alt="Languages" src="https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/TheBadRoger/term4k/main/.github/badges/languages.json&style=flat-square&labelColor=0f172a&color=0ea5e9"></a>
  <a href="https://github.com/TheBadRoger/term4k#live-code-statistics"><img alt="Top Language" src="https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/TheBadRoger/term4k/main/.github/badges/top-language.json&style=flat-square&labelColor=0f172a&color=a855f7"></a>
  <br><br>
  <strong>CI Status</strong><br>
  <a href="https://github.com/TheBadRoger/term4k/actions/workflows/build.yml"><img alt="Build" src="https://img.shields.io/github/actions/workflow/status/TheBadRoger/term4k/build.yml?branch=main&label=Build&style=flat-square&labelColor=0f172a&color=22c55e"></a>
  <a href="https://github.com/TheBadRoger/term4k/actions/workflows/unit-tests.yml"><img alt="Unit Tests" src="https://img.shields.io/github/actions/workflow/status/TheBadRoger/term4k/unit-tests.yml?branch=main&label=Unit%20Tests&style=flat-square&labelColor=0f172a&color=22c55e"></a>
</p>

## Documentation

- [Architecture Overview](documents/ARCHITECTURE.md)

term4k is a C++20 rhythm-game core logic project.

The current codebase is headless (no built-in UI layer).

It includes:

- chart parsing and gameplay timing logic
- user/account data management
- per-user runtime configuration persistence
- localization support (`en_US`, `zh_CN`)
- unit tests for core modules

<!-- README_STATS:START -->
## Live Code Statistics

> Last updated: `2026-04-15 04:42:08` (GMT).

| Metric | Value |
| --- | ---: |
| Total code lines | `14,077` |
| Class/Struct definitions (C++) | `93` |
| Function definitions (C++, heuristic) | `467` |
| Stack object declarations (C++, heuristic) | `600` |
| Static variable declarations (C++, heuristic) | `18` |
| Heap allocations via `new` (C++, heuristic) | `0` |
| `make_shared`/`make_unique` calls (C++, heuristic) | `1` |

### Distribution Chart

```mermaid
%%{init: {'theme': 'base', 'themeVariables': {'fontFamily': 'Fira Code, JetBrains Mono, Source Code Pro, Cascadia Code, Menlo, Consolas, monospace', 'pieTitleTextColor': '#e5e7eb', 'pieSectionTextColor': '#111827', 'pieOuterStrokeColor': 'transparent', 'pieOuterStrokeWidth': '0px', 'pieStrokeColor': 'transparent', 'pieStrokeWidth': '0px', 'pie1': '#6366f1', 'pie2': '#22d3ee', 'pie3': '#34d399', 'pie4': '#fbbf24', 'pie5': '#fb7185', 'pie6': '#a78bfa'}}}%%
pie showData
    title Code Distribution by Language (LOC)
    "C++ (86.2%)" : 12138
    "Markdown (5.0%)" : 710
    "JSON (3.2%)" : 456
    "Shell (3.0%)" : 422
    "CMake (2.5%)" : 351
```
<!-- README_STATS:END -->

## Install (Recommended)

Use the installer script in the project root:

```bash
./install.sh
```

The script will:

1. configure and build a Release package with CMake/CPack
2. detect your package manager (`apt`, `dnf`, `yum`, or `zypper`)
3. install the generated package automatically
4. fall back to `cmake --install` when no supported package manager is found

After installation:

```bash
term4k
```

## Remote Install (No Full Clone)

If you only download script files via `curl` or `wget`, install with:

```bash
curl -fsSL "https://raw.githubusercontent.com/TheBadRoger/term4k/main/install.sh" -o install.sh
sh install.sh --source-url "https://github.com/TheBadRoger/term4k/archive/refs/heads/main.tar.gz"
```

```bash
wget -qO install.sh "https://raw.githubusercontent.com/TheBadRoger/term4k/main/shell/install.sh"
sh install.sh --source-url "https://github.com/TheBadRoger/term4k/archive/refs/heads/main.tar.gz"
```

## Uninstall

To completely remove term4k from your system:

```bash
./uninstall.sh
```

Use non-interactive mode when needed:

```bash
./uninstall.sh --yes
```

Safe mode (remove program only, keep user data):

```bash
./uninstall.sh --keep-user-data
```

Remote uninstall (no clone):

```bash
curl -fsSL "https://raw.githubusercontent.com/TheBadRoger/term4k/main/shell/uninstall.sh" -o uninstall.sh
sh uninstall.sh --yes --keep-user-data
```

```bash
wget -qO uninstall.sh "https://raw.githubusercontent.com/TheBadRoger/term4k/main/shell/uninstall.sh"
sh uninstall.sh --yes --keep-user-data
```

## Update

Local update:

```bash
./update.sh
```

Remote update (no clone):

```bash
curl -fsSL "https://raw.githubusercontent.com/TheBadRoger/term4k/main/shell/update.sh" -o update.sh
sh update.sh --install-script-url "https://raw.githubusercontent.com/TheBadRoger/term4k/main/shell/install.sh" --source-url "https://github.com/TheBadRoger/term4k/archive/refs/heads/main.tar.gz"
```

```bash
wget -qO update.sh "https://raw.githubusercontent.com/TheBadRoger/term4k/main/shell/update.sh"
sh update.sh --install-script-url "https://raw.githubusercontent.com/TheBadRoger/term4k/main/shell/install.sh" --source-url "https://github.com/TheBadRoger/term4k/archive/refs/heads/main.tar.gz"
```

## Manual Build (Optional)

```bash
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release -j
./cmake-build-release/term4k
```

## Dependency Strategy

Third-party dependencies are resolved by `CMake` and kept outside project sources.

- Core audio: `miniaudio`
- Unit tests: `Catch2`
- TUI (optional): `FTXUI` (enabled by `TERM4K_ENABLE_TUI=ON`)

By default, dependencies are fetched automatically via `FetchContent`.

```bash
# Default mode: FetchContent
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
```

```bash
# Prefer system packages first, then fall back to FetchContent
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release -DTERM4K_USE_SYSTEM_DEPS=ON
```

## Packaging Options

By default, CPack generates DEB packages. RPM generation is controlled by `TERM4K_ENABLE_RPM` and requires `rpmbuild`.

```bash
# DEB only
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release -DTERM4K_ENABLE_RPM=OFF
cmake --build cmake-build-release --target package -j
```

```bash
# DEB + RPM (when rpmbuild is available)
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release -DTERM4K_ENABLE_RPM=ON
cmake --build cmake-build-release --target package -j
```

```bash
# CI strict mode: fail configure if RPM is requested but rpmbuild is missing
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release -DTERM4K_ENABLE_RPM=ON -DTERM4K_STRICT_RPM_CHECK=ON
cmake --build cmake-build-release --target package -j
```
