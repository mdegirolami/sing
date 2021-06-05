#include "buildupd.h"
#include "sio.h"
#include "str.h"
#include "sort.h"

// rows of interest to fixBuild() fall into two classes...
enum class RowType {dep, target, unknown};

// dep rows are of 3 types...
enum class DepType {sing2tmp, tmp2obj, src2obj, count};

class Target final {
public:
    Target();
    std::string basic_dep_;             // the row deprived of objs
    std::vector<std::string> objs_base_;
    std::string extras_;                // extra deps (not sing/cpp objs)
    int32_t row_;
};

class Dependency final {
public:
    Dependency();
    std::string path_;
    std::string base_;
    std::string ext_;                   // if extension is "sing" sing2tmp_.. fields can be filled

    int32_t sing2tmp_row_;
    std::string sing2tmp_rule_;
    int32_t x2obj_row_;
    std::string x2obj_rule_;
    int32_t target_idx_;

    bool deleted_;
};

static bool updateDescriptors(sing::map<std::string, int32_t> *base2idx, std::vector<Dependency> *deps, const sing::map<std::string, int32_t> &srcbase2idx,
    const std::vector<Source> &sources, std::vector<Target> *targets, const sing::array<int32_t, (size_t)DepType::count> &def_row);
static bool hasSingRule(const Dependency &dep);
static bool hasObjRule(const Dependency &dep);
static bool hasTargetIdx(const Dependency &dep);
static int32_t mostSimilarDep(const char *path, const char *ext, bool (*is_ok)(const Dependency &dep), const std::vector<Dependency> &deps);
static RowType extractDepOrTarget(std::vector<Dependency> *deps, sing::map<std::string, int32_t> *base2idx, std::vector<Target> *targets, const char *line,
    int32_t row);
static bool extractDependency(std::vector<Dependency> *deps, sing::map<std::string, int32_t> *base2idx, const char *right, int32_t row);
static void extractTarget(Target *target, const char *left, const char *right, int32_t row);
static std::string sing2tempSynth(const char *path, const char *base, const char *rule, int32_t max_field_len);
static std::string temp2objSynth(const char *path, const char *base, const char *rule, int32_t max_field_len);
static std::string cpp2objSynth(const char *path, const char *base, const char *ext, const char *rule, int32_t max_field_len);
static std::string targetSynth(const Target &tar);

Target::Target()
{
    row_ = 0;
}

Dependency::Dependency()
{
    sing2tmp_row_ = -1;
    x2obj_row_ = -1;
    target_idx_ = -1;
    deleted_ = false;
}

std::string Source::getFullName() const
{
    std::string path;
    if (ext_ == "sing") {
        path = "sing/" + this->path_;
    } else {
        path = "src/" + this->path_;
    }
    return (sing::pathJoin("", path.c_str(), base_.c_str(), ext_.c_str()));
}

