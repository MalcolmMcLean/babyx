/*
   Generate palettes with a colourscheme
   
   by Malcolm Mclean
*/

#include <math.h>
#include "colourschemes.h"

#define lerp(a, b, t)  ( (a) + ( (b) - (a) ) * (t) )
 

static double jetmap[64*3] =
{
         0,    0.0116,    1.0000,
         0,    0.0909,    1.0000,
         0,    0.1791,    1.0000,
         0,    0.2667,    1.0000,
         0,    0.3472,    1.0000,
         0,    0.4196,    1.0000,
         0,    0.4849,    1.0000,
         0,    0.5404,    1.0000,
         0,    0.5878,    1.0000,
         0,    0.6292,    1.0000,
         0,    0.6658,    1.0000,
         0,    0.6988,    1.0000,
         0,    0.7290,    1.0000,
         0,    0.7574,    1.0000,
         0,    0.7846,    1.0000,
         0,    0.8110,    1.0000,
         0,    0.8370,    1.0000,
         0,    0.8628,    1.0000,
         0,    0.8885,    1.0000,
         0,    0.9145,    1.0000,
         0,    0.9410,    1.0000,
         0,    0.9687,    1.0000,
    0.0005,    0.9975,    0.9995,
    0.0298,    1.0000,    0.9702,
    0.0653,    1.0000,    0.9347,
    0.1055,    1.0000,    0.8945,
    0.1524,    1.0000,    0.8476,
    0.2094,    1.0000,    0.7906,
    0.2787,    1.0000,    0.7213,
    0.3565,    1.0000,    0.6435,
    0.4364,    1.0000,    0.5636,
    0.5154,    1.0000,    0.4846,
    0.5903,    1.0000,    0.4097,
    0.6582,    1.0000,    0.3418,
    0.7183,    1.0000,    0.2817,
    0.7714,    1.0000,    0.2286,
    0.8176,    1.0000,    0.1824,
    0.8579,    1.0000,    0.1421,
    0.8936,    1.0000,    0.1064,
    0.9254,    1.0000,    0.0746,
    0.9540,    1.0000,    0.0460,
    0.9800,    1.0000,    0.0200,
    1.0000,    0.9963,         0,
    1.0000,    0.9743,         0,
    1.0000,    0.9537,         0,
    1.0000,    0.9344,         0,
    1.0000,    0.9162,         0,
    1.0000,    0.8987,         0,
    1.0000,    0.8821,         0,
    1.0000,    0.8659,         0,
    1.0000,    0.8500,         0,
    1.0000,    0.8343,         0,
    1.0000,    0.8186,         0,
    1.0000,    0.8025,         0,
    1.0000,    0.7859,         0,
    1.0000,    0.7683,         0,
    1.0000,    0.7491,         0,
    1.0000,    0.7276,         0,
    1.0000,    0.7026,         0,
    1.0000,    0.6728,         0,
    1.0000,    0.6363,         0,
    1.0000,    0.5915,         0,
    1.0000,    0.5346,         0,
    1.0000,    0.4602,         0, };

void createjet(unsigned char *rgb, int N)
{
  int i;
  double t;
  double red, green, blue;
  int low, high;
  
  for(i=0;i<N;i++)
  {
    t = ((double) i)/(N-1);
    low = (int) floor(t * 63);
    high = (int) ceil(t * 63);
    if(low == high)
    {
      red = jetmap[low*3];
      green = jetmap[low*3+1];
      blue = jetmap[low*3+2];
    }
    else
    {
      red = lerp(jetmap[low*3], jetmap[high*3], t * 63.0 - low);
      green = lerp(jetmap[low*3+1], jetmap[high*3+1], t * 63.0 - low);
      blue = lerp(jetmap[low*3+2], jetmap[high*3+2], t * 63.0 - low);
    }
    rgb[i*3] = (unsigned char ) (red * 255);
    rgb[i*3+1] = (unsigned char) (green * 255);
    rgb[i*3+2] = (unsigned char) (blue * 255); 
  }
}

void creategrey(unsigned char *rgb, int N)
{
  int i;
  for(i=0;i<N;i++)
  {
    rgb[i*3] = (i * 255)/(N-1);
    rgb[i*3+1] = rgb[i*3];
    rgb[i*3+2] = rgb[i*3];
  }
}


