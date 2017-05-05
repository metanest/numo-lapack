#define args_t <%=name%>_args_t

typedef struct {
  enum CBLAS_ORDER order;
  enum CBLAS_TRANSPOSE trans;
  dtype alpha, beta;
  blasint m, n, lda;
} args_t;

#define func_p <%=cblas_func%>_p

static <%=cblas_func%>_t func_p = 0;

static void
<%=c_iter%>(na_loop_t *const lp)
{
    dtype *a;
    char *p1, *p2;
    ssize_t s1, s2;
    args_t *g;

    a = (dtype*)NDL_PTR(lp,0);
    INIT_PTR(lp,1,p1,s1);
    INIT_PTR(lp,2,p2,s2);
    g = (args_t*)(lp->opt_ptr);

  <% if /[cz]/ =~ blas_char %>
    (*func_p)(g->order, g->trans, g->m, g->n,
              &g->alpha, a, g->lda, (dtype*)p1, s1/sizeof(dtype),
              &g->beta, (dtype*)p2, s2/sizeof(dtype));
  <% else %>
    (*func_p)(g->order, g->trans, g->m, g->n,
              g->alpha, a, g->lda, (dtype*)p1, s1/sizeof(dtype),
              g->beta, (dtype*)p2, s2/sizeof(dtype));
  <% end %>

}

/*
*  Definition:
*  ===========
*
*       SUBROUTINE DGEMV(TRANS,M,N,ALPHA,A,LDA,X,INCX,BETA,Y,INCY)
*
*       .. Scalar Arguments ..
*       DOUBLE PRECISION ALPHA,BETA
*       INTEGER INCX,INCY,LDA,M,N
*       CHARACTER TRANS
*       ..
*       .. Array Arguments ..
*       DOUBLE PRECISION A(LDA,*),X(*),Y(*)
*       ..
*
*
*> \par Purpose:
*  =============
*>
*> \verbatim
*>
*> DGEMV  performs one of the matrix-vector operations
*>
*>    y := alpha*A*x + beta*y,   or   y := alpha*A**T*x + beta*y,
*>
*> where alpha and beta are scalars, x and y are vectors and A is an
*> m by n matrix.
*> \endverbatim
*/
/*
  @overload <%=name%>( alpha, a, x [,beta, y, trans, order] )
  @param [Numo::DFloat] a  m-by-n matrix (>=2-dimentional NArray)
  @param [Numo::DFloat] x  vector (>=1-dimentional NArray)
  @param [Numo::DFloat] y  vector [in/out] (>=1-dimentional NArray)
  @return [Numo::DFloat]
  @raise

*/
static VALUE
<%=c_func(-1)%>(int argc, VALUE *argv, VALUE mod)
{
    int       i;
    VALUE     ans, arg_y = Qnil;
    VALUE     a, x, y, alpha, beta, trans, order;
    narray_t *na1, *na2, *na3;
    blasint   ma, na, nx, tmp;
    size_t    shape[1];
    ndfunc_arg_in_t ain[4] = {{cT,2},{cT,1},{OVERWRITE,1},{sym_init,0}};
    ndfunc_arg_out_t aout[1] = {{cT,1,shape}};
    ndfunc_t ndf = {<%=c_iter%>, NO_LOOP, 3, 0, ain, aout};
    args_t g = {CblasRowMajor, CblasNoTrans, m_one, m_zero};

    check_func((void*)(&func_p),"<%=cblas_func%>");

    i = rb_scan_args(argc, argv, "34",
                     &alpha, &a, &x, &beta, &y, &trans, &order);
    switch (i) {
    case 7: g.order = option_order(order);
    case 6: g.trans = option_trans(trans);
    case 5: arg_y = y;
    case 4: if (RTEST(beta)) {g.beta = m_num_to_data(beta);}
    }
    if (RTEST(alpha)) {g.alpha = m_num_to_data(alpha);}

    GetNArray(a,na1);
    GetNArray(x,na2);
    CHECK_DIM_GE(na1,2);
    CHECK_DIM_GE(na2,1);

    ma = na1->shape[na1->ndim-2]; // m
    na = na1->shape[na1->ndim-1]; // n (lda)
    g.lda = na;

    if ( (g.order==CblasRowMajor && g.trans!=CblasNoTrans) ||
         (g.order!=CblasRowMajor && g.trans==CblasNoTrans) ) {
        swap(ma,na);
    }
    g.m = ma;

    nx = na2->shape[na2->ndim-1];
    CHECK_INT_EQ("na",na,"nx",nx);
    g.n = na;

    if (arg_y == Qnil) { // c is not given.
        ndf.nout = 1;
        ain[2] = ain[3];
        y = INT2FIX(0);
        shape[0] = g.m;
    } else {
        y = rb_funcall(cT,rb_intern("cast"),1,arg_y);
        if (!TEST_INPLACE(y)) {
            y = na_copy(y);
        }
        GetNArray(y,na3);
        CHECK_DIM_GE(na3,1);
        CHECK_SIZE_GE(na3,g.m);
    }

    ans = na_ndloop3(&ndf, &g, 3, a, x, y);

    if (arg_y == Qnil) { // c is not given.
        return ans;
    } else {
        return y;
    }
}

#undef func_p
#undef args_t
