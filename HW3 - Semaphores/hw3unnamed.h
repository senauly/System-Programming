#ifndef HW3UNNAMED_H
#define HW3UNNAMED_H

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>


#define MAX_CHILDREN 6
#define INGREDIENT_COUNT 4
#define SHARED_MEMORY_NAME "/shared_memory"

#define NO_EINTR(stmt) while((stmt) < 0 && errno == EINTR);

typedef struct SharedMemory{
    char ingr[2];
    int isMilk;
    int isFlour;
    int isWalnut;
    int isSugar;
    sem_t salerSem;
    sem_t milk;
    sem_t flour;
    sem_t walnut;
    sem_t sugar;
    sem_t m;
    sem_t c0;
    sem_t c1;
    sem_t c2;
    sem_t c3;
    sem_t c4;
    sem_t c5;
}SharedMemory;

int initSharedMemory(SharedMemory **sharedMemory);
void initSemaphores(SharedMemory *sharedMemory);
void chef0(SharedMemory *sharedMemory);
void chef1(SharedMemory *sharedMemory);
void chef2(SharedMemory *sharedMemory);
void chef3(SharedMemory *sharedMemory);
void chef4(SharedMemory *sharedMemory);
void chef5(SharedMemory *sharedMemory);
void pusherMilk(SharedMemory *sharedMemory);
void pusherFlour(SharedMemory *sharedMemory);
void pusherWalnut(SharedMemory *sharedMemory);
void pusherSugar(SharedMemory *sharedMemory);
void getIngredients(SharedMemory *sm, int n);

#endif