package com.superpowered.frequencydomain;

import android.app.Activity;
import android.content.Context;
import android.media.AudioManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.view.View;
import android.widget.Button;


public class MainActivity extends Activity {

    private static final int UI_UPDATER_DELAY = 50;

    private Button mStopRecordButton;
    private Button mStartRecordButton;
    private Button mPlaySongButton;

    private Handler mHandler;

    private boolean isPlaying;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Runnable UIUpdater = new Runnable() {
            @Override
            public void run() {
                UpdateStatus();
                mPlaySongButton.setText(isPlaying ? R.string.pause_song : R.string.play_song);
                mHandler.postDelayed(this, UI_UPDATER_DELAY);
            }
        };
        mHandler = new Handler();
        mHandler.postDelayed(UIUpdater, UI_UPDATER_DELAY);

        mStopRecordButton = (Button) findViewById(R.id.stop_record_button);
        mStopRecordButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                StopRecord();
            }
        });

        mStartRecordButton = (Button) findViewById(R.id.start_record_button);
        mStartRecordButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                StartRecord();
            }
        });

        mPlaySongButton = (Button) findViewById(R.id.play_song_button);
        mPlaySongButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                TogglePlayer();
            }
        });

        // Get the device's sample rate and buffer size to enable low-latency Android audio output, if available.
        String samplerateString = null, buffersizeString = null;
        if (Build.VERSION.SDK_INT >= 17) {
            AudioManager audioManager = (AudioManager) this.getSystemService(Context.AUDIO_SERVICE);
            samplerateString = audioManager.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE);
            buffersizeString = audioManager.getProperty(AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER);
        }
        if (samplerateString == null) samplerateString = "44100";
        if (buffersizeString == null) buffersizeString = "512";

        String path = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_MUSIC).getAbsolutePath();

        System.loadLibrary("FrequencyDomain");
        Init(Integer.parseInt(samplerateString), Integer.parseInt(buffersizeString), path);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Cleanup();
    }

    private native void Init(long samplerate, long buffersize, String path);
    private native void StartRecord();
    private native void StopRecord();
    private native void TogglePlayer();
    private native void UpdateStatus();
    private native void Cleanup();
}
