
/**
 * Used to focus the current Navi.
 */
function focusNavi()
{
  pos = $('meruHeader').getPosition();
  $ND('focus').add({x: pos.x, y: pos.y}).send();
}

/**
 * Call when the header is selected. Allows moving the Navi.
 */ 
function selectHeader()
{
  $ND('selectHeader').send();
}

/**
 * Call when the header is deselected. Stops moving of the Navi.
 */
function releaseHeader()
{
  $ND('releaseHeader').send();
}

/**
 * Closes the navi
 */
function closeWindow()
{
  $ND('close').send();
}

function setMoveable(moveable)
{
  $ND('setMoveable', {moveable: moveable}).send();
}

function setOpacity(opacity)
{
  $ND('setOpacity', {opacity: opacity}).send();
}

function setVisible(visible)
{
  $ND("setVisible").add({visible: visible}).send();
}

/**
 * Parse key commands taken from YUI
 */
var getKeyCommand = function(e) {
  var c = Event.getCharCode(e);

  // special keys
  if (c === 38) { // up arrow
    return 3;
  }
  else if (c === 13) { // return
    return 6;
  }
  else if (c === 40) { // down array
    return 4;
  }
  else if (c >= 48 && c<=57) { // 0-9
    return 1;
  }
  else if (c >= 97 && c<=102) { // a-f
    return 2;
  }
  else if (c >= 65 && c<=70) { // A-F
    return 2;
  //} else if ("8, 9, 13, 27, 37, 39".indexOf(c) > -1 || 
  //              (c >= 112 && c <=123)) { // including F-keys
  // tab, delete, return, escape, left, right
  }
  else if ("8, 9, 13, 27, 37, 39".indexOf(c) > -1) { // special chars
    return 5;
  }
  else { // something we probably don't want
    return 0;
  }
};

function createVerticalSlider(bg, thumb) {
  return YAHOO.widget.Slider.getVertSlider(bg, thumb, 0, 200);
}

function createHorizontalSlider(bg, thumb, min, max, useInts, onChange)
{
  var slider = YAHOO.widget.Slider.getHorizSlider(bg, thumb, 0, 200);
 
  slider.max = max;
  slider.min = min;
  slider.range = max - min;
  slider.factor = slider.range / 200;
  
  slider.setRange = function(min, max) {
    slider.max = max;
    slider.min = min;
    slider.range = max - min;
    slider.factor = slider.range / 200;
  }
  slider.getMax = function() {
    return slider.max;
  }
  slider.getMin = function() {
    return slider.min;
  }

  if (useInts) {
    slider.getRealValue = function() {
      return Math.round(this.getValue() * this.factor + this.min);
    }
    slider.setRealValue = function(val, anim, force, silent) {
      this.setValue(Math.round(val / this.factor), anim, force, silent);
    }
    if (onChange) {
      slider.subscribe("change", function() {
        onChange(Math.round(this.getValue() * this.factor + this.min));
      });
    }
  }
  else {
    slider.getRealValue = function() {
      return (this.getValue() * this.factor + this.min).toFixed(2);
    }
    slider.setRealValue = function(val, anim, force, silent) {
      this.setValue(val / this.factor, anim, force, silent);
    }
    if (onChange) {
      slider.subscribe("change", function() {
        onChange((this.getValue() * this.factor + this.min).toFixed(2));
      });
    }
  }

  return slider;
}

function createColorPicker(divID, showHSVControls, showHEXControls)
{
  return new YAHOO.widget.ColorPicker(divID, {
    showhsvcontrols: showHSVControls,
    showhexcontrols: showHEXControls,
    images: {
      PICKER_THUMB: "images/picker_thumb.png",
      HUE_THUMB: "images/hue_thumb.png"
    }
  });
}

function createTabView(divID)
{
  return new YAHOO.widget.TabView(divID);
}