std::string fixBuild(bool *has_mods, const char *name, const std::vector<Source> &sources, const sing::map<std::string, int32_t> &srcbase2idx)
{
    std::vector<int32_t> row_priority;
    std::vector<std::string> rows;
    std::vector<Target> targets;
    std::vector<Dependency> deps;
    sing::map<std::string, int32_t> base2idx;               // to find a record in deps by the base name

    *has_mods = false;

    // read the original file
    // place interesting rows in descriptors, the other in rows[] 
    sing::File build_aux;
    std::string line;
    if (build_aux.open(name, "r") != 0) {
        return ("");
    }
    int32_t row_num = 0;
    while (build_aux.gets(10000000, &line) == 0 && sing::len(line.c_str()) > 0) {
        ++row_num;
        sing::cutSuffix(&line, "\n");
        RowType rtype = RowType::unknown;
        if (sing::hasPrefix(line.c_str(), "build")) {
            rtype = extractDepOrTarget(&deps, &base2idx, &targets, line.c_str(), row_num);
        }
        if (rtype == RowType::unknown) {
            rows.push_back((line + "\r\n").c_str());
            row_priority.push_back(row_num);
        }
    }
    build_aux.close();

    // create a default targets if there are none
    if (targets.size() < 1) {
        targets.resize(1);
        targets.at(0).basic_dep_ = "build $bin_target: ln";
        targets.at(0).row_ = row_num + 1;
    }

    // define where to place dependencies by default
    sing::array<int32_t, (size_t)DepType::count> def_row = {-1, -1, -1};
    int64_t count = -1;
    for(auto &row : rows) {
        ++count;
        if (row == "# sing->temp\r\n") {
            def_row.at((size_t)DepType::sing2tmp) = row_priority.at(count);
        } else if (row == "# temp->obj\r\n") {
            def_row.at((size_t)DepType::tmp2obj) = row_priority.at(count);
        } else if (row == "# cpp->obj\r\n") {
            def_row.at((size_t)DepType::src2obj) = row_priority.at(count);
        }
    }
    for(auto &def_value : def_row) {
        if (def_value == -1 && deps.size() > 0) {
            def_value = deps.at(deps.size() - 1).x2obj_row_;
        }
        if (def_value == -1) {
            def_value = row_num + 1;
        }
    }

    // update the dependencies (delete and add to align to available sources)
    *has_mods = updateDescriptors(&base2idx, &deps, srcbase2idx, sources, &targets, def_row);
    if (!*has_mods) {
        return ("");
    }

    // align all ':' the same
    sing::array<int32_t, (size_t)DepType::count> max_field_len = {0};
    for(auto &dep : deps) {
        const int32_t baselen = sing::numchars(dep.base_.c_str());
        const int32_t fulllen = baselen + sing::numchars(dep.path_.c_str());
        if (dep.ext_ == "sing") {
            max_field_len.at((size_t)DepType::sing2tmp) = std::max(max_field_len.at((size_t)DepType::sing2tmp), fulllen);
            max_field_len.at((size_t)DepType::tmp2obj) = std::max(max_field_len.at((size_t)DepType::tmp2obj), baselen);
        } else {
            max_field_len.at((size_t)DepType::src2obj) = std::max(max_field_len.at((size_t)DepType::src2obj), baselen);
        }
    }

    // add dep lines to rows[]
    for(auto &dep : deps) {
        if (dep.ext_ == "sing") {
            rows.push_back(
                sing2tempSynth(dep.path_.c_str(), dep.base_.c_str(), dep.sing2tmp_rule_.c_str(), max_field_len.at((size_t)DepType::sing2tmp)).c_str());
            row_priority.push_back(dep.sing2tmp_row_);

            rows.push_back(temp2objSynth(dep.path_.c_str(), dep.base_.c_str(), dep.x2obj_rule_.c_str(), max_field_len.at((size_t)DepType::tmp2obj)).c_str());
            row_priority.push_back(dep.x2obj_row_);
        } else {
            rows.push_back(
                cpp2objSynth(dep.path_.c_str(), dep.base_.c_str(), dep.ext_.c_str(), dep.x2obj_rule_.c_str(),
                max_field_len.at((size_t)DepType::src2obj)).c_str());
            row_priority.push_back(dep.x2obj_row_);
        }
    }

    // add target lines to rows[]
    for(auto &tar : targets) {
        rows.push_back(targetSynth(tar).c_str());
        row_priority.push_back(tar.row_);
    }

    // sort rows[]
    std::vector<int32_t> index;
    sing::indexInit(&index, rows.size());
    sing::ksort_i32(&index, row_priority);

    // print down the new file
    build_aux.open(name, "w");
    for(auto &idx : index) {
        build_aux.puts(rows.at(idx).c_str());
    }
    build_aux.close();

    // done !!
    return ("");
}

