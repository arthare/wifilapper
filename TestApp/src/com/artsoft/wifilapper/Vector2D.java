package com.artsoft.wifilapper;

public class Vector2D 
{
	public Vector2D(float x, float y)
	{
		this.x = x;
		this.y = y;
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
