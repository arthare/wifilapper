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
