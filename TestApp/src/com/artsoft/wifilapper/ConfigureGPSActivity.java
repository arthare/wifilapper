// Copyright 2011-2012, Art Hare
// This file is part of WifiLapper.

//WifiLapper is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//WifiLapper is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

//You should have received a copy of the GNU General Public License
//along with WifiLapper.  If not, see <http://www.gnu.org/licenses/>.

package com.artsoft.wifilapper;

import android.app.Activity;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.Spinner;

public class ConfigureGPSActivity extends Activity implements OnCheckedChangeListener
{

	@Override
	public void onCreate(Bundle extras)
	{
		super.onCreate(extras);
		setContentView(R.layout.configuregps);
	}
	
	@Override
	public void onResume()
	{
		super.onResume();
		
		SharedPreferences settings = this.getSharedPreferences(Prefs.SHAREDPREF_NAME, 0);
		
		Spinner spn = (Spinner)findViewById(R.id.spnGPS);
		CheckBox chk = (CheckBox)findViewById(R.id.chkGPS);
		
		String strDefault = settings.getString(Prefs.PREF_BTGPSNAME_STRING, "");
		
		boolean fGPS = strDefault != null && strDefault.length() > 0;
		
		LandingRaceBase.SetupBTSpinner(this, spn, strDefault);
		chk.setChecked(fGPS);
		chk.setOnCheckedChangeListener(this);
		spn.setEnabled(fGPS);
	}
	
	@Override
	public void onPause()
	{
		super.onPause();

		SharedPreferences settings = this.getSharedPreferences(Prefs.SHAREDPREF_NAME, 0);
		Spinner spn = (Spinner)findViewById(R.id.spnGPS);
		CheckBox chk = (CheckBox)findViewById(R.id.chkGPS);
		
		String strValue = "";
		Object selected = spn.getSelectedItem();
		if(selected != null)
		{
			strValue = selected.toString();
		}
		
		settings.edit().putString(Prefs.PREF_BTGPSNAME_STRING, chk.isChecked() ? strValue : "").commit();
	}

	@Override
	public void onCheckedChanged(CompoundButton arg0, boolean arg1) 
	{
		if(arg0.getId() == R.id.chkGPS)
		{
			Spinner spn = (Spinner)findViewById(R.id.spnGPS);
			spn.setEnabled(arg1);
		}
	}
}
