#include <jni.h>
#include <stdlib.h>
#include <stdio.h>
#include "Superpowered/SuperpoweredFrequencyDomain.h"
#include "Superpowered/SuperpoweredAndroidAudioIO.h"
#include "Superpowered/SuperpoweredRecorder.h"
#include "Superpowered/SuperpoweredSimple.h"
#include "Superpowered/SuperpoweredAdvancedAudioPlayer.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>
#include <android/log.h>
#include <math.h>

#include "FrequencyDomain.h"

static LowLevelMusicProcessor *lowLevelMusicProcessor;

/**
 * LowLevelMusicProcessor init of static members
 */
SuperpoweredAndroidAudioIO *LowLevelMusicProcessor::sAudioInput;
SuperpoweredAndroidAudioIO *LowLevelMusicProcessor::sAudioOutput;
SuperpoweredAdvancedAudioPlayer *LowLevelMusicProcessor::sPlayer;

const char *LowLevelMusicProcessor::sSavePath = "/storage/emulated/0/Music/saved_record.wav";
const char *song = "/storage/emulated/0/Music/System Of A Down-Toxicity.mp3";

char *LowLevelMusicProcessor::sLastRecordedFileName;
float *LowLevelMusicProcessor::sStereoBuffer;
FILE *LowLevelMusicProcessor::sFile;

SuperpoweredRoll *LowLevelMusicProcessor::sRoll;
SuperpoweredFilter *LowLevelMusicProcessor::sFilter;
SuperpoweredFlanger *LowLevelMusicProcessor::sFlanger;
SuperpoweredEcho *LowLevelMusicProcessor::sEcho;
SuperpoweredReverb *LowLevelMusicProcessor::sReverb;

bool LowLevelMusicProcessor::recordAudioProcessing(void *clientdata, short int *audioInput,
                                  int numberOfSamples, int samplerate) {
    fwrite(audioInput, sizeof(short int), numberOfSamples * CHANNELS_NUMBER, sFile);
    fflush(sFile);
    return true;
}

bool LowLevelMusicProcessor::outputAudioProcessing(void *clientdata, short int *audioOutput,
                           int numberOfSamples,
                           int samplerate) {
    if (sPlayer->process(sStereoBuffer, false, numberOfSamples)) {
        sRoll->process(sStereoBuffer, sStereoBuffer, numberOfSamples);
        sFilter->process(sStereoBuffer, sStereoBuffer, numberOfSamples);
        sFlanger->process(sStereoBuffer, sStereoBuffer, numberOfSamples);
        sEcho->process(sStereoBuffer, sStereoBuffer, numberOfSamples);
        sReverb->process(sStereoBuffer, sStereoBuffer, numberOfSamples);

        SuperpoweredFloatToShortInt(sStereoBuffer, audioOutput, numberOfSamples);
        return true;
    } else return false;
}

void LowLevelMusicProcessor::audioPlayEvents(void *clientData, SuperpoweredAdvancedAudioPlayerEvent event,
                     void *value) {
    switch (event) {
        case SuperpoweredAdvancedAudioPlayerEvent_LoadError:
            LOGE("LoadError, reason: %s", (const char *) value);
            break;
        case SuperpoweredAdvancedAudioPlayerEvent_LoadSuccess:
            LOGD("LoadSuccess");
            break;
        case SuperpoweredAdvancedAudioPlayerEvent_EOF:
            LOGD("EOF");
            break;
        default:
            LOGD("Audio Player events");
    }
}

inline void LowLevelMusicProcessor::setBoolField(JNIEnv *javaEnvironment,
                                                 jobject self,
                                                 jclass thisClass,
                                                 const char *name,
                                                 bool value) {
    javaEnvironment->SetBooleanField(self, javaEnvironment->GetFieldID(thisClass, name, "Z"), value);
}

inline float LowLevelMusicProcessor::floatToFrequency(float value) {
    if (value > 0.97f) return MAXFREQ;
    if (value < 0.03f) return MINFREQ;
    value = powf(10.0f, (value + ((0.4f - fabsf(value - 0.4f)) * 0.3f)) * log10f(MAXFREQ - MINFREQ)) + MINFREQ;
    return value < MAXFREQ ? value : MAXFREQ;
}

