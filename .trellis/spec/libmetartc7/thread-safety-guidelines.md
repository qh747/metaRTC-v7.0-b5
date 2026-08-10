# libmetartc7 线程安全编码规范

> 基于 `YangSysMessageHandle` 多线程竞态修复提炼的线程安全规范。适用于消息队列、条件变量、共享标志位和单例指针等场景。

---

## 1. 共享数据结构访问必须加锁

### 1.1 规则

所有被多线程读写的共享状态（如 `std::vector`、裸指针队列、计数器等）必须在同一把 mutex 下访问。

### 1.2 本地模式

消息队列的入队和出队应使用同一把 mutex：

```cpp
// 生产者
yang_thread_mutex_lock(&m_mutex);
m_sysMessages.push_back(mes);
yang_thread_mutex_unlock(&m_mutex);

// 消费者
yang_thread_mutex_lock(&m_mutex);
YangSysMessage* mes = m_sysMessages.front();
m_sysMessages.erase(m_sysMessages.begin());
yang_thread_mutex_unlock(&m_mutex);
```

### 1.3 参考文件

- `libmetartc7/src/yangutil/YangSysMessageHandle.cpp`：`putMessage()` / `startLoop()` 消息队列访问

### 1.4 反模式

```cpp
// 错误：在锁外读取 front()、在锁内才 erase
YangSysMessage* mes = m_sysMessages.front();  // 无锁访问，data race
yang_thread_mutex_lock(&m_mutex);
m_sysMessages.erase(m_sysMessages.begin());
yang_thread_mutex_unlock(&m_mutex);
```

`std::vector` 不是线程安全的，并发 `push_back` 可能触发重新分配，导致无锁的 `front()` 返回失效引用。

---

## 2. 条件变量 signal 必须配合同一把 mutex

### 2.1 规则

`cond_signal` 必须在持有与 `cond_wait` 相同的 mutex 时调用，否则可能丢失 signal 造成死锁。

### 2.2 本地模式

```cpp
// 消费者线程
yang_thread_mutex_lock(&m_lock);
while (m_loop.load()) {
    yang_thread_cond_wait(&m_cond_mess, &m_lock);
    // 处理消息...
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

// 读写
if (!m_loop.load()) return;
m_loop = yangfalse;
```

### 3.3 参考文件

- `include/yangutil/sys/YangSysMessageHandle.h`：`m_loop` 声明
- `libmetartc7/src/yangutil/YangSysMessageHandle.cpp`：`m_loop` 读写位置

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
yang_thread_mutex_lock(&m_mutex);
YangSysMessage* mes = m_sysMessages.front();
m_sysMessages.erase(m_sysMessages.begin());
yang_thread_mutex_unlock(&m_mutex);

// 锁外处理
this->handleMessage(mes);
delete mes;
```

### 5.3 参考文件

- `libmetartc7/src/yangutil/YangSysMessageHandle.cpp`：`startLoop()` 消息处理逻辑

---

## 6. 验证检查

新增或修改共享状态时，检查以下问题：

1. 是否有 `std::vector` / 队列 / 链表等容器被多线程无锁访问？
2. `cond_signal` 是否在持有对应 mutex 时调用？
3. 跨线程标志位是否为 `std::atomic`？
4. 单例指针是否有同步保护，是否暴露到了头文件？
5. 耗时操作是否持有不应长期持有的锁？

---

## 7. 相关修复记录

- `YangSysMessageHandle` 消息队列竞态修复：`m_sysMessages` 无锁访问导致 data race / use-after-free。
- `YangSysMessageHandle` 条件变量修复：移除 `m_waitState`，改为无条件 `cond_signal` + `m_lock`。
- `YangSysMessageHandle` 标志位修复：`m_loop` 改为 `std::atomic<int32_t>`。
- `YangSysMessageHandle` 单例修复：`m_instance` 改为文件级 `std::atomic<YangSysMessageHandle*>`。