// returns false if there is no update to be done.
static bool updateDescriptors(sing::map<std::string, int32_t> *base2idx, std::vector<Dependency> *deps, const sing::map<std::string, int32_t> &srcbase2idx,
    const std::vector<Source> &sources, std::vector<Target> *targets, const sing::array<int32_t, (size_t)DepType::count> &def_row)
{
    bool has_changes = false;

    // flag obsolete dependencies, 
    // also update path/extension based on actual source values. 
    for(auto &dep : *deps) {
        const int32_t idx = srcbase2idx.get_safe(dep.base_.c_str(), -1);
        if (idx == -1) {
            dep.deleted_ = true;
            has_changes = true;
        } else {
            if (dep.path_ != sources.at(idx).path_ || dep.ext_ != sources.at(idx).ext_) {
                has_changes = true;
                dep.path_ = sources.at(idx).path_;
                dep.ext_ = sources.at(idx).ext_;
            }
        }
    }

    // create dependencies for new sources
    for(auto &src : sources) {
        if (!(*base2idx).has(src.base_.c_str())) {
            const int32_t idx = (*deps).size();
            (*deps).resize(idx + 1);
            (*deps).at(idx).path_ = src.path_;
            (*deps).at(idx).base_ = src.base_;
            (*deps).at(idx).ext_ = src.ext_;
            (*base2idx).insert(src.base_.c_str(), idx);
            has_changes = true;
        }
    }

    // assign dependencies to respective targets
    int64_t count = -1;
    for(auto &tar : *targets) {
        ++count;
        for(auto &basename : tar.objs_base_) {
            const int32_t idx = (*base2idx).get_safe(basename.c_str(), -1);
            if (idx != -1) {
                (*deps).at(idx).target_idx_ = (int32_t)count;
            }
        }
    }

    // fix other uninited fields in undeleted deps
    for(auto &dep : *deps) {
        if (!dep.deleted_) {
            const bool is_sing = dep.ext_ == "sing";
            if (is_sing && dep.sing2tmp_row_ == -1) {
                const int32_t idx = mostSimilarDep(dep.path_.c_str(), dep.ext_.c_str(), hasSingRule, *deps);
                if (idx != -1) {
                    dep.sing2tmp_row_ = (*deps).at(idx).sing2tmp_row_;
                    dep.sing2tmp_rule_ = (*deps).at(idx).sing2tmp_rule_;
                    if (dep.x2obj_row_ == -1) {
                        dep.x2obj_row_ = (*deps).at(idx).x2obj_row_;
                        dep.x2obj_rule_ = (*deps).at(idx).x2obj_rule_;
                    }
                    if (dep.target_idx_ == -1) {
                        dep.target_idx_ = (*deps).at(idx).target_idx_;
                    }
                } else {
                    dep.sing2tmp_rule_ = "sc";
                    dep.sing2tmp_row_ = def_row.at((size_t)DepType::sing2tmp);
                }
                has_changes = true;
            }
            if (dep.x2obj_row_ == -1) {
                const int32_t idx = mostSimilarDep(dep.path_.c_str(), dep.ext_.c_str(), hasObjRule, *deps);
                if (idx != -1) {
                    dep.x2obj_row_ = (*deps).at(idx).x2obj_row_;
                    dep.x2obj_rule_ = (*deps).at(idx).x2obj_rule_;
                    if (dep.target_idx_ == -1) {
                        dep.target_idx_ = (*deps).at(idx).target_idx_;
                    }
                } else {
                    dep.x2obj_rule_ = "cc";
                    if (is_sing) {
                        dep.x2obj_row_ = def_row.at((size_t)DepType::tmp2obj);
                    } else {
                        dep.x2obj_row_ = def_row.at((size_t)DepType::src2obj);
                    }
                }
                has_changes = true;
            }
            if (dep.target_idx_ == -1) {
                const int32_t idx = mostSimilarDep(dep.path_.c_str(), dep.ext_.c_str(), hasTargetIdx, *deps);
                if (idx != -1) {
                    dep.target_idx_ = (*deps).at(idx).target_idx_;
                } else {
                    dep.target_idx_ = 0;
                }
                has_changes = true;
            }
        }
    }

    // rebuild targets objs lists
    for(auto &tar : *targets) {
        tar.objs_base_.clear();
    }
    for(auto &dep : *deps) {
        if (!dep.deleted_ && dep.target_idx_ >= 0) {
            (*targets).at(dep.target_idx_).objs_base_.push_back(dep.base_.c_str());
        }
    }
    return (has_changes);
}

