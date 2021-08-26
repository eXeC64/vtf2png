#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <png.h>
#include <sys/mman.h>
#include <argp.h>

enum
{
  IMAGE_FORMAT_NONE = -1,
  IMAGE_FORMAT_RGBA8888 = 0,
  IMAGE_FORMAT_ABGR8888,
  IMAGE_FORMAT_RGB888,
  IMAGE_FORMAT_BGR888,
  IMAGE_FORMAT_RGB565,
  IMAGE_FORMAT_I8,
  IMAGE_FORMAT_IA88,
  IMAGE_FORMAT_P8,
  IMAGE_FORMAT_A8,
  IMAGE_FORMAT_RGB888_BLUESCREEN,
  IMAGE_FORMAT_BGR888_BLUESCREEN,
  IMAGE_FORMAT_ARGB8888,
  IMAGE_FORMAT_BGRA8888,
  IMAGE_FORMAT_DXT1,
  IMAGE_FORMAT_DXT3,
  IMAGE_FORMAT_DXT5,
  IMAGE_FORMAT_BGRX8888,
  IMAGE_FORMAT_BGR565,
  IMAGE_FORMAT_BGRX5551,
  IMAGE_FORMAT_BGRA4444,
  IMAGE_FORMAT_DXT1_ONEBITALPHA,
  IMAGE_FORMAT_BGRA5551,
  IMAGE_FORMAT_UV88,
  IMAGE_FORMAT_UVWQ8888,
  IMAGE_FORMAT_RGBA16161616F,
  IMAGE_FORMAT_RGBA16161616,
  IMAGE_FORMAT_UVLX8888
};

// the header structure wants low_image_format and depth fields to be unaligned
#pragma pack(1)

typedef struct
{
  char           signature[4];       // File signature ("VTF\0"). (or as little-endian integer, 0x00465456)
  unsigned int   version[2];         // version[0].version[1] (currently 7.2).
  unsigned int   header_size;        // Size of the header struct (16 byte aligned; currently 80 bytes).
  unsigned short width;              // Width of the largest mipmap in pixels. Must be a power of 2.
  unsigned short height;             // Height of the largest mipmap in pixels. Must be a power of 2.
  unsigned int   flags;              // VTF flags.
  unsigned short frames;             // Number of frames, if animated (1 for no animation).
  unsigned short first_frame;        // First frame in animation (0 based).
  unsigned char  padding0[4];        // reflectivity padding (16 byte alignment).
  float          reflectivity[3];    // reflectivity vector.
  unsigned char  padding1[4];        // reflectivity padding (8 byte packing).
  float          bumpmap_scale;      // Bumpmap scale.
  int            image_format;       // High resolution image format.
  unsigned char  mipmap_count;       // Number of mipmaps.
  unsigned int   low_image_format;   // Low resolution image format (always DXT1).
  unsigned char  low_width;          // Low resolution image width.
  unsigned char  low_height;         // Low resolution image height.
  unsigned short depth;              // Depth of the largest mipmap in pixels.
                                     // Must be a power of 2. Can be 0 or 1 for a 2D texture (v7.2 only).
  // 7.3+
  unsigned char   padding2[3];       // depth padding (4 byte alignment).
  unsigned int    numResources;      // Number of resources this vtf has. The max appears to be 32.
  unsigned char   padding3[8];       // Necessary on certain compilers
} vtf_header_t;

typedef struct
{
  unsigned char   tag[3];            // A three-byte "tag" that identifies what this resource is.
  unsigned char   flags;             // Resource entry flags. The only known flag is 0x2,
                                     //   which indicates that no data chunk corresponds to this resource.
  unsigned int    offset;            // The offset of this resource's data in the file.
                                     //   If type is CRC, the value is embedded into this field.
} vtf_resurce_entry_t;

