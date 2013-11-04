# Crosshair effect for KWin

![Screenshot](http://bitbucket.org/tues/kwincrosshair/downloads/screenshot2.png)

## Requirements

KWinCrosshair has been tested with Qt 4.8 and KWin 4.9, but it should work with
Qt 4.6 and KWin 4.7 or newer.

## Building and installing (the easy way)

In your terminal, go to the directory containing the project sources and run:

    $ cmake -DCMAKE_INSTALL_PREFIX=~/.kde4
    $ make
    $ make install

This will install the effect for the current user only.

Proceed as described in "Enabling and configuring".

## (alternative) Building and installing in /usr

In your terminal, go to the directory containing the project sources and run:

    $ cmake -DCMAKE_INSTALL_PREFIX=/usr
    $ make

Run `make install` as root, for example:

    $ sudo make install

or:

    $ su
    # make install

This will install the effect into the /usr prefix, so it will be available
for all users.

## Enabling and configuring

First, to make sure that KWin detects the effect, log out and log in again
to restart KDE, or restart KWin only by running: 

    $ nohup kwin --replace > /dev/null &

Then, go to "System Settings -> Desktop Effects -> All Effects" and select
"Crosshair". Alternatively, you can run:

    $ kcmshell4 kwincompositing
