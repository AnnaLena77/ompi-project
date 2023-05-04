package org.openjfx.hellofx;

import java.net.UnknownHostException;

import com.mongodb.DB;
import com.mongodb.DBCollection;
import com.mongodb.MongoClient;
import com.mongodb.MongoClientURI;

public class MongoDB_Connector {
	
	public static void connection() throws UnknownHostException {
		MongoClient mongoClient = new MongoClient(new MongoClientURI("mongodb://10.35.8.10:27017"));
		DB database = mongoClient.getDB("DataFromMPI");
		//boolean auth = database.authenticate("test", "testpwd".toCharArray());
		database.getCollectionNames().forEach(System.out::println);
		//mongoClient.getDatabaseNames().forEach(System.out::println);
		//DBCollection collection = database.getCollection("MPI_Information");
	}

}
