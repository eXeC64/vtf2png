vtf2png
=======

I couldn't find a decent way to look at images in the VTF format (used by
Valve's source engine) on Linux, so I wrote this quick and dirty tool to export
these files to PNGs. The only dependency is libPNG, making this ultra-portable.

Exporting a single image: `./vtf2png some.vtf out.png`

Exporting a frame from an animated image: `./vtf2png -f 17 some.vtf out.png`
