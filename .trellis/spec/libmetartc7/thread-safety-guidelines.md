# libmetartc7 线程安全编码规范

> 基于 `YangSysMessageHandle` 多线程竞态修复提炼的线程安全规范。适用于消息队列、条件变量、共享标志位和单例指针等场景。

---

## 1. 共享数据结构访问必须加锁

### 1.1 规则

所有被多线程读写的共享状态（如 `std::vector`、裸指针队列、计数器等）必须在同一把 mutex 下访问。

### 1.2 本地模式

消息队列的入队和出队应使用同一把 mutex。在 `YangSysMessageHandle` 中，这把锁同时兼任条件变量互斥锁的职责：

```cpp
// 生产者
yang_thread_mutex_lock(&m_lock);
m_sysMessages.push_back(mes);
yang_thread_cond_signal(&m_cond_mess);
yang_thread_mutex_unlock(&m_lock);

// 消费者
yang_thread_mutex_lock(&m_lock);
YangSysMessage* mes = m_sysMessages.front();
m_sysMessages.erase(m_sysMessages.begin());
yang_thread_mutex_unlock(&m_lock);
```

### 1.3 参考文件

- `libmetartc7/src/yangutil/YangSysMessageHandle.cpp`：`putMessage()` / `startLoop()` 消息队列访问

### 1.4 反模式

```cpp
// 错误：在锁外读取 front()、在锁内才 erase
YangSysMessage* mes = m_sysMessages.front();  // 无锁访问，data race
yang_thread_mutex_lock(&m_lock);
m_sysMessages.erase(m_sysMessages.begin());
yang_thread_mutex_unlock(&m_lock);
```

`std::vector` 不是线程安全的，并发 `push_back` 可能触发重新分配，导致无锁的 `front()` 返回失效引用。

---

## 2. 条件变量 signal 必须配合同一把 mutex

### 2.1 规则

`cond_signal` 必须在持有与 `cond_wait` 相同的 mutex 时调用，否则可能丢失 signal 造成死锁。

### 2.2 本地模式

`cond_wait` 与 `cond_signal` 必须在同一把 mutex 上进行。在 `YangSysMessageHandle` 单锁设计中，这把 `m_lock` 同时保护消息队列：

```cpp
// 消费者线程
yang_thread_mutex_lock(&m_lock);
while (m_loop.load()) {
    yang_thread_cond_wait(&m_cond_mess, &m_lock);

    while (true) {
        if (m_sysMessages.empty()) {
            break;
        }
        YangSysMessage* mes = m_sysMessages.front();
        m_sysMessages.erase(m_sysMessages.begin());

        // 处理消息前必须释放锁，避免阻塞生产者
        yang_thread_mutex_unlock(&m_lock);
        this->handleMessage(mes);
        delete mes;
        yang_thread_mutex_lock(&m_lock);
    }
}
yang_thread_mutex_unlock(&m_lock);

// 生产者 / 停止线程
yang_thread_mutex_lock(&m_lock);
m_loop = yangfalse;
yang_thread_cond_signal(&m_cond_mess);
yang_thread_mutex_unlock(&m_lock);
```

### 2.3 参考文件

- `libmetartc7/src/yangutil/YangSysMessageHandle.cpp`：`startLoop()` / `stopLoop()` / `putMessage()`

### 2.4 反模式

```cpp
// 错误：无锁判断等待状态后再 signal
if (m_waitState) {              // 无锁读，可能读到旧值
    lock;
    cond_signal;
    unlock;
}
```

`m_waitState` 这类辅助标志位无法准确反映线程是否已经进入 `cond_wait`，无锁读写反而引入新的竞态。应改为**无条件 signal + 同一把 mutex**。

---

## 3. 跨线程共享的标志位使用 `std::atomic`

### 3.1 规则

被多个线程同时读写的布尔/整型标志位（如运行标志 `m_loop`），应声明为 `std::atomic<T>`，而不是普通成员变量。

### 3.2 本地模式

```cpp
// 头文件
#include <atomic>
std::atomic<int32_t> m_loop;
std::atomic<yangbool> m_isStart;

// 读写
if (!m_loop.load()) return;
m_loop = yangfalse;

m_isStart.store(yangtrue);
if (m_isStart.load()) { ... }
```

### 3.3 参考文件

- `include/yangutil/sys/YangSysMessageHandle.h`：`m_loop` / `m_isStart` 声明
- `libmetartc7/src/yangutil/YangSysMessageHandle.cpp`：`m_loop` / `m_isStart` 读写位置

### 3.4 反模式

```cpp
// 错误：普通成员变量跨线程读写
yangbool m_loop;    // 非原子

// 线程 A
while (m_loop) { ... }

// 线程 B
m_loop = yangfalse;
```

即使 `yangbool` 在目标平台上读写是原子的，C++ 标准不保证，且编译器可能重排读写顺序。

---

## 4. 单例指针隐藏到实现文件并用 atomic 保护

### 4.1 规则

如果某个类的单例指针只在 `.cpp` 内部被访问，不应暴露为 `public static` 成员变量。应改为文件级 `static std::atomic<T*>`，并通过 CAS 设置/清空，避免多线程同时构造时重复赋值。

### 4.2 本地模式

