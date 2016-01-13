Derandomization
============

Introduction
------------

Derandomization is a tool for derandomize OS kernel. It runs on
a 32-bit Linux host. 

Compile
-----------

    cd src
    make

Usage
-----------
1. Derandomize a snapshot by the brute force approach

        ./maldives -a memory-snapshot kernel-image

2. Derandomize a snapshot by the no-patch-code approach

        ./maldives -n memory-snapshot kernel-image

3. Derandomize a snapshot by the patch-code approach

        ./maldives -g memory-snapshot kernel-image

4. Derandomize a snapshot by the readonly-pointer approach

        ./maldives -o memory-snapshot kernel-image

