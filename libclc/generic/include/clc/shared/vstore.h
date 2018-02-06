#define _CLC_VSTORE_DECL(SUFFIX, PRIM_TYPE, VEC_TYPE, WIDTH, ADDR_SPACE, RND) \
  _CLC_OVERLOAD _CLC_DECL void vstore##SUFFIX##WIDTH##RND(VEC_TYPE vec, size_t offset, ADDR_SPACE PRIM_TYPE *out);

#define _CLC_VECTOR_VSTORE_DECL(SUFFIX, MEM_TYPE, PRIM_TYPE, ADDR_SPACE, RND) \
  _CLC_VSTORE_DECL(SUFFIX, MEM_TYPE, PRIM_TYPE##2, 2, ADDR_SPACE, RND) \
  _CLC_VSTORE_DECL(SUFFIX, MEM_TYPE, PRIM_TYPE##3, 3, ADDR_SPACE, RND) \
  _CLC_VSTORE_DECL(SUFFIX, MEM_TYPE, PRIM_TYPE##4, 4, ADDR_SPACE, RND) \
  _CLC_VSTORE_DECL(SUFFIX, MEM_TYPE, PRIM_TYPE##8, 8, ADDR_SPACE, RND) \
  _CLC_VSTORE_DECL(SUFFIX, MEM_TYPE, PRIM_TYPE##16, 16, ADDR_SPACE, RND)

#define _CLC_VECTOR_VSTORE_PRIM3(SUFFIX, MEM_TYPE, PRIM_TYPE, RND) \
  _CLC_VECTOR_VSTORE_DECL(SUFFIX, MEM_TYPE, PRIM_TYPE, __private, RND) \
  _CLC_VECTOR_VSTORE_DECL(SUFFIX, MEM_TYPE, PRIM_TYPE, __local, RND) \
  _CLC_VECTOR_VSTORE_DECL(SUFFIX, MEM_TYPE, PRIM_TYPE, __global, RND)

#define _CLC_VECTOR_VSTORE_PRIM1(PRIM_TYPE) \
  _CLC_VECTOR_VSTORE_PRIM3(,PRIM_TYPE, PRIM_TYPE, )

#define _CLC_VECTOR_VSTORE_HALF_PRIM1(PRIM_TYPE, RND) \
	_CLC_VSTORE_DECL(_half, half, PRIM_TYPE, , __private, RND) \
	_CLC_VSTORE_DECL(_half, half, PRIM_TYPE, , __local, RND) \
	_CLC_VSTORE_DECL(_half, half, PRIM_TYPE, , __global, RND) \
	_CLC_VECTOR_VSTORE_PRIM3(_half, half, PRIM_TYPE, RND) \
	_CLC_VSTORE_DECL(a_half, half, PRIM_TYPE, , __private, RND) \
	_CLC_VSTORE_DECL(a_half, half, PRIM_TYPE, , __local, RND) \
	_CLC_VSTORE_DECL(a_half, half, PRIM_TYPE, , __global, RND) \
	_CLC_VECTOR_VSTORE_PRIM3(a_half, half, PRIM_TYPE, RND)

_CLC_VECTOR_VSTORE_PRIM1(char)
_CLC_VECTOR_VSTORE_PRIM1(uchar)
_CLC_VECTOR_VSTORE_PRIM1(short)
_CLC_VECTOR_VSTORE_PRIM1(ushort)
_CLC_VECTOR_VSTORE_PRIM1(int)
_CLC_VECTOR_VSTORE_PRIM1(uint)
_CLC_VECTOR_VSTORE_PRIM1(long)
_CLC_VECTOR_VSTORE_PRIM1(ulong)
_CLC_VECTOR_VSTORE_PRIM1(float)

_CLC_VECTOR_VSTORE_HALF_PRIM1(float,)
_CLC_VECTOR_VSTORE_HALF_PRIM1(float, _rtz)
_CLC_VECTOR_VSTORE_HALF_PRIM1(float, _rtn)
_CLC_VECTOR_VSTORE_HALF_PRIM1(float, _rtp)

#ifdef cl_khr_fp64
  _CLC_VECTOR_VSTORE_PRIM1(double)
  _CLC_VECTOR_VSTORE_HALF_PRIM1(double,)
  _CLC_VECTOR_VSTORE_HALF_PRIM1(double, _rtz)
  _CLC_VECTOR_VSTORE_HALF_PRIM1(double, _rtn)
  _CLC_VECTOR_VSTORE_HALF_PRIM1(double, _rtp)
#endif

#ifdef cl_khr_fp16
  _CLC_VECTOR_VSTORE_PRIM1(half)
#endif


#undef _CLC_VSTORE_DECL
#undef _CLC_VECTOR_VSTORE_DECL
#undef _CLC_VECTOR_VSTORE_PRIM3
#undef _CLC_VECTOR_VSTORE_PRIM1
