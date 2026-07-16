#include "MAlloc/MAlloc.hpp"

#include <memory>
#include <print>
#include <string>

class Sensor {
public:
    Sensor(std::string name, double threshold) : name_(std::move(name)), threshold_(threshold) {}

    [[nodiscard]] const std::string& name() const noexcept { return name_; }

    [[nodiscard]] double threshold() const noexcept { return threshold_; }

private:
    std::string name_;
    double threshold_;
};

int main() {
    ma::Allocator allocator;

    void* storage = allocator.alignedAllocate(alignof(Sensor), sizeof(Sensor));
    if (storage == nullptr) {
        return 1;
    }

    Sensor* sensor = std::construct_at(static_cast<Sensor*>(storage), "thermal-1", 78.5);
    std::println("sensor {} with threshold {}", sensor->name(), sensor->threshold());

    std::destroy_at(sensor);
    allocator.deallocate(storage);
    return 0;
}
