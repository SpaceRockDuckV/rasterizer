#include <iostream>
#include <fstream>
#include <vector>
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
#include "ogr_api.h"
#include "VFTree.h"
#include "rasterize.h"

using namespace std;
static int cur_len=0;

VFTree::VFTree(const struct sconfig& c)
{
	config=c;
	lroot=NULL;
	for(int k=0;k<DIM;k++)
	{
		num_hier[k]=new int[config.num_lev+1];
		num_hier[k][config.num_lev]=1;
		for(int i=config.num_lev-1;i>=0;--i)
			num_hier[k][i]=num_hier[k][i+1]*config.scales[k][i];
	}
}

VFTree::~VFTree()
{
	if(lroot!=NULL)
		delete_vf_tree(lroot);

	for(int k=0;k<DIM;k++)
	{
		if(num_hier[k]!=NULL)
			delete num_hier[k] ;
	}
}

int VFTree::rasterizelayer(const OGRLayerH layer)
{
	lroot=new struct vftree;
	lroot->gindex[0]=0;
	lroot->gindex[1]=0;
	lroot->level=0;
	lroot->parent=NULL;
	lroot->rec=new set<short int>;

	int seq=0;
	OGR_L_ResetReading( layer );
	OGRFeatureH hFeat;
	int this_rings=0,this_points=0;
    while( (hFeat = OGR_L_GetNextFeature( layer )) != NULL )
    {
		OGRGeometry *poShape=(OGRGeometry *)OGR_F_GetGeometryRef( hFeat );
		if(poShape==NULL)
		{
			cerr<<"error:............shape is NULL"<<endl;
			continue;
		}
	    OGRwkbGeometryType eFlatType = wkbFlatten(poShape->getGeometryType());
		if( eFlatType == wkbPolygon )
		{
			OGRPolygon *poPolygon = (OGRPolygon *) poShape;
			this_rings+=(poPolygon->getNumInteriorRings()+1);
		}
		std::vector<double> aPointX;
		std::vector<double> aPointY;
		std::vector<int> aPartSize;
		GDALCollectRingsFromGeometry( poShape, aPointX, aPointY, aPartSize );
		vector<double>::iterator minx,maxx,miny,maxy;
		minx = min_element (aPointX.begin(), aPointX.end());
		maxx = max_element (aPointX.begin(), aPointX.end());
		miny = min_element (aPointY.begin(), aPointY.end());
		maxy = max_element (aPointY.begin(), aPointY.end());
		this_points=aPartSize.size();
		if(aPartSize.size()==0) continue;
		GDALdllImageFilledPolygon(aPartSize.size(), &(aPartSize[0]),&(aPointX[0]), &(aPointY[0]));
		seq++;
		OGR_F_Destroy( hFeat );
	}
	return seq;
}


int VFTree::gvBurnScanline(int nY, int nXStart, int nXEnd )
{
	if(nXStart<0||nXEnd>=num_hier[0][0])
	{
		if(nXStart<0) nXStart=0;
		if(nXEnd>=num_hier[0][0]) nXEnd=num_hier[0][0]-1;
	}
	if(nXStart>nXEnd)
		return 0;
	int num_cell=handleSingleLine(lroot,0,0,0,nY,nXStart,nXEnd);
	int num_real=nXEnd-nXStart+1;
	if(num_cell!=num_real)
	{
		cout<<"  "<<nY<<" "<<nXStart<<" "<<nXEnd<<"     "<<num_cell<<"  "<<num_real<<endl;
		int m=11;
	}
	return num_cell;
}

