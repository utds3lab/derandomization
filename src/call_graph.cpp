#include <vector>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <map>
#include <climits>
#include "common.h"
using namespace std;

extern int DEBUG;

extern unsigned TEXT_ADDR, TEXT_OFFSET, TEXT_SIZE;

struct graph_node{
    unsigned addr;
    map<int, graph_node *> children;
    graph_node(unsigned x): addr(x){};
};

graph_node * get_newnode(map<int, graph_node*> &allnodes, unsigned addr){
    if(allnodes.find(addr) != allnodes.end())
        return allnodes[addr];
    else
        return new graph_node(addr);
}

void call_graph_build_old(char *image_name, vector<graph_node *> &nodes){
    map<int, graph_node*> allnodes;
    graph_node * newnode = new graph_node(0xc0100000);
    allnodes[0xc1000000] = newnode;
    
    newnode->children[0x145] = get_newnode(allnodes, 0xc0100000);
    newnode->children[0x33f] = get_newnode(allnodes, 0xc0683000);
    newnode->children[0x384] = get_newnode(allnodes, 0xc03c1000);
    newnode->children[0x3c5] = get_newnode(allnodes, 0xc03bf000);

    nodes.push_back(newnode);
}


void dump_call_entries(char *image_name){
    string cmd;
    cmd += "objdump -d ";
    cmd += image_name;
    cmd += " | grep 'e8 .*call' | cut -c-8 > call_instructions";
    system(cmd.c_str());
}

vector<unsigned> call_graph_build(char *image_name){
    dump_call_entries(image_name);
    fstream infile;
    infile.open("call_instructions");
    vector<unsigned> offsets;
    unsigned offset;
    while(infile >>hex>> offset){
        offsets.push_back(offset - TEXT_ADDR); 
    }
    return offsets;
}

extern char *get_pattern_page(char *elf_image, unsigned block_index, unsigned block_size);

void call_graph_scan(char *elf_image, char *image_name, char *mem_image, unsigned mem_size){
    auto begin = std::chrono::high_resolution_clock::now();

    vector<unsigned> offsets = call_graph_build(image_name);
    if(offsets.size() <= 0) {
        puts("No input signature.");
        return;
    }

    double sig_gen_time= std::chrono::duration_cast<std::chrono::nanoseconds>(chrono::high_resolution_clock::now() - begin).count()/(double)1000000000;
    begin = std::chrono::high_resolution_clock::now();

    //page by page
    unsigned page_no = mem_size / PAGE_SIZE;
    int seq = 0;
    for(long i = 0; i < page_no; i++){
        char *page = 
            (char *) ((unsigned) mem_image + i * PAGE_SIZE);

        char *pattern_page = get_pattern_page(elf_image, 0, PAGE_SIZE);

        int match = 0;
        int mismatch = 0;
        int not_e8 = 0;
        for(auto offset : offsets){
            if((page[offset] & 0xff) == 0xe8){
                int value = *((unsigned *) (page + offset + 1 ));
                int value_pattern = *((unsigned *) (pattern_page + offset + 1));
                if(value == value_pattern){
                    match++;
                }else{
                    //printf("%x %x: %x, %x\n", page - mem_image, offset, value, value_pattern);
                    mismatch++;
                }
            }else{
                not_e8++; 
            }

            if(mismatch >= match) break; 
        }

        if(match > mismatch){
            int total =  offsets.size() * 4;
            int size_page =  total/(TEXT_SIZE/PAGE_SIZE);
            float diff_ratio = (float)(mismatch + not_e8)/offsets.size(); 
            printf("%d paddr %x, total %x bytes, %x, diff ratio %f\n", seq, page - mem_image,
                  total, not_e8, (float)(mismatch + not_e8)/offsets.size());
             write_log(seq, total, size_page, diff_ratio);
            seq++;
        }
            
    }
    
    double sig_match_time= std::chrono::duration_cast<std::chrono::nanoseconds>(chrono::high_resolution_clock::now() - begin).count()/(double)1000000000;
    printf("gen %lf, match %lf\n", sig_gen_time, sig_match_time);
    write_per_log(sig_gen_time, sig_match_time);
}
