#include <vector>
#include <set>
#include <iostream>
#include <cassert>
#include "gdal_alg.h"
//#include "gdal_alg_priv.h"
#include "ogr_api.h"
#include "ogr_geometry.h"
#include "ogr_spatialref.h"
#include "VFTree.h"
using namespace std;

extern int llCompareInt(const void *a, const void *b)
{
	return (*(const int *)a) - (*(const int *)b);
}

/************************************************************************/
/*                       dllImageFilledPolygon()                        */
/*                                                                      */
/*      Perform scanline conversion of the passed multi-ring            */
/*      polygon.  Note the polygon does not need to be explicitly       */
/*      closed.  The scanline function will be called with              */
/*      horizontal scanline chunks which may not be entirely            */
/*      contained within the valid raster area (in the X                */
/*      direction).                                                     */
/*                                                                      */
/*      NEW: Nodes' coordinate are kept as double  in order             */
/*      to compute accurately the intersections with the lines          */
/*                                                                      */
/*      Two methods for determining the border pixels:                  */
/*                                                                      */
/*      1) method = 0                                                   */
/*         Inherits algorithm from version above but with several bugs  */
/*         fixed except for the cone facing down.                       */
/*         A pixel on which a line intersects a segment of a            */
/*         polygon will always be considered as inside the shape.       */
/*         Note that we only compute intersections with lines that      */
/*         passes through the middle of a pixel (line coord = 0.5,      */
/*         1.5, 2.5, etc.)                                              */
/*                                                                      */
/*      2) method = 1:                                                  */
/*         A pixel is considered inside a polygon if its center         */
/*         falls inside the polygon. This is somehow more robust unless */
/*         the nodes are placed in the center of the pixels in which    */
/*         case, due to numerical inaccuracies, it's hard to predict    */
/*         if the pixel will be considered inside or outside the shape. */
/************************************************************************/

/*
 * NOTE: This code was originally adapted from the gdImageFilledPolygon()
 * function in libgd.
 *
 * http://www.boutell.com/gd/
 *
 * It was later adapted for direct inclusion in GDAL and relicensed under
 * the GDAL MIT/X license (pulled from the OpenEV distribution).
 */

int VFTree::GDALdllImageFilledPolygon(int nPartCount, int *panPartSize,
                               double *padfX, double *padfY)
{
	int num_gen_cells=0;
/*************************************************************************
2nd Method (method=1):
=====================
No known bug
*************************************************************************/

    int i;
    int y;
    double dx1, dy1;
    double dx2, dy2;
    double dy;
    double intersect;
	int ind1, ind2;
    int ints, n, part;
    int *polyInts, polyAllocated;
    int horizontal_x1, horizontal_x2;

    if (!nPartCount) {
        return 0;
    }
    n = 0;
    for( part = 0; part < nPartCount; part++ )
        n += panPartSize[part];

	double dminy =1e+10;
    double dmaxy =-1e+10 ;
	double dminx=1e+10;
	double dmaxx=-1e+10;
    for (i=0; (i < n); i++) 
	{
        if (padfX[i] < dminx) {
            dminx = padfX[i];
        }
        if (padfX[i] > dmaxx) {
            dmaxx = padfX[i];
        }
		if (padfY[i] < dminy) {
            dminy = padfY[i];
        }
        if (padfY[i] > dmaxy) {
            dmaxy = padfY[i];
        }
    }
    polyInts = (int *) malloc(sizeof(int) * n);
    polyAllocated = n;
	
	int iminx=(int)((dminx-config.org[0])/config.fscales[0]);
	int imaxx=(int)((dmaxx-config.org[0])/config.fscales[0]);
	int iminy=(int)((dminy-config.org[1])/config.fscales[1])+1;
	int imaxy=(int)((dmaxy-config.org[1])/config.fscales[1])+1;

    /* Fix in 1.3: count a vertex only once */
    for (y=iminy; y <=imaxy; y++) {
        int	partoffset = 0;
        dy = y +0.5; /* center height of line*/
        part = 0;
        ints = 0;
        /*Initialize polyInts, otherwise it can sometimes causes a seg fault */
        for (i=0; (i < n); i++) 
		{
            polyInts[i] = -1;
        }
        for (i=0; (i < n); i++) 
		{
            if( i == partoffset + panPartSize[part] ) {
                partoffset += panPartSize[part];
                part++;
            }
            if( i == partoffset ) {
                ind1 = partoffset + panPartSize[part] - 1;
                ind2 = partoffset;
            } else 
			{
                ind1 = i-1;
                ind2 = i;
            }
            dy1 = (int)((padfY[ind1]-config.org[1])/config.fscales[1]);
            dy2 = (int)((padfY[ind2]-config.org[1])/config.fscales[1]);
            if( (dy1 < dy && dy2 < dy) || (dy1 > dy && dy2 > dy) )
                continue;
            if (dy1 < dy2) 
			{
                dx1 = (int)((padfX[ind1]-config.org[0])/config.fscales[0]);
                dx2 = (int)((padfX[ind2]-config.org[0])/config.fscales[0]);
            } 
			else if (dy1 > dy2) 
			{
                dy2 =(int)((padfY[ind1]-config.org[1])/config.fscales[1]);
                dy1 =(int)((padfY[ind2]-config.org[1])/config.fscales[1]);;
                dx2 =(int)((padfX[ind1]-config.org[0])/config.fscales[0]);
                dx1 =(int)((padfX[ind2]-config.org[0])/config.fscales[0]);
            } 
			else /* if (fabs(dy1-dy2)< 1.e-6) */
	    {

                /*AE: DO NOT skip bottom horizontal segments
			  -Fill them separately-
			  They are not taken into account twice.*/
			if (padfX[ind1] > padfX[ind2])
			{
				horizontal_x1 = (int) floor((padfY[ind2]-config.org[1])/config.fscales[1]+0.5);
				horizontal_x2 = (int) floor((padfX[ind1]-config.org[0])/config.fscales[0]+0.5);
				if  ( (horizontal_x1 >  imaxx) ||  (horizontal_x2 <= iminx) )
					continue;
				/*fill the horizontal segment (separately from the rest)*/
				num_gen_cells+=gvBurnScanline( y, horizontal_x1, horizontal_x2 - 1 );
				continue;
			}
			else /*skip top horizontal segments (they are already filled in the regular loop)*/
				continue;
	    }

        if(( dy < dy2 ) && (dy >= dy1))
        {
            intersect = (dy-dy1) * (dx2-dx1) / (dy2-dy1) + dx1;
			polyInts[ints++] = (int) floor(intersect+0.5);
	    }
	}

        /*
         * It would be more efficient to do this inline, to avoid
         * a function call for each comparison.
	 * NOTE - mloskot: make llCompareInt a functor and use std
	 * algorithm and it will be optimized and expanded
	 * automatically in compile-time, with modularity preserved.
         */
        qsort(polyInts, ints, sizeof(int), llCompareInt);
		//std::cout<<ints<<endl;
        for (i=0; (i < (ints)); i+=2) 
		{
            if( polyInts[i] <= imaxx && polyInts[i+1] > iminx )
            {
				num_gen_cells+=gvBurnScanline(y, polyInts[i], polyInts[i+1] - 1 );
		    }
        }
    }	
    free( polyInts );
	return num_gen_cells;
}

