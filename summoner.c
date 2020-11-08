#include<stdio.h>
#include<sys/types.h>
#include<stdbool.h>
#include<stdlib.h>
#include<unistd.h>

int getChoice();
bool goodButton(int c);

int main(int argc, char const *argv[])
{
	pid_t generatorPid = 0;
	bool generator_on = false;
	for(;;){
		puts("Make a choice:\n1 - open generator\n2 - close generator\n3 - exit");
		int choice = getChoice();

		switch(choice){
			case 1:{
				if(!generator_on){
					generatorPid = fork();
					if(generatorPid == -1){
						puts("process creation failure");
						exit(1);
					}
					else if(generatorPid == 0){
						char* argv[] = {NULL};
						execv("bum", argv);
						exit(0);
					}
					generator_on = true;
					puts("generator launched");
				}
				else{
					puts("generator already on");
				}
				
			}
			break;
			case 2:{
					if(generator_on){
						kill(generatorPid);
						puts("closing generator");
						generator_on = false;
					}
					else{
						puts("generator already off");
					}
			}
			break;
			case 3:{
				if(generator_on){
					kill(generatorPid);
					puts("closing generator");
					generator_on = false;
				}
				else{
					puts("generator already off");
				}
				puts("closing system");
				exit(0);
			}
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
			puts("wrong button");
	}while(!goodButton(choice));
	return choice;
}
bool goodButton(int c){
	return c == 1 || c == 2 || c == 3;
}