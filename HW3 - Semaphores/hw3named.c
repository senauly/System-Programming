#include "hw3named.h"

sig_atomic_t interruptHappened = 0;

void sigIntHandler(int sigNum){
    if(sigNum == SIGINT)
        interruptHappened = 1;
}

int main(int argc, char *argv[]){
    //get arguments from command line 
    char *inputFilePath = NULL;
    char *semNames[] = {"salerSem", "milk", "flour", "walnut", "sugar", "m", "c0", "c1", "c2", "c3", "c4", "c5"};
    NamedSem semaphores;
    struct sigaction sigAction;
    memset(&sigAction, 0, sizeof(struct sigaction));
    sigAction.sa_handler = sigIntHandler;
    sigaction(SIGINT, &sigAction, NULL);
    
    int existFlag = 0;
    if(argc >= 5 && strcmp(argv[1], "-i") == 0 && strcmp(argv[3],"-n") == 0){
        inputFilePath = argv[2];
        if(argc > 4){
            for(int i = 4; i < argc; i++){
                existFlag = 0;
                if(i-4 >= 12) break;
                for(int j = 0; j < 12; j++){
                    if(j != i-5 && strcmp(argv[i], semNames[j]) == 0){
                        existFlag = 1;
                        break;
                    }
                }
                if(!existFlag)
                    semNames[i-5] = argv[i];
            }
        }
    }
    else{
        fprintf(stderr, "Error: Command should be like ./hw3named -i inputFilePath -n name\n");
        exit(EXIT_FAILURE);
    }

    SharedMemory *sm;
    if(initSharedMemory(&sm) == -1){
        exit(EXIT_FAILURE);
    }

    initSemaphores(&semaphores, semNames);

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
                    chef0(sm, &semaphores);
                }

                else if(i == 1){
                    fprintf(stdout,"chef%d (pid %d) is waiting for walnut and flour. [%s]\n", i, getpid(),sm->ingr);
                    chef1(sm, &semaphores);
                    
                }

                else if(i == 2){
                    fprintf(stdout,"chef%d (pid %d) is waiting for sugar and flour. [%s]\n", i, getpid(),sm->ingr);
                    chef2(sm, &semaphores);
                    
                }

                else if(i == 3){
                    fprintf(stdout,"chef%d (pid %d) is waiting for flour and milk. [%s]\n", i, getpid(),sm->ingr);
                    chef3(sm, &semaphores);
                    
                }

                else if(i == 4){
                    fprintf(stdout,"chef%d (pid %d) is waiting for walnut and milk. [%s]\n", i, getpid(),sm->ingr);
                    chef4(sm, &semaphores);
                    
                }   

                else if(i == 5){
                    fprintf(stdout,"chef%d (pid %d) is waiting for sugar and milk. [%s]\n", i, getpid(),sm->ingr);
                    chef5(sm, &semaphores);
                    
                }
                if(interruptHappened){
                    break;
                } 
                count++;
            }
           
           fprintf(stdout,"chef%d (pid %d) is exiting. [%s]\n", i, getpid(), sm->ingr);
           closeSemaphores(&semaphores);
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
                    pusherMilk(sm, &semaphores);
                }

                else if(i == 1){
                    pusherFlour(sm, &semaphores);
                }

                else if(i == 2){
                    pusherWalnut(sm, &semaphores);
                }

                else if(i == 3){
                    pusherSugar(sm, &semaphores);
                }
                if(interruptHappened){
                    break;
                }
            }
            closeSemaphores(&semaphores);
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

    int n=0;

    while(1){
        if(n == 2){
            break;
        }
        char buff[3];
        NO_EINTR(n = read(fd, buff, 3*sizeof(char)));
        if(interruptHappened){
            break;
        }
        
        if(n == 0 || n == 1){
            break;
        }
        
        if(n == -1){
            perror("read");
            exit(EXIT_FAILURE);
        }
        fflush(stdout);
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
                sem_post(semaphores.milk);
            }

            if(buff[i] == 'F'){
                if(i == 0) strcpy(ingredient1, "flour");
                else strcpy(ingredient2, "flour");
                sem_post(semaphores.flour);
            }

            if(buff[i] == 'W'){
                if(i == 0) strcpy(ingredient1, "walnut");
                else strcpy(ingredient2, "walnut");
                sem_post(semaphores.walnut);
            }

            if(buff[i] == 'S'){
                if(i == 0) strcpy(ingredient1, "sugar");
                else strcpy(ingredient2, "sugar");
                sem_post(semaphores.sugar);
            }
        }
        
        fprintf(stdout, "the wholesaler (pid %d) delivers %s and %s. [%s]\n", getpid(), ingredient1, ingredient2, sm->ingr);
        sem_post(semaphores.m);
        fprintf(stdout, "the wholesaler (pid %d) is waiting for the dessert. [%s]\n", getpid(), sm->ingr);
        sem_wait(semaphores.salerSem);
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

    closeSemaphores(&semaphores);
    unlinkSemaphores(semNames);

    //close input file
    close(fd);
    fprintf(stdout,"the wholesaler (pid %d) is done (total desserts: %d) [%s]\n", getpid(), totalDessert, sm->ingr);

    shm_unlink(SHARED_MEMORY_NAME);
    munmap(sm, sizeof(SharedMemory));
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

