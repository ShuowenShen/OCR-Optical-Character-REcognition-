/* OCR4 - Read glyphs from the specified scanned image and create a 
   text file from them. Input image must be thresholded.		*/
/* Use with LEARN2.C */

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
#define DB2 0
#define MARK 2
#define HOLE 3
#define SPLIT_THRESH 0.15
int PRINTMATCH = 0;

/* The bounding box of a line of text */
struct lrec {
	int rs, re, cs, ce;
} LINE[400];

int Widths[128];
int mask[128];
int Nlines = -1;
int Space = 1;
int Meanwidth = 0;
long seed = 13751;
int SHOW = 0;
int MaxWidth = 2;

typedef unsigned char ** GIMAGE;

struct grec {		/* Information concerning a glyph */
	char value;
	short int nr, nc;
	GIMAGE ptr;
	struct grec * next;
};
typedef struct grec * GPTR;
GPTR database[129];

/* Prototypes - all in this file, not alphabetical */
void lines   (IMAGE im);
void dumpglyph (IMAGE im, int rs, int cs, int re, int ce, char gly);
GIMAGE newglyph (int nr, int nc);
void initialize();
void txtread (IMAGE im, int lineno, char *result);
float match (GIMAGE a, int nra, int nca, GIMAGE b,
		 int nrb, int ncb, int act, int *dc);
GIMAGE nextglyph (IMAGE im, struct lrec *thisline, int *thiscol, int *space,
		int *nr, int *nc);
void clean (IMAGE im);
void remove_region (IMAGE im, int rs, int cs, int re, int ce);
void mark (IMAGE im, int row, int col);
void mark_region (IMAGE im, int *rs, int *cs, int *re, int *ce,
	 int row, int col);
void nextblackpixel (IMAGE im, int rs, int re, int this, int *row, int *col);
void nextblackcol (IMAGE im, int *this, int rs, int re);
void nextwhitecol (IMAGE im, int *this, int rs, int re);
void loaddatabase ();
int split (GIMAGE x, int nr, int nc, int act, int dc);
int resize (GIMAGE x, int *nr, int *nc, int act, int dc);
int bestmatch (GIMAGE x, int nr, int nc, float *val, int *dc);
float nmi (GIMAGE object, GIMAGE template,  int nro, int nco, 
                         int nrt, int nct, int r, int c);
GIMAGE gcopy (GIMAGE x, int nr, int nc);
void extract_multiple (GIMAGE x, int nr, int nc, int this, int csold, 
		char *result, int *next, int dc);
void showmatch  (GPTR p, GIMAGE a, int nra, int nca, int dc);
void bkmark (GIMAGE x, int r, int c, int nr, int nc, int val);
void holes (GIMAGE x, int nr, int nc);
void gprint (GIMAGE x, int nr, int nc);
void segment (GIMAGE x, int nr, int nc, char *res, int *ind);

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
			if(im->data[i][j]>128) im->data[i][j] = 1;
			else im->data[i][j] = 0;
			if (i==j) printf ("%d  %d\n", i, im->data[i][j]);
		}


/* Get the database */
	initialize();
	loaddatabase ();

	printf ("Enter the name of the output data file: ");
	scanf ("%s", name);
	sprintf (fullpath, "C:\\AIPCV\\ch9\\%s", name);

	clean (im);

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
	  mask[i] = 0;
	}
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
	  {
		  if (i>100 && i<120) printf ("%d ", im->data[i][j]);
	      if (im->data[i][j] == OBJECT) m++;
	  }
		if (i>100 && i<120) printf ("\n");
	  hpro[i] = m;
//	  if (DEBUG)
//	    printf ("Row %d:  %d\n", i, m);
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

	if (DEBUG)
	  printf ("A Space is %d pixels.\n", Space);
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
	int multiple, deltac, jj;
	float nmival;

