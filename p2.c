/*
 * Programmer: Chad Krauthamer
 * Class: CS570 Operating Systems
 * Professor: John Carroll
 * Semester: Fall 2018
 * Due Date: 12/5/18
 * */

#include "p2.h"
#include "CHK.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <signal.h>
#include <zconf.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

/************************* Global variables *************************/

char argv[MAXSTORAGE]; // Stores 254 characters, set in parse, filled by getword
char *newargv[MAXARGS]; // Char pointer array filled by parse
int argc; // Argument count
char *in, *out; // Pointers to file to redirect to/from
char prompt[PATH_MAX]; // Current directory
int pipe_commands[10]; // Index of pipe commands to be executed
int file_description[20]; // File descriptors for iterative pipeline
static int child = 0; // Whether it is a child process relative to main() 
char *delim; // Hereis deliminator

/************************* Global variables *************************/

int IN_FLAG;
int OUT_FLAG;
int AMP_FLAG;
int PIPE_FLAG;
int BAIL_FLAG;
int FORWARDSLASH_FLAG;
int HEREIS_FLAG;


/************************* File Descriptors *************************/

int input_file_description, output_file_description,
    devnull_file_description, hereis_file_description;

/************************* Main *************************/

int main() {
    pid_t shell_pgid;
    /* software termination signal from kill */
    (void) signal(SIGTERM, mySignalHandler);

    shell_pgid = getpid();
    if (setpgid(shell_pgid, shell_pgid) < 0) {
        report_error_and_exit("setpgid");
    }

    for (;;) {

        promptHelper();

        // Enter parse, exit when getword() catches an EOF
        if (parse() == -255) break;

        // No arguments, reprompt
        if (argc == 0) continue;

        // Null command
        if (newargv[0] == NULL) {
            (void) fprintf(stderr, "Invalid null command.\n");
            continue;
        }

        // handle bultins
        // CD Handler
        if (strcmp(newargv[0], "cd") == 0) {
            changeDir();
            continue;
        }

        // Environ Handler
        if (strcmp(newargv[0], "environ") == 0) {
            envHelper();
            continue;
        }

        // Handle Redirect Flags
        // Set Input File_Descriptors
        if (IN_FLAG != 0) {
            // More than one redirect
            if (IN_FLAG > 1) {
                (void) fprintf(stderr, "Ambiguous input redirect.\n");
                continue;
            }
            // Nowhere to redirect too
            if (in == NULL) {
                (void) fprintf(stderr, "Missing name for redirect.\n");
                continue;
            }
            /*
            * set the input file descriptions, if open is unsuccessful print error
            * in:
            * open for reading only
            * */
            if ((input_file_description = open(in, O_RDONLY)) < 0) {
                (void) fprintf(stderr, "%s: Invalid input file.\n", in);
                continue;
            }

        }

        // Set Output File Descriptors
        if (OUT_FLAG != 0) {
            /* Create if non-existant | error if already exists |  open for reading and writing */
            int flags = O_CREAT | O_EXCL | O_RDWR;

            // More than one redirect
            if (OUT_FLAG > 1) {
                (void) fprintf(stderr, "Ambiguous output redirect.\n");
                continue;
            }
            // Nowhere to redirect too
            if (out == NULL) {
                (void) fprintf(stderr, "Missing name for redirect.\n");
                continue;
            }
            /* Set the output file descriptions if open is successful
             * out:
             * flags: O_CREAT | O_EXCL | O_RDWR
             * [XSI] R for owner or [XSI] W for owner
             * */
            if ((output_file_description = open(out, flags, S_IRUSR | S_IWUSR)) < 0) {
                (void) fprintf(stderr, "%s: Invalid output file.\n", out);
                continue;
            }

        }

        // Set /dev/null File descriptors
        if (AMP_FLAG != 0 && IN_FLAG == 0) {
            int flags = O_RDONLY;

            /* Set the devnull file descriptions if open is successful
            * flags: O_RDONLY (open for reading only)
            * */
            if ((devnull_file_description = open("/dev/null", flags)) < 0) {
                report_error_and_exit("open");
            }
        }

        if(HEREIS_FLAG > 0 && BAIL_FLAG == 0) {
            // More than one redirect
            if (HEREIS_FLAG > 1) {
                (void) fprintf(stderr, "Ambiguous hereis.\n");
                continue;
            }
            // Nowhere to redirect too
            if (delim == NULL) {
                (void) fprintf(stderr, "No delimiter.\n");
                continue;
            }

            hereIsHelper(delim);
            if ((hereis_file_description = open("/tmp/foo.txt", O_RDONLY)) < 0){
                (void) fprintf(stderr, "%s: Invalid HEREIS file.\n", in);
                continue;
            }

            remove("/tmp/foo.txt");

        }

        if (BAIL_FLAG > 0) continue; // Undefined variable

        // No pipe, begin executing the args
        if (PIPE_FLAG == 0) {
            execute();
            continue;
        }

        // Check for correct amount of pipe values
        if (PIPE_FLAG < 10) {
            // Iterative Vertical Pipe
            pipe_line();
            // Recursive vertical or horizontal pipe
            // Vertical has issues (exec_pipe)
            // pipeHelper();
            continue;
        } else {
            (void) fprintf(stderr, "p2 only handles 10 pipes.\n");
        }

    }
    // kill as recommended in specs, a software termination signal from kill, exit with no errors
    killpg(getpgrp(), SIGTERM);
    exit(SUCCESS);
}


