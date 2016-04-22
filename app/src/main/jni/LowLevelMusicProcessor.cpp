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
SuperpoweredAdvancedAudioPlayer *LowLevelMusicProcessor::sVoicePlayer;
SuperpoweredAdvancedAudioPlayer *LowLevelMusicProcessor::sSongPlayer;

char *LowLevelMusicProcessor::sLastRecordedFileName;
float *LowLevelMusicProcessor::sFirstBuffer;
float *LowLevelMusicProcessor::sSecondBuffer;
float *LowLevelMusicProcessor::sFinalBuffer;
FILE *LowLevelMusicProcessor::sFile;
bool LowLevelMusicProcessor::sIsVoicePlaybackOn;
bool LowLevelMusicProcessor::sIsAudioOutputSamplesCalculated;
bool LowLevelMusicProcessor::sIsSavedWithFX;
bool LowLevelMusicProcessor::sIsDefaultFlowOn;
int LowLevelMusicProcessor::sNumberOfSamplesAudioOutput;

bool LowLevelMusicProcessor::recordAudioProcessing(void *clientdata, short int *audioInput,
                                  int numberOfSamples, int samplerate) {
    if (sSongPlayer->process(sFirstBuffer, false, numberOfSamples, SONG_VOLUME)) {
        SuperpoweredShortIntToFloat(audioInput, sSecondBuffer, numberOfSamples);
        SuperpoweredCrossMono(sFirstBuffer, sSecondBuffer, sFinalBuffer,
                              DEFAULT_GAIN, DEFAULT_GAIN,
                              DEFAULT_GAIN, DEFAULT_GAIN,
                              numberOfSamples * 2);

        if (sIsDefaultFlowOn) {
            fwrite(audioInput, sizeof(short int), numberOfSamples * CHANNELS_NUMBER, sFile);
        } else {
            SuperpoweredFloatToShortInt(sFinalBuffer, audioInput, numberOfSamples);
            fwrite(audioInput, sizeof(short int), numberOfSamples * CHANNELS_NUMBER, sFile);
        }
        // outputting of song and voice or only song
        SuperpoweredFloatToShortInt(sIsVoicePlaybackOn ?
                                    sFinalBuffer : sFirstBuffer,
                                    audioInput, numberOfSamples);

    return true;
    }
    else return false;
}

