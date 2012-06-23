var datax;
var datay;

function drawcanvas()
{
  var xmin = lapArrayDataMinMax[datax][0];
  var ymin = lapArrayDataMinMax[datay][0];

  var xmax = lapArrayDataMinMax[datax][1];
  var ymax = lapArrayDataMinMax[datay][1];

  var ixX = 1;
  var ixY = 1;

  var c=document.getElementById("graph");
  var ctx=c.getContext("2d");
  ctx.fillStyle="#FFFFFF";
  ctx.clearRect(0,0,800,600);

  ctx.beginPath();
  while(ixX < lapArrayData[datax].length && ixY < lapArrayData[datay].length)
  {
    var xValue;
	var yValue;
	if(lapArrayTime[datax][ixX] == lapArrayTime[datay][ixY])
	{
	  xValue = lapArrayData[datax][ixX];
	  yValue = lapArrayData[datay][ixY];
	  ixX++;
	  ixY++;
	}
    else if(lapArrayTime[datax][ixX] < lapArrayTime[datay][ixY])
    {
	  // the next x data was sampled before the next y one.  So we want to use it, and interpolate the y value
	  xValue = lapArrayData[datax][ixX];
	  var xTime = lapArrayTime[datax][ixX];
	  var yTime = xTime;
	  var yFraction = (yTime - lapArrayTime[datay][ixY-1]) / (lapArrayTime[datay][ixY] - lapArrayTime[ixY-1]);
	  yValue = (1-yFraction)*lapArrayData[datay][ixY-1] + (yFraction)*lapArrayData[datay][ixY];

	  ixX = ixX + 1;
    }
	else
	{
	  // the next y data was sampled before the next x one.  So we want to use it, and interpolate the y value
	  yValue = lapArrayData[datay][ixY];
	  var yTime = lapArrayTime[datay][ixY];
	  var xTime = yTime;
	  var xFraction = (xTime - lapArrayTime[datax][ixX-1]) / (lapArrayTime[datax][ixX] - lapArrayTime[datax][ixX-1]);
	  xValue = (1-xFraction)*lapArrayData[datax][ixX-1] + (xFraction)*lapArrayData[datax][ixX];

	  ixY = ixY + 1;
	}

	// so now we have xValue and yValue.  We also have the various mins and maxes.  So we should be able to draw a graph!
	var xfrac = (xValue-xmin)/(xmax-xmin);
	var yfrac = (yValue-ymin)/(ymax-ymin);
	ctx.lineTo((xfrac*c.width),((1-yfrac)*c.height));
  }
  ctx.stroke();

}

function onChangeXAxis()
{
  var combo = document.getElementById("xcombo");
  datax = combo.value;
  drawcanvas();
}
function onChangeYAxis()
{
  var combo = document.getElementById("ycombo");
  datay = combo.value; 
  drawcanvas();
}

function writeAxisCombo(changeFunc, useId, defaultData)
{
  document.write("<select id='" + useId + "' onchange='" + changeFunc + "'>");
  for(var x = 0; x < strData.length; x++)
  {
    if(ixData[x] == defaultData)
    {
      document.write("<option value='" + ixData[x] + "' selected>");
    }
    else
    {
      document.write("<option value='" + ixData[x] + "'>");
    }
    document.write(strData[x] + "</option>");
  }
  document.write("</select>");
}

function start()
{
  datax = 2;
  datay = 3;
  document.write("<canvas id='graph' width='800' height='600'></canvas><br/>");

  writeAxisCombo("onChangeXAxis()","xcombo",datax);
  writeAxisCombo("onChangeYAxis()","ycombo",datay);

  drawcanvas();
}