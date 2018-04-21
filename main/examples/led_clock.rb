i = 0

lines = []
aline = [20, 10, 3, 8, 2, 3, 2, -1]
def succ_line l
    x0, y0, x1, y1, dx0, dy0, dx1, dy1 = l[0], l[1], l[2], l[3], l[4], l[5], l[6], l[7]
    x0 += dx0; y0 += dy0; x1 += dx1; y1 += dy1
    (dx0 *= -1; x0 = 0) if x0 < 0
    (dy0 *= -1; y0 = 0) if y0 < 0
    (dx1 *= -1; x1 = 0) if x1 < 0
    (dy1 *= -1; y1 = 0) if y1 < 0
    (dx0 *= -1; x0 = 31) if x0 > 31
    (dy0 *= -1; y0 = 31) if y0 > 31
    (dx1 *= -1; x1 = 31) if x1 > 31
    (dy1 *= -1; y1 = 31) if y1 > 31
    [x0, y0, x1, y1, dx0, dy0, dx1, dy1]
end
8.times {|t|
  lines << aline
  aline = succ_line(aline)
}
while true do
  Led::clear 0
  lines.each {|l|
    Led::line l[0], l[1], l[2], l[3], 0, 5, 0
  }
  lines.shift()
  puts(lines.size())
  lines << succ_line(lines[-1])

  Led::flash

  i = (i + 1) % 32
  mem = ESP32::System.available_memory() / 1000
  puts "Free heap: #{mem}K"
  ESP32::System.delay(100)
end
