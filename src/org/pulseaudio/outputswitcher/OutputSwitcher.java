/*
 * Copyright (C) 2012 Collabora Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.pulseaudio.outputswitcher;

import android.app.Activity;
import android.os.Bundle;
import android.widget.*;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiManager.WifiLock;

public class OutputSwitcher extends Activity implements CompoundButton.OnCheckedChangeListener
{
    private WifiLock wifiLock;

    @Override
    public void onCheckedChanged(CompoundButton sw, boolean isChecked)
    {
        if (!isChecked) {
            if (switchToLocal())
                updateStatus("Switched to local playback");
            else
                updateStatus("Failed to switch to local playback");

            wifiLock.release();
        } else {
            EditText text_server = (EditText) findViewById(R.id.text_server);

            if (switchToNetwork(text_server.getText().toString()))
                updateStatus("Switched to remote playback on '" + text_server.getText().toString() + "'");
            else
                updateStatus("Failed to switch to remote playback on '" + text_server.getText().toString() + "'");

            wifiLock.acquire();
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        Switch sw = (Switch) findViewById(R.id.switch_remote);
        sw.setOnCheckedChangeListener(this);

        WifiManager wm = (WifiManager) getSystemService(WIFI_SERVICE);
        wifiLock = wm.createWifiLock(WifiManager.WIFI_MODE_FULL_HIGH_PERF, "PA_WifiLock");
        wifiLock.setReferenceCounted(false);
    }

    /* A native method that is implemented by the
     * 'hello-jni' native library, which is packaged
     * with this application.
     */
    public native boolean switchToNetwork(String serverName);
    public native boolean switchToLocal();

    private void updateStatus(String status)
    {
        TextView text_status = (TextView) findViewById(R.id.text_status);
        text_status.setText(status);
    }

    static {
        System.loadLibrary("output-switcher");
    }
}
