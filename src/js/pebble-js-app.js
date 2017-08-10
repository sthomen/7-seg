var initialized = false;
var defaults=JSON.stringify({
	"halftone":	true,
	"blink":	true,
	"vibrate":	true,
	"showtop":	true,
	"showbottom":	true
});

Pebble.addEventListener('showConfiguration',
	function(e) {
		var config=localStorage.getItem("config");
		if (config==null) {
			config=defaults;
		}

		var query=encodeURIComponent(config);

		console.log("showConfiguration: " + decodeURIComponent(query));

		Pebble.openURL("http://www.thomen.fi/pebble/7-seg-1.9.html#" + query);
	}
);

Pebble.addEventListener('ready',
	function() {
		initialized=true;
	}
);

Pebble.addEventListener('webviewclosed',
	function (e) {
		var config;
		var result = decodeURIComponent(e.response);

		console.log(result);

		if (result=="cancel") {
			return;
		}

		if (result=="default") {
			config=JSON.parse(defaults);
		} else {
			config=JSON.parse(result);
		}

		localStorage.clear();
		localStorage.setItem("config", JSON.stringify(config));

		Pebble.sendAppMessage(config);
	}
);