static const char* format_to_name(int format)
{
  switch(format) {
    case IMAGE_FORMAT_RGBA8888:
      return "RGBA8888";
    case IMAGE_FORMAT_ABGR8888:
      return "ABGR8888";
    case IMAGE_FORMAT_RGB888:
      return "RGB888";
    case IMAGE_FORMAT_BGR888:
      return "BGR888";
    case IMAGE_FORMAT_RGB565:
      return "RGB565";
    case IMAGE_FORMAT_I8:
      return "I8";
    case IMAGE_FORMAT_IA88:
      return "IA88";
    case IMAGE_FORMAT_P8:
      return "P8";
    case IMAGE_FORMAT_A8:
      return "A8";
    case IMAGE_FORMAT_RGB888_BLUESCREEN:
      return "RGB888_BLUESCREEN";
    case IMAGE_FORMAT_BGR888_BLUESCREEN:
      return "BGR888_BLUESCREEN";
    case IMAGE_FORMAT_ARGB8888:
      return "ARGB8888";
    case IMAGE_FORMAT_BGRA8888:
      return "BGRA8888";
    case IMAGE_FORMAT_DXT1:
      return "DXT1";
    case IMAGE_FORMAT_DXT3:
      return "DXT3";
    case IMAGE_FORMAT_DXT5:
      return "DXT5";
    case IMAGE_FORMAT_BGRX8888:
      return "BGRX8888";
    case IMAGE_FORMAT_BGR565:
      return "BGR565";
    case IMAGE_FORMAT_BGRX5551:
      return "BGRX5551";
    case IMAGE_FORMAT_BGRA4444:
      return "BGRA4444";
    case IMAGE_FORMAT_DXT1_ONEBITALPHA:
      return "DXT1_ONEBITALPHA";
    case IMAGE_FORMAT_BGRA5551:
      return "BGRA5551";
    case IMAGE_FORMAT_UV88:
      return "UV88";
    case IMAGE_FORMAT_UVWQ8888:
      return "UVWQ8888";
    case IMAGE_FORMAT_RGBA16161616F:
      return "RGBA16161616F";
    case IMAGE_FORMAT_RGBA16161616:
      return "RGBA16161616";
    case IMAGE_FORMAT_UVLX8888:
      return "UVLX8888";
  }
  return "unknown";
}

static void decode_rgba(vtf_header_t* header, uint8_t *image_end, int frame_offset, uint8_t** rgba_rows)
{
  int has_alpha;
  switch(header->image_format) {
    case IMAGE_FORMAT_RGBA8888:
    case IMAGE_FORMAT_ARGB8888:
    case IMAGE_FORMAT_ABGR8888:
    case IMAGE_FORMAT_BGRA8888:
      has_alpha = 1;
      break;
    default:
      has_alpha = 0;
      break;
  }

  int framesize = header->width * header->height * (has_alpha ? 4 : 3);
  uint8_t *data = image_end - (framesize*frame_offset);
  int pos = 0;

  uint8_t r,g,b,a;

  for(int y = 0; y < header->height; ++y) {
    for(int x = 0; x < header->width; ++x) {
      switch(header->image_format) {
        case IMAGE_FORMAT_RGBA8888:
          r = data[pos++];
          g = data[pos++];
          b = data[pos++];
          a = data[pos++];
          break;
        case IMAGE_FORMAT_ARGB8888:
          a = data[pos++];
          r = data[pos++];
          g = data[pos++];
          b = data[pos++];
          break;
        case IMAGE_FORMAT_ABGR8888:
          a = data[pos++];
          b = data[pos++];
          g = data[pos++];
          r = data[pos++];
          break;
        case IMAGE_FORMAT_BGRA8888:
          b = data[pos++];
          g = data[pos++];
          r = data[pos++];
          a = data[pos++];
          break;
        case IMAGE_FORMAT_RGB888:
          r = data[pos++];
          g = data[pos++];
          b = data[pos++];
          a = 255;
          break;
        case IMAGE_FORMAT_BGR888:
          b = data[pos++];
          g = data[pos++];
          r = data[pos++];
          a = 255;
          break;
        default:
          r = 255;
          g = 255;
          b = 255;
          a = 255;
      }
      rgba_rows[y][4*x+0] = r;
      rgba_rows[y][4*x+1] = g;
      rgba_rows[y][4*x+2] = b;
      rgba_rows[y][4*x+3] = a;
    }
  }
}

static void rgb565_to_rgb888(uint16_t in, uint8_t *out)
{
  uint8_t r,g,b;
  r = (uint8_t) (in >> 11) & 31;
  r = (r << 3) | (r >> 2);
  g = (uint8_t) (in >>  5) & 63;
  g = (g << 2) | (g >> 4);
  b = (uint8_t) (in >>  0) & 31;
  b = (b << 3) | (b >> 2);
  out[0] = r;
  out[1] = g;
  out[2] = b;
}

