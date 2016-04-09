//
// Created by antonsmusiienko on 09.04.16.
//

#ifndef FREQUENCYDOMAIN_FREQUENCYDOMAIN_H
#define FREQUENCYDOMAIN_FREQUENCYDOMAIN_H

#define  LOG_TAG    "LowLevelMusicProcessor"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)


static const int CHANNELS_NUMBER = 2;

class LowLevelMusicProcessor {
    static SuperpoweredAndroidAudioIO *sAudioInput;
    static SuperpoweredAndroidAudioIO *sAudioOutput;
    static SuperpoweredAdvancedAudioPlayer *sPlayer;

    static const char *sSavePath;
    static float *sFloatBuffer;
    static FILE *sFile;

    int mSampleRate;
    int mBufferSize;

    static bool recordAudioProcessing(void *clientdata, short int *audioInput,
                                      int numberOfSamples,
                                      int samplerate) {
        LOGD("Audio record processing, %d samples", numberOfSamples);
        fwrite(audioInput, sizeof(short int), numberOfSamples * CHANNELS_NUMBER, sFile);
        fflush(LowLevelMusicProcessor::sFile);
        return false;
    }

    static bool outputAudioProcessing(void *clientdata, short int *audioOutput,
                                      int numberOfSamples,
                                      int samplerate) {
        LOGD("Audio output processing");
        if (LowLevelMusicProcessor::sPlayer->process(LowLevelMusicProcessor::sFloatBuffer, false, numberOfSamples)) {
            SuperpoweredFloatToShortInt(LowLevelMusicProcessor::sFloatBuffer, audioOutput, numberOfSamples);
            return true;
        } else return false;
    }

    static void audioPlayEvents(void *clientData, SuperpoweredAdvancedAudioPlayerEvent event,
                                void *value) {
        switch (event) {
            case SuperpoweredAdvancedAudioPlayerEvent_LoadError:
                LOGD("LoadError");
                break;
            case SuperpoweredAdvancedAudioPlayerEvent_LoadSuccess:
                LOGD("LoadSucess");
                break;
            case SuperpoweredAdvancedAudioPlayerEvent_EOF:
                LOGD("EOF");
                break;
            default:
                LOGD("Audio Player events");
        }
    }

    static inline void setBoolField(JNIEnv *javaEnvironment, jobject self, jclass thisClass, const char *name, bool value) {
        javaEnvironment->SetBooleanField(self, javaEnvironment->GetFieldID(thisClass, name, "Z"), value);
    }

public:
    LowLevelMusicProcessor(int samplerate, int buffersize);
    ~LowLevelMusicProcessor();
    void StartRecording();
    void StopRecording();
    void TogglePlayer();
    void UpdateStatus(JNIEnv *javaEnvironment, jobject self);
};

#endif //FREQUENCYDOMAIN_FREQUENCYDOMAIN_H
