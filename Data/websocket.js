
var connection = new WebSocket('ws://' + location.hostname + ':81/', ['arduino']);
var first = true;

connection.onopen = function () {
  first = true;
  // connection.send('connected'); // Get value
};

connection.onerror = function (error) {
  console.log('WebSocket Error ', error);
};

connection.onmessage = function (evt) {
  console.log('Server: ', evt.data);
  document.getElementById("back").innerHTML  =  evt.data ;
  if (first) document.getElementById("value").value = evt.data;
};

connection.onclose = function () {
  console.log('WebSocket connection closed');
};

function sendValue () {
  first = false;
  var val = document.getElementById('value').value** 2 / 1023;
  var valstr = '#' + val.toString(16); // Transform to Hexa
  console.log('sendValue: ' + valstr);
  connection.send(valstr);
}
