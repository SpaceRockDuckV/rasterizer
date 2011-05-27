// compile: g++ llrasterize.cpp main.cpp tree.cpp  -lgdal -o sinlgeraster -O3
// output is sent to STDOUT
// run example: sinlgeraster /space/birds/Accipitridae/acci_bico_pl.shp 


#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <string>
#include <cassert>

#include "gdal.h"
#include "cpl_conv.h"
#include "ogr_api.h"
#include "ogr_srs_api.h"
#include "cpl_string.h"
#include "ogrsf_frmts.h"
#include "VFTree.h"
#include "rasterize.h"

using namespace std;

int rasterize_shp(VFTree& t,char *fn)
{
	OGRDataSource       *poDS = NULL;
	OGRSFDriver         *poDriver = NULL;
	poDS = OGRSFDriverRegistrar::Open(fn,TRUE, &poDriver);
	if(poDS==NULL)
	{
		  printf("poDS is NULL, skipping1......\n");
		  return (-1);	
	}
	OGRLayerH hLayer = OGR_DS_GetLayer( poDS,0 );
	if( hLayer == NULL )
	{
		  printf( "Unable to find layer 0, skipping2......\n");
		  return(-2);	
	}
	int num0=t.rasterizelayer(hLayer);
	if(num0==0)
	{
		  printf("zero features, skipping3......\n");
		  return(-3);
	}
	delete poDS;
	poDS=NULL;
	delete poDriver;
	return(0);
}

void quadgrid(sconfig& s)
{
	s.num_lev=16;
	s.org[0]=-180;
	s.org[1]=-90;
	for(int i=0;i<2;i++)
		s.scales[i]=new int[s.num_lev];
	for(int i=0;i<s.num_lev;i++)
	{
		for(int j=0;j<2;j++)
			s.scales[j][i]=2;
	}
	s.fscales[0]=360.0f/43200;
	s.fscales[1]=s.fscales[0];
}

void output_lnq(VFTree& t, char * id)
{
	t.textVFTree(id);
}


int main(int argc,char** argv)
{
	if(argc!=2)
	{
		printf("EXE IN_FILE_NAME \n");
		exit(-1);
	}
	sconfig s;
	quadgrid(s);
	RegisterOGRShape();
	VFTree t(s);
	char *in_fn=argv[1];
	int ret=-9999;
  int i=strlen(argv[1]);
	char * id=argv[1]+i-1;
	if(in_fn!=NULL)
		ret=rasterize_shp(t,in_fn);
	if(ret>=0) {
	  while (*id && *id!='/') {
	    if (*id=='.') *id=0;
	    id--;
	  }
	  output_lnq(t, id+1);
		
  }
	return ret;
}
