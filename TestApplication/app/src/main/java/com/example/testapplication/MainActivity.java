package com.example.testapplication;

import android.os.Bundle;
import android.os.RemoteException;
import android.util.Log;
import android.util.TypedValue;
import android.view.View;
import android.widget.RelativeLayout;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;

import com.example.testapplication.CustomViews.DrawSquares;
import com.example.testapplication.MQTT.MqttConnection;
import com.lemmingapex.trilateration.NonLinearLeastSquaresSolver;
import com.lemmingapex.trilateration.TrilaterationFunction;

import org.altbeacon.beacon.Beacon;
import org.altbeacon.beacon.BeaconConsumer;
import org.altbeacon.beacon.BeaconManager;
import org.altbeacon.beacon.BeaconParser;
import org.altbeacon.beacon.RangeNotifier;
import org.altbeacon.beacon.Region;
import org.apache.commons.math3.fitting.leastsquares.LeastSquaresOptimizer;
import org.apache.commons.math3.fitting.leastsquares.LevenbergMarquardtOptimizer;
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallbackExtended;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;

import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

public class MainActivity extends AppCompatActivity implements BeaconConsumer
{
    Map<String, DrawSquares> beaconMap = new HashMap<>();       // A map whose keys are the beacon UUIDs
    ArrayList<double[]> positions  = new ArrayList<>();         // Position ArrayList, to use in Trilateration
    ArrayList<Double> distances =  new ArrayList<>();           // Distance ArrayList, to use in Trilateration

    private BeaconManager beaconManager = null;
    private Region beaconRegion = null;

    int lastActiveState;                                        // Last active state of the beacon
    int systemTime;                                             // Present system time
    int noBeacons = 0;                                          // Total number of beacons

    //UUID uuid = UUID.randomUUID();                            // This smartphone's randomly generated UUID
    String uuid = "cb7fe7ff-cf91-47bb-914b-09211db823af";
    String topic = "/laterator/devices/" + uuid + "/";

    double smartphoneX;
    double smartphoneY;

    String bleUUID;
    double bleDistance;

    MqttConnection mqttConnection;



    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        //Start Beacon Monitoring
        beaconManager = BeaconManager.getInstanceForApplication(this);
        beaconManager.getBeaconParsers().add(new BeaconParser().setBeaconLayout("m:2-3=0215,i:4-19,i:20-21,i:22-23,p:24-24"));
        beaconManager.bind(this);

        //Start MQTT Connection
        mqttConnection = new MqttConnection(this);
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
                //Split the full topic structure into sections, parts[3] is the UUID and parts[4] is the topic message itself
                String parts[] = topic.split("/");

                //Instantiate a view variable to display the number of active beacons
                TextView text_noBeacons = findViewById(R.id.noBeaconsActive);

                //If it's the first time the beacon connects, we add it to the beacon lists
                if (!beaconMap.containsKey(parts[3]))
                {
                    beaconMap.put(parts[3], new DrawSquares(MainActivity.this));

                    DrawSquares aux = beaconMap.get(parts[3]);
                    aux.mViewId = noBeacons;
                    noBeacons++;

                    //Set the number of beacons
                    text_noBeacons.setText(noBeacons);
                    //Redraw the view
                    text_noBeacons.invalidate();
                }


