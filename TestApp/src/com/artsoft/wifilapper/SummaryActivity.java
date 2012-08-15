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

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.URI;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;
import java.util.Map;
import java.util.Vector;

import com.artsoft.wifilapper.LapAccumulator.DataChannel;
import com.artsoft.wifilapper.Prefs.UNIT_SYSTEM;
import com.artsoft.wifilapper.RaceDatabase.LapData;
import com.artsoft.wifilapper.RaceDatabase.RaceData;

import android.app.Activity;
import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.Intent;
import android.content.UriMatcher;
import android.content.res.Resources;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.graphics.Picture;
import android.graphics.Rect;
import android.net.Uri;
import android.os.Bundle;
import android.provider.MediaStore.Images;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.TextView;
import android.widget.Toast;

public class SummaryActivity extends Activity implements OnClickListener
{

	private long m_lRaceId;
	private String m_strImagePath;
	private Uri m_uriImage;
	private boolean m_fUseImperial;
	@Override
	public void onCreate(Bundle extras)
	{
		super.onCreate(extras);
		
		this.setContentView(R.layout.summaryscreen);

		Intent i = getIntent();
		if(i != null)
		{
			TextView txtRace = (TextView)findViewById(R.id.txtRaceName);
			txtRace.setText(i.getStringExtra(Prefs.IT_RACENAME_STRING));
			String strUnits = i.getStringExtra(Prefs.IT_UNITS_STRING);
			if(strUnits != null)
			{
				Prefs.UNIT_SYSTEM eSystem = UNIT_SYSTEM.valueOf(strUnits);
				m_fUseImperial = eSystem.equals(UNIT_SYSTEM.MPH);
			}
			else
			{
				m_fUseImperial = true;
			}
		}
		else
		{
			finish();
		}
	}
	
	@Override
	public void onResume()
	{
		super.onResume();
		
		Intent i = getIntent();
		if(i != null)
		{
			ImageButton btnImage = (ImageButton)findViewById(R.id.btnPickImage);
			Button btnShare = (Button)findViewById(R.id.btnShare);
			
			btnImage.setOnClickListener(this);
			btnShare.setOnClickListener(this);
			m_lRaceId = i.getLongExtra(Prefs.IT_RACEID_LONG, -1);
			if(m_lRaceId >= 0)
			{
				// we're good to go!
			}
			else
			{
				Toast.makeText(this, "Invalid race ID", Toast.LENGTH_SHORT).show();
				finish();
			}
		}
		else
		{
			finish();
		}
	}
	
	@Override
	public void onPause()
	{
		super.onPause();
	}
	@Override
	public void onClick(View v) 
	{
		if(v.getId() == R.id.btnPickImage)
		{
			// time to pick an image
			Intent photoPickerIntent = new Intent(Intent.ACTION_PICK);
			photoPickerIntent.setType("image/*");
			startActivityForResult(photoPickerIntent, 0);
		}
		else if(v.getId() == R.id.btnShare)
		{
			EditText txtLocation = (EditText)findViewById(R.id.txtLocation);
			EditText txtCar = (EditText)findViewById(R.id.txtCar);
			EditText txtRace = (EditText)findViewById(R.id.txtRaceName);
			
			Resources res = getResources();
			Bitmap bmp = CreateSummaryImage(res, RaceDatabase.Get(),txtRace.getText().toString(), txtLocation.getText().toString(), txtCar.getText().toString(),m_strImagePath,m_fUseImperial, m_lRaceId);
			if(bmp == null)
			{
				Toast.makeText(this, "Failed to generate summary for unknown reasons.  Try again maybe?", Toast.LENGTH_LONG);
			}
			File outFile;
			try
			{
				outFile=File.createTempFile("summary", ".jpg", getExternalCacheDir());
				FileOutputStream out = new FileOutputStream(outFile);
				
				bmp.compress(CompressFormat.PNG, 95, out);
				out.close();
				bmp.recycle();
				bmp = null;
			}
			catch(IOException e)
			{
				outFile = null;
			}
			if(outFile != null)
			{
				
				ContentValues image = new ContentValues();

				image.put(Images.Media.TITLE, "ImageTitle");
				image.put(Images.Media.MIME_TYPE, "image/png");
				image.put(Images.Media.ORIENTATION, 0);

				File parent = outFile.getParentFile();
				String path = parent.toString().toLowerCase();
				String name = parent.getName().toLowerCase();
				image.put(Images.ImageColumns.BUCKET_ID, path.hashCode());
				image.put(Images.ImageColumns.BUCKET_DISPLAY_NAME, name);
				image.put(Images.Media.SIZE, outFile.length());
				image.put(Images.Media.DATA, outFile.getAbsolutePath());
				
				Uri uri = getContentResolver().insert(Images.Media.EXTERNAL_CONTENT_URI, image);
				
				Intent share = new Intent(Intent.ACTION_SEND);
				share.setType("image/png");
				share.putExtra(Intent.EXTRA_STREAM, uri);
				startActivity(Intent.createChooser(share, "Share Image"));
	
			}
			else
			{
				Toast.makeText(this,"Failed to create share-able image",Toast.LENGTH_SHORT).show();
			}
		}
	}
	
