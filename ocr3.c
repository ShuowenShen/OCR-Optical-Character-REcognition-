/* Read the sample page and build the database. This is version 2, which
   uses scanned input instead of 'perfect' input. Image must be bi-level */
/* Creates a set of FEATURE VECTORS, measurements instead of templates */
/* Use with LEARN3.c */

#define MAX
#include "lib.h"
#define OBJECT 1
#define BACK 0
#define DEBUG 1
#define MARK 2
#define DTHRESH 33

struct lrec {
	int rs, re, cs, ce;
} LINE[400];
int Widths[128];

int Nlines = -1;
int Space = 1;
int Meanwidth = 0;
int MinWidth = 10000;
int MaxWidth = -1;
int MaxHeight = 0;

typedef unsigned char ** GIMAGE;

struct grec {		/* Information concerning a glyph */
	int fv1[32];
	char value;
	short int nr, nc;
	GIMAGE ptr;
	struct grec * next;
};
typedef struct grec * GPTR;
GPTR database[128];
int Nmax = 0, Mmax = 0;
float grid[128][128];

void orient  (IMAGE im);
void lines   (IMAGE im);
void extract (IMAGE im);
void dumpglyph (IMAGE im, int rs, int cs, int re, int ce, char gly);
void saveglyph (IMAGE im, int rs, int cs, int re, int ce, char gly, GPTR *x);
GIMAGE newglyph (int nr, int nc);
void initialize();
void txtread (IMAGE im, int lineno, char *result);
int match (GIMAGE a, int nra, int nca, GIMAGE b, int nrb, int ncb);
GIMAGE nextglyph (IMAGE im, struct lrec *thisline, int *thiscol, int *space,
		int *nr, int *nc);
void clean (IMAGE im);
void remove_mark (IMAGE im, int rs, int cs, int re, int ce);
void mark (IMAGE im, int row, int col);
void mark_region (IMAGE im, int *rs, int *cs, int *re, int *ce,
	 int row, int col);
void nextblackpixel (IMAGE im, int rs, int re, int this, int *row, int *col);
void nextblackcol (IMAGE im, int *this, int rs, int re);
void nextwhitecol (IMAGE im, int *this, int rs, int re);
void makegrid ();
float distn (int *v1, int *v2, int n);
int rect (GIMAGE x, int nr, int nc);
void loaddatabase();
void measure (GIMAGE x, int nr, int nc, int *fv2);
void mindist (int *fv2, float *d, int *val);
void interpret (GIMAGE x, int nr, int nc, char *str);
void segment (GIMAGE x, int nr, int nc, int sc, int *tc);
GIMAGE resize (GIMAGE x, int * nr, int nc, int newc);
void gprint (GIMAGE x, int nr, int nc);
int C1 (int a, int p);
int R1 (GIMAGE x, int nr, int nc, int b);
int perimeter (GIMAGE x, int val, int nr, int nc);
int area (GIMAGE x, int val, int nr, int nc);
int nay4 (GIMAGE x, int I, int J, int val, int nr, int nc);
void box(GIMAGE x, int val, float *x1, float *y1, int nr, int nc);

void main (int argc, char *argv[])
{
	IMAGE im;
	char text[80];
	FILE *f;
	int i;

	if (argc < 2)
	{
	  printf ("Usage: ocr5 <image> <text file>\n");
	  printf ("This program examines a scanned text image\n");
	  printf ("and attempts to recognize the characters. A text\n");
	  printf ("file will be created with the ASCII version of the\n");
	  printf ("image.\n");
	  exit (1);
	}

	im = Input_PBM (argv[1]);
	if (im == 0)
	{
	  printf ("The image file '%s' does not exist, or is unreadable.\n");
	  exit (2);
	}

	initialize();
	loaddatabase();
	clean (im);

/* Determine the orientation - this image is supposed to be perfect */
	orient (im);

/* Find the lines of text and remember where they were */
	lines (im);

/* Now extract and recognize the glyphs */
	f = fopen (argv[2], "w");
	if (f == NULL)
	{
		printf ("Can't open output file '%s'\n", argv[2]);
		exit(1);
	}

	for (i=0; i<Nlines; i++)
	{
	  txtread (im, i, text);
	  fprintf (f, "%s\n", text);
	}
	fclose (f);
}

/* Read the sample page and build the database. This is version 2, which
   uses scanned input instead of 'perfect' input. Image must be bi-level */
/* Creates a set of FEATURE VECTORS, measurements instead of templates */
/* Use with LEARN3.c */

#define MAX
#include "c:\AIPCV\ch9\lib.h"
#include <stdlib.h>
#include <stdio.h>
#include <cv.h>
#include <highgui.h>
#include <math.h>

#define OBJECT 1
#define BACK 0
#define DEBUG 1
#define MARK 2
#define DTHRESH 33

/* The bounding box of a line of text */
struct lrec {
	int rs, re, cs, ce;
} LINE[400];

int Widths[128];

int Nlines = -1;
int Space = 1;
int Meanwidth = 0;
int MinWidth = 10000;
int MaxHeight = 0;
long seed = 13751;
int SHOW = 0;
int MaxWidth = 2;

typedef unsigned char ** GIMAGE;

struct grec {		/* Information concerning a glyph */
	int fv1[32];
	char value;
	short int nr, nc;
	GIMAGE ptr;
	struct grec * next;
};

typedef struct grec * GPTR;
GPTR database[129];
int Nmax = 0, Mmax = 0;
float grid[128][128];


