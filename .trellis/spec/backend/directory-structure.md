# Directory Structure

> How C/C++ backend code is organized in metaRTC.

---

## Overview

metaRTC is a cross-platform real-time audio/video communication SDK written in C and C++11. The repository follows a strict separation between **public headers**, **library implementations**, **demos**, and **third-party dependencies**.

---

## Top-Level Layout

```
metaRTC/
├── include/                  # Public C/C++ headers (mirror of src modules)
├── libmetartccore7/          # Core RTC library (C)
├── libmetartc7/              # C++ wrapper / higher-level library
├── libyangwhip7/             # WHIP/WHEP signaling client
├── codec/                    # Platform-specific codec extensions
├── demo/                     # Qt / Android / Flutter / LVGL demo apps
├── thirdparty/               # Third-party headers (curl, jsoncpp, lws, ffmpeg)
├── cmake_lib_*.sh            # Library build scripts per platform
└── AGENTS.md                 # AI assistant workspace rules
```

---

## Header Organization (`include/`)

Headers are grouped by functional module. Each subdirectory matches the corresponding source directory under `libmetartc7/src/` or `libmetartccore7/src/`.

| Directory | Responsibility | Example |
|-----------|---------------|---------|
| `include/yangutil/` | System utilities, threading, buffers, logging | `YangSysMessageHandle.h`, `YangLog.h` |
| `include/yangavutil/` | Audio/video utility functions | `YangYuvConvert.h`, `YangAudioMix.h` |
| `include/yangcapture/` | Video/audio capture abstraction | `YangVideoCapture.h` |
| `include/yangencoder/` | Audio/video encoders | `YangVideoEncoder.h` |
| `include/yangdecoder/` | Audio/video decoders | `YangVideoDecoder.h` |
| `include/yangrtc/` | WebRTC peer connection | `YangPeerConnection7.h` |
| `include/yangpush/` | Publish stream abstraction | `YangPushFactory.h` |
| `include/yangplayer/` | Play/receive stream abstraction | `YangPlayerBase.h` |
| `include/yangstream/` | Stream management | `YangStreamManager.h` |
| `include/yangssl/` | DTLS/TLS abstraction | `YangSsl.h` |

---

## Source Organization (`libmetartc7/src/`)

`libmetartc7` is the C++ wrapper library. Source files follow the same module names as headers, with platform-specific subdirectories:

```
libmetartc7/src/
├── yangutil/           # Core utilities (threading, buffers, sys messages)
├── yangavutil/         # Audio/video utilities
├── yangcapture/        # Capture implementation
│   ├── linux/
│   ├── win/
│   ├── mac/
│   └── android/
├── yangencoder/        # Encoder implementations
├── yangdecoder/        # Decoder implementations
├── yangstream/         # Stream buffering
├── yangrtc/            # Peer connection wrapper
└── yangpush/           # Publish path
```

`libmetartccore7/src/` contains the lower-level C implementation of the RTC stack (RTP/RTCP/ICE/SDP/DTLS).

---

## Naming Conventions

- **Files**: `Yang<Feature>.h` / `Yang<Feature>.cpp` for C++; `Yang<Feature>.h` / `Yang<Feature>.c` for C.
- **Classes**: `Yang<Name>` (PascalCase).
- **Interfaces**: `Yang<Name>I` (e.g., `YangSysMessageHandleI`).
- **Factories**: `Yang<Name>Factory` (e.g., `YangPushFactory`).
- **Member variables**: `m_<camelCase>` (e.g., `m_context`, `m_isStart`).
- **Global/static variables**: often prefixed with `g_` or module prefix.
- **Type aliases**: `yangbool` is `int32_t`; `yangtrue`/`yangfalse` are `1`/`0`.

---

## Adding a New Module

1. Create the public header under `include/yang<module>/Yang<Feature>.h`.
2. Create the implementation under `libmetartc7/src/yang<module>/Yang<Feature>.cpp`.
3. If platform-specific code is needed, create subdirectories (`linux/`, `win/`, `mac/`, `android/`) and guard with `Yang_OS_*` macros.
4. Update the library's `CMakeLists.txt` (or `.pro` for Qt Creator) to include the new source directory via `aux_source_directory`.

---

## Examples

- Well-organized utility: `include/yangutil/sys/YangSysMessageHandle.h` + `libmetartc7/src/yangutil/YangSysMessageHandle.cpp`
- Platform-specific capture: `libmetartc7/src/yangcapture/linux/YangVideoCaptureLinux.cpp`
