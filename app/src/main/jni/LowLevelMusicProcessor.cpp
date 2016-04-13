#include "LowLevelMusicProcessor.h"

#include "Superpowered/SuperpoweredFrequencyDomain.h"
#include "Superpowered/SuperpoweredSimple.h"
#include "Superpowered/SuperpoweredRecorder.h"

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>

#include "Utils.h"
#include "FXManager.h"

/**
 * LowLevelMusicProcessor init of static members
 */
SuperpoweredAndroidAudioIO *LowLevelMusicProcessor::sAudioInput;
SuperpoweredAndroidAudioIO *LowLevelMusicProcessor::sAudioOutput;
SuperpoweredAdvancedAudioPlayer *LowLevelMusicProcessor::sPlayer;

char *LowLevelMusicProcessor::sLastRecordedFileName;
float *LowLevelMusicProcessor::sStereoBuffer;
FILE *LowLevelMusicProcessor::sFile;
bool LowLevelMusicProcessor::sIsVoicePlaybackOn;

bool LowLevelMusicProcessor::recordAudioProcessing(void *clientdata, short int *audioInput,
                                  int numberOfSamples, int samplerate) {

    fwrite(audioInput, sizeof(short int), numberOfSamples * CHANNELS_NUMBER, sFile);
    fflush(sFile);

    return sIsVoicePlaybackOn;
}

bool LowLevelMusicProcessor::outputAudioProcessing(void *clientdata, short int *audioOutput,
                                                   int numberOfSamples,
                                                   int samplerate) {
    if (sPlayer->process(sStereoBuffer, false, numberOfSamples)) {

        FXManager::processAllFX(sStereoBuffer, numberOfSamples);

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
        mIsRecording(false) {
    sIsVoicePlaybackOn = true;

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

    mFXManager = new FXManager(mSampleRate);
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
    Utils::createFileName(mSavePath, sLastRecordedFileName, mFXManager->getFxValue());
    mFXManager->onFxValue(ON_SAVE_WITH_FX_VALUE);
    copyToFile(mSavePath, sLastRecordedFileName, mSampleRate);
    mFXManager->offAllFX();
}

void LowLevelMusicProcessor::copyToFile(const char *sourcePath, const char *resultPath, int sampleRate) {
    FILE *sourceFile = fopen(sourcePath, "r");
    FILE *resultFile = createWAV(resultPath, sampleRate, CHANNELS_NUMBER);

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

    // skip first 44 bites of WAV Header
    fseek(sourceFile, SIZE_OF_WAV_HEADER, SEEK_SET);

    if (intBuffer == NULL) {
        intBuffer = new short int[SAMPLE_RATE * CHANNELS_NUMBER];
    }

    while ((n = fread(intBuffer, sizeof(short int), SAMPLE_RATE, sourceFile)) > 0)
    {
        SuperpoweredShortIntToFloat(intBuffer, sStereoBuffer, SAMPLE_RATE);

        numberOfSamplesToProcess = n / 2;
        FXManager::processAllFX(sStereoBuffer, numberOfSamplesToProcess);

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

void LowLevelMusicProcessor::toggleVoicePlayback() {
    LOGD("is voice playback on: %s", sIsVoicePlaybackOn ? "true" : "false");
    sIsVoicePlaybackOn = !sIsVoicePlaybackOn;
}

void LowLevelMusicProcessor::updateStatus(JNIEnv *javaEnvironment, jobject self) {
    jclass thisClass = javaEnvironment->GetObjectClass(self);
    Utils::setBoolField(javaEnvironment, self, thisClass, "isPlaying", sPlayer->playing);
    Utils::setBoolField(javaEnvironment, self, thisClass, "isRecording", mIsRecording);
}

void LowLevelMusicProcessor::setFX(int value) {
    mFXManager->setFxValue(value);
}

void LowLevelMusicProcessor::onFX(int value) {
    mFXManager->onFxValue(value);
}

void LowLevelMusicProcessor::offFX() {
    mFXManager->offAllFX();
}

LowLevelMusicProcessor::~LowLevelMusicProcessor() {
    // clear AudioIO, player and recorder pointers
    delete sAudioInput;
    delete sAudioOutput;
    delete sPlayer;
    delete mFXManager;

    // clear resources
    delete []sLastRecordedFileName;
    delete []mSavePath;
    delete []sStereoBuffer;
}