/* Prototypes - all in this file, not alphabetical */
void orient  (IMAGE im);
void lines   (IMAGE im);
void extract (IMAGE im);
void dumpglyph (IMAGE im, int rs, int cs, int re, int ce, char gly);
void saveglyph (IMAGE im, int rs, int cs, int re, int ce, char gly, GPTR *x);
GIMAGE newglyph (int nr, int nc);
void initialize();
void txtread (IMAGE im, int lineno, char *result);
int match (GIMAGE a, int nra, int nca, GIMAGE b, int nrb, int ncb);
GIMAGE nextglyph (IMAGE im, struct lrec *thisline, int *thiscol, int *space,
		int *nr, int *nc);
void clean (IMAGE im);
void remove_mark (IMAGE im, int rs, int cs, int re, int ce);
void mark (IMAGE im, int row, int col);
void mark_region (IMAGE im, int *rs, int *cs, int *re, int *ce,
	 int row, int col);
void nextblackpixel (IMAGE im, int rs, int re, int this, int *row, int *col);
void nextblackcol (IMAGE im, int *this, int rs, int re);
void nextwhitecol (IMAGE im, int *this, int rs, int re);
void makegrid ();
float distn (int *v1, int *v2, int n);
int rect (GIMAGE x, int nr, int nc);
void loaddatabase();
void measure (GIMAGE x, int nr, int nc, int *fv2);
void mindist (int *fv2, float *d, int *val);
void interpret (GIMAGE x, int nr, int nc, char *str);
void segment (GIMAGE x, int nr, int nc, int sc, int *tc);
GIMAGE resize (GIMAGE x, int * nr, int nc, int newc);
void gprint (GIMAGE x, int nr, int nc);
int C1 (int a, int p);
int R1 (GIMAGE x, int nr, int nc, int b);
int perimeter (GIMAGE x, int val, int nr, int nc);
int area (GIMAGE x, int val, int nr, int nc);
int nay4 (GIMAGE x, int I, int J, int val, int nr, int nc);
void box(GIMAGE x, int val, float *x1, float *y1, int nr, int nc);

struct image  *newimage (int nr, int nc);
void freeimage (struct image  *z);
void sys_abort (int val, char *mess);
void CopyVarImage (IMAGE *a, IMAGE *b);
void Display (IMAGE x);
float ** f2d (int nr, int nc);
void srand32 (long k);
double drand32 ();

//      Check that a pixel index is in range. Return TRUE(1) if so.     
int range (IMAGE im, int i, int j)
{
	if ((i<0) || (i>=im->info->nr)) return 0;
	if ((j<0) || (j>=im->info->nc)) return 0;
	return 1;
} 

struct image  *newimage (int nr, int nc)
{
	struct image  *x;                /* New image */
	int i;
	unsigned char *p;

	if (nr < 0 || nc < 0) {
		printf ("Error: Bad image size (%d,%d)\n", nr, nc);
		return 0;
	}

/*      Allocate the image structure    */
	x = (struct image  *) malloc( sizeof (struct image) );
	if (!x) {
		printf ("Out of storage in NEWIMAGE.\n");
		return 0;
	}

/*      Allocate and initialize the header      */

	x->info = (struct header *)malloc( sizeof(struct header) );
	if (!(x->info)) 
	{
		printf ("Out of storage in NEWIMAGE.\n");
		return 0;
	}
	x->info->nr = nr;       x->info->nc = nc;
	x->info->oi = x->info->oj = 0;

/*      Allocate the pixel array        */

	x->data = (unsigned char **)malloc(sizeof(unsigned char *)*nr); 

/* Pointers to rows */
	if (!(x->data)) 
	{
		printf ("Out of storage in NEWIMAGE.\n");
		return 0;
	}

	x->data[0] = (unsigned char *)malloc (nr*nc);
	p = x->data[0];
	if (x->data[0]==0)
	  {
		printf ("Out of storage. Newimage  \n");
		return 0;
	  }

	for (i=1; i<nr; i++) 
	{
	  x->data[i] = (p+i*nc);
	}

	return x;
}

void freeimage (struct image  *z)
{
/*      Free the storage associated with the image Z    */

	if (z != 0) 
	{
	   free (z->data[0]);
	   free (z->info);
	   free (z->data);
	   free (z);
	}
}

void sys_abort (int val, char *mess)
{
	fprintf (stderr, "**** System library ABORT %d: %s ****\n", 
			val, mess);
	exit (2);
}

// Small system random number generator 
double drand32 ()
{
	static long a=16807L, m=2147483647L,
		    q=127773L, r = 2836L;
	long lo, hi, test;

	hi = seed / q;
	lo = seed % q;
	test = a*lo -r*hi;
	if (test>0) seed = test;
	else seed = test + m;

	return (double)seed/(double)m;
}

void srand32 (long k)
{
	seed = k;
}

IplImage *toOpenCV (IMAGE x)
{
	IplImage *img;
	int i=0, j=0;
	CvScalar s;
	
	img = cvCreateImage(cvSize(x->info->nc, x->info->nr),8, 1);
	for (i=0; i<x->info->nr; i++)
	{
		for (j=0; j<x->info->nc; j++)
		{
			s.val[0] = x->data[i][j];
			cvSet2D (img, i,j,s);
		}
	}
	return img;
}

IMAGE fromOpenCV (IplImage *x)
{
	IMAGE img;
	int color=0, i=0;
	int k=0, j=0;
	CvScalar s;
	
	if ((x->depth==IPL_DEPTH_8U) &&(x->nChannels==1))								// 1 Pixel (grey) image
		img = newimage (x->height, x->width);
	else if ((x->depth==8) && (x->nChannels==3)) //Color
	{
		color = 1;
		img = newimage (x->height, x->width);
	}
	else return 0;

	for (i=0; i<x->height; i++)
	{
		for (j=0; j<x->width; j++)
		{
			s = cvGet2D (x, i, j);
			if (color) 
			  k = (unsigned char)((s.val[0] + s.val[1] + s.val[2])/3);
			else k = (unsigned char)(s.val[0]);
			img->data[i][j] = k;
		}
	}
	return img;
}

