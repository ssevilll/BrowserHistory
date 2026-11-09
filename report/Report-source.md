# CSCI 2304 - Data Structures & Algorithms
## Mini-Project 1: Browser History Management (C++23)

**Reference implementation report: course handout explicitly says DO NOT USE AI.**

**Group number:** [GROUP NUMBER]

| Team member | Student ID |
|---|---|
| [MEMBER 1] | [ID 1] |
| [MEMBER 2] | [ID 2] |
| [MEMBER 3] | [ID 3] |

### 1. Overview
This program simulates browser history navigation using a manually implemented doubly linked list, with a cursor for the currently active page. It supports visiting URLs, backward/forward navigation, branching, history display, maximum capacity, clear history, deletion and stack-based restoration, and saving/loading a TXT file. It is a console application and does not fetch live webpages.

### 2. Classes and responsibilities
- `PageInfo` contains ID, URL, optional title, and UTC timestamp as epoch seconds.
- `PageNode` contains `PageInfo` and bidirectional raw pointers (`prev`, `next`).
- `DoublyLinkedList` manually owns nodes and handles insertion, removal, traversal, cleanup, and invariant validation. Both directions are updated for each mutation; no `std::list`, deque or linked-list wrapper is used.
- `DeletedHistoryStack` is a separate handwritten singly linked LIFO stack, storing removed-page metadata, original neighbor IDs and the original index.
- `BrowserHistory` owns the list, stack, current-page pointer, capacity and next page ID.
- `main.cpp` implements the user-facing command-line menu.

### 3. Navigation behavior
History order is oldest at `head`, newest at `tail`. Visiting a URL after using Back first erases all forward entries, then inserts a new node after the cursor and makes it current. Back/Forward traverse the corresponding pointer until the requested step count or a boundary is reached. A query for the current page does not move the cursor.

### 4. Deletion, restoration and capacity
Exact-URL deletion removes the newest matching occurrence. The undo stack stores sufficient information to reinsert the page near the former neighbors. Restore pops the latest deletion (LIFO). If both neighboring nodes have since disappeared, a newer-ID anchor or original index acts as fallback. If capacity is reached when a page is visited, the oldest node (`head`) is deleted. Restore at full capacity also makes room by evicting the oldest existing node.

The assignment says 'oldest node from tail' but the chosen chronological order makes `head` the oldest; this is an explicit documented assumption. Empty history after Clear or deleting the last page is valid, and no page is current until another visit or restoration.

### 5. Import/export and error handling
A human-readable UTF-8 TXT file starts with `BROWSER_HISTORY_CPP_V1` and contains capacity, current page ID, number of pages, then page ID, quoted URL, quoted title, and UTC epoch timestamp. `std::quoted` preserves spaces, quotes and backslashes. Import validates the complete file into a temporary history before replacing the active state. Invalid input leaves the previous state unchanged. Undo history is not exported. Negative navigation steps and blank URLs are rejected.

### 6. Complexity
- Local insertion/unlink: O(1), plus any required node destruction.
- Back/Forward by s steps: O(s).
- Visit with k forward nodes: O(k) for clearing the branch.
- Exact URL deletion: O(n), as locating the latest match may scan the list.
- Restoration: O(n) worst-case for neighbor searches.
- Display, clear, export and import: O(n).

### 7. Testing and results
The `tests/native/BrowserHistoryNativeTests.cpp` executable contains 37 native C++ cases. They cover homepage creation, normal and boundary navigation, branching, current pointer state, display, deleting all node positions, duplicate URLs, LIFO restore, cap=1 and cap=3, empty/clear behavior, file round-trip and invalid input rollback, and a deterministic 1,500-step stress test with invariants. The packaged test run completed: **37 passed, 0 failed**.

An additional genuine `xUnit.net` C# test project is provided at `tests/xunit/`. It tests the compiled C++ console program as a black box through `[Fact]` and `[Theory]` tests. This is necessary because xUnit.net tests are written for .NET; it does not directly execute C++ unit tests. The xUnit suite **was not executed in the creation environment** because .NET SDK 8 is unavailable here. It needs to be run locally using `./scripts/test.sh --xunit` or `scripts\\test.bat --xunit`.

### 8. Reproduction and demonstration
Build with CMake 3.20+ and a C++23 compiler. Run `./scripts/run.sh` for the menu or `./scripts/test.sh` for native tests (Windows `.bat` equivalents available). The file `demo/demo-input.txt` records a full example interaction, `demo/demo-output.txt` contains its verified C++ output, and `demo/BrowserHistory-Demo.mp4` is a narrated reference walkthrough. Both the production code and native tests compile without third-party C++ packages; the xUnit project requires NuGet package restore.
