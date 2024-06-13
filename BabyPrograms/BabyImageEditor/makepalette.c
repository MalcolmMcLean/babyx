//
//  makepalette.c
//  BabyImageEditor
//
//  Created by Malcolm McLean on 13/06/2024.
//

#include "makepalette.h"

int makepalette(unsigned char *pal, int Npal, unsigned char *rgba, int width, int height)
{
    if (Npal == 256)
    {
        int red, green, blue;
        int i = 0;
        
        for (red = 0 ; red < 8; red++)
        {
            for (green = 0; green < 8; green++)
            {
                for (blue = 0; blue < 4; blue++)
                {
                    pal[i * 3] = red * 32;
                    pal[i * 3 + 1] = green * 32;
                    pal[i * 3 + 2] = blue * 64;
                    i++;
                }
            }
        }
        return  0;
        
    }
    return -1;
}
