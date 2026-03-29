#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import re
from dataclasses import dataclass
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
README_EN = REPO_ROOT / "README.md"
README_ZH = REPO_ROOT / "documents" / "README_zh_CN.md"
BADGES_DIR = REPO_ROOT / ".github" / "badges"

STATS_START = "<!-- README_STATS:START -->"
STATS_END = "<!-- README_STATS:END -->"
MAX_CHART_ITEMS = 6
DEFAULT_THEME = "dark"
DEFAULT_STYLE = "default"
FONT_STACK = "Fira Code, JetBrains Mono, Source Code Pro, Cascadia Code, Menlo, Consolas, monospace"

THEME_TEXTS = {
    "dark": {"title": "#e5e7eb", "section": "#f3f4f6"},
    "light": {"title": "#111827", "section": "#1f2937"},
}

PIE_STYLE_PALETTES = {
    "default": ["#6366f1", "#22d3ee", "#34d399", "#fbbf24", "#fb7185", "#a78bfa"],
    "neon": ["#00f5ff", "#39ff14", "#ff2bd6", "#ffe600", "#00ffa3", "#9d4edd"],
}

EXCLUDED_DIRS = {
    ".git",
    ".idea",
    ".github",
    "build",
    "cmake-build-debug",
    "cmake-build-release",
    "cmake-build-release-strict",
    "cmake-build-minsizerel",
    "cmake-build-relwithdebinfo",
    "_deps",
}

EXCLUDED_FILE_NAMES = {
    "catch_amalgamated.cpp",
    "catch_amalgamated.hpp",
    "miniaudio.h",
    "miniaudio_impl.cpp",
}


@dataclass(frozen=True)
class LanguageRule:
    name: str
    suffixes: tuple[str, ...]


@dataclass(frozen=True)
class CppSymbolStats:
    class_count: int
    function_count: int
    stack_object_count: int
    static_variable_count: int
    heap_new_count: int
    smart_factory_count: int


LANGUAGE_RULES = [
    LanguageRule("C++", (".cpp", ".cc", ".cxx", ".hpp", ".hh", ".hxx", ".h")),
    LanguageRule("CMake", (".cmake",)),
    LanguageRule("Shell", (".sh",)),
    LanguageRule("Markdown", (".md",)),
    LanguageRule("JSON", (".json",)),
    LanguageRule("YAML", (".yml", ".yaml")),
]

SPECIAL_FILES = {
    "CMakeLists.txt": "CMake",
}

CPP_SOURCE_SUFFIXES = {".cpp", ".cc", ".cxx", ".hpp", ".hh", ".hxx", ".h"}
CPP_BUILTIN_TYPES = {
    "auto", "bool", "char", "char8_t", "char16_t", "char32_t", "double", "float", "int", "long",
    "short", "signed", "size_t", "ssize_t", "std::size_t", "std::ptrdiff_t", "uint8_t", "uint16_t",
    "uint32_t", "uint64_t", "int8_t", "int16_t", "int32_t", "int64_t", "unsigned", "void", "wchar_t",
}

CLASS_DEF_RE = re.compile(r"\b(?:class|struct)\s+[A-Za-z_]\w*\b(?:\s+final)?\s*(?:[:{])")
VAR_DECL_RE = re.compile(
    r"^\s*(?:const\s+|constexpr\s+|static\s+|mutable\s+|inline\s+|thread_local\s+|volatile\s+)*"
    r"(?P<type>(?:[A-Za-z_]\w*(?:::[A-Za-z_]\w*)*(?:<[^;{}()]+>)?)|(?:unsigned|signed|long|short|int|double|float|bool|char|wchar_t)(?:\s+(?:long|int|short))?)"
    r"(?:\s*[*&]\s*)?\s+"
    r"(?P<name>[A-Za-z_]\w*)\s*"
    r"(?:\([^;{}]*\)|\{[^;{}]*\}|=[^;{}]*)?\s*;"
)
NEW_EXPR_RE = re.compile(r"(?<!operator\s)\bnew\b")
SMART_FACTORY_RE = re.compile(r"\b(?:std::)?make_(?:shared|unique)\s*<")
FUNCTION_DEF_RE = re.compile(
    r"^\s*(?!if\b|for\b|while\b|switch\b|catch\b|else\b|do\b|try\b)"
    r"(?:template\s*<[^;{}]+>\s*)?"
    r"(?:(?:inline|constexpr|consteval|constinit|static|virtual|explicit|friend|extern)\s+)*"
    r"(?:(?:[\w:<>~]+\s+)+)?"
    r"[~A-Za-z_]\w*(?:::[~A-Za-z_]\w*)*\s*\([^;{}]*\)\s*"
    r"(?:(?:const|noexcept|override|final)\s*)*"
    r"(?:->\s*[^;{}]+)?\s*\{\s*$",
    flags=re.MULTILINE,
)


