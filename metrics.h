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

template<typename T, typename... Rest>
inline constexpr bool all_unique_v = (!std::is_same_v<T, Rest> && ...) && all_unique_v<Rest...>;

template<typename T>
inline constexpr bool all_unique_v<T> = true;

// type T - the type of value we get when collecting the metric
template<typename T> requires std::is_default_constructible_v<T> && std::is_copy_assignable_v<T> &&
                              std::is_copy_constructible_v<T>
class metrics {
protected:
    std::string name;
    bool done_status = false;
    T value;

    explicit metrics(std::string name) : name(std::move(name)) {}

public:
    virtual void collect_metric_helper() = 0;

    void collect_metric() {
        collect_metric_helper();
        done_status = true;
    }

    std::string get_name() { return name; }

    T &get_value() { return value; }

    void set_value(T v) { value = v; }

    void reset_value() {
        value = T();
        done_status = false;
    }

    bool is_done() { return done_status; }
};

namespace metrics_details {
    template<typename T>
    struct is_metrics_derived {
        template<typename U>
        static std::true_type test(const metrics<U> *);

        static std::false_type test(...);

        static constexpr bool value = decltype(test(std::declval<T *>()))::value;
    };
}

template<typename T>
inline constexpr bool is_metrics_derived_v = metrics_details::is_metrics_derived<T>::value;

// counts the number of additions of 1 in 10 ms
class OP10ms_metric : public metrics<std::size_t> {
    size_t operation_count = 0;
public:
    OP10ms_metric() : metrics("OP10ms") {}

    void collect_metric_helper() override {
        auto last = operation_count;
        std::jthread th([this](const std::stop_token &st) {
            while (true) {
                ++this->operation_count;
                if (st.stop_requested()) break;
            }
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        th.request_stop();
        auto curr = operation_count;
        set_value(curr - last);
        operation_count = 0;
    }
};

// generates a random number from low to high
class RND_metric : public metrics<double> {
    std::mt19937 gen;
    int low = 1, high = 100;
public:
    RND_metric() : metrics("RND"), gen(std::random_device{}()) {}

    RND_metric(int low, int high) : metrics("RND with borders"), low(low), high(high) {}

    void collect_metric_helper() override {
        std::uniform_int_distribution<int> dist(low, high);
        set_value(dist(gen));
    }
};