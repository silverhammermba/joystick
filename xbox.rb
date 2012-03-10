#!/usr/bin/env ruby

require 'joystick'
require 'curses'

class Xbox360Controller < Joystick::Device
	attr_reader :axis, :button

	def loop typef = nil, numf = nil
		last = 0
		i = 0
		while true
			e = event(true)
			if e
				puts if e.time != last
				puts e if (not typef or e.type == typef) and (not numf or e.number == numf)
				last = e.time
			end
		end
	end

	def state_mode
		@axis = Array.new(axes)
		@button = Array.new(buttons)
		#Thread.new do
			while true
				if e = event(false)
					case e.type
					when :axis
						@axis[e.number] = e.value
					when :button
						@button[e.number] = e.value
					end
				end
				#sleep(0.1)
			end
		#end
	end

	def joy_test
		scr = Curses.init
		Curses.curs_set 0
		while e = event
			if e.type == :axis
				case e.number
				when 0
					x = (scr.columns - 1) * (e.value + 32767) / 65534
				when 1
					y = (scr.lines - 1) * (e.value + 32767) / 65534
				end
				scr.clear
				scr.print(y, x, "#") if y and x
				scr.refresh
			end
		end
		scr.print(0, 0, "THE FUCK")
	end

	def triggers
		while true
			lastn = nil
			lastp = nil
			e = event
			if e and e.type == :axis
				p = ((e.value + 32767) * 100) / 65534
				if e.number == 2
					if p != lastp
						puts "Left trigger: #{p}%"
						lastp = p
					end
				elsif e.number == 5
					if p != lastp
						puts "Right trigger: #{p}%"
						lastp = p
					end
				end
			end
		end
	end

	def get_button
		# return the number of the currently pressed button or nil
		ret = nil
		until (e = event(true)).nil?
			if e.type == :button and e.value == 1
				ret = e.number
			end
		end
		return ret
	end
end

class Joystick::Event
	def to_s
		case type
		when :axis
			case number
			when 0
				"Left thumb X: #{value}"
			when 1
				"Left thumb Y: #{value}"
			when 2
				"Left trigger: #{value}"
			when 3
				"Right thumb X: #{value}"
			when 4
				"Right thumb Y: #{value}"
			when 5
				"Right trigger: #{value}"
			when 6
				"D-pad X: #{value}"
			when 7
				"D-pad Y: #{value}"
			else
				"Axis #{number}: #{value}"
			end
		when :button
			"Button #{number} #{value == 0 ? "RELEASE" : "PRESS" }"
		else
			"Unknown #{number}: #{value}"
		end
	end
end

