# Quality Guidelines

> Code quality standards for the metaRTC C/C++ backend.

---

## Overview

metaRTC targets Linux, Windows, macOS, Android, and iOS. Code must be portable, avoid exceptions, and manage memory manually. The project mixes C and C++11, so each file should be written in the appropriate language for its layer.

---

## Language Boundaries

| Library | Primary Language |
|---------|-----------------|
| `libmetartccore7` | C (C99 / gnu11) |
| `libmetartc7` | C++11 |
| `libyangwhip7` | C |
| Demos | C++11 + Qt6 |

- Core RTC protocol code is C for easier embedded and cross-language binding.
- Higher-level capture/encode/player wrappers are C++11.

---

## Forbidden Patterns

- **Exceptions**: Do not use C++ exceptions. Use error codes and early returns.
- **STL in hot paths**: Avoid `std::vector`, `std::map`, or dynamic allocation inside per-frame loops.
- **Unchecked allocations**: Always check `malloc`/`new` results where practical.
- **Raw `delete`/`free`**: Prefer the project macros `yang_delete`, `yang_deleteA`, `yang_free` to nullify pointers after release.
- **Platform `#ifdef` scattered randomly**: Centralize platform code in `Yang_OS_*` guarded blocks or dedicated platform subdirectories.
- **Logging in tight loops**: Do not log per-frame/per-packet at INFO or higher.

---

## Required Patterns

- **Thread safety**: Use `YangThread`, `yang_thread_mutex_t`, `yang_thread_cond_t` wrappers. Use `std::atomic` for shared flags.
- **Single lock ownership**: Protect shared data with one mutex; condition variables use the same mutex. See `libmetartc7` thread-safety spec for details.
- **Resource cleanup in destructors**: C++ classes must clean up all owned resources in their destructors.
- **Header guards**: Use `#ifndef INCLUDE_<PATH>_<NAME>_H_` style.

---

## Memory Management

```cpp
YangSysMessage* mes = new YangSysMessage();
// ... use mes ...
yang_delete(mes);  // sets mes = NULL
```

For C arrays:

```cpp
char* buf = new char[size];
yang_deleteA(buf);
```

---

## Testing

There is no comprehensive unit test suite. Verification is primarily through:

- Building all target platforms (`cmake_lib_x64.sh`, `cmake_lib_android.sh`, etc.)
- Running Qt demos (`metapushstream7`, `metaplayer7`) end-to-end
- Checking for data races with ThreadSanitizer / Helgrind where feasible

When fixing concurrency bugs, add temporary diagnostic logs, reproduce the issue, then remove the logs before committing.

---

## Code Review Checklist

- [ ] Does the change build on Linux x64?
- [ ] Are platform-specific paths guarded correctly?
- [ ] Are mutexes held for the shortest possible time?
- [ ] Are error codes propagated correctly?
- [ ] Is memory freed on all error paths?
- [ ] Are debug logs removed before final commit?
