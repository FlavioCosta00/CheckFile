/**
 * @file functions.c
 * @brief checkFile
 * @date 2021-10-25
 * @author FLávio Costa 2201707
 * @author Simão Troeira 2201701
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <dirent.h>
#include <time.h>

#include "debug.h"
#include "memory.h"
#include "args.h"
#include "constants.h"
#include "functions.h"

char signal_file[MAX_FILE_NAME_BYTES];
int signal_file_num = 0;

int countlines(char *filename) // count the number of lines in the file called filename
{
    FILE *file;
    int ch = 0;
    int lines = 0;
    if ((file = fopen(filename, "r")) == NULL) //Open file and check if file exists
    {
        exit(EXIT_FAILURE);
    }
    while (!feof(file))
    {
        ch = fgetc(file);
        if (ch == '\n')
        {
            lines++;
        }
    }
    fclose(file);
    return lines;
}

void signal_treatment(int sig, siginfo_t *siginfo, void *context) //function to treat the signals recieved
{
    time_t mytime;
    mytime = time(NULL);
    struct tm tm = *localtime(&mytime);
    (void)context;
    (void)*siginfo;
    int aux;
    /* Copy of global variable errno */
    aux = errno;

    if (sig == SIGQUIT)
    { //verify if signal is SIGQUIT
        printf("Captured SIGQUIT signal (sent by PID: %d). Use SIGINT to terminate application.\n", getpid());
        pause();
    }

    if (sig == SIGUSR1)
    { //verify if signal is SIGUSR1
        printf("%d.%d.%d_%dh%d:%d\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
        printf("nº %d/%s\n", signal_file_num, signal_file);
    }

    /* Restores the global variable value of errno */
    errno = aux;
}

void deleteAuxFile(char filename[])
{
    int pid = fork();
    if (pid == -1)
    {
        exit(EXIT_FAILURE);
    }
    if (pid == 0)
    {
        //Child process
        int err = execlp("/bin/rm", "rm", filename, NULL);
        if (err == -1)
        {
            printf("Could not execute 'rm' command\n");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        int wstatus;
        //Parent process
        wait(&wstatus);
        if (WIFEXITED(wstatus))
        {
            int statusCode = WEXITSTATUS(wstatus);
            if (statusCode != 0)
            {
                printf("Failure with status code %d\n", statusCode);
            }
        }
    }
}

void getExtension(char *string)
{
    int fd[2];
    if (pipe(fd) == -1)
    {
        exit(EXIT_FAILURE);
    }
    int pid1 = fork();
    if (pid1 < 0)
    {
        exit(EXIT_FAILURE);
    }
    if (pid1 == 0)
    {
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);
        close(fd[0]);
        int err = execlp("/bin/echo", "echo", string, NULL);
        if (err == -1)
        {
            printf("Could not execute 'echo' command\n");
            exit(EXIT_FAILURE);
        }
    }
    int pid2 = fork();
    if (pid2 < 0)
    {
        exit(EXIT_FAILURE);
    }
    if (pid2 == 0)
    {
        int file = open(FILE_EXTENSION, O_WRONLY | O_CREAT, 0777);
        if (file == -1)
        {
            exit(EXIT_FAILURE);
        }
        dup2(fd[0], STDIN_FILENO);
        dup2(file, STDOUT_FILENO);
        close(fd[1]);
        close(fd[0]);
        close(file);
        int err = execlp("/bin/awk", "awk", "-F", ".", "{print $NF}", NULL);
        if (err == -1)
        {
            printf("Could not execute 'awk -F . {print $NF}' command\n");
            exit(EXIT_FAILURE);
        }
    }
    close(fd[1]);
    close(fd[0]);
    waitpid(pid2, NULL, 0);
    waitpid(pid1, NULL, 0);
}

