/*
 ============================================================================
 Name        : get_pgd.c
 Author      : Yufei Gu
 Version     :
 Copyright   : Copyright 2012 by UTD. all rights reserved. This material may
 be freely copied and distributed subject to inclusion of this
 copyright notice and our World Wide Web URL http://www.utdallas.edu
 Description : get pgd address from any mem
 ============================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <sys/times.h>
#include <stdint.h>

int real_pgd;

//determine whether two memory match each other,
//if match rate less and equal to 0.02 then return 0 (Match)
//else return 1 (No match)
int isEqual(char *mem, unsigned paddr1, unsigned paddr2, int length)
{
    int i;
    int noMatchCount = 0;
    unsigned value1s[512];
    unsigned value2s[512];
    int indexs[512];
    for (i = 0; i < length; i = i + 4) {
        unsigned value1 = *(unsigned *) ((unsigned) mem + paddr1 + i);
        unsigned value2 = *(unsigned *) ((unsigned) mem + paddr2 + i);
        if (value1 != value2) {
            value1s[noMatchCount] = value1;
            value2s[noMatchCount] = value2;
            indexs[noMatchCount] = i;
            noMatchCount++;
        }
    }
    float matchRate = (float) noMatchCount / (float) (length / 4);

    //rate less than 0.02
    if (matchRate <= 0.02) {
//              printf("paddr1:0x%x vs paddr2:0x%x noMatchcount:%d rate:%4.2f\n", paddr1, paddr2,
//                              noMatchCount, (float) noMatchCount / (float) (length / 4));
//              for (i = 0; i < noMatchCount; i++) {
//                      printf("0x%x: %x %x \n", indexs[i]+2048, value1s[i], value2s[i]);
//              }
        return 0;
    } else
        return 1;
}

//print pde item in the pgd
void printPdeItem(unsigned startAddr, char *mem, int mem_size)
{
    int j;
    //scan high 2k, from 0x800
    j = 2 * 1024;
    int pdeItemNo = 0;
    for (; j < 4 * 1024; j = j + 4) {
        unsigned pgdCandidate =
            *(unsigned *) ((unsigned) mem + startAddr + j);
        if ((pgdCandidate & ~0xfff) > mem_size)
            continue;

        //present,first bit;readonly, second bit, accessed, sixth bit; global, ninth bit.
        //linux 1e3, winxp 163, win7 63, freebsd 63,so get their common part: 63
        //but, solaris 27
        //              if ((pgdCandidate & 0xf) == 0x3 && (pgdCandidate & 0x60) == 0x60) {
        if ((pgdCandidate & 0x3) == 0x3 && (pgdCandidate & 0x20) == 0x20
            && (pgdCandidate & 0x618) == 0) {
            pdeItemNo++;
        }
    }
    printf("pde item no: %d\n", pdeItemNo);
}

void printAllRealPgd(int pgdcount, int countedpages[],
                     int maxIndex, int potengtialPdgArray[],
                     int pageSize)
{
     
      int i;
      //printf all real pgds
      for (i = 0; i < pgdcount; i++) {
              if (countedpages[i] == maxIndex) {
                      printf("pgd physical address:0x%x\n",
                                      potengtialPdgArray[i] * pageSize);
              }
      }
}

//get real pgd from potential pgds
unsigned getPgdReal(int pgdcount, int potential_PDGs[],
                    int pageSize, char *mem)
{
    //get real pgd page by compare all field of 3*1024 to 4*1024
    unsigned pgdPhyAddr = 0;

    //array to record match number,initiate all to 0
    int matchNumber[pgdcount];
    int i;
    for (i = 0; i < pgdcount; i++) {
        matchNumber[i] = 0;
    }

    //array to record the page which have been count as one of the same pages
    //if i is handled, set countedpages[i] by the first same index, else countedpages[i] is -1
    int countedpages[pgdcount];
    for (i = 0; i < pgdcount; i++) {
        countedpages[i] = -1;
    }

    //find the max match, and the physical address of pgd
    for (i = 0; i < pgdcount; i++) {
        //if page i is handled, do nothing
        if (countedpages[i] >= 0)
            continue;

        int j;
        //compare i and i+1,i+2...pgdcount-1
        for (j = i + 1; j < pgdcount; j++) {
            //if page j is handled, do nothing
            if (countedpages[j] >= 0)
                continue;

            //compare i and j
            int startIndex = 2;

            unsigned paddr1 =
                potential_PDGs[i] * pageSize + startIndex * 1024;

            unsigned paddr2 =
                potential_PDGs[j] * pageSize + startIndex * 1024;

            int ret =
                isEqual(mem, paddr1, paddr2, (4 - startIndex) * 1024);
            if (ret == 0) {
                matchNumber[i]++;
                countedpages[j] = i;
            }
        }
    }

    //if current match number is bigger than max Match number,
    //then update max Match number, and record the phycial address
    int maxMatch = 0;
    int maxIndex = 0;
    for (i = 0; i < pgdcount; i++) {
        if (matchNumber[i] > maxMatch) {
            maxMatch = matchNumber[i];
            maxIndex = i;
        }
    }

    pgdPhyAddr = potential_PDGs[maxIndex] * pageSize;

    //get the third pgd, because the first may not include the modules
    int count = 0;
    for (i = 0; i < pgdcount; i++) {
        if (countedpages[i] == maxIndex) {
            count ++;
            if(count == 3)
                pgdPhyAddr  = potential_PDGs[i] * pageSize;
        }
    }

    //printf all real pgds
//    printAllRealPgd(pgdcount, countedpages, maxIndex, potential_PDGs,
//                        pageSize);

    real_pgd = maxMatch + 1;
    printf("Real PGD Number is %d, PGD physical address:0x%x\n",
           maxMatch + 1, pgdPhyAddr);


    return pgdPhyAddr;
}

//check whether pte is valid, a real pte
int isPteValid(unsigned pteStart, char *mem, int mem_size)
{
    int isValid = 1;
    //    printf("pte Start: %x\n",  pteStart);
    int k = 0;
    int matchCount = 0;
    for (; k < 4 * 1024; k += 4) {
        unsigned pte = *(unsigned *) ((unsigned) mem + pteStart + k);

        //if (pte != 0)  printf("pte:%x\n",pte);
        if ((pte & 0x1) == 0x1 && (pte & ~0xfff) > mem_size) {
            continue;
        }
        if (pte == 0)
            continue;

        int us = pte & (1 << 2);
        int g = pte & (1 << 8);
        if ((pte & 0x1) == 0x1 && us == 0 && g == 256) {
            matchCount++;
        } else {
            isValid = 0;
            break;
        }
    }

    if (matchCount == 0)
        isValid = 0;

    return isValid;
}

/* get potential pgd page
 * return number of potential pgd*/
