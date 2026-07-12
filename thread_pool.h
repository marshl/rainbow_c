#ifndef RAINBOW_C_THREAD_POOL_H
#define RAINBOW_C_THREAD_POOL_H

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

/// A fixed-size pool of worker threads for parallel min-reductions.
///
/// Workers are started once by the constructor and reused for every call to
/// parallel_range(), so the cost of launching threads is paid once — not per
/// scan.
///
/// Not thread-safe against concurrent calls: only one thread may invoke
/// parallel_range() at a time. That's fine for our use — RainbowRenderer's
/// fill loop is single-producer.
class ThreadPool {
public:
    /// Signature of the callback passed to parallel_range().
    /// Arguments: (worker_id, start_index, end_index_exclusive).
    /// The callback runs on a worker thread, over the half-open range
    /// [start_index, end_index_exclusive). `worker_id` is stable across a
    /// single parallel_range() call and unique per running chunk.
    using RangeFunc = std::function<void(std::size_t, std::size_t, std::size_t)>;

    /// Launches `num_threads` workers. Defaults to hardware_concurrency(),
    /// which reports the number of hardware threads the CPU exposes. Some
    /// systems return 0 from that call, so we clamp to a minimum of 1.
    explicit ThreadPool(std::size_t num_threads = std::thread::hardware_concurrency());

    /// Signals shutdown and joins every worker before returning. Joining is
    /// mandatory: destroying a still-joinable std::thread terminates the
    /// whole program.
    ~ThreadPool();

    // Copying/moving a pool would be dangerous — two pools sharing the same
    // workers and queue would race. `= delete` tells the compiler to refuse
    // any code that tries.
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    /// Splits the index range [0, count) into up to num_workers() chunks and
    /// dispatches each to a worker. Blocks the caller until every chunk has
    /// finished executing.
    ///
    /// If `count < num_workers()`, some workers receive no chunk. Any
    /// per-worker output slot indexed by their worker_id will NOT be written
    /// in that case — callers should initialise output slots to a sentinel
    /// (e.g. std::numeric_limits<float>::max() for a min-reduction) so the
    /// final reduce step still produces a correct answer.
    void parallel_range(std::size_t count, RangeFunc func);

    /// Number of worker threads in the pool. Fixed for the pool's lifetime.
    std::size_t num_workers() const { return workers_.size(); }

private:
    /// One unit of work handed to a worker.
    struct Task {
        RangeFunc func;
        std::size_t worker_id;
        std::size_t start;
        std::size_t end;
    };

    // The worker threads. Owning `std::thread`s here means the pool controls
    // their lifetime — they're started in the constructor and joined in the
    // destructor.
    std::vector<std::thread> workers_;

    // Pending chunks waiting to be picked up. Guarded by `mutex_`.
    std::queue<Task> task_queue_;

    // The single mutex guarding all shared state: task_queue_, pending_,
    // shutdown_. Held only briefly — never across the user's callback.
    std::mutex mutex_;

    // Workers sleep on this when the queue is empty. Signalled by the main
    // thread when new chunks are pushed, or in the destructor to wake them
    // for shutdown.
    std::condition_variable task_available_;

    // The main thread sleeps on this while waiting for the current batch to
    // finish. Signalled by whichever worker completes the last chunk.
    std::condition_variable task_completed_;

    // How many chunks from the current batch are still running. Reaches
    // zero when the batch is fully done.
    std::size_t pending_ = 0;

    // Destructor sets this to true and wakes all workers so they exit.
    bool shutdown_ = false;

    /// The body of each worker thread. Loops until shutdown_ is set and the
    /// queue drains.
    void worker_loop();
};

#endif // RAINBOW_C_THREAD_POOL_H
