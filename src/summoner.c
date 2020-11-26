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

#define MENU_MESSAGE "\nMake a choice:\n1 - open generator\n2 - close generator\n3 - plot times\n4 - close plots\n5 - exit\n"
#define PARAM_TEXT "\nhow slow? (int from 1 to 3)\n"

int getChoice(const char* text, int max);
bool goodButton(int c, int max);
void openGenerator(bool* generator_on, pid_t* generator_pid);
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
	for(;;){
		puts(MENU_MESSAGE);
		choice = getChoice(MENU_MESSAGE, 5);

		switch(choice){
			case 1:
				openGenerator(&generator_on, &generator_pid);		
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



void openGenerator(bool* generator_on, pid_t* generator_pid){
	if(!*generator_on){
		puts(PARAM_TEXT);
		int param = getChoice(PARAM_TEXT, 3);
		*generator_pid = fork();
		if(*generator_pid == -1){
			fprintf(stderr, "process creation failed");
			exit(1);
		}
		else if(*generator_pid == 0){
			//char* argvgen[] = {NULL, "1"};
			//snprintf(argvgen[1], 2, "%d", param);
			//printf("%s%s%s\n", argvgen[0], argvgen[1], argvgen[2]);
			char* argvgen[] = {NULL};
			execv("build/generator", argvgen);
			fprintf(stderr, "generator execution failed");
			exit(errno);	//program dojdzie tu tylko przy bledzie
		}
		*generator_on = true;
		puts("generator launched");
		}
	else{
		puts("generator already on");
	}
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
	int choice;
	do{
		scanf("%d", &choice);
		while((getchar()!='\n')){};
		if(!goodButton(choice, max)){
			puts("unnactepable input");
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
