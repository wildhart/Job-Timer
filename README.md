# Job-Timer

This v1.3 branch is a work-in-progress for sending complete logs to a user via email once per week.

Done:
* Added message KEY_LOG to send log to the phone whenever a timer is stopped/paused
* Phone stores all the log messages in JSON.parse(localStorage.getItem("logs")) *IF* the user has opted for weekly logs (i.e. localStorage.getItem("log_settings") exists)
* The settings page has been updated to add settings for sending the log.  Settings page is wildhart.github.io/job-timer-config/index.html - this will only show these new settings for version 1.3+

Still to do:
* Every week (on log_settings.day [0=Monday]) the phone needs to send the log to the server.  This should be done in pebble-js-app.js function newLog()
* The server should send the log to the user.
* If successful, the phone can clear the log.
