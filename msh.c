/*-
 * msh.c
 *
 * Minishell C source
 * Show how to use "obtain_order" input interface function
 *
 * THIS FILE IS TO BE MODIFIED
 */

#include <stddef.h>			/* NULL */
#include <stdio.h>			/* setbuf, printf */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>

extern int obtain_order();		/* See parser.y for description */

struct command{
  // Store the number of commands in argvv
  int num_commands;
  // Store the number of arguments of each command
  int *args;
  // Store the commands
  char ***argvv;
  // Store the I/O redirection
  char *filev[3];
  // Store if the command is executed in background or foreground
  int bg;
};

void free_command(struct command *cmd)
{
   if((*cmd).argvv != NULL){
     char **argv;
     for (; (*cmd).argvv && *(*cmd).argvv; (*cmd).argvv++)
     {
      for (argv = *(*cmd).argvv; argv && *argv; argv++)
      {
        if(*argv){
           free(*argv);
           *argv = NULL;
        }
      }
     }
   }
   free((*cmd).args);
   int f;
   for(f=0;f < 3; f++)
   {
     free((*cmd).filev[f]);
     (*cmd).filev[f] = NULL;
   }
}

void store_command(char ***argvv, char *filev[3], int bg, struct command* cmd)
{
  int num_commands = 0;
  while(argvv[num_commands] != NULL){
    num_commands++;
  }

  int f;
  for(f=0;f < 3; f++)
  {
    if(filev[f] != NULL){
      (*cmd).filev[f] = (char *) calloc(strlen(filev[f]), sizeof(char));
      strcpy((*cmd).filev[f], filev[f]);
    } else {
      (*cmd).filev[f] = NULL;
    }
  }

  (*cmd).bg = bg;
  (*cmd).num_commands = num_commands;
  (*cmd).argvv = (char ***) calloc((num_commands+1), sizeof(char **));
  (*cmd).args = (int*) calloc(num_commands, sizeof(int));
  int i;
  for( i = 0; i < num_commands; i++){
    int args= 0;
    while( argvv[i][args] != NULL ){
      args++;
    }
    (*cmd).args[i] = args;
    (*cmd).argvv[i] = (char **) calloc((args+1), sizeof(char *));
    int j;
    for (j=0; j<args; j++) {
       (*cmd).argvv[i][j] = (char *)calloc(strlen(argvv[i][j]), sizeof(char));
       strcpy((*cmd).argvv[i][j], argvv[i][j] );
    }
  }
}

void seek_the_lost(int *bg_num) {
  /*Checking if theres someone potentially lost...*/
  if (*bg_num > 0) {
    int counter = 0;
    int flag;
    int rescued = 0;
    while (counter < *bg_num) {
      /*This wait allows the program to continue while waiting*/
      printf("lost_pid: %d\n", flag = waitpid(0, NULL, WNOHANG));
      counter++;
      /*If flag > 0 means that someone lost has been found*/
      if (flag > 0) rescued++;
    }
    /*Updating the lost counter for future executions...*/
    *bg_num = *bg_num - rescued;
  }
}

void file_redirection(int num_commands, char *filev[3], int *fd) {
  int flag = 0;

  /*In file redirection*/
  if ((filev[0] != NULL) && !flag) {
    close(0);
    if ((*fd = open(filev[0], O_RDONLY)) == -1) {
      fprintf(stderr, "Open fatal error\n");
      exit(-1);
    }
    if (dup(*fd) == -1) {
      fprintf(stderr, "Dup fatal error\n");
      exit(-1);
    }
    filev[0] = NULL;

    if (num_commands >= 2) flag = 1;
  }
  /*Out file redirection*/
  if ((filev[1] != NULL) && !flag)  {
    close(1);
    if ((*fd = open(filev[1], O_WRONLY | O_CREAT | O_TRUNC, 0777)) == -1) {
      fprintf(stderr, "Open fatal error\n");
      exit(-1);
    }
    if (dup(*fd) == -1) {
      fprintf(stderr, "Dup fatal error\n");
      exit(-1);
    }
    filev[1] = NULL;
  }
  /*Error file redirection*/
  if ((filev[2] != NULL) && !flag) {
    close(2);
    if ((*fd = open(filev[2], O_WRONLY | O_CREAT | O_TRUNC, 0777)) == -1) {
      fprintf(stderr, "Open fatal error\n");
      exit(-1);
    }
    if (dup(*fd) == -1) {
      fprintf(stderr, "Dup fatal error\n");
      exit(-1);
    }
    filev[2] = NULL;
  }
}

