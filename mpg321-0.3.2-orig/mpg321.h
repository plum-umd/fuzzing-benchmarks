/*
    mpg321 - a fully free clone of mpg123.
    Copyright (C) 2001 Joe Drew
    Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011, 2012 Nanakos Chrysostomos
    
    Originally based heavily upon:
    plaympeg - Sample MPEG player using the SMPEG library
    Copyright (C) 1999 Loki Entertainment Software
    
    Also uses some code from
    mad - MPEG audio decoder
    Copyright (C) 2000-2001 Robert Leslie
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef _MPG321_H_
#define _MPG321_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <stdio.h>
#include <limits.h>
#include <ao/ao.h>
#include <mad.h>
#include <semaphore.h>


#define FAKEVERSION "0.3.2-1"
#define VERSIONDATE "2012/03/25"

#ifndef PATH_MAX
#define PATH_MAX	4096
#endif

/* playlist structure */
typedef struct pl
{
    char **files;
    int numfiles;
    int files_size;
    int random_play;
    char remote_file[PATH_MAX];
} playlist;

/* Private buffer for passing around with libmad */
typedef struct
{
    /* The buffer of raw mpeg data for libmad to decode */
    void * buf;

    /* Cached data: pointers to the dividing points of frames
       in buf, and the playing time at each of those frames */
    void **frames;
    mad_timer_t *times;

    /* fd is the file descriptor if over the network, or -1 if
       using mmap()ed files */
    int fd;

    /* length of the current stream, corrected for id3 tags */
    ssize_t length;

    /* have we finished fetching this file? (only in non-mmap()'ed case */
    int done;

    /* total number of frames */
    unsigned long num_frames;

    /* number of frames to play */
    unsigned long max_frames;

    /* total duration of the file */
    mad_timer_t duration;

    /* filename as mpg321 has opened it */
    char filename[PATH_MAX];
    
    /* the playlist. */
    playlist *pl;
} buffer;

typedef struct
{
    int opt;
    char *devicetype;
    char *device;
    signed long seek;
    signed long maxframes;
    mad_fixed_t volume;
    int skip_printing_frames;
} mpg321_options;    

extern mpg321_options options;
extern ao_device *playdevice;
extern mad_timer_t current_time;
extern unsigned long current_frame;
extern int stop_playing_file;
extern int shuffle_play;
extern char *playlist_file;
extern int quit_now;
extern char remote_input_buf[PATH_MAX + 5];
extern int file_change;
int loop_remaining;

extern int status;
extern int scrobbler_time;
extern char *scrobbler_args[6];

enum
{
    MPG321_STOPPED       = 0x0001,
    MPG321_PAUSED        = 0x0002,
    MPG321_PLAYING       = 0x0004,
    MPG321_REWINDING     = 0x0008,
    MPG321_SEEKING       = 0x0010
};

enum
{
    MPG321_VERBOSE_PLAY   = 0x00000001,
    MPG321_QUIET_PLAY     = 0x00000002,
    MPG321_REMOTE_PLAY    = 0x00000004,

    MPG321_USE_OSS        = 0x00000010,
    MPG321_USE_SUN        = 0x00000020,
    MPG321_USE_ALSA       = 0x00000040,
    MPG321_USE_ESD        = 0x00000080,
    MPG321_USE_ARTS       = 0x00000100,
    MPG321_USE_NULL       = 0x00000200,
    MPG321_USE_STDOUT     = 0x00000400,
    MPG321_USE_WAV        = 0x00000800,
    MPG321_USE_AU         = 0x00001000,
    MPG321_USE_CDR        = 0x00002000,
    MPG321_USE_USERDEF    = 0x00004000,
    MPG321_USE_ALSA09     = 0x00008000,
    
    MPG321_FORCE_STEREO   = 0x00010000,
    MPG321_USE_SCROBBLER  = 0x00020000,
    MPG321_RECURSIVE_DIR  = 0x00040000,
    MPG321_PRINT_FFT	  = 0x00080000,
    MPG321_ENABLE_BASIC	  = 0x00100000,
    MPG321_ENABLE_BUFFER  = 0x01000000,
};

#define DEFAULT_PLAYLIST_SIZE 2048
#define BUF_SIZE 1048576 /* Size for read buffer for audio data */

