#include <systemc>
#include <systemc.h>
#include <stdint.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include "cache.hpp"
#include "main_memory.hpp"
#include "result.h"
#include "request.h"

struct Result run_simulation(
  uint32_t cycles,
  const char* tracefile,
  uint8_t numCacheLevels,
  uint32_t cachelineSize,
  uint32_t numLinesL1,
  uint32_t numLinesL2,
  uint32_t numLinesL3,
  uint32_t latencyCacheL1,
  uint32_t latencyCacheL2,
  uint32_t latencyCacheL3,
  uint8_t mappingStrategy,
  uint32_t numRequests,
  struct Request* requests){
    std::vector<uint32_t> line_nums = {};
    std::vector<uint32_t> latencies = {};
    if(numCacheLevels>=1){
        line_nums.push_back(numLinesL1);
        latencies.push_back(latencyCacheL1);
    }
    if(numCacheLevels>=2){
        line_nums.push_back(numLinesL2);
        latencies.push_back(latencyCacheL2);
    }
    if(numCacheLevels>=3){
        line_nums.push_back(numLinesL3);
        latencies.push_back(latencyCacheL3);
    }

    //Initilize modules
    Strategy strategyEnum = LRU;
    
    CACHE cache("CACHE", numCacheLevels, line_nums, cachelineSize, latencies, mappingStrategy, strategyEnum);
    MAIN_MEMORY memory("MAIN_MEMORY");

    //Clock
    sc_clock clk("clk", 10, SC_NS);
    cache.clk.bind(clk);
    memory.clk.bind(clk);

    //Signals
    sc_signal<uint32_t> addr;
    sc_signal<uint32_t> wdata;
    sc_signal<bool> r;
    sc_signal<bool> w;
    sc_signal<bool> mem_ready;
    sc_signal<uint32_t> mem_rdata;

    sc_signal<uint32_t> rdata;
    sc_signal<bool> ready;
    sc_signal<bool> miss;
    sc_signal<uint32_t> mem_addr;
    sc_signal<uint32_t> mem_wdata;
    sc_signal<bool> mem_r;
    sc_signal<bool> mem_w;

    //Input
    cache.addr.bind(addr);
    cache.wdata.bind(wdata);
    cache.r.bind(r);
    cache.w.bind(w);
    cache.mem_ready.bind(mem_ready);
    cache.mem_rdata.bind(mem_rdata);

    memory.addr.bind(mem_addr);
    memory.wdata.bind(mem_wdata);
    memory.w.bind(mem_w);
    memory.r.bind(mem_r);

    //Output
    cache.rdata.bind(rdata);
    cache.ready.bind(ready);
    cache.miss.bind(miss);
    cache.mem_addr.bind(mem_addr);
    cache.mem_wdata.bind(mem_wdata);
    cache.mem_r.bind(mem_r);
    cache.mem_w.bind(mem_w);

    memory.rdata.bind(mem_rdata);
    memory.ready.bind(mem_ready);
    
    //Trace file setup
    sc_trace_file* tf;
    bool traceFlag = false;
    if(strcmp(tracefile,"")!=0){
        std::string dir = "";
        for(int i=0;i<strlen(tracefile);++i){
            dir = dir + tracefile[i];
            if(tracefile[i]=='/'){
                if (mkdir(dir.c_str(), 0777)!=0 && errno!=EEXIST) {
                    std::cout<<"Invalid TraceFile path! No TraceFile will be created!\n";
                    traceFlag = true;
                    break;
                }
            }
        }
        if(traceFlag==false){
            tf = sc_create_vcd_trace_file(tracefile);

            sc_trace(tf, clk, "clk");
            sc_trace(tf, addr, "addr");
            sc_trace(tf, wdata, "wdata");
            sc_trace(tf, r, "r");
            sc_trace(tf, w, "w");
            sc_trace(tf, mem_ready, "mem_ready");
            sc_trace(tf, mem_rdata, "mem_rdata");
            sc_trace(tf, rdata, "rdata");
            sc_trace(tf, ready, "ready");
            sc_trace(tf, miss, "miss");
            sc_trace(tf, mem_addr, "mem_addr");
            sc_trace(tf, mem_wdata, "mem_wdata");
            sc_trace(tf, mem_r, "mem_r");
            sc_trace(tf, mem_w, "mem_w");
        }
    }
    
    //Result setups
    struct Result res;
    res.cycles = 0;
    res.hits = 0;
    res.misses = 0;
    if(numRequests==0){
        if(strcmp(tracefile,"")!=0 && traceFlag==false){
            sc_close_vcd_trace_file(tf);
        }
        return res;
    }

    for(int i=0;i<numRequests;++i){
        addr.write(requests[i].addr);
        wdata.write(requests[i].data);
        w.write(requests[i].w==1);
        r.write(requests[i].w==0);

        if(res.cycles>=cycles)return res;
        sc_start(10, SC_NS);
        res.cycles+=1;

        while(ready.read()==false){
            if(res.cycles>=cycles)return res;
            sc_start(10, SC_NS);
            res.cycles+=1;
        }

        if(miss.read()==true){
            ++res.misses;
        }
        else ++res.hits;
        
        if(requests[i].w==0){
            requests[i].data=cache.rdata.read();
        }
    }

    if(strcmp(tracefile,"")!=0 && traceFlag==false){
        sc_close_vcd_trace_file(tf);
    }

    return res;
}


int sc_main(int argc, char* argv[]) {
    std::cout<<"ERROR"<<std::endl;
    return 1;
}