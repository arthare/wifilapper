package com.artsoft.wifilapper;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.telephony.SmsManager;
import android.telephony.SmsMessage;

public class SMSReceiver extends BroadcastReceiver 
{
	@Override
	public void onReceive(Context ctx, Intent intent)
	{
		//---get the SMS message passed in---
        Bundle bundle = intent.getExtras();        
        SmsMessage[] msgs = null;     
        if (bundle != null)
        {
            //---retrieve the SMS message received---
            Object[] pdus = (Object[]) bundle.get("pdus");
            msgs = new SmsMessage[pdus.length];            
            for (int i=0; i<msgs.length; i++)
            {
                msgs[i] = SmsMessage.createFromPdu((byte[])pdus[i]);                
                String strBlah = msgs[i].getMessageBody();
            	ApiDemos pApp = ApiDemos.Get();
            	if(pApp != null)
            	{
            		pApp.NotifyNewSMS(strBlah, msgs[i].getOriginatingAddress());
            	}
            }
        }                         
	}
	
	// strAcknowledgeString = the string we'll send.  Example: "Ack 'ram red green...'"
	public static void SMSAcknowledgeMessage(String strAcknowledgeString, String strPhoneNumber)
	{
		if(strAcknowledgeString == null || strAcknowledgeString.length() <= 0)
		{
			return;
		}
		SmsManager sms = SmsManager.getDefault();
        sms.sendTextMessage(strPhoneNumber, null, strAcknowledgeString, null, null); 
	}
}
