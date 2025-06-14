#include "metrics_writer.h"

// ONLY C++20+

int main() {
    std::string file_path = "YOUR FILE PATH";
    metrics_writer<RND_metric, OP10ms_metric> metricsWriter;
    metrics_writer<RND_metric> metricsWriter2(file_path);
    metricsWriter.set_file(file_path);
    metricsWriter.start_collecting();
    metricsWriter2.start_collecting();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}