void createredgreen(unsigned char *rgb, int N)
{
  int i;
  
  for(i=0;i<N/2;i++)
  {
	rgb[i*3+0] = ( (N/2-i) * 255)/(N/2);
	rgb[i*3+1] = 0;
	rgb[i*3+2] = 0;
  }
  for(i=N/2;i<N;i++)
  {
	rgb[i*3+0] = 0;
	rgb[i*3+1] = ( (i - N/2) * 255)/(N/2);
	rgb[i*3+2] = 0;
  }
  
}

void createblueyellow(unsigned char *rgb, int N)
{
  int i;
  
  for(i=0;i<N/2;i++)
  {
	rgb[i*3+0] = 0;
	rgb[i*3+1] = 0;
	rgb[i*3+2] = ( (N/2-i) * 255)/(N/2);
  }
  for(i=N/2;i<N;i++)
  {
	rgb[i*3+0] = ( (i - N/2) * 255)/(N/2);
	rgb[i*3+1] = ( (i - N/2) * 255)/(N/2);
	rgb[i*3+2] = 0;
  }
  
}

void createflame(unsigned char *rgb, int N)
{
  int i;
  double red, green, blue;
  
  for(i=0;i<N;i++)
  {
    red = (255.0 * i) /(N/3.0);
    if(red > 255.0)
      red = 255.0;
    green = (255.0 * (i - N/3.0) ) / (N/3.0);
    if(green < 0.0)
      green = 0.0;
    if(green > 255.0)
       green = 255.0;
    blue = (255.0 * (i - 2*N/3.0) ) / (N/3.0);
    if(blue < 0.0)
      blue = 0.0;
    if(blue > 255.0)
      blue = 255.0;
      
    rgb[i*3+0] = (unsigned char) red;
    rgb[i*3+1] = (unsigned char) green;
    rgb[i*3+2] = (unsigned char) blue;
  }
}

static double rainbowmap[7*3] =
{
	1.0, 0.0, 0.0,
	1.0, 0.5, 0.0,
	1.0, 1.0, 0.0,
	0.0, 1.0, 0.0,
	0.0, 0.0, 1.0,
	75/255.0, 0.0 ,130/255.0,
	138/255.0, 	43/255.0, 	226/255.0,
	 
};




void createrainbow(unsigned char *rgb, int N)
{
  int i;
  int low, high;
  double red, green, blue;
  double t;
  
  for(i=0;i<N;i++)
  {
    t = ((double) i)/(N-1);
    low = (int) floor(t * 6);
    high = (int) ceil(t * 6);
    if(low == high)
    {
      red = rainbowmap[low*3];
      green = rainbowmap[low*3+1];
      blue = rainbowmap[low*3+2];
    }
    else
    {
      red = lerp(rainbowmap[low*3], rainbowmap[high*3], t * 6.0 - low);
      green = lerp(rainbowmap[low*3+1], rainbowmap[high*3+1], t * 6.0 - low);
      blue = lerp(rainbowmap[low*3+2], rainbowmap[high*3+2], t * 6.0 - low);
    }
    rgb[i*3] = (unsigned char ) (red * 255);
    rgb[i*3+1] = (unsigned char) (green * 255);
    rgb[i*3+2] = (unsigned char) (blue * 255); 
  }
}

static double landseamap[29 * 3] =
{
  245,244, 242,
  224,222, 216,
  202,195, 184,
  186,174, 154,
  172,154, 124,
  170,135, 83,
  185,152, 90,
  195,167, 107,
  202,185, 130,
  211,202, 157,
  222, 214, 163,
  232, 225, 182,
  239, 235, 192,
  225, 228, 181,
  209, 215, 171,
  189, 204, 150,
  168, 198, 143,
  148, 191, 139,
  172, 208, 165,
  216, 242, 254,
  198, 236, 255,
  185, 227, 255,
  172, 219, 251,
  161, 210, 247,
  150, 201, 240,
  141, 193, 234,
  132, 185, 227,
  121, 178, 222,
  113, 171, 216,
};

void createlandsea(unsigned char *rgb, int N)
{
  int i;
  int low, high;
  double red, green, blue;
  double t;
  
  for(i=0;i<N;i++)
  {
    t = ((double) i)/(N-1);
    low = (int) floor(t * 28);
    high = (int) ceil(t * 28);
    if(low == high)
    {
      red = landseamap[low*3];
      green = landseamap[low*3+1];
      blue = landseamap[low*3+2];
    }
    else
    {
      red = lerp(landseamap[low*3], landseamap[high*3], t * 28.0 - low);
      green = lerp(landseamap[low*3+1], landseamap[high*3+1], t * 28.0 - low);
      blue = lerp(landseamap[low*3+2], landseamap[high*3+2], t * 28.0 - low);
    }
    rgb[i*3] = (unsigned char ) (red );
    rgb[i*3+1] = (unsigned char) (green );
    rgb[i*3+2] = (unsigned char) (blue ); 
  }
  invertpalette(rgb, N);
}

