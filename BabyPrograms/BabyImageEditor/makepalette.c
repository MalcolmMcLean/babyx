//
//  makepalette.c
//  BabyImageEditor
//
//  Created by Malcolm McLean on 13/06/2024.
//
#include <stdlib.h>
#include <string.h>

#include "makepalette.h"
#include "anneal.h"

int sortpalette(unsigned char *pal, int Npal);

unsigned char *kmeans(unsigned char *rgba, int N, int Nmeans);

int makepalette(unsigned char *pal, int Npal, unsigned char *rgba, int width, int height)
{
    unsigned char *clusters;
 //   clusters = kmeans(rgba, width * height, 2);
    
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
#include <stdio.h>

typedef struct node
{
    unsigned char *pixels;
    int Npixels;
    unsigned char mean[4];
    struct node *child;
    struct node *next;
} NODE;

int node_split(NODE *node)
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

unsigned char *kmeans(unsigned char *rgba, int N, int Nmeans)
{
    unsigned char *clusters;
    unsigned char *nextcluster;
    unsigned char *means;
    int *totals;
    int *counts;
    int converged = 0;
    int i;
    
    clusters = malloc(N);
    nextcluster = malloc(N);
    means = malloc(Nmeans * 4);
    totals = malloc(Nmeans * 3 * sizeof(int));
    counts = malloc(Nmeans * sizeof(int));
    
    for (i = 0; i < N; i++)
        clusters[i] = rand() % Nmeans;
    
    int nsteps = 0;
    while (!converged)
    {
        printf("Nsteps %d\n", nsteps);
        nsteps++;
        
        converged = 1;
        
        for (i = 0; i <Nmeans * 3 ; i++)
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
        for (i =0; i <Nmeans; i++)
        {
            if (counts[i])
            {
                means[i*4] = totals[i*3] / counts[i];
                means[i*4+1] = totals[i*3+1] / counts[i];
                means[i*4+2] = totals[i*3+2] / counts[i];
            }
        }
        
        for (i =0; i < N; i++)
        {
            int best = 0x7FFF;
            int bestj = 0;
            int j;
            int d;
            
            for (j =0; j < Nmeans; j++)
            {
                d = abs(rgba[i*4] - means[j*3]) +
                abs(rgba[i*4+1] - means[j*3+1]) +
                abs(rgba[i*4+2] - means[j*3+2]);
                
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
    }
    
    
    
}

typedef struct
{
    unsigned char *rgb;
    int N;
} PALETTE;

void *clone(void *obj, void *ptr)
{
    void *answer;
    
    answer = malloc(sizeof(PALETTE));
    if (answer)
        memcpy(answer, obj, sizeof(PALETTE));
    
    return answer;
}

void  kill(void *obj, void *ptr)
{
    free (obj);
}

int  copy(void *dest, void *src, void *ptr)
{
    memcpy(dest, src, sizeof(PALETTE));
}

double score(void *obj, void *ptr)
{
    PALETTE *pal = obj;
    double answer = 0;
    int d;
    int i;
    
    for (i = 0; i <pal->N -1; i++)
    {
        d = abs(pal->rgb[i*3] - pal->rgb[(i+1)*3]) +
        abs(pal->rgb[i*3+1] - pal->rgb[(i+1)*3+1]) +
        abs(pal->rgb[i*3+2] - pal->rgb[(i+1)*3+2]);
        
        answer += d;
    }
    
    return answer;
}

void mutate(void *obj, void *ptr)
{
    PALETTE *pal = obj;
    int indexa;
    int indexb;
    int i;
    unsigned char temp;
    
    indexa = rand() % pal->N;
    indexb = rand() % pal->N;
    for (i = 0; i < 3; i++)
    {
        temp = pal->rgb[indexa*3+i];
        pal->rgb[indexa*3+i] = pal->rgb[indexb *3+ i];
        pal->rgb[indexb*3+i] = temp;
    }
}


void rgb2hsv(unsigned long rgb, double *h, unsigned char *s, unsigned char *v);

int compf(const void *e1, const void *e2)
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
