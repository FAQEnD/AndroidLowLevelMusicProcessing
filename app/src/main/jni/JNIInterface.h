//
// Created by antonsmusiienko on 13.04.16.
//

#ifndef FREQUENCYDOMAIN_JNIINTERFACE_H_H
#define FREQUENCYDOMAIN_JNIINTERFACE_H_H
#include "LowLevelMusicProcessor.h"

static LowLevelMusicProcessor *lowLevelMusicProcessor;

// Ugly Java-native bridges - JNI, that is.
extern "C" {
JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_Init(
        JNIEnv *javaEnvironment,
        jobject self,
        jlong samplerate,
        jlong buffersize,
        jstring path,
        jboolean isDefaultFlowOn);
JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_StartRecord(
        JNIEnv *javaEnvironment, jobject self);
JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_StopRecord(
        JNIEnv *javaEnvironment, jobject self);

JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_SaveWithEffect(
        JNIEnv *javaEnvironment, jobject self);

JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_TogglePlayer(
        JNIEnv *javaEnvironment, jobject self);

JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_ToggleVoicePlayback(
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

#endif //FREQUENCYDOMAIN_JNIINTERFACE_H_H
