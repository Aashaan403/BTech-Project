#include "ssarv/ir.hpp"
#include "ssarv/todo.hpp"

namespace ssarv {

std::size_t typeSizeBytes(std::string_view) {
    unimplemented("typeSizeBytes");
}

std::expected<Module, std::string> parseLLText(std::string_view, std::string) {
    unimplemented("parseLLText");
}

std::expected<Module, std::string> parseLL(const std::string&) {
    unimplemented("parseLL");
}

} // namespace ssarv