bool LowLevelMusicProcessor::outputAudioProcessing(void *clientdata, short int *audioOutput,
                                                   int numberOfSamples,
                                                   int samplerate) {
    // optimization, it helps us to calculate only once
    if (!sIsAudioOutputSamplesCalculated) {
        sNumberOfSamplesAudioOutput = numberOfSamples * 2;
        sIsAudioOutputSamplesCalculated = true;
    }
    if (sVoicePlayer->process(sFirstBuffer, false, numberOfSamples)) {
        if (sSongPlayer->playing) {
            if (sSongPlayer->process(sSecondBuffer, false, numberOfSamples, SONG_VOLUME)) {
                /**
                 * if voice saved with some FX, then process song buffer,
                 * else process two buffers (voice and song)
                 */
                if (sIsDefaultFlowOn) {
                    SuperpoweredCrossMono(sFirstBuffer, sSecondBuffer, sFinalBuffer,
                                          DEFAULT_GAIN, DEFAULT_GAIN,
                                          DEFAULT_GAIN, DEFAULT_GAIN,
                                          sNumberOfSamplesAudioOutput);
                    FXManager::processAllFX(sFinalBuffer, numberOfSamples);
                }
            } else {
                return false;
            }
        } else {
                // if voice doesn't saved with some FX and song doesn't playing, then process only voice
                FXManager::processAllFX(sFirstBuffer, numberOfSamples);
        }
        SuperpoweredFloatToShortInt(sSongPlayer->playing ?
                                    sFinalBuffer : sFirstBuffer,
                                    audioOutput, numberOfSamples);
        return true;
    } else {
        return false;
    }
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
                                               const char *musicFolderPath,
                                               bool isDefaultFlowOn):
        mSampleRate(samplerate),
        mBufferSize(buffersize),
        mIsRecording(false) {
    sIsDefaultFlowOn = isDefaultFlowOn;
    LOGD(sIsDefaultFlowOn ? "on" : "off");
    sIsVoicePlaybackOn = true;
    sIsAudioOutputSamplesCalculated = false;
    sIsSavedWithFX = false;

    mSavePath = new char [strlen(musicFolderPath) + strlen(SAVED_FILE_NAME) + 1];
    mSongPath = new char [strlen(musicFolderPath) + strlen(SONG_FILE_NAME) + 1];
    Utils::prepareNewPath(mSavePath, musicFolderPath, SAVED_FILE_NAME);
    Utils::prepareNewPath(mSongPath, musicFolderPath, SONG_FILE_NAME);

    sFirstBuffer = new float [CHANNELS_NUMBER * mBufferSize + 128];
    sSecondBuffer = new float [CHANNELS_NUMBER * mBufferSize + 128];
    sFinalBuffer = new float [CHANNELS_NUMBER * mBufferSize + 128];

    sAudioInput = new SuperpoweredAndroidAudioIO(mSampleRate, mBufferSize, true, true,
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

    sVoicePlayer = new SuperpoweredAdvancedAudioPlayer(NULL, audioPlayEvents, mSampleRate, 0);
    sSongPlayer = new SuperpoweredAdvancedAudioPlayer(NULL, audioPlayEvents, mSampleRate, 0);

    mFXManager = new FXManager(mSampleRate);
}

void LowLevelMusicProcessor::startRecording() {
    LOGD("Start recording");
    if (sVoicePlayer->playing) {
        togglePlayer();
    }
    if (!mIsRecording) {
        sFile = createWAV(mSavePath, mSampleRate, CHANNELS_NUMBER);

        sSongPlayer->open(mSongPath);
        sSongPlayer->togglePlayback();

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
        sSongPlayer->pause();
        closeWAV(sFile);
        if (fclose(sFile) != 0) {
            LOGE("Error in closing %s", mSavePath);
        }
        sVoicePlayer->open(mSavePath);
        mIsRecording = false;
        sIsSavedWithFX = false;
    } else {
        LOGD("already stopped");
    }
}

void LowLevelMusicProcessor::saveWithEffect() {
    if (sVoicePlayer->playing) {
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
    sIsSavedWithFX = true;
    LOGD("Save with effects completed");
}

void LowLevelMusicProcessor::copyToFile(const char *sourcePath, const char *resultPath, int sampleRate) {
    LOGD("source path: %s, result path: %s", sourcePath, resultPath);
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
        SuperpoweredShortIntToFloat(intBuffer, sFirstBuffer, SAMPLE_RATE);
        numberOfSamplesToProcess = n / 2;
        FXManager::processAllFX(sFirstBuffer, numberOfSamplesToProcess);
        SuperpoweredFloatToShortInt(sFirstBuffer, intBuffer, SAMPLE_RATE);
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
    if (sVoicePlayer->playing || sSongPlayer->playing) {
        sAudioOutput->stop();
        sVoicePlayer->pause();
        sSongPlayer->pause();
        LOGD("Stop");
        return;
    } else {
        if (sIsDefaultFlowOn) {
            chooseCorrectPath();
            sSongPlayer->open(mSongPath);
            sVoicePlayer->play(false);
            sSongPlayer->play(false);
            LOGD("Default flow on");
        } else {
            chooseCorrectPath();
            sVoicePlayer->play(false);
            LOGD("Default flow off");
        }
        sAudioOutput->start();
        LOGD("Play");
    }
}

void LowLevelMusicProcessor::chooseCorrectPath() {
    if (sIsSavedWithFX) {
        if (sLastRecordedFileName != NULL) {
            sVoicePlayer->open(sLastRecordedFileName);
        } else {
            LOGE("last recorded file name == NULL");
        }
    } else {
        if (mSavePath != NULL) {
            sVoicePlayer->open(mSavePath);
        } else {
            LOGE("save path == NULL");
        }
    }
}

void LowLevelMusicProcessor::toggleVoicePlayback() {
    LOGD("is voice playback on: %s", sIsVoicePlaybackOn ? "true" : "false");
    sIsVoicePlaybackOn = !sIsVoicePlaybackOn;
}

void LowLevelMusicProcessor::updateStatus(JNIEnv *javaEnvironment, jobject self) {
    jclass thisClass = javaEnvironment->GetObjectClass(self);
    Utils::setBoolField(javaEnvironment, self, thisClass, "isPlaying", sVoicePlayer->playing);
    Utils::setBoolField(javaEnvironment, self, thisClass, "isRecording", mIsRecording);
}

void LowLevelMusicProcessor::setTypeFX(int value) {
    mFXManager->setFxType(value);
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
    delete sVoicePlayer;
    delete sSongPlayer;
    delete mFXManager;

    // clear resources
    delete []sLastRecordedFileName;
    delete []mSavePath;
    delete []mSongPath;
    delete []sFirstBuffer;
    delete []sSecondBuffer;
    delete []sFinalBuffer;
}