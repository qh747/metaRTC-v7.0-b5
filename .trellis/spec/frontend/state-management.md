# State Management

> How application state is managed in the metaRTC Qt frontend.

---

## Overview

State is managed through a mix of:

- **`YangContext`** configuration structs passed from UI to backend
- **Backend-owned runtime state** (capture running, connection status, etc.)
- **UI widget state** (button enabled/disabled, preview visible, etc.)

There is no centralized global store like Redux. State ownership is split between the backend SDK and the main window.

---

## Configuration State

Demo apps create a `YangContext` (or equivalent configuration struct) and pass it to factories. Example from `demo/metapushstream7/main.cpp`:

```cpp
YangSysMessageHandle* sys = YangPushFactory::CreatePushMessageHandle(
    win.m_hasAudio, win.m_videoType, &win.m_screenInfo,
    &win.m_outInfo, win.m_context, &win, &win);
```

Configuration values are typically read from `yang_config.ini` and stored as public members on the main window.

---

## Runtime State

Backend objects own runtime state:

- `YangSysMessageHandle` owns the message loop state.
- `YangRecordThread` owns capture thread state.
- `YangPushCapture` / `YangPushPublish` own stream publish state.

The UI observes runtime state through:

- `YangSysMessageHandleI::receiveSysMessage`
- `YangSysMessageI::success` / `failure`
- Direct polling of backend flags (avoid if possible)

---

## UI State

Main window keeps lightweight UI state:

- Whether recording is active
- Which server type is selected (WHIP, Janus, etc.)
- Preview widget visibility

Update UI state only on the main thread.

---

## Common Mistakes

- Duplicating backend state in the UI and letting them drift.
- Modifying `YangContext` after passing it to a backend object.
- Accessing backend state from the UI thread without synchronization.

---

## Best Practices

- Treat `YangContext` as read-only after backend initialization.
- Drive UI state from backend callbacks, not from polling.
- Keep the main window as a thin coordinator between UI widgets and backend handlers.
