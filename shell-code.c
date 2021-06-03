#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

int BackGround[64];
int Para[64];
bool doExit = false;
int curCmd = -1;
bool stop = false;

/* Splits the string by space and returns the array of tokens
*
*/
char **tokenize(char *line)
{
  char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
  char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
  int i, tokenIndex = 0, tokenNo = 0;

  for(i =0; i < strlen(line); i++){

    char readChar = line[i];

    if (readChar == ' ' || readChar == '\n' || readChar == '\t'){
      token[tokenIndex] = '\0';
      if (tokenIndex != 0){
	tokens[tokenNo] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
	strcpy(tokens[tokenNo++], token);
	tokenIndex = 0; 
      }
    } else {
      token[tokenIndex++] = readChar;
    }
  }
 
  free(token);
  tokens[tokenNo] = NULL ;
  return tokens;
}

void HandleBack(char []); //handles back-ground processes
void HandleCd(char**);//handle cd command
void signalHandler();//handles ctrl+c
void HandleSeries(char**);//handles series commands
int HandlePara(char**);//handles parallel commands
void Perform(int, char *[], int, int);//execute command
void HandleExit();//handles exit command to exit the shell program

int main(int argc, char* argv[]) {
	doExit = false;
	char  line[MAX_INPUT_SIZE];            
	char  **tokens;  
	//char *com = (char *)malloc(MAX_TOKEN_SIZE*sizeof(char));            
	int i;
	bool isBack = false, isSerial = false, isPara = false;
	int pid;
	int totalTokens = 0;
	FILE* fp;
	if(argc == 2) {
		fp = fopen(argv[1],"r");
		if(fp < 0) {
			printf("File doesn't exists.");
			return -1;
		}
	}

	for(i=0;i<64 ;i++){
		BackGround[i] = -1;
		Para[i] = -1;
	}
	signal(SIGINT, signalHandler);
	while(1) {		

		totalTokens = 0;
		/* BEGIN: TAKING INPUT */
		bzero(line, sizeof(line));
		if(argc == 2) { // batch mode
			if(fgets(line, sizeof(line), fp) == NULL) { // file reading finished
				break;	
			}
			line[strlen(line) - 1] = '\0';
		} else { // interactive mode
			printf("$ ");
			scanf("%[^\n]", line);   
			getchar();
		}

		/* END: TAKING INPUT */

		line[strlen(line)] = '\n'; //terminate with new line
		tokens = tokenize(line);
   
		//NO COMMAND CASE
		if(!tokens[0]){
			goto FreeThings;
		}
        //find out whether series or parallel or background
		isBack = false, isSerial = false, isPara = false;
		for(i=0; tokens[i]!=NULL; i++){
			if(!strcmp(tokens[i], "&")){
				isBack = true;
				break;
			}
			if(!strcmp(tokens[i], "&&")){
				isSerial = true;
				break;
			}
			if(!strcmp(tokens[i], "&&&")){
				isPara = true;
				break;
			}
			//totalTokens++;
		}


		// Backgroud process case
		if(isBack){
			HandleBack(line);
			goto FreeThings;
		}

		// Serial Execution case
		if(isSerial){
			HandleSeries(tokens);
			goto FreeThings;
		}


		// Parallel Execution case
		if(isPara){
			int p = HandlePara(tokens);
			while(p--){
				wait(NULL);
			}
			for(i = 0; i<64; i++) Para[i] = -1;
			goto FreeThings;
		}

		// cd CASE
		if(!strcmp(tokens[0], "cd")){
			HandleCd(tokens);
			goto FreeThings;
		}


		// exit case
		if(!strcmp(tokens[0], "exit")){
			HandleExit();
			goto FreeThings;
		}

		//NORMAL COMMANDS CASE
		pid = fork();
		if(pid<0){
			printf("fork failed\n");
		}
		else if(pid==0){
			//strcpy(com, line);
			if(execvp(tokens[0], tokens) < 0){
				printf("ERROR: command not found or incorrect command or incorrect arguments\n");	
				exit(0);
			}
		}
		else{
			curCmd = pid;
			waitpid(pid, NULL, 0);
		}


		// Freeing the allocated memory	
		FreeThings:
		for(int i=0;tokens[i]!=NULL;i++){
			free(tokens[i]);
		}
		free(tokens);
		
		//Reaping finished Back Ground processes
		for(i =0; i<64; i++){
			if(BackGround[i]> 0){
				int b = waitpid(BackGround[i], NULL, WNOHANG);
				if(BackGround[i] == b){
					BackGround[i] = -1;
					printf("Shell: Back ground process finished\n");
				}
			}
		}


		stop = false;
		
		//exit the shell
		if(doExit) break;

	}
	return 0;
}

