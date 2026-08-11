# Error Handling

> How errors are handled in the metaRTC C/C++ backend.

---

## Overview

metaRTC uses integer error codes (`int32_t`) rather than exceptions. Success is defined as `0` (`Yang_Ok`). Non-zero values indicate specific error conditions. Functions that can fail should return `int32_t` and propagate errors up the call stack.

---

## Error Code Definitions

All error codes live in `include/yangutil/yangerrorcode.h`.

```c
#define Yang_Ok 0
#define ERROR_CODEC_Encode 20
#define ERROR_SYS_NoVideoDevice 111
#define ERROR_SOCKET_Timeout 202
#define ERROR_TLS 300
```

Categories:

| Range | Category |
|-------|----------|
| 0 | Success (`Yang_Ok`) |
| 1-19 | Session/room |
| 20-99 | Codec |
| 100-199 | System / capture / device |
| 200-299 | Socket / network |
| 300-399 | TLS / SSL / string / JSON |
| 2100-2199 | RTMP |
| 3000-3999 | SRT |

Add new error codes in the appropriate range with a descriptive name.

---

## Return Code Pattern

Functions return `int32_t`. Callers check for `Yang_Ok`:

```cpp
int32_t ret = yang_do_something();
if (ret != Yang_Ok) {
    yang_error("Failed to do something: %d", ret);
    return ret;
}
```

Higher-level functions should propagate errors rather than swallow them, unless the error is explicitly recoverable.

---

## Error Logging Helper

`yang_error_wrap` logs an error and returns the same code:

```c
return yang_error_wrap(ERROR_SOCKET, "connect failed");
```

This is the preferred way to attach context to an error before returning.

---

## Common Mistakes

- Returning `0`/`NULL` silently on failure without logging or an error code.
- Mixing boolean return types (`yangbool`) with error-code semantics.
- Ignoring return values from socket, SSL, or capture APIs.

---

## Thread Safety

Error logging is implemented in C (`YangCLog.c`) and uses internal globals. It is safe to call from multiple threads in practice, but avoid high-frequency logging from hot paths because it may contend on the log file lock.
