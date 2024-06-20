//
//  makepalette.c
//
//  Created by Malcolm McLean on 15/03/2014.
//  Copyright (c) 2014 Astute Graphics All rights reserved.
//

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "eig3.h"


typedef struct
{
    int start;
    int N;
    double error;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
} PALENTRY;

static double cdiff(unsigned char *a, unsigned char *b);
static void split(PALENTRY *entry, PALENTRY *split, unsigned char *buff);
static void splitpca(PALENTRY *entry, PALENTRY *split, unsigned char *buff);
static void calcerror(PALENTRY *entry, unsigned char *buff);
static int compred(const void *e1, const void *e2);
static int compgreen(const void *e1, const void *e2);
static int compblue(const void *e1, const void *e2);
static double variancecolour(unsigned char  *rgb, int N, int index);
static double meancolour(unsigned char *rgb, int N, int index);
static int getOtsuthreshold(unsigned char *rgb, int N, int index);
static int getOtsuthreshold2(unsigned char *rgb, int N, double *remap);
static int pca(double *ret, unsigned char *pixels, int N);
static int project(unsigned char *rgb, double *comp);

/*
   Convert an image to indexed form, using passed-in palette
 */
unsigned char *rgbtoindex(unsigned char *rgb, int width, int height, unsigned char *pal, int N)
{
    unsigned char *answer;
    int i, ii;
    int bestii;
    double best;
    double error;
    
    answer = malloc(width * height);
    if(!answer)
        return 0;
    for(i=0;i<width*height;i++)
    {
        best = cdiff(rgb+i*3, pal + 0);
        bestii =0;
        for(ii=1;ii<N;ii++)
        {
            error = cdiff(rgb+i*3, pal+ii*3);
            if(error < best)
            {
                best = error;
                bestii = ii;
            }
        }
        answer[i] = bestii;
    }
    
    return answer;
}

/*
   Generate a palette
   New algorithm - uses pca and Otsu thresholding
 */
int makepalette(unsigned char *rgb, int width, int height, unsigned char *pal, int N)
{
    unsigned char *buff = 0;
    PALENTRY *entry = 0;
    double best;
    int bestii;
    int i, ii;
    
    buff = malloc(width * height * 3);
    if(!buff)
        goto error_exit;
    memcpy(buff, rgb, width * height * 3);
    entry = malloc(N * sizeof(PALENTRY));
    if(!entry)
        goto error_exit;
    entry[0].start = 0;
    entry[0].N = width * height;
    calcerror(entry, buff);
    for(i=1;i<N;i++)
    {
        best = entry[0].error;
        bestii = 0;
        for(ii=0;ii<i;ii++)
        {
            if(entry[ii].error > best)
            {
                best = entry[ii].error;
                bestii = ii;
            }
        }
        splitpca(&entry[bestii], &entry[i], buff);
    }
    for(i=0;i<N;i++)
    {
        pal[i*3] = entry[i].red;
        pal[i*3+1] = entry[i].green;
        pal[i*3+2] = entry[i].blue;
    }
    free(buff);
    free(entry);
    return 0;
error_exit:
    free(buff);
    free(entry);
    return-1;
}

/*
   Colour difference function
 */
static double cdiff(unsigned char *a, unsigned char *b)
{
  return (a[0]-b[0])*(a[0]-b[0]) * 5 +
    (a[1]-b[1])*(a[1]-b[1]) * 8 +
    (a[2]-b[2])*(a[2]-b[2]) * 2;
}

