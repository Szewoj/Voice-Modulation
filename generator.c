#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<time.h>
#include<sys/time.h>

void generator(){
	struct timeval time_before, time_after, time_elapsed;
	srand(time(NULL));
	int i = 1;	
	for(;;){
		gettimeofday(&time_before, NULL);
		i = rand() %100;
		for(int i = rand()%10; i>0; --i){}		
		sleep(1);
		gettimeofday(&time_after, NULL);
		timersub(&time_after, &time_before, &time_elapsed);
		printf("generating %2d in time %ld.%06ld\n", i, (long int)time_elapsed.tv_sec, (long int)time_elapsed.tv_usec);
	}
}

int main(int argc, char const *argv[])
{
	generator();
	return 0;
}