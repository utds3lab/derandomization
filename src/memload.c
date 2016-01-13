/*
    memload.c

    Memory image loading module

    Yufei Gu
    Copyright : Copyright 2012 by UTD. All rights reserved. This material may
    be freely copied and distributed subject to inclusion of this
    copyright notice and our World Wide Web URL http://www.utdallas.edu
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "common.h"

extern int DEBUG;

void mem_free(char *mem)
{
    if (mem)
        free(mem);
}

char * mem_load(char const *name, unsigned *isize)
{
    struct stat fstat;
    int ret;
    char *mem = NULL;
    unsigned size;

    if (stat(name, &fstat) != 0)
    {
        printf("No snapshot : %s\n", name);
        exit(1);
    }

    size = fstat.st_size;

    mem = (char *)malloc(size);
    if (mem == NULL)
        return NULL;

    FILE *f = fopen(name, "r");
    ret = fread(mem, 1, size, f);
    fclose(f);
    *isize = size;

    if (ret == size){
        debug_log("File %s is loaded, %d bytes.\n", name, ret);
    }else{
        free(mem);
        return NULL;
    }

    return mem;
}
