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
