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

import java.text.NumberFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import com.artsoft.wifilapper.Utility.MultiStateObject.STATE;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.database.Cursor;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Paint.Align;
import android.graphics.Rect;
import android.graphics.Region.Op;
import android.net.Uri;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.provider.MediaStore;
import android.view.View;
import android.view.View.MeasureSpec;
import android.view.ViewGroup;
import android.widget.ListAdapter;
import android.widget.ListView;

public class Utility 
{
	public static void ShowAlert(Context ctx, String strText)
	{
		AlertDialog ad = new AlertDialog.Builder(ctx).create();  
		ad.setCancelable(false); // This blocks the 'BACK' button  
		ad.setMessage(strText);  
		ad.setButton("OK", new DialogInterface.OnClickListener() {  
		    @Override  
		    public void onClick(DialogInterface dialog, int which) {  
		        dialog.dismiss();                      
		    }  
		});  
		ad.show();  
	}
	public static int ParseInt(String str, int iDefault)
	{
		int iRet = 0;
		try
		{
			iRet = Integer.parseInt(str);
		}
		catch(NumberFormatException e)
		{
			iRet = iDefault;
		}
		return iRet;
	}
	// true -> a change was made
	// false -> no changes were made
	public static boolean ConnectToSSID(String strSSID, WifiManager pWifi)
	{
		WifiInfo pInfo = pWifi.getConnectionInfo();
		if(pInfo != null && pInfo.getSSID() != null && !pInfo.getSSID().equalsIgnoreCase(strSSID))
		{
			pWifi.disconnect();
		}
		else if(pInfo != null && pInfo.getSSID() != null && pInfo.getSSID().equalsIgnoreCase(strSSID))
		{
			return false; // nothing to do!
		}
		
		List<WifiConfiguration> lstNetworks = pWifi.getConfiguredNetworks();
		for(int x = 0;x < lstNetworks.size(); x++)
		{
			WifiConfiguration pNet = lstNetworks.get(x);
			if(pNet.SSID != null && pNet.SSID.equalsIgnoreCase("\"" + strSSID + "\""))
			{
				pWifi.enableNetwork(pNet.networkId, true);
				return true;
			}
		}
		return false;
	}
	public static void SetListViewHeightBasedOnChildren(ListView listView) 
	{
        ListAdapter listAdapter = listView.getAdapter();
        if (listAdapter == null) 
        {
            // pre-condition
            return;
        }

        int totalHeight = 0;
        int desiredWidth = MeasureSpec.makeMeasureSpec(listView.getWidth(), MeasureSpec.AT_MOST);
        for (int i = 0; i < listAdapter.getCount(); i++) 
        {
            View listItem = listAdapter.getView(i, null, listView);
            listItem.measure(desiredWidth, MeasureSpec.UNSPECIFIED);
            totalHeight += listItem.getMeasuredHeight();
        }

        ViewGroup.LayoutParams params = listView.getLayoutParams();
        params.height = totalHeight + (listView.getDividerHeight() * (listAdapter.getCount() - 1));
        listView.setLayoutParams(params);
        listView.requestLayout();
    }