inline bool LowLevelMusicProcessor::renameFile(const char *fileFrom, const char *fileTo) {
    if (rename(fileFrom, fileTo) == 0) {
        LOGD("file renamed");
        return true;
    } else {
        LOGE("File rename error");
        return false;
    }
}

inline void LowLevelMusicProcessor::createFileName(const char *fileNameFrom,
                                                   char *fileNameTo,
                                                   CurrentFX currentFX) {
    int fileNameFromStrLength = strlen(fileNameFrom);

    // remove suffix ".wav" in file
    strncpy(fileNameTo, fileNameFrom, fileNameFromStrLength - SIZE_OF_FILE_TYPE_SUFFIX);
    fileNameTo[fileNameFromStrLength - SIZE_OF_FILE_TYPE_SUFFIX] = '\0';

    // add suffix depends on type of FX effects
    switch (currentFX) {
        case FILTER:
            strcat(fileNameTo, FX_FILTER_SUFFIX);
            break;
        case ROLL:
            strcat(fileNameTo, FX_ROLL_SUFFIX);
            break;
        case ECHO:
            strcat(fileNameTo, FX_ECHO_SUFFIX);
            break;
        case REVERB:
            strcat(fileNameTo, FX_REVERB_SUFFIX);
            break;
        default:
            strcat(fileNameTo, FX_FLANGER_SUFFIX);
    }

    LOGD("Str: %s'", fileNameTo);
    LOGD("Str len: %d, original std len: %d", strlen(fileNameTo), fileNameFromStrLength);
}

LowLevelMusicProcessor::LowLevelMusicProcessor(int samplerate,
                                               int buffersize,
                                               int mixTrackStartOffset,
                                               int mixTrackLength):
        mSampleRate(samplerate),
        mBufferSize(buffersize),
        mMixTrackStartOffset(mixTrackStartOffset),
        mMixTrackLength(mixTrackLength),
        mCurrentFX(CurrentFX::FLANGER),
        mIsRecording(false) {
    sStereoBuffer = new float [CHANNELS_NUMBER * mBufferSize + 128];

    sAudioInput = new SuperpoweredAndroidAudioIO(mSampleRate, mBufferSize, true, true,
                                                recordAudioProcessing,
                                                NULL,
                                                -1,
                                                -1,
                                                mBufferSize * CHANNELS_NUMBER);
    sAudioInput->stop();

    sAudioOutput = new SuperpoweredAndroidAudioIO(mSampleRate, mBufferSize, false, true,
                                              outputAudioProcessing,
                                              NULL,
                                              -1,
                                              -1,
                                              mBufferSize * CHANNELS_NUMBER);
    sAudioOutput->stop();

    sPlayer = new SuperpoweredAdvancedAudioPlayer(NULL, audioPlayEvents, mSampleRate, 0);

    sRoll = new SuperpoweredRoll(mSampleRate);
    sFilter = new SuperpoweredFilter(SuperpoweredFilter_Resonant_Lowpass, mSampleRate);
    sFlanger = new SuperpoweredFlanger(mSampleRate);
    sEcho = new SuperpoweredEcho(mSampleRate);
    sReverb = new SuperpoweredReverb(mSampleRate);
}

void LowLevelMusicProcessor::startRecording() {
    LOGD("Start recording");
    if (sPlayer->playing) {
        togglePlayer();
    }
    if (!mIsRecording) {
        sFile = createWAV(sSavePath, mSampleRate, CHANNELS_NUMBER);
        sAudioInput->start();
        mIsRecording = true;
    } else {
        LOGD("already recording");
    }
}

void LowLevelMusicProcessor::stopRecording() {
    LOGD("Stop");
    if (mIsRecording) {
        sAudioInput->stop();
        closeWAV(sFile);
        if (fclose(sFile) != 0) {
            LOGE("Error in closing save_record.wav");
        }
        sPlayer->open(sSavePath);
        mIsRecording = false;
    } else {
        LOGD("already stopped");
    }
}

void LowLevelMusicProcessor::saveWithEffect() {
    if (sPlayer->playing) {
        togglePlayer();
    }
    if (mIsRecording) {
        stopRecording();
    }
    if (sLastRecordedFileName == NULL) {
        sLastRecordedFileName = new char[strlen(sSavePath) + SIZE_OF_FX_SUFFIX];
    }
    createFileName(sSavePath, sLastRecordedFileName, mCurrentFX);
    copyToFile();
}

