#include "sio_test.h"
#include "sio.h"

static bool test_File();
static bool test_dirfileop();
static bool test_paths();
static bool test_pipes();
static bool test_format();
static bool test_parse();
static bool test_console();
static bool test_unicode();

bool sio_test()
{
    // fix the starting conditions
    sing::fileRemove("testfile1.txt");
    sing::fileRemove("犬.txt");
    sing::fileRemove("ネコ.txt");
    sing::dirRemove("dir1", true);

    if (!test_File()) {
        return (false);
    }
    if (!test_dirfileop()) {
        return (false);
    }
    if (!test_paths()) {
        return (false);
    }
    if (!test_pipes()) {
        return (false);
    }
    if (!test_format()) {
        return (false);
    }
    if (!test_parse()) {
        return (false);
    }
    if (!test_unicode()) {
        return (false);
    }
    if (!test_console()) {
        return (false);
    }

    // cleanup
    sing::fileRemove("testfile1.txt");

    return (true);
}

static bool test_File()
{
    sing::File ff;
    sing::FileInfo nfo;
    sing::FileInfo nfo2;
    std::vector<uint8_t> buf = {(uint8_t)48, (uint8_t)49, (uint8_t)50, (uint8_t)51};
    std::string back;
    int64_t pos = 0;

    // write a file
    if (ff.open("testfile1.txt", "w") != 0) {
        return (false);
    }
    if (ff.put((uint8_t)48) != 0) {
        return (false);
    }
    if (ff.puts("hello") != 0) {
        return (false);
    }
    if (ff.write(2, buf, 1) != 0) {
        return (false);
    }

    // autoclose and read back
    if (ff.open("testfile1.txt", "r+") != 0) {
        return (false);
    }
    if (ff.read(2, &buf, true) != 0) {
        return (false);
    }
    if (ff.get(&buf[0]) != 0) {
        return (false);
    }
    if (ff.gets(5, &back) != 0) {
        return (false);
    }
    if (buf.size() < 6 || buf[4] != 48 || buf[5] != 104 || buf[0] != 101 || back != "llo12") {
        return (false);
    }

    // seek/tell/eof
    if (ff.eof()) { // flag is set when an attempt to read PAST eof has done
        return (false);
    }
    if (ff.get(&buf[0]) == 0) {         // the attempt must fail !
        return (false);
    }
    if (!ff.eof()) {
        return (false);
    }
    if (ff.seek(1) != 0) {
        return (false);
    }
    if (ff.gets(5, &back) != 0 || back != "hello") {
        return (false);
    }
    if (ff.seek(-5, sing::SeekMode::seek_cur) != 0) {
        return (false);
    }
    if (ff.gets(5, &back) != 0 || back != "hello") {
        return (false);
    }
    if (ff.seek(-7, sing::SeekMode::seek_end) != 0) {
        return (false);
    }
    if (ff.gets(5, &back) != 0 || back != "hello") {
        return (false);
    }
    if (ff.tell(&pos) != 0 || pos != 6) {
        return (false);
    }

    // try '+' operations

    // write after read
    if (ff.puts(" world") != 0) {
        return (false);
    }
    if (ff.seek(0) != 0) {
        return (false);
    }
    if (ff.puts(" ") != 0) {
        return (false);
    }

    // read after write
    if (ff.gets(100, &back) != 0 || back != "hello world") {
        return (false);
    }

    // handling line termination
    if (ff.puts("\n hello world\r\n hello world") != 0) {
        return (false);
    }
    if (ff.put((uint8_t)0) != 0) {
        return (false);
    }
    if (ff.seek(0) != 0) {
        return (false);
    }
    if (ff.gets(100, &back) != 0 || back != " hello world\n") {
        return (false);
    }
    if (ff.gets(100, &back) != 0 || back != " hello world\n") {
        return (false);
    }
    if (ff.gets(100, &back) != 0 || back != " hello world") {
        return (false);
    }
    if (ff.get(&buf[0]) == 0) {         // the attempt must fail !
        return (false);
    }

    // getinfo
    if (ff.getInfo(&nfo) != 0) {
        return (false);
    }
    if (nfo.is_dir_ || nfo.length_ != 40) {
        return (false);
    }

    // explicit close
    if (ff.close() != 0) {
        return (false);
    }
    if (ff.write(2, buf, 1) == 0) {     // must fail (file is close)
        return (false);
    }

    // close by destructor
    {
        sing::File temp;
        if (temp.open("testfile1.txt", "r") != 0) {
            return (false);
        }
    }

    // getinfo without opening 
    if (sing::fileGetInfo("testfile1.txt", &nfo2) != 0) {
        return (false);
    }

    return (true);
}

