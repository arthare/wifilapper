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
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.RadioGroup;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.RadioGroup.OnCheckedChangeListener;

public class ConfigureIOIOFilter extends Activity implements OnCheckedChangeListener, OnClickListener
{
	public static final int REQUESTCODE_CUSTOMFILTER = 1;
	
	
	// data state
	private int idChecked;
	
	@Override
	public void onCreate(Bundle extras)
	{
		super.onCreate(extras);
		setContentView(R.layout.configureioiofilter);
	}
	
	@Override
	public void onResume()
	{
		super.onResume();
		
		idChecked = R.id.rbNone;
		
		RadioGroup rg = (RadioGroup)findViewById(R.id.rgFilterType);
		rg.setOnCheckedChangeListener(this);
		
		Button btnApply = (Button)findViewById(R.id.btnApply);
		btnApply.setOnClickListener(this);
		
		{ // set up spinner
			Spinner spnCustom = (Spinner)findViewById(R.id.spnCustom);
			ArrayAdapter<CharSequence> adapter = ArrayAdapter.createFromResource(
		            this, R.array.ioiocustomnames, android.R.layout.simple_spinner_item);
		    adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
		    spnCustom.setAdapter(adapter);
		}
		
		UpdateUI();
	}
	
	@Override
	public void onPause()
	{
		super.onPause();
		
		Complete();
	}

	private void Complete()
	{
		TextView txt1 = (TextView)findViewById(R.id.txtParam1);
		String str1 = txt1.getText().toString();
		
		TextView txt2  = (TextView)findViewById(R.id.txtParam2);
		String str2 = txt2.getText().toString();
		
		Intent i = new Intent();
		
		try
		{
			double d1 = Double.parseDouble(str1);
			double d2 = Double.parseDouble(str2);
			
			int iFilterType = IOIOManager.PinParams.FILTERTYPE_NONE;
			
			if(idChecked == R.id.rbWheelSpeed)
			{
				iFilterType = IOIOManager.PinParams.FILTERTYPE_WHEELSPEED;
			}
			
			Spinner spnCustom = (Spinner)findViewById(R.id.spnCustom);
			int iCustomType = spnCustom.getSelectedItemPosition() == 0 ? 0 : spnCustom.getSelectedItemPosition() + LapAccumulator.DataChannel.CHANNEL_IOIOCUSTOM_START;
			
			i.putExtra("filtertype", iFilterType);
			i.putExtra("param1", d1);
			i.putExtra("param2", d2);
			i.putExtra("customtype", iCustomType);
			this.setResult(Activity.RESULT_OK, i);
			this.finish();
		}
		catch(NumberFormatException nfe)
		{
			Toast.makeText(this, "Bad numeric value", Toast.LENGTH_SHORT).show();
			this.setResult(Activity.RESULT_FIRST_USER);
		}
	}
	
	@Override
	public void onCheckedChanged(RadioGroup arg0, int arg1) 
	{
		idChecked = arg0.getCheckedRadioButtonId();
		
		UpdateUI();
	}
	
	private void SetupParamText(int idLabel, int idTextBox, String strLabelText, double dDefault)
	{
		TextView lbl= (TextView)findViewById(idLabel);
		EditText txt = (EditText)findViewById(idTextBox);
		
		txt.setVisibility(strLabelText.length() > 0 ? View.VISIBLE : View.GONE);
		lbl.setVisibility(strLabelText.length() > 0 ? View.VISIBLE : View.GONE);
		lbl.setText(strLabelText);
		txt.setText("" + Utility.FormatFloat((float)dDefault, 2));
	}
	
	private void UpdateUI()
	{
		String strParam1 = "";
		String strParam2 = "";
		double dDefault1 = 0;
		double dDefault2 = 0;
		if(idChecked == R.id.rbNone)
		{
			strParam1 = strParam2 = "";
		}
		else if(idChecked == R.id.rbWheelSpeedRPM)
		{
			strParam1 = "Pulses per revolution";
			strParam2 = "";
			dDefault1 = 8;
			dDefault2 = 0;
		}
		else if(idChecked == R.id.rbWheelSpeed)
		{
			strParam1 = "Pulses per revolution";
			strParam2 = "Wheel Diameter (mm)";
			dDefault1 = 8;
			dDefault2 = 711.2;
		}
		
		SetupParamText(R.id.lblParam1, R.id.txtParam1, strParam1, dDefault1);
		SetupParamText(R.id.lblParam2, R.id.txtParam2, strParam2, dDefault2);
	}

	@Override
	public void onClick(View arg0) 
	{
		if(arg0.getId() == R.id.btnApply)
		{
			Complete();
		}
	}
}