void getFileType(char string[])
{
    int pid = fork();
    if (pid == -1)
    {
        exit(EXIT_FAILURE);
    }
    if (pid == 0)
    {
        //Child process
        int file = open(FILE_TYPE, O_WRONLY | O_CREAT, 0777);
        if (file == -1)
        {
            exit(EXIT_FAILURE);
        }
        dup2(file, STDOUT_FILENO);
        close(file);
        int err = execlp("/bin/file", "file", "-b", "--mime-type", string, NULL);
        if (err == -1)
        {
            printf("Could not execute 'file -b --mime-type' command\n");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        int wstatus;
        //Parent process
        wait(&wstatus);
        if (WIFEXITED(wstatus))
        {
            int statusCode = WEXITSTATUS(wstatus);
            if (statusCode != 0)
            {
                printf("Failure with status code %d\n", statusCode);
            }
        }
    }
}

void getDirectoryFiles(char directory[])
{
    int pid = fork();
    if (pid == -1)
    {
        exit(EXIT_FAILURE);
    }
    if (pid == 0)
    {
        //Child process
        int file = open(DIRECTORY_FILE, O_WRONLY | O_CREAT, 0777);
        if (file == -1)
        {
            exit(EXIT_FAILURE);
        }
        dup2(file, STDOUT_FILENO);
        close(file);
        int err = execlp("/bin/find", "find", directory, "-maxdepth", "1", "-type", "f", NULL);
        if (err == -1)
        {
            printf("Could not execute 'find -type f command\n");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        int wstatus;
        //Parent process
        wait(&wstatus);
        if (WIFEXITED(wstatus))
        {
            int statusCode = WEXITSTATUS(wstatus);
            if (statusCode != 0)
            {
                printf("Failure with status code %d\n", statusCode);
            }
        }
    }
}

int compare(char filename[MAX_FILE_NAME_BYTES], char vector[][100], char *name_of_file)
{
    FILE *file;
    if ((file = fopen(filename, "rb")) == NULL)
    {
        fprintf(stderr, "[ERROR]: cannot open file <%s> -- %s\n", filename, strerror(errno));
        exit(EXIT_FAILURE);
    }
    char stringAux[MAX_FILE_NAME_BYTES];
    while (1)
    {
        if (fgets(stringAux, MAX_FILE_NAME_BYTES, file) == NULL)
        {
            break;
        }
    }
    stringAux[strcspn(stringAux, "\n")] = 0; //remove \n from String
    for (int i = 0; i < 7; i++)
    {
        if (strcmp(stringAux, vector[i]) == 0)
        {
            return i;
        }
    }
    char string[MAX_FILE_NAME_BYTES] = "";
    string[strcspn(string, "\n")] = 0; //remove \n from String
    sprintf(string, "cannot open `%s' (No such file or directory)", name_of_file);
    if (strcmp(stringAux, string) == 0)
    {
        strcpy(vector[8], string);
        return -1;
    }
    strcpy(vector[8], stringAux);
    return 8;
}

void output(int num_files, char *vector_files[], char file_extension[][100], char file_type[][100], int summary, int batch)
{
    int num_analyzed = 0, num_ok = 0, num_mismatch = 0, num_not_supported = 0, num_err = 0; //Accountants
    for (int i = 0; i < num_files; i++)
    {
        getFileType(vector_files[i]);
        int compared_type = compare(FILE_TYPE, file_type, vector_files[i]);
        getExtension(vector_files[i]);
        int compared_extension = compare(FILE_EXTENSION, file_extension, vector_files[i]);
        num_analyzed++;
        if (batch == 1)
        {
            struct sigaction act;
            act.sa_sigaction = signal_treatment; //Records signal handling routine
            sigemptyset(&act.sa_mask);           //Mask without signs to not block them
            act.sa_flags = SA_RESTART;           //retrieves blocking calls
            act.sa_flags |= SA_SIGINFO;          //additional info about the signal
            signal_file_num++;
            strcpy(signal_file, vector_files[i]);
            if (sigaction(SIGUSR1, &act, NULL) < 0) //SIGUSR1 signal capture
                ERROR(1, "sigaction - SIGUSR1");
        }
        if (compared_type == -1)
        {
            fprintf(stderr, "[ERROR] cannot open file '%s' -- No such file or directory\n", vector_files[i]);
            num_err++;
        }
        else
        {
            if (compared_type == 8)
            {
                printf("[INFO] '%s': type '%s' is not supported by checkFile\n", vector_files[i], file_type[8]);
                num_not_supported++;
            }
            else
            {
                if (compared_type == compared_extension)
                {
                    printf("[OK] '%s': extension '%s' matches file type '%s'\n", vector_files[i], file_extension[compared_extension], file_type[compared_type]);
                    num_ok++;
                }
                else
                {
                    printf("[MISMATCH] '%s': extension is '%s', file type is '%s'\n", vector_files[i], file_extension[8], file_type[compared_type]);
                    num_mismatch++;
                }
            }
        }
        deleteAuxFile(FILE_EXTENSION);
        deleteAuxFile(FILE_TYPE);
    }
    if (summary == 1)//Prints summary if mode is -b or -d
    {
        printf("[SUMMARY] files analyzed:%d; files OK:%d; files MISMATCH:%d; files not SUPPORTED: %d; errors:%d\n", num_analyzed, num_ok, num_mismatch, num_not_supported, num_err);
    }
}
