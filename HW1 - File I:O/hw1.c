#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

typedef struct str
{
    char *loc;
    int len;

} str;

typedef struct FilePath
{
    str str1;
    str str2;
    int insensitive;
    int start;
    int end;

} FilePath;

typedef struct Array
{
    char *str;
    int size;
    int capacity;
} Array;

void init_struct(FilePath *path)
{
    path->insensitive = 0;
    path->start = 0;
    path->end = 0;
}

void init_array(Array *arr)
{
    arr->capacity = 1024;
    arr->size = 0;
    arr->str = (char *)calloc(arr->capacity, (sizeof(char)));
}

void reallocate_arr(Array *arr)
{
    arr->capacity *= 2;
    arr = realloc(arr, sizeof(char) * arr->capacity);
}

int check_valid_char(const char *in)
{   
    int count = 0;
    for (int i = 0; i < strlen(in); i++)
    {
        if (((in[i] >= '0' && in[i] <= '9') || (in[i] >= 'a' && in[i] <= 'z') || (in[i] >= 'A' && in[i] <= 'Z') ||
            (in[i] == '/') || (in[i] == '*') || (in[i] == '$') || (in[i] == '^') || (in[i] == '[') || (in[i] == ']') || (in[i] == ';')))
        {
            count++;
        }
    }

    if(count == strlen(in)) return 1;

    return 0;
}

int check_arr_without_star(str *arr, char c)
{
    for (int i = 0; i < arr->len; i++)
    {
        if (c == arr->loc[i])
        {
            return 1;
        }
    }

    return 0;
}

int check_arr_cond(FilePath *path, int *t, const char *inputFile, int *fileInd)
{
    int count = 0;
    int flag = 0;
    char c = inputFile[*fileInd];
    while (path->str1.loc[*t + count] != ']')
    {
        count++;
    }

    str arr;
    arr.len = count;
    arr.loc = &(path->str1.loc[*t]);
    *t += (count + 1);
    if (path->str1.loc[*t] == '*')
    {
        flag = 1;
        (*t)++;
    }

    //[arr] condition
    if (flag == 0)
    {
        return check_arr_without_star(&arr, c);
    }

    //[arr]* condi
    while (*fileInd < strlen(inputFile) && check_arr_without_star(&arr, c))
    {
        (*fileInd)++;
        c = inputFile[*fileInd];
    }
    (*fileInd)--;
    return 1;
}

void check_special_chars(FilePath *path)
{

    if (path->str1.loc[0] == '^')
    {
        path->start = 1;
        path->str1.loc += 1;
        path->str1.len -= 1;
    }
    if (path->str1.loc[path->str1.len - 1] == '$')
    {
        path->end = 1;
        path->str1.len -= 1;
    }
}

void parse_path(char *input, FilePath **paths, int *fileCount)
{
    int i = 0;
    int count = 0;
    int flag = 0;

    while (i != strlen(input) - 1)
    {
        if (input[i] == '/')
        {
            count = 0;
            i++;

            if (flag == 0)
            {
                if (*fileCount > 0)
                {
                    *paths = (struct FilePath *)realloc(*paths, (*fileCount + 1) * sizeof(struct FilePath));
                }

                (*paths)[*fileCount].str1.loc = &input[i];
            }

            else
            {
                (*paths)[*fileCount].str2.loc = &input[i];
            }

            while (i < strlen(input) && input[i] != '/')
            {
                count++;
                i++;
            }

            if (flag == 0)
            {
                (*paths)[*fileCount].str1.len = count;
                check_special_chars(&(*paths)[*fileCount]);
                flag = 1;
            }
            else
            {

                (*paths)[*fileCount].str2.len = count;
                flag = 0;

                if (i < strlen(input) - 1 && input[i + 1] == 'i')
                {
                    i += 1;
                    (*paths)[*fileCount].insensitive = 1;
                }

                (*fileCount)++;

                if (i < strlen(input) - 1 && input[i + 1] == ';')
                {
                    i += 2;
                }

                else
                {
                    break;
                }
            }
        }
    }
}

void to_lowercase(char *str, int len)
{
    for (int i = 0; i < len; i++)
    {
        if (str[i] >= 'A' && str[i] <= 'Z')
        {
            str[i] = str[i] + 32;
        }
    }
}

int replace_str_for_a_path(FilePath *path, int *fileInd, const char *inputFile, const char *lowerInput)
{
    int t = 0;
    int flag = 1;
    int len = strlen(inputFile);
    int i;

    if (path->str1.loc[0] == '*')
    {
        write(2, "You should add a charachter before * symbol.\n", 46);
        exit(EXIT_FAILURE);
    }

    // ^ condition
    if (path->start && !(*fileInd == 0 || inputFile[*fileInd - 1] == '\n'))
    {
        flag = 0;
    }

    for (i = *fileInd; i < len && t < path->str1.len && flag == 1; i++)
    {

        // insensitive condition
        if (path->insensitive == 1)
        {
            to_lowercase(path->str1.loc, path->str1.len);
            inputFile = lowerInput;
        }

        //* condition
        if (t < path->str1.len && path->str1.loc[t + 1] == '*')
        {
            char repeat = path->str1.loc[t];
            while (i < len && repeat == inputFile[i])
            {
                i++;
            }
            i--;
            t += 2;
            flag = 1;
        }

        // arr condition
        else if (path->str1.loc[t] == '[')
        {
            t++;
            flag = check_arr_cond(path, &t, inputFile, &i);
        }

        // basic condition
        else if (inputFile[i] == path->str1.loc[t])
        {
            t++;
            flag = 1;
        }
        else
        {
            flag = 0;
        }
    }

    // $ condition
    if (path->end && !(i >= strlen(inputFile) - 1 || inputFile[i] == '\n'))
    {
        flag = 0;
    }

    if (t != path->str1.len)
    {
        flag = 0;
    }

    if (flag)
    {
        *fileInd = i;
    }
    return flag;
}