/************************* Execution Functions *************************/




void pipe_line() {
    // Initialize a process id for beginning executable and end executable
    pid_t beginning, middle, end;

    // Fill with filedescriptors
    int i;
    for (i = 0; i < PIPE_FLAG; i++) {
        pipe(file_description + (i * 2));
    }

    // Clear error and output stream to begin processing beginning pipe
    (void) fflush(stderr);
    (void) fflush(stdout);

    // Check for successful fork
    if ((beginning = fork()) < 0)
        fprintf(stderr, "Fork Failed...\n");
        // success
    else if (beginning == 0) {

        // Clear error and output stream to begin processing ending pipe statement
        (void) fflush(stderr);
        (void) fflush(stdout);

        // Check for successful fork
        if ((end = fork()) < 0)
            fprintf(stderr, "Fork Failed...\n");
        if (end == 0) {

            // Middle Command(s)
            int j;
            for (j = 0; j < PIPE_FLAG - 1; j++) {
                // Clear error and output stream to begin processing middle pipe statements
                (void) fflush(stderr);
                (void) fflush(stdout);
                middle = fork();

                // Parent execs child loops back and forks
                if (middle > 0) {
                    dup2(file_description[j * 2], STDIN_FILENO);
                    dup2(file_description[(j * 2) + 3], STDOUT_FILENO);

                    closePipes();

                    if (execvp(newargv[pipe_commands[j + 1]], newargv + pipe_commands[j + 1]) < 0) {
                        (void) fprintf(stderr, "%s: Command not found.\n", newargv[pipe_commands[j]]);
                        exit(1);
                    }
                }
            }

            dup2(file_description[(PIPE_FLAG - 1) * 2], STDIN_FILENO);

            if (OUT_FLAG == 1) {
                dup2(output_file_description, STDOUT_FILENO);
                close(output_file_description);
            }

            closePipes();

            if (execvp(newargv[pipe_commands[PIPE_FLAG]], newargv + pipe_commands[PIPE_FLAG]) < 0) {
                (void) fprintf(stderr, "%s: Command not found.\n", newargv[pipe_commands[PIPE_FLAG]]);
                exit(1);
            }
        }

        dup2(file_description[1], STDOUT_FILENO);

        if (IN_FLAG == 1) {
            dup2(input_file_description, STDIN_FILENO);
            close(input_file_description);
        }

        closePipes();
        /*
         * execute word at newargv[0], print an error if command is not found
         * */
        if (execvp(newargv[0], newargv) < 0) {
            (void) fprintf(stderr, "%s: Command not found.\n", newargv[0]);
            exit(1);
        }
    }

    // Parent Code
    closePipes();
    if (AMP_FLAG != 0) {
        (void) printf("%s [%d]\n", newargv[0], beginning);
        AMP_FLAG = 0;
    } else {
        for (;;) {
            pid_t pid;
            pid = wait(NULL);
            if (pid == beginning)
                break;
        }
    }
}

