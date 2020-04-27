#include "ezaudio/ezaudio.h"

#include <initguid.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <Functiondiscoverykeys_devpkey.h>  // w.t.f.
#include <ksmedia.h>

#include <cstring>
#include <thread>
#include <mutex>
#include <atomic>

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);

struct ez_audio_session {
    ez_audio_params params;

    IMMDeviceEnumerator* deviceEnumerator = nullptr;
    IAudioClient* audioClient = nullptr;
    IAudioRenderClient* renderClient = nullptr;
    IMMDevice* defaultDevice = nullptr;
    HANDLE renderEvent = 0;
    WAVEFORMATEX* waveformat = nullptr;
    UINT32 bufferFrameCount = 0;  /* this is in sample frames, not samples, not bytes. */

    bool processAudio = false;
    std::thread audioThread;
    std::mutex mutex;
};

ez_audio_session* ez_audio_create(ez_audio_params* params)
{
    auto session = new ez_audio_session();
    session->processAudio = false;

    if (params == nullptr)
        return nullptr;

    session->params = *params;

    return session;
}

namespace {

bool is_supported(ez_audio_session* session) {
    if (session->waveformat->wBitsPerSample != 32 || session->waveformat->nChannels != 2)
        return false;

    bool formatSupported = false;
    if (session->waveformat->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        const WAVEFORMATEXTENSIBLE* ext = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(session->waveformat);
        formatSupported = std::memcmp(&ext->SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, sizeof(GUID)) == 0 || std::memcmp(&ext->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM, sizeof(GUID)) == 0;
    }
    else {
        formatSupported = session->waveformat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT || session->waveformat->wFormatTag == WAVE_FORMAT_PCM;
    }

    return formatSupported;
}

const int frameSizeInBytes = 8;

void audio_thread(ez_audio_session* session)
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    session->processAudio = true;

    while (true) {
        BYTE* bufferData = nullptr;
        WaitForSingleObjectEx(session->renderEvent, 200, false);
        std::lock_guard<std::mutex>(session->mutex);
        if (!session->processAudio)
            break;

        UINT32 numFramesPadding = 0;
        session->audioClient->GetCurrentPadding(&numFramesPadding);

        UINT32 numFramesAvailable = session->bufferFrameCount - numFramesPadding;
        UINT32 dataSize = numFramesAvailable * frameSizeInBytes;

        session->renderClient->GetBuffer(numFramesAvailable, &bufferData);
        session->params.render_callback(bufferData, dataSize, numFramesAvailable, session->params.user_data);
        session->renderClient->ReleaseBuffer(numFramesAvailable, 0);
    }

    CoUninitialize();
}

}

 int ez_audio_start(ez_audio_session* session)
{
     CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

     HRESULT ret = S_OK;

     ret = CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL, IID_IMMDeviceEnumerator, reinterpret_cast<void**>(&session->deviceEnumerator));
     ret = session->deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &session->defaultDevice);

     ret = session->defaultDevice->Activate(IID_IAudioClient, CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&session->audioClient));

     ret = session->audioClient->GetMixFormat(&session->waveformat);

     if (!is_supported(session))
         return 1;

     DWORD streamflags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
     if (session->waveformat->nSamplesPerSec != session->params.frequency) {
         streamflags |= AUDCLNT_STREAMFLAGS_RATEADJUST;
         session->waveformat->nSamplesPerSec = session->params.frequency;
         session->waveformat->nAvgBytesPerSec = session->waveformat->nSamplesPerSec * session->waveformat->nChannels * (session->waveformat->wBitsPerSample / 8);
     }

     ret = session->audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, streamflags, 0, 0, session->waveformat, nullptr);
     session->renderEvent = CreateEventW(NULL, 0, 0, NULL);
     ret = session->audioClient->SetEventHandle(session->renderEvent);

     ret = session->audioClient->GetBufferSize(&session->bufferFrameCount);
     ret = session->audioClient->GetService(IID_IAudioRenderClient, reinterpret_cast<void**>(&session->renderClient));
     ret = session->audioClient->Start();

     session->audioThread = std::thread(audio_thread, session);

     SetThreadPriority(session->audioThread.native_handle(), THREAD_PRIORITY_HIGHEST);

     return 0;
 }

 void ez_audio_destroy(ez_audio_session* session)
 {
     session->processAudio = false;

     {
         std::lock_guard<std::mutex>(session->mutex);
         if (session->audioClient) session->audioClient->Stop();
     }
     
     if (session->audioThread.joinable())
         session->audioThread.join();

     if (session->waveformat) CoTaskMemFree(session->waveformat);
     if (session->deviceEnumerator) session->deviceEnumerator->Release();
     if (session->defaultDevice) session->defaultDevice->Release();
     if (session->audioClient) session->audioClient->Release();
     if (session->renderClient) session->renderClient->Release();
     if (session->renderEvent) CloseHandle(session->renderEvent);

     delete session;

     CoUninitialize();

 }
