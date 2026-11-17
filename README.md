# Catastrophe Mixer (3DS homebrew)
Top screen = stereoscopic 3D city (6 parallax depth layers driven by the 3D slider).
Bottom screen = touch mixer: tap two disasters, hit UNLEASH (or A). D-Pad/Circle Pad aims. Y rebuilds.
15 mixes (Lavanado, Meteornado, Thunder Tide, ...) each with x1.6 damage and real interactions
(tornado flings lava/meteors, lightning hits harder in tsunami water, etc).

## Build
    export DEVKITPRO=/opt/devkitpro DEVKITARM=$DEVKITPRO/devkitARM
    dkp-pacman -S 3ds-dev          # libctru, citro2d, citro3d, 3dstools
    python3 make_assets.py         # icon / banner / audio (already generated)
    make                           # -> catastrophe_mixer.3dsx  (Homebrew Launcher, no banner)
    make cia                       # -> catastrophe_mixer.cia   (installable, shows Home Menu banner)
Needs `bannertool` and `makerom` on PATH for the CIA.

## Home Menu banner (still image)
`banner.png` (256x128) is a still banner, shown only when the game is installed as a CIA.
`audio.wav` is the converted banner jingle (1.2s, 16-bit stereo). `make_assets.py` won't overwrite it.
Swap `banner.png` / `icon.png` (48x48) for your own art any time.

## No-install build (GitHub)
Upload this folder to a new GitHub repo (keep the `.github` folder), open the Actions tab, run "Build 3DS",
and download the `CatastropheMixer` artifact: it contains the .3dsx and .cia.
