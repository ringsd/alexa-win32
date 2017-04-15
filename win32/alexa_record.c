/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.

    File: alexa_record.c

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/01/16 10:50:34

*******************************************************************************/

#include <stdio.h>
#include <windows.h>
#include <Mmsystem.h>
#include "sys_log.h"
#include "alexa_platform.h"

#define TAG "record"

struct alexa_record{
    WAVEFORMATEX    waveformat;
    HWAVEIN         hwavein;
    WAVEHDR         wavehdr[5];

    int             sample_bit;

    int             wavehdr_data_size;
    int             wavehdr_count;
    int             wavehdr_r_idx;
    int             wavehdr_w_idx;
};

static void CALLBACK waveInProc(HWAVEIN hwavein, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
    struct alexa_record* record = (struct alexa_record*)dwInstance;
    hwavein = hwavein;
    dwParam1 = dwParam1;
    dwParam2 = dwParam2;

    switch (uMsg)
    {
    case WIM_OPEN:
        sys_log_i(TAG, "WIM_OPEN\n");
        break;
    case WIM_DATA:
        sys_log_i(TAG, "WIM_DATA %d\n", record->wavehdr_w_idx);
        record->wavehdr_w_idx++;
        if (record->wavehdr_w_idx >= record->wavehdr_count)
        {
            record->wavehdr_w_idx = 0;
        }
        break;
    case WIM_CLOSE:
        sys_log_i(TAG, "WIM_CLOSE\n");
        break;
    }
}

struct alexa_record* alexa_record_open(int channels, int sample_rate, int sample_bit)
{
    struct alexa_record* record;
    WAVEFORMATEX*   waveformat;
    HWAVEIN         hwavein;
    WAVEHDR*        wavehdr;

    int             i = 0;
    int             resPrepare;
    DWORD           datasize;

    record = alexa_new( struct alexa_record );
    if (record == NULL)
    {
        goto err;
    }
    record->wavehdr_count = sizeof(record->wavehdr) / sizeof(record->wavehdr[0]);
    //size is 100ms
    datasize = sample_rate * channels * 8 / 8 * 1000 / 1000;
    record->wavehdr_data_size = datasize;

    waveformat = &record->waveformat;

    //chech has wave in device
    if (!waveInGetNumDevs())
    {
        goto err1;
    }

    record->sample_bit = sample_bit;

    //set the record param
    waveformat->wFormatTag = WAVE_FORMAT_PCM;
    waveformat->nChannels = (WORD)channels;
    waveformat->nSamplesPerSec = (DWORD)sample_rate;
    waveformat->nBlockAlign = 1;
    waveformat->wBitsPerSample = 8;
    waveformat->cbSize = 0;
    waveformat->nAvgBytesPerSec = waveformat->nChannels * waveformat->nSamplesPerSec * waveformat->wBitsPerSample / 8;

    // open the wave in device
    int res = waveInOpen(&hwavein, WAVE_MAPPER, waveformat, (DWORD_PTR)waveInProc, (DWORD_PTR)record, CALLBACK_FUNCTION);
    if (res != MMSYSERR_NOERROR)
    {
        goto err2;
    }

    record->hwavein = hwavein;

    for (i = 0; i < record->wavehdr_count; i++)
    {
        wavehdr = &record->wavehdr[i];
        wavehdr->lpData = (char *)GlobalLock(GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, datasize));
        memset(wavehdr->lpData, 0, datasize);
        wavehdr->dwBufferLength = datasize;
        wavehdr->dwBytesRecorded = 0;
        wavehdr->dwUser = 0;
        wavehdr->dwFlags = 0;
        wavehdr->dwLoops = 0;

        //  prepare header
        resPrepare = waveInPrepareHeader(hwavein, wavehdr, sizeof(WAVEHDR));
        if (resPrepare != MMSYSERR_NOERROR)
        {
            goto err2;
        }

        resPrepare = waveInAddBuffer(hwavein, wavehdr, sizeof(WAVEHDR));
        if (resPrepare != MMSYSERR_NOERROR)
        {
            goto err2;
        }
    }

    waveInStart(hwavein);

    return record;

err2:
err1:
    alexa_delete(record);
err:
    return NULL;
}

