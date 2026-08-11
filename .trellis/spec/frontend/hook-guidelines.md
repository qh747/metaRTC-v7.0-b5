# Hook Guidelines

> How reusable UI logic and callbacks are organized in the metaRTC Qt frontend.

---

## Overview

Qt does not use React-style hooks. Instead, reusable UI logic is organized through:

- **C++ interfaces** (`YangSysMessageHandleI`, `YangSysMessageI`)
- **Qt signals/slots**
- **Factory classes** (`YangPushFactory`, `YangPlayFactory`)
- **Worker threads** (`YangRecordThread`)

This document documents the equivalent patterns for "hooks" in this codebase.

---

## Interface-Based Callbacks

Backend-to-frontend communication uses pure C++ interfaces. For example, `RecordMainWindow` implements `YangSysMessageHandleI` to receive system messages:

```cpp
class RecordMainWindow : public QMainWindow, public YangSysMessageHandleI {
public:
    virtual void receiveSysMessage(YangSysMessage* psm, int32_t phandleRet);
};
```

Keep interfaces small and focused on a single responsibility.

---

## Qt Signals and Slots

Use signals/slots for cross-thread communication and decoupled UI updates:

```cpp
// In worker thread
void YangRecordThread::captureFrame() {
    // ...
    emit frameReady(frame);
}

// In widget
connect(recordThread, &YangRecordThread::frameReady,
        playWidget, &YangPlayWidget::updateFrame);
```

- Use `Qt::QueuedConnection` when sender and receiver live in different threads.
- Avoid blocking the UI thread with synchronous signal emissions.

---

## Factories

Factories abstract backend creation. Prefer them over direct `new` in UI code:

```cpp
YangSysMessageHandle* sys = YangPushFactory::CreatePushMessageHandle(
    win.m_hasAudio, win.m_videoType, &win.m_screenInfo,
    &win.m_outInfo, win.m_context, &win, &win);
```

---

## Common Mistakes

- Mixing backend C callbacks with Qt UI code without a clear adapter.
- Calling backend methods that block from the UI thread.
- Creating worker objects on the UI thread but forgetting to move them to a worker thread.

---

## Best Practices

- Encapsulate each major feature (capture, encode, play, publish) in its own worker thread or handler class.
- Use interfaces for backend-to-frontend notifications.
- Use signals/slots for frontend-to-widget updates.
