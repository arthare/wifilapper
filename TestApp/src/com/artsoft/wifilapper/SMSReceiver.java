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
