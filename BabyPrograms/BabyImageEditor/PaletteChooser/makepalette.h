//
//  makepalette.h
//  PaletteChooser
//
//  Created by Malcolm McLean on 19/03/2014.
//  Copyright (c) 2014 Malcolm McLean. All rights reserved.
//

#ifndef PaletteChooser_makepalette_h
#define PaletteChooser_makepalette_h


unsigned char *rgbtoindex(unsigned char *rgb, int width, int height, unsigned char *pal, int N);
int makepalette(unsigned char *rgb, int width, int height, unsigned char *pal, int N);

#endif