void initSemaphores(NamedSem *namedSem, char *semNames[]){
    //initialize semaphores
    namedSem->salerSem = sem_open(semNames[0], O_CREAT | O_RDWR, 0666, 0);
    namedSem->milk = sem_open(semNames[1], O_CREAT | O_RDWR, 0666, 0);
    namedSem->flour = sem_open(semNames[2], O_CREAT | O_RDWR, 0666, 0);
    namedSem->walnut = sem_open(semNames[3], O_CREAT | O_RDWR, 0666, 0);
    namedSem->sugar = sem_open(semNames[4], O_CREAT | O_RDWR, 0666, 0);
    namedSem->m = sem_open(semNames[5], O_CREAT | O_RDWR, 0666, 0);
    namedSem->c0 = sem_open(semNames[6], O_CREAT | O_RDWR, 0666, 0);
    namedSem->c1 = sem_open(semNames[7], O_CREAT | O_RDWR, 0666, 0);
    namedSem->c2 = sem_open(semNames[8], O_CREAT | O_RDWR, 0666, 0);
    namedSem->c3 = sem_open(semNames[9], O_CREAT | O_RDWR, 0666, 0);
    namedSem->c4 = sem_open(semNames[10], O_CREAT | O_RDWR, 0666, 0);
    namedSem->c5 = sem_open(semNames[11], O_CREAT | O_RDWR, 0666, 0);
}

void closeSemaphores(NamedSem *namedSem){
    if(sem_close(namedSem->salerSem) < 0){
        perror("Error: sem_close failed.\n");
    }
    if(sem_close(namedSem->milk)){
        perror("Error: sem_close failed.\n");
    }
    if(sem_close(namedSem->flour)){
        perror("Error: sem_close failed.\n");
    }
    if(sem_close(namedSem->walnut)){
        perror("Error: sem_close failed.\n");
    }
    if(sem_close(namedSem->sugar)){
        perror("Error: sem_close failed.\n");
    }
    if(sem_close(namedSem->m)){
        perror("Error: sem_close failed.\n");
    }
    if(sem_close(namedSem->c0)){
        perror("Error: sem_close failed.\n");
    }
    if(sem_close(namedSem->c1)){
        perror("Error: sem_close failed.\n");
    }
    if(sem_close(namedSem->c2)){
        perror("Error: sem_close failed.\n");
    }
    if(sem_close(namedSem->c3)){
        perror("Error: sem_close failed.\n");
    }
    if(sem_close(namedSem->c4)){
        perror("Error: sem_close failed.\n");
    }
    if(sem_close(namedSem->c5)){
        perror("Error: sem_close failed.\n");
    }
}

void unlinkSemaphores(char *semNames[]){

    for (int i = 0; i < 12; i++)
    {
        if(sem_unlink(semNames[i])){
            perror("Error: sem_unlink failed.\n");
        }
    }
    
}

void pusherMilk(SharedMemory *sm, NamedSem *semaphores){
    sem_wait(semaphores->milk);
    if(interruptHappened){
        return;
    }
    sem_wait(semaphores->m);
    if(interruptHappened){
        return;
    }
    if(sm->isFlour){
        sm->isFlour = 0;
        sem_post(semaphores->c3);
    }
    else if(sm->isWalnut){
        sm->isWalnut = 0;
        sem_post(semaphores->c4);
    }
    else if(sm->isSugar){
        sm->isSugar = 0;
        sem_post(semaphores->c5);
    }
    else{
        sm->isMilk = 1;
    }
    sem_post(semaphores->m);
}

