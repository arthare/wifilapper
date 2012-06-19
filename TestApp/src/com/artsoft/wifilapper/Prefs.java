package com.artsoft.wifilapper;

import java.text.NumberFormat;
import java.util.List;

import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;

public class Prefs 
{
	// name of the pref file we use
	public static String SHAREDPREF_NAME = "RacingPrefs";
	
	// the strings we use as keys in SharedPreferences
	public static String PREF_SPEEDOSTYLE_STRING = "Speedostyle";
	public static String PREF_TESTMODE_BOOL = "TestMode";
	public static String PREF_IP_STRING = "IP"; // stores the target IP of their pitside laptop
	public static String PREF_SSID_STRING = "SSID"; // stores the ssid of the wifi network they use
	public static String PREF_BTGPSNAME_STRING = "btgps"; // stores the bluetooth name of their preferred btgps unit
	public static String PREF_BTOBD2NAME_STRING = "btobd2"; // stores the bluetooth name of their preferred btgps unit
	public static String PREF_RACENAME_STRING = "racename";
	public static String PREF_UNITS_STRING = "displayunits";
	public static String PREF_DBLOCATION_BOOL = "dbInternal";
	public static String PREF_USEIOIO_BOOLEAN = "useioio";
	public static String PREF_USEACCEL_BOOLEAN = "useaccel";
	public static String PREF_ACCEL_GRAV_X = "gravx";
	public static String PREF_ACCEL_GRAV_Y = "gravy";
	public static String PREF_ACCEL_GRAV_Z = "gravz";
	public static String PREF_ACKSMS_BOOLEAN = "acksms";
	public static String PREF_PRIVACYPREFIX_STRING ="privacy";
	
	// the strings we use in for defaults when a SharedPreference isn't available
	public static String DEFAULT_IP_STRING = "192.168.1.100";
	public static String DEFAULT_SSID_STRING = "belkinsucks";
	public static String DEFAULT_GPS_STRING = "";
	public static String DEFAULT_OBD2_STRING = "";
	public static String DEFAULT_RACENAME_STRING = "race";
	public static String DEFAULT_SPEEDOSTYLE_STRING = LandingOptions.rgstrSpeedos[0];
	public static boolean DEFAULT_TESTMODE_BOOL = false;
	public static Prefs.UNIT_SYSTEM DEFAULT_UNITS_STRING = Prefs.UNIT_SYSTEM.MPH;
	public static boolean DEFAULT_DBLOCATION_BOOL = true; // whether the DB is on internal storage
	public static boolean DEFAULT_USEACCEL = true; 
	public static boolean DEFAULT_ACKSMS = false; // whether to send a text to acknowledge a text
	public static String DEFAULT_PRIVACYPREFIX = "wflp";
	
	
	// strings we use for extra data in intents
	public static String IT_IP_STRING = PREF_IP_STRING; 
	public static String IT_SSID_STRING = PREF_SSID_STRING;
	public static String IT_BTGPS_STRING = PREF_BTGPSNAME_STRING;
	public static String IT_BTOBD2_STRING = PREF_BTOBD2NAME_STRING;
	public static String IT_STARTFINISH_ARRAY = "sf";
	public static String IT_STARTFINISHDIR_ARRAY = "sfdir";
	public static String IT_RACENAME_STRING = PREF_RACENAME_STRING;
	public static String IT_TESTMODE_BOOL = "testmode";
	public static String IT_RACEID_LONG = "raceid";
	public static String IT_LAPLOADMODE_LONG = "laploadmode";
	public static String IT_SPEEDOSTYLE_STRING = PREF_SPEEDOSTYLE_STRING;
	public static String IT_UNITS_STRING = PREF_UNITS_STRING;
	public static String IT_SELECTEDPIDS_ARRAY = "selectedpids";
	public static String IT_IOIOANALPINS_ARRAY = "selectedanalpins";
	public static String IT_IOIOPULSEPINS_ARRAY = "selectedpulsepins";
	public static String IT_USEACCEL_BOOLEAN = PREF_USEACCEL_BOOLEAN;
	public static String IT_ACKSMS_BOOLEAN = PREF_ACKSMS_BOOLEAN;
	public static String IT_PRIVACYPREFIX_STRING = PREF_PRIVACYPREFIX_STRING;
	
	public enum UNIT_SYSTEM {KMH, MPH, MS};
	
