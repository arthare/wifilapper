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
