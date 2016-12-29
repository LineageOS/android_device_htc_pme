/*
 * Copyright (C) 2016 The CyanogenMod Project
 * Copyright (C) 2017 The LineageOS Project
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

package com.cyanogenmod.settings.device;

import android.content.Context;
import android.util.Log;
import android.view.KeyEvent;

import com.android.internal.os.DeviceKeyHandler;

import cyanogenmod.hardware.CMHardwareManager;

public class KeyHandler implements DeviceKeyHandler {

    private static final String TAG = KeyHandler.class.getSimpleName();

    private final boolean DEBUG = false;

    private final Context mContext;

    public KeyHandler(Context context) {
        mContext = context;
    }

    public boolean handleKeyEvent(KeyEvent event) {
        if (event.getKeyCode() == KeyEvent.KEYCODE_HOME && event.getScanCode() == 143) {
            CMHardwareManager hardware = CMHardwareManager.getInstance(mContext);
            boolean virtualKeysEnabled = hardware.get(CMHardwareManager.FEATURE_KEY_DISABLE);

            if (DEBUG) {
                Log.d(TAG, "home key " + (virtualKeysEnabled ? "filtered" : "delivered"));
            }

            return virtualKeysEnabled;
        }

        return false;
    }

}
