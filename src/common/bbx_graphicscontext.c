/*
  Baby X Graphics Context

  by Malcolm McLean
  
  The idea is that it's a portable ANSI C graphics context, compatible with the 
  HTML canvas widget. So the calls and behaviour essentially match, with a few
  tweaks because of C's lack of dynamic typing.

  It writes to an rgba buffer.
  You call the graphics context on an rgba buffer

  int width = 100;
  int height = 161;
  unsgned char *rgba = malloc(width * height * 4);
  BBX_GC *gc;

  gc = bbx_graphicscontext(rgba, width, height);
  bbx_gc_setfillcolor(gc, bbx_rgba(200, 200, 0, 255));
  bbx_gc_fillcircle(gc, 50.0, 85.0, 20.0);
  bbx_graphicscontext_kill(gc);

  The RGBA buffer is released after the context is destroyed,
  and contains your graphics.

  The work is heavily based on Mikko Mononen's NanoSVG.

  Please retain copyright notices.
*/


/*
* Copyright (c) 2013-14 Mikko Mononen memon@inside.org
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*
* The SVG parser is based on Anti-Graim Geometry 2.4 SVG example
* Copyright (C) 2002-2004 Maxim Shemanarev (McSeem) (http://www.antigrain.com/)
*
* Arc calculation code based on canvg (https://code.google.com/p/canvg/)
*
* Bounding box calculation based on http://blog.hackers-cafe.net/2009/06/how-to-calculate-bezier-curves-bounding.html
*
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

	// NanoSVG is a simple stupid single-header-file SVG parse. The output of the parser is a list of cubic bezier shapes.
	//
	// The library suits well for anything from rendering scalable icons in your editor application to prototyping a game.
	//
	// NanoSVG supports a wide range of SVG features, but something may be missing, feel free to create a pull request!
	//
	// The shapes in the SVG images are transformed by the viewBox and converted to specified units.
	// That is, you should get the same looking data as your designed in your favorite app.
	//
	// NanoSVG can return the paths in few different units. For example if you want to render an image, you may choose
	// to get the paths in pixels, or if you are feeding the data into a CNC-cutter, you may want to use millimeters.
	//
	// The units passed to NanoVG should be one of: 'px', 'pt', 'pc' 'mm', 'cm', or 'in'.
	// DPI (dots-per-inch) controls how the unit conversion is done.
	//
	// If you don't know or care about the units stuff, "px" and 96 should get you going.


	/* Example Usage:
	// Load
	SNVGImage* image;
	image = nsvgParseFromFile("test.svg", "px", 96);
	printf("size: %f x %f\n", image->width, image->height);
	// Use...
	for (shape = image->shapes; shape != NULL; shape = shape->next) {
	for (path = shape->paths; path != NULL; path = path->next) {
	for (i = 0; i < path->npts-1; i += 3) {
	float* p = &path->pts[i*2];
	drawCubicBez(p[0],p[1], p[2],p[3], p[4],p[5], p[6],p[7]);
	}
	}
	}
	// Delete
	nsvgDelete(image);
	*/
#define NSVG_EPSILON (1e-12)
#define NSVG_PI (3.14159265358979323846264338327f)
#define NSVG_KAPPA90 (0.5522847493f)	// Length proportional to radius of a cubic bezier handle for 90deg arcs.

	enum NSVGpaintType {
		NSVG_PAINT_NONE = 0,
		NSVG_PAINT_COLOR = 1,
		NSVG_PAINT_LINEAR_GRADIENT = 2,
		NSVG_PAINT_RADIAL_GRADIENT = 3,
		NSVG_PAINT_PATTERN = 4,
	};

	enum NSVGspreadType {
		NSVG_SPREAD_PAD = 0,
		NSVG_SPREAD_REFLECT = 1,
		NSVG_SPREAD_REPEAT = 2,
	};

	enum NSVGlineJoin {
		NSVG_JOIN_MITER = 0,
		NSVG_JOIN_ROUND = 1,
		NSVG_JOIN_BEVEL = 2,
	};

	enum NSVGlineCap {
		NSVG_CAP_BUTT = 0,
		NSVG_CAP_ROUND = 1,
		NSVG_CAP_SQUARE = 2,
	};

	enum NSVGfillRule {
		NSVG_FILLRULE_NONZERO = 0,
		NSVG_FILLRULE_EVENODD = 1,
	};

	typedef struct NSVGgradientStop {
		unsigned int color;
		float offset;
	} NSVGgradientStop;

	typedef struct NSVGgradient {
		float xform[6];
		char spread;
		float fx, fy;
		int nstops;
		NSVGgradientStop stops[1];
	} NSVGgradient;

	typedef struct NSVGpaint {
		char type;
		union {
			unsigned int color;
			NSVGgradient* gradient;
		};
	} NSVGpaint;

	typedef struct NSVGpath
	{
		float* pts;					// Cubic bezier points: x0,y0, [cpx1,cpx1,cpx2,cpy2,x1,y1], ...
		int npts;					// Total number of bezier points.
		char closed;				// Flag indicating if shapes should be treated as closed.
		float bounds[4];			// Tight bounding box of the shape [minx,miny,maxx,maxy].
		struct NSVGpath* next;		// Pointer to next path, or NULL if last element.
	} NSVGpath;

	typedef struct NSVGshape
	{
		char id[64];				// Optional 'id' attr of the shape or its group
		NSVGpaint fill;				// Fill paint
		NSVGpaint stroke;			// Stroke paint
		float opacity;				// Opacity of the shape.
		float strokeWidth;			// Stroke width (scaled).
		char strokeLineJoin;		// Stroke join type.
		char strokeLineCap;			// Stroke cap type.
		char fillRule;				// Fille rule, see NSVGfillRule.
		float bounds[4];			// Tight bounding box of the shape [minx,miny,maxx,maxy].
		NSVGpath* paths;			// Linked list of paths in the image.
		struct NSVGshape* next;		// Pointer to next shape, or NULL if last element.
	} NSVGshape;

	typedef struct NSVGimage
	{
		float width;				// Width of the image.
		float height;				// Height of the image.
		NSVGshape* shapes;			// Linked list of shapes in the image.
	} NSVGimage;

	// Parses SVG file from a file, returns SVG image as paths.
	NSVGimage* nsvgParseFromFile(const char* filename, const char* units, float dpi);

	// Parses SVG file from a null terminated string, returns SVG image as paths.
	NSVGimage* nsvgParse(char* input, const char* units, float dpi);

	// Deletes list of paths.
	void nsvgDelete(NSVGimage* image);





#define NSVG_ALIGN_MIN 0
#define NSVG_ALIGN_MID 1
#define NSVG_ALIGN_MAX 2
#define NSVG_ALIGN_NONE 0
#define NSVG_ALIGN_MEET 1
#define NSVG_ALIGN_SLICE 2

#define NSVG_NOTUSED(v) do { (void)(1 ? (void)0 : ( (void)(v) ) ); } while(0)
#define NSVG_RGB(r, g, b) (((unsigned int)(r)) | ((unsigned int)(g) << 8) | ((unsigned int)(b) << 16))

#ifdef _MSC_VER
#pragma warning (disable: 4996) // Switch off security warnings
#pragma warning (disable: 4100) // Switch off unreferenced formal parameter warnings
#ifdef __cplusplus
#define NSVG_INLINE inline
#else
#define NSVG_INLINE
#endif
#else
#define NSVG_INLINE inline
#endif


static NSVG_INLINE float nsvg__minf(float a, float b) { return a < b ? a : b; }
static NSVG_INLINE float nsvg__maxf(float a, float b) { return a > b ? a : b; }

/* Simple SVG parser. */


#define NSVG_USER_SPACE 0
#define NSVG_OBJECT_SPACE 1

typedef struct NSVGlinearData {
	float x1, y1, x2, y2;
} NSVGlinearData;

typedef struct NSVGradialData {
	float cx, cy, r, fx, fy;
} NSVGradialData;

typedef struct NSVGgradientData
{
	char id[64];
	char ref[64];
	char type;
	union {
		NSVGlinearData linear;
		NSVGradialData radial;
	};
	char spread;
	char units;
	float xform[6];
	int nstops;
	NSVGgradientStop* stops;
	struct NSVGgradientData* next;
} NSVGgradientData;

typedef struct NSVGattrib
{
	char id[64];
	float xform[6];
	unsigned int fillColor;
	unsigned int strokeColor;
	float opacity;
	float fillOpacity;
	float strokeOpacity;
	char fillGradient[64];
	char strokeGradient[64];
	float strokeWidth;
	char strokeLineJoin;
	char strokeLineCap;
	char fillRule;
	float fontSize;
	unsigned int stopColor;
	float stopOpacity;
	float stopOffset;
	char hasFill;
	char hasStroke;
	char visible;
} NSVGattrib;


static void nsvg__xformIdentity(float* t)
{
	t[0] = 1.0f; t[1] = 0.0f;
	t[2] = 0.0f; t[3] = 1.0f;
	t[4] = 0.0f; t[5] = 0.0f;
}

static void nsvg__xformSetTranslation(float* t, float tx, float ty)
{
	t[0] = 1.0f; t[1] = 0.0f;
	t[2] = 0.0f; t[3] = 1.0f;
	t[4] = tx; t[5] = ty;
}

static void nsvg__xformSetScale(float* t, float sx, float sy)
{
	t[0] = sx; t[1] = 0.0f;
	t[2] = 0.0f; t[3] = sy;
	t[4] = 0.0f; t[5] = 0.0f;
}

static void nsvg__xformSetSkewX(float* t, float a)
{
	t[0] = 1.0f; t[1] = 0.0f;
	t[2] = tanf(a); t[3] = 1.0f;
	t[4] = 0.0f; t[5] = 0.0f;
}

static void nsvg__xformSetSkewY(float* t, float a)
{
	t[0] = 1.0f; t[1] = tanf(a);
	t[2] = 0.0f; t[3] = 1.0f;
	t[4] = 0.0f; t[5] = 0.0f;
}

static void nsvg__xformSetRotation(float* t, float a)
{
	float cs = cosf(a), sn = sinf(a);
	t[0] = cs; t[1] = sn;
	t[2] = -sn; t[3] = cs;
	t[4] = 0.0f; t[5] = 0.0f;
}

static void nsvg__xformMultiply(float* t, float* s)
{
	float t0 = t[0] * s[0] + t[1] * s[2];
	float t2 = t[2] * s[0] + t[3] * s[2];
	float t4 = t[4] * s[0] + t[5] * s[2] + s[4];
	t[1] = t[0] * s[1] + t[1] * s[3];
	t[3] = t[2] * s[1] + t[3] * s[3];
	t[5] = t[4] * s[1] + t[5] * s[3] + s[5];
	t[0] = t0;
	t[2] = t2;
	t[4] = t4;
}

static void nsvg__xformInverse(float* inv, float* t)
{
	double det = (double)t[0] * t[3] - (double)t[2] * t[1];
	double invdet = 1.0 / det;
	if (det > -1e-6 && det < -1e-6) {
		nsvg__xformIdentity(t);
		return;
	}
	inv[0] = (float)(t[3] * invdet);
	inv[2] = (float)(-t[2] * invdet);
	inv[4] = (float)(((double)t[2] * t[5] - (double)t[3] * t[4]) * invdet);
	inv[1] = (float)(-t[1] * invdet);
	inv[3] = (float)(t[0] * invdet);
	inv[5] = (float)(((double)t[1] * t[4] - (double)t[0] * t[5]) * invdet);
}

static void nsvg__xformPremultiply(float* t, float* s)
{
	float s2[6];
	memcpy(s2, s, sizeof(float) * 6);
	nsvg__xformMultiply(s2, t);
	memcpy(t, s2, sizeof(float) * 6);
}

static void nsvg__xformPoint(float* dx, float* dy, float x, float y, float* t)
{
	*dx = x*t[0] + y*t[2] + t[4];
	*dy = x*t[1] + y*t[3] + t[5];
}

static void nsvg__xformVec(float* dx, float* dy, float x, float y, float* t)
{
	*dx = x*t[0] + y*t[2];
	*dy = x*t[1] + y*t[3];
}



/*
   orientantion of three points,
   -1 = clockwise if the y axis points up
*/
static int nsvg__side(float x0, float y0, float x1, float y1, float x2, float y2)
{
	float cp = (x1 - x0) * (y2 - y1) - (x2 - x0) * (y1 - y0);
	return cp < 0 ? -1: cp > 0 ? 1 : 0;
}

static int nsvg__ptInBounds(float* pt, float* bounds)
{
	return pt[0] >= bounds[0] && pt[0] <= bounds[2] && pt[1] >= bounds[1] && pt[1] <= bounds[3];
}


static double nsvg__evalBezier(double t, double p0, double p1, double p2, double p3)
{
	double it = 1.0 - t;
	return it*it*it*p0 + 3.0*it*it*t*p1 + 3.0*it*t*t*p2 + t*t*t*p3;
}

static void nsvg__curveBounds(float* bounds, float* curve)
{
	int i, j, count;
	double roots[2], a, b, c, b2ac, t, v;
	float* v0 = &curve[0];
	float* v1 = &curve[2];
	float* v2 = &curve[4];
	float* v3 = &curve[6];

	// Start the bounding box by end points
	bounds[0] = nsvg__minf(v0[0], v3[0]);
	bounds[1] = nsvg__minf(v0[1], v3[1]);
	bounds[2] = nsvg__maxf(v0[0], v3[0]);
	bounds[3] = nsvg__maxf(v0[1], v3[1]);

	// Bezier curve fits inside the convex hull of it's control points.
	// If control points are inside the bounds, we're done.
	if (nsvg__ptInBounds(v1, bounds) && nsvg__ptInBounds(v2, bounds))
		return;

	// Add bezier curve inflection points in X and Y.
	for (i = 0; i < 2; i++) {
		a = -3.0 * v0[i] + 9.0 * v1[i] - 9.0 * v2[i] + 3.0 * v3[i];
		b = 6.0 * v0[i] - 12.0 * v1[i] + 6.0 * v2[i];
		c = 3.0 * v1[i] - 3.0 * v0[i];
		count = 0;
		if (fabs(a) < NSVG_EPSILON) {
			if (fabs(b) > NSVG_EPSILON) {
				t = -c / b;
				if (t > NSVG_EPSILON && t < 1.0 - NSVG_EPSILON)
					roots[count++] = t;
			}
		}
		else {
			b2ac = b*b - 4.0*c*a;
			if (b2ac > NSVG_EPSILON) {
				t = (-b + sqrt(b2ac)) / (2.0 * a);
				if (t > NSVG_EPSILON && t < 1.0 - NSVG_EPSILON)
					roots[count++] = t;
				t = (-b - sqrt(b2ac)) / (2.0 * a);
				if (t > NSVG_EPSILON && t < 1.0 - NSVG_EPSILON)
					roots[count++] = t;
			}
		}
		for (j = 0; j < count; j++) {
			v = nsvg__evalBezier(roots[j], v0[i], v1[i], v2[i], v3[i]);
			bounds[0 + i] = nsvg__minf(bounds[0 + i], (float)v);
			bounds[2 + i] = nsvg__maxf(bounds[2 + i], (float)v);
		}
	}
}

#define	TwoPi  6.28318530717958648
const double eps = 1e-14;
//---------------------------------------------------------------------------
// x - array of size 3
// In case 3 real roots: => x[0], x[1], x[2], return 3
//         2 real roots: x[0], x[1],          return 2
//         1 real root : x[0], x[1] ± i*x[2], return 1
static int SolveP3(double *x, double a, double b, double c) {	// solve cubic equation x^3 + a*x^2 + b*x + c
	double a2 = a*a;
	double q = (a2 - 3 * b) / 9;
	double r = (a*(2 * a2 - 9 * b) + 27 * c) / 54;
	double r2 = r*r;
	double q3 = q*q*q;
	double A, B;
	if (r2<q3) {
		double t = r / sqrt(q3);
		if (t<-1) t = -1;
		if (t> 1) t = 1;
		t = acos(t);
		a /= 3; q = -2 * sqrt(q);
		x[0] = q*cos(t / 3) - a;
		x[1] = q*cos((t + TwoPi) / 3) - a;
		x[2] = q*cos((t - TwoPi) / 3) - a;
		return(3);
	}
	else {
		A = -pow(fabs(r) + sqrt(r2 - q3), 1.0 / 3);
		if (r<0) A = -A;
		B = A == 0 ? 0 :  q / A;
		a /= 3;
		x[0] = (A + B) - a;
		x[1] = -0.5*(A + B) - a;
		x[2] = 0.5*sqrt(3.)*(A - B);
		if (fabs(x[2])<eps) { x[2] = x[1]; return 2; }
		return 1 ;
	}
}