static void split(PALENTRY *entry, PALENTRY *split, unsigned char *buff)
{
    double vred, vgreen, vblue;
    int cindex;
    int cut;
    int i;
    vred = variancecolour(buff + entry->start*3, entry->N, 0);
    vgreen = variancecolour(buff + entry->start*3, entry->N, 1);
    vblue = variancecolour(buff + entry->start*3, entry->N, 2);
    if(vred > vgreen && vred > vblue)
    {
        qsort(buff + entry->start*3, entry->N, 3, compred);
        cindex = 0;
    }
    else if(vgreen > vblue)
    {
        qsort(buff + entry->start*3, entry->N, 3, compgreen);
        cindex = 1;
    }
    else
    {
        qsort(buff + entry->start*3, entry->N, 3, compblue);
        cindex = 2;
    }
    cut = getOtsuthreshold(buff + entry->start*3, entry->N, cindex);
    for(i=0;i<entry->N-1;i++)
    {
        if(buff[(entry->start+i)*3+cindex] < cut && buff[(entry->start+i+1)*3 + cindex] >= cut )
            break;
    }
    
    split->start = entry->start + i+1;
    split->N = entry->N -i -1;
    entry->N = i+1;
    calcerror(entry, buff);
    calcerror(split, buff);
    
}


/*
   project rgb triplet onto vector comp.
 */
static int project(unsigned char *rgb, double *comp)
{
    return rgb[0] * comp[0] + rgb[1] * comp[1] + rgb[2] * comp[2];
}

/*
   split an entry using pca and Otsu thresholding.
   We find the pricipal component of variance in rgb space.
   Then we apply Itsu thresholding along that axis, and cut.
   We partition using one pass of quick sort.
 */
static void splitpca(PALENTRY *entry, PALENTRY *split, unsigned char *buff)
{
    int low, high;
    int cut;
    double comp[3];
    unsigned char temp;
    int i;
    
    pca(comp, buff + entry->start*3, entry->N);
    cut = getOtsuthreshold2(buff+entry->start*3, entry->N, comp);
    low = 0;
    high = entry->N -1;
    while(low < high)
    {
        while(low < high && project(&buff[((entry->start+low)*3)], comp) < cut)
            low++;
        while(low < high && project(&buff[((entry->start+high)*3)], comp) >= cut)
            high--;
        if(low < high)
        {
            for(i=0;i<3;i++)
            {
              temp = buff[(entry->start+low)*3+i];
              buff[(entry->start+low)*3+i] = buff[(entry->start+high)*3+i];
              buff[(entry->start+high)*3+i] = temp;
            }
        }
        low++;
        high--;
    }
    split->start = entry->start + low;
    split->N = entry->N -low;
    entry->N = low;
    calcerror(entry, buff);
    calcerror(split, buff);
    
}

static void calcerror(PALENTRY *entry, unsigned char *buff)
{
    int i;
    
    entry->red = (unsigned char) meancolour(buff + entry->start*3, entry->N, 0);
    entry->green = (unsigned char) meancolour(buff + entry->start*3, entry->N, 1);
    entry->blue = (unsigned char) meancolour(buff + entry->start*3, entry->N, 2);
    entry->error = 0;
    for(i=0;i<entry->N;i++)
    {
        entry->error += abs(buff[(entry->start+i)*3] - entry->red);
        entry->error += abs(buff[(entry->start+i)*3+1] - entry->green);
        entry->error += abs(buff[(entry->start+i)*3+2] - entry->blue);
    }
}

static int compred(const void *e1, const void *e2)
{
    const unsigned char *c1 = e1;
    const unsigned char *c2 = e2;
    
    return (int)  c1[0] - c2[0];
}

static int compgreen(const void *e1, const void *e2)
{
    const unsigned char *c1 = e1;
    const unsigned char *c2 = e2;
    
    return (int) c1[1] - c2[1];
}

static int compblue(const void *e1, const void *e2)
{
    const unsigned char *c1 = e1;
    const unsigned char *c2 = e2;
    
    return (int) c1[2] - c2[2];
}

static double variancecolour(unsigned char  *rgb, int N, int index)
{
    double mu;
    double variance = 0;
    int i;
    
    mu = meancolour(rgb, N, index);
    for(i=0;i<N;i++)
        variance += (rgb[i*3+index] - mu)*(rgb[i*3+index] - mu);
    return variance / N;
    
}

static double meancolour(unsigned char *rgb, int N, int index)
{
    double answer = 0;
    int i;
    
    if(N == 0)
        return 0;
    for(i=0;i<N;i++)
        answer += rgb[i*3+index];
    return answer / N;
    
}

