package com.superpowered.frequencydomain;

import android.app.Activity;
import android.content.Context;
import android.media.AudioManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.SeekBar;
import android.widget.ToggleButton;


public class MainActivity extends Activity {
    private static final String TAG = MainActivity.class.getSimpleName();

    private static final int UI_UPDATER_DELAY = 50;

    private Button mStartRecordButton;
    private Button mPlaySongButton;
    private Button mSaveButton;
    private ToggleButton mVoicePlaybackToggleButton;
    private SeekBar mSeekBar;
    private RadioGroup mRadioGroup;
    private boolean mIsDefaultFlowOn;

    private Handler mHandler;


    /**
     * this values come from native side
     *
     * @see #UpdateStatus()
     */
    private boolean isPlaying;
    private boolean isRecording;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        initUI();
        Bundle extras = getIntent().getExtras();
        if (extras != null) {
            mIsDefaultFlowOn = extras.getBoolean(SplashActivity.FLOW_TYPE);
            Log.d(TAG, "is default flow on: " + String.valueOf(mIsDefaultFlowOn));
        }

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
        Log.d(TAG, path);
        System.loadLibrary("FrequencyDomain");

        Init(Integer.parseInt(samplerateString), Integer.parseInt(buffersizeString), path, mIsDefaultFlowOn);
    }

    private void initUI() {
        setContentView(R.layout.activity_main);

        Runnable UIUpdater = new Runnable() {
            @Override
            public void run() {
                UpdateStatus();
                UpdateUI();
                mHandler.postDelayed(this, UI_UPDATER_DELAY);
            }
        };
        mHandler = new Handler();
        mHandler.postDelayed(UIUpdater, UI_UPDATER_DELAY);

        mStartRecordButton = (Button) findViewById(R.id.control_record_button);
        mStartRecordButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (!isRecording) {
                    StartRecord();
                } else {
                    StopRecord();
                }
            }
        });

        mPlaySongButton = (Button) findViewById(R.id.control_song_play_button);
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

        mVoicePlaybackToggleButton = (ToggleButton) findViewById(R.id.voice_playback_toggle);
        mVoicePlaybackToggleButton.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                ToggleVoicePlayback();
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

    private void UpdateUI() {
        mPlaySongButton.setText(isPlaying ? R.string.pause_song : R.string.play_song);
        mStartRecordButton.setText(isRecording ? R.string.stop_record : R.string.start_record);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Cleanup();
    }

    private native void Init(long samplerate, long buffersize, String path, boolean isDefaultFlowOn);

    private native void StartRecord();

    private native void StopRecord();

    private native void SaveWithEffect();

    private native void TogglePlayer();

    private native void ToggleVoicePlayback();

    private native void UpdateStatus();

    private native void OnFxSelect(int value);

    private native void OnFxValue(int value);

    private native void OnFxOff();

    private native void Cleanup();
}