static int cubic_roots(double *x, double A, double B, double C, double D)
{
	if (fabs(A) > NSVG_EPSILON)
	{
		return SolveP3(x, B / A, C / A, D / A);
	}
	else
	{
		int count = 0;
		double t;
		double b2ac;

		if (fabs(B) < NSVG_EPSILON) {
			if (fabs(C) > NSVG_EPSILON) {
				t = -D / C;
				if (t > NSVG_EPSILON && t < 1.0 - NSVG_EPSILON)
					x[count++] = t;
			}
		}
		else {
			b2ac = C*C - 4.0*D*B;
			if (b2ac > NSVG_EPSILON) {
				t = (-C + sqrt(b2ac)) / (2.0 * B);
				if (t > NSVG_EPSILON && t < 1.0 - NSVG_EPSILON)
					x[count++] = t;
				t = (-C - sqrt(b2ac)) / (2.0 * B);
				if (t > NSVG_EPSILON && t < 1.0 - NSVG_EPSILON)
					x[count++] = t;
			}
		}
		return count;
	}
}

static int nsvg__curveIntersectXRay(float *tret, float* curve, float x, float y)
{
	float bounds[4];
	float Ay, By, Cy, Dy;
	double t[3];
	int N;
	double ix;
	int i;
	int j = 0;


	//Dy = curve[1];
	//Cy = 3 * (curve[3] - Dy);
	//By = 3 * (curve[5] - curve[3]) - Cy;
	//Ay = curve[7] - curve[1] - Cy - By;

	Dy = curve[1];
	Cy = 3 * (curve[3] - curve[1]);
	By = 3 * curve[5] - 6 * curve[3] + 3 * curve[1];
	Ay = curve[7] - 3 * curve[5] + 3 * curve[3] - curve[1];

	nsvg__curveBounds(bounds, curve);
	if (y < bounds[1] || y > bounds[3] || x > bounds[2])
		return 0;

	N = cubic_roots(t, Ay, By, Cy, Dy - y);
	
	for (i = 0; i < N; i++)
	{
		if (t[i] >= 0 && t[i] <= 1.0)
		{
			ix = nsvg__evalBezier(t[i], curve[0], curve[2], curve[4], curve[6]);
			if (ix > x)
			{
				tret[j++] = (float) t[i];
			} 
		}

	}
	
	if (y == Dy)
	{
		for (i = 0; i < j; i++)
			if (tret[i] < 0.0001)
				break;
		if (i == j && j < 3)
		{
			ix = nsvg__evalBezier(0.0, curve[0], curve[2], curve[4], curve[6]);
			if (ix > x)
			{
				tret[j++] = 0.0;
			}
		}
	
	}
	if (y == curve[7])
	{
		int k = 0;
		for (i = 0; i < j; i++)
			if (tret[i] < 1.0 - 0.0001)
				tret[k++] = tret[i];
		j = k;
	}

	return j;
}

static nsvg__simplearc(float *ret, double a, double r)
{
	double x1, x2, x3, x4;
	double y1, y2, y3, y4;
	double k = 0.5522847498;
	double cosa, sina;
	
	cosa = cos(a / 2);
	sina = sin(a / 2);
	x4 = r * cos(a / 2);
	y4 = r * sin(a / 2);
	x1 = x4;
	y1 = -y4;

	x2 = x1 + k * tan(a / 2) * y4;
	y2 = y1 + k * tan(a / 2) * x4;
	x3 = x2;
	y3 = -y2;

	ret[0] = (float)(x1 * cosa - y1 * sina);
	ret[1] = (float)(x1 * sina + y1 * cosa);
	ret[2] = (float)(x2 * cosa - y2 * sina);
	ret[3] = (float)(x2 * sina + y2 * cosa);
	ret[4] = (float)(x3 * cosa - y3 * sina);
	ret[5] = (float)(x3 * sina + y3 * cosa);
	ret[6] = (float)(x4 * cosa - y4 * sina);
	ret[7] = (float)(x4 * sina + y4 * cosa);
}

static int nsvg__createarc(float *ret, float ox, float oy, float ax, float ay, float bx, float by, int dir)
{
	float r;
	double thetaa, thetab;
	double dtheta;
	int Nsegs;
	int i;
	float arc[8];
	float cost, sint;

	thetaa = atan2(ay - oy, ax - ox);
	thetab = atan2(by - oy, bx - ox);
	r = (float) sqrt((ax - ox)*(ax - ox) + (ay - oy)*(ay - oy));
	ret[0] = ax;
	ret[1] = ay;
	if (dir == 1)
	{
		if (thetab < thetaa)
			thetab += NSVG_PI * 2;
		Nsegs = (int) ceil((thetab - thetaa) * 2 / NSVG_PI);
		dtheta = (thetab - thetaa) / Nsegs;
		nsvg__simplearc(arc, dtheta, r);

		for (i = 0; i < Nsegs; i++)
		{
			cost = (float) cos(thetaa + dtheta * i);
			sint = (float) sin(thetaa + dtheta * i);
			ret[(i*3 + 1) * 2] = arc[2] * cost - arc[3] * sint + ox;
			ret[(i*3 + 1) * 2+1] = arc[2] * sint + arc[3] * cost + oy;
			ret[(i*3 + 2) * 2] = arc[4] * cost - arc[5] * sint + ox;
			ret[(i*3 + 2) * 2+1] = arc[4] * sint + arc[5] * cost + oy;
			ret[(i*3 + 3) * 2] = arc[6] * cost - arc[7] * sint + ox;
			ret[(i*3 + 3) * 2+1] = arc[6] * sint + arc[7] * cost + oy;
		}
	}
	else
	{
		if (thetaa < thetab)
			thetaa += NSVG_PI * 2;
		Nsegs = (int)ceil((thetaa - thetab) * 2 / NSVG_PI);
		dtheta = (thetab - thetaa) / Nsegs;
		nsvg__simplearc(arc, dtheta, r);

		for (i = 0; i < Nsegs; i++)
		{
			cost = (float) cos(thetaa + dtheta * i);
			sint = (float) sin(thetaa + dtheta * i);
			ret[(i * 3 + 1) * 2] = arc[2] * cost - arc[3] * sint + ox;
			ret[(i * 3 + 1) * 2 + 1] = arc[2] * sint + arc[3] * cost + oy;
			ret[(i * 3 + 2) * 2] = arc[4] * cost - arc[5] * sint + ox;
			ret[(i * 3 + 2) * 2 + 1] = arc[4] * sint + arc[5] * cost + oy;
			ret[(i * 3 + 3) * 2] = arc[6] * cost - arc[7] * sint + ox;
			ret[(i * 3 + 3) * 2 + 1] = arc[6] * sint + arc[7] * cost + oy;
		}
	}
	ret[((Nsegs - 1) * 3 + 3) * 2] = bx;
	ret[((Nsegs - 1) * 3 + 3) * 2+1] = by;

	return Nsegs * 3+1;
}


static void nsvg__deletePaths(NSVGpath* path)
{
	while (path) {
		NSVGpath *next = path->next;
		if (path->pts != NULL)
			free(path->pts);
		free(path);
		path = next;
	}
}

static void nsvg__deletePaint(NSVGpaint* paint)
{
	if (paint->type == NSVG_PAINT_LINEAR_GRADIENT || paint->type == NSVG_PAINT_RADIAL_GRADIENT)
		free(paint->gradient);
}

static void nsvg__deleteGradientData(NSVGgradientData* grad)
{
	NSVGgradientData* next;
	while (grad != NULL) {
		next = grad->next;
		free(grad->stops);
		free(grad);
		grad = next;
	}
}

static float nsvg__getAverageScale(float* t)
{
	float sx = sqrtf(t[0] * t[0] + t[2] * t[2]);
	float sy = sqrtf(t[1] * t[1] + t[3] * t[3]);
	return (sx + sy) * 0.5f;
}

static int nsvg__isspace(int ch)
{
	static char *spaces = " \t\n\r\b";
	int i;

	for (i = 0; spaces[i]; i++)
		if (ch == spaces[i])
			return 1;
	return 0;
}

static unsigned int nsvg__parseColorHex(const char* str)
{
	unsigned int c = 0, r = 0, g = 0, b = 0;
	int n = 0;
	str++; // skip #
		   // Calculate number of characters.
	while (str[n] && !nsvg__isspace(str[n]))
		n++;
	if (n == 6) {
		sscanf(str, "%x", &c);
	}
	else if (n == 3) {
		sscanf(str, "%x", &c);
		c = (c & 0xf) | ((c & 0xf0) << 4) | ((c & 0xf00) << 8);
		c |= c << 4;
	}
	r = (c >> 16) & 0xff;
	g = (c >> 8) & 0xff;
	b = c & 0xff;
	return NSVG_RGB(r, g, b);
}



static unsigned int nsvg__parseColorRGB(const char* str)
{
	int r = -1, g = -1, b = -1;
	char s1[32] = "", s2[32] = "";
	sscanf(str + 4, "%d%[%%, \t]%d%[%%, \t]%d", &r, s1, &g, s2, &b);
	if (strchr(s1, '%')) {
		return NSVG_RGB((r * 255) / 100, (g * 255) / 100, (b * 255) / 100);
	}
	else {
		return NSVG_RGB(r, g, b);
	}
}


typedef struct NSVGNamedColor {
	const char* name;
	unsigned int color;
} NSVGNamedColor;

NSVGNamedColor nsvg__colors[] = {

	{ "red", NSVG_RGB(255, 0, 0) },
	{ "green", NSVG_RGB(0, 128, 0) },
	{ "blue", NSVG_RGB(0, 0, 255) },
	{ "yellow", NSVG_RGB(255, 255, 0) },
	{ "cyan", NSVG_RGB(0, 255, 255) },
	{ "magenta", NSVG_RGB(255, 0, 255) },
	{ "black", NSVG_RGB(0, 0, 0) },
	{ "grey", NSVG_RGB(128, 128, 128) },
	{ "gray", NSVG_RGB(128, 128, 128) },
	{ "white", NSVG_RGB(255, 255, 255) },

#ifdef NANOSVG_ALL_COLOR_KEYWORDS
{ "aliceblue", NSVG_RGB(240, 248, 255) },
{ "antiquewhite", NSVG_RGB(250, 235, 215) },
{ "aqua", NSVG_RGB(0, 255, 255) },
{ "aquamarine", NSVG_RGB(127, 255, 212) },
{ "azure", NSVG_RGB(240, 255, 255) },
{ "beige", NSVG_RGB(245, 245, 220) },
{ "bisque", NSVG_RGB(255, 228, 196) },
{ "blanchedalmond", NSVG_RGB(255, 235, 205) },
{ "blueviolet", NSVG_RGB(138, 43, 226) },
{ "brown", NSVG_RGB(165, 42, 42) },
{ "burlywood", NSVG_RGB(222, 184, 135) },
{ "cadetblue", NSVG_RGB(95, 158, 160) },
{ "chartreuse", NSVG_RGB(127, 255, 0) },
{ "chocolate", NSVG_RGB(210, 105, 30) },
{ "coral", NSVG_RGB(255, 127, 80) },
{ "cornflowerblue", NSVG_RGB(100, 149, 237) },
{ "cornsilk", NSVG_RGB(255, 248, 220) },
{ "crimson", NSVG_RGB(220, 20, 60) },
{ "darkblue", NSVG_RGB(0, 0, 139) },
{ "darkcyan", NSVG_RGB(0, 139, 139) },
{ "darkgoldenrod", NSVG_RGB(184, 134, 11) },
{ "darkgray", NSVG_RGB(169, 169, 169) },
{ "darkgreen", NSVG_RGB(0, 100, 0) },
{ "darkgrey", NSVG_RGB(169, 169, 169) },
{ "darkkhaki", NSVG_RGB(189, 183, 107) },
{ "darkmagenta", NSVG_RGB(139, 0, 139) },
{ "darkolivegreen", NSVG_RGB(85, 107, 47) },
{ "darkorange", NSVG_RGB(255, 140, 0) },
{ "darkorchid", NSVG_RGB(153, 50, 204) },
{ "darkred", NSVG_RGB(139, 0, 0) },
{ "darksalmon", NSVG_RGB(233, 150, 122) },
{ "darkseagreen", NSVG_RGB(143, 188, 143) },
{ "darkslateblue", NSVG_RGB(72, 61, 139) },
{ "darkslategray", NSVG_RGB(47, 79, 79) },
{ "darkslategrey", NSVG_RGB(47, 79, 79) },
{ "darkturquoise", NSVG_RGB(0, 206, 209) },
{ "darkviolet", NSVG_RGB(148, 0, 211) },
{ "deeppink", NSVG_RGB(255, 20, 147) },
{ "deepskyblue", NSVG_RGB(0, 191, 255) },
{ "dimgray", NSVG_RGB(105, 105, 105) },
{ "dimgrey", NSVG_RGB(105, 105, 105) },
{ "dodgerblue", NSVG_RGB(30, 144, 255) },
{ "firebrick", NSVG_RGB(178, 34, 34) },
{ "floralwhite", NSVG_RGB(255, 250, 240) },
{ "forestgreen", NSVG_RGB(34, 139, 34) },
{ "fuchsia", NSVG_RGB(255, 0, 255) },
{ "gainsboro", NSVG_RGB(220, 220, 220) },
{ "ghostwhite", NSVG_RGB(248, 248, 255) },
{ "gold", NSVG_RGB(255, 215, 0) },
{ "goldenrod", NSVG_RGB(218, 165, 32) },
{ "greenyellow", NSVG_RGB(173, 255, 47) },
{ "honeydew", NSVG_RGB(240, 255, 240) },
{ "hotpink", NSVG_RGB(255, 105, 180) },
{ "indianred", NSVG_RGB(205, 92, 92) },
{ "indigo", NSVG_RGB(75, 0, 130) },
{ "ivory", NSVG_RGB(255, 255, 240) },
{ "khaki", NSVG_RGB(240, 230, 140) },
{ "lavender", NSVG_RGB(230, 230, 250) },
{ "lavenderblush", NSVG_RGB(255, 240, 245) },
{ "lawngreen", NSVG_RGB(124, 252, 0) },
{ "lemonchiffon", NSVG_RGB(255, 250, 205) },
{ "lightblue", NSVG_RGB(173, 216, 230) },
{ "lightcoral", NSVG_RGB(240, 128, 128) },
{ "lightcyan", NSVG_RGB(224, 255, 255) },
{ "lightgoldenrodyellow", NSVG_RGB(250, 250, 210) },
{ "lightgray", NSVG_RGB(211, 211, 211) },
{ "lightgreen", NSVG_RGB(144, 238, 144) },
{ "lightgrey", NSVG_RGB(211, 211, 211) },
{ "lightpink", NSVG_RGB(255, 182, 193) },
{ "lightsalmon", NSVG_RGB(255, 160, 122) },
{ "lightseagreen", NSVG_RGB(32, 178, 170) },
{ "lightskyblue", NSVG_RGB(135, 206, 250) },
{ "lightslategray", NSVG_RGB(119, 136, 153) },
{ "lightslategrey", NSVG_RGB(119, 136, 153) },
{ "lightsteelblue", NSVG_RGB(176, 196, 222) },
{ "lightyellow", NSVG_RGB(255, 255, 224) },
{ "lime", NSVG_RGB(0, 255, 0) },
{ "limegreen", NSVG_RGB(50, 205, 50) },
{ "linen", NSVG_RGB(250, 240, 230) },
{ "maroon", NSVG_RGB(128, 0, 0) },
{ "mediumaquamarine", NSVG_RGB(102, 205, 170) },
{ "mediumblue", NSVG_RGB(0, 0, 205) },
{ "mediumorchid", NSVG_RGB(186, 85, 211) },
{ "mediumpurple", NSVG_RGB(147, 112, 219) },
{ "mediumseagreen", NSVG_RGB(60, 179, 113) },
{ "mediumslateblue", NSVG_RGB(123, 104, 238) },
{ "mediumspringgreen", NSVG_RGB(0, 250, 154) },
{ "mediumturquoise", NSVG_RGB(72, 209, 204) },
{ "mediumvioletred", NSVG_RGB(199, 21, 133) },
{ "midnightblue", NSVG_RGB(25, 25, 112) },
{ "mintcream", NSVG_RGB(245, 255, 250) },
{ "mistyrose", NSVG_RGB(255, 228, 225) },
{ "moccasin", NSVG_RGB(255, 228, 181) },
{ "navajowhite", NSVG_RGB(255, 222, 173) },
{ "navy", NSVG_RGB(0, 0, 128) },
{ "oldlace", NSVG_RGB(253, 245, 230) },
{ "olive", NSVG_RGB(128, 128, 0) },
{ "olivedrab", NSVG_RGB(107, 142, 35) },
{ "orange", NSVG_RGB(255, 165, 0) },
{ "orangered", NSVG_RGB(255, 69, 0) },
{ "orchid", NSVG_RGB(218, 112, 214) },
{ "palegoldenrod", NSVG_RGB(238, 232, 170) },
{ "palegreen", NSVG_RGB(152, 251, 152) },
{ "paleturquoise", NSVG_RGB(175, 238, 238) },
{ "palevioletred", NSVG_RGB(219, 112, 147) },
{ "papayawhip", NSVG_RGB(255, 239, 213) },
{ "peachpuff", NSVG_RGB(255, 218, 185) },
{ "peru", NSVG_RGB(205, 133, 63) },
{ "pink", NSVG_RGB(255, 192, 203) },
{ "plum", NSVG_RGB(221, 160, 221) },
{ "powderblue", NSVG_RGB(176, 224, 230) },
{ "purple", NSVG_RGB(128, 0, 128) },
{ "rosybrown", NSVG_RGB(188, 143, 143) },
{ "royalblue", NSVG_RGB(65, 105, 225) },
{ "saddlebrown", NSVG_RGB(139, 69, 19) },
{ "salmon", NSVG_RGB(250, 128, 114) },
{ "sandybrown", NSVG_RGB(244, 164, 96) },
{ "seagreen", NSVG_RGB(46, 139, 87) },
{ "seashell", NSVG_RGB(255, 245, 238) },
{ "sienna", NSVG_RGB(160, 82, 45) },
{ "silver", NSVG_RGB(192, 192, 192) },
{ "skyblue", NSVG_RGB(135, 206, 235) },
{ "slateblue", NSVG_RGB(106, 90, 205) },
{ "slategray", NSVG_RGB(112, 128, 144) },
{ "slategrey", NSVG_RGB(112, 128, 144) },
{ "snow", NSVG_RGB(255, 250, 250) },
{ "springgreen", NSVG_RGB(0, 255, 127) },
{ "steelblue", NSVG_RGB(70, 130, 180) },
{ "tan", NSVG_RGB(210, 180, 140) },
{ "teal", NSVG_RGB(0, 128, 128) },
{ "thistle", NSVG_RGB(216, 191, 216) },
{ "tomato", NSVG_RGB(255, 99, 71) },
{ "turquoise", NSVG_RGB(64, 224, 208) },
{ "violet", NSVG_RGB(238, 130, 238) },
{ "wheat", NSVG_RGB(245, 222, 179) },
{ "whitesmoke", NSVG_RGB(245, 245, 245) },
{ "yellowgreen", NSVG_RGB(154, 205, 50) },
#endif
};

