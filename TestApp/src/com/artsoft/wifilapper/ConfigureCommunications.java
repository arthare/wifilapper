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
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.Toast;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.EditText;
import android.widget.Spinner;

public class ConfigureCommunications extends Activity implements OnClickListener
{
	@Override
	public void onCreate(Bundle extras)
	{
		super.onCreate(extras);
		setContentView(R.layout.configurecomms);
	}
	
	@Override
	public void onResume()
	{
		super.onResume();
		
		SharedPreferences settings = this.getSharedPreferences(Prefs.SHAREDPREF_NAME, 0);
		
		CheckBox chk = (CheckBox)findViewById(R.id.chkAckSMS);
		EditText txt = (EditText)findViewById(R.id.txtPrivacy);
		Button btnPrivacyHelp = (Button)findViewById(R.id.btnPrivacyHelp);
		Button btnSMSHelp = (Button)findViewById(R.id.btnSMSHelp);
		
		boolean fRespondWithSMS = settings.getBoolean(Prefs.PREF_ACKSMS_BOOLEAN, true);
		String strPrivacy = settings.getString(Prefs.PREF_PRIVACYPREFIX_STRING, Prefs.DEFAULT_PRIVACYPREFIX);
		
		txt.setText(strPrivacy);
		
		chk.setChecked(fRespondWithSMS);
		
		btnPrivacyHelp.setOnClickListener(this);
		btnSMSHelp.setOnClickListener(this);
	}
	
	@Override
	public void onPause()
	{
		super.onPause();

		SharedPreferences settings = this.getSharedPreferences(Prefs.SHAREDPREF_NAME, 0);
		CheckBox chk = (CheckBox)findViewById(R.id.chkAckSMS);
		EditText txt = (EditText)findViewById(R.id.txtPrivacy);
		
		settings.edit().putBoolean(Prefs.PREF_ACKSMS_BOOLEAN, chk.isChecked())
						.putString(Prefs.PREF_PRIVACYPREFIX_STRING, txt.getText().toString()).commit();
	}

	@Override
	public void onClick(View arg0) 
	{
		if(arg0.getId() == R.id.btnPrivacyHelp)
		{
			Toast.makeText(this, "To protect privacy, WifiLapper will not display any texts unless they start with the privacy prefix.  This also prevents other teams from sending your driver bogus instructions.  For example, with a privacy prefix of 'wflp', 'wflpPit Now' will display as 'Pit Now'", Toast.LENGTH_LONG).show();
		}
		else if(arg0.getId() == R.id.btnSMSHelp)
		{
			Toast.makeText(this,"When checked, WifiLapper will send a text to your phone once the driver acknowledges the message.", Toast.LENGTH_LONG).show();
		}
	}
}
