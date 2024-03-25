#ifndef _UTILS_COMMON_H_
#define _UTILS_COMMON_H_

#include <string>


#define getEnvOrEmptyString(VARNAME) std::getenv(VARNAME) == NULL ? "" : std::getenv(VARNAME)

// https://stackoverflow.com/a/2072890
inline bool ends_with(std::string const & value, std::string const & ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

inline bool areModulesEquivalent (const std::string &a, const std::string &b) {
    return (a == b) || 
        (a.size() > 0 && b.size() > 0 && a[0] == '/' && b[0] != '/' && ends_with(a, "/" + b)) ||
        (a.size() > 0 && b.size() > 0 && b[0] == '/' && a[0] != '/' && ends_with(b, "/" + a));
}


// compares [modA:]funA against [modB:]funB
inline bool areModuleColonFunctionNamesEquivalent (const std::string &a, const std::string &b) {
    std::string aMod = "", aFunc = a;
    std::string bMod = "", bFunc = b;
    bool sameMod = true;

    if (a.find(':') != std::string::npos) {
        aMod = a.substr(0, a.find(':'));
        aFunc = a.substr(a.find(':') + 1);
    }
    if (b.find(':') != std::string::npos) {
        bMod = b.substr(0, b.find(':'));
        bFunc = b.substr(b.find(':') + 1);
    }

    if (aMod.size() > 0 && bMod.size() > 0) {
        sameMod = areModulesEquivalent(aMod, bMod);
    }

    return sameMod && (aFunc == bFunc);
}

#endif // _UTILS_COMMON_H_
