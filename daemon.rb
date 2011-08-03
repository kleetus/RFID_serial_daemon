require 'serialport'
#system "echo #{$$} > daemon.pid"

class Daemon
  attr_accessor :port

  def initialize
    @db = {}
    @ans = '0'
  end

  def load_db
    db_file = open('/home/kleetus/RFID_serial_daemon/db_real.txt') 
    db_file.each_line do |l| 
      f = l.rstrip
      @db[f[6..-2]] = f[-1..-1]
    end
    puts @db
  end
  
  def open_serial_port
    port_str = "/dev/ttyUSB0"  
    baud_rate = 9600
    data_bits = 8
    stop_bits = 1
    parity = SerialPort::NONE
    @port = SerialPort.new(port_str, baud_rate, data_bits, stop_bits, parity)
    @port.read_timeout=80;
  end
  
  def listen_and_respond
    query = @port.read(6)
    if query and query != 0
      @port.write((@db.has_key?(query) ? @db[query] : '0'))
      puts "#{Time.now.strftime("%Y-%m-%d %H:%M:%S")} ::: query: #{query} ::: answered with: #{@db.has_key?(query) ? @db[query] : '0'}"
    end
  end

end

daemon = Daemon.new
daemon.load_db
daemon.open_serial_port

loop do
  daemon.listen_and_respond
  #daemon.port.write("1")
  #puts "writing"
  #sleep 1
end

daemon.port.close