// Execute first command and call itself for the rest of the commands.
// Vertical pipeline, has issues with input to p2, otherwise works fine
static void exec_pipe(size_t pos, int in_fd) {
    // Invalid Pipe has issues
    if (newargv[pipe_commands[pos]] == NULL) {
        report_error_and_exit("Invalid pipe");
    }
    else if (pipe_commands[pos + 1] == 0) { /* last command */
        redirect(in_fd, STDOUT_FILENO); /* read from in_fd, write to STDOUT */
        execvp(newargv[pipe_commands[PIPE_FLAG-pos]], newargv + pipe_commands[PIPE_FLAG-pos]);
        report_error_and_exit("execvp last command");
    } else { /* $ <in_fd cmds[pos] >fd[1] | <fd[0] cmds[pos+1] ... */
        int fd[2]; /* output pipe */
        if (pipe(fd) == -1)
            report_error_and_exit("pipe");
        switch(fork()) {
            case -1:
                report_error_and_exit("fork");
            case 0: /* child: execute the rest of the commands */
                CHK(close(fd[0])); /* unused */
                CHK(close(in_fd)); /* unused */
                exec_pipe(pos + 1, fd[1]); /* execute the rest */
            default: /* parent: execute current command `cmds[pos]` */
                child = 1;
                CHK(close(fd[1])); /* unused */
                redirect(in_fd, STDOUT_FILENO);  /* read from in_fd */
                redirect(fd[0], STDIN_FILENO); /* write to fd[0] */
                execvp(newargv[pipe_commands[PIPE_FLAG-pos]], newargv + pipe_commands[PIPE_FLAG-pos]);
                report_error_and_exit("execvp");
        }
    }
}

// Execute last command and call itself for the rest of the commands.
// Horizontal pipeline, no issues
static void exec_pipeline(size_t pos, int in_fd) {
    // Invalid Pipe has issues
    if (newargv[pipe_commands[pos]] == NULL)
        report_error_and_exit("Invalid pipe");
    if (pipe_commands[pos + 1] == 0) { /* last command */
        redirect(in_fd, STDIN_FILENO); /* read from in_fd, write to STDOUT */
        execvp(newargv[pipe_commands[pos]], newargv + pipe_commands[pos]);
        report_error_and_exit("execvp last command");
    }
    else { /* $ <in_fd cmds[pos] >fd[1] | <fd[0] cmds[pos+1] ... */
        int fd[2]; /* output pipe */
        if (pipe(fd) == -1)
            report_error_and_exit("pipe");
        switch(fork()) {
            case -1:
                report_error_and_exit("fork");
            case 0: /* child: execute current command `cmds[pos]` */
                child = 1;
                close(fd[0]); /* unused */
                redirect(in_fd, STDIN_FILENO);  /* read from in_fd */
                redirect(fd[1], STDOUT_FILENO); /* write to fd[1] */
                execvp(newargv[pipe_commands[pos]], newargv + pipe_commands[pos]);
                report_error_and_exit("execvp");
            default: /* parent: execute the rest of the commands */
                close(fd[1]); /* unused */
                close(in_fd); /* unused */
                exec_pipeline(pos + 1, fd[0]); /* execute the rest */
        }
    }
}

// Helper function to set up redirects