```cpp
// .cpp 文件顶部
#include <atomic>
static std::atomic<YangSysMessageHandle*> g_instance{nullptr};

// 构造函数中注册
YangSysMessageHandle* expected = nullptr;
g_instance.compare_exchange_strong(expected, this);

// 析构函数中注销
YangSysMessageHandle* expected = this;
g_instance.compare_exchange_strong(expected, nullptr);

// 外部 C 风格接口使用
void yang_post_message(...) {
    YangSysMessageHandle* inst = g_instance.load();
    if (inst) {
        inst->putMessage(...);
    }
}
```

### 4.3 参考文件

- `libmetartc7/src/yangutil/YangSysMessageHandle.cpp`：`g_instance` 定义与使用

### 4.4 反模式

```cpp
// 错误：public static 裸指针，无同步保护
class YangSysMessageHandle {
public:
    static YangSysMessageHandle* m_instance;
};

// 构造函数
m_instance = (m_instance == NULL) ? this : m_instance;
```

多线程同时构造时，可能都判断 `m_instance == NULL`，导致后创建的实例覆盖先创建的实例，先创建的实例无法再被外部访问到。

---

## 5. 处理耗时操作时不要持有队列锁

### 5.1 规则

从队列中取出元素后，应立即释放队列锁，再在锁外执行耗时处理。避免生产者线程被长时间阻塞。

### 5.2 本地模式

```cpp
yang_thread_mutex_lock(&m_lock);
YangSysMessage* mes = m_sysMessages.front();
m_sysMessages.erase(m_sysMessages.begin());
yang_thread_mutex_unlock(&m_lock);

// 锁外处理
this->handleMessage(mes);
delete mes;
```

### 5.3 参考文件

- `libmetartc7/src/yangutil/YangSysMessageHandle.cpp`：`startLoop()` 消息处理逻辑

---

## 6. 线程生命周期管理

### 6.1 规则

线程的“启动成功”标志与“正在运行”标志若被多个线程读写，必须声明为 `std::atomic`。基类 `YangThread::start()` 应声明为虚函数，以便派生类在启动成功后立即设置标志。析构时应使用 `join()` 等待线程结束，而不是轮询标志位空转。

### 6.2 本地模式

```cpp
// YangThread2.h
class YangThread {
public:
    virtual int32_t start();
    // ...
};

// YangSysMessageHandle.h
class YangSysMessageHandle : public YangThread {
public:
    virtual int32_t start();
    std::atomic<yangbool> m_isStart;
    // ...
};

// YangSysMessageHandle.cpp
int32_t YangSysMessageHandle::start() {
    int32_t ret = YangThread::start();
    if (ret == 0) {
        m_isStart.store(yangtrue);
    }
    return ret;
}

void YangSysMessageHandle::run() {
    this->startLoop();
    m_isStart.store(yangfalse);
}

YangSysMessageHandle::~YangSysMessageHandle() {
    if (m_isStart.load()) {
        this->stop();
        this->join();
    }
    // ...
}
```

### 6.3 参考文件

- `include/yangutil/sys/YangThread2.h`：`start()` 虚函数声明
- `include/yangutil/sys/YangSysMessageHandle.h`：`m_isStart` 声明、`start()` 覆盖声明
- `libmetartc7/src/yangutil/YangSysMessageHandle.cpp`：`start()` / `run()` / 析构函数

### 6.4 反模式

```cpp
// 错误：在工作线程 run() 里才设置 m_isStart，start() 与析构之间存在竞态窗口
void YangSysMessageHandle::run() {
    m_isStart = yangtrue;  // 线程已创建，但标志还没设置
    this->startLoop();
    m_isStart = yangfalse;
}

// 错误：析构里用自旋等待
~YangSysMessageHandle() {
    if (m_isStart) {
        this->stop();
        while (m_isStart) {  // 空转，且非原子时可能永远等不到
            yang_usleep(1000);
        }
    }
}
```

---

## 7. 验证检查

新增或修改共享状态时，检查以下问题：

1. 是否有 `std::vector` / 队列 / 链表等容器被多线程无锁访问？
2. `cond_signal` 是否在持有对应 mutex 时调用？
3. 跨线程标志位是否为 `std::atomic`？
4. 单例指针是否有同步保护，是否暴露到了头文件？
5. 耗时操作是否持有不应长期持有的锁？
6. 线程启动/停止标志是否在启动成功后立即设置，析构是否使用 `join()`？

---

## 8. 相关修复记录

- `YangSysMessageHandle` 消息队列竞态修复：`m_sysMessages` 无锁访问导致 data race / use-after-free；后合并 `m_mutex` 与 `m_lock` 为单锁，简化同步模型。
- `YangSysMessageHandle` 条件变量修复：移除 `m_waitState`，改为无条件 `cond_signal` + `m_lock`。
- `YangSysMessageHandle` 标志位修复：`m_loop` 改为 `std::atomic<int32_t>`，`m_isStart` 改为 `std::atomic<yangbool>`。
- `YangSysMessageHandle` 单例修复：`m_instance` 改为文件级 `std::atomic<YangSysMessageHandle*>`。
- `YangSysMessageHandle` 线程生命周期修复：`YangThread::start()` 改为虚函数，派生类在启动成功后立即设置 `m_isStart`；析构改用 `join()` 替代自旋等待。