static bool test_dirfileop()
{
    sing::FileInfo nfo;

    // get/set cwd
    const std::string home = sing::getCwd();

    if (sing::dirCreate("dir1/dir2") != 0) {
        return (false);
    }
    if (sing::setCwd("dir1") != 0) {
        return (false);
    }

    sing::File ff;

    // create a file
    if (ff.open("in_dir1.txt", "w") != 0) {
        return (false);
    }
    if (ff.getInfo(&nfo) != 0) {        // for comparison with nfos
        return (false);
    }
    if (ff.close() != 0) {
        return (false);
    }

    if (sing::setCwd(home.c_str()) != 0) {
        return (false);
    }

    // create another file (to test dirlist recursion)
    if (ff.open("dir1/dir2/leaf.txt", "w") != 0) {
        return (false);
    }
    if (ff.close() != 0) {
        return (false);
    }

    // dir listings
    std::vector<std::string> names;
    std::vector<sing::FileInfo> nfos;
    if (sing::dirRead("dir1", sing::DirFilter::regular, &names, &nfos) != 0) {
        return (false);
    }
    if (names.size() < 1 || names[0] != "dir1/in_dir1.txt") {
        return (false);
    }
    names.clear();
    if (sing::dirReadNames("dir1", sing::DirFilter::directory, &names) != 0) {
        return (false);
    }
    if (names.size() < 1 || names[0] != "dir1/dir2") {
        return (false);
    }
    names.clear();
    if (sing::dirReadNames("dir1", sing::DirFilter::all, &names) != 0) {
        return (false);
    }
    if (names.size() != 2) {
        return (false);
    }
    if (sing::dirReadNames("dir1", sing::DirFilter::all, &names, true) != 0) {
        return (false);
    }
    if (names.size() != 3) {
        return (false);
    }

    // file ops
    if (sing::fileRename("dir1/in_dir1.txt", "dir1/in_dir.txt") != 0) {
        return (false);
    }
    if (sing::fileRemove("dir1/in_dir.txt") != 0) {
        return (false);
    }

    // check deletion, create again
    if (ff.open("dir1/in_dir.txt", "r") == 0) {             // must fail !!
        return (false);
    }
    if (ff.open("dir1/in_dir1.txt", "w") != 0) {
        return (false);
    }
    if (ff.close() != 0) {
        return (false);
    }

    if (sing::dirRemove("dir1") == 0) { // must fail !!
        return (false);
    }
    if (sing::dirRemove("dir1", true) != 0) {
        return (false);
    }
    if (ff.open("dir1/in_dir.txt", "r") == 0) {             // must fail !!
        return (false);
    }

    std::string readback;
    std::vector<uint8_t> towrite = {(uint8_t)0, (uint8_t)1, (uint8_t)2, (uint8_t)3};
    std::vector<uint8_t> toread;
    if (sing::fileWriteText("testfile1.txt", "bang !!") != 0) {
        return (false);
    }
    if (sing::fileReadText("testfile1.txt", &readback) != 0 || readback != "bang !!") {
        return (false);
    }
    if (sing::fileWrite("testfile1.txt", towrite) != 0) {
        return (false);
    }
    if (sing::fileRead("testfile1.txt", &toread) != 0 || towrite != toread) {
        return (false);
    }

    return (true);
}

