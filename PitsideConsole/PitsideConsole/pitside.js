// JavaScript Document
var updateGraph = function() {
  if (!g2) {
    drawGraph();
  }
  populateGraphPosition();
  autoFillChannelSettings();
  // first of all, let's get the chosen data channels and stuff...
  var dataChans = getSelectedItem("data");
  var firstCol = "distance";
  dataChans.splice(0,0,firstCol);
  dataChans.splice(1,0,"velocity"); 
  var lineArry = laps.mergeLaps(dataChans);
  var headers = lineArry[0];
  var lineArry = lineArry.slice(1);
  var lapCols = []; 
  var firstRow = [];
  for (var i = 0; i < headers.indexOf('X'); i++) {
    lapCols.push(i);
    firstRow.push(headers[i]); 
  }
  var indexCol = headers.length - 1;
  lapCols.push(indexCol);
  firstRow.push(headers[indexCol]);
  var trackArry = arrColumns(lineArry, [headers.indexOf('X'), headers.indexOf('Y') ]);
  var distArry = arrColumns(lineArry, lapCols); 
  var trackHeader = ['latitude' , 'longitude'];
  // Draw the track graph!
  trk1.drawTrack(laps.master);
  // setup the visibility mask for the data graph.
  var vismask = [];
  for (var i = 0; i < firstRow.length; i++) {
    if (i < firstRow.length -2) {
      vismask.push(1);
    } else {
      vismask.push(0);
    }
  }
  initLegend(); // initialize the legend for the current lap/data
  g2.updateOptions({
                  file: distArry,
                  labels: firstRow,
                  strokeWidth: prefs.graph.strokeWidth,
                  highlightCircleSize: prefs.graph.dotSize,
                  visibility: vismask,
                  highlightCallback: function(e, x, pts, rowi) {
                      var xrange = g2.toDataXCoord(rowi);
                      rowi+=g2.getLeftBoundary_();
                      var series = -1;
                      var dots = [];
//                       console.log(pts);
                      drawLegend(pts);
                      for (var idx = 0; idx < pts.length; idx++) {
                        var name = pts[idx].name.split(" ");
                        if (name[1] === "velocity" && !(isNaN(pts[idx].y))) {
//                          console.log(name[0], " ",name[1], " is what we want!");
                        }
//                         console.log(pts[idx]);
                      }
//                       for (var i = 1; i < (g2.file_[rowi].length - 1); i++) {
//                         if (g2.file_[rowi][i]) {
//                           series = i - 1;
//                           dots[series] = g2.file_[rowi][g2.file_[rowi].length - 1];  
//                         }
//                       }
                      if (g2.file_[rowi][1] !== null) {
                        dots[0] = g2.file_[rowi][g2.file_[rowi].length - 1];
                      } else {
                        for (var i = rowi; i < g2.file_.length; i++) {
                          if (g2.file_[i][1] !== null) {
                            dots[0] = g2.file_[i][g2.file_[i].length - 1];
                            break;
                          } 
                        }
                      }
//                      console.log(dots);
                      trk1.drawDots(dots)
                  }
  });
}
var xline;

var drawGraph = function() {
  trk1 = new TrackMap(trackDiv, laps.master);

  g2 = new Dygraph(div,[[0,0,0],[1,1,1]], { 
      labels: ['nolap','nolap','nolap'], 
//       labelsDiv: "distLabel",
//       labelsDivStyles: {
//         'backgroundColor': 'transparent',
//       },
//       labelsSeparateLines: true,
//      rollPeriod: 4,
      showLabelsOnHighlight: false,
      interpolatePoints: true,
      connectSeparatedPoints: true,
      highlightOnHover: false,
      hideOverlayOnMouseOut: false,
      showRangeSelector: true,
      rangeSelectorHeight: 30,
      rangeSelectorPlotStrokeColor: "white",
      rangeSelectorPlotFillColor: null,
      showVerticalCrosshair: true,
      crosshairColor: "red",
      drawXAxis: false,
      drawYAxis: false,
      interactionModel : {
        'mousedown' : downV4,
        'mousemove' : moveV4,
        'mouseup' : upV4,
//        'dblclick' : dblClickV4,
      },
      axes: {
        x: {
          valueFormatter: function(ms) {
            return 'Distance: ' + ms.toFixed(2) + ' Units';
          }
        },
        y: {
          valueFormatter: function(y, opts, series) {
            var mySeries = series.split(" ");
//             console.log(mySeries);
//             var dataType = mySeries[1];
            var speed = y;
//            console.log("debug: ",prefs.layout[mySeries[1]]);
            if (prefs.layout[mySeries[1]]) {
//              console.log("using prefs: ",mySeries[1]);
              speed = prefs.layout[mySeries[1]].graphToValue(y);
              
            }            
            return ' ' + parseFloat(speed).toFixed(2);
          }
        }  
      }
      
    }
  );
}

