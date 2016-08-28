#include <stdlib.h>
#include <math.h>
#include <assert.h>

#define PI 3.1415926535897932384626433832795

void rgbaclear(unsigned char *rgba, int width, int height, unsigned long rgb)
{
  int i;

  for(i=0;i<width*height;i++)
  {
    rgba[i*4] = (rgb >> 16) & 0xFF;
    rgba[i*4+1] = (rgb >> 8) & 0xFF;
    rgba[i*4+2] = rgb & 0xFF;
    rgba[i*4+3] = 0xFF;
  }

}

void rgbapaste(unsigned char *rgba, int width, int height, unsigned char *sub, int swidth, int sheight, int x, int y)
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

unsigned int bilerp(unsigned char *rgba, int width, int height, float x, float y)
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
    index = ((int)y * width + (int) x + width) * 4;
    r[0] = rgba[index];
    g[0] = rgba[index+1];
    b[0] = rgba[index+2];
    a[0] = 0;
    index = ((int)y * width + (int) x + width + 1) * 4;
    r[1] = rgba[index];
    g[1] = rgba[index+1];
    b[1] = rgba[index+2];
    a[1] = 0;
     index = ((int)y * width + (int) x + width) * 4; 
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
  else if(y >= height -1 && x >= 0 && x < width -1)
  {
    index = ((int)y * width + (int) x - width) * 4;
    r[0] = rgba[index];
    g[0] = rgba[index+1];
    b[0] = rgba[index+2];
    a[0] = rgba[index+3];
    index = ((int)y * width + (int) x + 1 - width) * 4;
    r[1] = rgba[index];
    g[1] = rgba[index+1];
    b[1] = rgba[index+2];
    a[1] = rgba[index+3];
    index = ((int)y * width + (int) x - width) * 4;
    r[2] = rgba[index];
    g[2] = rgba[index+1];
    b[2] = rgba[index+2];
    a[2] = 0;

    index = ((int)y * width + (int) x + 1 - width) * 4;
    r[3] = rgba[index];
    g[3] = rgba[index+1];
    b[3] = rgba[index+2];
    a[3] = 0;
  }
  else if(x < 0 && y >= 0 && y < height -1)
  {
    index = ((int)y * width + (int) x + 1) * 4;
    r[0] = rgba[index];
    g[0] = rgba[index+1];
    b[0] = rgba[index+2];
    a[0] = 0;
    index = ((int)y * width + (int) x + 1) * 4 ;
    r[1] = rgba[index];
    g[1] = rgba[index+1];
    b[1] = rgba[index+2];
    a[1] = rgba[index+3];
    index = ((int)y * width + (int) x + width + 1) * 4;
    r[2] = rgba[index];
    g[2] = rgba[index+1];
    b[2] = rgba[index+2];
    a[2] = 0;
    index = ((int)y * width + (int) x + width + 1) * 4;
    r[3] = rgba[index];
    g[3] = rgba[index+1];
    b[3] = rgba[index+2];
    a[3] = rgba[index+3];

  }
  else if(x >= width-1 && y >= 0 && y < height -1)
  {
    index = ((int)y * width + (int) x -1) * 4;
    r[0] = rgba[index];
    g[0] = rgba[index+1];
    b[0] = rgba[index+2];
    a[0] = rgba[index+3];
    index = ((int)y * width + (int) x -1) * 4;
    r[1] = rgba[index];
    g[1] = rgba[index+1];
    b[1] = rgba[index+2];
    a[1] = 0;
    index = ((int)y * width + (int) x + width -1) * 4;
    r[2] = rgba[index];
    g[2] = rgba[index+1];
    b[2] = rgba[index+2];
    a[2] = rgba[index+3];
    index = ((int)y * width + (int) x + width -1) * 4;
    r[3] = rgba[index];
    g[3] = rgba[index+1];
    b[3] = rgba[index+2];
    a[3] = 0;
    }
  else if(x < 0 && y < 0)
  {
    index = ((int)y * width + (int) x + width + 1) * 4;
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

  red =  r[0] * (1-ta)*(1-tb) + r[1]*ta*(1-tb) + r[2]*(1-ta)*tb + r[3]*ta*tb;
  green =  g[0] * (1-ta)*(1-tb) + g[1]*ta*(1-tb) + g[2]*(1-ta)*tb + g[3]*ta*tb;
  blue =  b[0] * (1-ta)*(1-tb) + b[1]*ta*(1-tb) + b[2]*(1-ta)*tb + b[3]*ta*tb;
  alpha  =  a[0] * (1-ta)*(1-tb) + a[1]*ta*(1-tb) + a[2]*(1-ta)*tb + a[3]*ta*tb;

  if(red < 0) red = 0; if(red > 255) red = 255;
  if(green < 0) green = 0; if(green > 255) green = 255;
  if(blue < 0) blue = 0; if(blue > 255) blue = 255;
  if(alpha < 0) alpha = 0; if(alpha > 255) alpha = 255;
   
  return (red << 24) | (green << 16) | (blue << 8) | alpha;
  
   
}

void rgbapasterot(unsigned char *rgba, int width, int height, unsigned char *sub, int swidth, int sheight, int x, int y, double theta)
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
int rgbarotatebyshear(unsigned char *rgba, int width, int height, double cx, double cy, double theta, unsigned char *out)
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



unsigned char *rgbarot90(unsigned char *rgba, int width, int height)
{
  unsigned char *answer;
  int x, y;
  int ox, oy;
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


unsigned char *rgbarot270(unsigned char *rgba, int width, int height)
{
  unsigned char *answer;
  int x, y;
  int ox, oy;
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
