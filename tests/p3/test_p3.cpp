

#include "../mini_test.h"
#include "../test_helpers.h"     // TempDir

#include "tools/calculator.h"
#include "tools/tool_registry.h"
#include "tools/workspace.h"

#include <algorithm>
#include <cstdio>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace fs = std::filesystem;

//  Helpers

namespace {

// A tool with customizable info
class NamedTool : public Tool {
public:
    explicit NamedTool(std::string n) : n_(std::move(n)), schema_(JsonValue::object()) {}
    std::string name() const override { return n_; }
    std::string description() const override { return "test tool"; }
    const JsonValue& schema() const override { return schema_; }
    ToolResult invoke(const JsonValue&, ToolContext&) override { return ToolResult::success(n_); }
private:
    std::string n_;
    JsonValue   schema_;   //  empty JSON object
};

// "tool_0", "tool_1", ... "tool_9999"
std::string tool_name(int i) {
    return "tool_" + std::to_string(i);
}

// The WEAK hash from the spec, used only for comparison: first char + length.
std::size_t weak_hash(const std::string& s) {
    return static_cast<unsigned char>(s[0]) + s.size();
}

// Checks that input is accepted and resolves to expected
void check_accept(const Workspace& ws, const std::string& input, const fs::path& expected) {
    fs::path out;
    std::string reason;

    bool ok = ws.resolve(input, &out, &reason);

    CHECK(ok);
    CHECK(out == expected);
    if (!ok || out != expected) {
        std::printf("    (input was \"%s\")\n", input.c_str());
    }
}

// Checks that input is rejected with a non-empty reason.
void check_reject(const Workspace& ws, const std::string& input) {
    fs::path out;
    std::string reason;

    bool ok = ws.resolve(input, &out, &reason);

    CHECK(!ok);
    CHECK(!reason.empty());
    if (ok || reason.empty()) {
        std::printf("    (input was \"%s\")\n", input.c_str());
    }
}

// Tokenize with the provided tokenizer, then run calc_evaluate.
bool eval(const std::string& expr, double* value, std::string* error) {
    std::vector<CalcToken> tokens;
    if (!calc_tokenize(expr, &tokens, error)) {
        return false;
    }
    return calc_evaluate(tokens, value, error);
}

// Checks that expr evaluates to expected.
void check_calc(const std::string& expr, double expected) {
    double value = 0;
    std::string error;

    bool ok = eval(expr, &value, &error);

    CHECK(ok);
    CHECK(value == expected);
    if (!ok || value != expected) {
        std::printf("    (\"%s\" gave %g, error \"%s\")\n", expr.c_str(), value, error.c_str());
    }
}

// Checks that expr fails with exactly division by zero.
void check_div_by_zero(const std::string& expr) {
    double value = 0;
    std::string error;

    bool ok = eval(expr, &value, &error);

    CHECK(!ok);
    CHECK(error == "division by zero");
}

}  // namespace

//  REGISTRY 

// 1. A new registry is empty.
TEST(reg_empty_table) {
    ToolRegistry r;

    CHECK(r.size() == 0);
    CHECK(r.bucket_count() >= 1);
    CHECK(r.bucket_count() <= 64);
    CHECK(r.load_factor() == 0.0);
    CHECK(r.max_probe_length() == 0);
    CHECK(r.find("anything") == nullptr);
}

// 2. add() stores the tool, find() returns that same tool, and find() of a name that was never added returns nullptr.
TEST(reg_add_and_find) {
    ToolRegistry r;
    auto t = std::make_unique<NamedTool>("alpha");
    Tool* raw = t.get();                 // remember the address before moving

    REQUIRE(r.add(std::move(t)));

    CHECK(r.size() == 1);
    CHECK(r.find("alpha") == raw);       // same object
    CHECK(r.find("beta") == nullptr);    // never added
    CHECK(r.find("alph") == nullptr);    // a prefix is not a match
}

// 3. A duplicate name is refused and the ORIGINAL tool is kept.
TEST(reg_duplicate_keeps_original) {
    ToolRegistry r;
    auto first = std::make_unique<NamedTool>("alpha");
    Tool* original = first.get();
    REQUIRE(r.add(std::move(first)));

    bool added_again = r.add(std::make_unique<NamedTool>("alpha"));

    CHECK(!added_again);                 // duplicate is refused
    CHECK(r.find("alpha") == original);  //check if the first tool is still the one stored
    CHECK(r.size() == 1);
}

// 4. nullptr is refused, and size() only counts real adds.
TEST(reg_null_rejected) {
    ToolRegistry r;

    CHECK(!r.add(nullptr));
    CHECK(r.size() == 0);

    r.add(std::make_unique<NamedTool>("alpha"));
    CHECK(r.size() == 1);

    CHECK(!r.add(nullptr));
    CHECK(r.size() == 1);
}

// 5. for_each visits every tool exactly once.
TEST(reg_for_each_visits_each_once) {
    ToolRegistry r;
    r.add(std::make_unique<NamedTool>("read_file"));
    r.add(std::make_unique<NamedTool>("write_file"));
    r.add(std::make_unique<NamedTool>("list_dir"));
    r.add(std::make_unique<NamedTool>("calculator"));

    // Collect every name for_each gives us.
    std::vector<std::string> seen;
    r.for_each([&](const Tool& tool) {
        seen.push_back(tool.name());
    });

    // Sort so the order for_each used doesn't matter.
    std::sort(seen.begin(), seen.end());
    std::vector<std::string> expected = {"calculator", "list_dir", "read_file", "write_file"};

    CHECK(seen == expected);             // each tool exactly once, none missing
}

