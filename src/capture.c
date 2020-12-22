#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <semaphore.h>
#include <sys/mman.h>
#include "portaudio.h"
#include <sys/time.h>
#include <sys/stat.h>

#define SAMPLE_RATE  (20000)
#define FRAMES_PER_BUFFER (1024)
#define NUM_MILISECONDS     (20)
#define NUM_CHANNELS    (1)

#define PA_SAMPLE_TYPE  paInt16
typedef short SAMPLE;
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%d"

const char *semName = "/samp_raw";
sem_t* sem_id;
const char* shmName = "/raw";
char* addr;
int fd;

int main(void);
void SIGTERM_handler();

int main(void)
{
    PaStreamParameters inputParam, outputParam;
    PaStream *audioStream;
    PaError exception;
    SAMPLE *samplesRecorded;
    struct timeval start;
    
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = SIGTERM_handler;
    sigaction(SIGTERM, &action, NULL);

    sem_id = sem_open(semName, O_CREAT | O_RDWR, 0755, 1);
    fd = shm_open(shmName, O_CREAT, O_RDWR);
    addr = mmap(NULL, 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

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

    inputParam.device = Pa_GetDefaultInputDevice(); 
    if (inputParam.device == paNoDevice) 
    {
        printf("Error: No default input device.\n");
        goto error;
    }

    inputParam.channelCount = NUM_CHANNELS;
    inputParam.sampleFormat = PA_SAMPLE_TYPE;
    inputParam.suggestedLatency = Pa_GetDeviceInfo( inputParam.device )->defaultLowInputLatency;
    inputParam.hostApiSpecificStreamInfo = NULL;


    while(1)
    {
        //printf("capture loop\n");

        samplesRecorded = (SAMPLE *) malloc( amountOfBytes );

        if( samplesRecorded == NULL )
        {
            printf("Could not allocate capture array.\n");
            exit(1);
        }

        for( i=0; i<amountOfSamples; i++ ) samplesRecorded[i] = 0;

        exception = Pa_OpenStream(
              &audioStream,
              &inputParam,
              NULL,          
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

            //printf("Now recording!\n"); 

            exception = Pa_ReadStream( audioStream, samplesRecorded, amountOfFrames );
            if( exception != paNoError ) 
                goto error;
            
            exception = Pa_StopStream( audioStream );
            if( exception != paNoError ) 
                goto error;

            exception = Pa_CloseStream( audioStream );
            if( exception != paNoError ) 
                goto error;
        }

        if(sem_wait(sem_id) < 0)
            printf("[sem_wait] failed.\n");

        gettimeofday(&start, NULL);
        memcpy( addr, &start, sizeof(struct timeval));
        memcpy( addr + sizeof(struct timeval), samplesRecorded, NUM_CHANNELS * sizeof(SAMPLE)* amountOfFrames);

        printf("Wrote data to 'samp/raw.raw'.\n");

        if (sem_post(sem_id) < 0)
            printf("[sem_post] failed.\n");

        free( samplesRecorded );
    }
    
    sem_unlink(semName);

    Pa_Terminate();
    return 0;

error:
    Pa_Terminate();
    sem_unlink(semName);
    free( samplesRecorded );
    printf("An error occured while using the audio capture stream. Terminating...\n" );
    printf("Error number: %d\n", exception );
    printf("Error message: %s\n", Pa_GetErrorText( exception ) );
    return -1;
}
void SIGTERM_handler()
{

    sem_close(sem_id);
    sem_unlink(semName);
    Pa_Terminate();

    printf("Received kill signal. Terminating...\n" );
    exit(EXIT_SUCCESS);
}