CONTROL_HEADS = {
    "if", "for", "while", "switch", "return", "case", "throw", "catch", "using", "typedef",
    "delete", "new", "class", "struct", "namespace", "template", "friend", "public", "private",
    "protected", "else", "do", "goto", "static_assert", "requires",
}


def should_skip(path: Path) -> bool:
    if path.name in EXCLUDED_FILE_NAMES:
        return True

    for part in path.parts:
        if part in EXCLUDED_DIRS:
            return True

    # Keep generated badge json and workflow files out of language stats.
    if ".github" in path.parts:
        return True

    return False


def detect_language(path: Path) -> str | None:
    if path.name in SPECIAL_FILES:
        return SPECIAL_FILES[path.name]

    suffix = path.suffix.lower()
    for rule in LANGUAGE_RULES:
        if suffix in rule.suffixes:
            return rule.name

    return None


def count_lines(path: Path) -> int:
    with path.open("r", encoding="utf-8", errors="ignore") as f:
        return sum(1 for _ in f)


def collect_stats() -> dict[str, int]:
    totals: dict[str, int] = {}

    for file_path in REPO_ROOT.rglob("*"):
        if not file_path.is_file() or should_skip(file_path):
            continue

        language = detect_language(file_path)
        if language is None:
            continue

        totals[language] = totals.get(language, 0) + count_lines(file_path)

    return dict(sorted(totals.items(), key=lambda item: item[1], reverse=True))


def format_num(value: int) -> str:
    return f"{value:,}"


def compress_for_chart(language_lines: dict[str, int], max_items: int = MAX_CHART_ITEMS) -> list[tuple[str, int]]:
    items = list(language_lines.items())
    if len(items) <= max_items:
        return items

    head = items[:max_items]
    other_total = sum(lines for _, lines in items[max_items:])
    if other_total > 0:
        head.append(("Other", other_total))
    return head


def build_mermaid_pie_init(theme: str, style: str) -> str:
    text_colors = THEME_TEXTS.get(theme, THEME_TEXTS[DEFAULT_THEME])
    palette = PIE_STYLE_PALETTES.get(style, PIE_STYLE_PALETTES[DEFAULT_STYLE])

    return (
        "%%{init: {'theme': 'base', 'themeVariables': "
        f"{{'fontFamily': '{FONT_STACK}', "
        f"'pieTitleTextColor': '{text_colors['title']}', 'pieSectionTextColor': '{text_colors['section']}', "
        "'pieOuterStrokeColor': 'transparent', 'pieOuterStrokeWidth': '0px', "
        "'pieStrokeColor': 'transparent', 'pieStrokeWidth': '0px', "
        f"'pie1': '{palette[0]}', 'pie2': '{palette[1]}', 'pie3': '{palette[2]}', "
        f"'pie4': '{palette[3]}', 'pie5': '{palette[4]}', 'pie6': '{palette[5]}'}}}}}}%%"
    )


def build_mermaid_pie(language_lines: dict[str, int], total_lines: int, theme: str, style: str) -> str:
    chart_items = compress_for_chart(language_lines)
    lines = ["```mermaid", build_mermaid_pie_init(theme, style), "pie showData", '    title Code Distribution by Language (LOC)']
    for language, count in chart_items:
        label = f"{language} ({0.0 if total_lines == 0 else (count / total_lines * 100):.1f}%)"
        lines.append(f'    "{label}" : {count}')
    lines.append("```")
    return "\n".join(lines)


