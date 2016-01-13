#ifndef COMMON_H
#define COMMON_H

#define PAGE_SIZE 4096

#define debug_log(...) do {   \
        if(DEBUG) printf(__VA_ARGS__); \
    } while (0)

//#define WINDOWS
#define LINUX

#define LINUX3_14_8
//#define LINUX2_6_30
//#define LINUX2_6_38_8
//#define WINDOWS7

#define TEXT_REL  "../linux-2.6.30/text_rel" //obsolete

#ifdef LINUX3_14_8
#define DATA_BASE 0xc08bf000
#define DATA_OFFSET 0x7c0000
#define DATA_SIZE 0x05d4c0
#endif

#ifdef LINUX2_6_30
#define KERNEL_IMAGE "../linux-2.6.30/vmlinux-2.6.30"
#define MEM_SNAPSHOT "../mem-2.6.30"
#endif

#ifdef LINUX2_6_38_8
#define KERNEL_IMAGE "/home/cs3612/kernelSrc/linux-2.6.38.8/vmlinux"
#define MEM_SNAPSHOT "../mem-2.6.38.8"
#endif

#ifdef WINDOWS7 
#define TEXT_BASE 0xc1000000
#define TEXT_OFFSET 0x800
//#define TEXT_SIZE 0x1136d1
#define TEXT_SIZE 0x800
#define KERNEL_IMAGE "/home/cs3612/derandomization/win7/ntoskrnl.exe"
#define MEM_SNAPSHOT "/home/cs3612/derandomization/win7/mem-win7-716"
#endif

struct rel_entry{
    unsigned off; //offset
    unsigned val; //value
    rel_entry(unsigned o, unsigned v):off(o),val(v){};
};


void write_log(int seq, int total, int size_page, float diff_ratio);
void write_per_log(float match, float gen);

#endif
