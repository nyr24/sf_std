#include "test_manager.hpp"

using namespace sf;

int main() {
    TestManager manager{};
    manager.collect_all_tests();
    manager.run_all_tests();
}
