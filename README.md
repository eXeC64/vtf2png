vtf2png
=======

`vtf2png` is a command line tool to extract images from the VTF format used by
Valve's Source Engine. It supports a variety of pixel data formats, including
most of the RGBA variations and DXT compression formats.

The only dependencies are libpng and argp.

**Usage:**

Export an image: `./vtf2png some.vtf out.png`

Export a specific frame: `./vtf2png -f 17 some.vtf out.png`
