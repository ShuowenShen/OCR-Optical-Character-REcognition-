/* Read the sample page and build the database. This is version 2, which
   uses scanned input instead of 'perfect' input. Image must be bi-level 
   Creates a set of FEATURE VECTORS, measurements instead of templates */

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

/* The bounding box of a line of text */
struct lrec {
	int rs, re, cs, ce;
} LINE[400];
double WT[128] = {0.0000,0.0000,0.0000,0.0000,0.0000,0.0000,0.0000,0.0000,
     0.0000,0.0000,0.0000,0.0000,0.0000,0.0000,0.0000,0.0000,0.0000,0.0000,
     0.0000,0.0000,0.0000,0.0000,0.0000,0.0000,0.0000,0.0000,0.0000,0.0000,
     0.0000,0.0000,0.0000,0.0000,0.0000,0.0000,0.0000,0.0000,0.0000,0.0000,
     0.0000,0.0000,0.0000,0.0000,0.0000,0.0000,0.0000,0.0000,0.0000,0.0000,
     0.6522,0.3913,0.6522,0.5652,0.6522,0.6087,0.6087,0.6087,0.6087,0.6522,
     0.0000,0.0000,0.0000,0.0000,0.0000,0.0000,0.0000,1.0000,0.7826,0.8261,
     0.9565,0.7826,0.6957,0.9130,0.8696,0.3478,0.4783,0.9130,0.7826,1.1304,
     0.9565,0.9130,0.6957,0.9130,0.8696,0.6522,0.7826,0.9130,0.9130,1.2174,
     0.9130,0.9130,0.8261,0.0000,0.0000,0.0000,0.0000,0.0000,0.0000,0.6087,
     0.6522,0.5652,0.6522,0.5652,0.4783,0.6087,0.6522,0.3043,0.4348,0.6522,
     0.3043,1.0000,0.5652,0.6522,0.6087,0.6522,0.4348,0.4348,0.3043,0.6087,
     0.6087,0.9130,0.6087,0.6087,0.5217,0.0000,0.0000,0.0000,0.0000,0.0000};
int Widths[128];

int Nlines = -1;
int Space = 1;
int Meanwidth = 0;
long seed = 13751;

char *data[] = {
	"ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ", 
	"ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ",
	"ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ",
	"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz",
	"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz",
	"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz",
	"0123456789001234567890012345678901234567890",
	"0123456789001234567890012345678901234567890",
	"!@#$%^&*()_+-=|\\{}[]:\"~<>?;',./",
	"!@#$%^&*()_+-=|\\{}[]:\"~<>?;',./",
	"!@#$%^&*()_+-=|\\{}[]:\"~<>?;',./",
	"the quick brown fox jumps over the lazy dog.",
	"THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG.",
};


typedef unsigned char ** GIMAGE;

struct grec {		/* Information concerning a glyph */
	char value;
	short int nr, nc;
	int aspect, density;
	int circularity, rectangularity;
	int sample[9];
	int hrat, wrat;
	GIMAGE ptr;
	struct grec * next;
};
typedef struct grec * GPTR;
GPTR database[128];
int Nmax = 0, Mmax = 0;
float grid[128][128];

/* Prototypes - all in this file, not alphabetical */
int area (GIMAGE x, int val, int nr, int nc);
int bestmatch (GIMAGE x, int nr, int nc);
void box(GIMAGE x, int val, float *x1, float *y1, int nr, int nc);
void build_widths (int cs, int ce);
int C1 (int a, int p);
void clean (IMAGE im);
float distn (int *v1, int *v2, int n);
void dumpglyph (IMAGE im, int rs, int cs, int re, int ce, char gly);
void dump_db (void);
void extract (IMAGE im);
void find_A (IMAGE im, int *rrs, int *rcs, int *rre, int *rce);
int glyeq (GIMAGE x, int nr, int nc, char gc, GPTR p);
void initialize(void);
GPTR insert_to_database (GPTR x,  char gc);
void lines   (IMAGE im);
void makegrid (void);
void mark (IMAGE im, int row, int col);
int match (GIMAGE a, int nra, int nca, GIMAGE b, int nrb, int ncb);
void mark_region (IMAGE im, int *rs, int *cs, int *re, int *ce,
	 int row, int col);
