/**************************************************************
 * Copyright (C) 2017 by Chan Russell, Robert                 *
 *                                                            *
 * Project: Audio Looper                                      *
 *                                                            *
 * This file contains shared data types, defines, and         *
 * prototypes                                                 *
 *                                                            *
 *                                                            *
 *************************************************************/

#ifndef LOCAL_H
#define LOCAL_H

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <float.h>
#include <pthread.h>

#include <jack/jack.h>

/**************************************************************
 * Macros and defines                                         *
 *************************************************************/
#define MAX_SAMPLE_VALUE                (UINT16_MAX) // match to audio capture device, 220 is 16bit
#define SAMPLE_RATE_DEVICE              (44100) // match to audio device, 44100/48000/96000/192000
#define TRACK_MAX_LENGTH_S              (60)
#define NUM_GROUPS                      (4)
#define NUM_TRACKS                      (16)
#define SAMPLE_LIMIT                    (44100 * TRACK_MAX_LENGTH_S)
#define FRAME_COUNT                     (SAMPLE_LIMIT + 512)
#define GPIO_ISR_DEBOUNCE_MS            (500)
// Serial interface commands
#define SERIAL_TERMINAL                 "/dev/ttyACM0" // /dev/ttyACM0 for Teensy/Arduino, /dev/ttyAMA0 for Serial UART via Rx Tx pins
#define SERIAL_BAUD_RATE                (115200)
#define MIN_SERIAL_DATA_LENGTH          (6)
#define SERIAL_CMD_OFFSET               (0)
#define SERIAL_SUB_CMD_OFFSET           (3)
#define SERIAL_TRACK_UPPER_DIGIT        (1)
#define SERIAL_TRACK_LOWER_DIGIT        (2)
#define SERIAL_TRACK_GROUP_LOWER_DIGIT  (4)
#define SERIAL_GROUP_SELECT_LOWER_DIGIT (1)
#define SERIAL_LAST_CHAR                (5)
#define SERIAL_CMD_RECORD_LC            'r'
#define SERIAL_CMD_RECORD_UC            'R'
#define SERIAL_CMD_OPTION_REPEAT_ON     'r'
#define SERIAL_CMD_OPTION_REPEAT_OFF    's'
#define SERIAL_CMD_OVERDUB_LC           'o'
#define SERIAL_CMD_OVERDUB_UC           'O'
#define SERIAL_CMD_PLAY_LC              'p'
#define SERIAL_CMD_PLAY_UC              'P'
#define SERIAL_CMD_TRACK_MUTE_LC        'm'
#define SERIAL_CMD_TRACK_MUTE_UC        'M'
#define SERIAL_CMD_TRACK_UNMUTE_LC      'u'
#define SERIAL_CMD_TRACK_UNMUTE_UC      'U'
#define SERIAL_CMD_ADD_TRACK2GROUP_LC   't'
#define SERIAL_CMD_ADD_TRACK2GROUP_UC   'T'
#define SERIAL_CMD_RMV_TRACK_GROUP_LC   'd'
#define SERIAL_CMD_RMV_TRACK_GROUP_UC   'D'
#define SERIAL_CMD_GROUP_SELECT_LC      'g'
#define SERIAL_CMD_GROUP_SELECT_UC      'G'
#define SERIAL_CMD_SYSTEM_RESET_LC      's'
#define SERIAL_CMD_SYSTEM_RESET_UC      'S'
#define SERIAL_CMD_QUIT_LC              'q'
#define SERIAL_CMD_QUIT_UC              'Q'
#define SERIAL_CMD_ACCEPTED             'p'
#define SERIAL_CMD_REJECTED             'f'

// Debug
#define TRACK_TEST_PULSE_COUNT          8
#define TRACK_DEBUG_FRAME_COUNT         88200

// Timer defines
#define TIMER_COUNT	(5)
#define FOREACH_TIMER(TIMER) \
    TIMER(TIMER_RECORD_START_DELAY) \
    TIMER(TIMER_PLAY_RECORD_DELAY)  \
    TIMER(TIMER_PROCESS_TO_PROCESS_TIME) \
    TIMER(TIMER_RECORD_STOP_DELAY) \
    TIMER(TIMER_UART_PROCESS) \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

/**************************************************************
 * Data types                                                 *
 *************************************************************/
enum TIMER_ENUM {
    FOREACH_TIMER(GENERATE_ENUM)
};
/*
{
    TIMER_RECORD_START_DELAY,
    TIMER_PLAY_RECORD_DELAY,
    TIMER_PROCESS_TO_PROCESS_TIME,
    TIMER_RECORD_STOP_DELAY,
    TIMER_UART_PROCESS,
    TIMER_COUNT
};
*/