static bool test_paths()
{
    std::string drive;
    std::string path;
    std::string base;
    std::string ext;
    int32_t drive_idx = 0;
    bool absolute = false;
    std::vector<std::string> parts;

    // split/join
    sing::pathSplit("g:/one/two\\three.jpg", &drive, &path, &base, &ext);
    if (drive != "g:" || path != "/one/two\\" || base != "three" || ext != "jpg") {
        return (false);
    }
    if (sing::pathJoin(drive.c_str(), path.c_str(), base.c_str(), ext.c_str()) != "g:/one/two\\three.jpg" ||
        sing::pathJoin("g", "/one/two", "three.", ext.c_str()) != "g:/one/two/three.jpg") {
        return (false);
    }

    // split/join all
    sing::pathSplitAll("g:/one//two\\three.jpg", &drive_idx, &absolute, &parts);
    if (drive_idx != 6 || !absolute || parts.size() != 3) {
        return (false);
    }
    if (sing::pathJoinAll(drive_idx, absolute, parts) != "g:/one/two/three.jpg") {
        return (false);
    }
    sing::pathSplitAll("g:one//two\\three.jpg", &drive_idx, &absolute, &parts);
    if (drive_idx != 6 || absolute || parts.size() != 3) {
        return (false);
    }
    if (sing::pathJoinAll(drive_idx, absolute, parts) != "g:one/two/three.jpg") {
        return (false);
    }

    // path fix
    if (sing::pathFix("c:/one////.//two/three/../../four/") != "c:/one/four") {
        return (false);
    }
    if (sing::pathFix("c:/one/../..//.//two/three/../../four/") != "c:/four") {
        return (false);
    }
    if (sing::pathFix("c:one/../..//.//two/three/../../four/") != "c:../four") {
        return (false);
    }

    // set extension
    base = "file.txt";
    sing::pathSetExtension(&base, ".jpg");
    if (base != "file.jpg") {
        return (false);
    }
    base = "file";
    sing::pathSetExtension(&base, "jpg");
    if (base != "file.jpg") {
        return (false);
    }

    // to abs
    if (sing::pathToAbsolute(&path, "c:/", ".") || sing::pathToAbsolute(&path, "c:..", "d:/")) {
        return (false);
    }
    if (!sing::pathToAbsolute(&path, "../../one") || !sing::pathToAbsolute(&path, "../../one", "d:/first/second/third")) {
        return (false);
    }
    if (path != "d:/first/one") {
        return (false);
    }

    // to rel
    if (sing::pathToRelative(&path, "c:..", ".") || sing::pathToRelative(&path, "c:/first", "d:/")) {
        return (false);
    }
    if (!sing::pathToRelative(&path, "/absolute/path") || !sing::pathToRelative(&path, "d:/aa/.././first/one", "d:/first/second/third")) {
        return (false);
    }
    if (path != "../../one") {
        return (false);
    }
    return (true);
}

static bool test_pipes()
{
    return (true);
}

static bool test_format()
{
    if (sing::formatInt(123, 8) != "     123" || sing::formatInt(123, 8, sing::f_dont_omit_plus + sing::f_align_left) != "+123    " ||
        sing::formatInt(123, 8, sing::f_zero_prefix) != "00000123" || sing::formatInt(123, 2) != "##") {
        return (false);
    }

    if (sing::formatUnsigned((uint64_t)123, 8) != "     123" ||
        sing::formatUnsigned((uint64_t)123, 8, sing::f_dont_omit_plus + sing::f_align_left) != "+123    " ||
        sing::formatUnsigned((uint64_t)123, 8, sing::f_zero_prefix) != "00000123" || sing::formatUnsigned((uint64_t)123, 2) != "##") {
        return (false);
    }

    if (sing::formatUnsignedHex(0x8000000000000000LLU, 20) != "    8000000000000000" ||
        sing::formatUnsignedHex(0x8000000000000000LLU, 20, sing::f_dont_omit_plus + sing::f_align_left) != "+8000000000000000   " ||
        sing::formatUnsignedHex((uint64_t)0xa89, 8, sing::f_zero_prefix | sing::f_uppercase) != "00000A89" ||
        sing::formatUnsignedHex((uint64_t)0xa89, 2) != "##") {
        return (false);
    }

    if (sing::formatFloat(5.13f, 10) != "  5.130000" || sing::formatFloat(5.13f, 10, 3, sing::f_dont_omit_plus + sing::f_align_left) != "+5.130    " ||
        sing::formatFloat(5.13f, 10, 3, sing::f_zero_prefix) != "000005.130" || sing::formatFloat(5.13e20f, 10) != "##########") {
        return (false);
    }

    if (sing::formatFloatSci(10.43f, 15, 5) != "   1.04300e+1  " ||
        sing::formatFloatSci(10.43f, 15, 5, sing::f_dont_omit_plus + sing::f_zero_prefix + sing::f_uppercase + sing::f_align_left) != "+1.04300E+001  ") {
        return (false);
    }

    if (sing::formatFloatEng(12345.432f, 15, 5) != "  12.34543e+3  " ||
        sing::formatFloatEng(12345.432f, 20, 5, sing::f_dont_omit_plus + sing::f_zero_prefix + sing::f_uppercase + sing::f_align_left) !=
        "+012.34543E+003     " || sing::formatFloatEng(12345.432f, 15, 5, sing::f_zero_prefix) != " 012.34543e+003" ||
        sing::formatFloatEng(12345.432f, 5, 5, sing::f_zero_prefix) != "#####") {
        return (false);
    }

    // strings
    if (sing::formatString("abc", 5) != "  abc" || sing::formatString("abc", 5, sing::f_align_left) != "abc  " || sing::formatString("abcdef", 5) != "abcde") {
        return (false);
    }

    return (true);
}

