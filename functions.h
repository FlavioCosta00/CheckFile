#ifndef FUNCTIONS
#define FUNCTIONS

#include "debug.h"
#include "args.h"
#include "constants.h"

int countlines(char *filename);
void signal_treatment(int sig, siginfo_t *siginfo, void *context);
void deleteAuxFile(char filename[]);
void getExtension(char *string);
void getFileType(char *string);
void getDirectoryFiles(char directory[]);
int compare(char filename[MAX_FILE_NAME_BYTES], char vector[][100], char *name_of_file);
void output(int num_file, char *vector_files[], char file_extension[][100], char file_type[][100], int summary, int batch);

#endif