void GDALCollectRingsFromGeometry(
    OGRGeometry *poShape, 
    std::vector<double> &aPointX, std::vector<double> &aPointY, 
    std::vector<int> &aPartSize )

{
    if( poShape == NULL )
        return;

    OGRwkbGeometryType eFlatType = wkbFlatten(poShape->getGeometryType());
    int i;

    if ( eFlatType == wkbPoint )
    {
        OGRPoint    *poPoint = (OGRPoint *) poShape;
        int nNewCount = aPointX.size() + 1;

        aPointX.reserve( nNewCount );
        aPointY.reserve( nNewCount );
        aPointX.push_back( poPoint->getX());
        aPointY.push_back( poPoint->getY());
        aPartSize.push_back( 1 );
    }
    else if ( eFlatType == wkbLineString )
    {
        OGRLineString   *poLine = (OGRLineString *) poShape;
        int nCount = poLine->getNumPoints();
        int nNewCount = aPointX.size() + nCount;

        aPointX.reserve( nNewCount );
        aPointY.reserve( nNewCount );
        for ( i = nCount - 1; i >= 0; i-- )
        {
            aPointX.push_back( poLine->getX(i));
            aPointY.push_back( poLine->getY(i));
        }
        aPartSize.push_back( nCount );
    }
    else if ( EQUAL(poShape->getGeometryName(),"LINEARRING") )
    {
        OGRLinearRing *poRing = (OGRLinearRing *) poShape;
        int nCount = poRing->getNumPoints();
        int nNewCount = aPointX.size() + nCount;

        aPointX.reserve( nNewCount );
        aPointY.reserve( nNewCount );
        for ( i = nCount - 1; i >= 0; i-- )
        {
            aPointX.push_back( poRing->getX(i));
            aPointY.push_back( poRing->getY(i));
        }
        aPartSize.push_back( nCount );
    }
    else if( eFlatType == wkbPolygon )
    {
        OGRPolygon *poPolygon = (OGRPolygon *) poShape;
        
        GDALCollectRingsFromGeometry( poPolygon->getExteriorRing(), 
                                      aPointX, aPointY, aPartSize );

        for( i = 0; i < poPolygon->getNumInteriorRings(); i++ )
            GDALCollectRingsFromGeometry( poPolygon->getInteriorRing(i), 
                                          aPointX, aPointY, aPartSize );
    }
    
    else if( eFlatType == wkbMultiPoint
             || eFlatType == wkbMultiLineString
             || eFlatType == wkbMultiPolygon
             || eFlatType == wkbGeometryCollection )
    {
        OGRGeometryCollection *poGC = (OGRGeometryCollection *) poShape;

        for( i = 0; i < poGC->getNumGeometries(); i++ )
            GDALCollectRingsFromGeometry( poGC->getGeometryRef(i),
                                          aPointX, aPointY, aPartSize );
    }
    else
    {
        CPLDebug( "GDAL", "Rasterizer ignoring non-polygonal geometry." );
    }
}
