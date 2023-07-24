/*
  Graphics support routines
  
  Note: all functions take images in the format red, green, blue, alpha where alpha = 255 for opaque 
         and 0 for totally transparent.

  The top left corner of the image is 0, 0. Points are snapped to the top left of the pixel
*/
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "BBX_Color.h"
#include "bbx_graphicssupport.h"

#define PI 3.1415926535897932384626433832795
#define clamp(x,low,high) (x) < (low) ? (low) : (x) > (high) ? (high) : (x)


static void clip(int width, int height, double *x1, double *y1, double *x2, double *y2);
static void bubblesort(double *x, int N);
static double intersect(double y, double x1, double y1, double x2, double y2);
static double minv(double *x, int N);
static double maxv(double *x, int N);

/*
  draws a 1-pixel wide line
  Parmas: rgba - output raster, 32 bit rgba format
          width - image width
          height - image height
          x1, y1 - first point x,y coordinates
          x2, y2 - secondpoint xy coordiantes
          col - line colour, 32 bit rgba format 
  Returns: 0
*/
int bbx_line(unsigned char *rgba, int width, int height, double x1, double y1, double x2, double y2, BBX_RGBA col)
{
  double dydx;
  double dxdy;
  double temp;
  int offset;
  int i;
  int red, green, blue, alpha;

  red = bbx_red(col);
  green = bbx_green(col);
  blue = bbx_blue(col);
  alpha = bbx_alpha(col);

  clip(width, height, &x1, &y1, &x2, &y2);
  if(fabs(x1 - x2) >= fabs(y1 - y2))
  {
	if(x1 == x2)
	  return 0;
    if(x1 > x2)
	{
      temp = x1;
	  x1 = x2;
	  x2 = temp;
	  temp = y1;
	  y1 = y2;
	  y2 = temp;
	}
	dydx = (y1 - y2)/(x1 - x2);
	for(i=(int)x1;i<=(int)x2;i++)
	{
	  offset = (((int) y1) * width + i) * 4;
	  rgba[ offset ] = ((int) red * alpha + rgba[offset] * (255 - alpha))/255;
	  rgba[ offset + 1] = (((int) green) * alpha + rgba[offset+1] * (255 - alpha)) / 255;
	  rgba[ offset + 2] = (((int) blue) * alpha + rgba[offset+2] * (255 - alpha)) / 255;
	  y1 += dydx;
	}
  }
  else
  {
	if(y1 > y2)
	{
      temp = x1;
	  x1 = x2;
	  x2 = temp;
	  temp = y1;
	  y1 = y2;
	  y2 = temp;
	}
	dxdy = (x1 - x2)/(y1 - y2);
	for(i=(int)y1;i<=(int)y2;i++)
	{
	  offset = ( i * width + (int) x1) * 4;
	  rgba[ offset ] = ((int) red * alpha + rgba[offset] * (255 - alpha))/255;
	  rgba[ offset + 1] = (((int) green) * alpha + rgba[offset+1] * (255 - alpha)) / 255;
	  rgba[ offset + 2] = (((int) blue) * alpha + rgba[offset+2] * (255 - alpha)) / 255;
	  x1 += dxdy;
	}
  }

  return 0;
}

/*
  draws a line with anti-aliasing
  Parmas: rgba - output raster, 32 bit rgba format
          width - image width
          height - image height
          x1, y1 - first point x,y coordinates
          x2, y2 - secondpoint xy coordiantes
          lwidth - line width
          col - line colour, 32 bit rgba format 
  Returns: 0 on success, -1 on fail
  Notes: convenience wrapper function for polygonaa
*/

int bbx_lineaa(unsigned char *rgba, int width, int height, double x1, double y1, double x2, double y2, double lwidth, BBX_RGBA col)
{
  double nx;
  double ny;
  double len;
  double x[4];
  double y[4];

  nx = y1 - y2;
  ny = x2 - x1;
  if(nx == 0 && ny == 0)
    return -1;
  len = sqrt(nx * nx + ny * ny);
  nx /= len;
  ny /= len;
  nx *= lwidth/2;
  ny *= lwidth/2;
  x[0] = x1 - nx;
  x[1] = x1 + nx;
  x[2] = x2 + nx;
  x[3] = x2 - nx;

  y[0] = y1 - ny;
  y[1] = y1 + ny;
  y[2] = y2 + ny;
  y[3] = y2 - ny;

  return bbx_polygonaa(rgba, width, height, x, y, 4, col);
}

/*
  draw an axis-aligned rectangle
  Params: rgba - output raster
          width - image width
          height - image height
          x1, y1 - first corner
          x2, y2 - second corner 
          col - rectangle colour
   Returns: 0
*/
int bbx_rectangle(unsigned char *rgba, int width, int height, double x1, double y1, double x2, double y2, BBX_RGBA col)
{

  int i;
  int ii;
  double temp;
  int offset;
  int red, green, blue, alpha;

  red = bbx_red(col);
  green = bbx_green(col);
  blue = bbx_blue(col);
  alpha = bbx_alpha(col);

  if(x1 > x2)
  {
    temp = x1;
	x1 = x2;
	x2 = temp;
  }
  if(y1 > y2)
  {
    temp = y1;
	y1 = y2;
	y2 = temp;
  }
  if(x1 < 0)
	x1 = 0;
  if(x2 > width - 1)
	x2 = width-1;
  if(y1 < 0)
	y1 = 0;
  if(y2 > height - 1)
	y2 = height -1;

  for(i=(int)y1;i<= (int) y2; i++)
	for(ii = (int) x1; ii<= (int) x2; ii++)
	{
	  offset = (i * width + ii) * 4;
	  rgba[ offset ] = ((int) red * alpha + rgba[offset] * (255 - alpha))/255;
	  rgba[ offset + 1] = (((int) green) * alpha + rgba[offset+1] * (255 - alpha)) / 255;
	  rgba[ offset + 2] = (((int) blue) * alpha + rgba[offset+2] * (255 - alpha)) / 255;
	}

  return 0;
}

