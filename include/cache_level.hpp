#include <vector>
#include <random>
#include <stdexcept>
#include <chrono>

#include "strategy.hpp"
#include "cache_line.hpp"

struct CACHE_LEVEL {
    //Constants
    const uint32_t associativity;
    const uint32_t offset_size;
    const uint32_t index_size;
    const uint32_t offset_mask;
    const uint32_t index_mask;
    const Strategy strategy;

    //Memory
    std::vector<std::vector<CACHE_LINE>> sets;

    //Constructor
    CACHE_LEVEL(uint32_t line_num, uint32_t line_size, uint32_t associativity, Strategy strategy) :
    associativity(associativity),
    offset_size((uint32_t) log2(line_size)),
    index_size((uint32_t) log2(line_num / associativity)),
    offset_mask((1U << offset_size) - 1),
    index_mask((1U << index_size) - 1),
    strategy(strategy),
    sets(line_num / associativity)
    {
        for (std::vector<CACHE_LINE>& set : sets) {
            set.resize(associativity, CACHE_LINE(line_size));
        }
    }

    //Methods
    bool hasAddress(uint32_t addr) {
        uint32_t index = (addr >> offset_size) & index_mask;
        uint32_t tag = addr >> (offset_size + index_size);

        std::vector<CACHE_LINE>& set = sets[index];
        for (const CACHE_LINE& line : set) {
            uint32_t line_tag = line.addr >> (offset_size + index_size);
            if (line.valid == true && line_tag == tag) {
                return true;
            }
        }
        return false;
    }

    void load(uint32_t addr, CACHE_LINE& new_line) {
        new_line.counter = 0;
        new_line.last_access = std::chrono::system_clock::now();

        uint32_t index = (addr >> offset_size) & index_mask;

        std::vector<CACHE_LINE>& set = sets[index];

        for (CACHE_LINE& line : set) {
            if (line.valid == false) {
                line.copy(new_line);
                return;
            }
        }

        int chosen_index = 0;
        switch (strategy) {
            case LFU: {
                uint32_t min = set[0].counter;
                for (int i = 1; i < associativity; i++) {
                    if (set[i].counter < min) {
                        min = set[i].counter;
                        chosen_index = i;
                    }
                }
                break;
            }
            case LRU:
            case FIFO: {
                auto min = set[0].last_access;
                for (int i = 1; i < associativity; i++) {
                    if (set[i].last_access < min) {
                        min = set[i].last_access;
                        chosen_index = i;
                    }
                }
                break;
            }
            case MRU: {
                auto max = set[0].last_access;
                for (int i = 1; i < associativity; i++) {
                    if (set[i].last_access > max) {
                        max = set[i].last_access;
                        chosen_index = i;
                    }
                }
                break;
            }
            case RANDOM: {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> dist(0, associativity - 1);
                chosen_index = dist(gen);
                break;
            }
        }

        set[chosen_index].copy(new_line);
    }

    CACHE_LINE read_line(uint32_t addr) {
        uint32_t index = (addr >> offset_size) & index_mask;
        uint32_t tag = addr >> (offset_size + index_size);

        std::vector<CACHE_LINE>& set = sets[index];
        for (CACHE_LINE& line : set) {
            uint32_t line_tag = line.addr >> (offset_size + index_size);
            if (line.valid == true && line_tag == tag) {
                switch (strategy) {
                    case LRU:
                    case MRU:
                        line.last_access = std::chrono::system_clock::now();
                        break;
                    case LFU:
                        line.counter++;
                        break;
                    default:
                        break;
                }
                return line;
            }
        }
        throw std::runtime_error("Cache doesn't contain line!");
    }

    void write_byte(uint32_t addr, uint8_t data) {
        uint32_t offset = addr & offset_mask;
        uint32_t index = (addr >> offset_size) & index_mask;
        uint32_t tag = addr >> (offset_size + index_size);

        std::vector<CACHE_LINE>& set = sets[index];
        for (CACHE_LINE& line : set) {
            uint32_t line_tag = line.addr >> (offset_size + index_size);
            if (line.valid == true && line_tag == tag) {
                switch (strategy) {
                    case LRU:
                    case MRU:
                        line.last_access = std::chrono::system_clock::now();
                        break;
                    case LFU:
                        line.counter++;
                        break;
                    default:
                        break;
                }
                line.bytes[offset] = data;
                return;
            }
        }
        throw std::runtime_error("Cache doesn't contain line!");
    }
    
    uint8_t get_line_data_by_index(uint32_t lineIndex, uint32_t dataIndex){
        int counter = 0;
        for(int i=0;i<sets.size();++i){
            for(int j=0;j<sets[i].size();++j){
                if(counter==lineIndex){
                    if(dataIndex>=sets[i][j].bytes.size()){
                        throw std::runtime_error("Cache line doesn't contain data on specified index!");
                    }
                    return sets[i][j].bytes[dataIndex];
                }
                counter+=1;
            }
        }
        throw std::runtime_error("Cache level doesn't contain line on specified index!");
    }
};