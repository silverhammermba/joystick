Gem::Specification.new do |s|
	s.name = 'joystick'
	s.version = '0.0.0'
	s.summary = "binding for Linux kernel joysticks"
	s.description = <<EOS
This is a Ruby extension that wraps the fuctionality provided by
linux/joystick.h, allowing Ruby scripts to use input from Xbox 360
controllers, PS3 Sixaxis controllers, etc.

I originally took this code from Claudio Fiorini's rjoystick gem, made it a
little easier to use, made it work with Ruby's Threads, and added
documentation.
EOS
	s.authors = ["Max Anselm", "Claudio Fiorini"]
	s.email = 'silverhammermba@gmail.com'
	s.require_paths << 'ext'
	s.files = ["ext/joystick.c", "ext/extconf.rb"]
	s.extensions << 'ext/extconf.rb'
	s.homepage = 'http://github.com/silverhammermba/joystick'
	s.license = 'GPL-3'
end