/* Start at the specified line - use the saved line properties as bounds */
	cs = LINE[lineno].cs; ce = LINE[lineno].ce;
	csold = cs;
	k = 0;
	multiple = 0;
	do
	{

/* Extract a connected region */
	  x = nextglyph (im, &(LINE[lineno]), &cs, &space, &nr, &nc);
	  if (x == 0) break;
	  holes (x, nr, nc);

/* Try to match it */
	  deltac = 0;
	  j = bestmatch (x, nr, nc, &nmival, &deltac);
	  if (j == '!')
	  {
		printf ("Almost done!\n\n");
	  }
	  printf ("Character seen was '%c' (%o)\n", j, j);
	  fflush (stdout);

/* NR,NC are the size of the glyph X. deltac is the horizontal displacement
   observed for the best template match. 				    */

/* Was is more than one character? */
/*
	  multiple = split (x, nr, nc, j, deltac);
	  if (multiple && nmival > .45)
	  {
	    extract_multiple (x, nr, nc, j, cs-csold, result, &k, deltac);
	    for (jj=0; jj<128; jj++) mask[jj] = 0;
	  } else if (nc > Meanwidth &&nmival < 0.45)
	  {
		segment (x, nr, nc, result, &k);
	  }
	  else {
*/

/* A space or two? */
	      space = cs - csold - Widths[j];
	      while (space >= Space)
	      {
	        result[k++] = ' ';
	        space -= Space;
	      }

/* Two double quotes is actually only one. */
	    if (result[k] == '"' && j== (int)'"')
	      k--;
	    result[k++] = (char)j;

	    if (DEBUG || DB2)
	      printf ("Character seen was '%c'\n", j);

/*	  }	*/
	  csold = cs;
	  free(x);
	} while (cs < ce);

	result[k++] = '\0';
	printf ("String was \"%s\"\n", result);
}

/* Return the index of the best overall match in the database to X */
int bestmatch (GIMAGE x, int nr, int nc, float *val, int *deltac)
{
	int i,j,count, bc, w, wbest;
	GPTR p, psave;
	char result, act=' ';
	float k, bestk;

/*
	printf ("----------BESTMATCH ----------\n");
	printf ("NR=%d NC=%d  Delta = %d\n", nr, nc, *deltac);
	gprint (x, nr, nc);
	printf ("Actual? ");
	scanf ("%c", &act);
*/
//	while (act==' ' || act=='\n') scanf ("%c", &act);

	bestk = -1; result = '\0';
	for (i=0; i<127; i++)
	{
	  if (database[i] == 0 || mask[i]) continue;
	  p = database[i];
	  w = 0;
	  while (p)
	  {
/*
	    if (act == i) SHOW = 1;
	    else SHOW = 0;
*/
	    k = match (x, nr, nc, p->ptr, p->nr, p->nc, i, &bc);
	    if (k > bestk)
	    {
	        if (DEBUG || DB2)
		printf ("Previous best was %f now best is %f at '%c' template %d\n", 
			bestk, k, i, w);
		bestk = k;
		result = i;
		count = 1;
	        psave = p;
	        wbest = w;
	        *deltac = bc;
	    } else if (k == bestk)
		count++;
	    p = p->next;
	    w++;
	  }
	}
	if (DEBUG || DB2)
	printf   ("Overall best is '%c' at %f.\n", result, bestk);
	if (bestk < 0.5) printf ("The character '%c' is unlikely.\n",result);
	*val = bestk;
	if (DB2) printf ("----------BESTMATCH ENDS ----------\n");
	return result;
}