function drawLegend (pts) {
  
  var ptlen = pts.length;
  for (var i = 0; i < ptlen; i++) {
    var cell = document.getElementById(pts[i].name);
    if (cell) {
      var name = pts[i].name.split(" ");
      cell.innerHTML = Number(prefs.layout[name[1]].graphToValue(pts[i].yval)).toFixed(2);
    }
  }
}

function initLegend () {
  var table = document.getElementById("legendTable");
  var myLaps = getSelectedItem("lap");
  var myData = getSelectedItem("data");
  myData.splice(0,0,"distance");
  myData.splice(1,0,"velocity");
  table.innerHTML = ""; // nuke the entire table
  var tHead = table.createTHead();
  var myRow = tHead.insertRow(-1);
  myRow.insertCell(-1);
  for (var lap = 0; lap < myLaps.length; lap++) {
    var myCell = myRow.insertCell(-1);
    myCell.innerHTML = "Lap " + myLaps[lap];
  }
  var tBody = document.createElement("tbody");
  table.appendChild(tBody);
  
  for (var chan = 0; chan < myData.length; chan++) {
    var myRow = tBody.insertRow(-1);
    var myCell = myRow.insertCell(-1);
    myCell.innerHTML = prefs.layout[myData[chan]].name;
    for (var lap = 0; lap < myLaps.length; lap++) {
      var myCell = myRow.insertCell(-1);
      myCell.id = myLaps[lap] + " " + myData[chan];            
    }  
  }
  
}

var v4Active = false;
var v4Canvas = null;

function downV4 (event, g, context) {
  context.initializeMouseDown(event, g, context);
  v4Active = true;
  moveV4(event, g, context); // in case the mouse went down on a data point.
}


function moveV4 (event, g, context) {
  if (v4Active) {
    var points = g.layout_.points;
    if (points === undefined) return;
//     var canvasCoords = g.eventToDomCoords(event);
//     var canvasx = canvasCoords[0];
//     var canvasy = canvasCoords[1];
    var canvasx = Dygraph.pageX(event) - Dygraph.findPosX(g.graphDiv);
    
    var selectionChanged = false;
    var idx = g.findClosestRow(canvasx);
    selectionChanged = g.setSelection(idx);
    var callback = g.attr_("highlightCallback");
    if (callback && selectionChanged) {
      callback(event, g.lastx_, g.selPoints_, g.lastRow_, g.highlightSet_);
    } 
  }
}

function upV4(event, g, context) {
  if (v4Active) {
    v4Active = false;
  }
}

function dblClickV4(event, g, context) {
  restorePositioning(g);
}



var arrColumns = function(data, cols) {
  var arry = [];
  var rarry = [];
  for (var row = 0; row < data.length; row++){
    rarry = [];
    for (var i = 0; i < cols.length; i++) {
      rarry.push(data[row][cols[i]]);
    }
    arry.push(rarry);
  }
  return arry;
}

function mySorting(a,b) {
  a = a[0];
  b = b[0];
  return a == b ? 0 : (a < b ? -1 : 1)
}

function sortByY(a,b) {
  a = a[3];
  b = b[3];
  return a == b ? 0 : (a < b ? -1 : 1)
}

var toArray = function(data) {
  var lines = data.split("\n");
  var arry = [];
  for (var idx = 0; idx < lines.length; idx++) {
    var line = lines[idx];
    // Oftentimes there's a blank line at the end. Ignore it.
    if (line.length == 0) {
      continue;
    }
    var row = line.split(",");
    // Special processing for every row except the header.
    if (idx > 0) {
      for (var rowIdx = 0; rowIdx < row.length; rowIdx++) {
        // Turn "123" into 123.
        row[rowIdx] = parseFloat(row[rowIdx]);
        if (isNaN(row[rowIdx])) {
          row[rowIdx] = null;
        }
      }    
    }     
    arry.push(row);
  }     
  return arry;
}

