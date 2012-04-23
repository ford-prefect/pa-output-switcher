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

import java.util.Arrays;
import java.util.ArrayList;
import android.app.ListActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.*;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiManager.WifiLock;
import android.content.SharedPreferences;
import android.util.Log;

public class OutputSwitcher extends ListActivity implements CompoundButton.OnCheckedChangeListener, AdapterView.OnItemClickListener
{
    private static final String TAG = "PAOutputSwitcher";

    private WifiLock wifiLock;
    private ArrayList<String> remotes;

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
            String server = text_server.getText().toString();

            if (switchToNetwork(server)) {
                updateStatus("Switched to remote playback on '" + server + "'");
                addRecentRemote(server);

                ((ArrayAdapter<String>) getListAdapter()).notifyDataSetChanged();
            } else
                updateStatus("Failed to switch to remote playback on '" + text_server.getText().toString() + "'");

            wifiLock.acquire();
        }
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id)
    {
        EditText text_server = (EditText) findViewById(R.id.text_server);
        text_server.setText(((TextView) view).getText().toString());

        Switch sw = (Switch) findViewById(R.id.switch_remote);
        sw.setChecked(false);
        sw.setChecked(true);
    }

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        Switch sw = (Switch) findViewById(R.id.switch_remote);
        sw.setOnCheckedChangeListener(this);

        remotes = new ArrayList(Arrays.asList(getRecentRemotes().split(",")));
        setListAdapter(new ArrayAdapter<String>(this, android.R.layout.simple_list_item_1, remotes));
        getListView().setOnItemClickListener(this);

        WifiManager wm = (WifiManager) getSystemService(WIFI_SERVICE);
        wifiLock = wm.createWifiLock(WifiManager.WIFI_MODE_FULL_HIGH_PERF, "PA_WifiLock");
        wifiLock.setReferenceCounted(false);
    }

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

    /* Util functions for recent remotes */
    private String getRecentRemotes()
    {
        return getPreferences(MODE_PRIVATE).getString("recentRemotes", "");
    }

    private void saveRecentRemotes(String remotes)
    {
        if (remotes.length() == 0)
            return;

        SharedPreferences.Editor editor = getPreferences(MODE_PRIVATE).edit();
        
        editor.putString("recentRemotes", remotes);
        editor.commit();
    }

    private void addRecentRemote(String newRemote)
    {
        String curRemotes = getRecentRemotes();
        String prefString = newRemote;

        String[] remotesList = curRemotes.split(",");

        for (int i = 0; i < remotesList.length; i++) {
            if (remotesList[i].equals(newRemote)) {
                /* Dupe, get out of here */
                return;
            }

            prefString = prefString + "," + remotesList[i];
        }

        saveRecentRemotes(prefString);
        /* Add to list view as well */
        remotes.add(0, newRemote);
    }
}
