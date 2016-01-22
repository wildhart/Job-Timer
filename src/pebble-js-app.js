// https://github.com/pebble-examples/slate-config-example/blob/master/src/js/pebble-js-app.js

Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready!');
  var settings=localStorage.getItem("settings");
  // settings='{"101":"B|0","KEY_VERSION":1,"KEY_SHOW_CLOCK":1,"KEY_AUTO_SORT":0,"KEY_HRS_PER_DAY":80,"KEY_JOBS":"A|0"';
  var dict=settings ? JSON.parse(settings) : {};
  if (!dict.KEY_TIMESTAMP) { 
    var d=new Date();
    dict.KEY_TIMESTAMP = Math.floor(d.getTime()/1000 - d.getTimezoneOffset()*60);
  }
  console.log(JSON.stringify(dict));
  Pebble.sendAppMessage(dict, function() {
    console.log('Send successful: ' + JSON.stringify(dict));
  }, function() {
    console.log('Send failed!');
  });
});

Pebble.addEventListener("appmessage",	function(e) {
	console.log("Received message: " + JSON.stringify(e.payload));
  if (e.payload.KEY_EXPORT) {
    delete e.payload.KEY_EXPORT;
    localStorage.setItem("settings",JSON.stringify(e.payload));
    showConfiguration();
  } else {
    localStorage.setItem("settings",JSON.stringify(e.payload));
  }
});

Pebble.addEventListener('showConfiguration', showConfiguration);

function padZero(i) {
  return (i<10 ? "0":"")+i;
}

function showConfiguration() {
  var url = 'https://wildhart.github.io/job-timer-config/index.html';
  var settings=localStorage.getItem("settings");
  var config="";
  if (settings) {
    settings=JSON.parse(settings);
    config+="?app_version="+(settings.KEY_APP_VERSION ? settings.KEY_APP_VERSION : "1.1");
    config+="&data_version="+settings.KEY_VERSION;
    config+="&show_clock="+settings.KEY_SHOW_CLOCK;
    config+="&auto_sort="+settings.KEY_AUTO_SORT;
    config+="&hrs_per_day="+settings.KEY_HRS_PER_DAY/10;
    
    var d = new Date(0); // The 0 there is the key, which sets the date to the epoch
    d.setUTCSeconds(settings.KEY_LAST_RESET);
    d.setUTCSeconds(d.getTimezoneOffset()*60);
    var days = ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"];
    config+="&last_reset="+encodeURIComponent(days[d.getDay()]+" "+d.getFullYear()+"/"+padZero(d.getMonth()+1)+"/"+padZero(d.getDate())+" "+padZero(d.getHours())+":"+padZero(d.getMinutes()));
    
    var job=0;
    while (job===0 ? settings.KEY_JOBS : settings[100+job]) {
      config+="&job_"+job+"="+encodeURIComponent(job===0 ? settings.KEY_JOBS : settings[100+job]);
      job++;
    }
    if (settings.KEY_TIMER) {
      config+="&active_job="+settings.KEY_TIMER[8];
      var timer_secs=settings.KEY_TIMER[4]+(settings.KEY_TIMER[5]<<8)+(settings.KEY_TIMER[6]<<16)+(settings.KEY_TIMER[7]<<24);
      d=new Date();
      var now_secs=d.getTime()/1000 - d.getTimezoneOffset()*60;
      config+="&timer_secs="+Math.floor(now_secs - timer_secs);
    }
  }
  console.log('Showing configuration page: ' + url + config);

  Pebble.openURL(url+config);
}

Pebble.addEventListener('webviewclosed', function(e) {
  var configData = JSON.parse(decodeURIComponent(e.response));
  console.log('Configuration page returned: ' + JSON.stringify(configData));

  var dict = {};
  dict.KEY_CONFIG_DATA  = 1;
  dict.KEY_SHOW_CLOCK   = configData.show_clock ? 1 : 0;  // Send a boolean as an integer
  dict.KEY_AUTO_SORT    = configData.auto_sort ? 1 : 0;  // Send a boolean as an integer
  dict.KEY_HRS_PER_DAY  = configData.hrs_per_day * 10; 
  dict.KEY_VERSION = configData.data_version;
  var d=new Date();
  dict.KEY_TIMESTAMP = Math.floor(d.getTime()/1000 - d.getTimezoneOffset()*60);
  
  var job=0;
  while (configData["job_"+job]) {
    dict[100+job]=decodeURIComponent(configData["job_"+job]);
    job++;
  }
  
  if (configData.active_job>-1) {
    var settings=JSON.parse(localStorage.getItem("settings"));
    dict.KEY_TIMER=settings.KEY_TIMER;
    dict.KEY_TIMER[8]=configData.active_job;
  }

  // Send to watchapp
  Pebble.sendAppMessage(dict, function() {
    console.log('Send successful: ' + JSON.stringify(dict));
  }, function() {
    console.log('Send failed!');
  });
});