void copy_string(char *dest, char *source, int srcLen)
{
    for (int i = 0; i < srcLen; ++i)
    {
        dest[i] = source[i];
    }
}

Array *replace_str(FilePath *paths, int *fileCount, const char *inputFile)
{
    int isChanged = 0;
    Array *outputFile = (Array *)malloc(sizeof(Array));
    char *lowerInput = (char *)malloc(strlen(inputFile) * sizeof(char));

    strcpy(lowerInput, inputFile);
    to_lowercase(lowerInput, strlen(lowerInput));
    init_array(outputFile);

    for (int i = 0; i < strlen(inputFile); i++)
    {
        for (int j = 0; j < *fileCount; j++)
        {
            FilePath path = paths[j];

            if (replace_str_for_a_path(&path, &i, inputFile, lowerInput) == 1)
            {
                isChanged = 1;

                if (outputFile->size >= outputFile->capacity)
                {
                    reallocate_arr(outputFile);
                }
                i--;
                copy_string(outputFile->str + outputFile->size, path.str2.loc, path.str2.len);
                
                outputFile->size += path.str2.len;
                break;
            }

            else
            {
                isChanged = 0;
            }
        }
        if (isChanged == 0)
        {
            outputFile->str[outputFile->size] = inputFile[i];
            (outputFile->size)++;
        }
    }
    free(lowerInput);
    return outputFile;
}

char *split_argv(const char *argv)
{
    char *path = (char *)malloc(strlen(argv) * (sizeof(argv)));
    strncpy(path, argv, strlen(argv));
    return path;
}

void fill_array(char *buff, int len){
    for (int i = 0; i < len; i++)
    {
        buff[i] = '\0';
    }
    
}

void file_ops(const char *fileName, char *inputArgv)
{
    FilePath *paths = calloc(1, sizeof(struct FilePath));
    int pathCount = 0;
    char buffer[1024];
    char tempFile[] = "tempfile-XXXXXX"; // create temp file to temporary copy output string.
    char *bp;
    int bytesread;
    int byteswritten = 0;
    struct flock lock;
    int fCloseStatus;

    int fd = open(fileName, O_RDONLY); // open for reading
    if (fd < 0)
    {
        perror("File couldn't be opened.\n");
        exit(EXIT_FAILURE);
    }

    int tempFd = mkstemp(tempFile);
    lock.l_type = F_WRLCK;
    fcntl(fd, F_SETLKW, &lock);
    fcntl(tempFd, F_SETLKW, &lock);
    while (1)
    {
        while (((bytesread = read(fd, buffer, 1024)) == -1) && errno == EINTR); // handle intruption

        if (bytesread < 0)
        {
            perror("An error ocured while reading.");
            exit(EXIT_FAILURE);
        }
        
        else if (bytesread == 0)
        {
            break;
        }

        parse_path(inputArgv, &paths, &pathCount);
        Array *outputString = replace_str(paths, &pathCount, buffer);   
        bp = outputString->str;
        int outSize = outputString->size;

        while (outSize > 0)
        {
            while (((byteswritten = write(tempFd, bp, outSize)) == -1) && errno == EINTR);
            if (byteswritten < 0)
            {
                perror("write: "); 
                exit(EXIT_FAILURE);
            }

            outSize -= byteswritten;
            bp += byteswritten;

        }
        free(outputString->str);
        free(outputString);
        if (byteswritten == -1)
        {
            perror("Error occured while writing.\n");
            exit(EXIT_FAILURE);
        }
    }
    
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLKW, &lock);
    fcntl(tempFd, F_SETLKW, &lock);
    fCloseStatus = close(fd);
    if (fCloseStatus < 0)
    {
        perror("close: ");
        exit(EXIT_FAILURE);
    }
    rename(tempFile, fileName);
    
    fCloseStatus = close(tempFd);
    if (fCloseStatus < 0)
    {
        perror("close: ");
        exit(EXIT_FAILURE);
    }
    
    free(paths);
}

int main(int argc, char const *argv[])
{
    if (argc != 3)
    {
        write(2, "Invalid format. Format should be: ./program ‘/str1/str2/‘ inputFilePath\n", 61);
        exit(EXIT_FAILURE);
    }
    const char *fileName = argv[2];
    char *inputArgv = split_argv(argv[1]);
    if (!check_valid_char(inputArgv))
    {
        write(2, "Invalid format. Format should be: ./program ‘/str1/str2/‘ inputFilePath\n", 61);
        exit(EXIT_FAILURE);
    }

    file_ops(fileName, inputArgv);
    free(inputArgv);
    exit(EXIT_SUCCESS);
}