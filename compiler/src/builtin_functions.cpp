#include <string.h>
#include "builtin_functions.h"
#include "ast_nodes.h"

namespace SingNames {

struct IFDesc {
    const char *name;
    const char *signature;
};

static const char *GetSignatureByKey(IFDesc *map, const char *name);
static IAstTypeNode *GetTypeFromSignature(const char type_id, const ExpressionAttributes *attr, bool *owning);
static VarDeclaration *GetVarFromSignature(const char type_id, const ExpressionAttributes *attr);

// signature:
// optional prefix: M for muting.
// first letter is return value
// T = same type as implicit argument
// i = int
// v = void
// b = bool
// k = key type
// e = value/element type
// other are INPUT parms
// x = index
// s = size
// f = compare function f(T, x, x) b
// k = key type
// e = value/element type
// last letter is the implementation type:
// s = sing::name(T);
// c = (cast)name(T); // cast if not double
// p = name(T); // plain
// m = T.member()
// S = std::name(T)
// NOTE: functions on integers are all with 0 values and not muting (apply also to right values)
// the two letters are the returned value and the implementation type.
IFDesc g_integer_functs[] = {{"abs", "Ts"}, {"sqrt", "Tc"}, {"sgn", "is"}, {"", ""}};

IFDesc g_float_functs[] = {
{"abs", "Ts"}, {"sqrt", "Tc"}, {"sgn", "is"},
{"sin", "Tc"}, {"cos", "Tc"}, {"tan", "Tc"}, {"asin", "Tc"}, {"acos", "Tc"}, {"atan", "Tc"},
{"log", "Tc"}, {"exp", "Tc"}, {"log10", "Tc"}, {"exp10", "Ts"}, {"log2", "Tc"}, {"exp2", "Tc"},
{"floor", "Tc"}, {"ceil", "Tc"}, {"round", "Tc"}, {"", ""}
};

IFDesc g_complex_functs[] = {
{"abs", "eS"}, {"arg", "eS"}, {"imag", "eS"}, {"real", "eS"}, {"norm", "eS"}, 
{"sqrt", "TS"},
{"sin", "TS"}, {"cos", "TS"}, {"tan", "TS"}, {"asin", "TS"}, {"acos", "TS"}, {"atan", "TS"},
{"log", "TS"}, {"exp", "TS"}, {"", ""}
};

IFDesc g_array_functs[] = {
{"reserve", "Mvsm"}, {"capacity", "im"}, {"trim", "Mvm"},
{"resize", "Mvsm"}, {"clear", "Mvm"}, {"size", "im"}, {"isempty", "bm"},
{"last", "em"}, {"push_back", "Mvem"}, {"pop_back", "Mvm"}, {"insert", "Mvxsem"}, {"erase", "Mvxxm"},
//{"sort", "Mvxsf"}, {"radix_sort", "Mvxsf"}, {"find", "ie"}, {"binary_search", "ie"}, {"upper_bound", "ie"}, {"lower_bound", "ie"},
{"", ""}
};

IFDesc g_map_functs[] = {
{"reserve", "Mvsm"}, {"capacity", "im"}, {"trim", "Mvm"},
{"clear" ,"Mvm"}, {"size", "im"}, {"isempty", "bm"},
{"insert", "Mvkem"}, {"erase", "Mvkm"}, {"get", "ekm"}, {"get_safe", "ekem"}, {"has", "bkm"},
{"key_at", "kxm"}, {"value_at", "exm"},
//{"sort_by_key", "Mvxsf"}, {"radix_sort_by_key", "Mvxsf"}, {"sort_by_value", "Mvxsf"}, {"radix_sort_by_value", "Mvxsf"}, 
//{"find_value", "ie"}, {"binary_search_value", "ie"}, {"upper_bound_value", "ie"}, {"lower_bound_value", "ie"},
{"", ""}
};

static const char *GetSignatureByKey(IFDesc *map, const char *name)
{
    while (map->name[0] != 0) {
        if (strcmp(map->name, name) == 0) {
            return(map->signature);
        }
        ++map;
    }
    return(nullptr);
}

AstFuncType *GetFuncSignature(bool *ismuting, BInSynthMode *mode, const char *name, const ExpressionAttributes *attr)
{
    IFDesc *table;
    if (attr->IsInteger()) {
        table = g_integer_functs;
    } else if (attr->IsFloat()) {
        table = g_float_functs;
    } else if (attr->IsComplex()) {
        table = g_complex_functs;
    } else if (attr->IsArray()) {
        table = g_array_functs;
    } else if (attr->IsMap()) {
        table = g_map_functs;
    }
    const char *signature = GetSignatureByKey(table, name);
    if (signature == nullptr || signature[0] == 0) return(nullptr);
    if (signature[0] == 'M') {
        *ismuting = true;
        ++signature;
    } else {
        *ismuting = false;
    }
    int numargs = strlen(signature) - 2;
    if (numargs < 0) {
        return(nullptr);
    }
    AstFuncType *retvalue = new AstFuncType(true);  // note: all functions are pure !
    if (retvalue == nullptr) return(nullptr);
    bool owning;
    retvalue->SetReturnType(GetTypeFromSignature(*signature++, attr, &owning));
    retvalue->SetOwning(owning);
    for (int ii = 0; ii < numargs; ++ii) {
        retvalue->AddArgument(GetVarFromSignature(*signature++, attr));
    }
    retvalue->SetIsMember();    // to prevent undue copying

    switch (signature[strlen(signature) - 1]) {
    case 's':
        *mode = BInSynthMode::sing;
        break;
    case 'S':
        *mode = BInSynthMode::std;
        break;
    case 'c':
        *mode = BInSynthMode::cast;
        break;
    case 'm':
        *mode = BInSynthMode::member;
        break;
    case 'p':
    default:
        *mode = BInSynthMode::plain;
    }
    return(retvalue);
}

static IAstTypeNode *GetTypeFromSignature(const char type_id, const ExpressionAttributes *attr, bool *owning)
{
    static AstBaseType int_type(TOKEN_INT32);
    static AstBaseType void_type(TOKEN_VOID);
    static AstBaseType bool_type(TOKEN_BOOL);
    static AstBaseType size_type(TOKEN_INT64);
    static AstBaseType float32_type(TOKEN_FLOAT32);
    static AstBaseType float64_type(TOKEN_FLOAT64);

    *owning = false;
    IAstTypeNode *the_type = nullptr;
    switch (type_id) {
    case 'T':
        the_type = attr->GetTypeTree();
        if (the_type == nullptr) {
            Token base_type = attr->GetAutoBaseType();
            if (base_type != TOKENS_COUNT) {
                *owning = true;
                the_type = new AstBaseType(base_type);
            }
        }
        break;
    case 'i':
        the_type = &int_type;
        break;
    case 'v':
        the_type = &void_type;
        break;
    case 'b':
        the_type = &bool_type;
        break;
    case 'k':
        the_type = attr->GetTypeTree();
        if (the_type->GetType() == ANT_MAP_TYPE) {
            the_type = ((AstMapType*)the_type)->key_type_;
        }
        break;
    case 'e':
        the_type = attr->GetTypeTree();
        switch (the_type->GetType()) {
        case ANT_MAP_TYPE:
            the_type = ((AstMapType*)the_type)->returned_type_;
            break;
        case ANT_ARRAY_TYPE:
            the_type = ((AstArrayType*)the_type)->element_type_;
            break;
        default:
            switch (attr->GetAutoBaseType()) {
            case TOKEN_COMPLEX64:
                the_type = &float32_type;
                break;
            case TOKEN_COMPLEX128:
                the_type = &float64_type;
                break;
            default:
                break;
            }
            break;
        }
        break;
    case 'x':
    case 's':
        the_type = &size_type;
        break;
    default:
        break;
    }
    return(the_type);
}

static VarDeclaration *GetVarFromSignature(const char type_id, const ExpressionAttributes *attr)
{
    bool owning;
    IAstTypeNode *typedesc = GetTypeFromSignature(type_id, attr, &owning);
    if (typedesc == nullptr) return(nullptr);
    const char *name = "";
    switch (type_id) {
    case 'k':
        name = "key";
        break;
    case 'e':
        name = "element";
        break;
    case 'x':
        name = "idx";
        break;
    case 's':
        name = "size";
        break;
    case 'i':
    case 'T':
    case 'v':
    case 'b':
    default:
        break;
    }
    VarDeclaration *the_var = new VarDeclaration(name);
    the_var->SetFlags(VF_ISARG | VF_READONLY);
    if (owning) {
        the_var->SetType(typedesc);
    } else {
        the_var->weak_type_spec_ = typedesc;
    }
    return(the_var);
}

}