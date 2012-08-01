/** 
 *  trackmap.js
 *  Copyright 2012 - Jason Willis (jawillis72@gmail.com)
 *
 *  I have not decided on a license yet.
 *  Currently this code is only permitted in the context of
 *  the WifiLapper project (https://github.com/arthare/wifilapper)
 *
 *  Makes use of drawspline.js, which is under a different license
 *
 */         

// autoloader borrowed from dygraph-dev.js
(function() {
  var src=document.getElementsByTagName('script');
  var script = src[src.length-1].getAttribute("src");
  var source_files = [
    "drawspline.js",
  ];
  for (var i = 0; i < source_files.length; i++) {
    document.write('<script type="text/javascript" src="' + script.replace('trackmap.js', source_files[i]) + '"></script>\n');
  }
})();

var TrackMap = function(div, data) {
// "data" format:
// [[?, x, y],[?, x, y],...]
//
  this.minX = 0;
  this.maxX = 0;
  this.minY = 0;
  this.maxY = 0;
  this.width = div.clientWidth;
  this.height = div.clientHeight;
  this.canvas = document.createElement("canvas");
  this.dots = document.createElement("canvas");
  div.appendChild(this.canvas);
  div.appendChild(this.dots);
  this.canvas.style.position = "absolute";
  this.canvas.width = this.width;
  this.canvas.height = this.height;
  this.canvas_ctx = this.canvas.getContext("2d");
  this.dots.style.position = "absolute";
  this.dots.width = this.width;
  this.dots.height = this.height;
  this.dots_ctx = this.dots.getContext("2d");
  this.drawTrack(data);
}

TrackMap.prototype.drawTrack = function(data) {
//  alert(data);
  var points = Array();
  var t = 0;
  this.coords = data.points;
  // initialize the min and max values with the first row of data
  this.minX = this.coords[0][1][1];
  this.minY = this.coords[0][1][2];
  this.maxX = this.coords[0][1][1];
  this.maxY = this.coords[0][1][2];
  //step though each lap
  for (var lapix = 0; lapix < data.idx.length; lapix++) {
//    alert(data.idx[lapix]);
    points[lapix] = [];
    points[lapix].push(this.coords[lapix][1][1]);
    points[lapix].push(this.coords[lapix][1][2]);
  // we're doing two things here; getting the min and max values from the GPS
  // data and creating the point array for drawspline
    var len = this.coords[lapix].length;
    for (var row = 2; row < len; row++) {
      var numcols = this.coords[lapix][row].length;
      this.minX = Math.min(this.minX, this.coords[lapix][row][1]);
      this.minY = Math.min(this.minY, this.coords[lapix][row][2]);
      this.maxX = Math.max(this.maxX, this.coords[lapix][row][1]);
      this.maxY = Math.max(this.maxY, this.coords[lapix][row][2]);
      points[lapix].push(this.coords[lapix][row][1]);
      points[lapix].push(this.coords[lapix][row][2]);
    }
  }
  
  // get the midpoint and size of the map
  var meanX = (this.minX + this.maxX) / 2;
  var meanY = (this.minY + this.maxY) / 2;
  var deltaX = this.maxX - this.minX;
  var deltaY = this.maxY - this.minY;
  // the biggest scale value that will still fit.. plus a buffer of 5%
  var scaleX = Math.min((this.width *.95)/deltaX , (this.height *.95)/deltaY);
  // translate and scale.. this took me WAY too long to figure out
  var translateY = (this.height/2) - (meanY* scaleX);
  var translateX = (this.width/2) - (meanX * scaleX);
  if(!this.canvas_ctx){return}
  this.canvas_ctx.setTransform(1, 0, 0, 1, 0, 0);
  this.dots_ctx.setTransform(1, 0, 0, 1, 0, 0);
  
  this.canvas_ctx.clearRect(0,0,this.width,this.height);
  this.canvas_ctx.translate(translateX,translateY);
  this.canvas_ctx.scale(scaleX,scaleX);
  // 0,0 is at the top left... which means we need to flip this.  
  this.canvas_ctx.translate(0,meanY);
  this.canvas_ctx.scale(1,-1);
  this.canvas_ctx.translate(0,-meanY);
  
  // repeat that last bit for the dots canvas so everything will line up
  if(!this.dots_ctx){return}
  this.dots_ctx.clearRect(0,0,this.width,this.height);
  this.dots_ctx.translate(translateX,translateY);
  this.dots_ctx.scale(scaleX,scaleX);
  // 0,0 is at the top left... which means we need to flip this.  
  this.dots_ctx.translate(0,meanY);
  this.dots_ctx.scale(1,-1);
  this.dots_ctx.translate(0,-meanY);
  //   Drawing a spline takes one call.  The points are an array [x0,y0,x1,y1,...],
  //   the tension is t (typically 0.33 to 0.5), and true/false tells whether to
  //   connect the endpoints of the data to make a closed curve.
  
  var lnwidth = 1.5;
  var inlnwidth = lnwidth / scaleX;
  var outlnwidth = (lnwidth + 2) / scaleX;
  for (var lapix = 0; lapix < points.length; lapix++){
    //drawSpline(this.canvas_ctx,points[lapix],t,false);
    drawLine(this.canvas_ctx,this.coords[lapix],outlnwidth, "black");
    drawLine(this.canvas_ctx,this.coords[lapix],inlnwidth, "red");
//    alert(points[lapix]);
  }
  
//  drawSpline(ctx,points3,t,true);
//  drawSpline(ctx,points4,t,true);
//  this.drawDots([dotPos]);
  
}

