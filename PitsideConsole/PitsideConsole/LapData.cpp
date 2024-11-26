#include "stdafx.h"
#include "LapData.h"
#include "PitsideConsole.h"
#include "dlgPlotSelect.h"


struct PIDDATA
{
  LPCTSTR pListDesc;
  const char* pDataFormat; // format string for when we display the value
};

PIDDATA g_rgPIDData[] = 
{
  {L"PIDs supported [01 - 20]",""},
	{L"Monitor status since DTCs cleared. (Includes malfunction indicator lamp (MIL) status and number of DTCs.)",""},
	{L"Freeze DTC",""},
	{L"Fuel sys status",""},
	{L"Calc eng load%%","%3.1f"},
	{L"Eng coolant temp,C","%3.1f"},
	{L"ST fuel % trim—Bank 1%%","%4.1f"},
	{L"LT fuel % trim—Bank 1%%","%4.1f"},
	{L"ST fuel % trim—Bank 2%%","%4.1f"},
	{L"LT fuel % trim—Bank 2%%","%4.1f"},
	{L"Fuel press, kPa","%3.1f"},
	{L"Int mani abs press,kPa","%4.1f"},
	{L"Engine RPM","%5.0f"},
	{L"Vehicle speed,kph","%3.1f"},
	{L"Timing advance,d","%2.2f"},
	{L"Int air temp,C","%3.1f"},
	{L"MAF air flow rate,g/s","%3.2f"},
	{L"Throttle pos%%","%3.1f"},
	{L"Comm sec air status",""},
	{L"O2 sensors present",""},
	{L"Bank 1, Sens 1: O2 sens volt, Short term fuel trim",""},
	{L"Bank 1, Sens 2: O2 sens volt, Short term fuel trim",""},
	{L"Bank 1, Sens 3: O2 sens volt, Short term fuel trim",""},
	{L"Bank 1, Sens 4: O2 sens volt, Short term fuel trim",""},
	{L"Bank 2, Sens 1: O2 sens volt, Short term fuel trim",""},
	{L"Bank 2, Sens 2: O2 sens volt, Short term fuel trim",""},
	{L"Bank 2, Sens 3: O2 sens volt, Short term fuel trim",""},
	{L"Bank 2, Sens 4: O2 sens volt, Short term fuel trim",""},
	{L"OBD standards this vehicle conforms to",""},
	{L"O2 sens present",""},
	{L"Aux input status",""},
	{L"Run time since engine start,s","%5.0f"},
  {L"PIDs supported [21 - 40]",""},
	{L"Dist traveled w/MIL on,km","%5.1f"},
	{L"Fuel Rail Press (rel mani vacuum)","%5.1f"},
	{L"Fuel Rail Press (direct inject)kPa","%6.1f"},
	{L"O2S1_WR_lambda(1): Equivalence Ratio Voltage",""},
	{L"O2S2_WR_lambda(1): Equivalence Ratio Voltage",""},
	{L"O2S3_WR_lambda(1): Equivalence Ratio Voltage",""},
	{L"O2S4_WR_lambda(1): Equivalence Ratio Voltage",""},
	{L"O2S5_WR_lambda(1): Equivalence Ratio Voltage",""},
	{L"O2S6_WR_lambda(1): Equivalence Ratio Voltage",""},
	{L"O2S7_WR_lambda(1): Equivalence Ratio Voltage",""},
	{L"O2S8_WR_lambda(1): Equivalence Ratio Voltage",""},
	{L"Commanded EGR%%","%3.1f"},
	{L"EGR Error%%","%3.1f"},
	{L"Comm evap purge%%","%3.1f"},
	{L"Fuel Level Input%%","%3.1f"},
	{L"# of warm-ups since codes cleared","%3.0f"},
	{L"Dist traveled since codes cleared","%5.1f"},
	{L"Evap Sys Vapor Press,Pa","%4.1f"},
	{L"Barometric press,kPa","%3.1f"},
	{L"O2S1_WR_lambda(1): Equivalence Ratio Current",""},
	{L"O2S2_WR_lambda(1): Equivalence Ratio Current",""},
	{L"O2S3_WR_lambda(1): Equivalence Ratio Current",""},
	{L"O2S4_WR_lambda(1): Equivalence Ratio Current",""},
	{L"O2S5_WR_lambda(1): Equivalence Ratio Current",""},
	{L"O2S6_WR_lambda(1): Equivalence Ratio Current",""},
	{L"O2S7_WR_lambda(1): Equivalence Ratio Current",""},
	{L"O2S8_WR_lambda(1): Equivalence Ratio Current",""},
	{L"Cat Temp Bank 1, Sens 1",""},
	{L"Cat Temp Bank 2, Sens 1",""},
	{L"Cat Temp Bank 1, Sens 2",""},
	{L"Cat Temp Bank 2, Sens 2",""},
  {L"PIDs supported [41 - 60]",""},
	{L"Mon status this drive cycle",""},
	{L"Ctrl module voltage",""},
	{L"Abs load value",""},
	{L"Command equiv ratio",""},
	{L"Rel thrott pos",""},
	{L"Amb air temp",""},
	{L"Abs throt pos B",""},
	{L"Abs throt pos C",""},
	{L"Accel ped pos D",""},
	{L"Accel ped pos E",""},
	{L"Accel ped pos F",""},
	{L"Commanded throttle actuator",""},
	{L"Time run with MIL on",""},
	{L"Time since trouble codes cleared",""},
	{L"Max val equiv rat, O2 sens volt/current, mani press",""},
	{L"Max val air flow rate MAF sens",""},
	{L"Fuel Type",""},
	{L"Ethanol fuel %",""},
	{L"Abs Evap sys Vap Press",""},
	{L"Evap sys vap press",""},
	{L"Short term secondary oxygen sensor trim bank 1 and bank 3",""},
	{L"Long term secondary oxygen sensor trim bank 1 and bank 3",""},
	{L"Short term secondary oxygen sensor trim bank 2 and bank 4",""},
	{L"Long term secondary oxygen sensor trim bank 2 and bank 4",""},
	{L"Fuel rail press (abs)",""},
	{L"Rel accel pedal pos",""},
	{L"Hybrid batt pack rem life",""},
	{L"Eng oil temp",""},
	{L"Fuel inject timing",""},
	{L"Eng fuel rate",""},
	{L"Emission reqts",""},
  {L"PIDs supported [61 - 80]",""},
	{L"Drvr dmnd eng pct torque",""},
	{L"Act eng pct torque",""},
	{L"Eng ref torque",""},
	{L"Eng pct torque data",""},
	{L"Aux input/output supp",""},
	{L"Mass air flow sensor",""},
	{L"Eng coolant temp",""},
	{L"Int air temp sensor",""},
	{L"Comm EGR and EGR Error",""},
	{L"Diesel int air flow ctrl",""},
	{L"Exh gas recirc temp",""},
	{L"Throt act ctrl/rel thrott pos",""},
	{L"Fuel press ctrl system",""},
	{L"Inj press ctrl system",""},
	{L"Turbo comp inlet press",""},
	{L"Boost pressure control",""},
	{L"Var Geom turbo control",""},
	{L"Wastegate control",""},
	{L"Exhaust pressure",""},
	{L"Turbocharger RPM",""},
	{L"Turbocharger temp",""},
	{L"Turbocharger temp",""},
	{L"CACT temperature",""},
	{L"Exh Gas temp Bank 1",""},
	{L"Exh Gas temp Bank 2",""},
	{L"Diesel part filter DPF",""},
	{L"Diesel part filter DPF",""},
	{L"Diesel Part filter temp",""},
	{L"NOx NTE control status",""},
	{L"PM NTE control status",""},
	{L"Engine run time",""},
  {L"PIDs supported [81 - A0]",""},
	{L"Eng run time AECD",""},
	{L"Eng run time AECD",""},
	{L"NOx sensor",""},
	{L"Mani surface temp",""},
	{L"NOx reagent system",""},
	{L"Part matter sensor",""},
	{L"Intake man abs press",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"PIDs supported [A1 - C0]",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
	{L"",""},
  {L"PIDs supported [C1 - E0]",""},
};

PIDDATA g_rgIOIOCustomData[] = {
  {L"Fuel Level%%","%3.2f"},	//	0x301
  {L"RPM","%5.1f"},				//	0x302
  {L"Throttle Pos","%3.2f"},	//	0x303
  {L"Brake Pos","%3.2f"},		//	0x304
  {L"Clutch Pos","%3.2f"},		//	0x305
  {L"LF Wheelspeed","%3.2f"},	//	0x306
  {L"RF Wheelspeed","%3.2f"},	//	0x307
  {L"LR Wheelspeed","%3.2f"},	//	0x308
  {L"RR Wheelspeed","%3.2f"},	//	0x309
  {L"Exhaust Temp","%3.1f"},	//	0x30A
  {L"LF Tire Temp","%3.1f"},	//	0x30B
  {L"RF Tire Tempe","%3.1f"},	//	0x30C
  {L"LR Tire Tempe","%3.1f"},	//	0x30D
  {L"RR Tire Tempe","%3.1f"},	//	0x30E
  {L"LF Brake Temp","%3.1f"},	//	0x30F
  {L"RF Brake Temp","%3.1f"},	//	0x310
  {L"LR Brake Temp","%3.1f"},	//	0x311
  {L"RR Brake Temp","%3.1f"},	//	0x312
  {L"Oil Pressure","%3.2f"},	//	0x313
  {L"Oil Temp","%3.1f"},		//	0x314
  {L"Coolant Temp","%3.1f"},	//	0x315
  {L"Alt Voltage","%3.2f"},		//	0x316
  {L"Air Temp (F)","%3.2f"},	//	0x317
  {L"MAF (V)","%3.2f"},			//	0x318
  {L"O2 Sensor (V)","%3.2f"},	//	0x319
};

PIDDATA g_rgRaceDACData[] = {
  {L"RaceDAC A1","%3.2f"},		//	0x401
  {L"RaceDAC A2","%3.2f"},		//	0x402
  {L"RaceDAC A3","%3.2f"},		//	0x403
  {L"RaceDAC A4","%3.2f"},		//	0x404
  {L"RaceDAC A5","%3.2f"},		//	0x405
  {L"RaceDAC A6","%3.2f"},		//	0x406
  {L"RaceDAC A7","%3.2f"},		//	0x407
  {L"RaceDAC A8","%3.2f"},		//	0x408
  {L"RaceDAC D1","%4.1f"},		//	0x409
  {L"RaceDAC D2","%4.1f"},		//	0x40A
};

void GetDataChannelName(DATA_CHANNEL eDC, LPTSTR lpszName, int cch)
{
  LPCTSTR lpszDataName = NULL;
  switch(eDC)
  {
  case DATA_CHANNEL_X: lpszDataName = L"Longitude"; break;
  case DATA_CHANNEL_Y: lpszDataName = L"Latitude"; break;
  case DATA_CHANNEL_DISTANCE: lpszDataName = L"Distance"; break;
  case DATA_CHANNEL_TIME: lpszDataName = L"Time"; break;
  case DATA_CHANNEL_ELAPSEDTIME: lpszDataName = L"Elapsed Time"; break;
  case DATA_CHANNEL_LAPTIME_SUMMARY: lpszDataName = L"Lap Time"; break;
  case DATA_CHANNEL_VELOCITY: lpszDataName = L"Velocity"; break;
  case DATA_CHANNEL_VELOCITYDELTA: lpszDataName = L"Vel. Delta"; break;
  case DATA_CHANNEL_TIMESLIP: lpszDataName = L"Time-slip"; break;
  case DATA_CHANNEL_X_ACCEL: lpszDataName = L"X accel"; break;
  case DATA_CHANNEL_Y_ACCEL: lpszDataName = L"Y accel"; break;
  case DATA_CHANNEL_Z_ACCEL: lpszDataName = L"Z accel"; break;
  case DATA_CHANNEL_TEMP: lpszDataName = L"Temperature"; break;
  case DATA_CHANNEL_RECEPTION_X: lpszDataName = L"Wifi Dots X"; break;
  case DATA_CHANNEL_RECEPTION_Y: lpszDataName = L"Wifi Dots Y"; break;
  case DATA_CHANNEL_HRM: lpszDataName = L"Heart Rate"; break;
  case DATA_CHANNEL_PING: lpszDataName = L"Ping Time"; break;
  case DATA_CHANNEL_STRENGTH: lpszDataName = L"Conn. Level"; break;
  default:
    if(eDC >= DATA_CHANNEL_IOIOPIN_START && eDC <= DATA_CHANNEL_IOIOPIN_END)
    {
      _snwprintf(lpszName, cch, L"IOIO Pin %d",(eDC - DATA_CHANNEL_IOIOPIN_START));
      return;
    }
    else if(eDC >= DATA_CHANNEL_IOIOCUSTOM_START && eDC <= DATA_CHANNEL_IOIOCUSTOM_END)
    {
      const int custom = eDC - DATA_CHANNEL_IOIOCUSTOM_START;
      if(custom >= 0 && custom < NUMITEMS(g_rgIOIOCustomData))
      {
        lpszDataName = ::g_rgIOIOCustomData[custom].pListDesc;
      }
      else
      {
        lpszDataName = L"Ukwn IOIO pin";
      }
    }
    else if(eDC >= DATA_CHANNEL_RACEDAC_START && eDC <= DATA_CHANNEL_RACEDAC_END)
    {
      lpszDataName = ::g_rgRaceDACData[eDC - DATA_CHANNEL_RACEDAC_START].pListDesc;
	}
    else if(eDC >= DATA_CHANNEL_RACEDACCUSTOM_START && eDC <= DATA_CHANNEL_RACEDACCUSTOM_END)
    {
      const int custom = eDC - DATA_CHANNEL_RACEDACCUSTOM_START;
      if(custom >= 0 && custom < NUMITEMS(g_rgIOIOCustomData))
      {
        lpszDataName = ::g_rgIOIOCustomData[custom].pListDesc;
      }
      else
      {
        lpszDataName = L"Ukwn RaceDAC pin";
      }
	}
    else
    {
      DASSERT(eDC >= DATA_CHANNEL_PID_START && eDC < DATA_CHANNEL_PID_END);
      const int pid = eDC - DATA_CHANNEL_PID_START;
      lpszDataName = g_rgPIDData[pid].pListDesc;
    }
    break;
  }
  if(!lpszDataName)
  {
    DASSERT(FALSE);
    lpszDataName = L"Unknown";
  }
  if(lpszDataName)
  {
    // done!
    _snwprintf(lpszName, cch, lpszDataName);
    return;
  }
}

float ConvertSpeed(UNIT_PREFERENCE eConvertTo, float flValueInMetersPerSecond)
{
  
  switch(eConvertTo)
  {
  case UNIT_PREFERENCE_KMH: return flValueInMetersPerSecond / KMH_TO_MS(1);
  case UNIT_PREFERENCE_MPH: return flValueInMetersPerSecond / MPH_TO_MS(1);
  case UNIT_PREFERENCE_MS: return flValueInMetersPerSecond;
  }
  CASSERT(UNIT_PREFERENCE_COUNT == 3);

  return flValueInMetersPerSecond;
}
LPCSTR GetUnitText(UNIT_PREFERENCE eUnits)
{
  switch(eUnits)
  {
  case UNIT_PREFERENCE_KMH: return "km/h";
  case UNIT_PREFERENCE_MPH: return "mph";
  case UNIT_PREFERENCE_MS: return "m/s";
  }
  CASSERT(UNIT_PREFERENCE_COUNT == 3);
  return "";
}

void GetChannelString(DATA_CHANNEL eX, UNIT_PREFERENCE eUnits, float flValue, LPSTR lpsz, int cch)
{
  CASSERT(DATA_CHANNEL_COUNT == 0x601);

  switch(eX)
  {
    case DATA_CHANNEL_X:
    {
      char cEW = flValue < 0 ? 'W' : 'E';
      sprintf(lpsz, "%4.4f%c", abs(flValue), cEW);
      break;
    }
    case DATA_CHANNEL_Y:
    {
      char cNS = flValue < 0 ? 'N' : 'E';
      sprintf(lpsz, "%4.4f%c", abs(flValue), cNS);
      break;
    }
    case DATA_CHANNEL_DISTANCE:
    {
      sprintf(lpsz, "%4.1fkm", flValue/1000.0f);
      break;
    }
    case DATA_CHANNEL_LAPTIME_SUMMARY:
    {
      sprintf(lpsz, "%4.1f", flValue);
      break;
    }
	case DATA_CHANNEL_HRM:
    {
      sprintf(lpsz, "%4.1fBPM", flValue);
      break;
    }
	case DATA_CHANNEL_PING:
    {
      sprintf(lpsz, "%4.0fms", flValue);
      break;
    }
	case DATA_CHANNEL_STRENGTH:
    {
      sprintf(lpsz, "%4.1f", flValue);
      break;
    }
    case DATA_CHANNEL_TIME:
    {
      sprintf(lpsz, "%4.1fs", flValue/1000.0f);
      break;
    }
    case DATA_CHANNEL_ELAPSEDTIME:
    {
      sprintf(lpsz, "%4.2fs", flValue/1000.0f);
      break;
    }

	case DATA_CHANNEL_VELOCITY:
	case DATA_CHANNEL_VELOCITYDELTA:
    {
      LPCSTR lpszUnits = GetUnitText(eUnits);
      // note: velocity is in m/s, but most humans will like km/h (well... except for americans, but screw them for now)
      sprintf(lpsz, "%4.2f %s", ConvertSpeed(eUnits,flValue), lpszUnits);
      break;
    }
    case DATA_CHANNEL_TIMESLIP:
    {
      LPCSTR lpszAheadBehind = (flValue) > 0 ? "ahead" : "behind"; // the flip here is because we also do it for the GetChannel() function
      sprintf(lpsz, "%4.2fs %s", abs(flValue/1000.0), lpszAheadBehind);
      break;
    }
    case DATA_CHANNEL_X_ACCEL:
    {
      sprintf(lpsz, "%2.4fg", flValue);
      break;
    }
    case DATA_CHANNEL_Y_ACCEL:
    {
      sprintf(lpsz, "%2.4fg", flValue);
      break;
    }
    case DATA_CHANNEL_Z_ACCEL:
    {
      sprintf(lpsz, "%2.4fg", flValue);
      break;
    }
    case DATA_CHANNEL_TEMP:
    {
      sprintf(lpsz, "%4.2fc", flValue);
      break;
    }
    case DATA_CHANNEL_RECEPTION_X:
    case DATA_CHANNEL_RECEPTION_Y:
    {
      sprintf(lpsz,"%3.3f",flValue);
      break;
    }
    default:
    {
      if(eX >= DATA_CHANNEL_PID_START && eX < DATA_CHANNEL_PID_END)
      {
        const int pid = eX - DATA_CHANNEL_PID_START;
        sprintf(lpsz, g_rgPIDData[pid].pDataFormat, flValue);
      }
      else if(eX >= DATA_CHANNEL_IOIOPIN_START && eX <= DATA_CHANNEL_IOIOPIN_END)
      {
        const int pin = eX - DATA_CHANNEL_IOIOPIN_START;
        sprintf(lpsz, "%2.3fV", flValue);
      }
      else if(eX >= DATA_CHANNEL_IOIOCUSTOM_START && eX <= DATA_CHANNEL_IOIOCUSTOM_END)
      {
        const int custom = eX - DATA_CHANNEL_IOIOCUSTOM_START;
        sprintf(lpsz,g_rgIOIOCustomData[custom].pDataFormat, flValue);
      }
      else if(eX >= DATA_CHANNEL_RACEDAC_START && eX <= DATA_CHANNEL_RACEDAC_END)
      {
        const int pin = eX - DATA_CHANNEL_RACEDAC_START;
        sprintf(lpsz, "%4.1fV", flValue);
      }
      else if(eX >= DATA_CHANNEL_RACEDACCUSTOM_START && eX <= DATA_CHANNEL_RACEDACCUSTOM_END)
      {
        const int custom = eX - DATA_CHANNEL_RACEDACCUSTOM_START;
        sprintf(lpsz,g_rgIOIOCustomData[custom].pDataFormat, flValue);
      }
      else
      {
        sprintf(lpsz,"%4.1f",flValue);
      }
      break;
    }
  }
}

void GetChannelValue(DATA_CHANNEL eX, UNIT_PREFERENCE eUnits, float flValue, LPSTR lpsz, int cch)
{
  CASSERT(DATA_CHANNEL_COUNT == 0x601);

  switch(eX)
  {
    case DATA_CHANNEL_X:
    {
      sprintf(lpsz, "%1.4f", abs(flValue));
      break;
    }
    case DATA_CHANNEL_Y:
    {
      sprintf(lpsz, "%1.4f", abs(flValue));
      break;
    }
    case DATA_CHANNEL_DISTANCE:
    {
      sprintf(lpsz, "%3.2f", flValue/1000.0f);
      break;
    }
    case DATA_CHANNEL_LAPTIME_SUMMARY:
	case DATA_CHANNEL_HRM:
	case DATA_CHANNEL_PING:
	case DATA_CHANNEL_STRENGTH:
    {
      sprintf(lpsz, "%4.1f", flValue);
      break;
    }
    case DATA_CHANNEL_TIME:
    {
      sprintf(lpsz, "%4.1f", flValue/1000.0f);
      break;
    }
    case DATA_CHANNEL_ELAPSEDTIME:
    {
      sprintf(lpsz, "%4.2f", flValue/1000.0f);
      break;
    }
	case DATA_CHANNEL_VELOCITY:
	case DATA_CHANNEL_VELOCITYDELTA:
    {
      // note: velocity is in m/s, but most humans will like km/h (well... except for americans, but screw them for now)
      sprintf(lpsz, "%4.1f", ConvertSpeed(eUnits,flValue));
      break;
    }
    case DATA_CHANNEL_TIMESLIP:
    {
      sprintf(lpsz, "%3.2f", abs(flValue/1000.0));
      break;
    }
    case DATA_CHANNEL_X_ACCEL:
    {
      sprintf(lpsz, "%1.3f", flValue);
      break;
    }
    case DATA_CHANNEL_Y_ACCEL:
    {
      sprintf(lpsz, "%1.3f", flValue);
      break;
    }
    case DATA_CHANNEL_Z_ACCEL:
    {
      sprintf(lpsz, "%1.3f", flValue);
      break;
    }
    case DATA_CHANNEL_TEMP:
    {
      sprintf(lpsz, "%3.1f", flValue);
      break;
    }
    case DATA_CHANNEL_RECEPTION_X:
    case DATA_CHANNEL_RECEPTION_Y:
    {
      sprintf(lpsz,"%3.2f",flValue);
      break;
    }
    default:
    {
      if(eX >= DATA_CHANNEL_PID_START && eX < DATA_CHANNEL_PID_END)
      {
        const int pid = eX - DATA_CHANNEL_PID_START;
        sprintf(lpsz, g_rgPIDData[pid].pDataFormat, flValue);
      }
      else if(eX >= DATA_CHANNEL_IOIOPIN_START && eX <= DATA_CHANNEL_IOIOPIN_END)
      {
        const int pin = eX - DATA_CHANNEL_IOIOPIN_START;
        sprintf(lpsz, "%3.2f", flValue);
      }
      else if(eX >= DATA_CHANNEL_IOIOCUSTOM_START && eX <= DATA_CHANNEL_IOIOCUSTOM_END)
      {
        const int custom = eX - DATA_CHANNEL_IOIOCUSTOM_START;
        sprintf(lpsz,g_rgIOIOCustomData[custom].pDataFormat, flValue);
      }
      else if(eX >= DATA_CHANNEL_RACEDAC_START && eX <= DATA_CHANNEL_RACEDAC_END)
      {
        const int pin = eX - DATA_CHANNEL_RACEDAC_START;
        sprintf(lpsz, "%4.1f", flValue);
      }
      else if(eX >= DATA_CHANNEL_RACEDACCUSTOM_START && eX <= DATA_CHANNEL_RACEDACCUSTOM_END)
      {
        const int custom = eX - DATA_CHANNEL_RACEDACCUSTOM_START;
        sprintf(lpsz,g_rgIOIOCustomData[custom].pDataFormat, flValue);
      }
      else
      {
        sprintf(lpsz,"%3.2f",flValue);
      }
      break;
    }
  }
}


bool FindClosestTwoPoints(const TimePoint2D& p, int* pixStartIndex, double dInputPercentage, const vector<TimePoint2D>& lstPoints, TimePoint2D* pt1, TimePoint2D* pt2)
{
  if(lstPoints.size() < 2) return false;
  // dInputPercentage is the % of it's lap that p comes from.
  // example: if p is the 12th sample of a 60-sample lap, p = 0.20.
  // we use p to determine if we're mismatching with the beginning or end of a lap

  double dClosest = 1e30;
  int ixBestIndex = -1;
  
  const int cSize = lstPoints.size();
  double dNextX = lstPoints[1].flX - p.flX;
  double dNextY = lstPoints[1].flY - p.flY;

  int ixCheck = *pixStartIndex;
  if(ixCheck < 0) ixCheck = 0;
  while(true)
  {
    const int ixNext = ((ixCheck+1)%cSize);

    const double dx1 = dNextX;
    const double dy1 = dNextY;
    dNextX = lstPoints[ixNext].flX - p.flX;
    dNextY = lstPoints[ixNext].flY - p.flY;

    const double d1 = (dx1*dx1 + dy1*dy1);
    const double d2 = (dNextX*dNextX + dNextY*dNextY);
    const double dAvg = (d1+d2)/2.0;
    const double dPct = (double)ixCheck / (double)cSize;
    const double dPctDiff = abs(dPct - dInputPercentage);

    if((ixBestIndex == -1 || dAvg < dClosest) && dPctDiff < 0.25)		// If you are within 25% of reference lap in data string
    {
      dClosest = dAvg;
      ixBestIndex = ixCheck;
    }
//    if(dAvg < 5e-11 && ixBestIndex >= 0)		//	Lowered this due to 10Hz GPS triggering it too much (Shannonville)
//    {
//      break; // early-out: if we found one that is close enough, just stop here
//    }
    ixCheck = ixNext; // advance to next point

    if(ixCheck == *pixStartIndex)
    {
      break; // we've done the whole thing now
    }
  }
  *pixStartIndex = ixBestIndex;

  if(ixBestIndex >= 0)
  {
    *pt1 = lstPoints[ixBestIndex];
    *pt2 = lstPoints[(ixBestIndex+1)%cSize];
    return true;
  }
  return false;
}

void CExtendedLap::ComputeLapData(const vector<TimePoint2D>& lstPoints, CExtendedLap* pReferenceLap, const ILapReceiver* pLapDB, bool fComputeTimeSlip)
{
  if(lstPoints.size() <= 3) return;	//	Check so that following operations don't choke.
//  if(lstPoints.size() <= 2) return;	//	Check so that following operations don't choke.

  // for calculating distance and time-slip, we need the reference lap
  if(pReferenceLap != NULL)
  {
    const IDataChannel* pReferenceDistanceChannel = pReferenceLap->GetChannel(DATA_CHANNEL_DISTANCE);

    const vector<TimePoint2D>& lstReference = pReferenceLap->GetPoints();
    // here's the idea:
    // for a point P, we find the 2 closest points in lstReference (d1 to d2, making vector D)
    // we cast a vector perpindicular to D that intersects P.  That vector will intersect D.  Usually at a point from 0-100% (0 = hitting d1, 100 = hitting d2) of its length, but it's ok if it is off a bit.
    // the distance-from-start/finish for P is thus (percentage) * (d2's distance) + (100 - percentage) * (d1's distance)
    IDataChannel* pDistance = pLapDB->AllocateDataChannel();
    IDataChannel* pVelocity = pLapDB->AllocateDataChannel();
	IDataChannel* pVelocityDelta = pLapDB->AllocateDataChannel();
    IDataChannel* pX = pLapDB->AllocateDataChannel();
    IDataChannel* pY = pLapDB->AllocateDataChannel();
    IDataChannel* pTime = pLapDB->AllocateDataChannel();
    IDataChannel* pLapTime = pLapDB->AllocateDataChannel();
    IDataChannel* pLapTimeSummary = pLapDB->AllocateDataChannel();
    
	pTime->Init(GetLap()->GetLapId(), DATA_CHANNEL_TIME); 
	pLapTime->Init(GetLap()->GetLapId(), DATA_CHANNEL_ELAPSEDTIME);
	pLapTimeSummary->Init(GetLap()->GetLapId(), DATA_CHANNEL_LAPTIME_SUMMARY);
	pX->Init(GetLap()->GetLapId(), DATA_CHANNEL_X);
    pY->Init(GetLap()->GetLapId(), DATA_CHANNEL_Y);
    pDistance->Init(GetLap()->GetLapId(), DATA_CHANNEL_DISTANCE);
    pVelocity->Init(GetLap()->GetLapId(), DATA_CHANNEL_VELOCITY);
	pVelocityDelta->Init(GetLap()->GetLapId(), DATA_CHANNEL_VELOCITYDELTA);
    int iStartPoint = 0;
    for(int x = 0;x < lstPoints.size(); x++)
    {
      const TimePoint2D& p = lstPoints[x];

      pX->AddPoint(p.iTime,p.flX);
      pY->AddPoint(p.iTime,p.flY);
	  pTime->AddPoint(p.iTime, p.iTime); // this is a mapping from time to time
	  //	Let's get the last time in the lap (Lap Time) and use it for the result for each time point
	  const TimePoint2D& q = lstPoints[lstPoints.size()-1];
	  pLapTimeSummary->AddPoint(q.iTime, GetLap()->GetTime()); // This should be a mapping from time to Laptime Summaries
      TimePoint2D sfD1, sfD2;
      int iMatchedTime = 0;
      double dPct = (double)x / (double)lstPoints.size();
      if(FindClosestTwoPoints(p,&iStartPoint,dPct,lstReference,&sfD1,&sfD2))
      {
        const double dD1Distance = pReferenceDistanceChannel->GetValue(sfD1.iTime);
        const double dD2Distance = pReferenceDistanceChannel->GetValue(sfD2.iTime);
        // hooray, we found the closest two points.
        Vector2D vD = sfD2 - sfD1;
        Line<2> lnD(sfD1.V2D(),vD);
        Line<2> lnPerp(p.V2D(),vD.RotateAboutOrigin(PI/2));

        double dHitLength;
		if(lnD.IntersectLine(lnPerp, &dHitLength) && dHitLength >= -1 && dHitLength <= 2.0)	//	Widening the accept limit
        {
          // hooray, they intersect
          const double dPercent = dHitLength;
          if (abs(sfD1.iTime - sfD2.iTime) <= 3000)	
		  //	Check to make sure that points are not wrapping around, need to be within 3s of each other
		  {
			const double dThisDistance = (dD1Distance * (1-dPercent)) + (dD2Distance * dPercent);
			m_lstPoints.push_back(TimePoint2D(p));
			pDistance->AddPoint((int)p.iTime,dThisDistance);
			pVelocity->AddPoint((int)p.iTime,p.flVelocity);

			//	Let's compute the Velocity delta between the Reference and current lap
			const IDataChannel* pReferenceVelocityChannel = pReferenceLap->GetChannel(DATA_CHANNEL_VELOCITY);
			const double dD1Velocity = pReferenceVelocityChannel->GetValue(sfD1.iTime);
			const double dD2Velocity = pReferenceVelocityChannel->GetValue(sfD2.iTime);
			const double dRefVelocity = (dD1Velocity * (1-dPercent)) + (dD2Velocity * dPercent);
			pVelocityDelta->AddPoint((int)p.iTime,p.flVelocity - (float)dRefVelocity );	//	Insert the velocity delta into the vector for this channel
		  }
		}
		else
		{
          if (abs(sfD1.iTime - sfD2.iTime) <= 3000)	
		  //	Check to make sure that points are not wrapping around, need to be within 3s of each other
		  {
			const double dPercent = 0.5;
            const double dThisDistance = (dD1Distance * (1-dPercent)) + (dD2Distance * dPercent);
            m_lstPoints.push_back(TimePoint2D(p));
		    pDistance->AddPoint((int)p.iTime,dThisDistance);
            pVelocity->AddPoint((int)p.iTime,p.flVelocity);

			//	Let's compute the Velocity delta between the Reference and current lap
			const IDataChannel* pReferenceVelocityChannel = pReferenceLap->GetChannel(DATA_CHANNEL_VELOCITY);
			const double dD1Velocity = pReferenceVelocityChannel->GetValue(sfD1.iTime);
			const double dD2Velocity = pReferenceVelocityChannel->GetValue(sfD2.iTime);
			const double dRefVelocity = (dD1Velocity * (1-dPercent)) + (dD2Velocity * dPercent);
			pVelocityDelta->AddPoint((int)p.iTime,p.flVelocity - (float)dRefVelocity );	//	Insert the velocity delta into the vector for this channel
		  }
		}

      }
    }
    if(pX->IsValid())
    {
      pX->Lock();
      AddChannel(pX);
    }
    else
    {
      pLapDB->FreeDataChannel(pX);
      pX = NULL;
    }
    if(pY->IsValid())
    {
      pY->Lock();
      AddChannel(pY);
    }
    else
    {
      pLapDB->FreeDataChannel(pY);
      pY = NULL;
    }
    if(pDistance->IsValid())
    {
      pDistance->Lock();
      AddChannel(pDistance);
    }
    else
    {
      pLapDB->FreeDataChannel(pDistance);
      pDistance = NULL;
    }
    if(pTime->IsValid())
    {
      pTime->Lock();
      AddChannel(pTime);
    }
    else
    {
      pLapDB->FreeDataChannel(pTime);
      pTime = NULL;
	}
    if(pLapTimeSummary->IsValid())
    {
      pLapTimeSummary->Lock();
      AddChannel(pLapTimeSummary);
    }
    else
    {
      pLapDB->FreeDataChannel(pLapTimeSummary);
      pLapTimeSummary = NULL;
    }
    if(pLapTime && pLapTime->IsValid())
    {
		  //	Resets time to zero at the start of each lap, using offset iStartTime
		if(m_lstPoints.size() > 0)
		{
			IDataChannel* pTime = pLapDB->AllocateDataChannel();
			pTime->Init(GetLap()->GetLapId(), DATA_CHANNEL_TIME);

			const int iStartTime = m_lstPoints[0].iTime;
			for(int x = 1;x < m_lstPoints.size(); x++)
			{
			const int iElapsedTime = m_lstPoints[x].iTime - iStartTime;
			pLapTime->AddPoint(m_lstPoints[x].iTime, (double)iElapsedTime);
			}
		}
        pLapTime->Lock();
        AddChannel(pLapTime);
    }
    else
    {
    pLapDB->FreeDataChannel(pLapTime);
	pLapTime = NULL;
	}
    if(pVelocity->IsValid())
    {
      pVelocity->Lock();
      AddChannel(pVelocity);
    }
    else
    {
      pLapDB->FreeDataChannel(pVelocity);
      pVelocity = NULL;
    }
    if(pVelocityDelta->IsValid())
    {
      pVelocityDelta->Lock();
      AddChannel(pVelocityDelta);
    }
    else
    {
      pLapDB->FreeDataChannel(pVelocityDelta);
      pVelocityDelta = NULL;
    }

    if(fComputeTimeSlip && m_lstPoints.size() > 0)
    {
      IDataChannel* pTimeSlip = pLapDB->AllocateDataChannel();
      pTimeSlip->Init(GetLap()->GetLapId(), DATA_CHANNEL_TIMESLIP);

      const int iStartTime = m_lstPoints[0].iTime;
      const int iReferenceStartTime = lstReference[0].iTime;
      for(int x = 1;x < m_lstPoints.size(); x++)
      {
        const int iElapsedTime = m_lstPoints[x].iTime - iStartTime;
        const double dDistance = pDistance->GetValue(m_lstPoints[x].iTime);
        // this lap's time at {dDistance} was {iElapsedTime}.
        // we now need to estimate what the reference lap's time at {dDistance} was, and then we can get our time slip
        double dLastRefDist = 0;

        int iRefCheckStart = 1; 
		/* what index should we start at?  this gets changed each loop so that we always 
		start near the point that is most likely to be near the current m_lstPoints point */
        int ixCheck = iRefCheckStart;
        const int cReferenceSize = lstReference.size();

		while(true)
        {
          const double dRefDist = pReferenceDistanceChannel->GetValue(lstReference[ixCheck].iTime);
          if(dRefDist >= dDistance && dLastRefDist <= dDistance)
          {
            // we have found two points straddling the distance we're curious about
            const double dOffset = dDistance - dLastRefDist; // how far into the {dLastRefDist,dRefDist} x axis we are
            const double dWidth = dRefDist - dLastRefDist; // how far apart {dLastRefDist,dRefDist} are
            if(dWidth != 0)
            {
              const double dFraction = dOffset / dWidth; // the fraction that dDistance is between dLastRefDist and dRefDist
              if(dFraction >= 0.0 && dFraction <= 1.0)
              {
                const int iLastTime = lstReference[ixCheck-1].iTime;
                const int iThisTime = lstReference[ixCheck].iTime;
                const double dEstimatedElapsedTime = ((1.0-dFraction)*iLastTime + dFraction*iThisTime) - (double)iReferenceStartTime; 
				// this is the estimated time for the previous lap at this position
                if(dEstimatedElapsedTime >= 0)
                {
                  float dTimeSlip = dEstimatedElapsedTime - (double)iElapsedTime;
                  pTimeSlip->AddPoint(m_lstPoints[x].iTime, dTimeSlip);
				}
              }
            }
            break;
          }
          dLastRefDist = dRefDist;

          // update index.  Increment it, and see if we've looped around to the start yet
          ixCheck = (ixCheck+1)%cReferenceSize;
          if(ixCheck == iRefCheckStart) break; // we've done the whole loop
        }
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              }

      if(pTimeSlip && pTimeSlip->IsValid())
      {
        pTimeSlip->Lock();
        AddChannel(pTimeSlip);
      }
      else
      {
        pLapDB->FreeDataChannel(pTimeSlip);
		pTimeSlip = NULL;
      }
    }

    // now every point has a distance and a timeslip
    // let's add the rest of the data channels...
    set<DATA_CHANNEL> setChannels = pLapDB->GetAvailableChannels(GetLap()->GetLapId());
    for(set<DATA_CHANNEL>::iterator i = setChannels.begin(); i != setChannels.end(); i++)
    {
      const IDataChannel* pChannel = pLapDB->GetDataChannel(GetLap()->GetLapId(),*i);
      if(pChannel)
      {
        AddChannel(pChannel);
      }
    }
  }
  else
  {
    IDataChannel* pX = pLapDB->AllocateDataChannel();
    IDataChannel* pY = pLapDB->AllocateDataChannel();
    pX->Init(GetLap()->GetLapId(), DATA_CHANNEL_X);
    pY->Init(GetLap()->GetLapId(), DATA_CHANNEL_Y);
    // no reference lap.  That means WE'RE the reference lap!
    // it also makes distance computation stupid easy
    double dDistance = 0;
    TimePoint2D ptLast = lstPoints[0];
    m_lstPoints.push_back(TimePoint2D(ptLast));

    pX->AddPoint(ptLast.iTime,ptLast.flX);
    pY->AddPoint(ptLast.iTime,ptLast.flY);

    IDataChannel* pDistance = pLapDB->AllocateDataChannel();
    IDataChannel* pVelocity = pLapDB->AllocateDataChannel();
    IDataChannel* pVelocityDelta = pLapDB->AllocateDataChannel();
    IDataChannel* pTimeSlip = pLapDB->AllocateDataChannel();
    IDataChannel* pTime = pLapDB->AllocateDataChannel();
    IDataChannel* pLapTime = pLapDB->AllocateDataChannel();
    IDataChannel* pLapTimeSummary = pLapDB->AllocateDataChannel();
    pDistance->Init(GetLap()->GetLapId(), DATA_CHANNEL_DISTANCE);
    pTime->Init(GetLap()->GetLapId(), DATA_CHANNEL_TIME);
    pLapTime->Init(GetLap()->GetLapId(), DATA_CHANNEL_ELAPSEDTIME);
    pLapTimeSummary->Init(GetLap()->GetLapId(), DATA_CHANNEL_LAPTIME_SUMMARY);		// Preparing to add Laptime Summary channel for X-axis
    pVelocity->Init(GetLap()->GetLapId(), DATA_CHANNEL_VELOCITY);
    pVelocityDelta->Init(GetLap()->GetLapId(), DATA_CHANNEL_VELOCITYDELTA);
    pTimeSlip->Init(GetLap()->GetLapId(), DATA_CHANNEL_TIMESLIP);
    for(int x = 1;x < lstPoints.size(); x++)
    {
      const TimePoint2D& p = lstPoints[x];
      pX->AddPoint(p.iTime,p.flX);
      pY->AddPoint(p.iTime,p.flY);
	  pTime->AddPoint(p.iTime, p.iTime); // this is a mapping from time to time
     //   Let's get the last time in the lap (Lap Time) and use it for the result for each time point
     const TimePoint2D& q = lstPoints[lstPoints.size()-1];
     pLapTimeSummary->AddPoint(q.iTime, GetLap()->GetTime()); // This should be a mapping from time to Laptime Summaries
	  const int iElapsedTime = lstPoints[x].iTime - lstPoints[0].iTime;
	  pLapTime->AddPoint(p.iTime, (double)iElapsedTime);
      const double dX = p.flX - ptLast.flX;
      const double dY = p.flY - ptLast.flY;
	  double d;
	  if ( SetDistance(-1) )
	  {
	  	d = fReturnDistanceInMeters (p, ptLast);	// Return the distance in meters
	  }
	  else
	  {
	  	d = sqrt (dY*dY + dX*dX);	// Return distance in Theta of Long/Lat. Needed until Jason fixes web-side GUI
	  }
	  dDistance += d;
      m_lstPoints.push_back(TimePoint2D(p));
      ptLast = p;

      pDistance->AddPoint(p.iTime, dDistance);
      pVelocity->AddPoint(p.iTime, p.flVelocity);
      pVelocityDelta->AddPoint(p.iTime, 0);	//	No delta from reference lap, since we ARE the reference lap
      pTimeSlip->AddPoint(p.iTime, 0);
    }

    pX->Lock();
    pY->Lock();
    pDistance->Lock();
    pTime->Lock();
    pLapTimeSummary->Lock();
    pLapTime->Lock();
	pVelocity->Lock();
	pVelocityDelta->Lock();
    pTimeSlip->Lock();
    AddChannel(pDistance);
    AddChannel(pTime);
    AddChannel(pLapTime);
    AddChannel(pLapTimeSummary);
	AddChannel(pVelocity);
	AddChannel(pVelocityDelta);
    AddChannel(pTimeSlip);
    AddChannel(pX);
    AddChannel(pY);

    // let's add the rest of the data channels...
    set<DATA_CHANNEL> setChannels = pLapDB->GetAvailableChannels(GetLap()->GetLapId());
    for(set<DATA_CHANNEL>::iterator i = setChannels.begin(); i != setChannels.end(); i++)
    {
      const IDataChannel* pChannel = pLapDB->GetDataChannel(GetLap()->GetLapId(),*i);
      if(pChannel)
      {
        AddChannel(pChannel);
      }
    }

  }
}

const double fReturnDistanceInMeters (const TimePoint2D& p, TimePoint2D ptLast)
{
		  // Converting from LONG/LAT to distance in meters
		  double rad = 6371.0f;  // earth's mean radius in km 
		  double dLat, dLon, R, lat1, lat2, lon1, lon2;
		  R = rad;
		  lat1 = p.flY * 0.0174532925199433;	// Convert from degrees to radians
		  lon1 = p.flX * 0.0174532925199433;
		  lat2 = ptLast.flY * 0.0174532925199433;
		  lon2 = ptLast.flX * 0.0174532925199433;
		  dLat = (lat2 - lat1);
		  dLon = (lon2 - lon1);
		  double a = sin(dLat/2) * sin(dLat/2) + cos(lat1) * cos(lat2) * sin(dLon/2) * sin(dLon/2);
		  double c = 2 * atan2(sqrt(a), sqrt(1-a));
		  const double d = R * c * 1000;	// Return the distance in meters
		  return d;
}

const TimePoint2D GetPointAtTime(const vector<TimePoint2D>& lstPoints, int iTimeMs)
{
  int ixLow = 0;
  int ixHigh = lstPoints.size();
  while(true)
  {
    const int ixTest = (ixLow + ixHigh)/2;
    if(ixHigh - ixLow<= 1)
    {
      // we've found the closest point to the desired time
      // lstPoints[ixTest] could be earlier or later than iTimeMs though
      TimePoint2D dataFirst;
      TimePoint2D dataSecond;
      if(iTimeMs > lstPoints[ixLow].iTime)
      {
        dataFirst = lstPoints[ixLow];
        // we need to interpolate with the data point after
        if(ixLow >= lstPoints.size() - 1)
        {
          // but we're right at the end of the list, so just return this value
          dataSecond = lstPoints[ixLow];
        }
        else
        {
          dataSecond = lstPoints[ixLow+1];
        }
      }
      else
      {
        dataSecond = lstPoints[ixLow];
        if(ixLow > 0)
        {
          // we didn't find the first point, so we can use the previous point
          dataFirst = lstPoints[ixLow-1];
        }
        else
        {
          dataFirst = lstPoints[ixLow];
        }
      }
      DASSERT(dataFirst.iTime <= dataSecond.iTime);

      const int iOffset = iTimeMs - dataFirst.iTime;
      const float flWidth = (float)(dataSecond.iTime - dataFirst.iTime);

      float flWeight = 0;
      if(flWidth > 0)
      {
        flWeight = (float)iOffset / flWidth;
      }

      return TimePoint2D::Average(dataFirst,dataSecond,flWeight);
    }
    else if(lstPoints[ixTest].iTime < iTimeMs)
    {
      ixLow = ixTest;
    }
    else if(lstPoints[ixTest].iTime > iTimeMs)
    {
      ixHigh = ixTest;
    }
    else
    {
      return lstPoints[ixTest];
    }
  }
  return TimePoint2D();
}
