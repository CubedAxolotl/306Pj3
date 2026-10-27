// src/tools/workspace.cpp — YOU IMPLEMENT (spec §4.4).
//
// Workspace::resolve: a purely lexical check, steps 1–6 of the spec.
// Do not touch the filesystem here (no exists(), canonical(), ...).
// The constructor, root() and check_symlinks() are provided in
// workspace_provided.cpp.

#include "tools/workspace.h"

namespace fs = std::filesystem;

bool Workspace::resolve(std::string_view user_path, fs::path* out, std::string* reason) const {
    if (user_path.empty()){// check for a empty path
        *reason = "Empty path";
        return false;
    }

    if( user_path.find('\0')!= std::string_view::npos){// check for NULL in Userpath
        *reason = "path contains NULL";
        return false;
    }

    fs::path p{std::string(user_path)};
    
    if (p.is_absolute()|| p.has_root_path()){//check for absolute paths
        *reason = "path is absolute";
        return false;
    }
        
    const fs::path n = p.lexically_normal();

    if (!n.empty()&& *n.begin()==".."){
        *reason = "path escapes workspace";
        return false;
    }

    if(n=="."){
        *out = root();

    }else {
        *out=root()/n;
    }
    return true;
}