/* Display an image on the screen */
void display_image (IMAGE x)
{
	IplImage *image = 0;
	char wn[20];
	int i;

	image = toOpenCV (x);
	if (image <= 0) return;

	for (i=0; i<19; i++) wn[i] = (char)((drand32()*26) + 'a');
	wn[19] = '\0';
	cvNamedWindow( wn, CV_WINDOW_AUTOSIZE );
	cvShowImage( wn, image );
	cvWaitKey(0);
	cvReleaseImage( &image );
}

void save_image (IMAGE x, char *name)
{
	IplImage *image = 0;

	image = toOpenCV (x);
	if (image <0) return;

	cvSaveImage( name, image );
	cvReleaseImage( &image );
}

IMAGE get_image (char *name)
{
	IMAGE x=0;
	IplImage *image = 0;

 	image = cvLoadImage(name, 0);
	if (image <= 0) return 0;
	x = fromOpenCV (image);
	cvReleaseImage( &image );
	return x;
}

IMAGE grab_image ()
{
	CvCapture  *camera = 0;
	IplImage *image = 0;
	IMAGE x;

	camera = cvCaptureFromCAM( CV_CAP_ANY );
	if( !camera )								// Get a camera?
	{
		fprintf(stderr, "Canít initialize camera\n");
		return 0;
	}
	image = cvQueryFrame( camera );						
	x = fromOpenCV (image);
	cvReleaseCapture( &camera );
	return x;
}

int get_RGB (IMAGE *r, IMAGE *g, IMAGE *b, char *name)
{
	IplImage *image = 0;
	IMAGE i1, i2, i3;
	int i,j;

	// Read the image from a file into Ip1Image
	image = cvLoadImage(name, 1);
	if (image <= 0) return 0;

	// Create three AIPCV images of correct size
	i1 = newimage (image->height, image->width);
	i2 = newimage (image->height, image->width);
	i3 = newimage (image->height, image->width);

	// Copy pixels from Ip1Image to AIPCV images
	for (i=0; i<image->height; i++)
		for (j=0; j<image->width; j++)
		{
		  i1->data[i][j] = (image->imageData+i*image->widthStep)[j*image->nChannels+0];
		  i2->data[i][j] = (image->imageData+i*image->widthStep)[j*image->nChannels+1];
		  i3->data[i][j] = (image->imageData+i*image->widthStep)[j*image->nChannels+2];
		}
	cvReleaseImage(&image);
	*r = i3; 
	*g = i2;
	*b = i1;
	return 1;
}

int save_RGB (IMAGE r, IMAGE g, IMAGE b, char *name)
{
	IplImage *image = 0;
	int i,j,k;

	if ( (r->info->nc != g->info->nc) || (r->info->nr != g->info->nr) ) return 0;
	if ( (r->info->nc != b->info->nc) || (r->info->nr != b->info->nr) ) return 0;

	// Create an  IplImage
	image = cvCreateImage(cvSize(r->info->nc, r->info->nr),IPL_DEPTH_8U,3);
	if (image <= 0) return 0;
		
	for (i=0; i<image->height; i++)
		for (j=0; j<image->width; j++)
		{
		  (image->imageData+i*image->widthStep)[j*image->nChannels+0] = b->data[i][j];
		  (image->imageData+i*image->widthStep)[j*image->nChannels+1] = g->data[i][j];
		  (image->imageData+i*image->widthStep)[j*image->nChannels+2] = r->data[i][j];
		} 
	k = cvSaveImage(name, image);
	cvReleaseImage(&image);
	return 1-k;

}

int main (int argc, char *argv[])
{
	IMAGE im;
	char text[128];
	char inimage[128], fullpath[128], name[128];
	int i,j;
	FILE *f;

	  printf ("Usage: ocr1 <image> <text file>\n");
	  printf ("This program examines a scanned text image\n");
	  printf ("and attempts to recognize the characters. A text\n");
	  printf ("file will be created with the ASCII version of the\n");
	  printf ("image.\n");

	printf ("Enter the name of the file used as an input: ");
	scanf ("%s", inimage);
	sprintf (fullpath, "C:\\AIPCV\\ch9\\%s", inimage);
	im = get_image (fullpath);
	
	if (im == 0)
	{
	  printf ("The image file '%s' does not exist, or is unreadable.\n", 
				fullpath);
	  return (2);
	}

	for (i=0; i<im->info->nr; i++)
		for (j=0; j<im->info->nc; j++)
		{
			if(im->data[i][j]>128) im->data[i][j] = 0;
			else im->data[i][j] = 1;
			if (i==j) printf ("%d  %d\n", i, im->data[i][j]);
		}


/* Get the database */
	initialize();
	loaddatabase ();

	printf ("Enter the name of the output data file: ");
	scanf ("%s", name);
	sprintf (fullpath, "C:\\AIPCV\\ch9\\%s", name);

	clean (im);

/* Determine the orientation - this image is supposed to be perfect */
	orient (im);

/* Find the lines of text and remember where they were */
	lines (im);

/* Read ... */
	f = fopen (fullpath, "w");
	if (f == NULL)
	{
		printf ("Can't open output file '%s'\n", argv[2]);
		exit(1);
	}

	for (i=0; i<Nlines; i++)
	{
	  txtread (im, i, text);
	  fprintf (f, "%s\n", text);
	}
	fclose(f);
}

void initialize()
{
	int i;
	
	for (i=0; i<128; i++)
	{
	  database[i] = (GPTR)0;
	  Widths[i] = 0;
	}
}

void orient (IMAGE im)
{
}

