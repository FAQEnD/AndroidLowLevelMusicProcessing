//
// Created by antonsmusiienko on 09.04.16.
//

#ifndef FREQUENCYDOMAIN_FREQUENCYDOMAIN_H
#define FREQUENCYDOMAIN_FREQUENCYDOMAIN_H

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
static const int SIZE_OF_FILE_TYPE_SUFFIX = 4;
static const int WAV_HEADER_SIZE = 44;
static const int SAMPLE_RATE = 1024;

static const float MINFREQ = 60.0f;
static const float MAXFREQ = 20000.0f;

static const char *FX_FILTER_SUFFIX = "_filter.wav";
static const char *FX_ROLL_SUFFIX = "_roll.wav";
static const char *FX_FLANGER_SUFFIX = "_flanger.wav";
static const char *FX_ECHO_SUFFIX = "_echo.wav";
static const char *FX_REVERB_SUFFIX = "_reverb.wav";

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

    static const char *sSavePath;

    static char *sLastRecordedFileName;
    static float *sStereoBuffer;
    static FILE *sFile;

    // FX effects
    static SuperpoweredRoll *sRoll;
    static SuperpoweredFilter *sFilter;
    static SuperpoweredFlanger *sFlanger;
    static SuperpoweredEcho *sEcho;
    static SuperpoweredReverb *sReverb;

    int mSampleRate;
    int mBufferSize;
    int mMixTrackStartOffset;
    int mMixTrackLength;

    bool mIsRecording;

    CurrentFX mCurrentFX;

    static bool recordAudioProcessing(void *clientdata, short int *audioInput,
                                      int numberOfSamples, int samplerate);

    static bool outputAudioProcessing(void *clientdata, short int *audioOutput,
                                      int numberOfSamples, int samplerate);

    static void audioPlayEvents(void *clientData, SuperpoweredAdvancedAudioPlayerEvent event,
                                void *value);

    static inline void setBoolField(JNIEnv *javaEnvironment,
                                    jobject self,
                                    jclass thisClass,
                                    const char *name,
                                    bool value);

    static inline float floatToFrequency(float value);

    static inline bool renameFile(const char *fileFrom, const char *fileTo);

    static inline void createFileName(const char *fileNameFrom, char *fileNameTo, CurrentFX currentFX);

public:
    LowLevelMusicProcessor(int samplerate, int buffersize, int mixTrackStartOffset,
                           int mixTrackLength);

    void startRecording();

    void stopRecording();

    void saveWithEffect();

    void copyToFile();

    void togglePlayer();

    // UI Update on Java side
    void updateStatus(JNIEnv *javaEnvironment, jobject self);

    void onFxSelect(int value);

    void onFxValue(int iValue);

    void onFxOff();

    void offAllFX();

    ~LowLevelMusicProcessor();
};

#endif //FREQUENCYDOMAIN_FREQUENCYDOMAIN_H
