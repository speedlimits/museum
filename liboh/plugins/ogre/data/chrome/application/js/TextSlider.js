function TextSlider(bg, thumb, textID, min, max, useInts, sendFunc)
{
  this.setRealValue = setRealValue;
  this.addDependentSlider = addDependentSlider;
  this.updateDependentSliders = updateDependentSliders;
  this.useInts = useInts;
  this.sendFunc = sendFunc;
  var textSlider = this;
  
  var onChange = function(val) {
    document.getElementById(textID).value = val;
    sendFunc(val);
    textSlider.updateDependentSliders(val);
  }
  
  this.slider = createHorizontalSlider(bg, thumb, min, max, useInts, onChange);
  
  this.textID = textID;
  this.dependentSliders = new Array();
  
  var textField = document.getElementById(textID);
  textField.slider = this.slider;
  textField.parent = this;
  
  var keydown = function(event) {
    var c = YAHOO.util.Event.getCharCode(event);
    var inc = event.shiftKey ? 10 : 1;
  
    if (c == 38) { // up arrow, increment
      this.value = Math.min(parseInt(this.value, 10) + inc, this.slider.getMax());
      this.parent.sendFunc(this.value);
      this.slider.setRealValue(this.value, false, false, true);
      this.parent.updateDependentSliders(this.value);
    }
    else if (c == 40) { // down arrow, decrement
      this.value = Math.max(this.value - inc, this.slider.getMin());
      this.parent.sendFunc(this.value);
      this.slider.setRealValue(this.value, false, false, true);
      this.parent.updateDependentSliders(this.value);
    }
    else if (c == 13) {
      this.parent.sendFunc(this.value);
      this.slider.setRealValue(this.value, false, false, true);
      this.parent.updateDependentSliders(this.value);
    } 
  };
  
  var blurText = function(event) {
    this.parent.sendFunc(this.value);
    this.slider.setRealValue(this.value, false, false, true);
    this.parent.updateDependentSliders(this.value);
  }
  
  YAHOO.util.Event.on(textField, "keypress", isAlpha);
  YAHOO.util.Event.on(textField, "keydown", keydown);
  YAHOO.util.Event.on(textField, "blur", blurText);
}

function isAlpha(event)
{
  var c = YAHOO.util.Event.getCharCode(event);
  
  // tab, delete, return, escape, home, end, left, right, delete 
  if ((c < 48 || c > 57) && ("8, 9, 13, 27, 35, 36, 37, 39, 46".indexOf(c) == -1)) {
    YAHOO.util.Event.stopEvent(event);
  }
}

function addDependentSlider(slider)
{
  this.dependentSliders[this.dependentSliders.length] = slider;
}

function updateDependentSliders(val)
{
  for (var ndx = 0; ndx < this.dependentSliders.length; ndx++) {
    this.dependentSliders[ndx].setRealValue(val);
  }
}

function setRealValue(val)
{
  var textField = document.getElementById(this.textID);
  
  if (this.useInts)
    textField.value = val;
  else
    textField.value = new Number(val).toFixed(2);
  this.slider.setRealValue(val, true, true, true);
}