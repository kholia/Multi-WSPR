Written for 'Pico W' board.

```
[dhiru@zippy Multi-WSPR]$ /usr/bin/wsprd -C 2 -a /home/dhiru/.local/share/WSJT-X -f 14.095600 untight-with-delay-function.wav
tion  26  0.6  14.097030  0  VU3CER MK68 20
<DecodeFinished>
```

```
[dhiru@zippy Multi-WSPR]$ /usr/bin/wsprd -C 2 -a /home/dhiru/.local/share/WSJT-X -f 14.095600 tight-with-busy-loop.wav
loop  26 -0.4  14.097030  0  VU3CER MK68 20
loop -24 -0.3  14.097112  0  VU3CER MK68 20
loop -27 -0.4  14.097193  0  VU3CER MK68 20
<DecodeFinished>
```
