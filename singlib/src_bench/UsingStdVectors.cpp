#include "sing.h"

void vstdreceiver(const sing::array<int, 100> &v1, sing::array<int, 100> *v2)
{
    (*v2)[0] = v1[0];
}

void vstddreceiver(const std::vector<int> &v1, std::vector<int> *v2)
{
    (*v2)[0] = v1[0];
}

static std::vector<int> vv = { 0, 1, 2 ,3 };
static sing::array<int, 100> v2;
static sing::array<int, 100> v2b = { 0, 1, 2 ,3 };

void test_std_vectors(int size)
{
    std::vector<int>        vv_local = { 0, 1, 2, 3 };
    sing::array<int, 100>    v3 = {0};
    std::vector<int>        rtv(size);

    std::shared_ptr<std::vector<int>> vdynalloc = std::make_shared<std::vector<int>>();
    *vdynalloc = { 100, 50, 25 };
    v2 = v3;

    //// all the construction methods
    std::vector<int>   v_plain;
    std::vector<int>   v_list = {10, 2, 65};
    std::vector<int>   v_copy = vv_local;
    std::vector<int>   v_plain_size(size);

    std::vector<std::string>   vs_plain;
    std::vector<std::string>   vs_list = { "a", "b", "c" };
    std::vector<std::string>   vs_copy = vs_list;
    std::vector<std::string>   vs_plain_size(size);

    sing::array<int, 100>   vst_plain;
    sing::array<int, 100>   vst_list = { 10, 2, 65 };
    sing::array<int, 100>   vst_copy = vst_list;

    sing::array<std::string, 100>   vsts_plain;
    sing::array<std::string, 100>   vsts_list = {"ciccio", "b", "o", "m"};
    sing::array<std::string, 100>   vsts_copy;

    // init and copy with same and different type to see if default constructor and copy operator are gone.

    // inits with same type
    std::vector<int>              same1 = v_list;
    std::vector<std::string>      same2 = vs_list;
    sing::array<int, 100>          same3 = vst_list;
    sing::array<std::string, 100>  same4 = vsts_list;

    // inits with different type
    std::vector<int>              diff1 = vst_list;     // static to dyna
    std::vector<std::string>      diff2 = vsts_list;
    //sing::array<int, 100>          diff3 = v_list;       // dyna to static
    //sing::array<std::string, 100>  diff4 = vs_list;  

    // copy with same type
    same1 = v_list;
    same2 = vs_list;
    same3 = vst_list;
    same4 = vsts_list;

    // copy with different type
    sing::copy_array_to_vec(diff1, vst_list);       // static to dyna
    sing::copy_array_to_vec(diff2, vsts_list);
    //diff3 = v_list;         // dyna to static
    //diff4 = vs_list;

    // references
    vv[0] = 1;
    v2[0] = 1;
    v3[0] = 1;
    rtv[0] = 1;
    (*vdynalloc)[0] = 1;

    v_plain.push_back(0);
    vs_plain.push_back("");
    v_plain[0] = vst_list[0];
    vs_plain[0] = vsts_list[0];

    // parameter passing
    vstddreceiver(vv, &*vdynalloc);
    vstdreceiver(v2, &same3);

    bool isequal = vv_local == rtv;
    vv_local = rtv;
    isequal = vv_local == rtv;

    isequal = v3 == vst_plain;
    v3 = vst_plain;
    isequal = v3 == vst_plain;

    isequal = sing::iseq(v3, vv);
    sing::copy_array_to_vec(vv, v3);
    isequal = sing::iseq(vv, v3);

    sing::array<int, 3> small;
    sing::copy_array_to_vec(vv, small);

    sing::insert(v_plain, 3, 10, 33);
    sing::erase(v_plain, 2, 13);
    sing::insert_v(v_plain, 1, v_list);
    sing::append(v_plain, v_list);

    sing::array<sing::array<int, 3>, 3> aarrvec = {{1, 2, 3}, {4, 5}, {6, 7}};
    sing::array<std::vector<int>, 3> barrvec = {{1, 2, 3}, {4, 5}, {6, 7}};
    std::vector<sing::array<int, 3>> carrvec = {{1, 2, 3}, {4, 5}, {6, 7}};

    
    int kk;
    for (auto pippo = v_plain.begin(); pippo < v_plain.end(); ++pippo) {
        kk = *pippo;
    }
}