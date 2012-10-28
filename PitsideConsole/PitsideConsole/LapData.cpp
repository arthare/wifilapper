#include "stdafx.h"
#include "LapData.h"


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
	{L"Fuel system status",""},
	{L"Calculated engine load value","%3.0f%%"},
	{L"Engine coolant temperature","%3.0f C"},
	{L"Short term fuel % trim—Bank 1","%4.0f%%"},
	{L"Long term fuel % trim—Bank 1","%4.0f%%"},
	{L"Short term fuel % trim—Bank 2","%4.0f%%"},
	{L"Long term fuel % trim—Bank 2","%4.0f%%"},
	{L"Fuel pressure","%3.0f kPa"},
	{L"Intake manifold absolute pressure","%3.0f kPa"},
	{L"Engine RPM","%5.0f RPM"},
	{L"Vehicle speed","%3.0f km/h"},
	{L"Timing advance","%2.1f degrees"},
	{L"Intake air temperature","%3.0f C"},
	{L"MAF air flow rate","%3.2f grams/sec"},
	{L"Throttle position","%3.0f%%"},
	{L"Commanded secondary air status",""},
	{L"Oxygen sensors present",""},
	{L"Bank 1, Sensor 1: Oxygen sensor voltage, Short term fuel trim",""},
	{L"Bank 1, Sensor 2: Oxygen sensor voltage, Short term fuel trim",""},
	{L"Bank 1, Sensor 3: Oxygen sensor voltage, Short term fuel trim",""},
	{L"Bank 1, Sensor 4: Oxygen sensor voltage, Short term fuel trim",""},
	{L"Bank 2, Sensor 1: Oxygen sensor voltage, Short term fuel trim",""},
	{L"Bank 2, Sensor 2: Oxygen sensor voltage, Short term fuel trim",""},
	{L"Bank 2, Sensor 3: Oxygen sensor voltage, Short term fuel trim",""},
	{L"Bank 2, Sensor 4: Oxygen sensor voltage, Short term fuel trim",""},
	{L"OBD standards this vehicle conforms to",""},
	{L"Oxygen sensors present",""},
	{L"Auxiliary input status",""},
	{L"Run time since engine start","%5.0f seconds"},
	{L"PIDs supported [21 - 40]",""},
	{L"Distance traveled with malfunction indicator lamp (MIL) on","%5.0f km"},
	{L"Fuel Rail Pressure (relative to manifold vacuum)","%5.1f kPa"},
	{L"Fuel Rail Pressure (diesel, or gasoline direct inject)","%6.0f kPa"},
	{L"O2S1_WR_lambda(1): Equivalence Ratio Voltage",""},
	{L"O2S2_WR_lambda(1): Equivalence Ratio Voltage",""},
	{L"O2S3_WR_lambda(1): Equivalence Ratio Voltage",""},
	{L"O2S4_WR_lambda(1): Equivalence Ratio Voltage",""},
	{L"O2S5_WR_lambda(1): Equivalence Ratio Voltage",""},
	{L"O2S6_WR_lambda(1): Equivalence Ratio Voltage",""},
	{L"O2S7_WR_lambda(1): Equivalence Ratio Voltage",""},
	{L"O2S8_WR_lambda(1): Equivalence Ratio Voltage",""},
	{L"Commanded EGR","%3.0f%%"},
	{L"EGR Error","%3.0f%%"},
	{L"Commanded evaporative purge","%3.0f%%"},
	{L"Fuel Level Input","%3.0f%%"},
	{L"# of warm-ups since codes cleared","%3.0f"},
	{L"Distance traveled since codes cleared","%5.0f km"},
	{L"Evap. System Vapor Pressure","%4.0f Pa"},
	{L"Barometric pressure","%3.0f kPa"},
	{L"O2S1_WR_lambda(1): Equivalence Ratio Current",""},
	{L"O2S2_WR_lambda(1): Equivalence Ratio Current",""},
	{L"O2S3_WR_lambda(1): Equivalence Ratio Current",""},
	{L"O2S4_WR_lambda(1): Equivalence Ratio Current",""},
	{L"O2S5_WR_lambda(1): Equivalence Ratio Current",""},
	{L"O2S6_WR_lambda(1): Equivalence Ratio Current",""},
	{L"O2S7_WR_lambda(1): Equivalence Ratio Current",""},
	{L"O2S8_WR_lambda(1): Equivalence Ratio Current",""},
	{L"Catalyst Temperature Bank 1, Sensor 1",""},
	{L"Catalyst Temperature Bank 2, Sensor 1",""},
	{L"Catalyst Temperature Bank 1, Sensor 2",""},
	{L"Catalyst Temperature Bank 2, Sensor 2",""},
	{L"PIDs supported [41 - 60]",""},
	{L"Monitor status this drive cycle",""},
	{L"Control module voltage",""},
	{L"Absolute load value",""},
	{L"Command equivalence ratio",""},
	{L"Relative throttle position",""},
	{L"Ambient air temperature",""},
	{L"Absolute throttle position B",""},
	{L"Absolute throttle position C",""},
	{L"Accelerator pedal position D",""},
	{L"Accelerator pedal position E",""},
	{L"Accelerator pedal position F",""},
	{L"Commanded throttle actuator",""},
	{L"Time run with MIL on",""},
	{L"Time since trouble codes cleared",""},
	{L"Maximum value for equivalence ratio, oxygen sensor voltage, oxygen sensor current, and intake manifold absolute pressure",""},
	{L"Maximum value for air flow rate from mass air flow sensor",""},
	{L"Fuel Type",""},
	{L"Ethanol fuel %",""},
	{L"Absolute Evap system Vapour Pressure",""},
	{L"Evap system vapor pressure",""},
	{L"Short term secondary oxygen sensor trim bank 1 and bank 3",""},
	{L"Long term secondary oxygen sensor trim bank 1 and bank 3",""},
	{L"Short term secondary oxygen sensor trim bank 2 and bank 4",""},
	{L"Long term secondary oxygen sensor trim bank 2 and bank 4",""},
	{L"Fuel rail pressure (absolute)",""},
	{L"Relative accelerator pedal position",""},
	{L"Hybrid battery pack remaining life",""},
	{L"Engine oil temperature",""},
	{L"Fuel injection timing",""},
	{L"Engine fuel rate",""},
	{L"Emission requirements to which vehicle is designed",""},
	{L"PIDs supported [61 - 80]",""},
	{L"Driver's demand engine - percent torque",""},
	{L"Actual engine - percent torque",""},
	{L"Engine reference torque",""},
	{L"Engine percent torque data",""},
	{L"Auxiliary input / output supported",""},
	{L"Mass air flow sensor",""},
	{L"Engine coolant temperature",""},
	{L"Intake air temperature sensor",""},
	{L"Commanded EGR and EGR Error",""},
	{L"Commanded Diesel intake air flow control and relative intake air flow position",""},
	{L"Exhaust gas recirculation temperature",""},
	{L"Commanded throttle actuator control and relative throttle position",""},
	{L"Fuel pressure control system",""},
	{L"Injection pressure control system",""},
	{L"Turbocharger compressor inlet pressure",""},
	{L"Boost pressure control",""},
	{L"Variable Geometry turbo (VGT) control",""},
	{L"Wastegate control",""},
	{L"Exhaust pressure",""},
	{L"Turbocharger RPM",""},
	{L"Turbocharger temperature",""},
	{L"Turbocharger temperature",""},
	{L"Charge air cooler temperature (CACT)",""},
	{L"Exhaust Gas temperature (EGT) Bank 1",""},
	{L"Exhaust Gas temperature (EGT) Bank 2",""},
	{L"Diesel particulate filter (DPF)",""},
	{L"Diesel particulate filter (DPF)",""},
	{L"Diesel Particulate filter (DPF) temperature",""},
	{L"NOx NTE control area status",""},
	{L"PM NTE control area status",""},
	{L"Engine run time",""},
	{L"PIDs supported [81 - A0]",""},
	{L"Engine run time for AECD",""},
	{L"Engine run time for AECD",""},
	{L"NOx sensor",""},
	{L"Manifold surface temperature",""},
	{L"NOx reagent system",""},
	{L"Particulate matter (PM) sensor",""},
	{L"Intake manifold absolute pressure",""},
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
  {L"Fuel Level","%3.2f%%"},
  {L"RPM","%3.2f"},
  {L"Throttle Position","%3.2f"},
  {L"Brake Position","%3.2f"},
  {L"Clutch Position","%3.2f"},
  {L"LF Wheelspeed","%3.2f"},
  {L"RF Wheelspeed","%3.2f"},
  {L"LR Wheelspeed","%3.2f"},
  {L"RR Wheelspeed","%3.2f"},
  {L"Exhaust Temperature","%3.2f"},
  {L"LF Tire Temperature","%3.2f"},
  {L"RF Tire Temperature","%3.2f"},
  {L"LR Tire Temperature","%3.2f"},
  {L"RR Tire Temperature","%3.2f"},
  {L"LF Brake Temperature","%3.2f"},
  {L"RF Brake Temperature","%3.2f"},
  {L"LR Brake Temperature","%3.2f"},
  {L"RR Brake Temperature","%3.2f"},
  {L"Oil Pressure","%3.2f"},
  {L"Oil Temperature","%3.2f"},
  {L"Coolant Temperature","%3.2f"},
  {L"Alternator voltage","%3.2fV"},
};

