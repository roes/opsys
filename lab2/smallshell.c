/*
 *
 *
 *
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_COMMAND_SIZE 80
#define MAX_NB_ARGUMENTS 6
#define MAX_PATH_LENGTH 100
#define FOREGROUND 0
#define BACKGROUND 1


/* parse_command
 *
 */
void parse_command(char *cmd_string,    /* IN: Commandline input string */
                   char *cmd_argv[],    /* OUT: Array of arguments */
                   int *mode);          /* OUT: bg or fg process */


int main(int argc, char *argv[]){
  char cmd_string[MAX_COMMAND_SIZE];
  char *cmd_argv[MAX_NB_ARGUMENTS];
  int mode;
  char *home = getenv("HOME");
  char current_wd[MAX_PATH_LENGTH];
  getcwd(current_wd, MAX_PATH_LENGTH);

  while(1){
    int return_value;


    /*
     * Read and parse commandline input.
     */

    mode = FOREGROUND;
    fprintf(stdout, "%s$ ", current_wd);
    fgets(cmd_string, MAX_COMMAND_SIZE, stdin);
    parse_command(cmd_string, cmd_argv, &mode);


    if(cmd_argv[0] == '\0') continue;   /* No command read, continue */


    /*
     * Change working directory.
     */

    else if(strcmp(cmd_argv[0],"cd") == 0){
      if (cmd_argv[1] != '\0'){         /* If a path is supplied, change to it */
        return_value = chdir(cmd_argv[1]);
      }
      if (cmd_argv[1] == '\0' ||        /* If no argument or change failed */
          return_value == -1){          /* change to home */
        return_value = chdir(home);
        if (return_value == -1) fprintf(stderr, "Could not cd to HOME");
      }
      getcwd(current_wd, MAX_PATH_LENGTH);
    }


  }
}


void parse_command(char *cmd_string, char *cmd_argv[], int *mode){
  char *p = cmd_string;
  while(*p == ' ') p++;
  while(*p != '\0'){
    *cmd_argv = p;                      /* Adds word pointer from input to
                                           argument array */
    while(*p != ' ' && *p != '\0' &&
          *p != '\n'){                  /* Loops past all chars in current word */
      if(*p == '&') *mode = BACKGROUND;
      p++;
    }
    while(*p == ' ' || *p == '\n'){     /* Loops past all word separators and
                                           replace with null char */
      *p = '\0';
      p++;
    }
    cmd_argv++;                         /* Points to next element in argument
                                           array */
  }
  *cmd_argv = '\0';                     /* Set last array element to null char */
}

