#include "thread_pool.h"

#include <algorithm>

// ─── Constructor ──────────────────────────────────────────────────────────
// Start the worker threads. Each one immediately enters worker_loop() and
// stays there — sleeping when there's no work — until the destructor sets
// shutdown_ and wakes them up.
ThreadPool::ThreadPool(std::size_t num_threads) {
    // hardware_concurrency() may return 0 on systems that can't determine
    // the count (rare, but the standard permits it). We need at least one
    // worker or parallel_range() would deadlock.
    if (num_threads == 0) num_threads = 1;

    // reserve() pre-sizes the vector's storage so subsequent push_backs
    // don't reallocate. Optional, but tidy.
    workers_.reserve(num_threads);

    for (std::size_t i = 0; i < num_threads; ++i) {
        // emplace_back constructs a std::thread in place. The lambda is the
        // thread's start function — it runs on the new thread as soon as
        // the constructor returns.
        //
        // `[this]` captures the ThreadPool pointer by value so the lambda
        // can call the private worker_loop() method.
        workers_.emplace_back([this] { worker_loop(); });
    }
}

// ─── Destructor ───────────────────────────────────────────────────────────
// Tell workers to stop, wake them, wait for them to exit.
ThreadPool::~ThreadPool() {
    {
        // Take the mutex just long enough to publish shutdown_ = true.
        // std::unique_lock is RAII: locks on construction, unlocks when the
        // scope (the enclosing braces) ends. We use braces here to release
        // the mutex BEFORE we notify — otherwise workers would wake up and
        // immediately block on the mutex we still hold.
        std::unique_lock<std::mutex> lock(mutex_);
        shutdown_ = true;
    }

    // Wake every worker so they observe shutdown_ and exit their loop.
    // notify_all is important here — we need all workers to see the flag,
    // not just one.
    task_available_.notify_all();

    // Wait for each worker thread to fully exit. join() blocks until the
    // thread's start function (worker_loop) returns. If we destroyed the
    // std::thread objects without joining, the C++ runtime would call
    // std::terminate — a hard program exit.
    for (auto& t : workers_) {
        t.join();
    }
}

// ─── worker_loop ──────────────────────────────────────────────────────────
// The body that every worker thread runs. Sleep, wake, do work, repeat.
void ThreadPool::worker_loop() {
    while (true) {
        Task task;

        {
            // unique_lock (not lock_guard) because condition_variable::wait
            // needs to unlock the mutex while sleeping and re-lock on wake.
            std::unique_lock<std::mutex> lock(mutex_);

            // wait() sleeps this thread on the condition variable, releasing
            // the mutex atomically while asleep. When notified, it re-locks
            // the mutex and checks the predicate. If the predicate is false,
            // it goes back to sleep. This handles "spurious wakeups" — the
            // OS may wake threads without a matching notify, and the
            // predicate keeps us correct in that case.
            task_available_.wait(lock, [this] {
                return !task_queue_.empty() || shutdown_;
            });

            // If we're shutting down AND there's no more work to do, exit
            // the thread. We drain the queue even during shutdown so no
            // dispatched batch is silently dropped.
            if (shutdown_ && task_queue_.empty()) {
                return;
            }

            // Pop one task off the front of the queue. std::move avoids
            // copying the std::function; it transfers ownership to `task`.
            task = std::move(task_queue_.front());
            task_queue_.pop();
            // The lock releases here as `lock` goes out of scope at the `}`.
        }

        // Run the user's callback for our chunk. Crucially, we're NOT
        // holding the mutex — other workers can pop and run their own
        // chunks concurrently.
        task.func(task.worker_id, task.start, task.end);

        {
            // Reacquire the mutex to update pending_ safely.
            std::unique_lock<std::mutex> lock(mutex_);
            --pending_;

            // If we just finished the last chunk of the batch, wake the
            // main thread that's blocked in parallel_range() waiting for
            // pending_ to hit zero.
            if (pending_ == 0) {
                task_completed_.notify_one();
            }
        }
    }
}

// ─── parallel_range ───────────────────────────────────────────────────────
// The public work-dispatch entry point. Splits [0, count) into up to
// num_workers() chunks and blocks until every one finishes.
void ThreadPool::parallel_range(std::size_t count, RangeFunc func) {
    // Empty ranges are a no-op. Without this check the ceiling division
    // below would still work, but skipping the mutex is cheaper.
    if (count == 0) return;

    const std::size_t n = workers_.size();

    // Ceiling division: how many items each chunk gets. The last chunk may
    // be shorter (see the std::min below). Example: count=100, n=8 →
    // chunk_size=13, chunks are [0,13), [13,26), … , [91,100).
    const std::size_t chunk_size = (count + n - 1) / n;

    {
        // Push one task per worker under the lock so workers see a
        // consistent queue when they wake.
        std::unique_lock<std::mutex> lock(mutex_);

        for (std::size_t w = 0; w < n; ++w) {
            std::size_t start = w * chunk_size;

            // If count < n, later workers get no chunk. Their per-worker
            // output slots must be pre-initialised by the caller (see the
            // header docs).
            if (start >= count) break;

            std::size_t end = std::min(start + chunk_size, count);

            // Copies `func` into the queued Task. std::function copy is
            // cheap in practice — it's an internal pointer bump for
            // heap-allocated callables, and inline storage for small ones.
            task_queue_.push({func, w, start, end});
            ++pending_;
        }
    }

    // Wake every worker. Some may find the queue empty (if we dispatched
    // fewer chunks than workers) and go back to sleep — that's harmless.
    task_available_.notify_all();

    {
        // Block until pending_ reaches zero. The predicate handles the
        // case where the batch finishes before we even get here — wait()
        // checks the predicate before sleeping, so we return immediately
        // if pending_ is already 0.
        std::unique_lock<std::mutex> lock(mutex_);
        task_completed_.wait(lock, [this] { return pending_ == 0; });
    }
}
