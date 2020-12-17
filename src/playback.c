#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <semaphore.h>
#include "portaudio.h"

#define SAMPLE_RATE  (44100)
#define FRAMES_PER_BUFFER (1024)
#define NUM_SECONDS     (2)
#define NUM_CHANNELS    (1)

#define PA_SAMPLE_TYPE  paInt16
typedef short SAMPLE;
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%d"

const char *semName = "semAC";

int main(void);
int main(void)
{
    PaStreamParameters inputParam, outputParam;
    PaStream *audioStream;
    PaError exception;
    SAMPLE *samplesRecorded;

    sem_t* sem_id;

    sem_id = sem_open(semName, O_CREAT | O_RDWR, 0755, 1);
    sem_init(sem_id,1,1);

    int i;
    int amountOfFrames;
    int amountOfSamples;
    int amountOfBytes;

    amountOfFrames = NUM_SECONDS * SAMPLE_RATE; 
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

        printf("playback loop\n");

        samplesRecorded = (SAMPLE *) malloc( amountOfBytes );

        if( samplesRecorded == NULL )
        {
            printf("Could not allocate playback array.\n");
            exit(1);
        }

        for( i=0; i<amountOfSamples; i++ ) samplesRecorded[i] = 0;

        if(sem_wait(sem_id) < 0)
            printf("[sem_wait] failed.\n");

        FILE  *fid;
        fid = fopen("recorded.raw", "rb");
        if( fid != NULL )
        {        
            int error = fread( samplesRecorded, NUM_CHANNELS * sizeof(SAMPLE), amountOfFrames, fid );
            fclose( fid );

            printf("Read data to 'recorded.raw'.\n");
        }

        if (sem_post(sem_id) < 0)
            printf("[sem_post] failed.\n");

        printf("Begin playback.\n");

        if( audioStream )
        {
            exception = Pa_StartStream( audioStream );
            if( exception != paNoError ) 
                goto error;

            exception = Pa_WriteStream( audioStream, samplesRecorded, amountOfFrames );
            if( exception != paNoError ) 
                goto error;

            exception = Pa_CloseStream( audioStream );
            if( exception != paNoError ) 
                goto error;
            printf("Done.\n");
        }

        free( samplesRecorded );

    sem_unlink(semName);

    Pa_Terminate();
    return 0;

error:
    Pa_Terminate();
    printf("An error occured while using the audio playback stream. Terminating...\n" );
    printf("Error number: %d\n", exception );
    printf("Error message: %s\n", Pa_GetErrorText( exception ) );
    return -1;
}