	public static String FormatMetersPerSecond(float fl, NumberFormat num, UNIT_SYSTEM eSystem)
	{
		float flConverted = 0;
		String strSuffix;
		switch(eSystem)
		{
		case KMH: flConverted = fl * 3.6f; strSuffix = "km/h"; break;
		case MPH: flConverted = fl * 2.23693629f; strSuffix = "mph"; break;
		case MS: flConverted = fl; strSuffix = "m/s"; break;
		default: strSuffix = ""; break;
		}
		String strFormat = num.format(flConverted);
		return strFormat;
	}
	public static void LoadOBD2PIDs(SharedPreferences settings, List<Integer> lstOutCodes)
	{
		for(int x = 0;x < 256; x++)
		{
			if(settings.getBoolean("pid" + x, false))
			{
				lstOutCodes.add(new Integer(x));
			}
		}
	}
	
	private static SharedPreferences.Editor SavePins(SharedPreferences.Editor settings, List<IOIOManager.PinParams>lst, String strSuffix)
	{
		// clear out all the old pins
		for(int x = 0;x < 48; x++)
		{
			settings = settings.putBoolean("ioiopin" + strSuffix + x, false);
		}
		
		for(int x = 0;x < lst.size(); x++)
		{
			IOIOManager.PinParams pin = lst.get(x);
			settings = settings.putBoolean("ioiopin" + strSuffix + pin.iPin,true);
			settings = settings.putInt("ioiopinperiod" + strSuffix + pin.iPin, pin.iPeriod);
			settings = settings.putInt("ioiopintype" + strSuffix + pin.iPin, pin.iFilterType);
			settings = settings.putFloat("ioiopinparam1" + strSuffix + pin.iPin, (float)pin.dParam1);
			settings = settings.putFloat("ioiopinparam2" + strSuffix + pin.iPin, (float)pin.dParam2);
			settings = settings.putInt("ioiopincustomtype" + strSuffix + pin.iPin, pin.iCustomType);
		}
		return settings;
	}
	public static SharedPreferences.Editor SavePins(SharedPreferences.Editor settings, List<IOIOManager.PinParams>lstAnal, List<IOIOManager.PinParams>lstPulse)
	{
		settings = SavePins(settings, lstAnal, "");
		settings = SavePins(settings, lstPulse, "pulse");
		return settings;
	}
	private static IOIOManager.PinParams[] LoadPins(SharedPreferences settings, String strSuffix)
	{
		int c = 0;
		for(int x = 0;x < 48; x++)
		{
			if(settings.getBoolean("ioiopin" + strSuffix + x, false))
			{
				c++;
			}
		}
		IOIOManager.PinParams rgPins[] = new IOIOManager.PinParams[c];
		c = 0;
		for(int x = 0;x < 48; x++)
		{
			if(settings.getBoolean("ioiopin" + strSuffix + x, false))
			{
				int iPeriod = settings.getInt("ioiopinperiod" + strSuffix + x, 1000);
				int iType = settings.getInt("ioiopintype" + strSuffix + x, IOIOManager.PinParams.FILTERTYPE_NONE);
				double dParam1 = settings.getFloat("ioiopinparam1" + strSuffix + x, 0);
				double dParam2 = settings.getFloat("ioiopinparam2" + strSuffix + x, 0);
				int iCustomType = settings.getInt("ioiopincustomtype" + strSuffix + x, 0);
				rgPins[c] = new IOIOManager.PinParams(x,iPeriod, iType, dParam1, dParam2, iCustomType);
				c++;
			}
		}
		return rgPins;
	}
	
	public static IOIOManager.PinParams[] LoadIOIOAnalPins(SharedPreferences settings)
	{
		return LoadPins(settings,"");
	}
	public static IOIOManager.PinParams[] LoadIOIOPulsePins(SharedPreferences settings)
	{
		return LoadPins(settings,"pulse");
	}
	public static void SaveSharedPrefs(SharedPreferences prefs, String strIP, String strSSID, String strGPS, String strRaceName)
	{
		Editor edit = prefs.edit();
		if(strIP != null) edit = edit.putString(Prefs.PREF_IP_STRING, strIP);
		if(strGPS != null) edit = edit.putString(Prefs.PREF_BTGPSNAME_STRING, strGPS);
		if(strSSID != null) edit = edit.putString(Prefs.PREF_SSID_STRING, strSSID);
		if(strRaceName != null) edit = edit.putString(Prefs.PREF_RACENAME_STRING,strRaceName);
		edit.commit();
	}
}
