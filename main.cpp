#include "metrics_writer.h"

// ONLY C++20+

int main() {
    std::string file_path = "YOUR FILE PATH";
    metrics_writer<RND_metric, OP10ms_metric> metricsWriter;
    metrics_writer<RND_metric> metricsWriter2(file_path);
    metricsWriter.set_file(file_path);
    metricsWriter.register_metrics(RND_metric(2, 10), OP10ms_metric());

    metricsWriter.start_collecting();
    metricsWriter2.start_collecting();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    metricsWriter.stop_collecting();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}