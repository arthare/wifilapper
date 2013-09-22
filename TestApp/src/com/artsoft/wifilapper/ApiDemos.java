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
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Region.Op;

import android.net.Uri;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Debug;
import android.os.Handler;
import android.os.Message;
import android.os.Parcelable;
import android.location.*;
import android.util.AttributeSet;
import android.util.Log;
import android.view.*;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;
import android.graphics.*;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;

import java.util.Vector;

import com.artsoft.wifilapper.IOIOManager.IOIOListener;
import com.artsoft.wifilapper.LapAccumulator.DataChannel;
import com.artsoft.wifilapper.LapSender.LapSenderListener;

public class ApiDemos 
extends Activity 
implements 
	LocationListener, 
	Utility.MultiStateObject, 
	OnClickListener, 
	Handler.Callback, 
	SensorEventListener,
	MessageMan.MessageReceiver, 
	OBDThread.OBDListener, IOIOListener, LapSenderListener
{
	public enum RESUME_MODE {NEW_RACE, REUSE_SPLITS, RESUME_RACE};
	
	enum State {LOADING, WAITING_FOR_GPS, WAITINGFORSTART, WAITINGFORSTOP, MOVING_TO_STARTLINE, PLOTTING, DEMOSCREEN};
	
	private State m_eState;
	
	private LapSender m_lapSender;
	private MessageMan m_msgMan;
	private BluetoothGPS m_btgps = null;
	private OBDThread m_obd = null;
	private IOIOManager m_ioio = null;
	
	private SplitDecider m_startDecider;
	private SplitDecider m_stopDecider;
	
	private boolean m_fUseP2P;
	private int m_iP2PStartMode;
	private float m_flP2PStartParam;
	private int m_iP2PStopMode;
	private float m_flP2PStopParam;
	
	// message-man stuff
	private String m_strMessage;
	private String m_strMessagePhone; // phone number that sent this message
	private String m_strPrivacyPrefix; // the privacy prefix to use when determining whether to show a text
	private boolean m_fAcknowledgeBySMS; // whether this instance of the app will acknowledge SMSes with SMSes
	private boolean m_fSupportSMS; // whether we support SMS stuff
	private int m_iMsgTime; // the time, in (System.currentTimeMillis()/1000) until which we will display the message
	
	private View m_currentView;
	private StatusBarView m_statusBar;
	// threads
	private Handler m_pHandler;
		
	// lapping
	private LapAccumulator m_myLaps = null; // current lap
	private LapAccumulator.DataChannel m_XAccel;
	private LapAccumulator.DataChannel m_YAccel;
	private LapAccumulator.DataChannel m_ZAccel;
	
	private LapAccumulator.DataChannel m_XReception;
	private LapAccumulator.DataChannel m_YReception;
	private Map<Integer,DataChannel> m_mapPIDS;
	private Map<Integer,DataChannel> m_mapPins;
	
	private long m_tmLastLap = 0; // the system time that we saved the last lap at
	private LapAccumulator m_lastLap = null;
	private LapAccumulator m_driverBest = null;
	private LapAccumulator m_best = null;
	
	private LapAccumulator.LapAccumulatorParams m_lapParams = null;
	
	Point2D m_ptCurrent; // the most recent point to come in from the location manager
	public float m_tmCurrent; // the time (in seconds) of the m_ptCurrent
	Point2D m_ptLast; // the point that came before that one
	public float m_tmLast; // the time (in seconds) of m_ptLast
	
	float m_dLastSpeed; // how fast (in m/s) we were going at the last GPS point
	float m_dLastTime; // the time (in seconds) of the last point we received
	
	private boolean m_fRecordReception; // whether we're currently recording reception.  This gets set when the wifi class tells us it is connected/disconnected
	
	// data about this race
	private String m_strRaceName;
	private boolean m_fTestMode;
	private long m_lRaceId;
	private String m_strSpeedoStyle;
	private Prefs.UNIT_SYSTEM m_eUnitSystem;
	
	private long rgLastGPSGaps[] = new long[10];
	private int iLastGPSGap = 0;
	// the time (in milliseconds since 1970) of the first location received
	// times recorded in LapAccumulators are expressed in milliseconds since this time.
	// this allows us to store them as 32-bit integers (since it is unlikely anyone will be participating in a 2^32 millisecond-long race)
	long m_tmAppStartTime = 0; 
	// the time (in milliseconds since the phone startup) of the first location received.
	// times sent to us by the SensorManager are expressed in time since the phone started,
	// so we need this value to convert them into "time since first location" like the other data in the app
	long m_tmAppStartUptime = 0;
	
	private FakeLocationGenerator m_thdFakeLocs = null;
	
	private static final int MSG_STATE_CHANGED = 50;
	public static final int MSG_FAKE_LOCATION = 51;
	private static final int MSG_LOADING_PROGRESS = 52;
	private static final int MSG_IOIO_BUTTON = 53;
	
	
	private PendingIntent m_restartIntent;
	
	private static final int RESTART_NOTIFICATION_ID = 1;
	
	private static ApiDemos m_me;
	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) 
    {
    	super.onCreate(savedInstanceState);
    	synchronized(ApiDemos.class)
    	{
    		m_me = this;
    	}
    	
    	m_tmAppStartTime = 0;
    	setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
    	
    	getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN | WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    	
    	m_pHandler = new Handler(this);

    	Intent i = this.getIntent();
    	String strIP = i.getStringExtra(Prefs.IT_IP_STRING);
    	String strSSID = i.getStringExtra(Prefs.IT_SSID_STRING);
    	String strBTGPS = i.getStringExtra(Prefs.IT_BTGPS_STRING);
    	String strOBD2 = i.getStringExtra(Prefs.IT_BTOBD2_STRING);
    	
    	final boolean fUseAccel = i.getBooleanExtra(Prefs.IT_USEACCEL_BOOLEAN, Prefs.DEFAULT_USEACCEL);
    	m_strRaceName = i.getStringExtra(Prefs.IT_RACENAME_STRING);
    	m_lRaceId = i.getLongExtra(Prefs.IT_RACEID_LONG, -1);
    	m_fTestMode = i.getBooleanExtra(Prefs.IT_TESTMODE_BOOL, false);
    	m_strSpeedoStyle = i.getStringExtra(Prefs.IT_SPEEDOSTYLE_STRING);
    	m_fAcknowledgeBySMS = i.getBooleanExtra(Prefs.IT_ACKSMS_BOOLEAN, true);
    	m_strPrivacyPrefix = i.getStringExtra(Prefs.IT_PRIVACYPREFIX_STRING);
    	
    	m_fUseP2P = i.getBooleanExtra(Prefs.IT_P2P_ENABLED, false);
    	m_iP2PStartMode = i.getIntExtra(Prefs.IT_P2P_STARTMODE, Prefs.DEFAULT_P2P_STARTMODE);
    	m_flP2PStartParam = i.getFloatExtra(Prefs.IT_P2P_STARTPARAM, Prefs.DEFAULT_P2P_STARTPARAM);
    	m_iP2PStopMode = i.getIntExtra(Prefs.IT_P2P_STOPMODE, Prefs.DEFAULT_P2P_STOPMODE);
    	m_flP2PStopParam = i.getFloatExtra(Prefs.IT_P2P_STOPPARAM, Prefs.DEFAULT_P2P_STOPPARAM);
    	boolean fRequireWifi = i.getBooleanExtra(Prefs.IT_REQUIRE_WIFI, Prefs.DEFAULT_REQUIRE_WIFI);
    	
    	if(m_strPrivacyPrefix == null) m_strPrivacyPrefix = Prefs.DEFAULT_PRIVACYPREFIX;
    	
    	if(m_strSpeedoStyle == null) m_strSpeedoStyle = LandingOptions.rgstrSpeedos[0];
    	String strUnitSystem = i.getStringExtra(Prefs.IT_UNITS_STRING);
    	int rgSelectedPIDs[] = i.getIntArrayExtra(Prefs.IT_SELECTEDPIDS_ARRAY);
    	
    	Parcelable[] rgAnalParcel = i.getParcelableArrayExtra(Prefs.IT_IOIOANALPINS_ARRAY); // an array of indices indicating which pins we want to use
		Parcelable[] rgPulseParcel = i.getParcelableArrayExtra(Prefs.IT_IOIOPULSEPINS_ARRAY); // an array of indices indicating which pins we want to use
		IOIOManager.PinParams rgSelectedAnalPins[] = new IOIOManager.PinParams[rgAnalParcel.length];
		IOIOManager.PinParams rgSelectedPulsePins[] = new IOIOManager.PinParams[rgPulseParcel.length];
		int iButtonPin = i.getIntExtra(Prefs.IT_IOIOBUTTONPIN, Prefs.DEFAULT_IOIOBUTTONPIN);
		
		for(int x = 0;x < rgAnalParcel.length; x++) rgSelectedAnalPins[x] = (IOIOManager.PinParams)rgAnalParcel[x];
		for(int x = 0;x < rgPulseParcel.length; x++) rgSelectedPulsePins[x] = (IOIOManager.PinParams)rgPulseParcel[x];
		
    	m_eUnitSystem = Prefs.UNIT_SYSTEM.valueOf(strUnitSystem);
    	
    	int idLapLoadMode = (int)i.getLongExtra(Prefs.IT_LAPLOADMODE_LONG,RESUME_MODE.REUSE_SPLITS.ordinal());
    	
    	if(idLapLoadMode == RESUME_MODE.REUSE_SPLITS.ordinal())
    	{
    		// they only want to use the waypoints from this race.
    		m_lRaceId = -1;
    	}
    	
    	try
    	{
	    	ActivityInfo info = getPackageManager().getActivityInfo(getComponentName(), PackageManager.GET_META_DATA);
	    	String strAppMode = info.metaData.getString("appmode");
	    	m_fSupportSMS = !strAppMode.equals("tablet");
    	}
    	catch(Exception e)
    	{
    		m_fSupportSMS = true; // default to trying to SMS
    	}
    	
    	m_lapParams = (LapAccumulator.LapAccumulatorParams)i.getParcelableExtra(Prefs.IT_LAPPARAMS);
    	
    	StartupTracking(fRequireWifi, rgSelectedAnalPins, rgSelectedPulsePins, iButtonPin, rgSelectedPIDs, strIP, strSSID, strBTGPS, strOBD2, fUseAccel, m_fTestMode, idLapLoadMode);
    }
    public static Intent BuildStartIntent(boolean fRequireWifi, IOIOManager.PinParams rgAnalPins[], IOIOManager.PinParams rgPulsePins[], int iButtonPin, boolean fPointToPoint, int iStartMode, float flStartParam, int iStopMode, float flStopParam, List<Integer> lstSelectedPIDs, Context ctxApp, String strIP, String strSSID, LapAccumulator.LapAccumulatorParams lapParams, String strRaceName, String strPrivacy, boolean fAckSMS, boolean fUseAccel, boolean fTestMode, long idRace, long idModeSelected, String strBTGPS, String strBTOBD2, String strSpeedoStyle, String strUnitSystem)
    {
    	Intent myIntent = new Intent(ctxApp, ApiDemos.class);
    	myIntent.putExtra(Prefs.IT_REQUIRE_WIFI, fRequireWifi);
		myIntent.putExtra(Prefs.IT_IP_STRING, strIP).putExtra("SSID", strSSID);
		myIntent.putExtra(Prefs.IT_LAPPARAMS, lapParams);
		myIntent.putExtra(Prefs.IT_RACENAME_STRING, strRaceName);
		myIntent.putExtra(Prefs.IT_TESTMODE_BOOL, (boolean)fTestMode);
		myIntent.putExtra(Prefs.IT_RACEID_LONG, (long)idRace);
		myIntent.putExtra(Prefs.IT_LAPLOADMODE_LONG, (long)idModeSelected);
		myIntent.putExtra(Prefs.IT_BTGPS_STRING, strBTGPS);
		myIntent.putExtra(Prefs.IT_BTOBD2_STRING, strBTOBD2);
		myIntent.putExtra(Prefs.IT_SPEEDOSTYLE_STRING, strSpeedoStyle);
		myIntent.putExtra(Prefs.IT_UNITS_STRING, strUnitSystem);
		myIntent.putExtra(Prefs.IT_USEACCEL_BOOLEAN, fUseAccel);
		myIntent.putExtra(Prefs.IT_ACKSMS_BOOLEAN, fAckSMS);
		myIntent.putExtra(Prefs.IT_PRIVACYPREFIX_STRING, strPrivacy);
		myIntent.putExtra(Prefs.IT_IOIOBUTTONPIN, iButtonPin);
		myIntent.putExtra(Prefs.IT_P2P_ENABLED, fPointToPoint);
		myIntent.putExtra(Prefs.IT_P2P_STARTMODE, iStartMode);
		myIntent.putExtra(Prefs.IT_P2P_STARTPARAM, flStartParam);
		myIntent.putExtra(Prefs.IT_P2P_STOPMODE, iStopMode);
		myIntent.putExtra(Prefs.IT_P2P_STOPPARAM, flStopParam);
		{
			int rgArray[] = new int[lstSelectedPIDs.size()];
			for(int x = 0;x < rgArray.length; x++) rgArray[x] = lstSelectedPIDs.get(x).intValue();
			myIntent.putExtra(Prefs.IT_SELECTEDPIDS_ARRAY, rgArray);
		}
		
		{
			myIntent.putExtra(Prefs.IT_IOIOANALPINS_ARRAY, rgAnalPins);
			myIntent.putExtra(Prefs.IT_IOIOPULSEPINS_ARRAY, rgPulsePins);
		}
		
		return myIntent;
    }
    @Override
    public void onDestroy()
    {
    	super.onDestroy();
    	synchronized(ApiDemos.class)
    	{
    		m_me = null;
    	}
    	ShutdownLappingMode();
    }
    @Override
    public void onPause()
    {
    	super.onPause();
    	
    	Notification notification = new Notification(R.drawable.icon, "Wifilapper is still recording data", System.currentTimeMillis());
    	// we need to put up a little notification at the top to tell them we're still running.
    	// we need to set a bunch of persisted settings so that the next LandingScreen activity knows to hop to this 
    	Intent toLaunch = new Intent(getApplicationContext(),ApiDemos.class);
    	
		toLaunch.setAction("android.intent.action.MAIN");
		
		toLaunch.addCategory("android.intent.category.LAUNCHER");           
		
		
		PendingIntent intentBack = PendingIntent.getActivity(getApplicationContext(),   0,toLaunch, PendingIntent.FLAG_UPDATE_CURRENT);
		
		notification.setLatestEventInfo(getApplicationContext(),"WifiLapper is still running", "Click here to open the app.  You can use menu->quit to exit.", intentBack);
        
		NotificationManager mNotificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
		mNotificationManager.notify(RESTART_NOTIFICATION_ID, notification);
		
		m_restartIntent = intentBack;
    }
    public void onResume()
    {
    	super.onResume();
    }
    
    public static ApiDemos Get()
    {
    	synchronized(ApiDemos.class)
    	{
    		return m_me;
    	}
    }

	
    public void NotifyNewSMS(String strSMS, String strPhoneNumber)
    {
    	if(!m_fSupportSMS) return;
    	
    	final String strWifiIP = "setip";
    	final String strWifiSSID = "setssid";
    	
    	// someone has texted me.  How quaint
    	String strLowercase = strSMS.toLowerCase();
    	if(m_strPrivacyPrefix.length() <= 0 || strLowercase.startsWith(m_strPrivacyPrefix))
    	{
    		m_strMessage = strSMS.substring(4);
    		m_strMessagePhone = strPhoneNumber;
    		m_iMsgTime = (int)((System.currentTimeMillis()/1000) + 1*60); // show this message for a maximum of 4 minutes
    	}
    	else if(strLowercase.startsWith(strWifiIP))
    	{
    		String strIP = strLowercase.substring(strWifiIP.length());
    		m_lapSender.SetIP(strIP);
    	}
    	else if(strLowercase.startsWith(strWifiSSID))
    	{
    		String strSSID = strLowercase.substring(strWifiSSID.length());
    		m_lapSender.SetSSID(strSSID);
    	}
    	else
    	{
    		// not prefixed with privacy prefix, so ignore
    	}
    }
    
    @Override
    public boolean onCreateOptionsMenu(Menu menu) 
    {
		MenuInflater inflater = getMenuInflater();
		inflater.inflate(R.menu.closemenu, menu);
    	return true;
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
    private void ShutdownLappingMode()
    {
    	if(m_lapSender != null)
		{
			m_lapSender.Shutdown();
			m_lapSender = null;
		}
		if(this.m_thdFakeLocs != null)
		{
			m_thdFakeLocs.Shutdown();
			m_thdFakeLocs = null;
		}
		if(this.m_msgMan != null)
		{
			m_msgMan.Shutdown();
			m_msgMan = null;
		}
		SensorManager sensorMan = (SensorManager)getSystemService(SENSOR_SERVICE);
	    if(sensorMan != null)
	    {
	    	sensorMan.unregisterListener(this);
	    }
	    if(m_restartIntent != null)
	    {
	    	m_restartIntent.cancel();
	    }
		NotificationManager mNotificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
		mNotificationManager.cancel(RESTART_NOTIFICATION_ID);
		if(m_obd != null)
		{
			m_obd.Shutdown();
		}
		if(m_btgps != null)
		{
			m_btgps.Shutdown();
		}
		if(m_ioio != null)
		{
			m_ioio.Shutdown();
		}
		
		LocationManager locMan = (LocationManager)getApplicationContext().getSystemService(Context.LOCATION_SERVICE);
		if(locMan != null)
		{
			locMan.removeUpdates(this);
		}
		finish();
    }
    
    @Override
    public boolean onOptionsItemSelected(MenuItem menu)
    {
    	if(menu.getItemId() == R.id.mnuClose)
    	{
    		ShutdownLappingMode();
    		return true;
    	}
    	else if(menu.getItemId() == R.id.mnuBuyFull)
    	{
    		Intent intent = new Intent(Intent.ACTION_VIEW);
    		intent.setData(Uri.parse("market://details?id=com.artsoft.wifilapperfull"));
    		startActivity(intent);
    	}
    	return super.onOptionsItemSelected(menu);
    }
    @Override
    public void onConfigurationChanged(Configuration con)
    {
    	super.onConfigurationChanged(con);
    }
    // only called once the user has done all their settings stuff
    private void StartupTracking(boolean fRequireWifi, IOIOManager.PinParams rgAnalPins[], IOIOManager.PinParams rgPulsePins[], int iButtonPin, int rgSelectedPIDs[], String strIP, String strSSID, String strBTGPS, String strBTOBD2, boolean fUseAccel, boolean fTestMode, int idLapLoadMode)
    {
    	ApiDemos.State eEndState = ApiDemos.State.WAITING_FOR_GPS;

    	m_mapPIDS = new HashMap<Integer,DataChannel>();
    	m_mapPins = new HashMap<Integer,DataChannel>();

	    if(m_lapParams.IsValid(m_fUseP2P))
    	{
	    	if(m_fUseP2P)
	    	{
	    		m_startDecider = new LineSplitDecider(m_lapParams.lnStart, m_lapParams.vStart);
	    		m_stopDecider = new LineSplitDecider(m_lapParams.lnStop, m_lapParams.vStop);
	    	}
	    	else
	    	{
    			m_startDecider = m_stopDecider = new LineSplitDecider(m_lapParams.lnStart, m_lapParams.vStart);
	    	}
    		eEndState = State.WAITING_FOR_GPS;
    	}
    	else
    	{
    		// didn't include start-finish lines, so we have to start from the beginning
    		eEndState = State.WAITING_FOR_GPS;
    		SetupSplitDeciders(); // we'll need the automatic split deciders since splits were not included
    	}
	    
	    if(rgAnalPins.length > 0 || rgPulsePins.length > 0)
	    {
	    	m_ioio = new IOIOManager(this, this, rgAnalPins,rgPulsePins, iButtonPin);
	    }
	    else
	    {
	    	SetState(IOIOManager.class,STATE.OFF,"IOIO not selected");
	    }
	    
		WifiManager pWifi = (WifiManager)getSystemService(Context.WIFI_SERVICE);

	    m_lapSender = new LapSender(this, this, pWifi, strIP, strSSID, fRequireWifi);
	    m_msgMan = new MessageMan(this);
	    
	    if(m_lRaceId != -1 && m_lapParams.IsValid(m_fUseP2P))
	    {
	    	// they're resuming an old race
	    	m_lapSender.SetRaceId(m_lRaceId);
	    	LapAccumulator storedBestLap = RaceDatabase.GetBestLap(RaceDatabase.Get(), m_lapParams, m_lRaceId);
	    	if(storedBestLap != null)
	    	{
	    		m_best = storedBestLap;
	    		eEndState = State.LOADING;
	    		m_best.ThdDoDeferredLoad(this.m_pHandler, MSG_LOADING_PROGRESS, false);
	    	}
	    }
	    else if(m_lRaceId == -1 && m_lapParams.IsValid(m_fUseP2P))
	    {
	    	// they're running a new race with old splits
	    	m_lRaceId = RaceDatabase.CreateRaceIfNotExist(RaceDatabase.Get(), this.m_strRaceName, m_lapParams, fTestMode, m_fUseP2P, m_lapParams.iFinishCount);
	    	m_lapSender.SetRaceId(m_lRaceId);
	    }
	    
	    SetState(LocationManager.class,STATE.TROUBLE_GOOD,"Waiting for GPS...");
	    if(fTestMode)
	    {
	    	m_thdFakeLocs = new FakeLocationGenerator(m_pHandler,20);
	    }
	    else
	    {
	    	boolean fGPSSetup = false;
	    	if(strBTGPS.length() > 0)
	    	{
	    		// use our custom BT parser
	    		m_btgps = new BluetoothGPS(strBTGPS, this);
	    		fGPSSetup = m_btgps != null && m_btgps.IsValid();
	    	}
	    	if(m_btgps == null || !m_btgps.IsValid())
	    	{
			    LocationManager locMan = (LocationManager)getApplicationContext().getSystemService(Context.LOCATION_SERVICE);
			    if(locMan != null && locMan.isProviderEnabled(LocationManager.GPS_PROVIDER))
			    {
			    	try
			    	{
			    		locMan.requestLocationUpdates(LocationManager.GPS_PROVIDER, 100, 0.0f, (LocationListener)this);
			    		fGPSSetup = true;
			    	}
			    	catch(Exception e)
			    	{
			    		System.out.println("Failure: " + e.toString());
			    	}
			    }
	    	}
	    	if(!fGPSSetup)
	    	{
	    	    SetState(LocationManager.class,STATE.TROUBLE_BAD,"Failed to initialize GPS.  Is your device's GPS enabled or your external GPS on?");
	    	}
	    }
	    
	    SensorManager sensorMan = (SensorManager)getSystemService(SENSOR_SERVICE);
	    if(sensorMan != null)
	    {
	    	if(fUseAccel)
	    	{
		    	Sensor accel = sensorMan.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
		    	if(accel != null)
		    	{
		    		sensorMan.registerListener(this, accel, SensorManager.SENSOR_DELAY_GAME);
		    	}
	    	}
	    }
	    
	    m_dLastSpeed = 0;
	    m_ptLast = null;
	    m_ptCurrent = null;
	    
	    if(strBTOBD2 != null && strBTOBD2.length() > 0 && rgSelectedPIDs != null && rgSelectedPIDs.length> 0)
	    {
	    	m_obd = new OBDThread(this, this, strBTOBD2, rgSelectedPIDs);
	    }
	    else
	    {
	    	SetState(OBDThread.class, STATE.OFF, "Not using OBD2, invalid device selected, or no parameters selected.");
	    }
	    
	    this.SetState(eEndState);
    }
    
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) 
    {
        //Handle the back button
        if(keyCode == KeyEvent.KEYCODE_BACK || keyCode == KeyEvent.KEYCODE_HOME) 
        {
            //Ask the user if they want to quit
            new AlertDialog.Builder(this)
            .setIcon(android.R.drawable.ic_dialog_alert)
            .setTitle(R.string.quit)
            .setMessage(R.string.really_quit)
            .setPositiveButton(R.string.yes, new DialogInterface.OnClickListener() 
            {
                @Override
                public void onClick(DialogInterface dialog, int which) 
                {
                    //Stop the activity
                    ApiDemos.this.ShutdownLappingMode();    
                }

            }).setNegativeButton(R.string.no, null).show();

            return true;
        }
        else {
            return super.onKeyDown(keyCode, event);
        }

    }
    
    // click stuff
    @Override
    public void onClick(View v)
    {
    	if(m_eState == State.PLOTTING)
		{
    		// Reset the best lap time if someone clicked the screen while stopped (speed less than 7.2kph)
    		if( m_dLastSpeed < 2 ) m_best=null;
		}
	    	
    	if(m_eState == State.WAITINGFORSTART)
    	{
    		if(IsReadyForLineSet())
			{
    			m_startDecider.Tap();
    			if(m_startDecider.IsReady())
    			{
    				m_lapParams.lnStart = m_startDecider.GetLine();
    				m_lapParams.vStart = m_startDecider.GetDir();
    				if(m_stopDecider.IsReady())
    				{
    					m_lapParams.lnStop = m_stopDecider.GetLine();
    					m_lapParams.vStop = m_stopDecider.GetDir();
    					TrackLastLap(m_myLaps,true,false);
    				}
    				SetState(State.WAITINGFORSTOP);
    			}
			}
    	}
    	else if(m_eState == State.WAITINGFORSTOP)
    	{
    		if(IsReadyForLineSet())
    		{
    			m_stopDecider.Tap();
    			if(m_stopDecider.IsReady())
    			{
    				m_lapParams.lnStop = m_stopDecider.GetLine();
    				m_lapParams.vStop = m_stopDecider.GetDir();
					TrackLastLap(m_myLaps,true,false);
    				SetState(State.MOVING_TO_STARTLINE);
    			}
    		}
    	}
    	// if they tap the screen, clear the current message
    	
    	AcknowledgeMessage();
    }
    public String[] GetDeciderWaitingStrings()
    {
    	if(!m_startDecider.IsReady())
    	{
    		return m_startDecider.GetUnReadyStrings();
    	}
    	else if(!m_stopDecider.IsReady())
    	{
    		return m_stopDecider.GetUnReadyStrings();
    	}
    	else
    	{
    		String str[] = new String[2];
    		str[0] = "Bugs";
    		str[1] = "Complain at wifilapper.freeforums.org";
    		return str;
    	}
    }
    private void SetupSplitDeciders()
    {
    	if(m_fUseP2P)
    	{
    		// they're doing a point-to-point, so we need to be careful about how we set up the start and finish deciders...
    		switch(m_iP2PStartMode)
    		{
    		case Prefs.P2P_STARTMODE_ACCEL:
    			//m_startDecider = new AccelSplitDecider();
    			break;
    		case Prefs.P2P_STARTMODE_SCREEN:
    			m_startDecider = new LineSplitDecider("Tap to set start line");
    			break;
    		case Prefs.P2P_STARTMODE_SPEED:
    			m_startDecider = new SpeedSplitDecider(false, false, m_flP2PStartParam, m_eUnitSystem, "Start");
    			break;
    		}
    		switch(m_iP2PStopMode)
    		{
    		case Prefs.P2P_STOPMODE_DISTANCE:
    			m_stopDecider = new DistanceSplitDecider(m_flP2PStopParam, m_eUnitSystem);
    			break;
    		case Prefs.P2P_STOPMODE_SCREEN:
    			m_stopDecider = new LineSplitDecider("Tap to set finish line");
    			break;
    		case Prefs.P2P_STOPMODE_SPEED:
    			m_stopDecider = new SpeedSplitDecider(true, true, m_flP2PStopParam, m_eUnitSystem, "Finish");
    			break;
    		}
    	}
    	else
    	{
    		// just doing laps, so just use the normal split points
    		this.m_startDecider = new LineSplitDecider("Tap to set start/finish line");
    		this.m_stopDecider = m_startDecider; // if they're lapping, the start and end lines are identical
    	}
    }
    
    private void AcknowledgeMessage()
    {
    	if(m_fSupportSMS && m_fAcknowledgeBySMS && m_strMessage != null && m_strMessagePhone != null)
    	{
    		final int maxLen = 20;
    		String strEllipsis = m_strMessage.length() > maxLen ? "..." : "";
    		try
    		{
    			SMSReceiver.SMSAcknowledgeMessage("Driver acknowledged '" + m_strMessage.substring(0,Math.min(m_strMessage.length(),20)) + strEllipsis + "'", m_strMessagePhone);
    		}
    		catch(Exception e)
    		{
        		Toast.makeText(this, "Exception: " + e.toString(), Toast.LENGTH_SHORT).show();
    		}
    	}
    	this.m_strMessagePhone = null;
    	this.m_strMessage = null;
    }
    public boolean IsReadyForLineSet()
    {
    	return m_ptCurrent != null && m_ptLast != null && m_dLastSpeed > 0.5;
    }
    private void TrackLastLap(LapAccumulator lap, boolean fTransmit, boolean fSaveAsLastLap)
    {
    	if(lap == null) return;
    	
    	lap.ForceFinish();
		if(m_lRaceId < 0 && m_lapParams.IsValid(m_fUseP2P))
		{
			m_lRaceId = RaceDatabase.CreateRaceIfNotExist(RaceDatabase.Get(), this.m_strRaceName, m_lapParams, this.m_fTestMode, m_fUseP2P, this.m_lapParams.iFinishCount);
			m_lapSender.SetRaceId(m_lRaceId);
		}
    	m_tmLastLap = System.currentTimeMillis();
    	if(m_lapSender != null)
    	{
    		if(fSaveAsLastLap)
    		{
    			m_lastLap = lap.CreateCopy();
		    	if(m_driverBest == null || lap.GetLapTime() < m_driverBest.GetLapTime())
		    	{
		    		m_driverBest = lap.CreateCopy();
		    	}
		    	if(m_best == null || lap.GetLapTime() < m_best.GetLapTime())
		    	{
		    		m_best = lap.CreateCopy();
		    	}
	    	}
	    	if(fTransmit)
	    	{
	    		m_lapSender.SendLap(lap);
	    	}
    	}
    }

	@Override
	public void onAccuracyChanged(Sensor arg0, int arg1) 
	{
		// don't care!
	}

	@Override
	public void onSensorChanged(SensorEvent event) 
	{
		if(m_myLaps == null) return;
		
		long lTimeMs = event.timestamp / 1000000;
		int iTimeSinceAppStart = (int)(lTimeMs - this.m_tmAppStartUptime);
		if(event.sensor.getType() == Sensor.TYPE_ACCELEROMETER)
		{
			if(m_XAccel == null || !m_XAccel.IsParent(m_myLaps)) m_XAccel = new DataChannel(DataChannel.CHANNEL_ACCEL_X,m_myLaps);
			if(m_YAccel == null || !m_YAccel.IsParent(m_myLaps)) m_YAccel = new DataChannel(DataChannel.CHANNEL_ACCEL_Y,m_myLaps);
			if(m_ZAccel == null || !m_ZAccel.IsParent(m_myLaps)) m_ZAccel = new DataChannel(DataChannel.CHANNEL_ACCEL_Z,m_myLaps);
			
			if(m_XAccel != null && m_YAccel != null && m_ZAccel != null)
			{
				m_XAccel.AddData(event.values[1] / 9.81f,iTimeSinceAppStart);
				m_YAccel.AddData(event.values[2] / 9.81f,iTimeSinceAppStart);
				m_ZAccel.AddData(event.values[0] / 9.81f,iTimeSinceAppStart);
			}
		}
	}
    // location stuff
    @Override
    public void onLocationChanged(Location location) 
    {
    	SetState(LocationManager.class, STATE.ON, "GPS Working (" + (int)(GetGPSRate()+0.5) + "hz)");
    	if(m_tmAppStartTime == 0)
    	{
    		m_tmAppStartTime = location.getTime();
    		m_tmAppStartUptime = System.nanoTime() / 1000000;
    	}
    	location.setTime(location.getTime() - m_tmAppStartTime);

    	float dNow;
		
		dNow = location.getTime() / 1000.0f;
		
		if(m_ptCurrent != null)
		{
	    	float rgDistance[] = new float[1];
	    	Location.distanceBetween(location.getLatitude(), location.getLongitude(), m_ptCurrent.GetY(), m_ptCurrent.GetX(), rgDistance);
    	
    		double tmChange = (location.getTime()/1000.0f) - m_tmCurrent; // time difference in seconds
    		double dSpeed = rgDistance[0] / tmChange; // location difference in meters
    		if(dSpeed > 100.0)
    		{
    			// speed is fairly unbelievable (360km/h).  Just skip doing anything
    			return;
    		}
    	}
    	
		int iUnixTime = (int)(System.currentTimeMillis() / 1000);
    	m_ptLast = m_ptCurrent;
    	m_tmLast = m_tmCurrent;
    	m_ptCurrent = new Point2D((float)location.getLongitude(), (float)location.getLatitude());
    	m_tmCurrent = dNow;
    
    	
    	if(this.m_fRecordReception && m_myLaps != null)
    	{
    		if(m_XReception == null || !m_XReception.IsParent(m_myLaps)) m_XReception = new DataChannel(DataChannel.CHANNEL_RECEPTION_X,m_myLaps);
    		if(m_YReception == null || !m_YReception.IsParent(m_myLaps)) m_YReception = new DataChannel(DataChannel.CHANNEL_RECEPTION_Y,m_myLaps);

    		int iTimeSinceAppStart = (int)((System.nanoTime()/1000000) - this.m_tmAppStartUptime);
    		
    		m_XReception.AddData((float)location.getLongitude(), iTimeSinceAppStart);
    		m_YReception.AddData((float)location.getLatitude(), iTimeSinceAppStart);
    	}
    	
    	m_dLastSpeed = location.getSpeed();
    	
    	if(m_pLastLocation != null && location != null)
    	{
    		// track the incoming GPS rate
    		long lGap = location.getTime() - m_pLastLocation.getTime();
    		rgLastGPSGaps[iLastGPSGap] = lGap;
    		iLastGPSGap++;
    		iLastGPSGap = iLastGPSGap % rgLastGPSGaps.length;
    		
    		// determine the acceleration
    		if(m_pLastLocation.hasBearing() && location.hasBearing() && lGap > 0)
    		{
    			final float flGapSeconds = ((float)lGap) / 1000.0f;
    			final Vector2D vLastSpeed = Vector2D.FromBearing(m_pLastLocation.getBearing(),m_pLastLocation.getSpeed());
    			final Vector2D vThisSpeed = Vector2D.FromBearing(location.getBearing(), location.getSpeed());
    			
    			// finds the delta-v
    			final Vector2D vChangeSpeed = vThisSpeed.Minus(vLastSpeed);
    			
    			// finds the delta-v in terms of acceleration
    			final Vector2D vAcceleration = vChangeSpeed.Multiply(1/flGapSeconds);
    			
    			// finds the acceleration relative to the current heading of the car
    			final Vector2D vRelativeAcceleration = vAcceleration.Rotate(-location.getBearing());
    		}
    	}
    	
    	m_pLastLocation = location;
    	
        // Called when a new location is found by the network location provider.
    	if(m_eState == State.WAITING_FOR_GPS)
    	{
    		// if we get a location while waiting for GPS, then we're good to advance to setting start/finish lines
			SetState(State.WAITINGFORSTART);
    	}
    	else if(m_eState == State.WAITINGFORSTART || m_eState == State.WAITINGFORSTOP)
    	{
    		if(m_myLaps == null)
    		{
    			m_myLaps = new LapAccumulator(m_lapParams, m_ptCurrent, iUnixTime, -1, (int)location.getTime(), location.getSpeed());
    		}
	    	m_myLaps.AddPosition(m_ptCurrent, (int)location.getTime(), location.getSpeed());
	    	if(!m_startDecider.IsReady() && m_startDecider.NotifyPoint(m_myLaps, m_ptLast, m_ptCurrent, location.getSpeed()))
	    	{
	    		SetState(State.WAITINGFORSTOP);
	    		// start recording this lap
	    		m_lapParams.lnStart = m_startDecider.GetLine();
	    		m_lapParams.vStart = m_startDecider.GetDir();
	    		if(m_stopDecider.IsReady())
	    		{
	    			m_lapParams.lnStop = m_stopDecider.GetLine();
	    			m_lapParams.vStop = m_stopDecider.GetDir();
		    		TrackLastLap(m_myLaps, true, false);
	    			SetState(State.MOVING_TO_STARTLINE);
	    		}
	    		m_myLaps = new LapAccumulator(m_lapParams, m_ptCurrent, iUnixTime, -1, (int)location.getTime(), location.getSpeed());
	    	}
	    	else if(!m_stopDecider.IsReady() && m_stopDecider != m_startDecider)
	    	{
	    		if(m_stopDecider.NotifyPoint(m_myLaps, m_ptLast, m_ptCurrent, location.getSpeed()))
	    		{
		    		m_lapParams.lnStop = m_stopDecider.GetLine();
		    		m_lapParams.vStop = m_stopDecider.GetDir();
		    		TrackLastLap(m_myLaps, true, false);
	    			SetState(State.MOVING_TO_STARTLINE);
	    		}
	    	}
	    	else if(m_startDecider.IsReady() && m_stopDecider.IsReady())
	    	{
	    		SetState(State.MOVING_TO_STARTLINE);
	    	}
	    	m_currentView.invalidate();
    	}
    	else if(m_eState == State.MOVING_TO_STARTLINE)
    	{
    		if(m_myLaps == null)
    		{
    			m_myLaps = new LapAccumulator(m_lapParams, m_ptCurrent, iUnixTime, -1, (int)location.getTime(), location.getSpeed());
    		}
	    	m_myLaps.AddPosition(m_ptCurrent, (int)location.getTime(), location.getSpeed());
	    	if(this.m_startDecider.NotifyPoint(m_myLaps, m_ptLast, m_ptCurrent, location.getSpeed()))
	    	{
	    		m_myLaps.Prune();
	    		m_myLaps = new LapAccumulator(m_lapParams, m_ptCurrent, iUnixTime, -1, (int)location.getTime(), location.getSpeed());
	    		SetState(State.PLOTTING);
	    	}
    	}
    	else if(m_eState == State.PLOTTING)
    	{
    		if(m_myLaps == null)
    		{
    			m_myLaps = new LapAccumulator(m_lapParams, m_ptCurrent, iUnixTime, -1, (int)location.getTime(), location.getSpeed());
    		}
	    	m_myLaps.AddPosition(m_ptCurrent, (int)location.getTime(), location.getSpeed());
	    	if(m_myLaps.IsDoneLap())
	    	{
	    		TrackLastLap(m_myLaps, true, true);

	    		if(!m_fUseP2P) // if we're lapping, just start the next lap now
	    		{
	    			m_myLaps = new LapAccumulator(m_lapParams, m_myLaps.GetFinishPoint(), iUnixTime, -1, m_myLaps.GetLastCrossTime(), location.getSpeed());
	    			m_myLaps.AddPosition(m_ptCurrent, (int)location.getTime(), location.getSpeed());
	    		}
	    		else // if we're point-to-point, then start worrying about getting back to the start line
	    		{
	    			SetState(State.MOVING_TO_STARTLINE);
	    		}
	    	}
	    	m_currentView.invalidate();
    	}
    }
    
    public double GetGPSRate()
    {
    	// returns the rate in hz of the last (rgLastGPSGaps.length) GPS signals
    	long lSumGap = 0;
    	for(int x = 0; x < rgLastGPSGaps.length; x++)
    	{
    		lSumGap += rgLastGPSGaps[x];
    	}
    	double dAvgGap = ((double)lSumGap / (double)rgLastGPSGaps.length) / 1000.0;
    	return 1 / dAvgGap;
    }
    
    @Override
    public void onStatusChanged(String provider, int status, Bundle extras) 
    {
    	boolean fStillWaiting = true;
    	if(provider == LocationManager.GPS_PROVIDER && status == LocationProvider.AVAILABLE && m_eState == State.WAITING_FOR_GPS)
    	{
    		SetState(LocationManager.class,STATE.ON,"Signal Acquired");
    		SetState(State.WAITINGFORSTART);
    	}
    	else if(provider == LocationManager.GPS_PROVIDER && status == LocationProvider.OUT_OF_SERVICE || status == LocationProvider.TEMPORARILY_UNAVAILABLE)
    	{
    		if(status == LocationProvider.OUT_OF_SERVICE)
    		{
    			SetState(LocationManager.class, STATE.TROUBLE_BAD, "No GPS Device.  Is your bluetooth device on?");
    		}
    		SetState(State.WAITING_FOR_GPS);
    	}
    	else if(provider.equals(BluetoothGPS.NO_VTG_FOUND))
    	{
    		Toast.makeText(this, "Your GPS device isn't transmitting velocity information.  Try the qstarz configuration info on www.wifilapper.com.", Toast.LENGTH_SHORT).show();
    	}
    	
    	if(fStillWaiting && status != LocationProvider.OUT_OF_SERVICE)
    	{
    		if(extras != null && extras.containsKey("satellites"))
    		{
    			int cSatellites = extras.getInt("satellites");
    			SetState(LocationManager.class,STATE.TROUBLE_GOOD,"Waiting for GPS (" + cSatellites + " satellites");
    		}
    		else
    		{
    			SetState(LocationManager.class,STATE.TROUBLE_GOOD,"Waiting for GPS");
    		}
    	}
    }
    @Override
    public void onProviderEnabled(String provider) 
    {
    }
    @Override
    public void onProviderDisabled(String provider) 
    {
    }
    
    // state stuff.
    // this will instantiate and set the proper view
    LapAccumulator GetCurrentLap() {return m_myLaps;}
    LapAccumulator GetLastLap() {return m_lastLap;}
    LapAccumulator GetDriverBestLap() {return m_driverBest;}
    LapAccumulator GetBestLap() {return m_best;}
    
	public float GetTimeSinceLastSplit()
	{
		return ((float)(System.currentTimeMillis() - m_tmLastLap)) / 1000.0f;
	}
	public int GetLapSentCount()
	{
		if(m_lapSender != null)
		{
			return m_lapSender.GetLapSentCount();
		}
		else
		{
			return 0;
		}
	}
    void SetState(State eNewState)
    {
    	// first, let's do a state filter
    	if(eNewState == State.WAITINGFORSTART || eNewState == State.WAITINGFORSTOP)
    	{
    		if(m_startDecider.IsReady() && m_stopDecider.IsReady())
    		{
    			eNewState = State.MOVING_TO_STARTLINE;
    		}
    		else if(m_startDecider.IsReady() && !m_stopDecider.IsReady())
    		{
    			eNewState = State.WAITINGFORSTOP;
    		}
    		else if(!m_startDecider.IsReady())
    		{
    			eNewState = State.WAITINGFORSTART;
    		}
    	}
    	
    	m_currentView = null;
    	
    	switch(eNewState)
    	{
    	case LOADING:
    	{
    		View vLoading = View.inflate(this, R.layout.lapping_loading, null);
    		m_currentView = vLoading;
    		setContentView(vLoading);
    		vLoading.requestLayout();
    		break;
    	}
    	case WAITING_FOR_GPS:
    	{
    		View vGPS = View.inflate(this, R.layout.lapping_gpsview, null);
    		GPSWaitView vActualView = (GPSWaitView)vGPS.findViewById(R.id.gpsview);
    		m_currentView = vActualView;
    		setContentView(vGPS);
    		vGPS.requestLayout();
    		break;
    	}
    	case MOVING_TO_STARTLINE:
    	{
    		View vStartline = View.inflate(this, R.layout.lapping_movetostartline, null);
    		MoveToStartLineView vView = (MoveToStartLineView)vStartline.findViewById(R.id.movetostartline);
    		vView.DoInit(this);
    		m_currentView = vView;
    		m_currentView.setOnClickListener(this);
    		setContentView(vStartline);
    		vView.requestLayout();
    		m_myLaps = null;
    		break;
    	}
    	case DEMOSCREEN:
    	{
    		View vDemo = View.inflate(this, R.layout.lapping_demoscreen, null);
    		Button btn = (Button)vDemo.findViewById(R.id.btnMarket);
    		if(btn != null)
    		{
    			btn.setOnClickListener(this);
    		}
    		m_currentView = vDemo;
    		setContentView(vDemo);
    		vDemo.requestLayout();
    		if(m_lapSender != null) m_lapSender.Shutdown();
    		if(m_btgps != null) m_btgps.Shutdown();
    		if(m_obd != null) {m_obd.Shutdown();}
    		
    		if(m_thdFakeLocs != null) m_thdFakeLocs.Shutdown();

    		LocationManager locMan = (LocationManager)getApplicationContext().getSystemService(Context.LOCATION_SERVICE);
    		if(locMan != null) locMan.removeUpdates(this);
    		m_myLaps = null;
			SetState(LocationManager.class, Utility.MultiStateObject.STATE.OFF, "GPS shut down.");
			
			
    		break;
    	}
    	case WAITINGFORSTART:
    	case WAITINGFORSTOP:
    	{
    		// we're waiting for the start and stop deciders to be satisfied and ready to go
    		View vLineDraw = View.inflate(this, R.layout.lapping_startfinishautodraw, null);
    		DeciderWaitingView vActualView = (DeciderWaitingView)vLineDraw.findViewById(R.id.linesetter);
    		vActualView.SetData(this);
    		m_currentView = vLineDraw;
    		m_currentView.setOnClickListener(this);
    		m_currentView = vLineDraw;
    		setContentView(vLineDraw);
    		vLineDraw.requestLayout();
    		break;
    	}
    	case PLOTTING:
    		assert(this.m_lapParams != null && m_lapParams.IsValid(this.m_fUseP2P));
    		
    		View vView = View.inflate(this, R.layout.lapping_laptimeview, null);
    		MapPaintView view = (MapPaintView)vView.findViewById(R.id.laptimeview);
    		view.SetData(this, m_strSpeedoStyle, m_eUnitSystem);
    		m_currentView = vView;
    		m_currentView.setOnClickListener(this);
    		setContentView(vView);
    		vView.requestLayout();
    		break;
    	}
    	if(m_statusBar != null) m_statusBar.DeInit();
    	
    	m_statusBar = (StatusBarView)findViewById(R.id.statusbar);
    	m_statusBar.SetStateData(this);
    	
    	m_eState = eNewState;
    	assert(m_currentView != null);
    	m_currentView.invalidate();
    }
    
	@Override
	public boolean handleMessage(Message msg) 
	{
		switch(msg.what)
		{
		case MSG_STATE_CHANGED:
			m_currentView.invalidate();
			m_statusBar.Refresh();
			return true;
		case MSG_FAKE_LOCATION:
			Location l = (Location)msg.obj;
			onLocationChanged(l);
			return true;
		case MSG_LOADING_PROGRESS:
			if(this.m_eState == State.LOADING)
			{
				if(msg.arg1 >= msg.arg2)
				{
					SetState(State.WAITING_FOR_GPS);
				}
				else
				{
					ProgressBar prog = (ProgressBar)m_currentView.findViewById(R.id.prgLoading);
					prog.setMax(msg.arg2);
					prog.setProgress(msg.arg1);
				}
			}
			return true;
		case MSG_IOIO_BUTTON:
			// the user pressed a digital input on their IOIO that we are going to treat as a click.  So call the click code.
			onClick(m_currentView);
			return true;
		}
		return false;
	}
	
	private interface SplitDecider
	{
		public abstract boolean NotifyPoint(LapAccumulator currentLap, Point2D ptLast, Point2D ptCurrent, float flCurrentVel);
		public abstract boolean IsReady();
		public abstract String[] GetUnReadyStrings(); // returns the string we display when this split decider isn't ready (example: "tap to set start line")
		public abstract void Tap(); // user tapped the screen.  absorb this input
		public abstract LineSeg GetLine();
		public abstract Vector2D GetDir();
	}
	private abstract static class BaseSplitDecider implements SplitDecider
	{
		protected BaseSplitDecider()
		{
		}
		@Override
		public boolean NotifyPoint(LapAccumulator currentLap, Point2D ptLast, Point2D ptCurrent, float flCurrentVel)
		{
			// handles the case that all NotifyPoints have to handle: did this point cross the line and direction we have set?
			this.currentLap = currentLap;
			
			if(IsReady() && ptCurrent != null && ptLast != null)
			{
				LineSeg lnPoints = new LineSeg(ptCurrent,ptLast);
				LineSeg.IntersectData data = new LineSeg.IntersectData();
				
				Vector2D vNewDir = Vector2D.P1MinusP2(ptCurrent, ptLast);
				if(lnPoints.Intersect(ln, data, true, true) && vNewDir.DotProduct(vCrossDir) > 0)
				{
					return true;
				}
			}
			return false;
		}
		public abstract String[] GetUnReadyStrings(); // returns the string we display when this split decider isn't ready (example: "tap to set start line")
		public void Tap() {}; // user tapped the screen.  absorb this input

		@Override
		public final boolean IsReady()	{return ln != null && vCrossDir != null && vCrossDir.GetLength() > 0 && ln.GetLength() > 0;	}
		@Override
		public final LineSeg GetLine() {return ln;}
		@Override
		public final Vector2D GetDir() {return vCrossDir;}

		protected LapAccumulator currentLap;
		protected LineSeg ln;
		protected Vector2D vCrossDir;
		
		private int iLapsToGo;
	}
	private static class SpeedSplitDecider extends BaseSplitDecider
	{
		SpeedSplitDecider(boolean fArmedByDefault, boolean fOnSlowDown, float flSpeed, Prefs.UNIT_SYSTEM eUnitSystem, String strStartFinish)
		{
			super();
			this.fArmed = fArmedByDefault;
			this.fOnSlowDown = fOnSlowDown;
			this.flCrossSpeed = flSpeed;
			this.eSystem = eUnitSystem;
			this.strStartFinish = strStartFinish;
		}
		@Override
		public boolean NotifyPoint(LapAccumulator currentLap, Point2D ptLast, Point2D ptCurrent, float flCurrentVel)
		{
			if(super.NotifyPoint(currentLap, ptLast, ptCurrent, flCurrentVel)) return true;
			
			if(!IsReady() && fArmed && flLastSpeed >= 0)
			{
				float flLastDiff = flLastSpeed - flCrossSpeed;
				float flThisDiff = flCurrentVel - flCrossSpeed;
				
				final boolean fCross = flLastDiff * flThisDiff <= 0; // did we cross the setpoint?
				final boolean fSlowDownCross = fCross && flThisDiff < 0; // did we cross the setpoint while decelerating?
				final boolean fSpeedUpCross = fCross && flThisDiff > 0; // did we cross the setpoint while accelerating?
				if(fSpeedUpCross && !fOnSlowDown || fSlowDownCross && fOnSlowDown)
				{
					// this means we have cross the targeted speed, and crossed it in the direction this SplitDecider has been looking for
					if(currentLap.GetPositionCount() > 3)
					{
						vCrossDir = new Vector2D(0,0); // allocate a vector2D so that MakeSplitAtIndex can edit it for us
						ln = currentLap.MakeSplitAtIndex(currentLap.GetPositionCount()-1, vCrossDir);
						return true;
					}
				}
				
			}
			flLastSpeed = flCurrentVel;
			return false;
		}
		@Override
		public String[] GetUnReadyStrings()
		{
			String str[] = new String[2];
			if(fArmed)
			{
				str[0] = strStartFinish + " will be set when speed";
				str[1] = "crosses " + Prefs.FormatMetersPerSecond(flCrossSpeed, null, eSystem, true) + " (current: " + Prefs.FormatMetersPerSecond(flLastSpeed, null, eSystem, false) + ")";
			}
			else
			{
				str[0] = "Tap screen to arm speed";
				str[1] = "";
			}
			return str;
		}
		public void Tap()
		{
			fArmed = true;
		}
		
		private boolean fArmed; // we aren't ready to go until we're armed
		private String strStartFinish;
		Prefs.UNIT_SYSTEM eSystem;
		private float flCrossSpeed;
		private float flLastSpeed = -1;
		private boolean fOnSlowDown; // whether we're looking for the vehicle slowing down (true) or speeding up (false)
	}

	private static class DistanceSplitDecider extends BaseSplitDecider
	{
		DistanceSplitDecider(float flNeededDistance, Prefs.UNIT_SYSTEM eUnitSystem)
		{
			super();
			
			this.flNeededDistance = flNeededDistance;
			this.eSystem = eUnitSystem;
		}
		@Override
		public boolean NotifyPoint(LapAccumulator currentLap, Point2D ptLast, Point2D ptCurrent, float flCurrentVel)
		{
			if(super.NotifyPoint(currentLap, ptLast, ptCurrent, flCurrentVel)) return true;
			
			if(!IsReady())
			{
				if(currentLap.GetDistanceMeters() > flNeededDistance)
				{
					// this means that we have just changed sign in our difference-from-target-speed.
					// therefore, we just crossed it.
					if(currentLap.GetPositionCount() > 3)
					{
						vCrossDir = new Vector2D(0,0); // allocate a vector2D so that MakeSplitAtIndex can edit it for us
						ln = currentLap.MakeSplitAtIndex(currentLap.GetPositionCount()-1, vCrossDir);
						return true;
					}
				}
				
			}
			return false;
		}
		@Override
		public String[] GetUnReadyStrings()
		{
			String str[] = new String[2];
			str[0] = "Finish will be set when distance";
			str[1] = "crosses " + Prefs.FormatDistance(flNeededDistance, null, eSystem, true) + " (current: " + Prefs.FormatDistance(currentLap.GetDistanceMeters(), null, eSystem, true) + ")";
			return str;
		}
		
		Prefs.UNIT_SYSTEM eSystem;
		private float flNeededDistance; // how far do we need to go (in meters)?
		private float flGoneDistance; // how far have we gone? (in meters)
	}
	private static class LineSplitDecider extends BaseSplitDecider
	{
		LineSplitDecider(String strSetupLine)
		{
			super();
			this.strSetupLine = strSetupLine;
		}
		LineSplitDecider(LineSeg ln, Vector2D vDir)
		{
			super();
			this.ln = ln;
			this.vCrossDir = vDir;
		}
		@Override
		public boolean NotifyPoint(LapAccumulator currentLap, Point2D ptLast, Point2D ptCurrent, float flCurrentVel)
		{
			if(super.NotifyPoint(currentLap,ptLast,ptCurrent, flCurrentVel)) return true;
			
			LineSeg.IntersectData intersect = new LineSeg.IntersectData();
			LapAccumulator.CrossData cross = new LapAccumulator.CrossData();
			if(!IsReady() && currentLap.IsNowCrossingSelf(intersect,cross) && currentLap.GetFinishesNeeded() == 1)
			{
				if(cross.flSpeedOfCrossedLine > 5)
				{
					// the car crossed a line where it was going fast, while the car was going fast.  therefore, we should set start/finish lines
					// we need to fish out pairs of points so we can do proper start/finish setting
					vCrossDir = new Vector2D(0,0);
					ln = currentLap.MakeSplitAtIndex(cross.ixCrossedLine,vCrossDir);
					return true;
				}
			}
			return false;
		}
		@Override
		public String[] GetUnReadyStrings()
		{
			String str[] = new String[2];
			
			str[0] = strSetupLine;
			str[1] = "Tap screen to force start/finish and splits";
			
			return str;
		}
		
		@Override
		public void Tap()
		{
			vCrossDir = new Vector2D(0,0); // allocate a vector2D so that MakeSplitAtIndex can edit it for us
			ln = currentLap.MakeSplitAtIndex(currentLap.GetPositionCount()-1, vCrossDir);
		}

		private String strSetupLine;
	}
	
	private Location m_pLastLocation;
	public Location GetLastPosition()
	{ 
		return m_pLastLocation;
	}
	
	public static class FakeLocationGenerator extends Thread implements Runnable
	{
		private Handler m_listener;
		private int m_hz;
		private float m_goalSpeed;
		private long m_lLastGoalSpeedUpdate = 0;
		private boolean m_fShutdown = false;
		public FakeLocationGenerator(Handler h, int hz)
		{
			m_listener = h;
			m_hz = hz;
			m_goalSpeed = 20.0f;
			start();
		}
		public void Shutdown()
		{
			m_fShutdown = true;
		}
		
		@Override
		public void run()
		{
			Thread.currentThread().setName("Fake location generator");
			
			long lTimeToReport = System.currentTimeMillis(); // gets incremented by iTimeToSleep + iJiggle
			long lPositionTime = System.currentTimeMillis(); // always gets incremented by just iTimeToSleep
			double dLastX = 0;
			double dLastY = 0;
			int iJiggle = 0;
			while(!m_fShutdown)
			{
				int iTimeToSleep = 1000 / m_hz;
				try
				{
					Thread.sleep(iTimeToSleep);
					Location loc = new Location("fake");
					lTimeToReport += iTimeToSleep + iJiggle;
					lPositionTime += iTimeToSleep;
					
					if(lTimeToReport > m_lLastGoalSpeedUpdate + 20000)
					{
						iJiggle = (int)(Math.random() * (iTimeToSleep/2));
						m_lLastGoalSpeedUpdate = lTimeToReport;
					}
					double dTimeInSec = lPositionTime / 1000.0;
					double dAngle = (dTimeInSec * 2 * Math.PI) / m_goalSpeed;
					double dX = Math.sin(dAngle + (Math.PI/2)) * 0.0003;
					double dY = Math.cos(dAngle + (Math.PI/2)) * 0.0003;
					
					loc.setTime(System.currentTimeMillis());
					
					loc.setLatitude(dX);
					loc.setLongitude(dY);
					loc.setAltitude(0);
					
					double dChangeX = dX - dLastX;
					double dChangeY = dY - dLastY;
					double dChange = Math.sqrt(dChangeX*dChangeX + dChangeY*dChangeY);
					loc.setSpeed((float)Math.sqrt(System.currentTimeMillis() % 9000));
					float flBearing = ((float)Math.atan2(dChangeX, dChangeY))*180.0f/3.14159f;
					flBearing = (float)(((int)flBearing)%360);
					loc.setBearing(flBearing);
					
					Message m = Message.obtain(m_listener, ApiDemos.MSG_FAKE_LOCATION, loc);
					m_listener.sendMessage(m);
					dLastX = dX;
					dLastY = dY;
					
				}
				catch(InterruptedException e)
				{
					
				}
			}
		}
	}

	@Override
	public void SetMessage(int iTime, String strMsg) 
	{
		m_strMessage = strMsg;
		m_iMsgTime = iTime;
	}
	public String GetMessage()
	{
		int iCurTime = (int)(System.currentTimeMillis() / 1000);
		if(m_iMsgTime > iCurTime)
		{
			// if the message time is in the future...
			return m_strMessage;
		}
		return null;
	}
	@Override
	public void NotifyOBDParameter(int pid, float value) 
	{
		if(m_myLaps == null) return;
		
		int iTimeSinceAppStart = (int)((System.nanoTime()/1000000) - this.m_tmAppStartUptime);
		
		boolean fNeededNew = false;
		DataChannel chan = m_mapPIDS.get(new Integer(pid));
		if(chan == null || !chan.IsParent(m_myLaps))
		{
			fNeededNew = true;
			chan = new DataChannel(DataChannel.CHANNEL_PID_START + pid,m_myLaps);
		}
		chan.AddData((float)value, iTimeSinceAppStart);
		if(fNeededNew)
		{
			m_mapPIDS.put(new Integer(pid), chan);
		}
	}
	@Override
	public void NotifyIOIOValue(int pin, int iCustomType, float flValue) 
	{
		if(m_myLaps == null) return;
		
		int iTimeSinceAppStart = (int)((System.nanoTime()/1000000) - this.m_tmAppStartUptime);
		
		boolean fNeededNew = false;
		DataChannel chan = this.m_mapPins.get(new Integer(pin));
		if(chan == null || !chan.IsParent(m_myLaps))
		{
			fNeededNew = true;
			
			final int iDCType = iCustomType == 0 ? (DataChannel.CHANNEL_IOIO_START + pin) : iCustomType;
			chan = new DataChannel(iDCType,m_myLaps);
		}
		chan.AddData((float)flValue, iTimeSinceAppStart);
		if(fNeededNew)
		{
			m_mapPins.put(new Integer(pin), chan);
		}
	}
	
	
	@SuppressWarnings("rawtypes")
	Map<Class,StateData> m_mapStateData;
	
	@SuppressWarnings("rawtypes")
	@Override
	public synchronized void SetState(Class c, STATE eState, String strDesc) 
	{
		if(m_mapStateData == null) m_mapStateData = new HashMap<Class,StateData>();
		m_mapStateData.put(c, new StateData(eState,strDesc));
		m_pHandler.sendEmptyMessage(MSG_STATE_CHANGED);
	}
	@SuppressWarnings("rawtypes")
	@Override
	public synchronized StateData GetState(Class c) 
	{
		if(m_mapStateData == null) return new StateData(STATE.OFF,null);
		StateData stateFromMap = m_mapStateData.get(c);
		if(stateFromMap != null) return stateFromMap;
		return new StateData(STATE.OFF,null);
	}
	@Override
	public void NotifyIOIOButton() 
	{
		this.m_pHandler.sendEmptyMessage(MSG_IOIO_BUTTON);
	}
	@Override
	public void SetConnectionLevel(CONNLEVEL eLevel) 
	{
		this.m_fRecordReception = (eLevel == CONNLEVEL.CONNECTED) || (eLevel == CONNLEVEL.FULLYCONNECTED);
	}
}

