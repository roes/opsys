/* */

#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define READ 0
#define WRITE 1

pid_t childpid;
int pipe_filedesc[3][2]; /* Stores the pipe filedescriptors */
char** arguments; /* Arguments to grep */

void check(int return_value, char* message);
void child(char* program, int in_pipe, int out_pipe);

int main(int argc, char** argv){
  int return_value; /* Holds return values from system calls */
  int status; /* Holds return values from child processes */
  int i;
  /* Setup parameters to use in child processes */
  int use_grep = (argc > 1)?1:0; /* Has value 1 if grep should be used */
  arguments = argv;
  int num_programs = (use_grep)?4:3; /* Number of programs to run */
  char* pager = (getenv("PAGER")!=NULL)?getenv("PAGER"):"less"; /* pager to use */
  char* programs[4]; /* List of programs to execute */
  if (use_grep) { programs[0] = "printenv"; programs[1] = "grep"; programs[2] = "sort"; programs[3] = pager; }
  else { programs[0] = "printenv"; programs[1] = "sort"; programs[2] = pager; }

  for(i=0; i < num_programs; i++){
    if (i < num_programs-1){ /* Add new pipes for all but last child */
      return_value = pipe(pipe_filedesc[i]);
      check(return_value, "Cannot create pipe");
    }
    childpid = fork();
    if(0 == childpid){ /* Execute child process. Doesn't return from this */
      child(programs[i],(i==0)?-1:i-1,(i==num_programs-1)?-1:i); /* send -1 if read/write from stdin/stdout */
    }
    check(childpid, "Cannot fork()");
    if (i > 0){ /* Close pipe in parent, except after first child */
      return_value = close(pipe_filedesc[i-1][READ]);
      check(return_value, "Cannot close read end");
      return_value = close(pipe_filedesc[i-1][WRITE]);
      check(return_value, "Cannot close write end");
    }
  }

  for(i=0; i < num_programs; i++){ /* Wait for children to finish */
    childpid = wait(&status);
    check(childpid, "wait() failed unexpectedly");
    if(WIFEXITED(status)){ /* Child process exited */
      int child_status = WEXITSTATUS(status);
      if(0 != child_status){
        fprintf(stderr, "Child (pid %ld) failed with exit code %d\n",
                (long int) childpid, child_status);
      }
    } else if(WIFSIGNALED(status)){ /* Child process received signal */
      int child_signal = WTERMSIG(status);
      fprintf(stderr, "Child (pid %ld) was terminated by signal no. %d\n",
              (long int) childpid, child_signal);
    }
  }
  exit(0);
}

void check(int return_value, char* message){
  if (-1 == return_value){ perror(message); exit(1);}
}

void child(char* program, int in_pipe, int out_pipe){
  int return_value;

  if(-1 != in_pipe){ /* If a pipe should be used for input set it up and then close it */
    return_value = dup2(pipe_filedesc[in_pipe][READ], STDIN_FILENO);
    check(return_value, "Cannot dup");
    return_value = close(pipe_filedesc[in_pipe][READ]);
    check(return_value, "Cannot close read end");
    return_value = close(pipe_filedesc[in_pipe][WRITE]);
    check(return_value, "Cannot close write end");
  }
  if(-1 != out_pipe){ /* If a pipe should be used for output set it up and then close it */
    return_value = dup2(pipe_filedesc[out_pipe][WRITE], STDOUT_FILENO);
    check(return_value, "Cannot dup");
    return_value = close(pipe_filedesc[out_pipe][READ]);
    check(return_value, "Cannot close read end");
    return_value = close(pipe_filedesc[out_pipe][WRITE]);
    check(return_value, "Cannot close write end");
  }
  /* Exec the program */
  if (strcmp(program, "grep") == 0){ /* grep is special case since it takes parameters */
    arguments[0] = "grep";
    (void) execvp(program, arguments); /* Only returns if failed */
  }
  else {
    (void) execlp(program, program, (char *) 0); /* Only returns if failed */
  }
  check(-1,strcat(program," cannot exec"));
}

