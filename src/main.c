#include <getopt.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "result.h"
#include "request.h"

const char* USAGE = "[--num-cache-levels] number of cache levels (1 - 3) (default: 1)\n" 
                    "[--cacheline-size] size of cacheline in bytes (default: 64)\n"
                    "[--num-lines-l1] number of cache lines in l1 cache (default: 512)\n"
                    "[--num-lines-l2] number of cache lines in l2 cache (default: 4096)\n" 
                    "[--num-lines-l3] number of cache lines in l3 cache (default: 32768)\n"
                    "[--latency-cache-l1] latency of l1 cache in clock cycles (default: 4)\n" 
                    "[--latency-cache-l2] latency of l2 cache in clock cycles (default: 12)\n" 
                    "[--latency-cache-l3] latency of l3 cache in clock cycles (default: 32)\n"
                    "[--mapping-strategy] 0 = direct-mapped, 1 = fully associative, 2 = 2-way associative etc. (default: 1)\n"
                    "[--cycles] number of cycles to be simulated (default: 1000)\n"
                    "[--tf] path for creating tracefile (default: None)\n"
                    "<file> positional argument: the path for inputdata to be used\n"
                    "[--help] display help (this message)\n";  

const char* NUM_CACHE_ERROR = "ERROR: number of cache levels must be between 1 and 3\n";

// Defining standard values 
uint8_t num_cache_levels = 1; 
uint32_t cacheline_size = 64; 
uint32_t num_lines_l1 = 512; 
uint32_t num_lines_l2 = 4096;
uint32_t num_lines_l3 = 32768;
uint32_t latency_c1 = 4;
uint32_t latency_c2 = 12;
uint32_t latency_c3 = 32;
uint8_t mapping_strategy = 1;
uint32_t cycles = 1000;
char* tracefile_path = "";
char* inputfile_path = "";
FILE* trace_file;
FILE* in_file;

// Flags for error checking
uint8_t cache_level_flag = 0;
bool l1_lines;
bool l2_lines;
bool l3_lines;
bool l1_latency;
bool l2_latency;
bool l3_latency;
bool tracefile_present;
bool inputfile_present;

// Method declarations
void print_usage(char* name);
bool is_power_of_two(uint32_t n);
void print_power_two_error(char const* component);

extern struct Result run_simulation(uint32_t, const char*, uint8_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint8_t, uint32_t, struct Request*);

