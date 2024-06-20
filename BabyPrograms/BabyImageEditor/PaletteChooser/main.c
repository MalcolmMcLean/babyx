//
//  main.c
//  PaletteChooser
//
//  Created by Malcolm McLean on 19/03/2014.
//  Copyright (c) 2014 Malcolm McLean. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>

#include "options.h"
#include "loadtiff.h"
#include "gif.h"
#include "makepalette.h"

static void usage(void)
{
    printf("Palette chooser: chooses a palette using PCA-Otsu algorithm\n");
    printf("Usage: [options] <infile.tiff> <outfile.gif>\n");
    printf("infile.bmp - the input (24 bit tiff file)\n");
    printf("outfile.gif - output file, gif format\n");
    printf("options: -nc <N> - number of colours [default 256]\n");
    exit(EXIT_FAILURE);
}


static unsigned char *rgbatorgb(unsigned char *rgba, int width, int height)
{
    unsigned char *rgb;
    int i;
    
    rgb = malloc(width * height *3);
    if (!rgb)
        return 0;
    for (i = 0; i < width * height; i++)
    {
        rgb[i*3] = rgba[i*4];
        rgb[i*3+1] = rgba[i*4+1];
        rgb[i*3+2] = rgba[i*4+2];
    }
    
    return rgb;
}
int main(int argc, const char * argv[])
{

    OPTIONS *opt;
    char *infile;
    char *outfile;
    int Ncolours = 256;
    unsigned char *rgba;
    unsigned char *rgb;
    unsigned char *indexed;
    int width, height;
    unsigned char pal[256*3];
    int err;
    
    opt = options(argc, (char **) argv, "");
    opt_get(opt, "-nc", "%d", &Ncolours);
    if(opt_Nargs(opt) != 2)
        usage();
    infile = opt_arg(opt, 0);
    outfile = opt_arg(opt, 1);
    if(opt_error(opt, stderr))
        exit(EXIT_FAILURE);
    killoptions(opt);
    
    if(Ncolours < 2 || Ncolours > 256)
    {
        fprintf(stderr, "Must have between 2 and 256 colours\n");
        exit(EXIT_FAILURE);
    }
    
    FILE *fp = fopen(infile, "r");
    if (!fp)
    {
        fprintf(stderr, "Can't open %s\n", infile);
        exit(EXIT_FAILURE);
    }
    rgba = floadtiff(fp, &width, &height);
    if(!rgba)
    {
        fprintf(stderr, "Error loading %s\n", infile);
        exit(EXIT_FAILURE);
    }
    fclose(fp);
    rgb = rgbatorgb(rgba, width, height);
    if (!rgb)
    {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }
    free(rgba);
    rgba = 0;
    
    makepalette(rgb, width, height, pal, Ncolours);
    indexed = rgbtoindex(rgb, width, height, pal, Ncolours);
    err = savegif(outfile, indexed, width, height, pal, Ncolours, -1, 0, 0);
    if(err)
    {
        fprintf(stderr, "Error saving %s\n", outfile);
        exit(EXIT_FAILURE);
    }
    
    return 0;
}