static unsigned int nsvg__parseColorName(const char* str)
{
	int i, ncolors = sizeof(nsvg__colors) / sizeof(NSVGNamedColor);

	for (i = 0; i < ncolors; i++) {
		if (strcmp(nsvg__colors[i].name, str) == 0) {
			return nsvg__colors[i].color;
		}
	}

	return NSVG_RGB(128, 128, 128);
}

static unsigned int nsvg__parseColor(const char* str)
{
	size_t len = 0;
	while (*str == ' ') ++str;
	len = strlen(str);
	if (len >= 1 && *str == '#')
		return nsvg__parseColorHex(str);
	else if (len >= 4 && str[0] == 'r' && str[1] == 'g' && str[2] == 'b' && str[3] == '(')
		return nsvg__parseColorRGB(str);
	return nsvg__parseColorName(str);
}

static float nsvg__sqr(float x) { return x*x; }
static float nsvg__vmag(float x, float y) { return sqrtf(x*x + y*y); }

static float nsvg__vecrat(float ux, float uy, float vx, float vy)
{
	return (ux*vx + uy*vy) / (nsvg__vmag(ux, uy) * nsvg__vmag(vx, vy));
}

static float nsvg__vecang(float ux, float uy, float vx, float vy)
{
	float r = nsvg__vecrat(ux, uy, vx, vy);
	if (r < -1.0f) r = -1.0f;
	if (r > 1.0f) r = 1.0f;
	return ((ux*vy < uy*vx) ? -1.0f : 1.0f) * acosf(r);
}

static float nsvg__viewAlign(float content, float container, int type)
{
	if (type == NSVG_ALIGN_MIN)
		return 0;
	else if (type == NSVG_ALIGN_MAX)
		return container - content;
	// mid
	return (container - content) * 0.5f;
}

static void nsvg__scaleGradient(NSVGgradient* grad, float tx, float ty, float sx, float sy)
{
	grad->xform[0] *= sx;
	grad->xform[1] *= sx;
	grad->xform[2] *= sy;
	grad->xform[3] *= sy;
	grad->xform[4] += tx*sx;
	grad->xform[5] += ty*sx;
}



/*
* Copyright (c) 2013-14 Mikko Mononen memon@inside.org
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*
* The polygon rasterization is heavily based on stb_truetype rasterizer
* by Sean Barrett - http://nothings.org/
*
*/

	typedef struct NSVGrasterizer NSVGrasterizer;

	/* Example Usage:
	// Load SVG
	struct SNVGImage* image = nsvgParseFromFile("test.svg.");

	// Create rasterizer (can be used to render multiple images).
	struct NSVGrasterizer* rast = nsvgCreateRasterizer();
	// Allocate memory for image
	unsigned char* img = malloc(w*h*4);
	// Rasterize
	nsvgRasterize(rast, image, 0,0,1, img, w, h, w*4);
	*/

	// Allocated rasterizer context.
	NSVGrasterizer* nsvgCreateRasterizer();

	// Rasterizes SVG image, returns RGBA image (non-premultiplied alpha)
	//   r - pointer to rasterizer context
	//   image - pointer to image to rasterize
	//   tx,ty - image offset (applied after scaling)
	//   scale - image scale
	//   dst - pointer to destination image data, 4 bytes per pixel (RGBA)
	//   w - width of the image to render
	//   h - height of the image to render
	//   stride - number of bytes per scaleline in the destination buffer
	void nsvgRasterize(NSVGrasterizer* r,
		NSVGimage* image, float tx, float ty, float scale,
		unsigned char* dst, int w, int h, int stride);

	// Deletes rasterizer context.
	void nsvgDeleteRasterizer(NSVGrasterizer*);

#define NSVG__SUBSAMPLES	5
#define NSVG__FIXSHIFT		10
#define NSVG__FIX			(1 << NSVG__FIXSHIFT)
#define NSVG__FIXMASK		(NSVG__FIX-1)
#define NSVG__MEMPAGE_SIZE	1024

typedef struct NSVGedge {
	float x0, y0, x1, y1;
	int dir;
	struct NSVGedge* next;
} NSVGedge;

typedef struct NSVGpoint {
	float x, y;
	float dx, dy;
	float len;
	float dmx, dmy;
	unsigned char flags;
} NSVGpoint;

typedef struct NSVGactiveEdge {
	int x, dx;
	float ey;
	int dir;
	struct NSVGactiveEdge *next;
} NSVGactiveEdge;

typedef struct NSVGmemPage {
	unsigned char mem[NSVG__MEMPAGE_SIZE];
	int size;
	struct NSVGmemPage* next;
} NSVGmemPage;

typedef struct NSVGcachedPaint {
	char type;
	char spread;
	float xform[6];
	unsigned int colors[256];
	unsigned char *bitmap;
	int twidth;
	int theight;
} NSVGcachedPaint;

struct NSVGrasterizer
{
	float px, py;

	float tessTol;
	float distTol;

	NSVGedge* edges;
	int nedges;
	int cedges;

	NSVGpoint* points;
	int npoints;
	int cpoints;

	NSVGactiveEdge* freelist;
	NSVGmemPage* pages;
	NSVGmemPage* curpage;

	unsigned char* scanline;
	int cscanline;

	unsigned char* bitmap;
	int width, height, stride;
	unsigned char *clipmask;
};

NSVGrasterizer* nsvgCreateRasterizer()
{
	NSVGrasterizer* r = (NSVGrasterizer*)malloc(sizeof(NSVGrasterizer));
	if (r == NULL) goto error;
	memset(r, 0, sizeof(NSVGrasterizer));

	r->tessTol = 0.25f;
	r->distTol = 0.01f;

	return r;

error:
	nsvgDeleteRasterizer(r);
	return NULL;
}

void nsvgDeleteRasterizer(NSVGrasterizer* r)
{
	NSVGmemPage* p;

	if (r == NULL) return;

	p = r->pages;
	while (p != NULL) {
		NSVGmemPage* next = p->next;
		free(p);
		p = next;
	}

	if (r->edges) free(r->edges);
	if (r->points) free(r->points);
	if (r->scanline) free(r->scanline);

	free(r);
}

static NSVGmemPage* nsvg__nextPage(NSVGrasterizer* r, NSVGmemPage* cur)
{
	NSVGmemPage *newp;

	// If using existing chain, return the next page in chain
	if (cur != NULL && cur->next != NULL) {
		return cur->next;
	}

	// Alloc new page
	newp = (NSVGmemPage*)malloc(sizeof(NSVGmemPage));
	if (newp == NULL) return NULL;
	memset(newp, 0, sizeof(NSVGmemPage));

	// Add to linked list
	if (cur != NULL)
		cur->next = newp;
	else
		r->pages = newp;

	return newp;
}

static void nsvg__resetPool(NSVGrasterizer* r)
{
	NSVGmemPage* p = r->pages;
	while (p != NULL) {
		p->size = 0;
		p = p->next;
	}
	r->curpage = r->pages;
}

static unsigned char* nsvg__alloc(NSVGrasterizer* r, int size)
{
	unsigned char* buf;
	if (size > NSVG__MEMPAGE_SIZE) return NULL;
	if (r->curpage == NULL || r->curpage->size + size > NSVG__MEMPAGE_SIZE) {
		r->curpage = nsvg__nextPage(r, r->curpage);
	}
	buf = &r->curpage->mem[r->curpage->size];
	r->curpage->size += size;
	return buf;
}

static int nsvg__ptEquals(float x1, float y1, float x2, float y2, float tol)
{
	float dx = x2 - x1;
	float dy = y2 - y1;
	return dx*dx + dy*dy < tol*tol;
}

static void nsvg__addPathPoint(NSVGrasterizer* r, float x, float y, int flags)
{
	NSVGpoint* pt;

	if (r->npoints > 0) {
		pt = &r->points[r->npoints - 1];
		if (nsvg__ptEquals(pt->x, pt->y, x, y, r->distTol)) {
			pt->flags |= flags;
			return;
		}
	}

	if (r->npoints + 1 > r->cpoints) {
		r->cpoints = r->cpoints > 0 ? r->cpoints * 2 : 64;
		r->points = (NSVGpoint*)realloc(r->points, sizeof(NSVGpoint) * r->cpoints);
		if (r->points == NULL) return;
	}

	pt = &r->points[r->npoints];
	pt->x = x;
	pt->y = y;
	pt->flags = (unsigned char)flags;
	r->npoints++;
}

static void nsvg__addEdge(NSVGrasterizer* r, float x0, float y0, float x1, float y1)
{
	NSVGedge* e;

	// Skip horizontal edges
	if (y0 == y1)
		return;

	if (r->nedges + 1 > r->cedges) {
		r->cedges = r->cedges > 0 ? r->cedges * 2 : 64;
		r->edges = (NSVGedge*)realloc(r->edges, sizeof(NSVGedge) * r->cedges);
		if (r->edges == NULL) return;
	}

	e = &r->edges[r->nedges];
	r->nedges++;

	if (y0 < y1) {
		e->x0 = x0;
		e->y0 = y0;
		e->x1 = x1;
		e->y1 = y1;
		e->dir = 1;
	}
	else {
		e->x0 = x1;
		e->y0 = y1;
		e->x1 = x0;
		e->y1 = y0;
		e->dir = -1;
	}
}

static float nsvg__normalize(float *x, float* y)
{
	float d = sqrtf((*x)*(*x) + (*y)*(*y));
	if (d > 1e-6f) {
		float id = 1.0f / d;
		*x *= id;
		*y *= id;
	}
	return d;
}

static float nsvg__absf(float x) { return x < 0 ? -x : x; }

static void nsvg__flattenCubicBez(NSVGrasterizer* r,
	float x1, float y1, float x2, float y2,
	float x3, float y3, float x4, float y4,
	int level, int type)
{
	float x12, y12, x23, y23, x34, y34, x123, y123, x234, y234, x1234, y1234;
	float dx, dy, d2, d3;

	if (level > 10) return;

	x12 = (x1 + x2)*0.5f;
	y12 = (y1 + y2)*0.5f;
	x23 = (x2 + x3)*0.5f;
	y23 = (y2 + y3)*0.5f;
	x34 = (x3 + x4)*0.5f;
	y34 = (y3 + y4)*0.5f;
	x123 = (x12 + x23)*0.5f;
	y123 = (y12 + y23)*0.5f;

	dx = x4 - x1;
	dy = y4 - y1;
	d2 = nsvg__absf(((x2 - x4) * dy - (y2 - y4) * dx));
	d3 = nsvg__absf(((x3 - x4) * dy - (y3 - y4) * dx));

	if ((d2 + d3)*(d2 + d3) < r->tessTol * (dx*dx + dy*dy)) {
		nsvg__addPathPoint(r, x4, y4, type);
		return;
	}

	x234 = (x23 + x34)*0.5f;
	y234 = (y23 + y34)*0.5f;
	x1234 = (x123 + x234)*0.5f;
	y1234 = (y123 + y234)*0.5f;

	nsvg__flattenCubicBez(r, x1, y1, x12, y12, x123, y123, x1234, y1234, level + 1, 0);
	nsvg__flattenCubicBez(r, x1234, y1234, x234, y234, x34, y34, x4, y4, level + 1, type);
}

static void nsvg__flattenShape(NSVGrasterizer* r, NSVGshape* shape, float scale)
{
	int i, j;
	NSVGpath* path;

	for (path = shape->paths; path != NULL; path = path->next) {
		r->npoints = 0;
		// Flatten path
		nsvg__addPathPoint(r, path->pts[0] * scale, path->pts[1] * scale, 0);
		for (i = 0; i < path->npts - 1; i += 3) {
			float* p = &path->pts[i * 2];
			nsvg__flattenCubicBez(r, p[0] * scale, p[1] * scale, p[2] * scale, p[3] * scale, p[4] * scale, p[5] * scale, p[6] * scale, p[7] * scale, 0, 0);
		}
		// Close path
		nsvg__addPathPoint(r, path->pts[0] * scale, path->pts[1] * scale, 0);
		// Build edges
		for (i = 0, j = r->npoints - 1; i < r->npoints; j = i++)
			nsvg__addEdge(r, r->points[j].x, r->points[j].y, r->points[i].x, r->points[i].y);
	}
}

enum NSVGpointFlags
{
	NSVG_PT_CORNER = 0x01,
	NSVG_PT_BEVEL = 0x02,
	NSVG_PT_LEFT = 0x04,
};

static void nsvg__initClosed(NSVGpoint* left, NSVGpoint* right, NSVGpoint* p0, NSVGpoint* p1, float lineWidth)
{
	float w = lineWidth * 0.5f;
	float dx = p1->x - p0->x;
	float dy = p1->y - p0->y;
	float len = nsvg__normalize(&dx, &dy);
	float px = p0->x + dx*len*0.5f, py = p0->y + dy*len*0.5f;
	float dlx = dy, dly = -dx;
	float lx = px - dlx*w, ly = py - dly*w;
	float rx = px + dlx*w, ry = py + dly*w;
	left->x = lx; left->y = ly;
	right->x = rx; right->y = ry;
}

static void nsvg__buttCap(NSVGrasterizer* r, NSVGpoint* left, NSVGpoint* right, NSVGpoint* p, float dx, float dy, float lineWidth, int connect)
{
	float w = lineWidth * 0.5f;
	float px = p->x, py = p->y;
	float dlx = dy, dly = -dx;
	float lx = px - dlx*w, ly = py - dly*w;
	float rx = px + dlx*w, ry = py + dly*w;

	nsvg__addEdge(r, lx, ly, rx, ry);

	if (connect) {
		nsvg__addEdge(r, left->x, left->y, lx, ly);
		nsvg__addEdge(r, rx, ry, right->x, right->y);
	}
	left->x = lx; left->y = ly;
	right->x = rx; right->y = ry;
}

static void nsvg__squareCap(NSVGrasterizer* r, NSVGpoint* left, NSVGpoint* right, NSVGpoint* p, float dx, float dy, float lineWidth, int connect)
{
	float w = lineWidth * 0.5f;
	float px = p->x - dx*w, py = p->y - dy*w;
	float dlx = dy, dly = -dx;
	float lx = px - dlx*w, ly = py - dly*w;
	float rx = px + dlx*w, ry = py + dly*w;

	nsvg__addEdge(r, lx, ly, rx, ry);

	if (connect) {
		nsvg__addEdge(r, left->x, left->y, lx, ly);
		nsvg__addEdge(r, rx, ry, right->x, right->y);
	}
	left->x = lx; left->y = ly;
	right->x = rx; right->y = ry;
}

static void nsvg__roundCap(NSVGrasterizer* r, NSVGpoint* left, NSVGpoint* right, NSVGpoint* p, float dx, float dy, float lineWidth, int ncap, int connect)
{
	int i;
	float w = lineWidth * 0.5f;
	float px = p->x, py = p->y;
	float dlx = dy, dly = -dx;
	float lx = 0, ly = 0, rx = 0, ry = 0, prevx = 0, prevy = 0;

	for (i = 0; i < ncap; i++) {
		float a = i / (float)(ncap - 1)*NSVG_PI;
		float ax = cosf(a) * w, ay = sinf(a) * w;
		float x = px - dlx*ax - dx*ay;
		float y = py - dly*ax - dy*ay;

		if (i > 0)
			nsvg__addEdge(r, prevx, prevy, x, y);

		prevx = x;
		prevy = y;

		if (i == 0) {
			lx = x; ly = y;
		}
		else if (i == ncap - 1) {
			rx = x; ry = y;
		}
	}

	if (connect) {
		nsvg__addEdge(r, left->x, left->y, lx, ly);
		nsvg__addEdge(r, rx, ry, right->x, right->y);
	}

	left->x = lx; left->y = ly;
	right->x = rx; right->y = ry;
}

