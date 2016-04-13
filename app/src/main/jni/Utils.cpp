//
// Created by antonsmusiienko on 13.04.16.
//
#include "Utils.h"

#include <math.h>
#include <jni.h>
#include <stdlib.h>
#include <stdio.h>
float Utils::floatToFrequency(float value) {
    if (value > 0.97f) return MAXFREQ;
    if (value < 0.03f) return MINFREQ;
    value = powf(10.0f, (value + ((0.4f - fabsf(value - 0.4f)) * 0.3f)) * log10f(MAXFREQ - MINFREQ)) + MINFREQ;
    return value < MAXFREQ ? value : MAXFREQ;
}

void Utils::createFileName(const char *fileNameFrom,
                                                   char *fileNameTo,
                                                   LowLevelMusicProcessor::CurrentFX currentFX) {
    int fileNameFromStrLength = strlen(fileNameFrom);

    // remove suffix ".wav" in file
    strncpy(fileNameTo, fileNameFrom, fileNameFromStrLength - SIZE_OF_FILE_TYPE_SUFFIX);
    fileNameTo[fileNameFromStrLength - SIZE_OF_FILE_TYPE_SUFFIX] = '\0';

    // add suffix depends on type of FX effects
    switch (currentFX) {
        case LowLevelMusicProcessor::FILTER:
            strcat(fileNameTo, FX_FILTER_SUFFIX);
            break;
        case LowLevelMusicProcessor::ROLL:
            strcat(fileNameTo, FX_ROLL_SUFFIX);
            break;
        case LowLevelMusicProcessor::ECHO:
            strcat(fileNameTo, FX_ECHO_SUFFIX);
            break;
        case LowLevelMusicProcessor::REVERB:
            strcat(fileNameTo, FX_REVERB_SUFFIX);
            break;
        default:
            strcat(fileNameTo, FX_FLANGER_SUFFIX);
    }

    LOGD("Str: %s'", fileNameTo);
    LOGD("Str len: %d, original std len: %d", strlen(fileNameTo), fileNameFromStrLength);
}

void Utils::prepareSavePath(char *savePath, const char *musicFolderPath) {
    strcpy(savePath, musicFolderPath);
    strcat(savePath, SAVED_FILE_NAME);
    LOGD("current save path: %s", savePath);
}

void Utils::setBoolField(JNIEnv *javaEnvironment,
                         jobject self,
                         jclass thisClass,
                         const char *name,
                         bool value) {
    javaEnvironment->SetBooleanField(self, javaEnvironment->GetFieldID(thisClass, name, "Z"), value);
}