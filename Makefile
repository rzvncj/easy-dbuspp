all:
	g++ -W -Wall -Wno-unused-parameter -g -O0 `pkg-config --cflags --libs gio-2.0` \
		-o dbus-test dbus_object.cpp main.cpp session_manager.cpp
