# Research: V4L2 Video Capture Bug Analysis (YangVideoCaptureLinux)

- **Query**: Analyze V4L2 video capture code for all bugs/issues causing startup failure of ./metapushstream7
- **Scope**: internal
- **Date**: 2026-08-10

## Context

Config (`bin/app_debug/yang_config.ini`): width=1280, height=720, frame=30, videoCaptureFormat=1 (I420), vIndex=0

Camera capabilities (from runtime output):
- MJPG 1280x720 @ 30fps
- YUYV 1280x720 @ 10fps
- Does NOT support I420 directly

Failure output:
```
[02:46:47] Yang ERROR: set fmt error!                          -> S_FMT failed (line 273)
[02:46:53] Yang ERROR: ..........................set video frame error!  -> S_PARM failed (line 289)
[02:46:58] Yang ERROR: VIDIOC_STREAMON                         -> STREAMON failed (line 409)
[02:46:58] Yang ERROR: VIDIOC_DQBUF                            -> DQBUF failed (line 340)
QObject::killTimer: Timers cannot be stopped from another thread  -> exit(1) from capture thread (line 341)
```

Key files:
- [YangVideoCaptureLinux.cpp](file:///home/quhan/01_myProject/metaRTC/libmetartc7/src/yangcapture/linux/YangVideoCaptureLinux.cpp)
- [YangVideoCaptureLinux.h](file:///home/quhan/01_myProject/metaRTC/libmetartc7/src/yangcapture/linux/YangVideoCaptureLinux.h)
- [YangVideoCapture.cpp](file:///home/quhan/01_myProject/metaRTC/libmetartc7/src/yangcapture/YangVideoCapture.cpp) (thread base class)
- [YangThread2.h](file:///home/quhan/01_myProject/metaRTC/include/yangutil/sys/YangThread2.h) (thread primitive)
- [yangavinfo.h:110-119](file:///home/quhan/01_myProject/metaRTC/include/yangutil/yangavinfo.h#L110-L119) (YangYuvType enum: YangYuy2=0, YangI420=1, ...)

---

## Findings

### Issue Summary Table

| # | Location | Problem | Severity |
|---|---|---|---|
| 1 | cpp:272-275 | S_FMT failure only logged, not returned -> cascading failure | **root cause** |
| 2 | cpp:224-261 | init() ignores config's videoCaptureFormat, overwrites it | **root cause** |
| 3 | cpp:120-129 | MJPG not handled by setReselution(), forced to YUYV@10fps | **root cause** |
| 4 | cpp:285-289 | S_PARM requests 30fps but YUYV only supports 10fps | contributing |
| 5 | cpp:302-304 | REQBUFS error silently ignored (empty if-block) | contributing |
| 6 | cpp:317-318 | QUERYBUF failure prints "search!" and continues | contributing |
| 7 | cpp:408-435 | STREAMON failure logged but loop continues -> DQBUF fails | contributing |
| 8 | cpp:341 | exit(1) called from capture thread -> Qt timer cleanup error | contributing |
| 9 | cpp:307-310 | m_user_buffer==NULL check is dead code (fixed array) | minor |
| 10 | cpp:131 | v4l2_fmtdesc fmt not memset in setReselutionPara | minor (latent) |
| 11 | cpp:177 | v4l2_frmsizeenum frmsize not memset in setPara | minor |
| 12 | cpp:136 | v4l2_frmsizeenum frmsize not memset in setReselutionPara | minor (latent) |
| 13 | cpp:412-423 | fd_set not re-initialized inside select() loop | minor |
| 14 | cpp:309,325,341,358,373,382,390,428,432 | 9 exit() calls, 3 in destructor path (UB) | minor |

---

### Detailed Analysis

#### Issue 1 — S_FMT failure not propagated (ROOT CAUSE)

**File**: [YangVideoCaptureLinux.cpp:272-275](file:///home/quhan/01_myProject/metaRTC/libmetartc7/src/yangcapture/linux/YangVideoCaptureLinux.cpp#L272-L275)

```cpp
if ((ioctl(m_vd_id, VIDIOC_S_FMT, &v4_format)) != 0) {
    yang_error("\n set fmt error!");
    // NO return! Execution continues with an unconfigured format.
}
```

`v4_format` IS properly `memset`'d at line 264 — this is NOT the bug. The bug is that when `VIDIOC_S_FMT` fails (EINVAL, EBUSY, etc.), the code merely logs and falls through. All subsequent operations (`S_PARM`, `REQBUFS`, `QUERYBUF`, `mmap`, `STREAMON`, `DQBUF`) execute against a device whose format was never negotiated, producing the cascade of errors observed in the log. The function returns `Yang_OK` at line 328 even though format negotiation failed.

**Severity**: root cause — this is the primary code-level defect that turns a recoverable driver error into a fatal startup failure. The S_FMT failure itself may be driver-specific, but the code's response guarantees total failure.

---

#### Issue 2 — init() ignores config's videoCaptureFormat (ROOT CAUSE)

**File**: [YangVideoCaptureLinux.cpp:224-261](file:///home/quhan/01_myProject/metaRTC/libmetartc7/src/yangcapture/linux/YangVideoCaptureLinux.cpp#L224-L261)

Config sets `videoCaptureFormat=1` (= `YangI420`, loaded at [YangIni.c:160](file:///home/quhan/01_myProject/metaRTC/libmetartccore7/src/yangutil/sys/YangIni.c#L160)). The `init()` function **never reads** `m_para->videoCaptureFormat`. Instead it unconditionally starts with:

```cpp
uint32_t format = V4L2_PIX_FMT_YUYV;                    // line 224 — default, ignores config
if(m_hasYuy2>1||m_hasI420>1||m_hasNv12>1||m_hasYv12>1){  // line 225 — based on camera caps only
    if(m_hasI420>1) { format = V4L2_PIX_FMT_YUV420; ... }  // I420 branch
    ...
    else if(m_hasYuy2>1){ format = V4L2_PIX_FMT_YUYV; m_para->videoCaptureFormat=YangYuy2; }
}
```

The `m_has*` flags are set by `setReselution()` (line 120-129) which is called during `setPara()` enumeration. Values:
- `0` = format not supported by camera
- `1` = format supported (but not at the configured resolution)
- `2` = format supported at the exact configured resolution

Since the camera does NOT support I420 at all, `m_hasI420` stays `0`. Since it supports YUYV at 1280x720, `m_hasYuy2=2`. The code therefore takes the `m_hasYuy2>1` branch at line 235, setting `format=YUYV` and **overwriting** `m_para->videoCaptureFormat` to `YangYuy2`.

The user's config value (`YangI420`) is silently discarded. This is a design defect: config and code logic are disconnected.

**Severity**: root cause — the config-to-code contract is broken. While this particular camera doesn't support I420 (so the override is "accidentally correct" in choosing a supported format), the fundamental issue is that the user's intent is ignored and there is no fallback to MJPG.

---

#### Issue 3 — MJPG format not handled (ROOT CAUSE)

**File**: [YangVideoCaptureLinux.cpp:120-129](file:///home/quhan/01_myProject/metaRTC/libmetartc7/src/yangcapture/linux/YangVideoCaptureLinux.cpp#L120-L129)

```cpp
void YangVideoCaptureLinux::setReselution(__u32 format, int32_t val){
    if (format == V4L2_PIX_FMT_YUYV)  m_hasYuy2 = val;
    if (format == V4L2_PIX_FMT_YUV420) m_hasI420 = val;
    if (format == V4L2_PIX_FMT_NV12)  m_hasNv12 = val;
    if (format == V4L2_PIX_FMT_YVU420) m_hasYv12 = val;
    // V4L2_PIX_FMT_MJPEG not handled!
}
```

The camera's best option for 1280x720@30fps is MJPG, but `setReselution()` has no case for `V4L2_PIX_FMT_MJPEG`. The enumeration in `setPara()` (line 169-199) does detect MJPG (visible in the log output `{ pixelformat = ''MJPG'' ... }`), but since `setReselution()` ignores it, `m_hasMjpg` doesn't exist and MJPG is never a candidate in `init()`'s selection logic.

Result: the code is forced to pick YUYV, which only supports 10fps at this resolution — directly causing the S_PARM failure (Issue 4) and the 30fps-vs-10fps mismatch.

**Severity**: root cause — the absence of MJPG support is the architectural reason the code cannot use the camera's optimal format, forcing an incompatible YUYV+30fps combination.

---

#### Issue 4 — S_PARM requests unsupported frame rate

**File**: [YangVideoCaptureLinux.cpp:281-290](file:///home/quhan/01_myProject/metaRTC/libmetartc7/src/yangcapture/linux/YangVideoCaptureLinux.cpp#L281-L290)

```cpp
struct v4l2_streamparm Stream_Parm;
memset(&Stream_Parm, 0, sizeof(struct v4l2_streamparm));   // properly zeroed
Stream_Parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
Stream_Parm.parm.capture.timeperframe.denominator = m_para->frame;  // 30
Stream_Parm.parm.capture.timeperframe.numerator = 1;

if (ioctl(m_vd_id, VIDIOC_S_PARM, &Stream_Parm)) {
    yang_error("\n..........................set video frame error!");  // no return
}
```

`Stream_Parm` is properly `memset`'d — not a struct-init bug. The problem is semantic: `m_para->frame=30` but YUYV 1280x720 only supports 10fps (per camera enumeration). The UVC driver rejects 30fps for YUYV, producing `set video frame error!`. Like S_FMT, the error is logged but not returned.

This is a direct consequence of Issue 3 (no MJPG) — if MJPG were used, 30fps would be supported.

**Severity**: contributing — non-fatal in isolation (S_PARM failure doesn't block subsequent ioctls), but confirms the format/rate mismatch.

---

#### Issue 5 — REQBUFS error silently ignored

**File**: [YangVideoCaptureLinux.cpp:302-305](file:///home/quhan/01_myProject/metaRTC/libmetartc7/src/yangcapture/linux/YangVideoCaptureLinux.cpp#L302-L305)

```cpp
if (ioctl(m_vd_id, VIDIOC_REQBUFS, &tV4L2_reqbuf)) {
    // empty body — error completely swallowed
}
m_buffer_count = tV4L2_reqbuf.count;   // may be 0 or stale on failure
```

If `VIDIOC_REQBUFS` fails (which is likely after S_FMT failed), `tV4L2_reqbuf.count` may be 0 or unchanged. The code then proceeds to `mmap` buffers that were never allocated. If `count=0`, the `for` loop at line 311 is skipped, `m_buffer_count=0`, and `startLoop()` skips the QBUF loop and goes straight to STREAMON (which also fails).

**Severity**: contributing — masks the real failure and allows execution to continue in an invalid state.

---

#### Issue 6 — QUERYBUF error handling inadequate

**File**: [YangVideoCaptureLinux.cpp:317-318](file:///home/quhan/01_myProject/metaRTC/libmetartc7/src/yangcapture/linux/YangVideoCaptureLinux.cpp#L317-L318)

```cpp
if (ioctl(m_vd_id, VIDIOC_QUERYBUF, &tV4L2buf))
    printf("search!");    // debug print, no return, no skip
m_user_buffer[i].length = tV4L2buf.length;   // uses potentially garbage length
```

On `QUERYBUF` failure, the code prints the unhelpful string `"search!"` and continues to use `tV4L2buf.length` (which may contain garbage from the failed ioctl) for `mmap`. This can cause `mmap` to fail or map invalid memory.

**Severity**: contributing — poor error recovery in buffer setup.

---

#### Issue 7 — STREAMON failure does not abort the loop

**File**: [YangVideoCaptureLinux.cpp:407-435](file:///home/quhan/01_myProject/metaRTC/libmetartc7/src/yangcapture/linux/YangVideoCaptureLinux.cpp#L407-L435)

```cpp
enum v4l2_buf_type v4l2type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
if (ioctl(m_vd_id, VIDIOC_STREAMON, &v4l2type)) {
    yang_error("VIDIOC_STREAMON");    // logged, NOT returned
}
// ... falls through to select()/read_buffer() loop
while (m_isloop) {
    r = select(m_vd_id + 1, &fds, NULL, NULL, &tv);
    ...
    read_buffer();    // calls DQBUF -> fails -> exit(1)
}
```

When `STREAMON` fails (because S_FMT/REQBUFS never succeeded), the code enters the capture loop anyway. `select()` may return (fd in error state), `read_buffer()` calls `VIDIOC_DQBUF` which fails because streaming was never started, triggering `exit(1)` at line 341.

**Severity**: contributing — directly leads to the fatal `exit(1)` that kills the process.

---

#### Issue 8 — exit(1) from capture thread causes Qt timer error

**File**: [YangVideoCaptureLinux.cpp:341](file:///home/quhan/01_myProject/metaRTC/libmetartc7/src/yangcapture/linux/YangVideoCaptureLinux.cpp#L341)

```cpp
int32_t YangVideoCaptureLinux::read_buffer() {
    if (ioctl(m_vd_id, VIDIOC_DQBUF, &m_buf) != 0) {
        yang_error("VIDIOC_DQBUF");
        exit(1);    // <--- called from capture thread
    }
    ...
}
```

Threading model: `YangVideoCaptureLinux` extends `YangMultiVideoCapture` → `YangVideoCapture` → `YangThread` ([YangThread2.h:12](file:///home/quhan/01_myProject/metaRTC/include/yangutil/sys/YangThread2.h#L12)). `YangThread::start()` spawns a pthread that calls `run()` ([YangVideoCapture.cpp:16-20](file:///home/quhan/01_myProject/metaRTC/libmetartc7/src/yangcapture/YangVideoCapture.cpp#L16-L20)), which calls `startLoop()`. So `read_buffer()` executes in the **capture thread**, not the main/Qt thread.

`exit(1)` terminates the entire process. Qt's atexit handlers attempt to destroy `QTimer`/`QObject` instances, but `QObject` timers must be stopped from the thread that owns them. Stopping them from the capture thread triggers:
```
QObject::killTimer: Timers cannot be stopped from another thread
```

**Severity**: contributing — this is the source of the Qt warning. The fix should use `stopLoop()` + thread join instead of `exit()`.

---

#### Issue 9 — m_user_buffer==NULL check is dead code

**File**: [YangVideoCaptureLinux.cpp:307-310](file:///home/quhan/01_myProject/metaRTC/libmetartc7/src/yangcapture/linux/YangVideoCaptureLinux.cpp#L307-L310)

```cpp
if (m_user_buffer == NULL) {
    yang_error("calloc Error");
    exit(1);
}
```

Header declaration ([YangVideoCaptureLinux.h:62](file:///home/quhan/01_myProject/metaRTC/libmetartc7/src/yangcapture/linux/YangVideoCaptureLinux.h#L62)):
```cpp
buffer_type m_user_buffer[REQ_BUF_NUM];   // fixed array, NOT a pointer
```

`m_user_buffer` is a fixed-size array member (`REQ_BUF_NUM=4`). In the expression `m_user_buffer == NULL`, the array decays to a pointer to its first element — this address is the object's own memory and can **never** be `NULL` (as long as the object exists). The condition is always false; the `exit(1)` is unreachable dead code.

The comment `"calloc Error"` suggests this check was written for a dynamically-allocated buffer (via `calloc`/`malloc`) and was never updated when the implementation switched to a fixed array. There is no `calloc`/`malloc` for `m_user_buffer` anywhere in the codebase (confirmed via grep).

Note: the check does not protect against `tV4L2_reqbuf.count > REQ_BUF_NUM` either (would cause out-of-bounds write at lines 320-323), though UVC drivers typically never return more buffers than requested.

**Severity**: minor — dead code, no runtime effect, but misleading.

---

#### Issue 10 — v4l2_fmtdesc not memset in setReselutionPara

**File**: [YangVideoCaptureLinux.cpp:131](file:///home/quhan/01_myProject/metaRTC/libmetartc7/src/yangcapture/linux/YangVideoCaptureLinux.cpp#L131)

```cpp
void YangVideoCaptureLinux::setReselutionPara(__u32 pformat){
    struct v4l2_fmtdesc fmt;     // NOT memset'd!
    int32_t vet=0;
    while ((vet = ioctl(m_vd_id, VIDIOC_ENUM_FMT, &fmt)) != -1) {
        fmt.index++;            // fmt.index was garbage on first iteration
        ...
    }
}
```

`fmt` is not initialized. `fmt.index` and `fmt.type` contain stack garbage. For `VIDIOC_ENUM_FMT`, the kernel requires `fmt.type` to be set to `V4L2_BUF_TYPE_VIDEO_CAPTURE` and `fmt.index` to start at 0. With garbage values:
- If `fmt.type` is invalid, the ioctl returns EINVAL immediately (loop doesn't execute).
- If `fmt.index` is non-zero, enumeration starts from a random offset.

Compare with the correct usage in `setPara()` at [line 155-158](file:///home/quhan/01_myProject/metaRTC/libmetartc7/src/yangcapture/linux/YangVideoCaptureLinux.cpp#L155-L158) where `memset` + `fmt.type` are properly set.

This function is only called in the `else if` branch at line 258 (when a format is supported but NOT at the configured resolution). In the current failing scenario, `m_hasYuy2=2` (supported at 1280x720), so the `>1` branch is taken and `setReselutionPara` is NOT called. This is a latent bug that would manifest if the configured resolution didn't match any camera-supported resolution.

**Severity**: minor (latent) — not triggered in current scenario, but would cause incorrect behavior if reached.

---

#### Issue 11 — v4l2_frmsizeenum not memset in setPara

**File**: [YangVideoCaptureLinux.cpp:177-179](file:///home/quhan/01_myProject/metaRTC/libmetartc7/src/yangcapture/linux/YangVideoCaptureLinux.cpp#L177-L179)

```cpp
struct v4l2_frmsizeenum frmsize;                    // NOT memset'd!
frmsize.pixel_format = fmt.pixelformat;             // only these two set
frmsize.index = 0;
while (!ioctl(m_vd_id, VIDIOC_ENUM_FRAMESIZES, &frmsize)) {
```

`frmsize` is not fully initialized. `pixel_format` and `index` (the input fields) are set explicitly, but `type` and other fields contain garbage. Per V4L2 API, `VIDIOC_ENUM_FRAMESIZES` uses only `pixel_format` and `index` as input, so this works in practice on most drivers. However, not zeroing the struct is poor practice and some quirky drivers may read reserved fields.

This function IS called in the current scenario (during `setPara()`), and the enumeration succeeds (visible in the log). So this is not the cause of failure, but it is a code quality issue.

**Severity**: minor — works in practice, but fragile.

---

#### Issue 12 — v4l2_frmsizeenum not memset in setReselutionPara

**File**: [YangVideoCaptureLinux.cpp:136-138](file:///home/quhan/01_myProject/metaRTC/libmetartc7/src/yangcapture/linux/YangVideoCaptureLinux.cpp#L136-L138)

Same issue as #11 but in `setReselutionPara`. Latent (function not called in current scenario, see Issue 10).

**Severity**: minor (latent).

---

#### Issue 13 — fd_set not re-initialized inside select() loop

**File**: [YangVideoCaptureLinux.cpp:412-423](file:///home/quhan/01_myProject/metaRTC/libmetartc7/src/yangcapture/linux/YangVideoCaptureLinux.cpp#L412-L423)

```cpp
fd_set fds;
struct timeval tv;
int32_t r;
FD_ZERO(&fds);          // called ONCE, outside loop
FD_SET(m_vd_id, &fds);
m_isloop = 1;
...
while (m_isloop) {
    tv.tv_sec = 2;       // tv IS re-initialized inside loop
    tv.tv_usec = 0;
    r = select(m_vd_id + 1, &fds, NULL, NULL, &tv);   // fds NOT re-initialized!
    ...
}
```

`select()` modifies `fds` to indicate which descriptors are ready. After the first `select()` call, `fds` may have `m_vd_id` cleared. Subsequent iterations call `select()` with a potentially empty set, causing immediate timeout (return 0) -> `exit(EXIT_FAILURE)` at line 432.

The `tv` struct IS correctly re-initialized inside the loop, but `fds` is not. `FD_ZERO` + `FD_SET` should be inside the `while` loop, before each `select()` call.

In the current failure scenario, the loop exits via `exit(1)` at line 341 (DQBUF failure) on the first iteration, so this bug doesn't manifest. But in a working configuration, it would cause capture to fail after the first frame.

**Severity**: minor — not triggered in current failure, but would break capture even if other issues were fixed.

---

#### Issue 14 — 9 exit() calls, including in destructor path

**File**: [YangVideoCaptureLinux.cpp](file:///home/quhan/01_myProject/metaRTC/libmetartc7/src/yangcapture/linux/YangVideoCaptureLinux.cpp)

All `exit()` locations:

| Line | Context | Function | Thread |
|------|---------|----------|--------|
| 309 | m_user_buffer==NULL (dead code) | init() | main |
| 325 | mmap failed | init() | main |
| 341 | DQBUF failed | read_buffer() | **capture** |
| 358 | QBUF failed | read_buffer() | **capture** |
| 373 | STREAMOFF failed | stop_capturing() | varies (destructor) |
| 382 | munmap failed | uninit_camer_device() | varies (destructor) |
| 390 | close failed | close_camer_device() | varies (destructor) |
| 428 | select error | startLoop() | **capture** |
| 432 | select timeout | startLoop() | **capture** |

Total: **9 exit() calls**.

The destructor ([line 41-52](file:///home/quhan/01_myProject/metaRTC/libmetartc7/src/yangcapture/linux/YangVideoCaptureLinux.cpp#L41-L52)) calls `stop_capturing()`, `uninit_camer_device()`, `close_camer_device()` — if any of these fail, `exit(EXIT_FAILURE)` is called during destruction. Calling `exit()` from a destructor is undefined behavior (C++ standard) because it skips the rest of the stack unwinding and may re-enter destroyed objects.

The currently-triggered `exit(1)` is at **line 341** (DQBUF failure in `read_buffer()`, called from `startLoop()` running in the capture pthread). This is what produces the `QObject::killTimer` warning.

**Severity**: minor (design) — the `exit()` calls are a pervasive anti-pattern. The immediate symptom (Qt timer warning) comes from line 341, but all 9 should be replaced with error returns / `stopLoop()`.

---

### memset Audit of All v4l2 Structs

| Struct | Location | memset'd? | Notes |
|--------|----------|-----------|-------|
| `v4l2_frmivalenum frmival` | line 99 | YES (line 100) | OK. `type` commented out (line 104) but it's an output field. |
| `v4l2_fmtdesc fmt` (setReselutionPara) | line 131 | **NO** | Issue 10. `fmt.type` and `fmt.index` garbage. Latent. |
| `v4l2_fmtdesc fmt` (setPara) | line 155 | YES (line 156) | OK. `fmt.type` set at line 158. |
| `v4l2_frmsizeenum frmsize` (setPara) | line 177 | **NO** | Issue 11. Works in practice; input fields set explicitly. |
| `v4l2_frmsizeenum frmsize` (setReselutionPara) | line 136 | **NO** | Issue 12. Latent. |
| `v4l2_format v4_format` | line 263 | YES (line 264) | OK. User's hypothesis "is it memset'd?" — YES, it is. |
| `v4l2_streamparm Stream_Parm` | line 281 | YES (line 282) | OK. |
| `v4l2_requestbuffers tV4L2_reqbuf` | line 295 | YES (line 297) | OK. |
| `v4l2_buffer tV4L2buf` (init) | line 312 | YES (line 313) | OK. |
| `v4l2_buffer tV4L2buf` (startLoop) | line 397 | YES (line 398) | OK. |
| `v4l2_buffer m_buf` (member) | constructor | YES (line 31) | OK. `type`/`memory` set at lines 32-33. |
| `v4l2_capability cap` | line 154 | NO | Output-only struct, ioctl fills it. Acceptable. |

**Conclusion on user's question #1**: The `v4l2_format v4_format` at line 263 IS properly memset'd. The struct initialization is NOT the cause of the S_FMT failure. The S_FMT failure is due to driver-level format negotiation (likely USB bandwidth or driver quirk), and the real code bug is that the failure is not propagated (Issue 1).

---

### Failure Cascade (Root Cause Chain)

```
1. Camera supports MJPG@30fps and YUYV@10fps at 1280x720 (no I420)
2. Config requests: I420, 30fps  <-- ignored by code (Issue 2)
3. setReselution() ignores MJPG (Issue 3) -> m_hasMjpg doesn't exist
4. init() selects YUYV (only supported non-MJPG format at 1280x720)
   -> overwrites m_para->videoCaptureFormat to YangYuy2
5. S_FMT(YUYV, 1280x720) fails (driver/bandwidth issue)
   -> logged but NOT returned (Issue 1) -> continues
6. S_PARM(30fps) fails (YUYV only supports 10fps, Issue 4)
   -> logged but NOT returned -> continues
7. REQBUFS fails (format not set) -> silently swallowed (Issue 5)
8. QUERYBUF/mmap loop -> m_buffer_count may be 0
9. startLoop(): QBUF loop skipped, STREAMON fails (Issue 7)
   -> logged but NOT returned -> enters select loop
10. select() returns, DQBUF fails -> exit(1) at line 341 (Issue 8)
11. exit() from capture thread -> Qt atexit cleanup fails
    -> "QObject::killTimer: Timers cannot be stopped from another thread"
```

---

## Caveats / Not Found

1. **Exact driver-level cause of S_FMT failure**: Cannot be determined from static code analysis alone. The struct is properly initialized, the format/resolution IS supported by the camera (per enumeration output). Possible driver-level causes: USB bandwidth negotiation failure, driver requires S_PARM before S_FMT, device busy, or driver quirk. Requires runtime debugging with `strace` or driver logs to confirm. The code-level defect is that the failure is not handled (Issue 1), regardless of the driver-level root cause.

2. **Whether MJPG support would fix S_FMT**: Adding MJPG handling (Issue 3) would change the format selection to MJPG, which supports 30fps. This would likely make S_FMT and S_PARM succeed. However, this requires verifying that downstream code (`YangVideoCaptureHandle::putBuffer`, encoder) can handle MJPG frames — the current code path expects raw YUV (Yuy2/I420/Nv12/Yv12). This was not investigated.

3. **The 5-6 second gaps between error log lines** (02:46:47 -> 02:46:53 -> 02:46:58) are likely driver-level blocking in the ioctl calls (some UVC drivers block for several seconds before returning an error on format negotiation). Not a code bug, but worth noting for debugging timeouts.

4. **`enum_camera_frmival` (line 98-118)**: The `type` field is commented out (line 104), but per V4L2 API `type` is an output field for `VIDIOC_ENUM_FRAMEINTERVALS`, so this is acceptable. The struct IS memset'd. No issue.
