function pushbutton(value) {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", `/pushed?name=${value}`, true);
  xhr.send();
}