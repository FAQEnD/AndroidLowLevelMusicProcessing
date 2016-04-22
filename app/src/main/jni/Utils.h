//
// Created by antonsmusiienko on 13.04.16.
//

#ifndef FREQUENCYDOMAIN_UTILS_H
#define FREQUENCYDOMAIN_UTILS_H
#include <android/log.h>

#include "LowLevelMusicProcessor.h"
#include "FXManager.h"

static const int SIZE_OF_FILE_TYPE_SUFFIX = 4;

static const float MINFREQ = 60.0f;
static const float MAXFREQ = 20000.0f;

class Utils {
public:
    static float floatToFrequency(float value);

    static void createFileName(const char *fileNameFrom, char *fileNameTo, FXManager::CurrentFX currentFX);

    static void prepareNewPath(char *savePath, const char *musicFolderPath, const char *SUFFIX_TO_ADD);

    static void setBoolField(JNIEnv *javaEnvironment,
                                    jobject self,
                                    jclass thisClass,
                                    const char *name,
                                    bool value);
};


#endif //FREQUENCYDOMAIN_UTILS_H
