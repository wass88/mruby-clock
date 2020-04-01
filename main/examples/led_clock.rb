i = 0
infos = ["","KMC"]
ws = [70, 124, 55, 110, 50, 80, 63, 101, 76, 90, 54, 98, 69, 90]
week = (0...7).map{|i|
[27, 36, 66, ws[2*i], ws[2*i+1], 27, 40, 66].map{|k|k.chr}.join("")}
cmd = ""

Task::loop do
Led::clear 0
Time::update

if Task::updated?
w = Task.cmd
if w[0] == "$"
  infos = w.split("$")
else
  cmd = w
end
end


Led::font 2

Led::color 7, 3, 3
if Time.num(6) < 500000
Led::text 1, 11, Time::str("%H:%M")
else
Led::text 1, 11, Time::str("%H %M")
end


Led::font 0
n = (i.div(100)) % infos.size
if n == 0 then
Led::text 2, 3, Time::str("%m/%d")
Led::font 5
Led::text 23, 2, week[Time.num(2)]
else
s = infos[n]
if s.size >= 8
Led::show i.div(2), 3, s
else
Led::text 2, 3, s
end
end

h = Time.num(3)
if 6 <= h and h < 8; Led::color 7,4,3
elsif 8 <= h and h < 16; Led::color 7,4,0
elsif 16 <= h and h < 18; Led::color 7,1,0
else; Led::color 2,2,7
end
Led::font 5
Led::show i.div(2), 21, cmd
#hv = (h * 32 / 24).to_i
#Led::line 0, 31, hv, 31

n = Time.num(5)
if n == 0
n = 60
end
if n > 1
Led::line 1, 31, n.div(2), 31
end
if n % 2 == 1
Led::color 1,1,1
Led::set n.div(2)+1, 31
end

Led::flash
i += 1
end