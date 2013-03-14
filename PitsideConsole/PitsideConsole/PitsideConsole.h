#pragma once

enum NOTIFY
{
  NOTIFY_NEWLAP,
  NOTIFY_NEWNETSTATUS,
  NOTIFY_NEWDATA,
  NOTIFY_NEWMSGDATA,
  NOTIFY_NEWDATABASE, // an entire database has been shipped here from the net thread
  NOTIFY_NEEDRECVCONFIG,
};
/*
struct PlotPrefs
{
   TCHAR m_ChannelName[512];
   DATA_CHANNEL iDataChannel;
   bool iPlotView;
   double fMinValue;
   double fMaxValue;
};
extern PlotPrefs m_PlotPrefs[50]; // extern tells the compiler: "someone somewhere will declare this for real.  Until you encounter that, assume that it'll get declared eventually"
*/