/* Return the best match value found between the two given glyphs */
float match (GIMAGE a, int nra, int nca, GIMAGE b,
		 int nrb, int ncb, int act, int *deltac)
{
	int i,j,dr, dc, ii, jj, nr, nc, br, bc;
	GIMAGE x, y;
	float best, tot;

/* First parameter is the extracted glyph */
/* Second is the database template 
	dr = abs (nra-nrb) + 1;
	dc = abs (nca-ncb) + 1;
	if (dc > Widths[act]/2) dc = Widths[act]/2;
*/
	dr = dc = 4;
	bc = 0;
	best = 0;
	for (i= -1; i<=dr; i++)
	  for (j= -1; j<=dc; j++)
	  {
	    if (SHOW) printf ("Offsets are %d %d:\n", i, j);
	    tot = nmi(a, b, nra, nca, nrb, ncb, i, j);
	    if (best < tot)
	    {
	      best = tot;
	      br = i; bc = j;
	    }
	  }
	*deltac = bc;
	return best;
}

/* Extract the next glyph on the given line. Return a GIMAGE */
GIMAGE nextglyph (IMAGE im, struct lrec *thisline, int *thiscol, int *space,
		int *nr, int *nc)
{
	int i,j,k, rs, re, cs, ce, first, last;
	int rrs, rre, rcs, rce, scol, row, col;
	static int oldscol = -1;
	GIMAGE x;

/*	printf ("------------ NEXTGLYPH -------------\n"); */
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

	if (DEBUG || DB2)
	  gprint (x, *nr, *nc);

	*thiscol = rce + 1;
	remove_region (im, rrs, rcs, rre, rce);
	return x;
}

/* Remove single pixel black regions */
void clean (IMAGE im)
{
	int i,j, ii, jj;

	for (i=0; i<im->info->nr; i++)
	  for (j=0; j<im->info->nc; j++)
	    if (im->data[i][j]) im->data[i][j] = 0;
		else im->data[i][j] = 1;

	for (i=1; i<im->info->nr-2; i++)
	  for (j=1; j<im->info->nc-2; j++)
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

	if (DEBUG || DB2) printf ("Marking ...\n");
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
	for (j=(cmax+cmin)/2 - 1; j<=(cmax+cmin)/2+1; j++)
	  for (i= *rs; i<= *re; i++)
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

}

/* Recursive flood fill */
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

/* Clear the marked pixels ifrom the image in the given region */
void remove_region (IMAGE im, int rs, int cs, int re, int ce)
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

/* Read the database from the file */
void loaddatabase ()
{
	FILE *f;
	int i,j,k, n,m, nr, nc, ival, items, v;
	GPTR p, q;
	GIMAGE x;
	char cval;

	f = fopen ("C:\\AIPCV\\ch9\\tr.db", "r");
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

	  x = newglyph (nr, nc);
	  q->ptr = x;
	  for (n=0; n<q->nr; n++)
	    for (m=0; m<q->nc; m++)
	    {
	      fscanf (f, "%d", &v);
	      x[n][m] = (unsigned char)v;
	    }

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
	  Widths[i] = 0;
	  p = database[i];
	  if (p == 0) continue;
	  if (DEBUG) printf ("Database: '%c'\n", i);
	  j = k = 0;
	  while (p)
	  {
	    j += p->nc;
	    k++;
	    p = p->next;
	  }
	  Widths[i] = (int)( (float)j/k + 0.5 );
	  if (Widths[i] > MaxWidth) MaxWidth = Widths[i];
	  if (DEBUG)
	    printf ("Width of %c is %d pixels\n", i, Widths[i]);
	}
}

/* See if the glyph image is a multiple */
int split (GIMAGE x, int nr, int nc, int act, int dc)
{
	int i,j,k, count=0;

	k = Widths[act] + dc;
	if (DEBUG) printf ("----- SPLIT : NR,NC = %d,%d W=%d dc=%d k = %d\n",
		nr, nc, Widths[act],dc,k);
	if (k >= nc+1) return 0;
	for (i=0; i<nr; i++)
	  for (j=k+1; j<nc; j++)
	    if (x[i][j] > 0) count++;
	i = (nc - (k+1) + 1) * nr;

	if (DEBUG) printf ("      Starting at %d\n", i);
	if (i<= 0) return 0;
	printf ("Split occupancy: %f\n", (float)count/(float)i);
	if ((float)count/(float)i >= SPLIT_THRESH) return 1;
	return 0;
}