// 6. 10,000 tools. Everything is still findable, pointers survive resizes, the load factor never goes above 1.0, and chains stay short.
TEST(reg_10000_tools) {
    ToolRegistry r;
    Tool* first = nullptr;

    for (int i = 0; i < 10000; ++i) {
        REQUIRE(r.add(std::make_unique<NamedTool>(tool_name(i))));

        if (i == 0) {
            first = r.find("tool_0");    // remember the very first tool's address
        }

        CHECK(r.load_factor() <= 1.0);   // must hold after everu add
    }

    // Every name can be found.
    for (int i = 0; i < 10000; ++i) {
        CHECK(r.find(tool_name(i)) != nullptr);
    }

    // The first pointer survived all the resizes.
    CHECK(r.find("tool_0") == first);

    // Worst-case lookup stays small.
    CHECK(r.max_probe_length() <= 64);
    CHECK(r.size() == 10000);

    std::printf("  bucket_count     = %zu\n", r.bucket_count());
    std::printf("  load_factor      = %.3f\n", r.load_factor());
    std::printf("  max_probe_length = %zu\n", r.max_probe_length());
}

// 7. compare my hash with the weak hash s[0] + s.size().
TEST(reg_alternative_hash) {
    // Fill a real registry so we know its final bucket count.
    ToolRegistry r;
    for (int i = 0; i < 10000; ++i) {
        r.add(std::make_unique<NamedTool>(tool_name(i)));
    }
    std::size_t buckets = r.bucket_count();

    // Count how many names the weak hash would put in each bucket.
    std::vector<int> counts(buckets, 0);
    for (int i = 0; i < 10000; ++i) {
        std::size_t index = weak_hash(tool_name(i)) % buckets;
        counts[index] += 1;
    }
    int weak_longest = *std::max_element(counts.begin(), counts.end());

    // How many different values does the hash give
    std::set<std::size_t> distinct;
    for (int i = 0; i < 10000; ++i) {
        distinct.insert(ToolRegistry::hash(tool_name(i)));
    }

    std::printf("  buckets: %zu\n", buckets);
    std::printf("  FNV-1a longest chain:          %zu\n", r.max_probe_length());
    std::printf("  s[0] + s.size() longest chain: %d\n", weak_longest);
    std::printf("  distinct FNV-1a values:        %zu / 10000\n", distinct.size());

    CHECK(r.max_probe_length() < static_cast<std::size_t>(weak_longest));
    CHECK(distinct.size() >= 9990);
    CHECK(ToolRegistry::hash("ab") != ToolRegistry::hash("ba"));   // order matters
}


//  WORKSPACE 

// 8. Ordinary relative paths are accepted.
TEST(ws_accepts_normal_paths) {
    TempDir d;
    Workspace ws(d.path());

    check_accept(ws, "notes/todo.txt",    ws.root() / "notes" / "todo.txt");
    check_accept(ws, "./notes//todo.txt", ws.root() / "notes" / "todo.txt");
    check_accept(ws, "new/dir/file.txt",  ws.root() / "new" / "dir" / "file.txt");   // doesn't exist yet
    check_accept(ws, "my..notes.txt",     ws.root() / "my..notes.txt");              // ".." inside a name
}

// 9. "." and paths that go in and back out still land inside root.
TEST(ws_accepts_dot_and_folding) {
    TempDir d;
    Workspace ws(d.path());

    check_accept(ws, ".",      ws.root());
    check_accept(ws, "a/../b", ws.root() / "b");
    check_accept(ws, "a/..",   ws.root());
}

// 10. Empty, NUL-containing, and absolute paths are rejected.
TEST(ws_rejects_bad_input) {
    TempDir d;
    Workspace ws(d.path());

    // a "notes\0/x" literal would be cut off at the NUL.
    std::string with_nul = std::string("notes") + '\0' + "/x";

    check_reject(ws, "");
    check_reject(ws, with_nul);
    check_reject(ws, "/etc/passwd");
}

// 11. Anything that climbs above root is rejected.
TEST(ws_rejects_escapes) {
    TempDir d;
    Workspace ws(d.path());
    std::string root_name = ws.root().filename().string();

    check_reject(ws, "..");
    check_reject(ws, "../etc/passwd");
    check_reject(ws, "a/../../b");
    check_reject(ws, "notes/../../" + root_name + "/notes/todo.txt");
}


//  CALCULATOR  


// 12. * and / come first; equal operators go left to right.
TEST(calc_precedence_and_order) {
    check_calc("2 + 3 * 4",          14);
    check_calc("10 - 4 - 3",         3);
    check_calc("100 / 10 / 5",       2);
    check_calc("2 - 3 * 4 + 10 / 5", -8);   // all four operators
}

// 13. Parentheses group.
TEST(calc_parentheses) {
    check_calc("(2 + 3) * 4",         20);
    check_calc("2 * (3 + 4) - 5",     9);
    check_calc("((1 + 2) * (3 + 4))", 21);  //nested parentheses
}

// 14. Negative numbers and decimal results.
TEST(calc_negatives_and_decimals) {
    check_calc("-3 * -3", 9);
    check_calc("7 / 2",   3.5);
}

// 15. Dividing by zero is an error, not a crash.
TEST(calc_division_by_zero) {
    check_div_by_zero("1 / 0");
    check_div_by_zero("1 / (2 - 2)");
}

int main(int argc, char** argv) { return mt::run_all(argc, argv); }