////////////////////////////////////////////////////////////////////////////////
//               Communications between Javascript and C++                    //
////////////////////////////////////////////////////////////////////////////////

// Web browser functions
function back_web_page()		{ Client.event("navback");    }
function forward_web_page()		{ Client.event("navforward"); }
function refresh_web_page()		{ Client.event("navrefresh"); }
function new_web_page_tab()		{ Client.event("navnewtab");  }

// Camera motion functions
function move_forward_start()	{ Client.event("navmoveforward",  1.0); }
function move_forward_stop()	{ Client.event("navmoveforward",  0.0); }
function move_backward_start()	{ Client.event("navmoveforward", -1.0); }
function move_backward_stop()	{ Client.event("navmoveforward",  0.0); }
function turn_left_start()		{ Client.event("navturnleft",     1.0); }
function turn_left_stop()		{ Client.event("navturnleft",     0.0); }
function turn_right_start()		{ Client.event("navturnright",    1.0); }
function turn_right_stop()		{ Client.event("navturnright",    0.0); }

// Place art in a scene
function place_art(object_name, window_x, window_y) {
	Client.event("navcommand", "place_art " + object_name + " " + window_x + " " + window_y);
}


////////////////////////////////////////////////////////////////////////////////
// This is the way that things will evolve
////////////////////////////////////////////////////////////////////////////////

// JS to Sirikata Messages
// sirikata_send_message( message )
// 
// 
// moving:
// 
// // Set the velocity and direction
// walk [straight|turn] [speed]
// 
// // Move by a specified interval
// step [straight|turn] [distance]
// 
// - straight - forward/backward translation, forward positive
// - turn - rotation, counterclockwise (left) positive
// - speed - meters/sec, degrees/sec
// - distance - meters or degrees
// 
// 
// 
// artwork interaction:
// 
// // Placing an image after it has been draged and dropped in JS
// place_art window_x window_y art_id
// 
// 
// Sirikata to JS Messages
// 
// sirikata_receive_message (message)
// 
// art_selected art_id 
// art_in_view art_id
// 
// 
// Javascript Functions
// 
// // moving 
// move_forward_start()
// move_forward_stop()
// move_backward_start()
// move_backward_stop()
// turn_right_start()
// turn_right_stop()
// turn_left_start()
// turn_left_stop()
// 
// // inventory
// 
// place_art(objectname, x, y)