	@Override
	public void onActivityResult(int requestCode, int resultCode, Intent data) 
	{
		super.onActivityResult(requestCode, resultCode, data);
		if(requestCode == 0 && resultCode == Activity.RESULT_OK)
		{
			// let's look at the data!
			m_uriImage = data.getData();
			m_strImagePath = Utility.GetImagePathFromURI(this,m_uriImage);
		}
	}
	
	private static void DrawCornerImages(Resources res, String strChosenImage, Canvas canvas, Rect rcImage)
	{
		boolean fIsLogo = false;
		Bitmap bmpImage=null;
		if(strChosenImage != null)
		{
			bmpImage = BitmapFactory.decodeFile(strChosenImage);
		}
		if(bmpImage == null)
		{
			// if they didn't supply an image, use the WifiLapper logo
			bmpImage = BitmapFactory.decodeResource(res, R.drawable.iconbig);
			fIsLogo = true;
		}
		if(bmpImage != null)
		{
			if(fIsLogo) rcImage.bottom-=40;
			
			Rect rcSrc = new Rect(0,0,bmpImage.getWidth(), bmpImage.getHeight());
			Rect rcDst;
			double dAspect = (double)bmpImage.getWidth() / (double)bmpImage.getHeight();
			double dWidth;
			double dHeight;
			if(dAspect > 1)
			{
				// image is wider than high
				dWidth = rcImage.width() - 10;
				dHeight = dWidth / dAspect;
				rcDst = new Rect(rcImage.left+10,rcImage.top+10,rcImage.right,(int)(rcImage.top+10+dHeight));
			}
			else
			{
				// image is taller than wide
				dHeight = rcImage.height() - 10;
				dWidth = dHeight * dAspect;
				rcDst = new Rect(rcImage.left+10,rcImage.top+10,(int)(rcImage.left+10+dWidth),rcImage.bottom);
			}
			Rect rcBackground = new Rect(rcImage.left,rcImage.top,rcDst.right-10,rcDst.bottom-10);
			Paint paint = new Paint();
			paint.setShadowLayer(5, 5, 5, 0x00000000);
			paint.setARGB(255, 50, 50, 50);
			
			if(!fIsLogo)
			{
				canvas.drawRect(rcBackground, paint);
			}
			
			canvas.drawBitmap(bmpImage, rcSrc, rcDst, paint);

			if(fIsLogo)
			{
				rcImage.bottom+=40;
				Rect rcTitle = new Rect(rcImage.left,rcImage.bottom-40,rcImage.right,rcImage.bottom-20);
				Paint paintText = new Paint();
				paintText.setARGB(255,0, 0, 0);
				Utility.DrawFontInBox(canvas, "WifiLapper",paintText, rcTitle);
				
				Rect rcSite = new Rect(rcImage.left,rcImage.bottom-20,rcImage.right,rcImage.bottom);
				Utility.DrawFontInBox(canvas, "sites.google.com/site/wifilapper",paintText, rcSite);
			}
		}
	}
	private static void DrawHeader(LapAccumulator lapBest, RaceData rd, String strRaceName, String strLocation, String strVehicleName, boolean fDisplayImperial, float flLength, Canvas canvas, Rect rcHeader)
	{
		Rect rcTitle = new Rect(rcHeader.left,rcHeader.top,rcHeader.right,rcHeader.height()/3);
		Paint paintBigText = new Paint();
		paintBigText.setARGB(255,0,0,0);
		
		Utility.DrawFontInBox(canvas, strRaceName, paintBigText, rcTitle);
		
		final int boxesHeight = rcHeader.height() - rcTitle.height();
		Rect rcBoxes[][] = new Rect[3][2];
		for(int col = 0; col < 3; col++)
		{
			for(int row = 0;row < 2; row++)
			{
				rcBoxes[col][row] = new Rect(rcHeader.left + col*rcHeader.width()/rcBoxes.length, rcTitle.bottom + (row*boxesHeight)/rcBoxes[0].length, rcHeader.left + (col+1)*rcHeader.width()/rcBoxes.length-5, rcTitle.bottom + ((row+1)*boxesHeight)/rcBoxes[0].length);
			}
		}
		
		
		String strDistanceUnits = "km";
		String strSpeedUnits = "km/h";
		if(fDisplayImperial)
		{
			flLength = flLength / 1.609f;
			strDistanceUnits = "mi";
			strSpeedUnits = "mph";
		}
		final float flHours = (float)lapBest.GetLapTime() / 3600.0f;
		final float flAvgSpeed = flLength / flHours;
		String strBoxText[][] = new String[3][2];
		strBoxText[0][0] =  "Date: " + Utility.GetDateStringFromUnixTime(rd.unixTimeMsStart) + " to " + Utility.GetDateStringFromUnixTime(rd.unixTimeMsEnd);
		strBoxText[0][1] =  "Car: " + strVehicleName;
		strBoxText[1][0] =  "Location: " + strLocation;
		strBoxText[1][1] =  "Circuit Length: " + Utility.FormatFloat(flLength,2) + strDistanceUnits;
		strBoxText[2][0] =  "Best Lap: " + Utility.FormatSeconds((float)lapBest.GetLapTime());
		strBoxText[2][1] =  "Best Lap Avg Speed: " + Utility.FormatFloat(flAvgSpeed,2) + strSpeedUnits;
		// draw information.
		// first pass: figure out smallest font size
		// 2nd pass: draw with that font size
		Paint paintSmallText = new Paint();
		paintSmallText.setARGB(255, 0, 0, 0);
		float flNeededFontSize = 1000;
		for(int col = 0; col < 3; col++)
		{
			for(int row = 0; row < 2; row++)
			{
				Utility.DrawFontInBox(canvas, strBoxText[col][row], paintSmallText, rcBoxes[col][row], false);
				flNeededFontSize = Math.min(paintSmallText.getTextSize(),flNeededFontSize);
			}
		}
		paintSmallText.setTextSize(flNeededFontSize);
		for(int col = 0; col < 3; col++)
		{
			for(int row = 0; row < 2; row++)
			{
				Utility.DrawFontInBoxFinal(canvas, strBoxText[col][row], flNeededFontSize, paintSmallText, rcBoxes[col][row], true, false);				
			}
		}
	}
	private static class LabelData
	{
		public String strLabel;
		public int cyPosition; // position in pixels within rcGraph.  valid values start at rcGraph.top, end at rcGraph.bottom.
	}
	