static void nsvg__bevelJoin(NSVGrasterizer* r, NSVGpoint* left, NSVGpoint* right, NSVGpoint* p0, NSVGpoint* p1, float lineWidth)
{
	float w = lineWidth * 0.5f;
	float dlx0 = p0->dy, dly0 = -p0->dx;
	float dlx1 = p1->dy, dly1 = -p1->dx;
	float lx0 = p1->x - (dlx0 * w), ly0 = p1->y - (dly0 * w);
	float rx0 = p1->x + (dlx0 * w), ry0 = p1->y + (dly0 * w);
	float lx1 = p1->x - (dlx1 * w), ly1 = p1->y - (dly1 * w);
	float rx1 = p1->x + (dlx1 * w), ry1 = p1->y + (dly1 * w);

	nsvg__addEdge(r, lx0, ly0, left->x, left->y);
	nsvg__addEdge(r, lx1, ly1, lx0, ly0);

	nsvg__addEdge(r, right->x, right->y, rx0, ry0);
	nsvg__addEdge(r, rx0, ry0, rx1, ry1);

	left->x = lx1; left->y = ly1;
	right->x = rx1; right->y = ry1;
}

static void nsvg__miterJoin(NSVGrasterizer* r, NSVGpoint* left, NSVGpoint* right, NSVGpoint* p0, NSVGpoint* p1, float lineWidth)
{
	float w = lineWidth * 0.5f;
	float dlx0 = p0->dy, dly0 = -p0->dx;
	float dlx1 = p1->dy, dly1 = -p1->dx;
	float lx0, rx0, lx1, rx1;
	float ly0, ry0, ly1, ry1;

	if (p1->flags & NSVG_PT_LEFT) {
		lx0 = lx1 = p1->x - p1->dmx * w;
		ly0 = ly1 = p1->y - p1->dmy * w;
		nsvg__addEdge(r, lx1, ly1, left->x, left->y);

		rx0 = p1->x + (dlx0 * w);
		ry0 = p1->y + (dly0 * w);
		rx1 = p1->x + (dlx1 * w);
		ry1 = p1->y + (dly1 * w);
		nsvg__addEdge(r, right->x, right->y, rx0, ry0);
		nsvg__addEdge(r, rx0, ry0, rx1, ry1);
	}
	else {
		lx0 = p1->x - (dlx0 * w);
		ly0 = p1->y - (dly0 * w);
		lx1 = p1->x - (dlx1 * w);
		ly1 = p1->y - (dly1 * w);
		nsvg__addEdge(r, lx0, ly0, left->x, left->y);
		nsvg__addEdge(r, lx1, ly1, lx0, ly0);

		rx0 = rx1 = p1->x + p1->dmx * w;
		ry0 = ry1 = p1->y + p1->dmy * w;
		nsvg__addEdge(r, right->x, right->y, rx1, ry1);
	}

	left->x = lx1; left->y = ly1;
	right->x = rx1; right->y = ry1;
}

static void nsvg__roundJoin(NSVGrasterizer* r, NSVGpoint* left, NSVGpoint* right, NSVGpoint* p0, NSVGpoint* p1, float lineWidth, int ncap)
{
	int i, n;
	float w = lineWidth * 0.5f;
	float dlx0 = p0->dy, dly0 = -p0->dx;
	float dlx1 = p1->dy, dly1 = -p1->dx;
	float a0 = atan2f(dly0, dlx0);
	float a1 = atan2f(dly1, dlx1);
	float da = a1 - a0;
	float lx, ly, rx, ry;

	if (da < NSVG_PI) da += NSVG_PI * 2;
	if (da > NSVG_PI) da -= NSVG_PI * 2;

	n = (int)ceilf((nsvg__absf(da) / NSVG_PI) * ncap);
	if (n < 2) n = 2;
	if (n > ncap) n = ncap;

	lx = left->x;
	ly = left->y;
	rx = right->x;
	ry = right->y;

	for (i = 0; i < n; i++) {
		float u = i / (float)(n - 1);
		float a = a0 + u*da;
		float ax = cosf(a) * w, ay = sinf(a) * w;
		float lx1 = p1->x - ax, ly1 = p1->y - ay;
		float rx1 = p1->x + ax, ry1 = p1->y + ay;

		nsvg__addEdge(r, lx1, ly1, lx, ly);
		nsvg__addEdge(r, rx, ry, rx1, ry1);

		lx = lx1; ly = ly1;
		rx = rx1; ry = ry1;
	}

	left->x = lx; left->y = ly;
	right->x = rx; right->y = ry;
}

static void nsvg__straightJoin(NSVGrasterizer* r, NSVGpoint* left, NSVGpoint* right, NSVGpoint* p1, float lineWidth)
{
	float w = lineWidth * 0.5f;
	float lx = p1->x - (p1->dmx * w), ly = p1->y - (p1->dmy * w);
	float rx = p1->x + (p1->dmx * w), ry = p1->y + (p1->dmy * w);

	nsvg__addEdge(r, lx, ly, left->x, left->y);
	nsvg__addEdge(r, right->x, right->y, rx, ry);

	left->x = lx; left->y = ly;
	right->x = rx; right->y = ry;
}

static int nsvg__curveDivs(float r, float arc, float tol)
{
	float da = acosf(r / (r + tol)) * 2.0f;
	int divs = (int)ceilf(arc / da);
	if (divs < 2) divs = 2;
	return divs;
}

static void nsvg__flattenShapeStroke(NSVGrasterizer* r, NSVGshape* shape, float scale)
{
	int i, j, closed;
	int s, e;
	NSVGpath* path;
	NSVGpoint* p0, *p1;
	float miterLimit = 4;
	int lineJoin = shape->strokeLineJoin;
	int lineCap = shape->strokeLineCap;
	float lineWidth = shape->strokeWidth * scale;
	int ncap = nsvg__curveDivs(lineWidth*0.5f, NSVG_PI, r->tessTol);	// Calculate divisions per half circle.
	NSVGpoint left = { 0,0,0,0,0,0,0,0 }, right = { 0,0,0,0,0,0,0,0 }, firstLeft = { 0,0,0,0,0,0,0,0 }, firstRight = { 0,0,0,0,0,0,0,0 };

	for (path = shape->paths; path != NULL; path = path->next) {
		r->npoints = 0;
		// Flatten path
		nsvg__addPathPoint(r, path->pts[0] * scale, path->pts[1] * scale, NSVG_PT_CORNER);
		for (i = 0; i < path->npts - 1; i += 3) {
			float* p = &path->pts[i * 2];
			nsvg__flattenCubicBez(r, p[0] * scale, p[1] * scale, p[2] * scale, p[3] * scale, p[4] * scale, p[5] * scale, p[6] * scale, p[7] * scale, 0, NSVG_PT_CORNER);
		}
		if (r->npoints < 2)
			continue;

		closed = path->closed;

		// If the first and last points are the same, remove the last, mark as closed path.
		p0 = &r->points[r->npoints - 1];
		p1 = &r->points[0];
		if (nsvg__ptEquals(p0->x, p0->y, p1->x, p1->y, r->distTol)) {
			r->npoints--;
			p0 = &r->points[r->npoints - 1];
			closed = 1;
		}

		for (i = 0; i < r->npoints; i++) {
			// Calculate segment direction and length
			p0->dx = p1->x - p0->x;
			p0->dy = p1->y - p0->y;
			p0->len = nsvg__normalize(&p0->dx, &p0->dy);
			// Advance
			p0 = p1++;
		}

		// calculate joins
		p0 = &r->points[r->npoints - 1];
		p1 = &r->points[0];
		for (j = 0; j < r->npoints; j++) {
			float dlx0, dly0, dlx1, dly1, dmr2, cross;
			dlx0 = p0->dy;
			dly0 = -p0->dx;
			dlx1 = p1->dy;
			dly1 = -p1->dx;
			// Calculate extrusions
			p1->dmx = (dlx0 + dlx1) * 0.5f;
			p1->dmy = (dly0 + dly1) * 0.5f;
			dmr2 = p1->dmx*p1->dmx + p1->dmy*p1->dmy;
			if (dmr2 > 0.000001f) {
				float s2 = 1.0f / dmr2;
				if (s2 > 600.0f) {
					s2 = 600.0f;
				}
				p1->dmx *= s2;
				p1->dmy *= s2;
			}

			// Clear flags, but keep the corner.
			p1->flags = (p1->flags & NSVG_PT_CORNER) ? NSVG_PT_CORNER : 0;

			// Keep track of left turns.
			cross = p1->dx * p0->dy - p0->dx * p1->dy;
			if (cross > 0.0f)
				p1->flags |= NSVG_PT_LEFT;

			// Check to see if the corner needs to be beveled.
			if (p1->flags & NSVG_PT_CORNER) {
				if ((dmr2 * miterLimit*miterLimit) < 1.0f || lineJoin == NSVG_JOIN_BEVEL || lineJoin == NSVG_JOIN_ROUND) {
					p1->flags |= NSVG_PT_BEVEL;
				}
			}

			p0 = p1++;
		}

		// Build stroke edges
		if (closed) {
			// Looping
			p0 = &r->points[r->npoints - 1];
			p1 = &r->points[0];
			s = 0;
			e = r->npoints;
		}
		else {
			// Add cap
			p0 = &r->points[0];
			p1 = &r->points[1];
			s = 1;
			e = r->npoints - 1;
		}

		if (closed) {
			nsvg__initClosed(&left, &right, p0, p1, lineWidth);
			firstLeft = left;
			firstRight = right;
		}
		else {
			// Add cap
			float dx = p1->x - p0->x;
			float dy = p1->y - p0->y;
			nsvg__normalize(&dx, &dy);
			if (lineCap == NSVG_CAP_BUTT)
				nsvg__buttCap(r, &left, &right, p0, dx, dy, lineWidth, 0);
			else if (lineCap == NSVG_CAP_SQUARE)
				nsvg__squareCap(r, &left, &right, p0, dx, dy, lineWidth, 0);
			else if (lineCap == NSVG_CAP_ROUND)
				nsvg__roundCap(r, &left, &right, p0, dx, dy, lineWidth, ncap, 0);
		}

		for (j = s; j < e; ++j) {
			//			if (p1->flags & NSVG_PT_BEVEL) {
			if (p1->flags & NSVG_PT_CORNER) {
				if (lineJoin == NSVG_JOIN_ROUND)
					nsvg__roundJoin(r, &left, &right, p0, p1, lineWidth, ncap);
				else if (lineJoin == NSVG_JOIN_BEVEL || (p1->flags & NSVG_PT_BEVEL))
					nsvg__bevelJoin(r, &left, &right, p0, p1, lineWidth);
				else
					nsvg__miterJoin(r, &left, &right, p0, p1, lineWidth);
			}
			else {
				nsvg__straightJoin(r, &left, &right, p1, lineWidth);
			}
			p0 = p1++;
		}

		if (closed) {
			// Loop it
			nsvg__addEdge(r, firstLeft.x, firstLeft.y, left.x, left.y);
			nsvg__addEdge(r, right.x, right.y, firstRight.x, firstRight.y);
		}
		else {
			// Add cap
			float dx = p1->x - p0->x;
			float dy = p1->y - p0->y;
			nsvg__normalize(&dx, &dy);
			if (lineCap == NSVG_CAP_BUTT)
				nsvg__buttCap(r, &right, &left, p1, -dx, -dy, lineWidth, 1);
			else if (lineCap == NSVG_CAP_SQUARE)
				nsvg__squareCap(r, &right, &left, p1, -dx, -dy, lineWidth, 1);
			else if (lineCap == NSVG_CAP_ROUND)
				nsvg__roundCap(r, &right, &left, p1, -dx, -dy, lineWidth, ncap, 1);
		}
	}
}

static int nsvg__cmpEdge(const void *p, const void *q)
{
	NSVGedge* a = (NSVGedge*)p;
	NSVGedge* b = (NSVGedge*)q;

	if (a->y0 < b->y0) return -1;
	if (a->y0 > b->y0) return  1;
	return 0;
}


static NSVGactiveEdge* nsvg__addActive(NSVGrasterizer* r, NSVGedge* e, float startPoint)
{
	NSVGactiveEdge* z;

	if (r->freelist != NULL) {
		// Restore from freelist.
		z = r->freelist;
		r->freelist = z->next;
	}
	else {
		// Alloc new edge.
		z = (NSVGactiveEdge*)nsvg__alloc(r, sizeof(NSVGactiveEdge));
		if (z == NULL) return NULL;
	}

	float dxdy = (e->x1 - e->x0) / (e->y1 - e->y0);
	//	STBTT_assert(e->y0 <= start_point);
	// round dx down to avoid going too far
	if (dxdy < 0)
		z->dx = (int)(-floorf(NSVG__FIX * -dxdy));
	else
		z->dx = (int)floorf(NSVG__FIX * dxdy);
	z->x = (int)floorf(NSVG__FIX * (e->x0 + dxdy * (startPoint - e->y0)));
	//	z->x -= off_x * FIX;
	z->ey = e->y1;
	z->next = 0;
	z->dir = e->dir;

	return z;
}

static void nsvg__freeActive(NSVGrasterizer* r, NSVGactiveEdge* z)
{
	z->next = r->freelist;
	r->freelist = z;
}

static void nsvg__fillScanline(unsigned char* scanline, int len, int x0, int x1, int maxWeight, int* xmin, int* xmax)
{
	int i = x0 >> NSVG__FIXSHIFT;
	int j = x1 >> NSVG__FIXSHIFT;
	if (i < *xmin) *xmin = i;
	if (j > *xmax) *xmax = j;
	if (i < len && j >= 0) {
		if (i == j) {
			// x0,x1 are the same pixel, so compute combined coverage
			scanline[i] += (unsigned char)((x1 - x0) * maxWeight >> NSVG__FIXSHIFT);
		}
		else {
			if (i >= 0) // add antialiasing for x0
				scanline[i] += (unsigned char)(((NSVG__FIX - (x0 & NSVG__FIXMASK)) * maxWeight) >> NSVG__FIXSHIFT);
			else
				i = -1; // clip

			if (j < len) // add antialiasing for x1
				scanline[j] += (unsigned char)(((x1 & NSVG__FIXMASK) * maxWeight) >> NSVG__FIXSHIFT);
			else
				j = len; // clip

			for (++i; i < j; ++i) // fill pixels between x0 and x1
				scanline[i] += (unsigned char)maxWeight;
		}
	}
}

// note: this routine clips fills that extend off the edges... ideally this
// wouldn't happen, but it could happen if the truetype glyph bounding boxes
// are wrong, or if the user supplies a too-small bitmap
static void nsvg__fillActiveEdges(unsigned char* scanline, int len, NSVGactiveEdge* e, int maxWeight, int* xmin, int* xmax, char fillRule)
{
	// non-zero winding fill
	int x0 = 0, w = 0;

	if (fillRule == NSVG_FILLRULE_NONZERO) {
		// Non-zero
		while (e != NULL) {
			if (w == 0) {
				// if we're currently at zero, we need to record the edge start point
				x0 = e->x; w += e->dir;
			}
			else {
				int x1 = e->x; w += e->dir;
				// if we went to zero, we need to draw
				if (w == 0)
					nsvg__fillScanline(scanline, len, x0, x1, maxWeight, xmin, xmax);
			}
			e = e->next;
		}
	}
	else if (fillRule == NSVG_FILLRULE_EVENODD) {
		// Even-odd
		while (e != NULL) {
			if (w == 0) {
				// if we're currently at zero, we need to record the edge start point
				x0 = e->x; w = 1;
			}
			else {
				int x1 = e->x; w = 0;
				nsvg__fillScanline(scanline, len, x0, x1, maxWeight, xmin, xmax);
			}
			e = e->next;
		}
	}
}

static float nsvg__clampf(float a, float mn, float mx) { return a < mn ? mn : (a > mx ? mx : a); }

static unsigned int nsvg__RGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	return (r) | (g << 8) | (b << 16) | (a << 24);
}