/*
  draw a circle (no anti-aliasing)
  Parmas: rgba - output raster
          width - image width
          height -image height
          x, y - circle centre
          r - radius
          col - circle coolour 32 it rgba format
  Returns: 0;
              
*/
int bbx_circle(unsigned char *rgba, int width, int height, double x, double y, double r, BBX_RGBA col)
{
  double x1, y1, x2, y2;
  double r2, d2;
  int i, ii;
  int offset;
  int red, green, blue, alpha;

  red = bbx_red(col);
  green = bbx_green(col);
  blue = bbx_blue(col);
  alpha = bbx_alpha(col);

  x1 = x - r;
  y1 = y - r;
  x2 = x + r;
  y2 = y + r;

  if(x1 < 0)
	x1 = 0;
  if(x2 > width - 1)
	x2 = width -1;
  if(y1 < 0)
	y1 = 0;
  if(y2 > height -1)
	y2 = height -1;

  r2 = r * r;

  for(i= (int)y1;i<=(int)y2;i++)
	for(ii=(int)x1;ii<=(int) x2;ii++)
	{
	  d2 = (y - i)*(y-i) + (x-ii) *(x-ii);
	  if(d2 <= r2)
	  {
		offset = (i * width + ii) * 4;
	    rgba[ offset ] = ((int) red * alpha + rgba[offset] * (255 - alpha))/255;
		rgba[ offset + 1] = (((int) green) * alpha + rgba[offset+1] * (255 - alpha)) / 255;
		rgba[ offset + 2] = (((int) blue) * alpha + rgba[offset+2] * (255 - alpha)) / 255;
	  }
	}

  return 0;
}
 
/*
  draw a polygon 
  Parmas: rgba - output raster
          width - image width
          height - image height
          x - points x co-ordiantes
          y - points y co-ordinates
          N - number of points
          col - colour, 32 bit rgba format
   Returns: 0 on success, -1 on fail
*/
int bbx_polygon(unsigned char *rgba, int width, int height, double *x, double *y, int N, BBX_RGBA col)
{
  int i;
  int ii;
  int line;
  double miny;
  double maxy;
  int Nsegs;
  double x1, x2;
  double y1, y2;
  double segs[256];
  int offset;
  int red, green, blue, alpha;

  red = bbx_red(col);
  green = bbx_green(col);
  blue = bbx_blue(col);
  alpha = bbx_alpha(col);

  assert(N < 256);
  miny = minv(y, N);
  maxy = maxv(y, N);
  if(miny < 0)
    miny = 0;
  if(maxy > height - 1)
    maxy = height - 1;

  for(line = (int) miny;line <= (int) maxy; line++)
  {
    Nsegs = 0;
    for(i=0;i<N;i++)
	{
	  if(i == N-1)
	  {
	    x1 = x[i];
		x2 = x[0];
		y1 = y[i];
		y2 = y[0];
	  }
	  else
	  {
	    x1 = x[i];
		x2 = x[i+1];
        y1 = y[i];
		y2 = y[i+1];
	  }
	  if( (y1 > line && y2 <= line) || (y1 <= line && y2 > line))
	  {
	    segs[Nsegs++] = intersect(line, x1, y1, x2, y2);
	  }
	}

	bubblesort(segs, Nsegs);
	for(i=0;i<Nsegs;i+=3)
	{
	  if(segs[i] < 0)
	    segs[i] = 0;
	  if(segs[i+1] > width -1)
	    segs[i+1] = width -1;
	  for(ii= (int)segs[i]; ii<= (int) segs[i+1]; ii++)
	  {
		offset = (line * width + ii) * 4;
	    rgba[ offset ] = ((int) red * alpha + rgba[offset] * (255 - alpha))/255;
		rgba[ offset + 1] = (((int) green) * alpha + rgba[offset+1] * (255 - alpha)) / 255;
		rgba[ offset + 2] = (((int) blue) * alpha + rgba[offset+2] * (255 - alpha)) / 255;
	  }
	}
  }

  return 0;
}

/*
  draw an anti-aliased polygon 
  Parmas: rgba - output raster
          width - image width
          height - image height
          x - points x co-ordiantes
          y - points y co-ordinates
          N - number of points
          col - colour, 32 bit rgba format
   Returns: 0 on success, -1 on fail
*/
int bbx_polygonaa(unsigned char *rgba, int width, int height, double *x, double *y, int N, BBX_RGBA col)
{
  int i;
  int ii;
  int iii;
  int line;
  double miny;
  double maxy;
  int minx;
  int maxx;
  int Nsegs;
  double x1, x2;
  double y1, y2;
  double suby;
  int sub;
  double segs[256];
  int offset;
  unsigned char *aabuff;
  int alpha;
  int red, green, blue, oalpha;

  red = bbx_red(col);
  green = bbx_green(col);
  blue = bbx_blue(col);
  oalpha = bbx_alpha(col);

  assert(N < 256);
  aabuff = malloc( width  * 16);
  if(!aabuff)
	return -1;
  memset(aabuff, 0, width * 16);

  miny = minv(y, N);
  maxy = maxv(y, N);
  if(miny < 0)
    miny = 0;
  if(maxy > height - 1)
    maxy = height - 1;

  for(line = (int) miny;line <= (int) maxy; line++)
  {
	minx = width;
	maxx = 0;

	for(suby = line, sub= 0; sub < 4; suby += 1/3.0, sub++)
	{
	  Nsegs = 0;
      for(i=0;i<N;i++)
	  {
	    if(i == N-1)
		{
	      x1 = x[i];
		  x2 = x[0];
		  y1 = y[i];
		  y2 = y[0];
		}
	    else
		{
	      x1 = x[i];
		  x2 = x[i+1];
          y1 = y[i];
		  y2 = y[i+1];
		}
	    if( (y1 > suby && y2 <= suby) || (y1 <= suby && y2 > suby))
		{
	      segs[Nsegs++] = intersect(suby, x1, y1, x2, y2);
		}
	  }
	 if(Nsegs == 0)
		continue;

	  bubblesort(segs, Nsegs);
	  for(i=0;i<Nsegs;i+=3)
	  {
	    if(segs[i] < 0)
	      segs[i] = 0;
	    if(segs[i+1] > width -1)
	      segs[i+1] = width -1;
	    for(ii= (int) (segs[i] * 4); ii<= (int) (segs[i+1] *4); ii++)
		{
		  aabuff[ sub * width * 4 + ii] = 1;
		}
	  }
	  if(minx > (int) segs[0])
		minx = (int) segs[0];
	  if(maxx < (int) segs[Nsegs-1])
		maxx = (int) segs[Nsegs-1];
	}
    for(i=minx; i <= maxx; i++)
	{
	  alpha = 0;
	  for(ii=0;ii<4;ii++)
		for(iii=0;iii<4;iii++)
		{
		  alpha += aabuff[ii * 4 * width + i*4 + iii];
		  aabuff[ii *4 * width + i *4 + iii] = 0;
		}
      offset = (line * width + i) * 4;
	  alpha = (oalpha * alpha)/16;
	  rgba[ offset ] = ((int) red * alpha + rgba[offset] * (255 - alpha))/255;
	  rgba[ offset + 1] = (((int) green) * alpha + rgba[offset+1] * (255 - alpha)) / 255;
	  rgba[ offset + 2] = (((int) blue) * alpha + rgba[offset+2] * (255 - alpha)) / 255;
	}
  }

  free(aabuff);
  return 0;
}