                //We will update the values according to the messages we recieve
                if ("name".equals(parts[4]))
                {
                    DrawSquares aux = beaconMap.get(parts[3]);
                    aux.mName = parts[4];
                    beaconMap.put(parts[3], aux);
                }
                //Will only remove smartphones
                else if("lastActivity".equals(parts[4]) && "devices".equals(parts[2]))
                {
                    lastActiveState = Integer.parseInt(message.toString());
                    systemTime = ((int) System.currentTimeMillis() / 1000);

                    //Removes the View and the corresponding element in the beacon map if the
                    //smartphone has been idle for more than 30 seconds
                    if (systemTime - lastActiveState > 30)
                    {
                        DrawSquares drawSquares = beaconMap.get(parts[3]);
                        View view = findViewById(drawSquares.mViewId);
                        beaconMap.remove(parts[3]);

                        view.invalidate();
                        noBeacons--;
                    }
                }
                else if ("x".equals(parts[4]))
                {
                    DrawSquares aux = beaconMap.get(parts[3]);

                    //Multiply by the dimensions of our canvas
                    aux.mX = Integer.parseInt(message.toString()) * dpToPixels(295);
                    beaconMap.put(parts[3], aux);
                }
                else if ("y".equals(parts[4]))
                {
                    DrawSquares aux = beaconMap.get(parts[3]);

                    //Multiply by the dimensions of our canvas
                    aux.mY = Integer.parseInt(message.toString()) * dpToPixels(295);
                    beaconMap.put(parts[3], aux);

                    //Only draws here, at the most updated value for the coordinates
                    drawView(aux);
                }

            }

            @Override
            public void deliveryComplete(IMqttDeliveryToken token)
            {

            }
        });
    }

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
                Log.i("Where", "Beacon size " + beacons.size());
                Log.i("Where", "Number of beacons " + noBeacons);
                //Only display smartphone coordinates if we have more than 3 beacons available
                if (beacons.size() > 2 && noBeacons > 2)
                {
                    //Different iterators for different data
                    Iterator<Beacon> distanceIt = beacons.iterator();
                    Iterator<Beacon> uuidIt = beacons.iterator();

                    //Iterate through the available beacons and obtain their distances and coordinates
                    while (uuidIt.hasNext())
                    {
                        bleUUID = uuidIt.next().getId1().toString();
                        bleUUID = reverseUUID(bleUUID);                 //Little endian to Big endian conversion

                        bleDistance = distanceIt.next().getDistance();

                        //If the beacon scanned is in our beacon Map
                        if (beaconMap.containsKey(bleUUID))
                        {
                            DrawSquares aux = beaconMap.get(bleUUID);
                            double[] coordinates = {aux.mX, aux.mY};

                            positions.add(coordinates);
                            distances.add(bleDistance);
                        }
                    }

                    double[][] posInput = doubleListToArray(positions);
                    double[] disInput = listToArray(distances);

                    //Just in case the beacon scanned is not in our beacon map yet
                    if (disInput.length > 2)
                    {
                        NonLinearLeastSquaresSolver solver = new NonLinearLeastSquaresSolver(new TrilaterationFunction(posInput, disInput), new LevenbergMarquardtOptimizer());
                        LeastSquaresOptimizer.Optimum optimum = solver.solve();

                        //Our smartphone coordinates: centroid[0] is x, centroid[1] is y
                        double[] centroid = optimum.getPoint().toArray();

                        //Smartphone coordinates are added/updated
                        if (!beaconMap.containsKey(uuid))
                        {
                            DrawSquares aux = new DrawSquares(MainActivity.this);

                            aux.mX = (int) centroid[0];
                            aux.mY = (int) centroid[1];
                            aux.mViewId = noBeacons;
                            aux.mName = "OnePlus";

                            beaconMap.put(uuid, aux);
                            noBeacons++;

                            //Publish (x,y) coordinates, lastActivity and name through MQTT
                            mqttPublish(topic + "x", "" + (double) aux.mX / (double) dpToPixels(295));
                            mqttPublish(topic + "y", "" + (double) aux.mY / (double) dpToPixels(295));
                            mqttPublish(topic + "lastActivity", "" + ((int) System.currentTimeMillis() / 1000));
                            mqttPublish("/laterator/beacons/" + uuid + "/name", aux.mName);

                            drawView(aux);
                        }
                        else
                        {
                            DrawSquares aux = beaconMap.get(uuid);

                            aux.mX = (int) centroid[0];
                            aux.mY = (int) centroid[1];
                            aux.mName = "OnePlus";

                            smartphoneX = (double) aux.mX / (double) dpToPixels(295);
                            smartphoneY = (double) aux.mY / (double) dpToPixels(295);

                            //Publish (x,y) coordinates, lastActivity and name through MQTT
                            mqttPublish(topic + "x", "" + (double) aux.mX / (double) dpToPixels(295));
                            mqttPublish(topic + "y", "" + (double) aux.mY / (double) dpToPixels(295));
                            mqttPublish(topic + "lastActivity", "" + ((int) System.currentTimeMillis() / 1000));
                            mqttPublish("/laterator/beacons/" + uuid + "/name", aux.mName);

                            drawView(aux);
                        }
                    }
                }
            }
        });

        try
        {
            beaconManager.startRangingBeaconsInRegion(new Region("Region", null, null, null));
        }
        catch(RemoteException e)
        {}

    }

    public int dpToPixels(int dp)
    {
        int pixels;

        pixels = (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, dp, getResources().getDisplayMetrics());

        return pixels;
    }

    public void drawView(DrawSquares aux)
    {
        View v = findViewById(aux.mViewId);

        //If it's a new beacon connecting we add it to the parent view
        if (v == null)
        {
            RelativeLayout parent = findViewById(R.id.parent);
            View view = new DrawSquares(MainActivity.this);
            view.generateViewId();
            view.setId(aux.mViewId);


            RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(dpToPixels(300), dpToPixels(300));
            params.addRule(RelativeLayout.CENTER_HORIZONTAL);
            params.addRule(RelativeLayout.CENTER_VERTICAL);
            params.addRule(RelativeLayout.ALIGN_START);
            view.setLayoutParams(params);

            parent.addView(view);

            DrawSquares drawSquares  = findViewById(aux.mViewId);
            drawSquares.mY = aux.mY;
            drawSquares.mX = aux.mX;

            view.invalidate();


        }
        //If not, we simply update its values
        else
        {
            DrawSquares drawSquares = findViewById(aux.mViewId);

            drawSquares.mY = aux.mY;
            drawSquares.mX = aux.mX;

            v = drawSquares;
            v.invalidate();
        }
    }

    public static String reverseUUID(String originalUUID)
    {
        int i = 0;
        int j = originalUUID.length() - 1;
        char[] input = originalUUID.toCharArray();
        char[] output = new char[originalUUID.length()];

        output[8] = '-';
        output[13] = '-';
        output[18] = '-';
        output[23] = '-';


        while (i < originalUUID.length())
        {
            if (output[i] == '-')
                i++;

            if (input[j] == '-')
                j--;

            output[i] = input[j-1];
            output[i+1] = input[j];

            i = i+2;
            j = j-2;

        }

        return new String(output);
    }

    public double[][] doubleListToArray(ArrayList<double[]> input)
    {
        int listSize = input.size();
        double[][] output = new double[listSize][2];

        for (int i = 0; i < listSize; i++)
            output[i] = input.get(i);

        return output;
    }

    public double[] listToArray(ArrayList<Double> input)
    {
        int listSize = input.size();
        double[] output = new double[listSize];

        for (int i = 0; i < listSize; i++)
            output[i] = input.get(i);

        return output;
    }

    public void mqttPublish(String topic, String payload)
    {
        byte[] encodedPayload = new byte[0];

        Log.i("Where", "Topic " + topic);
        Log.i("Where", "Message " + payload);

        try
        {
            encodedPayload = payload.getBytes("UTF-8");
            MqttMessage message = new MqttMessage(encodedPayload);
            mqttConnection.mqttAndroidClient.publish(topic, message);
        }
        catch (UnsupportedEncodingException | MqttException e)
        {
            e.printStackTrace();
        }

    }
}