static unsigned int nsvg__lerpRGBA(unsigned int c0, unsigned int c1, float u)
{
	int iu = (int)(nsvg__clampf(u, 0.0f, 1.0f) * 256.0f);
	int r = (((c0)& 0xff)*(256 - iu) + (((c1)& 0xff)*iu)) >> 8;
	int g = (((c0 >> 8) & 0xff)*(256 - iu) + (((c1 >> 8) & 0xff)*iu)) >> 8;
	int b = (((c0 >> 16) & 0xff)*(256 - iu) + (((c1 >> 16) & 0xff)*iu)) >> 8;
	int a = (((c0 >> 24) & 0xff)*(256 - iu) + (((c1 >> 24) & 0xff)*iu)) >> 8;
	return nsvg__RGBA((unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a);
}

static unsigned int nsvg__applyOpacity(unsigned int c, float u)
{
	int iu = (int)(nsvg__clampf(u, 0.0f, 1.0f) * 256.0f);
	int r = (c)& 0xff;
	int g = (c >> 8) & 0xff;
	int b = (c >> 16) & 0xff;
	int a = (((c >> 24) & 0xff)*iu) >> 8;
	return nsvg__RGBA((unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a);
}

static int nsvg__div255(int x)
{
	return ((x + 1) * 257) >> 16;
}

static void nsvg__scanlineSolid(unsigned char* dst, int count, unsigned char* cover, int x, int y,
	float tx, float ty, float scale, NSVGcachedPaint* cache)
{

	if (cache->type == NSVG_PAINT_COLOR) {
		int i, cr, cg, cb, ca;
		cr = cache->colors[0] & 0xff;
		cg = (cache->colors[0] >> 8) & 0xff;
		cb = (cache->colors[0] >> 16) & 0xff;
		ca = (cache->colors[0] >> 24) & 0xff;

		for (i = 0; i < count; i++) {
			int r, g, b;
			int a = nsvg__div255((int)cover[0] * ca);
			int ia = 255 - a;
			// Premultiply
			r = nsvg__div255(cr * a);
			g = nsvg__div255(cg * a);
			b = nsvg__div255(cb * a);

			// Blend over
			r += nsvg__div255(ia * (int)dst[0]);
			g += nsvg__div255(ia * (int)dst[1]);
			b += nsvg__div255(ia * (int)dst[2]);
			a += nsvg__div255(ia * (int)dst[3]);

			dst[0] = (unsigned char)r;
			dst[1] = (unsigned char)g;
			dst[2] = (unsigned char)b;
			dst[3] = (unsigned char)a;

			cover++;
			dst += 4;
		}
	}
	else if (cache->type == NSVG_PAINT_LINEAR_GRADIENT) {
		// TODO: spread modes.
		// TODO: plenty of opportunities to optimize.
		float fx, fy, dx, gy;
		float* t = cache->xform;
		int i, cr, cg, cb, ca;
		unsigned int c;

		fx = (x - tx) / scale;
		fy = (y - ty) / scale;
		dx = 1.0f / scale;

		for (i = 0; i < count; i++) {
			int r, g, b, a, ia;
			gy = fx*t[1] + fy*t[3] + t[5];
			c = cache->colors[(int)nsvg__clampf(gy*255.0f, 0, 255.0f)];
			cr = (c)& 0xff;
			cg = (c >> 8) & 0xff;
			cb = (c >> 16) & 0xff;
			ca = (c >> 24) & 0xff;

			a = nsvg__div255((int)cover[0] * ca);
			ia = 255 - a;

			// Premultiply
			r = nsvg__div255(cr * a);
			g = nsvg__div255(cg * a);
			b = nsvg__div255(cb * a);

			// Blend over
			r += nsvg__div255(ia * (int)dst[0]);
			g += nsvg__div255(ia * (int)dst[1]);
			b += nsvg__div255(ia * (int)dst[2]);
			a += nsvg__div255(ia * (int)dst[3]);

			dst[0] = (unsigned char)r;
			dst[1] = (unsigned char)g;
			dst[2] = (unsigned char)b;
			dst[3] = (unsigned char)a;

			cover++;
			dst += 4;
			fx += dx;
		}
	}
	else if (cache->type == NSVG_PAINT_RADIAL_GRADIENT) {
		// TODO: spread modes.
		// TODO: plenty of opportunities to optimize.
		// TODO: focus (fx,fy)
		float fx, fy, dx, gd;
		float* t = cache->xform;
		int i, cr, cg, cb, ca;
		unsigned int c;

		fx = (x - tx) / scale;
		fy = (y - ty) / scale;
		dx = 1.0f / scale;

		for (i = 0; i < count; i++) {
			int r, g, b, a, ia;
			float sx, sy, ex, ey;
			float ds, de;
			sx = fx - t[0];
			sy = fy - t[1];
			ex = fx - t[2];
			ey = fx - t[3];
			ds = (float) sqrt(sx*sx + sy*sy) - t[4];
			de = (float) (t[5] - sqrt(ex*ex + ey*ey));
			gd  = ds < 0 ? 0 : de < 0 ? 1 : ds / (ds + de);
			c = cache->colors[(int)nsvg__clampf(gd*255.0f, 0, 255.0f)];
			cr = (c)& 0xff;
			cg = (c >> 8) & 0xff;
			cb = (c >> 16) & 0xff;
			ca = (c >> 24) & 0xff;

			a = nsvg__div255((int)cover[0] * ca);
			ia = 255 - a;

			// Premultiply
			r = nsvg__div255(cr * a);
			g = nsvg__div255(cg * a);
			b = nsvg__div255(cb * a);

			// Blend over
			r += nsvg__div255(ia * (int)dst[0]);
			g += nsvg__div255(ia * (int)dst[1]);
			b += nsvg__div255(ia * (int)dst[2]);
			a += nsvg__div255(ia * (int)dst[3]);

			dst[0] = (unsigned char)r;
			dst[1] = (unsigned char)g;
			dst[2] = (unsigned char)b;
			dst[3] = (unsigned char)a;

			cover++;
			dst += 4;
			fx += dx;
		}
	}
	else if (cache->type == NSVG_PAINT_PATTERN) {
		int u, v;
			int i;// cr, cg, cb, ca;
			v = y % cache->theight;
			u = x % cache->twidth;
			for (i = 0; i < count; i++) {
				int cr = cache->bitmap[((v*cache->twidth) + u) * 4];
				int cg = cache->bitmap[((v*cache->twidth) + u) * 4 + 1];
				int cb = cache->bitmap[((v*cache->twidth) + u) * 4 + 2];
				int ca = cache->bitmap[((v*cache->twidth) + u) * 4 + 3];
				int r, g, b;
				int a = nsvg__div255((int)cover[0] * ca);
				int ia = 255 - a;
				// Premultiply
				r = nsvg__div255(cr * a);
				g = nsvg__div255(cg * a);
				b = nsvg__div255(cb * a);

				// Blend over
				r += nsvg__div255(ia * (int)dst[0]);
				g += nsvg__div255(ia * (int)dst[1]);
				b += nsvg__div255(ia * (int)dst[2]);
				a += nsvg__div255(ia * (int)dst[3]);

				dst[0] = (unsigned char)r;
				dst[1] = (unsigned char)g;
				dst[2] = (unsigned char)b;
				dst[3] = (unsigned char)a;

				cover++;
				dst += 4;
				u++;
				if (u == cache->twidth)
					u = 0;
			}
	}
}

static void nsvg__rasterizeSortedEdges(NSVGrasterizer *r, float tx, float ty, float scale, NSVGcachedPaint* cache, char fillRule)
{
	NSVGactiveEdge *active = NULL;
	int x, y, s;
	int e = 0;
	int maxWeight = (255 / NSVG__SUBSAMPLES);  // weight per vertical scanline
	int xmin, xmax;

	for (y = 0; y < r->height; y++) {
		memset(r->scanline, 0, r->width);
		xmin = r->width;
		xmax = 0;
		for (s = 0; s < NSVG__SUBSAMPLES; ++s) {
			// find center of pixel for this scanline
			float scany = y*NSVG__SUBSAMPLES + s + 0.5f;
			NSVGactiveEdge **step = &active;

			// update all active edges;
			// remove all active edges that terminate before the center of this scanline
			while (*step) {
				NSVGactiveEdge *z = *step;
				if (z->ey <= scany) {
					*step = z->next; // delete from list
									 //					NSVG__assert(z->valid);
					nsvg__freeActive(r, z);
				}
				else {
					z->x += z->dx; // advance to position for current scanline
					step = &((*step)->next); // advance through list
				}
			}

			// resort the list if needed
			for (;;) {
				int changed = 0;
				step = &active;
				while (*step && (*step)->next) {
					if ((*step)->x > (*step)->next->x) {
						NSVGactiveEdge* t = *step;
						NSVGactiveEdge* q = t->next;
						t->next = q->next;
						q->next = t;
						*step = q;
						changed = 1;
					}
					step = &(*step)->next;
				}
				if (!changed) break;
			}

			// insert all edges that start before the center of this scanline -- omit ones that also end on this scanline
			while (e < r->nedges && r->edges[e].y0 <= scany) {
				if (r->edges[e].y1 > scany) {
					NSVGactiveEdge* z = nsvg__addActive(r, &r->edges[e], scany);
					if (z == NULL) break;
					// find insertion point
					if (active == NULL) {
						active = z;
					}
					else if (z->x < active->x) {
						// insert at front
						z->next = active;
						active = z;
					}
					else {
						// find thing to insert AFTER
						NSVGactiveEdge* p = active;
						while (p->next && p->next->x < z->x)
							p = p->next;
						// at this point, p->next->x is NOT < z->x
						z->next = p->next;
						p->next = z;
					}
				}
				e++;
			}

			// now process all active edges in non-zero fashion
			if (active != NULL)
				nsvg__fillActiveEdges(r->scanline, r->width, active, maxWeight, &xmin, &xmax, fillRule);
		}
		// Blit
		if (xmin < 0) xmin = 0;
		if (xmax > r->width - 1) xmax = r->width - 1;
		if (r->clipmask)
		{
			for (x = xmin; x <= xmax; x++)
			{
				r->scanline[x] = nsvg__div255(r->scanline[x] * r->clipmask[y * r->stride / 4 + x]);
			}
		}
		if (xmin <= xmax) {
			nsvg__scanlineSolid(&r->bitmap[y * r->stride] + xmin * 4, xmax - xmin + 1, &r->scanline[xmin], xmin, y, tx, ty, scale, cache);
		}
	}

}

static void nsvg__unpremultiplyAlpha(unsigned char* image, int w, int h, int stride)
{
	int x, y;

	// Unpremultiply
	for (y = 0; y < h; y++) {
		unsigned char *row = &image[y*stride];
		for (x = 0; x < w; x++) {
			int r = row[0], g = row[1], b = row[2], a = row[3];
			if (a != 0) {
				row[0] = (unsigned char)(r * 255 / a);
				row[1] = (unsigned char)(g * 255 / a);
				row[2] = (unsigned char)(b * 255 / a);
			}
			row += 4;
		}
	}

	// Defringe
	for (y = 0; y < h; y++) {
		unsigned char *row = &image[y*stride];
		for (x = 0; x < w; x++) {
			int r = 0, g = 0, b = 0, a = row[3], n = 0;
			if (a == 0) {
				if (x - 1 > 0 && row[-1] != 0) {
					r += row[-4];
					g += row[-3];
					b += row[-2];
					n++;
				}
				if (x + 1 < w && row[7] != 0) {
					r += row[4];
					g += row[5];
					b += row[6];
					n++;
				}
				if (y - 1 > 0 && row[-stride + 3] != 0) {
					r += row[-stride];
					g += row[-stride + 1];
					b += row[-stride + 2];
					n++;
				}
				if (y + 1 < h && row[stride + 3] != 0) {
					r += row[stride];
					g += row[stride + 1];
					b += row[stride + 2];
					n++;
				}
				if (n > 0) {
					row[0] = (unsigned char)(r / n);
					row[1] = (unsigned char)(g / n);
					row[2] = (unsigned char)(b / n);
				}
			}
			row += 4;
		}
	}
}


static void nsvg__initPaint(NSVGcachedPaint* cache, NSVGpaint* paint, float opacity)
{
	int i, j;
	NSVGgradient* grad;

	cache->type = paint->type;

	if (paint->type == NSVG_PAINT_COLOR || paint->type == NSVG_PAINT_PATTERN) {
		cache->colors[0] = nsvg__applyOpacity(paint->color, opacity);
		return;
	}

	grad = paint->gradient;

	cache->spread = grad->spread;
	memcpy(cache->xform, grad->xform, sizeof(float) * 6);

	if (grad->nstops == 0) {
		for (i = 0; i < 256; i++)
			cache->colors[i] = 0;
	} if (grad->nstops == 1) {
		for (i = 0; i < 256; i++)
			cache->colors[i] = nsvg__applyOpacity(grad->stops[i].color, opacity);
	}
	else {
		unsigned int ca, cb = 0;
		float ua, ub, du, u;
		int ia, ib, count;

		ca = nsvg__applyOpacity(grad->stops[0].color, opacity);
		ua = nsvg__clampf(grad->stops[0].offset, 0, 1);
		ub = nsvg__clampf(grad->stops[grad->nstops - 1].offset, ua, 1);
		ia = (int)(ua * 255.0f);
		ib = (int)(ub * 255.0f);
		for (i = 0; i < ia; i++) {
			cache->colors[i] = ca;
		}

		for (i = 0; i < grad->nstops - 1; i++) {
			ca = nsvg__applyOpacity(grad->stops[i].color, opacity);
			cb = nsvg__applyOpacity(grad->stops[i + 1].color, opacity);
			ua = nsvg__clampf(grad->stops[i].offset, 0, 1);
			ub = nsvg__clampf(grad->stops[i + 1].offset, 0, 1);
			ia = (int)(ua * 255.0f);
			ib = (int)(ub * 255.0f);
			count = ib - ia;
			if (count <= 0) continue;
			u = 0;
			du = 1.0f / (float)count;
			for (j = 0; j < count; j++) {
				cache->colors[ia + j] = nsvg__lerpRGBA(ca, cb, u);
				u += du;
			}
		}

		for (i = ib; i < 256; i++)
			cache->colors[i] = cb;
	}

}

/*
static void dumpEdges(NSVGrasterizer* r, const char* name)
{
float xmin = 0, xmax = 0, ymin = 0, ymax = 0;
NSVGedge *e = NULL;
int i;
if (r->nedges == 0) return;
FILE* fp = fopen(name, "w");
if (fp == NULL) return;

xmin = xmax = r->edges[0].x0;
ymin = ymax = r->edges[0].y0;
for (i = 0; i < r->nedges; i++) {
e = &r->edges[i];
xmin = nsvg__minf(xmin, e->x0);
xmin = nsvg__minf(xmin, e->x1);
xmax = nsvg__maxf(xmax, e->x0);
xmax = nsvg__maxf(xmax, e->x1);
ymin = nsvg__minf(ymin, e->y0);
ymin = nsvg__minf(ymin, e->y1);
ymax = nsvg__maxf(ymax, e->y0);
ymax = nsvg__maxf(ymax, e->y1);
}

fprintf(fp, "<svg viewBox=\"%f %f %f %f\" xmlns=\"http://www.w3.org/2000/svg\">", xmin, ymin, (xmax - xmin), (ymax - ymin));

for (i = 0; i < r->nedges; i++) {
e = &r->edges[i];
fprintf(fp ,"<line x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\" style=\"stroke:#000;\" />", e->x0,e->y0, e->x1,e->y1);
}

for (i = 0; i < r->npoints; i++) {
if (i+1 < r->npoints)
fprintf(fp ,"<line x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\" style=\"stroke:#f00;\" />", r->points[i].x, r->points[i].y, r->points[i+1].x, r->points[i+1].y);
fprintf(fp ,"<circle cx=\"%f\" cy=\"%f\" r=\"1\" style=\"fill:%s;\" />", r->points[i].x, r->points[i].y, r->points[i].flags == 0 ? "#f00" : "#0f0");
}

fprintf(fp, "</svg>");
fclose(fp);
}
*/

void nsvgRasterize(NSVGrasterizer* r,
	NSVGimage* image, float tx, float ty, float scale,
	unsigned char* dst, int w, int h, int stride)
{
	NSVGshape *shape = NULL;
	NSVGedge *e = NULL;
	NSVGcachedPaint cache;
	int i;

	r->bitmap = dst;
	r->width = w;
	r->height = h;
	r->stride = stride;

	if (w > r->cscanline) {
		r->cscanline = w;
		r->scanline = (unsigned char*)realloc(r->scanline, w);
		if (r->scanline == NULL) return;
	}

	for (i = 0; i < h; i++)
		memset(&dst[i*stride], 0, w * 4);

	for (shape = image->shapes; shape != NULL; shape = shape->next) {
		if (shape->fill.type != NSVG_PAINT_NONE) {
			nsvg__resetPool(r);
			r->freelist = NULL;
			r->nedges = 0;

			nsvg__flattenShape(r, shape, scale);

			// Scale and translate edges
			for (i = 0; i < r->nedges; i++) {
				e = &r->edges[i];
				e->x0 = tx + e->x0;
				e->y0 = (ty + e->y0) * NSVG__SUBSAMPLES;
				e->x1 = tx + e->x1;
				e->y1 = (ty + e->y1) * NSVG__SUBSAMPLES;
			}

			// Rasterize edges
			qsort(r->edges, r->nedges, sizeof(NSVGedge), nsvg__cmpEdge);

			// now, traverse the scanlines and find the intersections on each scanline, use non-zero rule
			nsvg__initPaint(&cache, &shape->fill, shape->opacity);

			nsvg__rasterizeSortedEdges(r, tx, ty, scale, &cache, shape->fillRule);
		}
		if (shape->stroke.type != NSVG_PAINT_NONE && (shape->strokeWidth * scale) > 0.01f) {
			nsvg__resetPool(r);
			r->freelist = NULL;
			r->nedges = 0;

			nsvg__flattenShapeStroke(r, shape, scale);

			//			dumpEdges(r, "edge.svg");

			// Scale and translate edges
			for (i = 0; i < r->nedges; i++) {
				e = &r->edges[i];
				e->x0 = tx + e->x0;
				e->y0 = (ty + e->y0) * NSVG__SUBSAMPLES;
				e->x1 = tx + e->x1;
				e->y1 = (ty + e->y1) * NSVG__SUBSAMPLES;
			}

			// Rasterize edges
			qsort(r->edges, r->nedges, sizeof(NSVGedge), nsvg__cmpEdge);

			// now, traverse the scanlines and find the intersections on each scanline, use non-zero rule
			nsvg__initPaint(&cache, &shape->stroke, shape->opacity);

			nsvg__rasterizeSortedEdges(r, tx, ty, scale, &cache, NSVG_FILLRULE_NONZERO);
		}
	}

	nsvg__unpremultiplyAlpha(dst, w, h, stride);

	r->bitmap = NULL;
	r->width = 0;
	r->height = 0;
	r->stride = 0;
}

//#include "BBX_Color.h"
typedef unsigned long BBX_RGBA;
#define bbx_rgba(r,g,b,a) ((BBX_RGBA)( (r << 24) | (g << 16) | (b << 8) | (a) ))
static void premultiply_alpha(unsigned char *rgba, int width, int height, int stride)
{
	int x, y;
	unsigned char *row;
	unsigned char r, g, b, a;
	for (y = 0; y < height; y++)
	{
		row = rgba + (y * stride);
		for (x = 0; x < width; x++)
		{
			r = row[0]; g = row[1]; b = row[2]; a = row[3];
			row[0] = nsvg__div255(r * a);
			row[1] = nsvg__div255(g * a);
			row[2] = nsvg__div255(b * a);
			row += 4;
		}
	}
}


static BBX_RGBA sampleTexture(unsigned char *rgba, int width, int height, float x, float y, int mode)
{
	float a, b;
	float red, green, blue, alpha;
	int index0, index1, index2, index3;

	a = x - (int) x;
	b = y - (int) y;

	index0 = (int)y * width + (int)x;
	index1 = index0 + 1;
	index2 = index0 + width;
	index3 = index0 + width + 1;

	red = rgba[index0 * 4] * (1.0f - a)*(1.0f - b);
	green = rgba[index0 * 4 + 1] * (1.0f - a)*(1.0f - b);
	blue = rgba[index0 * 4 + 2] * (1.0f - a)*(1.0f - b);
	alpha = rgba[index0 * 4 + 3] * (1.0f - a)*(1.0f - b);
	red += rgba[index1 * 4] * (a)*(1.0f - b);
	green += rgba[index1 * 4 + 1] * (a)*(1.0f - b);
	blue += rgba[index1 * 4 + 2] * (a)*(1.0f - b);
	alpha += rgba[index1 * 4 + 3] * (a)*(1.0f - b);
	red += rgba[index2 * 4] * (1.0f - a)*(b);
	green += rgba[index2 * 4 + 1] * (1.0f - a)*(b);
	blue += rgba[index2 * 4 + 2] * (1.0f - a)*(b);
	alpha += rgba[index2 * 4 + 3] * (1.0f - a)*(b);
	red += rgba[index3 * 4] * (a)*(b);
	green += rgba[index3 * 4 + 1] * (a)*(b);
	blue += rgba[index3 * 4 + 2] * (a)*(b);
	alpha += rgba[index3 * 4 + 3] * (a)*(b);

	return bbx_rgba( (int)red, (int)green, (int)blue, (int)alpha);
}

typedef struct bbx_gradient
{
	int type;
	double x1;
	double y1;
	double x2;
	double y2;
	double r1;
	double r2;
	NSVGgradientStop *stops;
	int nstops;
} BBX_GRADIENT;

NSVGgradient *nsvggradient(BBX_GRADIENT *bbxg, const float* bounds, char* paintType)
{
	NSVGgradient *grad;
	int nstops = bbxg->nstops;
	double dx, dy;
	float len;

	grad = (NSVGgradient*)malloc(sizeof(NSVGgradient) + sizeof(NSVGgradientStop)*(nstops - 1));
    if (grad == NULL) return NULL;

    if (bbxg->type == NSVG_PAINT_LINEAR_GRADIENT) 
    {
		float t[6];
		dx = bbxg->x2 - bbxg->x1;
		dy = bbxg->y2 - bbxg->y1;
		len = (float) sqrt(dx*dx + dy*dy);
		nsvg__xformIdentity(grad->xform);
		nsvg__xformSetTranslation(t, (float) -bbxg->x1, (float) -bbxg->y1);
		nsvg__xformMultiply(grad->xform, t);
		nsvg__xformSetScale(t, 1 / len, 1 / len);
		nsvg__xformMultiply(grad->xform, t);
		nsvg__xformSetRotation(t, (float) atan2(dy, dx));
		nsvg__xformMultiply(grad->xform, t);

    }
else {
	// Calculate transform aligned to the circle
	 grad->xform[0] = (float) bbxg->x1; grad->xform[1] = (float) bbxg->y1;
	 grad->xform[2] = (float) bbxg->x2; grad->xform[3] = (float) bbxg->y2;
	 grad->xform[4] = (float) bbxg->r1; grad->xform[5] = (float) bbxg->r2;
	
	//grad->xform[0] = bbxg->r; grad->xform[1] = 0;
	//grad->xform[2] = 0; grad->xform[3] = bbxg->r;
	//grad->xform[4] = bbxg->cx; grad->xform[5] = bbxg->cy;
	//grad->fx = bbxg->fx / bbxg->r;
	//grad->fy = bbxg->fy / bbxg->r;
}

if (0 /*data->units == NSVG_OBJECT_SPACE*/) {
	float boundingBox[6];
	dx = bounds[2] - bounds[0];
	dy = bounds[3] - bounds[1];
	boundingBox[0] = (float) dx; boundingBox[1] = 0.0f;
	boundingBox[2] = 0.0f; boundingBox[3] = (float) dy;
	boundingBox[4] = bounds[0]; boundingBox[5] = bounds[1];

	nsvg__xformMultiply(grad->xform, boundingBox);
	//nsvg__xformMultiply(grad->xform, data->xform);
	//nsvg__xformMultiply(grad->xform, attr->xform);
}
//nsvg__xformMultiply(grad->xform, data->xform);
 //  nsvg__xformMultiply(grad->xform, attr->xform);

//grad->spread = data->spread;
memcpy(grad->stops, bbxg->stops, nstops*sizeof(NSVGgradientStop));
grad->nstops = nstops;

*paintType = bbxg->type;

return grad;
}

BBX_GRADIENT *bbx_createlineargradient(double x1, double y1, double x2, double y2)
{
	BBX_GRADIENT *answer;

	answer = malloc(sizeof(BBX_GRADIENT));
	if (!answer)
		goto error;
	answer->type = NSVG_PAINT_LINEAR_GRADIENT;
	answer->x1 = x1;
	answer->y1 = y1;
	answer->x2 = x2;
	answer->y2 = y2;
	answer->stops = 0;
	answer->nstops = 0;

	return answer;
error:
	return 0;
}
BBX_GRADIENT *bbx_createradialgradient(double x1, double y1, double r1, double x2, double y2, double r2)
{
	BBX_GRADIENT *answer;

	answer = malloc(sizeof(BBX_GRADIENT));
	if (!answer)
		goto error;
	answer->type = NSVG_PAINT_RADIAL_GRADIENT;
	answer->x1 = x1;
	answer->y1 = y1;
	answer->x2 = x2;
	answer->y2 = y2;
	answer->r1 = r1;
	answer->r2 = r2;
	answer->stops = 0;
	answer->nstops = 0;

	return answer;
error:
	return 0;
}

BBX_GRADIENT *bbx_gradient_clone(BBX_GRADIENT *grad)
{
	BBX_GRADIENT *answer;

	if (!grad)
		return 0;

	answer = malloc(sizeof(BBX_GRADIENT));
	if (!answer)
		goto error;
	answer->type = grad->type;
	answer->x1 = grad->x1;
	answer->y1 = grad->y1;
	answer->x2 = grad->x2;
	answer->y2 = grad->y2;
	answer->r1 = grad->r1;
	answer->r2 = grad->r2;
	answer->stops = malloc(grad->nstops * sizeof(NSVGgradientStop));
	if (!answer->stops)
		goto error;
	memcpy(answer->stops, grad->stops, grad->nstops * sizeof(NSVGgradientStop));
	answer->nstops = grad->nstops;

	return answer;
error:
	if (answer)
	{
		free(answer->stops);
		free(answer);
	}
	return 0;
}

void bbx_gradient_kill(BBX_GRADIENT *g)
{
	if (g)
	{
		free(g->stops);
		free(g);
	}
}

int bbx_gradient_addcolorstop(BBX_GRADIENT *g, double offset, BBX_RGBA col)
{
	int i;
	NSVGgradientStop *temp;
	temp = realloc(g->stops, (g->nstops + 1) * sizeof(NSVGgradientStop));
	if (!temp)
		goto error;
	g->stops = temp;
	for (i = 0; i < g->nstops; i++)
		if (g->stops[i].offset > offset)
			break;
	memmove(&g->stops[i + 1], &g->stops[i], (g->nstops - i) * sizeof(NSVGgradientStop));
	g->stops[i].offset = (float) offset;
	g->stops[i].color = NSVG_RGB((col >> 24) & 0xFF, (col >> 16) & 0xFF, (col >> 8) & 0xFF) | 0xFF000000;
	g->nstops++;
	return 0;
error:
	return - 1;
}

/**
repeat	Default.The pattern repeats both horizontally and vertically
repeat - x	The pattern repeats only horizontally
repeat - y	The pattern repeats only vertically	
no-repeat
*/

typedef struct bbx_pattern
{
	unsigned char *rgba;
	int width;
	int height;
	int type; // type not implemeted yet
} BBX_PATTERN;

BBX_PATTERN *bbx_createpattern(unsigned char *rgba, int width, int height, int filltype)
{
	BBX_PATTERN *pat;

	pat = malloc(sizeof(BBX_PATTERN));
	if (!pat)
		goto error;

	pat->width = width;
	pat->height = height;
	pat->rgba = rgba;
	return pat;

error:
	return 0;

}

BBX_PATTERN *bbx_pattern_clone(BBX_PATTERN *pat)
{
	BBX_PATTERN *answer;

	if (!pat)
		return 0;

	answer = malloc(sizeof(BBX_PATTERN));
	if (!answer)
		goto error;

	answer->width = pat->width;
	answer->height = pat->height;
	answer->rgba = pat->rgba;
	return answer;

error:
	return 0;
}

void bbx_pattern_kill(BBX_PATTERN *p)
{
	if (p)
	{
		free(p);
	}
}

typedef struct bbx_gc
{
	unsigned char *rgba;
	int width;
	int height;
	NSVGrasterizer *rast;
	float mtx[6];
	NSVGattrib attr;
	BBX_GRADIENT *fillgrad;
	BBX_PATTERN *fillpat;
	BBX_GRADIENT *strokegrad;
	BBX_PATTERN *strokepat;
	int npts;
	float *pts;
	int cpts;
	NSVGpath* plist;
	struct bbx_gc *prevstate;
} BBX_GC;

static void bbx_gc_endsubpath(BBX_GC *gc, int closed);
void bbx_gc_lineto(BBX_GC *gc, double x, double y);

BBX_GC *bbx_graphicscontext(unsigned char *rgba, int width, int height)
{
	BBX_GC *gc;

	gc = malloc(sizeof(BBX_GC));
	if (!gc)
		goto error_exit;
	gc->width = width;
	gc->height = height; 
	gc->rgba = rgba;

	gc->rast = nsvgCreateRasterizer();
	gc->rast->bitmap = rgba;
	gc->rast->clipmask = 0;
	gc->rast->width = width;
	gc->rast->height = height;
	gc->rast->stride = width * 4;

    gc->rast->cscanline = width;
	gc->rast->scanline = (unsigned char*)malloc(gc->rast->stride + 8);
	if (gc->rast->scanline == NULL) return 0;


	//for (i = 0; i < height; i++)
		//memset(&rgba[i*gc->rast->stride], 0, width * 4);
	premultiply_alpha(rgba, width, height, gc->rast->stride);

	nsvg__xformIdentity(gc->attr.xform);
	gc->attr.fillColor = NSVG_RGB(255, 0, 0);
	gc->attr.strokeColor = NSVG_RGB(0, 0, 0);
	gc->attr.opacity = 1;
	gc->attr.fillOpacity = 1;
	gc->attr.strokeOpacity = 1;
	gc->attr.stopOpacity = 1;
	gc->attr.strokeWidth = 1;
	gc->attr.strokeLineJoin = NSVG_JOIN_MITER;
	gc->attr.strokeLineCap = NSVG_CAP_BUTT;
	gc->attr.fillRule = NSVG_FILLRULE_NONZERO;
	gc->attr.hasFill = 1;
	gc->attr.hasStroke = 0;
	gc->attr.visible = 1;

	gc->fillpat = 0;
	gc->fillgrad = 0;
	gc->strokepat = 0;
	gc->strokegrad = 0;

	gc->npts = 0;
	gc->pts = malloc(128 * sizeof(float));
	gc->cpts = 64;


	gc->plist = NULL;
	gc->prevstate = NULL;

	nsvg__xformIdentity(gc->mtx);

   	return gc;
error_exit:
	return 0;
}

static NSVGpath *nsvg__clonePaths(NSVGpath *path)
{
	NSVGpath *answer = 0;
	NSVGpath *prev = 0;
	NSVGpath *newpath = 0;

	while (path)
	{
		newpath = malloc(sizeof(NSVGpath));
		if (!newpath)
			goto error;
		newpath->pts = malloc(path->npts * sizeof(float) * 2);
		if (!newpath->pts)
			goto error;

		memcpy(newpath->pts, path->pts, path->npts * 2 * sizeof(float));
		newpath->npts = path->npts;
		newpath->closed = path->closed;
		memcpy(&newpath->bounds, &path->bounds, sizeof(float) * 4);
		newpath->next = 0;

		if (prev)
			prev->next = newpath;
		else
			answer = newpath;
		prev = newpath;
		path = path->next;
	}
	return answer;
error:
	if (newpath)
	{
		free(newpath->pts);
		free(newpath);
	}
	nsvg__deletePaths(answer);
	return 0;
}

static void deleteStates(BBX_GC *state)
{
	if (state)
	{
		bbx_gradient_kill(state->fillgrad);
		bbx_pattern_kill(state->fillpat);
		bbx_gradient_kill(state->strokegrad);
		bbx_pattern_kill(state->strokepat);
		free(state->pts);
		nsvg__deletePaths(state->plist);
		if (state->prevstate)
			deleteStates(state->prevstate);
		free(state);
	}
}
static BBX_GC *clonestate(BBX_GC *gc)
{
	BBX_GC *newgc;

	newgc = malloc(sizeof(BBX_GC));
	if (!newgc)
		goto error_exit;
	memset(newgc, 0, sizeof(BBX_GC));
	newgc->rast = gc->rast;

	newgc->rgba = gc->rgba;
	newgc->width = gc->width;
	newgc->height = gc->height;

	memcpy(newgc->mtx, gc->mtx, 6 * sizeof(float));
	memcpy(&newgc->attr, &gc->attr, sizeof(NSVGattrib));
	newgc->fillgrad = bbx_gradient_clone(gc->fillgrad);
	newgc->fillpat = bbx_pattern_clone(gc->fillpat);
	newgc->fillgrad = bbx_gradient_clone(gc->strokegrad);
	newgc->fillpat = bbx_pattern_clone(gc->strokepat);
	if (gc->fillgrad && !newgc->fillgrad)
		goto error_exit;
	if (gc->fillpat && !newgc->fillpat)
		goto error_exit;
	if (gc->strokegrad && !newgc->strokegrad)
		goto error_exit;
	if (gc->strokepat && !newgc->strokepat)
		goto error_exit;

	newgc->pts = malloc(gc->cpts *2 * sizeof(float));
	if (!newgc->pts)
		goto error_exit;
	memcpy(newgc->pts, gc->pts, gc->npts * 2 * sizeof(float));
	newgc->cpts = gc->cpts;
	newgc->npts = gc->npts;

	newgc->plist = nsvg__clonePaths(gc->plist);
	if (gc->plist && !newgc->plist)
		goto error_exit;
	newgc->prevstate = gc->prevstate;

	return newgc;

error_exit:
	deleteStates(newgc);
	return 0;
}



static int setState(BBX_GC *gc, BBX_GC *state)
{
	float *temp;

	memcpy(&gc->attr, &state->attr, sizeof(NSVGattrib));
	bbx_gradient_kill(gc->fillgrad);
	gc->fillgrad = bbx_gradient_clone(state->fillgrad);
	bbx_pattern_kill(gc->fillpat);
	gc->fillpat = bbx_pattern_clone(state->fillpat);
	bbx_gradient_kill(gc->strokegrad);
	gc->strokegrad = bbx_gradient_clone(state->strokegrad);
	bbx_pattern_kill(gc->strokepat);
	gc->strokepat = bbx_pattern_clone(state->strokepat);

	if (gc->cpts < state->npts)
	{
		temp = realloc(gc->pts, state->cpts * 2 * sizeof(float));
		if (!gc->pts)
			goto error_exit;
		gc->pts = temp;
	}
	memcpy(gc->pts, state->pts, state->npts * 2 * sizeof(float));
	gc->cpts = state->cpts;
	gc->npts = state->npts;

	nsvg__deletePaths(gc->plist);
	gc->plist = nsvg__clonePaths(gc->plist);

	memcpy(gc->mtx, state->mtx, 6 * sizeof(float));
	return 0;
error_exit:
	return -1;

}

void bbx_graphicscontext_kill(BBX_GC *gc)
{
	nsvg__unpremultiplyAlpha(gc->rgba, gc->width, gc->height, gc->rast->stride);
	bbx_gradient_kill(gc->fillgrad);
	bbx_pattern_kill(gc->fillpat);
	bbx_gradient_kill(gc->strokegrad);
	bbx_pattern_kill(gc->strokepat);
	free(gc->pts);
	nsvg__deletePaths(gc->plist);
	deleteStates(gc->prevstate);
	free(gc);
}


void bbx_gc_setstrokecolor(BBX_GC *gc, BBX_RGBA col)
{
	if (col == 0)
		gc->attr.hasStroke = 0;
	else
	{
		gc->attr.strokeColor = NSVG_RGB((col >> 24) & 0xFF, (col >> 16) & 0xFF, (col >> 8) & 0xFF);
		gc->attr.strokeOpacity = (col & 0xFF) / 255.0f;
		gc->attr.hasStroke = 1;
	}
}

void bbx_gc_setstrokewidth(BBX_GC *gc, double width)
{
	gc->attr.strokeWidth = (float) width;
}


void bbx_gc_setfillcolor(BBX_GC *gc, BBX_RGBA col)
{
	if (col == 0)
		gc->attr.hasFill = 0;
	else
	{
		gc->attr.fillColor = NSVG_RGB((col >> 24) & 0xFF, (col >> 16) & 0xFF, (col >> 8) & 0xFF);
		gc->attr.fillOpacity = (col & 0xFF) / 255.0f;
		gc->attr.hasFill = 1;
	}

}

void bbx_gc_setfillgradient(BBX_GC *gc, BBX_GRADIENT *g)
{
	gc->attr.hasFill = 2;
	bbx_gradient_kill(gc->fillgrad);
	gc->fillgrad = bbx_gradient_clone(g);
}

void bbx_gc_setfillpattern(BBX_GC *gc, BBX_PATTERN *p)
{
	gc->attr.hasFill = 3;
	bbx_pattern_kill(gc->fillpat);
	gc->fillpat = bbx_pattern_clone(p);
}

void bbx_gc_setstrokegradient(BBX_GC *gc, BBX_GRADIENT *g)
{
	gc->attr.hasStroke = 2;
	bbx_gradient_kill(gc->strokegrad);
	gc->strokegrad = bbx_gradient_clone(g);
}

void bbx_gc_setstrokepattern(BBX_GC *gc, BBX_PATTERN *p)
{
	gc->attr.hasStroke = 3;
	bbx_pattern_kill(gc->strokepat);
	gc->strokepat = bbx_pattern_clone(p);
}

void bbx_gc_setlinejoin(BBX_GC *gc, const char *jointype)
{
	if (!strcmp(jointype, "miter"))
		gc->attr.strokeLineJoin = NSVG_JOIN_MITER;
	else if(!strcmp(jointype, "round"))
		gc->attr.strokeLineJoin = NSVG_JOIN_ROUND;
	else if (!strcmp(jointype, "bevel"))
		gc->attr.strokeLineJoin = NSVG_JOIN_BEVEL;
}

void bbx_gc_setlinecap(BBX_GC *gc, const char *capstyle)
{
	if (!strcmp(capstyle, "butt"))
		gc->attr.strokeLineCap = NSVG_CAP_BUTT;
	else if (!strcmp(capstyle, "round"))
		gc->attr.strokeLineCap = NSVG_CAP_ROUND;
	else if (!strcmp(capstyle, "square"))
		gc->attr.strokeLineCap = NSVG_CAP_SQUARE;
}



static void bbx_gc_beginsubpath(BBX_GC *gc, double x, double y)
{
	gc->pts[0] = (float) x;
	gc->pts[1] = (float) y;
	gc->npts = 1;
}

static void bbx_gc_endsubpath(BBX_GC *gc, int closed)
{
	NSVGattrib* attr = &gc->attr;
	NSVGpath* path = NULL;
	float bounds[4];
	float* curve;
	int i;

	if (gc->npts < 4)
		return;

	if (closed && gc->pts[0] != gc->pts[(gc->npts-1)*2] && 
		gc->pts[1] != gc->pts[(gc->npts-1)*2+1])
		  bbx_gc_lineto(gc, gc->pts[0], gc->pts[1]);

	path = (NSVGpath*)malloc(sizeof(NSVGpath));
	if (path == NULL) goto error;
	memset(path, 0, sizeof(NSVGpath));

	path->pts = (float*)malloc(gc->npts * 2 * sizeof(float));
	if (path->pts == NULL) goto error;
	path->closed = closed;
	path->npts = gc->npts;

	// Transform path.
	for (i = 0; i < gc->npts; ++i)
		nsvg__xformPoint(&path->pts[i * 2], &path->pts[i * 2 + 1], gc->pts[i * 2], gc->pts[i * 2 + 1], attr->xform);

	// Find bounds
	for (i = 0; i < path->npts - 1; i += 3) {
		curve = &path->pts[i * 2];
		nsvg__curveBounds(bounds, curve);
		if (i == 0) {
			path->bounds[0] = bounds[0];
			path->bounds[1] = bounds[1];
			path->bounds[2] = bounds[2];
			path->bounds[3] = bounds[3];
		}
		else {
			path->bounds[0] = nsvg__minf(path->bounds[0], bounds[0]);
			path->bounds[1] = nsvg__minf(path->bounds[1], bounds[1]);
			path->bounds[2] = nsvg__maxf(path->bounds[2], bounds[2]);
			path->bounds[3] = nsvg__maxf(path->bounds[3], bounds[3]);
		}
	}

	path->next = gc->plist;
	gc->plist = path;
	gc->npts = 0;

	return;

error:
	if (path != NULL) {
		if (path->pts != NULL) free(path->pts);
		free(path);
	}
}


void bbx_gc_rotate(BBX_GC *gc, double theta)
{
	float mtx[6];
	nsvg__xformSetRotation(mtx, (float) theta);
	nsvg__xformMultiply(gc->mtx, mtx);
	memcpy(gc->attr.xform, gc->mtx, 6 * sizeof(float));
}

void bbx_gc_translate(BBX_GC *gc, double x, double y)
{
	float mtx[6];
	nsvg__xformSetTranslation(mtx, (float) x, (float) y);
	nsvg__xformMultiply(gc->mtx, mtx);
	memcpy(gc->attr.xform, gc->mtx, 6 * sizeof(float));
}

void bbx_gc_scale(BBX_GC *gc, double s)
{
	float mtx[6];
	nsvg__xformSetScale(mtx, (float) s, (float) s);
	nsvg__xformMultiply(gc->mtx, mtx);
	memcpy(gc->attr.xform, gc->mtx, 6 * sizeof(float));
}

void bbx_gc_scalexy(BBX_GC *gc, double sx, double sy)
{
	float mtx[6];
	nsvg__xformSetScale(mtx, (float) sx, (float) sy);
	nsvg__xformMultiply(gc->mtx, mtx);
	memcpy(gc->attr.xform, gc->mtx, 6 * sizeof(float));
}

/*
a	Scales the drawing horizontally	
b	Skew the the drawing horizontally	
c	Skew the the drawing vertically	
d	Scales the drawing vertically	
e	Moves the the drawing horizontally	
f	Moves the the drawing vertically
*/
void bbx_gc_transform(BBX_GC *gc, double a, double b, double c, double d, double e, double f)
{
	float mtx[6];

	mtx[0] = (float)a;
	mtx[1] = (float)b;	
	mtx[2] = (float)c;	
	mtx[3] = (float)d;	
	mtx[4] = (float)e;
	mtx[5] = (float)e;

	nsvg__xformMultiply(gc->mtx, mtx);
	memcpy(gc->attr.xform, gc->mtx, 6 * sizeof(float));
}

void bbx_gc_settransfrom(BBX_GC *gc, double a, double b, double c, double d, double e, double f)
{
	float mtx[6];

	mtx[0] = (float)a;
	mtx[1] = (float)b;
	mtx[2] = (float)c;
	mtx[3] = (float)d;
	mtx[4] = (float)e;
	mtx[5] = (float)e;

	memcpy(gc->mtx, mtx, 6 * sizeof(float));
	memcpy(gc->attr.xform, gc->mtx, 6 * sizeof(float));
}


void bbx_gc_addcubic(BBX_GC *gc, double x1, double y1, double x2, double y2, double x3, double y3)
{
	if (gc->npts >= gc->cpts + 3)
	{
		gc->pts = realloc(gc->pts, gc->cpts * 2 * 2 * sizeof(float));
		if (!gc->pts)
			goto error;
		gc->cpts *= 2;
	}
	gc->pts[gc->npts * 2] = (float) x1;
	gc->pts[gc->npts * 2 + 1] = (float) y1;
	gc->pts[gc->npts * 2 + 2] = (float) x2;
	gc->pts[gc->npts * 2 + 3] = (float) y2;
	gc->pts[gc->npts * 2 + 4] = (float) x3;
	gc->pts[gc->npts * 2 + 5] = (float) y3;
	gc->npts += 3;
error:
	return;
}

void bbx_gc_addquadratic(BBX_GC *gc, double x1, double y1, double x2, double y2)
{

	double cx, cy;
	double cx1, cy1, cx2, cy2;

	cx = gc->pts[(gc->npts - 1) * 2];
	cy = gc->pts[(gc->npts - 1) * 2 + 1];

	// Convert to cubic bezier
	cx1 = x1 + 2.0f / 3.0f*(cx - x1);
	cy1 = y1 + 2.0f / 3.0f*(cy - y1);
	cx2 = x2 + 2.0f / 3.0f*(cx - x2);
	cy2 = y2 + 2.0f / 3.0f*(cy - y2);

	bbx_gc_addcubic(gc, cx1, cy1, cx2, cy2, x2, y2);
}

void bbx_gc_lineto(BBX_GC *gc, double x, double y)
{
	float x0, y0;

	if (gc->npts)
	{
		x0 = gc->pts[(gc->npts - 1) * 2];
		y0 = gc->pts[(gc->npts - 1) * 2 + 1];
		bbx_gc_addcubic(gc, x0, y0, x, y, x, y);
	}

}

void bbx_gc_beginpath(BBX_GC *gc)
{
	nsvg__deletePaths(gc->plist);
	gc->plist = 0;
	gc->npts = 0;
}


void bbx_gc_moveto(BBX_GC *gc, double x, double y)
{
	bbx_gc_endsubpath(gc, 0);
	bbx_gc_beginsubpath(gc, x, y);
}

void bbx_gc_closepath(BBX_GC *gc)
{
	bbx_gc_endsubpath(gc, 1);
}



void bbx_gc_arc(BBX_GC *gc, double cx, double cy, double r, double stheta, double etheta, int ccw)
{
	int npts;
	float sx, sy, ex, ey;
	float arc[64];
	int i;

	sx = (float) (cx + cos(stheta) * r);
	sy = (float) (cy + sin(stheta) * r);
	ex = (float) (cx + cos(etheta) * r);
	ey = (float) (cy + sin(etheta) * r);

	npts = nsvg__createarc(arc, (float) cx, (float) cy, sx, sy, ex, ey, ccw ? -1 : 1);
	bbx_gc_moveto(gc, sx, sy);
	for (i = 1; i < npts; i += 3)
	{
		bbx_gc_addcubic(gc, arc[i * 2], arc[i * 2 + 1], arc[i * 2 + 2], arc[i * 2 + 3], arc[i * 2 + 4], arc[i * 2 + 5]);
	}
}


void bbx_gc_arcto(BBX_GC *gc, double tx1, double ty1, double x2, double y2, double r)
{
	float x0 = 0, y0 = 0;
	float dx, dy;
	float len;
	float ox, oy;
	float arc[44];
	int npts;
	int dir;
	int i;

	if (gc->npts)
	{
		x0 = gc->pts[(gc->npts - 1) * 2];
		y0 = gc->pts[(gc->npts - 1) * 2 + 1];
	}
	dir = nsvg__side(x0, y0, (float) tx1, (float) ty1, (float) x2, (float) y2);
	dx = (float) tx1 - x0;
	dy = (float) ty1 - y0;
	len = (float) sqrt(dx*dx + dy*dy);
	dx /= len;
	dy /= len;
	ox = (float) (x0 - dy * dir * r);
	oy = (float) (y0 + dx * dir * r);

	npts = nsvg__createarc(arc, ox, oy, x0, y0, (float) x2, (float) y2, dir);
	for (i = 1; i < npts; i += 3)
	{
		bbx_gc_addcubic(gc, arc[i * 2], arc[i * 2 + 1], arc[i * 2 + 2], arc[i * 2 + 3], arc[i * 2 + 4], arc[i * 2 + 5]);
	}

}

int bbx_gc_pointinpath(BBX_GC *gc, double x, double y)
{
	int i;
	float t[3];
	int Ni = 0;
	NSVGpath *ptr;

	for (i = 0; i < gc->npts-3; i+= 3)
	{
		Ni += nsvg__curveIntersectXRay(t, gc->pts + i * 2, (float) x, (float) y);
	}

	
	for (ptr = gc->plist; ptr; ptr=ptr->next)
	{
		for (i = 0; i < ptr->npts-3; i += 3)
		{
			Ni += nsvg__curveIntersectXRay(t, ptr->pts + i * 2, (float) x, (float) y);
		}
	}

	return Ni % 2;
}








void bbx_gc_fill(BBX_GC *gc)
{
	NSVGrasterizer *r = gc->rast;
	NSVGedge *e = NULL;
	NSVGcachedPaint cache;
	NSVGattrib* attr = &gc->attr;//nsvg__getAttr(p);
	float scale = 1.0f;
	NSVGshape *shape;
	NSVGpath* path;
	int i;
	float tx = 0, ty = 0;
	int pointsinbuffer = gc->npts;

	if (gc->npts)
		bbx_gc_endsubpath(gc, 0);

	if (gc->plist == NULL)
		return;

	shape = (NSVGshape*)malloc(sizeof(NSVGshape));
	if (shape == NULL) goto error;
	memset(shape, 0, sizeof(NSVGshape));

	memcpy(shape->id, attr->id, sizeof shape->id);
	scale = nsvg__getAverageScale(attr->xform);
	shape->strokeWidth = attr->strokeWidth * scale;
	shape->strokeLineJoin = attr->strokeLineJoin;
	shape->strokeLineCap = attr->strokeLineCap;
	shape->fillRule = attr->fillRule;
	shape->opacity = attr->opacity;

	shape->paths = gc->plist;

	// Calculate shape bounds
	shape->bounds[0] = shape->paths->bounds[0];
	shape->bounds[1] = shape->paths->bounds[1];
	shape->bounds[2] = shape->paths->bounds[2];
	shape->bounds[3] = shape->paths->bounds[3];
	for (path = shape->paths->next; path != NULL; path = path->next) {
		shape->bounds[0] = nsvg__minf(shape->bounds[0], path->bounds[0]);
		shape->bounds[1] = nsvg__minf(shape->bounds[1], path->bounds[1]);
		shape->bounds[2] = nsvg__maxf(shape->bounds[2], path->bounds[2]);
		shape->bounds[3] = nsvg__maxf(shape->bounds[3], path->bounds[3]);
	}

	// Set fill
	if (attr->hasFill == 0) {
		shape->fill.type = NSVG_PAINT_NONE;
	}
	else if (attr->hasFill == 1) {
		shape->fill.type = NSVG_PAINT_COLOR;
		shape->fill.color = attr->fillColor;
		shape->fill.color |= (unsigned int)(attr->fillOpacity * 255) << 24;
	}
	else if (attr->hasFill == 2) {
		shape->fill.gradient = nsvggradient(gc->fillgrad, shape->bounds, &shape->fill.type);
		if (shape->fill.gradient == NULL) {
		  shape->fill.type = NSVG_PAINT_NONE;
		}
	}
	else if (attr->hasFill == 3) {
		cache.bitmap = gc->fillpat->rgba;
		cache.twidth = gc->fillpat->width;
		cache.theight = gc->fillpat->height;
		shape->fill.type = NSVG_PAINT_PATTERN;
	}

	if (shape->fill.type != NSVG_PAINT_NONE) {
		nsvg__resetPool(r);
		r->freelist = NULL;
		r->nedges = 0;

		nsvg__flattenShape(r, shape, scale);

		// Scale and translate edges
		for (i = 0; i < r->nedges; i++) {
			e = &r->edges[i];
			e->x0 = tx + e->x0;
			e->y0 = (ty + e->y0) * NSVG__SUBSAMPLES;
			e->x1 = tx + e->x1;
			e->y1 = (ty + e->y1) * NSVG__SUBSAMPLES;
		}

		// Rasterize edges
		qsort(r->edges, r->nedges, sizeof(NSVGedge), nsvg__cmpEdge);

		// now, traverse the scanlines and find the intersections on each scanline, use non-zero rule
		nsvg__initPaint(&cache, &shape->fill, shape->opacity);

		nsvg__rasterizeSortedEdges(r, tx, ty, scale, &cache, shape->fillRule);
	}
	if (pointsinbuffer)
	{
		NSVGpath *path = gc->plist;
		gc->plist = path->next;
		path->next = 0;
		nsvg__deletePaths(path);
		gc->npts = pointsinbuffer;
	}
	return;
error:
	return;
}

void bbx_gc_stroke(BBX_GC *gc)
{
	NSVGrasterizer *r = gc->rast;
	NSVGedge *e = NULL;
	NSVGcachedPaint cache;
	NSVGattrib* attr = &gc->attr;//nsvg__getAttr(p);
	float scale = 1.0f;
	NSVGshape *shape;
	NSVGpath* path;
	int i;
	float tx = 0, ty = 0;
	int pointsinbuffer = gc->npts;;

	if (gc->npts)
		bbx_gc_endsubpath(gc, 0);

	if (gc->plist == NULL)
		return;

	shape = (NSVGshape*)malloc(sizeof(NSVGshape));
	if (shape == NULL) goto error;
	memset(shape, 0, sizeof(NSVGshape));

	memcpy(shape->id, attr->id, sizeof shape->id);
	scale = nsvg__getAverageScale(attr->xform);
	shape->strokeWidth = attr->strokeWidth * scale;
	shape->strokeLineJoin = attr->strokeLineJoin;
	shape->strokeLineCap = attr->strokeLineCap;
	shape->fillRule = attr->fillRule;
	shape->opacity = attr->opacity;

	shape->paths = gc->plist;

	// Calculate shape bounds
	shape->bounds[0] = shape->paths->bounds[0];
	shape->bounds[1] = shape->paths->bounds[1];
	shape->bounds[2] = shape->paths->bounds[2];
	shape->bounds[3] = shape->paths->bounds[3];
	for (path = shape->paths->next; path != NULL; path = path->next) {
		shape->bounds[0] = nsvg__minf(shape->bounds[0], path->bounds[0]);
		shape->bounds[1] = nsvg__minf(shape->bounds[1], path->bounds[1]);
		shape->bounds[2] = nsvg__maxf(shape->bounds[2], path->bounds[2]);
		shape->bounds[3] = nsvg__maxf(shape->bounds[3], path->bounds[3]);
	}

	// Set stroke
	if (attr->hasStroke == 0) {
		shape->stroke.type = NSVG_PAINT_NONE;
	}
	else if (attr->hasStroke == 1) {
		shape->stroke.type = NSVG_PAINT_COLOR;
		shape->stroke.color = attr->strokeColor;
		shape->stroke.color |= (unsigned int)(attr->strokeOpacity * 255) << 24;
	}
	else if (attr->hasStroke == 2) {
		shape->stroke.gradient = nsvggradient(gc->strokegrad, shape->bounds, &shape->stroke.type);
		if (shape->stroke.gradient == NULL) {
			shape->stroke.type = NSVG_PAINT_NONE;
		}
	}
	else if (attr->hasStroke == 3) {
		cache.bitmap = gc->strokepat->rgba;
		cache.twidth = gc->strokepat->width;
		cache.theight = gc->strokepat->height;
		shape->stroke.type = NSVG_PAINT_PATTERN;
	}


	if (shape->stroke.type != NSVG_PAINT_NONE && (shape->strokeWidth * scale) > 0.01f) {
		nsvg__resetPool(r);
		r->freelist = NULL;
		r->nedges = 0;

		nsvg__flattenShapeStroke(r, shape, scale);

		//			dumpEdges(r, "edge.svg");

		// Scale and translate edges
		for (i = 0; i < r->nedges; i++) {
			e = &r->edges[i];
			e->x0 = tx + e->x0;
			e->y0 = (ty + e->y0) * NSVG__SUBSAMPLES;
			e->x1 = tx + e->x1;
			e->y1 = (ty + e->y1) * NSVG__SUBSAMPLES;
		}

		// Rasterize edges
		qsort(r->edges, r->nedges, sizeof(NSVGedge), nsvg__cmpEdge);

		// now, traverse the scanlines and find the intersections on each scanline, use non-zero rule
		nsvg__initPaint(&cache, &shape->stroke, shape->opacity);

		nsvg__rasterizeSortedEdges(r, tx, ty, scale, &cache, NSVG_FILLRULE_NONZERO);
	}
	if (pointsinbuffer)
	{
		NSVGpath *path = gc->plist;
		gc->plist = path->next;
		path->next = 0;
		nsvg__deletePaths(path);
		gc->npts = pointsinbuffer;
	}
	return;
error:
	return;
}

int bbx_gc_clip(BBX_GC *gc)
{
	unsigned char *alphamask;
	unsigned char *tempbitmap;
	unsigned char *clipmask;
	int x, y;
	int width, height;
	float tempOpacity;

	width = gc->rast->width;
	height = gc->rast->height;

	if (gc->rast->clipmask)
		clipmask = gc->rast->clipmask;
	else
		clipmask = malloc(width * height);
	if (!clipmask)
		goto error_exit;

	alphamask = malloc(width * height * 4);
	if (!alphamask)
		goto error_exit;
	memset(alphamask, 0, 4 * width * height);
	tempbitmap = gc->rast->bitmap;
	gc->rast->bitmap = alphamask;
	tempOpacity = gc->attr.fillOpacity;
	gc->attr.fillOpacity = 1.0;
	bbx_gc_fill(gc);
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			clipmask[y*width + x] = alphamask[(y*width + x) * 4 + 3];
		}
	}
	gc->rast->clipmask = clipmask;
	gc->rast->bitmap = tempbitmap;
	gc->attr.fillOpacity = tempOpacity;
	free(alphamask);
	return 0;

error_exit:
	return -1;
}