void bbx_paste(unsigned char *rgba, int width, int height, unsigned char *sub, int swidth, int sheight, int x, int y)
{
  int sx, sy;
  int red, green, blue, alpha;
  int tx, ty, twidth, theight;
  int index;

  ty = y < 0 ? -y : 0;
  tx = x < 0 ? -x : 0;
  theight = y + sheight > height ? height - y : sheight;
  twidth = x + swidth > width ? width - x :  swidth; 
    
  for(sy=ty;sy<theight;sy++)
  {
    for(sx=tx;sx<twidth;sx++)
    {
      if(sub[(sy*swidth+sx)*4 + 3] == 0xFF)
      {
	index = ((sy+y)*width+sx+x)*4;
        rgba[index] = sub[(sy*swidth+sx)*4];
        rgba[index+1] = sub[(sy*swidth+sx)*4+1];
        rgba[index+2] = sub[(sy*swidth+sx)*4+2];
        rgba[index+3] = 0xFF;
      }
      else if(sub[(sy*swidth+sx)*4+3] != 0)
      {
        red = sub[(sy*swidth+sx)*4];
        green = sub[(sy*swidth+sx)*4+1];
        blue = sub[(sy*swidth+sx)*4+2];
        alpha = sub[(sy*swidth+sx)*4+3];
        index = ((sy+y)*width+sx+x)*4;
        rgba[index] = (alpha *(red - rgba[index]) + ((int)rgba[index] << 8))>>8; 
        rgba[index+1] = (alpha *(green - rgba[index+1]) + ((int)rgba[index+1] << 8))>>8; 
  rgba[index+2] = (alpha *(blue - rgba[index+2]) + ((int)rgba[index+2] << 8))>>8; 
  rgba[index+3] = 255 - (((255 - rgba[index+3]) * (255 - alpha)) >> 8); 
      }
    }  
  }
}