void pipeHelper() {
    pid_t dad;
    (void) fflush(stdin);
    (void) fflush(stdout);
    // Check for successful fork
    if ((dad = fork()) < 0)
        report_error_and_exit("fork");
    else if (dad == 0) {

        // Catch input redirect
        // Copy input file description to standard in descriptor then close
        if (IN_FLAG == 1) {
            redirect(input_file_description, STDIN_FILENO);
        }
        // Catch output redirect
        // Copy output file description to standard out descriptor then close
        if (OUT_FLAG == 1) {
            redirect(output_file_description, STDOUT_FILENO);
        }
        // Catch input redirect
        if (HEREIS_FLAG && BAIL_FLAG == 0) {
            redirect(hereis_file_description, STDIN_FILENO);
        }

        // Redirect to /dev/null
        if (AMP_FLAG != 0 && IN_FLAG == 0) {
            redirect(devnull_file_description, STDIN_FILENO);
        }
        // Vertical
        exec_pipe(0, STDIN_FILENO);
        // Horizontal
        // exec_pipeline(0, STDIN_FILENO);

    } else {
        if (AMP_FLAG != 0) {
            (void) printf("%s [%d]\n", newargv[0], dad);
            AMP_FLAG = 0;
        } else {
            for (;;) {
                pid_t pid;
                pid = wait(NULL);
                if (pid == dad)
                    break;
            }
        }
    }
}

void execute() {
    pid_t c_pid; // child process id

    // Clear stdin and output stream to begin processing
    // fflush forces a write to console of buffered data
    (void) fflush(stdin);
    (void) fflush(stdout);
    // Check for successful fork
    if ((c_pid = fork()) < 0)
        report_error_and_exit("fork");
    else if (c_pid == 0) {

        // Catch hereis redirect
        if (HEREIS_FLAG && BAIL_FLAG == 0) {
            redirect(hereis_file_description, STDIN_FILENO);
        }
        // Catch /dev/null redirect
        if (AMP_FLAG != 0 && IN_FLAG == 0) {
            redirect(devnull_file_description, STDIN_FILENO);
        }
        // Catch input redirect
        if (IN_FLAG == 1) {
            redirect(input_file_description, STDIN_FILENO);
        }
        // Redirect output to file
        if (OUT_FLAG == 1) {
            redirect(output_file_description, STDOUT_FILENO);
        }
        // Execute word at newargv[0], print an error if command is not found
        if (execvp(newargv[0], newargv) < 0) {
            (void) fprintf(stderr, "%s: Command not found.\n", newargv[0]);
        }

    } else {
        // parent code
        if (AMP_FLAG != 0) {
            (void) printf("%s [%d]\n", newargv[0], c_pid);
            AMP_FLAG = 0;
        } else {
            // Parent waits for child
            for (;;) {
                pid_t pid;
                pid = wait(NULL);
                if (pid == c_pid) break;
            }
        }
    }
}

/************************* Builtin Functions *************************/

void envHelper() {
    char *env;
    FORWARDSLASH_FLAG = 0; // Handles weird '/' case
    // One arg, try to print it
    if (argc == 2) {
        if ((env = getenv(newargv[1])) != 0)
            (void) printf("%s\n", env);
        else
            (void) printf("%s\n","\0");
      // Two args, try to change env
    } else if (argc == 3) {
        if (strcmp(newargv[2], "/") == 0) FORWARDSLASH_FLAG++;
        if (setenv(newargv[1], newargv[2], 1) < 0)
            (void) fprintf(stderr, "environ: Unable to set environment to %s.\n", newargv[2]);
    } else {
        (void) fprintf(stderr, "environ: Invalid input.\n");
    }
    if (AMP_FLAG != 0) {
        (void) printf("%s [%d]\n", newargv[0], getpid());
        AMP_FLAG = 0;
    }
}

void changeDir() {
    /**
     * Argc == 1 value means one word (cd)
     * Second value in newargv will be directory to try to change too
     * Handle errors
     * Set prompt so that it has current dir
     * */

    if (argc == 1) {
        if (chdir(getenv("HOME")) == 0) { getcwd(prompt, sizeof(prompt)); }
        else
            (void) fprintf(stderr, "Invalid Home Directory.\n");
    } else if (argc == 2) {
        if (chdir(newargv[1]) == 0) { getcwd(prompt, sizeof(prompt)); }
        else
            (void) fprintf(stderr, "%s: No Such FIle Or Directory.\n", newargv[1]);
    } else
        (void) fprintf(stderr, "cd: Too many arguments.\n");

    if (AMP_FLAG != 0) {
        (void) printf("%s [%d]\n", newargv[0], getpid());
        AMP_FLAG = 0;
    }
}

