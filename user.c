/**
 * Author: Taylor Freiner
 * Date: October 7th 2017
 * Log: Setting up semaphore
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <strings.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <semaphore.h>

void criticalSection();

int main(int argc, char* argv[]){
	printf("IN USER\n");
	srand(time(NULL));
	int execTime = (rand() % 999999) + 1;
	key_t key = ftok("keygen", 1);
	key_t key2 = ftok("keygen2", 1);
	key_t key3 = ftok("keygen3", 1);
	int memid = shmget(key, sizeof(int*)*2, 0);
	int memid2 = shmget(key2, sizeof(int*)*3, 0);
	int semid = semget(key3, 1, 0);
	sem_t *sem = (sem_t*)shmat(semid, NULL, 0);
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
	semctl(semid, 0, IPC_STAT);
	if(*clock == -1 || *shmMsg == -1){
		printf("%s: ", argv[0]);
		perror("Error\n");
	}
	shmMsg[2] = getpid();

	int *localClock = clock;
	if((localClock[1] + execTime) > 1000000000){
		localClock[1] = (localClock[1] + execTime) % 1000000000;
		clock[0]++;
	}
	else
		localClock[1] += execTime;

	printf("BEFORE CRITICAL\n");
	do{
		sem_wait(sem);
		criticalSection();
		sem_post(sem);
	}
	while(1);

	return 0;
}

void criticalSection(){
	printf("IN CRITICAL SECTION\n");
	return;
}
