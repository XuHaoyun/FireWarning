package com.example.mqtt;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Intent;
import android.os.Bundle;
import android.os.CountDownTimer;
import android.widget.TextView;

public class StartActivity extends AppCompatActivity {
    private TextView textView;
    private CountDownTimer countDownTimer;
    private long timeLeftInMillis = 500;

    //通过调用setContentView()方法将布局文件（R.layout.activity_start）与Activity关联起来
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_start);
        startCountDown();
    }
    //在onFinish()方法中，当倒计时结束时，会启动一个新的Activity（MainActivity）并关闭当前的StartActivity
    private void startCountDown() {
        countDownTimer = new CountDownTimer(timeLeftInMillis,500){
            @Override
            public void onTick(long millisUntilFinished) {
            }

            @Override
            public void onFinish() {
                startActivity(new Intent(StartActivity.this,MainActivity.class));
                finish();
            }
        }.start();
    }

    //当Activity被销毁时，会取消倒计时器的计时任务，以防止内存泄漏
    @Override
    protected void onDestroy() {
        super.onDestroy();
        if(countDownTimer != null){
            countDownTimer.cancel();
        }
    }
}