class GPSWaitView extends View
{
	Paint paintSettings;
	
	public GPSWaitView(Context context)
	{
		super(context);
		DoInit();
	}
	public GPSWaitView(Context context, AttributeSet attrs)
	{
		super(context,attrs);
		DoInit();
	}
	public GPSWaitView(Context context, AttributeSet attrs, int defStyle)
	{
		super(context,attrs,defStyle);
		DoInit();
	}
	private void DoInit()
	{
		paintSettings = new Paint();
		paintSettings.setARGB(255,255,255,255);
	}
	public void onDraw(Canvas canvas)
	{
		canvas.setMatrix(new Matrix());
		canvas.clipRect(getLeft(),getTop(),getRight(),getBottom(),Op.REPLACE);
		//canvas.scale(1.5f,1.5f);
		
		final Rect rc = new Rect(getLeft()+10,getTop()+10,getRight()-10,getBottom()-10);
		final String str = "Waiting for GPS";
		Utility.DrawFontInBox(canvas, str, paintSettings, rc);
	}
}

class MoveToStartLineView extends View
{
	Paint paintLines;
	Paint paintTrack;
	Paint paintSmallText;
	ApiDemos myApp;
	NumberFormat num;
	
	public MoveToStartLineView(Context context)
	{
		super(context);
	}
	public MoveToStartLineView(Context context, AttributeSet attrs)
	{
		super(context,attrs);
	}
	public MoveToStartLineView(Context context, AttributeSet attrs, int defStyle)
	{
		super(context,attrs,defStyle);
	}
	public void DoInit(ApiDemos app)
	{

		num = NumberFormat.getInstance();
		myApp = app;
		paintSmallText = new Paint();
		paintSmallText.setARGB(255,255,255,255);
		paintSmallText.setTextSize(20);
		
		paintTrack = new Paint();
		paintTrack.setARGB(255,50,50,50);
		
		paintLines = new Paint();
		paintLines.setARGB(255,255,0,0);
	}
	public void onDraw(Canvas canvas)
	{
		canvas.setMatrix(new Matrix());
		//canvas.scale(1.5f,1.5f);
		canvas.clipRect(getLeft(),getTop(),getRight(),getBottom(),Op.REPLACE);
		Rect rcAll = new Rect();
		rcAll.set(getLeft(),getTop(),getRight(), getBottom());

		String strMsg = myApp.GetMessage();
		if(myApp.GetTimeSinceLastSplit() < 2.0)
		{
			// draw the last split
			num.setMaximumFractionDigits(2);
			num.setMinimumFractionDigits(2);
			
			paintSmallText.setARGB(255,255,255,255); // just use white if they're going for absolute times
			
			final int cxLabels = getWidth()/10;
			final int cxSplit = getRight() - cxLabels;
			
			Rect rcLapTime = new Rect(getLeft(),getTop(),cxSplit,getBottom());
			Rect rcLapLabel = new Rect(cxSplit, getTop(),getRight(),getBottom());
			// curent lap has no splits, so we must have just finished a lap
			LapAccumulator lapLast = myApp.GetLastLap();
			if(lapLast != null)
			{
				final double dLastLap = lapLast.GetLapTime();
				String strLapText = Utility.FormatSeconds((float)dLastLap);
				Utility.DrawFontInBox(canvas, strLapText, paintSmallText, rcLapTime);
				Utility.DrawFontInBox(canvas, "Lap", paintSmallText, rcLapLabel);
			}
		}
		else if(strMsg != null)
		{
			Paint p = new Paint();
			Utility.DrawFontInBox(canvas, strMsg, paintSmallText, rcAll);
		}
		else
		{
			String str = "Proceed to start/finish line";
			
			LapAccumulator lap = myApp.GetCurrentLap();
			if(lap != null)
			{
				FloatRect rcInWorld = lap.GetBounds(false);
				Rect rcOnScreen = new Rect();
				rcOnScreen.left = getLeft();
				rcOnScreen.top = getTop();
				rcOnScreen.right = getRight();
				rcOnScreen.bottom = getBottom();
				
				List<LineSeg> lstSF = lap.GetSplitPoints(true);
				if(lstSF != null && lstSF.size() > 0)
				{
					FloatRect rcSF = lstSF.get(0).GetBounds();
					rcInWorld = rcInWorld.Union(rcSF);
					float flSFX = rcSF.ExactCenterX();
					float flSFY = rcSF.ExactCenterY();
					
					float rg[] = new float[2];
					Location.distanceBetween(flSFY, flSFX, lap.GetLastPoint().pt.y, lap.GetLastPoint().pt.x, rg);
					
					str += " (" + (int)rg[0] + " meters away)";
				}
				
				LapAccumulator.DrawLap(lap, false, rcInWorld, canvas, paintTrack, paintLines, new Rect(rcOnScreen));
			}
			Utility.DrawFontInBox(canvas, str, paintSmallText, rcAll);
		}
	}
}

