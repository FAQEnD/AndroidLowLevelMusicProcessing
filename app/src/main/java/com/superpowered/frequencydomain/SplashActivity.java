package com.superpowered.frequencydomain;

import android.content.Intent;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.View;

public class SplashActivity extends AppCompatActivity {

    public static final String FLOW_TYPE = "flow_type";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_splash);
        final Intent intent = new Intent(this, MainActivity.class);

        findViewById(R.id.default_flow_button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                intent.putExtra(FLOW_TYPE, true);
                startActivity(intent);
            }
        });

        findViewById(R.id.addition_flow_button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                intent.putExtra(FLOW_TYPE, false);
                startActivity(intent);
            }
        });
    }
}
