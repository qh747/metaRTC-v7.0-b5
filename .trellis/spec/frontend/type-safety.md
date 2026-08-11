# Type Safety

> Type safety patterns for the metaRTC Qt frontend.

---

## Overview

The frontend is written in C++11 with Qt6. Type safety is enforced through C++ types and Qt's meta-object system. Unlike TypeScript projects, there are no runtime type guards or JSON schema validators in the frontend layer.

---

## C++ Types

- Use fixed-width integer types from `<cstdint>`: `int32_t`, `uint32_t`, `int64_t`.
- Prefer `yangbool` (alias for `int32_t`) when interoperating with backend C APIs.
- Use `nullptr` instead of `NULL` or `0`.

```cpp
int32_t ret = yang_do_something();
if (ret != Yang_Ok) {
    return;
}
```

---

## Qt Meta-Object Types

- `QObject`-derived classes must declare `Q_OBJECT`.
- Use `signals:` and `slots:` sections for Qt signal/slot declarations.
- Use `QMetaType::registerType` if passing custom types through queued connections.

---

## JSON and Signaling

JSON payloads are handled by backend C code (`YangJson`) before reaching the UI. The frontend does not typically parse raw JSON. If you need to parse JSON in the frontend, use `QJsonDocument` / `QJsonObject`.

---

## Casting Rules

- Use `qobject_cast` for casting `QObject` pointers, especially in signal/slot contexts.
- Avoid `reinterpret_cast` unless interfacing with C APIs.
- Avoid C-style casts.

```cpp
YangPlayWidget* widget = qobject_cast<YangPlayWidget*>(sender);
if (!widget) return;
```

---

## Common Mistakes

- Passing raw pointers to backend objects without clear ownership.
- Using `std::vector` or `std::string` where Qt containers (`QVector`, `QString`) are more idiomatic.
- Mixing `NULL`, `0`, and `nullptr` inconsistently.

---

## Best Practices

- Prefer `QString` for UI strings; convert to `std::string` or `char*` only at backend boundaries.
- Use `const` correctness for methods that do not mutate state.
- Document ownership transfers in function signatures and comments.
