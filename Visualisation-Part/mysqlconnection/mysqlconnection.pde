import de.bezier.data.sql.*;

MySQL msql;
int sendvalue;
int receivevalue;
int rectsize = 400;

void setup(){
  size(800, 500);
  sendvalue=0;
  receivevalue=0;
  
  String name = "root";
  String password = "";
  String host = "127.0.0.1";
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
  text("Send-Data", 10, 10);
 
  fill(127, 255, 0);
  rect(10, rectsize+10 , 100, sendvalue*(-1));
  sendvalue = getSendValue();
  
  //Receive-Progress-Bar
  noFill();
  rect(200, 10, 100, rectsize);
  text("Receive-Data", 200, 10);
  
  fill(255, 0, 0);
  rect(200, rectsize+10 , 100, receivevalue*(-1));
  receivevalue = getReceiveValue();
  
}

int getSendValue(){
  msql.query("SELECT SUM(value) FROM MPI_Data WHERE type='send'");
  msql.next();
  int value = msql.getInt(1);
  return value/8*rectsize/10000;
}

int getReceiveValue(){
  msql.query("SELECT SUM(value) FROM MPI_Data WHERE type='receive'");
  msql.next();
  int value = msql.getInt(1);
  return value/8*rectsize/10000;
}
