#define UNICODE
#define _UNICODE

#include <audioclient.h>
#include <comdef.h>
#include <comip.h>
#include <fcntl.h>
#include <io.h>
#include <mmdeviceapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#define REFTIMES_PER_SEC 10000000
#define REFTIMES_PER_MILLISEC 10000

class CCoInitialize {
    HRESULT m_hr;

  public:
    CCoInitialize() : m_hr(CoInitialize(NULL)) {}
    ~CCoInitialize() {
        if (SUCCEEDED(m_hr))
            CoUninitialize();
    }
    operator HRESULT() const { return m_hr; }
};

_COM_SMARTPTR_TYPEDEF(IUnknown, __uuidof(IUnknown));
_COM_SMARTPTR_TYPEDEF(IMMDeviceEnumerator, __uuidof(IMMDeviceEnumerator));
_COM_SMARTPTR_TYPEDEF(IMMDevice, __uuidof(IMMDevice));
_COM_SMARTPTR_TYPEDEF(IAudioClient, __uuidof(IAudioClient));
_COM_SMARTPTR_TYPEDEF(IAudioCaptureClient, __uuidof(IAudioCaptureClient));

class EnsureCaptureStop {
    IAudioClientPtr m_client;

  public:
    EnsureCaptureStop(IAudioClientPtr pClient) : m_client(pClient) {}
    ~EnsureCaptureStop() { m_client->Stop(); }
};

class EnsureFree {
    void *m_memory;

  public:
    EnsureFree(void *memory) : m_memory(memory) {}
    ~EnsureFree() { free(m_memory); }
};

struct {
    HRESULT hr;
    LPWSTR error;
} error_table[] = {
    {AUDCLNT_E_UNSUPPORTED_FORMAT, L"Requested sound format unsupported"},
};

#define ensure(hr) ensure_(__FILE__, __LINE__, hr)
void ensure_(const char *file, int line, HRESULT hr) {
    if (FAILED(hr)) {
        _com_error err(hr);
        LPCWSTR msg = err.ErrorMessage();

        for (int i = 0; i < sizeof error_table / sizeof error_table[0]; ++i) {
            if (error_table[i].hr == hr) {
                msg = error_table[i].error;
            }
        }

        fwprintf(stderr, L"Error at %S:%d (0x%08x): %s\n", file, line, hr, msg);
        exit(1);
    }
}

[[noreturn]] void usage(char *argv0) {
    fprintf(stderr, "Usage: %s <channels> <sample rate> <bits per sample>\n", argv0);
    exit(2);
}

int parse_int_arg(int argc, char *argv[], int argn) {
    if (argn < argc) {
        int result = atoi(argv[argn]);
        if (result)
            return result;
    }
    usage(argv[0]);
}

int main(int argc, char *argv[]) {
    int channels = parse_int_arg(argc, argv, 1);
    int rate = parse_int_arg(argc, argv, 2);
    int bits = parse_int_arg(argc, argv, 3);

    if (_setmode(_fileno(stdout), _O_BINARY) == -1) {
        perror("_setmode failed");
        exit(1);
    }

    CCoInitialize comInit;
    ensure(comInit);

    IMMDeviceEnumeratorPtr pEnumerator;
    ensure(pEnumerator.CreateInstance(__uuidof(MMDeviceEnumerator)));

    IMMDevicePtr pDevice;
    ensure(pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice));

    IAudioClientPtr pClient;
    ensure(pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void **)&pClient));

    WAVEFORMATEX wfx;
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = (WORD)channels;
    wfx.nSamplesPerSec = (DWORD)rate;
    wfx.wBitsPerSample = (WORD)bits;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    ensure(pClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK,
                               16 * REFTIMES_PER_MILLISEC, 0, &wfx, nullptr));

    UINT32 bufferFrameCount;
    ensure(pClient->GetBufferSize(&bufferFrameCount));

    IAudioCaptureClientPtr pCapture;
    ensure(pClient->GetService(__uuidof(IAudioCaptureClient), (void **)&pCapture));

    DWORD dwDelay = (DWORD)(((double)REFTIMES_PER_SEC * bufferFrameCount / wfx.nSamplesPerSec) /
                            REFTIMES_PER_MILLISEC / 2);

    LPBYTE pSilence = (LPBYTE)malloc(bufferFrameCount * wfx.nBlockAlign);
    EnsureFree freeSilence(pSilence);
    ZeroMemory(pSilence, bufferFrameCount * wfx.nBlockAlign);

    ensure(pClient->Start());
    EnsureCaptureStop autoStop(pClient);

    for (;;) {
        Sleep(dwDelay);

        UINT32 packetLength;
        ensure(pCapture->GetNextPacketSize(&packetLength));

        while (packetLength) {
            LPBYTE pData;
            UINT32 numFramesAvailable;
            DWORD flags;

            ensure(pCapture->GetBuffer(&pData, &numFramesAvailable, &flags, nullptr, nullptr));

            if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
                pData = pSilence;

            _write(_fileno(stdout), pData, wfx.nBlockAlign * numFramesAvailable);
            ensure(pCapture->ReleaseBuffer(numFramesAvailable));
            ensure(pCapture->GetNextPacketSize(&packetLength));
        }
    }
}
