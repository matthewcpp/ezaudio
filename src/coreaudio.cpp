#include "ezaudio/ezaudio.h"

#include "AudioToolbox/AudioToolbox.h"

#include <cstdint>
#include <cstring>

constexpr int audioQueueBufferCount = 5;
constexpr int audioBufferSampleCount = 4096;

struct ez_audio_session
{
    ez_audio_params params;
    AudioQueueRef queue = nullptr;
};

ez_audio_session* ez_audio_create(ez_audio_params* params)
{
    if (!params) return nullptr;
    
    auto session = new ez_audio_session();
    session->params = *params;
    
    return session;
}

void coreaudio_callback(void* user_data, AudioQueueRef queue, AudioQueueBufferRef buffer)
{
    auto session = reinterpret_cast<ez_audio_session*>(user_data);
    
    uint32_t bytesWritten = session->params.render_callback(buffer->mAudioData, buffer->mAudioDataBytesCapacity, buffer->mAudioDataBytesCapacity / 8, session->params.user_data);
    
    buffer->mAudioDataByteSize = bytesWritten;
    AudioQueueEnqueueBuffer( queue, buffer, 0, 0 );
}

int ez_audio_start(ez_audio_session* session)
{
    AudioStreamBasicDescription description = { 0 };
    description.mBitsPerChannel = 32;
    description.mBytesPerFrame = 8; // 2 floats (1 for each channel)
    description.mBytesPerPacket = 8; // 1 frame per packet
    description.mChannelsPerFrame = 2;
    description.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    description.mFormatID = kAudioFormatLinearPCM;
    description.mFramesPerPacket = 1;
    description.mSampleRate = session->params.frequency;
    
    OSStatus startStatus = AudioQueueNewOutput(&description, coreaudio_callback, session, nullptr, nullptr, 0, &session->queue);
    if (startStatus) {
        return startStatus;
    }
    
    for (int i = 0; i < audioQueueBufferCount; i++) {
        AudioQueueBufferRef buffer = nullptr;
        startStatus = AudioQueueAllocateBuffer(session->queue, audioBufferSampleCount, &buffer);
        if (startStatus)
            return startStatus;
        
        std::memset(buffer->mAudioData, 0, buffer->mAudioDataBytesCapacity);
        buffer->mAudioDataByteSize = buffer->mAudioDataBytesCapacity;
        startStatus = AudioQueueEnqueueBuffer(session->queue, buffer, 0, 0);
        if (startStatus)
            return startStatus;
    }
    
    startStatus = AudioQueueSetParameter (session->queue, kAudioQueueParam_Volume, 1.0);
    startStatus = AudioQueueStart(session->queue, nullptr);
    
    return startStatus;
}

void ez_audio_destroy(ez_audio_session* session)
{
    if (session->queue)
        return;

    AudioQueueStop(session->queue, true);
    AudioQueueDispose(session->queue, true);
    
    delete session;
}
