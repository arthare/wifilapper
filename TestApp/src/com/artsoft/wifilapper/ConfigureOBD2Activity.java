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

import java.util.ArrayList;
import java.util.List;

import com.artsoft.wifilapper.OBDThread.OBDListener;
import com.artsoft.wifilapper.OBDThread.PIDParameter;
import com.artsoft.wifilapper.OBDThread.PIDSupportListener;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.ListView;
import android.widget.Spinner;
import android.widget.Toast;

public class ConfigureOBD2Activity extends Activity implements OnCheckedChangeListener,PIDSupportListener,Handler.Callback, OnClickListener
{
	private List<Integer> lstSelectedPIDs;
	private Handler m_handler;
	private String m_strOBD2Error;
	
	private static final int MSG_OBD2 = 50;
	
	@Override
	public void onCreate(Bundle extras)
	{
		super.onCreate(extras);
		m_handler = new Handler(this);
		setContentView(R.layout.configureobd2);
	}
	
	@Override
	public void onResume()
	{
		super.onResume();
		
		SharedPreferences settings = this.getSharedPreferences(Prefs.SHAREDPREF_NAME, 0);
		
		Spinner spn = (Spinner)findViewById(R.id.spnOBD2);
		CheckBox chk = (CheckBox)findViewById(R.id.chkOBD2);
		Button btnScan = (Button)findViewById(R.id.btnScanPIDs);
		
		String strDefault = settings.getString(Prefs.PREF_BTOBD2NAME_STRING, "");
		
		boolean fOBD2 = strDefault != null && strDefault.length() > 0;
		
		LandingRaceBase.SetupBTSpinner(this, spn, strDefault);
		chk.setChecked(fOBD2);
		chk.setOnCheckedChangeListener(this);
		spn.setEnabled(fOBD2);
		btnScan.setOnClickListener(this);
		
		lstSelectedPIDs = new ArrayList<Integer>();
		Prefs.LoadOBD2PIDs(settings, lstSelectedPIDs);
	}
	
	private SharedPreferences.Editor SaveOBD2PIDs(SharedPreferences.Editor edit, ListView lstOBD2)
	{
		// first, wipe out all the old PIDs
		for(int x = 0;x < 256; x++)
		{
			edit = edit.putBoolean("pid" + x, false);
		}
		for(int x = 0;x < lstOBD2.getChildCount(); x++)
		{
			PIDParameterItem pid = (PIDParameterItem)lstOBD2.getItemAtPosition(x);
			if(pid.IsChecked())
			{
				edit = edit.putBoolean("pid" + pid.ixCode, true);
			}
		}
		return edit;
	}
	
	@Override
	public void onPause()
	{
		super.onPause();

		SharedPreferences settings = this.getSharedPreferences(Prefs.SHAREDPREF_NAME, 0);
		Spinner spn = (Spinner)findViewById(R.id.spnOBD2);
		CheckBox chk = (CheckBox)findViewById(R.id.chkOBD2);
		ListView lstOBD2PIDs = (ListView)findViewById(R.id.lstPIDs);
		
		String strValue = "";
		Object obj = spn.getSelectedItem();
		if(obj != null)
		{
			strValue = obj.toString();
		}
		
		SharedPreferences.Editor edit = settings.edit();
		edit = edit.putString(Prefs.PREF_BTOBD2NAME_STRING, chk.isChecked() ? strValue : "");
		edit = SaveOBD2PIDs(edit, lstOBD2PIDs);
		edit.commit();
	}

	@Override
	public void onCheckedChanged(CompoundButton arg0, boolean arg1) 
	{
		if(arg0.getId() == R.id.chkOBD2)
		{
			Spinner spn = (Spinner)findViewById(R.id.spnOBD2);
			Button btnScan = (Button)findViewById(R.id.btnScanPIDs);
			
			spn.setEnabled(arg1);
			btnScan.setEnabled(arg1);
		}
	}
	
	private static class PIDParameterItem extends PIDParameter
	{
		public PIDParameterItem(int ixCode, String strDesc, boolean fChecked) 
		{
			super(ixCode, strDesc);
			this.fChecked = fChecked;
		}
		public void SetChecked(boolean f)
		{
			fChecked = f;
		}
		public boolean IsChecked()
		{
			return fChecked;
		}
		private boolean fChecked = false;
	}
	private static class PIDParameterAdapter extends ArrayAdapter<PIDParameterItem> implements OnCheckedChangeListener
	{
	    private List<PIDParameterItem> items;