int main(int argc, char** argv)
{
    // Check that number of input are correct
    if (argc < 2 || argc > 24)
    {
        print_usage(argv[0]);
        return 1;
    }

    // Parse command line option
    int opt;
    const struct option long_option[] = {
        {"help", no_argument, 0, 'h'},
        {"num-cache-levels", required_argument, 0, 'a'},
        {"cacheline-size", required_argument, 0, 'b'},
        {"num-lines-l1", required_argument, 0, 'c'},
        {"num-lines-l2", required_argument, 0, 'd'},
        {"num-lines-l3", required_argument, 0, 'e'},
        {"latency-cache-l1", required_argument, 0, 'f'},
        {"latency-cache-l2", required_argument, 0, 'g'},
        {"latency-cache-l3", required_argument, 0, 'i'},
        {"mapping-strategy", required_argument, 0, 'j'},
        {"cycles", required_argument, 0, 'k'},
        {"tf", required_argument, 0, 'l'},
    };

    while ((opt = getopt_long(argc, argv, "ha:b:c:d:e:f:g:i:j:k:l:", long_option, NULL)) != -1)
    {
        switch (opt)
        {
            case 'h':
                print_usage(argv[0]);
                return 0;

            case 'a':
                num_cache_levels = (uint8_t) atoi(optarg);
                if (num_cache_levels < 1 || num_cache_levels > 3)
                {
                    printf("%s", NUM_CACHE_ERROR);
                    return 1;
                }
                cache_level_flag = num_cache_levels;
                break;

            case 'b':
                cacheline_size = (uint32_t) atoi(optarg);
                if (!is_power_of_two(cacheline_size))
                {
                    print_power_two_error("Size of cache line");
                    return 1;
                }
                break;

            case 'c':
                num_lines_l1 = (uint32_t) atoi(optarg);
                if (!is_power_of_two(num_lines_l1))
                {
                    print_power_two_error("Number of L1 cache lines"); 
                    return 1;
                }
                l1_lines = true;

                break;
            
            case 'd':
                num_lines_l2 = (uint32_t) atoi(optarg);
                if (!is_power_of_two(num_lines_l2))
                {
                    print_power_two_error("Number of L2 cache lines"); 
                    return 1;
                }
                l2_lines = true;

                break;
            
            case 'e':
                num_lines_l3 = (uint32_t) atoi(optarg);
                if (!is_power_of_two(num_lines_l3))
                {
                    print_power_two_error("Number of L3 cache lines"); 
                    return 1;
                }
                l3_lines = true;

                break;

            case 'f':
                latency_c1 = (uint32_t) atoi(optarg);
                l1_latency = true;
                break;
            
            case 'g':
                latency_c2 = (uint32_t) atoi(optarg);
                l2_latency = true;
                break;

            case 'i':
                latency_c3 = (uint32_t) atoi(optarg);
                l3_latency = true;
                break;

            case 'j':
                mapping_strategy = (uint8_t) atoi(optarg);
                if (!is_power_of_two(mapping_strategy))
                {
                    print_power_two_error("Set associativity"); 
                    return 1;
                }
                
                break;
            case 'k':
                cycles = (uint32_t) atoi(optarg);

                break;
                
            case 'l':
                tracefile_path = optarg;
                tracefile_present = true;
                break;
        }
    }

    // Handle positional argument: input_file
    if (optind < argc)
    {
        inputfile_path = argv[optind];
        inputfile_present = true;
        if (optind != argc - 1)
        {
            printf("ERROR: Please provide only one input file path\n");
            return 1;
        }
    }

    // Perform checking if inputs are valid
    
    // 1. check number of cache lines match input
    if (cache_level_flag == 1) 
    {
        // Users should not input l2 l3 data if specifying there is only one cache level
        if (l2_lines || l3_lines || l2_latency || l3_latency)
        {
            printf("ERROR: you specified that there are 1 cache levels. please only provide arguments for L1\n");
            return 1;
        }
    }
    else if (cache_level_flag == 2)
    {
        // Users should not input l3 data if specifying there is only two cache level
        if (l3_lines || l3_latency)
        {
            printf("ERROR: you specified that there are 2 cache levels. Please only provide arguments for L1 and L2\n");
            return 1;
        }
        // number of lines in l1 should be strictly smaller than number of lines in l2
        if (num_lines_l1 >= num_lines_l2)
        {
            printf("ERROR: number of cache lines must be: L1 < L2\n");
            return 1;
        }
        // latency of l1 should be strictly less than latency of l2
        if (latency_c1 >= latency_c2)
    {
        printf("ERROR: latency must be: L1 < L2\n");
        return 1;
    }
    }
    else 
    {
        // number of lines l1 < l2 < l3
        if (num_lines_l1 >= num_lines_l2 || num_lines_l2 >= num_lines_l3)
        {
            printf("ERROR: number of cache lines must be: L1 < L2 < L3\n");
            return 1;
        }
        // latency l1 < l2 < l3
        if (latency_c1 >= latency_c2 || latency_c2 >= latency_c3)
        {
            printf("ERROR: latency must be: L1 < L2 < L3\n");
            return 1;
        }
    }

    // test if associativity divides the number of lines
    if (mapping_strategy > num_lines_l1)
    {
        printf("ERROR: set associativity must divide number of line in l1\n");
        return 1;
    }

    // test can open input file
    if (inputfile_present)
    {
        in_file = fopen(inputfile_path, "r");
        if (in_file == NULL)
        {
            printf("Error: cannot open input file\n");
            return 1;
        }
    }
    else 
    {
        printf("Error: please provide input file\n");
        return 1;
    }

    // read input csv file
    // get number of lines
    uint32_t numRequests = 0;
    int ch;
    int prev='\0';
    while ((ch=fgetc(in_file))!=EOF) {
        if (ch=='\n'){
            numRequests+=1;
        }
        prev = ch;
    }
    if (prev!='\n' && prev!='\0') {
        numRequests+=1;
    }

    numRequests-=1;
    if(numRequests<0){
        printf("Error: invalid CSV content\n");fclose(in_file);
        return 1;
    }
    rewind(in_file);
    
    struct Request *requests = malloc(numRequests * (sizeof(struct Request)));
    if(requests==NULL){
        printf("Error: CSV memory allocation failed\n");fclose(in_file);
        return 1;
    }
    char line[1024];
    char typeColumn[16];
    char addressColumn[16];
    char dataColumn[16];
    char *rest;

    // header handling
    if(fgets(line, sizeof(line), in_file)!=NULL){
        line[strcspn(line,"\r\n")]=0;
        
        rest = line; 
        int len;
        char *c1 = strchr(rest, ',');
        if(!c1){
            printf("Error: invalid CSV header content (Header does not have 3 Columns)\n");fclose(in_file);free(requests);
            return 1;
        }
        len = (c1-rest)<15 ? (c1-rest) : 15;
        strncpy(typeColumn,rest,len);typeColumn[len]='\0';
        
        rest = c1+1;
        char *c2 = strchr(rest, ',');
        if(!c2){
            printf("Error: invalid CSV header content (Header does not have 3 Columns)\n");fclose(in_file);free(requests);
            return 1;
        }
        len = (c2-rest)<15 ? (c2-rest) : 15;
        strncpy(addressColumn,rest,len);addressColumn[len]='\0';
        
        rest = c2+1;
        char *c3 = strchr(rest, '\0');
        len = (c3-rest)<15 ? (c3-rest) : 15;
        strncpy(dataColumn,rest,len);dataColumn[len]='\0';

        if(strcmp(typeColumn,"Type")!=0 || strcmp(addressColumn,"Address")!=0 || strcmp(dataColumn,"Data")!=0){
            printf("Error: invalid CSV header content\n");fclose(in_file);free(requests);
            return 1;
        }
    }
    else {
        printf("Error: invalid CSV header content (header does not exist)\n");fclose(in_file);free(requests);
        return 1;
    }

    // data handling
    for(int lineNum=0;lineNum<numRequests;lineNum+=1){
        if(fgets(line, sizeof(line), in_file)==NULL){
            printf("Error: invalid CSV content\n");fclose(in_file);free(requests);
            return 1;
        }
        
        line[strcspn(line,"\r\n")]=0;
        
        rest = line; 
        int len;
        char *c1 = strchr(rest, ',');
        if(!c1){
            printf("Error: invalid CSV content (Line does not have 3 Columns on line %d)\n", lineNum);fclose(in_file);free(requests);
            return 1;
        }
        len = (c1-rest)<15 ? (c1-rest) : 15;
        strncpy(typeColumn,rest,len);typeColumn[len]='\0';
        
        rest = c1+1;
        char *c2 = strchr(rest, ',');
        if(!c2){
            printf("Error: invalid CSV content (Line does not have 3 Columns on line %d)\n", lineNum);fclose(in_file);free(requests);
            return 1;
        }
        len = (c2-rest)<15 ? (c2-rest) : 15;
        strncpy(addressColumn,rest,len);addressColumn[len]='\0';
        
        rest = c2+1;
        char *c3 = strchr(rest, '\0');
        len = (c3-rest)<15 ? (c3-rest) : 15;
        strncpy(dataColumn,rest,len);dataColumn[len]='\0';

        // read operation
        if(strcmp(typeColumn,"R")==0){
            requests[lineNum].w=0;
            
            char *endptr;
            requests[lineNum].addr=(uint32_t)strtoul(addressColumn,&endptr,0);
            if(*endptr!='\0' || endptr==addressColumn){
                printf("Error: invalid CSV content (Cannot parse address in read on line %d)\n", lineNum);fclose(in_file);free(requests);
                return 1;
            }

            if(strcmp(dataColumn,"")!=0){
                printf("Error: invalid CSV content (Data in read is not empty on line %d)\n", lineNum);fclose(in_file);free(requests);
                return 1;
            }
            requests[lineNum].data=0;
        }
        //write operation
        else if(strcmp(typeColumn,"W")==0){
            requests[lineNum].w=1;
            
            char *endptr;
            requests[lineNum].addr=(uint32_t)strtoul(addressColumn,&endptr,0);
            if(*endptr!='\0' || endptr==addressColumn){
                printf("Error: invalid CSV content (Cannot parse address in write on line %d)\n", lineNum);fclose(in_file);free(requests);
                return 1;
            }

            requests[lineNum].data=(uint32_t)strtoul(dataColumn,&endptr,0);
            if(*endptr!='\0' || endptr==dataColumn){
                printf("Error: invalid CSV content (Cannot parse data in write on line %d)\n", lineNum);fclose(in_file);free(requests);
                return 1;
            }
        }
        else {
            printf("Error: invalid CSV content (Invalid type on line %d)\n", lineNum);fclose(in_file);free(requests);
            return 1;    
        }
    }
    fclose(in_file);

    struct Result res = run_simulation(
        cycles,
        tracefile_path,
        num_cache_levels,
        cacheline_size,
        num_lines_l1,
        num_lines_l2,
        num_lines_l3,
        latency_c1,
        latency_c2,
        latency_c3,
        mapping_strategy,
        numRequests,
        requests);
    
    printf("Number of Cycles: %d\nNumber of Misses: %d\nNumber of Hits: %d\n",res.cycles,res.misses,res.hits);

    free(requests);
    return 0;
}

void print_usage(char* prog_name)
{
    printf("usage: %s\n%s", prog_name, USAGE);
}

bool is_power_of_two(uint32_t n)
{
    if (n == 0)
    {
        return true;
    }
    uint32_t i = 1;
    while (i < n)
    {
        i *= 2;
    }
    return i == n;
}

void print_power_two_error(char const* component)
{
    printf("ERROR: %s must be a power of two\n", component);
}