var toArray2 = function(data) {
  var lines = data.split("\n");
  var arry = [];
  for (var idx = 0; idx < lines.length; idx++) {
    var line = lines[idx];
    // Oftentimes there's a blank line at the end. Ignore it.
    if (line.length == 0) {
      continue;
    }
    var row = line.split(",");
    arry.push(row);
  }     
  return arry;
}

function getPage(page, callback){
  var req = new XMLHttpRequest();
  req.onreadystatechange = function () {
    if (req.readyState == 4) {
      if (req.status === 200 || // Normal http
          req.status === 0) { // Chrome w/ --allow-file-access-from-files
        var data = req.responseText;
        callback(data);
      }
    }
  };
  req.open('GET', page , true);
  req.send(null);
}

function popRaces() {
  var req = new XMLHttpRequest();
  req.onreadystatechange = function () {
    if (req.readyState == 4) {
      if (req.status === 200 || // Normal http
          req.status === 0) { // Chrome w/ --allow-file-access-from-files
        var data = req.responseText;
        popRaces_cb(data);
      }
    }
  };
  req.open('GET', 'getdata?table=races', true);
  req.send(null);
}

function popRaces_cb(data) {

  var raceArr = toArray2(data);
  var raceSelect = document.getElementById("race");
  
  //wipe out the existing options
  raceSelect.options.length = 0;
  raceSelect.options[raceSelect.options.length] = new Option("Choose One",-1);
  for (var idx = 1; idx < raceArr.length; idx++) {
    raceSelect.options[raceSelect.options.length] = new Option(raceArr[idx][1]+" - "+raceArr[idx][2]+" - "+raceArr[idx][3]+" laps", raceArr[idx][0]);
  }
  
}



function popLaps() {
  var raceSelect = document.getElementById("race");
  var raceId = raceSelect.options[raceSelect.selectedIndex].value
  var req = new XMLHttpRequest();
  req.onreadystatechange = function () {
    if (req.readyState == 4) {
      if (req.status === 200 || // Normal http
          req.status === 0) { // Chrome w/ --allow-file-access-from-files
        var data = req.responseText;
        popLapsCallback(data);
      }
    }
  };
  req.open('GET', 'getlaps.php?action=getLaps&raceId='+raceId, true);
  req.send(null);
}

function popLapsCallback(data) {
  var lapArr = toArray(data);
  var lapSelect = document.getElementById("lap");
  var fastLap = lapArr[0][0];
  var fastLapIdx = 0
  var lastLap = lapArr[(lapArr.length - 1)][0]; 
  for (var idx = 0; idx < lapArr.length; idx++) {
    if (lapArr[idx][2] < lapArr[fastLapIdx][2]) {
      fastLapIdx = idx;
      fastLap = lapArr[fastLapIdx][0]
      
    }
  }
  //wipe out the existing options
  lapSelect.options.length = 0;
  lapSelect.options[lapSelect.options.length] = new Option("FASTEST LAP", fastLap);
  lapSelect.options[lapSelect.options.length] = new Option("LAST LAP", lastLap);
  lapSelect.options[0].selected = true;
  lapSelect.options[1].selected = true;
  for (var idx = 0; idx < lapArr.length; idx++) {
    lapSelect.options[lapSelect.options.length] = new Option(lapArr[idx][1]+" - "+sec2min(lapArr[idx][2]), lapArr[idx][0]);
  }
  // draw the graph with the fast/last lap defaulted...
  if (!g1) {
    graphUpdate();
//     laps.getLap(lastLap);
//     laps.getLap(fastLap);    
    //drawGraph();
//    updateGraph(laps.mergeLaps());
  }  
}

function popData() {
  // the data channels available are on a lap-by-lap basis.  We don't expect
  // it to change around, so we'll only look at the first selected lap
  var lapSelect = document.getElementById("lap");
  var lapId = lapSelect.options[lapSelect.selectedIndex].value;
  getPage("getdata?table=channels&parentId=" + lapId, popData_cb)  
}