/* Locate each line of text, and remember the start and end columns
   and rows. These are saved in the global array LINES.		    */
void lines (IMAGE im)
{
	int *hpro, i,j,k,n,m, N;
	int lstart, lend;

	N = im->info->nr;

/* Construct a horizontal projection and look for minima. */
	hpro = (int *) malloc (N * sizeof (int));
	for (i=0; i<N; i++)
	{
	  m = 0;
	  for (j=0; j<im->info->nc; j++)
	    if (im->data[i][j] == OBJECT) m++;
	  hpro[i] = m;
	}

/* Zeros in HPRO mean that the row was all white. */
	i = 0;   j= 0;
	while (i<N)
	{

/* Find the start of a line */
	  while (i<N && hpro[i] == 0) i++;
	  lstart = i;
	  while (i<N && hpro[i]  > 0) i++;
	  lend = i-1;
	  LINE[j].rs = lstart; LINE[j].re = lend;

/* Look for the start and end columns */
	  LINE[j].cs = -1;
	  for (m=0; m<im->info->nc; m++)
	  {
	    for (n=lstart; n<=lend; n++)
	      if (im->data[n][m] == OBJECT)
	      {
	        LINE[j].cs = m;
	        break;
	      }
	    if (LINE[j].cs >= 0) break;
	  }

	  LINE[j].ce = -1;
	  for (m=im->info->nc-1; m>=0; m--)
	  {
	    for (n=lstart; n<=lend; n++)
	      if (im->data[n][m] == OBJECT)
	      {
	        LINE[j].ce = m;
	        break;
	      }
	    if (LINE[j].ce > 0) break;
	  }
	  j = j + 1;
	}
	j--;

	free (hpro);
	if (DEBUG)
	{
	  printf ("%d lines seen. Summary:\n", j-1);
	  for (m=0; m<j; m++)
	  {
	    printf ("Line %3d: (%d,%d)		(%d,%d)\n", m,LINE[m].rs,
	  	LINE[m].cs, LINE[m].rs, LINE[m].ce);
	    printf ("          (%d,%d)		(%d,%d)\n", LINE[m].re,
	  	LINE[m].cs, LINE[m].re, LINE[m].ce);
	  }
	  printf ("\n");
	}
	Nlines = j;

	printf ("A Space is %d pixels.\n", Space);
	MaxHeight = 0;
	for (i=0; i<Nlines; i++)
	  if (LINE[i].re-LINE[i].rs > MaxHeight)
	    MaxHeight = LINE[i].re-LINE[i].rs;
}

/* Extract the templates from the training image */
void extract (IMAGE im)
{
	int i,j,k, scol, ecol, qflag = 0, tspace=0, spacing;
	int thisline, thischar, rs,re,cs,ce, oldscol, nspace = 0;
	int rre, rrs, rce,rcs, row, col, actual, width, wid2;
	float mspace = 0;
	GPTR thisptr;

/* Use the first 10 lines (all characters) */
	for (thisline = 0; thisline <= 10; thisline++)
	{

/* How wide and tall is this line? */
	  rs = LINE[thisline].rs; re = LINE[thisline].re;
	  cs = LINE[thisline].cs; ce = LINE[thisline].ce;
	  thischar = 0;

/* Extract individual characters */
	  col = cs;
	  oldscol = -1;
	  do
	  {

/* Find the next black pixel */
	    scol = col;
	    nextblackpixel (im, rs, re, scol, &row, &col);
	    if (row < 0) break;

/* Mark the connected region */
	    rrs = rs; rre = re; rcs = col; rce = ce;
	    if (thisline < 8 || thisline > 10)
	    {
	      mark_region (im, &rrs, &rcs, &rre, &rce, row, col);
	    } else {

/* Special characters have a space between them, and WILL NOT overlap */
	      rcs = scol;
	      nextblackcol (im, &rcs, rs, re);
	      if (rcs < 0) continue;
	      rce = rcs;
	      nextwhitecol (im, &rce, rs, re);
	      if (rce < 0) continue;
	      rre = re; rrs = rs;
	      for (i=rrs; i<=rre; i++)
		for (j=rcs; j<=rce; j++)
		  if (im->data[i][j] == OBJECT)
			im->data[i][j] = MARK;
	    }

/* Use predicted character widths to determine multiple glyphs */

	    if (oldscol < 0) oldscol = rcs;
	    else
	    {
		spacing = rcs - oldscol;

/* Special characters all have a space between them */
		if (thisline > 7) spacing -= Space;
		tspace += spacing;
		nspace++;
		oldscol = rcs;
	    }
	    col = ++rce;
	  } while (rcs >= 0 && rce >= 0);

	}
	mspace = (float)tspace/(float)nspace;
	if (DEBUG)
		printf ("Average character width: %f\n", mspace);
	Meanwidth = (int)(mspace+0.5);
}

/*	Return the index of the next column with a black pixel in it	*/
void nextblackpixel (IMAGE im, int rs, int re, int this, int *row, int *col)
{
	int i,j;

	for (j= this; j<im->info->nc; j++)
	  for (i=rs; i<=re; i++)
	    if (im->data[i][j] == OBJECT)
	    {
		*row = i; *col = j;
		return;
	    }
	*row = -1;
	return ;
}

/* Print a glyph */
void dumpglyph (IMAGE im, int rs, int cs, int re, int ce, char gly)
{
	int i,j;

	if (!DEBUG) return;
	printf ("========== Next character is '%c' ==========\n", gly);
	for (i=rs; i<=re; i++)
	{
	  printf ("====    ");
	  for (j=cs; j<=ce; j++)
	    if (im->data[i][j] == MARK) printf ("#");
	     else printf (" ");
	  printf ("	====\n");
	}
	printf ("===================================================\n\n");
}