/************************* Utility Functions *************************/

void mySignalHandler() { printf("p2 terminated.\n"); }

// Close pipes for pip_line
void closePipes() {
    int i;
    for (i = 0; i < PIPE_FLAG * 2; i++)
        close(file_description[i]);
}

// Prompt Helper function
void promptHelper() {
    char *last = strrchr(prompt, '/');
    if (last != NULL) {
        printf("%s:570: ", last + 1);
    } else {
        if (FORWARDSLASH_FLAG != 0)
            printf("/:570: "); // prompt user
        else
            printf(":570: "); // prompt user
    }

}

void report_error_and_exit(const char *msg) {
    perror(msg);
    (child ? _exit : exit)(EXIT_FAILURE);
}

// Move oldfd to newfd 
void redirect(int oldfd, int newfd) {
    if (oldfd != newfd) {
        if (dup2(oldfd, newfd) != -1)
            CHK(close(oldfd));
        else
            report_error_and_exit("dup2");
    }
}

// Handles ~username look ups
char *userLookUp(char *str) {
    FILE *fp;
    char *line = NULL;
    char *currLine = NULL;
    size_t len = 0;
    int field = 1;
    strtok(str, "/");
    
    if ((fp = fopen("/etc/passwd", "r")) == NULL) {
        report_error_and_exit("fopen");
    }

    // Check if username exists, then return home field 
    while (getline(&line, &len, fp) != -1) {
        currLine = strtok(line, ":");
        if ((strcmp(currLine, str)) == 0) {
            while (currLine != NULL) {
                if (field == 6) {
                    return currLine;
                }
                field++;
                currLine = strtok(NULL, ":");
            }
            break;
        }
    }

    // Loop failed
    free(line);

    //Close the file if still open.
    if (fp) {
        fclose(fp);
    }

    (void) fprintf(stderr, "%s: Username does not exist\n", str);
    BAIL_FLAG=1;
    return NULL;
}

// Hereis Helper
void hereIsHelper(char *delimiter){
    FILE *f;
    char *line = NULL;
    char *currLine = NULL;
    size_t len = 0;

    if ((f = fopen ("/tmp/foo.txt", "w+")) == NULL)
        report_error_and_exit("fopen");

    // Loop through stdin and get lines until delim is found
    while (getline(&line, &len, stdin) != -1) {
        currLine = strtok(line, "\n");
        // Delimiter found
        while (currLine != NULL) {
            if ((strcmp(currLine, delimiter)) == 0) {
                fclose(f);
                free(line);
                return;
            }
            currLine = strtok(NULL, " ");
        }
        if (strcmp(line, "\n") == 0) continue;
        else fprintf(f,"%s\n",line);
    }

    // Failure
    fclose(f);
    free(line);
    BAIL_FLAG=1;
    (void) fprintf(stderr, "Error: Delimiter not found\n");
}

