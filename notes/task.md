`Task<T>` 是一个“懒启动 coroutine 返回类型”：调用函数时只创建协程帧，不执行函数体；只有别人 `co_await` 它时，它才开始跑，跑完后再把等待它的父协程恢复。

**1. Task 本体**
在 [src/task.h](D:/code/coro/src/task.h:40)：

```cpp
template <typename T = void, typename Allocator = void>
class [[nodiscard]] Task {
 public:
  using value_type = T;
```

`Task<T>` 表示一个最终会产生 `T` 的协程对象，比如：

```cpp
coro::Task<int> foo() {
  co_return 42;
}
```

注意：`Task` 不是结果本身，它更像“持有一个 coroutine handle 的任务对象”。真正的返回值存在 `promise_type` 里面。

**2. promise_type 是协程的控制中心**
核心在 [src/task.h](D:/code/coro/src/task.h:45)：

```cpp
struct promise_type : private Noncopyable,
                      detail::TaskPromiseStorage<value_type> {
```

每个 coroutine 都会有一个 `promise_type`。编译器看到：

```cpp
Task<int> foo() {
  co_return 42;
}
```

大概会生成类似这样的流程：

```cpp
创建 coroutine frame
创建 promise_type
调用 promise.get_return_object()
调用 co_await promise.initial_suspend()
执行函数体
遇到 co_return，调用 promise.return_value(42)
调用 co_await promise.final_suspend()
```

这里继承了 `TaskPromiseStorage<T>`，它来自 [src/task_promise_storage.h](D:/code/coro/src/task_promise_storage.h:19)，作用是提供：

```cpp
return_value(value)   // co_return value 时调用
return_void()         // co_return; 时调用
unhandled_exception() // 协程体抛异常时调用
get()                 // await_resume 时取结果
```

底层结果存在 [src/storage.h](D:/code/coro/src/storage.h:22) 的 `std::variant` 里：要么没值，要么异常，要么真正的 `T`。

**3. initial_suspend：为什么 Task 是懒启动**
在 [src/task.h](D:/code/coro/src/task.h:47)：

```cpp
static auto initial_suspend() noexcept {
  return std::suspend_always{};
}
```

`std::suspend_always` 表示：协程一创建就先挂起。

所以：

```cpp
auto t = foo();
```

此时 `foo()` 函数体里面的 `co_return 42` 还没执行。只是创建了协程帧，并返回一个 `Task<int>`。

这就是 lazy task。

**4. get_return_object：把 promise 包装成 Task**
在 [src/task.h](D:/code/coro/src/task.h:66)：

```cpp
Task get_return_object() noexcept {
  return Task{*this};
}
```

`Task{*this}` 会走私有构造：

```cpp
explicit Task(promise_type& promise) : handle_{promise} {}
```

`handle_` 是 [src/coro_handle.h](D:/code/coro/src/coro_handle.h:10) 里的 RAII 包装。它内部持有：

```cpp
std::coroutine_handle<promise_type>
```

析构时会自动：

```cpp
handle_.destroy();
```

所以 `Task` 拥有这个 coroutine frame 的生命周期。

**5. co_await Task 时发生什么**
最关键的是 `Awaiter`，在 [src/task.h](D:/code/coro/src/task.h:89)。

当你写：

```cpp
int x = co_await child();
```

编译器大概拆成：

```cpp
auto task = child();
auto awaiter = task.operator co_await();

if (!awaiter.await_ready()) {
  awaiter.await_suspend(parent_handle);
}

int x = awaiter.await_resume();
```

这里的 `operator co_await()` 在 [src/task.h](D:/code/coro/src/task.h:115)：

```cpp
auto operator co_await() const& noexcept {
  return Awaiter{*handle_};
}
```

它返回一个 awaiter，里面保存的是 child coroutine 的 handle：

```cpp
std::coroutine_handle<promise_type> handle;
```

也就是说：awaiter 手里拿着“被等待的那个 child 协程”。

