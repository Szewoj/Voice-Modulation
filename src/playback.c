#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "portaudio.h"
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>

#define SAMPLE_RATE  (20000)
#define FRAMES_PER_BUFFER (1024)
#define NUM_MILISECONDS     (20)
#define NUM_CHANNELS    (1)

#define PA_SAMPLE_TYPE  paInt16
typedef short SAMPLE;
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%d"


const char* shmName = "/mod";
char* addr;
int fd;

sem_t* log3_semaphore;
FILE  *fid;

const char *slName = "/samp_mod";
pthread_spinlock_t* sl;

int fdsl;

int main(void);
void SIGTERM_handler();

void sl_try(char* sl)
{
    fprintf(stderr,"trying, sl = %d\n", *sl);
    while(*sl);
    memset(sl, 1,  sizeof(char));
    fprintf(stderr,"locked, sl = %d\n", *sl);
}

void sl_open(char* sl)
{
    fprintf(stderr,"openieng, sl = %d\n", *sl);
    memset(sl, 0,  sizeof(char));
    fprintf(stderr,"opened, sl = %d\n", *sl);
}

int main(void)
{
    fprintf(stderr,"running playback\n");
    PaStreamParameters outputParam;
    PaStream *audioStream;
    PaError exception;
    SAMPLE *samplesRecorded;

    struct timeval sendTime, receiveTime;

    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = SIGTERM_handler;
    sigaction(SIGINT, &action, NULL);

    struct sigaction action2;
    memset(&action2, 0, sizeof(struct sigaction));
    action2.sa_handler = SIGTERM_handler;
    sigaction(SIGINT, &action2, NULL);

    fid = fopen("logs/log3.txt", "w");
    fclose(fid);

    log3_semaphore = sem_open("/log3", O_CREAT, O_RDWR, 1);

    fd = shm_open(shmName, O_CREAT | O_RDWR, 0666);
    ftruncate(fd, 2048);
    addr = mmap(NULL, 2048, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);



    fdsl = shm_open(slName, O_CREAT | O_RDWR, 0666);
    ftruncate(fdsl, sizeof(pthread_spinlock_t));
    sl = (pthread_spinlock_t*) mmap(NULL, sizeof(pthread_spinlock_t), PROT_READ | PROT_WRITE, MAP_SHARED, fdsl, 0);


    int i;
    float amountOfFrames;
    int amountOfSamples;
    int amountOfBytes;

    amountOfFrames = NUM_MILISECONDS * SAMPLE_RATE/1000; 
    amountOfSamples = amountOfFrames * NUM_CHANNELS;
    amountOfBytes = amountOfSamples * sizeof(SAMPLE);

    exception = Pa_Initialize();

    if( exception != paNoError ) 
        goto error;

    outputParam.device = Pa_GetDefaultOutputDevice(); 
    if (outputParam.device == paNoDevice) 
    {
        printf("Error: No default output device.\n");
        goto error;
    }

    outputParam.channelCount = NUM_CHANNELS;
    outputParam.sampleFormat =  PA_SAMPLE_TYPE;
    outputParam.suggestedLatency = Pa_GetDeviceInfo( outputParam.device )->defaultLowOutputLatency;
    outputParam.hostApiSpecificStreamInfo = NULL;

    
    while(1)
    {
        //printf("playback loop\n");

        samplesRecorded = (SAMPLE *) malloc( amountOfBytes );
        long error = 0;

        if( samplesRecorded == NULL )
        {
            printf("Could not allocate playback array.\n");
            exit(1);
        }

        for( i=0; i<amountOfSamples; i++ ) samplesRecorded[i] = 0;


        pthread_spin_lock(sl);


        memcpy(&sendTime, addr, sizeof(struct timeval));
        memcpy(&error, addr, sizeof(long));
        memcpy(samplesRecorded, addr + sizeof(struct timeval), NUM_CHANNELS * sizeof(SAMPLE) * amountOfFrames);

        if(error){
            memset(addr, 0,  sizeof(long));
            printf("Read data from 'samp/mod.raw'.\n");
            //fprintf(stderr, "Read data from 'samp/mod.raw'.\n");
            }


        pthread_spin_unlock(sl);


        if(error)
        {
        gettimeofday(&receiveTime, NULL);

        if(sem_wait(log3_semaphore) < 0)
            printf("[sem_wait] failed.\n");

        fid = fopen("logs/log3.txt", "a");

        if( fid != NULL )
        {       
            unsigned int time = (receiveTime.tv_sec - sendTime.tv_sec) * 1000000 + (receiveTime.tv_usec - sendTime.tv_usec);
            fprintf(fid, "%d\n", time);
            //fwrite( &time, sizeof(unsigned int), 1, fid);
            fclose( fid );
            printf("write data to 'logs/log3.txt'.\n");
            //fprintf(stderr,"write data to 'logs/log3.txt'.\n");
        }

        if (sem_post(log3_semaphore) < 0)
            printf("[sem_post] failed.\n");
        //printf("Begin playback.\n");

        exception = Pa_OpenStream(
              &audioStream,
              NULL, 
              &outputParam,
              SAMPLE_RATE,
              FRAMES_PER_BUFFER,
              paClipOff,      
              NULL, 
              NULL ); 

        if( exception != paNoError ) 
            goto error;

        if( audioStream )
        {
            exception = Pa_StartStream( audioStream );
            if( exception != paNoError ) 
                goto error;

            exception = Pa_WriteStream( audioStream, samplesRecorded, amountOfFrames );
            if( exception != paNoError ) 
                goto error;

            exception = Pa_StopStream( audioStream );
            if( exception != paNoError ) 
                goto error;
            //printf("Done.\n");

            exception = Pa_CloseStream( audioStream );
            if( exception != paNoError ) 
                goto error;
        }
        }
        free( samplesRecorded );
    }

    Pa_Terminate();
    return 0;

error:
    Pa_Terminate();

    free( samplesRecorded );
    printf("An error occured while using the audio playback stream. Terminating...\n" );
    printf("Error number: %d\n", exception );
    printf("Error message: %s\n", Pa_GetErrorText( exception ) );
    return -1;
}

void SIGTERM_handler()
{

    Pa_Terminate();
    sl_open(sl);
    printf("Received kill signal. Terminating...\n" );
    exit(EXIT_SUCCESS);
}