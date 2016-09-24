/*
 * Copyright (C) 2016 The CyanogenMod Project
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
import android.content.Intent;
import android.os.UserHandle;
import android.os.Vibrator;
import android.provider.Settings;
import android.util.Log;
import android.view.HapticFeedbackConstants;
import android.view.KeyEvent;

import com.android.internal.os.DeviceKeyHandler;

public class KeyHandler implements DeviceKeyHandler {

    private static final String TAG = KeyHandler.class.getSimpleName();
    private static final int KEYCODE_WAKEUP = KeyEvent.KEYCODE_WAKEUP;
    private static final int KEYCODE_HOME = KeyEvent.KEYCODE_HOME;

    private final Context mContext;
    private Vibrator mVibrator;

    public KeyHandler(Context context) {
        mContext = context;
        mVibrator = (Vibrator) context.getSystemService(Context.VIBRATOR_SERVICE);
        if (mVibrator == null || !mVibrator.hasVibrator()) {
            mVibrator = null;
        }
    }

    public boolean handleKeyEvent(KeyEvent event) {
        if (event.getAction() != KeyEvent.ACTION_DOWN) {
            return false;
        }
        if (event.getKeyCode() == KeyEvent.KEYCODE_WAKEUP) {
            Log.v(TAG, "Detected KEYCODE_WAKEUP, emulating KEYCODE_HOME");
            doHapticFeedback();
            launchHomeIntent();
            return true;
        }
        return false;
    }

    private void launchHomeIntent() {
        Intent homeIntent = new Intent(Intent.ACTION_MAIN);
        homeIntent.addCategory(Intent.CATEGORY_HOME);
        try {
            mContext.startActivityAsUser(homeIntent, null, new UserHandle(UserHandle.USER_CURRENT));
        } catch (Exception e) {
            Log.e(TAG, "Unable to launch home intent");
        }
    }

    private void doHapticFeedback() {
        final long[] pattern = {0, 25, 8}; // config_virtualKeyVibePattern
        final boolean hapticsDisabled = Settings.System.getIntForUser(mContext.getContentResolver(),
                Settings.System.HAPTIC_FEEDBACK_ENABLED, 0, UserHandle.USER_CURRENT) == 0;
        if (mVibrator == null) {
            return;
        }
        if (hapticsDisabled) {
            return;
        }
        if (pattern.length == 1) {
            // One-shot vibration
            mVibrator.vibrate(pattern[0]);
        } else {
            // Pattern vibration
            mVibrator.vibrate(pattern, -1);
        }
    }
}
