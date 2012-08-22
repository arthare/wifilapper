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
  this.minX = this.coords[0][1].longitude;
  this.minY = this.coords[0][1].latitude;
  this.maxX = this.coords[0][1].longitude;
  this.maxY = this.coords[0][1].latitude;
  //step though each lap
  for (var lapix = 0; lapix < data.idx.length; lapix++) {
//    alert(data.idx[lapix]);
    points[lapix] = [];
    points[lapix].push(this.coords[lapix][1].longitude);
    points[lapix].push(this.coords[lapix][1].latitude);
  // we're doing two things here; getting the min and max values from the GPS
  // data and creating the point array for drawspline
    var len = this.coords[lapix].length;
    for (var row = 2; row < len; row++) {
      var numcols = this.coords[lapix][row].length;
      this.minX = Math.min(this.minX, this.coords[lapix][row].longitude);
      this.minY = Math.min(this.minY, this.coords[lapix][row].latitude);
      this.maxX = Math.max(this.maxX, this.coords[lapix][row].longitude);
      this.maxY = Math.max(this.maxY, this.coords[lapix][row].latitude);
      points[lapix].push(this.coords[lapix][row].longitude);
      points[lapix].push(this.coords[lapix][row].latitude);
    }
  }
  
  // get the midpoint and size of the map
  var meanX = (this.minX + this.maxX) / 2;
  var meanY = (this.minY + this.maxY) / 2;
  this.deltaX = this.maxX - this.minX;
  this.deltaY = this.maxY - this.minY;
  // the biggest scale value that will still fit.. plus a buffer of 5%
  this.scaleX = Math.min((this.width *.95)/this.deltaX , (this.height *.95)/this.deltaY);

  
//   // translate and scale.. this took me WAY too long to figure out
//   var translateY = (this.height/2) - (meanY* this.scaleX);
//   var translateX = (this.width/2) - (meanX * this.scaleX);
//   if(!this.canvas_ctx){return}
  this.canvas_ctx.setTransform(1, 0, 0, 1, 0, 0);
  this.dots_ctx.setTransform(1, 0, 0, 1, 0, 0);
  
  this.canvas_ctx.clearRect(0,0,this.width,this.height);
  this.canvas_ctx.translate(0,this.height);
  this.canvas_ctx.scale(1,-1);
  this.dots_ctx.translate(0,this.height);
  this.dots_ctx.scale(1,-1);
//   this.canvas_ctx.translate(translateX,translateY);
//   this.canvas_ctx.scale(this.scaleX,this.scaleX);
//   // 0,0 is at the top left... which means we need to flip this.  
//   this.canvas_ctx.translate(0,meanY);
//   this.canvas_ctx.scale(1,-1);
//   this.canvas_ctx.translate(0,-meanY);
//   
//   // repeat that last bit for the dots canvas so everything will line up
//   if(!this.dots_ctx){return}
//   this.dots_ctx.clearRect(0,0,this.width,this.height);
//   this.dots_ctx.translate(translateX,translateY);
//   this.dots_ctx.scale(this.scaleX,this.scaleX);
//   // 0,0 is at the top left... which means we need to flip this.  
//   this.dots_ctx.translate(0,meanY);
//   this.dots_ctx.scale(1,-1);
//   this.dots_ctx.translate(0,-meanY);
//   //   Drawing a spline takes one call.  The points are an array [x0,y0,x1,y1,...],
//   //   the tension is t (typically 0.33 to 0.5), and true/false tells whether to
//   //   connect the endpoints of the data to make a closed curve.
//   
   var lnwidth = prefs.trackMap.strokeWidth;
//   var inlnwidth = lnwidth / this.scaleX;
//   var outlnwidth = (lnwidth + 2) / this.scaleX;
  for (var lapix = 0; lapix < points.length; lapix++){
    //drawSpline(this.canvas_ctx,points[lapix],t,false);
//    drawLine(this,this.coords[lapix],(lnwidth + 3), "red");
    drawLine(this,this.coords[lapix],lnwidth, "white");
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
//  this.dots_ctx.clearRect(x,y,(deltaX * 1.5),(deltaY * 1.5));
  this.dots_ctx.clearRect(0,0,this.height,this.width);
  var ctx = this.dots_ctx;
  for (var lap = 0; lap < dotArray.length; lap++) {
//  for (var pnt in dotArray) {
    var dot = dotArray[lap];
//    alert(dot + " " + this.coords[dot][1] + "," + this.coords[dot][2]);
    // save canvas properties first
//    this.dots_ctx.save();
  var lnwidth = prefs.trackMap.dotSize;
  var inlnwidth = lnwidth / this.scaleX;
  
    if (dot) {
      var myPnt = this.pntToMap(this.coords[lap][dot]);
      drawPoint(ctx,myPnt.longitude,myPnt.latitude,lnwidth,"#00FF00");
    }
    
    // restore back to our previously saves state
//    this.dots_ctx.restore();
  }
}

TrackMap.prototype.pntToMap =  function (pnt) {
  var outPt = {};
  var paddingX = ((this.height) - (this.deltaX * this.scaleX)) / 2;
  var paddingY = ((this.width) - (this.deltaY * this.scaleX)) / 2;
  outPt.longitude = ((pnt.longitude - (+this.minX)) * this.scaleX) + paddingX;
  outPt.latitude = ((pnt.latitude - (+this.minY)) * this.scaleX) + paddingY;
  
  
  
  return outPt; 
}

function drawLine(_this,points,width,color) {
  if (!color) {
    color = "black";
  }
  var ctx = _this.canvas_ctx 
  ctx.strokeStyle = color;
  ctx.lineWidth = width;
  ctx.beginPath();
  var myPnt = _this.pntToMap(points[0]);
  ctx.moveTo(myPnt.longitude,myPnt.latitude);
  for (var i = 1; i < points.length; i++) {
    var myPnt = _this.pntToMap(points[i]);
    ctx.lineTo(myPnt.longitude,myPnt.latitude);
  }
  ctx.closePath();
  ctx.stroke();
}
function hexToCanvasColor(hexColor,opacity){
    // Convert #AA77CC to rbga() format for Firefox
    opacity=opacity || "1.0";
    hexColor=hexColor.replace("#","");
    var r=parseInt(hexColor.substring(0,2),16);
    var g=parseInt(hexColor.substring(2,4),16);
    var b=parseInt(hexColor.substring(4,6),16);
    return "rgba("+r+","+g+","+b+","+opacity+")";
}
function drawPoint(ctx,x,y,r,color){
    ctx.save();  
    ctx.beginPath();
    ctx.lineWidth=r;
    ctx.fillStyle=hexToCanvasColor(color,1);
    ctx.arc(x,y,r,0.0,2*Math.PI,false);
    ctx.closePath();
    ctx.stroke();
    ctx.fill();
    ctx.restore();
}

function advanceDot(x) {
  dotPos = dotPos + x;
  if (dotPos >= trk1.coords.length) {
     dotPos = 1;
  }
  trk1.drawDots([dotPos]);
}

