#include <iostream>
#include <thread>
#include <semaphore.h>
#include <csignal>
#include <fstream>
#include <sys/stat.h>
#include <fcntl.h>
#include <queue>
#include <cmath>
#include <cstring>
#include <sys/time.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>

#define SAMPLE_RATE (20000)
#define NUM_CHANNELS (1)

#define PITCH_SEMITONES (10)

#define BUFFER_SIZE (2048)
#define MAX_FRAME_LENGTH (1024)

#define FRAME_MS (20)
#define OVERLAP_MS (1)

#define NUM_MILISECONDS (20)

using namespace std;


sem_t* log1_semaphore;
sem_t* log2_semaphore;


char *addrIn, *addrOut, *samp_raw_sl, *samp_mod_sl;
int fdIn, fdOut, fd_samp_raw, fd_samp_mod;

queue<unsigned int> log1_time_diff;
queue<unsigned int> log2_time_diff;


void FFT(float* buffer, long int frame_size, long int direction); // direction: -1: FFT, 1: IFFT
void processSamples(long int semitones, long int numSamples, long int frame_size, long int osamp, float sampleRate, short int *indata, short int *outdata);

void sl_try(char* sl);
void sl_open(char* sl);
void log_handler();
void SIGTERM_handler(int signal_id);

int main(int argc, char const *argv[])
{
	cerr << "running modulator\n";
	/*************************************************************************************/
	// Main loop variables:
	bool isMod = strtol(argv[1], NULL, 10) == 1;

	int inSamples = NUM_MILISECONDS * SAMPLE_RATE/1000;
	short int inSampleBuffer[BUFFER_SIZE] = {0};
	short int outSampleBuffer[BUFFER_SIZE] = {0};


	int sframe = SAMPLE_RATE * FRAME_MS /1000;
	int overlap = SAMPLE_RATE * OVERLAP_MS /1000;

	long checkIn;
	/*************************************************************************************/
	// Time measurement variables:
	struct timeval sendTime, receiveTime;
	struct timeval postTime;
	struct timeval startTime, endTime;
	/*************************************************************************************/
	// Semaphore configuration
	log1_semaphore = sem_open("/log1", O_CREAT, O_RDWR, 1);
	log2_semaphore = sem_open("/log2", O_CREAT, O_RDWR, 1);


	signal(SIGTERM, SIGTERM_handler);
	signal(SIGINT, SIGTERM_handler);

	/*************************************************************************************/
	// Shared memory and spinlock initialisation:
	
	fdIn = shm_open("/raw", O_CREAT | O_RDWR, 0666);
	ftruncate(fdIn, 2048);
	addrIn = (char*)mmap(0, 2048, PROT_READ | PROT_WRITE, MAP_SHARED, fdIn, 0);

	fdOut = shm_open("/mod", O_CREAT | O_RDWR, 0666);
	ftruncate(fdOut, 2048);
	addrOut = (char*)mmap(0, 2048, PROT_READ | PROT_WRITE, MAP_SHARED, fdOut, 0);

	fd_samp_raw = shm_open("/samp_raw", O_CREAT | O_RDWR, 0666);
	ftruncate(fd_samp_raw, 1);
	samp_raw_sl = (char*)mmap(0, 1, PROT_READ | PROT_WRITE, MAP_SHARED, fd_samp_raw, 0);

	fd_samp_mod = shm_open("/samp_mod", O_CREAT | O_RDWR, 0666);
	ftruncate(fd_samp_mod, 1);
	samp_mod_sl = (char*)mmap(0, 1, PROT_READ | PROT_WRITE, MAP_SHARED, fd_samp_mod, 0);

	sl_open(samp_mod_sl);
	sl_open(samp_raw_sl);
	/*************************************************************************************/
	// Log file initialisation:
	fstream log_file;

	log_file.open("logs/log1.txt", fstream::out | fstream::trunc);
	log_file.close();

	log_file.open("logs/log2.txt", fstream::out | fstream::trunc);
	log_file.close();

	/*************************************************************************************/
	// Logging thread launch:
	thread logging_thread(log_handler);
	/*************************************************************************************/
	// Main processing loop:
	while (true) 
	{
		sl_try(samp_raw_sl);

		memcpy(&sendTime, addrIn, sizeof(struct timeval));
		memcpy(&checkIn, addrIn, sizeof(long));
		memcpy(inSampleBuffer, addrIn + sizeof(struct timeval), inSamples * sizeof(short int));
		if(!checkIn){
			sl_open(samp_raw_sl);
			continue;
		}

		gettimeofday(&receiveTime, NULL);
		log1_time_diff.push((receiveTime.tv_sec - sendTime.tv_sec) * 1000000 + receiveTime.tv_usec - sendTime.tv_usec);
		
		memset(addrIn, 0,  sizeof(long));

		sl_open(samp_raw_sl);



		gettimeofday(&startTime, NULL);
		if(isMod){
			processSamples(PITCH_SEMITONES, inSamples, sframe, overlap, SAMPLE_RATE, inSampleBuffer, outSampleBuffer);			
		}
		gettimeofday(&endTime, NULL);;
		log2_time_diff.push((endTime.tv_sec - startTime.tv_sec) * 1000000 + endTime.tv_usec - startTime.tv_usec);



		sl_try(samp_mod_sl);

		gettimeofday(&postTime, NULL);
		memcpy( addrOut, &postTime, sizeof(struct timeval));

		if(isMod){
			memcpy( addrOut + sizeof(struct timeval), outSampleBuffer, inSamples*sizeof(short int));			
		}else{
			memcpy( addrOut + sizeof(struct timeval), inSampleBuffer, inSamples*sizeof(short int));		
		}
		sl_open(samp_mod_sl);

	}
	/*************************************************************************************/
	// If somehow loop broke, exit with error
	return EXIT_FAILURE;
	/*************************************************************************************/
}

