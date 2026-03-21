#include "NNL/common/logger.hpp"

#include <atomic>
#include <iostream>

namespace nnl {

std::atomic<LogFun> log_f{[](const std::string& msg, LogLevel log_lvl) {
  std::cerr << (log_lvl == LogLevel::kError ? "error: " : "warn: ") << msg << std::endl;
}};

void SetGlobalLogCB(LogFun callback) { log_f.store(callback, std::memory_order_relaxed); }

void Log(const std::string& msg, LogLevel log_lvl) {
  LogFun current_log_f = log_f.load(std::memory_order_relaxed);
  if (current_log_f) {
    current_log_f(msg, log_lvl);
  }
}

}  // namespace nnl
