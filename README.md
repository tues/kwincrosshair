# Crosshair effect for KWin

![Screenshot](http://bitbucket.org/tues/kwincrosshair/downloads/screenshot.png)

## Building

    $ cmake ./
    $ make

## Installing

    $ sudo make install

The files will be most likely installed to the /usr/local/ prefix, which KDE
doesn't search by default. In order to tell KDE about this prefix you should
add /usr/local to your KDEDIRS environment variable, for example:

    $ export KDEDIRS="/usr:/usr/local"

You can add the above command to your ~/.bashrc or some distribution dependent
location. If you're using Gentoo, the best place is probably /etc/env.d/.

Alternatively, you can skip the "make install" part and copy the files
manually into the /usr/ prefix:

    $ sudo cp *.desktop /usr/share/kde4/services/kwin/
    $ sudo cp *.so /usr/lib/kde4/

## Running

Depending on how you installed the effect, you may need to restart KWin, whole
KDE or even reboot your system to make sure KWin detects the plugin. To enable
and configure the effect go to "System Settings -> Desktop Effects ->
All Effects" and select "Crosshair". Alternatively, you can run:

    $ kcmshell4 kwincompositing
