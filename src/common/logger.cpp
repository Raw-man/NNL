#include "NNL/common/logger.hpp"

#include <atomic>
#include <iostream>

namespace nnl {

void DefaultLog(std::string_view msg, LogLevel log_lvl) {
  std::cerr << (log_lvl == LogLevel::kError ? "error: " : "warn: ") << msg << std::endl;
}

std::atomic<LogFun> log_f{DefaultLog};

void SetGlobalLogCB(LogFun callback) noexcept { log_f.store(callback, std::memory_order_release); }

void Log(std::string_view msg, LogLevel log_lvl) {
  LogFun current_log_f = log_f.load(std::memory_order_acquire);
  if (current_log_f) {
    current_log_f(msg, log_lvl);
  }
}

}  // namespace nnl
