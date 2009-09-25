
function stopPropagation(event, ui) { event.stopPropagation() ; }

$(document).ready(function() {	

	
	$("#drop-screen").droppable({
      	drop: function(event, ui) {
			var curatorbox=$("#detail-screen");
			if (curatorbox) {
				var offset = curatorbox.offset();
				debug("Offset: "+offset.left+","+offset.top+"; event: "+event.pageX+","+event.pageY);
				debug("Width: " + curatorbox.width() +", Height: "+curatorbox.height());
				if ((event.pageX > offset.left
					&& event.pageX < offset.left + curatorbox.width()) &&
					(event.pageY > offset.top
					&& event.pageY < offset.top + curatorbox.height())) {
					debug("Dropped inside curator box! Ignoring...");
					return;
				}
			}
			debug("placing object");
			try {			  
			  debug("inventory placeObject " + ui.helper[0].children[0].id + " " + event.pageX + " " + event.pageY) ;
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