int getPotentialPgd(int totalPageNumber, int pageSize, char *mem,
                    int pageIndex[], int mem_size)
{
    int pgdcount = 0;
    int i;
    for (i = 0; i < totalPageNumber; i++) {
        int startAddr = i * pageSize;
        printf("\n%x\n", startAddr);
        int isPgd = 1;
        int j;
        for (j = 0; j < 2 * 1024; j = j + 4) {
            unsigned pdeItem =
                *(unsigned *) (mem + startAddr + j);

            printf("pde: %x\n", pdeItem);
            if ((pdeItem & 0x418) != 0) {
                isPgd = 0;
                break;
            }
        }
        if (isPgd == 0)
            continue;

        //scan high 2k, from 0x800
        j = 2 * 1024;

        int matchCount = 0;
        for (; j < 4 * 1024; j = j + 4) {
            unsigned pdeItem =
                *(unsigned *) ((unsigned) mem + startAddr + j);
            if (pdeItem == 0 || (pdeItem & ~0xfff) > mem_size)
                continue;

            //       if ((pdeItem & 0x418) != 0) {
            //    break;
            // }


#ifdef OLD_PGD_MATCH
            //present,first bit;readonly, second bit, accessed, sixth bit; global, ninth bit.
            //linux 1e3, winxp 163, win7 63, freebsd 63,so get their common part: 63
            //but, solaris 27
            if ((pdeItem & 0x3) == 0x3 && (pdeItem & 0x20) == 0x20
                && (pdeItem & 0x618) == 0) {
                proCount++;
                //if at least 30 value is match,then put it in potential pgd
                if (proCount > 30) {
                    //this page is potential pgd
                    pageIndex[i] = 1;
                    pgdcount++;
                    break;
                }
            }
#endif
            // if ((pdeItem & 0x3) == 0x3 && (pdeItem & 0x20) == 0x20
            //   && (pdeItem & 0x418) == 0) {

            // pde signature  | x | 0 | 0 | x | x | x | x | 0 | 0 | x | x | 1 |
            if ((pdeItem & 0x1) == 0x1 && (pdeItem & 0x418) == 0) {
                //check pte
                int isValid = isPteValid(pdeItem & ~0xfff, mem, mem_size);
                if (isValid == 1)
                    matchCount++;
            } else {
                //too strong, some special case, like 003 in FreeBSD,
                //and 1e1 in Linux
              isPgd = 0;
              break;
            }
        }

        //detemine whether this page is a pgd
        if (isPgd == 1 && matchCount >= 1) {
            pageIndex[i] = 1;
            pgdcount++;
        }

    }

    printf("potential pdg count: %d\n", pgdcount);

    return pgdcount;
}


