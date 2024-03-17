/* $begin shellmain */
#include "myshell.h"
#include<errno.h>
#define MAXARGS   128

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv); 
FILE *his;

int main() 
{
    char cmdline[MAXLINE]; /* Command line */
	his = fopen("phase1_history.txt", "r");
	if(his == NULL){
		history_idx = 0;
	}
	else{
		fscanf(his, "%d\n", &history_idx);
		history_first = history_idx;
		for(int i = 0; i < history_idx; i++){
			fscanf(his, "%s\n", history[i]);
			strcpy(last_command, history[i]);
		}
	}

	
    while (1) {
	/* Read */
	printf("CSE4100-MP-P1");
	printf("> ");                   
	fgets(cmdline, MAXLINE, stdin); 
	if (feof(stdin))
	    exit(0);

	/* Evaluate */
	eval(cmdline);
    } 
	
	//fclose(his);
}
/* $end shellmain */
  
/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline) 
{
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */
    

	char buf1[MAXLINE];
    strcpy(buf, cmdline);
	strcpy(buf1, cmdline);

    bg = parseline(buf, argv); 
    if (argv[0] == NULL)  
	return;   /* Ignore empty lines */
	
	if(argv[0][0] == '!'){
		if(argv[0][1] == '!'){
			if(history_idx != 0){
				//printf("%s\n", history[history_idx-1]);
			
				strcpy(buf, history[history_idx-1]);
				buf[strlen(buf)] = '\0';
				if(strcmp(buf, last_command)){
					strcpy(history[history_idx], buf);
					strcpy(last_command, history[history_idx]);
					history_idx++;
				}
				printf("%s\n", buf);
				//strcpy(*argv, buf);
				//parseline(buf, argv);
				//printf("%s\n", *argv);
			}
		}
		else{
			if(atoi(&argv[0][1]) > history_first && atoi(&argv[0][1])  <= history_idx){
				strcpy(buf, history[atoi(&argv[0][1])-1]);
				/*if(atoi(&argv[0][1]) <= history_first && (history_first != 0))
				{
					buf[strlen(buf)] = ' ';
					buf[strlen(buf)+1] = '\0';
				}
				else{
					buf[strlen(buf)] = ' ';
					buf[strlen(buf)] = '\0';
				}*/		
				if(strcmp(buf, last_command)){
					strcpy(history[history_idx], buf);
					strcpy(last_command, history[history_idx]);
					history_idx++;
				}	
			}
			else{
				strcpy(buf, history[atoi(&argv[0][1])-1]);
				buf[strlen(buf)] = ' ';
				buf[strlen(buf)+1] = '\0';
				if(strcmp(buf, last_command)){
					strcpy(history[history_idx], buf);
					strcpy(last_command, history[history_idx]);
					history_idx++;
				}
			}
			printf("%s\n", buf);
		}
		parseline(buf, argv);
	
	}
	else{
		char *argv_history;
		argv_history = calloc(MAXARGS, sizeof(char));

		for(int i = 0; argv[i] != NULL; i++){
			strcat(argv_history, argv[i]);
			if(argv[i+1] != NULL)
				strcat(argv_history, " ");
		}

		if(history_idx == 0){
			if(strcmp(argv_history, "history")){
				strcpy(history[history_idx], buf1);
				strcpy(last_command, argv_history);
				history_idx++;
			}
			else{
				printf("No command in history\n");
			}
		}
		else if(history_idx > 0 && strcmp(last_command, argv_history)){
			strcpy(history[history_idx], buf1);
			strcpy(last_command, argv_history);
			history_idx++;
		}
		//printf("%d\n", history_idx);
		his = fopen("phase1_history.txt", "w");
		fprintf(his, "%d\n", history_idx);
		for(int i = 0; i < history_idx; i++){
			if(i <= history_first && (history_first != 0))
				fprintf(his, "%s\n", history[i]);
			else
				fprintf(his, "%s", history[i]);
		}
		fclose(his);
		free(argv_history);
		//printf("%s %s\n", argv[0], argv[1]);
		//printf("%s\n", history[history_idx++]);    //for checking
	}

    if (!builtin_command(argv)) { //quit -> exit(0), & -> ignore, other -> run
		pid = fork();
        
		if(pid == 0){
			char *argv_n;
			argv_n = calloc(MAXARGS, sizeof(char));
			strcpy(argv_n, "/bin/");
			strcat(argv_n, argv[0]);
			//printf("%s\n", argv_n);
			if(execve(argv[0], argv, environ) < 0){
				if (execve(argv_n, argv, environ) < 0) {
					free(argv_n);
					if(argv[0][0] != '!'){
						printf("%s: Command not found.\n", argv[0]);
						exit(0);
					}
				}
			}	
        }

		else if(pid > 0){
		   int status;

			if(waitpid(pid, &status, 0) < 0){
				printf("waitpid error\n");
				exit(0);
			}
		}

		/* Parent waits for foreground job to terminate */
		if (!bg){ 
		   int status;


		}
		else//when there is backgrount process!
	    printf("%d %s", pid, cmdline);
	  }
    return;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv) 
{
	pid_t pid;
    if (!strcmp(argv[0], "quit") || !strcmp(argv[0], "exit")){ /* quit command */
		exit(0);  
	}
    else if (!strcmp(argv[0], "&"))    /* Ignore singleton & */
		return 1;

	else if (!strcmp(argv[0], "cd")){
		pid = fork();

		DIR *dir_ptr;
		char home[MAXARGS];

		if(pid == 0){
			if(argv[1] == NULL){
				strncpy(home, getenv("HOME"), sizeof(home));
				//printf("%s\n", getenv("HOME"));
				chdir(home);
				exit(0);
			}
			else{
				if(chdir(argv[1]) < 0){
					printf("-bash: cd: %s: No such file or directory\n", argv[1]);
					exit(0);
				}
			}
		}
		else if (pid > 0) { 
            int status;

			strncpy(home, getenv("HOME"), sizeof(home));
			chdir(home);
			
            if (waitpid(pid, &status, 0) < 0) {
                printf("waitpid failed\n");
                exit(0);
            }
        } 
		else {  
           printf("fork failed\n");
           exit(0);
        }
		return 1;
	}
	else if (!strcmp(argv[0], "history")){
		pid = fork();

		if(pid == 0){
			
			for(int i = 1; i <= history_idx; i++){
				if(i <= history_first && (history_first != 0))
					printf("%d %3s\n", i, history[i-1]);
				else
					printf("%d %3s", i, history[i-1]);
			}
			//printf("\n");
			exit(0);
		}
		else if(pid > 0){
			int status;

			if(waitpid(pid, &status, 0) < 0) {
				printf("waitpid failed\n");
				exit(0);
			}
		}
		else{
			printf("fork failed\n");
			exit(0);
		}
		return 1;
	}
    return 0;                     /* Not a builtin command */
}
/* $end eval */

