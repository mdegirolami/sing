#include "str_test.h"

char NumberSelector::id__;

bool NumberSelector::isGood(int32_t cp) const
{
    return (sing::isDigit(cp));
}

bool str_test()
{
    std::vector<int32_t> xxxxx;

    xxxxx.at(5) = 0;

    // len
    const std::string s0 = "testϏ";     // upper key = 0x3cf

    if (sing::len(s0.c_str()) != 6) {
        return (false);
    }
    if (sing::numchars(s0.c_str()) != 5) {
        return (false);
    }

    const std::string s2 = "pppppp akdsjlaj pp fddjkj ppp dsahjppp";
    if (sing::count(s2.c_str(), "ppp") != 4 || sing::count(s2.c_str(), "PPP", true) != 4) {
        return (false);
    }

    // casing
    if (sing::toupper(s0.c_str()) != "TESTϏ") {
        return (false);
    }
    if (sing::tolower(s0.c_str()) != "testϗ") {
        return (false);
    }
    if (sing::totitle(s0.c_str()) != "TESTϏ") {
        return (false);
    }
    if (sing::compare(s0.c_str(), "Testϗ") <= 0 || sing::compare(s0.c_str(), "testϏ") != 0 || sing::compare(s0.c_str(), "testϗ") >= 0) {
        return (false);
    }
    if (sing::compare(s0.c_str(), "Testϗ", true) != 0 || sing::compare(s0.c_str(), "UestϏ", true) >= 0 || sing::compare(s0.c_str(), "tesSϗ", true) <= 0) {
        return (false);
    }
    int32_t last_pos = 0;
    if (sing::compareAt(s0.c_str(), 4, "Ϗl", &last_pos) >= 0) {
        return (false);
    }

    // splitting
    std::string s1 = "c:/inside/thedir:/thefile.k.ext";
    std::string drive;
    std::string part;
    std::string left;
    std::string extension;
    sing::split(s1.c_str(), ":/", &drive, &s1, sing::SplitMode::sm_drop);
    sing::splitAny(s1.c_str(), "\\/", &s1, &part, sing::SplitMode::sm_separator_left);
    sing::rsplit(part.c_str(), ".", &left, &extension, sing::SplitMode::sm_separator_right);
    sing::rsplitAny(left.c_str(), "T", &left, &part, sing::SplitMode::sm_drop, true);
    const bool res0 = sing::split(s1.c_str(), "X", &drive, &s1, sing::SplitMode::sm_drop);
    if (drive != "c" || s1 != "inside/" || extension != ".ext" || left != "thedir:/" || part != "hefile.k" || res0) {
        return (false);
    }

    // replacing
    s1 = "bla          bla       bla";
    const int32_t n0 = sing::replace(&s1, "bla", "alb", false, 5);
    if (s1 != "bla          alb       bla" || n0 != 16) {
        return (false);
    }
    sing::replace(&s1, "BLA", "longer", true);
    if (s1 != "longer          alb       bla" || sing::replace(&s1, "k2", "alb") != sing::npos) {
        return (false);
    }
    sing::replace(&s1, "alb", "s");
    if (s1 != "longer          s       bla") {
        return (false);
    }

    s1 = "ppppp ppp ppp";
    if (sing::replaceAll(&s1, "PPP", "aaa", true) != 3 || s1 != "aaapp aaa aaa") {
        return (false);
    }

    s1 = "ppppp ppp ppp";
    sing::replaceAll(&s1, "ppp", "p");
    if (s1 != "ppp p p") {
        return (false);
    }

    s1 = "ppppp ppp ppp";
    sing::replaceAll(&s1, "ppp", "longer");
    if (s1 != "longerpp longer longer") {
        return (false);
    }

    if (sing::replaceAll(&s1, "longeR", "longer") != 0) {
        return (false);
    }

    // prefix/suffix
    s1 = "pre str post";
    if (sing::hasPrefix(s1.c_str(), "ll") || sing::hasPrefix(s1.c_str(), "PRE") || sing::hasSuffix(s1.c_str(), "ll") || sing::hasSuffix(s1.c_str(), "POST")) {
        return (false);
    }
    if (!sing::hasPrefix(s1.c_str(), "pre") || !sing::hasPrefix(s1.c_str(), "PRE", true) || !sing::hasSuffix(s1.c_str(), "post") ||
        !sing::hasSuffix(s1.c_str(), "POST", true)) {
        return (false);
    }
    sing::cutPrefix(&s1, "pre");
    sing::cutSuffix(&s1, "post");
    if (s1 != " str ") {
        return (false);
    }

    // cleanup
    sing::cutLeadingSpaces(&s1);
    if (s1 != "str ") {
        return (false);
    }
    sing::cutTrailingSpaces(&s1);
    if (s1 != "str") {
        return (false);
    }
    s1 = "     \t    ";
    sing::cutLeadingSpaces(&s1);
    if (s1 != "") {
        return (false);
    }
    s1 = "     \t    ";
    sing::cutTrailingSpaces(&s1);
    if (s1 != "") {
        return (false);
    }
    s1 = "jhgfjgd123aaaa123kjhssfhg";
    NumberSelector selector;
    sing::cutLeading(&s1, selector);
    sing::cutTrailing(&s1, selector);
    if (s1 != "123aaaa123") {
        return (false);
    }
    sing::cutFun(&s1, selector);
    if (s1 != "123123") {
        return (false);
    }
    s1 = "ϏϏϏϏϏ";
    sing::erase(&s1, 1, 2);             // damage first character
    if (s1 == "ϏϏϏϏ" || sing::compare(s1.c_str(), "ϏϏϏϏ", true) != 0) {
        return (false);
    }
    sing::makeUtf8Compliant(&s1);
    if (s1 != "ϏϏϏϏ") {
        return (false);
    }

    // working with indices
    // We dont test most find routines - we keep for good split already used them 
    s1 = "aaaaa1aaaaa2aaaaa345";
    sing::Range r0;
    sing::findFnc(s1.c_str(), selector, &r0);
    if (r0.begin_ != 5 || r0.end_ != 6) {
        return (false);
    }
    sing::findFnc(s1.c_str(), selector, &r0, r0.end_);
    if (r0.begin_ != 11) {
        return (false);
    }
    sing::rfindFnc(s1.c_str(), selector, &r0);
    if (r0.begin_ != 19 || r0.end_ != 20) {
        return (false);
    }
    sing::rfindFnc(s1.c_str(), selector, &r0, r0.begin_);
    if (r0.begin_ != 18) {
        return (false);
    }
    sing::rfind(s1.c_str(), "a", &r0, false, 5);
    if (r0.begin_ != 4) {
        return (false);
    }
    sing::findAny(s1.c_str(), "A", &r0, true, 5);
    if (r0.begin_ != 6) {
        return (false);
    }

    // must fail
    const std::string s3 = "xxxxxxxxxxxxxxxxxxx";
    if (sing::find(s1.c_str(), "notpresent", &r0) || sing::findAny(s1.c_str(), "notpresent", &r0) || sing::rfind(s1.c_str(), "notpresent", &r0) ||
        sing::rfindAny(s1.c_str(), "notpresent", &r0) || sing::findFnc(s3.c_str(), selector, &r0) || sing::rfindFnc(s3.c_str(), selector, &r0)) {
        return (false);
    }

    // working with indices
    r0.begin_ = 18;
    r0.end_ = 20;
    if (sing::sub(s1.c_str(), 5, 17) != "1aaaaa2aaaaa" || sing::subRange(s1.c_str(), r0) != "45") {
        return (false);
    }
    s1 = "ϏϏϏϏϏ";
    if (sing::pos2idx(s1.c_str(), 2) != 4 || sing::pos2idx(s1.c_str(), 5) != 10 || sing::pos2idx(s1.c_str(), 6) != sing::npos) {
        return (false);
    }
    if (sing::idx2pos(s1.c_str(), 4) != 2 || sing::idx2pos(s1.c_str(), 10) != 5 || sing::idx2pos(s1.c_str(), 12) != sing::npos ||
        sing::idx2pos(s1.c_str(), 1) != sing::npos) {
        return (false);
    }
    sing::insert(&s1, 4, "xx");
    if (s1 != "ϏϏxxϏϏϏ") {
        return (false);
    }
    sing::erase(&s1, 4);
    if (s1 != "ϏϏ") {
        return (false);
    }
    s1 = "1234567890";
    r0.begin_ = 5;
    r0.end_ = 8;
    sing::eraseRange(&s1, r0);
    if (s1 != "1234590") {
        return (false);
    }
    s1 = "12345ϏϏϏϏϏ67890";
    if (sing::skipFwd(s1.c_str(), 5, 5) != 15 || sing::skipFwd(s1.c_str(), 5, 100) != 20) {
        return (false);
    }
    if (sing::skipBkw(s1.c_str(), 15, 6) != 4 || sing::skipBkw(s1.c_str(), sing::npos, 100) != 0) {
        return (false);
    }

    // unicode encode/decode
    std::vector<int32_t> cps;
    sing::decode(s1.c_str(), &cps);
    if (sing::encode(cps) != s1) {
        return (false);
    }
    int32_t at = 5;
    if (sing::decodeOne(s1.c_str(), &at) != 0x3CF || sing::encodeOne(0x3CF) != "Ϗ" || at != 7) {
        return (false);
    }
    cps.push_back(0x800);
    cps.push_back(0x10000);
    std::vector<int32_t> cps_verify;
    sing::decode(sing::encode(cps).c_str(), &cps_verify);
    if (cps != cps_verify) {
        return (false);
    }

    // char classify
    if (sing::isDigit(0x2f) || sing::isDigit(0x3a) || !sing::isDigit(0x30) || !sing::isDigit(0x39)) {
        return (false);
    }
    if (sing::isXdigit(0x2f) || sing::isXdigit(0x3a) || !sing::isXdigit(0x30) || !sing::isXdigit(0x39) || sing::isXdigit(0x40) || sing::isXdigit(0x47) ||
        !sing::isXdigit(0x41) || !sing::isXdigit(0x46) || sing::isXdigit(0x60) || sing::isXdigit(0x67) || !sing::isXdigit(0x61) || !sing::isXdigit(0x66)) {
        return (false);
    }
    if (sing::isLetter(0x20) || !sing::isLetter(0x41) || !sing::isLetter(0x61) || !sing::isLetter(0x3CF) || !sing::isLetter(sing::ccTolower(0x3CF))) {
        return (false);
    }
    if (sing::isUpper(0x20) || !sing::isUpper(0x41) || sing::isUpper(0x61) || !sing::isUpper(0x3CF) || sing::isUpper(sing::ccTolower(0x3CF))) {
        return (false);
    }
    if (sing::isLower(0x20) || sing::isLower(0x41) || !sing::isLower(0x61) || sing::isLower(0x3CF) || !sing::isLower(sing::ccTolower(0x3CF))) {
        return (false);
    }
    if (sing::ccTolower(0x41) != 0x61) {
        return (false);
    }

    // all done
    return (true);
}
