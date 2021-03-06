#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "tokenizer.h"

/* Convenience macro to silence compiler warnings about unused function parameters. */
#define unused __attribute__((unused))

/* Whether the shell is connected to an actual terminal or not. */
bool shell_is_interactive;

/* File descriptor for the shell input */
int shell_terminal;

/* Terminal mode settings for the shell */
struct termios shell_tmodes;

/* Process group id for the shell */
pid_t shell_pgid;

int cmd_exit(struct tokens *tokens);
int cmd_help(struct tokens *tokens);
int cmd_pwd(struct tokens *tokens);
int cmd_cd(struct tokens *tokens);

/* Built-in command functions take token array (see parse.h) and return int */
typedef int cmd_fun_t(struct tokens *tokens);

/* Built-in command struct and lookup table */
typedef struct fun_desc {
  cmd_fun_t *fun;
  char *cmd;
  char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
  {cmd_help, "?", "show this help menu"},
  {cmd_exit, "exit", "exit the command shell"},
  {cmd_pwd,"pwd","print the current working directory"},
  {cmd_cd,"cd","change to the specified directory"}
};

/* Prints a helpful description for the given command */
int cmd_help(unused struct tokens *tokens) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
  return 1;
}

/* Exits this shell */
int cmd_exit(struct tokens *tokens) {
  tokens_destroy(tokens);
  exit(0);
}

int cmd_pwd(unused struct tokens *tokens){
  printf("%s\n",getcwd(NULL,0));
  return 1;
}

int cmd_cd(struct tokens *tokens){
  if(tokens_get_length(tokens)>2){
    printf("cd: too many arguments\n");
    return -1;
  }
  if(chdir(tokens_get_token(tokens,1))==-1){
    printf("cd : No such file or directory\n");
    return -1;
  }
  else return 0;
}

void sigintHandler(int sig_num)
{
    signal(SIGINT, sigintHandler);
    fflush(stdout);
}

int cmd_run_programs(struct tokens *tokens){
  size_t size = tokens_get_length(tokens);
  char * execv_str[size+1];
  int i = 0;
  int count = 0;
  int status;
  pid_t childpid = fork();
  if(childpid==0){



    char *path = getenv("PATH");
    char *paths[20];
    char *token = strtok(path,":");
    
    while( token != NULL ) {
      paths[count] = token;
      token = strtok(NULL, ":");
      ++count;
    }


    for (i=0;i<size;++i){
      execv_str[i] = tokens_get_token(tokens,i);
    }
    execv_str[size] = NULL;



    char buffer[200];
    for(i=0;i<count;++i){
      strcpy(buffer,paths[i]);
      strcat(buffer,"/");
      strcat(buffer,execv_str[0]);
      if(access(buffer, X_OK)==0) {
        break;
      }
      else{
        strcpy(buffer,tokens_get_token(tokens,0));
      }
    }


    if(size>2){
      if(strcmp(execv_str[size-2],"<")==0) {//in
        int fd0 = open(execv_str[size-1],O_RDONLY);
        dup2(fd0, STDIN_FILENO);
        close(fd0);
        execv_str[size-2] = NULL;
      }
      else if(strcmp(execv_str[size-2],">")==0){//out
        int fd1 = open(execv_str[size-1] , O_WRONLY|O_CREAT|O_TRUNC , 0777) ;
        dup2(fd1, STDOUT_FILENO);
        close(fd1);
        execv_str[size-2] = NULL;
      }
    }


    int result = execv(buffer,execv_str);
    if(result==-1) {
      printf("error");
      exit(-1);
    }
    exit(0);
  }
  else{
    wait(&status);
    i = WEXITSTATUS(status);
    return i;
  }
  
}


/* Looks up the built-in command, if it exists. */
int lookup(char cmd[]) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
      return i;
  return -1;
}

/* Intialization procedures for this shell */
void init_shell() {
  /* Our shell is connected to standard input. */
  shell_terminal = STDIN_FILENO;

  /* Check if we are running interactively */
  shell_is_interactive = isatty(shell_terminal);

  if (shell_is_interactive) {
    /* If the shell is not currently in the foreground, we must pause the shell until it becomes a
     * foreground process. We use SIGTTIN to pause the shell. When the shell gets moved to the
     * foreground, we'll receive a SIGCONT. */
    while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
      kill(-shell_pgid, SIGTTIN);

    /* Saves the shell's process id */
    shell_pgid = getpid();

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Save the current termios to a variable, so it can be restored later. */
    tcgetattr(shell_terminal, &shell_tmodes);
  }
}

int main(unused int argc, unused char *argv[]) {
  init_shell();
  signal(SIGINT, sigintHandler);
  static char line[4096];
  int line_num = 0;

  /* Please only print shell prompts when standard input is not a tty */
  if (shell_is_interactive)
    fprintf(stdout, "%d: ", line_num);

  while (fgets(line, 4096, stdin)) {
    /* Split our line into words. */
    struct tokens *tokens = tokenize(line);

    /* Find which built-in function to run. */
    int fundex = lookup(tokens_get_token(tokens, 0));

    if (fundex >= 0) {
      cmd_table[fundex].fun(tokens);
    } else {
      /* REPLACE this to run commands as programs. */
      if(cmd_run_programs(tokens)==-1) fprintf(stdout, "This shell doesn't know how to run programs.\n");
    }

    if (shell_is_interactive)
      /* Please only print shell prompts when standard input is not a tty */
      fprintf(stdout, "%d: ", ++line_num);

    /* Clean up memory */
    tokens_destroy(tokens);
  }

  return 0;
}