	// ret[0] = max
	// ret[1] = middle
	// ret[2] = min
	private static LabelData[] GetLabelData(FloatRect rcInWorld, Rect rcGraph, boolean fUseImperial)
	{
		float flMin,flMax,flMiddle;
		float flMinMeters,flMaxMeters,flMiddleMeters;
		flMinMeters = rcInWorld.top;
		flMaxMeters = rcInWorld.bottom;
		flMiddleMeters = (rcInWorld.bottom + rcInWorld.top)/2;
		
		String strSuffix;
		if(fUseImperial)
		{
			flMin = flMinMeters * 2.2369f;
			flMiddle = flMiddleMeters*2.2369f;
			flMax = flMaxMeters * 2.2369f; // converts to mph
			strSuffix = "mph";
		}
		else
		{
			flMin = flMinMeters * 3.6f;
			flMiddle = flMiddleMeters*3.6f;
			flMax = flMaxMeters * 3.6f;
			strSuffix = "km/h";
		}

		final int iMax = (int)flMax;
		final int iMin = ((int)flMin)+1; // figures out the integer speeds we'll use for the labels
		final int iMiddle = (int)flMiddle;
		
		LabelData[] ret = new LabelData[3];
		for(int x = 0;x < ret.length; x++) ret[x] = new LabelData();
		
		ret[0].strLabel = iMax + " " + strSuffix;
		ret[1].strLabel = iMiddle + " " + strSuffix;
		ret[2].strLabel = iMin + " " + strSuffix;
		
		{final float flPct = 1.0f - ((flMaxMeters-flMinMeters) / rcInWorld.height());
		final float flPixels = flPct * rcGraph.height();
		ret[0].cyPosition = (int)(flPixels + rcGraph.top);}
		
		{final float flPct = 1.0f - ((flMiddleMeters-flMinMeters) / rcInWorld.height());
		final float flPixels = flPct * rcGraph.height();
		ret[1].cyPosition = (int)(flPixels + rcGraph.top);}
		
		{final float flPct = 1.0f - ((flMinMeters-flMinMeters) / rcInWorld.height());
		final float flPixels = flPct * rcGraph.height();
		ret[2].cyPosition = (int)(flPixels + rcGraph.top);}
		
		return ret;
	}
	private static void DrawYAxis(Canvas canvas, Paint paintLine, Paint paintText, FloatRect rcInWorld, Rect rcGraph, Rect rcText, boolean fUseImperial)
	{
		LabelData[] labels = GetLabelData(rcInWorld, rcGraph, fUseImperial);
		for(int x = 0;x < labels.length; x++)
		{
			canvas.drawLine(rcGraph.left, labels[x].cyPosition, rcGraph.right, labels[x].cyPosition, paintLine);
			canvas.drawText(labels[x].strLabel, rcText.left, labels[x].cyPosition, paintText);
		}
	}
	private static void DrawSpeedDistance(Canvas canvas, LapAccumulator[] laps, Paint[] paintPercentiles, Rect rc, boolean fUseImperial)
	{
		
		FloatRect rcInWorld = null;
		for(int x = 0; x < laps.length; x++)
		{
			if(rcInWorld == null)
			{
				rcInWorld = laps[x].GetBounds(true);
			}
			else
			{
				FloatRect rcNew = laps[x].GetBounds(true);
				rcInWorld = rcInWorld.Union(rcNew);
			}
		}
		
		List<LineSeg> lstSF = laps[0].GetSplitPoints(true);

		final int cxLegendLine = 15;
		final int cxLegendTotal = 100;
		Rect rcGraph = new Rect(rc.left+cxLegendTotal,rc.top,rc.right-100,rc.bottom);
		Rect rcLegendText = new Rect(rc.left+cxLegendLine, rc.top,rcGraph.left,rc.bottom);
		Rect rcYAxisText = new Rect(rcGraph.right,rcGraph.top,rc.right,rcGraph.bottom);
		
		Rect rcPercentile[] = new Rect[4];
		for(int x = 0;x < rcPercentile.length; x++)
		{
			rcPercentile[x] = new Rect(rcLegendText.left,rcLegendText.top + x*rc.height()/rcPercentile.length, rcGraph.left, rcLegendText.top + (x+1)*rc.height()/rcPercentile.length);
		}

		// we want to label the integer just below peak speed, the integer just below mid(max,min), and the integer just above min.
		Paint paintLines = new Paint();
		paintLines.setARGB(255,150,150,150);
		DrawYAxis(canvas, paintLines, paintLines, rcInWorld, rcGraph, rcYAxisText, fUseImperial);
		
		
		String rgText[] = {"Fastest Lap", "75th percentile","50th percentile","25th percentile"};
		for(int x = rcPercentile.length-1;x >= 0; x--)
		{
			canvas.drawLine(rc.left,rcPercentile[x].exactCenterY(),rc.left+cxLegendLine,rcPercentile[x].exactCenterY(),paintPercentiles[x]);
			Utility.DrawFontInBox(canvas, rgText[x], paintPercentiles[x], rcPercentile[x]);
			LapAccumulator.DrawLap(laps[x], true, rcInWorld, canvas, paintPercentiles[x], paintPercentiles[x], rcGraph);
		}
		
	}
	private static class LapDataLapTimeSorter implements Comparator
	{
		@Override
		public int compare(Object arg0, Object arg1) 
		{
			LapData lap1 = (LapData)arg0;
			LapData lap2 = (LapData)arg1;
			if(lap1.flLapTime < lap2.flLapTime)return -1;
			if(lap1.flLapTime > lap2.flLapTime)return 1;
			return 0;
		}
		
	}
	private static class LapDataOrderSorter implements Comparator
	{
		@Override
		public int compare(Object arg0, Object arg1) 
		{
			LapData lap1 = (LapData)arg0;
			LapData lap2 = (LapData)arg1;
			if(lap1.lStartTime < lap2.lStartTime)return -1;
			if(lap1.lStartTime > lap2.lStartTime)return 1;
			return 0;
		}
		
	}
	private static Paint[] GetPercentilePaints()
	{
		Paint paintLine[] = new Paint[4];
		for(int x = 0;x < paintLine.length; x++)
		{
			paintLine[x] = new Paint();
		}
		paintLine[0].setARGB(255, 11, 60, 96);
		paintLine[1].setARGB(255, 23, 131, 210);
		paintLine[2].setARGB(255, 45, 152, 232);
		paintLine[3].setARGB(255, 102, 181, 238);
		paintLine[0].setStrokeWidth(3);
		paintLine[1].setStrokeWidth(1);
		paintLine[2].setStrokeWidth(1);
		paintLine[3].setStrokeWidth(1);
		return paintLine;
	}
	private static LapAccumulator[] GetPercentileLaps(SQLiteDatabase db, LapAccumulator.LapAccumulatorParams lapParams, int cSplits, LapData[] rgLaps)
	{
		LapAccumulator laps[] = new LapAccumulator[cSplits];
		{
			// figure out percentiles
			// 0 = best
			// 1 = 75th percentile
			// 2 = median
			// 3 = 25th percentile
			for(int x = 0;x < cSplits; x++)
			{
				int ix = (rgLaps.length * x) / cSplits;
				laps[x] = RaceDatabase.GetLap(db, lapParams, rgLaps[ix].lLapId);
				if(laps[x] != null)
				{
					laps[x].DoDeferredLoad(null, 0, x == 0); // load data for the fastest lap and for no others
				}
			}
		}
		return laps;
	}
	private static void DrawMap(Canvas canvas, LapAccumulator[] rgLaps, Paint[] paints, Rect rcMap)
	{
		FloatRect rcInWorld = null;
		for(int x = 0; x < rgLaps.length; x++)
		{
			if(rcInWorld == null)
			{
				rcInWorld = rgLaps[x].GetBounds(false);
			}
			else
			{
				FloatRect rcNew = rgLaps[x].GetBounds(false);
				rcInWorld = rcInWorld.Union(rcNew);
			}
		}
		Paint paintSplits = new Paint();
		paintSplits.setARGB(255, 255, 0, 0);
		paintSplits.setStrokeWidth(1);
		List<LineSeg> lstSF = rgLaps[0].GetSplitPoints(true);
		
		for(int x = rgLaps.length-1;x >= 0; x--)
		{
			LapAccumulator.DrawLap(rgLaps[x], false, rcInWorld, canvas, paints[x], paintSplits, rcMap);
		}
	}
	
