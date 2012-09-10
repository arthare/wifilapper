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

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Map.Entry;

import com.artsoft.wifilapper.Utility.MultiStateObject;
import com.artsoft.wifilapper.Utility.MultiStateObject.StateData;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.location.LocationManager;
import android.util.AttributeSet;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.Toast;
import android.view.View;
import android.view.View.OnClickListener;

public class StatusBarView extends LinearLayout implements OnClickListener
{
	private Utility.MultiStateObject pStateMan;
	private static final Class rgClasses[] = {LapSender.class, OBDThread.class, LocationManager.class, IOIOManager.class};
	
	
	private static class ButtonData
	{
		public ButtonData(Resources r, int on, int off, int tg, int tb, ImageButton btn)
		{
			this.on = r.getDrawable(on);
			this.off = r.getDrawable(off);
			this.troublegood = r.getDrawable(tg);
			this.troublebad = r.getDrawable(tb);
			this.btn = btn;
		}
		public Drawable on;
		public Drawable off;
		public Drawable troublegood;
		public Drawable troublebad;
		public ImageButton btn;
	}
	
	private Map<Class,ButtonData> mapButtons;
	
	public StatusBarView(Context context)
	{
		super(context);
		DoPopulate();
	}
	public StatusBarView(Context context, AttributeSet attrs)
	{
		super(context,attrs);
		DoPopulate();
	}
	
	public void SetStateData(Utility.MultiStateObject pStateMan)
	{
		this.pStateMan = pStateMan;
	}
	public void DeInit()
	{
		pStateMan = null;
		mapButtons.clear();
		mapButtons = null;
	}
	public void Refresh()
	{
		for(int x = 0;x < rgClasses.length; x++)
		{
			Class c = rgClasses[x];
			ButtonData btn = mapButtons.get(rgClasses[x]);
			if(btn != null)
			{
				StateData state = pStateMan.GetState(c);
				switch(state.eState)
				{
				case ON: btn.btn.setImageDrawable(btn.on); break;
				case OFF: btn.btn.setImageDrawable(btn.off); break;
				case TROUBLE_GOOD: btn.btn.setImageDrawable(btn.troublegood); break;
				case TROUBLE_BAD: btn.btn.setImageDrawable(btn.troublebad); break;
				}
			}
		}
	}
	private void DoPopulate()
	{
		mapButtons = new HashMap<Class,ButtonData>();
		
		this.setOrientation(LinearLayout.HORIZONTAL);
		
		AddButton(LocationManager.class, R.drawable.satellite_on, R.drawable.satellite_off, R.drawable.satellite_troublegood, R.drawable.satellite_troublebad);
		AddButton(OBDThread.class, R.drawable.obd2_on, R.drawable.obd2_off, R.drawable.obd2_troublegood, R.drawable.obd2_troublebad);
		AddButton(LapSender.class, R.drawable.wifi_on, R.drawable.wifi_off, R.drawable.wifi_troublegood, R.drawable.wifi_troublebad);
		AddButton(IOIOManager.class, R.drawable.ioio_on, R.drawable.ioio_off, R.drawable.ioio_troublegood, R.drawable.ioio_troublebad);
	}
	
	private void AddButton(Class c, int imgOn, int imgOff, int imgTroubleGood, int imgTroubleBad)
	{
		ImageButton btn = new ImageButton(getContext());
		btn.setImageDrawable(getResources().getDrawable(imgOff));
		btn.setBackgroundDrawable(null);
		
		mapButtons.put(c, new ButtonData(getResources(), imgOn, imgOff, imgTroubleGood, imgTroubleBad, btn));
		
		btn.setOnClickListener(this);
		this.addView(btn);
		this.requestLayout();
	}
	@Override
	public void onClick(View v) 
	{
		if(mapButtons == null) return;
		
		Iterator<Entry<Class,ButtonData>> i = mapButtons.entrySet().iterator();
		while(i.hasNext())
		{
			Entry<Class,ButtonData> e = i.next();
			if(e.getValue().btn == v)
			{
				// found the clicked button!
				Class c = e.getKey();
				StateData sd = pStateMan.GetState(c);
				if(sd.strDesc != null)
				{
					Toast.makeText(getContext(), sd.strDesc, Toast.LENGTH_LONG).show();
				}
				break;
			}			
		}
	}
	
	

}
