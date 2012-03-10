/**
 *   Joystick - Ruby binding for linux kernel joystick 
 *   Copyright (C) 2008  Claudio Fiorini <claudio@cfiorini.it>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

/* Any help and suggestions are always welcome */


#include "ruby.h"
#include "ruby/io.h"
#include <linux/joystick.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#define MAX_JS 32
#define NAME_LENGTH 128

static VALUE rb_mJoystick;
static VALUE rb_cDevice;
static VALUE rb_cEvent;
static VALUE rb_cSixaxis;

void Init_rjoystick();

static struct js_event jse[MAX_JS];

void jsdevice_mark(int* fd)
{
	rb_gc_mark(*fd);
}

void jsdevice_free(int* fd)
{
	free(fd);
}

void jssix_mark(int* fh)
{
	rb_gc_mark(*fh);
}

void jssix_free(int* fh)
{
	free(fh);
}

VALUE js_dev_init(VALUE klass, VALUE dev_path)
{
	int *fd;
	
	if((fd = malloc(sizeof(int))) != NULL) {
		if((*fd = open(RSTRING_PTR(dev_path), O_RDONLY)) >= 0) {
			if(*fd >= MAX_JS)
				rb_raise(rb_eException, "Error");
			
			return Data_Wrap_Struct(klass, jsdevice_mark, jsdevice_free, fd);
		}
	}	
	return Qnil;
}

VALUE js_dev_axes(VALUE klass)
{
	int *fd;
	unsigned char axes;

	Data_Get_Struct(klass, int, fd);
	if(ioctl(*fd, JSIOCGAXES, &axes) == -1) {
		rb_raise(rb_eException, "cannot retrive axes");
	}
	return INT2FIX(axes);
}

VALUE js_dev_buttons(VALUE klass)
{
	int *fd;
	unsigned char buttons;
	Data_Get_Struct(klass, int, fd);
	if(ioctl(*fd, JSIOCGBUTTONS, &buttons) == -1) {
		rb_raise(rb_eException, "cannot retrieve buttons");
	}

	return INT2FIX(buttons);
}

VALUE js_dev_name(VALUE klass)
{
	int *fd;
	char name[NAME_LENGTH] = "Unknow";

	Data_Get_Struct(klass, int, fd);
	if(ioctl(*fd, JSIOCGNAME(NAME_LENGTH), name) == -1) {
		rb_raise(rb_eException, "cannot retrieve name");
	}
	return rb_str_new2(name);
}

VALUE js_dev_axes_maps(VALUE klass)
{
	int *fd;

	uint8_t axes_maps[ABS_MAX + 1];
	Data_Get_Struct(klass, int, fd);
	if(ioctl(*fd, JSIOCGAXMAP, &axes_maps) == -1) {
		rb_raise(rb_eException, "cannot retrive axes");
	}
	return INT2FIX(axes_maps);
}

VALUE js_dev_version(VALUE klass)
{
	int *fd;
	int version = 0x000800;
	char js_version[16];
	Data_Get_Struct(klass, int, fd);
	if(ioctl(*fd, JSIOCGVERSION, &version) == -1) {
		rb_raise(rb_eException, "version error");
	}
		
	sprintf(js_version, "%d.%d.%d\n", 	
		version >> 16, (version >> 8) & 0xff, version & 0xff);

	return rb_str_new2(js_version);
}

VALUE js_dev_event_get(int argc, VALUE *argv, VALUE klass)
{
	int *fd;
	VALUE blocking;

	rb_scan_args(argc, argv, "01", &blocking);

	Data_Get_Struct(klass, int, fd);

	if(RTEST(blocking))
		fcntl(*fd, F_SETFL, O_NONBLOCK); /* non-blocking call */

	if(read(*fd, &jse[*fd], sizeof(struct js_event)) > 0)
		return Data_Wrap_Struct(rb_cEvent, 0, 0, fd);
	
	return Qnil;
}

VALUE js_dev_close(VALUE klass)
{
	int *fd;
	
	Data_Get_Struct(klass, int, fd);
	close(*fd);
	return Qnil;
}

VALUE js_event_number(VALUE klass)
{
	int *fd;
	Data_Get_Struct(klass, int, fd);
	return INT2FIX((fd && *fd >= 0) ? jse[*fd].number : -1);
}

