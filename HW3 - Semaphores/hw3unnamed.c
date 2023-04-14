#include "hw3unnamed.h"

sig_atomic_t interruptHappened = 0;

void sigIntHandler(int sigNum){
    if(sigNum == SIGINT)
        interruptHappened = 1;
}

int main(int argc, char *argv[]){
    //get arguments from command line 
    char *inputFilePath = NULL;

    struct sigaction sigAction;
    memset(&sigAction, 0, sizeof(struct sigaction));
    sigAction.sa_handler = sigIntHandler;
    sigaction(SIGINT, &sigAction, NULL);
    
    if(strcmp(argv[1], "-i") == 0 && argc == 3){
        inputFilePath = argv[2];
    }
    else{
        fprintf(stderr, "Error: Command should be like ./hw3unnamed -i inputFilePath.\n");
        exit(EXIT_FAILURE);
    }

    SharedMemory *sm;
    if(initSharedMemory(&sm) == -1){
        exit(EXIT_FAILURE);
    }

    initSemaphores(sm);

    //create children process
    pid_t cpid[MAX_CHILDREN];
    pid_t ppid[INGREDIENT_COUNT];
    int count = 0;
    for(int i = 0; i < MAX_CHILDREN; i++){
        cpid[i] = fork();
        if(cpid[i] == 0){
            //child process
            while(1){
                if(i == 0){
                    fprintf(stdout,"chef%d (pid %d) is waiting for walnut and sugar. [%s]\n", i, getpid(),sm->ingr);
                    chef0(sm);
                }

                else if(i == 1){
                    fprintf(stdout,"chef%d (pid %d) is waiting for walnut and flour. [%s]\n", i, getpid(),sm->ingr);
                    chef1(sm);
                    
                }

                else if(i == 2){
                    fprintf(stdout,"chef%d (pid %d) is waiting for sugar and flour. [%s]\n", i, getpid(),sm->ingr);
                    chef2(sm);
                    
                }

                else if(i == 3){
                    fprintf(stdout,"chef%d (pid %d) is waiting for flour and milk. [%s]\n", i, getpid(),sm->ingr);
                    chef3(sm);
                    
                }

                else if(i == 4){
                    fprintf(stdout,"chef%d (pid %d) is waiting for walnut and milk. [%s]\n", i, getpid(),sm->ingr);
                    chef4(sm);
                    
                }   

                else if(i == 5){
                    fprintf(stdout,"chef%d (pid %d) is waiting for sugar and milk. [%s]\n", i, getpid(),sm->ingr);
                    chef5(sm);
                    
                }
                if(interruptHappened){
                    break;
                } 
                count++;
            }
           
           fprintf(stdout,"chef%d (pid %d) is exiting. [%s]\n", i, getpid(), sm->ingr);
           munmap(sm, sizeof(sm));
           return count;
        }
        if(cpid[i] < 0){
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }

    for(int i = 0; i < INGREDIENT_COUNT; i++){
        ppid[i] = fork();
        if(ppid[i] == 0){
            while(1){
                if(i == 0){
                    pusherMilk(sm);
                }

                else if(i == 1){
                    pusherFlour(sm);
                }

                else if(i == 2){
                    pusherWalnut(sm);
                }

                else if(i == 3){
                    pusherSugar(sm);
                }
                if(interruptHappened){
                    break;
                }
            }
            munmap(sm, sizeof(sm));
            exit(EXIT_SUCCESS);
        }
        if(ppid[i] < 0){
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }

    //open input file
    int fd = open(inputFilePath, O_RDONLY);
    if(fd == -1){
        perror("open");
        exit(EXIT_FAILURE);
    }

    int n;

    while(1){
        //read input file 
        char buff[3];
        NO_EINTR(n = read(fd, buff, 3*sizeof(char)));
        if(interruptHappened){
            break;
        }
        
        if(n == 0){
            break;
        }
        
        if(n == -1){
            perror("read");
            exit(EXIT_FAILURE);
        }

        //write to shared memory
        sm->ingr[0] = buff[0];
        sm->ingr[1] = buff[1];
        
        char ingredient1[10];
        char ingredient2[10];
        if(interruptHappened){
            break;
        }

        for (int i = 0; i < 2; i++)
        {
            if(buff[i] == 'M'){
                if(i == 0) strcpy(ingredient1, "milk");
                else strcpy(ingredient2, "milk");
                sem_post(&sm->milk);
            }

            if(buff[i] == 'F'){
                if(i == 0) strcpy(ingredient1, "flour");
                else strcpy(ingredient2, "flour");
                sem_post(&sm->flour);
            }

            if(buff[i] == 'W'){
                if(i == 0) strcpy(ingredient1, "walnut");
                else strcpy(ingredient2, "walnut");
                sem_post(&sm->walnut);
            }

            if(buff[i] == 'S'){
                if(i == 0) strcpy(ingredient1, "sugar");
                else strcpy(ingredient2, "sugar");
                sem_post(&sm->sugar);
            }
        }
        
        fprintf(stdout, "the wholesaler (pid %d) delivers %s and %s. [%s]\n", getpid(), ingredient1, ingredient2, sm->ingr);
        sem_post(&sm->m);
        fprintf(stdout, "the wholesaler (pid %d) is waiting for the dessert. [%s]\n", getpid(), sm->ingr);
        sem_wait(&sm->salerSem);
        if(interruptHappened){
            break;
        }
        fprintf(stdout,"the wholesaler (pid %d) has obtained the dessert and left. [%s]\n", getpid(), sm->ingr);

    }

    int status;
    int totalDessert = 0;
    //kill children process
    for(int i = 0; i < MAX_CHILDREN; i++){
        kill(cpid[i], SIGINT);
        waitpid(cpid[i], &status, 0);
        totalDessert += WEXITSTATUS(status);
    }

    //kill pushers
    for(int i = 0; i < INGREDIENT_COUNT; i++){
        kill(ppid[i], SIGINT);
        wait(NULL);
    }

    //close input file
    close(fd);
    fprintf(stdout,"the wholesaler (pid %d) is done (total desserts: %d) [%s]\n", getpid(), totalDessert, sm->ingr);

    shm_unlink(SHARED_MEMORY_NAME);
    munmap(sm, sizeof(sm));
}

int initSharedMemory(SharedMemory **sharedMemory){
    //create shared memory
    int fd;
    fd = shm_open(SHARED_MEMORY_NAME, O_RDWR | O_CREAT, 0666);
    
    if(fd < 0){
        perror("shm_open");
        return -1;
    }

    //set shared memory size
    if(ftruncate(fd, sizeof(SharedMemory)) < 0){
        perror("ftruncate");
        shm_unlink(SHARED_MEMORY_NAME);
        return -1;
    }

    //map shared memory
    *sharedMemory = mmap(NULL, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(*sharedMemory == MAP_FAILED){
        perror("mmap");
        shm_unlink(SHARED_MEMORY_NAME);
        return -1;
    }

    (*sharedMemory)->isMilk = 0;
    (*sharedMemory)->isFlour = 0;
    (*sharedMemory)->isWalnut = 0;
    (*sharedMemory)->isSugar = 0;

    return 0;
}

void initSemaphores(SharedMemory *sharedMemory){
    //initialize semaphores
    sem_init(&sharedMemory->salerSem, 1, 0);
    sem_init(&sharedMemory->milk, 1, 0);
    sem_init(&sharedMemory->flour, 1, 0);
    sem_init(&sharedMemory->walnut, 1, 0);
    sem_init(&sharedMemory->sugar, 1, 0);
    sem_init(&sharedMemory->m, 1, 0);
    sem_init(&sharedMemory->c0, 1, 0);
    sem_init(&sharedMemory->c1, 1, 0);
    sem_init(&sharedMemory->c2, 1, 0);
    sem_init(&sharedMemory->c3, 1, 0);
    sem_init(&sharedMemory->c4, 1, 0);
    sem_init(&sharedMemory->c5, 1, 0);
}

void pusherMilk(SharedMemory *sm){
    sem_wait(&sm->milk);
    if(interruptHappened){
        return;
    }
    sem_wait(&sm->m);
    if(interruptHappened){
        return;
    }
    if(sm->isFlour){
        sm->isFlour = 0;
        sem_post(&sm->c3);
    }
    else if(sm->isWalnut){
        sm->isWalnut = 0;
        sem_post(&sm->c4);
    }
    else if(sm->isSugar){
        sm->isSugar = 0;
        sem_post(&sm->c5);
    }
    else{
        sm->isMilk = 1;
    }
    sem_post(&sm->m);
}

void pusherFlour(SharedMemory *sm){
    sem_wait(&sm->flour);
    if(interruptHappened){
        return;
    }
    sem_wait(&sm->m);
    if(interruptHappened){
        return;
    }
    if(sm->isMilk){
        sm->isMilk = 0;
        sem_post(&sm->c3);
    }
    else if(sm->isWalnut){
        sm->isWalnut = 0;
        sem_post(&sm->c1);
    }
    else if(sm->isSugar){
        sm->isSugar = 0;
        sem_post(&sm->c2);
    }
    else{
        sm->isFlour = 1;
    }
    sem_post(&sm->m);
}

void pusherWalnut(SharedMemory *sm){
    sem_wait(&sm->walnut);
    if(interruptHappened){
        return;
    }
    sem_wait(&sm->m);
    if(interruptHappened){
        return;
    }
    if(sm->isMilk){
        sm->isMilk = 0;
        sem_post(&sm->c4);
    }
    else if(sm->isFlour){
        sm->isFlour = 0;
        sem_post(&sm->c1);
    }
    else if(sm->isSugar){
        sm->isSugar = 0;
        sem_post(&sm->c0);
    }
    else{
        sm->isWalnut = 1;
    }
    sem_post(&sm->m);
}

void pusherSugar(SharedMemory *sm){
    sem_wait(&sm->sugar);
    if(interruptHappened){
        return;
    }
    sem_wait(&sm->m);
    if(interruptHappened){
        return;
    }
    if(sm->isMilk){
        sm->isMilk = 0;
        sem_post(&sm->c5);
    }
    else if(sm->isFlour){
        sm->isFlour = 0;
        sem_post(&sm->c2);
    }
    else if(sm->isWalnut){
        sm->isWalnut = 0;
        sem_post(&sm->c0);
    }
    else{
        sm->isSugar = 1;
    }
    sem_post(&sm->m);
}

void chef0(SharedMemory *sm){
    sem_wait(&sm->c0);
    if(interruptHappened){
        return;
    }
    sem_wait(&sm->m);
    if(interruptHappened){
        return;
    }
    getIngredients(sm,0);
    fprintf(stdout,"chef%d (pid %d) has delivered the dessert. [%s]\n",0,getpid(),sm->ingr);
    sem_post(&sm->salerSem);
}

void chef1(SharedMemory *sm){
    sem_wait(&sm->c1);
    if(interruptHappened){
        return;
    }
    sem_wait(&sm->m);
    if(interruptHappened){
        return;
    }
    getIngredients(sm,1);
    fprintf(stdout,"chef%d (pid %d) has delivered the dessert. [%s]\n",1,getpid(),sm->ingr);
    sem_post(&sm->salerSem);
}

void chef2(SharedMemory *sm){
    sem_wait(&sm->c2);
    if(interruptHappened){
        return;
    }
    sem_wait(&sm->m);
    if(interruptHappened){
        return;
    }
    getIngredients(sm,2);
    fprintf(stdout,"chef%d (pid %d) has delivered the dessert. [%s]\n",2,getpid(),sm->ingr);
    sem_post(&sm->salerSem);
}

void chef3(SharedMemory *sm){
    sem_wait(&sm->c3);
    if(interruptHappened){
        return;
    }
    sem_wait(&sm->m);
    if(interruptHappened){
        return;
    }
    getIngredients(sm,3);
    fprintf(stdout,"chef%d (pid %d) has delivered the dessert. [%s]\n",3,getpid(),sm->ingr);
    sem_post(&sm->salerSem);
}

void chef4(SharedMemory *sm){
    sem_wait(&sm->c4);
    if(interruptHappened){
        return;
    }
    sem_wait(&sm->m);
    if(interruptHappened){
        return;
    }
    getIngredients(sm,4);
    fprintf(stdout,"chef%d (pid %d) has delivered the dessert. [%s]\n",4,getpid(),sm->ingr);
    sem_post(&sm->salerSem);
}

void chef5(SharedMemory *sm){
    sem_wait(&sm->c5);
    if(interruptHappened){
        return;
    }
    sem_wait(&sm->m);
    if(interruptHappened){
        return;
    }
    getIngredients(sm,5);
    fprintf(stdout,"chef%d (pid %d) has delivered the dessert. [%s]\n",5,getpid(),sm->ingr);
    sem_post(&sm->salerSem);
}

void getIngredients(SharedMemory *sm, int n){
    char ingr[10];

    for (int i = 0; i < 2; i++)
    {
        if(sm->ingr[i] == 'M'){
            strcpy(ingr,"Milk");
        }

        else if(sm->ingr[i] == 'F'){
            strcpy(ingr,"Flour");
        }

        else if(sm->ingr[i] == 'W'){
            strcpy(ingr,"Walnut");
        }

        else if(sm->ingr[i] == 'S'){
            strcpy(ingr,"Sugar");
        }

        sm->ingr[i] = ':';

        fprintf(stdout,"chef%d (pid %d) has taken the %s. [%s]\n", n, getpid(), ingr, sm->ingr);
    }

    fprintf(stdout,"chef%d (pid %d) is preparing the dessert. [%s]\n", n, getpid(), sm->ingr);
    
}