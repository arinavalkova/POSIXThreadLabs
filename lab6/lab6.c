#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

char *inPath, *outPath;

int nonIntrRead(int fd, void* buf, size_t count)
{
    while (1)
    {
        int ret = read(fd, buf, count);
        if (ret == -1 && errno == EINTR)
            continue;
        return ret;
    }
}

int nonIntrWrite(int fd, void* buf, size_t count)
{
    while(1)
    {
        int wr = write(fd, buf, count);
        if(wr == -1 && errno == EINTR)
            continue;
        return wr;
    }
}

pthread_t newThreadCreate(void *(*function)(void *), void *path)
{
    pthread_t pthread;
    int threadCreateStatus;

    do
    {
        threadCreateStatus = pthread_create(&pthread, NULL, function, path);
        if(threadCreateStatus == EAGAIN)
        {
            printf("Waiting resources to create new thread...\n");
            sleep(1);
        }
    }
    while (threadCreateStatus != 0);
    pthread_detach(pthread);

    return pthread;
}

char *concatDirectoriesPaths(char *firstPath, char *secondPath)
{
    char* currentPath = (char*) malloc(strlen(firstPath) + strlen(secondPath) + 2);

    while(currentPath == NULL)
    {
        sleep(1);
        currentPath  = (char*) malloc(strlen(firstPath) + strlen(secondPath) + 2);
    }

    memcpy(currentPath, firstPath, strlen(firstPath));
    memcpy(currentPath + strlen(firstPath), "/", 1);
    memcpy(currentPath + strlen(firstPath) + 1, secondPath, strlen(secondPath) + 1);

    return currentPath;
}

int openInFile(char *path)
{
    int status;

    do
    {
        status = open(path, O_RDONLY);
        if(status < 0 && errno == EMFILE)
        {
            printf("Waiting opening file %s ...\n", path);
            sleep(1);
        }
        else
        {
            return status;
        }
    }
    while(status < 0);

    return status;
}

int openOutFile(char *inPath, char *outPath)
{
    int status;
    struct stat buff;

    if(stat(inPath, &buff) == -1)
    {
        printf("Error: Can't get information from path %s\n", inPath);
        return -1;
    }

    do
    {
        status = open(outPath, O_WRONLY | O_CREAT, buff.st_mode);
        if(status < 0 && errno == EMFILE)
        {
            printf("Waiting opening %s ...\n", outPath);
            sleep(1);
        }
        else
        {
            return status;
        }
    }
    while(status < 0);

    return status;
}

void *copyFile(void *parameters)
{
    int readStatus, writeStatus;
    char* gotPath = (char*)parameters;
    char *buf;

    char* allPathToGotInPath = concatDirectoriesPaths(inPath, gotPath);
    char* allPathToGotOutPath = concatDirectoriesPaths(outPath, gotPath);

    free(parameters);

    int inFile = openInFile(allPathToGotInPath);
    if(inFile == -1)
    {
        printf("Error: Can't open %s\n", allPathToGotInPath);

        free(allPathToGotInPath);
        free(allPathToGotOutPath);

        return (void *) EXIT_FAILURE;
    }

    int outFile = openOutFile(allPathToGotInPath, allPathToGotOutPath);
    if(outFile == -1)
    {
        printf("Error: Can't open %s\n", allPathToGotOutPath);

        if(close(inFile) == -1)
        {
            printf("Error: Can't close file %s\n", allPathToGotInPath);
        }
        free(allPathToGotInPath);
        free(allPathToGotOutPath);

        return (void *) EXIT_FAILURE;
    }

    buf = (char*) malloc(1024);
    do
    {
        readStatus = nonIntrRead(inFile, buf, 1024);
        if(readStatus == -1)
        {
            printf("Error: Read error %s\n", allPathToGotInPath);

            free(buf);

            if(close(inFile) == -1)
            {
                printf("Error: Can't close file %s\n", allPathToGotInPath);
            }
            if(close(outFile) == -1)
            {
                printf("Error: Can't close file %s\n", allPathToGotOutPath);
            }

            free(allPathToGotInPath);
            free(allPathToGotOutPath);

            return (void *) EXIT_FAILURE;
        }

        writeStatus = nonIntrWrite(outFile, buf, readStatus);
        if(writeStatus == -1)
        {
            printf("Error: Write error %s\n", allPathToGotOutPath);

            free(buf);

            if(close(inFile) == -1)
            {
                printf("Error: Can't close file %s\n", allPathToGotInPath);
            }
            if(close(outFile) == -1)
            {
                printf("Error: Can't close file %s\n", allPathToGotOutPath);
            }

            free(allPathToGotInPath);
            free(allPathToGotOutPath);

            return (void *) EXIT_FAILURE;
        }
    }
    while(readStatus > 0);

    printf("Copied %s ...\n", allPathToGotInPath);

    free(allPathToGotInPath);
    free(allPathToGotOutPath);

    if(close(inFile) == -1)
    {
        printf("Error: Can't close file %s\n", allPathToGotInPath);
    }
    if(close(outFile) == -1)
    {
        printf("Error: Can't close file %s\n", allPathToGotOutPath);
    }
    free(buf);

    return EXIT_SUCCESS;
}

