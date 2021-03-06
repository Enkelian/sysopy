#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <wait.h>

#define MAX_COMMAND_NUM 3
#define MAX_COMMAND_LEN 5
#define MAX_LINE_LEN 100


int get_commands_from_line(char* line, char* commands[MAX_COMMAND_NUM][MAX_COMMAND_LEN]){

	int command_idx = 0;

	char* com_buf[MAX_LINE_LEN];
	com_buf[command_idx] = strtok(line, "|\n" );

	while(com_buf[command_idx] != NULL){
		com_buf[++command_idx] = strtok(NULL, "|\n");
	}


	for(int i = 0; i < command_idx; i++){
		int arg_idx = 0;
		commands[i][arg_idx] = strtok(com_buf[i], " ");

		while(commands[i][arg_idx] != NULL){
			commands[i][++arg_idx] = strtok(NULL, " ");
		}
	}

	return command_idx;
}


void execute_line(char* buffer, char* commands[MAX_COMMAND_NUM][MAX_COMMAND_LEN]){
	int command_num = get_commands_from_line(buffer, commands);

	int fd_1[2];
	int fd_2[2];

	pid_t* pids = calloc(command_num, sizeof(pid_t));

	int i = 0;

	if(pipe(fd_1)<0){
		printf("Pipe failed\n");
		exit(EXIT_FAILURE);
	}

	pids[i] = fork();

	if(pids[i] == 0){
		if(dup2(fd_1[1], STDOUT_FILENO) == -1){
			printf("Dup2 failed\n");
			exit(EXIT_FAILURE);
		}
		close(fd_1[0]);
		close(fd_1[1]);
		execvp(commands[i][0], commands[i]);
		exit(EXIT_SUCCESS);
	}
	else if(pids[i]<0){
		printf("Fork failed\n");
		exit(EXIT_FAILURE);
	}


	for(i = 1; i < command_num; i++) {
		fd_2[0] = fd_1[0];
		fd_2[1] = fd_1[1];
		if(pipe(fd_1)<0){
			printf("Pipe failed\n");
			exit(EXIT_FAILURE);
		}

		pids[i] = fork();
		if (pids[i] == 0) {
			if(i != command_num - 1) {
				if(dup2(fd_1[1], STDOUT_FILENO) == -1){
					printf("Dup2 failed\n");
					exit(EXIT_FAILURE);
				}
				close(fd_1[0]);
				close(fd_1[1]);
			}

			if(dup2(fd_2[0], STDIN_FILENO) == -1){
				printf("Dup2 failed\n");
				exit(EXIT_FAILURE);
			}
			close(fd_2[0]);
			close(fd_2[1]);

			execvp(commands[i][0], commands[i]);
			exit(EXIT_SUCCESS);
		}
		else if (pids[i]<0){
			printf("Fork failed\n");
			exit(EXIT_FAILURE);
		}

		close(fd_2[0]);
		close(fd_2[1]);

	}

	for (int i = 0; i < command_num; i++){
		int status;
		waitpid(pids[i], &status, 0);
	}

	free(pids);
}


int main(int argc, char** argv){

	if(argc < 2){
		printf("Invalid arguments. Expected: file_name\n");
		exit(EXIT_FAILURE);
	}


	char* file_name = argv[1];
	FILE* f;
	if((f = fopen(file_name, "r")) == NULL){
		printf("fopen failed\n");
		exit(EXIT_FAILURE);
	}


	char buffer[MAX_LINE_LEN];

	char* commands[MAX_COMMAND_NUM][MAX_COMMAND_LEN];

	while(fgets(buffer, sizeof buffer, f) != NULL){
		execute_line(buffer, commands);
	}

}