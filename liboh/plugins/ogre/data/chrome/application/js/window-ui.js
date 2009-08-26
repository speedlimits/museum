$(document).ready(function() {	
	
	$("#mainContent").droppable({
      	drop: function(event, ui) {
			debug("placing object");
			debug("inventory placeObject " + ui.helper[0].children[0].id + " " + event.pageX + " " + event.pageY) ;
			Client.event("navcommand", "inventory placeObject " + ui.helper[0].children[0].id + " " + event.pageX + " " + event.pageY);

		}
		
	});
	
	$(".topbarbutton").bind('mouseover', function(event){
												  
		$(this).css('background-position-y', '-38px');						
	});
	$(".topbarbutton").bind('mouseout', function(event){
		$(this).css('background-position-y', '0px');						
	});
	$(".topbarbutton").bind('mousedown', function(event){
		$(this).css('background-position-y', '-76px');						
	});
	
	
	/* Navigation Stuff */
	
	$("#navi_up").bind('mousedown' , function() {
		Client.event("navmoveforward", 1.0)
	});
	$("#navi_up").bind('mouseup' , function() {
		Client.event("navmoveforward", 0.0)
	});
	$("#navi_up").bind('mouseout' , function() {
		Client.event("navmoveforward", 0.0)
	});
	
	$("#navi_down").bind('mousedown' , function() {
		debug('backwards is not working');		
		Client.event("navback", 1.0)
	});
	$("#navi_down").bind('mouseup' , function() {
		Client.event("navback", 0.0)
	});
	$("#navi_down").bind('mouseout' , function() {
		Client.event("navback", 0.0)
	});

	$("#navi_left").bind('mousedown' , function() {
		Client.event("navturnleft", 1.0)
	});
	$("#navi_left").bind('mouseup' , function() {
		Client.event("navturnleft", 0.0)
	});
	$("#navi_left").bind('mouseout' , function() {
		Client.event("navturnleft", 0.0)
	});

	$("#navi_right").bind('mousedown' , function() {
		Client.event("navturnright", 1.0)
	});
	$("#navi_right").bind('mouseup' , function() {
		Client.event("navturnright", 0.0)
	});
	$("#navi_right").bind('mouseout' , function() {
		Client.event("navturnright", 0.0)
	});




	
	$("#navi_right").click( function() {
		alert("Right") ;
		wizzard_showMessage();
	
	});
	
	$(".ui").disableSelection();
	
});
