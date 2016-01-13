#include <vector>
#include <stdio.h>
#include <iostream>
#include <algorithm>
#include <map>
#include <climits>
#include <stdlib.h>
#include "common.h"
using namespace std;

extern int DEBUG;
struct global_data_node{
    unsigned offset;
    int type;
    global_data_node(unsigned x, int t): offset(x), type(t){};
};

/*determine whether a value is a valid non-empty kernel point */
bool valid_kernel_address(unsigned vaddr){
    unsigned kernel_start_addr;
#ifdef WINDOWS
    kernel_start_addr = 0x80000000;
#endif

#ifdef LINUX 
    kernel_start_addr = 0xc0000000;
#endif
   
    if (vaddr > kernel_start_addr && vaddr != 0xffffffff) {
        return true;
    }
    return false;
}


vector<global_data_node> get_global_layout(char *elf_image){
    /*
    string cmd;
    cmd += "nm ";
    cmd += KERNEL_IMAGE;
    cmd += " | grep ' D ' | sort > global_data";
    system(cmd.c_str());
    */

    vector<global_data_node> nodes;
    for(unsigned i = DATA_OFFSET; i < DATA_OFFSET + DATA_SIZE; i += 4){
        unsigned value = *((unsigned *)(elf_image + i));
        unsigned vaddr =  (i - DATA_OFFSET) + DATA_BASE; 
        //printf("%x %x ", i, vaddr);

        if(value >= DATA_BASE && value <= DATA_BASE + DATA_SIZE){
        //if(valid_kernel_address(value)){
            global_data_node node = {i - DATA_OFFSET, value - DATA_BASE};
            nodes.push_back(node);
            debug_log("%x %x: %x\n", i - DATA_OFFSET, vaddr, value - DATA_BASE);
        }
    }

    cout<<nodes.size()<<" nodes found."<<endl;
    return nodes;
}

void global_data_layout_scan(char * elf_image, char *mem_image, unsigned mem_size){
    auto nodes = get_global_layout(elf_image);
    if(nodes.size() == 0) {
        puts("No global data signature.");
        return;
    }
    return;
    //page by page
    unsigned page_no = mem_size / PAGE_SIZE;
    int max_match = 0;
    for(long i = 0; i < page_no; i++){
        char *page = 
            (char *) ((unsigned) mem_image + i * PAGE_SIZE);

        int pos_matchcount = 0;
        int node_idx = 0;
        int neg_match = 0;
        for(int j = 0; j < DATA_SIZE; j += 4){
            unsigned value = *((unsigned *)page + j);
            if(j == nodes[node_idx].offset){
                if(valid_kernel_address(value))
                    pos_matchcount++;
                node_idx++;
            }else{
                if(!valid_kernel_address(value))
                    neg_match++;
            }
        }

        /*
        int misscount = 0;
        for(auto &node: nodes){
            unsigned value = *((unsigned *)page + node.offset);
            if(valid_kernel_address(value)){ 
                pos_matchcount++;
                //printf("%x: %x, %x\n", node.offset, value, node.type);
            }else{
                misscount++;
            }
            //if(misscount >= 100) break;
        }
        */   
        if(pos_matchcount >= 100){
            printf("Found %x, match %d neg match %d\n", page - mem_image, pos_matchcount, neg_match);
            max_match = max(max_match, pos_matchcount);
        }
    }

    cout<<"max match is "<<max_match<<endl; 
}
