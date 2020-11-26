
#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>
int main() 
  {
  long loopcount;
  int errcheck;
  int size;
  snd_pcm_t *pcmhandle;
  snd_pcm_hw_params_t *parameters;
  unsigned int samplerate;
  int ifequal;
  snd_pcm_uframes_t frameamount;
  char *periodbuffer;

  errcheck = snd_pcm_open(&pcmhandle, "default",
                    SND_PCM_STREAM_PLAYBACK, 0);
  if (errcheck < 0) 
  {
    fprintf(stderr,
            "unable to open pcm device: %s\n",
            snd_strerror(errcheck));
    exit(1);
  }

  snd_pcm_hw_params_alloca(&parameters);

  snd_pcm_hw_params_any(pcmhandle, parameters);

  snd_pcm_hw_params_set_access(pcmhandle, parameters,
                      SND_PCM_ACCESS_RW_INTERLEAVED);

  snd_pcm_hw_params_set_format(pcmhandle, parameters,
                              SND_PCM_FORMAT_S16_LE);

  snd_pcm_hw_params_set_channels(pcmhandle, parameters, 2);

  samplerate = 48000;
  snd_pcm_hw_params_set_rate_near(pcmhandle, parameters,
                                  &samplerate, &ifequal);

  frameamount = 32;
  snd_pcm_hw_params_set_period_size_near(pcmhandle,
                              parameters, &frameamount, &ifequal);

  errcheck = snd_pcm_hw_params(pcmhandle, parameters);
  if (errcheck < 0) 
  {
    fprintf(stderr,
            "unable to set hw parameters: %s\n",
            snd_strerror(errcheck));
    exit(1);
  }

  snd_pcm_hw_params_get_period_size(parameters, &frameamount,
                                    &ifequal);
  size = frameamount * 4;
  periodbuffer = (char *) malloc(size);

  snd_pcm_hw_params_get_period_time(parameters,
                                    &samplerate, &ifequal);
  loopcount = 5000000 / samplerate;

  while (loopcount > 0) 
  {
    loopcount--;
    errcheck = read(0, periodbuffer, size);
    if (errcheck == 0) 
    {
      fprintf(stderr, "end of file on input\n");
      break;
    } 
    else if (errcheck != size) 
    {
      fprintf(stderr,
              "short read: read %d bytes\n", errcheck);
    }
    errcheck = snd_pcm_writei(pcmhandle, periodbuffer, frameamount);

    if (errcheck == -EPIPE) 
    {
      fprintf(stderr, "underrun occurred\n");
      snd_pcm_prepare(pcmhandle);
    } 
    else if (errcheck < 0) 
    {
      fprintf(stderr,
              "error from writei: %s\n",
              snd_strerror(errcheck));
    }  
    else if (errcheck != (int)frameamount) 
    {
      fprintf(stderr,
              "short write, write %d frames\n", errcheck);
    }
  }

  snd_pcm_drain(pcmhandle);
  snd_pcm_close(pcmhandle);
  free(periodbuffer);

  return 0;
}