void bbx_gc_rect(BBX_GC *gc, double x, double y, double width, double height)
{
	bbx_gc_moveto(gc, x, y);
	bbx_gc_lineto(gc, x + width, y);
	bbx_gc_lineto(gc, x + width, y + height);
	bbx_gc_lineto(gc, x, y + height);
	bbx_gc_closepath(gc);
}

void bbx_gc_fillrect(BBX_GC *gc, double x, double y, double width, double height)
{
	bbx_gc_beginpath(gc);
	bbx_gc_rect(gc, x, y, width, height);
	bbx_gc_fill(gc);
}

void bbx_gc_strokerect(BBX_GC *gc, double x, double y, double width, double height)
{
	bbx_gc_beginpath(gc);
	bbx_gc_rect(gc, x, y, width, height);
	bbx_gc_stroke(gc);
}

int bbx_gc_clearrect(BBX_GC *gc, double x, double y, double width, double height)
{
	unsigned char *alphamask;
	unsigned char *tempbitmap;
	int ix, iy;
	int iwidth, iheight;
	float tempOpacity;

	iwidth = gc->rast->width;
	iheight = gc->rast->height;

	alphamask = malloc(iwidth * iheight * 4);
	if (!alphamask)
		goto error_exit;
	memset(alphamask, 0, 4 * iwidth * iheight);
	tempbitmap = gc->rast->bitmap;
	gc->rast->bitmap = alphamask;
	tempOpacity = gc->attr.fillOpacity;
	gc->attr.fillOpacity = 1.0;
	bbx_gc_fillrect(gc, x, y, width, height);
	for (iy = 0; iy < iheight; iy++)
	{
		for (ix = 0; ix < iwidth; ix++)
		{
			if (alphamask[(iy*iwidth + ix) * 4 + 3])
			{
				int alpha = tempbitmap[(iy*iwidth + ix) * 4 + 3];
				int newalpha = 255 - alphamask[(iy*iwidth + ix) * 4 + 3];
				tempbitmap[(iy*iwidth + ix) * 4 + 3] = nsvg__div255(alpha * newalpha);
			}
		}
	}
	gc->rast->bitmap = tempbitmap;
	gc->attr.fillOpacity = tempOpacity;
	free(alphamask);
	return 0;

error_exit:
	return -1;
}