//Decode the color information and apply it a 4x4 block of rows
static void decode_dxt_colors(int x, int y, uint16_t c0, uint16_t c1, uint32_t ci, uint8_t** rgba_rows)
{
  uint8_t c888[3], r[4], g[4], b[4];

  rgb565_to_rgb888(c0, &c888[0]);
  r[0] = c888[0];
  g[0] = c888[1];
  b[0] = c888[2];

  rgb565_to_rgb888(c1, &c888[0]);
  r[1] = c888[0];
  g[1] = c888[1];
  b[1] = c888[2];

  //and calculate the other two
  r[2] = (4*r[0] + 2*r[1] + 3)/6;
  g[2] = (4*g[0] + 2*g[1] + 3)/6;
  b[2] = (4*b[0] + 2*b[1] + 3)/6;
  r[3] = (2*r[0] + 4*r[1] + 3)/6;
  g[3] = (2*g[0] + 4*g[1] + 3)/6;
  b[3] = (2*b[0] + 4*b[1] + 3)/6;

  //Insert the colour data
  for(int yo = 0; yo < 4; ++yo) {
    for(int xo = 0; xo < 4; ++xo) {
      rgba_rows[y+yo][4*x+4*xo+0] = r[ci & 3];
      rgba_rows[y+yo][4*x+4*xo+1] = g[ci & 3];
      rgba_rows[y+yo][4*x+4*xo+2] = b[ci & 3];
      ci >>= 2;
    }
  }
}

static void decode_dxt1(vtf_header_t* header, uint8_t *image_end, int frame_offset, uint8_t** rgba_rows)
{
  int framesize = ((header->width+3)/4) * ((header->height+3)/4) * (64/8);
  uint8_t *data = image_end - (framesize*frame_offset);
  int pos = 0;

  uint16_t c0, c1; //packed color values
  uint32_t ci; //color indices

  for(int y = 0; y < header->height; y += 4) {
    for(int x = 0; x < header->width; x += 4) {
      //Unpack the colour information
      c0 = data[pos++];
      c0 |= data[pos++] << 8;
      c1 = data[pos++];
      c1 |= data[pos++] << 8;
      ci = 0;
      for(int i = 0; i <= 24; i += 8)
        ci |= (uint64_t)data[pos++] << i;
      //Apply the colour information
      decode_dxt_colors(x, y, c0, c1, ci, rgba_rows);

      //Set alpha channel to fully opaque
      for(int yo = 0; yo < 4; ++yo)
        for(int xo = 0; xo < 4; ++xo)
          rgba_rows[y+yo][4*x+4*xo+3] = 255;
    }
  }
}

static void decode_dxt3(vtf_header_t* header, uint8_t *image_end, int frame_offset, uint8_t** rgba_rows)
{
  int framesize = ((header->width+3)/4) * ((header->height+3)/4) * (128/8);
  uint8_t *data = image_end - (framesize*frame_offset);
  int pos = 0;

  uint16_t c0, c1; //packed color values
  uint32_t ci; //color indices
  uint64_t al;

  for(int y = 0; y < header->height; y += 4) {
    for(int x = 0; x < header->width; x += 4) {

      //Unpack the alpha data
      al = 0;
      for(int i = 0; i <= 48; i += 8)
        al |= (uint64_t)data[pos++] << i;

      //Unpack the colour information
      c0 = data[pos++];
      c0 |= data[pos++] << 8;
      c1 = data[pos++];
      c1 |= data[pos++] << 8;
      ci = 0;
      for(int i = 0; i <= 24; i += 8)
        ci |= (uint64_t)data[pos++] << i;
      //Apply the colour information
      decode_dxt_colors(x, y, c0, c1, ci, rgba_rows);

      //Set alpha using the unpacked data
      for(int yo = 0; yo < 4; ++yo) {
        for(int xo = 0; xo < 4; ++xo) {
          rgba_rows[y+yo][4*x+4*xo+3] = al & 15;
          al >>= 4;
        }
      }
    }
  }
}