void createrelief(unsigned char *rgb, int N)
{
  int i;
  int low, high;
  double red, green, blue;
  double t;
  
  for(i=0;i<N;i++)
  {
    t = ((double) i)/(N-1);
    low = (int) floor(t * 18);
    high = (int) ceil(t * 18);
    if(low == high)
    {
      red = landseamap[low*3];
      green = landseamap[low*3+1];
      blue = landseamap[low*3+2];
    }
    else
    {
      red = lerp(landseamap[low*3], landseamap[high*3], t * 18.0 - low);
      green = lerp(landseamap[low*3+1], landseamap[high*3+1], t * 18.0 - low);
      blue = lerp(landseamap[low*3+2], landseamap[high*3+2], t * 18.0 - low);
    }
    rgb[i*3] = (unsigned char ) (red );
    rgb[i*3+1] = (unsigned char) (green );
    rgb[i*3+2] = (unsigned char) (blue ); 
  }
  invertpalette(rgb, N);
}

void createzebra(unsigned char *rgb, int N)
{
  int i;
  
  for(i=0;i<N;i++)
  {
    if( (i % 2) == 0)
    {
	  rgb[i*3+0] = 0;
	  rgb[i*3+1] = 0;
	  rgb[i*3+2] = 0;
    }
    else
    {
	  rgb[i*3+0] = 255;
	  rgb[i*3+1] = 255;
	  rgb[i*3+2] = 255;
    }
  }
  
}

void createunionjack(unsigned char *rgb, int N)
{
  int i;
  double red, green, blue;
  double t;
  
  for(i=0;i<N;i++)
  {
    t = i/(N-1.0);
    
    if(t >= 0.5)
      red = 255.0;
    else
      red = lerp(0.0, 255.0, t/0.5);
      
    if(t <= 0.5)
      green = lerp(0, 255.0, t/0.5);
    else
      green = lerp(255.0, 0, (t-0.5)/0.5);  
    
    if(t <= 0.5)
      blue = 255.0;
    else
      blue = lerp(255.0, 0.0, (t-0.5)/0.5);
      
    rgb[i*3+0] = (unsigned char) red;
    rgb[i*3+1] = (unsigned char ) green;
    rgb[i*3+2] = (unsigned char) blue;
      
  }
}

void createocean(unsigned char *rgb, int N)
{
  int i;
  double t;
  double red, green, blue;
  
  for(i=0;i<N;i++)
  {
    t = i/(N-1.0);
    red = lerp(-50.0, 255.0, t);
    green = lerp(-50.0, 255.0, t);
    blue = lerp(0.0, 255.0, t);
    if(red < 0)
      red = 0;
    if(green < 0)
      green = 0;
    rgb[i*3+0] = (unsigned char) red;
    rgb[i*3+1] = (unsigned char) green;
    rgb[i*3+2] = (unsigned char) blue;
  }
}

void createnogreen(unsigned char *rgb, int N)
{
  int i;
  double t;
  double red, green, blue;
  
  for(i=0;i<N;i++)
  {
	 t = i/(N-1.0);
	 red = lerp(512.0, 0.0, t);
	 if(red > 255.0)
	   red = 255.0;
	 if(t <= 0.5)
	 	green = lerp(0, 255.0, t/0.5);
	 else
	   green = lerp(255.0, 0, (t-0.5)/0.5);
	 if(t >= 0.5)
	   blue = 255.0;
	 else
	   blue = 0;
	   
	rgb[i*3+0] = (unsigned char) red;
	rgb[i*3+1] = (unsigned char) green;
	rgb[i*3+2] = (unsigned char) blue;
  }
}

void invertpalette(unsigned char *pal, int N)
{
  int i, j, ii;
  unsigned char temp;
  
  for(i=0;i<N/2;i++)
  {
    j = N-i-1;
    for(ii=0;ii<3;ii++)
    {
      temp = pal[i*3+ii];
      pal[i*3+ii] = pal[j*3+ii];
      pal[j*3+ii] = temp;
    }
  }
}