/* $begin parseline */
/* parseline - Parse the command line and build the argv array */
int parseline(char *buf, char **argv) 
{
    char *delim;         /* Points to first space delimiter */
    int argc;            /* Number of args */
    int bg;              /* Background job? */

    buf[strlen(buf)-1] = ' ';  /* Replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* Ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;

    /*while ((delim = strchr(buf, ' '))) {
	argv[argc++] = buf;
	*delim = '\0';
	buf = delim + 1;
	while (*buf && (*buf == ' ')) 
            buf++;
    }*/
	
	delim = buf;
	while(*buf){
		if(*buf == '"'){
			delim = ++buf;
			argv[argc++] = delim;
			buf+=1;

			while((*buf != '"'))
				buf++;

			*buf = ' ';
			buf++;

			while(*buf && (*buf == ' '))
				buf++;

			delim = buf;
		}
		else if(*buf == ' '){
			argv[argc++] = delim;
			*buf = '\0';
			buf++;

			while(*buf && (*buf == ' '))
				buf++;

			delim = buf;
		}
		else
			buf++;
	}

    argv[argc] = NULL;
    
	
    if (argc == 0)  /* Ignore blank line */
	return 1;

    /* Should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0)
	argv[--argc] = NULL;

    return bg;
}
/* $end parseline */