static void decode_dxt5(vtf_header_t* header, uint8_t *image_end, int frame_offset, uint8_t** rgba_rows)
{
  int framesize = ((header->width+3)/4) * ((header->height+3)/4) * (128/8);
  uint8_t *data = image_end - (framesize*frame_offset);
  int pos = 0;

  uint8_t a[8]; //alpha values
  uint64_t ai; //alpha indices
  uint16_t c0, c1; //packed color values
  uint32_t ci; //color indices

  for(int y = 0; y < header->height; y += 4) {
    for(int x = 0; x < header->width; x += 4) {
      //First calculate our alpha values
      a[0] = data[pos++];
      a[1] = data[pos++];
      if(a[0] > a[1]) {
        for(int i = 0; i < 6; ++i)
          a[2+i] = ((12-(2*i))*a[0] + (2+2*i)*a[1] + 7) / 14;
      } else {
        for(int i = 0; i < 4; ++i)
          a[2+i] = ((8-(2*i))*a[0] + (2+2*i)*a[1] + 5) / 10;
        a[6] = 0; a[7] = 255;
      }

      //Now extract the alpha indices
      ai = 0;
      for(int i = 0; i <= 40; i += 8)
        ai |= (uint64_t)data[pos++] << i;

      //Apply the alpha information to this block
      for(int yo = 0; yo < 4; ++yo) {
        for(int xo = 0; xo < 4; ++xo) {
          rgba_rows[y+yo][4*x+4*xo+3] = a[ai & 7];
          ai >>= 3;
        }
      }

      //Unpack the colour information
      c0 = data[pos++];
      c0 |= data[pos++] << 8;
      c1 = data[pos++];
      c1 |= data[pos++] << 8;
      ci = 0;
      for(int i = 0; i <= 24; i += 8)
        ci |= (uint64_t)data[pos++] << i;

      //Apply the colour information
      decode_dxt_colors(x, y, c0, c1, ci, rgba_rows);
    }
  }
}

struct options {
  char* in_path;
  char* out_path;
  int frame;
  int verbose;
};