**6. await_ready**
在 [src/task.h](D:/code/coro/src/task.h:90)：

```cpp
bool await_ready() const noexcept {
  return handle.done();
}
```

如果 child 已经结束了，就不用挂起 parent，直接 `await_resume()` 取结果。

正常第一次等待时，child 因为 `initial_suspend()` 挂住了，所以 `done()` 是 false。

**7. await_suspend：保存 parent，然后切到 child**
在 [src/task.h](D:/code/coro/src/task.h:99)：

```cpp
std::coroutine_handle<> await_suspend(std::coroutine_handle<> h) const {
  handle.promise().continuation = h;
  return handle;
}
```

这里特别重要：

`h` 是谁？

是当前正在执行 `co_await` 的协程，也就是 parent。

`handle` 是谁？

是被等待的 child。

所以这段话的意思是：

```cpp
child.promise().continuation = parent;
return child;
```

返回 `child` 这个 handle 后，运行时会恢复 child，让 child 开始执行。

所以控制流变成：

```text
parent 执行到 co_await child
parent 挂起
child.promise().continuation = parent
resume(child)
```

**8. child 跑完后 final_suspend 恢复 parent**
child 执行到 `co_return` 后，会进入 [src/task.h](D:/code/coro/src/task.h:53)：

```cpp
static AwaiterOf<void> auto final_suspend() noexcept {
  struct final_awaiter : std::suspend_always {
    std::coroutine_handle<> await_suspend(
        std::coroutine_handle<promise_type> h) {
      return h.promise().continuation;
    }
  };
  return final_awaiter{};
}
```

这里的 `h` 是 child 自己的 handle。

之前在 `await_suspend()` 里已经保存过：

```cpp
child.promise().continuation = parent;
```

所以 final_suspend 这里：

```cpp
return h.promise().continuation;
```

就是返回 parent handle。

运行时接着恢复 parent。于是完整链路是：

```text
parent co_await child
        |
        v
保存 parent 到 child.promise().continuation
        |
        v
resume(child)
        |
        v
child co_return，结果写入 promise
        |
        v
child final_suspend
        |
        v
return parent handle
        |
        v
resume(parent)
        |
        v
parent 调 await_resume() 取 child 结果
```

**9. await_resume：取结果**
在 [src/task.h](D:/code/coro/src/task.h:108)：

```cpp
decltype(auto) await_resume() const {
  return handle.promise().get();
}
```

这里从 child 的 promise 里取结果。

如果 child 是：

```cpp
Task<int> foo() {
  co_return 42;
}
```

那 `co_return 42` 会调用：

```cpp
promise.return_value(42);
```

结果存在 `Storage<int>` 里。`await_resume()` 再调用 `get()` 拿出来。

**10. 左值 co_await 和右值 co_await 的区别**
这两段在 [src/task.h](D:/code/coro/src/task.h:115) 和 [src/task.h](D:/code/coro/src/task.h:120)：

```cpp
auto operator co_await() const& noexcept
```

表示：

```cpp
auto t = foo();
int x = co_await t;
```

这是等待一个左值 task，结果通常按 `const&` 取。

下面这个：

```cpp
auto operator co_await() && noexcept
```

表示：

```cpp
int x = co_await foo();
```

或者：

```cpp
int x = co_await std::move(t);
```

这是等待一个右值 task，所以 `await_resume()` 里：

```cpp
return std::move(this->handle.promise()).get();
```

可以把结果 move 出来。

**最核心记忆**
`Task` 的灵魂就三个 handle：

```text
Task::handle_                 持有 child coroutine
Awaiter::handle               指向 child coroutine
promise_type::continuation    保存 parent coroutine
```

一句话串起来：

`parent co_await child` 时，`await_suspend(parent)` 把 parent 存进 child.promise().continuation，然后返回 child 让 child 执行；child 在 `final_suspend()` 返回 continuation，把 parent 恢复回来。