#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <fcntl.h>   //open
#include <unistd.h>  //close
#include <time.h>   //time
#include <complex.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>

#define NO_EINTR(stmt) while((stmt) < 0 && errno == EINTR);
sig_atomic_t interruptHappened = 0;
int n;
int m;
char *matrix_1;
char *matrix_2;
double *matrix_multiplied;
double complex *matrix_fouriered;
int arrived = 0;
pthread_mutex_t mutex;
pthread_cond_t cond_variable;

void sigIntHandler(int sigNum){
    if(sigNum == SIGINT){
        interruptHappened = 1;
    }
}

void get_timestamp(char *ts);
void* thread_calc(void *arg);
void matrix_multiply(char *A, char *B, double *C, int n, int p, int start_ind);
void fourier_transform(int k, int l);
int read_file(char *filePath, char *buffer, int bufferSize);
void sync_barrier();

int main(int argc, char *argv[])
{
    //get arguments example: ./hw5 -i filePath1 -j filePath2 -o output -n 4 -m 2
    
    if(argc != 11){
        fprintf(stderr, "Error: Argument count is not valid. It should be like './hw5 -i filePath1 -j filePath2 -o output -n 4 -m 2'\n");
        exit(EXIT_FAILURE);
    }
    
    char *filePath1, *filePath2, *output;
    int matrix_size;
    
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            switch (argv[i][1])
            {
            case 'i':
                filePath1 = argv[i + 1];
                break;
            case 'j':
                filePath2 = argv[i + 1];
                break;
            case 'o':
                output = argv[i + 1];
                break;
            case 'n':
                n = atoi(argv[i + 1]);
                break;
            case 'm':
                m = atoi(argv[i + 1]);
                break;
            }
        }
    }

    struct sigaction handler;
    memset(&handler, 0, sizeof(handler));
    handler.sa_handler = sigIntHandler;
    sigaction(SIGINT, &handler, NULL);

    //give error if arguments are wrong
    if (filePath1 == NULL || filePath2 == NULL || output == NULL)
    {
        fprintf(stderr, "Error: Filepaths are not valid. It should be like './hw5 -i filePath1 -j filePath2 -o output -n 4 -m 2'\n");
        exit(EXIT_FAILURE);
    }

    if(n <= 2 || m < 2){
        fprintf(stderr, "Error: n should be greater than 2 and m should be greater than or equal to 2.\n");
        exit(EXIT_FAILURE);
    }

    //open files
    int fp1 = open(filePath1, O_RDONLY);
    if(fp1 == -1){
        perror("Error: Cannot open file");
        exit(EXIT_FAILURE);
    }
    close(fp1);

    int fp2 = open(filePath2, O_RDONLY);
    if(fp2 == -1){
        perror("Error: Cannot open file");
        exit(EXIT_FAILURE);
    }
    close(fp2);
    
    matrix_size = pow(2,n);

    //allocate memory for matrices
    matrix_1 = (char *)malloc(matrix_size * matrix_size * sizeof(char));
    if(matrix_1 == NULL){
        perror("Error: Cannot allocate memory for matrix 1");
        exit(EXIT_FAILURE);
    }

    matrix_2 = (char *)malloc(matrix_size * matrix_size * sizeof(char));
    if(matrix_2 == NULL){
        perror("Error: Cannot allocate memory for matrix 2");
        exit(EXIT_FAILURE);
    }
    
    
    //read files
    int fileSize1 = read_file(filePath1, matrix_1, matrix_size*matrix_size*sizeof(char));
    if(fileSize1 == -1){
        perror("Error: Cannot read file");
        exit(EXIT_FAILURE);
    }

    int fileSize2 = read_file(filePath2, matrix_2, matrix_size*matrix_size*sizeof(char));
    if(fileSize2 == -1){
        perror("Error: Cannot read file");
        exit(EXIT_FAILURE);
    }

    if(fileSize1 != fileSize2){
        fprintf(stderr, "Error: File char sizes are not equal.\n");
        exit(EXIT_FAILURE);
    }

    if(fileSize1 != matrix_size*matrix_size){
        fprintf(stderr, "Error: File char size should be equal to given matrix size.\n");
        exit(EXIT_FAILURE);
    }

    char ts[26];
    get_timestamp(ts);
    fprintf(stdout, "%s Two matrices of size %dx%d have been read. The number of thread is %d.\n", ts, matrix_size, matrix_size, m);
    double start_time = clock();
    //allocate memory for matrix_multiplied
    matrix_multiplied = (double *)malloc(matrix_size * matrix_size * sizeof(double));
    if(matrix_multiplied == NULL){
        perror("Error: Cannot allocate memory for matrix_multiplied");
        exit(EXIT_FAILURE);
    }

    //allocate memory for matrix_fouriered
    matrix_fouriered = (double complex *)malloc(matrix_size * matrix_size * sizeof(double complex));
    if(matrix_fouriered == NULL){
        perror("Error: Cannot allocate memory for matrix_fouriered");
        exit(EXIT_FAILURE);
    }

    //initialize mutex and cond_variable
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond_variable, NULL);

    //create m threads
    pthread_t *threads = (pthread_t *)malloc(m * sizeof(pthread_t));
    if(threads == NULL){
        perror("Error: Cannot allocate memory for threads");
        exit(EXIT_FAILURE);
    }
    int *indices = (int *)malloc(m * sizeof(int));
    for(int i = 0; i < m; i++){
        indices[i] = i;
        pthread_create(&threads[i], NULL, thread_calc, (void *)&indices[i]);
    }

    //wait for all threads to finish
    for(int i = 0; i < m; i++){
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_variable);
    free(threads);
    free(indices);
    free(matrix_1);
    free(matrix_2);
    free(matrix_multiplied);

    if(interruptHappened){
        fprintf(stderr, "Error: Interrupt happened.\n");
        free(matrix_fouriered);
        exit(EXIT_FAILURE);
    }

    //write to file
    FILE *fp = fopen(output, "w");
    if(fp == NULL){
        perror("Error: Cannot open file");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < matrix_size; i++){
        for(int j = 0; j < matrix_size - 1; j++){
            fprintf(fp, "%.3lf + %.3lfi,", creal(matrix_fouriered[i*matrix_size + j]), cimag(matrix_fouriered[i*matrix_size + j]));
        }
        fprintf(fp, "%.3lf + %.3lfi\n", creal(matrix_fouriered[i*matrix_size + matrix_size - 1]), cimag(matrix_fouriered[i*matrix_size + matrix_size - 1]));
    }

    fclose(fp);
    double end_time = clock();

    get_timestamp(ts);
    fprintf(stdout, "%s The process has written the output file. The total time spent is %.4lf seconds.\n", ts, (end_time - start_time)/CLOCKS_PER_SEC);

    free(matrix_fouriered);
    return 0;
}