/*
 get the Otusu threshold for image segmentation
 Params: gray - the grayscale image
 width - image width
 height - uimage height
 Returns: threshold at which to split pixels into foreground and
 background.
 */
static int getOtsuthreshold(unsigned char *rgb, int N, int index)
{
    int hist[256] = {0};
    int wB = 0;
    int wF;
    float mB, mF;
    float sum = 0;
    float sumB = 0;
    float varBetween;
    float varMax = 0.0f;
    int answer = 0;
    int i;
    int k;
    
    for(i=0;i<N;i++)
        hist[rgb[i*3 + index]]++;
    
    /* sum of all (for means) */
    for (k=0 ; k<256 ; k++)
        sum += k * hist[k];
    
    for(k=0;k<256;k++)
    {
        wB += hist[k];
        if (wB == 0)
            continue;
        
        wF = N - wB;
        if (wF == 0)
            break;
        
        sumB += (float) (k * hist[k]);
        
        mB = sumB / wB;            /* Mean Background */
        mF = (sum - sumB) / wF;    /* Mean Foreground */
        
        /* Calculate Between Class Variance */
        varBetween = (float)wB * (float)wF * (mB - mF) * (mB - mF);
        
        /* Check if new maximum found */
        if (varBetween > varMax)
        {
            varMax = varBetween;
            answer = k;
        }
        
    }
    return answer;
}

/*
 get the Otusu threshold for image segmentation
 Params: gray - the grayscale image
 width - image width
 height - uimage height
 Returns: threshold at which to split pixels into foreground and
 background.
 */
static int getOtsuthreshold2(unsigned char *rgb, int N, double *remap)
{
    int hist[1024] = {0};
    int wB = 0;
    int wF;
    float mB, mF;
    float sum = 0;
    float sumB = 0;
    float varBetween;
    float varMax = 0.0f;
    int answer = 0;
    int i;
    int k;
    
    for(i=0;i<N;i++)
    {
        int nc = rgb[i*3] * remap[0] + rgb[i*3+1] * remap[1] + rgb[i*3+2] * remap[2];
        hist[512+nc]++;
    }
    
    /* sum of all (for means) */
    for (k=0 ; k<1024 ; k++)
        sum += k * hist[k];
    
    for(k=0;k<1024;k++)
    {
        wB += hist[k];
        if (wB == 0)
            continue;
        
        wF = N - wB;
        if (wF == 0)
            break;
        
        sumB += (float) (k * hist[k]);
        
        mB = sumB / wB;            /* Mean Background */
        mF = (sum - sumB) / wF;    /* Mean Foreground */
        
        /* Calculate Between Class Variance */
        varBetween = (float)wB * (float)wF * (mB - mF) * (mB - mF);
        
        /* Check if new maximum found */
        if (varBetween > varMax)
        {
            varMax = varBetween;
            answer = k;
        }
        
    }
    return answer - 512;
}


/*
   Get principal components of variance
   Params: ret - return for components of major axis of variance
           pixels - the pixels
           N - count of pixels
 */
static int pca(double *ret, unsigned char *pixels, int N)
{
    double cov[3][3];
    double mu[3];
    int i, j, k;
    double var;
    double d[3];
    double v[3][3];
    
    for(i=0;i<3;i++)
        mu[i] = meancolour(pixels, N, i);
    
    /* calculate 3x3 channel covariance matrix */
    for(i=0;i<3;i++)
        for(j=0;j<=i;j++)
        {
            var  =0;
            for(k=0;k<N;k++)
            {
                var += (pixels[k*3+i] - mu[i])*(pixels[k*3+j] - mu[j]);
            }
            cov[i][j] = var / N;
            cov[j][i] = var / N;
        }
    eigen_decomposition(cov, v, d);
    /* main component in col 3 of eigenvector matrix */
    ret[0] = v[0][2];
    ret[1] = v[1][2];
    ret[2] = v[2][2];
    
    return 0;
    
}