//this is the view that sets the start/finish lines
class DeciderWaitingView extends View
{
	Paint paintText;
	Paint paintLines;
	Paint paintTrack;
	Paint paintSmallText;
	Paint paintAttention;
	ApiDemos myApp;
	public DeciderWaitingView(Context context)
	{
		super(context);
		DoInit();
	}
	public DeciderWaitingView(Context context, AttributeSet attrs)
	{
		super(context,attrs);
		DoInit();
	}
	public DeciderWaitingView(Context context, AttributeSet attrs, int defStyle)
	{
		super(context,attrs,defStyle);
		DoInit();
	}
	void SetData(ApiDemos api)
	{
		myApp = api;
	}
	private void DoInit()
	{
		paintText = new Paint();
		paintText.setARGB(255,255,255,255);
		paintText.setTextSize(40);

		paintSmallText = new Paint();
		paintSmallText.setARGB(255,255,255,255);
		paintSmallText.setTextSize(20);
		
		paintLines = new Paint();
		paintLines.setARGB(255,255,0,0);
		
		paintTrack = new Paint();
		paintTrack.setARGB(255,255,255,255);
		
		paintAttention = new Paint();
		paintAttention.setARGB(255,255,0,255);
	}
	public void onDraw(Canvas canvas)
	{
		canvas.setMatrix(new Matrix());
		//canvas.scale(1.5f,1.5f);
		canvas.clipRect(getLeft(),getTop(),getRight(),getBottom(),Op.REPLACE);
		
		LapAccumulator lap = myApp.GetCurrentLap();
		if(lap != null)
		{
			if(myApp.IsReadyForLineSet())
			{
				FloatRect rcInWorld = lap.GetBounds(false);
				Rect rcOnScreen = new Rect();
				rcOnScreen.left = getLeft();
				rcOnScreen.top = getTop();
				rcOnScreen.right = getRight();
				rcOnScreen.bottom = getBottom();
				
				LapAccumulator.DrawLap(lap, false, rcInWorld, canvas, paintTrack, paintLines, new Rect(rcOnScreen));
				
				String str[] = myApp.GetDeciderWaitingStrings();
				
				final int mid = getTop() + getHeight()/2;
				
				// Flash the screen for 0.5 second every 2, to alert driver to set start/finish line
				if( System.currentTimeMillis()%2048 < 512)
					canvas.drawPaint(paintAttention);
				
				Utility.DrawFontInBox(canvas, str[0], paintSmallText, new Rect(getLeft(),getTop(),getRight(),mid));
			}
			else
			{
				final String str1 = "You must be moving to set";
				final String str2 = "start/finish and split points";
				final int mid = getTop() + getHeight()/2;
				Utility.DrawFontInBox(canvas, str1, paintSmallText, new Rect(getLeft(),getTop(),getRight(),mid));
				Utility.DrawFontInBox(canvas, str2, paintSmallText, new Rect(getLeft(),mid,getRight(),getBottom()));
			}
		}
		//canvas.drawText("Wifi: " + pStateMan.GetState(), 50.0f, 110.0f, paintText);
		//canvas.drawText("GPS: " + (int)myApp.GetGPSRate() + "hz", 50.0f, 130.0f, paintText);
	}
	
}
class MapPaintView extends View
{
	Paint paintText;
	Paint paintBigText;
	ApiDemos myApp;
	String strSpeedoStyle;
	NumberFormat num;
	Prefs.UNIT_SYSTEM eDisplayUnitSystem;
	
