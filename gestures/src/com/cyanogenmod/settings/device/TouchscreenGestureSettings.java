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

import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.SwitchPreference;
import android.provider.Settings;

public class TouchscreenGestureSettings extends PreferenceActivity {

    private static final String KEY_DOUBLE_TAP_ENABLE = "double_tap_enable_key";

    private SwitchPreference mDoubleTapPreference;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.gesture_panel);

        mDoubleTapPreference = (SwitchPreference) findPreference(KEY_DOUBLE_TAP_ENABLE);
        mDoubleTapPreference.setChecked(isDoubleTapEnabled());
        mDoubleTapPreference.setOnPreferenceChangeListener(mDoubleTapPrefListener);
    }

    @Override
    protected void onResume() {
        super.onResume();

        getListView().setPadding(0, 0, 0, 0);
    }

    private boolean enableDoubleTap(boolean enable) {
        return Settings.Secure.putInt(getContentResolver(),
                Settings.Secure.DOUBLE_TAP_TO_WAKE, enable ? 1 : 0);
    }

    private boolean isDoubleTapEnabled() {
        return Settings.Secure.getInt(getContentResolver(),
                Settings.Secure.DOUBLE_TAP_TO_WAKE, 0) != 0;
    }

    private Preference.OnPreferenceChangeListener mDoubleTapPrefListener =
            new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange(Preference preference, Object newValue) {
            boolean enable = (boolean) newValue;
            return enableDoubleTap(enable);
        }
    };
}
