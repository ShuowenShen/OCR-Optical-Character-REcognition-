

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
#define HOLE 3

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
long seed = 132531;

/* The text that appears on the training file */
char *data[] = {
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ",
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ",
	"abcdefghijklmnopqrstuvwxyz",
	"abcdefghijklmnopqrstuvwxyz",
	"012345678901234567890123456789",
	"!@#$%^&*()_+-=|\\{",
	"}[]:\"~<>?;',./",
	"!@#$%^&*()_+-=|\\{",
	"}[]:\"~<>?;',./"
};

typedef unsigned char ** GIMAGE;

struct grec {		/* Information concerning a glyph */
	char value;
	short int nr, nc;
	GIMAGE ptr;
	struct grec * next;
};
typedef struct grec * GPTR;
GPTR database[256];

/* Prototypes - all in this file, not alphabetical */
void lines   (IMAGE im);
void extract (IMAGE im);
void dumpglyph (IMAGE im, int rs, int cs, int re, int ce, char gly);
void saveglyph (IMAGE im, int rs, int cs, int re, int ce, char gly, GPTR *x);
GIMAGE newglyph (int nr, int nc);
GPTR insert_to_database (GIMAGE x, int rs, int re, int cs, int ce, char gc);
int glyeq (GIMAGE x, int nr, int nc, char gc, GPTR p);
void dump_db (void);
void initialize(void);
GIMAGE nextglyph (IMAGE im, struct lrec *thisline, int *thiscol, int *space,
		int *nr, int *nc);
void savedatabase (char *filename);
void clean (IMAGE im);
void remove_gly(IMAGE im, int rs, int cs, int re, int ce);
void mark (IMAGE im, int row, int col);
void mark_region (IMAGE im, int *rs, int *cs, int *re, int *ce,
	 int row, int col);
void nextblackpixel (IMAGE im, int rs, int re, int this, int *row, int *col);
void nextblackcol (IMAGE im, int *this, int rs, int re);
void nextwhitecol (IMAGE im, int *this, int rs, int re);
void build_widths (int cs, int ce);
void find_A (IMAGE im, int *rrs, int *rcs, int *rre, int *rce);
void bkmark (GIMAGE x, int r, int c, int nr, int nc, int val);
void holes (GIMAGE x, int nr, int nc);
void gprint (GIMAGE x, int nr, int nc);
void loaddatabase ();

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
		exit(1);
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

	  printf ("This program examines a simple 'perfect' test image\n");
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
	loaddatabase ();

/* Find the lines of text and remember where they were */
	lines (im);

/* Now extract and recognize the glyphs */
	extract (im);

/* Save */
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
	  if (DEBUG)
	    printf ("Row %d:  %d\n", i, m);
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

	if (Space == 0)
	{
	  sp[0] = (LINE[1].cs - LINE[0].cs)/2;
	  sp[1] = (LINE[3].cs - LINE[2].cs)/2;
	  sp[2] = (LINE[6].cs - LINE[5].cs);
	  sp[3] = (LINE[8].cs - LINE[7].cs);
	  Space = (sp[0]+sp[1]+sp[2]+sp[3])/4;
	  if (DEBUG)
	  {
	    printf ("Spacing: %d %d %d %d\n", sp[0],sp[1], sp[2],sp[3]);
	    printf ("A Space is %d pixels.\n", Space);
	  }
	}
}