void inner_exit(struct command* my_commands, int command_counter) {
  int i;
  /*Releasing memory used in command structs...*/
  for(i = 0; i < command_counter; i++){
    free_command(&my_commands[i]);
  }
  fprintf(stdout, "Goodbye!\n");
  exit(0);
}

int mytime(time_t *t) {
  int flag = 1;
  time_t now = time(NULL);
  if (now == -1) {
    fprintf(stderr, "Time fatal error\n");
    exit(-1);
  }
  now = difftime(now, *t);
  int hours, mins, secs;
  hours = now / 3600;
  mins = (now - (hours * 3600)) / 60;
  secs = now - (hours * 3600) - (mins * 60);
  printf("Uptime: %d h. %d min. %d s.\n",
    hours, mins, secs);

  return flag;
}

int mycd(char ***argvv) {
  int flag = 1;
  char cwd[1024];
  memset(cwd, 0, sizeof(cwd));

  if (argvv[0][1] != NULL) {
    flag = chdir(argvv[0][1]);
    getcwd(cwd, sizeof(cwd));
    fprintf(stdout, "%s\n", cwd);
  } else {
    flag = chdir(getenv("HOME"));
    getcwd(cwd, sizeof(cwd));
    fprintf(stdout, "%s\n", cwd);
  }
  return flag;
}

int myhistory(struct command* my_commands, int command_counter, char ***argvv, char *filev[3], int *bg) {
  int flag = 1;

  if(argvv[0][1] == NULL) {
    /*Get command list...*/
    int i, j, k;
    /*Switching between commands...*/
    for (k = 0; k < command_counter; k++) {
      printf("%d ", k);
      /*Knowing the number of sentence commands...*/
      for (i = 0; i < my_commands[k].num_commands; i++) {
        if (i > 0) printf("| ");
        /*Handling the commands args...*/
        for (j = 0; (my_commands[k].argvv[i][j] != NULL); j++) {
          printf("%s ", my_commands[k].argvv[i][j]);
          if ((my_commands[k].filev[0] != NULL) && (i == my_commands[k].num_commands - 1) && (j == *my_commands[k].args - 1)) {
            printf(" < %s", my_commands[k].filev[0]);
          }
          if ((my_commands[k].filev[1] != NULL) && (i == my_commands[k].num_commands - 1) && (j == *my_commands[k].args - 1)) {
            printf(" > %s", my_commands[k].filev[1]);
          }
          if ((my_commands[k].filev[2] != NULL) && (i == my_commands[k].num_commands - 1) && (j == *my_commands[k].args - 1)) {
            printf(" >& %s", my_commands[k].filev[2]);
          }
        }
      }
      if ((my_commands[k].bg)) {
         printf("&");
      }
      printf("\n");
    }
  } else {
    int pos = atoi(argvv[0][1]);
    if (pos > 20){
      fprintf(stderr, "ERROR: Command not found\n");
      flag = -1;
    }else if (pos < 0){
      fprintf(stderr, "Only positive number\n");
      flag = -1;
    }else {
      printf("Running command %d\n", pos);
      flag = 2;
    }
  }
  return flag;
}

int check_inner_command(char ***argvv, time_t *t, struct command* my_commands, int command_counter, char *filev[3], int *bg, int *num_commands) {
  int flag = 0;
  int error, i, j;
  char *command = argvv[0][0];
  /*Error checking for*/
  for (i = 0; (argvv[i] != NULL); i++) {
    if (((strcmp(argvv[i][0], "exit") == 0) || (strcmp(argvv[i][0], "mytime") == 0)
      || (strcmp(argvv[i][0], "mycd") == 0) || (strcmp(argvv[i][0], "myhistory") == 0)) && (i != 0)) {
      fprintf(stderr, "Only one inner command per sentence or inner commands can´t appear with other commands\n");
      flag = -1;
    }
    for (j = 1; (argvv[i][j] != NULL); j++) {
      if (((strcmp(argvv[i][j], "exit") == 0) || (strcmp(argvv[i][j], "mytime") == 0)
        || (strcmp(argvv[i][j], "mycd") == 0) || (strcmp(argvv[i][j], "myhistory") == 0))) {
        fprintf(stderr, "Inner commands cant be args\n");
        flag = -1;
      }
    }
  }

  if (strcmp(command, "exit") == 0) {
    if ((argvv[0][1] == NULL) && (filev[0] == NULL) && (filev[1] == NULL) && (filev[2] == NULL) && (*bg == 0) && (*num_commands == 1)) {
      inner_exit(my_commands, command_counter);
      flag = 1;
    } else {
      fprintf(stderr, "Inner command 'exit' cant have args or file redirections or can´t be done in background or have 2 or more commands per sentence\n");
      flag = -1;
    }
  } else if (strcmp(command, "mytime") == 0) {
    if ((argvv[0][1] == NULL) && (filev[0] == NULL) && (filev[1] == NULL) && (filev[2] == NULL) && (*bg == 0) && (*num_commands == 1)) {
      mytime(t);
      flag = 1;
    } else {
      fprintf(stderr, "Inner command 'mytime' cant have args or file redirections or can´t be done in background or have 2 or more commands per sentence\n");
      flag = -1;
    }
  } else if (strcmp(command, "mycd") == 0) {
    if((*bg == 0)  && (argvv[0][2] == NULL) && (*num_commands == 1)){
      error = mycd(argvv);
      if (error == -1) {
        fprintf(stderr, "Chdir error, nothing happened...\n");
        flag = -1;
      } else {
        flag = 1;
      }
    } else {
        fprintf(stderr, "Inner command 'mycd' can´t be done in background/Bad number of arguments or have 2 or more commands per sentence\n");
        flag = -1;
    }
  } else if (strcmp(command, "myhistory") == 0) {
    if((*bg == 0)  && (argvv[0][2] == NULL) && (filev[0] == NULL) && (filev[1] == NULL) && (filev[2] == NULL) && (*num_commands == 1)){
      error = myhistory(my_commands, command_counter, argvv, filev, bg);
      flag = error;
    } else {
        fprintf(stderr, "Inner command 'myhistory' can´t be done in background/Bad number of arguments or have 2 or more commands per sentence or file redirections\n");
      flag = -1;
    }
  }
  return flag;
}