static unsigned long bilerp(unsigned char *rgba, int width, int height, float x, float y)
{
  unsigned char r[4];
  unsigned char g[4];
  unsigned char b[4];
  unsigned char a[4];
  int index;
  int red, green, blue, alpha;
  float ta, tb;

  if(x >= 0 && x < width-1 && y >= 0 && y < height-1)
  {
    index = ((int)y * width + (int) x) * 4;
    r[0] = rgba[index];
    g[0] = rgba[index+1];
    b[0] = rgba[index+2];
    a[0] = rgba[index+3];
    index = ((int)y * width + (int) x +1) * 4;
    r[1] = rgba[index];
    g[1] = rgba[index+1];
    b[1] = rgba[index+2];
    a[1] = rgba[index+3];
    index = ((int)y * width + (int) x + width) * 4 ;
    r[2] = rgba[index];
    g[2] = rgba[index+1];
    b[2] = rgba[index+2];
    a[2] = rgba[index+3];
    index = ((int)y * width + (int) x + width + 1) * 4;
    r[3] = rgba[index];
    g[3] = rgba[index+1];
    b[3] = rgba[index+2];
    a[3] = rgba[index+3];

   } 
  else if(y < 0 && x >= 0 && x < width -1)
  {
    index = ((int)0 * width + (int) x ) * 4;
    r[0] = rgba[index];
    g[0] = rgba[index+1];
    b[0] = rgba[index+2];
    a[0] = 0;
    index = ((int)0 * width + (int) x  + 1) * 4;
    r[1] = rgba[index];
    g[1] = rgba[index+1];
    b[1] = rgba[index+2];
    a[1] = 0;
     index = ((int)0 * width + (int) x ) * 4; 
    r[2] = rgba[index];
    g[2] = rgba[index+1];
    b[2] = rgba[index+2];
    a[2] = rgba[index+3];
     index = ((int)0 * width + (int) x + 1) * 4;
    r[3] = rgba[index];
    g[3] = rgba[index+1];
    b[3] = rgba[index+2];
    a[3] = rgba[index+3];
  
  }
  else if(y >= height -1 && x >= 0 && x < width -1)
  {
    index = ((int)height * width + (int) x - width) * 4;
    r[0] = rgba[index];
    g[0] = rgba[index+1];
    b[0] = rgba[index+2];
    a[0] = rgba[index+3];
    index = ((int)height * width + (int) x + 1 - width) * 4;
    r[1] = rgba[index];
    g[1] = rgba[index+1];
    b[1] = rgba[index+2];
    a[1] = rgba[index+3];
    index = ((int)height * width + (int) x - width) * 4;
    r[2] = rgba[index];
    g[2] = rgba[index+1];
    b[2] = rgba[index+2];
    a[2] = 0;

    index = ((int)height * width + (int) x + 1 - width) * 4;
    r[3] = rgba[index];
    g[3] = rgba[index+1];
    b[3] = rgba[index+2];
    a[3] = 0;
  }
  else if(x < 0 && y >= 0 && y < height -1)
  {
    index = ((int)y * width + (int) 0) * 4;
    r[0] = rgba[index];
    g[0] = rgba[index+1];
    b[0] = rgba[index+2];
    a[0] = 0;
    index = ((int)y * width + (int) 0 ) * 4 ;
    r[1] = rgba[index];
    g[1] = rgba[index+1];
    b[1] = rgba[index+2];
    a[1] = rgba[index+3];
    index = ((int)y * width + (int) 0 + width ) * 4;
    r[2] = rgba[index];
    g[2] = rgba[index+1];
    b[2] = rgba[index+2];
    a[2] = 0;
    index = ((int)y * width + (int) 0 + width ) * 4;
    r[3] = rgba[index];
    g[3] = rgba[index+1];
    b[3] = rgba[index+2];
    a[3] = rgba[index+3];

  }
  else if(x >= width-1 && y >= 0 && y < height -1)
  {
    index = ((int)y * width + (int) width -1) * 4;
    r[0] = rgba[index];
    g[0] = rgba[index+1];
    b[0] = rgba[index+2];
    a[0] = rgba[index+3];
    index = ((int)y * width + (int) width -1) * 4;
    r[1] = rgba[index];
    g[1] = rgba[index+1];
    b[1] = rgba[index+2];
    a[1] = 0;
    index = ((int)y * width + (int) width + width -1) * 4;
    r[2] = rgba[index];
    g[2] = rgba[index+1];
    b[2] = rgba[index+2];
    a[2] = rgba[index+3];
    index = ((int)y * width + (int) width + width -1) * 4;
    r[3] = rgba[index];
    g[3] = rgba[index+1];
    b[3] = rgba[index+2];
    a[3] = 0;
    }
  else if(x < 0 && y < 0)
  {
    index = 0; //  ((int)y * width + (int)x + width + 1) * 4;
    r[0] = rgba[index];
    g[0] = rgba[index+1];
    b[0] = rgba[index+2];
    a[0] = 0;
    
    r[1] = rgba[index];
    g[1] = rgba[index+1];
    b[1] = rgba[index+2];
    a[1] = 0;
    
    r[2] = rgba[index];
    g[2] = rgba[index+1];
    b[2] = rgba[index+2];
    a[2] = 0;
    r[3] = rgba[index];
    g[3] = rgba[index+1];
    b[3] = rgba[index+2];
    a[3] = rgba[index+3];
  }
  else if(x >= width -1 && y < 0)
  {
    index = ((int)y * width + (int) x -1 + width) * 4;
    r[0] = rgba[index];
    g[0] = rgba[index+1];
    b[0] = rgba[index+2];
    a[0] = 0;
    
    r[1] = rgba[index];
    g[1] = rgba[index+1];
    b[1] = rgba[index+2];
    a[1] = 0;
    
    r[2] = rgba[index];
    g[2] = rgba[index+1];
    b[2] = rgba[index+2];
    a[2] = rgba[index+3];

    r[3] = rgba[index];
    g[3] = rgba[index+1];
    b[3] = rgba[index+2];
    a[3] = 0;
  }
  else if(x < 0 && y >= height -1)
  {
    index = ((int)(y-1) * width + (int) x +1) * 4;
    r[0] = rgba[index];
    g[0] = rgba[index+1];
    b[0] = rgba[index+2];
    a[0] = 0;
    
    r[1] = rgba[index];
    g[1] = rgba[index+1];
    b[1] = rgba[index+2];
    a[1] = rgba[index+3];
    
    r[2] = rgba[index];
    g[2] = rgba[index+1];
    b[2] = rgba[index+2];
    a[2] = 0;

    r[3] = rgba[index];
    g[3] = rgba[index+1];
    b[3] = rgba[index+2];
    a[3] = 0;
  }
  else if(x >= width -1 && y >= height -1)
  {
    index = ((int)(y-1) * width + (int) x-1) * 4 ;
    r[0] = rgba[index];
    g[0] = rgba[index+1];
    b[0] = rgba[index+2];
    a[0] = rgba[index+3];
    
    r[1] = rgba[index];
    g[1] = rgba[index+1];
    b[1] = rgba[index+2];
    a[1] = 0;
    
    r[2] = rgba[index];
    g[2] = rgba[index+1];
    b[2] = rgba[index+2];
    a[2] = 0;
    r[3] = rgba[index];
    g[3] = rgba[index+1];
    b[3] = rgba[index+2];
    a[3] = 0;
  } 
  
  ta = x - (int) floor(x);
  tb = y - (int) floor(y);

  red =  (int) (r[0] * (1-ta)*(1-tb) + r[1]*ta*(1-tb) + r[2]*(1-ta)*tb + r[3]*ta*tb);
  green =  (int) (g[0] * (1-ta)*(1-tb) + g[1]*ta*(1-tb) + g[2]*(1-ta)*tb + g[3]*ta*tb);
  blue =  (int) (b[0] * (1-ta)*(1-tb) + b[1]*ta*(1-tb) + b[2]*(1-ta)*tb + b[3]*ta*tb);
  alpha  =  (int) (a[0] * (1-ta)*(1-tb) + a[1]*ta*(1-tb) + a[2]*(1-ta)*tb + a[3]*ta*tb);

  if(red < 0) red = 0; if(red > 255) red = 255;
  if(green < 0) green = 0; if(green > 255) green = 255;
  if(blue < 0) blue = 0; if(blue > 255) blue = 255;
  if(alpha < 0) alpha = 0; if(alpha > 255) alpha = 255;
   
  return (red << 24) | (green << 16) | (blue << 8) | alpha;
  
   
}

