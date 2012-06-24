var datax;
var datay;

function min(a,b)
{
  return a < b ? a : b;
}
function max(a,b)
{
  return a < b ? b : a;
}

function drawcanvas(lapids)
{
  var xmin = 1e30;
  var ymin = 1e30;
  var xmax = -1e30;
  var ymax = -1e30;

  for(var x = 0; x < lapids.length; x++)
  {
	  xmin = min(xmin,lapArrayDataMinMax[lapids[x]][datax][0]);
	  ymin = min(ymin,lapArrayDataMinMax[lapids[x]][datay][0]);

	  xmax = max(xmax,lapArrayDataMinMax[lapids[x]][datax][1]);
	  ymax = max(ymax,lapArrayDataMinMax[lapids[x]][datay][1]);
  }


  var c=document.getElementById("graph");
  var ctx=c.getContext("2d");
  ctx.fillStyle="#FFFFFF";
  ctx.clearRect(0,0,800,600);

  for(var lap = 0; lap < lapids.length; lap++)
  {
	  var ixX = 1;
	  var ixY = 1;
	  ctx.beginPath();
	  ctx.strokeStyle = lapColors[lap];
	  while(ixX < lapArrayData[lapids[lap]][datax].length && ixY < lapArrayData[lapids[lap]][datay].length)
	  {
		var xValue;
		var yValue;
		if(lapArrayTime[lapids[lap]][datax][ixX] == lapArrayTime[lapids[lap]][datay][ixY])
		{
		  xValue = lapArrayData[lapids[lap]][datax][ixX];
		  yValue = lapArrayData[lapids[lap]][datay][ixY];
		  ixX++;
		  ixY++;
		}
		else if(lapArrayTime[lapids[lap]][datax][ixX] < lapArrayTime[lapids[lap]][datay][ixY])
		{
		  // the next x data was sampled before the next y one.  So we want to use it, and interpolate the y value
		  xValue = lapArrayData[lapids[lap]][datax][ixX];
		  var xTime = lapArrayTime[lapids[lap]][datax][ixX];
		  var yTime = xTime;
		  var yFraction = (yTime - lapArrayTime[lapids[lap]][datay][ixY-1]) / (lapArrayTime[lapids[lap]][datay][ixY] - lapArrayTime[lapids[lap]][ixY-1]);
		  yValue = (1-yFraction)*lapArrayData[lapids[lap]][datay][ixY-1] + (yFraction)*lapArrayData[lapids[lap]][datay][ixY];

		  ixX = ixX + 1;
		}
		else
		{
		  // the next y data was sampled before the next x one.  So we want to use it, and interpolate the y value
		  yValue = lapArrayData[lapids[lap]][datay][ixY];
		  var yTime = lapArrayTime[lapids[lap]][datay][ixY];
		  var xTime = yTime;
		  var xFraction = (xTime - lapArrayTime[lapids[lap]][datax][ixX-1]) / (lapArrayTime[lapids[lap]][datax][ixX] - lapArrayTime[lapids[lap]][datax][ixX-1]);
		  xValue = (1-xFraction)*lapArrayData[lapids[lap]][datax][ixX-1] + (xFraction)*lapArrayData[lapids[lap]][datax][ixX];

		  ixY = ixY + 1;
		}

		// so now we have xValue and yValue.  We also have the various mins and maxes.  So we should be able to draw a graph!
		var xfrac = (xValue-xmin)/(xmax-xmin);
		var yfrac = (yValue-ymin)/(ymax-ymin);
		ctx.lineTo((xfrac*c.width),((1-yfrac)*c.height));
	  }
	  ctx.stroke();
  }
}

var alllaps = [];

function onChangeXAxis()
{
  var combo = document.getElementById("xcombo");
  datax = combo.value;
  drawcanvas(alllaps);
}
function onChangeYAxis()
{
  var combo = document.getElementById("ycombo");
  datay = combo.value; 
  drawcanvas(alllaps);
}
function onChangeLap()
{
	var combo = document.getElementById("lapcombo");
	requestLap(combo.value);
}
function writeAxisCombo(changeFunc, useId, defaultData, lapids)
{
  document.write("<select id='" + useId + "' onchange='" + changeFunc + "'>");
  for(var x = 0; x < strData[lapids[0]].length; x++)
  {
    if(ixData[lapids[0]][x] == defaultData)
    {
      document.write("<option value='" + ixData[lapids[0]][x] + "' selected>");
    }
    else
    {
      document.write("<option value='" + ixData[lapids[0]][x] + "'>");
    }
    document.write(strData[lapids[0]][x] + "</option>");
  }
  document.write("</select>");
}
function writeLapsCombo(changeFunc, useId)
{
	document.write("<select id='" + useId + "' onchange='" + changeFunc + "'>");
	for(var x = 0; x < lapsAvailable.length; x++)
	{
		document.write("<option value='" + lapsAvailable[x] + "'>" + lapTexts[x] + "</option>");
	}
	document.write("</select>");
}

function requestLap(lapid)
{
	function handler()
	{
		//alert('response1!' + this.status + ' readystate ' + this.readyState + ' response ' + this.responseXML);
		// must have received new XMLHttp response
		if(this.readyState == this.DONE) 
		{
			if(this.status == 200 && this.responseText != null) 
			{
			  eval(this.responseText);
			  alllaps.push(lapid);
			  drawcanvas(alllaps);
			  return;
			}
	  }
	}
	var client = new XMLHttpRequest();
	client.onreadystatechange = handler;
	client.open("GET", "lapdata?id=" + lapid);
	client.send();
}

function DoInitialInit()
{
	for(var x = 0; x < alllaps.length; x++)
	{
		if(alllaps[x] != defaultLap)
		{
			lapArrayTime[alllaps[x]] = null;
			lapArrayData[alllaps[x]] = null;
			lapArrayDataMinMax[alllaps[x]] = null;
			lapArrayTimeMinMax[alllaps[x]] = null;
			ixData[alllaps[x]] = null;
			strData[alllaps[x]] = null;
		}
	}
	
	alllaps = new Array();
	alllaps.push(defaultLap);
};

function clearLaps()
{
  DoInitialInit();
  drawcanvas(alllaps);
}

function start()
{
  alllaps.push(defaultLap);

  datax = 2;
  datay = 3;
  document.write("<canvas id='graph' width='800' height='600'></canvas><br/>");

  writeAxisCombo("onChangeXAxis()","xcombo",datax,alllaps);
  writeAxisCombo("onChangeYAxis()","ycombo",datay,alllaps);
  writeLapsCombo("onChangeLap()","lapcombo");
  document.write("<button onclick=\"clearLaps()\">Clear</button>");

  drawcanvas(alllaps);
}