VALUE js_event_type(VALUE klass)
{
	int *fd;
	Data_Get_Struct(klass, int, fd);
	switch(((fd && *fd >= 0) ? jse[*fd].type : -1) & ~JS_EVENT_INIT)
	{
		case JS_EVENT_AXIS:
			return ID2SYM(rb_intern("axis"));
		case JS_EVENT_BUTTON:
			return ID2SYM(rb_intern("button"));
		default:
			return INT2FIX(((fd && *fd >= 0) ? jse[*fd].type : -1) & ~JS_EVENT_INIT);
	}
}

VALUE js_event_time(VALUE klass)
{
	int *fd;
	Data_Get_Struct(klass, int, fd);
	return INT2FIX((fd && *fd >= 0) ? jse[*fd].time : -1);
}

VALUE js_event_value(VALUE klass)
{
	int *fd;
	Data_Get_Struct(klass, int, fd);
	return INT2FIX((fd && *fd >= 0) ? jse[*fd].value : -1);
}

VALUE js_six_init(VALUE klass, VALUE path)
{
	int *fh;
	if((fh = malloc(sizeof(int))) != NULL) {
		if((*fh = open(RSTRING_PTR(path), O_RDONLY)) >= 0) {	
			return Data_Wrap_Struct(klass, jssix_mark, jssix_free, fh);
		} else
			rb_raise(rb_eException, "Error opening %s", RSTRING_PTR(path));
	}
	return Qnil;	
}

VALUE js_six_get_six(VALUE klass)
{
	int *fh;
	int res;
	int x = -1;
	int y = -1;
	int z = -1;
	unsigned char buf[128];
	VALUE saxis = rb_hash_new();


	Data_Get_Struct(klass, int, fh);
	if(res = read(*fh, buf, sizeof(buf))) {
		if(res == 48) {
   			x = buf[40]<<8 | buf[41];
			y = buf[42]<<8 | buf[43];
			z = buf[44]<<8 | buf[45];
		} else if(res == 49) {
			x = buf[41]<<8 | buf[42];
			y = buf[43]<<8 | buf[44];
			z = buf[45]<<8 | buf[46];
		}

		rb_hash_aset(saxis, ID2SYM(rb_intern("x")), INT2FIX(x));	
		rb_hash_aset(saxis, ID2SYM(rb_intern("y")), INT2FIX(y));	
		rb_hash_aset(saxis, ID2SYM(rb_intern("z")), INT2FIX(z));	

		return saxis;
	} else
		rb_raise(rb_eException, "error");

	return Qnil;
}

VALUE js_six_close(VALUE klass)
{
	int *fh;
	
	Data_Get_Struct(klass, int, fh);

	return INT2FIX(close(*fh));
}

void Init_rjoystick()
{
	rb_mJoystick = rb_define_module("Joystick");

	rb_cDevice = rb_define_class_under(rb_mJoystick, "Device", rb_cObject);
	rb_define_singleton_method(rb_cDevice, "new", js_dev_init, 1);
	rb_define_method(rb_cDevice, "axes", js_dev_axes, 0);
	rb_define_method(rb_cDevice, "buttons", js_dev_buttons, 0);
	rb_define_method(rb_cDevice, "axes_maps", js_dev_axes_maps, 0);
	rb_define_method(rb_cDevice, "name", js_dev_name, 0);
	rb_define_method(rb_cDevice, "version", js_dev_version, 0);
	rb_define_method(rb_cDevice, "event", js_dev_event_get, 0);
	rb_define_method(rb_cDevice, "close", js_dev_close, 0);

	rb_cEvent = rb_define_class_under(rb_mJoystick, "Event", rb_cObject);	
	rb_define_method(rb_cEvent, "time", js_event_time, 0);
	rb_define_method(rb_cEvent, "value", js_event_value, 0);
	rb_define_method(rb_cEvent, "number", js_event_number, 0);
	rb_define_method(rb_cEvent, "type", js_event_type, 0);
		
	rb_cSixaxis = rb_define_class_under(rb_mJoystick, "SixAxis", rb_cObject);
	rb_define_singleton_method(rb_cSixaxis, "new", js_six_init, 1);
	rb_define_method(rb_cSixaxis, "get_sixaxis", js_six_get_six, 0); 
	rb_define_method(rb_cSixaxis, "close", js_six_close, 0);

	/*
	 * only used for identifying event types, so no need to make them available
	rb_define_const(rb_cEvent, "BUTTON", INT2FIX(JS_EVENT_BUTTON));
	rb_define_const(rb_cEvent, "AXIS", INT2FIX(JS_EVENT_AXIS));
	rb_define_const(rb_cEvent, "INIT", INT2FIX(JS_EVENT_INIT));
	*/
}