void bbx_pasterot(unsigned char *rgba, int width, int height, unsigned char *sub, int swidth, int sheight, int x, int y, double theta)
{
  float sint = (float) sin(theta);
  float cost = (float) cos(theta);
  int cornerx[4];
  int cornery[4];
  int minx = width + 1;
  int maxx = -1;
  int miny = height + 1;
  int maxy = -1;
  float cx, cy;
  
  float tx, ty;
  int ix, iy;
  float sx, sy;
  int red, green, blue, alpha;
  int index;
  int i;
  unsigned int col;

  cx = swidth/2.0f;
  cy = sheight/2.0f;

  cornerx[0] = 0;
  cornerx[1] = swidth;
  cornerx[2] = swidth;
  cornerx[3] = 0;
  cornery[0] = 0;
  cornery[1] = 0;
  cornery[2] = sheight;
  cornery[3] = sheight;

  for(i=0;i<4;i++)
  {
    tx = (cornerx[i] - cx) * cost + (cornery[i]-cy) * sint;
    ty = (cornerx[i] - cx) *-sint + (cornery[i]-cy) * cost;
    tx += x + cx;
    ty += y + cy;
    if(tx < minx)
      minx = (int) floor(tx);
    if(tx > maxx)
      maxx = (int) ceil(tx);
    if(ty < miny)
      miny = (int) floor(ty);
    if(ty > maxy)
      maxy = (int) ceil(ty);
  }
  if(minx < 0)
    minx = 0;
  if(maxx > width-1)
    maxx = height -1;
  if(miny < 0)
    miny = 0;
  if(maxy > height-1)
    maxy = height-1;

  for(iy = miny; iy<=maxy;iy++)
    for(ix = minx; ix <= maxx; ix++)
    {
      sx = ix - x - cx;
      sy = iy - y - cy;
      tx = sx * cost + sy * -sint + cx;
      ty = sx * sint + sy * cost + cy;
      if(tx > -1.0f && tx < swidth && ty > -1.0f && ty < sheight)
      {
	col = bilerp(sub, swidth, sheight, tx, ty);
        alpha = col & 0xFF;
        if(alpha == 0)
          continue;
        if(alpha == 0xFF)
	{
          rgba[(iy*width+ix)*4] = (col >> 24) & 0xFF;
          rgba[(iy*width+ix)*4+1] = (col >> 16) & 0xFF;
          rgba[(iy*width+ix)*4+2] = (col >> 8) & 0xFF;
	  rgba[(iy*width+ix)*4+3] = 0xFF;
	}
        else
	{
        index = (iy*width+ix)*4;
        red = (col >> 24) & 0xFF;
        green = (col >> 16) & 0xFF;
        blue = (col >> 8) & 0xFF;
        alpha = col & 0xFF;
        rgba[index] = (alpha *(red - rgba[index]) + ((int)rgba[index] << 8))>>8; 
        rgba[index+1] = (alpha *(green - rgba[index+1]) + ((int)rgba[index+1] << 8))>>8; 
        rgba[index+2] = (alpha *(blue - rgba[index+2]) + ((int)rgba[index+2] << 8))>>8; 
        rgba[index+3] = 255 - (((255 - rgba[index+3]) * (255 - alpha)) >> 8); 
	}
      }
    } 
 
}



/*
  rotate an image, using the shearing method
  Params: rgba - the rgba image
          width - image width
		  height - image height
		  cx, cy - centre x, y co-ordinates (pass in 1.5, 1.5 for the centre of a 3x3 image)
		  theta - angle to rotate by
		  out - buffer for output (can't be the same as input)
  Returns: 0 for success

  Notes: conventional image rotation by the matrix method causes destination pixels to be
    sub-samples of source pixels. This isn't a problem with continous tone images where
	the new pixel values can be obtained by interpolation. However with binary or 
	colour-index images, interpolation isn't possible. The shearing method preserves the
	pixels, at some cost in rotational accuracy.
  */
int bbx_rotatebyshear(unsigned char *rgba, int width, int height, double cx, double cy, double theta, unsigned char *out)
{
	double alpha;
	double beta;
	int dpx;
	int tx, ty;
	int x, y;
        unsigned int *rgbai = (unsigned int *) rgba;
        unsigned int *outi = (unsigned int *) out;

	assert(rgba != out);
	theta = fmod(theta, 2 * PI);

	if(theta >= -PI/2 && theta <= PI/2)
	{
	  alpha = -tan(theta/2);
          beta = sin(theta);


          for(y=0;y<height;y++)
	  {
	     dpx = (int) floor(alpha * (y - cy) + 0.5);
	     for(x=0;x<width;x++)
	     {
		   ty = y + (int) floor(beta * (x + dpx - cx) + 0.5);
		   tx = x + dpx + (int) floor(alpha * (ty - cy) + 0.5); 
		   if(tx >= 0 && tx < width && ty >= 0 && ty < height)
		     outi[y*width+x] = rgbai[ty*width+tx];
		   else
		     outi[y*width+x] = 0;
	     }
	  }
	}
	else
	{
		alpha = -tan( (theta + PI) / 2);
		beta = sin(theta + PI);
		for(y=0;y<height;y++)
	    {
	     dpx = (int) floor(alpha * (y - cy) + 0.5);
	     for(x=0;x<width;x++)
	     {
		   ty = y + (int) floor(beta * (x + dpx - cx) + 0.5);
		   tx = x + dpx + (int) floor(alpha * (ty - cy) + 0.5);
		   tx = (int) (cx-(tx - cx));
		   ty = (int) (cy-(ty - cy));
		   if(tx >= 0 && tx < width && ty >= 0 && ty < height)
		     outi[y*width+x] = rgbai[ty*width+tx];
		   else
		     outi[y*width+x] = 0;
	     }
	  }
	}

	return 0;
}



unsigned char *bbx_rot90(unsigned char *rgba, int width, int height)
{
  unsigned char *answer;
  int i, j;

  answer = malloc(width * height * 4);
  if(!answer)
    return 0;

  for (i = width-1 ; i >= 0 ; --i)
  {
    for(j = height-1 ; j >= 0 ; --j)
    {
      answer[(i*height+j)*4] = rgba[(j*width+width-i-1)*4];
      answer[(i*height+j)*4+1] = rgba[(j*width+width-i-1)*4+1];
      answer[(i*height+j)*4+2] = rgba[(j*width+width-i-1)*4+2];
      answer[(i*height+j)*4+3] = rgba[(j*width+width-i-1)*4+3];
    }
    
  }
  return answer;
}