int parse() {
    // Variable declarations
    int wordSize = 0;
    char *ch = argv;

    // Reset Global Variables
    in = NULL, out = NULL, argc = 0,
            AMP_FLAG = 0, PIPE_FLAG = 0, BAIL_FLAG = 0,
                           OUT_FLAG = 0, IN_FLAG = 0, HEREIS_FLAG = 0;

    devnull_file_description = 0, hereis_file_description = 0;

    memset(newargv, 0, sizeof(newargv));
    memset(argv, 0, sizeof(argv));
    memset(pipe_commands, 0, sizeof(pipe_commands));

    for (;;) {
        // Getword returns word size
        wordSize = getword(ch);

        // Delimiter found, break loop
        if (wordSize == 0 || wordSize == -255) break;

        // Handle '&' stuff
        // Check for backslash flag
        if (*ch == '&' && wordSize == 1 && BACKSLASH_FLAG == 0) {
            wordSize = getword(ch += 2);
            // If it is the last word, set flag
            if (wordSize == 0 || wordSize == -255) {
                AMP_FLAG = 1;
                break;
            }
            newargv[argc++] = ch - 2;
        }
        if (*ch == '<' && wordSize == 2) {
            HEREIS_FLAG++;
            wordSize = getword(ch += 2);
            if (wordSize < -255) {
                if (getenv(ch + 1) == NULL) {
                    (void) fprintf(stderr, "%s: Undefined variable.\n", ch);
                    argc = 0;
                    BAIL_FLAG=1;
                    break;
                }
            } else if (wordSize < 0) {
                if (getenv(ch) == NULL) {
                    (void) fprintf(stderr, "%s: Undefined environment variable.\n", ch);
                    BAIL_FLAG=1;
                    break;
                }
            } else if (wordSize > 0)
                delim = ch;
                //hereIsHelper(ch);
            else{
                HEREIS_FLAG = 0;
                BAIL_FLAG=1;
                (void) fprintf(stderr, "No hereis delimiter.\n");
                break;
            }
        }
        // I/O redirection
        // Input Redirection
        else if (*ch == '<' && wordSize == 1) {
            IN_FLAG++;
            // Move down *ch to get name of input file
            wordSize = getword(ch += 2);

            if (wordSize < -255) {
                if ((in = getenv(ch + 1)) == NULL) {
                    (void) fprintf(stderr, "%s: Undefined variable.\n", ch);
                    argc = 0;
                    BAIL_FLAG=1;
                }
            } else if (wordSize < 0) {
                if ((in = getenv(ch)) == NULL) {
                    (void) fprintf(stderr, "%s: Undefined environment variable.\n", ch);
                    BAIL_FLAG=1;
                }
            } else
                in = ch;
            // Output Redirection
        } else if (*ch == '>' && wordSize == 1) {
            OUT_FLAG++;
            wordSize = getword(ch += 2);

            if (wordSize < -255) {
                if ((out = getenv(ch + 1)) == NULL) {
                    (void) fprintf(stderr, "%s: Undefined variable.\n", ch);
                    argc = 0;
                    BAIL_FLAG=1;
                }
            } else if (wordSize < 0) {
                if ((out = getenv(ch)) == NULL) {
                    (void) fprintf(stderr, "%s: Undefined environment variable.\n", ch);
                    BAIL_FLAG=1;
                }
            }else if (wordSize == 1) {
                if (*ch == '>') {
                    (void) fprintf(stderr, "%s: Bad Usage.\n", ch);
                    BAIL_FLAG=1;
                }
            } else
                out = ch;
        } else if (*ch == '|' && wordSize == 1) {
            PIPE_FLAG++;
            // Place a null after command, place command in
            // Pipe commands at index of PIPE_FLAG
            newargv[argc++] = NULL;
            pipe_commands[PIPE_FLAG] = argc;
        } else {
            if (wordSize < -255) {
                if ((newargv[argc++] = getenv(ch + 1)) == NULL) {
                    (void) fprintf(stderr, "%s: Undefined variable.\n", ch);
                    argc = 0;
                }
            } else {
                // Dollar sign found, replace word
                if (wordSize < 0) {
                    if ((newargv[argc++] = getenv(ch)) == NULL) {
                        (void) fprintf(stderr, "%s: Undefined environment variable.\n", ch);
                        BAIL_FLAG=1;
                    }
                }
                // User look up, replace word
                else if (USER_FLAG) {
                    char *dest;
                    char *src = strpbrk(ch, "/");

                    if (src != NULL) {
                        char *ssrc = strdup(src);
                        dest = userLookUp(ch);
                        strcat(dest, ssrc);
                    } else
                        dest = userLookUp(ch);
                    newargv[argc++] = dest;
                } else
                    newargv[argc++] = ch;
            }
        }
        ch += abs(wordSize) + 1;
    }
    // Success
    newargv[argc] = NULL;
    return wordSize;
}
