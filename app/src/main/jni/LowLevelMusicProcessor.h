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

#include "FXManager.h"

#define  LOG_TAG    "LowLevelMusicProcessor"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static const int CHANNELS_NUMBER = 2;
static const int SIZE_OF_FX_SUFFIX = 15;
static const int SIZE_OF_WAV_HEADER = 44;
static const int SAMPLE_RATE = 1024;
static const int ON_SAVE_WITH_FX_VALUE = 70;
static const float SONG_VOLUME = 0.2f;
static const float DEFAULT_GAIN = 1.0f;

static const char *SAVED_FILE_NAME = "/saved_record.wav";
static const char *SONG_FILE_NAME = "/System Of A Down-Toxicity.mp3";

class LowLevelMusicProcessor {

private:

    static SuperpoweredAndroidAudioIO *sAudioInput;
    static SuperpoweredAndroidAudioIO *sAudioOutput;
    static SuperpoweredAdvancedAudioPlayer *sVoicePlayer;
    static SuperpoweredAdvancedAudioPlayer *sSongPlayer;

    static char *sLastRecordedFileName;
    static float *sFirstBuffer;
    static float *sSecondBuffer;
    static float *sFinalBuffer;
    static FILE *sFile;
    static bool sIsVoicePlaybackOn;
    static bool sIsAudioOutputSamplesCalculated;
    static bool sIsSavedWithFX;
    static bool sIsDefaultFlowOn;
    static int sNumberOfSamplesAudioOutput;

    int mSampleRate;
    int mBufferSize;
    bool mIsRecording;
    char *mSavePath;
    char *mSongPath;

    FXManager *mFXManager;

    static bool recordAudioProcessing(void *clientdata, short int *audioInput,
                                      int numberOfSamples, int samplerate);

    static bool outputAudioProcessing(void *clientdata, short int *audioOutput,
                                      int numberOfSamples, int samplerate);

    static void audioPlayEvents(void *clientData, SuperpoweredAdvancedAudioPlayerEvent event,
                                void *value);

public:
    LowLevelMusicProcessor(int samplerate,
                           int buffersize,
                           const char *musicFolderPath,
                           bool isDefaultFlowOn);

    void startRecording();

    void stopRecording();

    void saveWithEffect();

    void copyToFile(const char *sourcePath, const char *resultPath, int sampleRate);

    void togglePlayer();

    void chooseCorrectPath();

    void toggleVoicePlayback();

    // UI Update on Java side
    void updateStatus(JNIEnv *javaEnvironment, jobject self);

    void setTypeFX(int value);

    void onFX(int value);

    void offFX();

    ~LowLevelMusicProcessor();
};

#endif //FREQUENCYDOMAIN_FREQUENCYDOMAIN_H