static bool hasSingRule(const Dependency &dep)
{
    return (dep.sing2tmp_row_ >= 0);
}

static bool hasObjRule(const Dependency &dep)
{
    return (dep.x2obj_row_ >= 0);
}

static bool hasTargetIdx(const Dependency &dep)
{
    return (dep.target_idx_ >= 0);
}

static int32_t mostSimilarDep(const char *path, const char *ext, bool (*is_ok)(const Dependency &dep), const std::vector<Dependency> &deps)
{
    const int32_t len = sing::len(path);
    int32_t best = -1;
    int32_t bestidx = -1;
    int64_t count = -1;
    for(auto &dep : deps) {
        ++count;
        if (ext == dep.ext_ && is_ok(dep)) {
            int32_t matching = -1;
            if (sing::compareAt(dep.path_.c_str(), 0, path, &matching, true) == 0 && matching == len) {
                return ((int32_t)count);
            } else if (matching > best) {
                best = matching;
                bestidx = (int32_t)count;
            }
        }
    }
    return (bestidx);
}

static RowType extractDepOrTarget(std::vector<Dependency> *deps, sing::map<std::string, int32_t> *base2idx, std::vector<Target> *targets, const char *line,
    int32_t row)
{
    // split at ':'
    std::string left_term;
    std::string right_term;
    if (!sing::split(line, ":", &left_term, &right_term)) {
        return (RowType::unknown);
    }
    sing::cutLeadingSpaces(&right_term);

    // if has a .o dependee is a target, else a depencdency
    sing::Range range;
    if (!sing::find(right_term.c_str(), ".o", &range)) {
        if (extractDependency(&*deps, &*base2idx, right_term.c_str(), row)) {
            return (RowType::dep);
        } else {
            return (RowType::unknown);
        }
    }
    const int32_t num_targets = (*targets).size();
    (*targets).resize(num_targets + 1);
    extractTarget(&(*targets).at(num_targets), left_term.c_str(), right_term.c_str(), row);
    return (RowType::target);
}

static bool extractDependency(std::vector<Dependency> *deps, sing::map<std::string, int32_t> *base2idx, const char *right, int32_t row)
{
    // extract rule and dependee
    std::string rule;
    std::string dependee;
    std::string temp;
    if (!sing::splitAny(right, " \t", &rule, &temp)) {
        return (false);
    }
    sing::cutLeadingSpaces(&temp);
    if (!sing::splitAny(temp.c_str(), " \t", &dependee, &temp)) {
        dependee = temp;
    }
    dependee = sing::pathFix(dependee.c_str());

    // detect the type of dependency and cut the path prefix
    DepType rtype = DepType::sing2tmp;
    if (sing::hasPrefix(dependee.c_str(), "$sing/") && sing::hasSuffix(dependee.c_str(), ".sing")) {
        sing::cutPrefix(&dependee, "$sing/");
        rtype = DepType::sing2tmp;
    } else if (sing::hasPrefix(dependee.c_str(), "$temp/") && sing::hasSuffix(dependee.c_str(), ".cpp")) {
        sing::cutPrefix(&dependee, "$temp/");
        rtype = DepType::tmp2obj;
    } else if (sing::hasPrefix(dependee.c_str(), "$cpp/")) {
        sing::cutPrefix(&dependee, "$cpp/");
        rtype = DepType::src2obj;
    } else {
        return (false);
    }

    // get the path components
    Dependency dep;
    sing::pathSplit(dependee.c_str(), &temp, &dep.path_, &dep.base_, &dep.ext_);

    // sing2tmp and tmp2obj have 2 rules, is the record already there ?
    if (rtype != DepType::src2obj) {
        int32_t idx = (*base2idx).get_safe(dep.base_.c_str(), -1);
        if (idx == -1) {
            idx = (*deps).size();
            (*deps).push_back(dep);
            (*base2idx).insert(dep.base_.c_str(), idx);
        }
        if (rtype == DepType::sing2tmp) {
            (*deps).at(idx).sing2tmp_row_ = row;
            (*deps).at(idx).sing2tmp_rule_ = rule;
        } else {
            (*deps).at(idx).x2obj_row_ = row;
            (*deps).at(idx).x2obj_rule_ = rule;
        }
    } else {
        (*base2idx).insert(dep.base_.c_str(), (*deps).size());
        dep.x2obj_row_ = row;
        dep.x2obj_rule_ = rule;
        (*deps).push_back(dep);
    }
    return (true);
}

