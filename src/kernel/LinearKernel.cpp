#include "lib/common.h"
#include "kernel/LinearKernel.h"
#include "features/Features.h"
#include "features/RealFeatures.h"

#include <assert.h>

CLinearKernel::CLinearKernel(bool rescale_) 
  : CKernel(),rescale(rescale_),scale(1.0)
{
}

CLinearKernel::~CLinearKernel() 
{
}
  
void CLinearKernel::init(CFeatures* f)
{
  if (rescale)
    init_rescale(f) ;
}

void CLinearKernel::init_rescale(CFeatures* f)
{
  fprintf(stderr,"CLinearKernel::init_rescale not implemented yet\n") ;
}

void CLinearKernel::cleanup()
{
}
  
bool CLinearKernel::check_features(CFeatures* f) 
{
  return (f->get_feature_type()==CFeatures::F_REAL);
}

REAL CLinearKernel::compute(CFeatures* a, long idx_a, CFeatures* b, long idx_b)
{
  long alen, blen;
  bool afree, bfree;

  //fprintf(stderr, "LinKernel.compute(%ld,%ld)\n", idx_a, idx_b) ;
  REAL* avec=((CRealFeatures*) a)->get_feature_vector(idx_a, alen, afree);
  REAL* bvec=((CRealFeatures*) b)->get_feature_vector(idx_b, blen, bfree);
  
  assert(alen==blen);
  fprintf(stderr, "LinKernel.compute(%ld,%ld) %d\n", idx_a, idx_b, alen) ;

  REAL result=ddot_(alen, avec, 1, bvec, 1) ;
  ((CRealFeatures*) a)->free_feature_vector(avec, afree);
  ((CRealFeatures*) b)->free_feature_vector(bvec, bfree);

  return result;
}

