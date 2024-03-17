/* $begin shellmain */
#include "myshell.h"
#include<errno.h>
#define MAXARGS   128
FILE *his;

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv); 
void signalintHandler(int signal);
void signaltstpHandler(int signal);
void signalchldHandler(int signal);
pid_t fg_pid();
void fg_wait();
void add_job(pid_t pid, char *command, job_state state);
void remove_job_by_pid(pid_t pid);
void remove_job(int job_number);
JOBS_T *find_job_by_pid(pid_t pid);
void print_jobs();
void bg_job(int job_number);
void fg_job(int job_number);
void kill_job(int job_number);

int main() 
{
    char cmdline[MAXLINE]; /* Command line */
	his = fopen("phase2_history.txt", "r");
	if(his == NULL){
		history_idx = 0;
	}
	else{
		fscanf(his, "%d", &history_idx);
		history_first = history_idx;
		for(int i = 0; i < history_idx; i++){
			fscanf(his, "%s\n", history[i]);
			strcpy(last_command, history[i]);
		}
	}

	signal(SIGINT, signalintHandler);
	signal(SIGTSTP, signaltstpHandler);
	signal(SIGCHLD, signalchldHandler);
	
	for(int i = 0; i < MAXJOBS; i++){
		JOBS[i].pid = 0;
		JOBS[i].job_id = 0;
		strcpy(JOBS[i].command, "");
		JOBS[i].state = UNDEFINED;
	}

    while (1) {
	/* Read */
	printf("> ");                   
	fgets(cmdline, MAXLINE, stdin); 
	if (feof(stdin))
	    exit(0);
	
	//printf("%s\n", cmdline);
	/* Evaluate */
	if(!strcmp(cmdline, "\n"))
		continue;
	else
		eval(cmdline);
    } 
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
	sigset_t mask;
    
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
		his = fopen("phase2_history.txt", "w");
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
	if(pipe_flag == 1){

		int status;
		for(int i = 0; pipes[i] != NULL; i++){
			//printf("%s\n", cmds_for_pipe[i].argv[0]);
			if(!builtin_command(cmds_for_pipe[i].argv)){
				if(sigemptyset(&mask) < 0){
					unix_error("sigemptyset error");
					exit(0);
				}
				if(sigaddset(&mask, SIGCHLD) < 0){
					unix_error("sigaddset error");
					exit(0);
				}
				if(sigprocmask(SIG_BLOCK, &mask, NULL) < 0){
					unix_error("sigprocmask error");
					exit(0);
				}
				
				if(pipe(cmds_for_pipe[i].fd) < 0){
					unix_error("pipe error");
					exit(0);
				}

				pid = fork();

				if(pid == 0){
					if(setpgid(0,0) < 0){
						unix_error("setpgid error");
						exit(0);
					}
					
					if(sigprocmask(SIG_UNBLOCK, &mask, NULL) < 0){
						unix_error("sigprocmask error");
						exit(0);
					}
					

					if(pipes[i+1] != NULL){
						if(dup2(cmds_for_pipe[i].fd[1], STDOUT_FILENO) < 0){
							unix_error("dup2 error");
							exit(0);
						}
					}

					if(i > 0){
						if(dup2(cmds_for_pipe[i-1].fd[0], STDIN_FILENO) < 0){
							unix_error("dup2 error");
							exit(0);
						}
					}

					if (execve(cmds_for_pipe[i].argv[0], cmds_for_pipe[i].argv, environ) < 0) {

						char argv_n[MAXLINE] = "/bin/";
						strcat(argv_n, cmds_for_pipe[i].argv[0]);
						
						  if (execve(argv_n, cmds_for_pipe[i].argv, environ) < 0) {
							  printf("%s : Command not found.\n", cmds_for_pipe[i].argv[0]);
							  exit(0);
						  }
					}


				}
				else if(pid < 0){
					unix_error("fork error");
					exit(0);
				}
				else{
					
					if(bg == 1){
						add_job(pid, pipes[i], RUNNING);
						
						if(sigprocmask(SIG_UNBLOCK, &mask, NULL) < 0){
							unix_error("sigprocmask error");
							exit(0);
						}

						JOBS_T *tmp_job = find_job_by_pid(pid);
						printf("[%d] %d\n", tmp_job->job_id, tmp_job->pid);

					}
					if(bg == 0){
						add_job(pid, pipes[i], FOREGROUND);

						if(sigprocmask(SIG_UNBLOCK, &mask, NULL) < 0){
							unix_error("sigprocmask error");
							exit(0);
						}

						fg_wait(pid);

					}
					
					close(cmds_for_pipe[i].fd[1]);
					if(i > 0){
						close(cmds_for_pipe[i-1].fd[0]);
					}
					if(i < number_of_pipes - 1){
						close(cmds_for_pipe[i].fd[1]);
					}
					
					//printf("%d\n", number_of_jobs);
					/*for(int i = number_of_jobs-1; i >= 0; i--){
						printf("[%d] %d\n", JOBS[i].job_id, JOBS[i].pid);
					}*/
				}
			}
		}
	}
	else{
		 if (!builtin_command(argv)) { //quit -> exit(0), & -> ignore, other -> run
	
			if(sigemptyset(&mask) < 0){
				unix_error("sigemptyset error");
				exit(0);
			}
			if(sigaddset(&mask, SIGCHLD) < 0){
				unix_error("sigaddset error");
				exit(0);
			}
			if(sigprocmask(SIG_BLOCK, &mask, NULL) < 0){
				unix_error("sigprocmask error");
				exit(0);
			}
			
			int status;
			pid = fork();
        
			if(pid == 0){
				if(setpgid(0,0) < 0){
					unix_error("setpgid error");
					exit(0);
				}

				if(sigprocmask(SIG_UNBLOCK, &mask, NULL) < 0){
					unix_error("sigprocmask error");
					exit(0);
				}


				char *argv_n;
				argv_n = calloc(MAXARGS, sizeof(char));
				strcpy(argv_n, "/bin/");
				strcat(argv_n, argv[0]);

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
			 else if(pid < 0){
				 unix_error("fork error");
				 exit(0);
			 }
			 else if (pid > 0)
			 {
				if(bg == 1){
					add_job(pid, argv[0], RUNNING);
					if(sigprocmask(SIG_UNBLOCK, &mask, NULL) < 0){
						unix_error("sigprocmask error");
						exit(0);
					}

					JOBS_T *tmp_job = find_job_by_pid(pid);
					printf("[%d] %d\n", tmp_job->job_id, tmp_job->pid);

				}
				if(bg == 0){
					add_job(pid, argv[0], FOREGROUND);
					if(sigprocmask(SIG_UNBLOCK, &mask, NULL) < 0){
						unix_error("sigprocmask error");
						exit(0);
					}	
					fg_wait(pid);
				}
			
				//waitpid(pid, &status, 0);
			 }
		 }
	}

    return;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv) 
{
	pid_t pid;
    if (!strcmp(argv[0], "quit") || !strcmp(argv[0], "exit")) /* quit command */{
		exit(0);  
	}
    else if (!strcmp(argv[0], "&"))    /* Ignore singleton & */
		return 1;
	else if (!strcmp(argv[0], "jobs")){
		print_jobs();
		//printf("%d\n", number_of_jobs);
		return 1;
	}
	else if	(!strcmp(argv[0], "kill")){	
		if(argv[1] == NULL || argv[1][0] != '%'){
			printf("Wrong input. Input must be given in format bg %%[job_id]");
			return 0;
		}
		int j_id = atoi(&argv[1][1]);
		//printf("%d\n", j_id);

		for(int i = 0; i < number_of_jobs; i++){
			if(j_id == JOBS[i].job_id){
				kill_job(JOBS[i].job_id);
				return 1;
			}
		}
	
		printf("No such job\n");
	}
	else if(!strcmp(argv[0], "bg")){
		if(argv[1] == NULL || argv[1][0] != '%'){
			printf("Wrong input. Input must be given in format bg %%[job_id]");
			return 0;
		}
		int id_tmp = atoi(&argv[1][1]);
		int flag = 0;
		for(int i = 0; i < number_of_jobs; i++){
			if(id_tmp == JOBS[i].job_id){
				flag = 1;
				JOBS[i].state = RUNNING;
				printf("[%d] %s %s\n", JOBS[i].job_id, "RUNNING", JOBS[i].command);
				if(kill(-JOBS[i].pid, SIGCONT) < 0){
					unix_error("kill error");
					exit(0);
				}
			}
		}
		if(flag == 0)
			printf("No such job\n");
	}
	else if(!strcmp(argv[0], "fg")){
		if(argv[1] == NULL || argv[1][0] != '%'){
			printf("Wrong input. Input must be given in format bg %%[job_id]");
			return 0;
		}
		int id_tmp = atoi(&argv[1][1]);
		int flag = 0;
		for(int i = 0; i < number_of_jobs; i++){
			if(id_tmp == JOBS[i].job_id){
				JOBS[i].state = FOREGROUND;
				printf("[%d] %s %s\n", JOBS[i].job_id, "FOREGROUND", JOBS[i].command);
				flag = 1;
				if(kill(-JOBS[i].pid, SIGCONT) < 0){
					unix_error("kill error");
					exit(0);
				}
				fg_wait(JOBS[i].pid);
			}
		
		}
		if(flag == 0)
			printf("No such job\n");
	}
	else if(argv[0][0] == '\n')
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
	else
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
	pipe_flag = 0;
	int parsing = 0;

    buf[strlen(buf)-1] = ' ';  /* Replace trailing '\n' with space */
	while (*buf && (*buf == ' ')) /* Ignore leading spaces */
		buf++;
	

	char buf_pipe[MAXLINE];
	strcpy(buf_pipe, buf);
	//printf("%s\n", buf_pipe);

	number_of_pipes = 0;
	char *pipeptr = buf_pipe;
	char *quote_pos, *pipe_pos, *pipe_delim = buf_pipe;
   
	while(*pipeptr){
		if(*pipeptr == '"'){
			if(parsing == 0){
				parsing = 1;
				pipeptr++;
			}
			else{
				parsing = 0;
				pipeptr++;
			}
		}
		else if(*pipeptr == '|'){
			pipe_flag = 1;
			pipes[number_of_pipes++] = pipe_delim;
			*pipeptr = '\0';
			pipeptr++;
			while(*pipeptr && (*pipeptr == ' '))
				pipeptr++;
			pipe_delim = pipeptr;
		}
		else
			pipeptr++;
	}
	if(pipe_flag == 1){
		*pipeptr = '\0';

	
		pipes[number_of_pipes++] = pipe_delim;
		pipes[number_of_pipes] = NULL;
		
		int andpercentidx = 0;

		for(int i = 0; i < number_of_pipes; i++){
			cmds_for_pipe[i].argc = 0;
			buf = pipes[i];
			delim = buf;

			while(*buf && (*buf == ' ' ))
				buf++;

			parsing = 0;

			while(*buf){
				if(*buf == '"'){
					if(parsing == 0){
						delim = ++buf;
						cmds_for_pipe[i].argv[cmds_for_pipe[i].argc++] = delim;
						buf+=1;
						
						while(*buf != '"')
							buf++;

						*buf = '\0';
						buf++;
						while (*buf && (*buf == ' ')) 
							 buf++;
						delim = buf;
					}
				}
				else if(*buf == ' '){
					cmds_for_pipe[i].argv[cmds_for_pipe[i].argc++] = delim;
					*buf = '\0';
					buf++;
					while (*buf && (*buf == ' ')) 
					  buf++;
					 delim = buf;
				}
				else
					buf++;

			}
			if(*cmds_for_pipe[i].argv[cmds_for_pipe[i].argc-1] == '&'){
				cmds_for_pipe[i].background = 1;
				andpercentidx = i;
			}
			else
				cmds_for_pipe[i].background = 0;

			cmds_for_pipe[i].argv[cmds_for_pipe[i].argc] = NULL;	
			
		}
		  ((bg = ((*(cmds_for_pipe[andpercentidx].argv[cmds_for_pipe[andpercentidx].argc-1])) == '&')) != 0);
		 
		  if(bg == 1){
			  cmds_for_pipe[andpercentidx].argv[cmds_for_pipe[andpercentidx].argc-1] = NULL;
			  cmds_for_pipe[andpercentidx].argc -= 1;
		  }
		//printf("%d\n", bg);
		  return bg;
	}
    /* Build the argv list */
	else{
	    argc = 0;

	    delim = buf;
		while(*buf){
			if(*buf == '"'){
				delim = ++buf;
				argv[argc++] = delim;
				buf+=1;
	
				while(*buf != '"')
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
}
/* $end parseline */

/*void unix_error(char *msg) 
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}*/

void signalintHandler(int signal){
	pid_t pid;

	pid = fg_pid();

	if(pid != 0){
		if(kill(-pid, signal) < 0){
			unix_error("kill error");
		}
	}
}

void signaltstpHandler(int signal){
	pid_t pid = 0;
	printf("[CTRL+Z]\n");
	pid = fg_pid();
	
	if(pid != 0){
		if(kill(-pid, signal) < 0){
			unix_error("kill error");
		}
	}
	//printf("%d\n", number_of_jobs);
}

void signalchldHandler(int signal){
	pid_t pid;
	int status;

	while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
		if(WIFSTOPPED(status)){
			JOBS_T *JOB_TMP = find_job_by_pid(pid);
			JOB_TMP->state = SUSPENDED;
		}
		else if(WIFSIGNALED(status)){
			remove_job_by_pid(pid);
		}
		else if(WIFEXITED(status)){
			remove_job_by_pid(pid);
		}
	}
}

void print_jobs(){
	for(int i = 0; i < number_of_jobs; i++){
		if (JOBS[i].state == RUNNING)
			printf("[%d] %d %s %s\n", JOBS[i].job_id, JOBS[i].pid, "RUNNING", JOBS[i].command);
		else if (JOBS[i].state == SUSPENDED)
			printf("[%d] %d %s %s\n", JOBS[i].job_id, JOBS[i].pid, "SUSPENDED", JOBS[i].command);
	}
}

void add_job(pid_t pid, char *command, job_state state){
	if(number_of_jobs < MAXJOBS){
		JOBS[number_of_jobs].pid = pid;
		strcpy(JOBS[number_of_jobs].command, command);
		JOBS[number_of_jobs].job_id = number_of_jobs + 1;
		JOBS[number_of_jobs].state = state;
	}
	number_of_jobs++;
}

void remove_job_by_pid(pid_t pid){
	for(int i = 0; i < MAXJOBS; i++){
		if(pid == JOBS[i].pid){
			JOBS[i].pid = 0;
			JOBS[i].job_id = 0;
			JOBS[i].state = UNDEFINED;
			strcpy(JOBS[i].command, "");
		}
	}
}

void remove_job(int job_id){
	for(int i = job_id-1; i < number_of_jobs; i++){
		JOBS[i].pid = JOBS[i+1].pid;
		strcpy(JOBS[i].command, JOBS[i+1].command);
		JOBS[i].state = JOBS[i+1].state;
		JOBS[i].job_id = JOBS[i+1].job_id;
	}
	number_of_jobs--;
}

void kill_job(int job_id){
	kill(JOBS[job_id-1].pid, SIGKILL);
	remove_job(job_id);
}

JOBS_T *find_job_by_pid(pid_t pid){
	for(int i = 0; i < number_of_jobs; i++){
		if(pid == JOBS[i].pid){
			return &JOBS[i];
		}
	}
	return NULL;
}

pid_t fg_pid(){
	for(int i = 0; i < number_of_jobs; i++){
		if(JOBS[i].state == FOREGROUND && JOBS[i].state != UNDEFINED)
			return JOBS[i].pid;
	}
	return 0;
}

void fg_wait(pid_t pid){
	while(1){
		int pid_tmp = fg_pid();
		sleep(1);
		if(pid_tmp != pid)
			break;
	}
}