/*
 * NAME:
 *   smallshell  -  a small shell that runs basic commands
 *
 * SYNTAX:
 *   smallshell
 *
 * DESCRIPTION:
 *   Smallshell is a small shell. It supports running commands in
 *   the foreground or background (using &). It supports commands
 *   found in the directories listed in PATH. Built-in commands are
 *   cd [path] and exit.
 *
 * ENVIRONMENT:
 *   PATH, HOME
 *
 * NOTES:
 *   |, <, > and ; are not supported.
 *   A command can be up to 70 characters and have a maximum
 *   of 5 parameters.
 */
 
#define _XOPEN_SOURCE 500				/* Needed for sighold and sigrelse */

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define MAX_COMMAND_SIZE 80
#define MAX_NB_ARGUMENTS 7
#define MAX_PATH_LENGTH 100
#define FOREGROUND 0
#define BACKGROUND 1


/* parse_command
 *
 * parse_command parses commandline input and stores
 * it in the given argument array. Also sets mode to
 * FOREGROUND or BACKGROUND.
 */
void parse_command(char *cmd_string,    /* IN: Commandline input string */
                   char *cmd_argv[],    /* OUT: Array of arguments */
                   int *mode);          /* OUT: bg or fg process */


/* time_passed
 *
 * time_passed returns the time passed from start to end.
 */
struct timeval time_passed(struct timeval start, /* Start time */
                           struct timeval end);  /* End time */


/* sighandler
 *
 * sighandler handles the parameters to sigaction()
 */
void sighandler(int signal_code,          /* The signal */
                void (*handler)(int sig), /* Function that should handle the signal */
 			    int flags);               /* flags */


int main(int argc, char *argv[]){
  char cmd_string[MAX_COMMAND_SIZE];    /* Current commandline entry */
  char *cmd_argv[MAX_NB_ARGUMENTS];     /* Arguments to exec */
  int mode;                             /* fg or bg process */
  char *home = getenv("HOME");          /* Home directory path */
  char current_wd[MAX_PATH_LENGTH];     /* Current working directory */
  getcwd(current_wd, MAX_PATH_LENGTH);

  while(1){
    int return_value;                   /* Return value from system calls */
	sighandler(SIGINT, SIG_IGN, 0);	    /* The shell should not terminate
	                                       from a SIGINT */                    
	
    /*         
     * Read and parse commandline input.
     */

    mode = FOREGROUND;
    fprintf(stdout, "%s$ ", current_wd);
    fgets(cmd_string, MAX_COMMAND_SIZE, stdin);
    parse_command(cmd_string, cmd_argv, &mode);


    if(cmd_argv[0] == '\0') continue;   /* No command read, continue */
    else if(strcmp(cmd_argv[0],"exit") == 0) exit(0);


    /*
     * Change working directory.
     */

    else if(strcmp(cmd_argv[0],"cd") == 0){
      if(cmd_argv[1] != '\0'){          /* If a path is supplied, change to it */
        return_value = chdir(cmd_argv[1]);
      }
      if(cmd_argv[1] == '\0' ||         /* If no argument or change failed */
         return_value == -1){           /* change to home */
        return_value = chdir(home);
        if(return_value == -1){ perror("Could not cd to HOME"); exit(1); }
      }
      getcwd(current_wd, MAX_PATH_LENGTH);
    }


    /*
     * Start new process running supplied command.
     */

    else {
      pid_t childpid;                   /* Holds pid returned from fork() */
      sighandler(SIGINT, NULL,          /* Reset SIGINT handler, the children should */
      			 SA_RESETHAND);	        /* not ignore SIGINT */
                                                   
      
      childpid = fork();
      if(childpid < 0){ perror("Cannot fork()"); exit(1); }


      /*
       * Child process
       */

      else if(childpid == 0){
        (void) execvp(cmd_argv[0], cmd_argv);
        perror("Cannot exec");
        exit(1);
      }


      /*
       * Parent process
       */
      else {       
      	sighandler(SIGINT, SIG_IGN, 0); /* Reignore SIGINT */
        int status;                     /* Holds status returned from wait() */
        pid_t return_pid;               /* Holds pid returned from wait() */
        struct timeval start, end, diff;/* Holds start, end and run time */
        fprintf(stderr, "Spawned %s process pid: %d\n",
            (mode == FOREGROUND)?"foreground":"background", childpid);


        /*
         * Running a foreground process.
         * Timing and waiting for the process to finish.
         */

        if(mode == FOREGROUND){
          int terminated = 0;			/* Set to 1 when the forground 
                                           process is terminated */
          return_value = gettimeofday(&start, NULL);          
          if(return_value == -1){ perror("Couldn't get time"); exit(1); }
          do {                          /* Wait for processes to finish until
                                           current foreground process is done */
            return_pid = wait(&status);
            
            if(return_pid == -1){ perror("Wait failed"); exit(1); }
            if(WIFEXITED(status) ||     /* Check if the child terminated */
               WIFSIGNALED(status)){
              fprintf(stderr, "%s process %d terminated\n",
                      (return_pid == childpid)?"Foreground":"Background",
                      return_pid);
              if(return_pid == childpid) terminated = 1;}
          } while(!terminated);
          return_value = gettimeofday(&end, NULL);
          if(return_value == -1){ perror("Couldn't get time"); exit(1); }
          diff = time_passed(start, end);
          fprintf(stderr, "process took: %d:%.6d s\n",
                  (int)diff.tv_sec, (int)diff.tv_usec);


        /*
         * Running a background process.
         * Checking if any background processes have finished
         * since last process start.
         */

        } else {
          do {                          /* Checks while there still are already
                                           terminated processes, non-blocking */
            return_pid = waitpid(-1, &status, WNOHANG);
            if(return_pid == -1){ perror("Wait failed"); exit(1); }
            if(return_pid != 0){
              if(WIFEXITED(status) ||     /* Check if the child terminated */
                 WIFSIGNALED(status))
                fprintf(stderr, "Background process %d terminated\n", return_pid);
            }
          } while(return_pid != 0);
        }
      }
    }
  }
}


void parse_command(char *cmd_string, char *cmd_argv[], int *mode){
  char *p = cmd_string;                 /* Holds current character position */
  while(*p == ' ' || *p == '\n') p++;
  while(*p != '\0'){
    *cmd_argv = p;                      /* Adds word pointer from input to
                                           argument array */
    while(*p != ' ' && *p != '\0' &&
          *p != '&' && *p != '\n'){     /* Loops past all chars in current word */
      p++;
    }
    while(*p == ' ' || *p == '\n' ||     /* Loops past all word separators and */
          *p == '&'){                    /* replace with null char */
      if(*p == '&') *mode = BACKGROUND;
      *p = '\0';
      p++;
    }
    cmd_argv++;                         /* Points to next element in argument
                                           array */
  }
  *cmd_argv = '\0';                     /* Set last array element to null char */
}


struct timeval time_passed(struct timeval start, struct timeval end){
  struct timeval diff;                  /* Holds time difference end-start */
  diff.tv_sec = end.tv_sec-start.tv_sec;
  diff.tv_usec = end.tv_usec-start.tv_usec;
  if(diff.tv_usec<0){ diff.tv_usec += 1000000; diff.tv_sec--; }
  return diff;
}

void sighandler(int signal_code, void (*handler)(int sig), int flags ) {
	int return_value;
	struct sigaction new_sa;
	new_sa.sa_handler = handler;
	new_sa.sa_flags = flags;
	return_value = sigaction( signal_code, &new_sa, (void *) 0 );	
	if(return_value == -1){ perror("sigaction() failed"); exit(1); }
}

