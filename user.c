/**
 * Author: Taylor Freiner
 * Date: October 5th 2017
 * Log: Allocating shared memory
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/shm.h>

int main(int argc, char* argv[]){

	srand(time(NULL));
	int execTime = (rand() % 999999) + 1;

	key_t key = ftok("keygen", 1);
	key_t key2 = ftok("keygen2", 1);
	int memid = shmget(key, sizeof(int*)*2, IPC_CREAT | 0644);
	int memid2 = shmget(key2, sizeof(int*)*2, IPC_CREAT | 0644);
	if(memid == -1 || memid2 == -1){
		printf("%s: ", argv[0]);
		perror("Error\n");
	}

	int *clock = (int *)shmat(memid, NULL, 0);	
	int *shmMsg = (int *)shmat(memid2, NULL, 0);
	if(*clock == -1 || *shmMsg == -1){
		printf("%s: ", argv[0]);
		perror("Error\n");
	}

	execTime = clock[0] * 1000000000 + clock[1] + execTime;	

	return 0;
}
