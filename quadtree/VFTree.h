#ifndef code_stattree_h
#define code_stattree_h

#define DIM		  2
#define NOT_INTERSECTS	0
#define CONTAINS	1
#define INTESECTS  2
#define REC_LEN	100
#define MAX_ID	10000

typedef struct vftree
{
	short     level;
	short     gindex[DIM];
	short	  xorg;
	short	  yorg;
	bool	  isfull;
	struct  vftree **children;
	struct  vftree *parent;
	std::set<short int> *rec;
	vftree()
	{
		level=-1;
		parent=NULL;
		children=NULL;
		rec=NULL;
		isfull=false;
	}
	vftree(int lev,int xind,int yind)
	{
		level=lev;
		gindex[0]=xind;
		gindex[1]=yind;
		children=NULL;
		rec=NULL;
		isfull=false;
	}
} vftree;

typedef struct sconfig
{
  int num_lev;
  double org[DIM];
  double fscales[DIM];
  int *scales[DIM];

  struct sconfig& operator=(const struct sconfig& src)
  {
	num_lev=src.num_lev;
	for(int i=0;i<DIM;i++)
	{
		org[i]=src.org[i];
		fscales[i]=src.fscales[i];
	}
	for(int k=0;k<DIM;k++)
	{
		scales[k]=new int[num_lev];
		for(int i=0;i<num_lev;i++)
			scales[k][i]=src.scales[k][i];
	}
	return *this;
  }
 ~sconfig()
 {
		for(int k=0;k<DIM;k++)
			delete scales[k];
 }
} sconfig;

class VFTree
{
public:
	VFTree(const struct sconfig&);
	~VFTree();
	int rasterizelayer(const OGRLayerH layer);
	void textVFTree();
private:
	struct vftree * lroot;	
	int		*num_hier[DIM];
	sconfig config;
	//template <class T> void delete_tree(T *& root);
	int gvBurnScanline(int nY, int nXStart, int nXEnd );
	int GDALdllImageFilledPolygon(int nPartCount, int *panPartSize, double *padfX, double *padfY);
	int handleSingleLine(struct vftree *croot,int xorg,int yorg,int lev,int y,int x1,int x2);
	void trimtree(vftree *& root);
	template <class T> std::string print_path(const T * croot);
	void text_vf_tree(const vftree *croot);
	int calcYOrg(const vftree *croot);
	void node2coord(const vftree * croot,int& nx1,int& ny1,int& nx2,int& ny2);
	void delete_vf_tree(vftree *&);
};

#endif