int read_file(char *filePath, char *buffer, int bufferSize){
    int fp = open(filePath, O_RDONLY);
    if(fp == -1){
        perror("Error: Cannot open file");
        return -1;
    }
    
    int readBytes = read(fp, buffer, bufferSize);        
    if(readBytes == -1){
        perror("Error: Cannot read file");
        return -1;
    }
    close(fp);

    return readBytes;
}
//calculate matrix multiplication of C = AXB
void matrix_multiply(char *A, char *B, double *C, int n, int p, int start_ind){
    for(int i = 0; i < n; i++){
        for(int j = start_ind; j < p; j+=m){
            C[i*p + j] = 0;
            for(int k = 0; k < n; k++){
                if(interruptHappened){
                    return;
                }
                C[i * p + j] += A[i * n + k] * B[k * p + j];
            }
        }
    }
}

void fourier_transform(int k, int l){
    double val;
    int matrix_size = pow(2, n);
    matrix_fouriered[k*matrix_size + l] = 0;

    for(int m = 0; m < matrix_size; ++m){    
        for(int n = 0; n < matrix_size; ++n){
            if(interruptHappened)
                return;
            val = matrix_multiplied[m * matrix_size + n];
            matrix_fouriered[k * matrix_size + l] += (val) * cexp(-2 * I * (M_PI * (double)k * (double)m / (double)matrix_size)) * cexp(-2 * I * (M_PI * (double)l * (double)n / (double)matrix_size));
        }
    }
}

void* thread_calc(void *arg){
    char ts[26];
    double start_time = clock();
    int index = *(int *)arg;
    int matrix_size = pow(2, n);

    matrix_multiply(matrix_1, matrix_2, matrix_multiplied, matrix_size, matrix_size, index);
    if(interruptHappened){
        pthread_exit(NULL);
    }
    double end_time = clock();
    get_timestamp(ts);
    fprintf(stdout, "%s Thread %d has reached the randezvous point in %.4lf seconds.\n", ts, index, (end_time - start_time) / CLOCKS_PER_SEC);
    sync_barrier();
    
    get_timestamp(ts);
    fprintf(stdout, "%s Thread %d is advancing to the second part\n", ts, index);

    start_time = clock();
    for(int k = 0; k < matrix_size; ++k){
        for(int l = index; l < matrix_size; l+=m){
            if(interruptHappened){
                pthread_exit(NULL);
            }
            fourier_transform(k, l);
        }
    }
    
    if(interruptHappened){
        pthread_exit(NULL);
    }
    
    end_time = clock();
    get_timestamp(ts);
    fprintf(stdout, "%s Thread %d has finished the second part in %.4lf seconds.\n", ts, index, (end_time - start_time) / CLOCKS_PER_SEC);

    pthread_exit(NULL);
}

void sync_barrier(){
    //synchronization barrier
    pthread_mutex_lock(&mutex);
    arrived++;

    if(arrived < m){
        pthread_cond_wait(&cond_variable, &mutex);
    }
    else{
        pthread_cond_broadcast(&cond_variable);
    }
    pthread_mutex_unlock(&mutex);
}
   
void get_timestamp(char *ts){
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(ts, 26, "%Y-%m-%d %H:%M:%S", tm);
}

