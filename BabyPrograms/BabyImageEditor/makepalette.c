//
//  makepalette.c
//  BabyImageEditor
//
//  Created by Malcolm McLean on 13/06/2024.
//
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "makepalette.h"

static int makepalette_splitk(unsigned char *pal, int Npal, unsigned char *rgba, int width, int height);
int sortpalette(unsigned char *pal, int Npal);

static unsigned char *kmeans(unsigned char *rgba, int N, int Nmeans);

int makepalette(unsigned char *pal, int Npal, unsigned char *rgba, int width, int height)
{
    if (Npal > 256)
        return -1;
    makepalette_splitk(pal, Npal, rgba, width, height);
    sortpalette(pal, Npal);
    return 0;
    
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
        sortpalette(pal, Npal);
        return  0;
        
    }
    return -1;
}

typedef struct node
{
    unsigned char *pixels;
    int Npixels;
    unsigned char mean[4];
    struct node *child;
    struct node *next;
} NODE;

static void node_kill_r(NODE *node)
{
    if (node)
    {
        if (node->child)
            node_kill_r(node->child);
        if (node->next)
            node_kill_r(node->next);
        free(node->pixels);
        free(node);
    }
}

static int node_split(NODE *node)
{
    NODE *first_child = 0;
    NODE *second_child = 0;
    unsigned char *clusters = 0;
    unsigned char *first_pixels = 0;
    unsigned char *second_pixels = 0;
    int Nfirst = 0;
    int Nsecond = 0;
    int i;
    int j_s, j_f;
    
    if (node->child)
        return -1;
    if (node->Npixels <= 1)
        return -1;
    
    clusters = kmeans(node->pixels, node->Npixels, 2);
    first_child = malloc(sizeof(NODE));
    second_child = malloc(sizeof(NODE));
    
    for (i = 0; i < node->Npixels; i++)
    {
        if (clusters[i] == 0)
            Nfirst++;
        else if (clusters[i] == 1)
            Nsecond++;
    }

    if (Nfirst == 0 || Nsecond == 0)
    {
        return -1;
    }
    first_child->pixels = malloc(Nfirst * 4);
    second_child->pixels = malloc(Nsecond * 4);
    j_f = 0;
    j_s = 0;
    for (i = 0; i < node->Npixels; i++ )
    {
        if (clusters[i] == 0)
        {
            first_child->pixels[j_f * 4] = node->pixels[i*4];
            first_child->pixels[j_f * 4+1] = node->pixels[i*4+1];
            first_child->pixels[j_f * 4+2] = node->pixels[i*4+2];
            first_child->pixels[j_f * 4+3] = node->pixels[i*4+3];
            
            j_f++;
        }
        else if (clusters[i] == 1)
        {
            second_child->pixels[j_s * 4] = node->pixels[i*4];
            second_child->pixels[j_s * 4+1] = node->pixels[i*4+1];
            second_child->pixels[j_s * 4+2] = node->pixels[i*4+2];
            second_child->pixels[j_s * 4+3] = node->pixels[i*4+3];
            
            j_s++;
        }
    }
    first_child->Npixels = Nfirst;
    second_child->Npixels = Nsecond;
    
    first_child->child = 0;
    second_child->child = 0;
    second_child->next = 0;
    first_child->next = second_child;
    node->child = first_child;
    
    free(node->pixels);
    node->pixels = 0;
    node->Npixels = 0;
    
    free(clusters);
    
    return 0;
}

static void getmean(unsigned char *rgba, int width, int height, unsigned char *mean)
{
    int i;
    int red = 0;
    int green = 0;
    int blue = 0;
    int alpha = 0;;
    
    for (i = 0; i <width * height; i++)
    {
        red += rgba[i*4];
        green += rgba[i*4+1];
        blue += rgba[i*4+2];
        alpha += rgba[i*4+3];
    }
    
    if (width * height)
    {
        red /= (width * height);
        green /= (width * height);
        blue /= (width * height);
        alpha /= (width * height);
    }
    mean[0] = red;
    mean[1] = green;
    mean[2] = blue;
    mean[3] = alpha;
}

static void split_r(NODE *node)
{
    while (node)
    {
        if (node->child)
            split_r(node->child);
        else
            node_split(node);
        node = node->next;
    }
}

static void split_r_n(NODE *node, int *n)
{
    while (node)
    {
        if (*n <= 0)
            return;
        if (node->child)
            split_r(node->child);
        else
        {
            node_split(node);
            (*n)--;
        }
        node = node->next;
    }
}

static void getmeans_r(NODE *node, unsigned char *pal, int *n)
{
    while (node)
    {
        if (*n <= 0)
            break;
        if (node->child)
            getmeans_r(node->child, pal, n);
        else
        {
            int redtot = 0;
            int greentot = 0;
            int bluetot = 0;
            int index = *n - 1;
            int i;
            
            for (i = 0 ; i < node->Npixels; i++)
            {
                redtot += node->pixels[i*4];
                greentot += node->pixels[i*4+1];
                bluetot += node->pixels[i*4+2];
            }
            if (node->Npixels)
            {
                redtot /= node->Npixels;
                greentot /= node->Npixels;
                bluetot /= node->Npixels;
            }
            pal[index *3] = redtot;
            pal[index * 3 + 1] = greentot;
            pal[index * 3 + 2] = bluetot;
            (*n)--;
        }
        node = node->next;
    }
}

