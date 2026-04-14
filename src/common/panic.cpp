#include "NNL/common/panic.hpp"

#include <atomic>
#include <cstdlib>
#include <iostream>

namespace nnl {

void DefaultPanic(std::string_view reason) noexcept {
  std::cerr << "fatal: " << reason << std::endl;
  std::abort();
}

std::atomic<PanicFun> panic_f{DefaultPanic};

void SetGlobalPanicCB(PanicFun callback) noexcept { panic_f.store(callback, std::memory_order_release); }

[[noreturn]] void Panic(std::string_view reason) noexcept {
  PanicFun current_panic_f = panic_f.load(std::memory_order_acquire);
  if (current_panic_f) {
    current_panic_f(reason);
  }

  std::abort();
}

}  // namespace nnl