/* playlist functions */
playlist * new_playlist();
void resize_playlist(playlist *pl);
char * get_next_file(playlist *pl, buffer *buf);
void add_cmdline_files(playlist *pl, char *argv[]);
void add_cmdline_files_recursive_dir(playlist *pl, char *argv[]);
void add_file(playlist *pl, char *file);
void load_playlist(playlist *pl, char *filename);
void set_random_play(playlist *pl);
void play_remote_file(playlist *pl, char *filename);
void clear_remote_file(playlist *pl);
void shuffle_files(playlist *pl);
void trim_whitespace(char *);

/* network functions */
int tcp_open(char * address, int port);
int udp_open(char * address, int port);
int raw_open(char * arg);
int http_open(char * arg);
int ftp_open(char * arg);

/* libmad interfacing functions */
enum mad_flow read_from_mmap(void *data, struct mad_stream *stream);
enum mad_flow read_from_fd(void *data, struct mad_stream *stream);
enum mad_flow read_header(void *data, struct mad_header const * header);
enum mad_flow output(void *data, struct mad_header const *header, struct mad_pcm *pcm);
int calc_length(char *file, buffer*buf );

static enum mad_flow handle_error(void *data, struct mad_stream *stream, struct mad_frame *frame);

enum mad_flow move(buffer *buf, signed long frames);
void seek(buffer *buf, signed long frame);
void pause_play(buffer *buf, playlist *pl);

/* libao interfacing and general audio-out functions */
void check_ao_default_play_device();
void check_default_play_device();
int playdevice_is_live();
void open_ao_playdevice(struct mad_header const *header);

/* remote control (-R) functions */
void remote_get_input_wait(buffer *buf);
enum mad_flow remote_get_input_nowait(buffer *buf);

/* options */
void parse_options(int argc, char *argv[], playlist *pl);

void mpg321_error(char *file);
void usage(char *);

RETSIGTYPE handle_signals(int sig) ;

/* xterm functions */
int tty_control();
int set_tty_raw();
int set_tty_restore();
int raw_print(char *ctlseq);
int osc_print(int ps1,int ps2,char *pt);
char *ctty_path();
void get_term_title(char *title);
int tty_read(char *output,size_t size);

/* AudioScrobbler functions */
void scrobbler_report(void);
void scrobbler_set_time(long);
void scrobbler_set_verbose(int);
RETSIGTYPE handle_sigchld(int sig);

/* FFT data structures */
#define FFT_BUFFER_SIZE_LOG 9
#define FFT_BUFFER_SIZE (1 << FFT_BUFFER_SIZE_LOG) /* 512 */
/*Temporary data stores to perform FFT in */
double real[FFT_BUFFER_SIZE];
double imag[FFT_BUFFER_SIZE];

typedef struct {
	double real[FFT_BUFFER_SIZE];
	double imag[FFT_BUFFER_SIZE];
} fft_state;

typedef short int sound_sample;
//void fft_perform(const sound_sample *input, double *output, fft_state *state);

fft_state *fft_init(void);

/*Basic control keys */
#define KEY_CTRL_VOLUME_UP	'*'
#define KEY_CTRL_VOLUME_DOWN	'/'
#define KEY_CTRL_NEXT_SONG	'n'
#define KEY_CTRL_PAUSE_SONG	'p'
#define KEY_CTRL_MUTE		'm'
/* This is it for the moment */


/* Output buffer process */
void frame_buffer_p();
/* Semaphore array */
int semarray;
/* Input/Output buffer position */
int mad_decoder_position;
int output_buffer_position;
/* Output Frame including needed information */
typedef struct {
	unsigned char data[4*1152];
	unsigned short length;
	signed long seconds;
	char time[80];
	unsigned long num_frames;
	struct mad_header header;
} output_frame;
/* Control structure with detailed information about each frame */
typedef struct {
	unsigned long total_decoded_frames;
	unsigned long total_played_frames;
	int done;
	int is_http;
	int is_file;
	int quit_now;
	int stop_playing_file;
	int timer;
	long bvolume;
} decoded_frames;

/* Output frame queue pointer */
output_frame *Output_Queue;

/* Shared total decoded frames */
decoded_frames *Decoded_Frames;

#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
/* */
#else
union semun {
	int val;
	struct semid_ds *buf;
	unsigned short *array;
	struct seminfo *__buf;
};
#endif
/* Get the next encoding/decoding place in the frame buffer */
int getnext_place(int position);

#endif /* _MPG321_H_ */