void LowLevelMusicProcessor::copyToFile() {
    FILE *sourceFile = fopen(sSavePath, "r");
    FILE *resultFile = createWAV(sLastRecordedFileName, mSampleRate, CHANNELS_NUMBER);
    unsigned int n;
    short int *intBuffer = NULL;

    if (sourceFile == NULL) {
        LOGE("Can't open source file");
        return;
    }
    if (resultFile == NULL) {
        LOGE("Can't open result file");
        return;
    }
    onFxValue(80);

    fseek(sourceFile, WAV_HEADER_SIZE, SEEK_SET);

    if (intBuffer == NULL) {
        LOGD("new short int[2048]");
        intBuffer = new short int[SAMPLE_RATE * CHANNELS_NUMBER];
    }
    LOGD("Copy To File");
    while ((n = fread(intBuffer, sizeof(short int), SAMPLE_RATE, sourceFile)) > 0)
    {
        SuperpoweredShortIntToFloat(intBuffer, sStereoBuffer, SAMPLE_RATE);

        sFlanger->process(sStereoBuffer, sStereoBuffer, n / 2);
        sFilter->process(sStereoBuffer, sStereoBuffer, n / 2);
        sRoll->process(sStereoBuffer, sStereoBuffer, n / 2);
        sEcho->process(sStereoBuffer, sStereoBuffer, n / 2);
        sReverb->process(sStereoBuffer, sStereoBuffer, n / 2);

        SuperpoweredFloatToShortInt(sStereoBuffer, intBuffer, SAMPLE_RATE);

        if (fwrite(intBuffer, sizeof(short int), n, resultFile) != n) {
            LOGE("Error in file copying");
            return;
        } else {
            fflush(resultFile);
        }
    }
    LOGD("Before clear intBuffer");
    delete []intBuffer;
    LOGD("Copy completed");
    offAllFX();

    if (fclose(sourceFile) != 0) {
        LOGE("Error in closing source file");
    }
    closeWAV(resultFile);
    if (fclose(resultFile) != 0) {
        LOGE("Error in closing result file");
    }
}

void LowLevelMusicProcessor::togglePlayer() {
    LOGD("Toggle Playback");
    if (mIsRecording) {
        stopRecording();
    }
    if (sPlayer->playing) {
        sAudioOutput->stop();
        LOGD("Pause");
    } else {
        if (sLastRecordedFileName == NULL) {
            sPlayer->open(sSavePath);
        } else {
            sPlayer->open(sLastRecordedFileName);
        }
        sAudioOutput->start();
        LOGD("Play");
    }
    sPlayer->togglePlayback();
}

void LowLevelMusicProcessor::updateStatus(JNIEnv *javaEnvironment, jobject self) {
    jclass thisClass = javaEnvironment->GetObjectClass(self);
    setBoolField(javaEnvironment, self, thisClass, "isPlaying", sPlayer->playing);
}

/**
 * Audio FX
 */
void LowLevelMusicProcessor::onFxSelect(int value) {
    mCurrentFX = (CurrentFX) value;
    LOGD("CurrentFX: %d", mCurrentFX);
}

void LowLevelMusicProcessor::onFxValue(int iValue) {
    float value = (float) iValue * 0.01f;
    // TODO: need to decrease calls of offAllFX() method
    offAllFX();

    switch (mCurrentFX) {
        case FILTER:
            LOGD("Filter enabled");
            sFilter->setResonantParameters(floatToFrequency(1.0f - value), 0.2f);
            sFilter->enable(true);
            break;
        case ROLL:
            LOGD("Roll enabled");
            if (value > 0.8f) {
                sRoll->beats = 0.0625f;
            } else if (value > 0.6f) {
                sRoll->beats = 0.125f;
            } else if (value > 0.4f) {
                sRoll->beats = 0.25f;
            } else if (value > 0.2f) {
                sRoll->beats = 0.5f;
            } else {
                sRoll->beats = 1.0f;
            }
            sRoll->enable(true);
            break;
        case ECHO:
            LOGD("Echo enabled");
            sEcho->setMix(value);
            sEcho->enable(true);
            break;
        case REVERB:
            LOGD("Reverb enabled");
            sReverb->setMix(value);
            sReverb->enable(true);
            break;
        default:
            LOGD("Flanger enabled");
            sFlanger->setWet(value);
            sFlanger->enable(true);
            break;
    }
}

