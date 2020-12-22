#include <stdio.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <Python.h>
#include <string.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <pthread.h>

pthread_spinlock_t  *samp_raw_sl, *samp_mod_sl;

void openSystem(bool* system_on, pid_t* system_pid);
void runCapture(pid_t* system_pid);
void runModulator(pid_t* system_pid, int param);
void runPlayback(pid_t* system_pid);
void closeSystem(bool* system_on, pid_t* system_pid);
void createPlots(bool* plotter_on, pid_t* plotter_pid_ptr);
void closePlots(bool* plotter_on, pid_t plotter_pid);
int getChoice(const char* text, int max);
bool goodButton(int c, int max);
void unlinkSemaphores();
void unlinkShm();
void unlinkSpinlock();

int main(int argc, char const *argv[])
{
	pid_t system_pid[3];
	pid_t plotter_pid;
	bool system_on = false;
	bool plotter_on = false;
	int choice;
	const char* MENU_TEXT = "\nMake a choice:\n1 - open system\n2 - close system\n3 - plot times\n4 - close plots\n5 - exit\n";

	int fd_samp_raw, fd_samp_mod;

	fd_samp_raw = shm_open("/samp_raw", O_CREAT | O_RDWR, 0666);
	ftruncate(fd_samp_raw, sizeof(pthread_spinlock_t));
	samp_raw_sl = mmap(0, sizeof(pthread_spinlock_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd_samp_raw, 0);

	fd_samp_mod = shm_open("/samp_mod", O_CREAT | O_RDWR, 0666);
	ftruncate(fd_samp_mod, sizeof(pthread_spinlock_t));
	samp_mod_sl = mmap(0, sizeof(pthread_spinlock_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd_samp_mod, 0);


	pthread_spin_init(samp_raw_sl, PTHREAD_PROCESS_SHARED);
	pthread_spin_init(samp_mod_sl, PTHREAD_PROCESS_SHARED);



	
	for(;;){
		choice = getChoice(MENU_TEXT, 5);
		switch(choice){
			case 1:
				openSystem(&system_on, system_pid);
			break;
			case 2:
				closeSystem(&system_on, system_pid);
			break;
			case 3:
				createPlots(&plotter_on, &plotter_pid);
			break;
			case 4:
				closePlots(&plotter_on, plotter_pid);
			break;
			case 5:
				closeSystem(&system_on, system_pid);
				closePlots(&plotter_on, plotter_pid);
				unlinkSemaphores();
				unlinkSpinlock();
				unlinkShm();
				puts("closing system");
				exit(0);
			break;
			default:
			break;
		}
	}
	return 0;
}

void openSystem(bool* system_on, pid_t* system_pid){
	if(!*system_on){
		const char* PARAM_TEXT = "\n1 - modulation\n2 - no effects\n";
		int param = getChoice(PARAM_TEXT, 2);

		runCapture(system_pid);
		runModulator(system_pid, param);
		runPlayback(system_pid);

		*system_on = true;
		puts("system launched");
	}
	else{
		puts("system already on");
	}
}

void runCapture(pid_t* system_pid){
	system_pid[0] = fork();
	if(system_pid[0] == -1){
		fprintf(stderr, "process creation failed");
		exit(errno);
	}
	else if(system_pid[0] == 0){
		execlp("build/capture", "capture", NULL);
		fprintf(stderr, "capture execution failed");
		exit(errno);
	}	
}

void runModulator(pid_t* system_pid, int param){
	system_pid[1] = fork();
	if(system_pid[1] == -1){
		fprintf(stderr, "process creation failed");
		exit(errno);
	}
	else if(system_pid[1] == 0){
		char p[2];
		snprintf(p, 2, "%d", param);
		execlp("build/modulator", "modulator", p, NULL);
		fprintf(stderr, "modulator execution failed");
		exit(errno);
	}
}

void runPlayback(pid_t* system_pid){
	system_pid[2] = fork();
	if(system_pid[2] == -1){
		fprintf(stderr, "process creation failed");
		exit(errno);
	}
	else if(system_pid[2] == 0){
		execlp("build/playback", "playback", NULL);
		fprintf(stderr, "playback execution failed");
		exit(errno);
	}	
}

void closeSystem(bool* system_on, pid_t system_pid[]){
	if(*system_on){
		for(int i = 2; i >= 0; --i){
			kill(system_pid[i], SIGTERM);
			wait(NULL);
		}
		puts("system closed");
		*system_on = false;
	}
	else{
		puts("system already off");
	}
}

void createPlots(bool* plotter_on, pid_t* plotter_pid_ptr){
	if(!*plotter_on){
		puts("launching plotter");
		*plotter_pid_ptr = fork();
		if(*plotter_pid_ptr == -1){
			fprintf(stderr, "process creation failed");
			exit(1);
		}else if(*plotter_pid_ptr == 0){
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

	sem = sem_open("/samp_raw", O_CREAT, O_RDWR, 1);
	sem_close(sem);
	sem_unlink("/samp_raw");
	
	sem = sem_open("/samp_mod", O_CREAT, O_RDWR, 1);
	sem_close(sem);
	sem_unlink("/samp_mod");
}

void unlinkShm(){
	shm_unlink("/raw");
	shm_unlink("/mod");
	shm_unlink("/samp_raw");
	shm_unlink("/samp_mod");

}

void unlinkSpinlock(){
	pthread_spin_destroy(samp_raw_sl);
	pthread_spin_destroy(samp_mod_sl);
}
