/**
 * @file main.c
 * @brief checkFile
 * @date 2021-10-25
 * @author FLávio Costa 2201707
 * @author Simão Troeira 2201701
 */
//Libraries
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

int main(int argc, char *argv[])
{
    struct gengetopt_args_info args_info;//struct of gengetopt
    struct sigaction act;//struct of signals
    char file_type[8][100] = {"image/png", "image/gif", "video/mp4", "application/zip", "text/html", "image/jpeg", "application/pdf", ""};//Array of supported types
    char file_extension[8][100] = {"png", "gif", "mp4", "zip", "html", "jpg", "pdf", ""};//Array of supported extensions
    if (cmdline_parser(argc, argv, &args_info) != 0)
        exit(1);

    act.sa_sigaction = signal_treatment;    //Records signal handling routine
    sigemptyset(&act.sa_mask);              //Mask without signs to not block them
    act.sa_flags = SA_RESTART;              //retrieves blocking calls
    act.sa_flags |= SA_SIGINFO;             //additional info about the signal
    if (sigaction(SIGQUIT, &act, NULL) < 0) //SIGQUIT signal capture
        ERROR(1, "sigaction - SIGQUIT");

    if (args_info.batch_given)
    {
        char line[MAX_FILE_NAME_BYTES];
        FILE *file;
        if ((file = fopen(args_info.batch_arg, "r")) == NULL)
        {
            fprintf(stderr, "[ERROR]: cannot open file <%s> -- %s\n", args_info.batch_arg, strerror(errno));
            exit(EXIT_FAILURE);
        }
        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        rewind(file);
        if (0 == size) // Check if file is empty
        {
            printf("File: <%s> its empty ! \n", args_info.batch_arg);
            exit(EXIT_SUCCESS);
        }
        int num_files = countlines(args_info.batch_arg);
        char *vector_files[num_files];
        int i = 0;
        while (fgets(line, MAX_FILE_NAME_BYTES, file) != NULL)
        {
            vector_files[i] = strdup(line);
            vector_files[i][strcspn(vector_files[i], "\n")] = 0; //Remove \n from string
            i++;
        }
        fclose(file);
        printf("[INFO] analyzing files listed in ‘%s’\n", args_info.batch_arg);
        output(num_files, vector_files, file_extension, file_type, 1, 1);
    }

    if (args_info.dir_given)
    {
        DIR *dir = opendir(args_info.dir_arg); // Open Directory
        if (dir)                               // Directory exists
        {
            getDirectoryFiles(args_info.dir_arg);
            char line[MAX_FILE_NAME_BYTES];
            FILE *file;
            if ((file = fopen(DIRECTORY_FILE, "r")) == NULL)
            {
                fprintf(stderr, "[ERROR]: cannot open file <%s> -- %s\n", DIRECTORY_FILE, strerror(errno));
                exit(EXIT_FAILURE);
            }
            fseek(file, 0, SEEK_END);
            long size = ftell(file);
            rewind(file);
            if (0 == size) // Check if file is empty
            {
                deleteAuxFile(DIRECTORY_FILE);
                printf("Directory: <%s> its empty ! \n", args_info.dir_arg);
                exit(EXIT_SUCCESS);
            }
            int num_files = countlines(DIRECTORY_FILE);
            char *vector_files[num_files];
            int i = 0;
            while (fgets(line, MAX_FILE_NAME_BYTES, file) != NULL)
            {
                vector_files[i] = strdup(line);
                vector_files[i][strcspn(vector_files[i], "\n")] = 0;
                i++;
            }
            fclose(file);
            printf("[INFO] analyzing files of directory '%s'\n", args_info.dir_arg);
            output(num_files, vector_files, file_extension, file_type, 1, 0);
            deleteAuxFile(DIRECTORY_FILE);
            closedir(dir);
        }
        else //Directory does not exist
        {
            fprintf(stderr, "[ERROR]: cannot open dir <%s> -- %s\n", args_info.dir_arg, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    if (args_info.file_given)
    {
        int num_files = 0;
        char *vector_files[args_info.file_given];
        for (unsigned int i = 0; i < args_info.file_given; i++)
        {

            vector_files[i] = malloc((strlen(args_info.file_arg[i]) + 1));
            num_files++;
            strcpy(vector_files[i], args_info.file_arg[i]);
        }
        output(num_files, vector_files, file_extension, file_type, 0, 0);
    }

    cmdline_parser_free(&args_info);
    return 0;
}
