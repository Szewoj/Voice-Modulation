#include<stdio.h>
#include<sys/types.h>
#include<stdbool.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>
#include<python2.7/Python.h>
#include<string.h>
#include<semaphore.h>
#include<sys/stat.h>
#include<fcntl.h>

int getChoice(const char* text, int max);
bool goodButton(int c, int max);
void openSystem(bool* generator_on, pid_t* generator_pid);
void execGenerator(int param);
void closeGenerator(bool* generator_on, pid_t generator_pid);
void createPlots(bool* plotter_on, pid_t* plotter_pid);
void closePlots(bool* plotter_on, pid_t plotter_pid);
void unlinkSemaphores();

int main(int argc, char const *argv[])
{
	pid_t generator_pid = 0;
	pid_t plotter_pid = 0;
	bool generator_on = false;
	bool plotter_on = false;
	int choice;
	const char* MENU_TEXT = "\nMake a choice:\n1 - open generator\n2 - close generator\n3 - plot times\n4 - close plots\n5 - exit\n";
	
	for(;;){
		choice = getChoice(MENU_TEXT, 5);
		switch(choice){
			case 1:
				openSystem(&generator_on, &generator_pid);
			break;
			case 2:
				closeGenerator(&generator_on, generator_pid);
			break;
			case 3:
				createPlots(&plotter_on, &plotter_pid);
			break;
			case 4:
				closePlots(&plotter_on, plotter_pid);
			break;
			case 5:
				closeGenerator(&generator_on, generator_pid);
				closePlots(&plotter_on, plotter_pid);
				unlinkSemaphores();
				puts("closing system");
				exit(0);
			break;
			default:
			break;
		}
	}
	return 0;
}

void openSystem(bool* generator_on, pid_t* generator_pid){
	if(!*generator_on){
		const char* PARAM_TEXT = "\nhow slow? (int from 1 to 3)\n";
		int param = getChoice(PARAM_TEXT, 3);

		*generator_pid = fork();
		if(*generator_pid == -1){
			fprintf(stderr, "process creation failed");
			exit(1);
		}
		else if(*generator_pid == 0){
			execGenerator(param);
		}
		*generator_on = true;
		puts("generator launched");
	}
	else{
		puts("generator already on");
	}
}

void execGenerator(int param){
	char p[2];
	snprintf(p, 2, "%d", param);
	//fprintf(stderr,"summoner: %s %s\n", "build/generator", p);
	execlp("build/modulator", "build/modulator", p, NULL);
	fprintf(stderr, "generator execution failed");
	exit(errno);
}

void closeGenerator(bool* generator_on, pid_t generator_pid){
	if(*generator_on){
		puts("generator closed");
		kill(generator_pid, SIGTERM);
		*generator_on = false;
	}
	else{
		puts("no generator active");
	}
}

void createPlots(bool* plotter_on, pid_t* plotter_pid){
	if(!*plotter_on){
		puts("launching plotter");
		*plotter_pid = fork();
		if(*plotter_pid == -1){
			fprintf(stderr, "process creation failed");
			exit(1);
		}else if(*plotter_pid == 0){
			execlp("python", "python", "scripts/plotter.py", NULL);
			fprintf(stderr, "plotter execution failed");
			exit(errno);
		}
		*plotter_on = true;
	}
	else{
		puts("plots already created");
	}
}

void closePlots(bool* plotter_on, pid_t plotter_pid){
	if(*plotter_on){
		puts("closing plots");
		kill(plotter_pid, SIGTERM);
		*plotter_on = false;
	}
	else{
		puts("no plots open");
	}	
}

int getChoice(const char* text, int max){
	puts(text);
	int choice;
	do{
		scanf("%d", &choice);
		while((getchar()!='\n')){};
		if(!goodButton(choice, max)){
			puts("unaccteptable input");
			puts(text);
		}
	}while(!goodButton(choice, max));
	return choice;
}

bool goodButton(int c, int max){
	int maxButton = max;
	for(int i = maxButton; i>0; --i){
		if(c == i)
			return true;
	}
	return false;
}

void unlinkSemaphores(){
	sem_t* sem;
	char semName[6];
	for(int i = 3; i>0; --i){
		snprintf(semName, 6, "/log%d", i);
		sem = sem_open(semName, O_CREAT, O_RDWR, 1);
		sem_close(sem);
		sem_unlink(semName);
	}
}
