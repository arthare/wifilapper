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

public class Point2D
{
	public Point2D(float x, float y)
	{
		this.x = x;
		this.y = y;
	}
	public Point2D(Parcel in)
	{
		this.x = in.readFloat();
		this.y = in.readFloat();
	}
	public Point2D Add(Vector2D vec)
	{
		return new Point2D(x + vec.GetX(), y + vec.GetY());
	}
	public Point2D Subtract(Vector2D vec)
	{
		return new Point2D(x - vec.GetX(), y - vec.GetY());
	}
	public float GetX() {return x;}
	public float GetY() {return y;}
	
	public float x;
	public float y;
}