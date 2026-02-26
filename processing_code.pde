import processing.serial.*;

// Serial object
Serial myPort;

// Data variables
String packet = "";
int iAngle = 0;
int iDistance = 0;

PFont orcFont;

void setup() {
  size (1200, 700);
  smooth();

  // Change COM port if needed
  myPort = new Serial(this, "COM6", 9600);
  myPort.clear();
  
  println("Radar Started...");
}

void draw() {

  // ---------------------------------------------------------
  // READ SERIAL DATA (NEW FIXED METHOD)
  // ---------------------------------------------------------
  while (myPort.available() > 0) {
    packet = myPort.readStringUntil('.');

    if (packet != null) {
      packet = packet.trim();  // remove spaces and newline

      int commaIndex = packet.indexOf(',');
      if (commaIndex > 0) {
        try {
          iAngle = int(packet.substring(0, commaIndex)); 
          iDistance = int(packet.substring(commaIndex + 1));
        } catch (Exception e) {
          println("Bad packet: " + packet);
        }
      }
    }
  }

  // ---------------------------------------------------------
  // RADAR DISPLAY DRAWING
  // ---------------------------------------------------------

  fill(98,245,31);

  // Motion blur background
  noStroke();
  fill(0,4);
  rect(0, 0, width, height-height*0.065);

  fill(98,245,31);

  drawRadar();
  drawLine();
  drawObject();
  drawText();
}

void drawRadar() {
  pushMatrix();
  translate(width/2,height-height*0.074);
  noFill();
  strokeWeight(2);
  stroke(98,245,31);

  arc(0,0,width*0.9,width*0.9,PI,TWO_PI);
  arc(0,0,width*0.7,width*0.7,PI,TWO_PI);
  arc(0,0,width*0.5,width*0.5,PI,TWO_PI);
  arc(0,0,width*0.3,width*0.3,PI,TWO_PI);

  line(-width/2,0,width/2,0);
  line(0,0,(-width/2)*cos(radians(30)),(-width/2)*sin(radians(30)));
  line(0,0,(-width/2)*cos(radians(60)),(-width/2)*sin(radians(60)));
  line(0,0,(-width/2)*cos(radians(90)),(-width/2)*sin(radians(90)));
  line(0,0,(-width/2)*cos(radians(120)),(-width/2)*sin(radians(120)));
  line(0,0,(-width/2)*cos(radians(150)),(-width/2)*sin(radians(150)));

  popMatrix();
}

void drawLine() {
  pushMatrix();
  strokeWeight(9);
  stroke(30,255,60);
  translate(width/2,height-height*0.074);
  line(0,0,(height-height*0.12)*cos(radians(iAngle)),-(height-height*0.12)*sin(radians(iAngle)));
  popMatrix();
}

void drawObject() {
  pushMatrix();
  translate(width/2,height-height*0.074);
  strokeWeight(9);
  stroke(255,10,10);

  float pixDist = iDistance * ((height-height*0.1666) * 0.025);

  if (iDistance < 40) { // draw only within 40cm
    line(pixDist*cos(radians(iAngle)),-pixDist*sin(radians(iAngle)),
         (width-width*0.505)*cos(radians(iAngle)),
         -(width-width*0.505)*sin(radians(iAngle)));
  }

  popMatrix();
}

void drawText() {
  pushMatrix();
  fill(0,0,0);
  noStroke();
  rect(0, height-height*0.0648, width, height);

  fill(98,245,31);
  textSize(40);
  text("Indian Lifehacker", 10, height - 10);

  text("Angle: " + iAngle + "°", width*0.45, height - 10);

  text("Distance: " + iDistance + " cm", width*0.7, height - 10);

  popMatrix();
}
