# Quality Guidelines

> Code quality standards for the metaRTC Qt frontend.

---

## Overview

Qt demos should be simple coordinators between UI widgets and the backend SDK. Keep business logic in backend libraries; keep UI code focused on presentation and user interaction.

---

## Forbidden Patterns

- **Business logic in UI files**: Do not implement encoding, networking, or protocol logic in `.cpp` files under `demo/`.
- **Direct backend API calls from UI thread**: Long-running backend calls should be moved to worker threads.
- **Blocking the main thread**: Avoid `QThread::sleep` or synchronous waits on the UI thread.
- **Hard-coded platform paths**: Use `QStandardPaths` or configuration files instead.

---

## Required Patterns

- **Worker threads for heavy work**: Capture, encode, and publish run on `YangRecordThread` or similar threads.
- **Signal/slot for UI updates**: Update widgets from the main thread via signals.
- **Resource cleanup in `closeEvent`**: Stop threads and release backend objects before the window closes.
- **`.ui` files for layouts**: Use Qt Designer forms for static UI layouts.

---

## Example: Main Window Cleanup

```cpp
void RecordMainWindow::closeEvent(QCloseEvent* event) {
    closeAll();  // stop threads, release backend
    event->accept();
}
```

---

## Testing

Frontend testing is primarily manual:

1. Build the demo with CMake.
2. Run it on the target platform.
3. Verify video preview, start/stop streaming, and clean shutdown.

There are no automated UI tests. Keep demos simple enough to verify by inspection.

---

## Code Review Checklist

- [ ] Does the demo build with Qt6?
- [ ] Are backend operations running on worker threads?
- [ ] Are UI updates performed on the main thread?
- [ ] Is cleanup handled in `closeEvent` or destructor?
- [ ] Are `.ui` files used for static layouts?
- [ ] No backend protocol logic leaked into UI code?
