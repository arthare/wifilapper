#include "Stdafx.h"
#include "ArtVector.h"


Vector2D V2D(double x, double y)
{
  Vector2D v;
  v.m_v[0] = x;
  v.m_v[1] = y;
  return v;
}