void update_history(struct command* cmd, struct command* my_commands, int command_counter) {
  if (command_counter < 20) {
    my_commands[command_counter] = *cmd;
  } else {
    /*Performing left shift...*/
    int i;
    for (i = 1; i < 20; i++) {
      my_commands[i - 1] = my_commands[i];
    }
    my_commands[19] = *cmd;
  }
}

int main(void) {
  time_t start = time(NULL);
	char ***argvv;
	int command_counter = 0;
	int num_commands;
	char *filev[3];
	int bg;
	int ret;
  int i, pid, pid2, pid3, status;
  int p1[2];
  int p2[2];
  int fd;
  int bg_num = 0;
  int is_inner = 0;
  struct command cmd;
  struct command my_commands[20];
  memset(my_commands, 0, sizeof(my_commands));


	setbuf(stdout, NULL);			/* Unbuffered */
	setbuf(stdin, NULL);

	while (1) {
		fprintf(stderr, "%s", "msh> ");	/* Prompt */
		ret = obtain_order(&argvv, filev, &bg);
		if (ret == 0) break;		/* EOF */
		if (ret == -1) continue;	/* Syntax error */
		num_commands = ret - 1;		/* Line */
		if (num_commands == 0) continue;	/* Empty line */
    /*Max 3 commands checking*/
    if (num_commands > 3) {
      fprintf(stderr, "Max 3 commands\n");
      continue;
    }

    /*Looking for the lost...*/
    seek_the_lost(&bg_num);

    store_command(argvv, filev, bg, &cmd);
    update_history(&cmd, my_commands, command_counter);
    if (command_counter < 20) command_counter++;
    /*Checking for inner command...*/
    is_inner = check_inner_command(argvv, &start, my_commands, command_counter, filev, &bg, &num_commands);
    if(is_inner == -1){
      continue;
    } else if(is_inner == 1) {
      is_inner = 0;
      continue;
    } else if(is_inner == 2) {
      int is_inner2;
      is_inner = 0;
      int pos = atoi(argvv[0][1]);
      if (command_counter > 19) pos--;

      argvv = my_commands[pos].argvv;
      num_commands =  my_commands[pos].num_commands;
      filev[0] = my_commands[pos].filev[0];
      filev[1] = my_commands[pos].filev[1];
      filev[2] = my_commands[pos].filev[2];
      bg = my_commands[pos].bg;

      store_command(argvv, filev, bg, &cmd);
      update_history(&cmd, my_commands, command_counter - 1);

      is_inner2 = check_inner_command(argvv, &start, my_commands, command_counter, filev, &bg, &num_commands);
      if(is_inner == - 1){
        continue;
      }else if(is_inner2 == 1){
        is_inner = 0;
        continue;
      }
    }

    /*Setting up command environment...*/
    if (num_commands >= 2) {
      if (pipe(p1) == -1) {
          fprintf(stderr, "Pipe fatal error\n");
          exit(-1);
        }
    }
    if (num_commands == 3) {
      if (pipe(p2) == -1) {
          fprintf(stderr, "Pipe fatal error\n");
          exit(-1);
        }
    }

    /*First child*/
    pid = fork();
    if (pid == - 1) {
      fprintf(stderr, "Fork fatal error\n");
      exit(-1);
    }
    /*Child behavior*/
    if (pid == 0) {
      /*Simple command execution/First chained command*/
      if (num_commands < 2) fprintf(stdout, "Child <%d>\n", getpid());
      /*Checking redirections...*/
      file_redirection(num_commands, filev, &fd);

      if (num_commands >= 2) {
        /*Pipe sdtout redirection*/
        if (close(1) == -1) {
          fprintf(stderr, "Close fatal error\n");
          exit(-1);
        }
        if (dup(p1[1]) == -1) {
          fprintf(stderr, "Dup fatal error\n");
          exit(-1);
        }
        if (close(p1[0])) {
          fprintf(stderr, "Close fatal error\n");
          exit(-1);
        }
        if (close(p1[1]) == -1) {
          fprintf(stderr, "Close fatal error\n");
          exit(-1);
        }
        if (num_commands > 2) {
          if (close(p2[0]) == -1) {
            fprintf(stderr, "Close fatal error\n");
            exit(-1);
          }
          if (close(p2[1]) == -1) {
            fprintf(stderr, "Close fatal error\n");
            exit(-1);
          }
        }
      }
      if (execvp(argvv[0][0], argvv[0]) == -1) {
        fprintf(stderr, "Execvp fatal error\n");
        exit(-1);
      }
    } else {
      if (num_commands >= 2) close(p1[1]);
      /*Child creation loop...*/
      for (i = 0; i < num_commands; i++) {
        /*Creating second child in 2 commands case*/
        if (i == 1) {
          pid2 = fork();
          if (pid2 == 0) {
            /*Normal execution*/
            if (close(0) == -1) {
              fprintf(stderr, "Close fatal error\n");
              exit(-1);
            }
            if (dup(p1[0]) == -1) {
              fprintf(stderr, "Dup fatal error\n");
              exit(-1);
            }
            if (close(p1[0]) == -1) {
              fprintf(stderr, "Close fatal error\n");
              exit(-1);
            }

            if (num_commands == 2) {
              file_redirection(num_commands, filev, &fd);
            }

            if (num_commands == 3) {
              /*Pipe stdout redirection...*/
              if (close(1) == -1) {
                fprintf(stderr, "Close fatal error\n");
                exit(-1);
              }
              if (dup(p2[1]) == -1) {
                fprintf(stderr, "Dup fatal error\n");
                exit(-1);
              }
              if (close(p2[1]) == -1) {
                fprintf(stderr, "Close fatal error\n");
                exit(-1);                
              }
              if (close(p2[0]) == -1) {
                fprintf(stderr, "Close fatal error\n");
                exit(-1);
              }
            }

            if (num_commands < 3) fprintf(stdout, "Child <%d>\n", getpid());
            if (execvp(argvv[i][0], argvv[i]) == -1) {
              fprintf(stderr, "Execvp fatal error\n");
              exit(-1);
            }
          } else {
            close(p1[0]);
            if (num_commands > 2) {
              if (close(p2[1]) == -1) {
                fprintf(stderr, "Close fatal error\n");
                exit(-1);
              }
            }
            if (bg == 0) {
              wait(&status);
            } else {
              bg_num++;
            }
          }
        /*Creating third child in 3 commands case*/
        } else if (i == 2) {
          pid3 = fork();
          if (pid3 == 0) {
            /*Checking redirections...*/
            file_redirection(num_commands, filev, &fd);
            /*Normal execution*/
            if (close(0) == -1) {
              fprintf(stderr, "Close fatal error\n");
              exit(-1);
            }
            if (dup(p2[0]) == -1) {
              fprintf(stderr, "Dup fatal error\n");
              exit(-1);
            }
            if (close(p2[0]) == -1) {
              fprintf(stderr, "Close fatal error\n");
              exit(-1);
            }
            fprintf(stdout, "Child <%d>\n", getpid());
            if (execvp(argvv[i][0], argvv[i]) == -1) {
              fprintf(stderr, "Execvp fatal error\n");
              exit(-1);
            }
          } else {
            if (close(p2[0]) == -1) {
              fprintf(stderr, "Close fatal error\n");
              exit(-1);
            }
            if (bg == 0) {
              wait(&status);
            } else {
              bg_num++;
            }
          }
        }
      }
      if (bg == 0) {
        fprintf(stdout, "Wait child <%d>\n", pid);
        wait(&status);
      } else {
        fprintf(stdout, "Background <%d>\n", pid);
        bg_num++;
      }
    }
	} //fin while
	return 0;
} //end main
