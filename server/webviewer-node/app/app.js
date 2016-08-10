// config from cmdline
var host = process.argv[2], 
		port = process.argv[3], 
		dataDir = process.argv[4],
		userListFile = process.argv[5];
// incl
var express = require('express'), 
		fs = require('fs'),
		http = require('http'), 
		path = require('path');
auth = function(user, pass) {
	// Slow to read file every time, but easier on admin than restarting
	var userData=fs.readFileSync(userListFile,'utf8');
	var userList=userData.split("\n");
	for(var i=0;i<userList.length;i++) {
		if(userList[i]==user) return true;
	}
	return false;
}
style = function() {
	var s = 
		"<style>a { display: inline-block; position: relative; }" +
		"a img { width: 160px; height: 120px; border: 0; }" +
		"a i { color: #333; position: absolute; bottom: 0; left: 0; font-size: .5em; }" +
		"</style>";
	return s;
}
postsIndex = function(req, res) {
	var st=style();
	var dirs=fs.readdirSync(dataDir).reverse();
	var h="<html>"+st+"<body><div id=posts>";
	dirs.forEach(function(v){ 
		if(!(v.match(/\.tmp/)))
			h+="<a href='data/"+v+"'><i>"+v+"</i><img src='data/"+v+"'></a>";
	});
	h+="</posts><div id=stats>"+dirs.length+"</stats></body>";
	res.send(h);
};
// configure express crust
var app = express();
app.configure(function(){
  app.set('port', process.env.PORT || port);
  app.use(express.favicon());
  app.use(express.logger('dev'));
  app.use(express.bodyParser());
  app.use(express.methodOverride());
	app.use(express.basicAuth(auth));
  app.use(app.router);
  app.use('/data', express.static(dataDir));
  app.use(express.errorHandler());
});
// serve
app.get('/', postsIndex);
http.createServer(app).listen(app.get('port'), function(){
  console.log("web server port = " + app.get('port'));
});