	public static int GetNeededFontSize(String str, Paint p, Rect rcScreen)
	{
		final int cxPermitted = rcScreen.right - rcScreen.left;
		final int cyPermitted = rcScreen.bottom - rcScreen.top;
		int iFontHigh = 200;
		int iFontLow = 10;
		
		Rect rcBounds = new Rect();
		while(true)
		{
			if(iFontHigh - iFontLow <= 1)
			{
				// we've found the answer
				return iFontLow;
			}
			else
			{
				int iFontCheck = (iFontHigh+iFontLow)/2;
				p.setTextSize(iFontCheck);
				p.getTextBounds(str, 0, str.length(), rcBounds);
				final int cxFont = rcBounds.right - rcBounds.left;
				final int cyFont = rcBounds.bottom - rcBounds.top;
				final boolean fTooBig = cxFont > cxPermitted || cyFont > cyPermitted;
				if(fTooBig)
				{
					iFontHigh = iFontCheck;
				}
				else
				{
					iFontLow = iFontCheck;
				}
			}
		}
	}
	public static String FormatSeconds(float seconds)
	{
		final long ms= (int)(seconds*1000);
		final long tenthsLeftOver = (ms/100) % 10;
		final long secondsLeftOver = (ms / 1000) % 60;
		final long minutes = ms/60000;
		
		String strSeconds = (secondsLeftOver<10 && minutes > 0) ? ("0" + secondsLeftOver) : ("" + secondsLeftOver);
		if(minutes > 0)
		{
			return minutes + ":" + strSeconds + "." + tenthsLeftOver;
		}
		else
		{
			return strSeconds + "." + tenthsLeftOver;
		}
	}
	public static void DrawFontInBox(Canvas c, final String str, Paint p, final Rect rcScreen)
	{
		DrawFontInBox(c, str, p, rcScreen, true);
	}
	public static String GetDateStringFromUnixTime(long timeInMilliseconds)
	{
		Date d = new Date(timeInMilliseconds);
		return (d.getMonth()+1) + "/" + d.getDate() + "/" + (d.getYear()+1900);
	}
	public static String GetReadableTime(long timeInMilliseconds)
	{
		Date d = new Date(timeInMilliseconds);
		final int hours = d.getHours();
		final int minutes = d.getMinutes();
		String strMinutes = minutes < 10 ? "0" + minutes : "" + minutes;
		if(hours > 12)
		{
			return (hours-12) + ":" + strMinutes + "pm";
		}
		else
		{
			return hours + ":" + strMinutes + ((hours == 12) ? "pm" : "am");
		}
		
	}
	public static String FormatFloat(float fl, int digits)
	{
		NumberFormat num = NumberFormat.getInstance();
		num.setMaximumFractionDigits(digits);
		num.setMinimumFractionDigits(digits);
		return num.format(fl);
	}
	public static void DrawFontInBoxFinal(Canvas c, final String str, float flFontSize, Paint p, final Rect rcScreen, boolean fLeftJustify, boolean fRightJustify)
	{
		// we've found the answer
		p.setTextSize(flFontSize);
		
		Rect rcBounds = new Rect();
		p.getTextBounds(str, 0, str.length(), rcBounds);
		
		// we want to put the centerline of the font rect on the centre of the screen rect
		float flShiftX = 0;
		if(fLeftJustify)
		{
			flShiftX = -rcScreen.left;
		}
		else if(fRightJustify)
		{
			float flNeededPos = rcScreen.right - rcBounds.width();
			flShiftX = -flNeededPos;
		}
		else
		{
			flShiftX = rcBounds.exactCenterX() - rcScreen.exactCenterX();
		}
		final float flShiftY = rcBounds.exactCenterY() - rcScreen.exactCenterY();
		//c.clipRect(new Rect(0,0,rcScreen.right,320), Op.REPLACE);
		c.drawText(str, -flShiftX, -flShiftY, p);
	}
	public static void DrawFontInBox(Canvas c, final String str, Paint p, final Rect rcScreen, boolean fActuallyDraw)
	{
		final int cxPermitted = rcScreen.right - rcScreen.left;
		final int cyPermitted = rcScreen.bottom - rcScreen.top;
		int iFontHigh = 200;
		int iFontLow = 10;
		
		Rect rcBounds = new Rect();
		while(true)
		{
			if(iFontHigh - iFontLow <= 1)
			{
				// we've found the answer
				if(fActuallyDraw)
				{
					DrawFontInBoxFinal(c, str, iFontLow, p, rcScreen, false,false);
				}
				else
				{
					p.setTextSize(iFontLow);
				}
				return;
			}
			else
			{
				int iFontCheck = (iFontHigh+iFontLow)/2;
				p.setTextSize(iFontCheck);
				p.getTextBounds(str, 0, str.length(), rcBounds);
				final int cxFont = rcBounds.right - rcBounds.left;
				final int cyFont = rcBounds.bottom - rcBounds.top;
				final boolean fTooBig = cxFont > cxPermitted || cyFont > cyPermitted;
				if(fTooBig)
				{
					iFontHigh = iFontCheck;
				}
				else
				{
					iFontLow = iFontCheck;
				}
			}
		}
	}
	public static String GetImagePathFromURI(Activity activity, Uri uri)
	{
        String[] projection = { MediaStore.Images.Media.DATA };
        Cursor cursor = activity.managedQuery(uri, projection, null, null, null);
        if(cursor!=null)
        {
            //HERE YOU WILL GET A NULLPOINTER IF CURSOR IS NULL
            //THIS CAN BE, IF YOU USED OI FILE MANAGER FOR PICKING THE MEDIA
            int column_index = cursor.getColumnIndexOrThrow(MediaStore.Images.Media.DATA);
            cursor.moveToFirst();
            return cursor.getString(column_index);
        }
        else return null;
	}
	public interface MultiStateObject
	{
		public enum STATE {ON, OFF, TROUBLE_GOOD, TROUBLE_BAD};

		public static class StateData
		{
			public StateData(STATE eState, String str) {this.eState = eState; strDesc = str;}
			
			@Override
			public int hashCode()
			{
				return eState.hashCode() ^ (strDesc != null ? strDesc.hashCode() : 0);
			}
			
			public STATE eState;
			public String strDesc;
		}
		
		public abstract void SetState(Class c, STATE eState, String strData);
		public abstract StateData GetState(Class c);
	}
}