/* Extract the templates from the training image */
void extract (IMAGE im)
{
	int i,j, scol, qflag = 0, tspace=0, spacing;
	int thisline, thischar, rs,re,cs,ce, oldscol, nspace = 0;
	int rre, rrs, rce,rcs, row, col, actual, width;
	float mspace = 0;
	GPTR thisptr;

/* Use the first 10 lines (all characters) */
	for (thisline = 0; thisline <= 8; thisline++)
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
/*
	    mark_region (im, &rrs, &rcs, &rre, &rce, row, col);
	    if (rrs < 0 || rcs < 0) continue;
*/

/* No overlap - spaces between each character */
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

/* Is this the first letter? If so, extract carefully and measure widths */
	    if (thisline == 0 && thischar == 0)
	    {
	      if ( (float)(rre-rrs)/(float)(rce-rcs) > 1.5 ||
		   (float)(rce-rcs)/(float)(rre-rrs) > 1.5)
		find_A (im, &rrs, &rcs, &rre, &rce);
	      build_widths (rcs, rce);
	    }

/* Use predicted character widths to determine multiple glyphs */
	    actual = (int)(data[thisline][thischar]);
	    width  = rce-rcs+1;
	    printf ("Character '%c' (%o) Actual width %d predicted %d delta %d\n",
		actual, actual, width, Widths[actual], width-Widths[actual]);
	    if (Widths[actual] == 0)
		Widths[actual] = rce-rcs+1;

/* We now have a glyph - dump it */
/* EXCEPTION: A double quote has two vertically separated sets of black. */
	    if (DEBUG)
	      if (rrs>=0 && rcs>=0)
	       dumpglyph (im, rrs, rcs, rre, rce, data[thisline][thischar]);

/* Save the glyph as a template for its character class */
	   if (rrs>=0 && rcs>=0)
	   {
	    saveglyph (im, rrs, rcs, rre, rce,
		 data[thisline][thischar], &thisptr);
	    remove_gly (im, rrs, rcs, rre, rce);
	   }

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
		spacing -= Space;
		tspace += spacing;
		nspace++;
		oldscol = rcs;
	    }
	    col = rce++;
	  } while (rcs >= 0 && rce >= 0);

	}
	mspace = (float)tspace/(float)nspace;
	if (DEBUG)
		printf ("Average character width: %f\n", mspace);
	if (Meanwidth == 0)
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
	int i,j,k;
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
	    for (i=0; i<p->nr; i++)
	    {
	      for (j=0; j<p->nc; j++)
		if (p->ptr[i][j] == BACK) printf (" ");
		 else printf ("#");
	      printf ("\n");
	    }
	    p = p->next;
	  }
	  printf ("****************************************************\n");
	}
}

/* Find bounding box for the extracted glyph and put it into the database */
void saveglyph (IMAGE im, int rs, int cs, int re,
		 int ce, char gly, GPTR *entry)
{
	int i,j;
	int first, last;
	GIMAGE x;

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
	if (cs < 0) return;

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
	if (rs < 0) return;

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
	x = newglyph (re-rs+2, ce-cs+2);
	for (i=rs; i<=re; i++)
	  for (j=cs; j<=ce; j++)
	    x[i-rs][j-cs] = im->data[i][j]==2;

/* OPTIONAL: Holes */
	holes (x, re-rs+2,  ce-cs+2);

/* Fill the remainder of the glyph record */
	*entry = insert_to_database (x, rs, re, cs, ce, gly);
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
GPTR insert_to_database (GIMAGE x, int rs, int re, int cs, int ce, char gc)
{
	int k;
	GPTR gp, p;

	k = (int)gc;			/* ASCII value is database index */
	p = database[k];
	if (database[k] != 0)		/* Add to existing entry */
	{

/* Look for a duplicate */
	  do
	  {
	    if (glyeq(x,re-rs+1,ce-cs+1,gc, database[k]))
	    {
		printf ("EQUAL: no insertion.\n");
		free(x);
		return 0;
	    }
	    if (p->next) p = p->next;
	    else break;
	  } while (p);
	}

/* Create a new entry */
	gp = (GPTR) malloc (sizeof (struct grec));
	gp->nr = re-rs+1;
	gp->nc = ce-cs+1;
	gp->value= gc;
	gp->ptr = x;
	gp->next = (GPTR)0;

/* Add new entry to the end of the list for this character */
	if (p)
	  p->next= gp;
	else
	  database[k] = gp;
	return gp;
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
	int i,k, n,m;
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
			 p->nr, p->nc, Widths[k]);
	    for (n=0; n<p->nr; n++)
	    {
	      for (m=0; m<p->nc; m++)
	        fprintf (f, "%2d", p->ptr[n][m]);
	      fprintf (f, "\n");
	    }
	    p = p->next;
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
	for (j=(cmax+cmin)/2 - 2; j<=(cmax+cmin)/2+2; j++)
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

void remove_gly (IMAGE im, int rs, int cs, int re, int ce)
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

void holes (GIMAGE x, int nr, int nc)
{
	int i,j;

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

	gprint (x, nr, nc);
	printf ("Holes:\n");

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

	for (i=0; i<nr; i++)
	{
	  for (j=0; j<nc; j++)
		if (x[i][j] > 0) printf ("%1d", x[i][j]); else printf (" ");
	  printf ("\n");
	}
}

/* Read the database from the file */
void loaddatabase ()
{
	FILE *f;
	int i,j,k, n,m, nr, nc, ival, items, v;
	GPTR p, q;
	GIMAGE x;
	char cval;

	f = fopen ("tr.db", "r");
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
	  printf ("Database: '%c'\n", i);
	  j = k = 0;
	  while (p)
	  {
	    j += p->nc;
	    k++;
	    p = p->next;
	  }
	  Widths[i] = (int)( (float)j/k + 0.5 );
	  if (DEBUG)
	    printf ("Width of %c is %d pixels\n", i, Widths[i]);
	}
}

