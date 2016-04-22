//
// Created by antonsmusiienko on 13.04.16.
//

#ifndef FREQUENCYDOMAIN_FXMANAGER_H
#define FREQUENCYDOMAIN_FXMANAGER_H

#include "Superpowered/SuperpoweredRoll.h"
#include "Superpowered/SuperpoweredFilter.h"
#include "Superpowered/SuperpoweredFlanger.h"
#include "Superpowered/SuperpoweredEcho.h"
#include "Superpowered/SuperpoweredReverb.h"

static const char *FX_SUFFIX_LIST[] = {
        "_flanger.wav",
        "_filter.wav",
        "_roll.wav",
        "_echo.wav",
        "_reverb.wav"
};

class FXManager {
public:
    enum CurrentFX {
        FLANGER,
        FILTER,
        ROLL,
        ECHO,
        REVERB
    };

    // FX effects
    static SuperpoweredRoll *sRoll;
    static SuperpoweredFilter *sFilter;
    static SuperpoweredFlanger *sFlanger;
    static SuperpoweredEcho *sEcho;
    static SuperpoweredReverb *sReverb;

    CurrentFX mCurrentFX;

    FXManager(int sampleRate);

    // Audio FX
    void setFxType(int value);

    CurrentFX getFxValue();

    void onFxValue(int iValue);

    static void processAllFX(float *sStereoBuffer, int numberOfSamples);

    void offAllFX();

    static const char *getActualFXSuffix(CurrentFX currentFX);

    ~FXManager();
};


#endif //FREQUENCYDOMAIN_FXMANAGER_H
