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

struct no_patch_node{
    unsigned offset;
    int len;
    no_patch_node(unsigned x, int l): offset(x), len(l){};
};

void dump_no_patch(char *image_name){
    string cmd;
    cmd += "objdump -d ";
    cmd += image_name;
    cmd += " > objdump";
    system(cmd.c_str());

    cmd = "./no_patch.py objdump > no_patch_nodes";
    system(cmd.c_str());
}

vector<no_patch_node> no_patch_build(char *image_name){
    vector<no_patch_node> nodes;
    dump_no_patch(image_name);
    fstream infile;
    infile.open("no_patch_nodes");
    unsigned offset = 0;
    int len = 0;
    while(infile>>hex>> offset){
        infile>>hex>>len;
        no_patch_node node(offset - TEXT_ADDR, len);
        nodes.push_back(node);
    }
    return nodes;
}

extern char *get_pattern_page(char *elf_image, unsigned block_index, unsigned block_size);

void no_patch_scan(char *elf_image, char *image_name, char *mem_image, unsigned mem_size){
    auto begin = std::chrono::high_resolution_clock::now();

    vector<no_patch_node> nodes = no_patch_build(image_name);
    if(nodes.size() <= 0) {
        puts("No input signature.");
        return;
    }
    int total_bytes = 0;
    for(auto & node: nodes)
        total_bytes += node.len;

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
        for(auto& node: nodes){
            bool equal = true;
            for(int j = 0; j < node.len; j++){
                char value = *((char *) (page + node.offset ));
                char value_pattern = *((char *) (pattern_page + node.offset));
                if(value != value_pattern){
                    mismatch++;
                    equal = false;
                    break;
                }
            }

            if(equal) match++;

            //search at least one page
            if(mismatch >= match && node.offset > PAGE_SIZE) break; 
        }

        if(match > mismatch){
            float diff_ratio = (float)(mismatch)/(mismatch + match);
            int size_page = total_bytes/(TEXT_SIZE/PAGE_SIZE); 
            printf("%d paddr %x, total %d bytes, diff ratio %f, bytes/page %d\n",
                   seq, page - mem_image, total_bytes, diff_ratio, size_page);
            write_log(seq, total_bytes, size_page, diff_ratio);
            seq++;
        }
    }
    
    double sig_match_time= chrono::duration_cast<chrono::nanoseconds>(chrono::high_resolution_clock::now() - begin).count()/(double)1000000000;
    printf("gen %lf, match %lf\n", sig_gen_time, sig_match_time);
    write_per_log(sig_gen_time, sig_match_time);
}
