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

	def joy_test
		# blocking joystick test
		scr = Curses.init
		Curses.curs_set 0
		Curses.echo = false
		while e = event
			if @axis[0] and @axis[1]
				x = (scr.columns - 1) * (@axis[0] + 32767) / 65534
				y = (scr.lines - 1) * (@axis[1] + 32767) / 65534
				scr.clear
				scr.print(y, x, "#")
				scr.refresh
			end
			if e.type == :button and e.number == 7 and e.value == 1
				break
			end
		end
		Curses.close
	end

	def joy_test2
		# non-blocking CPU hog joystick test
		scr = Curses.init
		Curses.curs_set 0
		Curses.echo = false
		while true
			if e = event
				if e.type == :axis and @axis[0] and @axis[1]
					y = (scr.lines - 1) * (@axis[1] + 32767) / 65534
					x = (scr.columns - 1) * (@axis[0] + 32767) / 65534
					scr.clear
					scr.print(y, x, "#")
					scr.refresh
				end
				if e.type == :button and e.number == 7 and e.value == 1
					break
				end
			end
		end
		Curses.close
	end

	def joy_test3
		# threaded CPU friendly blocking joystick test

		put = nil
		get = Thread.new do
			while true
				e = event
				if e.type == :button and e.number == 7 and e.value == 1
					get.exit
					put.exit
				end
			end
		end

		scr = Curses.init
		Curses.curs_set 0
		Curses.echo = false
		put = Thread.new do
			while true
				sleep 0.01
				if @axis[0] and @axis[1]
					y = (scr.lines - 1) * (@axis[1] + 32767) / 65534
					x = (scr.columns - 1) * (@axis[0] + 32767) / 65534
					scr.clear
					scr.print(y, x, "#")
					scr.refresh
				end
			end
		end
		get.join
		Curses.close
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

	def get_axis deadzone = 0
		@axis = Array.new(axes) unless @axis
		until (e = event(true)).nil?
			if e.type == :axis and e.number <= 1
				@axis[e.number] = e.value
			end
		end
		return @axis[0..1]
		#magnitude = Math.sqrt(ret[0] ** 2 + ret[1] ** 2)
		#return [Math.atan2(ret[1], ret[0]), magnitude] if magnitude >= deadzone
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