unsigned char *bbx_rot270(unsigned char *rgba, int width, int height)
{
  unsigned char *answer;
  int i, j;

  answer = malloc(width * height * 4);
  if(!answer)
    return 0;

  for (i = width-1 ; i >= 0 ; --i)
  {
    for(j = height-1 ; j >= 0 ; --j)
    {
       answer[(i*height+j)*4] = rgba[((height-j-1)*width+i)*4];
       answer[(i*height+j)*4+1] = rgba[((height-j-1)*width+i)*4+1];
       answer[(i*height+j)*4+2] = rgba[((height-j-1)*width+i)*4+2];
       answer[(i*height+j)*4+3] = rgba[((height-j-1)*width+i)*4+3];
    }
    
  }
  return answer;
}

/*
  clip a line to the viewport
  Params: width - viewport width
          height - viewport height
          x1, y1 - point 1 xy coords, also return
          x2, y2 - point 2 xy coords, also return
*/
static void clip(int width, int height, double *x1, double *y1, double *x2, double *y2)
{
  if(*x1 < 0 && *y1 !=  *y2)
  {
	*y1 = intersect(0, *y1, *x1, *y2, *x2);
	*x1 = 0;
  }
  if(*x1 > width -1)
  {
	*y1 = intersect(width-1, *y1, *x1, *y2, *x2);
	*x1 = width-1;
  }
  if(*x2 < 0 && *y1 != *y2)
  {
	*y2 = intersect(0, *y1, *x1, *y2, *x2);
	*x2 = 0;
  }
  if(*x2 > width -1)
  {
	*y2 = intersect(width-1, *y1, *x1, *y2, *x2);
	*x2 = width-1;
  }
  if(*y1 < 0 && *x1 != *x2)
  {
    *x1 = intersect(0, *x1, *y1, *x2, *y2);
	*y1 = 0;
  }
  if(*y1 > height -1 && *x1 != *x2)
  {
    *x1 = intersect(height -1, *x1, *y1, *x2, *y2);
	*y1 = height -1;
  }
  if(*y2 < 0 && *x1 != *x2)
  {
    *x2 = intersect(0, *x1, *y1, *x2, *y2);
	*y2 = 0;
  }
  if(*y2 > height -1 && *x1 != *x2)
  {
    *x2 = intersect(height -1, *x1, *y1, *x2, *y2);
	*y2 = height -1;
  }
  if(*x1 < 0 || *x2 < 0 || *y1 < 0 || *y2 < 0 || 
	 *x1 > width -1 || *x2 > width -1 || *y1 > height -1 || *y2 > height -1)
  {
    *x1 = 0;
	*y1 = 0;
	*x2 = 0;
	*y2 = 0;
  }
}

/*
  bubble sort a vector
  Params: x - list of values
          N - N values
*/
static void bubblesort(double *x, int N)
{
  int flag;
  double temp;
  int i;

  do
  {
    flag = 0;
	for(i=0;i<N-1;i++)
	  if(x[i] > x[i+1])
	  {
	    temp = x[i];
		x[i] = x[i+1];
		x[i+1] = temp;
		flag = 1;
	  }
  } while(flag);
}

/*
  get point at which line intersects raster line
  Params: y - the raster line / y value
          x1, y1 - line point 1 xy coords
          x2, y2 - line point2 xy coords
  Returns: x cordiante of intersect
  Notes: do not call for parallel line to x axis.
*/
static double intersect(double y, double x1, double y1, double x2, double y2)
{
  double g = (x1 - x2) / (y1 - y2);
  double c = x1 - y1 * g;

  return y * g + c;   
}

/*
  minimum of a vector
*/
static double minv(double *x, int N)
{
  double answer;
  int i;

  answer = x[0];
  for(i=1;i<N;i++)
    if(answer > x[i])
	  answer = x[i];
  return answer; 
}

/*
  maximum of a vector
*/
static double maxv(double *x, int N)
{
  double answer;
  int i;

  answer = x[0];
  for(i=1;i<N;i++)
    if(answer < x[i])
	  answer = x[i];
	  
  return answer; 
}


/*
  resize an image using the averaging method.
  Note that dwidth and dheight must be smaller than or equal to swidth, sheight.

 */
