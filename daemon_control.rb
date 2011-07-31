require 'daemons'
  
Daemons.run('daemon.rb', :dir_mode => :script)
