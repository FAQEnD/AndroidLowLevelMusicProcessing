package com.superpowered.frequencydomain;

import android.app.Activity;
import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.media.AudioManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.view.View;
import android.widget.Button;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.SeekBar;

import java.io.IOException;


public class MainActivity extends Activity {

    private static final int UI_UPDATER_DELAY = 50;

    private Button mStopRecordButton;
    private Button mStartRecordButton;
    private Button mPlaySongButton;
    private Button mSaveButton;
    private SeekBar mSeekBar;
    private RadioGroup mRadioGroup;

    private Handler mHandler;

    // this values come from native side
    private boolean isPlaying;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        initUI();

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

        AssetFileDescriptor musicFile = getResources().openRawResourceFd(R.raw.lycka);
        long startOffset = musicFile.getStartOffset();
        long lenght = musicFile.getLength();

        try {
            musicFile.close();
        } catch (IOException e) {
            e.printStackTrace();
        }

        System.loadLibrary("FrequencyDomain");
        Init(Integer.parseInt(samplerateString), Integer.parseInt(buffersizeString), path, startOffset, lenght);
    }

    private void initUI() {
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

        mSaveButton = (Button) findViewById(R.id.save_with_effect_button);
        mSaveButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                SaveWithEffect();
            }
        });

        mSeekBar = (SeekBar) findViewById(R.id.fx_fader);
        mSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                OnFxValue(progress);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                OnFxValue(seekBar.getProgress());
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                OnFxOff();
            }
        });

        mRadioGroup = (RadioGroup) findViewById(R.id.fx_effects_radio_group);
        mRadioGroup.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup group, int checkedId) {
                RadioButton checkedRadioButton = (RadioButton) findViewById(checkedId);
                OnFxSelect(mRadioGroup.indexOfChild(checkedRadioButton));
            }
        });
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Cleanup();
    }

    private native void Init(long samplerate, long buffersize, String path, long startOffset, long length);

    private native void StartRecord();

    private native void StopRecord();

    private native void SaveWithEffect();

    private native void TogglePlayer();

    private native void UpdateStatus();

    private native void OnFxSelect(int value);

    private native void OnFxValue(int value);

    private native void OnFxOff();

    private native void Cleanup();
}