/* Remove the first part of a multiple glyph */
int resize (GIMAGE x, int *nr, int *nc, int act, int dc)
{
	int i,j,jj,k, *vproj;

	if (DEBUG || DB2) printf (" ------------ RESIZE --------\n");
	vproj = (int *)malloc (sizeof(int)*(*nc));
	for (j=0; j<*nc; j++)
	{
	  k = 0;
	  for (i=0; i<*nr; i++)
	    if (x[i][j] > 0) k++;
	  vproj[j] = k;
	}

	k = Widths[act]/4;
	j = Widths[act]+dc; jj = j;
	printf ("Estimated split point is %d\n", j);
	for (i=j-k; i<j+k; i++)
	  if (vproj[i]<vproj[jj]) jj = i;
	free(vproj);
	j = jj;

/* Copy right part to the left */
	for (k=j+1; k<*nc; k++)
	  for (i=0; i<*nr; i++)
	    x[i][k-(j+1)] = x[i][k];
	*nc = *nc - (j+1);

/* Shrink vertically? */
	k = -1;
	for (i=0; i<*nr; i++)
	{
	  for (jj=0; jj<*nc; jj++)
	    if (x[i][jj] > 0)
	    {
	      k = i;
	      break;
	    }
	  if (k>=0) break;
	}

	if (k>3)
	{
	  for (i=k; i<*nr; i++)
	    for (jj=0; jj<*nc; jj++)
	      x[i-k][jj] = x[i][jj];
	  *nr = *nr - k + 1;
	}

	if (*nc > 1 && DEBUG)
		gprint (x, *nr, *nc);
	return j;
}

/* Compute the normalized match index */
float nmi (GIMAGE object, GIMAGE template,  int nro, int nco, 
                         int nrt, int nct, int r, int c)
{
	int i,j, count, n, nr, nc, po, pt, m;

	count = 0;      m = n = 0;
/*	if (nro >= nrt+nrt || nrt >= nro+nro) return 0.0; */
	if (nro > nrt) nr = nro; else nr = nrt;
	if (nco > nct) nc = nco; else nc = nct;
	for (i=0; i<nr; i++)
	{
	  for (j=0; j<nc; j++)
	  {
	    if ( (i+r>=0 && i+r<nro && j+c>=0 && j+c<nco) )
		po = object[i+r][j+c];
	    else po = -1;

	    if (i<nrt && j<nct)
		pt = template[i][j];
	    else pt = -1;

	    switch (po) {
case -1:
		switch (pt) {
	case OBJECT:	n++; m++; if (SHOW) printf ("-"); break;
	case HOLE:	n++; m++; if (SHOW) printf ("_"); break;
	case -1:	if (SHOW) printf ("X"); break;
	default:	if (SHOW) printf (" ");
		} 
		break;

case BACK:	switch(pt) {
	case OBJECT:	n++; m++; if (SHOW) printf ("-"); break;
	case HOLE:	n++; m++; if (SHOW) printf ("_"); break;
	case -1:	if (SHOW) printf ("X"); break;
	default:	if (SHOW) printf (" ");
		}
		break;

case HOLE:	switch (pt) {
	case OBJECT:	n++; m++; if (SHOW) printf ("-"); break;
	case HOLE:	n++; count++; if (SHOW) printf ("*"); break;
	case -1:
	case BACK:	n++; m++; if (SHOW) printf ("-"); break;
		}
		break;

case OBJECT:	switch (pt) {
	case OBJECT:	n++; count++; if (SHOW) printf ("+"); break;
	case HOLE:	n++; m++; if (SHOW) printf ("-"); break;
	case -1:
	case BACK:	n++; m++; if (SHOW) printf ("-"); break;
		}
		break;

default:	printf ("?");
	    }
	  }
	  if (SHOW) printf ("\n");
	}
	return (float)(count-m)/(float)(count+m);
 }

