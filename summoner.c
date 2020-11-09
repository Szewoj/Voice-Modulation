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
void closeGenerator(bool generator_on, pid_t generatorPid);
void openGenerator(bool generator_on, pid_t* generatorPid);

int main(int argc, char const *argv[])
{
	pid_t generatorPid = 0;
	bool generator_on = false;
	FILE* fp;
	for(;;){
		puts("Make a choice:\n1 - open generator\n2 - close generator\n3 - plot times\n4 - exit");
		int choice = getChoice();

		switch(choice){
			case 1:
				//openGenerator(generator_on, &generatorPid);
				if(!generator_on){
					generatorPid = fork();
					if(generatorPid == -1){
						puts("process creation failure");
						exit(1);
					}
					else if(generatorPid == 0){
						char* argv[] = {NULL};
						execv("generator", argv);
						exit(0);
					}
					generator_on = true;
					puts("generator launched");
				}
			else{
				puts("generator already on");
			}				
			break;
			case 2:
				closeGenerator(generator_on, generatorPid);
			break;
			case 3:{
				puts("launching plotter");
				pid_t plotterPid = fork();
				if(plotterPid == -1){
					puts("process creation failure");
					exit(1);
				}else if(plotterPid == 0){
					execlp("python", "python", "plotter.py", NULL);
					exit(0);
				}
			}
			break;
			case 4:
				closeGenerator(generator_on, generatorPid);
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
	int maxButton = 4;
	for(int i = maxButton; i>0; --i){
		if(c == i)
			return true;
	}
	return false;
}

void closeGenerator(bool generator_on, pid_t generatorPid){
	if(generator_on){
		puts("closing generator");
		kill(generatorPid, SIGKILL);
		generator_on = false;
	}
	else{
		puts("generator already off");
	}
}

void openGenerator(bool generator_on, pid_t* generatorPid){
	if(!generator_on){
		*generatorPid = fork();
		if(*generatorPid == -1){
			puts("process creation failure");
			exit(1);
		}
		else if(*generatorPid == 0){
			char* argv[] = {NULL};
			execv("generator", argv);
			exit(0);
		}
		generator_on = true;
		puts("generator launched");
	}
	else{
		puts("generator already on");
	}
}