void HandleBack(char line[]){
	char* Line = strtok(line, "&");
	char** Tokens = tokenize(Line);
	int pid = fork();
	if(pid<0){
		printf("ERROR: Problem while forking the child\n");
	}
	if(pid==0){
		setpgid(0, 0);
		if(execvp(Tokens[0], Tokens) < 0){
			printf("ERROR: wrong commnad\n");
			exit(0);
		}
	}
	else{
		for(int i=0; i<64; i++){
			if(BackGround[i] == -1){
				BackGround[i] = pid;
				break;
			}
		}
	}
	for(int i=0;Tokens[i]!=NULL;i++){
		free(Tokens[i]);
	}
	free(Tokens);
	return;
}

void Perform(int tokensLength, char *commands[64], int k, int ins){
	char *command[tokensLength + 1];
	for(int j=0;j<tokensLength; j++){
		command[j] = commands[j];
	}
	command[tokensLength] = NULL;
	int pid = fork();
	if(pid<0){
		printf("ERROR: Problem while forking the child\n");
	}
	else if(pid == 0){
		if(execvp(command[0], command)< 0){
			printf("ERROR: wrong command\n");
			exit(0);
		}
	}
	else if(pid>0){
		if(k==1) {
			curCmd = pid;
			waitpid(pid, NULL, 0);
		}
		else{
			if(ins) Para[ins -1] = pid;
		}
	}
	return;
}

void HandleSeries(char** tokens){
	int tokensLength = 0;
	char *commands[64];
	if(stop == true) return;
	for(int i=0; tokens[i]!= NULL; i++){
		//printf("%s.....2\n", tokens[i]);
		if(!strcmp(tokens[i], "&&")){
			if(!strcmp(commands[0], "cd")){
				HandleCd(commands);
				continue;
			}
			if(!strcmp(commands[0], "exit")){
				HandleExit();
				return;
			}
			Perform(tokensLength, commands, 1, 0);
			if(stop == true) return;
			tokensLength = 0;
		}
		else{
			commands[tokensLength] = tokens[i];
			tokensLength++;
		}
	}
	if(!strcmp(commands[0], "cd")) HandleCd(commands);
	else if(!strcmp(commands[0], "exit")) HandleExit();
	else Perform(tokensLength, commands, 1, 0);
	return ;
}

int HandlePara(char** tokens){
	int tokensLength = 0, totalIns = 0;
	char *commands[64];
	for(int i=0; tokens[i]!= NULL; i++){
		//printf("%s.....2\n", tokens[i]);
		if(!strcmp(tokens[i], "&&&")){
			if(!strcmp(commands[0], "cd")){
				HandleCd(commands);
			}
			if(!strcmp(commands[0], "exit")){
				HandleExit();
				return 0;
			}
			else{
				totalIns++;
				Perform(tokensLength, commands, 0, totalIns);
			}
			tokensLength = 0;
		}
		else{
			commands[tokensLength] = tokens[i];
			tokensLength++;
		}
	}
	if(!strcmp(commands[0], "cd")) HandleCd(commands);
	else if(!strcmp(commands[0], "exit")){
		HandleExit();
		return 0;
	}
	else{
		totalIns++;
		Perform(tokensLength, commands, 0, totalIns);
	}
	return totalIns;
}

void HandleCd(char** tokens){
	if(tokens[2]!=NULL){
		printf("Error: expected only one argument\n");
	}
	else{
		if(chdir(tokens[1]) < 0){
			printf("Shell: Incorrect Command\n");
		};
	}
	return;
}

void signalHandler(){
	for(int i=0; i<64; i++){
		if(Para[i] > -1){
			kill(Para[i], SIGKILL);
			wait(0);
		}
		Para[i] = -1;
	}
	if(curCmd > -1){
		kill(curCmd, SIGKILL);
		wait(0);
	}
	stop = true;
	printf("\n");
	return;
}

void HandleExit(){
	for(int i=0; i<64; i++){
		if(BackGround[i] > -1){
			kill(BackGround[i], SIGKILL);
			wait(0);
		}
		if(Para[i] > -1){
			kill(Para[i], SIGKILL);
			wait(0);
		}
		BackGround[i] = -1;
	}
	doExit = true;
	return;
}