void bbx_gc_circle(BBX_GC *gc, double cx, double cy, double r)
{
	bbx_gc_moveto(gc, cx + r, cy);
	bbx_gc_addcubic(gc, cx + r, cy + r*NSVG_KAPPA90, cx + r*NSVG_KAPPA90, cy + r, cx, cy + r);
	bbx_gc_addcubic(gc, cx - r*NSVG_KAPPA90, cy + r, cx - r, cy + r*NSVG_KAPPA90, cx - r, cy);
	bbx_gc_addcubic(gc, cx - r, cy - r*NSVG_KAPPA90, cx - r*NSVG_KAPPA90, cy - r, cx, cy - r);
	bbx_gc_addcubic(gc, cx + r*NSVG_KAPPA90, cy - r, cx + r, cy - r*NSVG_KAPPA90, cx + r, cy);
	bbx_gc_closepath(gc);
}

void bbx_gc_fillcircle(BBX_GC *gc, double cx, double cy, double r)
{
	bbx_gc_beginpath(gc);
	bbx_gc_circle(gc, cx, cy, r);
	bbx_gc_fill(gc);
}

void bbx_gc_drawimage(BBX_GC *gc, unsigned char *rgba, int width, int height, int x, int y)
{
	int ix, iy;
	int iystart, ixstart;
	int iwidth, iheight;

	iwidth = x + width <= gc->width ? width : gc->width - x;
	iheight = y + height <= gc->height ? height : gc->height - y;
	ixstart = x >= 0 ? 0 : -x;
	iystart = y >= 0 ? 0 : -y;
	for (iy = iystart; iy < iheight; iy++)
	{
		for (ix = ixstart; ix < iwidth; ix++)
		{
			int r, g, b;
			int a = rgba[(iy*width + ix) * 4 + 3];
			int ia = 255 - a;
			unsigned char *dst = gc->rast->bitmap + (y + iy)*gc->rast->stride + (x + ix) * 4;
			// Premultiply
			r = nsvg__div255(rgba[(iy*width + ix) * 4] * a);
			g = nsvg__div255(rgba[(iy*width + ix) * 4 + 1] * a);
			b = nsvg__div255(rgba[(iy*width + ix) * 4 + 2] * a);

			// Blend over
			r += nsvg__div255(ia * (int)dst[0]);
			g += nsvg__div255(ia * (int)dst[1]);
			b += nsvg__div255(ia * (int)dst[2]);
			a += nsvg__div255(ia * (int)dst[3]);

			dst[0] = (unsigned char)r;
			dst[1] = (unsigned char)g;
			dst[2] = (unsigned char)b;
			dst[3] = (unsigned char)a;
		}
	}
}

