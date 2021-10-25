#pragma once

#include <mutex>
#include <queue>
#include <functional>
#include <utility>

class thread_pool_t {

    std::mutex m_queue_mutex;
    std::queue<std::function<void()>> m_tasks;

    std::unique_ptr<std::thread[]> m_threads;
    size_t m_thread_count;

    std::atomic<size_t> m_pending_tasks = 0;

    std::atomic<bool> m_active;

public:
    thread_pool_t(size_t workers) 
        : m_threads(new std::thread[workers])
        , m_thread_count(workers) {

        auto fn = [&]() {

            for (;;) {

                std::unique_lock g(m_queue_mutex);

                if (m_tasks.empty()) {

                    if (!m_active) {
                        break;
                    }

                    std::this_thread::sleep_for(std::chrono::microseconds(1000));
                    continue;
                }

                auto task = std::move(m_tasks.front());
                m_tasks.pop();

                g.unlock();

                task();
                m_pending_tasks--;
            }
        };

        for (size_t i = 0; i < m_thread_count; i++) {
            m_threads[i] = std::thread(fn);
        }
    }

    void push_task(std::function<void()> fn) {
        std::lock_guard g(m_queue_mutex);
        m_tasks.push(fn);
    }

    ~thread_pool_t() {

        m_active = false;

        for (size_t i = 0; i < m_thread_count; i++) {
            m_threads[i].join();
        }
    }
};