int makeOutPathDirectory(char *path)
{
    int makeDirStatus = mkdir(path, ACCESSPERMS);

    if (makeDirStatus == -1 && errno == EACCES)
    {
        printf("Error: Search permission is denied on a component of the path prefix, or write permission"
               " is denied on the parent directory of the directory to be created.\n");
        return makeDirStatus;
    }
    else if (makeDirStatus == -1 && errno == EEXIST)
    {
        printf("Error: The named file exists.\n");
        return makeDirStatus;
    }

    return  makeDirStatus;
}

DIR *openInDirectory(char *path)
{
    DIR *openedDirectory;
    do
    {
        openedDirectory = opendir(path);
        if(openedDirectory == NULL && errno == EACCES)
        {
            printf("Error: Access is denied %s\n", path);
            break;
        }
        else if(openedDirectory == NULL && errno == EMFILE)
        {
            sleep(1);
        }
        else if(openedDirectory == NULL && errno == ENFILE)
        {
            sleep(1);
        }
        else if(openedDirectory == NULL)
        {
            break;
        }
    }
    while(openedDirectory == NULL);

    return openedDirectory;
}

int isInPathIsFile(struct stat buff)
{
    return S_ISREG(buff.st_mode);
}

int isInPathIsDirectory(struct stat buff)
{
    return S_ISDIR(buff.st_mode);
}


void* copyDirectory(void* parameters)
{
    DIR *openedDirectory;

    char* gotPath = (char*)parameters;
    char* currentInPath = concatDirectoriesPaths(inPath, gotPath);
    char* currentOutPath = concatDirectoriesPaths(outPath, gotPath);

    char* spaceForDirent[sizeof(struct dirent) + pathconf(currentInPath, _PC_NAME_MAX) + 2];
    struct dirent *unknownFile = (struct dirent *)spaceForDirent;
    struct dirent *result;

    struct stat buff;

    char *newPathWithGottenDirectoryWithoutInPath;
    char *newPathWithGottenDirectory;

    openedDirectory = openInDirectory(currentInPath);
    if(openedDirectory == NULL)
    {
        printf("Error: Can't open directory %s\n", currentInPath);

        free(currentInPath);
        free(currentOutPath);
        free(parameters);

        return (void *) EXIT_FAILURE;
    }

    if(makeOutPathDirectory(currentOutPath) == -1)
    {
        if(closedir(openedDirectory) == -1)
        {
            printf("Error: Can't close directory %s\n", currentInPath);
        }
        free(currentInPath);
        free(currentOutPath);
        free(parameters);

        return (void *) EXIT_FAILURE;
    }

    while(readdir_r(openedDirectory, unknownFile, &result) == 0 && result != NULL )
    {
        pthread_t thread;

        char* currentGottenFromDirectoryPath = unknownFile->d_name;
        if(strcmp(currentGottenFromDirectoryPath, ".") == 0 || strcmp(currentGottenFromDirectoryPath, "..") == 0)
            continue;

        newPathWithGottenDirectory = concatDirectoriesPaths(currentInPath, currentGottenFromDirectoryPath);
        if(stat(newPathWithGottenDirectory, &buff) == -1)
        {
            printf("Error: Can't get information from path %s\n", newPathWithGottenDirectory);
            continue;
        }

        free(newPathWithGottenDirectory);

        newPathWithGottenDirectoryWithoutInPath = concatDirectoriesPaths(gotPath, currentGottenFromDirectoryPath);

        if(isInPathIsDirectory(buff))
        {
            thread = newThreadCreate(copyDirectory, newPathWithGottenDirectoryWithoutInPath);
        }
        else if(isInPathIsFile(buff))
        {
            thread = newThreadCreate(copyFile, newPathWithGottenDirectoryWithoutInPath);
        }
    }

    printf("Copied %s ...\n", currentInPath);

    if(closedir(openedDirectory) == -1)
    {
        printf("Error: Can't close directory %s\n", currentInPath);
    }

    free(currentInPath);
    free(currentOutPath);
    free(parameters);

    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    pthread_t thread;
    struct stat buff;
    char* emptyLine = malloc(1);

    if (argc < 3)
    {
        printf("Usage: %s inPath outPath\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    inPath = argv[1];
    outPath = argv[2];

    if(stat(inPath, &buff) == -1)
    {
        printf("Error: Can't get information about path %s\n", inPath);
        exit(EXIT_FAILURE);
    }

    if(isInPathIsDirectory(buff))
    {
        thread = newThreadCreate(copyDirectory, emptyLine);
    }
    else
    {
        printf("Error: Can't find directory to copy\n");
        exit(EXIT_FAILURE);
    }

    pthread_exit(NULL);
}