static bool test_parse()
{
    int64_t value = 0;
    uint64_t uvalue = 0;
    double fvalue = 0;
    int32_t last = 0;

    if (!sing::parseInt(&value, "567345 ", 1, &last) || value != 67345 || last != 6) {
        return (false);
    }
    if (!sing::parseUnsignedInt(&uvalue, "567345 ", 1, &last) || !sing::iseq(uvalue, 67345) || last != 6) {
        return (false);
    }
    if (!sing::parseUnsignedHex(&uvalue, "678z", 0, &last) || !sing::iseq(uvalue, 0x678) || last != 3) {
        return (false);
    }
    if (!sing::parseInt(&value, "0x100 ", 0, &last) || value != 0x100) {
        return (false);
    }
    if (!sing::parseFloat(&fvalue, "   102.3e2", 3, &last) || fvalue != 102.3e2f) {
        return (false);
    }

    return (true);
}

static bool test_console()
{
    bool ok = false;
    while (!ok) {
        sing::scrClear();
        sing::print("犬");
        sing::printError("no error");
        sing::print("\nplease input the word: test\n> ");
        ok = sing::kbdInput(20) == "test";
    }
    sing::print("\nplease press a key.");
    const std::string pressed = sing::kbdGet();
    sing::print(("\n\nYou pressed: " + pressed).c_str());
    sing::print("\nnow press any key to exit.");
    sing::kbdGet();
    return (true);
}

static bool test_unicode()
{
    sing::File ff;
    sing::FileInfo nfo;

    // write a file
    if (ff.open("犬.txt", "w") != 0) {
        return (false);
    }
    if (ff.close() != 0) {
        return (false);
    }

    // file ops
    if (sing::fileRename("犬.txt", "ネコ.txt") != 0) {
        return (false);
    }
    if (sing::fileGetInfo("ネコ.txt", &nfo) != 0) {
        return (false);
    }
    if (sing::fileRemove("ネコ.txt") != 0) {
        return (false);
    }
    if (ff.open("ネコ.txt", "r") == 0) {  // should fail !!
        return (false);
    }

    // dir create
    if (sing::dirCreate("その上/下") != 0) {
        return (false);
    }
    if (sing::setCwd("その上/下") != 0) {
        return (false);
    }

    // create a file
    if (ff.open("in_depth.txt", "w") != 0) {
        return (false);
    }
    if (ff.close() != 0) {
        return (false);
    }

    // get/set cwd
    const std::string deep = sing::getCwd();
    sing::setCwd("../..");
    sing::setCwd(deep.c_str());
    if (ff.open("in_depth.txt", "r") != 0) {                // should fail
        return (false);
    }
    if (ff.close() != 0) {
        return (false);
    }
    sing::setCwd("../..");

    // dir list
    std::vector<std::string> names;
    std::vector<sing::FileInfo> nfos;
    if (sing::dirRead("その上", sing::DirFilter::all, &names, &nfos, true) != 0) {
        return (false);
    }
    if (names.size() != 2) {
        return (false);
    }

    // remove !
    if (sing::dirRemove("その上", true) != 0) {
        return (false);
    }
    if (ff.open("その上/下/in_depth.txt", "r") == 0) {          // must fail !!
        return (false);
    }

    return (true);
}