/* Create a new glyph record and return a pointer */
GIMAGE newglyph (int nr, int nc)
{
	unsigned char **a, *b;
	int i,j;

	a = (unsigned char **) malloc (nr * sizeof (unsigned char *));
	b = (unsigned char *)  malloc (nr*nc);
	a[0] = b;
	for (i=1; i<nr; i++)
	  a[i] = a[i-1] + nc;

	for (i=0; i<nr; i++)
	  for (j=0; j<nc; j++)
	    a[i][j] = (unsigned char)0;

	return a;
}

/* Read the specified line of text from the image */
void txtread (IMAGE im, int lineno, char *result)
{
	int i,j,k, cs,ce, nr,nc, space, csold;
	GIMAGE x;
	char temp[128];

	cs = LINE[lineno].cs; ce = LINE[lineno].ce;
	csold = cs;
	k = 0;
	do
	{
	  x = nextglyph (im, &(LINE[lineno]), &cs, &space, &nr, &nc);
	  if (x == 0) break;

	  interpret (x, nr, nc, temp);

	  for (i=0; temp[i] != '\0'; i++)
	    result[k++] = temp[i];

	  free(x);
	} while (cs < ce);

	result[k++] = '\0';
	printf ("String was \"%s\"\n", result);
}

void interpret (GIMAGE x, int nr, int nc, char *str)
{
	int i,j,k, val, cmin, ctry, scmin;
	int fv2[32], vmin, nnr;
	float d, dmin;
	GIMAGE xx;

	printf ("+++++++++ INTERPRET +++++++++\n");
	str[0] = '\0';
	if (nc <MinWidth) return;

/* One character or many? */
	if (nc < MaxWidth+MinWidth)
	{

/* One - measure and match */
	  measure (x, nr, nc, fv2);
	  mindist (fv2, &d, &val);

/* The character has been identified */
	  if (d < DTHRESH)
	  {
	    str[0] = (char)val;
	    str[1] = '\0';
	    return;
	  }
	  printf ("Recognition failed.\n");
	}

/* At this point no match has been achieved. Tty splitting the image up */
	printf ("Trying a pair:\n");
	cmin = MinWidth; dmin = 10000; vmin = 0;

	while (cmin < MaxWidth+MinWidth)
	{
	  segment (x, nr, nc, cmin, &ctry);
	  measure (x, nr, ctry, fv2);
	  mindist (fv2, &d, &val);
	  if (d < DTHRESH-DTHRESH/3)
	  {
	    str[0] = (char)val;
	    nnr = nr;
	    xx = resize(x, &nnr, nc, ctry);
	    if (xx == 0) break;
	    printf ("Found '%c', recurring ...\n", val);
	    interpret (xx, nnr, nc-ctry-1, &(str[1]));
	    printf ("Back from recursion. String is '%s'\n", str);
	    free (xx);
	    return;
	  }
	  else if (d < dmin)
	  {
	    dmin = d;
	    vmin = val;
	    scmin = ctry;
	    if (vmin)
	      printf ("Best so far is '%c'\n", vmin);
	    else printf ("No best!\n");
	  }
	  cmin = ctry+1;
	  if (cmin >= nc) break;
	}

	if (vmin == 0)
	{
	  printf ("Giving up!\n");
	  str[0] = '?'; str[1] = '\0';
	  return;
	}
	str[0] = (char)vmin;
	nnr = nr;
	xx = resize(x, &nnr, nc, scmin);
	if (xx == 0)
	{
	  str[1] = '\0';
	  return;
	}
	printf ("Found '%c', recurring ,,,\n", vmin);
	interpret (xx, nnr, nc-scmin-1, &(str[1]));
	printf ("Back from recursion. String is '%s'\n", str);
	free (xx);
}

GIMAGE resize (GIMAGE x, int *nr, int nc, int newc)
{
	int i,j, newr;
	GIMAGE y;

	if (nc <= newc)
	  return 0;
	newr = -1;
	for (i=0; i<*nr; i++)
	{
	  for (j=newc+1; j<nc; j++)
	    if (x[i][j] > 0)
	    {
	      newr = i;
	      break;
	    }
	  if (newr >= 0) break;
	}
	if (newr < 0) newr = 0;

	y = newglyph ( *nr-newr, nc-newc-1);
	for (i=newr; i<*nr; i++)
	  for (j=newc+1; j<nc; j++)
	    y[i-newr][j-newc-1] = x[i][j];
	*nr = *nr - newr;
	printf ("Resized:\n");
	gprint (y, *nr, nc-newc-1);
	return y;
}

void segment (GIMAGE x, int nr, int nc, int sc, int *tc)
{
	int i,j,k, *pro, min, T;

	T = MaxHeight/8;
	if (nc-sc < MinWidth)
	{
	  *tc = nc;
	  return;
	}
	pro = (int *)malloc (nc*sizeof(int));
	for (j=sc; j<nc; j++)
	{
	  pro[j] = 0;
	  for (i=0; i<nr; i++)
	    if (x[i][j] > 0) pro[j] += 1;
	}

	min = sc;
	for (j=sc; j<nc-MinWidth; j++)
	  if (pro[j] < T) { min = j; break; };

	j = min + 1;
	while (j <nc-MinWidth && pro[j]<pro[j-1]) j++;
	*tc = min = j-1;
	printf ("Segmentation is :\n");
	for (i=0; i<nr; i++)
	{
	  for (j=0; j<=min; j++)
	    if (x[i][j] > 0) printf ("#"); else printf (" ");
	  printf("\n");
	}
	free (pro);
}

