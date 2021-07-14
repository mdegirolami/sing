#include "json_test.h"
#include "json.h"
#include "sio.h"

bool jsonTest()
{
    std::string original;
    std::vector<sing::JsonEntry> entries;
    std::vector<sing::JsonError> errors;

    // parse
    if (sing::fileReadText("json_test/tst.json", &original) != 0) {
        return (false);
    }
    if (!sing::parseJson(original.c_str(), &entries, &errors)) {
        return (false);
    }

    // list the found entries
    std::string list;
    for(auto &record : entries) {
        const sing::JsonEntryType record_type = record.entry_type_;
        switch (record_type) {
        case sing::JsonEntryType::number: 
            list += "number";
            break;
        case sing::JsonEntryType::string_type: 
            list += "string";
            break;
        case sing::JsonEntryType::boolean: 
            list += "bool";
            break;
        case sing::JsonEntryType::null_type: 
            list += "null";
            break;
        case sing::JsonEntryType::array: 
            list += "array";
            break;
        case sing::JsonEntryType::object: 
            list += "object";
            break;
        case sing::JsonEntryType::comment: 
            list += "comment";
            break;
        }
        list += sing::s_format("%s%d%s%s%s%s%s", " ", record.level_, " ", record.property_name_.c_str(), " ", record.string_rep_.c_str(), "\r\n");
    }
    sing::fileWriteText("json_test/list.txt", list.c_str());

    // rebuild
    std::string rebuilt;
    if (!sing::writeJson(entries, &rebuilt)) {
        return (false);
    }

    // compare, write down if in error
    std::string fixed;
    if (sing::fileReadText("json_test/fixed.json", &fixed) != 0) {
        return (false);
    }
    if (rebuilt != fixed) {
        sing::fileWriteText("json_test/rebuilt.json", rebuilt.c_str());
        return (false);
    }
    return (true);
}
