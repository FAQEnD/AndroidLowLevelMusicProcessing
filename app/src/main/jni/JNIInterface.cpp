//
// Created by antonsmusiienko on 13.04.16.
//
#include "JNIInterface.h"
#include <jni.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
/**
 * Initialization of JNI environment interface
 */
JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_Init(
        JNIEnv *javaEnvironment,
        jobject self,
        jlong samplerate,
        jlong buffersize,
        jstring path,
        jboolean isDefaultFlowOn) {
    const char *tempPath = javaEnvironment->GetStringUTFChars(path, 0);

    lowLevelMusicProcessor = new LowLevelMusicProcessor(samplerate, buffersize, tempPath, isDefaultFlowOn);

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

JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_ToggleVoicePlayback(
        JNIEnv *javaEnvironment, jobject self) {
    lowLevelMusicProcessor->toggleVoicePlayback();
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
    lowLevelMusicProcessor->setTypeFX(value);
}

JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_OnFxValue(
        JNIEnv *javaEnvironment, jobject self, jint value) {
    lowLevelMusicProcessor->onFX(value);
}

JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_OnFxOff(
        JNIEnv *javaEnvironment, jobject self) {
    lowLevelMusicProcessor->offFX();
}

/**
 * Clean JNI resources interface
 */
JNIEXPORT void Java_com_superpowered_frequencydomain_MainActivity_Cleanup(
        JNIEnv *javaEnvironment, jobject self) {
    delete lowLevelMusicProcessor;
}
