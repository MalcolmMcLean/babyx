#include "libc.h"
#include "minimp3.h"

#define BUFFER_COUNT 8

static WAVEFORMATEX wf = {
	1,  // wFormatTag
	0,  // nChannels
	0,  // nSamplesPerSec
	0,  // nAvgBytesPerSec
	4,  // nBlockAlign
	16, // wBitsPerSample
	sizeof(WAVEFORMATEX) // cbSize
};

static const WAVEHDR wh_template = {
	NULL, // lpData
	0, // dwBufferLength
	0, // dwBytesRecorded
	0, // dwUser
	0, // dwFlags
	1, // dwLoops
	NULL, // lpNext
	0 // reserved
};

typedef struct
{
	mp3_decoder_t mp3;
	mp3_info_t info;
	unsigned char *stream_pos;
	int bytes_left;
	int byte_count;
	WAVEHDR wh[BUFFER_COUNT];
    signed short sample_buffer[MP3_MAX_SAMPLES_PER_FRAME * BUFFER_COUNT];


} BBX_SOUNDSTREAM;

typedef struct
{
	int dummy;
} BBX_AUDIO;

static void ShowTag(char *tag, char *data, int N)
{

}

static void out(char *str)
{

}

void CALLBACK AudioCallback(
	HWAVEOUT hwo,
	UINT uMsg,
	DWORD_PTR dwInstance,
	DWORD dwParam1,
	DWORD dwParam2
	) 
    {
	
	LPWAVEHDR wh = (LPWAVEHDR)dwParam1;
	BBX_SOUNDSTREAM *stream = (BBX_SOUNDSTREAM *) dwInstance;
	if (!wh) return;
	if (stream->byte_count) 
	{
		stream->stream_pos += stream->byte_count;
		stream->bytes_left -= stream->byte_count;
		waveOutUnprepareHeader(hwo, wh, sizeof(WAVEHDR));
		waveOutPrepareHeader(hwo, wh, sizeof(WAVEHDR));
		waveOutWrite(hwo, wh, sizeof(WAVEHDR));
	}
	else
	{

	}
	stream->byte_count = mp3_decode(stream->mp3, stream->stream_pos, 
		stream->bytes_left, (signed short *)wh->lpData, &stream->info);

}


void *playmp3(unsigned char *mp3_stream, int stream_size)
{
	int i;
	BBX_SOUNDSTREAM *stream;
	unsigned char *inptr;
	unsigned char *stream_pos;
	mp3_decoder_t mp3;
	HWAVEOUT hwo;

	stream = malloc(sizeof(BBX_SOUNDSTREAM));
	stream_pos = mp3_stream;
	stream->bytes_left = stream_size - 128;

	// check for a ID3 tag
	inptr = stream_pos + stream->bytes_left;
	if (((*(unsigned long *)inptr) & 0xFFFFFF) == 0x474154)
	{
		ShowTag("\nTitle: ", inptr + 3, 30);
		ShowTag("\nArtist: ", inptr + 33, 30);
		ShowTag("\nAlbum: ", inptr + 63, 30);
		ShowTag("\nYear: ", inptr + 93, 4);
		ShowTag("\nComment: ", inptr + 97, 30);
	}

	// set up minimp3 and decode the first frame
	mp3 = mp3_create();
	stream->mp3 = mp3;
	stream->stream_pos = stream_pos;
	stream->byte_count = mp3_decode(mp3, stream_pos, 
		stream->bytes_left, stream->sample_buffer, &stream->info);
	if (!stream->byte_count)
	{
		out("\nError: not a valid MP2 audio file!\n");
		return 0;
	}

	// set up wave output
	wf.nSamplesPerSec = stream->info.sample_rate;
	wf.nChannels = stream->info.channels;
	if (waveOutOpen(&hwo, WAVE_MAPPER, &wf, (INT_PTR)AudioCallback, (INT_PTR)stream, CALLBACK_FUNCTION)
		!= MMSYSERR_NOERROR)
	{
		out("\nError: cannot open wave output!\n");
		return 0;
	}

	// allocate buffers
	//out("\n\nPress Ctrl+C or close the console window to stop playback.\n");
	inptr = (char*)stream->sample_buffer;
	for (i = 0; i < BUFFER_COUNT; ++i)
	{
		stream->wh[i] = wh_template;
		stream->wh[i].lpData = inptr;
		stream->wh[i].dwBufferLength = stream->info.audio_bytes;
		AudioCallback(hwo, 0, stream, (DWORD)&stream->wh[i], 0);
		inptr += MP3_MAX_SAMPLES_PER_FRAME * 2;
	}

	return stream;
}

