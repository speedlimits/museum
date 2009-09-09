
function stopPropagation(event, ui) { event.stopPropagation() ; }

$(document).ready(function() {	
	
	$("#mainContent").droppable({
      	drop: function(event, ui) {
			debug("placing object");
			var msg = "inventory placeObject " + ui.helper[0].children[0].id + " " + event.pageX + " " + event.pageY";

			debug(msg) ;
			
			Client.event("navcommand", msg);
			return false; //reject it so that position gets reverted

		}
		
	} ) ;

	$("#development-screen").draggable();
	$("#development-console-screen").draggable();


	$("#development-console-screen p").bind('mousedown', stopPropagation ) ;
	$("#development-console-screen p").bind('mouseup', stopPropagation ) ;

	$("#navigation-screen").draggable();

	
	$("li.topbarbutton").bind('mouseover', function(event){
		$(this).css('background-position-y', '-38px');	
					
	});
	$("li.topbarbutton").bind('mouseout', function(event){
		$(this).css('background-position-y', '0px');						
	});
	$("li.topbarbutton").mousedown( function(event){
		$(this).css('background-position-y', '-76px');						
	});
	
	
	/* Navigation Stuff */
	
	$("#navi_up").bind('mousedown' , function() {
		navi_pic("up");
		Client.event("navmoveforward", 1.0);
	});
	$("#navi_up").bind('mouseover' , function() {
		navi_pic("up_h");
	});
	$("#navi_up").bind('mouseup' , function() {			
		navi_pic("up_h");
		Client.event("navmoveforward", 0.0);
	});
	$("#navi_up").bind('mouseout' , function() {
		navi_pic("default");									 
		Client.event("navmoveforward", 0.0);
	});
	
	$("#navi_down").bind('mousedown' , function() {
		navi_pic("down");
		Client.event("navmoveforward", -1.0);
	});
	$("#navi_down").bind('mouseover' , function() {
		navi_pic("down_h");
	});
	$("#navi_down").bind('mouseup' , function() {
		navi_pic("down_h");
		Client.event("navmoveforward", 0.0);
	});
	$("#navi_down").bind('mouseout' , function() {
		navi_pic("default");
		Client.event("navmoveforward", 0.0);
		
	});

	$("#navi_left").bind('mousedown' , function() {
		navi_pic("left");
		Client.event("navturnleft", 1.0);
	});
	$("#navi_left").bind('mouseover' , function() {
		navi_pic("left_h");
	});
	$("#navi_left").bind('mouseup' , function() {
		navi_pic("left_h");
		Client.event("navturnleft", 0.0);
	});
	$("#navi_left").bind('mouseout' , function() {
		navi_pic("default");											   
		Client.event("navturnleft", 0.0);
	});

	$("#navi_right").bind('mousedown' , function() {
		navi_pic("right");
		Client.event("navturnright", 1.0);
	});
	$("#navi_right").bind('mouseover' , function() {
		navi_pic("right_h");
	});
	$("#navi_right").bind('mouseup' , function() {
		navi_pic("right_h");
		Client.event("navturnright", 0.0);
	});
	$("#navi_right").bind('mouseout' , function() {
		navi_pic("default");
		Client.event("navturnright", 0.0)
	});
	
	/* Critique Mode Stuff */
	/*  
        picture-navigation-next
        picture-navigation-map */
	$("#picture-navigation-previous").bind('click', function (event, ui) {
		debug('CMD go to previous picture');
	});
	$("#picture-navigation-next").bind('click', function (event, ui) {
		debug('CMD go ot next picture');
	});
	$("#picture-navigation-map").bind('click', function (event, ui) {
		debug('show map');
	});

	$(".ui").disableSelection();
	
});


function navi_pic (pic) {
	$("#navigation-picture").attr('src', "images/navigation/navigation_" + pic + ".png");
}
