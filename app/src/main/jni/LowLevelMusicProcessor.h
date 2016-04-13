//
// Created by antonsmusiienko on 09.04.16.
//

#ifndef FREQUENCYDOMAIN_FREQUENCYDOMAIN_H
#define FREQUENCYDOMAIN_FREQUENCYDOMAIN_H

#include <android/log.h>
#include <jni.h>
#include <stdlib.h>
#include <stdio.h>

#include "Superpowered/SuperpoweredAndroidAudioIO.h"
#include "Superpowered/SuperpoweredAdvancedAudioPlayer.h"
#include "Superpowered/SuperpoweredRoll.h"
#include "Superpowered/SuperpoweredFilter.h"
#include "Superpowered/SuperpoweredFlanger.h"
#include "Superpowered/SuperpoweredEcho.h"
#include "Superpowered/SuperpoweredReverb.h"

#define  LOG_TAG    "LowLevelMusicProcessor"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)


static const int CHANNELS_NUMBER = 2;
static const int SIZE_OF_FX_SUFFIX = 15;
static const int SIZE_OF_WAV_HEADER = 44;
static const int SAMPLE_RATE = 1024;
static const int ON_SAVE_WITH_FX_VALUE = 70;

static const char *SAVED_FILE_NAME = "/saved_record.wav";

class LowLevelMusicProcessor {
public:
    enum CurrentFX {
        FLANGER,
        FILTER,
        ROLL,
        ECHO,
        REVERB
    };

private:

    static SuperpoweredAndroidAudioIO *sAudioInput;
    static SuperpoweredAndroidAudioIO *sAudioOutput;
    static SuperpoweredAdvancedAudioPlayer *sPlayer;

    static char *sLastRecordedFileName;
    static float *sStereoBuffer;
    static FILE *sFile;
    static bool sIsVoicePlaybackOn;

    // FX effects
    static SuperpoweredRoll *sRoll;
    static SuperpoweredFilter *sFilter;
    static SuperpoweredFlanger *sFlanger;
    static SuperpoweredEcho *sEcho;
    static SuperpoweredReverb *sReverb;

    int mSampleRate;
    int mBufferSize;

    bool mIsRecording;

    char *mSavePath;

    CurrentFX mCurrentFX;

    static bool recordAudioProcessing(void *clientdata, short int *audioInput,
                                      int numberOfSamples, int samplerate);

    static bool outputAudioProcessing(void *clientdata, short int *audioOutput,
                                      int numberOfSamples, int samplerate);

    static void audioPlayEvents(void *clientData, SuperpoweredAdvancedAudioPlayerEvent event,
                                void *value);

    static inline void processAllFX(int numberOfSamples);

public:
    LowLevelMusicProcessor(int samplerate, int buffersize, const char *musicFolderPath);

    void startRecording();

    void stopRecording();

    void saveWithEffect();

    void copyToFile(const char *sourcePath, const char *resultPath, int sampleRate);

    void togglePlayer();

    void toggleVoicePlayback();

    // UI Update on Java side
    void updateStatus(JNIEnv *javaEnvironment, jobject self);

    void onFxSelect(int value);

    void onFxValue(int iValue);

    void onFxOff();

    void offAllFX();

    ~LowLevelMusicProcessor();
};

#endif //FREQUENCYDOMAIN_FREQUENCYDOMAIN_H
