
var connection = new WebSocket('ws://' + location.hostname + ':81/', ['arduino']);

connection.onopen = function () {
  connection.send('Connect ' + new Date());
};

connection.onerror = function (error) {
  console.log('WebSocket Error ', error);
};

connection.onmessage = function (evt) {
  console.log('Server: ', evt.data);
  document.getElementById("back").innerHTML  =  evt.data ;
};

connection.onclose = function () {
  console.log('WebSocket connection closed');
};

function sendValue () {
  var val = document.getElementById('value').value** 2 / 1023;
  var valstr = '#' + val.toString(16); // Transform to Hexa
  console.log('sendValue: ' + valstr);
  connection.send(valstr);
}
