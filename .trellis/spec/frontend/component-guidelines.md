# Component Guidelines

> How UI components are built in the metaRTC Qt frontend.

---

## Overview

Desktop demos use **Qt Widgets** (not QML). Components are standard C++ classes inheriting from `QWidget`, `QMainWindow`, `QOpenGLWidget`, or `QThread`. Layouts are typically defined in `.ui` files created with Qt Designer.

---

## Component Types

| Type | Base Class | Example | Responsibility |
|------|-----------|---------|----------------|
| Main window | `QMainWindow` | `RecordMainWindow` | Top-level application window, menu, status |
| Video widget | `QOpenGLWidget` | `YangPlayWidget` | Render YUV/RGB video frames via OpenGL |
| YUV widget | `QWidget` | `YangYuvPlayWidget` | Software YUV rendering fallback |
| Worker thread | `QThread` / custom | `YangRecordThread` | Run capture/encode loop off the UI thread |
| Dialog | `QDialog` | `YangJanus` | Configuration or signaling dialog |

---

## Qt Widget Rules

- Always declare `Q_OBJECT` in classes that use signals/slots.
- Prefer `.ui` files for static layouts; dynamic layouts can be built in code.
- Connect Qt signals/slots explicitly; avoid `autoConnect` naming magic when possible.
- Override `closeEvent` to clean up threads and resources before shutdown.

Example:

```cpp
class RecordMainWindow : public QMainWindow, public YangSysMessageHandleI {
    Q_OBJECT
public:
    RecordMainWindow(QWidget* parent = nullptr);
    ~RecordMainWindow();
    void receiveSysMessage(YangSysMessage* psm, int32_t phandleRet);

private slots:
    void on_m_b_rec_clicked();

private:
    void closeEvent(QCloseEvent* event);
    Ui::RecordMainWindow* ui;
};
```

---

## Video Widgets

- `YangPlayWidget` uses OpenGL for GPU-accelerated rendering.
- `YangYuvPlayWidget` is used on platforms where OpenGL setup differs (e.g., macOS).
- Widgets should not block the UI thread; frame updates are typically triggered from a worker thread via signal/slot or direct method invocation with proper synchronization.

---

## Common Mistakes

- Creating `QObject`-derived classes on non-Qt threads without proper parent.
- Calling GUI methods directly from capture/encode worker threads.
- Forgetting to call `QObject::moveToThread` for worker objects.
- Not handling `QCloseEvent` to stop background threads.

---

## Platform Notes

- macOS requires Core Profile OpenGL setup in `main.cpp` before `QApplication`.
- Qt6 is preferred; Qt5 compatibility code should be guarded with `QT_VERSION_CHECK`.