	int cPaintCounts = 0;
	public MapPaintView(Context context)
	{
		super(context);
		DoInit();
	}
	public MapPaintView(Context context, AttributeSet attrs)
	{
		super(context,attrs);
		DoInit();
	}
	public MapPaintView(Context context, AttributeSet attrs, int defStyle)
	{
		super(context,attrs,defStyle);
		DoInit();
	}
	public void SetData(ApiDemos api, String strSpeedoStyle, Prefs.UNIT_SYSTEM eUnitSystem)
	{
		this.myApp = api;
		this.strSpeedoStyle = strSpeedoStyle;
		this.eDisplayUnitSystem = eUnitSystem;
	}
	private void DoInit()
	{
		num = NumberFormat.getInstance();
		paintText = new Paint();
		paintText.setARGB(255,255,255,255);
		
		paintBigText = new Paint();
		paintBigText.setTextSize(150);
		paintBigText.setARGB(255, 255, 255, 255);
	}
	private void DrawSpeedDistance(Canvas canvas, Rect rcOnScreen, LapAccumulator lap, LapAccumulator lapBest)
	{
		Paint paintLap = new Paint();
		Paint paintSplits = new Paint();
		
		if(lap != null)
		{
			FloatRect rcWindow = lapBest != null ? lapBest.GetBounds(true) : lap.GetBounds(true);
			if(lap != null) rcWindow = rcWindow.Union(lap.GetBounds(true));
			if(lapBest != null && lap != null)
			{
				final float flTotalDistance = lapBest.GetDistance();
				final float flCurDistance = lap.GetDistance();
				
				// have the window show the previous 20% of the lap and the upcoming 5%
				rcWindow.left = flCurDistance - flTotalDistance*0.20f;
				rcWindow.right = flCurDistance + flTotalDistance*0.05f;
			}
			
			if(lapBest != null)
			{
				// draw the best lap in purple
				paintLap.setARGB(255, 255, 0, 255);
				paintLap.setStrokeWidth(6);
				paintSplits.setARGB(255, 255, 0, 0);
				LapAccumulator.DrawLap(lapBest, true, rcWindow, canvas, paintLap, paintSplits, new Rect(rcOnScreen));
			}
			if(lap != null)
			{
				// draw the current lap in a brighter color
				paintLap.setARGB(255, 255, 255, 255); // current lap in white if we're doing speed-distance
				paintLap.setStrokeWidth(6);
				paintSplits.setARGB(255, 255, 0, 0);
				LapAccumulator.DrawLap(lap, true, rcWindow, canvas, paintLap, paintSplits, new Rect(rcOnScreen));
			}
		}
	}
	private void DrawPlusMinus(Canvas canvas, Rect rcOnScreen, LapAccumulator lap, LapAccumulator lapBest)
	{
		if(lapBest == null)
		{
			DrawSpeedDistance(canvas,rcOnScreen,lap,lapBest);
		}
		else
		{
			final TimePoint2D ptCurrent = lap.GetLastPoint();
			double flPercentage = ptCurrent.dDistance / lapBest.GetDistance();
			final double flThisTime = ((double)lap.GetAgeInMilliseconds())/1000.0;
			final double flBestTime = (double)(lapBest.GetTimeAtPosition(ptCurrent,flPercentage)/1000.0);
			num.setMaximumFractionDigits(1);
			num.setMinimumFractionDigits(1);
			
			// according to a phone call at 1:51pm on Sunday Jan 29, if you're ahead, it should be minus
			// it took us "flThisTime" seconds to get to the current distance
			// on the best lap, it took us "flLastTime"
			float flToPrint = (float)(flThisTime - flBestTime);
			String strPrefix;
			Paint p = new Paint();
			if(flToPrint < 0)
			{
				// the current lap is ahead...
				paintBigText.setARGB(255, 128, 255, 128);
				p.setARGB(255, 128, 255, 128);
				
				strPrefix = "";
			}
			else
			{
				paintBigText.setARGB(255, 255, 128, 128);
				p.setARGB(255, 255, 128, 128);
				strPrefix = "+";
			}
			String strText = num.format(flToPrint);
			
			String strToPaint = strPrefix + strText;
			Utility.DrawFontInBox(canvas, strToPaint, p, rcOnScreen);
		}
	}
	private void DrawComparative(Canvas canvas, Rect rcOnScreen, LapAccumulator lap, LapAccumulator lapBest)
	{
		// this displays the driver's current speed, as well as their speed at that point for the best lap
		num.setMaximumFractionDigits(1);
		num.setMinimumFractionDigits(1);
		
		Rect rcTop = new Rect();
		Rect rcBottom = new Rect();
		final int cyTarget = rcOnScreen.bottom - rcOnScreen.top;
		rcTop.set(rcOnScreen.left,rcOnScreen.top,rcOnScreen.right,rcOnScreen.top + cyTarget/2);
		rcBottom.set(rcOnScreen.left, rcTop.bottom, rcOnScreen.right, rcOnScreen.bottom);
		if(lap != null)
		{
			Paint p = new Paint();
			p.setARGB(255, 255, 255, 255);
			
			final TimePoint2D ptCurrent = lap.GetLastPoint();
			
			final float flSpeed = (float)ptCurrent.dVelocity;
			String strSpeed = Prefs.FormatMetersPerSecond(flSpeed,num,eDisplayUnitSystem,false);
			Utility.DrawFontInBox(canvas, strSpeed, p, rcTop);
		}
		if(lapBest != null)
		{
			Paint p = new Paint();
			p.setARGB(255, 255, 0, 255);
			final TimePoint2D ptCurrent = lap.GetLastPoint();
			final float flBestSpeed = (float)lapBest.GetSpeedAtPosition(ptCurrent);
			String strSpeed = Prefs.FormatMetersPerSecond(flBestSpeed,num,eDisplayUnitSystem,false);
			Utility.DrawFontInBox(canvas, strSpeed, p, rcBottom);
		}
	}
	private void DrawLapTimer(Canvas canvas, Rect rcOnScreen, LapAccumulator lap, LapAccumulator lapBest)
	{
		
		// todo: move the paint declaration to the constructor, or reuse existing
		Paint p = new Paint();
		final Rect rcTimeDiff = new Rect();
		final Rect rcLapSeconds = new Rect();
		final Rect rcLapTenths = new Rect();
		final Rect rcBestSeconds = new Rect();
		final Rect rcBestTenths = new Rect();
		final float myFontSize;
		
		if( rcOnScreen.width() < rcOnScreen.height() )
		{
			// Portrait mode
			final int midSplit  = rcOnScreen.centerY();
			final int lowSplit  = rcOnScreen.top  + rcOnScreen.height() * 3/4;
			final int rightSplit= rcOnScreen.left + rcOnScreen.width() * 4/5;

			rcTimeDiff.set(rcOnScreen.left, rcOnScreen.top, rcOnScreen.right, midSplit);
			rcLapSeconds.set(rcOnScreen.left, midSplit, rightSplit, lowSplit);
			rcLapTenths.set(rightSplit, midSplit, rcOnScreen.right, lowSplit);
			rcBestSeconds.set(rcOnScreen.left, lowSplit, rightSplit, rcOnScreen.bottom);
			rcBestTenths.set(rightSplit, lowSplit, rcOnScreen.right, rcOnScreen.bottom);
			myFontSize = (float)0.9*Utility.GetNeededFontSize("0:00.0 ", p, rcLapSeconds);
			}
		else
		{
			// Landscape mode
			final int midXSplit  = rcOnScreen.centerX();
			final int midXSplit2 = (int)(midXSplit * 1.1);
			final int midYSplit  = rcOnScreen.centerY();
			final int labelSplit = rcOnScreen.height()/10;

			rcTimeDiff.set(rcOnScreen.left, rcOnScreen.top, midXSplit, rcOnScreen.bottom);
			rcLapTenths.set(midXSplit2, rcOnScreen.top, rcOnScreen.right, rcOnScreen.top+labelSplit);
			rcLapSeconds.set(midXSplit2, rcOnScreen.top+labelSplit, rcOnScreen.right, midYSplit);
			rcBestTenths.set(midXSplit2, midYSplit, rcOnScreen.right, midYSplit+labelSplit);
			rcBestSeconds.set(midXSplit2, midYSplit+labelSplit, rcOnScreen.right, rcOnScreen.bottom);
			myFontSize = (float)0.9*Utility.GetNeededFontSize("0:00.0 ", p, rcLapSeconds);
			
		}
		
		LapAccumulator lapLast = myApp.GetLastLap();

		final double dLastLap;
		final String strLast;
		final double dBestLap;
		final String strBest;

		if(lapLast != null && lapBest != null)
		{
			dLastLap = lapLast.GetLapTime();
			DrawPlusMinus(canvas, rcTimeDiff, lap, lapBest);
			dBestLap = lapBest.GetLapTime();
			strBest = buildLapTime(dBestLap);
			
			if( dLastLap > dBestLap )
				p.setARGB(255,255,128,128); // last lap worse, make red
			else
				p.setARGB(255,128,255,128); // last lap better/equal, make green

			strLast = buildLapTime(dLastLap);
			Utility.DrawFontInBoxFinal(canvas, strLast, myFontSize, p, rcLapSeconds, true, false);
			Utility.DrawFontInBox(canvas, "Last", p, rcLapTenths);
		}
		else
		{
			final double flThisTime = ((double)lap.GetAgeInMilliseconds())/1000.0;
			String strLapTime = buildLapTime(flThisTime);
			strBest = "-:--.-";

			p.setARGB(255,255,255,255); // reset to white
			Utility.DrawFontInBoxFinal(canvas, strLapTime, myFontSize, p, rcLapSeconds, true,false);
			Utility.DrawFontInBox(canvas, "Lap", p, rcLapTenths);
		}
		
		p.setARGB(255,255,255,255); // Best lap in white
		Utility.DrawFontInBoxFinal(canvas, strBest, myFontSize, p, rcBestSeconds, true, false);
		Utility.DrawFontInBox(canvas, "Best", p, rcBestTenths);
	
	}
	
