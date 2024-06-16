package com.example.mqtt;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.NotificationCompat;

import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence;
import org.json.JSONException;
import org.json.JSONObject;

//后端逻辑
public class MainActivity extends AppCompatActivity {
    private Handler handler;
    private String serverUri = "tcp://broker.emqx.io:1883";
    /*
     * 这里可以填上各种云平台的物联网云平的域名+1883端口号，什么阿里云腾讯云百度云天工物接入都可以，
     * 这里我填的是我在我的阿里云服务器上搭建的EMQ平台的地址，
     * 注意：前缀“tcp：//”不可少，之前我没写，怎么都连不上，折腾了好久
     */

    private String userName = " ";
    private String passWord = " ";
    private String clientId = "app" + System.currentTimeMillis(); // clientId很重要，不能重复，否则就会连不上，所以我定义成 app+当前时间
    private String mqtt_sub_topic = "/monitor51/post";            // 需要订阅的主题
    private String mqtt_pub_topic = "/monitorA/post";             // 需要发布的主题
    private MqttClient mqtt_client;                               // 创建一个mqtt_client对象
    MqttConnectOptions options;

    private TextView air_temp;
    private TextView air_humi;
    private TextView light;
    private TextView smog_humidity;

    // 按钮的背景
    private LinearLayout linearLayout1;
    private LinearLayout linearLayout2;
    private TextView switch_t1;
    private TextView switch_t2;

    private NotificationManager systemService;
    private Notification notification;

