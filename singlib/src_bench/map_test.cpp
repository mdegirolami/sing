#include "sing.h"

void test_map(void)
{
    // constructors, copy operator, compare operator
    sing::map<std::string, int32_t> map1;
    sing::map<std::string, int32_t> map2 = { {"uno", 100}, {"due", 25}, {"tre", 150} };
    map1 =  { {"uno", 100}, {"due", 25} };
    sing::map<std::string, int32_t> map3 = map1;
    bool eq1 = map1 == map2;
    map1 = map2;
    bool eq2 = map1 == map2;

    // modifications
    map1.insert("added", 123);
    map1.erase("uno");
    map2.clear();
    map2.reserve(10);
    map2.insert("first", 1000);

    // queries
    int val = map1.get("due");
    val = map1.get_safe("doesn't exist", -1);
    val = map1.get_safe("due", -1);
    eq1 = map1.has("uno");
    eq2 = map1.has("due");
    val = map1.size();
    val = map1.capacity();
    val = map2.capacity();
    map2.shrink_to_fit();
    val = map2.capacity();
    eq1 = map1.isempty();
    map1.clear();
    eq2 = map1.isempty();
    map3.insert("uno", 101);    // overwrite
    for (int ii = 0; ii < map3.size(); ++ii) {
        const std::string *key = &map3.key_at(ii);
        int val = map3.value_at(ii);
        printf("\n%s = %d", key->c_str(), val);
    }
    map1 =  { {"uno", 100}, {"tre", 110}, {"added", 26}, {"first", 32}, {"due", 25} };
    map1.erase("added");    // deleted is not first of the list
    map1.erase("uno");      // moved is not first of the list
    val = map1.get_safe("uno", -1);
    val = map1.get_safe("due", -1);
    val = map1.get_safe("tre", -1);
    val = map1.get_safe("added", -1);
    val = map1.get_safe("first", -1);
}