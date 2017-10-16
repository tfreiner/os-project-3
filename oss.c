/**
 * Author: Taylor Freiner
 * Date: October 16th, 2017
 * Log: Wrapping up
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/types.h>

int sharedmem[3];

union semun {
	int val;
};

int main(int argc, char* argv[])  {

	union semun arg;
	arg.val = 1;
	int i, option;
	int execTime = 0;
	int processCount = 0;
	int processNum = 0;
	int processIds[100];
	char argval;
	char* filename = (char *)malloc(100);
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
	key_t key3 = ftok("keygen3", 1);
	int memid = shmget(key, sizeof(int*)*2, IPC_CREAT | 0644);
	int memid2 = shmget(key2, sizeof(int*)*3, IPC_CREAT | 0644);
	int semid = semget(key3, 1, IPC_CREAT | 0644);
	if(memid == -1 || memid2 == -1){
		printf("%s: ", argv[0]);
		perror("Error\n");
	}
	sharedmem[0] = memid;
	sharedmem[1] = memid2;
	sharedmem[2] = semid;
	int *clock = (int *)shmat(memid, NULL, 0);
	int *shmMsg = (int *)shmat(memid2, NULL, 0);
	if(*clock == -1 || *shmMsg == -1){
		printf("%s: ", argv[0]);
		perror("Error\n");
	}
	int clockVal = 0;
	int exitId = -1;
	for(i = 0; i < 2; i++){
		memcpy(&clock[i], &clockVal, 4);
		memcpy(&shmMsg[i], &clockVal, 4);
	}
	memcpy(&shmMsg[2], &exitId, 4);
	semctl(semid, 0, SETVAL, 1, arg);
	if(errno){
		fprintf(stderr, "%s\n", strerror(errno));
		exit(1);
	}

	//CREATING PROCESSES
	pid_t childpid;
	for(i = 0; i < processNum; i++){
		childpid = fork();
		if(errno){
			fprintf(stderr, "%s", strerror(errno));
			exit(1);
		}	
	
		if(childpid == 0)
			execl("./user", "user", NULL);

		if(errno){
			fprintf(stderr, "%s", strerror(errno));
			exit(1);
		}
		processIds[i] = childpid;
		processCount++;
	}

	while(clock[0] < 2 && processCount  < 100 && elapsedTime < execTime){
		if(shmMsg[2] != -1){
			fprintf(file, "Master: Child %d is terminating at my time %d.%d because it reached %d.%d in slave\n", shmMsg[2], clock[0], clock[1], shmMsg[0], shmMsg[1]);
			shmMsg[0] = 0;
			shmMsg[1] = 0;
			shmMsg[2] = -1;
			childpid = fork();
			if(childpid == 0){
				execl("./user", "user", NULL);
			}
			processIds[processCount] = childpid;
			processCount++;		
		}
		if((clock[1] + 100000) >= 1000000000){
			clock[1] = (clock[1] + 1000000) % 1000000000;
			clock[0]++;	
		}
		else
			clock[1] += 100000;
		time(&endTime);
		elapsedTime = difftime(endTime, startTime);	
		
		if(clock[0] >= 2){
			printf("2 seconds passed of the simulated clock.\n");
			printf("Time: %d seconds, %d nanoseconds\n.", clock[0], clock[1]);
		}
		if(processCount >= 100)
			printf("100 processes have been created.\n");
		if(elapsedTime >= execTime)
			printf("%d seconds passed of the real clock.\n", execTime);
	}	
	fclose(file);
	shmctl(memid, IPC_RMID, NULL);
	shmctl(memid2, IPC_RMID, NULL);
	semctl(semid, 0, IPC_RMID);

	for(i = 0; i < processCount; i++)
		kill(processIds[i], SIGKILL);

	return 0;
}