TrackMap.prototype.drawDots = function(dotArray) {
  // draws dots on the canvas
  // dotArray is of the format:
  //  [[x,y],[x,y],[x,y]]
  //
  //
//  alert(dotArray);
  var deltaX = this.maxX - this.minX;
  var deltaY = this.maxY - this.minY;
  var x = this.minX - (deltaX * .1);
  var y = this.minY - (deltaY * .1);
  this.dots_ctx.clearRect(x,y,(deltaX * 1.5),(deltaY * 1.5));
  var ctx = this.dots_ctx;
  for (var lap = 0; lap < dotArray.length; lap++) {
//  for (var pnt in dotArray) {
    var dot = dotArray[lap];
//    alert(dot + " " + this.coords[dot][1] + "," + this.coords[dot][2]);
    // save canvas properties first
//    this.dots_ctx.save();
    if (dot) {
    
      drawPoint(ctx,this.coords[lap][dot][1],this.coords[lap][dot][2],0.0001,"#FF0000");
    }
    
    // restore back to our previously saves state
//    this.dots_ctx.restore();
  }
}

function drawLine(ctx,points,width,color) {
  if (!color) {
    color = "black";
  } 
  ctx.strokeStyle = color;
  ctx.lineWidth = width;
  ctx.beginPath();
  ctx.moveTo(points[0][1],points[0][2]);
  for (var i = 1; i < points.length; i++) {
    ctx.lineTo(points[i][1],points[i][2]);
  }
  ctx.closePath();
  ctx.stroke();
}

function advanceDot(x) {
  dotPos = dotPos + x;
  if (dotPos >= trk1.coords.length) {
     dotPos = 1;
  }
  trk1.drawDots([dotPos]);
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
//      row[0] = new Date(row[0]); // Turn the string date into a Date.
      for (var rowIdx = 0; rowIdx < row.length; rowIdx++) {
        // Turn "123" into 123.
        row[rowIdx] = parseFloat(row[rowIdx]);
        if (isNaN(row[rowIdx])) {
//          alert("NaN detected at "+idx+" "+rowIdx);
          row[rowIdx] = null;
        }
      }    
    }     
    arry.push(row);
  }     
  return arry;
}
