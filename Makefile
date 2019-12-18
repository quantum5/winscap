CC = cl /nologo
CFLAGS = /W4 /EHsc
LD = link /nologo
LDFLAGS = /opt:ref

all: winscap.exe

winscap.exe: winscap.obj
	$(LD) $(LDFLAGS) $** /out:$@

.cpp.obj::
	$(CC) $(CFLAGS) /c $<

clean:
	del winscap.exe *.obj
