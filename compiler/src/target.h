#ifndef SING_TARGET_H
#define SING_TARGET_H

namespace SingNames {

static const int KPointerSize = sizeof(void*);  // x86

//const char *get_cwd(void);
//int get_drive(void);
bool is_same_file(const char *p0, const char *p1);

// based on platform case sensstive or insensitive
bool is_same_filename(const char *name1, const char *name2);

} // namespace

#endif
