
function leaveModal() {
	$("#mode-large-modal").fadeOut("slow");
	$("#mode-small-modal").fadeOut("slow");
	$("#mode-screen").fadeOut("slow");
}
function loadLargeModal(page) {
	mw = $("#mode-large-modal");
	mw.load(page);
	$("#mode-screen").fadeIn("slow");
	mw.fadeIn("slow");
}
function loadSmallModal(page, noModeScreen) {
	mw = $("#mode-small-modal");
	mw.load(page);
	if(noModeScreen != true) 
		$("#mode-screen").fadeIn("slow");
	mw.fadeIn("slow");
}
function loadInfoScreen(page, showModeScreen) {
	mw = $("#info-screen");
	mw.load(page);
	if(showModeScreen == true) 
		$("#mode-screen").fadeIn("slow");
//	mw.fadeIn("slow");
	debug('change toggle');
	mw.fadeIn("slow");
}
function loadSideBar(page) {
	leaveModal();
	
	$('#wizzard-screen').fadeIn('slow');
	$("#sidebar1").load(page);
	setTimeout('$("#sidebar1").fadeIn("slow")', 1000);
}
function debug(msg) {
	$("#development-console").append("<br>" + msg );

}

