#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/wait.h>

extern char **environ;

#define NO_EINTR(stmt) while((stmt) < 0 && errno == EINTR);
#define ARGUMENT_COUNT 5
#define NUMBER_OF_COORDINATE 10
#define NUMBER_OF_DIMENSION 3
#define BUFF_SIZE 300

typedef struct PIDList
{
    int *arr;
    int size;
    int cap;
} PIDList;

int get_arguments(int argc, char *argv[], char **input, char **output);
int create_child_process(int fd, const char *outputFile, PIDList *pidList);
int read_output_file(int fd, int matrixCount, double ***result);
double frobius_norm(double **matrix);
void forward_interrupt_to_childs(const PIDList *pidList, int childCount, char *output);
void print_coordinates(int pNum, const uint8_t arr[NUMBER_OF_COORDINATE][NUMBER_OF_DIMENSION]);

sig_atomic_t sigintFlag = 0;
sig_atomic_t sigusr1Count = 0;

void sigint_handler(int signo)
{
    sigintFlag = 1;
}

void sigusr1_handler(int signo){
    sigusr1Count++;
}

int main(int argc, char *argv[])
{
    struct sigaction sigAct;
    sigAct.sa_handler = &sigint_handler;
    if(sigaction(SIGINT, &sigAct, NULL) < 0)
    {
        perror("Sigaction:");
        exit(EXIT_FAILURE);
    }

    struct sigaction sigusr1Act;
    sigusr1Act.sa_handler = &sigusr1_handler;
    sigfillset(&sigusr1Act.sa_mask);
    sigusr1Act.sa_flags = SA_RESTART;
    if(sigaction(SIGUSR1, &sigusr1Act, NULL) < 0)
    {
        perror("Sigaction:");
        exit(EXIT_FAILURE);
    }

    char *input;
    char *output;
    double ***matrixList;
    double *frobiusNorm;
    int childCount = 0;
    int fd;
    int waitReturn = 0;
    double min = 0;
    int resultMatrix1 = 0, resultMatrix2 = 1;
    PIDList pidList;
    pidList.arr = NULL;
    pidList.size = 0;
    pidList.cap = 0;

    if (get_arguments(argc, argv, &input, &output) == -1)
    {
        fprintf(stderr, "Usage: ./processP -i inputFilePath -o outputFilePath\n");
        exit(EXIT_FAILURE);
    }
    fd = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
    {
        perror("Open: ");
        exit(EXIT_FAILURE);
    }
    NO_EINTR(fd = close(fd));
    if (fd < 0)
    {
        perror("Close: ");
        exit(EXIT_FAILURE);
    }

    fd = open(input, O_RDONLY);
    if (fd < 0)
    {
        perror("Open: ");
        exit(EXIT_FAILURE);
    }
    

    fprintf(stdout,"Process P reading %s\n", input);
    pidList.size = 0;
    pidList.cap = 8;
    pidList.arr = (int *)malloc(pidList.cap * sizeof(int));
    if (pidList.arr == NULL){
        perror("Malloc: ");
        exit(EXIT_FAILURE);
    }

    if(sigintFlag){
        close(fd);
        if (fd < 0) perror("Close: ");
        free(pidList.arr);
        remove(output);
        fprintf(stderr, "Parent received SIGINT signal\n");
        exit(EXIT_FAILURE);
    }

    childCount = create_child_process(fd, output, &pidList);
    if(childCount < 2){
        close(fd);
        if (fd < 0) perror("Close: ");
        fprintf(stderr, "2 child process (2 different matrices) is required for the calculation.\n");
        free(pidList.arr);
        remove(output);
        exit(EXIT_FAILURE);
    }

    sigset_t sigset;
    sigemptyset(&sigset);
    while(sigusr1Count < childCount){
        printf("%d test\n", sigusr1Count);
        sigsuspend(&sigset);
    }
    printf("Parent received SIGUSR1 signal\n");
    NO_EINTR(fd = close(fd));
    if (fd < 0)
    {
        perror("Close: ");
        exit(EXIT_FAILURE);
    }

    if(sigintFlag){
        forward_interrupt_to_childs(&pidList, childCount, output);
        free(pidList.arr);
        fprintf(stderr, "Parent received SIGINT signal\n");
        exit(EXIT_FAILURE);
    }
    int status;

    for (int i = 0; i < childCount; i++)
    {
        kill(pidList.arr[i], SIGUSR1);
        NO_EINTR(waitReturn = wait(&status));
        if (waitReturn < 0)
        {
            perror("Wait: ");
        }
    }

    free(pidList.arr);

    if(sigintFlag){
        remove(output);
        fprintf(stderr, "Parent received SIGINT signal\n");
        exit(EXIT_FAILURE);
    }

    fd = open(output, O_RDONLY);
    if (fd < 0)
    {
        perror("Open: ");
        exit(EXIT_FAILURE);
    }

    matrixList = (double ***)malloc(sizeof(double **) * childCount);
    if (matrixList == NULL){
        perror("Malloc: ");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < childCount; ++i)
    {
        matrixList[i] = (double **)malloc(sizeof(double *) * NUMBER_OF_DIMENSION);
        if (matrixList[i] == NULL){
            perror("Malloc: ");
            exit(EXIT_FAILURE);
        }
        
        for (int j = 0; j < NUMBER_OF_DIMENSION; ++j)
        {
            matrixList[i][j] = (double *)malloc(sizeof(double) * NUMBER_OF_DIMENSION);
            if (matrixList[i][j] == NULL){
                perror("Malloc: ");
                exit(EXIT_FAILURE);
            }
        }
    }


    if(sigintFlag){
        remove(output);
        close(fd);
        if (fd < 0) perror("Close: ");
        for (int i = 0; i < childCount; ++i){
            for (int j = 0; j < NUMBER_OF_DIMENSION; ++j){
                free(matrixList[i][j]);
            }
            free(matrixList[i]);
        }
        free(matrixList);
        fprintf(stderr, "Parent received SIGINT signal\n");
        exit(EXIT_FAILURE);
    }

    if (read_output_file(fd, childCount, matrixList) == -1){
        NO_EINTR(fd = close(fd));
        if (fd < 0)
        {
            perror("Close: ");
        }
        exit(EXIT_FAILURE);
    }

    NO_EINTR(fd = close(fd));
    if (fd < 0){
        perror("Close: ");
        exit(EXIT_FAILURE);
    }

    frobiusNorm = (double *)malloc(sizeof(double) * childCount);
    if (frobiusNorm == NULL){
        perror("Malloc: ");
        exit(EXIT_FAILURE);
    }
    
    for (int i = 0; i < childCount; ++i)
    {
       frobiusNorm[i] = frobius_norm(matrixList[i]);
    }
    
    for (int i = 0; i < childCount; ++i){
        for (int j = 0; j < NUMBER_OF_DIMENSION; ++j){
            free(matrixList[i][j]);
        }
        free(matrixList[i]);
    }
    free(matrixList);

    if(sigintFlag){
        remove(output);
        free(frobiusNorm);
        fprintf(stderr, "Parent received SIGINT signal\n");
        exit(EXIT_FAILURE);
    }


    min = fabs(frobiusNorm[0] - frobiusNorm[1]);
    for (int i = 1; i < childCount; ++i)
    {
        for(int j = i + 1; j < childCount; ++j)
        {
            if (fabs(frobiusNorm[i] - frobiusNorm[j]) < min)
            {
                min = fabs(frobiusNorm[i] - frobiusNorm[j]);
                resultMatrix1 = i;
                resultMatrix2 = j;
            }
        }
    }

    free(frobiusNorm);

    if(sigintFlag){
        remove(output);
        fprintf(stderr, "Parent received SIGINT signal\n");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "The closest 2 matrices are Matrix %d and %d, and their distance is %.3f.\n", resultMatrix1 + 1, resultMatrix2 + 1, min);

    exit(EXIT_SUCCESS);
}

