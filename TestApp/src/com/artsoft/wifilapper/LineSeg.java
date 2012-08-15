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

import android.os.Parcel;

public class LineSeg 
{
	public LineSeg()
	{
		this.p1 = new Point2D(0,0);
		this.p2 = new Point2D(0,0);
	}
	public LineSeg(Parcel in)
	{
		this.p1 = new Point2D(in);
		this.p2 = new Point2D(in);
	}
	public LineSeg(Point2D p1, Point2D p2)
	{
		assert(p1 != null && p2 != null);
		this.p1 = p1;
		this.p2 = p2;
	}
	public LineSeg(Point2D ptStart, Vector2D vDirection)
	{
		this.p1 = ptStart;
		this.p2 = new Point2D(ptStart.x + vDirection.GetX(), ptStart.y + vDirection.GetY());
	}
	public Point2D GetP1()
	{
		return p1;
	}
	public Point2D GetP2()
	{
		return p2;
	}
	public FloatRect GetBounds()
	{
		FloatRect rc = new FloatRect();
		rc.left = Math.min(p1.x, p2.x);
		rc.right = Math.max(p1.x, p2.x);
		rc.top = Math.min(p1.y, p2.y);
		rc.bottom = Math.max(p1.y, p2.y);
		return rc;
	}
	public boolean Intersect(LineSeg lnOther, IntersectData data, boolean fNeedsToBeInLineSegmentA, boolean fNeedsToBeInLineSegmentB)
	{
		// we need to represent each line parametrically
		// La = lnThis.p1 + ua(lnThis.p2 - lnThis.p1)
		// Lb = lnOther.p1 + ub(lnOther.p2 - lnOther.p2)
		// ua = [stuff] / [stuff] at the point of intersection
		// ub = [stuff] / [stuff] at the point of intersection
		
		final float x1 = this.p1.x;
		final float x2 = this.p2.x;
		final float x3 = lnOther.p1.x;
		final float x4 = lnOther.p2.x;
		
		final float y1 = this.p1.y;
		final float y2 = this.p2.y;
		final float y3 = lnOther.p1.y;
		final float y4 = lnOther.p2.y;
		
		final float uA = ((x4 - x3) * (y1 - y3) - (y4 - y3) * (x1 - x3)) / ((y4 - y3)*(x2 - x1) - (x4 - x3)*(y2 - y1));
		final float uB = ((x2 - x1) * (y1 - y3) - (y2 - y1) * (x1 - x3)) / ((y4 - y3)*(x2 - x1) - (x4 - x3)*(y2 - y1));
	
		if((uA >= 0 && uA <= 1.0 || !fNeedsToBeInLineSegmentA) && (uB >= 0 && uB <= 1.0 || !fNeedsToBeInLineSegmentB)) // make sure this intersection is within our line segment!
		{
			Vector2D vec = Vector2D.P1MinusP2(p2, p1);
			Point2D ptIntersect = p1.Add(vec.Multiply(uA));
			data.Set(uA, uB, ptIntersect, true);
			return true;
		}
		else
		{
			data.Set(0,0,null,false);
			return false;
		}
	}
	public float GetLength()
	{
		float dX = p1.x - p2.x;
		float dY = p1.y - p2.y;
		return (float)Math.sqrt(dX*dX + dY*dY);
	}
	public static class IntersectData
	{
		private float dFractionThis; // how far along this "this" line segment (the one Intersect() was called on) the hit was.
		private float dFractionOther; // how far along the "other" line segment (the one passed as a param to Intersect) the hit was.
		private boolean fWasIntersect;
		private Point2D pt;
		
		public IntersectData()
		{
			dFractionThis = 0;
			dFractionOther = 0;
			fWasIntersect = false;
			pt = null;
		}
		public void Set(float dFractionThis, float dFractionOther, Point2D pt, boolean fWasIntersect)
		{
			this.dFractionThis = dFractionThis;
			this.dFractionOther = dFractionOther;
			this.fWasIntersect = fWasIntersect;
			this.pt = pt;
		}
		public Point2D GetPoint() {assert(fWasIntersect); return this.pt;}
		public float GetThisFraction() {assert(fWasIntersect); return dFractionThis;}
		public float GetOtherFraction() {assert(fWasIntersect); return dFractionOther;}
	}
	
	private Point2D p1;
	private Point2D p2;
}
