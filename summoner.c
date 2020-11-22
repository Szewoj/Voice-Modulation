#include<stdio.h>
#include<sys/types.h>
#include<stdbool.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>
#include<python2.7/Python.h>
#include<string.h>

int getChoice();
bool goodButton(int c);
void openGenerator(bool* generator_on, pid_t* generator_pid);
void closeGenerator(bool* generator_on, pid_t generator_pid);
void createPlots(bool* plotter_on, pid_t* plotter_pid);
void closePlots(bool* plotter_on, pid_t plotter_pid);

int main(int argc, char const *argv[])
{
	pid_t generator_pid = 0;
	pid_t plotter_pid = 0;
	bool generator_on = false;
	bool plotter_on = false;
	FILE* fp;
	for(;;){
		puts("\nMake a choice:\n1 - open generator\n2 - close generator\n3 - plot times\n4 - close plots\n5 - exit\n");
		int choice = getChoice();

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
				puts("closing system");
				exit(0);
			break;
			default:
			break;
		}
	}
	return 0;
}

int getChoice(){
	int choice;
	do{
		scanf("%d", &choice);
		while((getchar()!='\n')){};
		if(!goodButton(choice))
			puts("wrong button\nMake a choice:\n1 - open generator\n2 - close generator\n3 - plot times\n4 - exit");
	}while(!goodButton(choice));
	return choice;
}

bool goodButton(int c){
	int maxButton = 5;
	for(int i = maxButton; i>0; --i){
		if(c == i)
			return true;
	}
	return false;
}

void openGenerator(bool* generator_on, pid_t* generator_pid){
	if(!*generator_on){
			*generator_pid = fork();
			if(*generator_pid == -1){
				puts("process creation failure");
				exit(1);
			}
			else if(*generator_pid == 0){
				char* argv[] = {NULL};
				execv("build/generator", argv);
				exit(0);
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
		kill(generator_pid, SIGKILL);
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
			puts("process creation failure");
			exit(1);
		}else if(*plotter_pid == 0){
			execlp("python", "python", "plotter.py", NULL);
			exit(0);
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
		kill(plotter_pid, SIGKILL);
		*plotter_on = false;
	}
	else{
		puts("no plots open");
	}	
}