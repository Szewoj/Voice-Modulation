#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>

sem_t* sem;

void term(){
	sem_close(sem); //zamkniece semafora przy sigterm
}

int main(int argc, char const *argv[])
{
	struct timeval time_before, time_after, time_elapsed;

	struct sigaction action;	//przypisane term() do SIGTERM
	memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = term;
    sigaction(SIGTERM, &action, NULL);

	FILE* fp ;

	const char* semName = "/times";
	sem = sem_open(semName, O_CREAT, O_RDWR, 1);
	
	sem_wait(sem); //lock
	fp = fopen("build/times.txt", "w");
	if(fp == NULL){
		puts("file opening failure");
		exit(1);
	}
	fclose(fp);
	sem_post(sem); //unlock

	srand(time(NULL));
	int i = 0;
	for(;;){
		gettimeofday(&time_before, NULL);
		i = 10000 + rand() %30000;
		usleep(i);
		sleep(1);
		gettimeofday(&time_after, NULL);
		timersub(&time_after, &time_before, &time_elapsed);

		sem_wait(sem);
		fp = fopen("build/times.txt", "a");
		if(fp == NULL){
			puts("file opening failure");
			exit(1);
		}
		fprintf(fp, "%ld\n", (long int)time_elapsed.tv_usec );
		fclose(fp);
		sem_post(sem);
	}
	return 0;
}
