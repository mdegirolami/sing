#include "sing.h"

void vreceiver(const sing::vect<int> &v1, sing::vect<int> &v2)
{
    v2[0] = v1[0];
}

sing::dpvect<int> vv = { 0, 1, 2 ,3 };
sing::spvect<int, 100> v2;
sing::spvect<int, 100> v2b = { 0, 1, 2 ,3 };

void test_vectors(int size)
{
    sing::dpvect<int>       vv_local = { 0, 1, 2, 3 };
    sing::spvect<int, 100>  v3;
    sing::dpvect<int>       rtv(size);
    sing::dpvect<int>       rtv_inited(size, { 0, 1, 2, 3 });

    sing::ptr<sing::dpvect<int>> vdynalloc(new sing::wrapper<sing::dpvect<int>>);
    *vdynalloc = { 100, 50, 25 };
    v2 = *vdynalloc;

    //// all the construction methods
    sing::dpvect<int>   v_plain;
    sing::dpvect<int>   v_list = {10, 2, 65};
    sing::dpvect<int>   v_copy = vv_local;
    sing::dpvect<int>   v_plain_size(size);
    sing::dpvect<int>   v_list_size(size, { 0, 1, 2, 3 });
    sing::dpvect<int>   v_copy_size(size, vv_local);

    sing::dvect<sing::string>   vs_plain;
    sing::dvect<sing::string>   vs_list = { "a", "b", "c" };
    sing::dvect<sing::string>   vs_copy = vs_list;
    sing::dvect<sing::string>   vs_plain_size(size);
    sing::dvect<sing::string>   vs_list_size(size, { "d", "ecco" });
    sing::dvect<sing::string>   vs_copy_size(size, vs_list);

    sing::spvect<int, 100>   vst_plain;
    sing::spvect<int, 100>   vst_list = { 10, 2, 65 };
    sing::spvect<int, 100>   vst_copy = v_list;

    sing::svect<sing::string, 100>   vsts_plain;
    sing::svect<sing::string, 100>   vsts_list = {"ciccio", "b", "o", "m"};
    sing::svect<sing::string, 100>   vsts_copy = vs_list_size;

    // init and copy with same and different type to see if default constructor and copy operator are gone.

    // inits with same type
    sing::dpvect<int>               same1 = v_list;
    sing::dvect<sing::string>       same2 = vs_list;
    sing::spvect<int, 100>          same3 = vst_list;
    sing::svect<sing::string, 100>  same4 = vsts_list;

    // inits with different type
    sing::dpvect<int>               diff1 = vst_list;
    sing::dvect<sing::string>       diff2 = vsts_list;
    sing::spvect<int, 100>          diff3 = v_list;
    sing::svect<sing::string, 100>  diff4 = vs_list;

    // copy with same type
    same1 = v_list;
    same2 = vs_list;
    same3 = vst_list;
    same4 = vsts_list;

    // copy with different type
    diff1 = vst_list;
    diff2 = vsts_list;
    diff3 = v_list;
    diff4 = vs_list;

    // copies
    v_copy = vst_list;
    vs_copy = vsts_list;
    vst_list = v_copy;
    vsts_list = vs_copy;

    // using slices
    v2 = *vdynalloc;
    //sing::slice<int>(vst_plain, 2, 4) = sing::slice<int>(*vdynalloc, 0, 2);
    v2 = *vdynalloc;
    //sing::slice<int>(vst_plain, 1, 10) = *vdynalloc;
    v2 = *vdynalloc;

    // references
    vv[0] = 1;
    v2[0] = 1;
    v3[0] = 1;
    rtv[0] = 1;
    (*vdynalloc)[0] = 1;
    v2 = *vdynalloc;

    v_plain[0] = vst_list[0];
    vs_plain[0] = vsts_list[0];

    // open slices
    v2 = *vdynalloc;
    sing::oslice<int>(vst_plain, 3) = *vdynalloc;

    // parameter passing
    vreceiver(vv, *vdynalloc);
    vreceiver(v2, vv);
    vreceiver(rtv, v3);
    //vreceiver(sing::slice<int>(vst_plain, 2, 3), sing::slice<int>(*vdynalloc, 0, 2));

    const int pippo = 10;
    sing::svect<sing::string, 5>  xxxxx;
}