void mindist (int *fv2, float *d, int *val)
{
	GPTR p;
	int i,j;
	float dd=10000, x;

	*val = '!'; 
	for (i=0; i<127; i++)
	{
	  if (database[i])
	  {
	    p = database[i];
	    while (p)
	    {
	      x = distn (p->fv1, fv2, 15);
	      if (x < dd)
	      {
	        printf("Best was (%c, %f) now (%c,%f)\n", *val, dd, i, x);
		dd = x;
		*val = i;
	      }
	      p = p->next;
	    }
	  }
	}
	printf ("Classed as '%c'\n", *val);
	*d = dd;
}


void measure (GIMAGE x, int nr, int nc, int *fv2)
{
	int i,j, n1,n2,m1,m2, ii,jj, k, a, p;
	float z;
	int c[3][3] = {0,0,0,0,0,0,0,0,0};
	int g[3][3] = {0,0,0,0,0,0,0,0,0};

	printf ("== Measuring :\n");
	gprint (x, nr, nc);

/* Aspect */
	z = (float)(nr)/(float)(nc);
	fv2[0] = (int)(z*10.0 + 0.5);
	if (fv2[0] > 100) fv2[0] = 100;

/* Density */
	z = 0.0;
	for (i=0; i<nr; i++)
	  for (j=0; j<nc; j++)
	    if (x[i][j] > 0) z += 1.0;
	z = z/(float)(nr*nc);
	fv2[1] = (int)(z * 100.0 + 0.5);

/* Height ratio */
	fv2[2] = (int) ( ((float)nr/(float)MaxHeight * 100) + 0.5 );

/* Width ratio */
	fv2[3] = 0;

/* Circularity */
	a = area (x, 1, nr, nc);
	p = perimeter (x, 1, nr, nc);
	fv2[4] = C1 (a, p);

/* Rectangularity */
	fv2[5] = R1 (x, nr, nc, a);

/* Samples */
	n1 = nr/3;       n2 = n1+n1;
	m1 = nc/3;       m2 = m1+m1;
	for (i=0; i<3; i++)
	  for (j=0; j<3; j++)
	    g[i][j] = c[i][j] = 0;

	for (i=0; i<nr; i++)
	{
	  if (i<=n1) ii = 0;
	  else if (i<n2) ii = 1;
	  else ii = 2;
	  for (j=0; j<nc; j++)
	  {
	    if (j<=m1) jj = 0;
	    else if (j<m2) jj = 1;
	    else jj = 2;
	    if (x[i][j]>0) g[ii][jj] += 1;
	    c[ii][jj] +=1;
	  }
	}
	k = 6;
	for (i=0; i<3; i++)
	  for (j=0; j<3; j++)
	    if (c[i][j] > 0)
	      fv2[k++] = (int) (((float)g[i][j]/(float)c[i][j] *100)+0.5);
	    else fv2[k++] = 0;

	printf ("Vector:\n");
	for (i=0; i<15; i++)
	  printf ("%3d ", fv2[i]);
	printf("\n");
}

/* Extract the next glyph on the given line. Return a GIMAGE */
GIMAGE nextglyph (IMAGE im, struct lrec *thisline, int *thiscol, int *space,
		int *nr, int *nc)
{
	int i,j,k, rs, re, cs, ce, first, last;
	int rrs, rre, rcs, rce, scol, row, col;
	static int oldscol = -1;
	GIMAGE x;

	printf ("------------ NEXTGLYPH -------------\n");
	*space = -1;
	*nr = *nc = 0;
	rs = thisline->rs; re = thisline->re;
	cs = thisline->cs; ce = thisline->ce;
	scol = *thiscol;
	nextblackpixel (im, rs, re, scol, &row, &col);
	if (row < 0)
	  return 0;
	rrs = rs; rre = re; rcs = cs; rce = ce;
	mark_region (im, &rrs, &rcs, &rre, &rce, row, col);

	if (rcs >= ce) 
	  return 0;
	*space = 0;

/* We now have a minimal bounding box. Save the glyph */
	*nr = rre-rrs+1;	*nc =  rce-rcs+1;
	x = newglyph (rre-rrs+1, rce-rcs+1);
	for (i=rrs; i<=rre; i++)
	  for (j=rcs; j<=rce; j++)
	    x[i-rrs][j-rcs] = im->data[i][j]==MARK;


	*thiscol = rce + 1;
	remove_mark (im, rrs, rcs, rre, rce);
	return x;
}

void clean (IMAGE im)
{
	int i,j, ii, jj;

	for (i=1; i<im->info->nr-1; i++)
	  for (j=1; j<im->info->nc-1; j++)
	    if (im->data[i][j] == OBJECT)
	    {
		if (im->data[i][j-1] == BACK && im->data[i][j+1] == BACK &&
		    im->data[i-1][j] == BACK && im->data[i+1][j] == BACK &&
		    im->data[i-1][j-1] == BACK && im->data[i-1][j+1] == BACK &&
		    im->data[i+1][j-1] == BACK && im->data[i+1][j+1] == BACK)
		im->data[i][j] = BACK;
	    }
}

