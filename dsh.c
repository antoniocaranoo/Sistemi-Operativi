/*
* Antonio Carano 902447
* Camilla Cantaluppi 894557
*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_COMMAND_LENGTH 1024
#define MAX_HISTORY_SIZE 10
#define MAX_LINE 4096
#define MAX_ARGS 256
#define MAX_PATH 512
#define MAX_PROMPT 512
#define MAX_CHILD_PROCESSES 10

char history[MAX_HISTORY_SIZE][MAX_COMMAND_LENGTH];
char _path[MAX_PATH] = "/bin/:/usr/bin/";
int history_count = 0;
pid_t child_processes[MAX_CHILD_PROCESSES];
int num_child_processes = 0;

void panic (const char* msg) {
    if(errno){
        fprintf(stderr, "PANIC: %s: %s\n\n", msg, strerror(errno));
    } else {
        fprintf(stderr, "PANIC: %s\n\n", msg);
    }
    exit(EXIT_FAILURE);
}

// Aggiungi un PID alla lista dei processi figli
void add_child_process(pid_t pid) {
    if (num_child_processes < MAX_CHILD_PROCESSES) {
        child_processes[num_child_processes++] = pid;
    } else {
        fprintf(stderr, "Errore: troppi processi figli\n");
    }
}

// Rimuovi un PID dalla lista dei processi figli
void remove_child_process(pid_t pid) {
    int i;
    for (i = 0; i < num_child_processes; i++) {
        if (child_processes[i] == pid) {
            // Shift degli elementi nella lista
            for (int j = i; j < num_child_processes - 1; j++) {
                child_processes[j] = child_processes[j + 1];
            }
            num_child_processes--;
            return;
        }
    }
}

// Gestisci i processi zombie
void handle_zombies() {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        remove_child_process(pid);
    }
}

void add_to_history(const char* command) {
    if (history_count < MAX_HISTORY_SIZE) {
        strcpy(history[history_count], command);
        history_count++;

    } else {
        int i;
        for (i = 0; i < MAX_HISTORY_SIZE - 1; i++) {
            strcpy(history[i], history[i + 1]);
        }
        strcpy(history[MAX_HISTORY_SIZE - 1], command);
        remove_history(0);
    }
}

void print_history() {
    int i;
    for (i = 0; i < history_count; i++) {
        printf("%d: %s\n", i + 1, history[i]);
    }
}


int prompt(char* buf, size_t buf_size, const char* prompt_string) {
    char* input = readline(prompt_string);
    if (!input) {
        // EOF or error
        return EOF;
    }
    
    if (strlen(input) > 0) {
        strncpy(buf, input, buf_size); // Copia l'input nel buffer
        buf[buf_size - 1] = '\0'; // Assicura che il buffer sia terminato correttamente
        free(input);
        return strlen(buf); // Restituisce la lunghezza dell'input
    } else {
        free(input);
        return -1; // Input vuoto, richiede un altro input
    }
}
void set_path(const char* new_path) {
	if(new_path != NULL) {
		//sostituiamo il PATH
#if USE_DEBUG_PRINTF
	printf("Debug: new_path %s\n", new_path);
#endif
	int cur_pos = 0;
	while(new_path[cur_pos] != '\0') {
		cur_pos++;
		if(cur_pos>= MAX_PATH -1 && new_path[cur_pos] != '\0') {
			fprintf(stderr, "Error: PATH string too long\n");
			return;
		}
	}
	if (cur_pos > 0)
		memcpy(_path, new_path, cur_pos +1);
	}
	printf("%s\n", _path);
}

void path_lookup(char* abs_path, const char* rel_path) {
	char* prefix;
	char buf[MAX_PATH];
	if(abs_path == NULL || rel_path == NULL)
		panic ("path_lookup: parameter error");
	prefix = strtok(_path, ":");
	while( prefix != NULL) {
		strcpy(buf, prefix);
		strcat(buf, rel_path);
		if(access(buf, X_OK) == 0) {
			strcpy(abs_path, buf);
			return;
		}
		prefix = strtok(NULL, ":");
	}
	strcpy(abs_path, rel_path);
}
	

void exec_rel2abs(char** arg_list) {
	if(arg_list[0][0] == '/') {
		//assume absolute path
		execv(arg_list[0], arg_list);
	} else {
		//assume relative path
		char abs_path[MAX_PATH];
		path_lookup(abs_path, arg_list[0]);
		execv(abs_path, arg_list);
	}
}

void do_redir(const char* out_path, char** arg_list, const char* mode){
    int i = 0;
    char* last_element;

	while(arg_list[i] != NULL){
		last_element = arg_list[i];
		i++;
	}
	if(strcmp(last_element,"&") == 0)
		arg_list[i-1] = NULL;

	if(out_path == NULL)
		panic("do_redir: no path");
	int pid = fork();
	if(pid > 0) {
        int wpid = 0;
		if(!(strcmp(last_element,"&") == 0))
			wpid = waitpid(pid, NULL, 0);
        else
            add_child_process(pid);
		if(wpid < 0) panic("do_redir: wait");
	} else if (pid == 0) {
		FILE* out = fopen(out_path, mode);
		if(out == NULL) {
			perror(out_path);
			exit(EXIT_FAILURE);
		}
	dup2(fileno(out), 1);
	exec_rel2abs(arg_list);
	perror(arg_list[0]);
	exit(EXIT_FAILURE);
	} else {
		panic("do_redir: fork");
	}
}

void do_pipe(size_t pipe_pos, char** arg_list) {
     int i = 0;
    char* last_element;

	while(arg_list[i] != NULL){
		last_element = arg_list[i];
		i++;
	}
	if(strcmp(last_element,"&") == 0)
		arg_list[i-1] = NULL;

    int pipefd[2];
    int pid;
    if(pipe(pipefd) < 0) panic("do_pipe: pipe");
    //left side of the pipe
    pid = fork();
    if(pid > 0){
        int wpid = 0;
		if(!(strcmp(last_element,"&") == 0))
			wpid = waitpid(pid, NULL, 0);
        else
            add_child_process(pid);
        if(wpid < 0) 
            panic("do_pipe: wait");
    } else if (pid == 0){
    	close(pipefd[0]);
    	dup2(pipefd[1], 1);
    	close(pipefd[1]);
        exec_rel2abs(arg_list);
        perror(arg_list[0]);
        exit(EXIT_FAILURE);
    } else {
        panic("do_pipe: fork");
    }
    
    //right side of the pipe
    pid = fork();
    if(pid > 0){
    	close(pipefd[0]);
    	close(pipefd[1]);
        int wpid = 0;
		if(!(strcmp(last_element,"&") == 0))
			wpid = waitpid(pid, NULL, 0);
        else
            add_child_process(pid);
        if(wpid < 0) 
            panic("do_pipe: wait");
    } else if (pid == 0){
    	close(pipefd[1]);
    	dup2(pipefd[0], 0);
    	close(pipefd[0]);
        exec_rel2abs(arg_list + pipe_pos +1);
        perror(arg_list[pipe_pos +1]);
        exit(EXIT_FAILURE);
    } else {
        panic("do_pipe: fork");
    }
}

void do_exec(char** arg_list){
	int pid = fork();
	int i = 0;
	char* last_element;

	while(arg_list[i] != NULL){
		last_element = arg_list[i];
		i++;
	}

	if(strcmp(last_element,"&") == 0)
		arg_list[i-1] = NULL; 

	if(pid > 0){
		if(!(strcmp(last_element,"&") == 0))
			waitpid(pid, NULL, 0);
        else
            add_child_process(pid);
			
	}else if(pid == 0){
		if(arg_list[0][0] == '/'){
			execv(arg_list[0], arg_list);
		}else{
			char abs_path[MAX_PATH];
			path_lookup(abs_path, arg_list[0]);
			execv(abs_path, arg_list);
		}

		perror(arg_list[0]);
		exit(EXIT_FAILURE);

	}else{
		panic("fork");
	}
}

int main (void) {

    char input_buffer[MAX_LINE];
    size_t arg_count;
    char* arg_list[MAX_ARGS];
    char prompt_string[MAX_PROMPT] = "\0";
    using_history();

    if(isatty(0)){
    	//we're in an interactive session
    	strcpy(prompt_string, "dsh$ \0");
    }
    while(prompt (input_buffer, MAX_LINE, prompt_string) >= 0) {

        handle_zombies();

        //do shell stuff
        //tokenize input
        char save_command[MAX_HISTORY_SIZE][MAX_COMMAND_LENGTH];
        strcpy(save_command[0], input_buffer);
        arg_count=0;
        arg_list[arg_count] = strtok(input_buffer, " ");
        if(arg_list[arg_count] == NULL){
            //nothing was specivied at the command prompt
            continue;
        } else {
            do{
                arg_count++;
                if(arg_count > MAX_ARGS) break;
                arg_list[arg_count] = strtok(NULL, " ");
            } while(arg_list[arg_count] != NULL);
        }
#if USE_DEBUG_PRINTF
       	printf("DEBUG: tokens:");
        //on specific condition
        for(size_t i = 0; i< arg_count; i++){
       		printf(" %s", arg_list[i]);
        }
        puts("");
#endif       
        
        //builtins
        if(strcmp(arg_list[0], "exit") == 0){
            break;
        }
        
        if(strcmp(arg_list[0], "setpath") == 0){
            set_path(arg_list[1]);
            add_to_history(save_command[0]);
		    add_history(save_command[0]);
            continue;
        }

        if (strcmp(arg_list[0], "history") == 0) {
            print_history();
            add_to_history(save_command[0]);
		    add_history(save_command[0]);
            continue;
        }


        {
        //check for special characters
        size_t redir_pos = 0;
        size_t append_pos = 0;
        size_t pipe_pos =0; 
        for(size_t i = 0; i < arg_count; i++){
        	if(strcmp(arg_list[i], ">") == 0){
        		redir_pos = i;
        		break;
        	}
        	if(strcmp(arg_list[i], ">>") == 0){
        		append_pos = i;
        		break;
        	}
        	if(strcmp(arg_list[i], "|") == 0){
        		pipe_pos = i;
        		break;
        	}
        }
        // do shell ops	
        if(redir_pos != 0) {
        	arg_list[redir_pos] = NULL;
        	do_redir(arg_list[redir_pos +1], arg_list, "w+");
        } else if(append_pos!= 0){
        	arg_list[append_pos] = NULL;
        	do_redir(arg_list[append_pos +1], arg_list, "a+");
        } else if(pipe_pos != 0){
        	arg_list[pipe_pos] = NULL;
        	do_pipe(pipe_pos, arg_list);
        } else {
        	//exec	
        	do_exec(arg_list);
        	}

	}	
        add_to_history(save_command[0]);
        add_history(save_command[0]);		
    }
    //puts("");
    exit(EXIT_SUCCESS);
}