/*
 * Programmer: Chad Krauthamer
 * Class: CS570 Operating Systems
 * Professor: John Carroll
 * Semester: Fall 2018
 * Due Date: 11/27/18
 * */

#include "getword.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <signal.h>
#include <zconf.h>
#include <fcntl.h>

/************************* Storage *************************/

#define MAXSTORAGE 2048 /* One more than getword()'s maximum wordsize */
#define MAXARGS 100 // Maximum number of arguments
#define PATH_MAX 4096 // POSIX Standard

/************************* Exit Codes *************************/

#define SUCCESS 0
#define SHELL_GPID_ERROR 1
#define DEVNULL_ERROR 2
#define COMMAND_NOT_FOUND 3
#define EXIT_FILE_FAILURE 4
#define EXIT_USER_FAILURE 5

/************************* Global Flags *************************/

int BACKSLASH_FLAG;
int USER_FLAG;

/************************* Builtin Functions *************************/

void changeDir();
void envHelper();

/************************* Execution Functions *************************/

void execute();
void pipe_line();
void pipeHelper();

/************************* Utility Functions *************************/

void promptHelper();
char *userLookUp(char *str);
void report_error_and_exit(const char *msg);
void redirect(int oldfd, int newfd);
int parse();
void mySignalHandler();
void hereIsHelper(char *delimiter);
void closePipes();
void pipe_line();