	public String buildLapTime( double flLapTime)
	{
		final int minutes = (int)(flLapTime/60);
		final int seconds = (int)(flLapTime - minutes*60);
		final int tenths = (int)((10* (flLapTime - minutes*60 - seconds)));
		String strLapTime = minutes + ":";
		if (seconds <10 )
			strLapTime = strLapTime + "0" + seconds + "." + tenths;
		else
			strLapTime = strLapTime + seconds + "." + tenths;
		return strLapTime;
	}
	public void onDraw(Canvas canvas)
	{
		canvas.setMatrix(new Matrix());
		//canvas.scale(1.5f,1.5f);
		canvas.clipRect(getLeft(),getTop(),getRight(),getBottom(),Op.REPLACE);
		
		String strMsg = myApp.GetMessage();
		Rect rcAll = new Rect();
		rcAll.set(getLeft(),getTop(),getRight(), getBottom());
		
		if(myApp.GetTimeSinceLastSplit() < 2.0)
		{
			// draw the last split
			num.setMaximumFractionDigits(2);
			num.setMinimumFractionDigits(2);
			
			paintBigText.setARGB(255,255,255,255); // just use white if they're going for absolute times
			
			final int cxLabels = getWidth()/10;
			final int cxSplit = getRight() - cxLabels;
			
			Rect rcLapTime = new Rect(getLeft(),getTop(),cxSplit,getBottom());
			Rect rcLapLabel = new Rect(cxSplit, getTop(),getRight(),getBottom());
			// curent lap has no splits, so we must have just finished a lap
			LapAccumulator lapLast = myApp.GetLastLap();
			if(lapLast != null)
			{
				final double dLastLap = lapLast.GetLapTime();
				String strLapText = Utility.FormatSeconds((float)dLastLap);
				Utility.DrawFontInBox(canvas, strLapText, paintBigText, rcLapTime);
				Utility.DrawFontInBox(canvas, "Lap", paintBigText, rcLapLabel);
			}
		}
		else if(strMsg != null)
		{
			Paint p = new Paint();
			Utility.DrawFontInBox(canvas, strMsg, paintBigText, rcAll);
		}
		else
		{
			final float cxSecondaryPct = 0.25f;
			final int cxSecondaryPixels = (int)(getWidth() * cxSecondaryPct);
			Rect rcMain = new Rect();
			rcMain.set(getLeft(), getTop(), getRight()-cxSecondaryPixels, getBottom());
			Rect rcSecondary = new Rect();
			rcSecondary.set(rcMain.right,getTop(),getRight(),getBottom());

			LapAccumulator lap = myApp.GetCurrentLap();
			LapAccumulator lapBest = myApp.GetBestLap();
			if(strSpeedoStyle.equals(LandingOptions.SPEEDO_SPEEDDISTANCE))
			{
				DrawSpeedDistance(canvas, rcMain, lap, lapBest);
				if(lapBest != null)
				{
					DrawPlusMinus(canvas, rcSecondary, lap, lapBest);
				}
			}
			else if(strSpeedoStyle.equals(LandingOptions.SPEEDO_COMPARATIVE))
			{
				DrawComparative(canvas, rcMain, lap, lapBest);
			}
			else if(strSpeedoStyle.equals(LandingOptions.SPEEDO_LIVEPLUSMINUS))
			{
				DrawPlusMinus(canvas, rcMain, lap, lapBest);
			}
			else if(strSpeedoStyle.equals(LandingOptions.SPEEDO_SIMPLE))
			{
				DrawComparative(canvas, rcMain, lap, null);
			}
		}
	}
}
