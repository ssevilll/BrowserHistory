# Browser History Management — C++23 + xUnit.net integration tests

**CSCI 2304 — Data Structures & Algorithms, Mini-Project 1**

> **Academic-use warning:** The assignment PDF says *DO NOT USE AI*. This is an AI-generated reference implementation, not evidence of independent group work. Review your course rules before using or submitting it. Group names and student IDs are intentionally placeholders.

## What is included

A fully runnable **C++23** browser-history simulator with a command-line menu. It uses a **manually implemented doubly linked list** with `prev` and `next` pointers and a separate manually implemented linked LIFO stack for deleted pages; no `std::list`, `std::forward_list`, `std::deque`, or linked-list library is used for the core structures.

Features:

1. `BrowserHistory(homepage)` creates a homepage; blank homepage allows empty imported state.
2. `visit(url)` drops the forward branch and makes the inserted page current.
3. `back(steps)` and `forward(steps)` clamp at the nearest boundary.
4. `current_page()` reports URL, title, ID, UTC visit timestamp.
5. `display_history()` prints oldest to newest, marking `[CURRENT]`.
6. Configurable capacity (default 50) prunes the **oldest** entry at the head.
7. `delete_history()` / `Delete_histroy()` clears all history and undo entries.
8. Export and import portable TXT files with the active cursor and metadata.
9. `delete_url(url)` removes the latest matching URL and saves its information in a custom stack; `restore_deleted()` restores in LIFO order near its previous position.

### Why two test suites?

**xUnit.net is a testing framework for .NET/C#, not a native C++ framework.** To meet the request for *actual xUnit*, `tests/xunit` contains a real C# **xUnit.net** project (`[Fact]` and `[Theory]` tests), which launches the compiled C++ executable, enters menu commands, and asserts on its output. The production application itself is entirely C++.

The included **native C++ unit tests** exercise the linked list implementation directly, including invariants, capacity, restoration, and file errors. They do not require .NET and can run in a C++-only environment.

The project explicitly targets **C++23** in CMake and VS Code. A compile-time check rejects older language modes; the native test suite also uses the C++23 `std::string::contains()` method.

## Requirements

- CMake 3.20+
- A compiler supporting C++23 (GCC, Clang, or Microsoft Visual C++)
- **For xUnit only:** .NET SDK 8+ and package restore/internet access the first time xUnit dependencies are installed

## Run it

macOS / Linux:

```bash
./scripts/run.sh
```

Windows (Visual Studio C++ build tools + CMake):

```bat
scripts\run.bat
```

Or use CMake manually:

```bash
cmake -S . -B build
cmake --build build --config Release
./build/browser_history             # Linux/macOS
# build\Release\browser_history.exe   (Windows with multi-config generators)
```

## Tests

### Native C++ tests (verified in this package)

```bash
./scripts/test.sh
```

Expected verified output at the bottom: `RESULT: 37 passed, 0 failed`.

### xUnit.net integration tests (actual xUnit C# test project)

macOS / Linux (requires .NET SDK):

```bash
./scripts/test.sh --xunit
```

Windows (requires .NET SDK):

```bat
scripts\test.bat --xunit
```

Or run manually after building C++:

```bash
export BROWSER_HISTORY_EXE="$(pwd)/build/browser_history"
dotnet test tests/xunit/BrowserHistory.XunitTests.csproj
```

On Windows, set `BROWSER_HISTORY_EXE` to the absolute path of your built `browser_history.exe` and run `dotnet test`.

**Verification status:** The native C++ tests were executed and passed in the creation environment. The xUnit.net suite is provided as source but **not executed here**, because the .NET SDK was unavailable. It must be run locally before relying on its results.

## Files

```text
BrowserHistoryCPP/
├── CMakeLists.txt
├── include/BrowserHistory.hpp
├── src/BrowserHistory.cpp
├── src/main.cpp
├── tests/native/BrowserHistoryNativeTests.cpp
├── tests/xunit/BrowserHistory.XunitTests.csproj
├── tests/xunit/BrowserHistoryIntegrationTests.cs
├── scripts/run.sh, test.sh, run.bat, test.bat
├── demo/demo-input.txt, demo/demo-output.txt
├── demo/native-test-results.txt, demo/BrowserHistory-Demo.mp4
├── report/Report-source.md, report/Report.pdf
└── assignment/Mini-Project1.pdf
```

## Design decisions and course-handout ambiguities

- `head` is **oldest**; `tail` is **newest**. The handout says prune oldest *tail*, which conflicts with this conventional chronological ordering. This project removes the oldest node (`head`).
- Delete removes the **newest matching URL** when duplicates exist. The removed node's metadata, former neighbor IDs, and previous index are stacked.
- Restore searches surviving neighbors; when both are gone, it prefers placement before any newer-ID history entry, then falls back to the old index. It doesn't change current unless history was empty.
- Empty history is valid; back/forward return `std::nullopt` until a new page is visited.
- A full-capacity restore evicts the oldest *existing* entry before insertion.
- The saved format uses `std::quoted` for URL and title, allowing whitespace and quotes. Import validates into a separate temporary manager and commits only on success. The deleted/undo stack is not serialized.
- Node timestamps are UTC epoch seconds, not rendered local times.

## Expected complexity

| Action | Time |
|---|---|
| Insert/unlink adjacent node | O(1) |
| Back or forward by *s* steps | O(s) |
| Visit with *k* abandoned forward pages | O(k) to delete the branch |
| Delete by exact URL | O(n) |
| Restore (search original neighbors) | O(n) worst-case |
| Display/export/import/clear | O(n) |

## Deliverable reminder

All three team members submit the same source ZIP, video MP4 and report PDF; the report must list the three names and IDs, and output filenames should begin with the group number and student ID. The included placeholder names and group identifiers are **not ready-to-submit personalized details**. The narrated video shows a replay of verified console output; it isn't a recording of your group explaining its own code.
