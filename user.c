/**
 * Author: Taylor Freiner
 * Date: October 17th 2017
 * Log: Finished 
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <strings.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <errno.h>

int criticalSection(int *, int *, int *, int);

int main(int argc, char* argv[]){
	struct sembuf sb;
	srand(time(NULL) ^ (getpid()<<16));
	int execTime = (rand() % 999999) + 1;
	key_t key = ftok("keygen", 1);
	key_t key2 = ftok("keygen2", 1);
	key_t key3 = ftok("keygen3", 1);
	int memid = shmget(key, sizeof(int*)*2, 0);
	int memid2 = shmget(key2, sizeof(int*)*3, 0);
	int semid = semget(key3, 1, 0);
	if(memid == -1 || memid2 == -1){
		printf("%s: ", argv[0]);
		perror("Shared memory creation error: ");
	}
	if(semid < 0){
		printf("%s: ", argv[0]);
		perror("Semaphore creation error: ");
	}

	int *clock = (int *)shmat(memid, NULL, 0);
	int *shmMsg = (int *)shmat(memid2, NULL, 0);
	if(*clock == -1 || *shmMsg == -1){
		printf("%s: ", argv[0]);
		perror("Error\n");
	}

	int *localClock = clock;
	if((localClock[1] + execTime) > 1000000000){
		localClock[1] = (localClock[1] + execTime) % 1000000000;
		localClock[0]++;
	}
	else
		localClock[1] += execTime;
	int localTime = localClock[0] * 1000000000 + localClock[1];
	int status = 1;
	do{
		sb.sem_op = -1;
		sb.sem_num = 0;
		sb.sem_flg = 0;
		semop(semid, &sb, 1);	
		
		status = criticalSection(shmMsg, clock, localClock, localTime);
		
		sb.sem_op = 1;
		semop(semid, &sb, 1);
		
		if(status == 0)
			exit(0);
	}
	while(status == 1);

	return 0;
}

int criticalSection(int *shmMsg, int *clock, int *localClock, int localTime){
	if(((clock[0] * 1000000000) + clock[1]) > localTime){
		if(shmMsg[2] == -1){
			shmMsg[0] = localClock[0];
			shmMsg[1] = localClock[1];
			shmMsg[2] = (int)getpid();
			return 0;
		}
	}

	return 1;
}