function popData_cb(data){
  var channelArr = toArray2(data);
  channelArr = channelArr.slice(1);
  var dataSelect = document.getElementById("data");
  var chanTable = document.getElementById("channeltable");
  dataSelect.options.length = 0;
  for (var idx = 0; idx < channelArr.length; idx++) {
    dataSelect.options[dataSelect.options.length] = new Option(channelArr[idx][1], channelArr[idx][0]);
    addChannel(chanTable,channelArr[idx][1],channelArr[idx][0]);
  }
  
}

function addChannel(table, chname, chidx) {
  if (!prefs.layout.hasOwnProperty(chidx)) {
    // create entry in prefs if new
    prefs.layout[chidx] = new dataLayout(chname,0,1);
    prefs.layout[chidx].enabled = true;
  }
  if (!prefs.layout[chidx].tRow) {
    // create table row if it doesn't already exist    
    var tRow = table.tBodies[0].insertRow(-1); //adds row at end of table
    prefs.layout[chidx].tRow = tRow;
    tRow.id = chidx;
    var cell = tRow.insertCell(-1);     //adds cell at end of row
    var checkbox = document.createElement("input");
    checkbox.type = "checkbox";
    checkbox.id = chidx + " enabled";
    checkbox.checked = prefs.layout[chidx].enabled;
    checkbox.onchange = settingChanged;
    cell.appendChild(checkbox);
    var cellArr = ["position","size","vMin","vMax","name"];
    for (var i = 0; i < cellArr.length; i++) {
      var key = cellArr[i];
      var cell = tRow.insertCell(-1);
      cell.contentEditable = true;
      cell.onblur = settingChanged;
      cell.id = chidx + " " + key;
      cell.innerHTML = prefs.layout[chidx][key];
    }
  }
}

function settingChanged () {
  var id = this.id.split(" ");
  var chan = id[0];
  var option = id[1];
  if (this.type == "checkbox") {
    var value = this.checked;
  } else if (option == "name") {
    var value = this.innerHTML;
  } else {
  var value = parseFloat(this.innerHTML);
  var oldvalue = prefs.layout[chan][option];
  }
  if (!isNaN(value) || option == "name") {
//    console.log("Changing ",oldvalue," to ",value);  
    prefs.layout[chan][option] = value;
  }
}

function autoFillChannelSettings () {
  //we only loop through the first
  var dataArr = laps.master.data;
  var temp = {};
  for (var x = 0; x < 2; x++) {
    for (var lap = 0; lap < dataArr.length; lap++) {
      var dataLen = dataArr[lap].length;
      for (var i = 0; i < dataLen; i++) {
        for (key in prefs.layout) {
          if (prefs.layout.hasOwnProperty(key)) {
            if (dataArr[lap][i].hasOwnProperty(key) && prefs.layout[key].enabled) {
              if (!temp.hasOwnProperty(key)) {
                temp[key] = {};
                temp[key].vMin = dataArr[lap][i][key];
                temp[key].vMax = dataArr[lap][i][key];
              }
              temp[key].vMin = Math.min(dataArr[lap][i][key], temp[key].vMin);
              temp[key].vMax = Math.max(dataArr[lap][i][key], temp[key].vMax);
              prefs.layout[key].vMin = temp[key].vMin.toFixed(3);
              prefs.layout[key].vMax = temp[key].vMax.toFixed(3);
              if (prefs.layout[key].vMin == prefs.layout[key].vMax) {
                prefs.layout[key].vMax += 1;
              }
            }
          }
        }
      }
    }
    dataArr = laps.master.points;
  }
  for (key in prefs.layout) {
    if (prefs.layout.hasOwnProperty(key)) {
      document.getElementById(key+" vMin").innerHTML = prefs.layout[key].vMin;
      document.getElementById(key+" vMax").innerHTML = prefs.layout[key].vMax;
    }
  } 
}

function graphUpdate() {
  var lapSelect = document.getElementById("lap");
  var len = lapSelect.length ;
  var lapList = [];
  var lapListCSV = "";
  for (var i = 0; i < len; i++) {
    if (lapSelect[i].selected) {
      var thisLap = lapSelect[i].value;
      lapList.push(thisLap);
      lapListCSV = lapListCSV + "," + thisLap;
    }
  }
  laps.prune(lapList);
  for (var i = 0; i < lapList.length; i++){
    if (laps.master.idx.indexOf(lapList[i]) < 0) {
      laps.getLap(lapList[i]);
    }
  }
  popData();  
}

