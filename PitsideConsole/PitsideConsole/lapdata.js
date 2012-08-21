// define a point object.  This may seem redundant, but it allows me to add
// prototypes to the point_ object later
var point_ = function (seed) {
  this.time = null;
  this.distance = null;
  this.velocity = null;
  this.latitde = null;
  this.longitude = null;
  this.index = null;
  for (key in seed) {
    this[key] = seed[key];
  }
}
var pointsKeyList = ["time","distance","velocity","latitude","longitude","index"];
//define an arrayOfObjects() constructor as a copy of Array() and add some
// prototypes
var arrayOfObjects = Array;
arrayOfObjects.prototype.sortByObj = function (property) {
  return this.sort(objectSort(property))
}

function objectSort (property) {
    return function (a,b) {
        return (a[property] < b[property]) ? -1 : (a[property] > b[property]) ? 1 : 0;
    }
}

function dataLayout (name, rangeL, rangeU, suffix, pos) {
  this.name = name;
  if (pos === undefined) {
    pos = nextAvailablePosition();
  }
  this.position = pos;
  this.size = 10;
  this.pMin = 0;
  this.pMax = 1;
  this.vMin = rangeL;
  this.vMax = rangeU;
  this.suffix = suffix;
  this.enabled = true;
  this.valueToGraph = function(value) {
    var ratio = ((+this.pMax) - (+this.pMin)) / ((+this.vMax) - (+this.vMin));
    var output = (((+value) - (+this.vMin)) * (+ratio)) + (+this.pMin);
    return output; 
  },
  this.graphToValue = function(value) {
    var ratio = ((+this.vMax) - (+this.vMin)) / ((+this.pMax) - (+this.pMin));
    var output = (((+value) - (+this.pMin)) * (+ratio)) + (+this.vMin);
    return output;
  }
}                               

var prefs = {
  layout: {},
  settings: {},
  graph: {},
  trackMap: {}
}

function populateGraphPosition() {
  var sorted = [];
  var i;
  // objects can't be easily sorted, so making a temp array to sort
  for (i in prefs.layout) {
    sorted.push(prefs.layout[i]);
    sorted[sorted.length-1].index = i;
  }
  sorted.sortByObj("position");
  i = sorted.length;
  var nextP = 0;
  while (i--) { //countdown from last position
    if (prefs.layout[sorted[i].index].enabled) {
      if ((i < sorted.length-1) && (prefs.layout[sorted[i].index].position === prefs.layout[sorted[i+1].index].position) &&
          (prefs.layout[sorted[i+1].index].enabled == true)) {
        prefs.layout[sorted[i].index].pMin = prefs.layout[sorted[i+1].index].pMin;
        prefs.layout[sorted[i].index].pMax = prefs.layout[sorted[i+1].index].pMax;
      } else {
        var size = prefs.layout[sorted[i].index].size;
        prefs.layout[sorted[i].index].pMin = nextP;
        nextP += size;
        prefs.layout[sorted[i].index].pMax = nextP;
      }
    }
  }
}

function nextAvailablePosition () {
  var nextPosition = 0;
  for (var key in prefs.layout) {
    if (prefs.layout.hasOwnProperty(key)) {
      nextPosition = Math.max(nextPosition, prefs.layout[key].position);
    }
  }
  return +nextPosition + 1;
}

prefs.layout.distance = new dataLayout("Distance",13,40);
prefs.layout.velocity = new dataLayout("Velocity",13,40);
prefs.layout["5"] = new dataLayout("Accel X",-2,2);
prefs.layout["6"] = new dataLayout("Accel Y",-2,2);
prefs.graph.strokeWidth = 2;
prefs.graph.dotSize = 5;
prefs.trackMap.strokeWidth = 2;
prefs.trackMap.dotSize = 5;



var lapData = function(lapList) {  
  for (var lap in lapList) {
    //check if we already have this lap
    if (this.master.idx.indexOf(lap) < 0) {
    //get lap via XMLHttpRequest  
      this.getLap(lap);    
    }    
  }
  // prune unused laps from master array
  this.prune(lapList);
}

lapData.prototype.master = {
  "idx": [],
  "points": [],
  "data": [],
  "prefs": []
};

lapData.prototype.waiting = 0;


lapData.prototype.getPage = function (page, callback) {
  this.waiting++;
  console.log("this.waiting ", this.waiting);
  var _this = this;
  var req = new XMLHttpRequest();
  req.onreadystatechange = function () {
    if (req.readyState == 4) {
      _this.waiting--;
      console.log("this.waiting ", _this.waiting);
      if (req.status === 200 || // Normal http
      req.status === 0) { // Chrome w/ --allow-file-access-from-files
        var data = req.responseText;
        callback.call(_this, data, lap);
      } else if (_this.waiting == 0) {
        updateGraph();
        spinner.stop();
      }
    
    }
  };
  req.open('GET', page , true);
  req.send(null);
}

