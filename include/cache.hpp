#include <systemc>
#include <systemc.h>
#include <vector>

#include <utility>
#include <cstdint>

#include "cache_level.hpp"
#include "result.h"
#include "request.h"

extern "C" {struct Result run_simulation(uint32_t, const char*, uint8_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint8_t, uint32_t, struct Request*);}

SC_MODULE(CACHE) {
    //Input Ports
    sc_in<bool> clk;
    sc_in<uint32_t> addr;
    sc_in<uint32_t> wdata;
    sc_in<bool> r;
    sc_in<bool> w;
    sc_in<bool> mem_ready;
    sc_in<uint32_t> mem_rdata;

    //Output Ports
    sc_out<uint32_t> rdata;
    sc_out<bool> ready;
    sc_out<bool> miss;
    sc_out<uint32_t> mem_addr;
    sc_out<uint32_t> mem_wdata;
    sc_out<bool> mem_r;
    sc_out<bool> mem_w;

    //Cache Levels
    std::vector<CACHE_LEVEL> levels;

    //Constants
    const std::vector<uint32_t> latencies;
    const Strategy strategy;
    const uint32_t line_size;
    const uint32_t offset_mask;

    //Constructor
    SC_HAS_PROCESS(CACHE);
    CACHE(sc_module_name module_name, uint8_t level_num, std::vector<uint32_t> line_nums, uint32_t line_size, std::vector<uint32_t> latencies, uint8_t associativity, Strategy strategy) :
    sc_module(module_name),
    latencies(latencies),
    strategy(strategy),
    line_size(line_size),
    offset_mask((1U << (uint32_t) log2(line_size)) - 1)
    {
        //Initialize Levels
        int lps;
        for (int i = 0; i < level_num; i++) {
            if (associativity == 0) {
                lps = 1;
            }
            else if (associativity == 1) {
                lps = line_nums[i];
            }
            else {
                lps = associativity;
            }
            
            levels.push_back(CACHE_LEVEL(line_nums[i], line_size, lps, strategy));
        }
        //Set Thread
        SC_CTHREAD(run, clk);
    }

    //Methods
    bool hasAddress(uint32_t addr) {
        bool broke = false;
        int i = 0;
        for (CACHE_LEVEL& level : levels) {
            while (i < 4) {
                if (!level.hasAddress(addr + i)) {
                    if (broke) {
                        return false;
                    }
                    broke = true;
                    break;
                }
                i++;
            }
        }
        return true;
    }

    bool firstHasAddress(uint32_t addr) {
        for (int i = 0; i < 4; i++) {
            if (!levels[0].hasAddress(addr + i)) {
                return false;
            }
        }
        return true;
    }

    CACHE_LINE read_memory(uint32_t addr) {
        uint32_t curr_addr = addr - (addr % line_size);
        CACHE_LINE new_line(line_size);
        new_line.addr = curr_addr;

        uint32_t index = 0;
        for (uint32_t i = 0; i < line_size / 4; i++) {            
            mem_addr.write(curr_addr);
            mem_r.write(1);
            mem_w.write(0);
            wait(10);
            mem_r.write(0);
            while (!mem_ready.read()) {
                wait();
            }

            uint32_t word = mem_rdata.read();

            for (int j = 0; j < 4; j++) {
                new_line.bytes[index + j] = static_cast<uint8_t>(word);
                word >>= 8;
            }
            index += 4;
            curr_addr += 4;
        }
        return new_line;
    }

    uint8_t read_byte(uint32_t addr, CACHE_LINE line) {
        uint32_t offset = addr & offset_mask;
        return line.bytes[offset];
    }

    void write_memory(uint32_t addr, uint32_t data) {
        mem_addr.write(addr);
        mem_wdata.write(data);
        mem_w.write(1);
        mem_r.write(0);
        wait(2);
        mem_w.write(0);
        while (!mem_ready.read()) {
            wait();
        }
    }

    uint8_t getCacheLineContent(uint32_t level, uint32_t lineIndex, uint32_t index){
        if(level>levels.size()){
            throw std::runtime_error("Cache level doesn't contain specified level!");
        }
        return levels[level-1].get_line_data_by_index(lineIndex,index);
    }

    void run() {
        while (true) {
            wait();
            //Read Operation
            if (r.read() && !w.read()) {
                ready.write(false);
                miss.write(false);
                
                uint8_t index;
                uint8_t max_index = 0;
                uint32_t word = 0;
                uint32_t start_addr = addr.read();
            
                //Loop over byte addresses
                for (int i = 0; i < 4; i++) {
                    index = 0;
                    //Loop over levels
                    while (index < levels.size()) {
                        CACHE_LEVEL& curr_level = levels[index];
                        //Check if current level has the byte address
                        if (curr_level.hasAddress(start_addr + i)) {
                            CACHE_LINE new_line = curr_level.read_line(start_addr + i);
                            //Load the cache line to all cache levels below
                            for (int j = 0; j < index; j++) {
                                levels[j].load(start_addr + i, new_line);
                            }
                            //Add byte to word (little-endian)
                            word |= read_byte(start_addr + i, new_line) << (i * 8);
                            //Stop looping over levels if adress was already found
                            break;
                        }
                        else {
                            //Move onto next level
                            index++;
                            //Save max reached level index for latency calculation
                            if (index > max_index) {
                                max_index = index;
                            }
                        }
                    }
                    //Cache Miss if none of the levels contain address
                    if (index >= levels.size()) {
                        miss.write(true);
                        //Get new cache line and load into each cache level
                        CACHE_LINE new_line = read_memory(start_addr + i);
                        for (CACHE_LEVEL& level : levels) {
                            level.load(start_addr + i, new_line);
                        }
                        word |= read_byte(start_addr + i, new_line) << (i * 8);
                    }
                }

                //Wait according latency of max_index level in case of Cache Hit
                if (max_index < levels.size()) {
                    wait(latencies[max_index]);
                }
                //Write word to output and set flags
                rdata.write(word);
                ready.write(true);
            }
            //Write Operation
            else if (!r.read() && w.read()) {
                ready.write(false);
                miss.write(false);

                uint8_t i = 0;
                uint32_t start_addr = addr.read();
                uint32_t word = wdata.read();

                bool hit;

                //Loop over byte addresses
                while (i < 4) {
                    hit = false;
                    //Loop over levels
                    for (uint32_t j = 0; j < levels.size(); j++) {
                        CACHE_LEVEL& curr_level = levels[j];
                        //Check if current level has the byte address
                        if (hit || curr_level.hasAddress(start_addr + i)) {
                            hit = true;
                            //Write the byte to current level
                            curr_level.write_byte(start_addr + i, (uint8_t) ((word >> (8 * i)) & 0b11111111));
                        }
                    }

                    //Cache Miss if none of the levels contain address
                    if (!hit) {
                        miss.write(true);
                        //Get new cache line, update and load into each cache level
                        CACHE_LINE new_line = read_memory(start_addr + i);
                        for (CACHE_LEVEL& level : levels) {
                            level.load(start_addr + i, new_line);
                        }
                    }
                    //Move onto next byte if no miss occured
                    //If miss occured loop again to update cache levels
                    else {
                        i++;
                    }
                }

                //Write to main memory
                write_memory(start_addr, word);
                ready.write(true);
            }
        }
    }
};