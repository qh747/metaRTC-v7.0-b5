# Directory Structure

> How frontend/demo code is organized in metaRTC.

---

## Overview

metaRTC is primarily an SDK, not a web application. The "frontend" layer consists of **Qt-based desktop demos** and platform-specific demo apps (Android, Flutter, LVGL). This document focuses on the Qt desktop demos, which are the canonical reference frontend.

---

## Demo Layout

```
demo/
├── metapushstream7/          # Qt6 desktop push/stream demo
├── metaplayer7/              # Qt6 desktop player demo
├── metaplayer7_lvgl/         # LVGL embedded demo
├── metaplayer7_flutter/      # Flutter cross-platform demo
├── metapushstream7_android/  # Android native demo
└── metaplayer7_android/      # Android native player demo
```

Each demo is self-contained and links against `libmetartc7` / `libmetartccore7`.

---

## Qt Demo Structure (`demo/metapushstream7/`)

```
demo/metapushstream7/
├── main.cpp                  # QApplication entry point
├── recordmainwindow.h/.cpp   # Main QMainWindow
├── recordmainwindow.ui       # Qt Designer form
├── yangjanus.h/.cpp/.ui      # Janus-specific UI dialog
├── video/                    # Video capture/render widgets
│   ├── YangPlayWidget.h/.cpp
│   ├── YangYuvPlayWidget.h/.cpp
│   ├── yangrecordthread.h/.cpp
│   └── yangvideotype.h
└── yangpush/                 # Push stream integration
    ├── YangPushFactory.cpp
    ├── YangPushHandleImpl.h/.cpp
    ├── YangPushMessageHandle.h/.cpp
    └── ...
```

---

## Naming Conventions

- **Main window classes**: `<Feature>MainWindow` (e.g., `RecordMainWindow`).
- **Widget classes**: `Yang<Feature>Widget` (e.g., `YangPlayWidget`).
- **Thread classes**: `Yang<Feature>Thread` (e.g., `YangRecordThread`).
- **UI files**: Lowercase matching the class name (`recordmainwindow.ui`).
- **Member variables**: `m_<camelCase>` consistent with backend code.

---

## Adding a New Demo

1. Create a new directory under `demo/`.
2. Add a `CMakeLists.txt` that links against Qt6 and the metaRTC libraries.
3. Add `main.cpp` with `QApplication` setup.
4. Implement a main window class and any custom widgets.
5. Reuse existing `YangPlayWidget`, `YangRecordThread`, etc., where possible.

---

## Examples

- Typical Qt demo entry: `demo/metapushstream7/main.cpp`
- Main window + UI form: `demo/metapushstream7/recordmainwindow.h`, `.cpp`, `.ui`
- OpenGL video widget: `demo/metapushstream7/video/YangPlayWidget.h`