void LowLevelMusicProcessor::onFxOff() {
    offAllFX();
}

void LowLevelMusicProcessor::offAllFX() {
    LOGD("Off all FX");
    sRoll->enable(false);
    sFilter->enable(false);
    sFlanger->enable(false);
    sEcho->enable(false);
    sReverb->enable(false);
}

LowLevelMusicProcessor::~LowLevelMusicProcessor() {
    // clear audioIO and player pointers
    delete sAudioInput;
    delete sAudioOutput;
    delete sPlayer;

    // clear effects pointers
    delete sRoll;
    delete sFilter;
    delete sFlanger;
    delete sEcho;
    delete sReverb;

    // clear resources
    delete []sLastRecordedFileName;

//    free(sStereoBuffer);
    delete []sStereoBuffer;
}

// Ugly Java-native bridges - JNI, that is.
extern "C" {
JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_Init(
        JNIEnv *javaEnvironment, jobject self, jlong samplerate, jlong buffersize, jstring path, jlong startOffset, jlong length);
JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_StartRecord(
        JNIEnv *javaEnvironment, jobject self);
JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_StopRecord(
        JNIEnv *javaEnvironment, jobject self);

JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_SaveWithEffect(
        JNIEnv *javaEnvironment, jobject self);

JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_TogglePlayer(
        JNIEnv *javaEnvironment, jobject self);

JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_UpdateStatus(
        JNIEnv *javaEnvironment, jobject self);

JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_OnFxSelect(
        JNIEnv *javaEnvironment, jobject self, jint value);
JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_OnFxValue(
        JNIEnv *javaEnvironment, jobject self, jint value);
JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_OnFxOff(
        JNIEnv *javaEnvironment, jobject self);

JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_Cleanup(
        JNIEnv *javaEnvironment, jobject self);
}

/**
 * Initialization of JNI environment interface
 */
JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_Init(
        JNIEnv *javaEnvironment, jobject self, jlong samplerate, jlong buffersize, jstring path, jlong startOffset, jlong length) {
    const char *tempPath = javaEnvironment->GetStringUTFChars(path, 0);

    lowLevelMusicProcessor = new LowLevelMusicProcessor(samplerate, buffersize, startOffset, length);

    javaEnvironment->ReleaseStringUTFChars(path, tempPath);
}

/**
 * Recording interface
 */
JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_StartRecord(
        JNIEnv *javaEnvironment, jobject self) {
    lowLevelMusicProcessor->startRecording();
}

JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_StopRecord(
        JNIEnv *javaEnvironment, jobject self) {
    lowLevelMusicProcessor->stopRecording();
}

JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_SaveWithEffect(
        JNIEnv *javaEnvironment, jobject self) {
    lowLevelMusicProcessor->saveWithEffect();
}

/**
 * Player interface
 */
JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_TogglePlayer(
        JNIEnv *javaEnvironment, jobject self) {
    lowLevelMusicProcessor->togglePlayer();
}

/**
 * UI update interface
 */
JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_UpdateStatus(
        JNIEnv *javaEnvironment, jobject self) {
    lowLevelMusicProcessor->updateStatus(javaEnvironment, self);
}

/**
 * FX effects interface
 */
JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_OnFxSelect(
        JNIEnv *javaEnvironment, jobject self, jint value) {
    lowLevelMusicProcessor->onFxSelect(value);
}

JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_OnFxValue(
        JNIEnv *javaEnvironment, jobject self, jint value) {
    lowLevelMusicProcessor->onFxValue(value);
}

JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_OnFxOff(
        JNIEnv *javaEnvironment, jobject self) {
    lowLevelMusicProcessor->onFxOff();
}

/**
 * Clean JNI resources interface
 */
JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_Cleanup(
        JNIEnv *javaEnvironment, jobject self) {
    delete lowLevelMusicProcessor;
}