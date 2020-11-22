#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<time.h>
#include<sys/time.h>

int main(int argc, char const *argv[])
{
	struct timeval time_before, time_after, time_elapsed;
	FILE* fp ;
	fp = fopen("build/times.txt", "w");
	if(fp == NULL){
		puts("file opening failure");
		exit(1);
	}
	fclose(fp);
	srand(time(NULL));
	int i = 1;
	for(;;){
		gettimeofday(&time_before, NULL);
		i = 10000 + rand() %30000;
		usleep(i);
		gettimeofday(&time_after, NULL);
		timersub(&time_after, &time_before, &time_elapsed);
		fp = fopen("build/times.txt", "a");
		if(fp == NULL){
			puts("file opening failure");
			exit(1);
		}
		fprintf(fp, "%ld\n", (long int)time_elapsed.tv_usec );
		fclose(fp);
	}
	return 0;
}
