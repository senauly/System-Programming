#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

#define NO_EINTR(stmt) while((stmt) < 0 && errno == EINTR);

#define ARGUMENT_COUNT 5
#define NUMBER_OF_COORDINATE 10
#define NUMBER_OF_DIMENSION 3
#define BUFF_SIZE 300

void get_environments(int coordinates[NUMBER_OF_COORDINATE][NUMBER_OF_DIMENSION]);
void calculate_covariance_matrix(const int coordinates[NUMBER_OF_COORDINATE][NUMBER_OF_DIMENSION], double **covarianceMatrix);

sig_atomic_t contin = 0;
sig_atomic_t sigintFlag = 0;

void sigusr1_handler(int sig) {
    contin = 1;
}

void sigint_handler(int sig) {
    sigintFlag = 1;
}

int main(int argc, const char *argv[]){
    int coordinates[NUMBER_OF_COORDINATE][NUMBER_OF_DIMENSION];
    int fd;
    double **covarienceMatrix;
    covarienceMatrix = (double **)malloc(NUMBER_OF_DIMENSION * sizeof(double *));
    
    if (covarienceMatrix == NULL){
        perror("Malloc: ");
        exit(EXIT_FAILURE);
    }
    for(int i = 0; i < NUMBER_OF_DIMENSION; i++){
        covarienceMatrix[i] = (double *)malloc(NUMBER_OF_DIMENSION * sizeof(double));
        if (covarienceMatrix[i] == NULL){
            perror("Malloc: ");
            exit(EXIT_FAILURE);
        }
    }
    
    struct sigaction act;
    act.sa_handler = &sigusr1_handler;
    sigfillset(&act.sa_mask);
    act.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &act, NULL);

    struct sigaction sigintAct;
    sigintAct.sa_handler = &sigint_handler;
    sigaction(SIGINT, &sigintAct, NULL);

    if(kill(getppid(), SIGUSR1) < 0){
        perror("Kill: ");
        exit(EXIT_FAILURE);
    }

    if(sigintFlag){
        for(int i = 0; i < NUMBER_OF_DIMENSION; i++){
            free(covarienceMatrix[i]);
        }
        free(covarienceMatrix);
        fprintf(stderr, "Child received SIGINT signal\n");
        exit(EXIT_FAILURE);
    }
    
    get_environments(coordinates);
    calculate_covariance_matrix(coordinates, covarienceMatrix);
    
    sigset_t sigset;
    sigemptyset(&sigset);
    
    
    fd = open(argv[0], O_WRONLY | O_APPEND);
    if(fd < 0){
        perror("Open: ");
        for(int i = 0; i < NUMBER_OF_DIMENSION; i++){
            free(covarienceMatrix[i]);
        }
        free(covarienceMatrix);
        exit(EXIT_FAILURE);
    }


    if(sigintFlag){
        for(int i = 0; i < NUMBER_OF_DIMENSION; i++){
            free(covarienceMatrix[i]);
        }
        free(covarienceMatrix);
        if(close(fd) < 0){
            perror("Close: ");
        }
        fprintf(stderr, "Child received SIGINT signal\n");
        exit(EXIT_FAILURE);
    }

    
    if(sigintFlag){
        for(int i = 0; i < NUMBER_OF_DIMENSION; i++){
            free(covarienceMatrix[i]);
        }
        free(covarienceMatrix);
        if(close(fd) < 0){
            perror("Close: ");
        }
        fprintf(stderr, "Child received SIGINT signal\n");
        exit(EXIT_FAILURE);
    }
    double buffer[NUMBER_OF_DIMENSION * NUMBER_OF_DIMENSION];
    for(int i = 0; i < NUMBER_OF_DIMENSION; i++){
        for(int j = 0; j < NUMBER_OF_DIMENSION; j++){
            buffer[i * NUMBER_OF_DIMENSION + j] = covarienceMatrix[i][j];
        }
    }

    
    if(!contin){
        sigsuspend(&sigset);
    }
    NO_EINTR(write(fd, buffer, NUMBER_OF_DIMENSION * NUMBER_OF_DIMENSION* sizeof(double)));

    for(int i = 0; i < NUMBER_OF_DIMENSION; i++){
        free(covarienceMatrix[i]);
    }
    free(covarienceMatrix);
    
    close(fd);
    if (fd < 0){
        perror("Close: ");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

void get_environments(int coordinates[NUMBER_OF_COORDINATE][NUMBER_OF_DIMENSION]){
    char environmentKey[128];
    char *environmentValue;
    for(int i = 0; i < NUMBER_OF_COORDINATE; i++){
        for (int j = 0; j < NUMBER_OF_DIMENSION; j++){
            sprintf(environmentKey, "C_%d_%d", i, j);
            environmentValue = getenv(environmentKey);
            sscanf(environmentValue, "%d", &coordinates[i][j]);
        }
    }
}

/**
 * @brief Multiply 2 matrices with given column and row sizes
 * 
 * @param A first matrix
 * @param rowA row size of matrix A
 * @param colA column size of matrix B
 * @param B second matrix
 * @param rowB row size of matrix B
 * @param colB column size of matrix B
 * @param res result of multiplication matrix
 */
void multiply_matrix(double **A, int rowA, int colA, double **B, int rowB, int colB, double **res){
    for (int i = 0; i < rowA; i++){
        for (int j = 0; j < colB; j++){
            res[i][j] = 0.0;
            for (int k = 0; k < colA; k++){
                res[i][j] += (A[i][k] * B[k][j]);
            }
        }
    }
}

/**
 * @brief Substract 2 matrices with given column and row sizes
 * 
 * @param A first matrix
 * @param rowA row size of matrix A
 * @param colA column size of matrix B
 * @param B second matrix
 * @param rowB row size of matrix B
 * @param colB column size of matrix B
 * @param res result of substraction matrix
 */
void substract_matrices(double **A, int rowA, int colA, double **B, int rowB, int colB, double **res){
    for (int i = 0; i < rowA; i++){
        for (int j = 0; j < colA; j++){
            res[i][j] = A[i][j] - B[i][j];
        }
    }
}

/**
 * @brief Allocate memory for 2D array and initialize it with coordinates input
 * 
 * @param coordinates array of coordinates
 * @return new 2D array
 */
double** malloc_2D(const int coordinates[NUMBER_OF_COORDINATE][NUMBER_OF_DIMENSION]){
    double **temp = (double **)malloc(sizeof(double *) * NUMBER_OF_COORDINATE);
    if (temp == NULL){
        perror("Malloc: ");
        exit(EXIT_FAILURE);
    }
    
    for (int i = 0; i < NUMBER_OF_COORDINATE; i++){
        temp[i] = (double *)malloc(sizeof(double) * NUMBER_OF_DIMENSION);
        if (temp[i] == NULL){
            perror("Malloc: ");
            exit(EXIT_FAILURE);
        }
        for (int j = 0; j < NUMBER_OF_DIMENSION; j++){
            temp[i][j] = (double)coordinates[i][j];
        }
    }

    return temp;
}

/**
 * @brief Calculate covariance matrix of given coordinates.
 * 
 * @param coordinates given coordinates
 * @param covarianceMatrix result of the calculation
 */
void calculate_covariance_matrix(const int coordinates[NUMBER_OF_COORDINATE][NUMBER_OF_DIMENSION], double **covarianceMatrix){
    double **transposeMatrix = (double **)malloc(sizeof(double *) * NUMBER_OF_DIMENSION);
    if (transposeMatrix == NULL){
        perror("Malloc: ");
        exit(EXIT_FAILURE);
    }
    for(int i = 0; i < NUMBER_OF_DIMENSION; i++){
        transposeMatrix[i] = (double *)malloc(sizeof(double) * NUMBER_OF_COORDINATE);
        if (transposeMatrix[i] == NULL){
            perror("Malloc: ");
            exit(EXIT_FAILURE);
        }
    }

    double **tempMatrix1 = malloc_2D(coordinates);
    double **tempMatrix2 = malloc_2D(coordinates);

    //allocate unit matrix
    double **unitMatrix = (double**)malloc(NUMBER_OF_COORDINATE * sizeof(double *));
    if (unitMatrix == NULL){
        perror("Malloc: ");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < NUMBER_OF_COORDINATE; i++){
        unitMatrix[i] = (double*)malloc(NUMBER_OF_COORDINATE * sizeof(double));
        if (unitMatrix[i] == NULL){
            perror("Malloc: ");
            exit(EXIT_FAILURE);
        }
        for (int j = 0; j < NUMBER_OF_COORDINATE; j++){
            unitMatrix[i][j] = 1.0;
        }
    }

    //apply formula
    multiply_matrix(unitMatrix, NUMBER_OF_COORDINATE, NUMBER_OF_COORDINATE, tempMatrix2, NUMBER_OF_COORDINATE, NUMBER_OF_DIMENSION, tempMatrix1);
    for (int i = 0; i < NUMBER_OF_COORDINATE; i++){
        for (int j = 0; j < NUMBER_OF_DIMENSION; j++){
            tempMatrix1[i][j] /= (double)NUMBER_OF_COORDINATE;
        }
    }

    substract_matrices(tempMatrix2, NUMBER_OF_COORDINATE, NUMBER_OF_DIMENSION, tempMatrix1, NUMBER_OF_COORDINATE, NUMBER_OF_DIMENSION, tempMatrix1);

    for (int i = 0; i < NUMBER_OF_DIMENSION; i++){
        for (int j = 0; j < NUMBER_OF_COORDINATE; j++){
            transposeMatrix[i][j] = tempMatrix1[j][i];
        }
    }

    multiply_matrix(transposeMatrix, NUMBER_OF_DIMENSION, NUMBER_OF_COORDINATE, tempMatrix1, NUMBER_OF_COORDINATE, NUMBER_OF_DIMENSION, covarianceMatrix);

    for (int i = 0; i < NUMBER_OF_DIMENSION; i++){
        for (int j = 0; j < NUMBER_OF_DIMENSION; j++){
            covarianceMatrix[i][j] /= NUMBER_OF_COORDINATE;
        }
    }
    

    for (int i = 0; i < NUMBER_OF_COORDINATE; i++){
        free(tempMatrix1[i]);
        free(tempMatrix2[i]);
        free(unitMatrix[i]);
    }
    
    for (int i = 0; i < NUMBER_OF_DIMENSION; i++){
        free(transposeMatrix[i]);
    }
    
    free(tempMatrix1);
    free(tempMatrix2);
    free(unitMatrix);       
    free(transposeMatrix);   
}