/**
 * @brief Get the arguments and place them in input/output strings accordingly.
 *
 * @param argc argument count
 * @param argv argument array
 * @param input input string
 * @param output output string
 * @return 0 when it's succesful, -1 when fails
 */
int get_arguments(int argc, char *argv[], char **input, char **output)
{
    int input_flag = 0;
    int output_flag = 0;

    if (argc != ARGUMENT_COUNT)
    {
        return -1;
    }
    for (int i = 1; i < argc - 1; ++i)
    {
        if (!input_flag && strcmp(argv[i], "-i") == 0)
        {
            *input = argv[i + 1];
            input_flag = 1;
        }
        else if (!output_flag && strcmp(argv[i], "-o") == 0)
        {
            *output = argv[i + 1];
            output_flag = 1;
        }
    }

    return (input_flag && output_flag) ? 0 : -1;
}

/**
 * @brief Copy from 1D array to 2D array.
 *
 * @param arr 2D array
 * @param buff 1D array
 */
void __copy_2D(uint8_t arr[NUMBER_OF_COORDINATE][NUMBER_OF_DIMENSION], const uint8_t buff[BUFF_SIZE])
{

    for (int i = 0; i < NUMBER_OF_COORDINATE; ++i)
    {
        for (int j = 0; j < NUMBER_OF_DIMENSION; ++j)
        {
            arr[i][j] = buff[(NUMBER_OF_DIMENSION * i) + j];
        }
    }
}

/**
 * @brief Create a child process for calculations
 *
 * @param arrCoordinate array of coordinates
 * @param outputFile output file name
 * @return the id of the process
 */