	    public PIDParameterAdapter(Context context, int textViewResourceId, List<PIDParameterItem> objects) 
	    {
	        super(context, textViewResourceId, objects);

	        this.items = objects;
	    }

	    @Override
	    public View getView(int position, View convertView, ViewGroup parent) 
	    {
	        View v = convertView;
	        if (v == null) 
	        {
	            LayoutInflater vi = (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
	            v = vi.inflate(R.layout.obd2list_item, null);
	        }
	        PIDParameterItem myobject = items.get(position);

	        if (myobject!=null)
	        {
	            //checkbox
	            CheckBox cbEnabled = (CheckBox) v.findViewById(R.id.chkSelect);
	            if(cbEnabled != null)
	            {
	                cbEnabled.setChecked(myobject.fChecked);
	                cbEnabled.setText(myobject.toString());
	                cbEnabled.setTag(new Integer(position));
	                cbEnabled.setOnCheckedChangeListener(this);
	            }
	        }

	        return v;
	    }

		@Override
		public void onCheckedChanged(CompoundButton buttonView,boolean isChecked) 
		{
			int iPos = ((Integer)buttonView.getTag()).intValue();
			items.get(iPos).SetChecked(isChecked);
		}
	}
	
	private void FillOBD2List(ListView lstUI, List<PIDParameter> lstData)
	{
		List<PIDParameterItem> lstItems = new ArrayList<PIDParameterItem>();
		if(lstData != null) // call with null to empty the list?
		{
			for(int x = 0; x < lstData.size(); x++)
			{
				PIDParameter pid = lstData.get(x);
				lstItems.add(new PIDParameterItem(pid.ixCode, pid.strDesc, this.lstSelectedPIDs.contains(new Integer(pid.ixCode))));
			}
		}
		
		PIDParameterAdapter adapter = new PIDParameterAdapter(this, R.layout.obd2list_item, lstItems);

		lstUI.setAdapter(adapter);
		Utility.SetListViewHeightBasedOnChildren(lstUI);
	}
	@Override
	public void NotifyPIDSupport(List<PIDParameter> lstPIDs)
	{
		Message m = Message.obtain(m_handler, MSG_OBD2, lstPIDs);
		m_handler.sendMessage(m);
	}

	@Override
	public boolean handleMessage(Message msg) 
	{
		switch(msg.what)
		{
		case MSG_OBD2:
			if(m_strOBD2Error != null)
			{
				Toast.makeText(this, "OBD2 error: " + m_strOBD2Error, Toast.LENGTH_LONG).show();
				m_strOBD2Error = null;
			}
			List<PIDParameter> lstPIDs = (List<PIDParameter>)msg.obj;
			if(lstPIDs != null)
			{
				ListView lstOBD2 = (ListView)findViewById(R.id.lstPIDs);
				FillOBD2List(lstOBD2, lstPIDs);
				Toast.makeText(this, "Found " + lstPIDs.size() + " supported OBD2 parameters.", Toast.LENGTH_LONG).show();
			}
			else
			{
				Toast.makeText(this, "No available OBD2 parameters.  Did you select the correct device and is it in range?", Toast.LENGTH_LONG).show();
			}
			Button btnSearch = (Button)findViewById(R.id.btnScanPIDs);
			btnSearch.setEnabled(true);
			return true;
		}
		return false;
	}

	@Override
	public void onClick(View arg0) 
	{
		if(arg0.getId() == R.id.btnScanPIDs)
		{
			Spinner spnOBD2 = (Spinner)findViewById(R.id.spnOBD2);
			if(spnOBD2.getChildCount() > 0 && spnOBD2.getSelectedItem() != null)
			{
				final String strBT = spnOBD2.getSelectedItem().toString();
				OBDThread.ThdGetSupportedPIDs(strBT,this);

				Button btnSearch = (Button)findViewById(R.id.btnScanPIDs);
				btnSearch.setEnabled(false);
				Toast.makeText(this, "Querying device '" + strBT + "'", Toast.LENGTH_LONG).show();
			}
			else
			{
				Toast.makeText(this, "You must select a bluetooth device first", Toast.LENGTH_LONG).show();
			}
		}
	}

	@Override
	public void NotifyOBD2Error(String str) 
	{
		m_strOBD2Error = str;
		this.m_handler.sendEmptyMessage(MSG_OBD2);
	}
}
