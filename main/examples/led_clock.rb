puts "Getting ready to start wifi"

wifi = ESP32::WiFi.new

wifi.on_connected do |ip|
  puts "Connected: #{ip}"
end

wifi.on_disconnected do
  puts "Disconnected"
end

puts "Connecting to wifi"
wifi.connect('KMC', 'XXXX')

i = 0

while true do
  32.times {|x|
    32.times {|y|

      if x == i and y == i then
        Led::set(x, y, 7, 0, 0)
      else
        Led::set(x, y, 5, 5, 5)
      end
    }
  }
  i = (i + 1) % 32
  mem = ESP32::System.available_memory() / 1000
  puts "Free heap: #{mem}K"
  ESP32::System.delay(300)
end