    // 选择框
    private Switch switch1;
    private Switch switch2;

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity2);

        /*绑定UI文件*/
        UI_Init();
        mqtt_init_Connect();/*初始化连接函数*/
        sendManageData("欢迎使用环境检测系统");

        /*回调函数，数据会在回调函数里面*/
        mqtt_client.setCallback(new MqttCallback() {
            @Override
            public void connectionLost(Throwable throwable) {
            }

            @Override
            public void messageArrived(String s, MqttMessage mqttMessage) throws Exception {
                /*数据到这了*/
                final String msg = new String(mqttMessage.getPayload());
                Log.d("MQTTRCV", msg);//日志信息，可以查看
                System.out.println(msg);/*打印得到的数据*/
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        // 开始解析数据
                        // 数据解析部分这里使用的是Json解析
                        // 为了避免闪退，使用try来处理异常情况
                        try {
                            JSONObject jsonObject = new JSONObject(String.valueOf(msg));

                            int currentTemperature = jsonObject.getInt("Temperature");
                            int currentHumidity = jsonObject.getInt("Humidity");
                            int Illumination = jsonObject.getInt("Illumination");

                            // 兼容普通版本
                            try {
                                int smogHumidity = jsonObject.getInt("smog");
                                smog_humidity.setText("烟雾:" + String.valueOf(smogHumidity) + "%");
                            } catch (JSONException e) {
                                smog_humidity.setText("烟雾:00%");
                                e.printStackTrace();
                            }

                            // 更新数据到UI
                            air_temp.setText("温度：" + String.valueOf(currentTemperature) + "℃");
                            air_humi.setText("湿度:" + String.valueOf(currentHumidity) + "%");
                            light.setText("光照:" + String.valueOf(Illumination) + "%");
                        } catch (JSONException e) {
                            e.printStackTrace();
                            Toast.makeText(MainActivity.this, "数据解析异常！", Toast.LENGTH_LONG).show();
                        }
                    }
                });
            }

            @Override
            public void deliveryComplete(IMqttDeliveryToken iMqttDeliveryToken) {
            }
        });
        switch1.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean isCheck) {

                if (isCheck) {
                    switch_t1.setText("开");
                    linearLayout1.setBackgroundColor(Color.parseColor("#00BBFF"));
                    if (!compoundButton.isPressed()) {
                        return;
                    }
                    new Thread(new Runnable() {
                        @Override
                        public void run() {
                            PublishMessage(mqtt_pub_topic, "{\"LED\":1}");
                        }
                    }).start();
                    return;
                }
                switch_t1.setText("关");
                linearLayout1.setBackgroundColor(Color.parseColor("#6495ED"));
                if (!compoundButton.isPressed()) {
                    return;
                }
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        PublishMessage(mqtt_pub_topic, "{\"LED\":0}");
                    }
                }).start();
            }
        });
        switch2.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean isCheck) {

                if (isCheck) {
                    switch_t2.setText("开");
                    linearLayout2.setBackgroundColor(Color.parseColor("#00BBFF"));
                    if (!compoundButton.isPressed()) {
                        return;
                    }
                    new Thread(new Runnable() {
                        @Override
                        public void run() {
                            PublishMessage(mqtt_pub_topic, "{\"FAN\":1}");

                        }
                    }).start();
                    return;
                }
                switch_t2.setText("关");
                linearLayout2.setBackgroundColor(Color.parseColor("#6495ED"));
                if (!compoundButton.isPressed()) {
                    return;
                }
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        PublishMessage(mqtt_pub_topic, "{\"FAN\":0}");
                    }

                }).start();
            }
        });
    }

    public void mqtt_init_Connect() {
        try {
            //实例化mqtt_client，填入我们定义的serverUri和clientId，然后MemoryPersistence设置clientid的保存形式，默认为以内存保存
            mqtt_client = new MqttClient(serverUri, clientId, new MemoryPersistence());
            //创建并实例化一个MQTT的连接参数对象
            options = new MqttConnectOptions();
            //然后设置对应的参数
            options.setUserName(userName);                  //设置连接的用户名
            options.setPassword(passWord.toCharArray());    //设置连接的密码
            options.setConnectionTimeout(30);               //设置超时时间，单位为秒
            options.setKeepAliveInterval(50);               //设置心跳,30s
            options.setAutomaticReconnect(true);            //是否重连
            //设置是否清空session,设置为false表示服务器会保留客户端的连接记录，设置为true表示每次连接到服务器都以新的身份连接
            options.setCleanSession(false);
            /*初始化成功之后就开始连接*/
            connect();
            Toast.makeText(MainActivity.this, "连接成功", Toast.LENGTH_SHORT).show();
        } catch (Exception e) {
            e.printStackTrace();
            Toast.makeText(MainActivity.this, "失败", Toast.LENGTH_LONG).show();
        }
    }

    public void connect() {
        //连接mqtt服务器
        try {
            mqtt_client.connect(options);
            mqtt_client.subscribe(mqtt_sub_topic);
            Toast.makeText(this, "开始建立连接.....！", Toast.LENGTH_SHORT).show();
        } catch (Exception e) {
            e.printStackTrace();
            Log.d("MQTTCon", "mqtt连接失败");
            Toast.makeText(this, "mqtt连接失败！", Toast.LENGTH_LONG).show();
        }
    }

    /*绑定各个UI控件*/
    public void UI_Init() {
        switch1 = findViewById(R.id.check1);
        switch2 = findViewById(R.id.check2);

        air_temp = findViewById(R.id.data1);    // 空气温度
        air_humi = findViewById(R.id.data2);    // 空气湿度
        light = findViewById(R.id.data3);        // 光照强度
        smog_humidity = findViewById(R.id.data4);        // 二氧化碳

        linearLayout1 = findViewById(R.id.blank1);
        linearLayout2 = findViewById(R.id.blank2);

        switch_t1 = findViewById(R.id.switch_t1);
        switch_t2 = findViewById(R.id.switch_t2);
    }

    /*发布函数！*/
    private void PublishMessage(String topic, String Message2) {
        if (mqtt_client == null || !mqtt_client.isConnected()) {
            return;
        }
        MqttMessage message = new MqttMessage();
        message.setPayload(Message2.getBytes());
        try {
            mqtt_client.publish(topic, message);

        } catch (MqttException e) {
            e.printStackTrace();
        }
    }

    // 发送官方通知
    private void sendManageData(String data) {
        systemService = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel channel = new NotificationChannel("leo", "测试通知",
                    NotificationManager.IMPORTANCE_HIGH);
            systemService.createNotificationChannel(channel);
        }
        notification = new NotificationCompat.Builder(this, "leo")
                .setContentTitle("官方通知")
                .setContentText(data)
                .setSmallIcon(R.drawable.baseline_arrow_upward_24)
                .build();
        systemService.notify(11, notification);
    }

    // button点击的页面绘制
    public void popupWindow(View view) {
        View popupView = getLayoutInflater().inflate(R.layout.popwindow, null);
        // popWindow弹出框
        Button query = popupView.findViewById(R.id.edit_query1);
        Button cancel = popupView.findViewById(R.id.edit_cancel);

        // 设置按钮
        Button edit1 = popupView.findViewById(R.id.edit1);
        Button edit2 = popupView.findViewById(R.id.edit2);
        Button edit3 = popupView.findViewById(R.id.edit3);
        Button edit4 = popupView.findViewById(R.id.edit4);


        // 第一个位置获取
        EditText edit1_up = popupView.findViewById(R.id.edit1_input_up);
        EditText edit1_down = popupView.findViewById(R.id.edit1_input_down);
        // 第二个位置获取
        EditText edit2_up = popupView.findViewById(R.id.edit2_input_up);
        EditText edit2_down = popupView.findViewById(R.id.edit2_input_down);
        // 第三个位置获取
        EditText edit3_up = popupView.findViewById(R.id.edit3_input_up);
        EditText edit3_down = popupView.findViewById(R.id.edit3_input_down);
        // 第四个位置获取
        EditText edit4_up = popupView.findViewById(R.id.edit4_input_up);
        EditText edit4_down = popupView.findViewById(R.id.edit4_input_down);

        DisplayMetrics displayMetrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
        int popupWidth = (int) (displayMetrics.widthPixels);
        int popupHeight = (int) (displayMetrics.heightPixels * 0.75);


        PopupWindow popupWindow = new PopupWindow(popupView, popupWidth, popupHeight, true);
        popupWindow.setAnimationStyle(R.style.popwindowShow);
        popupWindow.setBackgroundDrawable(getResources().getDrawable(R.drawable.ok));
        popupWindow.showAtLocation(MainActivity.this.getWindow().getDecorView(), Gravity.CENTER, 0, 0);

        new Thread(new Runnable() {
            @Override
            public void run() {
                // 设置一按钮按下
                edit1.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        Toast.makeText(popupView.getContext(), "温度设置成功", Toast.LENGTH_SHORT).show();
                        PublishMessage(mqtt_pub_topic, "{\"E_TEMP\":1," + "\"temp_up\":" + edit1_up.getText().toString() +
                                ",\"temp_down\":" + edit1_down.getText().toString() + "}");
                    }
                });
                edit2.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        Toast.makeText(popupView.getContext(), "湿度设置成功", Toast.LENGTH_SHORT).show();
                        PublishMessage(mqtt_pub_topic, "{\"E_HUM\":1," + "\"hum_up\":" + edit2_up.getText().toString() +
                                ",\"hum_down\":" + edit2_down.getText().toString() + "}");
                    }
                });
                edit3.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        Toast.makeText(popupView.getContext(), "光照设置成功", Toast.LENGTH_SHORT).show();
                        PublishMessage(mqtt_pub_topic, "{\"E_SUN\":1," + "\"sun_up\":" + edit3_up.getText().toString() +
                                ",\"sun_down\":" + edit3_down.getText().toString() + "}");
                    }
                });
                edit4.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        Toast.makeText(popupView.getContext(), "烟雾浓度设置成功", Toast.LENGTH_SHORT).show();
                        PublishMessage(mqtt_pub_topic, "{\"E_SMOG\":1," + "\"smog_up\":" + edit4_up.getText().toString() +
                                ",\"smog_down\":" + edit4_down.getText().toString() + "}");
                    }
                });

                query.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        Toast.makeText(MainActivity.this, "开始上传数据", Toast.LENGTH_SHORT).show();
                        sendManageData("正在上传数据");
                        popupWindow.dismiss();
                    }
                });
                cancel.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        Toast.makeText(MainActivity.this, "取消上传数据", Toast.LENGTH_SHORT).show();
                        popupWindow.dismiss();
                    }
                });
            }
        }).start();
    }
}