long long timeval_diff(struct timeval *difference,
                       struct timeval *end_time,
                       struct timeval *start_time)
{
    struct timeval temp_diff;

    if (difference == NULL) {
        difference = &temp_diff;
    }
//      printf("start_time tv_sec %ld,tv_usec %ld\n",start_time->tv_sec,start_time->tv_usec);
//      printf("end_time tv_sec %ld,tv_usec %ld\n",end_time->tv_sec,end_time->tv_usec);
    difference->tv_sec = end_time->tv_sec - start_time->tv_sec;
    difference->tv_usec = end_time->tv_usec - start_time->tv_usec;

    /* Using while instead of if below makes the code slightly more robust. */

    while (difference->tv_usec < 0) {
        difference->tv_usec += 1000000;
        difference->tv_sec -= 1;
    }

//      printf("tv_sec %ld,tv_usec %ld\n",difference->tv_sec,difference->tv_usec);
    return 1000000LL * difference->tv_sec + difference->tv_usec;

}

unsigned get_pgd(char *mem, int mem_size)
{
    int pageSize = 4 * 1024;
    int totalPageNumber = mem_size / pageSize; 
    int pageIndex[totalPageNumber];

    struct timeval earlier;
    struct timeval later;
    
    int pgdcount =
        getPotentialPgd(totalPageNumber, pageSize, mem, pageIndex,
                        mem_size);

//      pgdcount = getMorePotentialPgd(totalPageNumber, pageSize, mem, pageIndex,
//                      pageIndex2, mem_size);

    //generate the potential pgd index array
    int potengtialPdgArray[pgdcount];
    int i, j = 0;
    for (i = 0; i < totalPageNumber; i++) {
        if (pageIndex[i] == 1) {
            potengtialPdgArray[j] = i;
            j++;
        }
    }

    //print the potential pgd physical address
//      for (i = 0; i < pgdcount; i++) {
//              printf("pgd physical address:0x%x\n", potengtialPdgArray[i] * pageSize);
//      }

    //get real pgd page by compare all field of 3*1024 to 4*1024
    unsigned pgd = getPgdReal(pgdcount, potengtialPdgArray, pageSize, mem);

//      printPdeItem(pgd,mem,mem_size);
    return pgd;
}