lapData.prototype.getLap = function (lap) {
  if (!this.waiting) {
    spinner.spin(document.getElementById("renderArea"));
  }
//  this.waiting++;
  var _this = this;
  var refLap = lap.split(",")[0];
  this.getPage('getdata?table=points&parentId=' + lap + '&refLap=' + refLap, _this.getLap_cb);
}

lapData.prototype.getLap_cb = function(data) {
  if (data != "") {
    var myLaps = toArrayOfObjects(data);
    for (var key in myLaps) {
      if (myLaps.hasOwnProperty(key)) {
        this.insertToMaster(myLaps[key], key);
      }
    }
  }
  if (this.waiting == 0) {
    updateGraph();
    spinner.stop();
  }
}


lapData.prototype.getData = function (lap, channelid) {
  if (!this.waiting) {
    spinner.spin(document.getElementById("renderArea"));
  }
//  this.waiting ++ ;
  var _this = this;
  this.getPage('getdata?table=data&lapId='+lap+'&dataType='+channelid, _this.getData_cb);  
}

lapData.prototype.getData_cb = function(data) {
  if (data != "") {
    var myData = toArrayOfObjects(data);
    for (var key in myData) {
      if (myData.hasOwnProperty(key)) {
        this.insertToMaster(myData[key], key);
      }
    }
  }
  if (this.waiting == 0) {
    updateGraph();
    spinner.stop();
  }  
}

lapData.prototype.insertToMaster = function(lpArr, lap) {
  //console.log(lpArr);
  console.time("insertToMaster");
  // merges data with the master array
  // is this point or data channel info?  data channel will not have gps info
  var dataType = "gps"; 
  if (isNaN(lpArr[0].latitude)) {
    dataType = "data";
//    console.log("data array detected");
  }
  //check to see if the lap already exists in master
  if (this.master.idx.indexOf(lap) < 0 && dataType === "gps") {
    this.master.idx.push(lap);
    this.master.points.push(lpArr);
    this.master.data.push([]); 
  } else if (dataType === "data") {
    if (this.master.idx.indexOf(lap) < 0) {
      this.master.idx.push(lap);
    }
    var lpIdx = this.master.idx.indexOf(lap); 
    // the data type selection could have changed.  Rather than 
    // compare old and new, we'll just wipe it out and add fresh.
    // TODO: investigate speed issues with this strategy.
    this.master.data.splice(lpIdx, 1, lpArr);
    // TODO: do some extra stuff, like interpolating distance values, etc.
    //sort on time, yo!
    //this.master.points[lpIdx].sortByObj("time");
    // step through and add distances to object without 'em
    var beforeIdx = 0;
    var afterIdx = 0;
    for (var row = 0; row < this.master.data[lpIdx].length; row++) {
      if (isNaN(this.master.data[lpIdx][row].distance)) {
      var myTime = this.master.data[lpIdx][row].time;  
    // get before and after time indexes from points data
        var beforePnt = null;
        var afterPnt = null;
        for (var rowi = beforeIdx; rowi < this.master.points[lpIdx].length; rowi++) {
          if (this.master.points[lpIdx][rowi].time < myTime) {
            beforePnt = this.master.points[lpIdx][rowi];
            beforeIdx = rowi;
            continue;
          }
          if (this.master.points[lpIdx][rowi].time === myTime) {
            beforePnt = this.master.points[lpIdx][rowi];
            afterPnt = this.master.points[lpIdx][rowi];
            beforeIdx = rowi;
            afterIdx = rowi;
            break;
          }
          if (this.master.points[lpIdx][rowi].time > myTime) {
            afterPnt = this.master.points[lpIdx][rowi];
            afterIdx = rowi;
//             console.log("myTime: ", myTime);
//             console.log("before: ", beforePnt.time);
//             console.log("after: ", afterPnt.time);
            break;      
          } 
        }
    // interpolate the distance from the time values
    // our known values are time
    // need to guess the distance
        if (beforePnt === null || afterPnt === null) {
        // data before the first or after the last lap position is useless!
          this.master.data[lpIdx].splice(row,1);
          row--;
          continue;
        }
        var t_delta = afterPnt.time - beforePnt.time;
        if (t_delta === 0) {
          this.master.data[lpIdx][row].distance = beforePnt.distance;
          continue;
        }
        var d_delta = afterPnt.distance - beforePnt.distance;
        var t_segment = this.master.data[lpIdx][row].time - beforePnt.time;
        var t_ratio = t_segment / t_delta;
        var d_segment = d_delta * t_ratio;
        this.master.data[lpIdx][row].distance = beforePnt.distance + d_segment;        
    // have some ice cream!
        // yum!
      }
    }
  }
  console.timeEnd("insertToMaster");
}

lapData.prototype.removeFromMaster = function(lap) {
  var idx = this.master.idx.indexOf(lap);
  if (idx >= 0) {
    this.master.idx.splice(lap,1);
    this.master.points.splice(lap,1);
    this.master.data.splice(lap,1);
  }
}       

lapData.prototype.header = function() {
  var header = [];
  header.push("Distance");
  for (var i=0; i<this.master.idx.length; i++) {
    header.push("Lap " + this.master.idx[i]);
  }
  header.push("X");
  header.push("Y");
  header.push("Index");
  return header;
}
/**
 *  Input: an array of keys - 1st key is the y value
 *  keys x, y and index are implicit and always added 
 *  Output: array of data series
 *  Example: mergeLaps([distance,velocity,5,6])
 */     