/* Mark a glyph with '2' valued pixels. */
void mark_region (IMAGE im, int *rs, int *cs, int *re, int *ce, int row, int col)
{
	int i,j,k,rmax,rmin,cmax,cmin, delta, flag = 0;

	if (DEBUG) printf ("Marking ...\n");
	mark (im, row, col);

	rmax = cmax = -1;	rmin = cmin = 10000;
	delta = (*ce-*cs+1)/4;
	for (j= *cs-delta; j<= *ce+delta; j++)
	{
	  for (i= *rs; i<= *re; i++)
	  {
	    if (range(im,i,j) == 0) continue;
	    if (im->data[i][j] == MARK)
	    {
		if (i<rmin) rmin = i;
		if (j<cmin) cmin = j;
		if (i>rmax) rmax = i;
		if (j>cmax) cmax = j;
	    }
	  }
	}

/* Look for dots above i and j: */
/*	for (j=(cmax+cmin)/2 - 1; j<=(cmax+cmin)/2+1; j++) */
	for (j=MinWidth/2+1+cmin; j<=cmax-MinWidth/2; j++)
	  for (i= *rs; i<=*re; i++)
	    if (im->data[i][j] == 1)
	    {
		mark (im, i, j);
		flag = 1;
	    }

	if (flag)
	{
	  rmax = cmax = -1;	rmin = cmin = 10000;
	  for (j= *cs-delta; j<= *ce+delta; j++)
	    for (i= *rs; i<= *re; i++)
	    {
	      if (range(im,i,j) == 0) continue;
	      if (im->data[i][j] == MARK)
	      {
	  	if (i<rmin) rmin = i;
	  	if (j<cmin) cmin = j;
	  	if (i>rmax) rmax = i;
	  	if (j>cmax) cmax = j;
	      }
	    }
	}

	*rs = rmin; *re = rmax;
	*cs = cmin; *ce = cmax;

	if (DEBUG)
	for (i= rmin; i<= rmax; i++)
	{
	  for (j=cmin; j<=cmax; j++)
	    printf ("%1d", im->data[i][j]);
	  printf ("\n");
	}
}

void mark (IMAGE im, int row, int col)
{
	im->data[row][col] = MARK;
	if (im->data[row-1][col-1] == OBJECT) mark (im, row-1,col-1);
	if (im->data[row-1][col] == OBJECT) mark (im, row-1,col);
	if (im->data[row-1][col+1] == OBJECT) mark (im, row-1,col+1);
	if (im->data[row][col-1] == OBJECT) mark (im, row,col-1);
	if (im->data[row][col+1] == OBJECT) mark (im, row,col+1);
	if (im->data[row+1][col-1] == OBJECT) mark (im, row+1,col-1);
	if (im->data[row+1][col] == OBJECT) mark (im, row+1,col);
	if (im->data[row+1][col+1] == OBJECT) mark (im, row+1,col+1);
}

void remove_mark (IMAGE im, int rs, int cs, int re, int ce)
{
	int i,j;

	for (i=rs; i<=re; i++)
	  for (j=cs; j<=ce; j++)
	    if (im->data[i][j] == MARK) im->data[i][j] = BACK;
}

/*      Return the index of the next column with a black pixel in it    */
void nextblackcol (IMAGE im, int *this, int rs, int re)
{
	int i,j;

	for (j= *this; j<im->info->nc; j++)
	  for (i=rs; i<=re; i++)
	    if (im->data[i][j] == OBJECT)
	    {
	        *this = j;
	        return;
	    }
	*this = -1;
	return ;
}

/*      Return the index of the next column with a black pixel in it    */
void nextwhitecol (IMAGE im, int *this, int rs, int re)
{
	int i,j, flag;

	for (j=*this; j<im->info->nc; j++)
	{
	  flag = 1;
	  for (i=rs; i<=re; i++)
	    if (im->data[i][j] == OBJECT)
	    {
	      flag = 0;
	      break;
	    }
	  if (flag)
	  {
	    *this = j;
	    return;
	  }
	}
	*this = -1;
	return ;
}

float distn (int *v1, int *v2, int n)
{
	int i,sum=0;

	for (i=0; i<n; i++)
	  sum += (v1[i]-v2[i])*(v1[i]-v2[i]);
	return (float)sqrt((double)sum);
}

/* Read the database from the file */
void loaddatabase ()
{
	FILE *f;
	int i,j,k, n,m, nr, nc, ival, items, v;
	GPTR p, q;
	char cval;

	f = fopen ("c:\\aipcv\\ch9\\trf.db", "r");
	if (f == NULL)
	{
	  printf ("Cannot read database from file '%s'\n", "tr.db");
	  return;
	}

	fscanf (f, "%d", &Space);
	fscanf (f, "%d", &Meanwidth);
	do
	{
	  items = fscanf (f, "%c", &cval);
	  if (items < 1) break;

	  while (cval == ' ' || cval == '\n')
	  {
	    items = fscanf (f, "%c", &cval);
	    if (items < 1) break;
	  }

	  ival = (int)cval;
	  items = fscanf (f, "%d", &k);
	  items = fscanf (f, "%d", &nr);
	  items = fscanf (f, "%d", &nc);
	  items = fscanf (f, "%d", &(Widths[ival]));
	  if (items < 1) break;

	  q = (GPTR) malloc (sizeof (struct grec));
	  q->nr = nr; q->nc = nc; q->value = cval; q->next = (GPTR)0;


	  for (i=0; i<15; i++)
	    fscanf (f, "%d", &(q->fv1[i]));
	  q->fv1[3] = 0;

	  if (database[ival] == (GPTR)0)
	    database[ival] = q;
	  else {
	    p = database[ival];
	    while (p->next) p = p->next;
	    p->next = q;
	  }
	} while (items > 0);
	fclose (f);

/* Compute maximum character widths and save them */
	for (i=0; i<127; i++)
	{
	  if (database[i]) 
	  {
	    if (Widths[i] <= 0) continue;
	    if (Widths[i] < MinWidth) MinWidth = Widths[i];
	    if (Widths[i] > MaxWidth) MaxWidth = Widths[i];

	    p = database[i];
	    while (p)
	    {
	      if (Widths[i] <= 0) continue;
	      if (Widths[i] < MinWidth) MinWidth = Widths[i];
	      if (Widths[i] > MaxWidth) MaxWidth = Widths[i];
	      p = p->next;
	    }
	  }
	}
	printf ("Minimum width is %d pixels, maximum is %d\n\n", MinWidth, MaxWidth);
}

