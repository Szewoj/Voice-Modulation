
#include <iostream>
#include <thread>
#include <ctime>
#include <chrono>
#include <semaphore.h>
#include <csignal>
#include <fstream>
#include <sys/stat.h>
#include <fcntl.h>
#include <queue>
#include "SoundTouch.h"

#define SAMPLE_RATE (44100)
#define NUM_CHANNELS (1)

#define TEMPO_CHANGE (0)
#define PITCH_SEMITONES (10)
#define RATE_CHANGE (0)

#define BUFFER_SIZE (20)

using namespace soundtouch;
using namespace std;


sem_t* log1_semaphore;
sem_t* log2_semaphore;

queue<chrono::high_resolution_clock::time_point> processing_start_times;
queue<unsigned int> log1_time_diff;
queue<unsigned int> log2_time_diff;


void log_handle();
void SIGTERM_handler(int signal_id);

int main()
{	/*************************************************************************************/
	// Main loop variables:	
	int inSamples;
	int outSamples;

	int inSampleBuffer[BUFFER_SIZE] = {0};
	int outSampleBuffer[BUFFER_SIZE] = {0};

	unsigned int diff;
	SoundTouch ST;


	ST.setSampleRate(SAMPLE_RATE);
	ST.setChannels(NUM_CHANNELS);

	ST.setTempoChange(TEMPO_CHANGE);
	ST.setPitchSemiTones(PITCH_SEMITONES);
	ST.setRateChange(RATE_CHANGE);

	ST.setSetting(SETTING_USE_QUICKSEEK, 1);
	ST.setSetting(SETTING_USE_AA_FILTER, 0);

	ST.setSetting(SETTING_SEQUENCE_MS, 40);
	ST.setSetting(SETTING_SEEKWINDOW_MS, 15);
	ST.setSetting(SETTING_OVERLAP_MS, 8);

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
	thread logging_thread(log_handler);
	/*************************************************************************************/
	// Main processing loop:
	while (EXIT_FAILURE) 
	{
		//data in
		//ST.putSamples(sampleBuffer,inSamples);



		t_start = chrono::high_resolution_clock::now();
		//for(int i = 0; i <= inSamples; ++i)
		processing_start_times.push(t_start);



		do 
		{
			outSamples = ST.receiveSamples(inSampleBuffer, inSamples);

			t_end = chrono::high_resolution_clock::now();

			for(int i = 0; i <= outSamples; ++i){
				time_span = processing_start_times.front() - t_end;
				processing_start_times.pop();

				log2_time_diff.push(time_span.count())
			}

			// write to output file

		} while (outSamples != 0);

		

	}
	/*************************************************************************************/
	// If somehow loop broke, exit with error
	return EXIT_FAILURE;
	/*************************************************************************************/
}


void log_handler()
{
	fstream log_file;

	while (EXIT_FAILURE)
	{

		if (!log1_time_diff.empty())
		{
			sem_wait(log1_semaphore);
			log_file.open("logs/log1.txt", fstream::out | fstream::in | fstream::app);

			do {

				log_file << log1_time_diff.front() << '\n';
				log1_time_diff.pop();

			} while(!log1_time_diff.empty())

			log_file.close();
			sem_post(log1_semaphore);
		}

		if (!log2_time_diff.empty())
		{
			sem_wait(log2_semaphore);
			log_file.open("logs/log2.txt", fstream::out | fstream::in | fstream::app);

			do {

				log_file << log2_time_diff.front() << '\n';
				log2_time_diff.pop();

			} while(!log2_time_diff.empty())

			log_file.close();
			sem_post(log2_semaphore);
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
