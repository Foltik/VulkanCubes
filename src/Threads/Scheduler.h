#pragma once

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "concurrentqueue.h"

class Scheduler {
private:
    class IJob {
    public:
        virtual void execute() = 0;
    };

    template<typename Func>
    class Job : public IJob {
    public:
        explicit Job(Func&& func) : m_func{std::move(func)} {}
        void execute() override { m_func(); }
    private:
        Func m_func;
    };

public:
    template<typename T>
    class Future {
    public:
        explicit Future(std::future<T>&& future) : m_future{std::move(future)} {}
        ~Future() { if (m_future.valid()) m_future.get(); }

        Future(const Future& rhs) = delete;
        Future& operator=(const Future& rhs) = delete;
        Future(Future&& other) = default;
        Future& operator=(Future&& other) = default;

        auto get() { return m_future.get(); }
    private:
        std::future<T> m_future;
    };

public:
    Scheduler() : Scheduler(std::max(std::thread::hardware_concurrency(), 2u) - 1u) {}
    explicit Scheduler(const std::uint32_t numThreads) : m_numThreads(numThreads) {
        try {
            for (auto i = 0; i < numThreads; i++)
                m_threads.emplace_back(&Scheduler::worker, this);
        } catch (...) {
            cleanup();
            throw;
        }
    }
    ~Scheduler() { cleanup(); }

    Scheduler(const Scheduler& rhs) = delete;
    Scheduler& operator=(const Scheduler& rhs) = delete;

    template<typename Func, typename... Args>
    auto run(Func&& func, Args&& ... args) {
        auto bound = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
        using ResultType = std::result_of_t<decltype(bound)()>;
        using Packaged = std::packaged_task<ResultType()>;

        Packaged job{std::move(bound)};
        Future<ResultType> result{job.get_future()};
        m_jobQueue.enqueue(std::make_unique<Job<Packaged>>(std::move(job)));
        return result;
    }

    template <typename Func>
    auto runRange(Func&& func, int start, int end) {
        std::vector<Scheduler::Future<void>> futures;

        int blockSize = (end - start) / m_numThreads;
        int remainder = (end - start) % m_numThreads;

        for (int i = 0; i < m_numThreads; i++) {
            futures.push_back(run([=]() {
                for (int j = start + (i * blockSize); j < start + ((i + 1) * blockSize); j++) {
                    func(j);
                }
            }));
        }

        for (int i = end - remainder; i < end; i++) {
            futures.push_back(run(std::forward<Func>(func), i));
        }

        return futures;
    };

    inline void sync() {
        while (m_activeJobs != 0);
    }

private:
    void worker() {
        return;
        while (m_running) {
            std::unique_ptr<IJob> job;
            if (m_jobQueue.try_dequeue(job)) {
                m_activeJobs++;
                job->execute();
                m_activeJobs--;
            }
            //std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    void cleanup() {
        m_running = false;
        for (auto& thread : m_threads)
            if (thread.joinable())
                thread.join();
    }

private:
    int m_numThreads;
    std::atomic_bool m_running = true;
    std::atomic_int m_activeJobs = 0;
    moodycamel::ConcurrentQueue<std::unique_ptr<IJob>> m_jobQueue;
    std::vector<std::thread> m_threads{};
};
