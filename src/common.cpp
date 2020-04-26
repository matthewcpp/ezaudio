#include "ezaudio/ez_audio.h"

void ez_audio_init_params(tiny_audio_params* params)
{
    params->frequency = 44100;
    params->render_callback = nullptr;
    params->user_data = nullptr;
}