GIMAGE gcopy (GIMAGE x, int nr, int nc)
{
	int i,j;
	GIMAGE y;

	y = newglyph (nr, nc);
	for (i=0; i<nr; i++)
	  for (j=0; j<nc; j++)
		y[i][j] = x[i][j];
	return y;
}

void extract_multiple (GIMAGE x, int nr, int nc, int this, int csold, 
		char *result, int *next, int deltac)
{
	char lresult[128];
	int lk=0, lwid[128], lsp[128], lold, snr, snc, j, again=0;
	GIMAGE saved, xx;
	float val;
	int i, sp, k, dc;

/* Save a copy of the original */
	xx = 0;
	saved = gcopy (x, nr, nc);
	xx = gcopy (x, nr, nc);
	snr = nr; snc = nc;

/* First, deal with the character that has already been matched */
	lresult[lk] = this;
	lwid[lk] = Widths[this];
	lold = lsp[lk] = resize (xx, &nr, &nc, this, deltac);
	lk++;

/* Now repeatedly extract items until no more remain */
	do
	{
	  dc = 0;
	  j = bestmatch (xx, nr, nc, &val, &dc);
	  if (val > 0.5)
	  {
	    lresult[lk] = j;
	    lwid[lk] = Widths[j];
	    free(saved);
	    saved = gcopy (xx, nr, nc);
	    snr = nr; snc = nc;
	    lold = lsp[lk] = resize(xx, &nr, &nc, j, dc);
	    again = split (xx, nr, nc, j, dc);
	    if (nc <= 1) again = 0;
	    lk++;
	  } else {
	    if (lk == 0) return ;
	    lk--;
	    printf ("Retry: Forbidding '%c'\n", lresult[lk]);
	    mask[(int)(lresult[lk])] = 1;
	    nr = snr; nc = snc;
	    free(xx);
	    xx = gcopy (saved, snr, snc);
	    again = 1;
	  }
	} while (again);
	lresult[lk] = '\0';
	printf ("Multiple result is '%s'\n", lresult);

/* Spaces can occurs only at the beginning! (Else we would not have a multiple) */
	k = *next;
	sp = 0;
	for (i=0; i<lk; i++)
	  sp += lsp[i];
	while (csold-sp >= Space-1)
	{
	  sp += Space;
	  result[k++] = ' ';
	}
/* Copy the characters seen into the result */
	for (i=0; i<lk; i++)
	  result[k++] = lresult[i];
	*next = k;

/* Clean up */
	free(saved);
	if (xx) free(xx);
}


/* Display the ebst match */
void showmatch  (GPTR p, GIMAGE a, int nra, int nca, int dc)
{
	int i,j,dr, ii, jj, nr, nc, br;
	int ncb, nrb;
	GIMAGE x, y, b;
	float best, tot;

	return;
	b = p->ptr; nrb = p->nr; ncb = p->nc;
	if (nca < 2*ncb/3) return ;  /* Too small */

	if (nra <= nrb && nca <= ncb)
	{
	  x = a; dr = nrb-nra; dc = ncb-nca;
	  y = b; nr = nra; nc = nca;
	} else if (nrb <= nra && ncb <= nca)
	{
	  x = b; dr = nra-nrb; dc = nca-ncb;
	  y = a; nr = nrb; nc = ncb;
	} else return ;

	if (dr > 5) dr = 5;
	best = 0;
	for (i= -1; i<=dr; i++)
	    tot = nmi(a, b, nra, nc, nrb, ncb, i, dc);
	    if (best < tot)
	    {
	      best = tot;
	      br = dr;
	    }

	printf ("Best this time was %f\n", best);
	nmi (a, b, nra, nc, nrb, nc, br, dc);
}

