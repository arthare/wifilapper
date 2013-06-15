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

public class FloatRect 
{
	public FloatRect() {}
	public FloatRect(float dLeft, float dTop, float dRight, float dBottom)
	{
		left = dLeft;
		top = dTop;
		right = dRight;
		bottom = dBottom;
	}
	public FloatRect(FloatRect rcOther)
	{
		left = rcOther.left;
		top = rcOther.top;
		right = rcOther.right;
		bottom = rcOther.bottom;
	}
	public FloatRect Union(FloatRect rcOther)
	{
		return new FloatRect(Math.min(left,rcOther.left),   Math.min(top,rcOther.top),  Math.max(right,rcOther.right),  Math.max(bottom,rcOther.bottom));
	}
	public float ExactCenterX()
	{
		return (left + right)/2.0f;
	}
	public float ExactCenterY()
	{
		return (top + bottom)/2.0f;
	}
	public float height()
	{
		return bottom - top;
	}
	public float width()
	{
		return right-left;
	}
	public float left;
	public float top;
	public float right;
	public float bottom;
}