void FFT(float* buffer, long int frame_size, long int direction)
{
	float wr, wi, arg, *p1, *p2, temp;
	float tr, ti, ur, ui, *p1r, *p1i, *p2r, *p2i;
	long int bitm, j;
	
	for (long int i = 2; i < 2*frame_size-2; i += 2) {
		for (bitm = 2, j = 0; bitm < 2*frame_size; bitm <<= 1) {
			if (i & bitm) j++;
			j <<= 1;
		}
		if (i < j) {
			p1 = buffer+i; p2 = buffer+j;
			temp = *p1; *(p1++) = *p2;
			*(p2++) = temp; temp = *p1;
			*p1 = *p2; *p2 = temp;
		}
	}

	int le2;

	for (long int k = 0, le = 2; k < (long int)(log(frame_size)/log(2.)+.5); ++k) {
		le <<= 1;
		le2 = le>>1;
		ur = 1.0;
		ui = 0.0;
		arg = M_PI / (le2>>1);
		wr = cos(arg);
		wi = direction*sin(arg);
		for (j = 0; j < le2; j += 2) {
			p1r = buffer+j; p1i = p1r+1;
			p2r = p1r+le2; p2i = p2r+1;
			for (int i = j; i < 2*frame_size; i += le) {
				tr = *p2r * ur - *p2i * ui;
				ti = *p2r * ui + *p2i * ur;
				*p2r = *p1r - tr; *p2i = *p1i - ti;
				*p1r += tr; *p1i += ti;
				p1r += le; p1i += le;
				p2r += le; p2i += le;
			}
			tr = ur*wr - ui*wi;
			ui = ur*wi + ui*wr;
			ur = tr;
		}
	}

}

