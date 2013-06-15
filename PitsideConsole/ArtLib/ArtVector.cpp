#include "Stdafx.h"
#include "ArtVector.h"


Vector2D V2D(double x, double y)
{
  Vector2D v;
  v.m_v[0] = x;
  v.m_v[1] = y;
  return v;
}
Vector2D V2D(const Vector2D& pos, double dAng, double dLen)
{
  Vector2D v;
  v.m_v[0] = pos.m_v[0] + cos(dAng)*dLen;
  v.m_v[1] = pos.m_v[1] + sin(dAng)*dLen;
  return v;
}