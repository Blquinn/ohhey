# ohhey

## DISCLAIMER: This shit is almost certainly not secure at all, use at your own risk!!!

But it is cool 😎 so you should totally try it out.

### Building

Build dependencies are meson, cmake, gcc

#### Steps:

1. Create a build dir `meson build`
    - Optionally select a C and C++ compiler with standard `CC` and `CXX` environment variables before creating the build directory.
1. (Optionally) Configure as a release build `meson configure -Drelease=true -Dstrip=true`
1. Install with `meson install` or just build with `ninja`

Check the meson output to see what files and executables were installed.

#### Post install steps

**Configure your face**

Configure your face with `ohhey add ${USER}` (You'll need to be logged in as root.)

You can confirm that you face matches the stored face with `ohhey compare -v ${USER}`.

**Add PAM config**

Add pam module to whatever pam configs you want.

E.g. system-local-login, sudo etc..

Append

```
auth    sufficient  libpam_ohhey.so
```

to one of the configs in /etc/pam.d

**Configure ohhey (optional)**

Check out the various configs in /etc/ohhey/config.ini.

#### Notes on IR Cameras

It is preferrable to use the IR camera if your machine has one. This will make the detection more acurate, regardless of lighting.

If your computer has an IR camera and emitter, you'll probably need
to configure linux to use the emitter when the IR camera is on. You
can find instructions to install the [here](https://github.com/EmixamPP/linux-enable-ir-emitter).

I then use a systemd init script to run the script

```
[Unit]
Description=Enables the IR camera emitter.
 
[Service]
ExecStart=/home/<user>/.local/bin/run-enable-ir-emitter.sh
 
[Install]
WantedBy=multi-user.target
```

Then the actual script would just be

```
#!/bin/bash

linux-enable-ir-emitter run
```
