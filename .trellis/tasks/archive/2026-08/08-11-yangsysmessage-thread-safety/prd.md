# YangSysMessageHandle 线程安全修复

## Goal

修复 `YangSysMessageHandle` 中消息队列、条件变量、共享标志位和单例指针的多线程竞态问题，避免 data race、signal 丢失和死锁。

## Requirements

1. **消息队列访问加锁**
   - `m_sysMessages` 的所有入队、出队、读取操作必须在同一把 mutex (`m_mutex`) 下完成。
   - 取出消息后应立即释放队列锁，再在锁外调用 `handleMessage()` 处理，避免阻塞生产者。

2. **条件变量正确使用**
   - `cond_signal` 必须在持有与 `cond_wait` 相同的 mutex (`m_lock`) 时调用。
   - 移除 `m_waitState` 辅助标志，改为无条件 signal，消除无锁读写导致的 signal 丢失风险。

3. **共享标志位原子化**
   - `m_loop` 声明为 `std::atomic<int32_t>`，所有跨线程读写使用 `.load()` / 直接赋值。

4. **单例指针隐藏与同步**
   - 删除头文件中 `public static YangSysMessageHandle* m_instance` 声明。
   - 在 `.cpp` 中使用文件级 `static std::atomic<YangSysMessageHandle*> g_instance`。
   - 构造/析构时使用 CAS (`compare_exchange_strong`) 设置/清空，保证多线程首次创建时的安全性。

5. **头文件注释补充**
   - 为构造函数和析构函数补充 Doxygen 风格注释，与 [YangSysMessageI.h](../../include/yangutil/sys/YangSysMessageI.h) 保持一致。

## Acceptance Criteria

- [ ] `include/yangutil/sys/YangSysMessageHandle.h` 中 `m_instance` 和 `m_waitState` 已删除，`m_loop` 改为 `std::atomic<int32_t>`，并包含 `<atomic>`。
- [ ] `libmetartc7/src/yangutil/YangSysMessageHandle.cpp` 中消息队列访问全程在 `m_mutex` 保护下。
- [ ] `stopLoop()` 和 `putMessage()` 中的 `cond_signal` 均在 `m_lock` 临界区内调用。
- [ ] `m_waitState` 相关代码已完全移除。
- [ ] `m_loop` 的所有跨线程读写使用原子操作。
- [ ] `g_instance` 使用 `std::atomic` + CAS 实现线程安全注册/注销。
- [ ] 代码能通过项目编译（cmake / make）。

## Notes

- 参考已写入 `.trellis/spec/libmetartc7/thread-safety-guidelines.md` 的线程安全规范。
- 本次修改不引入新依赖，仅使用 C++11 标准库的 `<atomic>`。
