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

void dump_rel_rodata(char *image_name){
    string cmd;
    cmd += "readelf -r ";
    cmd += image_name;
    cmd += " > rel_entries";
    system(cmd.c_str());
    cmd = "./rel_rodata.py rel_entries rel_text";
    system(cmd.c_str());
}

extern rel_entry new_rel_entry(unsigned off, unsigned val);

vector<rel_entry> rel_rodata_sig_build(char *image_name, unsigned rodata_base){
    dump_rel_rodata(image_name);
    vector<rel_entry>  entries;
    //read files
    fstream sigfile;
    sigfile.open("rel_text");
    unsigned off, val;
    while (sigfile >> hex >> off >> hex >> val) {
        entries.push_back(new_rel_entry(off - rodata_base, val));
    }

    return entries;
}

void get_rodata_section_info(char * image_name, unsigned &offset, unsigned &size, unsigned &base){
    string cmd;
    cmd += "readelf -S ";
    cmd += image_name;
    cmd += " > section_headers";
    system(cmd.c_str());
    cmd = "./get_rodata_section.py section_headers > rodata_section";
    system(cmd.c_str());

    fstream infile;
    infile.open("rodata_section");
	
    infile >> hex >> base >> hex >> offset >> hex >> size;
}

void rel_rodata_scan(char * elf_image, char *image_name, char *mem_image, unsigned mem_size){
    auto begin = std::chrono::high_resolution_clock::now();

    unsigned rodata_offset, rodata_size, rodata_base;
    get_rodata_section_info(image_name, rodata_offset, rodata_size, rodata_base);

    vector<rel_entry> entries = rel_rodata_sig_build(image_name, rodata_base);
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

        char *pattern_page =  (char *) ((unsigned) elf_image + rodata_offset); 

        int offset = -1;
        int match = 0;
        int mismatch= 0;
        for(auto& entry: entries){
            unsigned value = *((unsigned *) (page + entry.off));
            unsigned value_pattern = *((unsigned *) (pattern_page + entry.off));
            unsigned rand_off  = abs(value - value_pattern);
            if(offset == -1){
                offset = rand_off;
                match = 1;
                //debug_log("%x: s: %x m: %x offset: %x, first\n",
                //          entry.off, value, value_pattern, rand_off);
            }else if(offset == rand_off){
                match++;
                debug_log("%x: s: %x m: %x offset: %x, match\n",
                          entry.off, value, value_pattern, rand_off);
            }else{
                // debug_log("%x: s: %x m: %x offset: %x, mismatch\n",
                //          entry.off, value, value_pattern, rand_off);
                mismatch++;
            }

            //search at least one page
            if(mismatch >= match && entry.off > PAGE_SIZE) break;
        }

        if(match > mismatch && match > entries.size()/2){
            int total = entries.size() * 4;
            int size_page = total/ (rodata_size/PAGE_SIZE);
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
