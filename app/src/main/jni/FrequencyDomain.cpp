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

#include "FrequencyDomain.h"

static LowLevelMusicProcessor *lowLevelMusicProcessor;

/**
 * LowLevelMusicProcessor init of static members
 */
SuperpoweredAndroidAudioIO *LowLevelMusicProcessor::sAudioInput;
SuperpoweredAndroidAudioIO *LowLevelMusicProcessor::sAudioOutput;
SuperpoweredAdvancedAudioPlayer *LowLevelMusicProcessor::sPlayer;

const char *LowLevelMusicProcessor::sSavePath = "/storage/emulated/0/Music/saved_record3.wav";
float *LowLevelMusicProcessor::sFloatBuffer;
FILE *LowLevelMusicProcessor::sFile;

LowLevelMusicProcessor::LowLevelMusicProcessor(int samplerate, int buffersize): mSampleRate(samplerate), mBufferSize(buffersize) {
    sFloatBuffer = (float *) malloc(sizeof(float) * CHANNELS_NUMBER * mBufferSize + 128);

    sAudioInput = new SuperpoweredAndroidAudioIO(mSampleRate, mBufferSize, true, false,
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
}

LowLevelMusicProcessor::~LowLevelMusicProcessor() {
    delete sAudioInput;
    delete sAudioOutput;
    delete sPlayer;
    free(sFloatBuffer);
}

void LowLevelMusicProcessor::StartRecording() {
    LOGD("Start");
    sFile = createWAV(sSavePath, mSampleRate, CHANNELS_NUMBER);
    sAudioInput->start();
}

void LowLevelMusicProcessor::StopRecording() {
    LOGD("Stop");
    sAudioInput->stop();
    closeWAV(sFile);
    fclose(LowLevelMusicProcessor::sFile);
    sPlayer->open(sSavePath);
}

void LowLevelMusicProcessor::TogglePlayer() {
    LOGD("Toggle Playback");
    if (sPlayer->playing) {
        sAudioOutput->stop();
        LOGD("Pause");
    } else {
        if (sPlayer->positionPercent <= 0) {
            sPlayer->open(sSavePath);
        }
        sAudioOutput->start();
        LOGD("Play");
    }
    sPlayer->togglePlayback();
}

void LowLevelMusicProcessor::UpdateStatus(JNIEnv *javaEnvironment, jobject self) {
    jclass thisClass = javaEnvironment->GetObjectClass(self);
    setBoolField(javaEnvironment, self, thisClass, "isPlaying", sPlayer->playing);
}

// Ugly Java-native bridges - JNI, that is.
extern "C" {
JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_Init(
        JNIEnv *javaEnvironment, jobject self, jlong samplerate, jlong buffersize, jstring path);
JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_StartRecord(
        JNIEnv *javaEnvironment, jobject self);
JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_StopRecord(
        JNIEnv *javaEnvironment, jobject self);
JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_TogglePlayer(
        JNIEnv *javaEnvironment, jobject self);
JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_UpdateStatus(
        JNIEnv *javaEnvironment, jobject self);
JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_Cleanup(
        JNIEnv *javaEnvironment, jobject self);
}

JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_Init(
        JNIEnv *javaEnvironment, jobject self, jlong samplerate, jlong buffersize, jstring path) {
    const char *tempPath = javaEnvironment->GetStringUTFChars(path, 0);

    lowLevelMusicProcessor = new LowLevelMusicProcessor(samplerate, buffersize);

    javaEnvironment->ReleaseStringUTFChars(path, tempPath);
}

JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_StartRecord(
        JNIEnv *javaEnvironment, jobject self) {
    lowLevelMusicProcessor->StartRecording();
}

JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_StopRecord(
        JNIEnv *javaEnvironment, jobject self) {
    lowLevelMusicProcessor->StopRecording();
}

JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_TogglePlayer(
        JNIEnv *javaEnvironment, jobject self) {
    lowLevelMusicProcessor->TogglePlayer();
}

JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_UpdateStatus(
        JNIEnv *javaEnvironment, jobject self) {
    lowLevelMusicProcessor->UpdateStatus(javaEnvironment, self);
}

JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_Cleanup(
        JNIEnv *javaEnvironment, jobject self) {
    delete lowLevelMusicProcessor;
}