static void extractTarget(Target *target, const char *left, const char *right, int32_t row)
{
    (*target).basic_dep_ = sing::s_format("%s%s", left, ":");

    std::string temp = right;
    std::string element;
    std::string discard;
    bool first = true;
    while (sing::len(temp.c_str()) > 0 && !sing::hasPrefix(temp.c_str(), "#")) {
        if (!sing::splitAny(temp.c_str(), " \t", &element, &temp)) {
            element = temp;
            temp = "";
        }
        sing::cutLeadingSpaces(&temp);
        if (first) {
            // the rule
            (*target).basic_dep_ += " " + element;
            first = false;
        } else if (sing::hasSuffix(element.c_str(), ".o")) {
            sing::rsplit(element.c_str(), ".", &element, &discard);
            sing::rsplitAny(element.c_str(), "/\\", &discard, &element);
            (*target).objs_base_.push_back(element.c_str());
        } else {
            // a dependee which is not a .o
            (*target).extras_ += " " + element;
        }
    }
    (*target).row_ = row;
}

static std::string sing2tempSynth(const char *path, const char *base, const char *rule, int32_t max_field_len)
{
    const std::string path_and_base = sing::s_format("%s%s", path, base);
    const std::string left = sing::s_format("%s%s%s%s%s", "build $temp/", path_and_base.c_str(), ".h | $temp/", path_and_base.c_str(), ".cpp");

    // add the extra characters 'left' would have if path_and_base was max_field_len long
    const int32_t left_adj = sing::numchars(left.c_str()) + (max_field_len - sing::numchars(path_and_base.c_str())) * 2;

    return (sing::s_format("%s%s%s%s%s%s%s", sing::formatString(left.c_str(), left_adj, sing::f_align_left).c_str(), " : ", rule, " $sing/",
        path_and_base.c_str(), ".sing", "\r\n"));
}

static std::string temp2objSynth(const char *path, const char *base, const char *rule, int32_t max_field_len)
{
    const std::string left = sing::s_format("%s%s%s", "build ", base, ".o");

    // add the extra characters 'left' would have if path_and_base was max_field_len long
    const int32_t left_adj = sing::numchars(left.c_str()) + max_field_len - sing::numchars(base);

    return (sing::s_format("%s%s%s%s%s%s%s%s", sing::formatString(left.c_str(), left_adj, sing::f_align_left).c_str(), " : ", rule, " $temp/", path, base,
        ".cpp", "\r\n"));
}

static std::string cpp2objSynth(const char *path, const char *base, const char *ext, const char *rule, int32_t max_field_len)
{
    const std::string left = sing::s_format("%s%s%s", "build ", base, ".o");

    // add the extra characters 'left' would have if path_and_base was max_field_len long
    const int32_t left_adj = sing::numchars(left.c_str()) + max_field_len - sing::numchars(base);

    return (sing::s_format("%s%s%s%s%s%s%s%s%s", sing::formatString(left.c_str(), left_adj, sing::f_align_left).c_str(), " : ", rule, " $cpp/", path, base,
        ".", ext, "\r\n"));
}

static std::string targetSynth(const Target &tar)
{
    std::string line = tar.basic_dep_;
    for(auto &obj : tar.objs_base_) {
        line += sing::s_format("%s%s%s", " ", obj.c_str(), ".o");
    }
    line += tar.extras_;
    return (line + "\r\n");
}