GIMAGE newglyph (int nr, int nc);
GIMAGE nextglyph (IMAGE im, struct lrec *thisline, int *thiscol, int *space,
		int *nr, int *nc);
int nay4 (GIMAGE x, int I, int J, int val, int nr, int nc);
void nextblackcol (IMAGE im, int *this, int rs, int re);
void nextblackpixel (IMAGE im, int rs, int re, int this, int *row, int *col);
void nextwhitecol (IMAGE im, int *this, int rs, int re);
void orient  (IMAGE im);
int perimeter (GIMAGE x, int val, int nr, int nc);
int R1 (GIMAGE x, int nr, int nc, int b);
void remove_mark (IMAGE im, int rs, int cs, int re, int ce);
void savedatabase (char *filename);
void saveglyph (IMAGE im, int rs, int cs, int re, int ce, char gly, GPTR *x);
void separate (IMAGE im, int *rrs, int *rcs, int *rre, int *rce, int act);
void txtread (IMAGE im, int lineno, char *result);

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
	char inimage[128], fullpath[128], name[128];
	int i,j;

	  printf ("Usage: learn <image> <template file>\n");
	  printf ("This program examines a simple perfect test image\n");
	  printf ("and attempts to recognize the characters. A data\n");
	  printf ("file will be created with a database of templates\n");
	  printf ("for recognizing the characters.\n");

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
			if(im->data[i][j]>128) im->data[i][j] = 0;
			else im->data[i][j] = 1;

	printf ("Enter the name of the output data file: ");
	scanf ("%s", name);
	sprintf (fullpath, "C:\\AIPCV\\ch9\\%s", name);

	initialize();
	clean (im);

/* Determine the orientation - this image is supposed to be perfect */
	orient (im);

/* Find the lines of text and remember where they were */
	lines (im);

/* Now extract and recognize the glyphs */
	extract (im);

/* Save */
	dump_db();
	makegrid();
	savedatabase (fullpath);
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
	int *hpro, i,j,n,m, N;
	int lstart, lend, sp[4];

	N = im->info->nr;

/* Construct a horizontal projection and look for minima. */
	hpro = (int *) malloc (N * sizeof (int));
	for (i=0; i<N; i++)
	{
	  m = 0;
	  for (j=0; j<im->info->nc; j++)
	    if (im->data[i][j] == OBJECT) m++;
	  hpro[i] = m;
/*
	  if (DEBUG)
	    printf ("Row %d:  %d\n", i, m);
*/
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
	free (hpro);

	sp[0] = LINE[1].cs - LINE[0].cs;
	sp[1] = (LINE[2].cs - LINE[1].cs)/2;
	sp[2] = LINE[4].cs - LINE[3].cs;
	sp[3] = LINE[5].cs - LINE[4].cs;
	Space = (sp[0]+sp[1]+sp[2]+sp[3])/4;
	if (DEBUG)
	{
	  printf ("Spacing: %d %d %d %d\n", sp[0],sp[1], sp[2],sp[3]);
	  printf ("A Space is %d pixels.\n", Space);
	}
}

