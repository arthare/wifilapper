package com.artsoft.wifilapper;

public class Point2D
{
	public Point2D(float x, float y)
	{
		this.x = x;
		this.y = y;
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