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
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.List;
import java.util.Vector;

import com.artsoft.wifilapper.LandingRaceBase.RenameDialog;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnDismissListener;
import android.database.sqlite.SQLiteDatabase;
import android.os.Bundle;
import android.view.ContextMenu;
import android.view.KeyEvent;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnKeyListener;
import android.view.inputmethod.EditorInfo;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.AdapterView.AdapterContextMenuInfo;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.TextView.OnEditorActionListener;

public class LandingDBManage extends Activity implements OnClickListener, OnEditorActionListener, OnKeyListener, OnItemClickListener, OnDismissListener, android.content.DialogInterface.OnClickListener
{
	private String m_strTargetFilename;
	
	@Override
	public void onCreate(Bundle bun)
	{
		super.onCreate(bun);
	}
	
	@Override
	public void onResume()
	{
		super.onResume();
		setContentView(R.layout.landingdbmanager);
		FirstTimeSetup();
		OnChange(); // UI -> member variables
		PopulateUI(); // member variables -> UI
	}
	
	private boolean IsSdPresent()
	{
		return android.os.Environment.getExternalStorageState().equals(android.os.Environment.MEDIA_MOUNTED);
	}
	
	private static class DBPathEntry
	{
		private File m_file;
		DBPathEntry(File f)
		{
			m_file = f;
		}
		public String toString()
		{
			String strName = m_file.getName();
			int ixWFLP = strName.lastIndexOf(".wflp");
			
			return strName.substring(0,ixWFLP);
		}
		public File GetFile()
		{
			return m_file;
		}
	}
	
	private void FirstTimeSetup()
	{
		{ // db export side
			EditText edtFilename = (EditText)findViewById(R.id.edtFilename);
			Button btnSave = (Button)findViewById(R.id.btnSaveDB);
			edtFilename.setOnEditorActionListener(this);
			edtFilename.setOnKeyListener(this);
			btnSave.setOnClickListener(this);
		}
		
		{ // db import side
			ListView list = (ListView)findViewById(android.R.id.list);
			
			// search for all the files in our parent path
			File fDir = new File(MakeParentPath());
			if(fDir.exists())
			{
				File children[] = fDir.listFiles();
				ArrayAdapter<DBPathEntry> adapter = new ArrayAdapter<DBPathEntry>(this, R.layout.simplelistitem_defaultcolor);
				for(int x = 0;x < children.length; x++)
				{
					String strName = children[x].getName();
					int ixWFLP = strName.lastIndexOf(".wflp");
					
					if(ixWFLP >= 0)
					{
						adapter.add(new DBPathEntry(children[x]));
					}
				}
				
				list.setAdapter(adapter);
				list.setOnItemClickListener(this);
				registerForContextMenu(list);
			}
			else
			{
				list.setEnabled(false);
			}
		}
	}
	private void OnChange()
	{
		EditText edtFilename = (EditText)findViewById(R.id.edtFilename);
		
		m_strTargetFilename = edtFilename.getText().toString();
	}
	private String MakeParentPath()
	{
		return "/sdcard/wifilapper/";
	}
	private String MakeFilePath(String strFilename)
	{
		return MakeParentPath() + strFilename + ".wflp";
	}
	private void PopulateUI()
	{
		{ // db export side
			EditText edtFilename = (EditText)findViewById(R.id.edtFilename);
			TextView txtDestPath = (TextView)findViewById(R.id.txtSavePath);
			Button btnSave = (Button)findViewById(R.id.btnSaveDB);
			if(IsSdPresent())
			{
				// let's do the normal things
				edtFilename.setEnabled(true);
				btnSave.setEnabled(true);
				
				txtDestPath.setText("Your file will be saved at " + MakeFilePath(m_strTargetFilename));
			}
			else
			{
				// disable everything and put up a toast
				edtFilename.setEnabled(false);
				btnSave.setEnabled(false);
				txtDestPath.setText("SD Card missing");
				Toast.makeText(this, "Cannot import/export DB.  SD Card is missing", Toast.LENGTH_LONG).show();
			}
		}
	}

	@Override
	public void onClick(View v) 
	{
		if(v.getId() == R.id.btnSaveDB)
		{
			// they've hit apply.
			OnChange(); // make sure we've got everything.
			
			File fDirectory = new File(MakeParentPath());
			if(!fDirectory.exists() && !fDirectory.mkdir())
			{
				Toast.makeText(this, "Couldn't create directory.  Not saved", Toast.LENGTH_LONG).show();
			}
			else
			{
				File f = new File(MakeFilePath(m_strTargetFilename));
				if(f.exists())
				{
					Toast.makeText(this, "File already exists.  Not saved", Toast.LENGTH_LONG).show();
				}
				else
				{
					SQLiteDatabase db = RaceDatabase.Get();
					db.beginTransaction();
					
					String strDBPath = db.getPath();
					try
					{
						if(CopyFile(new File(strDBPath),f))
						{
							Toast.makeText(this, "DB Saved", Toast.LENGTH_LONG).show();
						}
						else
						{
							Toast.makeText(this, "DB failed to save", Toast.LENGTH_LONG).show();
						}
					}
					catch(IOException e)
					{
						Toast.makeText(this, "Failed to copy DB: " + e.toString(), Toast.LENGTH_LONG).show();
					}
					catch(Exception e)
					{
						Toast.makeText(this, "Unexpected failure: " + e.toString(), Toast.LENGTH_LONG).show();
					}
					
					db.endTransaction();
				}
			}
			
		}
	}

	@Override
	public boolean onEditorAction(TextView arg0, int arg1, KeyEvent arg2) 
	{
		if(arg0.getId() == R.id.edtFilename)
		{
			OnChange();
			PopulateUI();
			return true;
		}
		return false;
	}