def build_stats_block_en(
    language_lines: dict[str, int],
    total_lines: int,
    cpp_stats: CppSymbolStats,
    theme: str,
    style: str,
) -> str:
    pie = build_mermaid_pie(language_lines, total_lines, theme, style)
    table = "\n".join([
        "| Metric | Value |",
        "| --- | ---: |",
        f"| Total code lines | `{format_num(total_lines)}` |",
        f"| Class/Struct definitions (C++) | `{format_num(cpp_stats.class_count)}` |",
        f"| Function definitions (C++, heuristic) | `{format_num(cpp_stats.function_count)}` |",
        f"| Stack object declarations (C++, heuristic) | `{format_num(cpp_stats.stack_object_count)}` |",
        f"| Static variable declarations (C++, heuristic) | `{format_num(cpp_stats.static_variable_count)}` |",
        f"| Heap allocations via `new` (C++, heuristic) | `{format_num(cpp_stats.heap_new_count)}` |",
        f"| `make_shared`/`make_unique` calls (C++, heuristic) | `{format_num(cpp_stats.smart_factory_count)}` |",
    ])
    return (
        f"{STATS_START}\n"
        "## Live Code Statistics\n\n"
        "> Auto-updated by `.github/workflows/readme-stats.yml`.\n\n"
        f"> Chart theme preset: `{theme}` + `{style}`.\n"
        "> Switch quickly: `python3 scripts/update_readme_stats.py --theme light --style neon`.\n\n"
        f"{table}\n\n"
        "### Distribution Chart\n\n"
        f"{pie}\n"
        f"{STATS_END}"
    )


def build_stats_block_zh(
    language_lines: dict[str, int],
    total_lines: int,
    cpp_stats: CppSymbolStats,
    theme: str,
    style: str,
) -> str:
    pie = build_mermaid_pie(language_lines, total_lines, theme, style)
    table = "\n".join([
        "| 指标 | 数值 |",
        "| --- | ---: |",
        f"| 代码总行数 | `{format_num(total_lines)}` |",
        f"| 类/结构体定义数量（C++） | `{format_num(cpp_stats.class_count)}` |",
        f"| 函数定义数量（C++，启发式） | `{format_num(cpp_stats.function_count)}` |",
        f"| 栈对象声明数量（C++，启发式） | `{format_num(cpp_stats.stack_object_count)}` |",
        f"| 静态变量声明数量（C++，启发式） | `{format_num(cpp_stats.static_variable_count)}` |",
        f"| `new` 堆分配次数（C++，启发式） | `{format_num(cpp_stats.heap_new_count)}` |",
        f"| `make_shared`/`make_unique` 调用次数（C++，启发式） | `{format_num(cpp_stats.smart_factory_count)}` |",
    ])
    return (
        f"{STATS_START}\n"
        "## 实时代码统计\n\n"
        "> 由 `.github/workflows/readme-stats.yml` 自动更新。\n\n"
        f"> 当前图表主题：`{theme}` + `{style}`。\n"
        "> 快速切换：`python3 scripts/update_readme_stats.py --theme light --style neon`。\n\n"
        f"{table}\n\n"
        "### 分布图\n\n"
        f"{pie}\n"
        f"{STATS_END}"
    )


def replace_between_markers(content: str, replacement: str) -> str:
    if STATS_START in content and STATS_END in content:
        start = content.index(STATS_START)
        end = content.index(STATS_END) + len(STATS_END)
        return content[:start] + replacement + content[end:]

    return content.rstrip() + "\n\n" + replacement + "\n"


def write_badges(language_lines: dict[str, int], total_lines: int, cpp_stats: CppSymbolStats) -> None:
    BADGES_DIR.mkdir(parents=True, exist_ok=True)

    top_language = next(iter(language_lines.keys()), "N/A")
    top_share = 0.0 if total_lines == 0 else language_lines[top_language] / total_lines * 100 if top_language in language_lines else 0.0

    badge_payloads = {
        "loc.json": {
            "schemaVersion": 1,
            "label": "LOC",
            "message": format_num(total_lines),
            "color": "2ea44f",
        },
        "languages.json": {
            "schemaVersion": 1,
            "label": "Languages",
            "message": str(len(language_lines)),
            "color": "1f6feb",
        },
        "top-language.json": {
            "schemaVersion": 1,
            "label": "Top Language",
            "message": f"{top_language} {top_share:.1f}%",
            "color": "8250df",
        },
    }

    for file_name, payload in badge_payloads.items():
        (BADGES_DIR / file_name).write_text(
            json.dumps(payload, ensure_ascii=False, indent=2) + "\n",
            encoding="utf-8",
        )


