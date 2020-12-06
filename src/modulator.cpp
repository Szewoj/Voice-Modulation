
#include <iostream>
#include <thread>
#include <ctime>
#include <chrono>
#include <semaphore.h>
#include <csignal>
#include <fstream>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

sem_t* log1_semaphore;
sem_t* log2_semaphore;

unsigned long int diff;
bool msg_ready = false;
short int receiver = 1;


void log_handle();
void SIGTERM_handler(int signal_id);

void log_request(int msg);

int main()
{	/*************************************************************************************/
	// Main loop variables:	
	int value_in;

	/*************************************************************************************/
	// Time measurement variables:
	chrono::high_resolution_clock::time_point t_start;
	chrono::high_resolution_clock::time_point t_end;
	chrono::duration<double, std::milli> time_span;
	/*************************************************************************************/
	// Semaphore configuration
	log1_semaphore = sem_open("/log1", O_CREAT, O_RDWR, 1);
	log2_semaphore = sem_open("/log2", O_CREAT, O_RDWR, 1);

	signal(SIGTERM, SIGTERM_handler);
	/*************************************************************************************/
	// Log file initialisation:
	fstream log_file;

	log_file.open("logs/log1.txt", fstream::out | fstream::in | fstream::trunc);
	log_file.close();

	log_file.open("logs/log2.txt", fstream::out | fstream::in | fstream::trunc);
	log_file.close();

	/*************************************************************************************/
	// Logging thread launch:
	thread logging_thread(log_handle);
	/*************************************************************************************/
	// Main processing loop:
	while (EXIT_FAILURE) 
	{

		cin >> value_in;
		t_start = chrono::high_resolution_clock::now();

		while (msg_ready);
		
		t_end = chrono::high_resolution_clock::now();

		time_span = t_start - t_end;

		diff = time_span.count();
		msg_ready = true;

	}
	/*************************************************************************************/
	// If somehow loop broke, exit with error
	return 1;
	/*************************************************************************************/
}


void log_handle()
{
	fstream log_file;

	while (EXIT_FAILURE)
	{

		if (msg_ready)
		{
			sem_wait(log1_semaphore);

			log_file.open("logs/log1.txt", fstream::out | fstream::in | fstream::app);
			
			log_file << diff << '\n';

			log_file.close();

			sem_post(log1_semaphore);

			msg_ready = false;
		}

	}

}

void SIGTERM_handler(int signal_id)
{
	int tmp = 0;

	sem_getvalue(log1_semaphore, &tmp);
	if(!tmp)
		sem_post(log1_semaphore);

	sem_close(log1_semaphore);


	sem_getvalue(log2_semaphore, &tmp);
	if(!tmp)
		sem_post(log2_semaphore);

	sem_close(log2_semaphore);
	
	exit(EXIT_SUCCESS);
}

void log_request(int msg)
{
	
}