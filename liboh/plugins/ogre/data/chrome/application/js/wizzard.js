var wizzard_current_message = "";
var wizzard_current_mode = "&nbsp;";
var wizzard_qued_message = "";
var wizzard_qued_mode = "&nbsp;";
var wizzard_is_showing = false;
function wizzard_click() {if(wizzard_is_showing == false) wizzard_nextMessage()}
function wizzard_nextMessage() {
	wizzard_setMessage(wizzard_qued_message);
	wizzard_qued_message = "I have nothing to say <a href='javascript:wizzard_hideMessage()'> Ok </a>"; 
	wizzard_qued_mode = "&nbsp;";
	wizzard_showMessage();}
function wizzard_repeatMessage() {wizzard_queMessage(wizzard_current_message, wizzard_current_mode);}
function wizzard_queMessage(message, mode) {wizzard_qued_message = message; wizzard_qued_mode = mode;}
function wizzard_setMode(mode) {
	if(mode == "") { mode = "&nbsp;"; }
	document.getElementById('wizzard-mode').innerHTML = mode;
	wizzard_current_mode = mode;}
function wizzard_setMessage(message) {
	document.getElementById('wizzard-text').innerHTML = message;
	wizzard_current_message = message;
	}
function wizzard_showMessage() {$('#wizzard-text').fadeIn('slow'); wizzard_is_showing = true;}
function wizzard_hideMessage() {
	$('#wizzard-text').fadeOut('slow'); 
	wizzard_is_showing = false;
	wizzard_setMode(wizzard_qued_mode);}
function wizzard_reset() {
	wizzard_hideMessage(); 
	wizzard_current_message = "";
	wizzard_current_mode = "&nbsp;";
	wizzard_qued_message = "";
	wizzard_qued_mode = "&nbsp;";
	wizzard_is_showing = false;}