enum TrackState
{
    TRACK_STATE_OFF,                // Empty track or available for recording
    TRACK_STATE_PLAYBACK,           // In playback mode
    TRACK_STATE_RECORDING,          // In recording mode
    TRACK_STATE_MUTE                // In mute state, don't mixdown
};

enum SystemEvents
{
    SYSTEM_EVENT_PASSTHROUGH,               // System->passthrough, all tracks->off, all indexes set to 0
    SYSTEM_EVENT_RECORD_TRACK,              // System->recording, track->recording - track & group # required
    SYSTEM_EVENT_OVERDUB_TRACK,             // System->overdubbing, track->recording - track & group # required
    SYSTEM_EVENT_PLAY_TRACK,                // Reset's track's current index to start index and state to play - track # required
    SYSTEM_EVENT_MUTE_TRACK,                // Place particular track into Mute state - track # required
    SYSTEM_EVENT_UNMUTE_TRACK,              // Changes track to Play state - track # required
    SYSTEM_EVENT_ADD_TRACK_TO_GROUP,        // Adds a track to a group - nothing more - track # & group # required
    SYSTEM_EVENT_REMOVE_TRACK_FROM_GROUP,   // Removes track from a group - track # & group # required
    SYSTEM_EVENT_SET_ACTIVE_GROUP,          // Sets the currently active group - group # required
};

enum SystemStates
{
    SYSTEM_STATE_PASSTHROUGH,       // No mixdown or recording
    SYSTEM_STATE_PLAYBACK,          // Tracks available for mixing and playing
    SYSTEM_STATE_RECORDING,         // Copying data to selected track
    SYSTEM_STATE_OVERDUBBING,       // Overdubbing selected track
    SYSTEM_STATE_CALIBRATION        // For sychronization configuration
};

struct Track
{
    // data buffer - should be an array of samples to allow easier access
    jack_default_audio_sample_t channelLeft[FRAME_COUNT];
    jack_default_audio_sample_t channelRight[FRAME_COUNT];
    uint32_t currIdx;               // Current index into samples, range is 0 to sampleIndexEnd
    uint32_t startIdx;              // Start location - assigned to master's current location
    uint32_t endIdx;                // Number of samples for this track - ie track length
    uint32_t pulseIdxArr[TRACK_TEST_PULSE_COUNT];
    uint8_t  pulseIdx;
    enum TrackState state;
    bool repeat;                    // If track isn't the longest track, we can repeat it:
                                    //      if we get to the end of this track but not master track
                                    //      we can repeat this track (or part of it) until master track resets
};

struct MasterLooper
{
    // pointers to the tracks
    struct Track tracks[NUM_TRACKS];
    struct Track *groupedTracks[NUM_GROUPS][NUM_TRACKS];
    jack_port_t *input_portL;
    jack_port_t *output_portL;
    jack_port_t *input_portR;
    jack_port_t *output_portR;
    jack_client_t *client;
    pthread_t controlTh;                    // Thread to monitor the UART/Interfaces
    uint32_t    masterLength[NUM_GROUPS];   // Longest track, some tracks may be on repeat, others silent
    uint32_t    masterCurrIdx;              // Current index of master track
    uint32_t    callCounter;
    // Frame counters for synchronization
    jack_nframes_t ui_frames_cmd_rx;        // Frame count when command received
    jack_nframes_t process_frames;          // Frame count when Process called
    jack_nframes_t rec_frame_delay;         // Frames between rec and actual update
    jack_nframes_t play_frame_delay;        // Frames between play and actual update
    int         sfd;                        // Serial port file description
    uint8_t     selectedTrack;              // Track number we're recording to, 0xFF if playback only
    uint8_t     selectedGroup;              // 0 for no groups - mute - 1+ if recording
    uint8_t     min_serial_data_length;     // minimum UART data received before command processed
    enum        SystemStates state;         // Current state of the system
    bool        monitoringOff;              // Allow system input to be output, turn off when tuning or not wanting any noise going through
    bool        controlLocked;              // Prevent updates to states while Jack is processing
    bool        exitNow;                    // Enables program to exit via UART cmd
};


/**************************************************************
 * Public function prototypes
 *************************************************************/

void overdub(
    jack_default_audio_sample_t *in, 
    jack_default_audio_sample_t *track,
    jack_nframes_t nframes);

void updateIndices(struct MasterLooper *looper, jack_nframes_t nframes); 
int playRecord (
    struct MasterLooper *looper,
    jack_default_audio_sample_t *mixdownLeft,
    jack_default_audio_sample_t *mixdownRight,
    jack_nframes_t nframes);

void controlStateCheck(void);
bool controlInit(struct MasterLooper *mLooper);

int getNumActiveTracks(void);

void startTimer(uint8_t index);
void stopTimer(uint8_t index);
void printTimers(void);

#endif // local.h
