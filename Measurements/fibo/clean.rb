#!/usr/bin/env ruby

time_lines = []

STDIN.each do |line|
	time_lines << line.chomp if line =~ /seconds/
end

puts "="*80

user_times = []
sys_times = []

time_lines.each do |line|
	description, time = line.split(/:/)
	time = time.chomp.to_f
	if description =~ /User/
		user_times << time
	else
		sys_times << time
	end
end

def dump_times(times)
	times.each { |t| print "#{t}\t"}
	puts
end

dump_times(user_times)
dump_times(sys_times)