void sprshrink(unsigned char *dest, int dwidth, int dheight, unsigned char *src, int swidth, int sheight)
{
  int x, y;
  int i, ii;
  float red, green, blue, alpha;
  float xfrag, yfrag, xfrag2, yfrag2;
  float xt, yt, dx, dy;
  int xi, yi;


  dx = ((float)swidth)/dwidth;
  dy = ((float)sheight)/dheight;

  for(yt= 0, y=0;y<dheight;y++, yt += dy)
  {
    yfrag = (float) ceil(yt) - yt;
    if(yfrag == 0)
      yfrag = 1;
    yfrag2 = yt+dy - (float) floor(yt + dy);
    if(yfrag2 == 0 && dy != 1.0f)
      yfrag2 = 1;
    
    for(xt = 0, x=0;x<dwidth;x++, xt+= dx)
    {
      xi = (int) xt;
      yi = (int) yt;

      xfrag = (float) ceil(xt) - xt;
      if(xfrag == 0)
        xfrag = 1;
      xfrag2 = xt+dx - (float) floor(xt+dx);
      if(xfrag2 == 0 && dx != 1.0f)
        xfrag2 = 1;
      red = xfrag * yfrag * src[(yi*swidth+xi)*4];
      green =  xfrag * yfrag * src[(yi*swidth+xi)*4+1];
      blue =   xfrag * yfrag * src[(yi*swidth+xi)*4+2];
      alpha =  xfrag * yfrag * src[(yi*swidth+xi)*4+3];
      
      for(i=0; xi + i + 1 < xt+dx-1; i++)
      {
        red += yfrag * src[(yi*swidth+xi+i+1)*4];
        green += yfrag * src[(yi*swidth+xi+i+1)*4+1];
        blue += yfrag * src[(yi*swidth+xi+i+1)*4+2];
        alpha += yfrag * src[(yi*swidth+xi+i+1)*4+3];
      } 
      
      red += xfrag2 * yfrag * src[(yi*swidth+xi+i+1)*4];
      green += xfrag2 * yfrag * src[(yi*swidth+xi+i+1)*4+1];
      blue += xfrag2 * yfrag * src[(yi*swidth+xi+i+1)*4+2];
      alpha += xfrag2 * yfrag * src[(yi*swidth+xi+i+1)*4+3];
      for(i=0; yi+i+1 < yt +dy-1;i++)
      {
        red += xfrag * src[((yi+i+1)*swidth+xi)*4];
        green += xfrag * src[((yi+i+1)*swidth+xi)*4+1];
        blue += xfrag * src[((yi+i+1)*swidth+xi)*4+2];
        alpha += xfrag * src[((yi+i+1)*swidth+xi)*4+3];
        
        for(ii=0;xi+ii+1< xt + dx-1; ii++)
	{
          red += src[((yi+i+1)*swidth+xi+ii+1)*4];
          green += src[((yi+i+1)*swidth+xi+ii+1)*4+1];
          blue += src[((yi+i+1)*swidth+xi+ii+1)*4+2];
          alpha += src[((yi+i+1)*swidth+xi+ii+1)*4+3];
	}
        
        red += xfrag2 * src[((yi+i+1)*swidth+xi+ii+1)*4]; 
        green += xfrag2 * src[((yi+i+1)*swidth+xi+ii+1)*4+1]; 
        blue += xfrag2 * src[((yi+i+1)*swidth+xi+ii+1)*4+2]; 
        alpha += xfrag2 * src[((yi+i+1)*swidth+xi+ii+1)*4+3]; 
      }

      red += xfrag * yfrag2 * src[((yi+i+1)*swidth+xi)*4];
      green += xfrag * yfrag2 * src[((yi+i+1)*swidth+xi)*4+1];
      blue += xfrag * yfrag2 * src[((yi+i+1)*swidth+xi)*4+2];
      alpha += xfrag * yfrag2 * src[((yi+i+1)*swidth+xi)*4+3];

      for(ii=0;xi+ii+1<xt+dx-1;ii++)
      {
	red += yfrag2 * src[((yi+i+1)*swidth+xi+ii+1)*4];
	green += yfrag2 * src[((yi+i+1)*swidth+xi+ii+1)*4+1];
	blue += yfrag2 * src[((yi+i+1)*swidth+xi+ii+1)*4+2];
	alpha += yfrag2 * src[((yi+i+1)*swidth+xi+ii+1)*4+3];
      }
 
      red += xfrag2 * yfrag2 * src[((yi+i+1)*swidth+xi+ii+1)*4];
      green += xfrag2 * yfrag2 * src[((yi+i+1)*swidth+xi+ii+1)*4+1];
      blue += xfrag2 * yfrag2 * src[((yi+i+1)*swidth+xi+ii+1)*4+2];
      alpha += xfrag2 * yfrag2 * src[((yi+i+1)*swidth+xi+ii+1)*4+3];

     red /= dx * dy;
     green /= dx * dy;
     blue /= dx * dy;
     alpha /= dx * dy;
     red = clamp(red, 0, 255);
     green = clamp(green, 0, 255);
     blue = clamp(blue, 0, 255);
     alpha = clamp(alpha, 0, 255);
   
     dest[(y*dwidth+x)*4] = (unsigned char) red;
     dest[(y*dwidth+x)*4+1] = (unsigned char) green;
     dest[(y*dwidth+x)*4+2] = (unsigned char) blue;
     dest[(y*dwidth+x)*4+3] = (unsigned char) alpha;
    }
  }


}

typedef struct
{
    int R; // circle radius
    int x_center, y_center; // circle center
    int I; //Intesity (255 - alpha)
    int Z; //sub-pixel resolution
    int y, y_hat; // exact and predicted values of f(l);
    int delta_y, delta2_y; // first and second order differences of y
    int f2; // the value of f(l) * f(l)
    int delta_f2, delta2_f2; // first and second order difference of f2
    unsigned char *rgba;
    int width;
    int height;
    BBX_RGBA colour;
    int mode; // AA_CIRCLE_FILL or AA_CIRCLE_HOLE
} CircleSettings;

static void caa_initialize(CircleSettings *p);
static void caa_predict(CircleSettings *p);
static void caa_minimize(CircleSettings *p);
static void caa_correct(CircleSettings *p);
static void caa_eight_pixel(CircleSettings *p, int x, int y, int c);
static void caa_fill(CircleSettings *p, int x, int ymax, int ymin, int c);
static void caa_pixel(CircleSettings *p, int x, int y, int c);

/*
  Portable C routine to draw an anti-aliased filled circle
  Params: cx, cy - circle centre
          radius - circle radius
          hole - pass BBX_AACIRCLE_FILL or BBX_AA_CIRCLE_HOLE
          writepixel - routine to write a pixel with given alpha
          setalpha - routine to set the alpha value (used for punching hole)
          ptr - conext pointer to pass back to pixel routines
 Note: to draw a donut, draw a circle with AACIRCLE_FILL< then a hole with AACIRCLE_HOLE.
    It's not possible to draw a donut in situ over other artwork. So draw to a temporary
    image and then composit.
 
 Ref: Algorithms for Drawing Anti-aliased Circles and Ellipses
 Dan Field, 1984
 */
