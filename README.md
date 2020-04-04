Ochim - An up to 4 player action puzzle game for Nintendo 64

# What's this?

This is something like _some consumer title_'s Versus mode, but incomplete.

Imbalance garbage conversion rate, no combo/REN, no team, etc., but 20G and I.R.S. is there.

# Requirement

* [GNU binutils that supports RSP assembly](https://github.com/murachue/binutils-gdb) + gcc for mipseb
* tools(chksum64 and n64tool) and bootcode("header") from libdragon
* Imagemagick
* sox
* Ruby

# Build

if you want to run on real N64 with EverDrive-64,

```
make
```

or, if you want to run on a N64 emulator such as cen64,

```
make EMU=1
```

(EMU=1 enables special treatments for emulator...)

then run `ochim.z64`.

# Thanks

Sfx are taken from...

* http://www.kurage-kosho.info/
* https://otologic.jp/
* http://sfx-g.net/
* http://musmus.main.jp/
* https://soundeffect-lab.info/

| filename | what | source |
| --- | --- | --- |
| brmpshrr.wav | gameover | http://www.kurage-kosho.info/mp3/one06.mp3 |
| garbage.wav | mino-rock | http://www.kurage-kosho.info/mp3/taiko03.mp3 |
| go.wav | game-GO | http://www.kurage-kosho.info/mp3/radio-wave02.mp3 |
| hold.wav | mino-hold | http://www.kurage-kosho.info/mp3/small-bell02.mp3 |
| holdfail.wav | mino-holdfail | http://www.kurage-kosho.info/mp3/bell07.mp3 |
| initialhold.wav | mino-ihs | http://www.kurage-kosho.info/mp3/hand-drum01.mp3 |
| initialrotate.wav | mino-irs | http://www.kurage-kosho.info/mp3/clappers01.mp3 |
| lock.wav | mino-piecelock | http://www.kurage-kosho.info/mp3/switch01.mp3 |
| move.wav | mino-pieceshift | http://www.kurage-kosho.info/mp3/button45.mp3 |
| pageenter2.wav | pause-enter | http://www.kurage-kosho.info/mp3/button19.mp3 |
| pageexit2.wav | pause-leave | http://www.kurage-kosho.info/mp3/button20.mp3 |
| point1.wav | turu-erase1 | http://www.kurage-kosho.info/mp3/one04.mp3 |
| point2.wav | turu-erase2 | http://www.kurage-kosho.info/mp3/one07.mp3 |
| point3.wav | turu-erase3 | http://www.kurage-kosho.info/mp3/one08.mp3 |
| point4.wav | turu-erase4 | http://www.kurage-kosho.info/mp3/one10.mp3 |
| ready.wav | game-READY | http://www.kurage-kosho.info/mp3/radio-wave01.mp3 |
| tamaf.wav | conf-changevalue | http://www.kurage-kosho.info/mp3/button40.mp3 |
| tamaido.wav | conf-cancelready | http://www.kurage-kosho.info/mp3/button50.mp3 |
| erase1.wav | mino-erase1 | https://otologic.jp/sounds/se/mp3-zip/Accent25-mp3.zip |
| erase2.wav | mino-erase2 | https://otologic.jp/sounds/se/mp3-zip/Accent24-mp3.zip |
| linefall.wav | mino-erase-fall | https://otologic.jp/sounds/se/mp3-zip/Onmtp-Impact01-mp3.zip |
| rakka.wav | turu-rockfall | https://otologic.jp/sounds/se/mp3-zip/Onmtp-Negative10-mp3.zip |
| regret.wav | rock | https://otologic.jp/sounds/se/mp3-zip/Accent26-mp3.zip |
| sewol001.wav | rock-cleared | https://otologic.jp/sounds/se/mp3-zip/Onmtp-Flash08-mp3.zip |
| ssshk1_r.wav | turu-piecelock | https://otologic.jp/sounds/se/mp3-zip/Motion-Pop36-mp3.zip |
| rotate.wav | turu-piecerotate | http://sfx-g.net/sound/wind-sweep-lite3-%e3%83%92%e3%83%a5%e3%83%83%ef%bc%89-4/ |
| se000026.wav | turu-pieceshift | http://musmus.main.jp/se/musmus_cancel_set.zip 05 |
| shakin.wav | conf-ready | https://soundeffect-lab.info/sound/button/mp3/decision23.mp3 |
| step.wav | mino-piecetouch | https://soundeffect-lab.info/sound/button/mp3/cursor4.mp3 |

Gfx is taken from...

* https://pixabay.com/illustrations/fractal-undersea-sea-underwater-2033916/

# License

See `LICENSE`
