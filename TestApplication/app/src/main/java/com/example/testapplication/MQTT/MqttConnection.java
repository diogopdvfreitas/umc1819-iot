package com.example.testapplication.MQTT;

import android.content.Context;
import android.util.Log;

import org.eclipse.paho.android.service.MqttAndroidClient;
import org.eclipse.paho.client.mqttv3.DisconnectedBufferOptions;
import org.eclipse.paho.client.mqttv3.IMqttActionListener;
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.IMqttToken;
import org.eclipse.paho.client.mqttv3.MqttCallbackExtended;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;

public class MqttConnection
{
    private final String clientId = "Francisco";
    private final String serverUri = "tcp://192.168.43.202:1883";

    private final String username = "android";
    private final String password = "smartphone";

    final String subscriptionTopic = "/laterator/beacons/+/+";

    public MqttAndroidClient mqttAndroidClient;

    public MqttConnection(Context context)
    {
        mqttAndroidClient = new MqttAndroidClient(context, serverUri, clientId);

        /*Get notified when events ocurr for example:
        *   A message recieved
        *   A lost connection
        *   A message sent
        * */
        mqttAndroidClient.setCallback(new MqttCallbackExtended()
        {
            @Override
            public void connectComplete(boolean reconnect, String serverURI)
            {
                Log.w("mqtt", serverURI);
            }

            @Override
            public void connectionLost(Throwable cause)
            {

            }

            @Override
            public void messageArrived(String topic, MqttMessage message) throws Exception
            {
                Log.w("Mqtt", message.toString());
            }

            @Override
            public void deliveryComplete(IMqttDeliveryToken token)
            {

            }
        });

        /*Connect to the broker*/
        connect();
    }

    public void setCallback(MqttCallbackExtended callback)
    {
        mqttAndroidClient.setCallback(callback);
    }

    public void connect()
    {
        MqttConnectOptions mqttConnectOptions = new MqttConnectOptions();

        mqttConnectOptions.setAutomaticReconnect(true);
        mqttConnectOptions.setCleanSession(false);
        mqttConnectOptions.setUserName(username);
        mqttConnectOptions.setPassword(password.toCharArray());

        try
        {
            mqttAndroidClient.connect(mqttConnectOptions, null, new IMqttActionListener()
            {
                @Override
                public void onSuccess(IMqttToken asyncActionToken)
                {
                    DisconnectedBufferOptions disconnectedBufferOptions = new DisconnectedBufferOptions();
                    disconnectedBufferOptions.setBufferEnabled(true);
                    disconnectedBufferOptions.setBufferSize(100);
                    disconnectedBufferOptions.setPersistBuffer(false);
                    disconnectedBufferOptions.setDeleteOldestMessages(false);
                    mqttAndroidClient.setBufferOpts(disconnectedBufferOptions);
                    subscribeToTopic();
                }

                @Override
                public void onFailure(IMqttToken asyncActionToken, Throwable exception)
                {
                    Log.w("Mqtt", "Failed to connect to: " + serverUri + exception.toString());
                }
            });
        }
        catch (MqttException ex)
        {
            ex.printStackTrace();
        }
    }

    public void subscribeToTopic()
    {
        try
        {
            mqttAndroidClient.subscribe(subscriptionTopic, 0, null, new IMqttActionListener()
            {
                @Override
                public void onSuccess(IMqttToken asyncActionToken)
                {
                    Log.w("Mqtt","Subscribed!");
                }

                @Override
                public void onFailure(IMqttToken asyncActionToken, Throwable exception)
                {
                    Log.w("Mqtt", "Subscribed fail!");
                }
            });
        }
        catch (MqttException ex)
        {
            System.err.println("Exception whilst subscribing");
            ex.printStackTrace();
        }
    }
}
