#pragma once
#include <iostream>
#include <random>
#include <ctime>

#include <string>
#include <utility>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <type_traits>
#include <thread>
#include <chrono>
#include <fstream>

#include "metrics.h"

template<typename... T> requires all_unique_v<T...> && (is_metrics_derived_v<T> && ...)
class metrics_writer {
    std::tuple<T...> metrics_storage;
    std::vector<std::jthread> threads;
    std::mutex printing_mutex;
    std::atomic<bool> is_running = false;
    std::ofstream log_file;

    // to collect metrics by index from tuple and output to file if all metrics are collected
    template<size_t I>
    void process_metric() {
        auto &metric = std::get<I>(metrics_storage);
        while (is_running.load(std::memory_order_relaxed)) {
            metric.collect_metric();

            {
                std::lock_guard lock(printing_mutex);
                if (is_all_done()) print_metrics_and_reset();
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    bool is_all_done() {
        return std::apply([](auto &... m) { return (m.is_done() && ...); }, metrics_storage);
    }

    void print_metrics_and_reset() {
        std::string s = get_curr_time();
        std::apply([&s](auto &... m) {
            ((s += " \"" + m.get_name() + "\" " + std::to_string(m.get_value())), ...);
            ((m.reset_value()), ...);
        }, metrics_storage);
        log_file << s << "\n";
    }

    std::string get_curr_time() {
        auto now = std::chrono::system_clock::now();

        auto now_time_t = std::chrono::system_clock::to_time_t(now);

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) % 1000;

        std::ostringstream oss;
        oss << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S")
            << '.' << std::setfill('0') << std::setw(3) << ms.count();

        return oss.str();
    }

public:
    metrics_writer() = default;

    metrics_writer(const metrics_writer &) = default;

    metrics_writer(metrics_writer &&) = default;

    metrics_writer &operator=(const metrics_writer &) = default;

    metrics_writer &operator=(metrics_writer &&) = default;

    explicit metrics_writer(const std::string& file): log_file(file, std::ios::app) {}

    ~metrics_writer() {
        stop_collecting();
    }

    void set_file(const std::string& file) {
        std::lock_guard lock(printing_mutex);
        log_file.open(file, std::ios::app);
    }

    void start_collecting() {
        if (is_running || !log_file.is_open()) return;
        is_running = true;
        threads.reserve(sizeof...(T));
        [this]<size_t... I>(std::index_sequence<I...>) {
            ((this->threads.emplace_back([this]() {
                this->process_metric<I>();
            })), ...);
        }(std::index_sequence_for<T...>{});
    }

    void stop_collecting() noexcept {
        is_running = false;
        std::lock_guard lock(printing_mutex);
        threads.clear();
        if (log_file.is_open()) log_file.close();
    }
};

// deduction guide to aid CTAD
template<typename... Types>
metrics_writer(Types...) -> metrics_writer<Types...>;

