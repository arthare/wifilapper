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

public class Vector2D 
{
	public Vector2D(float x, float y)
	{
		this.x = x;
		this.y = y;
	}
	public Vector2D(Parcel in)
	{
		this.x = in.readFloat();
		this.y = in.readFloat();
	}
	public static Vector2D P1MinusP2(Point2D p1, Point2D p2)
	{
		return new Vector2D(p1.GetX() - p2.GetX(), p1.GetY() - p2.GetY());
	}
	public static Vector2D FromBearing(float flBearingDegrees, float flMagnitude)
	{
		final float flBearingRadians = flBearingDegrees / 180.0f * 3.14159f;
		return new Vector2D((float)(flMagnitude*Math.sin(flBearingRadians)), (float)(flMagnitude*Math.cos(flBearingRadians)));
	}
	public Vector2D RotateAboutOrigin(double dAngRadians)
	{
		Vector2D vResult = new Vector2D((float)(x * Math.cos(dAngRadians) - y * Math.sin(dAngRadians)), (float)(y * Math.cos(dAngRadians) + x * Math.sin(dAngRadians)));
		return vResult;
	}
	public Vector2D Minus(Vector2D vOther)
	{
		return new Vector2D(x - vOther.x,y - vOther.y);
	}
	public void Set(float x, float y)
	{
		this.x = x;
		this.y = y;
	}
	
	// returns a vector2D rotated about (0,0) by flAngleDegrees
	public Vector2D Rotate(float flAngleDegrees)
	{
		flAngleDegrees = -flAngleDegrees; // we want these rotations to go clockwise
		final float flRotateRadians = flAngleDegrees / 180.0f * 3.14159f;
		float flNewX = (float)(x*Math.cos(flRotateRadians) - y * Math.sin(flRotateRadians));
		float flNewY = (float)(y*Math.cos(flRotateRadians) + x * Math.sin(flRotateRadians));
		return new Vector2D(flNewX,flNewY);
	}
	public float GetX()
	{
		return x;
	}
	public float GetY()
	{
		return y;
	}
	public double DotProduct(Vector2D vOther)
	{
		return x * vOther.x + y * vOther.y;
	}
	public Vector2D Multiply(float d)
	{
		return new Vector2D(d * x, d * y);
	}
	public float GetLength()
	{
		return (float)Math.sqrt(x*x + y*y);
	}
	public Vector2D GetPerpindicular()
	{
		return new Vector2D(-y, x);
	}
	public Vector2D GetUnitVector()
	{
		float dLength = GetLength();
		return new Vector2D(x / dLength, y / dLength);
	}
	private float x;
	private float y;
}
