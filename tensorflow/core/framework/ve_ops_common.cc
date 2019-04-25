#ifdef TENSORFLOW_USE_VE

#include "tensorflow/core/framework/ve_ops_common.h"
#include "tensorflow/core/common_runtime/ve/ve_device.h"
#include "tensorflow/core/common_runtime/dma_helper.h"

namespace tensorflow {

namespace {

struct _Tensor {
  int32_t dtype;
  uint64_t addr;
  int32_t dims;
  int64_t nelems;
  int64_t dim_size[1];

  static size_t size(int dims_) {
    return sizeof(_Tensor) + sizeof(int64_t) * (dims_ - 1);
  }

  Status init(const Tensor& t);
} __attribute__((__packed__));

Status _Tensor::init(const Tensor& t) {
  dtype = t.dtype();
  addr = (uint64_t)DMAHelper::base(&t);
  dims = t.dims();
  nelems = t.NumElements();
  for (int i = 0; i < dims; ++i) {
    dim_size[i] = t.dim_size(i);
  }
  return Status::OK();
}

} // namespace

template<typename T>
Status VEOpKernelHelper::Args::addArg(const T& v) {
  size_t size = sizeof(T) ;
  if (curr_ + sizeof(size_t) + size >= end_)
    return errors::Internal("buffer is too small");

  *reinterpret_cast<size_t*>(curr_) = size ;
  curr_ += sizeof(size_t) ;

  *reinterpret_cast<T*>(curr_) = v ;
  curr_ += size;

  ++pHeader_->nVariables;

  return Status::OK();
}

template<>
Status VEOpKernelHelper::Args::addArg<Tensor>(const Tensor& t) {

  size_t size = _Tensor::size(t.dims());
  if (curr_ + sizeof(size_t) + size >= end_)
    return errors::Internal("buffer is too small");

  *reinterpret_cast<size_t*>(curr_) = size ;
  curr_ += sizeof(size_t) ;

  _Tensor* p = reinterpret_cast<_Tensor*>(curr_);
  p->init(t);
  curr_ += size;

  ++pHeader_->nVariables;

  return Status::OK();
}


void VEOpKernelHelper::Call(OpKernelContext* context, 
                            const std::string& name, 
                            const Args& buf,
                            const OpKernel* op) {
  VEDeviceContext* vectx = context->op_device_context<VEDeviceContext>();
  Status s = vectx->Compute(name.c_str(), buf.buf(), buf.size(), op);
  if (!s.ok())
    context->SetStatus(s);
}

template Status VEOpKernelHelper::Args::addArg<bool>  (const bool&   v) ;
template Status VEOpKernelHelper::Args::addArg<uint64>(const uint64& v) ;
template Status VEOpKernelHelper::Args::addArg<int64> (const int64&  v) ;
template Status VEOpKernelHelper::Args::addArg<float> (const float&  v) ;

}; // namespace tensorflow

#endif // TENSORFLOW_USE_VE
