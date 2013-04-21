/*
 *
 *
 *
 */

#include <sys/types.h>
#include <stdio.h>

#define MAX_COMMAND_SIZE 80
#define MAX_NB_ARGUMENTS 6
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

  while(1){

    /*
     * Read and parse commandline input.
     */

    mode = FOREGROUND;
    fgets(cmd_string, MAX_COMMAND_SIZE, stdin);
    parse_command(cmd_string, cmd_argv, &mode);


  }
}


void parse_command(char *cmd_string, char *cmd_argv[], int *mode){
  char *p = cmd_string;
  while(*p == ' ') p++;
  while(*p != '\0'){
    *cmd_argv = p;                      /* Adds word pointer from input to
                                           argument array */
    while(*p != ' ' && *p != '\0'){     /* Loops past all chars in current
                                           word */
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

