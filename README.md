# System Programming

- This repository contains my homeworks for GTU System Programming Spring 2022.

## HW1: File I/O
- The goal is to develop a simple text editor accomplishing a common task: string replacement.
- The program might be executed as multiple instances with different replacement commands, on the same file. This will result in multiple processes manipulating the same input file with potentially wrong results. You must protect file access by making sure than no second instance of the program runs on the same file, before the first one has finished its task with it.
- Low-level system calls and not C library functions.

## HW2: Process creation, waiting and signals
- The goal is to have a process read an input file, interpret its contents as coordinates, and forward them for calculations to children processes. Then it will collect their outputs and make a final calculation.

## HW3: Semaphores
- The goal is to prepare a small bakers simulation inspired by a classic synchronization problem.
- Implemented the program as the chefs and the wholesaler in the form of 6+1 processes that print on screen their activities.
- Implemented two version of the program, one solving it with named semaphores and another solving it with unnamed semaphores.

## HW4: Threads and System V Semaphores
- There is one supplier thread and multiple consumer threads. The supplier brings materials, one by one. And the consumers consume them, two by two. Each actor is modeled by its own thread.

## HW5: Threads Synchronization
- The goal is to use POSIX threads to parallelize a couple of simple mathematical tasks.
- First, they will calculate C=AxB in a parallel fashion.
- Once C has been calculated, then the threads will switch to the second task and calculate the 2D Discrete Fourier Transform of C, the same way as before. 
- Once they have all finished, the process will collect the outputs of each thread and write them to the output file (relative or absolute path), in CSV format. 

