import de.bezier.data.sql.*;

MySQL msql;
int sendvalue;
int receivevalue;
int rectsize = 400;

int max = 100000;

void setup(){
  size(350, 500);
  sendvalue=0;
  receivevalue=0;
  
  String name = "AnnaLena";
  String password = "annalena";
  String host = "192.168.42.9";
  String database = "DataFromMPI";
  msql = new MySQL(this, host, database, name, password);
  
  if(!msql.connect()){
    System.out.println("Connection failed!");
  }
}

void draw(){
  background(51);
  
  //Send-Progress-Bar
  noFill();
  rect(10,10,100,rectsize);
  fill(255,255,255);
  text("Send-Data", 10, rectsize+40);
 
  fill(127, 255, 0);
  rect(10, rectsize+10 , 100, sendvalue*(-1));
  sendvalue = getSendValue();
  
  //Receive-Progress-Bar
  noFill();
  rect(200, 10, 100, rectsize);
  fill(255,255,255);
  text("Receive-Data", 200, rectsize+40);
  
  fill(255, 0, 0);
  rect(200, rectsize+10 , 100, receivevalue*(-1));
  receivevalue = getReceiveValue();
  
}

int getSendValue(){
  msql.query("SELECT SUM(datasize) FROM MPI_Data WHERE operation='send'");
  msql.next();
  int value = msql.getInt(1);
  System.out.println("send" + value);
  return value/8*rectsize/max;
}

int getReceiveValue(){
  msql.query("SELECT SUM(datasize) FROM MPI_Data WHERE operation='receive'");
  msql.next();
  int value = msql.getInt(1);
  System.out.println("receive" + value);
  return value/8*rectsize/max;
}
