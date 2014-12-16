var initialized = false;

Pebble.addEventListener('showConfiguration',
	function(e) {
		var config=localStorage.getItem("config");
		if (config==null) {
			config=JSON.stringify({"halftone":true, "invert":false, "blink": true});
		}
		var query=encodeURIComponent(config);
		console.log(query);

		console.log("showConfiguration: " + query);
		Pebble.openURL("http://www.thomen.fi/pebble/7-seg.html#" + query);
	}
);

Pebble.addEventListener('ready',
	function() {
		initialized=true;
	}
);

Pebble.addEventListener('webviewclosed',
	function (e) {
		var result = decodeURIComponent(e.response);
		if (result=="cancel") {
			return;
		} else {
			var config=JSON.parse(result);
			localStorage.clear();
			localStorage.setItem("config", JSON.stringify(config));

			Pebble.sendAppMessage(config);
		}
	}
);
