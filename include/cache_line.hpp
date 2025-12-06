#include<vector>
#include <chrono>

struct CACHE_LINE {
    //Variables
    uint32_t addr;
    bool valid;
    uint64_t counter;
    std::chrono::time_point<std::chrono::system_clock> last_access;
    
    //Memory
    std::vector<uint8_t> bytes;

    //Constructor
    CACHE_LINE(uint32_t line_size) : addr(0), valid(false), counter(0), last_access(std::chrono::system_clock::now()), bytes(line_size) {}

    //Method
    void copy(CACHE_LINE& other) {
        valid = true;
        addr = other.addr;
        counter = other.counter;
        last_access = other.last_access;
        bytes = other.bytes;
    }
};