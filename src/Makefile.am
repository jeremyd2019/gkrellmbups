bin_PROGRAMS = gkrellmbups

gkrellmbups_SOURCES = gkrellmbups.c gkrellmbups.h \
	chart.c chart.h \
	prefs.c prefs.h \
	ups_connect.c ups_connect.h \
	version.h


GTK_INCLUDE   = `pkg-config gtk+-2.0 --cflags`
GTK_LIB       = `pkg-config gtk+-2.0 --libs`

CPPFLAGS = $(GTK_INCLUDE)
CFLAGS = -O2 -Wall -fPIC
LDFLAGS = -shared
LIBS = $(GTK_LIB)

install:
	if [ -d $(prefix)/lib/gkrellm2/plugins/ ] ; then \
		cp gkrellmbups $(prefix)/lib/gkrellm2/plugins/gkrellmbups.so ; \
	    chmod 644 $(prefix)/lib/gkrellm2/plugins/gkrellmbups.so ; \
	fi
#	elif [ -d /usr/share/gkrellm2/plugins/ ] ; then \
#		cp gkrellmbups /usr/share/gkrellm2/plugins/gkrellmbups.so ; \
#	    chmod 644 /usr/share/gkrellm2/plugins/gkrellmbups.so ; \
#	elif [ -d /usr/local/lib/gkrellm2/plugins/ ] ; then \
#		cp gkrellmbups /usr/local/lib/gkrellm2/plugins/gkrellmbups.so ; \
#	    chmod 644 /usr/local/lib/gkrellm2/plugins/gkrellmbups.so ; \
#	else \
#		cp gkrellmbups /usr/lib/gkrellm2/plugins/gkrellmbups.so ; \
#	    chmod 644 /usr/lib/gkrellm2/plugins/gkrellmbups.so ; \
#	fi

uninstall:
	rm -f $(prefix)/lib/gkrellm2/plugins/gkrellmbups.so

