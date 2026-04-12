# term4k UI (Phase 1)

This module contains the first homepage-only TUI implementation.

## Scope

- FTXUI-based homepage screen (`HomePageUI`)
- Runtime theme resolution from `RuntimeConfigs::theme`
- Theme preset loading through `ThemePresetService`
- Truecolor render path with 256-color quantization fallback

## Build

Enable the UI explicitly at configure time:

```bash
cmake -S . -B cmake-build-debug -DTERM4K_ENABLE_TUI=ON
cmake --build cmake-build-debug -j
./cmake-build-debug/term4k
```

When `TERM4K_ENABLE_TUI=OFF`, `term4k` stays headless.

