//
// Created by antonsmusiienko on 13.04.16.
//

#ifndef FREQUENCYDOMAIN_UTILS_H
#define FREQUENCYDOMAIN_UTILS_H
#include <android/log.h>

#include "LowLevelMusicProcessor.h"

//#define  LOG_TAG_UTILS    "Utils"
//#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_UTILS, __VA_ARGS__)

static const int SIZE_OF_FILE_TYPE_SUFFIX = 4;

static const float MINFREQ = 60.0f;
static const float MAXFREQ = 20000.0f;

static const char *FX_FLANGER_SUFFIX = "_flanger.wav";
static const char *FX_FILTER_SUFFIX = "_filter.wav";
static const char *FX_ROLL_SUFFIX = "_roll.wav";
static const char *FX_ECHO_SUFFIX = "_echo.wav";
static const char *FX_REVERB_SUFFIX = "_reverb.wav";

class Utils {
public:
    static float floatToFrequency(float value);

    static void createFileName(const char *fileNameFrom, char *fileNameTo, LowLevelMusicProcessor::CurrentFX currentFX);

    static void prepareSavePath(char *savePath, const char *musicFolderPath);

    static void setBoolField(JNIEnv *javaEnvironment,
                                    jobject self,
                                    jclass thisClass,
                                    const char *name,
                                    bool value);
};


#endif //FREQUENCYDOMAIN_UTILS_H