	@Override
	public boolean onKey(View arg0, int arg1, KeyEvent arg2) 
	{
		if(arg0.getId() == R.id.edtFilename)
		{
			OnChange();
			PopulateUI();
		}
		return false;
	}

	private static boolean CopyFile(File src, File dst) throws IOException
	{
		InputStream in = new FileInputStream(src);
		FileOutputStream out = new FileOutputStream(dst);
		
		byte rgBuf[] = new byte[65536];
		while(true)
		{
			int cbRead = in.read(rgBuf,0,rgBuf.length);
			out.write(rgBuf,0,cbRead);
			
			if(cbRead < rgBuf.length)
			{
				// end of file
				out.flush();
				out.close();
				in.close();
				return true;
			}
		}
	}

	public void onCreateContextMenu (ContextMenu menu, View v, ContextMenu.ContextMenuInfo menuInfo)
	{
		if(v == findViewById(android.R.id.list))
		{
			// ok, they've contextmenu'd on the race selection list.  We want to show the "delete/rename" menu
	    	MenuInflater inflater = getMenuInflater();
	    	inflater.inflate(R.menu.deleterename, menu);
		}
	}
	@Override
	public boolean onContextItemSelected(MenuItem item)
	{
		if(item.getItemId() == R.id.mnuDelete)
		{
			// they have requested that we delete the selected race
			AdapterContextMenuInfo info = (AdapterContextMenuInfo)item.getMenuInfo();
			ListView list = (ListView)info.targetView.getParent();
			DBPathEntry db = (DBPathEntry)list.getItemAtPosition(info.position);

			try
			{
				if(db.GetFile().delete())
				{
					Toast.makeText(this, "Database deleted", Toast.LENGTH_LONG).show();
				}
				else
				{
					Toast.makeText(this, "Failed to delete DB", Toast.LENGTH_LONG).show();
				}
			}
			catch(Exception e)
			{
				Toast.makeText(this, "Failed to delete file " + e.toString(), Toast.LENGTH_LONG).show();
			}
			FirstTimeSetup(); // refresh the list
			return true;
		}
		else if(item.getItemId() == R.id.mnuRename)
		{
			AdapterContextMenuInfo info = (AdapterContextMenuInfo)item.getMenuInfo();
			ListView list = (ListView)info.targetView.getParent();
			DBPathEntry db = (DBPathEntry)list.getItemAtPosition(info.position);
			
			// they have requested that we rename the selected race
			Dialog d = new RenameDialog<DBPathEntry>(this, "Set the new DB name", db, R.id.edtRename);
			d.setOnDismissListener(this);
			d.show();
			
			return true;
		}
		return false;
	}
	
	File m_copyThis=null;
	AlertDialog m_dbWarning=null;
	
	@Override
	public void onItemClick(AdapterView<?> list, View view, int position, long id) 
	{
		if(list.getId() == android.R.id.list)
		{
			synchronized(RaceDatabase.class)
			{
				// they've selected something from our list
				DBPathEntry db = (DBPathEntry)list.getItemAtPosition(position);
				// the filename they want is GetParentPath + cs + ".wflp"
				String strFilename = MakeParentPath() + db.toString() + ".wflp";
				File fPicked = new File(strFilename);
				if(fPicked.exists())
				{
					AlertDialog ad = new AlertDialog.Builder(this).create();
	    			ad.setMessage("You are about to overwrite all your recorded race sessions.  Data will be lost.  Make sure to back them up using the save button (right side) first.");
	    			ad.setButton(AlertDialog.BUTTON_POSITIVE,"Ok", this);
	    			ad.setButton(AlertDialog.BUTTON_NEGATIVE,"Cancel", this);
					m_copyThis = fPicked;
					m_dbWarning = ad;
	    			ad.show();
				}
				else
				{
					Toast.makeText(this, "Could not find file " + strFilename, Toast.LENGTH_LONG).show();
				}
			}
		}
	}

	@Override
	public void onDismiss(DialogInterface arg0) 
	{
		if(arg0.getClass().equals(RenameDialog.class))
		{
			RenameDialog<DBPathEntry> rd = (RenameDialog<DBPathEntry>)arg0;
			if(rd.GetResultText().length() > 0)
			{
				File fNewPath = new File(MakeFilePath(rd.GetResultText()));
				if(!fNewPath.exists())
				{
					File fSrc = rd.GetData().GetFile();
					if(fSrc.renameTo(fNewPath))
					{
						Toast.makeText(this, "DB renamed", Toast.LENGTH_LONG).show();
					}
					else
					{
						Toast.makeText(this, "Failed to rename DB", Toast.LENGTH_LONG).show();
					}
				}
				else
				{
					Toast.makeText(this, "There is already a DB at " + fNewPath.getPath(), Toast.LENGTH_LONG).show();
				}
			}
			FirstTimeSetup(); // refresh the list
		}
	}

	@Override
	public void onClick(DialogInterface arg0, int choice) 
	{
		// only one alert dialog for this sucker at this point
		if(arg0 == m_dbWarning)
		{
			if(choice == AlertDialog.BUTTON_POSITIVE)
			{
				try
				{
					String strDBPath = RaceDatabase.Get().getPath();
					RaceDatabase.Get().close();
					if(CopyFile(m_copyThis, new File(strDBPath)))
					{
						RaceDatabase.CreateOnPath(getApplicationContext(), strDBPath);
						Toast.makeText(this, "Successfully imported database", Toast.LENGTH_LONG).show();
					}
				}
				catch(IOException e)
				{
					Toast.makeText(this, "Failed to import database: " + e.toString(), Toast.LENGTH_LONG).show();
				}
			}
			m_dbWarning = null;
		}
	}
}
