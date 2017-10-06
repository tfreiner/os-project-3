/**
 * Author: Taylor Freiner
 * Date: October 5th, 2017
 * Log: Setting up shared memory, getopt, processes, and file management
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <sys/shm.h>

int sharedmem[2];

int main(int argc, char* argv[])  {

	int i, option;
	int execTime = 0;
	int processCount = 0;
	int processNum = 0;
	int processIds[100];
	char argval;
	char* filename = (char *)malloc(100);
	pid_t childpid = 0;
	time_t startTime, endTime;
	double elapsedTime;

	

	//OPTIONS
	if (argc != 7 && argc != 3 && argc != 2){
		fprintf(stderr, "%s Error: Incorrect number of arguments\n", argv[0]);
		return 1;
	}

	while ((option = getopt(argc, argv, "hs:l:t:")) != -1){
		switch (option){
			case 'h':
				printf("Usage: %s [-s positive integer] <-l filename> [-t positive integer]\n", argv[0]);
				printf("\t-s: maximum number of slave processes\n");
				printf("\t-l: name of log file\n");
				printf("\t-t: execution time\n");
				return 0;
				break;
			case 's':
				argval = *optarg;
				if(isdigit(argval) && (atoi(optarg) > 0))
					processNum = atoi(optarg);
				else{
					fprintf(stderr, "%s Error: Argument must be a positive integer\n", argv[0]);
					return 1;
				}
				break;
			case 'l':
				filename = optarg;
				break;
			case 't':
				argval = *optarg;
				if(isdigit(argval) && (atoi(optarg) > 0))
					execTime = atoi(optarg);
				else{
					fprintf(stderr, "%s Error: Argument must be a positive integer\n", argv[0]);
					return 1;
				}
				break;
			case '?':
				printf("Usage: %s [-s positive integer] <-l filename> [-t positive integer]\n", argv[0]);
				return 1;
				break;
		}
	}
	if(execTime == 0)
		execTime = 20;
	if(processNum == 0)
		processNum = 5;
	
	time(&startTime);

	//FILE MANAGEMENT
	FILE *file = fopen(filename, "w");
	
	if(file == NULL){
		printf("%s: ", argv[0]);
		perror("Error: \n");
		return 1;
	}

	//SHARED MEMORY
	key_t key = ftok("keygen", 1);
	key_t key2 = ftok("keygen2", 1);
	int memid = shmget(key, sizeof(int*)*2, IPC_CREAT | 0644);
	int memid2 = shmget(key2, sizeof(int*)*2, IPC_CREAT | 0644);
	if(memid == -1 || memid2 == -1){
		printf("%s: ", argv[0]);
		perror("Error\n");
	}
	sharedmem[0] = memid;
	sharedmem[1] = memid2;
	
	int *clock = (int *)shmat(memid, NULL, 0);
	int *shmMsg = (int *)shmat(memid2, NULL, 0);
	if(*clock == -1 || *shmMsg == -1){
		printf("%s: ", argv[0]);
		perror("Error\n");
	}
	int clockVal = 0;
	for(i = 0; i < 2; i++){
		memcpy(&clock[i], &clockVal, 4);
		memcpy(&shmMsg[i], &clockVal, 4);
	}

	//CREATING PROCESSES
	for(i = 0; i < processNum; i++){
		childpid = fork();
		execl("user", "user", NULL);
		processIds[i] = childpid;
		processCount++;
	}

	while(clock[0] < 2 && processCount  < 100 && elapsedTime < execTime){

		if(shmMsg[0] != 0 && shmMsg[1] != 0){
			fprintf(file, "Master: Child pid is terminating at my time %d.%d because it reached %d.%d in slave\n", clock[0], clock[1], shmMsg[0], shmMsg[1]);
			shmMsg[0] = 0;
			shmMsg[1] = 0;
			childpid = fork();
			processIds[processCount] = childpid;
			processCount++;		
		}	
		if((clock[1] + 10000000) >= 1000000000){
			clock[1] = 0;
			clock[0]++;	
		}
		else
			clock[1] += 10000000;
		time(&endTime);
		elapsedTime = difftime(endTime, startTime);	
	}	

	fclose(file);

	shmctl(memid, IPC_RMID, NULL);
	shmctl(memid2, IPC_RMID, NULL);

	for(i = 0; i < processCount; i++)
		kill(processIds[i], SIGKILL);

	return 0;
}