/* Extract the templates from the training image */
void extract (IMAGE im)
{
	int i,j,scol, qflag = 0, tspace=0, spacing;
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

/* Is this the first letter? If so, extract carefully and measure widths */
	    if (thisline == 0 && thischar == 0)
	    {
	      if ( (float)(rre-rrs)/(float)(rce-rcs) > 1.5 ||
		   (float)(rce-rcs)/(float)(rre-rrs) > 1.5)
		find_A (im, &rrs, &rcs, &rre, &rce);
	      build_widths (rcs, rce);
	    }

/* Use predicted character widths to determine multiple glyphs */
	  if (thisline < 8)
	  {
	    actual = (int)(data[thisline][thischar]);
	    width  = rce-rcs+1;
	    if (Widths[actual+1] == 0) wid2 = Widths[actual];
	      else wid2 = Widths[actual+1];
	    if (width > (Widths[actual]+wid2/2 + 2))
	      separate (im, &rrs, &rcs, &rre, &rce, actual);
	  } else {		/* Special characters */
	      actual = (int)(data[thisline][thischar]);
	      if (Widths[actual] == 0)
		Widths[actual] = rce-rcs+1;
	  }

/* We now have a glyph - dump it */
/* EXCEPTION: A double quote has two vertically separated sets of black. */
	    if (DEBUG)
	      dumpglyph (im, rrs, rcs, rre, rce, data[thisline][thischar]);

/* Measure the glyph and save the feature vector */
	    saveglyph (im, rrs, rcs, rre, rce,
		 data[thisline][thischar], &thisptr);
	    remove_mark (im, rrs, rcs, rre, rce);

	    if (data[thisline][thischar] == '"' && qflag == 0)
	    {
	      oldscol = -1;
	      qflag = 1;	/* The first of two black columns for a " */
	    }
	    else
	    {
	      thischar++;
	      if (qflag)
	      {
		qflag = 0;
		oldscol = -1;
	      }
	    }
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

/*	Print the entire database on standard output */
void dump_db ()
{
	int i,k;
	GPTR p;

	if (!DEBUG) return;
	for (k=0; k<128; k++)
	{
	  if (database[k] == (GPTR)0) continue;
	  p = database[k];
	  printf ("Character number %d (%o) '%c':\n", k, k, k);
	  while (p)
	  {
	    printf ("==== Rows: %d  Columns: %d   Value: '%c'\n",
		p->nr, p->nc, p->value);
	    printf ("Aspect: %3d Density: %3d Circ: %3d Rect %3d\n", 
		p->aspect, p->density, p->circularity, p->rectangularity);
	    p->hrat = (int) ( ((float)p->nr/(float)Mmax * 100) + 0.5 );
	    p->wrat = (int) ( ((float)p->nc/(float)Nmax * 100) + 0.5 );
	    printf ("Height ratio: %3d  Width ratio: %3d\n", 
		p->hrat, p->wrat);
	    for (i=0; i<9; i+=3)
	      printf ("%3d %3d %3d\n",
		p->sample[i], p->sample[i+1], p->sample[i+2]);

	    p = p->next;
	  }
	  printf ("****************************************************\n");
	}
}

/* Find bounding box for the extracted glyph and put it into the database */
void saveglyph (IMAGE im, int rs, int cs, int re,
		 int ce, char gly, GPTR *entry)
{
	int i,j,N, M, ii, jj, c[9], a, p;
	int first, last, n1, n2, m1, m2;
	GIMAGE x;
	GPTR this;
	float z;

/* Find the first black column */
	first = -1;
	for (j=cs; j<=ce; j++)
	{
	  for (i=rs; i<=re; i++)
	    if (im->data[i][j] == MARK)
	    {
		first = j;
		break;
	    }
	  if (first >= 0) break;
	}
	cs = first;

/* Find the last black column */
	last = -1;
	for (j=ce; j>=cs; j--)
	{
	  for (i=rs; i<=re; i++)
	    if (im->data[i][j] == MARK)
	    {
	      last = j;
	      break;
	    }
	  if (last >= 0) break;
	}
	ce = last;

/* First and last black rows */
	first = -1;
	for (i=rs; i<=re; i++)
	{
	  for (j=cs; j<=ce; j++)
	    if (im->data[i][j] == MARK)
	    {
		first = i;
		break;
	    }
	  if (first >= 0) break;
	}
	rs = first;

	last = -1;
	for (i=re; i>=rs; i--)
	{
	  for (j=cs; j<=ce; j++)
	    if (im->data[i][j] == MARK)
	    {
		last = i;
		break;
	    }
	  if (last >= 0) break;
	}
	re = last;

/* We now have a minimal bounding box. Save the glyph */
	N = re-rs+1;	M = ce-cs+1;
	x = newglyph (N, M);
	for (i=rs; i<=re; i++)
	  for (j=cs; j<=ce; j++)
	    x[i-rs][j-cs] = im->data[i][j]==2;

	this = (GPTR)malloc (sizeof(struct grec));
	this->ptr = x;
	this->nr = N; this->nc = M;
	if (N > Nmax) Nmax = N;
	if (M > Mmax) Mmax = M;
	this->value = gly;

/* Measure properties: 1. Aspect ratio */
	z = (float)(N)/(float)(M);
	this->aspect = (int)(z*10.0 + 0.5);
	if (this->aspect > 100) this->aspect = 100;
	
/* 2. Mean density */
	z = 0.0;
	for (i=0; i<N; i++)
	  for (j=0; j<M; j++)
	    if (x[i][j] > 0) z += 1.0;
	z = z/(float)(N*M);
	this->density = (int)(z * 100.0 + 0.5);
	if (this->density > 100) this->aspect = 100;

/* 3. Resampling */
	n1 = N/3;	n2 = n1+n1;
	m1 = M/3;	m2 = m1+m1;
	for (i=0; i<9; i++) { this->sample[i] = 0; c[i] = 0; }
	for (i=0; i<N; i++)
	{
	  if (i<=n1) ii = 0;
	  else if (i<n2) ii = 1;
	  else ii = 2;
	  for (j=0; j<M; j++)
	  {
	    if (j<=m1) jj = 0;
	    else if (j<m2) jj = 1;
	    else jj = 2;
	    if (x[i][j]>0) this->sample[ii*3+jj] += 1;
	    c[ii*3+jj] +=1;
	  }
	}
	for (i=0; i<9; i++)
	  if (c[i] > 0)
	    this->sample[i] = 
		 (int) (((float)this->sample[i]/(float)c[i] *100)+0.5);
	  else this->sample[i] = 0;

/* Circularity */
	a = area(x, 1, N, M);
	p = perimeter (x, 1, N, M);
	this->circularity = C1 (a, p);

/* Rectangularity */
	this->rectangularity = R1 (x, N, M, a);

	insert_to_database (this, gly);
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

/* Insert a new template into the database */
GPTR insert_to_database (GPTR x,  char gc)
{
	int k;
	GPTR p;

	k = (int)gc;			/* ASCII value is database index */
	p = database[k];
	if (database[k] != 0)		/* Add to existing entry */
	{

/* Look for a duplicate */
	  do
	  {
	    if (glyeq(x->ptr,x->nr,x->nc,gc, database[k]))
		return 0;
	    if (p->next) p = p->next;
	    else break;
	  } while (p);
	}

/* Create a new entry */
	x->next = (GPTR)0;

/* Add new entry to the end of the list for this character */
	if (p)
	  p->next= x;
	else
	  database[k] = x;
	return x;
}

/* Are the two glyphs the same? */
int glyeq (GIMAGE x, int nr, int nc, char gc, GPTR p)
{
	int i,j;
	
	if (p->nr != nr) return 0;
	if (p->nc != nc) return 0;
	if (p->value != gc) return 0;
	for (i=0; i<nr; i++)
	  for (j=0; j<nc; j++)
	    if (p->ptr[i][j] != x[i][j]) return 0;
	return 1;
}

/* Read the specified line of text from the image */
void txtread (IMAGE im, int lineno, char *result)
{
	int j,k, cs,ce, nr,nc, space, csold;
	GIMAGE x;

	cs = LINE[lineno].cs; ce = LINE[lineno].ce;
	csold = cs;
	k = 0;
	do
	{
	  x = nextglyph (im, &(LINE[lineno]), &cs, &space, &nr, &nc);
	  if (x == 0) break;

	  j = bestmatch (x, nr, nc);

/* A space or two? */
	  space = cs - csold - Meanwidth;
	  while (space >= Space)
	  {
	    result[k++] = ' ';
	    space -= Space;
	  }
	  csold = cs;

	  result[k++] = (char)j;
	  if (DEBUG)
	    printf ("Character seen was '%c'\n", j);
	  free(x);
	} while (cs < ce);

	result[k++] = '\0';
	printf ("String was \"%s\"\n", result);
}

/* Return the index of the best overall match in the database to X */
int bestmatch (GIMAGE x, int nr, int nc)
{
	int i,k, bestk, count;
	GPTR p;
	char result;

	bestk = -1; result = '\0';
	for (i=0; i<128; i++)
	{
	  if (database[i] == 0) continue;
	  p = database[i];
	  while (p)
	  {
	    k = match (x, nr, nc, p->ptr, p->nr, p->nc);
	    if (k > bestk)
	    {
	        if (DEBUG)
		printf ("Previous best was %d now best is %d at '%c'\n", 
			bestk, k, i);
		bestk = k;
		result = i;
		count = 1;
	    } else if (k == bestk)
	    {
		count++;
		if (DEBUG)
		printf ("Tie with '%c'\n", i);
	    }
	    p = p->next;
	  }
	}
	if (DEBUG)
	printf   ("Overall best is '%c' at %d.\n", result, bestk);
	return result;
}

/* Return the best match value found between the two given glyphs */
int match (GIMAGE a, int nra, int nca, GIMAGE b, int nrb, int ncb)
{
	int i,j,dr, dc, tot, best, ii, jj, nr, nc;
	GIMAGE x, y;

	if (nra <= nrb && nca <= ncb)
	{
	  x = a; dr = nrb-nra; dc = ncb-nca;
	  y = b; nr = nra; nc = nca;
	} else if (nrb <= nra && ncb <= nca)
	{
	  x = b; dr = nra-nrb; dc = nca-ncb;
	  y = a; nr = nrb; nc = ncb;
	} else return 0;

	best = 0;
	for (i=0; i<=dr; i++)
	  for (j=0; j<=dc; j++)
	  {
	    tot = 0;
	    for (ii=0; ii<nr; ii++)
	    {
	      for (jj=0; jj<nc; jj++)
		if (y[i+ii][j+jj] == OBJECT && x[ii][jj]==OBJECT)
		{
		  tot++;
	        }
		else if(y[i+ii][j+jj] == OBJECT || x[ii][jj]==OBJECT)
		{ 
		  tot--; 
		}
	    }
	    if (best < tot)
	      best = tot;
	  }
	return best;
}

GIMAGE nextglyph (IMAGE im, struct lrec *thisline, int *thiscol, int *space,
		int *nr, int *nc)
{
	int i,j,rs, re, cs, ce;
	int ecol, scol;
	GIMAGE x;

	*space = -1;
	*nr = *nc = 0;
	rs = thisline->rs; re = thisline->re;
	cs = thisline->cs; ce = thisline->ce;
	scol = *thiscol;
	nextblackcol (im, &scol, rs, re);
	if (scol < 0)
	  return 0;
	if (scol >= ce) 
	  return 0;
	*space = 0;

	ecol = scol;
	nextwhitecol (im, &ecol, rs, re);
	if (ecol < 0) ecol = ce;
	else ecol--;

/* First and last black rows 
	first = -1;
	for (i=rs; i<=re; i++)
	{
	  for (j=scol; j<=ecol; j++)
	    if (im->data[i][j] == OBJECT)
	    {
	        first = i;
	        break;
	    }
	  if (first >= 0) break;
	}
	rs = first;
 
	last = -1;
	for (i=re; i>=rs; i--)
	{
	  for (j=scol; j<=ecol; j++)
	    if (im->data[i][j] == OBJECT)
	    {
	        last = i;
	        break;
	    }
	  if (last >= 0) break;
	}
	re = last;
*/
 
/* We now have a minimal bounding box. Save the glyph */
	*nr = re-rs+1;	*nc =  ecol-scol+1;
	x = newglyph (re-rs+1, ecol-scol+1);
	for (i=rs; i<=re; i++)
	  for (j=scol; j<=ecol; j++)
	    x[i-rs][j-scol] = im->data[i][j];

	if (DEBUG)
         for (i=0; i<*nr; i++)
         {
           for (j=0; j<*nc; j++)
             if (x[i][j] == OBJECT) printf ("#");
               else printf (" ");
             printf ("\n");
         }

	*thiscol = ecol + 1;
	return x;
}

void savedatabase (char *filename)
{
	FILE *f;
	int i,j,k;
	GPTR p;

	f = fopen (filename, "w");
	if (f == NULL)
	{
	  printf ("Cannot save database in file '%s'\n", filename);
	  return;
	}

	fprintf (f, "%d %d\n", Space, Meanwidth);
	for (i=0; i<128; i++)
	{
	  if (database[i] == 0) continue;
	  p = database[i];
	  k = 0;
	  while (p)
	  {
	    fprintf (f, "%c %d %d %d %d\n", p->value, k,
			 p->nr, p->nc, Widths[i]);
	    fprintf (f, "%3d %3d %3d %3d %3d %3d\n",
		p->aspect, p->density, p->hrat, p->wrat, 
		p->circularity, p->rectangularity);
	    for (j=0; j<9; j++)
		fprintf (f, " %3d", p->sample[j]);
	    fprintf (f, "\n");

	    p = p->next;
	    k++;
	  }
	}
	fclose (f);
}

void clean (IMAGE im)
{
	int i,j;

	for (i=0; i<im->info->nr; i++)
	  for (j=0; j<im->info->nc; j++)
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
	int i,j,rmax,rmin,cmax,cmin, delta, flag = 0;

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
	for (j=(cmax+cmin)/2 - 1; j<=(cmax+cmin)/2+1; j++)
	  for (i= *rs; i<=rmin; i++)
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

/* Build a table of predicted character widths using the width of an 'A' */
void build_widths (int cs, int ce)
{
	int i;
	float x;

	x = (float)(ce - cs + 1);
	for (i=0; i<128; i++)
	  Widths[i] = (int)(WT[i] * x + 0.5);
}

void find_A (IMAGE im, int *rrs, int *rcs, int *rre, int *rce)
{
	int *vproj, i, j, k;

	if (DEBUG) printf ("Finding an 'A' character to calibrate:\n");
	vproj = (int *)malloc (sizeof(int)*(*rce-*rcs+1));
	for (j= *rcs; j<= *rce; j++)
	{
	  k = 0;
	  for (i= *rrs; i<= *rre; i++)
	    if (im->data[i][j] == MARK) k++;
	  vproj[j- *rcs] = k;
	}

	j = (*rre - *rrs + 1);
	k = j/4;
	for (i=j-k; i<j+k; i++)
	  if (vproj[i]<vproj[j]) j = i;
	j += *rcs;

	for (i= *rrs; i<= *rre; i++)
	  for (k = j; k<= *rce; k++)
	    if (im->data[i][k] == MARK) im->data[i][k] = 1;
	*rce = j-1;
	free(vproj);
}

void separate (IMAGE im, int *rrs, int *rcs, int *rre, int *rce, int act)
{
	int *vproj, i, j, k;

	if (DEBUG) printf ("Splitting a joined glyph\n");
	vproj = (int *)malloc (sizeof(int)*(*rce-*rcs+1));
	for (j= *rcs; j<= *rce; j++)
	{
	  k = 0;
	  for (i= *rrs; i<= *rre; i++)
	    if (im->data[i][j] == MARK) k++;
	  vproj[j- *rcs] = k;
	}

	k = (Widths[act]+Widths[act+1])/8;
	j = Widths[act];
	for (i=Widths[act]-k; i<Widths[act]+k; i++)
	  if (vproj[i]<vproj[j]) j = i;
	j += *rcs;

	for (i= *rrs; i<= *rre; i++)
	  for (k = j; k<= *rce; k++)
	    if (im->data[i][k] == MARK) im->data[i][k] = 1;

	*rce = j;
	free(vproj);
}

int C1 (int a, int p)
{
	if (a <= 0.0) return 0;
	else return (int)( (p*p/(3.1415926535*4.0*a)) * 10 + 0.5);
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

/* Array X has row coordinates, Y has columns. Order is:
	x1[0],y1[0] : Upper left (min,min)
	x1[1],y1[1] : Lower left (max,min)
	x1[2],y1[2] : Lower right (max,max)
	x1[3],y1[3] : Upper right (min,max)                */

	y1[0] = (float) jp1;    x1[0] = (float) ip1;
	y1[1] = (float) jp1;    x1[1] = (float) ip2;
	y1[2] = (float) jp2;    x1[2] = (float) ip2;
	y1[3] = (float) jp2;    x1[3] = (float) ip1;
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

float distn (int *v1, int *v2, int n)
{
	int i,sum=0;

	for (i=0; i<n; i++)
	  sum += (v1[i]-v2[i])*(v1[i]-v2[i]);
	return (float)sqrt((double)sum);
}

void makegrid ()
{
	int i,j, v1[32], v2[32], k=0, min, mini;

	for (i=0; i<127; i++)
	  if (database[i])
	  {
	    v1[0] = database[i]->aspect;
	    v1[1] = database[i]->density;
	    v1[2] = database[i]->hrat;
	    v1[3] = database[i]->wrat;
	    v1[4] = database[i]->sample[0];
	    v1[5] = database[i]->sample[1];
	    v1[6] = database[i]->sample[2];
	    v1[7] = database[i]->sample[3];
	    v1[8] = database[i]->sample[4];
	    v1[9] = database[i]->sample[5];
	    v1[10] = database[i]->sample[6];
	    v1[11] = database[i]->sample[7];
	    v1[12] = database[i]->sample[8];
	    printf ("Row %d:\n", i); k = 0;
	    min = 10000; mini = 0;
	    for (j=0; j<127; j++)
	      if (database[j])
	      {
		v2[0] = database[j]->aspect;
		v2[1] = database[j]->density;
		v2[2] = database[j]->hrat;
		v2[3] = database[j]->wrat;
		v2[4] = database[j]->sample[0];
		v2[5] = database[j]->sample[1];
		v2[6] = database[j]->sample[2];
		v2[7] = database[j]->sample[3];
		v2[8] = database[j]->sample[4];
		v2[9] = database[j]->sample[5];
		v2[10] = database[j]->sample[6];
		v2[11] = database[j]->sample[7];
		v2[12] = database[j]->sample[8];
		grid[i][j] = distn(v1, v2, 13);
		printf ("%10.4f ", grid[i][j]);
		k++;
		if (k>7) { k=0; printf ("\n"); }
		if (grid[i][j] < min && grid[i][j] > 0)
		{
			min = (int)grid[i][j]; mini = j;
		}
	      }
	    printf ("\n");
	    printf ("Non zero matchg is at '%c' with %d\n", mini, min);
	  }
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
	    	p += 1.207f;
	    	continue;
	    }

	    if (t == 0201 || t == 044 || t == 041 ||
	        t == 0204 || t ==0240 || t == 005) 
	    {
	    	p += 1.414f;
	    	continue;
	    }

	    if (t == 030 || t == 0102 || t == 80 ||
	        t == 10 || t == 18) 
	    {
	    	p += 1.0;
	    	continue;
	    }

	    p += 1.207f;
	  }
	}
	free (y);
	return (int)(p+0.5);
}

