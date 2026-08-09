# V4L2 视频采集编码规范 (YangVideoCaptureLinux)

> 基于实际 Bug 分析提炼的 V4L2 采集层编码规范。所有规则均有源码或运行时证据支撑。

---

## 1. 背景：启动失败根因

### 1.1 现象

`./metapushstream7` 启动后级联失败：

```
[02:46:47] Yang ERROR: set fmt error!                          -> VIDIOC_S_FMT 失败
[02:46:53] Yang ERROR: ..........................set video frame error!  -> VIDIOC_S_PARM 失败
[02:46:58] Yang ERROR: VIDIOC_STREAMON                         -> VIDIOC_STREAMON 失败
[02:46:58] Yang ERROR: VIDIOC_DQBUF                            -> VIDIOC_DQBUF 失败
QObject::killTimer: Timers cannot be stopped from another thread  -> exit(1) 从采集线程调用
```

### 1.2 运行时根因 (EIO)

通过 `v4l2-ctl` 独立验证，`VIDIOC_S_FMT` 返回 **EIO (Input/output error)**：

```bash
$ v4l2-ctl --device=/dev/video0 --set-fmt-video=width=1280,height=720,pixelformat=YUYV
VIDIOC_S_FMT: failed: Input/output error
```

**EIO 的含义**：UVC 驱动无法为 YUYV (未压缩) 1280x720 分配足够的 USB 等时带宽。YUYV 1280x720@10fps 需要 ~17.6 MB/s 持续带宽，USB 2.0 总线可能无法满足。

**关键对比**：同一摄像头 MJPG 1280x720@30fps 可正常工作（压缩后带宽需求大幅降低），但代码不支持 MJPG 格式。

### 1.3 代码级根因 (3 个)

