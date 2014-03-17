/*
    This source code from MPG123/output/win32
    Reprogrammed by rageworx@gmail.com

    ----------------------------------------------------------------------------

    Original description:

    win32: audio output for Windows 32bit

    copyright ?-2013 by the mpg123 project - free software under the terms of the LGPL 2.1
    see COPYING and AUTHORS files in distribution or http://mpg123.org

    initially written (as it seems) by Tony Million
    rewrite of basic functionality for callback-less and properly ringbuffered operation by ravenexp
    Closing buffer playback fixed by David Wohlferd <limegreensocks (*) yahoo dod com>
*/
#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <tchar.h>

using namespace std;

#include "fecodec_win32.h"

#define BUFFER_SIZE     0x10000
#define NUM_BUFFERS     8


struct queue_state
{
    WAVEHDR     buffer_headers[NUM_BUFFERS];
    int         next_buffer;
    HANDLE      play_done_event;
    HWAVEOUT    waveout;
};

////////////////////////////////////////////////////////////////////////////////

static void wait_for_buffer(WAVEHDR* hdr, HANDLE hEvent);
static void drain_win32( FEContext* context );
static void write_final_buffer(struct queue_state *state);

////////////////////////////////////////////////////////////////////////////////

FECodecWin32::FECodecWin32()
{
    _info = new FEInformation();
    if ( _info != NULL )
    {
        _info->version      = 0x00010000;
        _info->name         = "window32 audio output";
        _info->description  = "Audio output for Windows Multimedia(winmm)";
        _info->revision     = "";
        _info->handle       = NULL;
    }
}

int  FECodecWin32::GetFormats( FEContext* context )
{
    return 0;
}

int  FECodecWin32::Open( FEContext* context )
{
    struct
    queue_state*    state = NULL;
    MMRESULT        res;
    WAVEFORMATEX    out_fmt;
    UINT            dev_id;

    if ( context == NULL )
        return -1;

    if( context->rate == 0 )
        return -2;

    /* Allocate queue state struct for this device */
    state = new queue_state();
    if( state == NULL )
        return -3;

    context->userptr = state;

    state->play_done_event = CreateEvent( 0, FALSE, FALSE, 0 );
    if( state->play_done_event == INVALID_HANDLE_VALUE )
        return -1;

    dev_id                  = WAVE_MAPPER;
    out_fmt.wFormatTag      = WAVE_FORMAT_PCM;
    out_fmt.wBitsPerSample  = 16;
    out_fmt.nChannels       = context->channels;
    out_fmt.nSamplesPerSec  = context->rate;
    out_fmt.nBlockAlign     = out_fmt.nChannels * out_fmt.wBitsPerSample / 8;
    out_fmt.nAvgBytesPerSec = out_fmt.nBlockAlign * out_fmt.nSamplesPerSec;
    out_fmt.cbSize          = 0;

    res = waveOutOpen( &state->waveout,
                       dev_id,
                       &out_fmt,
                       (DWORD_PTR)state->play_done_event,
                       0,
                       CALLBACK_EVENT );

    switch( res )
    {
        case MMSYSERR_NOERROR:
            break;

        case MMSYSERR_ALLOCATED:
            _lasterr = "Audio output device is already allocated.";
            return -1;

        case MMSYSERR_NODRIVER:
            _lasterr = "No device driver is present.";
            return -1;

        case MMSYSERR_NOMEM:
            _lasterr = "Unable to allocate or lock memory.";
            return -1;

        case WAVERR_BADFORMAT:
            _lasterr = "Unsupported waveform-audio format.";
            return -1;

        default:
            _lasterr = "Unable to open wave output device.";
            return -1;
    }

    /* Reset event from the "device open" message */
    ResetEvent(state->play_done_event);
    /* Allocate playback buffers */

    for(int cnt=0; cnt<NUM_BUFFERS; cnt++)
    {
        state->buffer_headers[ cnt ].lpData = new char[ BUFFER_SIZE ];

        if( state->buffer_headers[ cnt ].lpData == NULL )
        {
            _lasterr = "Out of memory for playback buffers.";
            return -1;
        }
        else
        {
            /* Tell waveOutPrepareHeader the maximum value of dwBufferLength
            we will ever send */
            state->buffer_headers[ cnt ].dwBufferLength = BUFFER_SIZE;
            state->buffer_headers[ cnt ].dwFlags = 0;

            res = waveOutPrepareHeader(state->waveout, &state->buffer_headers[ cnt ], sizeof(WAVEHDR));
            if( res != MMSYSERR_NOERROR )
            {
                _lasterr = "Can't write to audio output device (prepare).";
                return -1;
            }

            /* set the current size of the buffer to 0 */
            state->buffer_headers[ cnt ].dwBufferLength = 0;

            /* set flags to unprepared - must reset this to WHDR_PREPARED before calling write */
            state->buffer_headers[ cnt ].dwFlags = 0;
        }
    }

    return 0;
}

