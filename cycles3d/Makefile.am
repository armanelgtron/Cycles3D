####### kdevelop will overwrite this part!!! (begin)##########
bin_PROGRAMS = cycles3d

## INCLUDES were found outside kdevelop specific part

cycles3d_SOURCES = ttimer.cpp setup.cpp recog.cpp proj.cpp player.cpp menuutil.cpp menuopt.cpp menugfx.cpp menugamefind.cpp menu.cpp main.cpp 
cycles3d_LDADD   = ./audio/libaudio.a ./gfx/libgfx.a ./net/libnet.a ./gns/libgns.a -lglut -lSDL -lpthread

SUBDIRS = audio gfx net gns data docs 

EXTRA_DIST = cycles3d.h main.cpp menu.cpp menugamefind.cpp menugfx.cpp menu.h menuopt.cpp menuutil.cpp player.cpp proj.cpp recog.cpp setup.cpp ttimer.cpp ttimer.h c3dgfpkt.h menugamefind.h 

####### kdevelop will overwrite this part!!! (end)############
# set the include path found by configure
INCLUDES= $(all_includes)
 
# the library search path.
cycles3d_LDFLAGS = $(all_libraries) 
                                                                                
