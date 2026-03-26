# term4k

[Chinese documentation / 中文文档](documents/README_zh_CN.md)

term4k is a terminal-based rhythm game project written in C++20.

It includes:

- chart parsing and gameplay timing logic
- user/account data management
- per-user runtime configuration persistence
- localization support (`en_US`, `zh_CN`)
- unit tests for core modules

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
curl -fsSL "<raw-install-script-url>" -o install.sh
sh install.sh --source-url "<term4k-source-archive-tar-gz-url>"
```

```bash
wget -qO install.sh "<raw-install-script-url>"
sh install.sh --source-url "<term4k-source-archive-tar-gz-url>"
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
curl -fsSL "<raw-uninstall-script-url>" -o uninstall.sh
sh uninstall.sh --yes --keep-user-data
```

```bash
wget -qO uninstall.sh "<raw-uninstall-script-url>"
sh uninstall.sh --yes --keep-user-data
```

## Update

Local update:

```bash
./update.sh
```

Remote update (no clone):

```bash
curl -fsSL "<raw-update-script-url>" -o update.sh
sh update.sh --install-script-url "<raw-install-script-url>" --source-url "<term4k-source-archive-tar-gz-url>"
```

```bash
wget -qO update.sh "<raw-update-script-url>"
sh update.sh --install-script-url "<raw-install-script-url>" --source-url "<term4k-source-archive-tar-gz-url>"
```

## Manual Build (Optional)

```bash
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release -j
./cmake-build-release/term4k
```