static int arg_parse_opt(int key, char* arg, struct argp_state *state)
{
  struct options *options = state->input;
  switch(key) {
    case ARGP_KEY_INIT:
      options->in_path = NULL;
      options->out_path = NULL;
      options->frame = 1;
      options->verbose = 0;
      break;
    case 'f':
      options->frame = strtol(arg, NULL, 10);
      break;
    case 'v':
      options->verbose = 1;
      break;
    case ARGP_KEY_ARG:
      switch(state->arg_num) {
        case 0:
          options->in_path = arg;
          break;
        case 1:
          options->out_path = arg;
          break;
        default:
          fprintf(stderr, "Too many arguments.\n");
          argp_usage(state);
      }
      break;
    case ARGP_KEY_END:
      if(state->arg_num < 2) {
        fprintf(stderr, "Insufficient arguments.\n");
        argp_usage(state);
      }
      break;
    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}


int main(int argc, char** argv)
{

  struct argp_option arg_options[] = {
    { "frame", 'f', "FRAME", 0, "Output a specific frame", 0},
    { "verbose", 'v', 0, 0, "Verbose output", 0},
    {0},
  };

  struct argp argp = {arg_options, arg_parse_opt, "INPUT.vtf OUTPUT.png", 0, 0, 0, 0};

  struct options options;
  argp_parse(&argp, argc, argv, 0, 0, &options);

  int fd = open(options.in_path, O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "Unable to open \"%s\"\n", options.in_path);
    return 1;
  }

  int filesize = lseek(fd, 0, SEEK_END);
  uint8_t* filedata = mmap(0, filesize, PROT_READ, MAP_PRIVATE, fd, 0);
  vtf_header_t* header = (vtf_header_t*)filedata;

  if(strncmp(&header->signature[0], "VTF", 4) != 0
     || header->image_format == IMAGE_FORMAT_NONE) {
    fprintf(stderr, "Input file is invalid\n");
    return 1;
  }

  if(options.verbose) {
    printf("version: %i.%i\n"
        "width: %i\n"
        "height: %i\n"
        "frames: %i\n"
        "mipmaps: %i\n"
        "format: %s\n",
        header->version[0], header->version[1],
        header->width,
        header->height,
        header->frames,
        header->mipmap_count,
        format_to_name(header->image_format));
  }

  if(header->version[0] > 7 || header->version[1] > 5) {
    fprintf(stderr, "Unsupported VTF file version: %i.%i\n",
        header->version[0], header->version[1]);
    return 1;
  }

  //Validate the chosen frame and calculate the offset
  if(options.frame < 0 || options.frame > header->frames) {
    fprintf(stderr, "Invalid frame number: %i\n", options.frame);
    return 1;
  }

  //Image position depends on VTF version
  //We need the end of the high resolution image, because mipmaps are stored in order of increasing size
  int image_end = 0;
  if(header->version[1] > 2) {
    enum {
      FOUND_NOTHING,
      FOUND_IMAGEDATA,
      FOUND_END
    } search_state = FOUND_NOTHING;
    vtf_resurce_entry_t *resources = (vtf_resurce_entry_t*)(filedata+sizeof(vtf_header_t));
    for(unsigned i=0; i<header->numResources; i++) {
        switch(search_state) {
          case FOUND_NOTHING:
            if (resources[i].tag[0] == 0x30) {
              search_state = FOUND_IMAGEDATA;
            }
            break;
          case FOUND_IMAGEDATA:
            //CRC is embedded in the offset field
            if (resources[i].tag[0] != 'C') {
              image_end = resources[i].offset;
              search_state = FOUND_END;
            }
            break;
          case FOUND_END:
            break;
        }
    }
    if(search_state == FOUND_NOTHING) {
      fprintf(stderr, "VTF file doesn't contain High-res image data !?!\n");
      return 1;
    } else if(search_state == FOUND_IMAGEDATA) {
      //High-res image data was the last resource with an offset
      image_end = filesize;
    }
  } else {
    //Up until v7.2 the end of the image data is at the end of the file
    image_end = filesize;
  }

  if(header->frames > 1) {
    printf("VTF file has %u frames.\n", header->frames);
  }

  //Which frame we want to look at, 1 being the last frame, 2 the previous, etc.
  int frame_offset = 1 + header->frames - options.frame;

  //Array of pointers to each row array
  uint8_t** rgba_rows = malloc(sizeof(uint8_t*)*header->height);
  for(int i = 0; i < header->height; ++i) {
    rgba_rows[i] = malloc(header->width * 4);
    memset(rgba_rows[i], 0xFF, header->width * 4);
  }

  switch(header->image_format) {
    case IMAGE_FORMAT_RGBA8888:
    case IMAGE_FORMAT_ARGB8888:
    case IMAGE_FORMAT_ABGR8888:
    case IMAGE_FORMAT_BGRA8888:
    case IMAGE_FORMAT_RGB888:
    case IMAGE_FORMAT_BGR888:
      decode_rgba(header, filedata+image_end, frame_offset, rgba_rows);
      break;
    case IMAGE_FORMAT_DXT1:
      decode_dxt1(header, filedata+image_end, frame_offset, rgba_rows);
      break;
    case IMAGE_FORMAT_DXT3:
      decode_dxt3(header, filedata+image_end, frame_offset, rgba_rows);
      break;
    case IMAGE_FORMAT_DXT5:
      decode_dxt5(header, filedata+image_end, frame_offset, rgba_rows);
      break;
    default:
      fprintf(stderr, "Unsupported format: %s\n", format_to_name(header->image_format));
      return 1;
  }

  //We should now have valid pixel data, so write a png
  FILE* of = fopen(options.out_path, "wb");
  if(!of) {
    fprintf(stderr, "Could not open \"%s\" for writing\n", options.out_path);
    return 1;
  }

  png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  png_infop info_ptr = png_create_info_struct(png_ptr);
  png_init_io(png_ptr, of);

  png_set_IHDR(png_ptr, info_ptr, header->width, header->height, 8,
      PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
      PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
  png_write_info(png_ptr, info_ptr);
  png_write_image(png_ptr, rgba_rows);
  png_write_end(png_ptr, NULL);

  fclose(of);

  for(int i = 0; i < header->height; ++i)
    free(rgba_rows[i]);
  free(rgba_rows);

  munmap(filedata, filesize);
  close(fd);

  return 0;
}
