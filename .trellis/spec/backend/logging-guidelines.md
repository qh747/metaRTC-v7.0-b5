# Logging Guidelines

> How logging is done in the metaRTC C/C++ backend.

---

## Overview

Logging is implemented in C and exposed through macros in `include/yangutil/sys/YangLog.h`. The backend uses level-based macros that automatically include file, line, and function information.

---

## Log Levels

Defined in `YangLog.h`:

```c
#define YANG_LOG_FATAL     0
#define YANG_LOG_ERROR     1
#define YANG_LOG_WARNING   2
#define YANG_LOG_INFO      3
#define YANG_LOG_DEBUG     4
#define YANG_LOG_TRACE     5
```

| Level | When to use |
|-------|-------------|
| FATAL | Unrecoverable crash condition |
| ERROR | Failure that affects functionality |
| WARNING | Suspicious or non-fatal issue |
| INFO | Important lifecycle events |
| DEBUG | Diagnostic details during development |
| TRACE | Very verbose entry/exit or state dumps |

The default runtime level is `YANG_LOG_ERROR`. Use `yang_setLogLevel(level)` to increase verbosity for debugging.

---

## Macros

Use these macros instead of calling `yang_clog` directly:

```c
yang_error(fmt, ...)   // ERROR level
yang_warn(fmt, ...)    // WARNING level
yang_info(fmt, ...)    // INFO level
yang_debug(fmt, ...)   // DEBUG level
yang_trace(fmt, ...)   // TRACE level
yang_fatal(fmt, ...)   // FATAL level
```

Each macro prepends `[file:line function]` automatically.

Example:

```c
yang_error("Failed to open video device: %s", deviceName);
yang_debug("Captured frame size: %d", frameSize);
```

---

## Log Files

On desktop platforms, logs can be written to `yang_log.log` in the working directory:

```c
yang_setLogFile(yangtrue, "/path/to/logs");
```

On Android and iOS, logs are routed to the system log via `__android_log_print`.

---

## What to Log

- Stream lifecycle events (start, stop, connect, disconnect)
- Capture/encode/decode failures
- Network errors and timeouts
- Significant state transitions

## What NOT to Log

- Raw video/audio payload data
- Encryption keys or passwords
- High-frequency per-frame data at INFO or higher
- Personal information from users

---

## Common Mistakes

- Leaving temporary `yang_debug` spam in production code.
- Logging at ERROR level for expected conditions (e.g., clean disconnect).
- Forgetting to remove sensitive URLs or tokens from log messages.

---

## Best Practices

- Prefer `yang_error_wrap` when returning an error code to both log and propagate.
- Keep log messages concise and actionable.
- Use `##__VA_ARGS__`-safe format strings; do not pass user-controlled strings as the format argument.