	private static class Stint
	{
		private LapData m_rgLaps[] = null;
		private int m_ixEnd;
		private int m_ixStart;
		public Stint(LapData[] rgLaps, int start, int end)
		{
			m_ixStart = start;
			m_ixEnd = end;
			m_rgLaps = rgLaps;
		}
		
		public String GetString(int iStintNbr, int iString, boolean fLeft)
		{
			float flAverage = 0;
			float flBest = 1e30f;
			float flTotal = 0;
			final int cLaps = m_ixEnd - m_ixStart;
			
			for(int x = m_ixStart; x < m_ixEnd; x++)
			{
				flAverage += m_rgLaps[x].flLapTime;
				flTotal += m_rgLaps[x].flLapTime;
				flBest = Math.min(m_rgLaps[x].flLapTime, flBest);
			}
			
			if(cLaps != 0)
			{
				flAverage /= cLaps;
			}
			// returns the i'th string that describes this stint
			switch(iString)
			{
			case 0:
				String strTime = Utility.GetReadableTime(m_rgLaps[m_ixStart].lStartTime*1000);
				return fLeft ? "Stint " + (iStintNbr+1) + " (" + strTime + "):" : cLaps + " laps";
			case 1:
				return fLeft ? "Average Time:" : Utility.FormatSeconds(flAverage);
			case 2:
				return fLeft ? "Best Time:" : Utility.FormatSeconds(flBest);
			case 3:
				return fLeft ? "Length:" : Utility.FormatSeconds(flTotal);
			default:
				return null;
			}
		}
	}
	