function dataUpdate() {
  var dataChans = getSelectedItem("data");
  var lapList = getSelectedItem("lap");
  var channels = "";
  if (dataChans[0]) {
    channels = dataChans[0];
  }
  for (var i = 1; i< dataChans.length; i++) {
    channels = channels + "," + dataChans[i];
  }
//  alert(channels);
  for (var i = 0; i < lapList.length; i++){
    laps.getData(lapList[i], channels);

  }
}
function graphUpdate_old() {
  var lapSelect = document.getElementById("lap");
//  var refLapSelect = document.getElementById("refLap");
//  var raceSelect = document.getElementById("race");
  // let's get a little fancier and use a single listbox...  
//  var lap = lapSelect.options[lapSelect.selectedIndex].value;
//  var refLap = refLapSelect.options[refLapSelect.selectedIndex].value;
  var len = lapSelect.length ;
  var lap = 0;
  var refLap = 0;
  for (i = 0; i < len && (lap == 0 || refLap == 0); i++) {
    if (lapSelect[i].selected) {
      var lap = lapSelect[i].value;
      for (i++ ; i < len && (lap == 0 || refLap == 0); i++){
        if (lapSelect[i].selected) {
          var refLap = lapSelect[i].value;
        }
      }      
    } 
  }
//  var chosen = lap + "\n" + refLap; 
//  alert(chosen);
  if (!refLap) {
    refLap = lap;
  }
  var race = 0;
  var req = new XMLHttpRequest();
  req.onreadystatechange = function () {
    if (req.readyState == 4) {
      if (req.status === 200 || // Normal http
          req.status === 0) { // Chrome w/ --allow-file-access-from-files
        var data = req.responseText;
        updateGraph(data);
      }
    }
  };
  req.open('GET', 'lapdistance.php?lap='+lap+'&refLap='+refLap+'&race='+race+'&allCols=1', true);
  req.send(null);
}

function getSelectedItem(name) {
  var selection = document.getElementById(name);
  len = selection.length ;
  i = 0 ;
  chosen = [] ;
  
  for (i = 0; i < len; i++) {
    if (selection[i].selected) {
      chosen.push(selection[i].value) ;
    } 
  }  
//  alert(chosen);
  return chosen ;
} 

function sec2min(time) {
  var mins = Math.floor(time / 60);
  var seconds = time % 60;
  return mins + ":" + seconds.toFixed(3);
}

function createCookie(name,value,days) {
	if (days) {
		var date = new Date();
		date.setTime(date.getTime()+(days*24*60*60*1000));
		var expires = "; expires="+date.toGMTString();
	}
	else var expires = "";
	document.cookie = name+"="+value+expires+"; path=/";
}

function readCookie(name) {
	var nameEQ = name + "=";
	var ca = document.cookie.split(';');
	for(var i=0;i < ca.length;i++) {
		var c = ca[i];
		while (c.charAt(0)==' ') c = c.substring(1,c.length);
		if (c.indexOf(nameEQ) == 0) return c.substring(nameEQ.length,c.length);
	}
	return null;
}

function eraseCookie(name) {
	createCookie(name,"",-1);
}

function init() {
  var chanTable = document.getElementById("channeltable");
  
  for (key in prefs.layout) {
    if (prefs.layout.hasOwnProperty(key)) {
      addChannel(chanTable,prefs.layout[key].name,key);
    }
  }
  var opts = {
  lines: 13, // The number of lines to draw
  length: 7, // The length of each line
  width: 4, // The line thickness
  radius: 10, // The radius of the inner circle
  corners: 1, // Corner roundness (0..1)
  rotate: 0, // The rotation offset
  color: '#fff', // #rgb or #rrggbb
  speed: 1, // Rounds per second
  trail: 60, // Afterglow percentage
  shadow: false, // Whether to render a shadow
  hwaccel: false, // Whether to use hardware acceleration
  className: 'spinner', // The CSS class to assign to the spinner
  zIndex: 9000, // The z-index (defaults to 2000000000)
  top: 'auto', // Top position relative to parent in px
  left: 'auto' // Left position relative to parent in px
};
var target = document.getElementById('renderArea');
spinner = new Spinner(opts);

  toggle2('datamenuframe','datamenutab');
  
}