void processSamples(long int semitones, long int numSamples, long int frame_size, long int osamp, float sampleRate, short int *indata, short int *outdata)
{
	cout << "modulating\n";
	if(!numSamples)
		return;
	static float inputFIFO[MAX_FRAME_LENGTH];
	static float outputFIFO[MAX_FRAME_LENGTH];
	static float fftWorkspace[2*MAX_FRAME_LENGTH];
	static float lastPhase[MAX_FRAME_LENGTH/2+1];
	static float sumOfPhase[MAX_FRAME_LENGTH/2+1];
	static float outputAccumulator[2*MAX_FRAME_LENGTH];
	static float analisedFreq[MAX_FRAME_LENGTH];
	static float analisedMagn[MAX_FRAME_LENGTH];
	static float synthesisedFreq[MAX_FRAME_LENGTH];
	static float synthesisedMagn[MAX_FRAME_LENGTH];

	static bool init = false;
	static long int rover = 0;

	double magn, phase, tmp, window, real, imag;
	double freqPerBin, expectedFrequency;

	long int qpd, index, inputLatency, stepSize, frame_size_2;

	float psFactor = pow(2.0, semitones/12.0);

	frame_size_2 = frame_size / 2;
	stepSize = frame_size/osamp;
	freqPerBin = sampleRate/(double)frame_size;
	expectedFrequency = 2.*M_PI*(double)stepSize/(double)frame_size;
	inputLatency = frame_size - stepSize;

	if(!rover) rover = inputLatency;

	if (init == false) {
		memset(inputFIFO, 0, MAX_FRAME_LENGTH*sizeof(float));
		memset(outputFIFO, 0, MAX_FRAME_LENGTH*sizeof(float));
		memset(fftWorkspace, 0, 2*MAX_FRAME_LENGTH*sizeof(float));
		memset(lastPhase, 0, (MAX_FRAME_LENGTH/2+1)*sizeof(float));
		memset(sumOfPhase, 0, (MAX_FRAME_LENGTH/2+1)*sizeof(float));
		memset(outputAccumulator, 0, 2*MAX_FRAME_LENGTH*sizeof(float));
		memset(analisedFreq, 0, MAX_FRAME_LENGTH*sizeof(float));
		memset(analisedMagn, 0, MAX_FRAME_LENGTH*sizeof(float));
		init = true;
	}

	for (long int i = 0; i < numSamples; ++i){

		inputFIFO[rover] = indata[i];
		outdata[i] = outputFIFO[rover-inputLatency];
		++rover;

		if (rover >= frame_size) {
			rover = inputLatency;

			for (long int k = 0; k < frame_size; ++k) {
				window = -0.5*cos(2.0*M_PI*(double)k/(double)frame_size)+0.5;
				fftWorkspace[2*k] = inputFIFO[k] * window;
				fftWorkspace[2*k+1] = 0.0;
			}

			// Analise:
			FFT(fftWorkspace, frame_size, -1);

			for (long int k = 0; k <= frame_size_2; ++k) {

				
				real = fftWorkspace[2*k];
				imag = fftWorkspace[2*k+1];

				
				magn = 2.*sqrt(real*real + imag*imag);
				phase = atan2(imag,real);

				
				tmp = phase - lastPhase[k];
				lastPhase[k] = phase;


				tmp -= (double)k*expectedFrequency;

				
				qpd = tmp/M_PI;
				if (qpd >= 0) qpd += qpd&1;
				else qpd -= qpd&1;
				tmp -= M_PI*(double)qpd;

				tmp = osamp*tmp/(2.*M_PI);

				tmp = (double)k*freqPerBin + tmp*freqPerBin;

				analisedMagn[k] = magn;
				analisedFreq[k] = tmp;

			}

			// Process:
			memset(synthesisedMagn, 0, frame_size*sizeof(float));
			memset(synthesisedFreq, 0, frame_size*sizeof(float));
			for (long int k = 0; k <= frame_size_2; ++k) { 
				index = k*psFactor;
				if (index <= frame_size_2) { 
					synthesisedMagn[index] += analisedMagn[k]; 
					synthesisedFreq[index] = analisedFreq[k] * psFactor; 
				} 
			}

			// Synthesise:
			for (long int k = 0; k <= frame_size_2; ++k) {

				magn = synthesisedMagn[k];
				tmp = synthesisedFreq[k];

				tmp -= (double)k*freqPerBin;

				tmp /= freqPerBin;

				tmp = 2.*M_PI*tmp/osamp;

				tmp += (double)k*expectedFrequency;

				sumOfPhase[k] += tmp;
				phase = sumOfPhase[k];

				fftWorkspace[2*k] = magn*cos(phase);
				fftWorkspace[2*k+1] = magn*sin(phase);
			} 

			for (long int k = frame_size+2; k < 2*frame_size; ++k) fftWorkspace[k] = 0.0;

			FFT(fftWorkspace, frame_size, 1);
 
			for(long int k=0; k < frame_size; ++k) {
				window = -0.5*cos(2.0*M_PI*(double)k/(double)frame_size)+0.5;
				outputAccumulator[k] += 2.0*window*fftWorkspace[2*k]/(frame_size_2*osamp);
			}
			for (long int k = 0; k < stepSize; ++k) outputFIFO[k] = outputAccumulator[k];


			memmove(outputAccumulator, outputAccumulator+stepSize, frame_size*sizeof(float));

			for (long int k = 0; k < inputLatency; ++k) inputFIFO[k] = inputFIFO[k+stepSize];
		}
	}

}

void sl_try(char* sl)
{
    cerr << "trying sl ="<< (int)*sl <<" \n";
    while(*sl);
    memset(sl, 1,  sizeof(char));
    cerr << "locked sl ="<< (int)*sl <<" \n";
}

void sl_open(char* sl)
{
    cerr << "opening sl ="<< (int)*sl <<" \n";
    memset(sl, 0,  sizeof(char));
    cerr << "opened sl ="<< (int)*sl <<" \n";
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

			} while(!log1_time_diff.empty());

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

			} while(!log2_time_diff.empty());

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

	sl_open(samp_mod_sl);

	sl_open(samp_raw_sl);


	
	exit(EXIT_SUCCESS);
}
