#ifndef EZ_AUDIO_H
#define EZ_AUDIO_H

#include <cstdint>

typedef struct ez_audio_session ez_audio_session;

typedef uint32_t(*ez_audio_render_callback)(void* buffer_data, uint32_t buffer_data_size, uint32_t sample_count, void* user_data);

typedef struct {
    int frequency;
    ez_audio_render_callback render_callback;
    void* user_data;
} ez_audio_params;

#ifdef __cplusplus
extern "C" {
#endif

    void ez_audio_init_params(ez_audio_params* params);

    ez_audio_session* ez_audio_create(ez_audio_params* params);

    int ez_audio_start(ez_audio_session* session);
    void ez_audio_destroy(ez_audio_session* session);

#ifdef __cplusplus
}
#endif
#endif