void gprint (GIMAGE x, int nr, int nc)
{
	int i,j;

	for (i=0; i<nr; i++)
	{
	  for (j=0; j<nc; j++)
		if (x[i][j] > 0) printf ("#"); else printf (" ");
	  printf ("\n");
	}
}

 int R1 (GIMAGE x, int nr, int nc, int b)
 {
         float x1[5], y1[5], a;
 
         box (x, 1, x1, y1, nr, nc);
         a = (float) ((fabs((double)(x1[1]-x1[0]))+1.0) *
                       (fabs((double)(y1[2]-y1[0]))+1.0) );
         if (a == 0) return 0;
         return (int)(b/a *100 + 0.5);
 }

 void box(GIMAGE x, int val, float *x1, float *y1, int nr, int nc)
 {
         int i,j,ip1,jp1,ip2,jp2;
         
         ip1 = 10000;    jp1 = 10000;
         ip2 = -1;       jp2 = -1;
 
 /* Find the min and max coordinates, both row and column */
         for (i=0; i<nr; i++)
           for(j=0; j<nc; j++)
                 if (x[i][j] == val) {
                       if (i < ip1) ip1 = i;
                       if (i > ip2) ip2 = i;
                       if (j < jp1) jp1 = j;
                       if (j > jp2) jp2 = j;
                 }
         if (jp2 < 0) 
                 return;
 
 
         y1[0] = (float) jp1;    x1[0] = (float) ip1;
         y1[1] = (float) jp1;    x1[1] = (float) ip2;
         y1[2] = (float) jp2;    x1[2] = (float) ip2;
         y1[3] = (float) jp2;    x1[3] = (float) ip1;
 }

 int C1 (int a, int p)
 {
         if (a <= 0.0) return 0;
         else return (int)( (p*p/(3.1415926535*4.0*a)) * 10 + 0.5);
 }
 
int area (GIMAGE x, int val, int nr, int nc)
{
	int i,j,k;
	
	k = 0;
	for (i=0; i<nr; i++)
	  for (j=0; j<nc; j++)
	    if (x[i][j] == val) k++;
	return k;
}

int nay4 (GIMAGE x, int I, int J, int val, int nr, int nc)
{
	int k=0;

	if (I+1<nr && x[I+1][J]==val) k++;
	if (I-1>=0 && x[I-1][J]==val) k++;
	if (J+1<nc && x[I][J+1]==val) k++;
	if (J-1>=0 && x[I][J-1]==val) k++;
	return k;
}

int perimeter (GIMAGE x, int val, int nr, int nc)
{
	int i,j,k, ii,jj,t;
	float p;
	GIMAGE y;

	p = 0; y = 0;
	y = newglyph (nr+2, nc+2);
	for (i=0; i<nr+2; i++)
	  for (j=0; j<nc+2; j++)
	    y[i][j] = 0;
	for (i=0; i<nr; i++)
	  for (j=0; j<nc; j++)
	    y[i+1][j+1] = x[i][j];
	nr +=2; nc+=2;

/* Remove all pixels except those having value VAL */
	for (i=1; i<nr-1; i++) 
	{
	  for (j=1; j<nc-1; j++) 
	  {
	    if (x[i-1][j-1] != val) 
	    {
		y[i][j] = BACK;
		continue;
	    }
	    k = nay4(x, i-1, j-1, val, nr-2, nc-2); /* How many neighbors are VAL */
	    if (k < 4)	      /* If not all, this is on perim */
	    	y[i][j] = 1;
	    else y[i][j] = BACK;
	  }
	}

	for (i=0; i<nr; i++) 
	{
	  for (j=0; j<nc; j++) 
	  {
	    if (y[i][j] != OBJECT) continue;
	    if (i==0 || j==0 || i==nr-1 || j == nc-1)
		    continue;

/*      Match one of the templates      */

	    k = 1;  t = 0;
	    for (ii= -1; ii<=1; ii++) 
	    {
	      for (jj = -1; jj<=1; jj++) 
	      {
	    	if (ii==0 && jj==0) continue;
	    	if (y[i+ii][j+jj] == OBJECT)
	    		t = t + k;
	    	k = k << 1;
	      }
	    }

/*      Templates for 1.207:
     o o o   o o #   o # o   o # o    # o o    o o #    o o o   # o o
     # # o   # # o   o # o   o # o    o # o    o # o    o # #   o # #
     o o #   o o o   # o o   o o #    o # o    o # o    # o o   o o o
 T=   210    014       042      202      101      104      060     021

	Templates for 1.414:
	 # o o   o o #   # o o   o o #   o o o   # o #
	 o P o   o P o   o P o   o P o   o P o   o P o
	 o o #   # o o   # o o   o o #   # o #   o o o
  T=	201     044     041     204    240    005

       Templates for 1.0:

		o o o	   o # o    o o o   o o o   o # o   o # o
		# # #	   o # o    # # o   o # #   # # o   o # #
		o o o	   o # o    o # o   o # o   o o o   o o o
		030	       102     72      80      10      18

*/
	    if (t==0210 || t == 014 || t == 042 ||
	        t==0202 || t ==0101 || t ==0104 ||
	        t== 060 || t == 021) 
	    {
	    	p += 1.207;
	    	continue;
	    }

	    if (t == 0201 || t == 044 || t == 041 ||
	        t == 0204 || t ==0240 || t == 005) 
	    {
	    	p += 1.414;
	    	continue;
	    }

	    if (t == 030 || t == 0102 || t == 80 ||
	        t == 10 || t == 18) 
	    {
	    	p += 1.0;
	    	continue;
	    }

	    p += 1.207;
	  }
	}
	free (y);
	return (int)(p+0.5);
}