void holes (GIMAGE x, int nr, int nc)
{
	int i,j,ii,jj;

/* Mark any background pixels connected to the bounding box with a 3 */
	for (i=0; i<nr; i++)
	{
	  if (x[i][0] == BACK) bkmark (x, i, 0, nr, nc, 3);
	  if (x[i][nc-1] == BACK) bkmark(x, i, nc-1, nr, nc, 3);
	}

	for (j=0; j<nc; j++)
	{
	  if (x[0][j] == BACK) bkmark (x, 0, j, nr, nc, 3);
	  if (x[nr-1][j] == BACK) bkmark (x, nr-1, j, nr, nc, 3);
	}

/* Any remaining background pixels are holes - mark them */
	for (i=0; i<nr; i++)
	  for (j=0; j<nc; j++)
	    if (x[i][j] == 3) x[i][j] = 0;
	    else if (x[i][j] == 0) x[i][j] = 3;

	if (DEBUG) gprint (x, nr, nc);
	if (DEBUG) printf ("Holes:\n");

}

void bkmark (GIMAGE x, int r, int c, int nr, int nc, int val)
{
	x[r][c] = val;
	if (r-1>=0 && x[r-1][c]==0) bkmark (x, r-1, c, nr, nc, val);
	if (r+1<nr && x[r+1][c]==0) bkmark (x, r+1, c, nr, nc, val);
	if (c-1>=0 && x[r][c-1]==0) bkmark (x, r, c-1, nr, nc, val);
	if (c+1<nc && x[r][c+1]==0) bkmark (x, r, c+1, nr, nc, val);
}

void gprint (GIMAGE x, int nr, int nc)
{
	int i,j;

	return;
	for (i=0; i<nr; i++)
	{
	  for (j=0; j<nc; j++)
		if (x[i][j] > 0) printf ("%1d", x[i][j]); else printf (" ");
	  printf ("\n");
	}
}

void segment (GIMAGE x, int nr, int nc, char *res, int *ind)
{
	int i,j,jj,k, *vproj, dc;
	GIMAGE y;
	char c1;
	float nmiv;

	vproj = (int *)malloc (sizeof(int)*(nc));
	for (j=0; j<nc; j++)
	{
	  k = 0;
	  for (i=0; i<nr; i++)
	    if (x[i][j] > 0) k++;
	  vproj[j] = k;
	}

	k = nc/Meanwidth;
	if (DEBUG) printf ("Segment: %d characters ...\n", k);
	j = Meanwidth - Meanwidth/4;
	jj = j;
	printf ("Estimated split point is %d\n", j);
	for (i=j; i<j+Meanwidth/2; i++)
	  if (vproj[i]<vproj[jj]) jj = i;
	free(vproj);
	j = jj;
	if (DEBUG) gprint (y, nr, j);

	y = newglyph (nr, j);
	for (k=0; k<j; k++)
	  for (i=0; i<nr; i++)
	    y[i][k] = x[i][k];
	c1 = bestmatch (y, nr, j, &nmiv, &dc);
	res[*ind] = c1;
	printf ("Found character '%c'\n", c1);
	*ind += 1;

/* Copy right part to the left */
	for (k=j+1; k<nc; k++)
	  for (i=0; i<nr; i++)
	    x[i][k-(j+1)] = x[i][k];
	nc = nc - (j+1);

/* Shrink vertically? */
	k = -1;
	for (i=0; i<nr; i++)
	{
	  for (jj=0; jj<nc; jj++)
	    if (x[i][jj] > 0)
	    {
	      k = i;
	      break;
	    }
	  if (k>=0) break;
	}

	if (k>3)
	{
	  for (i=k; i<nr; i++)
	    for (jj=0; jj<nc; jj++)
	      x[i-k][jj] = x[i][jj];
	  nr = nr - k + 1;
	}

	if (nc > 1)
		if (DEBUG) gprint (x, nr, nc);
	c1 = bestmatch (x, nr, nc, &nmiv, &dc);
	printf ("Found character '%c'\n", c1);
	res[*ind] = c1;
	*ind += 1;
}