void pusherFlour(SharedMemory *sm, NamedSem *semaphores){
    sem_wait(semaphores->flour);
    if(interruptHappened){
        return;
    }
    sem_wait(semaphores->m);
    if(interruptHappened){
        return;
    }
    if(sm->isMilk){
        sm->isMilk = 0;
        sem_post(semaphores->c3);
    }
    else if(sm->isWalnut){
        sm->isWalnut = 0;
        sem_post(semaphores->c1);
    }
    else if(sm->isSugar){
        sm->isSugar = 0;
        sem_post(semaphores->c2);
    }
    else{
        sm->isFlour = 1;
    }
    sem_post(semaphores->m);
}

void pusherWalnut(SharedMemory *sm, NamedSem *semaphores){
    sem_wait(semaphores->walnut);
    if(interruptHappened){
        return;
    }
    sem_wait(semaphores->m);
    if(interruptHappened){
        return;
    }
    if(sm->isMilk){
        sm->isMilk = 0;
        sem_post(semaphores->c4);
    }
    else if(sm->isFlour){
        sm->isFlour = 0;
        sem_post(semaphores->c1);
    }
    else if(sm->isSugar){
        sm->isSugar = 0;
        sem_post(semaphores->c0);
    }
    else{
        sm->isWalnut = 1;
    }
    sem_post(semaphores->m);
}

void pusherSugar(SharedMemory *sm, NamedSem *semaphores){
    sem_wait(semaphores->sugar);
    if(interruptHappened){
        return;
    }
    sem_wait(semaphores->m);
    if(interruptHappened){
        return;
    }
    if(sm->isMilk){
        sm->isMilk = 0;
        sem_post(semaphores->c5);
    }
    else if(sm->isFlour){
        sm->isFlour = 0;
        sem_post(semaphores->c2);
    }
    else if(sm->isWalnut){
        sm->isWalnut = 0;
        sem_post(semaphores->c0);
    }
    else{
        sm->isSugar = 1;
    }
    sem_post(semaphores->m);
}

void chef0(SharedMemory *sm, NamedSem *semaphores){
    sem_wait(semaphores->c0);
    if(interruptHappened){
        return;
    }
    sem_wait(semaphores->m);
    if(interruptHappened){
        return;
    }
    getIngredients(sm,0);
    fprintf(stdout,"chef%d (pid %d) has delivered the dessert. [%s]\n",0,getpid(),sm->ingr);
    
    sem_post(semaphores->salerSem);
}

void chef1(SharedMemory *sm, NamedSem *semaphores){
    sem_wait(semaphores->c1);
    if(interruptHappened){
        return;
    }
    sem_wait(semaphores->m);
    if(interruptHappened){
        return;
    }
    getIngredients(sm,1);
    fprintf(stdout,"chef%d (pid %d) has delivered the dessert. [%s]\n",1,getpid(),sm->ingr);
    
    sem_post(semaphores->salerSem);
}

void chef2(SharedMemory *sm, NamedSem *semaphores){
    sem_wait(semaphores->c2);
    if(interruptHappened){
        return;
    }
    sem_wait(semaphores->m);
    if(interruptHappened){
        return;
    }
    getIngredients(sm,2);
    fprintf(stdout,"chef%d (pid %d) has delivered the dessert. [%s]\n",2,getpid(),sm->ingr);
    
    sem_post(semaphores->salerSem);
}

void chef3(SharedMemory *sm, NamedSem *semaphores){
    sem_wait(semaphores->c3);
    if(interruptHappened){
        return;
    }
    sem_wait(semaphores->m);
    if(interruptHappened){
        return;
    }
    getIngredients(sm,3);
    fprintf(stdout,"chef%d (pid %d) has delivered the dessert. [%s]\n",3,getpid(),sm->ingr);
    
    sem_post(semaphores->salerSem);
}

void chef4(SharedMemory *sm, NamedSem *semaphores){
    sem_wait(semaphores->c4);
    if(interruptHappened){
        return;
    }
    sem_wait(semaphores->m);
    if(interruptHappened){
        return;
    }
    getIngredients(sm,4);
    fprintf(stdout,"chef%d (pid %d) has delivered the dessert. [%s]\n",4,getpid(),sm->ingr);
    
    sem_post(semaphores->salerSem);
}

void chef5(SharedMemory *sm, NamedSem *semaphores){
    sem_wait(semaphores->c5);
    if(interruptHappened){
        return;
    }
    sem_wait(semaphores->m);
    if(interruptHappened){
        return;
    }
    getIngredients(sm,5);
    fprintf(stdout,"chef%d (pid %d) has delivered the dessert. [%s]\n",5,getpid(),sm->ingr);
    
    sem_post(semaphores->salerSem);
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