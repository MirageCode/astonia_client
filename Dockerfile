#
# MinGW-64 environment for building Astonia for Windows on a Linux host
#
# Build and tag the Docker image first:
#   docker build -t astonia-make .
#
# And then run it:
#   docker run --rm -i -u "$(id -u):$(id -g)" -v "$PWD:/app" -w /app astonia-make
#
# Make targets can be appended:
#   docker run --rm -i -u "$(id -u):$(id -g)" -v "$PWD:/app" -w /app astonia-make distrib
#

FROM archlinux

RUN pacman --noconfirm -Syu && \
    pacman --noconfirm -S pacman-contrib curl

RUN curl -L https://github.com/msys2/msys2-installer/releases/download/2023-10-26/msys2-base-x86_64-20231026.tar.xz | \
    tar -xvJ msys64/usr/share/pacman/keyrings/ msys64/var/lib/pacman/sync/ \
    msys64/etc/pacman.d/ msys64/etc/pacman.conf \
    --strip-components 1 \
    --transform 's|.*/pacman.conf$|/tmp/tmp/pacman.conf|' && \
    grep --no-group-separator -B 1 '^Include' /tmp/pacman.conf >> /etc/pacman.conf && \
    pacman-key --init && \
    pacman-key --populate msys2 && \
    rm /usr/share/pacman/keyrings/msys2* /tmp/pacman.conf && \
    pacman --noconfirm -Syu msys/msys2-keyring

RUN pacman --noconfirm -S \
    mingw-w64-gcc make zip binutils \
    mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_net mingw-w64-x86_64-SDL2_mixer \
    mingw-w64-x86_64-libpng mingw-w64-x86_64-libzip \
    mingw-w64-x86_64-dwarfstack

RUN sed -e $'s/\r//g' -e '/cygpath/ d' -i /mingw64/bin/sdl2-config

# Simple script to simulate ldd for the purposes of "make distrib"
COPY <<-"EOT" /bin/find-mingw-dlls
	#!/bin/sh
	objdump -p "$@" | awk -F': ' '$1 ~ /^\s*DLL Name$/ { print $2; }' | while read f
	do
	  find /mingw64/bin/ -maxdepth 1 -name "$f" -printf '%f => %p ()\n'
	done
EOT

RUN chmod +x /bin/find-mingw-dlls

ENTRYPOINT [ \
    "make", "CC=x86_64-w64-mingw32-gcc -I/mingw64/include -L/mingw64/lib", \
    "WINDRES=x86_64-w64-mingw32-windres", "SDL_CONFIG=/mingw64/bin/sdl2-config", \
    "LDD=/bin/find-mingw-dlls" ]
