#include "ssarv/driver.hpp"
#include <exception>
#include <iostream>
int main(int argc, char** argv) {
    try { return ssarv::runCompiler(argc, argv); }
    catch (const std::exception& error) {
        std::cerr << "ssarv: " << error.what() << '\n';
        return 2;
    }
}