void GetDataChannelName(DATA_CHANNEL eDC, LPTSTR lpszName, int cch)
{
  LPCTSTR lpszDataName = NULL;
  switch(eDC)
  {
  case DATA_CHANNEL_X: lpszDataName = L"Longitude"; break;
  case DATA_CHANNEL_Y: lpszDataName = L"Latitude"; break;
  case DATA_CHANNEL_DISTANCE: lpszDataName = L"Distance"; break;
  case DATA_CHANNEL_VELOCITY: lpszDataName = L"Velocity"; break;
  case DATA_CHANNEL_TIMESLIP: lpszDataName = L"Time-slip"; break;
  case DATA_CHANNEL_X_ACCEL: lpszDataName = L"X accel"; break;
  case DATA_CHANNEL_Y_ACCEL: lpszDataName = L"Y accel"; break;
  case DATA_CHANNEL_Z_ACCEL: lpszDataName = L"Z accel"; break;
  case DATA_CHANNEL_TEMP: lpszDataName = L"Temperature"; break;
  case DATA_CHANNEL_RECEPTION_X: lpszDataName = L"Wifi Dots X"; break;
  case DATA_CHANNEL_RECEPTION_Y: lpszDataName = L"Wifi Dots Y"; break;
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
        lpszDataName = L"Unrecognized IOIO pin";
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
  CASSERT(DATA_CHANNEL_COUNT == 0x401);

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
      sprintf(lpsz, "%4.4f", flValue);
      break;
    }
    case DATA_CHANNEL_VELOCITY:
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
      sprintf(lpsz, "%4.4fg", flValue);
      break;
    }
    case DATA_CHANNEL_Y_ACCEL:
    {
      sprintf(lpsz, "%4.4fg", flValue);
      break;
    }
    case DATA_CHANNEL_Z_ACCEL:
    {
      sprintf(lpsz, "%4.4fg", flValue);
      break;
    }
    case DATA_CHANNEL_TEMP:
    {
      sprintf(lpsz, "%4.4fc", flValue);
      break;
    }
    case DATA_CHANNEL_RECEPTION_X:
    case DATA_CHANNEL_RECEPTION_Y:
    {
      sprintf(lpsz,"%4.4f",flValue);
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
        sprintf(lpsz, "%3.3fV", flValue);
      }
      else if(eX >= DATA_CHANNEL_IOIOCUSTOM_START && eX <= DATA_CHANNEL_IOIOCUSTOM_END)
      {
        const int custom = eX - DATA_CHANNEL_IOIOCUSTOM_START;
        sprintf(lpsz,g_rgIOIOCustomData[custom].pDataFormat, flValue);
      }
      else
      {
        sprintf(lpsz,"%f",flValue);
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

    if((ixBestIndex == -1 || dAvg < dClosest) && dPctDiff < 0.25)
    {
      dClosest = dAvg;
      ixBestIndex = ixCheck;
    }
    if(dAvg < 5e-9 && ixBestIndex >= 0)
    {
      break; // early-out: if we found one that is close enough, just stop here
    }


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
  if(lstPoints.size() <= 3) return;


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
    IDataChannel* pX = pLapDB->AllocateDataChannel();
    IDataChannel* pY = pLapDB->AllocateDataChannel();

    pX->Init(GetLap()->GetLapId(), DATA_CHANNEL_X);
    pY->Init(GetLap()->GetLapId(), DATA_CHANNEL_Y);
    pDistance->Init(GetLap()->GetLapId(), DATA_CHANNEL_DISTANCE);
    pVelocity->Init(GetLap()->GetLapId(), DATA_CHANNEL_VELOCITY);
    int iStartPoint = 0;
    for(int x = 0;x < lstPoints.size(); x++)
    {
      const TimePoint2D& p = lstPoints[x];

      pX->AddPoint(p.iTime,p.flX);
      pY->AddPoint(p.iTime,p.flY);

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
        if(lnD.IntersectLine(lnPerp, &dHitLength) && dHitLength >= 0 && dHitLength <= 1.0)
        {
          // hooray, they intersect
          const double dPercent = dHitLength;
          const double dThisDistance = (dD1Distance * (1-dPercent)) + (dD2Distance * dPercent);
          m_lstPoints.push_back(TimePoint2D(p));
          pDistance->AddPoint((int)p.iTime,dThisDistance);
          pVelocity->AddPoint((int)p.iTime,p.flVelocity);
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

        int iRefCheckStart = 1; // what index should we start at?  this gets changed each loop so that we always start near the point that is most likely to be near the current m_lstPoints point
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
                const double dEstimatedElapsedTime = ((1.0-dFraction)*iLastTime + dFraction*iThisTime) - (double)iReferenceStartTime; // this is the estimated time for the previous lap at this position
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
    IDataChannel* pTimeSlip = pLapDB->AllocateDataChannel();
    pDistance->Init(GetLap()->GetLapId(), DATA_CHANNEL_DISTANCE);
    pVelocity->Init(GetLap()->GetLapId(), DATA_CHANNEL_VELOCITY);
    pTimeSlip->Init(GetLap()->GetLapId(), DATA_CHANNEL_TIMESLIP);
    for(int x = 1;x < lstPoints.size(); x++)
    {
      const TimePoint2D& p = lstPoints[x];
      pX->AddPoint(p.iTime,p.flX);
      pY->AddPoint(p.iTime,p.flY);

      const double dX = p.flX - ptLast.flX;
      const double dY = p.flY - ptLast.flY;
      const double d = sqrt(dX*dX + dY*dY);
      dDistance += d;
      m_lstPoints.push_back(TimePoint2D(p));
      ptLast = p;

      pDistance->AddPoint(p.iTime, dDistance);
      pVelocity->AddPoint(p.iTime, p.flVelocity);
      pTimeSlip->AddPoint(p.iTime, 0);
    }

    pDistance->Lock();
    pVelocity->Lock();
    pTimeSlip->Lock();
    AddChannel(pDistance);
    AddChannel(pVelocity);
    AddChannel(pTimeSlip);
    AddChannel(pX);
    AddChannel(pY);
  }
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