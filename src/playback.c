#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include "portaudio.h"
#include <sys/time.h>

#define SAMPLE_RATE  (20000)
#define FRAMES_PER_BUFFER (1024)
#define NUM_MILISECONDS     (20)
#define NUM_CHANNELS    (1)

#define PA_SAMPLE_TYPE  paInt16
typedef short SAMPLE;
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%d"

const char *semName = "/samp_mod";
sem_t* log3_semaphore;
sem_t* sem_id;
FILE  *fid;

int main(void);
void SIGTERM_handler();

int main(void)
{
    PaStreamParameters inputParam, outputParam;
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

    sem_id = sem_open(semName, O_CREAT | O_RDWR, 0755, 1);
    log3_semaphore = sem_open("/log3", O_CREAT, O_RDWR, 1);

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
        int error = 0;

        if( samplesRecorded == NULL )
        {
            printf("Could not allocate playback array.\n");
            exit(1);
        }

        for( i=0; i<amountOfSamples; i++ ) samplesRecorded[i] = 0;

        if(sem_wait(sem_id) < 0)
            printf("[sem_wait] failed.\n");

        fid = fopen("samp/mod.raw", "rb");
        if( fid != NULL )
        {        
            error = fread(&sendTime, sizeof(struct timeval),1,fid);
            error = fread( samplesRecorded, NUM_CHANNELS * sizeof(SAMPLE), amountOfFrames, fid );
            if(error)
                {
                    fclose(fid);
                    fid = fopen("samp/mod.raw", "w");
                    printf("Read data from 'samp/mod.raw'.\n");
                }
            fclose( fid );
        }

        if (sem_post(sem_id) < 0)
            printf("[sem_post] failed.\n");

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
        
    sem_unlink(semName);

    Pa_Terminate();
    return 0;

error:
    Pa_Terminate();
    sem_close(sem_id);
    sem_unlink(semName);
    free( samplesRecorded );
    printf("An error occured while using the audio playback stream. Terminating...\n" );
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