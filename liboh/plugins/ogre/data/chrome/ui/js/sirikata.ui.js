
function stopPropagation(event, ui) { event.stopPropagation() ; }

$(document).ready(function() {	
	
	$("#drop-screen").droppable({
      	drop: function(event, ui) {
			debug("placing object");
			debug("inventory placeObject " + ui.helper[0].children[0].id + " " + event.pageX + " " + event.pageY) ;
			try {			  
			  Client.event("navcommand", "inventory placeObject " + ui.helper[0].children[0].id + " " + event.pageX + " " + event.pageY);
			} catch(err) {debug('error placing object');}
			
			//return false; //reject it so that position gets reverted

		}
		
	} ) ;

/*	$("#navigation-screen").draggable();
	$("#development-console-screen").draggable();


	$("#development-console-screen p").bind('mousedown', stopPropagation ) ;
	$("#development-console-screen p").bind('mouseup', stopPropagation ) ;
*/

	
	
	
	
	$(".ui").disableSelection();
	
});


function navi_pic (pic) {
	$("#navigation-picture").attr('src', "images/navigation/navigation_" + pic + ".png");
}