| # | 位置 | 问题 | 严重程度 |
|---|------|------|----------|
| 1 | [YangVideoCaptureLinux.cpp:120-129](file:///home/quhan/01_myProject/metaRTC/libmetartc7/src/yangcapture/linux/YangVideoCaptureLinux.cpp#L120-L129) | `setReselution()` 不处理 MJPG，强制使用 YUYV | **根因** |
| 2 | [YangVideoCaptureLinux.cpp:224-261](file:///home/quhan/01_myProject/metaRTC/libmetartc7/src/yangcapture/linux/YangVideoCaptureLinux.cpp#L224-L261) | `init()` 忽略配置的 `videoCaptureFormat`，基于摄像头能力覆盖 | **根因** |
| 3 | [YangVideoCaptureLinux.cpp:272-275](file:///home/quhan/01_myProject/metaRTC/libmetartc7/src/yangcapture/linux/YangVideoCaptureLinux.cpp#L272-L275) | S_FMT 失败仅记录日志不返回，级联失败 | **根因** |

### 1.4 失败级联链

```
摄像头支持: MJPG@30fps ✓, YUYV@10fps ✓ (1280x720)
  ↓
配置请求: videoCaptureFormat=I420, frame=30  ← 被代码忽略 (根因 #2)
  ↓
setReselution() 不处理 MJPG (根因 #1) → m_hasMjpg 不存在
  ↓
init() 选择 YUYV (唯一支持的非 MJPG 格式)
  ↓
S_FMT(YUYV, 1280x720) → EIO (USB 带宽不足)
  ↓ 失败未返回 (根因 #3)
S_PARM(30fps) → 失败 (YUYV 仅支持 10fps)
  ↓ 失败未返回
REQBUFS → 失败 → 静默忽略
  ↓
STREAMON → 失败 → 不中止循环
  ↓
DQBUF → 失败 → exit(1) 从采集线程
  ↓
Qt atexit 清理 → "QObject::killTimer: Timers cannot be stopped from another thread"
```

---

## 2. 编码规范

### 2.1 V4L2 结构体必须 memset 清零

**规则**：所有 `v4l2_*` 结构体在传递给 `ioctl()` 前必须 `memset` 清零。

**反例**（当前代码中存在）：

- [YangVideoCaptureLinux.cpp:131](file:///home/quhan/01_myProject/metaRTC/libmetartc7/src/yangcapture/linux/YangVideoCaptureLinux.cpp#L131) — `setReselutionPara()` 中 `v4l2_fmtdesc fmt` 未清零
- [YangVideoCaptureLinux.cpp:177](file:///home/quhan/01_myProject/metaRTC/libmetartc7/src/yangcapture/linux/YangVideoCaptureLinux.cpp#L177) — `setPara()` 中 `v4l2_frmsizeenum frmsize` 未清零
- [YangVideoCaptureLinux.cpp:136](file:///home/quhan/01_myProject/metaRTC/libmetartc7/src/yangcapture/linux/YangVideoCaptureLinux.cpp#L136) — `setReselutionPara()` 中 `v4l2_frmsizeenum frmsize` 未清零

**正例**（当前代码中正确做法）：

```cpp
// YangVideoCaptureLinux.cpp:263-264 — v4l2_format 正确清零
struct v4l2_format v4_format;
memset(&v4_format, 0, sizeof(v4_format));
```

### 2.2 ioctl 失败必须传播，不得仅记录后继续

**规则**：`VIDIOC_S_FMT`、`VIDIOC_REQBUFS`、`VIDIOC_STREAMON` 等关键 ioctl 失败时，必须返回错误码，不得仅记录日志后继续执行。

**反例**（当前代码）：

```cpp
// YangVideoCaptureLinux.cpp:272-275 — S_FMT 失败不返回
if ((ioctl(m_vd_id, VIDIOC_S_FMT, &v4_format)) != 0) {
    yang_error("\n set fmt error!");
    // 缺少 return！后续所有操作在未配置格式的设备上执行
}
```

**正例**：

```cpp
if ((ioctl(m_vd_id, VIDIOC_S_FMT, &v4_format)) != 0) {
    yang_error("VIDIOC_S_FMT failed: %s", strerror(errno));
    return ERROR_SYS_Linux_VideoDeveceOpenFailure;
}
```

### 2.3 禁止从采集线程调用 exit()

**规则**：采集线程 (`startLoop()`/`read_buffer()`) 中禁止调用 `exit()`。应通过 `stopLoop()` + 线程 join 退出。

**原因**：`exit()` 触发 atexit 处理器，Qt 的 `QObject` 析构必须在拥有它的线程执行。从采集线程 `exit()` 会导致：

```
QObject::killTimer: Timers cannot be stopped from another thread
```

**当前 exit() 位置**（共 9 处，需全部移除）：

| 行号 | 函数 | 线程 |
|------|------|------|
| 309 | `init()` | 主线程 |
| 325 | `init()` | 主线程 |
| 341 | `read_buffer()` | **采集线程** |
| 358 | `read_buffer()` | **采集线程** |
| 373 | `stop_capturing()` | 析构 |
| 382 | `uninit_camer_device()` | 析构 |
| 390 | `close_camer_device()` | 析构 |
| 428 | `startLoop()` | **采集线程** |
| 432 | `startLoop()` | **采集线程** |

### 2.4 格式选择应优先 MJPG (高分辨率时)

**规则**：当目标分辨率 ≥ 1280x720 时，应优先选择 MJPG（若摄像头支持），因为未压缩 YUYV 在 USB 2.0 上常因带宽不足返回 EIO。

**当前缺陷**：`setReselution()` ([YangVideoCaptureLinux.cpp:120-129](file:///home/quhan/01_myProject/metaRTC/libmetartc7/src/yangcapture/linux/YangVideoCaptureLinux.cpp#L120-L129)) 不处理 `V4L2_PIX_FMT_MJPEG`，导致 MJPG 永远不会被选为采集格式。

### 2.5 select() 循环内必须重置 fd_set

**规则**：`select()` 会修改 `fd_set`，每次调用前必须重新 `FD_ZERO` + `FD_SET`。

**反例**（[YangVideoCaptureLinux.cpp:414-421](file:///home/quhan/01_myProject/metaRTC/libmetartc7/src/yangcapture/linux/YangVideoCaptureLinux.cpp#L414-L421)）：

```cpp
FD_ZERO(&fds);          // 循环外仅调用一次
FD_SET(m_vd_id, &fds);
while (m_isloop) {
    r = select(m_vd_id + 1, &fds, NULL, NULL, &tv);  // fds 可能已被清空
}
```

**正例**：

```cpp
while (m_isloop) {
    FD_ZERO(&fds);
    FD_SET(m_vd_id, &fds);
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    r = select(m_vd_id + 1, &fds, NULL, NULL, &tv);
}
```

---

## 3. 修复指导

### 3.1 即时缓解 (配置层面，无需改代码)

修改 `bin/app_debug/yang_config.ini`：

```ini
[video]
width=640
height=480
frame=30
```

640x480 YUYV@30fps 已验证可正常工作（`v4l2-ctl --get-fmt-video` 确认当前格式即为 640x480 YUYV）。

### 3.2 代码修复方案 (按优先级)

#### 修复 A：S_FMT 失败传播 (必须)

**文件**：[YangVideoCaptureLinux.cpp:272-275](file:///home/quhan/01_myProject/metaRTC/libmetartc7/src/yangcapture/linux/YangVideoCaptureLinux.cpp#L272-L275)

```cpp
if ((ioctl(m_vd_id, VIDIOC_S_FMT, &v4_format)) != 0) {
    yang_error("VIDIOC_S_FMT failed: %s (format=%c%c%c%c %dx%d)",
        strerror(errno),
        format & 0xFF, (format >> 8) & 0xFF,
        (format >> 16) & 0xFF, (format >> 24) & 0xFF,
        m_width, m_height);
    return ERROR_SYS_Linux_VideoDeveceOpenFailure;
}
```

同样需要对 `VIDIOC_S_PARM`、`VIDIOC_REQBUFS`、`VIDIOC_STREAMON` 添加错误返回。

#### 修复 B：添加 MJPG 格式支持 (推荐)

1. 在 [YangVideoCaptureLinux.h:58](file:///home/quhan/01_myProject/metaRTC/libmetartc7/src/yangcapture/linux/YangVideoCaptureLinux.h#L58) 添加 `m_hasMjpg` 成员
2. 在 `setReselution()` ([line 120-129](file:///home/quhan/01_myProject/metaRTC/libmetartc7/src/yangcapture/linux/YangVideoCaptureLinux.cpp#L120-L129)) 添加 MJPG 分支
3. 在 `init()` 格式选择逻辑中添加 MJPG 优先选择
4. 在 `putBuffer()` 下游添加 MJPG → YUV 解码（使用 libjpeg 或项目已有的解码器）

**注意**：MJPG 输出的是 JPEG 压缩帧，下游编码器期望原始 YUV。需确认 metaRTC 是否已有 JPEG 解码能力。

#### 修复 C：移除 exit() 调用 (建议)

将 `read_buffer()` 和 `startLoop()` 中的 `exit(1)` 替换为 `stopLoop()` + `yang_error()` + `return`。析构函数中的 `exit()` 替换为 `yang_error()` + `return`。

#### 修复 D：帧率适配 (建议)

在 `VIDIOC_S_PARM` 前，检查 `enum_camera_frmival()` 枚举的帧率，使用摄像头实际支持的最大帧率，而非配置中的固定值。

---

## 4. memset 审计表

| 结构体 | 位置 | 已清零? | 状态 |
|--------|------|---------|------|
| `v4l2_frmivalenum frmival` | line 99 | ✅ (line 100) | 正常 |
| `v4l2_fmtdesc fmt` (setPara) | line 155 | ✅ (line 156) | 正常 |
| `v4l2_fmtdesc fmt` (setReselutionPara) | line 131 | ❌ | 需修复 (潜在) |
| `v4l2_frmsizeenum frmsize` (setPara) | line 177 | ❌ | 需修复 (低风险) |
| `v4l2_frmsizeenum frmsize` (setReselutionPara) | line 136 | ❌ | 需修复 (潜在) |
| `v4l2_format v4_format` | line 263 | ✅ (line 264) | 正常 |
| `v4l2_streamparm Stream_Parm` | line 281 | ✅ (line 282) | 正常 |
| `v4l2_requestbuffers tV4L2_reqbuf` | line 295 | ✅ (line 297) | 正常 |
| `v4l2_buffer tV4L2buf` (init) | line 312 | ✅ (line 313) | 正常 |
| `v4l2_buffer tV4L2buf` (startLoop) | line 397 | ✅ (line 398) | 正常 |
| `v4l2_buffer m_buf` (成员) | 构造函数 | ✅ (line 31) | 正常 |
| `v4l2_capability cap` | line 154 | ❌ | 可接受 (仅输出) |

---

## 5. 验证命令

```bash
# 查看摄像头支持的格式和帧率
v4l2-ctl --device=/dev/video0 --list-formats-ext

# 查看当前格式
v4l2-ctl --device=/dev/video0 --get-fmt-video

# 测试设置 YUYV 1280x720 (预期 EIO)
v4l2-ctl --device=/dev/video0 --set-fmt-video=width=1280,height=720,pixelformat=YUYV

# 测试设置 MJPG 1280x720 (预期成功)
v4l2-ctl --device=/dev/video0 --set-fmt-video=width=1280,height=720,pixelformat=MJPG

# 检查 USB 带宽
lsusb -t
```
