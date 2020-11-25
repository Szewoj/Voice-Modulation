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

sem_t* semarr[3];

void term(){
	int val = 0;
	for(int i = 3; i>0; --i){
		sem_getvalue(semarr[i], &val);
		if(val < 1){
			sem_post(semarr[i]);	
		}
		printf("semafor /log%d, wartosc %d", i, val);
		puts("");
		sem_close(semarr[i]); //zamkniece semafora przy sigterm	
	}
}

int main(int argc, char const *argv[])
{

	struct timeval t[4], dt[3];

	struct sigaction action;	//przypisane term() do SIGTERM
	memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = term;
    sigaction(SIGTERM, &action, NULL);



	FILE* log;
	char fname[3][15];
	char semName[6];
	int val;
	for(int i = 3; i>0; --i){

		snprintf(semName, 6, "/log%d", i);
		semarr[i] = sem_open(semName, O_CREAT, O_RDWR, 1);
		sem_getvalue(semarr[i], &val);
		printf("semafor %s, wartosc %d", semName, val);
		puts("");

		sem_wait(semarr[i]); //lock
		snprintf(fname[i], 15, "logs/log%d.txt", i);
		log = fopen(fname[i], "w");
		if(log == NULL){
			puts("file opening failure");
			exit(1);
		}
		fclose(log);
		sem_post(semarr[i]); //unlock
	}


	srand(time(NULL));
	int rando = 0;
	for(;;){
		for(int i = 3; i>0; --i){
			gettimeofday(&t[i+1], NULL);
			rando = 10000 + rand() %30000;
			usleep(rando);
			gettimeofday(&t[i], NULL);
			timersub(&t[i+1], &t[i], &dt[i]);

			sem_wait(semarr[i]);
			log = fopen(fname[i], "a");
			if(log == NULL){
				puts("file opening failure");
				exit(1);
			}
			fprintf(log, "%ld\n", (long int)dt[i].tv_usec );
			fclose(log);
			sem_post(semarr[i]);
		}
	}
	return 0;
}