int VFTree::handleSingleLine(struct vftree *croot,int lev,int xorg,int yorg,int y,int x1,int x2)
{
	int num_cell=0;
	croot->level=lev;
	croot->xorg=xorg;
	croot->yorg=yorg;
	int num_children=config.scales[0][lev]*config.scales[1][lev];
	if(croot->children==NULL)
	{
		croot->children=new struct vftree *[num_children];
		for(int i=0;i<num_children;i++)
			croot->children[i]=NULL;
	}
	if(lev+1==config.num_lev)
	{
		//last level
		for(int i=x1;i<=x2;i++)
		{
			int ind=(i-xorg)*config.scales[1][lev]+(y-yorg);
			if(croot->children[ind]==NULL)
			{
				croot->children[ind]=new struct vftree;
				croot->children[ind]->level=lev+1;
				croot->children[ind]->gindex[0]=i-xorg;
				croot->children[ind]->gindex[1]=y-yorg;
				croot->children[ind]->parent=croot;
				croot->children[ind]->rec=new set<short int>;
			}
			croot->children[ind]->rec->insert(y);
		}
		return (x2-x1+1);
	}
	int xpos1=(x1-xorg)/num_hier[0][lev+1];
	int xpos2=(x2-xorg+1)/num_hier[0][lev+1];
	int ypos=(y-yorg)/num_hier[1][lev+1];
	int next_y_org=yorg+ypos*num_hier[1][lev+1];
	bool left_align=(x1-xorg)%num_hier[0][lev+1]==0;
	bool right_align=(x2-xorg+1)%num_hier[0][lev+1]==0;
	//boundary
	int start_ind=left_align?xpos1:xpos1+1;
	int end_ind=xpos2;
	//middle part
	for(int i=start_ind;i<end_ind;i++)
	{
		int ind=i*config.scales[1][lev]+ypos;
		if(croot->children[ind]==NULL)
		{
			croot->children[ind]=new struct vftree;
			croot->children[ind]->level=lev+1;
			croot->children[ind]->gindex[0]=i;
			croot->children[ind]->gindex[1]=ypos;
			croot->children[ind]->rec=new set<short int>;
			croot->children[ind]->parent=croot;
		}
		croot->children[ind]->rec->insert (y);
		//print_path(croot->children[ind]);
		num_cell+=num_hier[0][lev+1];
	}
	int next_x1=xorg+start_ind*num_hier[0][lev+1]-1;
	int next_x1_org=xorg+xpos1*num_hier[0][lev+1];
	int next_ind1=xpos1*config.scales[1][lev]+ypos;

	if(x2<=next_x1)
	{
		//drill down
		if(croot->children[next_ind1]==NULL)
		{
			croot->children[next_ind1]=new struct vftree(lev+1,xpos1,ypos);
			croot->children[next_ind1]->level=lev+1;
			croot->children[next_ind1]->gindex[0]=xpos1;
			croot->children[next_ind1]->gindex[1]=ypos;
			croot->children[next_ind1]->rec=new set<short int>;
			croot->children[next_ind1]->parent=croot;
		}
		num_cell+=handleSingleLine(croot->children[next_ind1],lev+1,next_x1_org,next_y_org,y,x1,x2);
	}
	else
	{
		if(!left_align)
		{
			//left part
			if(croot->children[next_ind1]==NULL)
			{
				croot->children[next_ind1]=new struct vftree;
				croot->children[next_ind1]->level=lev+1;
				croot->children[next_ind1]->gindex[0]=xpos1;
				croot->children[next_ind1]->gindex[1]=ypos;
				croot->children[next_ind1]->rec=new set<short int>;
				croot->children[next_ind1]->parent=croot;
			}
			num_cell+=handleSingleLine(croot->children[next_ind1],lev+1,next_x1_org,next_y_org,y,x1,next_x1);
		}
		int next_x2=xorg+xpos2*num_hier[0][lev+1];
		if(!right_align)
		{
			//right part
			int next_x2_org=xorg+xpos2*num_hier[0][lev+1];
			int next_ind2=xpos2*config.scales[1][lev]+ypos;
			if(croot->children[next_ind2]==NULL)
			{
				croot->children[next_ind2]=new struct vftree(lev+1,xpos2,ypos);
				croot->children[next_ind2]->level=lev+1;
				croot->children[next_ind2]->gindex[0]=xpos2;
				croot->children[next_ind2]->gindex[1]=ypos;
				croot->children[next_ind2]->rec=new set<short int>;
				croot->children[next_ind2]->parent=croot;
			}
			num_cell+=handleSingleLine(croot->children[next_ind2],lev+1,next_x2_org,next_y_org,y,next_x2,x2);
		}
	}
	return num_cell;
}

template <class T>
string VFTree::print_path(const T * croot)
{
	const T *temp=croot;
	vector<short int> vx,vy,vind;
	string ret="";
	while(temp->parent!=NULL)
	{
		vx.push_back(temp->gindex[0]);
		vy.push_back(temp->gindex[1]);
		int d=temp->gindex[0]*config.scales[1][temp->parent->level]+temp->gindex[1];
		vind.push_back(d);
		temp=temp->parent;
	}
	for(size_t i=0;i<vx.size();i++)
	{
		char ss[20];
		sprintf(ss,"%d.",vind[vind.size()-1-i]);
		ret+=ss;
	}
	return ret.substr(0,ret.size()-1);
}

void VFTree::delete_vf_tree(vftree *& croot)
{
	if(croot==NULL)
		return;
	if(croot->children==NULL)
	{
		if(croot->rec!=NULL)
		{
			delete croot->rec;
			croot->rec=NULL;
		}
		delete croot;
		croot=NULL;
		return;
	}
	int lev=croot->level;
	for(int i=0;i<config.scales[0][lev];i++)
		for(int j=0;j<config.scales[1][lev];j++)
		{
			int ind=i*config.scales[1][lev]+j;
			delete_vf_tree(croot->children[ind]);
		}
	delete [] croot->children;
	int num_children=config.scales[0][lev]*config.scales[1][lev];
	croot->children=NULL;
	if(croot->rec!=NULL)
	{
		delete croot->rec;
		croot->rec=NULL;
	}
	delete croot;
	croot=NULL;
}

void VFTree::textVFTree()
{
	text_vf_tree(lroot);
}

void VFTree::text_vf_tree(const struct vftree *croot)
{
	if(croot==NULL)
		return;
	if(croot->children==NULL)
	{
		cout<<print_path(croot)<<"\t";
		int x1=0,y1=0,x2=0,y2=0;
		node2coord(croot,x1,y1,x2,y2);
		cout<<x1<<" "<<y1<<" "<<x2<<" "<<y2<<string(" 1 ")<<croot->level<<endl;
		return;
	}
	int lev=croot->level;
	for(int i=0;i<config.scales[0][lev];i++)
		for(int j=0;j<config.scales[1][lev];j++)
		{
			int ind=i*config.scales[1][lev]+j;
			text_vf_tree(croot->children[ind]);
		}
}

void VFTree::node2coord(const vftree * croot,int& nx1,int& ny1,int& nx2,int& ny2)
{
	int num=0;
	const vftree * temp=croot;
	while(temp!=lroot)
	{
		assert(temp!=NULL);
		nx1+=temp->gindex[0]*num_hier[0][temp->level];
		ny1+=temp->gindex[1]*num_hier[1][temp->level];
		temp=temp->parent;
	}
	nx2=nx1+num_hier[0][croot->level];
	ny2=ny1+num_hier[1][croot->level];
}

