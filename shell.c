#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

/* definition of functions */
static char *read_line(char *line, int buffer);
char ** parse_args();
int my_system(const char *command);

void exit_(void);
void cd(const char *path);

struct cmd *getcmd(const char *name);

extern int errno;

/* table structure for extra commands on the mini-shell */
struct cmd {
    const char *c_name; // command name

    /* 
     * Handlers for command depending of nature of parameters. Values can be
     * NULL; however, one of them must be non-NULL
     */
    void (*c_handler_v)(int, char **);
    void (*c_handler_0)(void);
    void (*c_handler_1)(const char *);
};

/* List of extra commands for the mini-shell */
struct cmd cmdtab[] = {
    { "exit", NULL, exit_, NULL },
    { "cd", NULL, NULL, cd },
    { 0, 0, 0, 0 }
};

/* line to be read from the stdin */
char line[200];

/**
 * Main function of the mini-shell.
 * 
 * @return int
 */
int main(void)
{   
    int l;
    while (1) {
        if (read_line(line, sizeof(line)) == NULL) {
            fprintf(stderr, "Unexpected error while reading command.\n");
            break;
        }  
        
        /* Cleaning up break line from the input*/
        l = strlen(line);
        if (l == 0)
            break;
        if (line[--l] == '\n') {
            if (l == 0)
                continue;
            line[l] = '\0';
        } else if (l == sizeof(line) - 2) {
            
            /* Prints error if command line is longer that buffer characters */
            fprintf(stderr, "Command line is too long.\n");
            continue;
        }   
        
        /* Executes command read from the terminal */
        if(my_system(line) == -1)
            fprintf(stderr, "Unexpected error while executing command.\n");
    }
    return EXIT_SUCCESS;
}

/**
 * Imitates the C library's system function behaviour using the execvp function.
 * 
 * @param command
 * @return 
 */
int my_system(const char *command)
{
    char *tmpcmd;
    int l;
    char **margv;
    int amp = 0; // Flag ampersand
    pid_t cpid;
    int cstatus;
    int wait_opt;
    struct cmd *c;
    
    /* creates a copy of the command */
    tmpcmd = strdup(command);
    l = strlen(tmpcmd);
    
    /* Verifies if last parameter is an & */
    if (tmpcmd[l - 1] == '&') {
        tmpcmd[l - 1] = '\0';
        amp = 1;
    }

    /* Parsing line to get the command and args entered */
    margv = parse_args(tmpcmd);
    
    /* Exits if command line is equal to exit */
/*
    if (strcmp(margv[0], cmd[0]) == 0)
        exit(EXIT_SUCCESS);
*/
    
    /* 
     * Searches command in the custom command lists first; if not found, continues
     * execution with the execvp() function.
     */
    c = getcmd(margv[0]);
    
    /* If command exists on the list, executes handler. */
    if (c != NULL ) {
        if (c->c_handler_v) {
            c->c_handler_v(*margv[0], &margv[0]);
            return 0;
        } else if (c->c_handler_0) {
            c->c_handler_0();
            return 0;
        } else if (c->c_handler_1) {
            c->c_handler_1(margv[1]);
            return 0;
        }
    }

    /* Creates a child process to execute command */
    cpid = fork();
    if (cpid == -1) {
        return -1;
    } else if(cpid == 0) {
        
        /* If process is a child */
        errno = 0;
        execvp(margv[0], &margv[0]);
        if (errno == ENOENT)
            fprintf(stderr, "%s: command not found\n", margv[0]);
        else
            fprintf(stderr, "Unexpected error while executing command.\n");
        exit(EXIT_FAILURE);
    } else if (cpid > 0) {
        
        /* Default: shell waits for children. */
        wait_opt = 0;
        
        /* If &, shell continues. */
        if (amp) 
            wait_opt = WNOHANG;
            
        if (waitpid(cpid, &cstatus, wait_opt) == -1)
            return -1;
        else if (WIFEXITED(cstatus)) 
            return WEXITSTATUS(cstatus);
        
        /* If child process is running on background means cstatus >= 0 */
        if (amp && &cstatus != NULL && cstatus >= 0)
            return 0;
        
        return -1;
    }
}

/**
 * Read command input from stdin
 * 
 * @param line
 * @param buffer
 * @return 
 */
static char *read_line(char *line, int buffer)
{
    printf("CLI> ");
    fflush(stdout);
    return fgets(line, buffer, stdin);
}

/**
 * Parse command string into an array.
 * 
 * @param command
 * @return 
 */
char ** parse_args(char *command)
{
    static char *rargv[20];
    char **argp;
    const char delim[] = " ";
    char *tmpline;
    
    /* Creates a copy of the read line */
    tmpline = strdup(command);

    argp = rargv;

    /* get the first token */
    *argp = strtok(tmpline, delim);
       
    /* walk through other tokens */
    while (*argp != NULL) {
        argp++;
        *argp = strtok(NULL, delim);
    }
    return rargv;
}

struct cmd *getcmd(const char *name)
{
    const char *p, *q;
    struct cmd *c;
    for (c = cmdtab; (p = c->c_name) != NULL; c++) {
        for (q = name; *q == *p++; q++)
                if (*q == 0)
                        return (c);
    }
    return NULL;
}

/**
 * Function that executes the 'exit command'
 * @type command handler
 */
void exit_(void)
{
    exit(EXIT_SUCCESS);
}

/**
 * Executes directory change for the shell
 * @type command handler
 * @param path
 */
void cd(const char *path)
{
    // executes chdir
    chdir(path);
}