void bbx_gc_drawimagex(BBX_GC *gc, unsigned char *rgba, int width, int height, 
	int sx, int sy, int swidth, int sheight, int dx, int dy, int dwidth, int dheight)
{
	unsigned char *safe = 0;
	float scalemtx[6];
	float invmtx[6];
	unsigned char *sptr;
	int x, y;
	float cx[4], cy[4];
	float xlow, xhigh, ylow, yhigh;
	float tx, ty;
	float t1x, t1y, t2x, t2y, t3x, t3y;

	if (sx < 0 || sx >= width || swidth <= 0 ||
		sy < 0 || sy >= height || sheight <= 0)
		goto error;

	if (sheight + sy > height)
		sheight = height - sy;
	if (width + sx > width)
		swidth = width - sx;

	nsvg__xformIdentity(scalemtx);
	nsvg__xformSetScale(scalemtx, ((float)dwidth) / swidth, ((float)dheight) / sheight);
    nsvg__xformMultiply(scalemtx, gc->mtx);
	nsvg__xformInverse(invmtx, scalemtx);

	safe = malloc((swidth + 2) * (sheight + 3) * 4);
	memset(safe, 0, (swidth + 2) * (sheight + 3) * 4);
	for (y = 0; y < sheight; y++)
	{
		sptr = rgba + ((y + sy)*width + sx) * 4;
		memcpy(safe + ((y+1)*(swidth + 2) + 1) * 4, sptr, swidth * 4);
	}

	nsvg__xformPoint(&cx[0], &cy[0], dx-1.0f, dy-1.0f, gc->mtx);
	nsvg__xformPoint(&cx[1], &cy[1], (float) (dx + dwidth), dy-1.0f, gc->mtx);
	nsvg__xformPoint(&cx[2], &cy[2], (float) (dx + dwidth), (float) (dy + dheight), gc->mtx);
	nsvg__xformPoint(&cx[3], &cy[3], dx-1.0f, (float) (dy + dheight), gc->mtx);

	xlow = nsvg__minf(nsvg__minf(cx[0], cx[1]), nsvg__minf(cx[2], cx[3]));
	ylow = nsvg__minf(nsvg__minf(cy[0], cy[1]), nsvg__minf(cy[2], cy[3]));
	xhigh = nsvg__maxf(nsvg__maxf(cx[0], cx[1]), nsvg__maxf(cx[2], cx[3]));
	yhigh = nsvg__maxf(nsvg__maxf(cy[0], cy[1]), nsvg__maxf(cy[2], cy[3]));

	nsvg__xformPoint(&t1x, &t1y, xlow, ylow, invmtx);
	nsvg__xformPoint(&t2x, &t2y, xhigh, ylow, invmtx);
	nsvg__xformPoint(&t3x, &t3y, xlow, yhigh, invmtx);
	
	if (ylow < 0)
		ylow = 0;
	if (yhigh > gc->height - 1)
		yhigh = (float) (gc->height - 1);
	if (xlow < 0)
		xlow = 0;
	if (xhigh > gc->width - 1)
		xhigh = (float)(gc->width - 1);

	for (y = (int)ylow; y <= (int)yhigh; y++)
	{
		for (x = (int)xlow; x <= (int)xhigh; x++)
		{
			nsvg__xformPoint(&tx, &ty, (float) x, (float) y, invmtx);
			tx -= dx-1;
			ty -= dy-1;
		
			if (tx >= 0.0 && ty >= 0.0 && tx < swidth + 2 -1 && ty < sheight + 2 -1)
			{
				BBX_RGBA col;
				unsigned char *dst;
				int r, g, b, a;
				int ia;

				col = sampleTexture(safe, swidth + 2, sheight + 2, tx, ty, 0);
				dst = gc->rast->bitmap + y*gc->rast->stride + x * 4;
				// Premultiply
				a = col & 0xFF;
				ia = 255 - a;

				r = nsvg__div255(((col >> 24) & 0xFF) * a);
				g = nsvg__div255(((col >> 16) & 0xFF) * a);
				b = nsvg__div255(((col >> 8) & 0xFF) * a);

				// Blend over
				r += nsvg__div255(ia * (int)dst[0]);
				g += nsvg__div255(ia * (int)dst[1]);
				b += nsvg__div255(ia * (int)dst[2]);
				a += nsvg__div255(ia * (int)dst[3]);
				dst[0] = (unsigned char)r;
				dst[1] = (unsigned char)g;
				dst[2] = (unsigned char)b;
				dst[3] = (unsigned char)a;
			}
		}
	}
	free(safe);
error:
	return;

}
/*
source - over	Default.Displays the source image over the destination image	Play it »
source - atop	Displays the source image on top of the destination image.The part of the source image that is outside the destination image is not shown	Play it »
source - in	Displays the source image in to the destination image.Only the part of the source image that is INSIDE the destination image is shown, and the destination image is transparent	Play it »
source - out	Displays the source image out of the destination image.Only the part of the source image that is OUTSIDE the destination image is shown, and the destination image is transparent	Play it »
destination - over	Displays the destination image over the source image	Play it »
destination - atop	Displays the destination image on top of the source image.The part of the destination image that is outside the source image is not shown	Play it »
destination - in	Displays the destination image in to the source image.Only the part of the destination image that is INSIDE the source image is shown, and the source image is transparent	Play it »
destination - out	Displays the destination image out of the source image.Only the part of the destination image that is OUTSIDE the source image is shown, and the source image is transparent	Play it »
lighter	Displays the source image + the destination image	Play it »
copy	Displays the source image.The destination image is ignored	Play it »
xor	The source image is combined by using an exclusive OR with the destination image
*/
void bbx_gc_setglobalcompositeoperation(BBX_GC *gc, int op)
{
   /* not implemeted*/
}


void bbx_gc_setglobalapha(BBX_GC *gc, double alpha)
{
	/* not implemeted */
}
int bbx_gc_save(BBX_GC *gc)
{
	BBX_GC *state;

	state = clonestate(gc);
	if (!state)
		goto error;
	gc->prevstate = state;
	return 0;
error:
	return -1;
}

int bbx_gc_restore(BBX_GC *gc)
{
	int err;
	BBX_GC *state;
	if (!gc->prevstate)
		goto error;

	err = setState(gc, gc->prevstate);
	if (err)
		goto error;
	state = gc->prevstate;
	gc->prevstate = gc->prevstate->prevstate;
	state->prevstate = 0;
	deleteStates(state);
	return 0;
error:
	return -1;
}

