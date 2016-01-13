#include <vector>
#include <stdio.h>
#include <iostream>
#include <algorithm>
#include <map>
#include <climits>
#include <chrono>
#include <fstream>
#include "common.h"
using namespace std;

extern int DEBUG;
extern unsigned TEXT_ADDR, TEXT_OFFSET, TEXT_SIZE;

void dump_rel_data(char *image_name){
    string cmd;
    cmd += "readelf -r ";
    cmd += image_name;
    cmd += " > rel_entries";
    system(cmd.c_str());
    cmd = "./rel_data.py rel_entries rel_text";
    system(cmd.c_str());
}

extern rel_entry new_rel_entry(unsigned off, unsigned val);

vector<rel_entry> rel_data_sig_build(char *image_name){
    dump_rel_data(image_name);
    vector<rel_entry>  entries;
    //read files
    fstream sigfile;
    sigfile.open("rel_text");
    unsigned off, val;
    while (sigfile >> hex >> off >> hex >> val) {
        entries.push_back(new_rel_entry(off - TEXT_ADDR, val));
    }

    return entries;
}

extern char *get_pattern_page(char *elf_image, unsigned block_index, unsigned block_size);

void rel_data_scan(char * elf_image, char *image_name, char *mem_image, unsigned mem_size){
    auto begin = std::chrono::high_resolution_clock::now();

    vector<rel_entry> entries = rel_data_sig_build(image_name);
    if(entries.size() == 0){
        puts("No input signature.");
        return;
    }else{
        debug_log("%d rel entries\n", entries.size());
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

        int offset = -1;
        int match = 0;
        int mismatch= 0;
        for(auto& entry: entries){
            int value = *((unsigned *) (page + entry.off));
            int value_pattern = *((unsigned *) (pattern_page + entry.off));
            int tmp  = abs(value - value_pattern);
            if(offset == -1){
                offset = tmp;
                match = 1;
            }else if(offset == tmp){
                match++;
                debug_log("%x: s: %x m: %x offset: %x\n",
                          entry.off, value, value_pattern, tmp);
            }else{
                if(match > 1)
                    debug_log("%x: s: %x m: %x offset: %x\n",
                              entry.off, value, value_pattern, tmp);
                mismatch++;
            }

            //search at least one page
            if(mismatch >= match && entry.off > PAGE_SIZE) break;
        }

        if(match > mismatch && match > entries.size()/2){
            int total = entries.size() * 4;
            int size_page = total/ (TEXT_SIZE/PAGE_SIZE);
            float diff_ratio = (float)mismatch/entries.size() ;
            printf("%d paddr %x, total %d bytes, diff ratio %f, bytes/page %d\n",
                   seq, page - mem_image, total, diff_ratio, size_page );
            
            write_log(seq, total, size_page, diff_ratio);
            seq++;
        }
    }

    double sig_match_time= std::chrono::duration_cast<std::chrono::nanoseconds>(chrono::high_resolution_clock::now() - begin).count()/(double)1000000000;
    printf("gen %lf, match %lf\n", sig_gen_time, sig_match_time);
    write_per_log(sig_gen_time, sig_match_time);

}
