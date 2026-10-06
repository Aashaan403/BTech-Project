#pragma once
#include <stdexcept>
#include <string>
#include <string_view>

namespace ssarv {
[[noreturn]] inline void unimplemented(std::string_view function) {
    throw std::logic_error("TODO: implement " + std::string(function));
}
} // namespace ssarv
