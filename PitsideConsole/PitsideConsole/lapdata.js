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
};

lapData.prototype.waiting = 0;

lapData.prototype.getLap = function (lap) {
  this.waiting++
  var _this = this;
  var req = new XMLHttpRequest();
  req.onreadystatechange = function () {
    if (req.readyState == 4) {
    if (req.status === 200 || // Normal http
    req.status === 0) { // Chrome w/ --allow-file-access-from-files
      var data = req.responseText;
//      drawGraph(data);
//      glbArr = toArray(data);
        _this.getLap_cb(data, lap);
      }
      if (--_this.waiting == 0) {
        updateGraph(_this.mergeLaps());
      }
    }
    
  };
  req.open('GET', 'getdata?table=points&parentId=' + lap, true);
  req.send(null);
}

lapData.prototype.getLap_cb = function(data, lap) {
  // is valid?
  if (data == "") { return; }
  //convert data to array and add indexes
  var thisLap = toArray(data);
  thisLap[0].push("index");
  for (var row = 1; row < thisLap.length; row++) {
    thisLap[row].push(row-1);
  }
  //merge with master array
  this.insertToMaster(thisLap, lap);
}

lapData.prototype.insertToMaster = function(lpArr, lap) {
  // merges data with the master array
  this.master.idx.push(lap);
  this.master.points.push(lpArr); 
}

lapData.prototype.removeFromMaster = function(lap) {
  var idx = this.master.idx.indexOf(lap);
  if (idx >= 0) {
    this.master.idx.splice(lap,1);
    this.master.points.splice(lap,1)
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

lapData.prototype.mergeLaps = function() {
  // I thought about sorting through every entry and adding them one by one
  // but it's easier to make one big array and then sort it.
  var outArr = [];
  var numLaps = this.master.points.length;
//  alert(numLaps);
//  alert(this.master.points[0]);
  var blanks = [[], [null]]; //an array of blanks for use later...  
  for (var i = 1; i < numLaps; i++) {
    blanks.push(blanks[i].concat(null));
  }
  for (var lap = 0; lap < numLaps; lap++) {
//    alert(lap);
    var lapLen = this.master.points[lap].length;
    for (var row = 1; row < lapLen ; row++){
      var outRow = [];
      var distance = this.master.points[lap][row][DISTANCE];
      var velocity = this.master.points[lap][row][VELOCITY];
      var lat = this.master.points[lap][row][LAT];
      var lon = this.master.points[lap][row][LON];
      var idx = this.master.points[lap][row][IDX];
      
      outRow.push(distance);
      outRow = outRow.concat(blanks[lap]);
      outRow.push(velocity);
      outRow = outRow.concat(blanks[numLaps - lap -1]);
      outRow.push(lon);
      outRow.push(lat);
      outRow.push(idx);
      outArr.push(outRow);
//      alert("outRow\n" + outRow);
    }
  }
  // Do the final sort and put the headers in at the top.
  outArr.sort();
  // not done yet... we need to merge duplicate rows
  // crap.. that will mess up index column.  need to sot that out first
//   for (var i = 0; i < (outArr.length -1); i++) {
//     if (outArr[i][0] == outArr[i+1][0]) {
// //      alert("duplicate found at " + i);
// 
//     }
//   }
//  alert("outArr\n" + outArr)
  outArr.splice(0,0,this.header());      
  return outArr;
}
lapData.prototype.prune = function(goodLaps) {
  // removes extra laps from master array leaving only the goodLaps
  for (var i = 0; i < this.master.idx.length; i++) {    
    if (goodLaps.indexOf(this.master.idx[i]) < 0) {
      this.master.idx.splice(i,1);
      this.master.points.splice(i,1);
      i--;
    }
  }
}