int  FECodecWin32::Write( FEContext* context , unsigned char* buff, unsigned bufflen )
{
    struct
    queue_state*    state = NULL;
    WAVEHDR*        hdr   = NULL;
    MMRESULT        res;

    unsigned rest_len; /* Input data bytes left for next recursion. */
    unsigned bufill;   /* Bytes we stuff into buffer now. */

    if ( context == NULL )
        return -1;

    if ( context->userptr == NULL )
        return -1;

    if( buff == NULL || bufflen == 0 )
        return 0;

    state = (struct queue_state*)context->userptr;
    hdr = &state->buffer_headers[state->next_buffer];

    wait_for_buffer(hdr, state->play_done_event);

    /* Now see how much we want to stuff in and then stuff it in. */
    bufill = BUFFER_SIZE - hdr->dwBufferLength;
    if( bufflen < bufill )
        bufill = bufflen;

    rest_len = bufflen - bufill;
    memcpy( hdr->lpData + hdr->dwBufferLength, buff, bufill );
    hdr->dwBufferLength += bufill;
    if(hdr->dwBufferLength == BUFFER_SIZE)
    { /* Send the buffer out when it's full. */
        hdr->dwFlags |= WHDR_PREPARED;

        res = waveOutWrite(state->waveout, hdr, sizeof(WAVEHDR));
        if(res != MMSYSERR_NOERROR)
        {
            _lasterr = "Can't write to audio output device.";
            return -1;
        }

        /* Cycle to the next buffer in the ring queue */
        state->next_buffer = (state->next_buffer + 1) % NUM_BUFFERS;
    }
    /* I'd like to propagate error codes or something... but there are no catchable surprises left.
       Anyhow: Here is the recursion that makes ravenexp happy;-) */
    if( rest_len && Write( context, buff + bufill, rest_len ) < 0 ) /* Write the rest. */
        return -1;

    return bufflen;
}

void FECodecWin32::Flush( FEContext* context )
{
    struct
    queue_state*    state = NULL;
    WAVEHDR*        hdr   = NULL;

    if ( context == NULL )
        return;

    if ( context->userptr == NULL )
        return;

    state = (struct queue_state*)context->userptr;

    /* Cancel any buffers in queue.  Ignore errors since we are void and
    can't return them anyway */
    waveOutReset(state->waveout);

    /* Discard any partial buffer */
    hdr = &state->buffer_headers[state->next_buffer];

    /* If WHDR_PREPARED is not set, this is (potentially) a partial buffer */
    if ( !(hdr->dwFlags & WHDR_PREPARED) )
    {
        hdr->dwBufferLength = 0;
    }

    /* Finish processing the buffers */
    drain_win32( context );
}

int  FECodecWin32::Close( FEContext* context )
{
    struct
    queue_state* state = NULL;

    if ( context == NULL )
        return -1;

    if ( context->userptr == NULL )
        return -1;

    state = (struct queue_state*)context->userptr;

    /* wait for all active buffers to complete */
    drain_win32( context );

    CloseHandle(state->play_done_event);

    for(int cnt=0;cnt<NUM_BUFFERS; cnt++)
    {
        state->buffer_headers[cnt].dwFlags |= WHDR_PREPARED;
        waveOutUnprepareHeader(state->waveout, &state->buffer_headers[ cnt ], sizeof(WAVEHDR));
        delete[] state->buffer_headers[ cnt ].lpData;
    }

    waveOutClose(state->waveout);
    delete (struct queue_state*)context->userptr;
    context->userptr = NULL;

    return 0;
}

int  FECodecWin32::Reset( FEContext* context )
{
    return 0;
}

////////////////////////////////////////////////////////////////////////////////

void wait_for_buffer(WAVEHDR* hdr, HANDLE hEvent)
{
    /* At this point there are several possible states:
    1) Empty or partial buffer (unqueued) - dwFlags == 0
    2) Buffer queued or being played - dwFlags == WHDR_PREPARED | WHDR_INQUEUE
    3) Buffer unqueued and finished being played - dwFlags == WHDR_PREPARED | WHDR_DONE
    4) Buffer removed from queue, but not yet marked as done - dwFlags == WHDR_PREPARED
    */

    /* Check buffer header and wait if it's being played. */
    if ( hdr->dwFlags & WHDR_PREPARED )
    {
        while( !(hdr->dwFlags & WHDR_DONE) )
        {
            /*debug1("waiting for buffer %i...", state->next_buffer);*/
            /* Waits for *a* buffer to finish.  May not be the one we
            want, so check again */
            WaitForSingleObject(hEvent, INFINITE);
        }
        hdr->dwFlags = 0;
        hdr->dwBufferLength = 0;
    }
}

void drain_win32( FEContext* context )
{
    struct queue_state* state;

    if ( context == NULL )
        return;

    if ( context->userptr == NULL )
        return;

    state = (struct queue_state*)context->userptr;

    /* output final buffer (if any) */
    write_final_buffer(state);

    /* I _think_ I understood how this should work. -- ThOr */
    int nbc = state->next_buffer;
    for(int cnt=0; cnt<NUM_BUFFERS; cnt++)
    {
        wait_for_buffer(&state->buffer_headers[ nbc ], state->play_done_event);
        nbc = ( nbc + 1 ) % NUM_BUFFERS;
    }
}

/* output final buffer (if any) */
void write_final_buffer( struct queue_state *state )
{
    if ( state == NULL )
        return;

    WAVEHDR* hdr = &state->buffer_headers[state->next_buffer];

    if( ( !(hdr->dwFlags & WHDR_PREPARED) ) &&
        ( hdr->dwBufferLength != 0 ) )
    {
        hdr->dwFlags |= WHDR_PREPARED;
        /* ignore any errors */
        waveOutWrite(state->waveout, hdr, sizeof(WAVEHDR));

        /* Cycle to the next buffer in the ring queue */
        state->next_buffer = (state->next_buffer + 1) % NUM_BUFFERS;
    }
}
