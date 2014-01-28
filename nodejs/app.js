var http = require('http'),
    dgram = require('dgram'),
    socketio = require('socket.io');

var fs = require('fs');

var app = http.createServer(),
    io = socketio.listen(app),
    socket = dgram.createSocket('udp4');

var sensorlist = new Array();
var sensorvalues = new Array();
var termperatura = 0;

var debug = false;

//Cada paquete que entra tipo 01#T#2212 equivale a : Sensor 01, tipo Temperatura, valor: 22,12ºC
socket.on('message', function (content, rinfo) {
    if(debug){console.log(content.toString()); }

    var res = content.toString().split("#");
    var i = sensorlist.indexOf(res[0] + res[1].toUpperCase());

    if (i < 0) {
        sensorlist.push(res[0] + res[1].toUpperCase());
        sensorvalues.push((res[2] / 100).toFixed(2));
    } else {
        sensorvalues[i] = (res[2] / 100).toFixed(2);
    }
    io.sockets.emit('udp message', content.toString());
});

//Cada 5 segundos ejecutamos la función que sube los datos a emoncms
setInterval(function () {
    actualizaremoncms()
}, 5000);

function actualizaremoncms() {

  //Modificar url si se ejecuta en un sistema local o en una versión de emoncms instalada en servidor propio.
    var url = "http://emoncms.com/input/post.json?json={";
  // Introducir clave API proporcionada por emoncms con propiedad para escribir.
    var apikey = "API";

    if (sensorlist.length > 0) {
            
        var param = "";

        for (i = 0; i < sensorlist.length; i++) {
            var node = sensorlist[i];

            if (i != sensorlist.length && i > 0) {
                param = param + ",";
            }

            param = param + sensorlist[i] + ":" + sensorvalues[i];
        }

        url = url + param + "}&apikey=" + apikey;

        http.get(url, function (res) {
            if(debug){console.log("Got response: " + res.statusCode)};
        }).on('error', function (e) {
            console.log("Got error: " + e.message);
        });

        if(debug){console.log(url)};
    }
}
