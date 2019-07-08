package com.example.testapplication;


import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;

import com.example.testapplication.CustomViews.BeaconInfo;
import com.example.testapplication.CustomViews.DrawSquares;
import com.example.testapplication.MQTT.MqttConnection;

import org.altbeacon.beacon.BeaconManager;
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallbackExtended;
import org.eclipse.paho.client.mqttv3.MqttMessage;

import java.util.ArrayList;

public class MainActivity extends AppCompatActivity /*implements BeaconConsumer*/
{
    ArrayList<String> beaconUUID = new ArrayList<>();
    ArrayList<BeaconInfo>  beacons = new ArrayList<>();
    //ArrayList<DrawSquares> beacons = new ArrayList<>();
    int lastActiveState;
    int systemTime;
    int index;
    int noBeacons = 0;


    private BeaconManager beaconManager = null;

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        final DrawSquares drawSquares = new DrawSquares(this);


        MqttConnection mqttConnection = new MqttConnection(this);

        mqttConnection.setCallback(new MqttCallbackExtended()
        {
            @Override
            public void connectComplete(boolean reconnect, String serverURI)
            {

            }

            @Override
            public void connectionLost(Throwable cause)
            {

            }

            @Override
            public void messageArrived(String topic, MqttMessage message) throws Exception
            {
                /*Split the full topic structure into sections, parts[3] is the UUID and parts[4] is
                the topic subject*/
                String parts[] = topic.split("/");

                TextView text_noBeacons = findViewById(R.id.noBeaconsActive);

                //If it's the first time the beacon connects, we add it to the beacon lists

                if (!beaconUUID.contains(parts[3]))
                {
                    beaconUUID.add(parts[3]);
                    beacons.add(new BeaconInfo());

                    text_noBeacons.setText(noBeacons);
                }

                //We will update the values according to the messages we recieve
                if ("name".equals(parts[4]))
                {
                    index = beaconUUID.indexOf(parts[3]);

                    BeaconInfo aux = beacons.get(index);
                    aux.mName = message.toString();
                    beacons.set(index, aux);

                    //TESTING PURPOSES
                    aux = beacons.get(index);
                    Log.w("NAME", aux.mName);
                }
                else if("lastActivity".equals(parts[4]))
                {

                    index = beaconUUID.indexOf(parts[3]);

                    lastActiveState = Integer.parseInt(message.toString());
                    systemTime = ((int) System.currentTimeMillis() / 1000);

                    //Removes The View, and the corresponding beacons in the beaconUUID and beacon lists
                    if (systemTime - lastActiveState > 30)
                    {
                        BeaconInfo aux = beacons.get(index);

                        //((ViewManager)aux.getParent()).removeView(aux);
                        beacons.remove(index);
                        noBeacons--;
                    }
                }
                else if ("x".equals(parts[4]))
                {
                    BeaconInfo aux = beacons.get(index);
                    aux.mX = Integer.parseInt(message.toString()) * 200;
                    Log.w("X", "X COORDINATE " + aux.mX);
                    beacons.set(index, aux);
                }
                else if ("y".equals(parts[4]))
                {
                    BeaconInfo aux = beacons.get(index);
                    aux.mY = Integer.parseInt(message.toString()) * 200;
                    beacons.set(index, aux);
                }
            }

            @Override
            public void deliveryComplete(IMqttDeliveryToken token)
            {

            }
        });
/*
        requestPermissions(new String[] {Manifest.permission.ACCESS_COARSE_LOCATION}, 1234);
        beaconManager = BeaconManager.getInstanceForApplication(this);
        beaconManager.getBeaconParsers().add(new BeaconParser().setBeaconLayout("m:2-3=0215,i:4- 19,i:20-21,i:22-23,p:24-24"));
        beaconManager.bind(this);*/


    }
/*
    @Override
    protected void onDestroy()
    {
        super.onDestroy();
        beaconManager.unbind(this);
    }

    @Override
    public void onBeaconServiceConnect()
    {
        beaconManager.removeAllMonitorNotifiers();
        beaconManager.addRangeNotifier(new RangeNotifier()
        {
            @Override
            public void didRangeBeaconsInRegion(Collection<Beacon> beacons, Region region)
            {
                if (beacons.size() > 0)
                    Log.i("Monitoring Activity", "The first beacon I see is about "+beacons.iterator().next().getDistance()+" meters away.");
            }
        });

        try
        {
            beaconManager.startRangingBeaconsInRegion(new Region("Region", null, null, null));
        }
        catch(RemoteException e)
        {}

    }   */


}