void bbx_aacircle(unsigned char *rgba, int width, int height, int cx, int 
cy, int radius, int hole, BBX_RGBA col)
{
    int c;
    int i, q;  // Current pixel address;
    int v = 0;  /// v/Z = percentage of pixel intersected by circle
    int v_old = 0;
    CircleSettings p;
    
    p.x_center = cx;
    p.y_center = cy;
    p.width = width;
    p.height = height;
    p.rgba = rgba;
    p.colour = col;
    
    p.mode = hole;
    
    p.R = radius;
    p.I = 255;
    
    caa_initialize(&p);
    
    q = p.R;
    i = 0;
    while (i < q)
    {
        caa_predict(&p);
        caa_minimize(&p);
        caa_correct(&p);
        v_old = v;
        v = v + p.delta_y;
        
        if (v >= 0) // Single pixel on perimeter
        {
            caa_eight_pixel(&p, i, q, (v + v_old)>> 1);
            caa_fill(&p, i, q - 1, -q, p.I);
            caa_fill(&p, -i-1, q- 1, -q, p.I);
        }
        else // two pixels on perimeter
        {
            v = v + p.Z;
         //   eight_pixel(i, q, v_old >> 1);
            caa_eight_pixel(&p, i, q, v_old >> 1);
            q = q - 1;
            caa_fill(&p, i, q-1, -q, p.I);
            caa_fill(&p, -i-1, q-1, -q, p.I);
            if (i < q)
            {
                caa_eight_pixel(&p, i, q, (v + p.Z) >> 1);
                caa_fill(&p, q, i-1, -i, p.I);
                caa_fill(&p, -q-1, i-1, -i, p.I);
            }
            else // went below diagonal, fix v for final pixels
                v = v + p.Z;
        }
        i = i + 1;
    }
    // Fill in 4 remaining pixels
    c = v >> 1;
    caa_pixel(&p, q, q, c);
    caa_pixel(&p, -q-1, q, c);
    caa_pixel(&p, -q-1, -q-1, c);
    caa_pixel(&p, q, -q-1, c);
    
}

static void caa_initialize(CircleSettings *p)
{
    p->Z = p->I;
    
    p->delta2_y = 0;
    p->delta_y = 0;
    p->y = p->Z * p->R;
    
    p->delta_f2 = p->Z * p->Z;
    p->delta2_f2 = -p->delta_f2 - p->delta_f2;
    p->f2 = p->y * p->y;
}

static void caa_predict(CircleSettings *p)
{
    p->delta_y = p->delta_y + p->delta2_y;
    p->y_hat = p->y + p->delta_y; // y_hat is predicted value of f(i)
}

static void caa_minimize(CircleSettings *p)
{
    int e, e_old, d;

    e_old = 0;
    
    // Innitialize the minimzation
    p->delta_f2 = p->delta_f2 + p->delta2_f2;
    p->f2 = p->f2 + p->delta_f2;
    
    e = p->y_hat * p->y_hat - p->f2;
    d = 1;
    p->y = p->y_hat;
    
    // Force e negative
    if (e > 0)
    {
        e = -e;
        d = -1;
    }
    
    // Minimize
    while (e < 0)
    {
        p->y = p->y + d;
        e_old = e;
        e = e + p->y + p->y - d;
    }
    
    // e or e_old is minimum squared error
    if (e + e_old > 0)
        p->y = p->y - d;
    
}

static void caa_correct(CircleSettings *p)
{
    int error;
    
    error = p->y - p->y_hat;
    p->delta2_y = p->delta2_y + error;
    p->delta_y = p->delta_y + error;
}

static void caa_eight_pixel(CircleSettings *p, int x, int y, int c)
{
    caa_pixel(p, x, y, c);
    caa_pixel(p, x, -y-1, c);
    caa_pixel(p, -x-1, -y-1, c);
    caa_pixel(p, -x-1, y, c);
    caa_pixel(p, y, x, c);
    caa_pixel(p, y, -x-1, c);
    caa_pixel(p, -y-1, -x-1, c);
    caa_pixel(p, -y-1, x, c);
}

static void caa_fill(CircleSettings *p, int x, int ymax, int ymin, int c)
{
    while (ymin <= ymax)
    {
        caa_pixel(p, x, ymin, c);
        ymin = ymin + 1;
    }
}


static void caa_pixel(CircleSettings *p, int x, int y, int c)
{
    int red, green, blue, alpha;
    int ix, iy;
    int index;
    
    ix = x + p->x_center;
    iy = y + p->y_center;
    
    if (ix < 0 || ix >= p->width || iy < 0 || iy >= p->height)
        return;
    
    index = iy * p->width + ix;

    red = bbx_red(p->colour);
    green = bbx_green(p->colour);
    blue = bbx_blue(p->colour);
    alpha = (bbx_alpha(p->colour) * c)/255;
    
    if (p->mode == BBX_AA_CIRCLE_HOLE)
    {
        p->rgba[index * 4 +3] = 255 - c;
    }
    else
    {
        BBX_RGBA fg = bbx_rgba(red, green, blue, alpha);
        BBX_RGBA bg = bbx_rgba(p->rgba[index*4], p->rgba[index*4+1],
          p->rgba[index*4+2], p->rgba[index*4+3]);
        BBX_RGBA result = bbx_alphablend(fg, bg);
        p->rgba[index * 4] = bbx_red(result);
        p->rgba[index * 4 +1] = bbx_green(result);
        p->rgba[index * 4 +2] = bbx_blue(result);
        p->rgba[index * 4 +3] = bbx_alpha(result);
    }
}

/*
BBX_RGBA bbx_alphablend(BBX_RGBA fg, BBX_RGBA bg)
{
    unsigned int a2  = bbx_alpha(fg);  
    unsigned int alpha = a2;  
    if (alpha == 0) return bg;  
    if (alpha == 255) return fg;  
    unsigned int a1  = bbx_alpha(bg);  
    unsigned int nalpha = 0x100 - alpha;  
    unsigned int rb1 = (nalpha * ((bg >> 8) & 0xFF00FF)) >> 8;  
    unsigned int rb2 = (alpha * ((fg >> 8) & 0xFF00FF)) >> 8;  
    unsigned int g1  = (nalpha * ((bg >> 8) & 0x00FF00)) >> 8;  
    unsigned int g2  = (alpha * ((fg >> 8) & 0x00FF00)) >> 8;  
    unsigned int anew = a1 + a2;  
    if (anew > 255) {anew = 255;}  
    return ((((rb1 + rb2) & 0xFF00FF) + ((g1 + g2) & 0x00FF00)) << 8) + 
anew;
}  

*/