def is_cpp_file(path: Path) -> bool:
    return path.suffix.lower() in CPP_SOURCE_SUFFIXES


def strip_cpp_comments_and_strings(content: str) -> str:
    # Remove comments first, then strings to reduce regex false positives.
    content = re.sub(r"//.*?$", "", content, flags=re.MULTILINE)
    content = re.sub(r"/\*.*?\*/", "", content, flags=re.DOTALL)
    content = re.sub(r'"(?:\\.|[^"\\])*"', '""', content)
    content = re.sub(r"'(?:\\.|[^'\\])*'", "''", content)
    return content


def collect_cpp_symbol_stats() -> CppSymbolStats:
    class_count = 0
    function_count = 0
    stack_object_count = 0
    static_variable_count = 0
    heap_new_count = 0
    smart_factory_count = 0

    for file_path in REPO_ROOT.rglob("*"):
        if not file_path.is_file() or should_skip(file_path) or not is_cpp_file(file_path):
            continue

        raw = file_path.read_text(encoding="utf-8", errors="ignore")
        text = strip_cpp_comments_and_strings(raw)

        class_count += len(CLASS_DEF_RE.findall(text))
        function_count += len(FUNCTION_DEF_RE.findall(text))
        heap_new_count += len(NEW_EXPR_RE.findall(text))
        smart_factory_count += len(SMART_FACTORY_RE.findall(text))

        for line in text.splitlines():
            match = VAR_DECL_RE.match(line)
            if not match:
                continue

            stripped = line.strip()
            head = stripped.split("(")[0].split()[0]
            if head in CONTROL_HEADS:
                continue

            type_name = match.group("type").split("<", 1)[0].strip()
            if type_name in CPP_BUILTIN_TYPES:
                continue

            name = match.group("name")
            # Avoid counting obvious function declarations as object instantiations.
            if re.search(rf"\b{name}\s*\([^)]*\)\s*;", stripped) and "=" not in stripped and "{" not in stripped:
                continue

            if re.search(r"\bstatic\b", stripped):
                static_variable_count += 1

            if "::" in type_name:
                base = type_name.split("::")[-1]
            else:
                base = type_name
            if base.isupper():
                continue

            stack_object_count += 1

    return CppSymbolStats(
        class_count=class_count,
        function_count=function_count,
        stack_object_count=stack_object_count,
        static_variable_count=static_variable_count,
        heap_new_count=heap_new_count,
        smart_factory_count=smart_factory_count,
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Update README stats blocks and shields badge payloads.")
    parser.add_argument("--theme", choices=sorted(THEME_TEXTS.keys()), default=None, help="Mermaid chart text theme.")
    parser.add_argument("--style", choices=sorted(PIE_STYLE_PALETTES.keys()), default=None, help="Mermaid chart color style preset.")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    theme = (args.theme or os.getenv("TERM4K_README_THEME") or DEFAULT_THEME).strip().lower()
    style = (args.style or os.getenv("TERM4K_README_STYLE") or DEFAULT_STYLE).strip().lower()
    if theme not in THEME_TEXTS:
        theme = DEFAULT_THEME
    if style not in PIE_STYLE_PALETTES:
        style = DEFAULT_STYLE

    language_lines = collect_stats()
    total_lines = sum(language_lines.values())
    cpp_stats = collect_cpp_symbol_stats()

    README_EN.write_text(
        replace_between_markers(
            README_EN.read_text(encoding="utf-8"),
            build_stats_block_en(language_lines, total_lines, cpp_stats, theme, style),
        ),
        encoding="utf-8",
    )

    README_ZH.write_text(
        replace_between_markers(
            README_ZH.read_text(encoding="utf-8"),
            build_stats_block_zh(language_lines, total_lines, cpp_stats, theme, style),
        ),
        encoding="utf-8",
    )

    write_badges(language_lines, total_lines, cpp_stats)


if __name__ == "__main__":
    main()