int __child_process(const uint8_t arrCoordinate[NUMBER_OF_COORDINATE][NUMBER_OF_DIMENSION], const char *outputFile)
{
    int pid;
    char *const argv[] = {(char *const)outputFile, NULL};
    char environmentKey[NUMBER_OF_COORDINATE * NUMBER_OF_DIMENSION][128];
    char environmentValue[NUMBER_OF_COORDINATE * NUMBER_OF_DIMENSION][128];

    pid = fork();

    if (pid < 0)
    {
        perror("Fork:");
        return -1;
    }
    else if (pid == 0)
    {
        for (int i = 0; i < NUMBER_OF_COORDINATE; i++)
        {
            for (int j = 0; j < NUMBER_OF_DIMENSION; j++)
            {
                sprintf(environmentKey[i], "C_%d_%d", i, j);
                sprintf(environmentValue[i], "%d", arrCoordinate[i][j]);
                setenv(environmentKey[i], environmentValue[i], 1);
            }
        }
        execve("./processR", argv, environ);
    }
    else
    {
        return pid;
    }

    return pid;
}

/**
 * @brief print coordinates of each process.
 *
 * @param pNum child process number
 * @param arr array of coordinates
 */
void print_coordinates(int pNum, const uint8_t arr[NUMBER_OF_COORDINATE][NUMBER_OF_DIMENSION])
{

    fprintf(stdout,"Created R_%d with ", pNum);
    for (int i = 0; i < NUMBER_OF_COORDINATE; i++)
    {
        fprintf(stdout,"(");
        for (int j = 0; j < NUMBER_OF_DIMENSION; j++)
        {
            fprintf(stdout,"%d", arr[i][j]);
            if (j != NUMBER_OF_DIMENSION - 1)
                fprintf(stdout,",");
        }
        fprintf(stdout,")");
        if (i != NUMBER_OF_COORDINATE - 1)
            fprintf(stdout,",");
    }
    fprintf(stdout,"\n");
}

/**
 * @brief Read input file for coordinates and create child processes for each coordinate.
 * *
 * @param fd file descriptor
 * @param outputFile output file name
 * @param pidList list of process ids to fill
 * @return 0 when it's succesful, -1 when fails
 */
int create_child_process(int fd, const char *outputFile, PIDList *pidList)
{
    int rd;
    uint8_t buff[BUFF_SIZE];
    uint8_t arrCoordinate[NUMBER_OF_COORDINATE][NUMBER_OF_DIMENSION];
    int found_coordinate_count = 0;
    int total_child = 0;

    do
    {
        NO_EINTR(rd = read(fd, buff, BUFF_SIZE));
        found_coordinate_count = rd / (NUMBER_OF_COORDINATE * NUMBER_OF_DIMENSION);

        for (int i = 0; i < found_coordinate_count; i++)
        {
            total_child++;
            __copy_2D(arrCoordinate, buff + (i * NUMBER_OF_COORDINATE * NUMBER_OF_DIMENSION));
            if (pidList->size >= pidList->cap)
            {
                pidList->cap *= 2;
                pidList->arr = realloc(pidList->arr, pidList->cap * sizeof(int));
                if (pidList->arr == NULL)
                {
                    perror("Realloc: ");
                    return -1;
                }
            }
            pidList->arr[(pidList->size)++] = __child_process(arrCoordinate, outputFile);
            if (pidList->arr[(pidList->size) - 1] == -1)
                return -2;
            print_coordinates(pidList->size, arrCoordinate);
        }
    } while (rd > 0);
    return pidList->size;
}

/**
 * @brief Reads the output file
 * 
 * @param fd file descriptor
 * @param matrixCount number of matrices from processes
 * @param result array of matrices
 * @return int 0 when succesful, -1 when fails
 */
int read_output_file(int fd, int matrixCount, double ***result)
{
    int rd;
    for (int i = 0; i < matrixCount; ++i)
    {
        for (int j = 0; j < NUMBER_OF_DIMENSION; ++j)
        {
            NO_EINTR(rd = read(fd, result[i][j], NUMBER_OF_DIMENSION * sizeof(double)));
            if (rd < 0)
            {
                perror("Read: ");
                return -1;
            }
        }
    }
    return 0;
}

/**
 * @brief Calculate frobius norm according to its formula.
 * 
 * @param matrix a matrix from a process
 * @return double result of the calculation 
 */
double frobius_norm(double **matrix)
{
    double sum = 0;
    for (int i = 0; i < NUMBER_OF_DIMENSION; ++i)
    {
        for (int j = 0; j < NUMBER_OF_DIMENSION; ++j)
        {
            sum += matrix[i][j] * matrix[i][j];
        }
    }
    return sqrt(sum);
}


void forward_interrupt_to_childs(const PIDList *pidList, int childCount, char *output){
    for (int i = 0; i < pidList->size; ++i){
        kill(pidList->arr[i], SIGINT);
    }

    for (int i = 0; i < childCount; ++i){
        NO_EINTR(wait(NULL));
    }
    remove(output);
}