	private static void DrawStints(Canvas canvas, LapData[] rgRawLaps, Rect rcStints)
	{
		LapData rgSortedLaps[] = new LapData[rgRawLaps.length];
		for(int x = 0; x < rgSortedLaps.length; x++)
		{
			rgSortedLaps[x] = new LapData(rgRawLaps[x]); 
		}
		Arrays.sort(rgSortedLaps, new LapDataOrderSorter());
		
		// first, let's figure out the stint distribution
		float flAverageLap = 0;
		for(int x = 0;x < rgSortedLaps.length; x++)
		{
			flAverageLap += rgSortedLaps[x].flLapTime;
		}
		flAverageLap /= rgSortedLaps.length;
		
		List<Stint> lstStints = new ArrayList<Stint>();
		{
			int iLastStint = 0;
			for(int x = 1;x < rgSortedLaps.length; x++)
			{
				if(rgSortedLaps[x].lStartTime - rgSortedLaps[x-1].lStartTime > flAverageLap + 240 && iLastStint < x-1)
				{
					// this lap was 4 minutes slower, so let's consider that a stint
					Stint st = new Stint(rgSortedLaps, iLastStint, x);
					lstStints.add(st);
					iLastStint = x;
				}
			}
			if(iLastStint != rgSortedLaps.length)
			{
				lstStints.add(new Stint(rgSortedLaps, iLastStint, rgSortedLaps.length));
			}
		}

		Paint paintText = new Paint();
		paintText.setARGB(255, 0, 0, 0);
		
		final int cyLineHeight = 15;
		final int cyFluffHeight = 5; // a once-per-stint gap 
		final int cLinesAvailable = (rcStints.height()-cyFluffHeight*lstStints.size()) / cyLineHeight;
		final int cLinesPerStint = cLinesAvailable / lstStints.size();
		float flTextSize = 1e30f;
		
		int cyWritePos = rcStints.top;
		for(int x = 0;x < lstStints.size(); x++)
		{
			Stint s = lstStints.get(x);
			for(int y = 0; y < cLinesPerStint-1; y++)
			{
				Rect rcText = new Rect(rcStints.left,cyWritePos,rcStints.right,cyWritePos+cyLineHeight);
				String strValue = s.GetString(x, y, true) + " - " + s.GetString(x, y, false);
				if(strValue != null)
				{
					Utility.DrawFontInBox(canvas, strValue, paintText, rcText, false);
					flTextSize = Math.min(paintText.getTextSize(), flTextSize);
					cyWritePos += cyLineHeight;
				}
				else
				{
					break; // this stint is done
				}
			}
			// after each stint, we want a gap and a line so we don't have a wall of text
			cyWritePos += cyFluffHeight;
			
		}
		// now we've figured out how small our font needs to be.  2nd pass draws things
		cyWritePos = rcStints.top;
		for(int x = 0;x < lstStints.size(); x++)
		{
			Stint s = lstStints.get(x);
			for(int y = 0; y < cLinesPerStint; y++)
			{
				Rect rcText = new Rect(rcStints.left,cyWritePos,rcStints.right,cyWritePos+cyLineHeight);
				String strLeft = s.GetString(x, y, true);
				String strRight = s.GetString(x, y, false);
				if(strLeft != null && strRight != null)
				{
					Utility.DrawFontInBoxFinal(canvas, strLeft, flTextSize, paintText, rcText, true, false);
					Utility.DrawFontInBoxFinal(canvas, strRight, flTextSize, paintText, rcText, false, true);
					flTextSize = Math.min(paintText.getTextSize(), flTextSize);
					cyWritePos += cyLineHeight;
				}
				else
				{
					break; // this stint is done
				}
			}
			// after each stint, we want a gap and a line so we don't have a wall of text
			cyWritePos += cyFluffHeight;
			
			Paint paintLine = new Paint();
			paintLine.setARGB(250,150,150,150);
			canvas.drawLine(rcStints.left, cyWritePos-cyFluffHeight/2, rcStints.right, cyWritePos-cyFluffHeight/2, paintLine);
		}
	}
	private static void DrawGMeter(Canvas canvas, LapAccumulator lap, Rect rc)
	{
		Paint paintLines = new Paint();
		paintLines.setARGB(250,150,150,150);
		paintLines.setStrokeWidth(1);
		paintLines.setStyle(Style.STROKE);
		
		Paint paintDots = new Paint();
		paintDots.setARGB(50, 22, 139, 221);
		
		Paint paintText = new Paint();
		paintText.setARGB(255,0,0,0);
		
		// first: draw the background.  We'll draw a 1g and 2g circle
		Rect rcTitle = new Rect(rc.left,rc.top,rc.right,rc.top+20);
		Utility.DrawFontInBox(canvas, "Traction Circle (best lap)", paintText, rcTitle);
		
		Rect rcChart = new Rect(rc.left,rcTitle.bottom,rc.right,rc.bottom);
		float flRadius2G = Math.min(rcChart.width(), rcChart.height())/2.0f;
		float flRadius1G = flRadius2G/2.0f;
		
		canvas.drawRect(rc, paintLines); // draws the outline of the traction circle
		canvas.drawCircle(rcChart.exactCenterX(), rcChart.exactCenterY(), flRadius2G, paintLines);
		canvas.drawCircle(rcChart.exactCenterX(), rcChart.exactCenterY(), flRadius1G, paintLines);
		canvas.drawLine(rcChart.exactCenterX(), rcChart.exactCenterY()-flRadius2G, rcChart.exactCenterX(), rcChart.exactCenterY()+flRadius2G, paintLines);
		canvas.drawLine(rcChart.exactCenterX()-flRadius2G, rcChart.exactCenterY(), rcChart.exactCenterX()+flRadius2G, rcChart.exactCenterY(), paintLines);
		
		Rect rc1GLabel = new Rect((int)(rcChart.exactCenterX()),(int)(rcChart.exactCenterY()-flRadius1G+3),(int)(rcChart.exactCenterX()+15),(int)(rcChart.exactCenterY()-flRadius1G+10));
		Rect rc2GLabel = new Rect((int)(rcChart.exactCenterX()),(int)(rcChart.exactCenterY()-flRadius2G+3),(int)(rcChart.exactCenterX()+15),(int)(rcChart.exactCenterY()-flRadius2G+10));
		Utility.DrawFontInBox(canvas, "1G",paintLines, rc1GLabel);
		Utility.DrawFontInBox(canvas, "2G",paintLines, rc2GLabel);
		
		DataChannel dataX = lap.GetDataChannel(DataChannel.CHANNEL_ACCEL_X);
		DataChannel dataY = lap.GetDataChannel(DataChannel.CHANNEL_ACCEL_Y);
		if(dataX != null && dataY != null && dataX.GetPointCount() == dataY.GetPointCount())
		{
			// we know from the way these get filled that they'll actually have the same number of points, and each point will correspond to the same point in the other axis
			final int cPoints = dataX.GetPointCount();
			
			// we have both channels
			for(int x = 0;x < cPoints; x++)
			{
				Float ptX = dataX.GetData(x);
				Float ptY = dataY.GetData(x);
				double dG = Math.sqrt(ptX.floatValue()*ptX.floatValue() + ptY.floatValue()*ptY.floatValue());
				if(dG <= 2.0f)
				{
					float flPixelX = ptX.floatValue()*flRadius1G + rcChart.exactCenterX();
					float flPixelY = ptY.floatValue()*flRadius1G + rcChart.exactCenterY();
					canvas.drawCircle(flPixelX, flPixelY, 3, paintDots);
				}
			}
		}
	}
	private static void DrawLaptimeChart(Canvas canvas, LapData[] rgRawLaps, Rect rc)
	{
		if(rgRawLaps.length <= 0) return;
	
		LapData rgSortedLaps[] = new LapData[rgRawLaps.length];
		for(int x = 0; x < rgSortedLaps.length; x++)
		{
			rgSortedLaps[x] = new LapData(rgRawLaps[x]); 
		}
		Arrays.sort(rgSortedLaps, new LapDataOrderSorter());
		
		Paint paintBlock = new Paint();
		paintBlock.setARGB(255, 11, 60, 96);
		
		Paint paintFade = new Paint();
		paintFade.setARGB(255, 150, 150, 150);
		
		Paint paintText = new Paint();
		paintText.setARGB(255,0,0,0);
		
		float flAverage = 0;
		float flSD = 0;
		float flMin = 1e30f;
		for(int x = 0;x < rgSortedLaps.length; x++)
		{
			flAverage += rgSortedLaps[x].flLapTime;
			flMin = Math.min(flMin, rgSortedLaps[x].flLapTime);
		}
		flAverage /= rgSortedLaps.length;
		for(int x = 0;x < rgSortedLaps.length; x++)
		{
			flSD += Math.pow(rgSortedLaps[x].flLapTime - flAverage,2);
		}
		flSD /= rgSortedLaps.length;
		flSD = (float)Math.sqrt(flSD);
		
		// two standard deviations usually contain 95% of the data set.  That should do a good job excluding pits or spin/break laps
		flMin -= 1.0f; // make it so the better of the chart is 1 second below the fastest lap
		float flMax = flAverage + 2*flSD;
		float flSpan = flMax - flMin;
		Rect rcChart = new Rect(rc.left+50,rc.top+20,rc.right,rc.bottom-20);
		Rect rcTitle = new Rect(rc.left,rc.top+5,rc.right,rcChart.top);
		Utility.DrawFontInBox(canvas, "Lap Times", paintText, rcTitle);
		// draw text and guidelines
		{
			final float flSlowest = (int)flMax;
			final float flFastest = (int)(flMin-1.0f);
			final float flMid = (int)((flSlowest + flFastest)/2);
			
			final int cySlowest = (int)(  (1.0f-((flSlowest-flMin)/flSpan))*rcChart.height()) + rcChart.top;
			final int cyFastest = (int)((1.0f-((flFastest-flMin)/flSpan))*rcChart.height()) + rcChart.top;
			final int cyMid = (int)((1.0f-((flMid-flMin)/flSpan))*rcChart.height()) + rcChart.top;
			
			Rect rcText;
			
			rcText = new Rect(rc.left,cySlowest,rcChart.left,cySlowest+10);
			Utility.DrawFontInBox(canvas, Utility.FormatSeconds(flSlowest), paintFade, rcText);
			rcText = new Rect(rc.left,cyFastest-10,rcChart.left,cyFastest);
			Utility.DrawFontInBox(canvas, Utility.FormatSeconds(flFastest), paintFade, rcText);
			
			canvas.drawLine(rcChart.left, cySlowest, rcChart.right, cySlowest, paintFade);
			canvas.drawLine(rcChart.left, cyFastest, rcChart.right, cyFastest, paintFade);
			if(flMid != flSlowest && flMid != flFastest)
			{
				rcText = new Rect(rc.left,cyMid-5,rcChart.left,cyMid+5);
				Utility.DrawFontInBox(canvas, Utility.FormatSeconds(flMid), paintFade, rcText);
				canvas.drawLine(rcChart.left, cyMid, rcChart.right, cyMid, paintFade);
			}
		}
		
		// draw chart
		for(int x = 0;x < rgSortedLaps.length;x++)
		{
			float flStartX = (x*rcChart.width())/rgSortedLaps.length + rcChart.left;
			float flEndX = ((x+1)*rcChart.width())/rgSortedLaps.length + rcChart.left;
			
			final float flLT = rgSortedLaps[x].flLapTime;
			float flPct = (flLT - flMin)/flSpan;
			float flHeight = flPct * rcChart.height();
			Rect rcBlock = new Rect((int)flStartX, (int)(rcChart.bottom-flHeight),(int)flEndX, rcChart.bottom);
			canvas.drawRect(rcBlock, paintBlock);
		}
	}
	private static void DrawTopLaps(Canvas canvas, LapData[] rgLaps, Rect rcChart)
	{
		// columns: lap rank, lap time, time of day
		float flFontSize = 300;
		Paint paint = new Paint();
		
		
		Rect rc = new Rect(rcChart.left,rcChart.top+30,rcChart.right,rcChart.bottom);
		int rgColPos[] = new int[5];
		rgColPos[0] = rc.left;
		rgColPos[1] = rc.left + (int)(rc.width()*0.16f); // right side of lap rank
		rgColPos[2] = rgColPos[1] + (int)(rc.width()*0.28f);
		rgColPos[3] = rgColPos[2] + (int)(rc.width()*0.28f);
		rgColPos[4] = rgColPos[3] + (int)(rc.width()*0.28f);
		
		// draw headers
		String strHeaders[] = {"","Laptime","Time of day","Date"};
		
		
		for(int x = 0; x < 2; x++)
		{
			for(int ixCol = 0; ixCol < strHeaders.length; ixCol++)
			{
				Rect rcHeader = new Rect(rgColPos[ixCol],rcChart.top,rgColPos[ixCol+1],rc.top);
				if(x == 0)
				{
					Utility.DrawFontInBox(canvas, strHeaders[ixCol], paint, rcHeader, false);
					flFontSize = Math.min(paint.getTextSize(), flFontSize);
				}
				else
				{
					Utility.DrawFontInBoxFinal(canvas, strHeaders[ixCol], flFontSize, paint, rcHeader, false, false);
				}
			}
		}
		
		flFontSize = 300;
		int iRowHeight = 15;
		
		for(int ixDraw = 0; ixDraw < 2; ixDraw++) // two-pass: once to measure, once to draw
		{
			flFontSize -= 1.0f;
			for(int ixRow = 0;ixRow < rgLaps.length; ixRow++)
			{
				for(int ixCol = 0; ixCol < rgColPos.length-1; ixCol++)
				{
					String str = null;
					Rect rcBox = null;
					final int ixTop = rc.top+iRowHeight*(ixRow);
					final int ixBottom = rc.top+iRowHeight*(ixRow+1);
					boolean fLeft = false;
					boolean fRight = false;
					switch(ixCol)
					{
					case 0: // lap rank
						rcBox = new Rect(rgColPos[0],ixTop,rgColPos[1],ixBottom);
						str = "#" + (ixRow+1);
						fLeft = true;
						fRight = false;
						break;
					case 1: // lap time
						rcBox = new Rect(rgColPos[1],ixTop,rgColPos[2],ixBottom);
						str = Utility.FormatSeconds(rgLaps[ixRow].flLapTime);
						fLeft = false;
						fRight = false;
						break;
					case 2: // lap time of day
						rcBox = new Rect(rgColPos[2],ixTop,rgColPos[3],ixBottom);
						str = Utility.GetReadableTime(rgLaps[ixRow].lStartTime*1000);
						fLeft = false;
						fRight = true;
						break;
					case 3: // lap date
						rcBox = new Rect(rgColPos[3],ixTop,rgColPos[4],ixBottom);
						str = Utility.GetDateStringFromUnixTime(rgLaps[ixRow].lStartTime*1000);
						fLeft = false;
						fRight = false;
						break;
					}
					if(rc != null && str != null)
					{
						if(ixDraw == 0)
						{
							Utility.DrawFontInBox(canvas, str, paint, rcBox, false);
							flFontSize = Math.min(flFontSize, paint.getTextSize());
						}
						else
						{
							Utility.DrawFontInBoxFinal(canvas, str, flFontSize, paint, rcBox, fLeft, fRight);
						}
					}
				}
			}
		}
	}
	private static Bitmap CreateSummaryImage(Resources res, SQLiteDatabase db, String strRaceName, String strLocation, String strVehicleName, String strChosenImage, boolean fUseImperial, long lRaceId)
	{
		System.gc();
		
		try
		{
			final int cxWidth = 1024;
			final int cyHeight = 768;
			final float flScale = 1.5f;
			Bitmap bmp = Bitmap.createBitmap((int)(cxWidth*flScale), (int)(cyHeight*flScale), Bitmap.Config.ARGB_8888);
			Canvas canvas = new Canvas(bmp);
			canvas.scale(flScale,flScale);
			Paint paint = new Paint();
			
			// fills white
			paint.setARGB(255, 255, 255, 255);
			canvas.drawRect(0, 0, cxWidth, cyHeight, paint);
			
			// draw corner image
			final Rect rcCorner = new Rect(cxWidth-200,0,cxWidth,200);
			DrawCornerImages(res, strChosenImage, canvas, rcCorner);
			final Rect rcLogo = new Rect(0,0,200,200);
			DrawCornerImages(res, null, canvas, rcLogo);
			
			// get DB info
			RaceData rd = RaceDatabase.GetRaceData(db, lRaceId, -1);
			
			// figure out percentile data
			LapData[] rgLaps = RaceDatabase.GetLapDataList(db, lRaceId);
			Arrays.sort(rgLaps, new LapDataLapTimeSorter());
			
			Paint paintLine[] = GetPercentilePaints();
			LapAccumulator lapPercentiles[] = GetPercentileLaps(db, rd.lapParams, 4, rgLaps);
			
			if(lapPercentiles.length < 4) return null;
			for(int x = 0;x < 4; x++) { if(lapPercentiles[x] == null) return null;}
			
			float flLength = lapPercentiles[0].GetCalculatedLength();
			
			// ok, now we've got the LapAccumulators loaded, so let's draw them
	
			// draw header info
			final Rect rcHeader = new Rect(200,0,824,100);
			DrawHeader(lapPercentiles[0], rd, strRaceName, strLocation, strVehicleName, fUseImperial, flLength, canvas, rcHeader);
			
			// Draw map
			Rect rcMap = new Rect(rcLogo.right,rcHeader.bottom,rcCorner.left, cyHeight-210);
			DrawMap(canvas, lapPercentiles, paintLine, rcMap);
			
			// draw speed-distances
			Rect rcSpeedDistance = new Rect(rcMap.left,rcMap.bottom+10,rcMap.right,cyHeight-100);
			DrawSpeedDistance(canvas, lapPercentiles, paintLine, rcSpeedDistance, fUseImperial);
			
			// draw Laptime-vs-lap number chart
			Rect rcLaptimeChart = new Rect(rcSpeedDistance.left,rcSpeedDistance.bottom,rcSpeedDistance.right,cyHeight);
			DrawLaptimeChart(canvas, rgLaps, rcLaptimeChart);
			
			// draw stints
			Rect rcStints = new Rect(0,rcLogo.bottom+20,rcMap.left,cyHeight);
			DrawStints(canvas, rgLaps, rcStints);
			
			// draw g-meter
			Rect rcGMeter = new Rect(rcMap.right, rcCorner.bottom, cxWidth, rcCorner.bottom + (cxWidth-rcMap.right));
			DrawGMeter(canvas, lapPercentiles[0], rcGMeter);
			
			// draw top laps
			Rect rcTopLaps = new Rect(rcGMeter.left, rcGMeter.bottom, rcGMeter.right, cyHeight);
			DrawTopLaps(canvas, rgLaps, rcTopLaps);
			
			return bmp;
		}
		catch(Exception e)
		{
			return null;
		}
	}

	
}
