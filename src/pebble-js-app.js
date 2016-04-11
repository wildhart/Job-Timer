// https://github.com/pebble-examples/slate-config-example/blob/master/src/js/pebble-js-app.js

Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready!');
  var settings=localStorage.getItem("settings");
  //settings='{"101":"B|0","KEY_VERSION":1,"KEY_SHOW_CLOCK":1,"KEY_AUTO_SORT":0,"KEY_HRS_PER_DAY":80,"KEY_JOBS":"A|0"}';
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
  if (e.payload.KEY_LOG) {
    var log_settings=JSON.parse(localStorage.getItem("log_settings"));
    if (log_settings) newLog(e.payload.KEY_LOG, e.payload, log_settings);
    // don't bother storing log in settings.
    delete e.payload.KEY_LOG;
  }
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
    
    var d = new Date(0);
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
    
    var log_settings=JSON.parse(localStorage.getItem("log_settings"));
    if (log_settings) {
      config+="&log_email="+log_settings.email;
      config+="&log_day="+log_settings.day;
      config+="&log_raw="+(log_settings.raw?"1":"0");
      config+="&log_reset="+(log_settings.reset?"1":"0");
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
  
  if (configData.log_email) {
    var log_settings={
      email:configData.log_email,
      day:configData.log_day,
      raw:configData.log_raw,
      reset:configData.log_reset,
    };
    console.log(JSON.stringify(log_settings));
    localStorage.setItem("log_settings",JSON.stringify(log_settings));
  }

  // Send to watchapp
  Pebble.sendAppMessage(dict, function() {
    console.log('Send successful: ' + JSON.stringify(dict));
  }, function() {
    console.log('Send failed!');
  });
});

function newLog(new_log, payload, log_settings) {
  // get log and convert to array
  new_log=new_log.split("|");
  // convert job number to job name
  var job=(new_log[0]=="0") ? payload.KEY_JOBS : payload[100+new_log[0]*1];  // *1 to convert string to integer
  new_log[0]=job.split("|")[0];
  new_log=new_log.join("|");
  
  // add new log to list of old logs
  var logs=JSON.parse(localStorage.getItem("logs")) || [];
  logs.push(new_log);
  
  
  
  // save old logs
  localStorage.setItem("logs",JSON.stringify(logs));
  
  console.log(JSON.stringify(logs));
  
}