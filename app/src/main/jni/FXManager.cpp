//
// Created by antonsmusiienko on 13.04.16.
//

#include "FXManager.h"
#include "Utils.h"

SuperpoweredRoll *FXManager::sRoll;
SuperpoweredFilter *FXManager::sFilter;
SuperpoweredFlanger *FXManager::sFlanger;
SuperpoweredEcho *FXManager::sEcho;
SuperpoweredReverb *FXManager::sReverb;


/**
 * Audio FX
 */

FXManager::FXManager(int sampleRate): mCurrentFX(CurrentFX::FLANGER) {
    sRoll = new SuperpoweredRoll(sampleRate);
    sFilter = new SuperpoweredFilter(SuperpoweredFilter_Resonant_Lowpass, sampleRate);
    sFlanger = new SuperpoweredFlanger(sampleRate);
    sEcho = new SuperpoweredEcho(sampleRate);
    sReverb = new SuperpoweredReverb(sampleRate);
}

void FXManager::setFxType(int value) {
    mCurrentFX = (CurrentFX) value;
    LOGD("CurrentFX: %d", mCurrentFX);
}

FXManager::CurrentFX FXManager::getFxValue() {
    return mCurrentFX;
}

void FXManager::onFxValue(int iValue) {
    float value = (float) iValue * 0.01f;
    // TODO: investigate how to to decrease calls of offAllFX() method
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

void FXManager::processAllFX(float *sStereoBuffer, int numberOfSamples) {
    sFlanger->process(sStereoBuffer, sStereoBuffer, numberOfSamples);
    sFilter->process(sStereoBuffer, sStereoBuffer, numberOfSamples);
    sRoll->process(sStereoBuffer, sStereoBuffer, numberOfSamples);
    sEcho->process(sStereoBuffer, sStereoBuffer, numberOfSamples);
    sReverb->process(sStereoBuffer, sStereoBuffer, numberOfSamples);
}

void FXManager::offAllFX() {
    LOGD("Off all FX");
    sRoll->enable(false);
    sFilter->enable(false);
    sFlanger->enable(false);
    sEcho->enable(false);
    sReverb->enable(false);
}

const char *FXManager::getActualFXSuffix(CurrentFX currentFX) {
    return FX_SUFFIX_LIST[(int) currentFX];
}

FXManager::~FXManager() {
    // clear FX pointers
    delete sFlanger;
    delete sFilter;
    delete sRoll;
    delete sEcho;
    delete sReverb;
}