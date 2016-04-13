#include <jni.h>
#include <stdlib.h>
#include <stdio.h>
#include "Superpowered/SuperpoweredFrequencyDomain.h"
#include "Superpowered/SuperpoweredSimple.h"
#include "Superpowered/SuperpoweredRecorder.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>

#include "LowLevelMusicProcessor.h"
#include "Utils.h"

static LowLevelMusicProcessor *lowLevelMusicProcessor;

/**
 * LowLevelMusicProcessor init of static members
 */
SuperpoweredAndroidAudioIO *LowLevelMusicProcessor::sAudioInput;
SuperpoweredAndroidAudioIO *LowLevelMusicProcessor::sAudioOutput;
SuperpoweredAdvancedAudioPlayer *LowLevelMusicProcessor::sPlayer;

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

        processAllFX(numberOfSamples);

        SuperpoweredFloatToShortInt(sStereoBuffer, audioOutput, numberOfSamples);
        return true;
    } else return false;
}

void LowLevelMusicProcessor::audioPlayEvents(void *clientData,
                                             SuperpoweredAdvancedAudioPlayerEvent event,
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

LowLevelMusicProcessor::LowLevelMusicProcessor(int samplerate,
                                               int buffersize,
                                               const char *musicFolderPath):
        mSampleRate(samplerate),
        mBufferSize(buffersize),
        mCurrentFX(CurrentFX::FLANGER),
        mIsRecording(false) {
    mSavePath = new char [strlen(musicFolderPath) + strlen(SAVED_FILE_NAME) + 1];
    Utils::prepareSavePath(mSavePath, musicFolderPath);

    sStereoBuffer = new float [CHANNELS_NUMBER * mBufferSize + 128];

    sAudioInput = new SuperpoweredAndroidAudioIO(mSampleRate, mBufferSize, true, true,
                                                recordAudioProcessing,
                                                NULL,
                                                -1,
                                                -1,
                                                mBufferSize * CHANNELS_NUMBER * 2);
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
        sFile = createWAV(mSavePath, mSampleRate, CHANNELS_NUMBER);
        sAudioInput->start();
        mIsRecording = true;
    } else {
        LOGD("already recording");
    }
}

void LowLevelMusicProcessor::stopRecording() {
    LOGD("Stop recording");
    if (mIsRecording) {
        sAudioInput->stop();
        closeWAV(sFile);
        if (fclose(sFile) != 0) {
            LOGE("Error in closing save_record_fwrite.wav");
        }
        sPlayer->open(mSavePath);
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
        sLastRecordedFileName = new char[strlen(mSavePath) + SIZE_OF_FX_SUFFIX];
    }
    Utils::createFileName(mSavePath, sLastRecordedFileName, mCurrentFX);
    copyToFile();
}

void LowLevelMusicProcessor::copyToFile() {
    FILE *sourceFile = fopen(mSavePath, "r");
    FILE *resultFile = createWAV(sLastRecordedFileName, mSampleRate, CHANNELS_NUMBER);

    unsigned int n;
    unsigned int numberOfSamplesToProcess;
    short int *intBuffer = NULL;

    if (sourceFile == NULL) {
        LOGE("Can't open source file");
        return;
    }
    if (resultFile == NULL) {
        LOGE("Can't open result file");
        return;
    }
    onFxValue(ON_SAVE_WITH_FX_VALUE);

    // skip first 44 bites of WAV Header
    fseek(sourceFile, SIZE_OF_WAV_HEADER, SEEK_SET);

    if (intBuffer == NULL) {
        intBuffer = new short int[SAMPLE_RATE * CHANNELS_NUMBER];
    }
    while ((n = fread(intBuffer, sizeof(short int), SAMPLE_RATE, sourceFile)) > 0)
    {
        SuperpoweredShortIntToFloat(intBuffer, sStereoBuffer, SAMPLE_RATE);

        numberOfSamplesToProcess = n / 2;
        processAllFX(numberOfSamplesToProcess);

        SuperpoweredFloatToShortInt(sStereoBuffer, intBuffer, SAMPLE_RATE);

        if (fwrite(intBuffer, sizeof(short int), n, resultFile) != n) {
            LOGE("Error in file copying");
            return;
        } else {
            fflush(resultFile);
        }
    }
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
            sPlayer->open(mSavePath);
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
    Utils::setBoolField(javaEnvironment, self, thisClass, "isPlaying", sPlayer->playing);
    Utils::setBoolField(javaEnvironment, self, thisClass, "isRecording", mIsRecording);
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
            sFilter->setResonantParameters(Utils::floatToFrequency(1.0f - value), 0.2f);
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

inline void LowLevelMusicProcessor::processAllFX(int numberOfSamples) {
    sFlanger->process(sStereoBuffer, sStereoBuffer, numberOfSamples);
    sFilter->process(sStereoBuffer, sStereoBuffer, numberOfSamples);
    sRoll->process(sStereoBuffer, sStereoBuffer, numberOfSamples);
    sEcho->process(sStereoBuffer, sStereoBuffer, numberOfSamples);
    sReverb->process(sStereoBuffer, sStereoBuffer, numberOfSamples);
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
    // clear AudioIO, player and recorder pointers
    delete sAudioInput;
    delete sAudioOutput;
    delete sPlayer;

    // clear FX pointers
    delete sRoll;
    delete sFilter;
    delete sFlanger;
    delete sEcho;
    delete sReverb;

    // clear resources
    delete []sLastRecordedFileName;
    delete []mSavePath;
    delete []sStereoBuffer;
}

// Ugly Java-native bridges - JNI, that is.
extern "C" {
JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_Init(
        JNIEnv *javaEnvironment, jobject self, jlong samplerate, jlong buffersize, jstring path);
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
        JNIEnv *javaEnvironment, jobject self, jlong samplerate, jlong buffersize, jstring path) {
    const char *tempPath = javaEnvironment->GetStringUTFChars(path, 0);

    lowLevelMusicProcessor = new LowLevelMusicProcessor(samplerate, buffersize, tempPath);

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