int alexa_record_read(struct alexa_record* record, char* out, int out_len)
{
    MMTIME mmt;
    int read_len = 0;
    int max_data_size = 0;
    mmt.wType = TIME_BYTES;

    if (record->sample_bit == 8) max_data_size = record->wavehdr_data_size;
    else if(record->sample_bit == 16) max_data_size = record->wavehdr_data_size * 2;

    if (out_len < max_data_size)
    {
        return -1;
    }

    while (record->wavehdr_r_idx != record->wavehdr_w_idx)
    {
        if (out_len >= max_data_size)
        {
            WAVEHDR* wavehdr;
            int read_bytes = 0;

            wavehdr = &(record->wavehdr[record->wavehdr_r_idx]);

            if (record->sample_bit == 8)
            {
                memcpy(out, wavehdr->lpData, wavehdr->dwBytesRecorded);
                read_bytes = wavehdr->dwBytesRecorded;
            }
            else if(record->sample_bit == 16)
            {
                unsigned int i = 0;
                short* data_out = (short*)out;
                unsigned char* in = (unsigned char*)wavehdr->lpData;
                for (i = 0; i < wavehdr->dwBytesRecorded; i++)
                {
                    data_out[i] = (short)((in[i] - 0x80) << 8);
                }
                read_bytes = wavehdr->dwBytesRecorded * 2;
            }

            read_len += read_bytes;
            out_len -= read_bytes;
            out += read_bytes;

            waveInUnprepareHeader(record->hwavein, wavehdr, sizeof(WAVEHDR));

            memset(wavehdr->lpData, 0, wavehdr->dwBufferLength);
            wavehdr->dwBytesRecorded = 0;
            wavehdr->dwUser = 0;
            wavehdr->dwFlags = 0;
            wavehdr->dwLoops = 0;
            waveInPrepareHeader(record->hwavein, wavehdr, sizeof(WAVEHDR));
            waveInAddBuffer(record->hwavein, wavehdr, sizeof(WAVEHDR));

            record->wavehdr_r_idx++;
            if (record->wavehdr_r_idx >= record->wavehdr_count)
            {
                record->wavehdr_r_idx = 0;
            }
        }
        else
        {
            return read_len;
        }
    }
    return read_len;
}


void alexa_record_close(struct alexa_record* record)
{
    HWAVEIN hwavein = record->hwavein;
    int i = 0;

    if (waveInStop(hwavein))
    {
        sys_log_e( TAG, "waveInStop fail.\n" );
    }

    if (waveInReset(hwavein))
    {
        sys_log_e(TAG, "waveInReset fail.\n");
    }

    for (i = 0; i < record->wavehdr_count; i++)
    {
        if (waveInUnprepareHeader(hwavein, &record->wavehdr[i], sizeof(WAVEHDR)))
        {
            sys_log_e(TAG, "waveInUnprepareHeader fail.\n");
        }

        if (GlobalFree(GlobalHandle(record->wavehdr[i].lpData)))
        {
            sys_log_e(TAG, "GlobalFree fail.\n");
        }
    }

    waveInClose(hwavein);

    alexa_delete(record);
    return;
}

static DWORD FCC(LPSTR lpStr)
{
    DWORD Number = lpStr[0] + lpStr[1] * 0x100 + lpStr[2] * 0x10000 + lpStr[3] * 0x1000000;
    return Number;
}

void alexa_record_save_file(struct alexa_record* record, const char* file, const char* data, int len)
{
    DWORD NumToWrite = 0;
    DWORD dwNumber = 0;
    WAVEFORMATEX* waveformat;
    WORD recovery_sample_bit;
    HANDLE FileHandle = CreateFile(file, GENERIC_WRITE,
        FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    dwNumber = FCC("RIFF");
    WriteFile(FileHandle, &dwNumber, 4, &NumToWrite, NULL);
    dwNumber = len + 18 + 20;
    WriteFile(FileHandle, &dwNumber, 4, &NumToWrite, NULL);
    dwNumber = FCC("WAVE");
    WriteFile(FileHandle, &dwNumber, 4, &NumToWrite, NULL);
    dwNumber = FCC("fmt ");
    WriteFile(FileHandle, &dwNumber, 4, &NumToWrite, NULL);
    dwNumber = 18L;
    WriteFile(FileHandle, &dwNumber, 4, &NumToWrite, NULL);

    waveformat = &record->waveformat;
    recovery_sample_bit = waveformat->wBitsPerSample;
    waveformat->wBitsPerSample = (WORD)record->sample_bit;
    waveformat->nAvgBytesPerSec = waveformat->nChannels * waveformat->nSamplesPerSec * waveformat->wBitsPerSample / 8;
    WriteFile(FileHandle, &record->waveformat, sizeof(WAVEFORMATEX), &NumToWrite, NULL);
    waveformat->wBitsPerSample = recovery_sample_bit;
    waveformat->nAvgBytesPerSec = waveformat->nChannels * waveformat->nSamplesPerSec * waveformat->wBitsPerSample / 8;
    dwNumber = FCC("data");
    WriteFile(FileHandle, &dwNumber, 4, &NumToWrite, NULL);
    dwNumber = len;
    WriteFile(FileHandle, &dwNumber, 4, &NumToWrite, NULL);
    WriteFile(FileHandle, data, len, &NumToWrite, NULL);
    SetEndOfFile(FileHandle);
    CloseHandle(FileHandle);
    FileHandle = INVALID_HANDLE_VALUE;
}

/*******************************************************************************
    END OF FILE
*******************************************************************************/
