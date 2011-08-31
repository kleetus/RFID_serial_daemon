require 'daemons'
  
options = {
  :app_name => "rfid_serial_daemon",
  :backtrace => true,
  :monitor => true,
  :dir_mode => :script,
  :log_output => true
  }
Daemons.run('daemon.rb', options) 