static int makepalette_splitk(unsigned char *pal, int Npal, unsigned char *rgba, int width, int height)
{
    NODE *root;
    int pallog2;
    int temp;
    
    memset(pal, 0, Npal * 3);
    
    root = malloc(sizeof(NODE));
    root->child = 0;
    root->next = 0;
    root->Npixels = width * height;
    root->pixels = malloc(width * height * 4);
    memcpy(root->pixels, rgba, width * height * 4);
    
    temp = Npal;
    pallog2 = -1;
    while (temp)
    {
        pallog2++;
        temp >>= 1;
    }
    temp = pallog2;
    
    while (temp--)
    {
        split_r(root);
    }
    temp = Npal - (1 << pallog2);
    split_r_n(root, &temp);
    
    temp = Npal;
    getmeans_r(root, pal, &temp);
    
    node_kill_r(root);
    
    return 0;
}

static unsigned char *kmeans(unsigned char *rgba, int N, int Nmeans)
{
    unsigned char *clusters;
    unsigned char *means;
    unsigned char rgba_mean[4];
    double rgba_var[3] = {0};
    int *totals;
    int *counts;
    int converged = 0;
    int i;
    
    clusters = malloc(N);
    means = malloc(Nmeans * 4);
    totals = malloc(Nmeans * 3 * sizeof(int));
    counts = malloc(Nmeans * sizeof(int));
    
    for (i =0; i <Nmeans; i++)
    {
        int loopbreaker = 0;
        do
        {
            int target = rand() % N;
            
            means[i*4] = rgba[target * 4];
            means[i*4+1] = rgba[target*4+1];
            means[i*4+2] = rgba[target*4+2];
            means[i*4+3] = rgba[target*4+3];
            loopbreaker++;
        } while (i > 0 && !memcmp(&means[i*4], &means[(i-1)*4], 4) && loopbreaker < N);
    }
    
    for (i = 0; i < N; i++)
        clusters[i] = rand() % Nmeans;
    
   
    
    int nsteps = 0;
    while (!converged)
    {
        nsteps++;
        converged = 1;
        
        for (i =0; i < N; i++)
        {
            int best = 0x7FFF;
            int bestj = 0;
            int j;
            int d;
            
            for (j =0; j < Nmeans; j++)
            {
                d = abs((int) rgba[i*4] - means[j*4]) +
                abs((int) rgba[i*4+1] - means[j*4+1]) +
                abs((int) rgba[i*4+2] - means[j*4+2]);
                
                if (d < best)
                {
                    best = d;
                    bestj = j;
                }
            }
            if (clusters[i] != bestj)
            {
                clusters[i] = bestj;
                converged = 0;
            }
            
        }
        
        for (i = 0; i <Nmeans * 4 ; i++)
            totals[i] = 0;
        for (i =0; i <Nmeans; i++)
            counts[i] = 0;
        for (i = 0; i < N; i++)
        {
            totals[clusters[i]*3] += rgba[i* 4];
            totals[clusters[i]*3+1] += rgba[i*4+1];
            totals[clusters[i]*3+2] += rgba[i*4+2];
            counts[clusters[i]]++;
        }
        for (i = 0; i <Nmeans; i++)
        {
            if (counts[i])
            {
                means[i*4] = totals[i*3] / counts[i];
                means[i*4+1] = totals[i*3+1] / counts[i];
                means[i*4+2] = totals[i*3+2] / counts[i];
            }
        }
    }
    
    free(means);
    free(totals);
    free(counts);
    
    return clusters;
    
}


#define max2(a,b) ( (a) > (b) ? (a) : (b) )
#define min2(a,b) ( (a) < (b) ? (a) : (b) )
#define max3(a,b,c) max2(max2((a),(b)),(c))
#define min3(a,b,c) min2(min2((a),(b)),(c))
#define PI 3.14159265359 
  
static void rgb2hsv(unsigned long rgb, double *h, unsigned char *s, unsigned char *v)
{
  int r, g, b;
  int V;
  int S = 0;
  double hue;

  r = (rgb >> 16) & 0xFF;
  g = (rgb >> 8) & 0xFF;
  b = rgb & 0xFF;
   
  V = max3(r, g, b);
  if(V > 0)
    S = ((V - min3(r, g, b)) * 255)/ V;
  if(V == r)
    hue = (g - b)/(6.0)/S;
  else if(V == g)
    hue = 1.0/3.0 + (b - r)/(6.0)/S;
  else if(V == b)
    hue = 2.0/3.0 +  (r - g)/(6.0)/S;

  *s = S;
  *v = V;
  if(hue < 0.0)
    hue += 1.0;
  if(hue >= 1.0)
    hue -= 1.0;
    
  *h = hue * 2 * PI;
}

static int compf(const void *e1, const void *e2)
{
    const unsigned char *rgb_a = (const unsigned char *)e1;
    const unsigned char *rgb_b = (const unsigned char *)e2;
    long a = (rgb_a[0] << 16) + (rgb_a[1] << 8) + rgb_a[2];
    long b = (rgb_b[0] << 16) + (rgb_b[1] << 8) + rgb_b[2];
    double hue_a, hue_b;
    unsigned char sat_a, sat_b;
    unsigned char val_a, val_b;
    
    rgb2hsv(a, &hue_a, &sat_a, &val_a);
    rgb2hsv(b, &hue_b, &sat_b, &val_b);

    if(hue_a < hue_b)
        return -1;
    else if (hue_a > hue_b)
        return 1;
    else
        return 0;
}
int sortpalette(unsigned char *pal, int Npal)
{
    qsort(pal, Npal, 3, compf);
}