lapData.prototype.mergeLaps = function(keyArray) {
  console.time("mergeLaps");
  var outArr = [];
  var numLaps = this.master.points.length;
  var numKey = keyArray.length -1;
//  alert(numLaps);
//  alert(this.master.points[0]);
  var blanks = [[], [null]]; //an array of blanks for use later...  
  for (var i = 1; i < (numLaps * numKey); i++) {
    blanks.push(blanks[i].concat(null));
  }
  for (var lap = 0; lap < numLaps; lap++) {
//    alert(lap);
    
    // Add data from both arrays, then merge later.    
    var myArray = this.master.points; // this will be changed at the end of the loop
    for (var i = 0; i < 2; i++ ) {
      var lapLen = myArray[lap].length;
      for (var row = 1; row < lapLen ; row++){
        var outRow = [];
        if (isNaN(myArray[lap][row][keyArray[0]])) {
          console.log("Debug: invalid first row found -", myArray[lap][row][keyArray[0]]);
          // skip this row if it doesn't have a valid first key
          continue;
        } 
        outRow.push(myArray[lap][row][keyArray[0]]);
        outRow = outRow.concat(blanks[lap*numKey]);
        
        for (var keyi = 1; keyi < keyArray.length; keyi++) {
          var key = keyArray[keyi];
          var value = myArray[lap][row][key];
          if (prefs.layout[key]){
            value = prefs.layout[key].valueToGraph(value);
          }
          if (isNaN(value)) {
            value = null;
          }
          outRow.push(value);      
        }
        outRow = outRow.concat(blanks[(numLaps - lap -1)*numKey ]);
        var endKeys = ["longitude","latitude","index"];
        for (var keyi = 0; keyi < endKeys.length; keyi++) {
          var key = endKeys[keyi];
          var value = myArray[lap][row][key];
          if (isNaN(value)) {
            value = null;
          }
          outRow.push(value);      
        }
        outArr.push(outRow);
  //      alert("outRow\n" + outRow);
      }
      myArray = this.master.data;
    }
  }
  // Do the final sort and put the headers in at the top.
  console.time("sort");
  outArr.sort();
  console.timeEnd("sort");
  // not done yet... we need to merge duplicate rows
  // crap.. that will mess up index column.  need to sot that out first
//   for (var i = 0; i < (outArr.length -1); i++) {
//     if (outArr[i][0] == outArr[i+1][0]) {
// //      alert("duplicate found at " + i);
// 
//     }
//   }
//  alert("outArr\n" + outArr)
  var header = [];
  header.push(keyArray[0]);
  for (var lapi = 0; lapi < numLaps; lapi++) {
    for (var keyi = 1; keyi < keyArray.length; keyi++) {
      var lapName = this.master.idx[lapi];
      header.push(lapName + " " +keyArray[keyi]);
    }    
  }
  header.push("X");
  header.push("Y");
  header.push("idx");
  
  outArr.splice(0,0,header);
  console.timeEnd("mergeLaps");      
  return outArr;
}

lapData.prototype.prune = function(goodLaps) {
  // removes extra laps from master array leaving only the goodLaps
  for (var i = 0; i < this.master.idx.length; i++) {    
    if (goodLaps.indexOf(this.master.idx[i]) < 0) {
      this.master.idx.splice(i,1);
      this.master.points.splice(i,1);
      this.master.data.splice(i,1);
      i--;
    }
  }
}

var toArrayOfObjects = function(data) {
  var lines = data.split("\n");
  var arry = [];
  var outObj = {};
  var idxObj = {};
  for (var idx = 0; idx < lines.length; idx++) {
    var line = lines[idx];
    // Oftentimes there's a blank line at the end. Ignore it.
    if (line.length == 0) {
      continue;
    }
    var row = line.split(",");
    // Get the headers
    if (idx === 0) {
      var colNames = [];
      var numCols = row.length; 
      for (rowIdx = 0; rowIdx < numCols; rowIdx++) {
        colNames.push(String(row[rowIdx]).toLowerCase());
      }
    }
    
    // Special processing for every row except the header.
    if (idx > 0) {
      var rowObj = {};
      for (var rowIdx = 1; rowIdx < numCols; rowIdx++) {
        // Turn "123" into 123.
        row[rowIdx] = parseFloat(row[rowIdx]);
        if (isNaN(row[rowIdx])) {
          row[rowIdx] = null;
        }
        rowObj[colNames[rowIdx]]= row[rowIdx];
      }
      if (!outObj[row[0]]) {
//      console.log("Creating lap ",row[0]);
        outObj[row[0]] = new arrayOfObjects;
        idxObj[row[0]] = 0;
      }
      rowObj.index = idxObj[row[0]]++;    
      outObj[row[0]].push(rowObj);
//      console.log(arry[row[0]]);
    }     
    
  }     
  return outObj;
}



