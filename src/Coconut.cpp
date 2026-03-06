#include "gcc-plugin.h"
#include "plugin-version.h"
#include "tree.h"
#include "tree-cfg.h"  
#include "tree-cfg.h"      // label_to_block_fn, CASE_* accessors

#include "gimple.h"
#include "cp/cp-tree.h"
#include "tree-pass.h"
#include "context.h"
#include "tree-iterator.h"
#include "diagnostic.h"
#include "gimple-iterator.h"
#include "cgraph.h"
#include "function.h"
#include <cxxabi.h>
#include <iostream>
#include <cstring>
#include "cfgloop.h"
#include <assert.h>
#include "cfghooks.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <queue>
#include <tuple>
#include <string>
#include <queue>
#include <map>
#include "tree-dump.h"
#include <set>
#include "tree-pretty-print.h"
#include <fstream>
#include "basic-block.h"
#include "print-tree.h"
#include "dumpfile.h"
#include <set>
#include <sstream>

#include <cmath>      // for std::fabs
#include <bitset>
#include <optional>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <string>
#include "wide-int.h"   // for wide_int / wi::

#include "tree.h"
#include "cgraph.h"
#include "symtab.h"

#include <string>
#include <vector>

#include <cstdio>
#include <cstring>
#include <cstdlib>

 long depth_r =0;
// Put near your includes, once.
#include "tree.h"

// Portable wrapper: works on GCC 7 (needs precision) and newer (no precision).
#ifndef TO_WIDE
#  if defined(GCCPLUGIN_VERSION) && (GCCPLUGIN_VERSION < 8000)
// GCC 7.x plugin headers
#    define TO_WIDE(T_) wi::to_wide((T_), TYPE_PRECISION (TREE_TYPE (T_)))
#  else
// GCC 8+ (or anything that provides the 1-arg overload)
#    define TO_WIDE(T_) wi::to_wide((T_))
#  endif
#endif





int plugin_is_GPL_compatible = 1;

struct AnalysisContextKey {
    tree fn_decl;
    tree obj;
    int state;
    const char* alias_head;
    bool operator==(AnalysisContextKey const& o) const {
        return fn_decl==o.fn_decl
            && obj    ==o.obj
            && state  ==o.state
            && strcmp(alias_head,o.alias_head)==0;
    }
};
namespace std {
    template<> struct hash<AnalysisContextKey> {
        size_t operator()(AnalysisContextKey const& k) const noexcept {
            return (size_t)k.fn_decl
                 ^ (size_t)k.obj
                 ^ hash<int>()(k.state)
                 ^ hash<const char*>()(k.alias_head);
        }
    };
}

using ParamMap  = std::unordered_map<tree,tree>;
using WorkItem  = std::tuple<tree, ParamMap, AnalysisContextKey>;

static std::unordered_set<AnalysisContextKey> analyzed_contexts;
static std::queue<WorkItem>               work_queue;
static std::map<int, tree> EnumToClassMap;

std::unordered_set<std::string> g_recursive_method_transitions;
static std::map<int, std::vector<std::pair<std::string, int>>> Typestate_Rules;
static std::vector<std::string> TypestateClassConnector_args;
std::unordered_map<tree, std::map<std::string, int>> FSM_by_Class;

std::queue<std::pair<tree, std::unordered_map<tree, tree>>> functions_to_Analyse_with_args;
std::unordered_set<tree> functions_Analysed;
struct CallContext {
     std::unordered_map<tree, tree> alias_map;  // Local alias map for the function
};
std::vector<CallContext> call_stack;  
std::unordered_set<tree> function_params;
std::unordered_set<tree> class_member_vars;
//std::unordered_map<tree, tree> alias_map ;
std::unordered_map<tree, tree> global_alias_map;
std::unordered_map<tree, std::unordered_set<tree>> alias_map;
struct BranchRule {
    std::string method;
    int cond;   // True/False/Undefined as int
    int next;
};

static std::map<int, std::vector<BranchRule>> Branch_Rules;


static std::unordered_set<tree> g_recursive_fn_decls;

// Which transitions are recursive (state self-loops)
struct RecursiveTransitionInfo {
    std::string method;   // function name, e.g. "AdjustActuators"
    int state;            // state id where it self-loops
};

// Filled by validate_wcet_vs_timed_rules when it sees self-loops
extern std::vector<RecursiveTransitionInfo> g_recursive_transitions;

// Fixed recursion bounds per function (filled from main or config)
extern std::unordered_map<std::string, int> g_recursion_bound_by_method;


std::vector<RecursiveTransitionInfo> g_recursive_transitions;
std::unordered_map<std::string, int> g_recursion_bound_by_method;

static std::unordered_map<gimple*, int> condition_id_map;
static int next_if_else_id = 0;
std::map<int, bool> if_else_status;
std::map<int, std::pair<int,int>> if_else_context_map;
static std::vector<std::tuple<tree, gimple*, tree>> deferred_branch_statements;
static std::unordered_map<tree, int> branch_context_map;  
static bool is_new_branch = true;                        
static int current_branch_id = -1;  
static int branch_counter = 0;    
int current_if_else_id = 0;  
std::set<int> joint_states;
std::map<int, bool> branch_context_status;    
static int last_finalized_if_else_id = -1;
static int last_finalized_first_state = -1;

static int g_cycle_n = -1;   // Cycle<N> -> N (e.g. 20)
// --- Helpers to read class-template name and its template args
static const char* class_template_name(tree ty) {
    if (!ty || TREE_CODE(ty) != RECORD_TYPE) return nullptr;
    tree td = TYPE_NAME(ty);
    if (!td || TREE_CODE(td) != TYPE_DECL) return nullptr;
    tree id = DECL_NAME(td);
    return id ? IDENTIFIER_POINTER(id) : nullptr;
}

static tree class_template_args(tree ty) {
    if (!ty || TREE_CODE(ty) != RECORD_TYPE) return nullptr;
    if (!TYPE_TEMPLATE_INFO(ty)) return nullptr;
    return TI_ARGS(TYPE_TEMPLATE_INFO(ty)); // TREE_VEC
}


// Per-edge timing info
struct TimedInfo {
    long long period_ns  = 0;   // period
    long long offest_ns=0;
    long long U_deadline_ns= 0;   // upper duratio (relative)
    long long L_deadline_ns= 0;   // lower duration (relative)
    int criticality      = 0;   // enum integral value

   // long long jitter_ns  = 0;   // optional last param
};

// from_state -> [(method, next_state, timing)]
static std::map<int,
    std::vector<std::tuple<std::string,int,TimedInfo>>
> Timed_Typestate_Rules;




static bool as_sll(tree t, long long &out) {
    if (!t || TREE_CODE(t) != INTEGER_CST) return false;
    out = (long long)TREE_INT_CST_LOW(t);
    return true;
}

static tree get_template_args_from_type(tree ty) {
    if (!ty) return nullptr;
    if (TREE_CODE(ty) == RECORD_TYPE && TYPE_TEMPLATE_INFO(ty))
        return TI_ARGS(TYPE_TEMPLATE_INFO(ty));
    return nullptr;
}

// TimeGuard<Release, Period, Deadline, Criticality, Jitter?>
static bool parse_timeguard(tree tg, TimedInfo &ti) {
    tree args = nullptr;

    // TimeGuard is a template-id type; get its args
    if (TYPE_P(tg)) args = get_template_args_from_type(tg);
    if (!args || TREE_CODE(args) != TREE_VEC) return false;

    const int n = TREE_VEC_LENGTH(args);
    if (n < 4) return false;

    long long  p=0, d=0, c=0, j=0 , ld=0;
    tree a0 = TREE_VEC_ELT(args,0);
    tree a1 = TREE_VEC_ELT(args,1);
    tree a2 = TREE_VEC_ELT(args,2);
    tree a3 = TREE_VEC_ELT(args,3);
    tree a4 = TREE_VEC_ELT(args,4); 
   // tree a4 = (n >= 5 ? TREE_VEC_ELT(args,4) : nullptr);

    as_sll(a0, p);
    as_sll(a2,ld);
    as_sll(a3, d);
   // as_sll(a2, c);
    //if (a4) as_sll(a4, j);

   
    if (TREE_CODE(a4) == INTEGER_CST) {
        c = (int)TREE_INT_CST_LOW(a4 );
    } else if (TREE_CODE(a4) == CONST_DECL) {
        tree v = DECL_INITIAL(a4);
        if (v && TREE_CODE(v) == INTEGER_CST) c = (int)TREE_INT_CST_LOW(v);
    }

  
    ti.period_ns   = p;
    ti.U_deadline_ns = d;
    ti.L_deadline_ns = ld;
    ti.criticality = c;
//    ti.jitter_ns   = j;
    return true;
}


static inline std::string unqualified_name(const std::string& demangled) {
    auto pos = demangled.rfind("::");
    return (pos == std::string::npos) ? demangled : demangled.substr(pos + 2);
}

// Timed_State<STATE, &Class::method, C<>, TimeGuard<...>, NEXT>

static void process_timed_typestate_args(tree targs) {
    if (!targs || TREE_CODE(targs) != TREE_VEC || TREE_VEC_LENGTH(targs) < 4) return;

    int from = -1, to = -1 ;
    std::string method_name;
    TimedInfo tinfo{};
    tree class_tree = nullptr;

    // 0) from-state enum value
    tree a0 = TREE_VEC_ELT(targs, 0);
    if (a0 && TREE_CODE(a0) == INTEGER_CST)
        from = (int)TREE_INT_CST_LOW(a0);

    // 1) &Class::method
    tree a1 = TREE_VEC_ELT(targs, 1);
    if (a1 && TREE_CODE(a1) == PTRMEM_CST) {
        tree member = PTRMEM_CST_MEMBER(a1);
        if (member && TREE_CODE(member) == FUNCTION_DECL) {
            class_tree = DECL_CONTEXT(member);
            if (DECL_NAME(member)) {
                const char* nm = IDENTIFIER_POINTER(DECL_NAME(member));
                if (nm) method_name = nm;
            }
        }
    }
   
    // 2) TimeGuard<...>
    tree a2 = TREE_VEC_ELT(targs, 2);
    parse_timeguard(a2, tinfo); // best-effort; 0s mean "unspecified"

    // 3) to-state enum value
    tree a3 = TREE_VEC_ELT(targs, 3);
    if (a3 && TREE_CODE(a3) == INTEGER_CST)
        to = (int)TREE_INT_CST_LOW(a3);

    if (from != -1 && !method_name.empty() && to != -1) {
        Timed_Typestate_Rules[from].emplace_back(method_name, to, tinfo);

        // (optional) keep your per-class FSM map in sync
        if (class_tree && TREE_CODE(class_tree) == RECORD_TYPE) {
            FSM_by_Class[class_tree][method_name] = to;
        }

        // (optional) map enum value → class 
        EnumToClassMap[from] = class_tree;
    }
}









static void print_timed_typestate_rules() {

    printf("Timed Typestate Rules\n");
    for (const auto &[from, edges] : Timed_Typestate_Rules) {
        printf("From state %d:\n", from);

        for (const auto &edge : edges) {
            const std::string &method = std::get<0>(edge);
            int to = std::get<1>(edge);
            const TimedInfo &ti = std::get<2>(edge);

            printf("  --[%s]--> %d\n", method.c_str(), to);
            printf("     Timing: Period=%lld, Offest=%lld, L_deadline=%lld, Upper_deadline=%lld, Criticality=%d\n",
                   ti.period_ns, ti.offest_ns, ti.L_deadline_ns, ti.U_deadline_ns, ti.criticality);
        }
    }

    
}






















bool is_subclass(tree subclass_tree, tree superclass_tree) {
    if (subclass_tree == superclass_tree) {
        return false;  // A class is not a subclass of itself
    }

    tree subclass_binfo = TYPE_BINFO(subclass_tree);
    if (!subclass_binfo) {
        return false;  // No base information, cannot be a subclass
    }

    // Iterate over all base classes using BINFO_BASE_ITERATE
    for (int i = 0; i < BINFO_N_BASE_BINFOS(subclass_binfo); i++) {
        tree base = BINFO_BASE_BINFO(subclass_binfo, i);
        tree base_type = BINFO_TYPE(base);
        if (base_type == superclass_tree) {
            return true;  // Found superclass in the inheritance chain
        }
    }

    return false;
}





/**
 * This following functions are responsible for extracting typestate rules from Typestate_Templates and saving them for validation.
 * They extract the arguments of TypestateClassConnector to tag the type that is subject to typestate validation.
 * Lastly, it visualises the typestate rules in the form of an FSM using Graphviz.
 */

static std::map<int, std::vector<std::pair<std::string, int>>> Subtype_Typestate_Rules;


void process_typestate_template_args(tree tmpl_args) {
    int num_args = TREE_VEC_LENGTH(tmpl_args);
    int current_enum_value = -1;
    std::string enum_type_name;
    tree arg_type = nullptr;

    // First, extract the current enum value and its type name
    if (num_args >= 1) {
        tree arg = TREE_VEC_ELT(tmpl_args, 0);
        if (TREE_CODE(arg) == INTEGER_CST) {
            arg_type = TREE_TYPE(arg);
            if (arg_type && TREE_CODE(arg_type) == ENUMERAL_TYPE) {
                HOST_WIDE_INT arg_val = TREE_INT_CST_LOW(arg);
                current_enum_value = static_cast<int>(arg_val);

                if (TYPE_NAME(arg_type)) {
                    const char* type_name_cstr = IDENTIFIER_POINTER(DECL_NAME(TYPE_NAME(arg_type)));
                    enum_type_name = type_name_cstr ? type_name_cstr : "<unknown_enum_type>";
                } else {
                    enum_type_name = "<unknown_enum_type>";
                }
            }
        }
    }

    if (current_enum_value == -1 || enum_type_name.empty()) {
        return;  // Exit if enum extraction failed
    }

    std::vector<std::pair<std::string, int>> transitions;
    tree class_tree = nullptr;  // To store the class type of the current function

    for (int i = 1; i + 1 < num_args; i += 2) {
        std::string function_name;
        int next_enum_value = -1;
        tree func_arg = TREE_VEC_ELT(tmpl_args, i);

        // Extract the class name from the function pointer
        if (TREE_CODE(func_arg) == PTRMEM_CST) {
            tree member = PTRMEM_CST_MEMBER(func_arg);
            if (TREE_CODE(member) == FUNCTION_DECL) {
                class_tree = DECL_CONTEXT(member);  // Get the class type
                const char* func_name = (DECL_NAME(member)) ? IDENTIFIER_POINTER(DECL_NAME(member)) : "<unnamed_function>";
                function_name = func_name;
            }
        }

        // Extract next enum value
        tree next_state_arg = TREE_VEC_ELT(tmpl_args, i + 1);
        if (TREE_CODE(next_state_arg) == INTEGER_CST) {
            tree next_arg_type = TREE_TYPE(next_state_arg);
            if (next_arg_type && TREE_CODE(next_arg_type) == ENUMERAL_TYPE) {
                HOST_WIDE_INT arg_val = TREE_INT_CST_LOW(next_state_arg);
                next_enum_value = static_cast<int>(arg_val);
            }
        }

        if (!function_name.empty() && next_enum_value != -1) {
            transitions.emplace_back(function_name, next_enum_value);
        }
    }

    // Store the class-tree mapping for the current enum value
    EnumToClassMap[current_enum_value] = class_tree;

    // Append transitions to the appropriate map
    Typestate_Rules[current_enum_value].insert(
        Typestate_Rules[current_enum_value].end(),
        transitions.begin(),
        transitions.end());

    //  Also track FSM by class tree for LSP checking
    if (class_tree && TREE_CODE(class_tree) == RECORD_TYPE) {
        for (const auto& [method_name, next_state] : transitions) {
            if (!method_name.empty()) {
                FSM_by_Class[class_tree][method_name] = next_state;
            }
        }
    }
}


static inline tree canon_type(tree t) {
    return t ? TYPE_MAIN_VARIANT(t) : t;
}


// ADD: pragma support + small containers
#include "c-family/c-pragma.h"
#include <deque>
#include <string>
static std::deque<std::string> g_wcet_pending;

static inline bool is_fn_definition(tree fndecl){
    return fndecl && TREE_CODE(fndecl)==FUNCTION_DECL
           && DECL_INITIAL(fndecl) && !DECL_EXTERNAL(fndecl);
}
static inline bool is_from_system_header(tree t){
    return in_system_header_at(DECL_SOURCE_LOCATION(t));
}
static inline bool is_artificial(tree fndecl){
    return DECL_ARTIFICIAL(fndecl);
}

static std::string qualified_name_for_decl(tree fndecl){
    if (!fndecl || TREE_CODE(fndecl)!=FUNCTION_DECL) return "<fn>";
    const char* base = DECL_NAME(fndecl) ? IDENTIFIER_POINTER(DECL_NAME(fndecl)) : "<fn>";
    std::string name = base ? base : "<fn>";
    tree ctx = DECL_CONTEXT(fndecl);
    if (ctx && TREE_CODE(ctx)==RECORD_TYPE) {
        tree tn = TYPE_NAME(ctx);
        if (tn && TREE_CODE(tn)==TYPE_DECL) {
            tree id = DECL_NAME(tn);
            if (id) name = std::string(IDENTIFIER_POINTER(id)) + "::" + name;
        }
    }
    return name;
}

// Parse fields out of the payload string (simple, tolerant)
struct AnnotRow {
    std::string qualified, file, method;
    int line = 0;
    double wcet_ms = 0.0;
    long long cycles=0, us_ceil=0, f_cpu=0;
};
static bool parse_double_field(const char* s, const char* key, double& out){
    const char* p = strstr(s, key); if (!p) return false;
    p += strlen(key); while (*p==' '||*p=='\t') ++p;
    char* e=nullptr; double v=strtod(p,&e); if (e==p) return false; out=v; return true;
}
static bool parse_ll_field(const char* s, const char* key, long long& out){
    const char* p = strstr(s, key); if (!p) return false;
    p += strlen(key); while (*p==' '||*p=='\t') ++p;
    char* e=nullptr; long long v=strtoll(p,&e,10); if (e==p) return false; out=v; return true;
}
static bool parse_payload(const char* s, AnnotRow& row){
    if (!s) return false;
    bool ok = parse_double_field(s, "wcet_ms_exact=", row.wcet_ms);
    parse_ll_field(s, "cycles=",  row.cycles);
    parse_ll_field(s, "us_ceil=", row.us_ceil);
    parse_ll_field(s, "F_CPU=",   row.f_cpu);
    return ok;
}


static std::vector<AnnotRow> g_rows;
static std::unordered_map<const void*, std::unordered_map<std::string, double>> g_annot_wcet_ms; // class -> method -> ms
static std::vector<AnnotRow> g_annot_rows; // for printing a list

// ADD if not present
#include <cmath>

static inline const void* canon_key_from_record(tree t){
    return (t && TREE_CODE(t) == RECORD_TYPE) ? (const void*) TYPE_MAIN_VARIANT(t) : (const void*) t;
}

static void record_wcet_row(const void* class_key_canon_in,
                            const std::string& method,
                            tree fndecl,
                            double ms)
{
    // Canonicalise (safe even if already canonical)
    const void* class_key_canon = class_key_canon_in;

    // If caller passed a raw RECORD_TYPE, canonicalise here too:
    if (!class_key_canon && fndecl) {
        tree ctx = DECL_CONTEXT(fndecl);
        if (ctx && TREE_CODE(ctx) == RECORD_TYPE)
            class_key_canon = (const void*) TYPE_MAIN_VARIANT(ctx);
    }

    // 1) WRITE TO THE MAP  (this is what the validator reads)
    g_annot_wcet_ms[class_key_canon][method] = ms;

    // 2) Also keep a row for pretty printing
    AnnotRow row;
    row.wcet_ms  = ms;
    row.method   = method;
    row.qualified= qualified_name_for_decl(fndecl);
    if (fndecl) {
        location_t loc = DECL_SOURCE_LOCATION(fndecl);
        row.file = LOCATION_FILE(loc) ? LOCATION_FILE(loc) : "";
        row.line = LOCATION_LINE(loc);
    }
    g_rows.push_back(row);

    // Optional debug
   //  fprintf(stderr, "[wcet] attached %.6f ms to %s\n", ms, row.qualified.c_str());
}




// Parse "wcet_ms_exact=..." out of the payload string
static bool parse_wcet_ms_from_annot(const char* msg, double& out_ms) {
    if (!msg) return false;
    const char* p = strstr(msg, "wcet_ms_exact=");
    if (!p) return false;
    p += strlen("wcet_ms_exact=");
    while (*p == ' ' || *p == '\t') ++p;

    char* endp = nullptr;
    double v = strtod(p, &endp);
    if (endp == p) return false;

    out_ms = v;
    return true;
}


static void dump_attrs_for_fn(const char* tag, tree fndecl) {
    auto dump_list = [](tree attrs) {
        for (tree a = attrs; a; a = TREE_CHAIN(a)) {
            tree purpose = TREE_PURPOSE(a);
            const char* aname = (purpose && TREE_CODE(purpose) == IDENTIFIER_NODE)
                                ? IDENTIFIER_POINTER(purpose) : "<non-id>";
            fprintf(stderr, "    attr: %s\n", aname ? aname : "<null>");

            tree args = TREE_VALUE(a);
            if (!args) continue;

            // single value?
            if (TREE_CODE(args) == STRING_CST) {
                fprintf(stderr, "      arg[string]: \"%s\"\n", TREE_STRING_POINTER(args));
            } else if (TREE_CODE(args) == INTEGER_CST) {
                fprintf(stderr, "      arg[int]: %lld\n", (long long)TREE_INT_CST_LOW(args));
            } else if (TREE_CODE(args) == TREE_LIST) {
                for (tree it = args; it; it = TREE_CHAIN(it)) {
                    tree v = TREE_VALUE(it);
                    if (!v) continue;
                    if (TREE_CODE(v) == STRING_CST) {
                        fprintf(stderr, "      list[string]: \"%s\"\n", TREE_STRING_POINTER(v));
                    } else if (TREE_CODE(v) == INTEGER_CST) {
                        fprintf(stderr, "      list[int]: %lld\n", (long long)TREE_INT_CST_LOW(v));
                    } else {
                        fprintf(stderr, "      list[node code=%d]\n", (int)TREE_CODE(v));
                    }
                }
            } else {
                fprintf(stderr, "      args node code=%d\n", (int)TREE_CODE(args));
            }
        }
    };

    const char* nm = (DECL_NAME(fndecl) ? IDENTIFIER_POINTER(DECL_NAME(fndecl)) : "<noname>");
    fprintf(stderr, "[attrs:%s] %s\n", tag, nm);
    dump_list(DECL_ATTRIBUTES(fndecl));

    tree ft = TREE_TYPE(fndecl);
    if (ft) {
        fprintf(stderr, "[attrs:%s] %s (fn-type)\n", tag, nm);
        dump_list(TYPE_ATTRIBUTES(ft));
        tree rt = TREE_TYPE(ft);
        if (rt) {
            fprintf(stderr, "[attrs:%s] %s (ret-type)\n", tag, nm);
            dump_list(TYPE_ATTRIBUTES(rt));
        }
    }
}

static void wcet_on_finish_decl_wcet(void* data, void* /*user*/) {
       tree decl = static_cast<tree>(data);
    if (!decl) return;

    // 1) If this is our marker: VAR_DECL in section ".wcet_next" with STRING_CST init
    if (TREE_CODE(decl)==VAR_DECL) {
        const char* sec = DECL_SECTION_NAME(decl);
        if (sec && strcmp(sec, ".wcet_next")==0) {
            tree init = DECL_INITIAL(decl);
            if (init && TREE_CODE(init)==STRING_CST) {
                const char* s = TREE_STRING_POINTER(init);
                g_wcet_pending.emplace_back(s);
                fprintf(stderr, "[wcet] marker queued: \"%s\"\n", s);
            }
            return; // nothing else to do for VAR_DECL
        }
    }

    // 2) If this is a function definition, and we have a pending marker, attach it
    if (TREE_CODE(decl)==FUNCTION_DECL && !g_wcet_pending.empty()) {
        if (!is_fn_definition(decl))     return;  // skip prototypes
        if (is_from_system_header(decl)) return;  // skip libstdc++/builtins
        if (is_artificial(decl))         return;  // skip synthesized

        std::string payload = std::move(g_wcet_pending.front());
        g_wcet_pending.pop_front();

        AnnotRow row;
        if (!parse_payload(payload.c_str(), row)) {
            const char* nm = DECL_NAME(decl) ? IDENTIFIER_POINTER(DECL_NAME(decl)) : "<noname>";
            warning(0, "malformed wcet payload for %qs: %qs", nm, payload.c_str());
            return;
        }

        row.qualified = qualified_name_for_decl(decl);
        location_t loc = DECL_SOURCE_LOCATION(decl);
        row.file = LOCATION_FILE(loc) ? LOCATION_FILE(loc) : "";
        row.line = LOCATION_LINE(loc);
        row.method = DECL_NAME(decl) ? IDENTIFIER_POINTER(DECL_NAME(decl)) : "";

        // store/print (you probably already have a vector; reuse it)
 //   std::vector<AnnotRow> g_rows; // or keep it file-static if you prefer
        g_rows.push_back(row);

        fprintf(stderr, "[wcet] attached %.6f ms to %s @ %s:%d\n",
                row.wcet_ms, row.qualified.c_str(), row.file.c_str(), row.line);
    }
}



static void collect_annot_payload_strings(tree args, std::vector<std::string>& out) {
    if (!args) return;

    // Single literal e.g. __attribute__((annotate("...")))
    if (TREE_CODE(args) == STRING_CST) {
        out.emplace_back(TREE_STRING_POINTER(args));
        return;
    }

    // List of arguments
    if (TREE_CODE(args) == TREE_LIST) {
        for (tree it = args; it; it = TREE_CHAIN(it)) {
            tree v = TREE_VALUE(it);
            if (!v) continue;
            if (TREE_CODE(v) == STRING_CST) {
                out.emplace_back(TREE_STRING_POINTER(v));
            }
            // ignore non-string nodes
        }
    }
    // else: ignore other forms
}


static bool harvest_one_attr_list(tree attrs, const void* class_key_raw,
                                  const std::string& method, tree fndecl)
{
    if (!attrs) return false;

    tree class_key = (tree)class_key_raw;
    if (class_key) class_key = TYPE_MAIN_VARIANT(class_key);
    const void* class_key_canon = (const void*)class_key;

    bool found = false;
    for (tree a = attrs; a; a = TREE_CHAIN(a)) {
        tree purpose = TREE_PURPOSE(a);
        if (!purpose || TREE_CODE(purpose) != IDENTIFIER_NODE) continue;
        const char* aname = IDENTIFIER_POINTER(purpose);
        if (!aname) continue;
        if (strcmp(aname, "annotate") && strcmp(aname, "gnu::annotate")) continue;

        std::vector<std::string> payloads;
        collect_annot_payload_strings(TREE_VALUE(a), payloads); // << enabled

        for (const auto& s : payloads) {
            if (s.find("wcet_ms_exact=") == std::string::npos) continue;
            double ms = 0.0;
            if (!parse_wcet_ms_from_annot(s.c_str(), ms)) continue;
            record_wcet_row(class_key_canon, method, fndecl, ms);
            found = true;
        }
    }
    return found;
}


static bool harvest_from_class_declaration(tree class_type,
                                           const std::string& method,
                                           const void* class_key,
                                           tree fndecl_for_name_only)
{
    if (!class_type || TREE_CODE(class_type) != RECORD_TYPE) return false;
    // iterate class members (fields + methods)
    for (tree fld = TYPE_FIELDS(class_type); fld; fld = TREE_CHAIN(fld)) {
        if (TREE_CODE(fld) != FUNCTION_DECL) continue;
        if (!DECL_NAME(fld)) continue;

        const char* nm = IDENTIFIER_POINTER(DECL_NAME(fld));
        if (!nm) continue;

        if (method == nm) {
            // Found the member *declaration*; harvest its attributes
            if (harvest_one_attr_list(DECL_ATTRIBUTES(fld), class_key, method, fndecl_for_name_only))
                return true;
        }
    }
    return false;
}

static void harvest_annot_from_decl(tree fndecl) {
    if (!fndecl || TREE_CODE(fndecl) != FUNCTION_DECL || !DECL_NAME(fndecl)) return;

    std::string method = IDENTIFIER_POINTER(DECL_NAME(fndecl));
    tree ctx = DECL_CONTEXT(fndecl);
    tree class_type = (ctx && TREE_CODE(ctx) == RECORD_TYPE) ? TYPE_MAIN_VARIANT(ctx) : nullptr;
    const void* class_key = (const void*)class_type;

    // 1) attributes on the definition
    if (harvest_one_attr_list(DECL_ATTRIBUTES(fndecl), class_key, method, fndecl)) return;

    // 2) attributes on function type
    tree fn_type = TREE_TYPE(fndecl);
    if (fn_type && harvest_one_attr_list(TYPE_ATTRIBUTES(fn_type), class_key, method, fndecl)) return;

    // 3) attributes on return type
    if (fn_type) {
        tree ret_type = TREE_TYPE(fn_type);
        if (ret_type && harvest_one_attr_list(TYPE_ATTRIBUTES(ret_type), class_key, method, fndecl)) return;
    }

    //  4) attributes on the *member declaration* inside the class
    if (class_type) {
        if (harvest_from_class_declaration(class_type, method, class_key, fndecl)) return;
    }

    // Debug (optional)
   // fprintf(stderr, "[decl] %s: decl_attrs=%s\n",
        //    IDENTIFIER_POINTER(DECL_NAME(fndecl)),
         //   DECL_ATTRIBUTES(fndecl) ? "yes" : "no");
    //log_decl_annotate_values(fndecl);
}






static tree peel_tree(tree t) {
    while (t) {
        enum tree_code c = TREE_CODE(t);
        if (c == NOP_EXPR || c == CONVERT_EXPR || c == NON_LVALUE_EXPR) {
            t = TREE_OPERAND(t, 0);
            continue;
        }
        break;
    }
    return t;
}

static const char* safe_ident_ptr(tree id) {
    return (id && TREE_CODE(id) == IDENTIFIER_NODE) ? IDENTIFIER_POINTER(id) : NULL;
}

// Works whether TYPE_NAME is TYPE_DECL or IDENTIFIER_NODE.
static const char* type_base_name(tree ty) {
    if (!ty) return NULL;
    tree n = TYPE_NAME(ty);
    if (!n) return NULL;

    if (TREE_CODE(n) == TYPE_DECL) {
        tree dn = DECL_NAME(n);
        return dn ? IDENTIFIER_POINTER(dn) : NULL;
    }
    if (TREE_CODE(n) == IDENTIFIER_NODE) {
        return IDENTIFIER_POINTER(n);
    }
    return NULL;
}

// INTEGER_CST or enum const CONST_DECL -> DECL_INITIAL -> INTEGER_CST
static bool get_int_cst_value(tree t, int *out) {
    if (!out) return false;
    t = peel_tree(t);
    if (!t) return false;

    if (TREE_CODE(t) == INTEGER_CST) {
        *out = (int)TREE_INT_CST_LOW(t);
        return true;
    }

    if (TREE_CODE(t) == CONST_DECL) {
        tree init = peel_tree(DECL_INITIAL(t));
        if (init && TREE_CODE(init) == INTEGER_CST) {
            *out = (int)TREE_INT_CST_LOW(init);
            return true;
        }
    }

    return false;
}

// Robustly extract member function name from template arg:
// handles PTRMEM_CST, ADDR_EXPR(PTRMEM_CST...), OVERLOAD, BASELINK.
static const char* extract_method_name(tree t) {
    t = peel_tree(t);
    if (!t) return "unknown";

    if (TREE_CODE(t) == ADDR_EXPR) {
        t = peel_tree(TREE_OPERAND(t, 0));
        if (!t) return "unknown";
    }

    if (TREE_CODE(t) == PTRMEM_CST) {
        tree mem = peel_tree(PTRMEM_CST_MEMBER(t));
        if (!mem) return "unknown";

        if (TREE_CODE(mem) == FUNCTION_DECL) {
            tree dn = DECL_NAME(mem);
            return dn ? IDENTIFIER_POINTER(dn) : "unknown";
        }

        if (TREE_CODE(mem) == OVERLOAD) {
            tree fn = OVL_FUNCTION(mem); // first overload candidate
            if (fn && TREE_CODE(fn) == FUNCTION_DECL) {
                tree dn = DECL_NAME(fn);
                return dn ? IDENTIFIER_POINTER(dn) : "unknown";
            }
        }

        if (TREE_CODE(mem) == BASELINK) {
            tree fns = BASELINK_FUNCTIONS(mem);
            if (fns && TREE_CODE(fns) == OVERLOAD) {
                tree fn = OVL_FUNCTION(fns);
                if (fn && TREE_CODE(fn) == FUNCTION_DECL) {
                    tree dn = DECL_NAME(fn);
                    return dn ? IDENTIFIER_POINTER(dn) : "unknown";
                }
            }
        }
    }

    return "unknown";
}


static const int MERGED_BRANCH_STATE = -1;

static void ensure_merged_rule(int from_st, const char* mn) {
    auto &vec = Typestate_Rules[from_st];
    for (auto &p : vec) {
        if (p.first == mn && p.second == MERGED_BRANCH_STATE) return;
    }
    vec.push_back({mn, MERGED_BRANCH_STATE});
}

static void record_branch_rule(int from_st, const char* m_name, int cond, int next) {
    const std::string mn = (m_name ? m_name : "unknown");

    // Store option-specific rule ONLY here
    Branch_Rules[from_st].push_back({mn, cond, next});

}



static std::vector<int> branch_all_next(int src, const std::string& method) {
    std::vector<int> out;
    std::unordered_set<int> seen;

    const std::string uq = unqualified_name(method);
    auto itB = Branch_Rules.find(src);
    if (itB == Branch_Rules.end()) return out;

    for (const auto& br : itB->second) {
        bool name_match = (br.method == method) || (br.method == uq);
        if (!name_match) continue;
        if (!seen.count(br.next)) { seen.insert(br.next); out.push_back(br.next); }
    }
    return out;
}

static int branch_next_for_cond(int src, const std::string& method, int cond_val) {
    const std::string uq = unqualified_name(method);

    auto itB = Branch_Rules.find(src);
    if (itB == Branch_Rules.end()) return -1;

    for (const auto& br : itB->second) {
        bool name_match = (br.method == method) || (br.method == uq);
        if (!name_match) continue;
        if (br.cond == cond_val) return br.next;
    }
    return -1;
}

static tree strip_nops(tree t) {
    while (t && (TREE_CODE(t) == NOP_EXPR ||
                 TREE_CODE(t) == CONVERT_EXPR ||
                 TREE_CODE(t) == VIEW_CONVERT_EXPR)) {
        t = TREE_OPERAND(t, 0);
    }
    return t;
}

static bool get_const_int(tree t, int &K) {
    t = strip_nops(t);
    if (!t) return false;

    if (TREE_CODE(t) == INTEGER_CST) {
        K = (int) tree_to_shwi(t);
        return true;
    }

    // enum constant: CONST_DECL with DECL_INITIAL as INTEGER_CST
    if (TREE_CODE(t) == CONST_DECL) {
        tree init = strip_nops(DECL_INITIAL(t));
        if (init && TREE_CODE(init) == INTEGER_CST) {
            K = (int) tree_to_shwi(init);
            return true;
        }
    }

    return false;
}





static void process_choice_list_type(tree choice_list_ty,
                                     int from_st,
                                     const char* method_name)
{
    if (!choice_list_ty || !TYPE_P(choice_list_ty)) return;

    choice_list_ty = TYPE_MAIN_VARIANT(choice_list_ty);

    // ChoiceList<Choices...> template args
    tree choices = class_template_args(choice_list_ty);

    // Unwrap cases:
    //  (A) choices is TYPE_ARGUMENT_PACK directly
    if (choices && TREE_CODE(choices) == TYPE_ARGUMENT_PACK) {
        choices = ARGUMENT_PACK_ARGS(choices);
    }

    //  (B) choices is TREE_VEC with one element: TYPE_ARGUMENT_PACK
    if (choices && TREE_CODE(choices) == TREE_VEC && TREE_VEC_LENGTH(choices) == 1) {
        tree only = TREE_VEC_ELT(choices, 0);
        if (only && TREE_CODE(only) == TYPE_ARGUMENT_PACK) {
            choices = ARGUMENT_PACK_ARGS(only);
        }
    }

    if (!choices || TREE_CODE(choices) != TREE_VEC) {
      //  fprintf(stderr, "[Coconut] ChoiceList: could not expand choices\n");
       // fflush(stderr);
        return;
    }

    const int C = TREE_VEC_LENGTH(choices);
   // fprintf(stderr, "[Coconut] ChoiceList expanded choices=%d (from=%d method=%s)\n",
       //     C, from_st, method_name ? method_name : "unknown");
   // fflush(stderr);

    for (int j = 0; j < C; ++j) {
        tree choice_item = TREE_VEC_ELT(choices, j);
        if (!choice_item) continue;

        if (PACK_EXPANSION_P(choice_item))
            choice_item = PACK_EXPANSION_PATTERN(choice_item);

        if (!choice_item || !TYPE_P(choice_item)) continue;

        choice_item = TYPE_MAIN_VARIANT(choice_item);

        // Choice<Cond, Next>
        tree c_args = class_template_args(choice_item);
        if (!c_args || TREE_CODE(c_args) != TREE_VEC || TREE_VEC_LENGTH(c_args) < 2) continue;

        int cond, next;
if (!get_int_cst_value(TREE_VEC_ELT(c_args, 0), &cond)) continue;
if (!get_int_cst_value(TREE_VEC_ELT(c_args, 1), &next)) continue;


        // STORE rule
        record_branch_rule(from_st, method_name ? method_name : "unknown", cond, next);

        //fprintf(stderr, "  [Branch Rule] %d --(%s)--> if %d goto %d\n",
          //      from_st, method_name ? method_name : "unknown", cond, next);
        //fflush(stderr);
    }
}
// Optional: prints a short description of a tree node without dumping everything
static void dbg_tree_short(const char* tag, tree t) {
    fprintf(stderr, "[Coconut][DBG] %s: %s\n", tag, t ? get_tree_code_name(TREE_CODE(t)) : "NULL");
    fflush(stderr);
}


static bool apply_branch_if_exists(int &cur_state, const std::string &method) {
    auto it = Branch_Rules.find(cur_state);
    if (it == Branch_Rules.end()) return false;

    for (const auto &r : it->second) {
        if (r.method == method) {
            // Single-state merge policy: pick the first recorded option
            int picked_cond = r.cond;
            int picked_next = r.next;

            fprintf(stderr,
                    "[Coconut] BRANCH MERGE: state %d method '%s' -> picked (cond=%d) next=%d\n",
                    cur_state, method.c_str(), picked_cond, picked_next);

            cur_state = picked_next;
            return true;
        }
    }
    return false;
}

//

static void process_typestate_template_pack(tree ts_args_vec) {
    if (!ts_args_vec || TREE_CODE(ts_args_vec) != TREE_VEC) return;
    const int N = TREE_VEC_LENGTH(ts_args_vec);
    if (N <= 0) return;

    auto peel = [&](tree t) -> tree {
        while (t) {
            enum tree_code c = TREE_CODE(t);
            if (c == NOP_EXPR || c == CONVERT_EXPR || c == NON_LVALUE_EXPR) {
                t = TREE_OPERAND(t, 0);
                continue;
            }
            break;
        }
        return t;
    };

    auto type_name_cstr = [&](tree ty) -> const char* {
        if (!ty || !TYPE_P(ty)) return NULL;
        tree n = TYPE_NAME(ty);
        if (!n) return NULL;
        if (TREE_CODE(n) == TYPE_DECL) {
            tree dn = DECL_NAME(n);
            return dn ? IDENTIFIER_POINTER(dn) : NULL;
        }
        if (TREE_CODE(n) == IDENTIFIER_NODE) {
            return IDENTIFIER_POINTER(n);
        }
        return NULL;
    };

    auto get_int = [&](tree t, int* out) -> bool {
        if (!out) return false;
        t = peel(t);
        if (!t) return false;

        if (TREE_CODE(t) == INTEGER_CST) {
            *out = (int)TREE_INT_CST_LOW(t);
            return true;
        }
        if (TREE_CODE(t) == CONST_DECL) {
            tree init = peel(DECL_INITIAL(t));
            if (init && TREE_CODE(init) == INTEGER_CST) {
                *out = (int)TREE_INT_CST_LOW(init);
                return true;
            }
        }
        return false;
    };

    auto method_name = [&](tree t) -> const char* {
        t = peel(t);
        if (!t) return "unknown";

        if (TREE_CODE(t) == ADDR_EXPR) {
            t = peel(TREE_OPERAND(t, 0));
            if (!t) return "unknown";
        }

        if (TREE_CODE(t) == PTRMEM_CST) {
            tree mem = peel(PTRMEM_CST_MEMBER(t));
            if (!mem) return "unknown";
            if (TREE_CODE(mem) == FUNCTION_DECL) {
                tree dn = DECL_NAME(mem);
                return dn ? IDENTIFIER_POINTER(dn) : "unknown";
            }
            if (TREE_CODE(mem) == OVERLOAD) {
                tree fn = OVL_FUNCTION(mem);
                if (fn && TREE_CODE(fn) == FUNCTION_DECL) {
                    tree dn = DECL_NAME(fn);
                    return dn ? IDENTIFIER_POINTER(dn) : "unknown";
                }
            }
            if (TREE_CODE(mem) == BASELINK) {
                tree fns = BASELINK_FUNCTIONS(mem);
                if (fns && TREE_CODE(fns) == OVERLOAD) {
                    tree fn = OVL_FUNCTION(fns);
                    if (fn && TREE_CODE(fn) == FUNCTION_DECL) {
                        tree dn = DECL_NAME(fn);
                        return dn ? IDENTIFIER_POINTER(dn) : "unknown";
                    }
                }
            }
        }
        return "unknown";
    };

    auto process_one_type = [&](tree arg_ty) {
        if (!arg_ty) return;

        if (PACK_EXPANSION_P(arg_ty))
            arg_ty = PACK_EXPANSION_PATTERN(arg_ty);

        if (TYPE_P(arg_ty)) arg_ty = TYPE_MAIN_VARIANT(arg_ty);
        if (!TYPE_P(arg_ty)) return;

        const char* nm = type_name_cstr(arg_ty);
        if (!nm) return;

        tree inner = class_template_args(arg_ty);
        if (!inner || TREE_CODE(inner) != TREE_VEC) return;
        const int M = TREE_VEC_LENGTH(inner);

        if (strcmp(nm, "State") == 0 && M >= 3) {
            process_typestate_template_args(inner);
            return;
        }

        if (strcmp(nm, "Branch_State") == 0 && M >= 3) {
            int from_st;
            if (!get_int(TREE_VEC_ELT(inner, 0), &from_st)) return;
            const char* m = method_name(TREE_VEC_ELT(inner, 1));

            tree list_ty = TREE_VEC_ELT(inner, 2);
            if (list_ty && TYPE_P(list_ty)) {
            //    fprintf(stderr, "[Coconut] DETECTED Branch_State: from=%d method=%s\n", from_st, m);
               // fflush(stderr);
                process_choice_list_type(list_ty, from_st, m); // <--- call your new function
            }
            return;
        }

           if (strcmp(nm, "Timed_Branch_State") == 0 && M >= 4) {
            int from_st;
            if (!get_int(TREE_VEC_ELT(inner, 0), &from_st)) return;
            const char* m = method_name(TREE_VEC_ELT(inner, 1));

            tree list_ty = TREE_VEC_ELT(inner, 3);
            if (list_ty && TYPE_P(list_ty)) {
                fprintf(stderr, "[Coconut] DETECTED Timed_Branch_State: from=%d method=%s\n", from_st, m);
                fflush(stderr);
                process_choice_list_type(list_ty, from_st, m); // <--- call your new function
            }
            return;
        }
    };

    // ---- Cycle<N> at index 0 ----
    tree cycle_ty = TREE_VEC_ELT(ts_args_vec, 0);
    if (cycle_ty && TYPE_P(cycle_ty)) {
        cycle_ty = TYPE_MAIN_VARIANT(cycle_ty);
        tree cycle_args = class_template_args(cycle_ty);
        if (cycle_args && TREE_CODE(cycle_args) == TREE_VEC && TREE_VEC_LENGTH(cycle_args) >= 1) {
            int n;
            if (get_int(TREE_VEC_ELT(cycle_args, 0), &n)) {
                g_cycle_n = n;
             //   fprintf(stderr, "[Coconut] cycle_n=%d\n", g_cycle_n);
              //  fflush(stderr);
            }
        }
    }

    // ---- Ts... unpacking ----
    for (int i = 1; i < N; ++i) {
        tree arg = TREE_VEC_ELT(ts_args_vec, i);
        if (!arg) continue;

        if (TREE_CODE(arg) == TYPE_ARGUMENT_PACK) {
            tree packed = ARGUMENT_PACK_ARGS(arg);
            if (packed && TREE_CODE(packed) == TREE_VEC) {
                const int P = TREE_VEC_LENGTH(packed);
                for (int k = 0; k < P; ++k)
                    process_one_type(TREE_VEC_ELT(packed, k));
            }
        } else {
            process_one_type(arg);
        }
    }
}



enum CondOp { OP_EQ, OP_NE, OP_OTHER };


static bool extract_eq_ne_with_const(
    gimple* cond_stmt, tree res,
    CondOp &op, int &K)
{
    if (!cond_stmt || gimple_code(cond_stmt) != GIMPLE_COND) return false;

    enum tree_code code = gimple_cond_code(cond_stmt);
    if (code != EQ_EXPR && code != NE_EXPR) return false;

    op = (code == EQ_EXPR) ? OP_EQ : OP_NE;

    tree lhs = strip_nops(gimple_cond_lhs(cond_stmt));
    tree rhs = strip_nops(gimple_cond_rhs(cond_stmt));
    res      = strip_nops(res);
    if (!lhs || !rhs || !res) return false;

    if (lhs == res) return get_const_int(rhs, K);
    if (rhs == res) return get_const_int(lhs, K);
    return false;
}


// ----------------------------------------------------------------------
// Main function: process_typestate_template_pack
// template<typename C, typename TFirst, typename... TRest>
// ----------------------------------------------------------------------


static void harvest_all_functions() {
    cgraph_node* n;
    FOR_EACH_FUNCTION (n) {
        if (!n->decl) continue;
        harvest_annot_from_decl(n->decl);
    }
}

static void print_annotated_wcet_list() {
    if (g_rows.empty()) { printf("WCET list (empty)\n"); return; }

    std::sort(g_rows.begin(), g_rows.end(),
              [](const AnnotRow& a, const AnnotRow& b){ return a.qualified < b.qualified; });

    printf("WCET list\n");
    for (const auto& r : g_rows) {
        printf("  %s : wcet_ms_exact=%.6f\n",
               r.qualified.c_str(), r.wcet_ms);
    }
    fflush(stdout);

   
}




static inline const void* canon_key_from_record(tree t); // from step 1




static const bool   kDebugValidate        = true;   // verbose stderr logging
static const bool   kFailBuildOnViolation = false;  // set true to error out
static const double kAbsTolMs             = 1e-3;   // 1 µs
static const double kRelTol               = 1e-6;   // 1 ppm


static inline double to_ms_ll(long long v_ms) { return static_cast<double>(v_ms); }




// Build a fallback WCET map from g_rows (class unknown → nullptr bucket)
static std::unordered_map<const void*, std::unordered_map<std::string,double>>
build_wcet_map_fallback_from_rows()
{
    std::unordered_map<const void*, std::unordered_map<std::string,double>> m;
    for (const auto& r : g_rows) {
        std::string meth = r.method;
        if (meth.empty()) {
            auto pos = r.qualified.rfind("::");
            meth = (pos == std::string::npos) ? r.qualified : r.qualified.substr(pos+2);
        }
        m[nullptr][meth] = r.wcet_ms;
    }
    return m;
}


static void dump_wcet_buckets(
    const std::unordered_map<const void*, std::unordered_map<std::string,double>>& wcet_by_class)
{
    if (!kDebugValidate) return;
   // std::fprintf(stderr, "[validate] ---- WCET buckets dump ----\n");
    if (wcet_by_class.empty()) {
   //     std::fprintf(stderr, "[validate] (empty)\n");
    } else {
        for (const auto& cls : wcet_by_class) {
       //     std::fprintf(stderr, "[validate] class_key=%p%s\n",
               //          cls.first, cls.first ? "" : " (nullptr bucket)");
            for (const auto& kv : cls.second) {
              //  std::fprintf(stderr, "           %-24s -> %.6f ms\n",
                 //            kv.first.c_str(), kv.second);
            }
        }
    }
 //   std::fprintf(stderr, "[validate] --------------------------------\n");
}

// Safe reporter: preformat to buffer, then use warning_at/error_at with "%s"
static void report_violation(bool as_error, const char* fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (as_error) {
        error_at(UNKNOWN_LOCATION, "%s", buf);
    } else {
        warning_at(UNKNOWN_LOCATION, 0, "%s", buf);
    }
}



// -----------------------------------------------------------------------------
// Timing validation per the paper's semantics (HARD/FIRM vs SOFT; sporadic; and
// system-level feasibility via a per-activation timeline).
// -----------------------------------------------------------------------------

enum class Contract { HARD, SOFT }; // FIRM folds into HARD

static inline Contract contract_from_criticality(int crit) {
    switch (crit) {
        case 1: // HARD
        case 2: // FIRM -> enforce like HARD
            return Contract::HARD;
        case 3: // SOFT
            return Contract::SOFT;
        default:
            // Be conservative if unknown
            return Contract::HARD;
    }
}

static inline double ns_to_ms(long long ns) { return to_ms_ll(ns); }

// If period present but no deadline, treat as SPORADIC: period = min inter-arrival,
// not a per-transition budget.
static inline bool is_sporadic(const TimedInfo& ti) {
    return (ti.period_ns > 0 && ti.U_deadline_ns == 0);
}

// Worst-case release instant (release + jitter)
static inline long long worst_release_ns(const TimedInfo& ti) {
    long long j = ti.offest_ns;
    if (j < 0) j = 0;
    return  j;
}

// ===== window helpers (milliseconds) =========================================
static const double kTimelineAbsTolMs = 1e-3; // 1 µs tolerance

static inline double current_window_start(double t_now,
                                          double period_ms,
                                          double offset_ms)
{
    if (period_ms <= 0.0) return offset_ms; // aperiodic: offset as anchor
    const double k = std::floor((t_now - offset_ms) / period_ms);
    return k * period_ms + offset_ms;
}

static inline const char* crit_label_int(int crit) {
    switch (crit) {
        case 0: return "SOFT";
        case 1: return "HARD";
        case 2: return "FIRM";
        case 3: return "SOFT";
        default: return "?";
    }
}

// Validate one call inside its period/offset window and advance absolute time.
// Returns true if deadline met within the window; false if missed.
static bool check_edge_timing_ms(int state_id,
                                 const std::string& method,
                                 const TimedInfo& ti,
                                 double wcet_ms,
                                 double& t_abs /* in/out */)
{
    // TimedInfo fields are stored in *milliseconds* in your project:
    const double period_ms   = static_cast<double>(ti.period_ns);
   
    const double deadline_ms = static_cast<double>(ti.U_deadline_ns);
    const double l_deadline_ms = static_cast<double>(ti.L_deadline_ns);
    const double offset_ms   = static_cast<double>(ti.offest_ns); // <-- if spelled offest_ns, change here
    const int    crit        = static_cast<int>(ti.criticality);

    const double tol = kTimelineAbsTolMs;

    // Window that contains the *current* absolute time
    const double win_start  = current_window_start(t_abs, period_ms, offset_ms);
      printf("Cycle=%d\n",g_cycle_n);
    // This job's release/deadline inside that window
   // const double job_release  = win_start + release_ms;
   // const double job_deadline = (deadline_ms > 0.0)
                            //  ? (job_release + deadline_ms)
                           //   : (period_ms > 0.0 ? (win_start + period_ms)
                                       //          : std::numeric_limits<double>::infinity());

    // Wait until release if we arrived early
    //if (t_abs + tol < job_release) {
      //  std::fprintf(stderr,
         //   "[timeline] state=%d  wait: t=%.4f -> %.4f ms (win_start=%.4f, rel=%.4f, off=%.4f, per=%.4f)\n",
          //  state_id, t_abs, job_release, win_start, release_ms, offset_ms, period_ms);
     //   t_abs = job_release;
   // }
   float c= g_cycle_n;
    const double t_start = t_abs;
    const double t_end   = t_start + wcet_ms;
    // Progress line (matches your style)
    std::fprintf(stderr,
        "  t=%.4f ms  --%s()-->  t=%.4f ms   (d<%.4f ms)   (Cycle=%.4f ms)  (%s)\n",
        t_start, method.c_str(), t_end,
        (deadline_ms > 0.0 ? deadline_ms : (period_ms > 0.0 ? period_ms : 0.0)),
        c, crit_label_int(crit));

    // Check deadline *within this window*.
   // const bool miss = (t_end > job_deadline + tol);
    if (wcet_ms>g_cycle_n) {
        // Report relative to window to avoid confusion
     //   const double elapsed_end = t_end - win_start;
      error_at(UNKNOWN_LOCATION,
                 "System-level infeasible within cycle exceeds bound %.6f ms "
                 "",
               g_cycle_n);
     //  return false;
     }

   // t_abs = t_end; // advance absolute time
    return true;
}

// Run a windowed timeline for one state's edges (period-window view)
static void run_timeline_for_state_ms(
    int state_id,
    const std::vector<std::tuple<std::string,int,TimedInfo>>& edges,
    const std::unordered_map<std::string,double>& wcet_bucket)
{
    std::fprintf(stderr, "[timeline] state=%d  window=period\n", state_id);

    double t_abs = 0.0; // start-of-frame; windowing uses offset internally
    bool all_ok = true;

    for (const auto& e : edges) {
        const std::string& method = std::get<0>(e);
        const TimedInfo&   ti     = std::get<2>(e);

        auto itW = wcet_bucket.find(method);
        const double wcet_ms = (itW != wcet_bucket.end()) ? itW->second : 0.0;

        const bool ok = check_edge_timing_ms(state_id, method, ti, wcet_ms, t_abs);
        all_ok = all_ok && ok;
    }

    if (all_ok && !edges.empty()) {
        const TimedInfo& last = std::get<2>(edges.back());
        const double period_ms = static_cast<double>(last.period_ns);
       const double offset_ms   = static_cast<double>(last.offest_ns); // <-- if spelled offest_ns, change here
        if (period_ms > 0.0) {
            const double win_start = current_window_start(t_abs, period_ms, offset_ms);
            const double slack = (win_start + period_ms) - t_abs;
            std::fprintf(stderr,
                "  [timeline] completes at t=%.4f ms within budget %.4f ms (slack=%.4f ms)\n",
                t_abs, period_ms, slack);
        } else {
            std::fprintf(stderr,
                "  [timeline] completes at t=%.4f ms (no period)\n", t_abs);
        }
    }
}

// Emit an ASAP (no waiting) end-to-end trace that follows the first edge each step.
#include <set>

// Deduplicate rules (call this once before validating/printing)
static inline bool same_ti(const TimedInfo& a, const TimedInfo& b) {
    return
           a.period_ns   == b.period_ns   &&
           a.U_deadline_ns == b.U_deadline_ns &&
           a.criticality == b.criticality;
}
static void normalize_rules() {
    for (auto& kv : Timed_Typestate_Rules) {
        auto& v = kv.second;
        std::sort(v.begin(), v.end(),
            [](const auto& x, const auto& y){
                if (std::get<0>(x) != std::get<0>(y)) return std::get<0>(x) < std::get<0>(y);
                if (std::get<1>(x) != std::get<1>(y)) return std::get<1>(x) < std::get<1>(y);
                const auto& ax = std::get<2>(x);
                const auto& ay = std::get<2>(y);
              //  if (ax.release_ns  != ay.release_ns)  return ax.release_ns  < ay.release_ns;
                if (ax.period_ns   != ay.period_ns)   return ax.period_ns   < ay.period_ns;
                if (ax.U_deadline_ns != ay.U_deadline_ns) return ax.U_deadline_ns < ay.U_deadline_ns;
                return ax.criticality < ay.criticality;
            });
        v.erase(std::unique(v.begin(), v.end(),
            [](const auto& x, const auto& y){
                return std::get<0>(x)==std::get<0>(y) &&
                       std::get<1>(x)==std::get<1>(y) &&
                       same_ti(std::get<2>(x), std::get<2>(y));
            }), v.end());
    }
}

// Robust WCET lookup (same as before; keep yours if already added)
static double wcet_lookup_ms_for_state_method(
    int from_state,
    const std::string& method,
    const std::unordered_map<const void*, std::unordered_map<std::string,double>>& wcet_by_class)
{
    const void* cls_key = nullptr;
    if (auto itc = EnumToClassMap.find(from_state); itc != EnumToClassMap.end())
        cls_key = canon_key_from_record(itc->second);

    auto hit = wcet_by_class.find(cls_key);
    if (hit != wcet_by_class.end()) {
        auto fit = hit->second.find(method);
        if (fit != hit->second.end()) return fit->second;
    }
    hit = wcet_by_class.find(nullptr);
    if (hit != wcet_by_class.end()) {
        auto fit = hit->second.find(method);
        if (fit != hit->second.end()) return fit->second;
    }
    // last resort: scan all buckets
    for (const auto& b : wcet_by_class) {
        auto it = b.second.find(method);
        if (it != b.second.end()) return it->second;
    }
    return 0.0;
}

// End-to-end ASAP trace that stops on repeat
static void emit_end_to_end_asap_trace(
    const std::unordered_map<const void*, std::unordered_map<std::string,double>>& wcet_by_class)
{
    if (Timed_Typestate_Rules.empty()) return;

    // choose a start (prefer 0 if present)
    int start_state = Timed_Typestate_Rules.begin()->first;
    if (Timed_Typestate_Rules.count(0)) start_state = 0;

    std::fprintf(stderr, "[trace] --- end-to-end time trace ---\n");

    double t_ms = 0.0;
    int cur = start_state;

    // Stop criteria
    std::set<std::pair<int,std::string>> seen_edges; // (from_state, method)
    std::set<int> seen_states;                       // optional: track states too
    const int hard_cap = (int)Timed_Typestate_Rules.size() * 10;
  std::fprintf(stderr,
    "[trace] states=%zu, hard_cap=%d\n",
    Timed_Typestate_Rules.size(), hard_cap);

    for (int step = 0; step < hard_cap; ++step) {
        auto it = Timed_Typestate_Rules.find(cur);
        if (it == Timed_Typestate_Rules.end() || it->second.empty()) break;

        // deterministic: first outgoing edge after normalization
        const auto& edge   = it->second.front();
        const std::string& method   = std::get<0>(edge);
        const int          to_state = std::get<1>(edge);
        const TimedInfo&   ti       = std::get<2>(edge);

        // stop if we loop back to start after first hop
        if (step > 0 && cur == start_state) {
            std::fprintf(stderr, "[trace] stop: returned to start_state=%d\n", start_state);
            break;
        }
        // stop if this (state,method) already seen
        auto key = std::make_pair(cur, method);
        if (seen_edges.count(key)) {
            std::fprintf(stderr, "[trace] stop: cycle detected at state %d via '%s'\n",
                         cur, method.c_str());
            break;
        }
        seen_edges.insert(key);
        seen_states.insert(cur);

        const double wcet_ms = wcet_lookup_ms_for_state_method(cur, method, wcet_by_class);
        const double t_next  = t_ms + wcet_ms;

        std::fprintf(stderr,
    "[trace] t_%d=%.2f ms --%s()--> t_%d=%.2f ms\n",
    step, t_ms, method.c_str(), step + 1, t_next);


        if (ti.U_deadline_ns > 0) {
            std::fprintf(stderr, " (d<%.4f ms)", ns_to_ms(ti.U_deadline_ns));
        } else if (ti.period_ns > 0) {
            std::fprintf(stderr, " (p%s%.4f ms)", is_sporadic(ti) ? ">=" : "=", ns_to_ms(ti.period_ns));
        }
        std::fprintf(stderr, " [state %d -> %d]\n", cur, to_state);

        t_ms = t_next;
        cur = to_state;
    }
}









// ------------------------ Delay API matching ------------------------
using TimedEdge = std::tuple<std::string, int, TimedInfo>;
using TimedTypestateRules = std::unordered_map<int, std::vector<TimedEdge>>;


// ------------------------ Internal indices ------------------------

struct EdgeRule {
  int from_state;
  int to_state;
  TimedInfo ti;
};

static std::unordered_map<std::string, std::vector<EdgeRule>> g_rules_by_method;

// Optional WCET map (ns)
static std::unordered_map<std::string, long long> g_wcet_ns_by_func;

// plugin args
static std::string g_out_dir = ".";
static std::string g_wcet_json_path;
static int g_warn_on_missing_delay_criticality_ge = 0; // 0 = off by default
static int g_search_window_stmts = 8;                  // proximity window in a BB

// ------------------------ Small helpers ------------------------

static std::string trim(std::string s) {
  while (!s.empty() && std::isspace((unsigned char)s.front())) s.erase(s.begin());
  while (!s.empty() && std::isspace((unsigned char)s.back()))  s.pop_back();
  return s;
}

static bool ends_with(const std::string& s, const std::string& suffix) {
  return s.size() >= suffix.size() && s.compare(s.size()-suffix.size(), suffix.size(), suffix) == 0;
}

// Extract readable function key from fndecl
// For C: usually just the identifier.
// For C++: DECL_NAME is often mangled (e.g. _ZN12TrafficLight13SwitchToAmberEv).
// You can choose to store rules using mangled names, OR store unqualified “SwitchToAmber”.
static std::string get_method_key_from_fndecl(tree fndecl) {
  if (!fndecl) return {};
  tree name = DECL_NAME(fndecl);
  if (!name) return {};

  const char* id = IDENTIFIER_POINTER(name);
  if (!id) return {};
  return std::string(id);
}

// Best-effort unqualified extraction from C++ pretty name is non-trivial without demangling.
// This very simple heuristic tries to match your rules if they’re unqualified:
// rule method "SwitchToAmber" matches keys that end with "SwitchToAmberEv" (common Itanium mangling)
// OR exactly equals the unqualified C function name.
static bool method_matches_rule(const std::string& callee_key, const std::string& rule_method) {
  if (callee_key == rule_method) return true;

  // If rule_method is unqualified and callee is mangled, attempt suffix match.
  // Example: rule "SwitchToAmber" matches "_ZN12TrafficLight13SwitchToAmberEv"
  if (!rule_method.empty() && callee_key.find(rule_method) != std::string::npos) {
    // Slightly stricter: require it appears near end
    if (ends_with(callee_key, rule_method + "Ev") || ends_with(callee_key, rule_method) )
      return true;
    // If you prefer a looser match, you can return true here.
  }
  return false;
}

// Returns vector of all rules that match this callee key.
// We index by rule_method string, but we allow suffix match as fallback.
static std::vector<std::pair<std::string, const std::vector<EdgeRule>*>>
lookup_rules_for_callee(const std::string& callee_key) {
  std::vector<std::pair<std::string, const std::vector<EdgeRule>*>> out;

  // Fast path: direct key hit
  auto it = g_rules_by_method.find(callee_key);
  if (it != g_rules_by_method.end()) {
    out.push_back({callee_key, &it->second});
    return out;
  }

  // Fallback: suffix/contains matching against all rule method keys
  for (auto& kv : g_rules_by_method) {
    const std::string& rule_method = kv.first;
    if (method_matches_rule(callee_key, rule_method)) {
      out.push_back({rule_method, &kv.second});
    }
  }
  return out;
}

// ------------------------ Delay API matching ------------------------

enum class DelayUnit { NS, US, MS };
static double to_ms(double v, DelayUnit u) {
  switch (u) {
    case DelayUnit::NS: return v / 1e6;
    case DelayUnit::US: return v / 1e3;
    case DelayUnit::MS: return v;
  }
  return 0.0;
}
struct DelayApi {
  const char* name;
  DelayUnit unit;
  int arg_index;
};

static const DelayApi g_delay_apis[] = {
    {"hw_delay_ms", DelayUnit::MS, 0},
  {"DelayMs", DelayUnit::MS, 0},
  {"delay_ms", DelayUnit::MS, 0},
  {"HAL_Delay", DelayUnit::MS, 0},
  {"delay_us", DelayUnit::US, 0},
  {"simulate_", DelayUnit::MS, 0},
};

static long long to_ns(long long v, DelayUnit u) {
  switch (u) {
    case DelayUnit::NS: return v;
    case DelayUnit::US: return v * 1000LL;
    case DelayUnit::MS: return v * 1000LL * 1000LL;
  }
  return -1;
}

static const DelayApi* find_delay_api_by_callee_key(const std::string& key) {
  for (const auto& api : g_delay_apis) {
    if (key == api.name) return &api;
    // allow matching when the key is mangled and name appears
    if (key.find(api.name) != std::string::npos) return &api;
  }
  return nullptr;
}

#include "real.h"
// Extract a constant integer argument from gimple call
static bool extract_const_number_arg(gimple* call, int arg_index, double* out) {
  if (!call || !is_gimple_call(call)) return false;
  if (arg_index < 0 || arg_index >= (int)gimple_call_num_args(call)) return false;

  tree arg = gimple_call_arg(call, arg_index);
  if (!arg) return false;

  if (TREE_CODE(arg) == INTEGER_CST) {
    *out = (double)TREE_INT_CST_LOW(arg);
    return true;
  }

 if (TREE_CODE(arg) == REAL_CST) {
  char buf[64];
  real_to_decimal(buf, &TREE_REAL_CST(arg), sizeof(buf), 0, 1);
  *out = atof(buf);
  return true;
}

  return false;
}
// ------------------------ Index timed rules ------------------------

static void index_timed_rules() {
  g_rules_by_method.clear();

  for (const auto& kv : Timed_Typestate_Rules) {
    int from = kv.first;
    const auto& edges = kv.second;

    for (const auto& edge : edges) {
      const std::string& method = std::get<0>(edge);
      int to = std::get<1>(edge);
      const TimedInfo& ti = std::get<2>(edge);

      g_rules_by_method[method].push_back(EdgeRule{from, to, ti});
    }
  }
}


struct NearbyDelay {
  bool found = false;
  bool const_value = false;
   double delay_ms = 0.0;      // <-- ms
  location_t loc = UNKNOWN_LOCATION;
  std::string api_key;
};

static std::vector<gimple*> collect_bb_stmts(basic_block bb) {
  std::vector<gimple*> v;
  for (gimple_stmt_iterator gsi = gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi)) {
    v.push_back(gsi_stmt(gsi));
  }
  return v;
}



static NearbyDelay find_nearby_delay_in_bb(const std::vector<gimple*>& stmts, int call_index) {
  NearbyDelay nd;

  int lo = std::max(0, call_index - g_search_window_stmts);
  int hi = std::min((int)stmts.size() - 1, call_index + g_search_window_stmts);

  // Search backward first (common pattern: Delay(); Transition();)
  for (int j = call_index - 1; j >= lo; --j) {
    gimple* st = stmts[j];
    if (!is_gimple_call(st)) continue;

    tree callee = gimple_call_fndecl(st);
    std::string key = get_method_key_from_fndecl(callee);
    const DelayApi* api = find_delay_api_by_callee_key(key);
    if (!api) continue;

    nd.found = true;
    nd.loc = gimple_location(st);
    nd.api_key = key;

    double raw = 0.0;
    if (extract_const_number_arg(st, api->arg_index, &raw)) {
      nd.const_value = true;
      nd.delay_ms = to_ms(raw, api->unit);
    }
    return nd;
  }

  // Then search forward (pattern: Transition(); Delay();)
  for (int j = call_index + 1; j <= hi; ++j) {
    gimple* st = stmts[j];
    if (!is_gimple_call(st)) continue;

    tree callee = gimple_call_fndecl(st);
    std::string key = get_method_key_from_fndecl(callee);
    const DelayApi* api = find_delay_api_by_callee_key(key);
    if (!api) continue;

    nd.found = true;
    nd.loc = gimple_location(st);
    nd.api_key = key;

    double raw = 0.0;
    if (extract_const_number_arg(st, api->arg_index, &raw)) {
      nd.const_value = true;
      nd.delay_ms = to_ms(raw, api->unit);
    }
    return nd;
  }

  return nd;
}

static void report_delay_violation(location_t where,
                                  const std::string& method_rule_key,
                                  const EdgeRule& er,
                                  const NearbyDelay& nd)
{
  // Rules are milliseconds (per your last message)
  const double L = (double)er.ti.L_deadline_ns; // actually ms in your struct
  const double U = (double)er.ti.U_deadline_ns; // actually ms

  char d[64], l[64], u[64];
  std::snprintf(d, sizeof(d), "%.3f", (double)nd.delay_ms);
  std::snprintf(l, sizeof(l), "%.3f", L);
  std::snprintf(u, sizeof(u), "%.3f", U);

  const char* api = nd.api_key.empty() ? "<none>" : nd.api_key.c_str();

  warning_at(where, 0,
    "Timed typestate violation for method rule %qs: delay=%s ms (via %qs) not in [%s, %s] ms "
    "(edge %d->%d, criticality=%d) It will not compile regardless of the hardware",
    method_rule_key.c_str(),
    d,
    api,
    l,
    u,
    er.from_state,
    er.to_state,
    er.ti.criticality);
}

static void report_missing_delay(location_t where,
                                 const std::string& method_rule_key,
                                 const EdgeRule& er)
{
  const double L = (double)er.ti.L_deadline_ns; // actually ms
  const double U = (double)er.ti.U_deadline_ns; // actually ms

  char l[64], u[64];
  std::snprintf(l, sizeof(l), "%.3f", L);
  std::snprintf(u, sizeof(u), "%.3f", U);

  warning_at(where, 0,
    "Timed typestate: no explicit delay found for rule %qs "
    "(edge %d->%d, deadline [%s,%s] ms, criticality=%d)",
    method_rule_key.c_str(),
    er.from_state,
    er.to_state,
    l,
    u,
    er.ti.criticality);
}

static void report_unknown_delay(location_t where,
                                 const std::string& method_rule_key,
                                 const EdgeRule& er,
                                 const NearbyDelay& nd)
{
  const double L = (double)er.ti.L_deadline_ns; // actually ms
  const double U = (double)er.ti.U_deadline_ns; // actually ms

  char l[64], u[64];
  std::snprintf(l, sizeof(l), "%.3f", L);
  std::snprintf(u, sizeof(u), "%.3f", U);

  const char* api = nd.api_key.empty() ? "<none>" : nd.api_key.c_str();

  warning_at(where, 0,
    "Timed typestate: found delay call (%qs) for rule %qs but delay argument is not a constant; cannot verify bounds "
    "(edge %d->%d, deadline [%s,%s] ms, criticality=%d)",
    api,
    method_rule_key.c_str(),
    er.from_state,
    er.to_state,
    l,
    u,
    er.ti.criticality);
}


static int g_debug_timed = 1; // set to 0 to silence

static const char* safe_ident(tree id) {
  return id ? IDENTIFIER_POINTER(id) : "<null>";
}

static std::string fndecl_key(tree fndecl) {
  if (!fndecl) return {};
  tree n = DECL_NAME(fndecl);
  if (!n) return {};
  const char* s = IDENTIFIER_POINTER(n);
  return s ? std::string(s) : std::string{};
}

static NearbyDelay find_any_delay_in_current_function() {
  NearbyDelay nd;

  basic_block bb;
  FOR_ALL_BB_FN(bb, cfun) {
    auto stmts = collect_bb_stmts(bb);
    for (gimple* st : stmts) {
      if (!is_gimple_call(st)) continue;

      tree callee = gimple_call_fndecl(st);
      if (!callee) continue;

      std::string key = get_method_key_from_fndecl(callee);
      if (key.empty()) continue;

      const DelayApi* api = find_delay_api_by_callee_key(key);
      if (!api) continue;

      nd.found = true;
      nd.loc = gimple_location(st);
      nd.api_key = key;

      double raw = 0.0;
      if (extract_const_number_arg(st, api->arg_index, &raw)) {
        nd.const_value = true;
        nd.delay_ms = to_ms(raw, api->unit);
      }
      return nd; // take first delay found
    }
  }

  return nd; // not found
}

static void validate_one_function(tree fndecl) {
  if (!fndecl) return;
  if (!DECL_STRUCT_FUNCTION(fndecl)) return;
  if (g_rules_by_method.empty()) return;

  const std::string fn_key = fndecl_key(fndecl);

  push_cfun(DECL_STRUCT_FUNCTION(fndecl));

  // ---------------- Debug counters ----------------
  long long calls_total = 0;
  long long calls_with_fndecl = 0;
  long long calls_matched_rule = 0;
  long long callsite_delay_found = 0;
  long long callsite_delay_const = 0;

  // ==========================================================
  // Part A: “Self-match” check
  // If the *function being validated* is itself one of the timed
  // rule methods (e.g., Green/Red/Amber/RedAmber), then look for
  // delay inside the function (simulate_, delay_ms, etc.) and check
  // bounds directly against the rules for that method.
  // ==========================================================
  {
    auto self_matches = lookup_rules_for_callee(fn_key);
    if (!self_matches.empty()) {
      NearbyDelay nd = find_any_delay_in_current_function();

      if (g_debug_timed) {
        fprintf(stderr,
                "[timed] SELF fn=%s rules=%zu delay_found=%d const=%d ns=%lld api=%s\n",
                fn_key.c_str(),
                self_matches.size(),
                nd.found ? 1 : 0,
                nd.const_value ? 1 : 0,
                (long long)nd.delay_ms,
                nd.api_key.empty() ? "<none>" : nd.api_key.c_str());
      }

      // Use a stable location: function decl location
      location_t where = DECL_SOURCE_LOCATION(fndecl);

      for (auto& m : self_matches) {
        const std::string& method_rule_key = m.first;
        const std::vector<EdgeRule>& edges = *m.second;

        for (const EdgeRule& er : edges) {
          if (!nd.found) {
            // No delay inside transition method
            if (g_warn_on_missing_delay_criticality_ge > 0 &&
                er.ti.criticality >= g_warn_on_missing_delay_criticality_ge) {
              report_missing_delay(where, method_rule_key, er);
            }
            continue;
          }

          if (!nd.const_value) {
            if (g_warn_on_missing_delay_criticality_ge > 0 &&
                er.ti.criticality >= g_warn_on_missing_delay_criticality_ge) {
              report_unknown_delay(where, method_rule_key, er, nd);
            }
            continue;
          }

          if (nd.delay_ms < er.ti.L_deadline_ns || nd.delay_ms > er.ti.U_deadline_ns) {
            report_delay_violation(where, method_rule_key, er, nd);
          } else if (g_debug_timed) {
            fprintf(stderr,
                    "[timed] SELF-OK fn=%s delay_ms=%f bounds=[%f,%f] crit=%d\n",
                    fn_key.c_str(),
                    nd.delay_ms,
                    er.ti.L_deadline_ns,
                    er.ti.U_deadline_ns,
                    er.ti.criticality);
          }
        }
      }
    }
  }

  // ==========================================================
  // Part B: “Call-site” check
  // For calls *inside this function* that invoke a timed-rule method,
  // look for a nearby delay call in the same basic block and check bounds.
  // This matches patterns like:
  //    DelayMs(3000); obj.Green();
  // ==========================================================
  basic_block bb;
  FOR_ALL_BB_FN(bb, cfun) {
    auto stmts = collect_bb_stmts(bb);

    for (int idx = 0; idx < (int)stmts.size(); ++idx) {
      gimple* st = stmts[idx];
      if (!is_gimple_call(st)) continue;
      calls_total++;

      tree callee = gimple_call_fndecl(st);
      if (!callee) continue;
      calls_with_fndecl++;

      std::string callee_key = get_method_key_from_fndecl(callee);
      if (callee_key.empty()) continue;

      // Debug: print first few calls per function
      if (g_debug_timed && calls_total <= 8) {
        fprintf(stderr, "[timed] fn=%s call=%s\n",
                fn_key.c_str(), callee_key.c_str());
      }

      // Is this call a transition method referenced by any timed rule?
      auto matches = lookup_rules_for_callee(callee_key);
      if (matches.empty()) continue;
      calls_matched_rule++;

      if (g_debug_timed) {
        fprintf(stderr, "[timed] MATCH fn=%s callee=%s (matched %zu rule-keys)\n",
                fn_key.c_str(), callee_key.c_str(), matches.size());
      }

      NearbyDelay nd = find_nearby_delay_in_bb(stmts, idx);
      if (nd.found) {
        callsite_delay_found++;
        if (nd.const_value) callsite_delay_const++;
      }

      if (g_debug_timed) {
        if (nd.found) {
          fprintf(stderr, "[timed]   nearby delay: api=%s const=%d ns=%lld\n",
                  nd.api_key.c_str(), nd.const_value ? 1 : 0, (long long)nd.delay_ms);
        } else {
          fprintf(stderr, "[timed]   nearby delay: NONE (window=%d stmts)\n",
                  g_search_window_stmts);
        }
      }

      location_t call_loc = gimple_location(st);

      // For each matched rule-method key and its edges
      for (auto& m : matches) {
        const std::string& method_rule_key = m.first;
        const std::vector<EdgeRule>& edges = *m.second;

        for (const EdgeRule& er : edges) {
          if (nd.found) {
            if (!nd.const_value) {
              if (g_warn_on_missing_delay_criticality_ge > 0 &&
                  er.ti.criticality >= g_warn_on_missing_delay_criticality_ge) {
                report_unknown_delay(call_loc, method_rule_key, er, nd);
              }
            } else {
              if (nd.delay_ms < er.ti.L_deadline_ns || nd.delay_ms > er.ti.U_deadline_ns) {
                report_delay_violation(call_loc, method_rule_key, er, nd);
              }
            }
          } else {
            if (g_warn_on_missing_delay_criticality_ge > 0 &&
                er.ti.criticality >= g_warn_on_missing_delay_criticality_ge) {
              report_missing_delay(call_loc, method_rule_key, er);
            }
          }
        }
      }
    }
  }

  if (g_debug_timed && (calls_total > 0 || calls_matched_rule > 0)) {
    fprintf(stderr,
            "[timed] SUMMARY fn=%s calls=%lld fndecl_calls=%lld matched=%lld callsite_delay_found=%lld callsite_delay_const=%lld\n",
            fn_key.c_str(),
            calls_total,
            calls_with_fndecl,
            calls_matched_rule,
            callsite_delay_found,
            callsite_delay_const);
  }

  pop_cfun();
}

static void validate_logic_with_timed_rules() {
  index_timed_rules();

  //fprintf(stderr, "[validate] indexed timed rules: %zu unique method keys\n",
   //       g_rules_by_method.size());

  if (g_debug_timed) {
    int shown = 0;
    for (auto& kv : g_rules_by_method) {
      fprintf(stderr, "[validate] rule_key[%d]=%s edges=%zu\n",
              shown, kv.first.c_str(), kv.second.size());
      if (++shown >= 8) break;
    }
  }

  long long funcs_seen = 0;
  long long funcs_validated = 0;

  cgraph_node* node = nullptr;
  FOR_EACH_DEFINED_FUNCTION(node) {
    funcs_seen++;
    tree fndecl = node->decl;
    if (!fndecl) continue;
    if (!DECL_STRUCT_FUNCTION(fndecl)) continue;
    if (DECL_ARTIFICIAL(fndecl)) continue;

    if (g_debug_timed && funcs_seen <= 20) {
   //   fprintf(stderr, "[validate] visit fn=%s\n", fndecl_key(fndecl).c_str());
    }

    funcs_validated++;
    validate_one_function(fndecl);
  }

 // fprintf(stderr, "[validate] funcs_seen=%lld funcs_validated=%lld\n",
          //funcs_seen, funcs_validated);
}



static void validate_wcet_vs_timed_rules() {
    // 1) WCET source (prefer class-aware; else fallback)
    normalize_rules();
    std::unordered_map<const void*, std::unordered_map<std::string,double>> wcet_by_class = g_annot_wcet_ms;
    bool used_fallback = false;
    if (wcet_by_class.empty()) {
        wcet_by_class = build_wcet_map_fallback_from_rows();
        used_fallback = true;
    }

    if (kDebugValidate) {
        std::fprintf(stderr, "[validate] using %s for WCETs\n",
                     used_fallback ? "g_rows (fallback)" : "g_annot_wcet_ms");
        dump_wcet_buckets(wcet_by_class);
    }

    bool ok                  = true;  // overall success (warnings make this false)
    bool any_missing         = false; // missing buckets/entries (warning)
    bool any_violation       = false; // any violation (soft or hard)
    bool any_hard_violation  = false; // hard/firm violations (can fail build)

    // Keep a bucket pointer per originating state for the timeline phase
    std::unordered_map<int, const std::unordered_map<std::string,double>*> state_bucket;

    // For a global end-to-end trace across states
    struct EdgeSample {
        int from_state;
        int to_state;
        std::string method;
        double wcet_ms;
        TimedInfo ti;
        bool recursive;  // true if from_state == to_state
    };
    std::vector<EdgeSample> all_edges;

    double cycle_ms = 0.0;

    for (const auto& kv : Timed_Typestate_Rules) {
        int from_state = kv.first;
        const auto& edges = kv.second;

        // class key for this FSM state
        const void* cls_key = nullptr;
        if (auto itc = EnumToClassMap.find(from_state); itc != EnumToClassMap.end())
            cls_key = canon_key_from_record(itc->second);

        // exact class bucket or nullptr fallback
        auto bucket_it = wcet_by_class.find(cls_key);
        bool using_null_bucket = false;
        if (bucket_it == wcet_by_class.end()) {
            bucket_it = wcet_by_class.find(nullptr);
            using_null_bucket = (bucket_it != wcet_by_class.end());
        }

        if (bucket_it != wcet_by_class.end())
            state_bucket[from_state] = &bucket_it->second;
        else
            state_bucket[from_state] = nullptr;

        if (kDebugValidate) {
            std::fprintf(stderr, "[validate] state=%d class_key=%p  bucket=%s\n",
                         from_state, cls_key,
                         (bucket_it == wcet_by_class.end()) ? "NONE" :
                         (using_null_bucket ? "nullptr (fallback)" : "exact class"));
        }

        for (const auto& e : edges) {
            const std::string& method   = std::get<0>(e);
            const int          to_state = std::get<1>(e);
            const TimedInfo&   ti       = std::get<2>(e);

            const bool is_recursive = (from_state == to_state);
            if (is_recursive) {
                g_recursive_transitions.push_back({ method, from_state });
                g_recursive_method_transitions.insert(method);
                if (kDebugValidate) {
                    std::fprintf(stderr,
                        "[validate] detected recursive transition: state %d, method '%s'\n",
                        from_state, method.c_str());
                }
            }

            // Pick the per-transition budget (ms): deadline preferred; else non-sporadic period
            double rule_ms = 0.0;
            const char* rule_src = "none";
            const bool spor = is_sporadic(ti);
            const Contract ctr = contract_from_criticality(ti.criticality);
            double lower_rule_ms =0.0;
            if (ti.U_deadline_ns > 0) {
                rule_ms = ns_to_ms(ti.U_deadline_ns);
                lower_rule_ms=ns_to_ms(ti.L_deadline_ns);
                rule_src = "deadline";
            } else if (ti.period_ns > 0 && !spor) {
                rule_ms = ns_to_ms(ti.period_ns);
                rule_src = "period-as-budget";
            } else {
                // No per-transition budget → skip compare, but still collect for timeline
                if (bucket_it != wcet_by_class.end()) {
                    const auto& bucket = bucket_it->second;
                    if (auto fn_it = bucket.find(method); fn_it != bucket.end()) {
                        const double wcet_ms = fn_it->second;
                        all_edges.push_back({from_state, to_state, method, wcet_ms, ti, is_recursive});
                    } else {
                        // report_violation(false,
                        //   "Missing WCET for '%s' (state %d). No deadline; sporadic or unspecified.",
                        //   method.c_str(), from_state);
                        any_missing = true; ok = false;
                    }
                } else {
                    report_violation(false,
                        "No WCETs recorded for class of state %d; missing '%s' (no deadline).",
                        from_state, method.c_str());
                    any_missing = true; ok = false;
                }
                continue;
            }

            if (bucket_it == wcet_by_class.end()) {
                // Missing class bucket → WARNING only
                report_violation(false,
                    "No WCETs recorded for class of state %d; missing '%s' (rule=%.6f ms).",
                    from_state, method.c_str(), rule_ms);
                if (kDebugValidate)
                    std::fprintf(stderr, "  [miss] %s: bucket missing (rule %.6f from %s)\n",
                                 method.c_str(), rule_ms, rule_src);
                any_missing = true; ok = false;
                continue;
            }

            const auto& bucket = bucket_it->second;
            auto fn_it = bucket.find(method);
            if (fn_it == bucket.end()) {
                // Missing method entry → WARNING only
                report_violation(false,
                    "Missing WCET for '%s' (state %d). Expected %.6f ms from %s.",
                    method.c_str(), from_state, rule_ms, rule_src);

                if (kDebugValidate) {
                    std::fprintf(stderr, "  [miss] %s: not in bucket. Have:", method.c_str());
                    int shown = 0;
                    for (const auto& kv2 : bucket) {
                        if (shown++ >= 12) { std::fprintf(stderr, " ..."); break; }
                        std::fprintf(stderr, " %s", kv2.first.c_str());
                    }
                    std::fprintf(stderr, "\n");
                }
                any_missing = true; ok = false;
                continue;
            }

            const double wcet_ms = fn_it->second;
            const double tol = std::max(kAbsTolMs, std::fabs(rule_ms) * kRelTol);

            bool hard_violate = false;
            bool soft_warn    = false;
            bool soft_over    = false;

            if (ctr == Contract::HARD) {
                // HARD/FIRM: WCET must not exceed the budget (within tolerance) witin lower and upper 
               hard_violate = (wcet_ms < lower_rule_ms) || (wcet_ms > rule_ms);
            } else {
                // SOFT: warn if near/equal or exceeding
                if (wcet_ms > rule_ms + tol) {
                    soft_warn = true; soft_over = true;
                } else if (wcet_ms >= rule_ms - tol) {
                    soft_warn = true;
                }
            }

            if (kDebugValidate) {
                std::fprintf(stderr,
                    "  [cmp] %-22s wcet=%.6f  rule=%.6f (%s)  tol=%.3g  (%s) -> %s\n",
                    method.c_str(), wcet_ms, rule_ms, rule_src, tol,
                    (ctr == Contract::HARD ? "HARD/FIRM" : "SOFT"),
                    (ctr == Contract::HARD ? (hard_violate ? "VIOLATION" : "ok")
                                           : (soft_warn ? (soft_over ? "warning(over)" : "warning")
                                                        : "ok")));
            }

            // Record for global path (raw wcet_ms; recursion handled later via effective WCET)
            all_edges.push_back({from_state, to_state, method, wcet_ms, ti, is_recursive});

            // Emit diagnostics
            if (ctr == Contract::HARD && hard_violate) {
                report_violation(true,
                    "WCET violates Time contract for '%s' (state %d): wcet=%.6f ms, "
                    "limit between=%.3f ms and %.3f ms.",
                    method.c_str(), from_state, wcet_ms, rule_ms, lower_rule_ms);
                any_violation = true; any_hard_violation = true; ok = false;
            } else if (ctr == Contract::SOFT && soft_warn) {
                if (soft_over) {
                    report_violation(false,
                        "WCET exceeds SOFT limit for '%s' (state %d): wcet=%.6f ms, "
                        "limit=%.6f ms (tol=%.3g).",
                        method.c_str(), from_state, wcet_ms, rule_ms, tol);
                } else {
                    report_violation(false,
                        "WCET near/equal to SOFT limit for '%s' (state %d): wcet=%.6f ms, "
                        "limit=%.6f ms (tol=%.3g).",
                        method.c_str(), from_state, wcet_ms, rule_ms, tol);
                }
                any_violation = true; ok = false;
            }
        }
    }

    // ---------------- System-level timing feasibility (windowed) ---------------
    for (const auto& kv2 : Timed_Typestate_Rules) {
        const int state = kv2.first;
        const auto& edges = kv2.second;

        static const std::unordered_map<std::string,double> kEmpty;
        const auto itB = state_bucket.find(state);
        const std::unordered_map<std::string,double>& bucket =
            (itB != state_bucket.end() && itB->second) ? *(itB->second) : kEmpty;

        run_timeline_for_state_ms(state, edges, bucket);
    }

    // --------- (optional) simple end-to-end trace across states ----------------
    if (kDebugValidate && !all_edges.empty()) {
        const int initial_state = 0; // adjust if your FSM uses another initial state
        std::vector<const EdgeSample*> path;
        std::vector<bool> used(all_edges.size(), false);

        // helper: effective WCET with recursion applied
        auto effective_wcet_ms = [](const EdgeSample& s) -> double {
            
            return s.recursive ? (depth_r * s.wcet_ms) : s.wcet_ms;
        };

        int current = initial_state;
        for (std::size_t step = 0; step < all_edges.size(); ++step) {
            std::size_t next_idx = all_edges.size();
            double max_eff_wcet = -1.0;

            // Pick the slowest (max *effective* WCET) outgoing edge from the current state
            for (std::size_t i = 0; i < all_edges.size(); ++i) {
                if (!used[i] && all_edges[i].from_state == current) {
                    double eff = effective_wcet_ms(all_edges[i]);
                    if (eff > max_eff_wcet) {
                        max_eff_wcet = eff;
                        next_idx = i;
                    }
                }
            }

            if (next_idx == all_edges.size())
                break; // no more outgoing edges from this state

            used[next_idx] = true;
            path.push_back(&all_edges[next_idx]);
            current = all_edges[next_idx].to_state;
        }

        if (!path.empty()) {
            std::fprintf(stderr, "[ttrace] --- end-to-end time trace ---\n");
            double t_ms = 0.0;

            for (std::size_t i = 0; i < path.size(); ++i) {
                const EdgeSample& s = *path[i];
                const TimedInfo& ti = s.ti;

                double rel_ms = ns_to_ms(worst_release_ns(ti));
                if (rel_ms > t_ms) t_ms = rel_ms;

                const double eff_wcet = effective_wcet_ms(s);
                if (s.recursive) {
                    std::printf("Recursive WCET applied for %s: %.4f ms\n",
                                s.method.c_str(), eff_wcet);
                }

                const double t_next = t_ms + eff_wcet;
                std::fprintf(stderr,
                    "[ttrace] t_%zu=%.4f ms --%s()%s--> t_%zu=%.4f ms",
                    i, t_ms,
                    s.method.c_str(),
                    s.recursive ? " [recursive]" : "",
                    i + 1, t_next);

                if (ti.U_deadline_ns > 0) {
                    std::fprintf(stderr, " (d<%.4f ms)", ns_to_ms(ti.U_deadline_ns));
                } else if (ti.period_ns > 0) {
                    std::fprintf(stderr, " (p%s%.4f ms)",
                                 is_sporadic(ti) ? ">=" : "=",
                                 ns_to_ms(ti.period_ns));
                }

                std::fprintf(stderr, " [state %d -> %d]\n", s.from_state, s.to_state);
                t_ms = t_next;

                if (i == path.size()-1){
                    std::fprintf(stderr,
                        "[ttrace] end at t_%zu=%.4f ms\n",
                        i + 1, t_ms);
                    cycle_ms = t_ms;
                }
            }
        }
    }
    // ---------------------------------------------------------------------------

    if (!ok) {
        if (kDebugValidate) {
            std::fprintf(stderr, "[validate] FAIL: %s%s%s\n",
                any_hard_violation ? "hard violations " : "",
                (any_violation && !any_hard_violation) ? "violations " : "",
                any_missing ? "(missing entries)" : "");
        }
        // Only hard-stop on HARD/FIRM violations
        if (kFailBuildOnViolation && any_hard_violation) {
            fatal_error(UNKNOWN_LOCATION,
                        "WCET vs Timed Typestate HARD/FIRM timing violations detected.");
        }
    } else {
        if (kDebugValidate) std::fprintf(stderr, "[validate] PASS: all checks ok\n");
    }

    if (cycle_ms > g_cycle_n) {
        error_at(UNKNOWN_LOCATION,
                 "End-to-end cycle time %.6f ms exceeds bound %.6f ms.",
                 cycle_ms, g_cycle_n);
    }

    emit_end_to_end_asap_trace(wcet_by_class);
}


void validate_LSP_compliance() {
    for (const auto& [subclass, sub_fsm] : FSM_by_Class) {
        tree sub_binfo = TYPE_BINFO(subclass);
        if (!sub_binfo) continue;

        for (int i = 0; i < BINFO_N_BASE_BINFOS(sub_binfo); ++i) {
            tree base_binfo = BINFO_BASE_BINFO(sub_binfo, i);
            tree base_class = BINFO_TYPE(base_binfo);

            if (FSM_by_Class.find(base_class) == FSM_by_Class.end()) continue;
            const auto& base_fsm = FSM_by_Class[base_class];

            for (const auto& [method, base_next_state] : base_fsm) {
                auto it = sub_fsm.find(method);
                if (it == sub_fsm.end()) {
                    fprintf(stderr, "[LSP Violation] Subclass '%s' missing method '%s' from base class.\n",
                            IDENTIFIER_POINTER(DECL_NAME(TYPE_NAME(subclass))), method.c_str());
                } else if (it->second != base_next_state) {
                    fprintf(stderr, "[LSP Violation] Subclass '%s' alters method '%s' transition. Expected %d, got %d.\n",
                            IDENTIFIER_POINTER(DECL_NAME(TYPE_NAME(subclass))), method.c_str(), base_next_state, it->second);
                }
            }
        }
    }
    
}


// Function to extract and store all template arguments of Flag
void process_TypestateClassConnector_args(tree tmpl_args) {
    int num_args = TREE_VEC_LENGTH(tmpl_args);

   
    TypestateClassConnector_args.clear();

   
    for (int i = 0; i < num_args; ++i) {
        tree arg = TREE_VEC_ELT(tmpl_args, i);

       
        if (TYPE_P(arg)) {
           
            tree type_decl = TYPE_NAME(arg);
            if (type_decl && TREE_CODE(type_decl) == TYPE_DECL) {
                const char* type_name = IDENTIFIER_POINTER(DECL_NAME(type_decl));
                if (type_name) {
                    TypestateClassConnector_args.push_back(type_name);  
                 
                } else {
               
                }
            } else {
               
            }
        } else if (TREE_CODE(arg) == INTEGER_CST) {
           
            HOST_WIDE_INT val = TREE_INT_CST_LOW(arg);
            TypestateClassConnector_args.push_back(std::to_string(val));  
   
        } else {
         
        }
    }

    if (TypestateClassConnector_args.empty()) {
        printf("Failed to extract any template arguments of TypestateClassConnectorFlag.\n");
    }
}
// Template instantiation processing

static void on_template_instantiation(void* gcc_data, void* user_data) {
    tree fn_decl = (tree)gcc_data; (void)user_data;
    if (!fn_decl || !DECL_TEMPLATE_INSTANTIATION(fn_decl)) return;

    if (TREE_CODE(fn_decl) == TYPE_DECL || TREE_CODE(fn_decl) == FUNCTION_DECL) {
        tree type_decl = DECL_CONTEXT(fn_decl);
        if (!type_decl || TREE_CODE(type_decl) != RECORD_TYPE) return;

        const char* decl_name = IDENTIFIER_POINTER(DECL_NAME(TYPE_NAME(type_decl)));
        if (!decl_name) return;

        if (decl_name) {
    // 1) Existing direct cases (keep them, harmless if they fire)
    if (strcmp(decl_name, "State") == 0) {
        if (tree ta = DECL_TI_ARGS(fn_decl)) process_typestate_template_args(ta);
        return;
    }
    if (strcmp(decl_name, "Timed_State") == 0) {
        if (tree ta = DECL_TI_ARGS(fn_decl)) process_timed_typestate_args(ta);
        return;
    }
    if (strcmp(decl_name, "TypestateClassConnector") == 0) {
        if (tree ta = DECL_TI_ARGS(fn_decl)) process_TypestateClassConnector_args(ta);
        return;
    }

    // 2) The important case: the OUTER template that contains the pack
    if (strcmp(decl_name, "Typestate_Template") == 0) {
        if (tree pack_args = DECL_TI_ARGS(fn_decl)) {
            // Each element of this pack is State<...> or Timed_State<...>
            process_typestate_template_pack(pack_args);
        }
        return;
    }
}
    }
}

void Visualize_FSMs_Per_Class() {
    std::unordered_map<tree, std::map<int, std::vector<std::pair<std::string, int>>>> FSMs_by_class;

    for (const auto& [enum_val, transitions] : Typestate_Rules) {
        if (EnumToClassMap.find(enum_val) == EnumToClassMap.end()) continue;

        tree class_tree = EnumToClassMap[enum_val];
        FSMs_by_class[class_tree][enum_val] = transitions;
    }

    for (const auto& [class_tree, class_fsms] : FSMs_by_class) {
        if (!class_tree || !TYPE_NAME(class_tree)) continue;
        const char* class_name = IDENTIFIER_POINTER(DECL_NAME(TYPE_NAME(class_tree)));
        if (!class_name) continue;

        std::string dot_file = std::string(class_name) + "_fsm.dot";
        std::string png_file = std::string(class_name) + "_fsm.png";

        std::ofstream dot(dot_file);
        dot << "digraph " << class_name << "_FSM {\n";
        dot << "  rankdir=LR;\n";
        dot << "  node [shape=circle];\n";

        for (const auto& [enum_val, transitions] : class_fsms) {
            for (const auto& [method, next_state] : transitions) {
                dot << "  " << enum_val << " -> " << next_state << " [label=\"" << method << "\"];\n";
            }
        }

        dot << "}\n";
        dot.close();

        std::string command = "dot -Tpng " + dot_file + " -o " + png_file;
        std::system(command.c_str());
        //std::cout << "[FSM] Graph for " << class_name << " written to " << png_file << "\n";
    }
}




static void Typestate_Visualisation(const std::string& filename,
                                    const std::string& imageFilename) {
    std::ofstream dotFile(filename);
    if (!dotFile.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    auto fmt = [](long long v) { return v ? std::to_string(v) : std::string("-"); };

    auto escape_label = [](std::string s) {
        // Escape backslashes and quotes for DOT label strings
        std::string out;
        out.reserve(s.size() * 2);
        for (char c : s) {
            if (c == '\\' || c == '"') out.push_back('\\');
            out.push_back(c);
        }
        return out;
    };

    dotFile << "digraph StateMachine {\n";
    dotFile << "    rankdir=LR;\n";
    dotFile << "    node [shape=circle];\n";

    // Track emitted edges to avoid duplicates.
    // Key is (from, to, method)
    struct EdgeKey {
        int from{};
        int to{};
        std::string method;
        bool operator<(const EdgeKey& other) const {
            if (from != other.from) return from < other.from;
            if (to   != other.to)   return to   < other.to;
            return method < other.method;
        }
    };
    std::set<EdgeKey> emitted;

    // 1) Timed edges with multiline labels (deduped)
    for (const auto& [from, edges] : Timed_Typestate_Rules) {
        for (const auto& e : edges) {
            const std::string& method = std::get<0>(e);
            int to                    = std::get<1>(e);
            const TimedInfo& ti       = std::get<2>(e);

            EdgeKey key{from, to, method};
            if (emitted.count(key)) continue; // already emitted

            emitted.insert(key);

            std::ostringstream lbl;
            lbl << method
               // << "\\nR=" << fmt(ti.release_ns)
                << " P="  << fmt(ti.period_ns)
                << " offest="  << fmt(ti.offest_ns)
                << " Upper=" << fmt(ti.U_deadline_ns)
                << " Lower="  << fmt(ti.L_deadline_ns)
                << " C="  << fmt(ti.criticality);

            dotFile << "    " << from << " -> " << to << " "
                    << "[label=\"" << escape_label(lbl.str()) << "\"];\n";
        }
    }

    // 2) Untimed edges only if not already emitted (deduped)
    for (const auto& [from, transitions] : Typestate_Rules) {
        for (const auto& [method, to] : transitions) {
            EdgeKey key{from, to, method};
            if (emitted.count(key)) continue; // a timed or prior untimed already covered it
            emitted.insert(key);

            dotFile << "    " << from << " -> " << to
                    << " [label=\"" << escape_label(method) << "\"];\n";
        }
    }

    dotFile << "}\n";
    dotFile.close();

    std::cout << "Graphviz .dot file generated: " << filename << std::endl;

    const std::string command = "dot -Tpng " + filename + " -o " + imageFilename;
    const int result = std::system(command.c_str());
    if (result == 0) {
        std::cout << "Graphviz image generated: " << imageFilename << std::endl;
    } else {
        std::cerr << "Error generating image. Ensure Graphviz is installed and available in PATH.\n";
    }
}




   void merge_typestate_rules_carefully() {
    std::map<int, std::vector<std::pair<std::string, int>>> Merged_Typestate_Rules;

    // Iterate over all entries in Typestate_Rules
    for (const auto& [enum_value, main_transitions] : Typestate_Rules) {
        // Check if the same enum_value exists in Subtype_Typestate_Rules
        if (Subtype_Typestate_Rules.find(enum_value) != Subtype_Typestate_Rules.end()) {
            const auto& subtype_transitions = Subtype_Typestate_Rules[enum_value];
            std::vector<std::pair<std::string, int>> merged_transitions;

            // Find the merge point (common prefix of transitions)
            size_t merge_point = 0;
            while (merge_point < main_transitions.size() && merge_point < subtype_transitions.size() &&
                   main_transitions[merge_point] == subtype_transitions[merge_point]) {
                merged_transitions.push_back(main_transitions[merge_point]);
                merge_point++;
            }

            // Add the remaining transitions separately after the merge point
            merged_transitions.insert(merged_transitions.end(),
                                      main_transitions.begin() + merge_point, main_transitions.end());
            merged_transitions.insert(merged_transitions.end(),
                                      subtype_transitions.begin() + merge_point, subtype_transitions.end());

            Merged_Typestate_Rules[enum_value] = merged_transitions;
        } else {
            // If no matching enum_value in Subtype_Typestate_Rules, just copy main transitions
            Merged_Typestate_Rules[enum_value] = main_transitions;
        }
    }

    // Add any remaining entries from Subtype_Typestate_Rules that are not in Typestate_Rules
    for (const auto& [enum_value, subtype_transitions] : Subtype_Typestate_Rules) {
        if (Merged_Typestate_Rules.find(enum_value) == Merged_Typestate_Rules.end()) {
            Merged_Typestate_Rules[enum_value] = subtype_transitions;
        }
    }

    // Overwrite Typestate_Rules with the merged result
    Typestate_Rules = Merged_Typestate_Rules;

    // Clear Subtype_Typestate_Rules (optional)
    Subtype_Typestate_Rules.clear();

    // Print the final merged typestate rules
    printf("\n--- Final Typestate Rules ---\n");
    for (const auto& [enum_value, transitions] : Typestate_Rules) {
        printf("Enum value: %d\n", enum_value);
        for (const auto& [function_name, next_state] : transitions) {
            printf("  Function: %s -> Next state: %d\n", function_name.c_str(), next_state);
        }
    }
}

    void ProcessSuperSubStates() {
   
    std::unordered_map<int, std::unordered_map<std::string, int>> state_tracker;
    int counter = 0;

   
    std::map<int, std::vector<std::pair<std::string, int>>> processed_states;

     for (const auto& [key, states] : Typestate_Rules) {
        std::vector<std::pair<std::string, int>> super_sub_states;

        for (const auto& [state, value] : states) {
            if (state_tracker[key].find(state) == state_tracker[key].end()) {
                // Superstate
                state_tracker[key][state] = ++counter;
                super_sub_states.emplace_back(state + "-superstate", state_tracker[key][state]);
                std::cout<<state<<"is superstate"<<"\n";
            } else {
            // Substate
                super_sub_states.emplace_back(state + "-substate", state_tracker[key][state]);
                std::cout<<state<< "is substate"<<"\n";
            }
        }

       
        processed_states[key] = super_sub_states;
    }

}




void print_typestate_rules() {
    printf("\n Typestate Rules \n");
    for (const auto& [enum_value, transitions] : Typestate_Rules) {
        printf("Enum value: %d\n", enum_value);
        for (const auto& [function_name, next_state] : transitions) {
            printf("  Function: %s -> Next state: %d\n", function_name.c_str(), next_state);
        }
    }

    printf("\n Subtype Typestate Rules \n");
    for (const auto& [enum_value, transitions] : Subtype_Typestate_Rules) {
        printf("Enum value: %d\n", enum_value);
        for (const auto& [function_name, next_state] : transitions) {
            printf("  Function: %s -> Next state: %d\n", function_name.c_str(), next_state);
        }
    }
}

bool leads_to(
    const std::map<int, std::vector<std::pair<std::string, int>>>& fsm,
    int from,
    int expected_dest,
    std::unordered_set<int>& visited
) {
    if (from == expected_dest) return true;
    if (visited.count(from)) return false;
    visited.insert(from);

    auto it = fsm.find(from);
    if (it == fsm.end()) return false;

    for (const auto& [_, next] : it->second) {
        if (leads_to(fsm, next, expected_dest, visited)) {
            return true;
        }
    }

    return false;
}

bool leads_to(
    const std::map<int, std::vector<std::pair<std::string, int>>>& fsm,
    int from,
    int expected_dest
) {
    std::unordered_set<int> visited;
    return leads_to(fsm, from, expected_dest, visited);
}
bool is_reachable_from_base_start(
    const std::map<int, std::vector<std::pair<std::string, int>>>& fsm,
    int base_start_state,
    int target
) {
    std::unordered_set<int> visited;
    return leads_to(fsm, base_start_state, target, visited);
}

bool method_reused_with_different_transition(
    const std::string& method,
    int start_state,
    int next_state,
    const std::map<int, std::vector<std::pair<std::string, int>>>& base_fsm
) {
    for (const auto& [base_src, base_transitions] : base_fsm) {
        for (const auto& [base_method, base_dst] : base_transitions) {
            if (base_method == method) {
                if (base_src != start_state || base_dst != next_state) {
                    return true;  // found a conflict
                }
            }
        }
    }
    return false;
}


bool merge_typestate_fsm_respecting_lsp(
    const std::string& subclass_name,
    const std::string& base_name,
    const std::map<int, std::vector<std::pair<std::string, int>>>& base_fsm,
    const std::map<int, std::vector<std::pair<std::string, int>>>& subclass_fsm,
    std::map<int, std::vector<std::pair<std::string, int>>>& merged_fsm
) {
    merged_fsm = base_fsm;

    // Collect all method names used in base FSM
    std::set<std::string> base_method_names;
    for (const auto& [_, transitions] : base_fsm) {
        for (const auto& [method, __] : transitions) {
            base_method_names.insert(method);
        }
    }

   

    for (const auto& [start_state, subclass_transitions] : subclass_fsm) {
        for (const auto& [method, next_state] : subclass_transitions) {

            // Detect reuse of any base method name from a different state
            if (base_method_names.count(method)) {
                bool identical_transition_found = false;
                for (const auto& [base_state, base_transitions] : base_fsm) {
                    for (const auto& [base_method, base_next] : base_transitions) {
                        if (base_method == method && base_state == start_state && base_next == next_state) {
                            identical_transition_found = true;
                            break;
                        }
                    }
                    if (identical_transition_found) break;
                }

                if (!identical_transition_found) {
                    error_at(UNKNOWN_LOCATION,
                        "Typestate LSP violation: method '%s' from base FSM is reused in subclass '%s' "
                        "with a different transition (subclass: %d → %d). Reusing method names with different semantics is not allowed.",
                        method.c_str(), subclass_name.c_str(),
                        start_state, next_state);
                    return false;
                }
            }

            bool method_in_base = false;
            bool allowed = false;

            if (method_reused_with_different_transition(method, start_state, next_state, base_fsm)) {
    error_at(UNKNOWN_LOCATION,
        "Typestate LSP violation: method '%s' reused in subclass '%s' with a different transition "
        "(subclass: %d → %d).",
        method.c_str(), subclass_name.c_str(),
        start_state, next_state);
    return false;
}


            // Case 1: method exists at this start state in base
            if (base_fsm.count(start_state)) {
                for (const auto& [base_method, base_next_state] : base_fsm.at(start_state)) {
                    if (base_method == method) {
                        method_in_base = true;

                        if (leads_to(subclass_fsm, next_state, base_next_state)) {
                            allowed = true;
                            break;
                        } else {
                            error_at(UNKNOWN_LOCATION,
                                "Typestate LSP violation: subclass '%s' redirects method '%s' from state %d "
                                "to incompatible state %d (base expects %d).",
                                subclass_name.c_str(), method.c_str(),
                                start_state, next_state, base_next_state);
                            return false;
                        }
                    }
                }

                // Case 2: new method at a known state — allow if leads to base-valid state
                if (!method_in_base) {
                    for (const auto& [_, base_next_state] : base_fsm.at(start_state)) {
                        if (leads_to(subclass_fsm, next_state, base_next_state)) {
                            allowed = true;
                            break;
                        }
                    }
                }
            }

            // Case 3: method added from unknown state — must be reachable
            if (!method_in_base && !allowed) {
                bool known_start = merged_fsm.count(start_state);

                if (!known_start) {
                    for (const auto& [_, transitions] : merged_fsm) {
                        for (const auto& [_, dst] : transitions) {
                            if (leads_to(subclass_fsm, dst, start_state)) {
                                known_start = true;
                                break;
                            }
                        }
                        if (known_start) break;
                    }
                }

                if (!known_start) {
                    error_at(UNKNOWN_LOCATION,
                        "Typestate LSP violation: subclass '%s' introduces method '%s' from state %d, "
                        "which is not reachable from any known base or merged FSM state.",
                        subclass_name.c_str(), method.c_str(), start_state);
                    return false;
                }

                allowed = true;
            }

            // Merge transition if allowed
            if (allowed) {
                merged_fsm[start_state].emplace_back(method, next_state);
            }
        }
    }

    return true;
}




bool try_merge_fsms_respecting_lsp_for_class_pair() {
    static std::map<std::string, std::map<int, std::vector<std::pair<std::string, int>>>> FSMs_by_class;

    // Step 1: Build FSMs_by_class safely
    for (const auto& [enum_val, transitions] : Typestate_Rules) {
        if (EnumToClassMap.find(enum_val) == EnumToClassMap.end()) continue;

        tree class_tree = EnumToClassMap[enum_val];
        if (!class_tree || TREE_CODE(class_tree) != RECORD_TYPE) continue;

        const char* cname = nullptr;
        if (TYPE_NAME(class_tree) && DECL_NAME(TYPE_NAME(class_tree))) {
            cname = IDENTIFIER_POINTER(DECL_NAME(TYPE_NAME(class_tree)));
        }
        if (!cname) continue;

        FSMs_by_class[cname][enum_val] = transitions;
    }

    // Step 2: Pair subclass/baseclass combinations and try merge
    for (const auto& [e_sub, subclass_tree] : EnumToClassMap) {
        if (!subclass_tree || TREE_CODE(subclass_tree) != RECORD_TYPE) continue;

        for (const auto& [e_base, base_tree] : EnumToClassMap) {
            if (!base_tree || TREE_CODE(base_tree) != RECORD_TYPE) continue;
            if (subclass_tree == base_tree) continue;
            if (!is_subclass(subclass_tree, base_tree)) continue;

            const char* sub_cname = nullptr;
            const char* base_cname = nullptr;

            if (TYPE_NAME(subclass_tree) && DECL_NAME(TYPE_NAME(subclass_tree)))
                sub_cname = IDENTIFIER_POINTER(DECL_NAME(TYPE_NAME(subclass_tree)));
            if (TYPE_NAME(base_tree) && DECL_NAME(TYPE_NAME(base_tree)))
                base_cname = IDENTIFIER_POINTER(DECL_NAME(TYPE_NAME(base_tree)));

            if (!sub_cname || !base_cname) continue;

            // Look up FSMs
            if (FSMs_by_class.find(base_cname) == FSMs_by_class.end() ||
                FSMs_by_class.find(sub_cname) == FSMs_by_class.end())
                continue;

            const auto& base_fsm = FSMs_by_class[base_cname];
            const auto& subclass_fsm = FSMs_by_class[sub_cname];

            std::map<int, std::vector<std::pair<std::string, int>>> merged_result;
            merge_typestate_fsm_respecting_lsp(
                sub_cname, base_cname, base_fsm, subclass_fsm, merged_result
            );
        }
    }

    return true;
}


// Callback function to be called at the end of compilation
static void on_finish(void* gcc_data, void* user_data) {
   
 (void)gcc_data;
 (void)user_data;
   
    if (TypestateClassConnector_args.empty()) {
      printf("Flag template was not instantiated or its template arguments were not extracted.\n");
    }

std::string base_name = TypestateClassConnector_args[0];
std::string dot_file = base_name + "_state_machine.dot";
std::string png_file = base_name + "_state_machine.png";

Typestate_Visualisation(dot_file.c_str(), png_file.c_str());
// LSP Subclass FSM Validation
for (const auto& [sub_enum, sub_class] : EnumToClassMap) {
    if (!sub_class || TREE_CODE(sub_class) != RECORD_TYPE) continue;

    for (const auto& [super_enum, super_class] : EnumToClassMap) {
        if (!super_class || TREE_CODE(super_class) != RECORD_TYPE || sub_class == super_class) continue;

        if (is_subclass(sub_class, super_class)) {
            const auto& sub_fsm = FSM_by_Class[sub_class];
            const auto& super_fsm = FSM_by_Class[super_class];

            for (const auto& [method, sub_target] : sub_fsm) {
                auto it = super_fsm.find(method);
                if (it != super_fsm.end()) {
                    int super_target = it->second;
                    if (sub_target != super_target) {
                        const char* sub_class_name = "<unknown>";
                        const char* super_class_name = "<unknown>";

                        if (TYPE_NAME(sub_class) && DECL_NAME(TYPE_NAME(sub_class)))
                            sub_class_name = IDENTIFIER_POINTER(DECL_NAME(TYPE_NAME(sub_class)));
                        if (TYPE_NAME(super_class) && DECL_NAME(TYPE_NAME(super_class)))
                            super_class_name = IDENTIFIER_POINTER(DECL_NAME(TYPE_NAME(super_class)));

                        warning(0, "Typestate LSP violation: method '%s' in subclass '%s' transitions to %d, but in superclass '%s' it transitions to %d.",
                            method.c_str(), sub_class_name, sub_target, super_class_name, super_target);
                    }
                }
            }
        }
    }
    //validate_LSP_compliance();
 
 //   print_annotated_wcet_list();     // 1) show the collected list
   
     //Visualize_FSMs_Per_Class();
}

//print_typestate_rules();
//merge_typestate_rules_carefully();

}




/**
 Methods to Resolve Object References and Aliases:
   - `get_original_object` function simplifies complex references into their base forms, handling various cases
   like dereferencing and address calculations.
   - `track_all_aliases` function manages to track aliases across different scopes using
   local and global alias maps and resolves them to their original addresses.
**/

tree get_original_object(tree arg) {
    if (!arg) return arg;

    while (true) {
        enum tree_code code = TREE_CODE(arg);

        if (code == SSA_NAME) {
            // Resolve SSA_NAME to its underlying variable
            tree base_var = SSA_NAME_VAR(arg);
            if (base_var) {
                arg = base_var;
            } else {
                break;
            }
        } else if (code == MEM_REF || code == COMPONENT_REF || code == INDIRECT_REF || code == ARRAY_REF) {
            // Handle member access, pointer dereference, and array access
            arg = TREE_OPERAND(arg, 0);
        } else if (code == NOP_EXPR || code == CONVERT_EXPR || code == NON_LVALUE_EXPR) {

            arg = TREE_OPERAND(arg, 0);
        } else if (code == ADDR_EXPR) {
            // Address-of operator
            arg = TREE_OPERAND(arg, 0);
        } else {
            break;
        }
        if (!arg) {
            break;
        }
    }

    return arg;

}



tree track_all_aliases(tree obj_address) {
    if (!obj_address) return nullptr;


    std::unordered_set<tree> visited;

    // Resolve references first
    obj_address = get_original_object(obj_address);
    auto resolve_with_map = [&](std::unordered_map<tree, tree>& alias_map, const char* map_name) {
        while (alias_map.find(obj_address) != alias_map.end()) {
            if (visited.find(obj_address) != visited.end()) {
              (void)map_name;
                break;
            }
            visited.insert(obj_address);
            obj_address = alias_map[obj_address];
        }
    };

    // Check alias maps in the call stack
    for (auto& frame : call_stack) {
        resolve_with_map(frame.alias_map, "call_stack");
    }

    // Check the global alias map
    resolve_with_map(global_alias_map, "global_alias_map");
    return obj_address;
}


/**
 *
 * State_Manager Class
 * This class is central to managing the state of objects within the program.
 * The class also logs state changes, validates method calls against the current state of an object,
 * and manages transitions to new states as dictated by the info extracted from `typestate` template.
 * This is crucial for enforcing correct program behavior and debugging complex state-dependent interactions in the software.
   
 **/


class State_Manager {
public:
    // Register an alias for an object
    void register_alias(tree alias_address, tree original_address) {
    if (!alias_address || !original_address) return;


    original_address = track_all_aliases(original_address);
    call_stack.back().alias_map[alias_address] = original_address;

}

    // Register a method call and enforce the state transitions
void Typestate_Checking(tree obj, const std::string& method_name, location_t location) {
    tree obj_address = get_original_object(obj);
    tree resolved_obj = track_all_aliases(obj_address);
    if (!resolved_obj) return;

    if (object_states.find(resolved_obj) == object_states.end()) {
        object_states[resolved_obj] = 0;
    }

    int current_state = object_states[resolved_obj];

    // -------------------- CHANGE #1: recognize branch methods too --------------------
    bool is_transition_method = false;

    // normal rules
    for (const auto& state_entry : Typestate_Rules) {
        for (const auto& trans : state_entry.second) {
            if (trans.first == method_name) { is_transition_method = true; break; }
        }
        if (is_transition_method) break;
    }

    // branch rules (only if not already found)
    if (!is_transition_method) {
        auto itB = Branch_Rules.find(current_state);
        if (itB != Branch_Rules.end()) {
            for (const auto& br : itB->second) {
                if (br.method == method_name) { is_transition_method = true; break; }
            }
        } else {
            // also consider branch methods that exist in ANY state (optional but helpful)
            for (const auto& state_entry : Branch_Rules) {
                for (const auto& br : state_entry.second) {
                    if (br.method == method_name) { is_transition_method = true; break; }
                }
                if (is_transition_method) break;
            }
        }
    }
    // -------------------------------------------------------------------------------

    if (!is_transition_method) {
        method_calls[resolved_obj].push_back(method_name);
        return;
    }

    int next_state = -1;

    // -------------------- Normal typestate lookup --------------------
    auto it = Typestate_Rules.find(current_state);
    if (it != Typestate_Rules.end()) {
        for (const auto& trans : it->second) {
            if (trans.first == method_name) {
                next_state = trans.second;
                break;
            }
        }
    }

    // -------------------- CHANGE #2: branch fallback (single-state merge) --------------------
    // If no normal rule found, try Branch_Rules[current_state] for this method and pick ONE.
    if (next_state == -1) {
        auto itBr = Branch_Rules.find(current_state);
        if (itBr != Branch_Rules.end()) {
            for (const auto& br : itBr->second) {
                if (br.method == method_name) {
                    // single-state merge policy: pick the first matching branch rule
                    next_state = br.next;

                    fprintf(stderr,
                            "[Coconut] BRANCH PICK: state %d method '%s' picked cond=%d next=%d\n",
                            current_state, method_name.c_str(), br.cond, br.next);
                    fflush(stderr);

                    break;
                }
            }
        }
    }
    // ----------------------------------------------------------------------------------------

    if (next_state != -1) {
        object_states[resolved_obj] = next_state;
        printf("Transition: Method '%s' caused the state change from %d to %d.",
               method_name.c_str(), current_state, next_state);
    } else {
        error_at(location, "Error: Method '%s' is not allowed in the current state %d.",
                 method_name.c_str(), current_state);
    }

    method_calls[resolved_obj].push_back(method_name);
}


   
   tree track_all_aliases(tree obj_address) {
    if (!obj_address) return nullptr;

   
    for (auto& frame : call_stack) {
        if (frame.alias_map.find(obj_address) != frame.alias_map.end()) {
            obj_address = frame.alias_map[obj_address];
        }
    }

    std::unordered_set<tree> visited;
    while (global_alias_map.find(obj_address) != global_alias_map.end()) {
        if (visited.find(obj_address) != visited.end()) {
            printf("Alias resolution detected a cycle; breaking.\n");
            break;
        }
        visited.insert(obj_address);
        obj_address = global_alias_map[obj_address];
    }

    return obj_address;
}

    void print_state() {
        for (const auto& entry : object_states) {
            printf("  Object %p is in state %d\n", (void*)entry.first, entry.second);
        }

        for (const auto& entry : method_calls) {
            printf("  Object %p called methods: ", (void*)entry.first);
            for (const auto& method : entry.second) {
                printf("%s ", method.c_str());
            }
           printf("\n");
        }
    }


      std::unordered_map<tree, int> object_states;      
       std::unordered_map<tree, std::vector<std::string>> method_calls;  
private:
    std::unordered_map<tree, tree> global_alias_map;  
   
};

State_Manager state_manager;


/**
 * methods are used for analysing typestate transitions in branching
 * control-flow constructs (e.g., if-else or switch) within a program.
 * detect common joint states for all branches,
 * and once the branches are completed, the object is moved to the first common jintstate.
 * ensure that each method call inside the branches adheres to the specified typestate rules.
 *
 */
// Print incoming-edge counts for every state, then return the "joint" set (incoming > 1).
static std::set<int>
findCommonJointStatesAndPrint(const std::map<int, std::vector<std::pair<std::string,int>>>& rules) {
    std::unordered_map<int,int> incoming_count;

    for (const auto& [src, vec] : rules) {
        (void)src;
        for (const auto& [name, dst] : vec) {
            (void)name;
            incoming_count[dst]++;
        }
    }

    fprintf(stderr, " Typestate incoming-edge counts \n");
    for (const auto& [state, cnt] : incoming_count) {
        fprintf(stderr, "  state %d <- %d incoming\n", state, cnt);
    }

    std::set<int> joint_states;
    for (const auto& [state, cnt] : incoming_count) {
        if (cnt > 1) joint_states.insert(state);
    }

    fprintf(stderr, "Joint states (>1 incoming): {");
    bool first = true;
    for (int s : joint_states) { fprintf(stderr, "%s%d", first?"":", ", s); first = false; }
    fprintf(stderr, "}\n");
    return joint_states;
}


int get_if_else_id_for_condition(gimple* cond_stmt) {
    auto it = condition_id_map.find(cond_stmt);
    if (it != condition_id_map.end()) {
        return it->second;
    } else {
        int id = next_if_else_id++;
        condition_id_map[cond_stmt] = id;
   
        if_else_context_map[id] = {3, 0};
        if_else_status[id] = false;
        return id;
    }

     
}


std::unordered_map<gimple*, int> if_else_id_map;  // Maps conditionals to IDs
std::unordered_map<basic_block, int> bb_to_if_else_id;  // Maps BBs to IDs
int if_else_counter = 0;

gimple* get_controlling_cond(basic_block bb) {
    edge e;
    edge_iterator ei;
    FOR_EACH_EDGE(e, ei, bb->preds) {
        basic_block pred = e->src;
        gimple_stmt_iterator gsi = gsi_last_bb(pred);
        if (gsi_end_p(gsi))
            continue;
        gimple* stmt = gsi_stmt(gsi);
        if (gimple_code(stmt) == GIMPLE_COND)
            return stmt;
    }
    return nullptr;
}


std::string demangle(const char* mangled) {
    if (!mangled || !*mangled) {
        return "";
    }
    int status = 0;
    char* demangled_name = abi::__cxa_demangle(mangled, nullptr, nullptr, &status);
    std::string result;
    if (status == 0 && demangled_name) {
        result = demangled_name;
        free(demangled_name);
    } else {
        result = mangled;
    }
    return result;
}

bool is_inside_branch(basic_block bb) {
    if (!bb) return false;

    edge e;
    edge_iterator ei;
    FOR_EACH_EDGE(e, ei, bb->preds) {
        basic_block pred_bb = e->src;
        for (gimple_stmt_iterator gsi = gsi_start_bb(pred_bb); !gsi_end_p(gsi); gsi_next(&gsi)) {
            gimple* stmt = gsi_stmt(gsi);
            if (gimple_code(stmt) == GIMPLE_COND) {
                // Found a conditional statement
                
                printf("Debug: Entering conditional branch in basic block %p.\n", (void*)bb);
                return true;
            }
            if (gimple_code(stmt) == GIMPLE_SWITCH) {
                // Found a `switch` conditional statement
            // printf("Debug: Entering switch branch in basic block %p.\n", (void*)bb);
                return true;
            }
        }
    }
    return false;
}

int finalize_if_else_block(int if_else_id) {
    if (if_else_status[if_else_id]) {
        // Compute joint states now that all branches are complete
        std::set<int> detected_joint_states = findCommonJointStatesAndPrint(Typestate_Rules);

        // Update the global joint_states set
        for (int st : detected_joint_states) {
            joint_states.insert(st);
        }

        int first_joint_state = -1;
        if (!detected_joint_states.empty()) {
            first_joint_state = *detected_joint_states.begin();
        }

       // printf("Debug: Finalizing if-else block %d with %d joint states.\n", if_else_id, detected_joint_states.size());

        last_finalized_if_else_id = if_else_id;
        last_finalized_first_state = first_joint_state;

        current_if_else_id++;
        return first_joint_state;
    } else {
       //printf("Debug: If-else block %d is incomplete.\n", if_else_id);
        return -1;
    }
}

#include "gcc-plugin.h"
#include "basic-block.h"
#include "gimple.h"
#include "gimple-iterator.h"
#include "cfgloop.h"
#include "cfgloop.h"
#include "cfgloopmanip.h"

// 1) Normalize names (rules may be unqualified; analysis often demangles qualified)

// 2) Rule lookups that accept both qualified and unqualified names
#include <unordered_set>
#include <cstdio>



static inline std::pair<int,int> pick_two_arms(const std::vector<int>& cand) {
    int t = -1, f = -1;
    for (int s : cand) {
        if (t < 0) t = s;
        else if (f < 0 && s != t) { f = s; break; }
    }
    if (t >= 0 && f < 0) f = t; // single unique candidate → both arms same
    return {t, f};
}

// Works for plain bool/int literals (INTEGER_CST) and enum constants (CONST_DECL -> INTEGER_CST).
static bool is_falsey_const(tree t) {
    if (!t) return false;
    if (TREE_CODE(t) == INTEGER_CST) return integer_zerop(t);
    if (TREE_CODE(t) == CONST_DECL) {
        tree val = DECL_INITIAL(t);
        return val && TREE_CODE(val) == INTEGER_CST && integer_zerop(val);
    }
    return false;
}

static bool is_truthy_const(tree t) {
    if (!t) return false;
    if (TREE_CODE(t) == INTEGER_CST) return !integer_zerop(t);
    if (TREE_CODE(t) == CONST_DECL) {
        tree val = DECL_INITIAL(t);
        return val && TREE_CODE(val) == INTEGER_CST && !integer_zerop(val);
    }
    return false;
}



static bool ssa_derives_from(tree a, tree b, int depth = 6) {
    if (!a || !b) return false;
    if (a == b) return true;
    if (TREE_CODE(a) != SSA_NAME || depth <= 0) return false;
    gimple* def = SSA_NAME_DEF_STMT(a);
    if (!def) return false;

    if (is_gimple_call(def)) return gimple_call_lhs(def) == b;

    if (is_gimple_assign(def)) {
        if (gimple_num_ops(def) >= 2) {
            tree r1 = gimple_assign_rhs1(def);
            if (r1 == b || (TREE_CODE(r1)==SSA_NAME && ssa_derives_from(r1,b,depth-1))) return true;
        }
        if (gimple_num_ops(def) >= 3) {
            tree r2 = gimple_assign_rhs2(def);
            if (r2 == b || (TREE_CODE(r2)==SSA_NAME && ssa_derives_from(r2,b,depth-1))) return true;
        }
        if (gimple_num_ops(def) >= 4) {
            tree r3 = gimple_assign_rhs3(def);
            if (r3 == b || (TREE_CODE(r3)==SSA_NAME && ssa_derives_from(r3,b,depth-1))) return true;
        }
    }
    return false;
}

// Follow SSA back to the defining call (up to small depth)
static gimple* ssa_def_call(tree ssa, int depth = 6) {
    if (!ssa || depth <= 0 || TREE_CODE(ssa) != SSA_NAME) return nullptr;
    gimple* def = SSA_NAME_DEF_STMT(ssa);
    if (!def) return nullptr;
    if (is_gimple_call(def)) return def;
    if (is_gimple_assign(def)) {
        for (int i = 1; i <= 3 && gimple_num_ops(def) >= i+1; ++i) {
            tree r = (i==1)? gimple_assign_rhs1(def)
                   : (i==2)? gimple_assign_rhs2(def)
                           : gimple_assign_rhs3(def);
            if (TREE_CODE(r) == SSA_NAME)
                if (auto* c = ssa_def_call(r, depth-1)) return c;
        }
    }
    return nullptr;
}

// Given a GIMPLE_COND, recover the *call* whose result it tests.
static gimple* find_guard_call_for_cond(gimple* term) {
    if (!term || gimple_code(term) != GIMPLE_COND) return nullptr;
    tree L = gimple_cond_lhs(term), R = gimple_cond_rhs(term);
    if (TREE_CODE(L) == SSA_NAME) if (auto* c = ssa_def_call(L)) return c;
    if (TREE_CODE(R) == SSA_NAME) if (auto* c = ssa_def_call(R)) return c;

    // Fallback: scan the terminator’s block
    basic_block bb = gimple_bb(term);
    for (gimple_stmt_iterator gsi = gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi)) {
        gimple* s = gsi_stmt(gsi);
        if (!is_gimple_call(s)) continue;
        tree res = gimple_call_lhs(s);
        if (!res || TREE_CODE(res) != SSA_NAME) continue;
        bool lhs_from = (L == res) || (TREE_CODE(L)==SSA_NAME && ssa_derives_from(L,res));
        bool rhs_from = (R == res) || (TREE_CODE(R)==SSA_NAME && ssa_derives_from(R,res));
        if (lhs_from || rhs_from) return s;
    }
    return nullptr;
}



// Does the COND compare the call result 'res' to a constant K ?

// Is the COND effectively "if (res)" or "if (!res)

// Does this call's result feed a GIMPLE_SWITCH index?
static bool call_feeds_switch(gimple* call_stmt, gimple* term) {
    if (!call_stmt || !term || gimple_code(term) != GIMPLE_SWITCH) return false;
    tree res = gimple_call_lhs(call_stmt);
    if (!res || TREE_CODE(res) != SSA_NAME) return false;

    const gswitch* gs = as_a<const gswitch*>(term);
    tree idx = gimple_switch_index(gs);
    if (!idx) return false;

    if (idx == res) return true;
    if (TREE_CODE(idx) == SSA_NAME && ssa_derives_from(idx, res)) return true;
    return false;
}

// Extract (edge, case-value) pairs. Treat default separately in your code.


// Does the TRUE edge of 'term' correspond to "call_result == true"?
static bool true_edge_means_call_true(gimple* term, gimple* call_stmt) {
    if (!term || gimple_code(term) != GIMPLE_COND || !call_stmt) return true; // default
    tree res = gimple_call_lhs(call_stmt);
    if (!res || TREE_CODE(res) != SSA_NAME) return true;

    enum tree_code code = gimple_cond_code(term); // EQ_EXPR / NE_EXPR / etc.
    tree lhs = gimple_cond_lhs(term);
    tree rhs = gimple_cond_rhs(term);

    bool lhs_is_call = (lhs == res) || (TREE_CODE(lhs)==SSA_NAME && ssa_derives_from(lhs,res));
    bool rhs_is_call = (rhs == res) || (TREE_CODE(rhs)==SSA_NAME && ssa_derives_from(rhs,res));
    if (!lhs_is_call && !rhs_is_call) return true; // default

    tree other = lhs_is_call ? rhs : lhs;

    if (code == EQ_EXPR || code == NE_EXPR) {
        bool other_falsey = is_falsey_const(other);
        bool other_truthy = !other_falsey;

        // TRUE edge for EQ means (call == other); for NE means (call != other).
        // If 'other' is truthy → EQ: TRUE means call==true; NE: TRUE means call==false.
        // If 'other' is falsey → EQ: TRUE means call==false; NE: TRUE means call==true.
        if (code == EQ_EXPR) return other_truthy;
        else                 return !other_truthy;
    }

  
    return true;
}


static int branch_next_for_cond(int src_state,
                                const std::string& method,
                                long long K)
{
    auto itB = Branch_Rules.find(src_state);
    if (itB == Branch_Rules.end()) return -1;

    const std::string uq = unqualified_name(method);

    for (const auto& br : itB->second) {
        // br.method is usually unqualified already, but match both.
        if (br.method == method || br.method == uq) {
            if (br.cond == (int)K) return br.next;
        }
    }
    return -1;
}

static std::vector<int> transitions_for(int src, const std::string& method, location_t loc = UNKNOWN_LOCATION) {
    std::vector<int> out;

    const std::string uq = unqualified_name(method);
    std::unordered_set<int> seen;

    // -------------------- NEW: read both normal + branch tables --------------------
    bool any_table_for_src = false;

    // 1) Normal rules
    auto it = Typestate_Rules.find(src);
    if (it != Typestate_Rules.end()) {
        any_table_for_src = true;

        for (const auto& kv : it->second) {
            const std::string& name = kv.first;
            int dst = kv.second;

            bool name_match = (name == method) || (name == uq);
            if (!name_match) continue;

            if (!seen.count(dst)) {
                seen.insert(dst);
                out.push_back(dst);
            }
        }
    }

    // 2) Branch rules (ALL options)
    auto itB = Branch_Rules.find(src);
    if (itB != Branch_Rules.end()) {
        any_table_for_src = true;

        for (const auto& br : itB->second) {
            // br.method is already unqualified typically, but we match both ways anyway
            bool name_match = (br.method == method) || (br.method == uq);
            if (!name_match) continue;

            int dst = br.next;
            if (!seen.count(dst)) {
                seen.insert(dst);
                out.push_back(dst);
            }
        }
    }

    // Only error if src truly has no rules anywhere
    if (!any_table_for_src) {
        error_at(loc, "Typestate: no rule entry for state %d and method '%s'.",
                 src, method.c_str());
        return out;
    }

    return out;
}

// 3) Robustly detect if a call feeds this BB's terminator (through SSA wrappers)
static bool tree_derives_from(tree t, tree base, int depth = 6) {
    if (!t || !base) return false;
    if (t == base) return true;
    if (TREE_CODE(t) != SSA_NAME || depth <= 0) return false;

    gimple* def = SSA_NAME_DEF_STMT(t);
    if (!def) return false;

    if (is_gimple_call(def)) {
        return gimple_call_lhs(def) == base;
    } else if (is_gimple_assign(def)) {
        if (gimple_num_ops(def) >= 2) {
            tree r1 = gimple_assign_rhs1(def);
            if (r1 == base) return true;
            if (TREE_CODE(r1) == SSA_NAME && tree_derives_from(r1, base, depth - 1)) return true;
        }
        if (gimple_num_ops(def) >= 3) {
            tree r2 = gimple_assign_rhs2(def);
            if (r2 == base) return true;
            if (TREE_CODE(r2) == SSA_NAME && tree_derives_from(r2, base, depth - 1)) return true;
        }
        if (gimple_num_ops(def) >= 4) {
            tree r3 = gimple_assign_rhs3(def);
            if (r3 == base) return true;
            if (TREE_CODE(r3) == SSA_NAME && tree_derives_from(r3, base, depth - 1)) return true;
        }
    }
    return false;
}

// 4) Choose (TRUE,FALSE) destinations deterministically from candidate states
static inline std::pair<int,int> pick_true_false(const std::vector<int>& cand) {
    int t = -1, f = -1;
    for (int s : cand) {
        if (t < 0) t = s;
        else if (f < 0 && s != t) { f = s; break; }
    }
    if (t >= 0 && f < 0) f = t;  // only one candidate → both arms same
    return {t, f};
}


static std::unordered_map<gimple*, int> cond_to_ifelse_id;
std::unordered_map<tree, int> object_state_branch;
std::unordered_map<tree, int> object_stateper_branch;
std::map<int, std::map<tree, int>> branch_state_map;





// Return the controlling terminator (COND/SWITCH) that leads into 'bb'.
// Skips loop back-edges so loops are not confused with branches.
static gimple* controlling_terminator_of(basic_block bb) {
    if (!bb) return nullptr;
    edge e; edge_iterator ei;
    FOR_EACH_EDGE(e, ei, bb->preds) {
        basic_block pred = e->src;
        gimple_stmt_iterator gsi = gsi_last_bb(pred);
        if (gsi_end_p(gsi)) continue;
        gimple* last = gsi_stmt(gsi);
        enum gimple_code code = gimple_code(last);
        if (code == GIMPLE_COND || code == GIMPLE_SWITCH) {
            if (e->flags & EDGE_DFS_BACK) continue; // skip loop back-edges
            return last;
        }
    }
    return nullptr;
}

// Stable id for a given controlling terminator (so both arms share it).
static int get_if_else_id_for_terminator(gimple* term) {
    if (!term) return -1;
    auto it = cond_to_ifelse_id.find(term);
    if (it != cond_to_ifelse_id.end()) return it->second;
    int id = if_else_counter++;
    cond_to_ifelse_id[term] = id;
    return id;
}

// Stable id for a successor BB (uses its controlling predecessor terminator).
int get_if_else_id_for_bb(basic_block bb) {
    if (!bb) return -1;
    if (auto it = bb_to_if_else_id.find(bb); it != bb_to_if_else_id.end())
        return it->second;

    gimple* term = controlling_terminator_of(bb);
    if (!term) return -1;

    int id = get_if_else_id_for_terminator(term);
    bb_to_if_else_id[bb] = id;
    return id;
}

// True if BB is reached from an if/switch (non loop-back).
bool is_inside_if_or_switch(basic_block bb) {
    return controlling_terminator_of(bb) != nullptr;
}








//  Branch context & strict join 

using Bitset = std::bitset<64>;      
int NUM_STATES = -1;        // discovered elsewhere

static std::unordered_map<tree, Bitset> g_else_chain_cands;
static std::unordered_map<tree, std::vector<int>> g_saved_state_stack;
struct ArmChoiceSet {
    Bitset tset;
    Bitset fset;
};

enum BranchKind { BK_NONE, BK_IF, BK_SWITCH };

struct BranchCtx {
    int expected = 0;
    int visited  = 0;
    std::unordered_map<tree,int> pre;
    std::unordered_map<tree,std::vector<Bitset>> outs;

    // TRUE/FALSE seed per object
    std::unordered_map<tree, std::pair<int,int>> arm_choice;

    // final state per arm, per object
    std::unordered_map<tree, std::vector<int>> finals;
    std::unordered_map<tree, int> sw_current_case_bb; // last case BB index we were in (-1 if none)
    std::unordered_map<tree, int> sw_active_case_val;   // per object: current case value (-1 for default)
std::unordered_map<tree, int> sw_active_case_bb;    // per object: current case bb index
    int guard_pre_state = -1;
    std::string guard_method;

    Bitset remaining;                 // ELSE-chain candidate states (Bitset)
    bool remaining_init = false;      // ELSE-chain init flag (bool)

    // Per-object per-ifid candidate sets for TRUE/FALSE successors
    std::unordered_map<tree, ArmChoiceSet> arm_sets;
    std::unordered_map<tree, Bitset> active_set;

    std::unordered_map<tree, std::vector<int>> state_stack;

    
    gimple* sw_term = nullptr;                     // which SWITCH terminator this ctx corresponds to
    int sw_expected_default = 0;                   // number of switch arms (labels or unique dests)

    // per-object countdown for this switch (key = norm_obj_key(obj))
    std::unordered_map<tree, int> sw_remaining;
    std::unordered_map<tree, int> sw_remaining_init;

    // union of end states seen across completed cases (per object)
    std::unordered_map<tree, Bitset> sw_exit_union;

    // in BranchCtx:
std::unordered_map<tree, basic_block> if_current_arm_bb;
};


 
static inline tree norm_obj_key(tree t) {
    t = get_original_object(t);
    return track_all_aliases(t);
}

static void push_obj_state(BranchCtx& ctx, tree obj, int s) {
    tree key = norm_obj_key(obj);
    ctx.state_stack[key].push_back(s);
}

static void pop_obj_state(BranchCtx& ctx, tree obj, int &out_s) {
    tree key = norm_obj_key(obj);
    auto &stk = ctx.state_stack[key];
    if (!stk.empty()) {
        out_s = stk.back();
        stk.pop_back();
    }
}

// Cache "the guard call that produced the variable used in the if-chain"
// Keyed by (normalized object key).
static std::unordered_map<tree, int>         LAST_GUARD_PRE;
static std::unordered_map<tree, std::string> LAST_GUARD_METHOD;
static std::unordered_map<tree, Bitset>      LAST_GUARD_REMAINING;
static std::unordered_map<tree, bool>        LAST_GUARD_REMAINING_INIT;


static void save_last_guard(tree obj_address, int pre, const std::string& method,
                            const Bitset* remaining = nullptr, bool remaining_init = false)
{
    tree k = norm_obj_key(obj_address);
    if (!k) return;

    LAST_GUARD_PRE[k] = pre;
    LAST_GUARD_METHOD[k] = method;

    if (remaining) {
        LAST_GUARD_REMAINING[k] = *remaining;
        LAST_GUARD_REMAINING_INIT[k] = remaining_init;
    }

    fprintf(stderr, "Debug: Save guard origin: objkey=%p pre=%d method=%s\n",
            (void*)k, pre, method.c_str());
}


static bool load_last_guard(tree obj_address, int &pre_out, std::string &method_out,
                            Bitset* remaining_out = nullptr, bool* remaining_init_out = nullptr)
{
    tree k = norm_obj_key(obj_address);
    if (!k) return false;

    auto itp = LAST_GUARD_PRE.find(k);
    auto itm = LAST_GUARD_METHOD.find(k);
    if (itp == LAST_GUARD_PRE.end() || itm == LAST_GUARD_METHOD.end()) return false;

    pre_out = itp->second;
    method_out = itm->second;

    if (remaining_out && remaining_init_out) {
        auto itr = LAST_GUARD_REMAINING.find(k);
        auto iti = LAST_GUARD_REMAINING_INIT.find(k);
        if (itr != LAST_GUARD_REMAINING.end() && iti != LAST_GUARD_REMAINING_INIT.end()) {
            *remaining_out = itr->second;
            *remaining_init_out = iti->second;
        }
    }

    return true;
}


static std::pair<int,int> pick_true_false(const Bitset& bs) {
    int d1 = -1, d2 = -1;
    for (int i = 0; i < (int)bs.size(); ++i) if (bs.test(i)) {
        if (d1 < 0) d1 = i;
        else if (d2 < 0 && i != d1) { d2 = i; break; }
    }
    if (d1 >= 0 && d2 < 0) d2 = d1; // single candidate → both arms same
    return {d1, d2};
}

static std::unordered_map<int, BranchCtx> IFCTX; // ifid -> context

// Normalize object (base + aliases); reuse your helpers
static inline tree norm_obj(tree t) {
    t = get_original_object(t);
    t = track_all_aliases(t);
    return t;
}

static inline void record_pre_state(int ifid, tree obj, int pre_state) {
    BranchCtx& ctx = IFCTX[ifid];
    if (!ctx.pre.count(obj)) ctx.pre[obj] = pre_state;
}



static BranchCtx& ensure_ifctx(int id, gimple* term) {
    auto& ctx = IFCTX[id];

    // Keep your original expected/visited logic (IF and SWITCH both set expected)
    if (ctx.expected == 0 && term) {
        switch (gimple_code(term)) {
            case GIMPLE_COND:
                ctx.expected = 2;
                break;

            case GIMPLE_SWITCH: {
                const gswitch* gs = as_a<const gswitch*>(term);
                int n = (int)gimple_switch_num_labels(gs);
                ctx.expected = std::max(1, n);
                break;
            }

            default:
                ctx.expected = 2;
                break;
        }
    }
    if (ctx.expected == 0) ctx.expected = 2;

    // SWITCH-only cache (uses the fields you actually added)
    if (term && gimple_code(term) == GIMPLE_SWITCH) {
        if (ctx.sw_term != term) {
            ctx.sw_term = term;
            ctx.sw_expected_default = ctx.expected;   // reuse computed expected
        }
        if (ctx.sw_expected_default <= 0) ctx.sw_expected_default = ctx.expected;
    }

    return ctx;
}

static void record_arm_out_set(int ifid, tree obj, const Bitset& outset) {
    auto& ctx = IFCTX[ifid];
    obj = norm_obj(obj);
    if (!obj) return;
    ctx.outs[obj].push_back(outset);
}

static void end_arm(int ifid) {
    IFCTX[ifid].visited++;
}

// Intersect all arm out-sets per object; require a single concrete state.
static bool try_finalize_if_group(int ifid) {
    auto it = IFCTX.find(ifid);
    if (it == IFCTX.end()) return false;
    auto& ctx = it->second;
    if (ctx.visited < ctx.expected) return false;

    for (auto& kv : ctx.outs) {
        tree obj = kv.first;
        const auto& vec = kv.second;
        if (vec.empty()) continue;

        Bitset inter = vec.front();
        for (size_t i = 1; i < vec.size(); ++i) inter &= vec[i];

        if (inter.count() != 1) {
            error_at(UNKNOWN_LOCATION,
                     "Typestate: branches merge with %zu possible states (need exactly 1).",
                     (size_t)inter.count());
            // fallback: keep pre-state
            auto itp = ctx.pre.find(obj);
            if (itp != ctx.pre.end())
                state_manager.object_states[obj] = itp->second;
            continue;
        }

        int merged = -1;
        for (int s = 0; s < (int)inter.size(); ++s) if (inter.test(s)) { merged = s; break; }
        if (merged >= 0)
            state_manager.object_states[obj] = merged;
    }

    IFCTX.erase(it);
    return true;
}


// helper: does pred_bb → bb correspond to the TRUE edge?
static bool edge_is_true_successor(basic_block pred_bb, basic_block bb) {
    edge e; edge_iterator ei;
    FOR_EACH_EDGE(e, ei, pred_bb->succs) {
        if (e->dest == bb) return (e->flags & EDGE_TRUE_VALUE) != 0;
    }
    return false;
}




// Helper: wide-int -> signed host long long using the node's type precision
static inline long long to_ll_with_prec(const tree t) {
    // t is assumed INTEGER_CST by callers that use this; guard anyway.
    if (!t || TREE_CODE(t) != INTEGER_CST) return 0;
    return wi::to_wide(t, TYPE_PRECISION (TREE_TYPE (t))).to_shwi();
}

static bool is_zero(const tree t) {
    if (!t) return false;
    if (TREE_CODE(t) == INTEGER_CST)      return integer_zerop(t);
    if (TREE_CODE(t) == CONST_DECL) {
        tree v = DECL_INITIAL(t);
        return v && TREE_CODE(v) == INTEGER_CST && integer_zerop(v);
    }
    return false;
}

static bool const_name_and_value(tree t, std::string& name_out, long long& val_out) {
    name_out.clear(); val_out = 0;
    if (!t) return false;

    if (TREE_CODE(t) == CONST_DECL) {
        if (DECL_NAME(t)) name_out = IDENTIFIER_POINTER(DECL_NAME(t));
        tree v = DECL_INITIAL(t);
        if (v && TREE_CODE(v) == INTEGER_CST)
            val_out = to_ll_with_prec(v);     // <-- pass precision
        return true;
    }

    if (TREE_CODE(t) == INTEGER_CST) {
        val_out = to_ll_with_prec(t);          // <-- pass precision
        name_out = std::to_string(val_out);
        return true;
    }

    return false;
}

// Extracts: operator (== or !=), the constant 'K', and whether 'res' is on LHS.
// Returns true iff pattern is (res ==/!= K) or (K ==/!= res) with K a const/enum const.
static bool extract_eq_ne_with_const(gimple* term, tree res,
                                     CondOp& op_out, long long& K_out,
                                     bool& res_on_lhs, std::string& K_name)
{
    op_out = OP_OTHER; K_out = 0; res_on_lhs = false; K_name.clear();
    if (!term || gimple_code(term) != GIMPLE_COND || !res || TREE_CODE(res) != SSA_NAME)
        return false;

    enum tree_code op = gimple_cond_code(term);          // EQ_EXPR / NE_EXPR / ...
    if (op != EQ_EXPR && op != NE_EXPR) return false;

    tree L = gimple_cond_lhs(term);
    tree R = gimple_cond_rhs(term);

    auto is_res = [&](tree t){
        return t == res || (t && TREE_CODE(t) == SSA_NAME && ssa_derives_from(t, res));
    };

    // res ? K
    if (is_res(L) && (TREE_CODE(R) == INTEGER_CST || TREE_CODE(R) == CONST_DECL)) {
        long long Kv = 0; std::string Kn;
        const_name_and_value(R, Kn, Kv);
        op_out = (op == EQ_EXPR) ? OP_EQ : OP_NE;
        K_out = Kv; K_name = Kn; res_on_lhs = true;
        return true;
    }

    // K ? res
    if (is_res(R) && (TREE_CODE(L) == INTEGER_CST || TREE_CODE(L) == CONST_DECL)) {
        long long Kv = 0; std::string Kn;
        const_name_and_value(L, Kn, Kv);
        op_out = (op == EQ_EXPR) ? OP_EQ : OP_NE;
        K_out = Kv; K_name = Kn; res_on_lhs = false;
        return true;
    }

    return false;
}



static void record_final_state(int ifid, tree obj, int final_state) {
    auto& ctx = IFCTX[ifid];
    obj = norm_obj_key(obj);
    if (!obj) return;
    ctx.finals[obj].push_back(final_state);
}


void defer_branch_handling(int branch_id_param,
                           tree obj,
                           const std::string& method_name,
                           location_t location,
                           gimple* site_stmt)
{
    tree obj_key = norm_obj_key(obj);
    if (!obj_key) return;

    // Find current arm BB and its controlling terminator
    basic_block bb_here = site_stmt ? gimple_bb(site_stmt) : nullptr;
    if (!bb_here) {
        for (basic_block b = ENTRY_BLOCK_PTR_FOR_FN(cfun); b; b = b->next_bb) {
            gimple_stmt_iterator gsi = gsi_last_bb(b);
            if (!gsi_end_p(gsi)) { bb_here = b; break; }
        }
    }
    gimple* term = controlling_terminator_of(bb_here);

    // Recompute canonical branch id from the terminator; use this id everywhere
    int branch_id = (term ? get_if_else_id_for_terminator(term) : branch_id_param);
    if (term && branch_id >= 0 && branch_id != branch_id_param) {
        fprintf(stderr,
            "Debug: Branch id mismatch: passed=%d, recomputed=%d. Using recomputed id.\n",
            branch_id_param, branch_id);
        fflush(stderr);
    }

    // Ensure IFCTX exists for this branch id
    if (branch_id >= 0 && term) {
        (void)ensure_ifctx(branch_id, term);
    }

    // Store pre-branch state ONCE per (branch_id, object)
    if (branch_state_map[branch_id].find(obj_key) == branch_state_map[branch_id].end()) {
        if (state_manager.object_states.find(obj_key) == state_manager.object_states.end())
            state_manager.object_states[obj_key] = 0;
        branch_state_map[branch_id][obj_key] = state_manager.object_states[obj_key];
    }
    const int pre_branch = branch_state_map[branch_id][obj_key];

    if (object_stateper_branch.find(obj_key) == object_stateper_branch.end())
        object_stateper_branch[obj_key] = pre_branch;

    // ------------------- Build missing arm plan on demand -------------------
    auto build_arm_plan_now = [&](int ifid, tree key_obj, int pre_state, gimple* t) -> bool {
        if (!t) return false;

        auto& ctx = ensure_ifctx(ifid, t);
        if (ctx.arm_choice.count(key_obj)) return true;

        // Find guard call producing the value tested by this IF
        gimple* guard_call = (gimple_code(t) == GIMPLE_COND) ? find_guard_call_for_cond(t) : nullptr;
        if (!guard_call || !is_gimple_call(guard_call)) return false;

        tree gd = gimple_call_fndecl(guard_call);
        if (!gd || TREE_CODE(gd) != FUNCTION_DECL || !DECL_NAME(gd)) return false;

        std::string guard_name = demangle(IDENTIFIER_POINTER(DECL_NAME(gd)));

        // Candidates for the guard method from the pre-state
        std::vector<int> cands = transitions_for(pre_state, guard_name, gimple_location(guard_call));
        if (cands.empty()) return false;

        auto two = pick_two_arms(cands);
        std::pair<int,int> choice;

        // Try to read (==/!=) and K from the terminator relative to guard result
        tree res = gimple_call_lhs(guard_call);
        CondOp op; long long K; bool res_on_lhs; std::string K_name;

        bool used_map = false;
        if (res && extract_eq_ne_with_const(t, res, op, K, res_on_lhs, K_name)) {
            int mapped_eq = branch_next_for_cond(pre_state, guard_name, K);
            if (mapped_eq >= 0) {
                int mapped_neq = -1;
                for (int s : cands) if (s != mapped_eq) { mapped_neq = s; break; }
                if (mapped_neq < 0) mapped_neq = mapped_eq;

                if (op == OP_EQ) choice = { mapped_eq, mapped_neq };
                else             choice = { mapped_neq, mapped_eq };

                used_map = true;

                fprintf(stderr,
                    "Debug: Built plan via Branch_Rules: ifid=%d guard=%s K=%lld op=%s TRUE→%d FALSE→%d pre=%d\n",
                    ifid, guard_name.c_str(), K, (op == OP_EQ ? "==" : "!="),
                    choice.first, choice.second, pre_state);
                fflush(stderr);
            }
        }

        if (!used_map) {
            // Fallback heuristic
            bool t_true = true_edge_means_call_true(t, guard_call);
            choice = t_true ? std::make_pair(two.first, two.second)
                            : std::make_pair(two.second, two.first);

            fprintf(stderr,
                "Debug: Built plan via fallback: ifid=%d guard=%s TRUE→%d FALSE→%d pre=%d\n",
                ifid, guard_name.c_str(), choice.first, choice.second, pre_state);
            fflush(stderr);
        }

        ctx.arm_choice[key_obj] = choice;
        return true;
    };
    // -----------------------------------------------------------------------

    // Seed once using the stored TRUE/FALSE plan
    if (object_stateper_branch[obj_key] == pre_branch) {
        auto itCtx = IFCTX.find(branch_id);
        if (itCtx != IFCTX.end() && term) {
            auto& ctx = itCtx->second;

            // declare iterator BEFORE using it (fixes your compile error)
            auto itPlan = ctx.arm_choice.find(obj_key);

            // If missing, build it now (fixes nested else-if / branch-id gaps)
            if (itPlan == ctx.arm_choice.end()) {
                (void)build_arm_plan_now(branch_id, obj_key, pre_branch, term);
                itPlan = ctx.arm_choice.find(obj_key);
            }

            if (itPlan == ctx.arm_choice.end()) {
                fprintf(stderr,
                    "Debug: Missing arm plan for object %p in branch %d; staying at pre=%d.\n",
                    (void*)obj_key, branch_id, pre_branch);
                fflush(stderr);
            } else {
                bool is_true_arm = edge_is_true_successor(gimple_bb(term), bb_here);
                int seed = is_true_arm ? itPlan->second.first : itPlan->second.second;
                if (seed >= 0) {
                    object_stateper_branch[obj_key] = seed;
                    fprintf(stderr,
                        "Debug: Entering %s arm (id=%d) for object %p: apply seed=%d "
                        "(plan TRUE→%d, FALSE→%d), pre=%d.\n",
                        is_true_arm ? "TRUE" : "FALSE",
                        branch_id, (void*)obj_key, seed,
                        itPlan->second.first, itPlan->second.second, pre_branch);
                    fflush(stderr);
                }
            }
        }
    }

    int cur = object_stateper_branch[obj_key];

    // Compute transitions from the branch-local state
    auto cand = transitions_for(cur, method_name, location);

    if (cand.empty()) {
        state_manager.method_calls[obj_key].push_back(method_name);
        return;
    }
    if (cand.size() > 1) {
        fprintf(stderr,
            "Debug: Ambiguous transitions for '%s' in branch state %d (", method_name.c_str(), cur);
        for (int s : cand) fprintf(stderr, "%d ", s);
        fprintf(stderr, "); deferring.\n");
        fflush(stderr);
        state_manager.method_calls[obj_key].push_back(method_name);
        return;
    }

    int next = cand.front();
    object_stateper_branch[obj_key] = next;

    state_manager.method_calls[obj_key].push_back(method_name);
}



void finalize_branch_states(tree obj) {
    tree obj_address = get_original_object(obj);

    // Resolve aliases to track the actual object
    tree resolved_obj = track_all_aliases(obj_address);
    if (!resolved_obj) {
        //printf("Debug: Could not resolve object aliases for %p.\n", (void*)obj_address);
        return;
    }

    for (const auto& [objbr, state] : object_state_branch) {
        state_manager.object_states[objbr] = state;
     printf("Debug: Finalized state for object %p to %d.\n", (void*)objbr, state);
    }
    object_state_branch.clear();
state_manager.object_states[resolved_obj] =  object_stateper_branch[resolved_obj]; // Clear branch-specific states after finalization
     int current_state =  object_stateper_branch[resolved_obj];
    // printf("Debug: object %p in state %d.\n", (void*)resolved_obj, current_state);

   
}

bool check_if_else_completion() {
    int if_else_id = current_if_else_id;
    if_else_context_map[if_else_id].second++;

    int total_branches = if_else_context_map[if_else_id].first;
    int completed_branches = if_else_context_map[if_else_id].second;
    bool check = false;
   //printf("Debug: Checking completion for if-else block %d. Total branches = %d, Completed branches = %d.\n",
          // if_else_id, total_branches, completed_branches);

    if (completed_branches == total_branches ) {
        if_else_status[if_else_id] = true;
        finalize_if_else_block(if_else_id);
       
     check =true;
    }
    else {
        // printf("not complete");
       check=false;
    }
    return check;
}


void end_branch_processing(tree obj_key, basic_block arm_bb) {
    if (!arm_bb) return;

    // The terminator that controls THIS arm
    gimple* term = controlling_terminator_of(arm_bb);
    if (!term) return;

    int ifid = get_if_else_id_for_terminator(term);
    if (ifid < 0) return;

    // Mark this arm complete
    end_arm(ifid);

    auto it = IFCTX.find(ifid);
    if (it != IFCTX.end()) {
        auto& ctx = it->second;

        // (Optional) Log finals once all arms visited
        if (ctx.visited >= ctx.expected) {
            for (auto& kv : ctx.finals) {
                tree k = kv.first;
                const std::vector<int>& finals = kv.second;

                if (!finals.empty()) {
                    bool all_same = true;
                    int first = finals.front();
                    for (int s : finals) {
                        if (s != first) { all_same = false; break; }
                    }
                    if (all_same) {
                        fprintf(stderr,
                            "Debug: Branch %d: all %d arms end in SAME state %d for object %p.\n",
                            ifid, (int)finals.size(), first, (void*)k);
                    } else {
                        fprintf(stderr,
                            "Debug: Branch %d: arms end in DIFFERENT states for object %p: ",
                            ifid, (void*)k);
                        for (size_t i = 0; i < finals.size(); ++i)
                            fprintf(stderr, "%s%d", (i ? "," : ""), finals[i]);
                        fprintf(stderr, ".\n");
                    }
                }
            }
        }
    }

    // Finalize the group for this ifid
    try_finalize_if_group(ifid);
}


static bool both_arms_end_same_state(int ifid, tree obj, int* out_state = nullptr) {
    auto it = IFCTX.find(ifid);
    if (it == IFCTX.end()) return false;
    auto key = norm_obj_key(obj);
    auto itv = it->second.finals.find(key);
    if (itv == it->second.finals.end()) return false;

    const auto& v = itv->second;
    if ((int)v.size() < it->second.expected) return false; // not all arms processed yet

    int first = v.front();
    for (int s : v) if (s != first) {
        if (out_state) *out_state = -1;
        return false;
    }
    if (out_state) *out_state = first;
    return true;
}






// hepler Function to get readble C++ names


static std::string base_type_org;


// Function to detect if a tree node represents an object subject to typestate checking or a subtype of
// such an object. This ensures that in cases of inheritance, both supertypes and their subtypes
// are checked against typestate rules.

bool is_object_of_Flagged(tree arg, tree& obj_address) {
    obj_address = nullptr;

    if (!arg) {
       
        return false;
    }

   
    tree resolved_arg = track_all_aliases(get_original_object(arg));
    if (!resolved_arg) {
       
        return false;
    }

    tree type = TREE_TYPE(resolved_arg);
    if (!type) {
       
        return false;
    }

   
    if (TREE_CODE(type) != RECORD_TYPE) {
     
        return false;
    }

    const char* type_name = nullptr;
    if (TYPE_NAME(type) && DECL_NAME(TYPE_NAME(type))) {
        type_name = IDENTIFIER_POINTER(DECL_NAME(TYPE_NAME(type)));
    }

    if (!type_name) {
       
        return false;
    }


    if (TypestateClassConnector_args.empty() || TypestateClassConnector_args[0].empty()) {
       
        return false;
    }

    if (strcmp(type_name, TypestateClassConnector_args[0].c_str()) == 0) {
        obj_address = resolved_arg;
       
        return true;
    }

    // Check base classes for type match
    tree binfo = TYPE_BINFO(type);
    if (!binfo) {
       
        return false;
    }

    int num_bases = BINFO_N_BASE_BINFOS(binfo);
    if (num_bases < 0 || num_bases > 100) {
   
        return false;
    }

    for (int i = 0; i < num_bases; ++i) {
        tree base_binfo = BINFO_BASE_BINFO(binfo, i);
        if (!base_binfo) {
            continue;
        }

        tree base_type = BINFO_TYPE(base_binfo);
        if (!base_type) {

            continue;
        }

        const char* base_type_name = nullptr;
        if (TYPE_NAME(base_type) && DECL_NAME(TYPE_NAME(base_type))) {
            base_type_name = IDENTIFIER_POINTER(DECL_NAME(TYPE_NAME(base_type)));
        }

        if (!base_type_name) {
            continue;
        }
       

        if (strcmp(base_type_name, TypestateClassConnector_args[0].c_str()) == 0) {
            obj_address = resolved_arg;
   
            return true;
        }
    }

    return false;
}




/**
 * This is part of an inter-procedural analysis.
 * It starts by analysing GIMPLE statements from the main function. Whenever there is a function call,
 * the analysis expands to process and analyse statements within the called functions.
 * Whenever there is a call to a state transition function for a flagged object,
 * it is validated against the typestate rules.
 */

#include <bitset>
#include <optional>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <string>


//  PSA guard: true while the path-sensitive driver is running 
bool g_psa_enabled = false;

//  Strict-join policy toggle (set true to enforce identical singleton states) 
static bool g_strict_join = true;





extern tree get_original_object(tree);
extern tree track_all_aliases(tree);



// Discover number of states from  Typestate_Rules
inline int discover_num_states() {
    int mx = -1;
    for (auto &kv : Typestate_Rules) {
        mx = std::max(mx, kv.first);
        for (auto &p : kv.second) mx = std::max(mx, p.second);
    }
    return mx + 1;
}


struct ObjectId {
    tree t;
    friend bool operator==(const ObjectId& a, const ObjectId& b) { return a.t == b.t; }
};
struct ObjectIdHash {
    size_t operator()(const ObjectId& x) const { return reinterpret_cast<size_t>(x.t); }
};

// Normalize an object root once (base + alias resolution)
inline ObjectId norm_object(tree obj) {
    tree base = get_original_object(obj);
    tree ali  = track_all_aliases(base);
    return ObjectId{ ali ? ali : base };
}

// Facts at a program point: object -> set of possible states
struct Facts {
    std::unordered_map<ObjectId, Bitset, ObjectIdHash> m;
    bool operator==(const Facts& r) const { return m == r.m; }
    bool operator!=(const Facts& r) const { return !(*this == r); }
};

// For the tracked object, remember what the *first* predecessor proposes
// at a given successor block; later predecessors must match it exactly.
static std::unordered_map<basic_block, Bitset>     JOIN_EXPECTED_FOR_BB;
static std::unordered_map<basic_block, location_t> JOIN_ERROR_LOC;

// Meet at joins: intersection (state must be possible on ALL incoming paths)
inline Facts meet(const Facts& A, const Facts& B) {
    Facts R;
    std::unordered_set<ObjectId, ObjectIdHash> keys;
    for (auto &kv : A.m) keys.insert(kv.first);
    for (auto &kv : B.m) keys.insert(kv.first);
    for (auto &o : keys) {
        Bitset a{}, b{};
        if (auto ia = A.m.find(o); ia != A.m.end()) a = ia->second;
        if (auto ib = B.m.find(o); ib != B.m.end()) b = ib->second;
        R.m[o] = (a & b); // (could be empty)
    }
    return R;
}

// DFA helpers using your Typestate_Rules
inline Bitset step_states_union(const Bitset& cur, const std::string& method) {
    Bitset next;
    for (int s = 0; s < NUM_STATES; ++s) if (cur.test(s)) {
        auto it = Typestate_Rules.find(s);
        if (it == Typestate_Rules.end()) continue;
        for (auto &e : it->second) if (e.first == method) {
            int dst = e.second;
            if (0 <= dst && dst < NUM_STATES) next.set(dst);
        }
    }
    return next;
}
inline bool call_valid_from_all(const Bitset& cur, const std::string& method) {
    for (int s = 0; s < NUM_STATES; ++s) if (cur.test(s)) {
        bool ok = false;
        auto it = Typestate_Rules.find(s);
        if (it != Typestate_Rules.end()) {
            for (auto &e : it->second) if (e.first == method) { ok = true; break; }
        }
        if (!ok) return false;
    }
    return true;
}
inline std::vector<int> all_dsts_for(int src, const std::string& method) {
    std::vector<int> v;
    auto it = Typestate_Rules.find(src);
    if (it == Typestate_Rules.end()) return v;
    for (auto &e : it->second) if (e.first == method) v.push_back(e.second);
    return v;
}
inline bool method_is_guarded(const Bitset& cur, const std::string& method) {
    for (int s = 0; s < NUM_STATES; ++s) if (cur.test(s)) {
        if (all_dsts_for(s, method).size() > 1) return true;
    }
    return false;
}
inline Bitset get_obj_states_from_facts(const Facts& F, tree obj_address) {
    auto it = F.m.find(norm_object(obj_address));
    return (it == F.m.end()) ? Bitset{} : it->second;
}

static inline const char* gcode_name(gimple* s) {
    if (!s) return "NULL";
    return gimple_code_name[gimple_code(s)];
}

static inline int get_obj_state(tree obj) {
    auto it = state_manager.object_states.find(obj);
    return (it == state_manager.object_states.end()) ? -777 : it->second;
}

static void dump_guard_ctx(const char* tag, int ifid, BranchCtx& ctx, tree key, tree obj, gimple* pred_term, basic_block bb) {
    fprintf(stderr,
        "[DBG][%s] ifid=%d bb=%d pred=%s obj=%p key=%p state=%d pre=%d guard='%s' stack_sz=%zu "
        "arm_sets=%d active_cnt=%lu\n",
        tag,
        ifid,
        bb ? bb->index : -1,
        gcode_name(pred_term),
        (void*)obj,
        (void*)key,
        get_obj_state(obj),
        ctx.guard_pre_state,
        ctx.guard_method.c_str(),
        (size_t)ctx.state_stack[key].size(),
        (int)ctx.arm_sets.count(key),
        (unsigned long)(ctx.active_set.count(key) ? ctx.active_set[key].count() : 0)
    );
}

static void dump_cond(gimple* pred_term) {
    if (!pred_term || gimple_code(pred_term) != GIMPLE_COND) return;
    const gcond* gc = as_a<const gcond*>(pred_term);
    fprintf(stderr, "      cond_code=%d lhs=%p rhs=%p\n",
            (int)gimple_cond_code(gc),
            (void*)gimple_cond_lhs(gc),
            (void*)gimple_cond_rhs(gc));
}


static std::unordered_map<basic_block, Facts> IN_facts;     // facts at BB entry
static std::unordered_map<basic_block, Facts> CUR_facts;    // running facts while scanning BB
static std::unordered_map<basic_block, Facts> EDGE_TRUE_OUT;  // edge-specific OUT (true)
static std::unordered_map<basic_block, Facts> EDGE_FALSE_OUT; // edge-specific OUT (false)

inline Facts& ensure_bb_facts(gimple* stmt) {
    basic_block bb = gimple_bb(stmt);
    auto it = CUR_facts.find(bb);
    if (it == CUR_facts.end()) CUR_facts[bb] = IN_facts[bb]; // seed with IN at BB entry
    return CUR_facts[bb];
}
inline Bitset get_states(Facts& F, tree obj) { return F.m[norm_object(obj)]; }
inline void set_states(Facts& F, tree obj, const Bitset& bs) { F.m[norm_object(obj)] = bs; }

// Does a call feed this GIMPLE_COND?
static bool call_feeds_terminator_cond(gimple* call_stmt, gimple* term) {
    if (!call_stmt || !term) return false;
    if (gimple_code(term) != GIMPLE_COND) return false;

    tree res = gimple_call_lhs(call_stmt);
    if (!res || TREE_CODE(res) != SSA_NAME) return false;

    tree lhs = gimple_cond_lhs(term);
    tree rhs = gimple_cond_rhs(term);
    if (lhs == res || rhs == res) return true;
    return tree_derives_from(lhs, res) || tree_derives_from(rhs, res);
}


void process_statement_in_function(gimple* stmt, tree obj_address) {
    if (!stmt) return;

    if (is_gimple_call(stmt)) {
        tree called_fn = gimple_call_fndecl(stmt);
        if (called_fn) {
            const char* callee_name_raw = IDENTIFIER_POINTER(DECL_NAME(called_fn));
            std::string callee_name = callee_name_raw ? demangle(callee_name_raw) : "unknown_function";

            tree this_arg = NULL_TREE;
            if (DECL_CONTEXT(called_fn) && TREE_CODE(DECL_CONTEXT(called_fn)) == RECORD_TYPE &&
                !DECL_STATIC_FUNCTION_P(called_fn)) {
                this_arg = gimple_call_arg(stmt, 0);
            }

            if (this_arg) {
                this_arg = get_original_object(this_arg);
                tree resolved_this_arg = track_all_aliases(this_arg);

                auto handle_on_obj = [&](tree which_obj) {
                    // path-sensitive check/step (replaces Typestate_Checking) 
                    Facts& facts = ensure_bb_facts(stmt);
                    Bitset cur = get_states(facts, which_obj);

                    // 1) validity check at the call site
                    if (!call_valid_from_all(cur, callee_name)) {
                       error_at(gimple_location(stmt),
                             "Typestate: call '%s' not valid from all possible states here.",
                                callee_name.c_str());
                    }

                    // 2) split if guarded call feeds this BB's terminator condition
                    gimple_stmt_iterator tail = gsi_last_bb(gimple_bb(stmt));
                    gimple* term = gsi_end_p(tail) ? nullptr : gsi_stmt(tail);

                    if (call_feeds_terminator_cond(stmt, term) && method_is_guarded(cur, callee_name)) {
                        Bitset union_next = step_states_union(cur, callee_name);

                        // choose two destinations deterministically
                        int d1 = -1, d2 = -1;
                        for (int s = 0; s < NUM_STATES; ++s) if (cur.test(s)) {
                            auto v = all_dsts_for(s, callee_name);
                            for (int d : v) {
                                if (d1 < 0) d1 = d;
                                else if (d2 < 0 && d != d1) { d2 = d; break; }
                            }
                            if (d2 >= 0) break;
                        }
                        if (d1 < 0) d1 = 0;
                        if (d2 < 0) d2 = d1;

                        Bitset next_true{}, next_false{};
                        for (int d = 0; d < NUM_STATES; ++d) if (union_next.test(d)) {
                            if (d == d1)  next_true.set(d);
                            if (d == d2)  next_false.set(d);
                            if (d != d1 && d != d2) { next_true.set(d); next_false.set(d); } // conservative
                        }

                        basic_block bb = gimple_bb(stmt);
                        EDGE_TRUE_OUT[bb]  = facts; set_states(EDGE_TRUE_OUT[bb],  which_obj, next_true);
                        EDGE_FALSE_OUT[bb] = facts; set_states(EDGE_FALSE_OUT[bb], which_obj, next_false);
                    
                    } else {
                        Bitset next = step_states_union(cur, callee_name);
                        set_states(facts, which_obj, next);
                    }
                };

                if (resolved_this_arg == obj_address) {
                    handle_on_obj(obj_address);
                } else {
                    tree final_alias = track_all_aliases(resolved_this_arg);
                    if (final_alias == obj_address) {
                        handle_on_obj(obj_address);
                    } else {
                        // not the tracked object → ignore
                    }
                }

                if (callee_name == "operator->") {
                    tree lhs = gimple_call_lhs(stmt);
                    if (lhs) {
                      
                        state_manager.register_alias(lhs, obj_address);
                    }
                }
            }
        }
    }
    else if (is_gimple_assign(stmt)) {
       
        tree lhs = gimple_assign_lhs(stmt);
        tree rhs = gimple_assign_rhs1(stmt);

        if (TREE_CODE(lhs) == COMPONENT_REF) {
            tree base = TREE_OPERAND(lhs, 0);
            tree member = TREE_OPERAND(lhs, 1);

            base = get_original_object(base);
            tree resolved_base = track_all_aliases(base);

            tree this_param = DECL_ARGUMENTS(current_function_decl);
            if (resolved_base == this_param) {
                tree resolved_rhs = track_all_aliases(get_original_object(rhs));
                if (resolved_rhs == obj_address) {
                    state_manager.register_alias(lhs, obj_address);
                }
            }
        }
    }
}

// Seed facts for entry (set your real initial state index here)
static Facts initial_facts_for_object(tree obj_address) {
    Facts F;
    Bitset start{}; start.set(0); // assume state 0 = Idle (change if needed)
    F.m[norm_object(obj_address)] = start;
    return F;
}




function* g_psa_fn            = nullptr;   // which function PSA is analyzing
tree      g_psa_tracked_obj   = nullptr;   // root object PSA is analyzing



// Normalize to a stable root (matches what your maps use)
inline tree root_of(tree t) {
    return track_all_aliases(get_original_object(t));
}

// RAII guard to set the PSA context for this run
struct PsaGuard {
    function* saved_fn;
    tree      saved_obj;
    bool      saved_enabled;
    PsaGuard(function* fn, tree obj) {
        saved_fn      = g_psa_fn;
        saved_obj     = g_psa_tracked_obj;
        saved_enabled = g_psa_enabled;

        g_psa_fn          = fn;
        g_psa_tracked_obj = root_of(obj);
        g_psa_enabled     = true;
    }
    ~PsaGuard() {
        g_psa_fn          = nullptr;
        g_psa_tracked_obj = nullptr;
        g_psa_enabled     = false;
    }
};


inline void ts_check_guarded(tree obj, const std::string& name, location_t loc) {
  if (!g_psa_enabled) state_manager.Typestate_Checking(obj, name, loc);
}

void analyze_function_typestate(function* fn, tree obj_address) {
     PsaGuard _guard_(fn, obj_address);
    if (NUM_STATES < 0) NUM_STATES = discover_num_states();

   
    IN_facts.clear();
    CUR_facts.clear();
    EDGE_TRUE_OUT.clear();
    EDGE_FALSE_OUT.clear();
    JOIN_EXPECTED_FOR_BB.clear();
    JOIN_ERROR_LOC.clear();

    // Init empty IN facts for all blocks (includes fake ENTRY/EXIT)
    for (basic_block b = ENTRY_BLOCK_PTR_FOR_FN(fn); b; b = b->next_bb)
        IN_facts[b] = Facts{};

    // Seed the FIRST REAL block (ENTRY is fake in GCC 13)
    basic_block entry_fb = ENTRY_BLOCK_PTR_FOR_FN(fn);
    edge e0 = nullptr; edge_iterator ei0;
    FOR_EACH_EDGE (e0, ei0, entry_fb->succs) { break; }
    if (!e0) return;                   // malformed CFG
    basic_block start_bb = e0->dest;

    // Initial state for the tracked object (adjust index if Idle != 0)
    {
        Facts F;
        Bitset start{}; start.set(0);  // state 0 = Idle (or your real start)
        F.m[norm_object(obj_address)] = start;
        IN_facts[start_bb] = F;
    }

    std::queue<basic_block> wl;
    wl.push(start_bb);

    while (!wl.empty()) {
        basic_block bb = wl.front(); wl.pop();

        // Start from current IN facts for this bb
        CUR_facts[bb] = IN_facts[bb];

        // Clear per-bb edge outs
        EDGE_TRUE_OUT.erase(bb);
        EDGE_FALSE_OUT.erase(bb);

        //  Walk statements in BB (updates CUR_facts[bb]) 
        for (gimple_stmt_iterator gsi = gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi)) {
            gimple* s = gsi_stmt(gsi);
            process_statement_in_function(s, obj_address);
        }

        // Default OUT for edges without split
        Facts out = CUR_facts[bb];

        // Clear expectations for these successors on this visit to avoid stale compares
        {
            edge e1; edge_iterator ei1;
            FOR_EACH_EDGE(e1, ei1, bb->succs) {
                JOIN_EXPECTED_FOR_BB.erase(e1->dest);
                JOIN_ERROR_LOC.erase(e1->dest);
            }
        }

        //  Propagate to successors with MEET; enforce strict join if requested 
        edge e; edge_iterator ei;
        FOR_EACH_EDGE (e, ei, bb->succs) {
            Facts cand = out;
            if ((e->flags & EDGE_TRUE_VALUE)  && EDGE_TRUE_OUT.count(bb))  cand = EDGE_TRUE_OUT[bb];
            if ((e->flags & EDGE_FALSE_VALUE) && EDGE_FALSE_OUT.count(bb)) cand = EDGE_FALSE_OUT[bb];

            if (g_strict_join) {
                // Candidate state set for the tracked object on this incoming edge
                Bitset cand_set = get_obj_states_from_facts(cand, obj_address);

                // Record a reasonable location for join diagnostics (first stmt of successor)
                if (!JOIN_ERROR_LOC.count(e->dest)) {
                    gimple_stmt_iterator it = gsi_start_bb(e->dest);
                    JOIN_ERROR_LOC[e->dest] = gsi_end_p(it) ? UNKNOWN_LOCATION : gimple_location(gsi_stmt(it));
                }

                auto itExp = JOIN_EXPECTED_FOR_BB.find(e->dest);
                if (itExp == JOIN_EXPECTED_FOR_BB.end()) {
                    // First incoming defines the expected set at this join
                    JOIN_EXPECTED_FOR_BB[e->dest] = cand_set;
                } else {
                    Bitset expected = itExp->second;

                    // 1) Both branches must propose EXACTLY the same set
                    if (expected != cand_set) {
                        error_at(JOIN_ERROR_LOC[e->dest],
                                 "Typestate: branches merge with different states for the object "
                                 "(%zu vs %zu possibilities).",
                                 (size_t)expected.count(), (size_t)cand_set.count());
                    }
                    // 2) That set must be a SINGLE concrete state
                    if (cand_set.count() != 1) {
                        error_at(JOIN_ERROR_LOC[e->dest],
                                 "Typestate: state is indeterminate at join "
                                 "(expected a single state, got %zu).",
                                 (size_t)cand_set.count());
                    }
                }
            }

            // Normal meet-based propagation (sound even with strict join enabled)
            Facts joined = IN_facts.count(e->dest) ? meet(IN_facts[e->dest], cand) : cand;
            if (joined != IN_facts[e->dest]) {
                IN_facts[e->dest] = joined;
                wl.push(e->dest); // iterate to fixed point (handles loops)
            }
        }
    }
}



void process_function_body(tree function_decl, tree obj_address) {
    if (!function_decl) return;

    struct function* fn = DECL_STRUCT_FUNCTION(function_decl);
    if (!fn || !fn->cfg) return;

    static std::unordered_set<function*> processed_functions;
    if (processed_functions.find(fn) != processed_functions.end()) return;
    processed_functions.insert(fn);

    push_cfun(fn);
    call_stack.emplace_back();
g_psa_enabled = true;
analyze_function_typestate(fn, obj_address);  // your PSA worklist driver
g_psa_enabled = false;
    // >>> This single call runs the path-sensitive analysis for the one object you passed.
 //   analyze_function_typestate(fn, obj_address);

    pop_cfun();
    call_stack.pop_back();
}



void Analyse_gimple_statement(gimple* stmt,gimple_stmt_iterator* gsi);




static void analyze_switch_cases(basic_block switch_bb, gimple *switch_stmt) {
    if (!switch_stmt || gimple_code(switch_stmt) != GIMPLE_SWITCH)
        return;

    // Cast to gswitch*
    const gswitch *gs = as_a<const gswitch *>(switch_stmt);

    printf("\n[INFO] Found a switch statement in BB %d\n", switch_bb->index);

    // Get the number of cases (including default)
    int num_labels = gimple_switch_num_labels(gs);
    printf("[INFO] Number of cases (including default if present): %d\n", num_labels);

    edge e;
    edge_iterator ei;

    FOR_EACH_EDGE(e, ei, switch_bb->succs) {
        basic_block case_bb = e->dest;

        // Iterate over the labels in the switch statement
        for (int i = 0; i < num_labels; i++) {
            tree label_expr = gimple_switch_label(gs, i);

            // If the edge destination matches, print information
            if (e->dest == case_bb) {
                if (TREE_CODE(label_expr) == CASE_LABEL_EXPR) {
                    tree case_low = CASE_LOW(label_expr);
                    tree case_high = CASE_HIGH(label_expr);

                    if (case_high && case_high != case_low) {
                        // Case range (e.g., case 3 ... 5)
                        printf("[INFO] Case range %ld ... %ld -> BB %d\n",
                               TREE_INT_CST_LOW(case_low),
                               TREE_INT_CST_LOW(case_high),
                               case_bb->index);
                    } else {
                        // Single case value (e.g., case 3)
                        printf("[INFO] Case %ld -> BB %d\n",
                               TREE_INT_CST_LOW(case_low),
                               case_bb->index);
                    }
                } else if (TREE_CODE(label_expr) == LABEL_EXPR) {
                    // Default case
                    printf("[INFO] Default case -> BB %d\n", case_bb->index);
                }
            }
        }
    }
}

// Main function to iterate through basic blocks and detect switches
void Analyse_gimple_in_function(std::unordered_map<tree, tree> param_arg_map = {}) {

    //  detect direct recursion for this function ----------
    function* fn = cfun;
    if (fn && fn->decl && !g_recursive_fn_decls.count(fn->decl)) {
        bool is_rec = false;
        tree self = fn->decl;

        basic_block bb;
        FOR_EACH_BB_FN(bb, fn) {
            for (gimple_stmt_iterator gsi = gsi_start_bb(bb);
                 !gsi_end_p(gsi);
                 gsi_next(&gsi))
            {
                gimple* s = gsi_stmt(gsi);
                if (!is_gimple_call(s)) continue;

                tree callee = gimple_call_fndecl(s);
                if (!callee) continue;

                if (TREE_CODE(callee) == FUNCTION_DECL && callee == self) {
                    is_rec = true;   // found self-call: e.g. AdjustActuators(n-1)
                    break;
                }
            }
            if (is_rec) break;
        }

        if (is_rec) {
            g_recursive_fn_decls.insert(self);
            printf("[INFO] Detected direct recursion in function: %s\n",
                   IDENTIFIER_POINTER(DECL_NAME(self)));
        }
    }
    // ---------- END NEW ----------

    call_stack.emplace_back();
    CallContext& context = call_stack.back();

    tree function_decl = current_function_decl;
    if (function_decl) {
        tree param;
        for (param = DECL_ARGUMENTS(function_decl); param; param = TREE_CHAIN(param)) {
            if (param_arg_map.find(param) != param_arg_map.end()) {
                tree arg = param_arg_map[param];
                arg = get_original_object(arg);
                arg = track_all_aliases(arg);  

                context.alias_map[param] = arg;
                global_alias_map[param] = arg;  
            }
        }
    }

    // Analyse the function body (preserve existing logic)
    basic_block bb;
    FOR_EACH_BB_FN(bb, cfun) {
        gimple_stmt_iterator gsi;
        for (gsi = gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi)) {
            gimple* stmt = gsi_stmt(gsi);
            Analyse_gimple_statement(stmt, &gsi);
        }
    }

    // Pop the call context
    call_stack.pop_back();
}



/*───────────────────────────────────────────────────────────────────*\
 |  Examine one call statement                                      
 |  – typestate check on the receiver                                
 |  – propagate operator-> aliases                                    
 |  – build parameter→argument map                                    
 |  – queue callee when <fn,obj,state> not seen before                
\*───────────────────────────────────────────────────────────────────*/


extern bool g_psa_enabled;
extern function* g_psa_fn;
extern tree g_psa_tracked_obj;



std::unordered_map<int, int> branchExpectedCount; // Expected number of branches for each if-else group.
std::unordered_map<int, int> branchVisitedCount;
bool is_if_else_structure_complete(int if_else_id) {
    // If we haven't set an expected count yet, we can't say it's complete.
    if (branchExpectedCount.find(if_else_id) == branchExpectedCount.end() ||
        branchVisitedCount.find(if_else_id) == branchVisitedCount.end())
    {
        return false;
    }
   
    // The structure is complete when the number of visited branches equals the expected count.
    return branchVisitedCount[if_else_id] == branchExpectedCount[if_else_id];
}


std::vector<int> get_candidate_next_states(int current_state, const std::string &method_name) {
    std::vector<int> candidates;
    if (Typestate_Rules.find(current_state) != Typestate_Rules.end()) {
        for (const auto &trans : Typestate_Rules[current_state]) {
            if (trans.first == method_name) {
                candidates.push_back(trans.second);
            }
        }
    }
    return candidates;
}



//std::unordered_map<int, std::unordered_map<tree, int>> branch_state_map;
std::map<tree, std::set<int>> nondet_object_states;
#include "wide-int.h"
#include "tree-cfg.h"


static tree strip_typedefs_quals(tree t) {
    if (!t) return t;
#ifdef TYPE_CANONICAL
    if (TYPE_CANONICAL(t)) return TYPE_CANONICAL(t);
#endif
    return TYPE_MAIN_VARIANT(t) ? TYPE_MAIN_VARIANT(t) : t;
}

static bool method_returns_boolean_or_enum(tree callee_decl) {
    if (!callee_decl || TREE_CODE(callee_decl) != FUNCTION_DECL) return false;
    tree fntype = TREE_TYPE(callee_decl);
    if (!fntype || TREE_CODE(fntype) != FUNCTION_TYPE) return false;
    tree ret = TREE_TYPE(fntype);
    if (!ret) return false;
    ret = strip_typedefs_quals(ret);
    if (TREE_CODE(ret) == BOOLEAN_TYPE)   return true;
    if (TREE_CODE(ret) == ENUMERAL_TYPE)  return true;
    if (TREE_CODE(ret) == INTEGER_TYPE)   return TYPE_PRECISION(ret) == 1; // bool-like typedef
    return false;
}

//  const handling + condition analysis (prints enum name/value) 
static bool const_to_wide_int(tree t, wide_int* out) {
    if (!t || !out) return false;

    if (TREE_CODE(t) == INTEGER_CST) {
        *out = TO_WIDE(t);
        return true;
    }

    if (TREE_CODE(t) == CONST_DECL) {
        tree v = DECL_INITIAL(t);
        if (v) {
            STRIP_NOPS(v); // handle enum/const wrapped in NOP/CONVERT
            if (TREE_CODE(v) == INTEGER_CST) {
                *out = TO_WIDE(v);
                return true;
            }
        }
    }

    return false;
}


#include "tree.h"

// Use once in a common header/source:
#ifndef TO_WIDE
#  if defined(GCCPLUGIN_VERSION) && (GCCPLUGIN_VERSION < 8000)
// GCC 7.x needs precision
#    define TO_WIDE(T_) wi::to_wide((T_), TYPE_PRECISION (TREE_TYPE (T_)))
#  else
// GCC 8+ has the 1-arg overload
#    define TO_WIDE(T_) wi::to_wide((T_))
#  endif
#endif

static bool const_name_and_value(tree t, std::string& name_out, long long* val_out) {
    name_out.clear();
    if (!t) return false;

    if (TREE_CODE(t) == CONST_DECL) {
        if (DECL_NAME(t)) name_out = IDENTIFIER_POINTER(DECL_NAME(t));
        tree v = DECL_INITIAL(t);
        if (v) {
            STRIP_NOPS(v); // handle enum constants or folded expressions wrapped in NOP/CONVERT
            if (TREE_CODE(v) == INTEGER_CST) {
                if (val_out) *val_out = TO_WIDE(v).to_shwi();
                return true;
            }
        }
        return false; // CONST_DECL without INTEGER_CST initializer
    }

    if (TREE_CODE(t) == INTEGER_CST) {
        long long v = TO_WIDE(t).to_shwi();
        if (val_out) *val_out = v;
        name_out = std::to_string(v);
        return true;
    }

    return false;
}


// Does COND compare call result 'res' to a constant K?
static bool cond_compares_res_to_const(gimple* term, tree res, bool* out_is_eq, wide_int* out_val) {
    if (!term || gimple_code(term) != GIMPLE_COND || !res) return false;
    enum tree_code code = gimple_cond_code(term);
    if (code != EQ_EXPR && code != NE_EXPR) return false;

    tree L = gimple_cond_lhs(term);
    tree R = gimple_cond_rhs(term);
    auto is_res = [&](tree t){ return t==res || (TREE_CODE(t)==SSA_NAME && ssa_derives_from(t,res)); };

    if (is_res(L) && const_to_wide_int(R, out_val)) { *out_is_eq = (code==EQ_EXPR); return true; }
    if (is_res(R) && const_to_wide_int(L, out_val)) { *out_is_eq = (code==EQ_EXPR); return true; }
    return false;
}

// Is COND effectively "if (res)" or "if (!res)"?
static bool cond_is_simple_truth_test(gimple* term, tree res, bool* true_when_nonzero) {
    if (!term || gimple_code(term) != GIMPLE_COND || !res) return false;
    enum tree_code code = gimple_cond_code(term);
    tree L = gimple_cond_lhs(term);
    tree R = gimple_cond_rhs(term);
    auto is_res = [&](tree t){ return t==res || (TREE_CODE(t)==SSA_NAME && ssa_derives_from(t,res)); };

    if (code == NE_EXPR || code == EQ_EXPR) {
        if (is_res(L) && is_falsey_const(R)) { *true_when_nonzero = (code==NE_EXPR); return true; }
        if (is_res(R) && is_falsey_const(L)) { *true_when_nonzero = (code==NE_EXPR); return true; }
    }
    return false;
}



struct BranchSemantics {
    bool compares_to_const = false;
    bool is_eq = true;
    wide_int const_val;

    bool simple_truth_test = false;
    bool true_when_nonzero = true;
};

//  prints what it understands about the branch w.r.t the call result
static BranchSemantics analyze_branch_semantics(gimple* term, gimple* call_stmt) {
    BranchSemantics bs{};
    if (!term || gimple_code(term) != GIMPLE_COND || !call_stmt) return bs;

    tree res = gimple_call_lhs(call_stmt);
    if (!res || TREE_CODE(res) != SSA_NAME) return bs;

    if (cond_compares_res_to_const(term, res, &bs.is_eq, &bs.const_val)) {
        bs.compares_to_const = true;

        // Print the exact constant name/value seen in the condition
        tree L = gimple_cond_lhs(term);
        tree R = gimple_cond_rhs(term);
        std::string cname; long long cval = 0;
        if (TREE_CODE(L) == CONST_DECL || TREE_CODE(L) == INTEGER_CST) const_name_and_value(L, cname, &cval);
        else if (TREE_CODE(R) == CONST_DECL || TREE_CODE(R) == INTEGER_CST) const_name_and_value(R, cname, &cval);
    //    fprintf(stderr, "Debug: condition compares call result to const %s (=%lld); TRUE means %s.\n",
          //      cname.c_str(), cval, bs.is_eq ? "== const" : "!= const");
        return bs;
    }

    if (cond_is_simple_truth_test(term, res, &bs.true_when_nonzero)) {
 //       fprintf(stderr, "Debug: condition is simple truth test; TRUE means (res %s 0).\n",
       //         bs.true_when_nonzero ? "!=" : "==");
        bs.simple_truth_test = true;
        return bs;
    }

  //  fprintf(stderr, "Debug: condition analysis inconclusive; using heuristic.\n");
    return bs;
}





static const char* arm_label_then_else(gimple* term, basic_block succ) {
    if (!term) return "?";
    basic_block pred = gimple_bb(term);
    basic_block btrue=nullptr, bfalse=nullptr;
    edge e; edge_iterator ei;
    FOR_EACH_EDGE(e, ei, pred->succs) {
        if (e->flags & EDGE_TRUE_VALUE)  btrue  = e->dest;
        if (e->flags & EDGE_FALSE_VALUE) bfalse = e->dest;
    }
    if (succ == btrue)  return "THEN";
    if (succ == bfalse) return "ELSE";
    return "?";
}


static bool is_end_of_arm_bb(basic_block bb) {
    if (!bb) return false;

    // Must be inside an if/switch arm
    gimple* term = controlling_terminator_of(bb);
    if (!term) return false;
    int ifid = get_if_else_id_for_terminator(term);

    // Heuristic: if this BB has a single successor whose own if-id differs,
    // or the successor has >=2 predecessors (join),  consider this the end.
    edge e; edge_iterator ei;
    int succs = 0;
    basic_block succ_only = nullptr;
    FOR_EACH_EDGE(e, ei, bb->succs) {
        ++succs;
        if (succs == 1) succ_only = e->dest;
    }
    if (succs != 1) return false; // still branching, not a straight fallthrough

    // Successor’s if-id (if any)
    int succ_ifid = get_if_else_id_for_bb(succ_only);

    // If successor has multiple predecessors, it’s likely the join
    int pred_count = 0;
    FOR_EACH_EDGE(e, ei, succ_only->preds) { ++pred_count; }

    // End-of-arm if we’re leaving this if-group OR we’re falling into a join
    if (succ_ifid != ifid) return true;
    if (pred_count >= 2)   return true;

    return false;
}


// Map: function declaration -> recursion bound (n)
static std::unordered_map<tree, int> g_recursion_bounds;
// Any FUNCTION_DECL in here is known to be directly recursive.
static std::unordered_set<tree> g_recursive_fn_names;

static int arm_for_bb_under_cond(gimple* cond, basic_block bb) {
    if (!cond || gimple_code(cond) != GIMPLE_COND) return -1;
    basic_block cbb = gimple_bb(cond);
    if (!cbb || !bb) return -1;

    basic_block tbb = nullptr;
    basic_block fbb = nullptr;

    edge e;
    edge_iterator ei;
    FOR_EACH_EDGE(e, ei, cbb->succs) {
        if (e->flags & EDGE_TRUE_VALUE)  tbb = e->dest;
        if (e->flags & EDGE_FALSE_VALUE) fbb = e->dest;
    }

    if (bb == tbb) return 1;
    if (bb == fbb) return 0;

    calculate_dominance_info(CDI_DOMINATORS);

    // bb is "inside" the arm if it is dominated by the arm entry
    if (tbb && dominated_by_p(CDI_DOMINATORS, bb, tbb)) return 1;
    if (fbb && dominated_by_p(CDI_DOMINATORS, bb, fbb)) return 0;

    return -1;
}

static Bitset bitset_from_vec(const std::vector<int>& v) {
    Bitset bs;
    for (int s : v) if (0 <= s && s < (int)bs.size()) bs.set(s);
    return bs;
}

static int pick_one_from_bitset(const Bitset& b) {
    for (int i = 0; i < (int)b.size(); ++i)
        if (b.test(i)) return i;
    return -1;
}

static void init_remaining_for_first_if(BranchCtx& ctx,
                                        const std::vector<int>& candidates,
                                        int mapped_true)
{
    if (ctx.remaining_init) return;

    ctx.remaining = Bitset(); // assumes default ctor makes correct size
    for (int s : candidates) {
        if (0 <= s && s < (int)ctx.remaining.size())
            ctx.remaining.set(s);
    }

    // remove the TRUE state => remaining is ELSE side
    if (0 <= mapped_true && mapped_true < (int)ctx.remaining.size())
        ctx.remaining.reset(mapped_true);

    ctx.remaining_init = true;

    fprintf(stderr, "Debug: init remaining ELSE-set: remove_true=%d => rem_count=%zu\n",
            mapped_true, ctx.remaining.count());
}

// For else-if: FALSE state should come from the remaining set (excluding mapped_true)
static int compute_false_from_remaining(BranchCtx& ctx, int mapped_true) {
    if (!ctx.remaining_init) return -1;

    Bitset else_set = ctx.remaining;
    if (0 <= mapped_true && mapped_true < (int)else_set.size())
        else_set.reset(mapped_true);

    int f = pick_one_from_bitset(else_set);

    fprintf(stderr, "Debug: else-if narrowing: mapped_true=%d else_count=%zu picked_false=%d\n",
            mapped_true, else_set.count(), f);

    return f;
}



static void consume_from_remaining(BranchCtx& ctx, int mapped_true) {
    if (!ctx.remaining_init) return;
    if (0 <= mapped_true && mapped_true < (int)ctx.remaining.size())
        ctx.remaining.reset(mapped_true);
}


static bool is_direct_recursive_call(tree callee_decl) {
    if (!callee_decl || !cfun) return false;
    if (TREE_CODE(callee_decl) != FUNCTION_DECL) return false;
    return (cfun->decl == callee_decl);
}


static void register_recursion_bound(tree fn_decl, int bound) {
    if (!fn_decl) return;
    g_recursion_bounds[fn_decl] = bound;
}

static bool get_recursion_bound(tree fn_decl, int& out_bound) {
    auto it = g_recursion_bounds.find(fn_decl);
    if (it == g_recursion_bounds.end()) return false;
    out_bound = it->second;
    return true;
}

static bool is_function_marked_recursive(tree fn_decl) {
    int dummy;
    return get_recursion_bound(fn_decl, dummy);
}


static bool is_constant_int_bound(tree t, long &out_val) {
    if (!t) return false;

    if (TREE_CODE(t) == INTEGER_CST) {
        out_val = (long)TREE_INT_CST_LOW(t);
        return true;
    }

    if (TREE_CODE(t) == SSA_NAME) {
        gimple *def = SSA_NAME_DEF_STMT(t);
        if (!def) return false;
        if (is_gimple_assign(def) && gimple_assign_single_p(def)) {
            tree rhs = gimple_assign_rhs1(def);
            if (rhs && TREE_CODE(rhs) == INTEGER_CST) {
                out_val = (long)TREE_INT_CST_LOW(rhs);
                return true;
            }
        }
    }

    return false;
}


static bool function_has_direct_recursion(function* fn) {
    if (!fn || !fn->decl) return false;

    tree self = fn->decl;

    basic_block bb;
    FOR_EACH_BB_FN(bb, fn) {
        for (gimple_stmt_iterator gsi = gsi_start_bb(bb);
             !gsi_end_p(gsi);
             gsi_next(&gsi))
        {
            gimple* stmt = gsi_stmt(gsi);
            if (!is_gimple_call(stmt)) continue;

            tree callee = gimple_call_fndecl(stmt);
            if (!callee) continue;

            if (TREE_CODE(callee) == FUNCTION_DECL && callee == self) {
                // Found a direct self-call
                return true;
            }
        }
    }
    return false;
}

static bool is_recursive_transition_method(const std::string& name) {
    for (const auto& rt : g_recursive_transitions) {
        if (rt.method == name) {
            return true;
        }
    }
    return false;
}


static bool build_arm_plan_now(int branch_id,
                               tree obj_key,
                               int pre_state,
                               gimple* term)
{
    if (!term || !obj_key) return false;

    auto& ctx = ensure_ifctx(branch_id, term);
    if (ctx.arm_choice.count(obj_key)) return true;

    // Guard call that produced the value tested by this IF
    gimple* guard_call =
        (gimple_code(term) == GIMPLE_COND) ? find_guard_call_for_cond(term) : nullptr;
    if (!guard_call || !is_gimple_call(guard_call)) return false;

    tree gd = gimple_call_fndecl(guard_call);
    if (!gd || TREE_CODE(gd) != FUNCTION_DECL || !DECL_NAME(gd)) return false;

    // Use a stable name to match your rules
    std::string guard_name = demangle(IDENTIFIER_POINTER(DECL_NAME(gd)));
    std::string guard_uq   = unqualified_name(guard_name);

    // Candidates from the pre-state for THIS guard method
    // (use UNKNOWN_LOCATION here; we are just building a plan)
    std::vector<int> cands = transitions_for(pre_state, guard_uq, UNKNOWN_LOCATION);
    if (cands.empty()) {
        // Try with the full name if unqualified didn't match
        cands = transitions_for(pre_state, guard_name, UNKNOWN_LOCATION);
    }
    if (cands.empty()) return false;

    // Read (==/!=) and K from the IF relative to guard result
    tree res = gimple_call_lhs(guard_call);
    if (!res) return false;

    CondOp op; long long K; bool res_on_lhs; std::string K_name;

    auto two = pick_two_arms(cands);
    std::pair<int,int> choice;
    bool used_map = false;

    if (extract_eq_ne_with_const(term, res, op, K, res_on_lhs, K_name)) {
        // Map K to exact next state using Branch_Rules
        int mapped_eq = branch_next_for_cond(pre_state, guard_uq, K);
        if (mapped_eq < 0) mapped_eq = branch_next_for_cond(pre_state, guard_name, K);

        if (mapped_eq >= 0) {
            int mapped_neq = -1;
            for (int s : cands) {
                if (s != mapped_eq) { mapped_neq = s; break; }
            }
            if (mapped_neq < 0) mapped_neq = mapped_eq;

            if (op == OP_EQ) choice = { mapped_eq, mapped_neq };
            else             choice = { mapped_neq, mapped_eq };

            used_map = true;

            fprintf(stderr,
                "Debug: build_arm_plan_now(branch=%d): guard=%s (uq=%s) K=%lld op=%s "
                "cands=%zu TRUE→%d FALSE→%d pre=%d\n",
                branch_id,
                guard_name.c_str(), guard_uq.c_str(),
                K, (op == OP_EQ ? "==" : "!="),
                cands.size(),
                choice.first, choice.second, pre_state);
            fflush(stderr);
        }
    }

    if (!used_map) {
        // fallback: keep your old two-arm heuristic
        bool t_true = true_edge_means_call_true(term, guard_call);
        choice = t_true ? std::make_pair(two.first, two.second)
                        : std::make_pair(two.second, two.first);

        fprintf(stderr,
            "Debug: build_arm_plan_now(branch=%d): fallback guard=%s (uq=%s) "
            "cands=%zu TRUE→%d FALSE→%d pre=%d\n",
            branch_id,
            guard_name.c_str(), guard_uq.c_str(),
            cands.size(),
            choice.first, choice.second, pre_state);
        fflush(stderr);
    }

    ctx.arm_choice[obj_key] = choice;
    return true;
}


static std::unordered_set<std::string> g_recursive_fn_names_str;


std::unordered_map<tree, ArmChoiceSet> arm_choice;
//std::unordered_map<tree, ArmChoiceSet> arm_sets;     // key = norm_obj_key(obj)
//std::unordered_map<tree, Bitset>       active_set;   // key = norm_obj_key(obj), set for the BB you're in


static std::string tree_name(tree t) {
    if (!t) return "<null>";
    t = strip_nops(t);

    if (TREE_CODE(t) == CONST_DECL && DECL_NAME(t)) {
        const char* r = IDENTIFIER_POINTER(DECL_NAME(t));
        return r ? std::string(r) : "const_decl";
    }

    if (TREE_CODE(t) == INTEGER_CST) {
        long long k = tree_to_shwi(t);
        return std::to_string(k);
    }

    if (TREE_CODE(t) == SSA_NAME) {
        tree v = SSA_NAME_VAR(t);
        if (v && DECL_NAME(v)) {
            const char* r = IDENTIFIER_POINTER(DECL_NAME(v));
            return r ? std::string(r) : "ssa";
        }
        return "ssa";
    }

    if ((TREE_CODE(t) == VAR_DECL || TREE_CODE(t) == PARM_DECL) && DECL_NAME(t)) {
        const char* r = IDENTIFIER_POINTER(DECL_NAME(t));
        return r ? std::string(r) : "var";
    }

    return std::string(get_tree_code_name(TREE_CODE(t)));
}

static const char* cond_code_name(enum tree_code code) {
    switch (code) {
        case EQ_EXPR: return "==";
        case NE_EXPR: return "!=";
        case LT_EXPR: return "<";
        case LE_EXPR: return "<=";
        case GT_EXPR: return ">";
        case GE_EXPR: return ">=";
        default: return "?";
    }
}

static void debug_dump_branch_rules(int pre, const std::string& method) {
    const std::string uq = unqualified_name(method);

    fprintf(stderr, "Debug: Branch_Rules dump for pre=%d method=%s (uq=%s):\n",
            pre, method.c_str(), uq.c_str());

    auto itB = Branch_Rules.find(pre);
    if (itB == Branch_Rules.end()) {
        fprintf(stderr, "  (none for this pre state)\n");
        return;
    }
    for (const auto& br : itB->second) {
        bool name_match = (br.method == method) || (br.method == uq);
        if (!name_match) continue;

        fprintf(stderr, "  cond=%d -> next=%d (stored method=%s)\n",
                br.cond, br.next, br.method.c_str());
    }
}


static void debug_print_condition(gimple* cond, const char* tag, int ifid) {
    if (!cond || gimple_code(cond) != GIMPLE_COND) {
        fprintf(stderr, "Debug[%s]: ifid=%d cond=<not GIMPLE_COND>\n", tag, ifid);
        return;
    }

    tree lhs = strip_nops(gimple_cond_lhs(cond));
    tree rhs = strip_nops(gimple_cond_rhs(cond));
    enum tree_code cc = gimple_cond_code(cond);

    fprintf(stderr, "Debug[%s]: ifid=%d cond=(%s %s %s)\n",
            tag, ifid,
            tree_name(lhs).c_str(),
            cond_code_name(cc),
            tree_name(rhs).c_str());
}


static bool cond_compares_var_to_const(gimple* cond_stmt, tree &var_out, CondOp &op_out, int &K_out) {
    if (!cond_stmt || gimple_code(cond_stmt) != GIMPLE_COND) return false;

    enum tree_code code = gimple_cond_code(cond_stmt);
    if (code != EQ_EXPR && code != NE_EXPR) return false;

    op_out = (code == EQ_EXPR) ? OP_EQ : OP_NE;

    tree lhs = strip_nops(gimple_cond_lhs(cond_stmt));
    tree rhs = strip_nops(gimple_cond_rhs(cond_stmt));

    // One side must be "a variable-ish thing", other must be const/enum
    int K;
    if (get_const_int(rhs, K)) { var_out = lhs; K_out = K; return true; }
    if (get_const_int(lhs, K)) { var_out = rhs; K_out = K; return true; }

    return false;
}

static bool cond_var_and_const(gimple* cond, tree &var_out, CondOp &op_out, int &K_out) {
    if (!cond || gimple_code(cond) != GIMPLE_COND) return false;

    enum tree_code code = gimple_cond_code(cond);
    if (code == EQ_EXPR) op_out = OP_EQ;
    else if (code == NE_EXPR) op_out = OP_NE;
    else return false;

    tree lhs = strip_nops(gimple_cond_lhs(cond));
    tree rhs = strip_nops(gimple_cond_rhs(cond));

    int K;
    if (get_const_int(rhs, K)) { var_out = lhs; K_out = K; return true; }
    if (get_const_int(lhs, K)) { var_out = rhs; K_out = K; return true; }
    return false;
}

static std::string guard_name_from_ssa(tree v) {
    v = strip_nops(v);
    if (!v || TREE_CODE(v) != SSA_NAME) return "<unknown>";

    gimple* def = SSA_NAME_DEF_STMT(v);
    if (!def || !is_gimple_call(def)) return "<unknown>";

    tree gd = gimple_call_fndecl(def);
    if (!gd || !DECL_NAME(gd)) return "<unknown>";

    return demangle(IDENTIFIER_POINTER(DECL_NAME(gd)));
}


static void ensure_guard_origin(BranchCtx &ctx, tree obj_address) {
    if (ctx.guard_pre_state >= 0 && !ctx.guard_method.empty())
        return;

    int pre = -1;
    std::string meth;
    if (load_last_guard(obj_address, pre, meth)) {
        ctx.guard_pre_state = pre;
        ctx.guard_method = meth;

        fprintf(stderr, "Debug: Inherit guard origin: pre=%d method=%s\n",
                ctx.guard_pre_state, ctx.guard_method.c_str());
    } else {
        fprintf(stderr, "Debug: No guard origin available to inherit.\n");
    }
}



static int first_set_bit(const Bitset& b) {
    for (int i = 0; i < (int)b.size(); ++i) {
        if (b.test(i)) return i;
    }
    return -1;
}



static tree switch_index_from_stmt(gimple* sw) {
    const gswitch* gs = as_a<const gswitch*>(sw);
    return gimple_switch_index(gs);
}

// Returns true if bb is one of the switch destinations.
// If it’s default, is_default_out=true (case_value_out ignored).
static bool switch_case_value_for_bb(gimple* sw,
                                     basic_block bb,
                                     int &case_value_out,
                                     bool &is_default_out)
{
    const gswitch* gs = as_a<const gswitch*>(sw);
    unsigned n = gimple_switch_num_labels(gs);

    for (unsigned i = 0; i < n; ++i) {
        tree lab = gimple_switch_label(gs, i);
        if (!lab) continue;

        // In GCC, switch labels are usually CASE_LABEL_EXPR.
        // label_to_block wants the LABEL_DECL (CASE_LABEL(lab)).
        tree tgt = lab;
        if (TREE_CODE(lab) == CASE_LABEL_EXPR && CASE_LABEL(lab))
            tgt = CASE_LABEL(lab);

        basic_block dest = label_to_block(cfun, tgt);

        // Fallback: some forms might already be a label-like node
        if (dest != bb) {
             basic_block dest2 = label_to_block(cfun, lab);
        //    fprintf(stderr,
         //   "Debug: SWITCH match? cur_bb=%d lab_code=%s tgt_code=%s dest=%d alt_dest=%d\n",
           // bb->index,
           // get_tree_code_name(TREE_CODE(lab)),
         //   get_tree_code_name(TREE_CODE(tgt)),
           // dest ? dest->index : -1,
           // dest2 ? dest2->index : -1);
          

            if (dest2 != bb) continue;
        }

        tree low = CASE_LOW(lab);   // default has no CASE_LOW
        if (!low) {
            is_default_out = true;
            return true;
        }

        if (TREE_CODE(low) == INTEGER_CST) {
            case_value_out = (int)tree_to_shwi(low);
            is_default_out = false;
            return true;
        }

        // Not a simple integer case (range case etc.)
        return false;
    }

    return false;
}

// Full, robust handle_switch_entry:
// - seeds the case state
// - ALWAYS pushes incoming state to ctx.state_stack[key] so restore works
// - stores "active case value" so we can finalize later
// - does NOT rely on g_saved_state_stack
// - uses %lu not %zu

static void handle_switch_entry(gimple* sw, basic_block bb, tree obj_address)
{
    if (!sw || gimple_code(sw) != GIMPLE_SWITCH) return;

    int ifid = get_if_else_id_for_terminator(sw);
    BranchCtx& ctx = ensure_ifctx(ifid, sw);

    tree key = norm_obj_key(obj_address);

    // Must know which guard call produced the switch value and the pre-state
    ensure_guard_origin(ctx, obj_address);

    int pre = ctx.guard_pre_state;
    const std::string& gname = ctx.guard_method;

    if (pre < 0 || gname.empty()) {
      //  fprintf(stderr, "Debug: SWITCH: no guard origin (ifid=%d)\n", ifid);
        return;
    }

    int case_val = 0;
    bool is_default = false;

    if (!switch_case_value_for_bb(sw, bb, case_val, is_default)) {
    //    fprintf(stderr,
              //  "Debug: SWITCH: could not match BB to a case/default (ifid=%d bb=%d)\n",
              //  ifid, bb ? bb->index : -1);
        return;
    }

    // Candidate transitions from pre under this guard method
    std::vector<int> candidates =
        transitions_for(pre, gname, gimple_location(sw));

    Bitset possible;
    possible.reset();

    if (!is_default) {
        int mapped = branch_next_for_cond(pre, gname, case_val);
        if (mapped >= 0 && mapped < (int)possible.size()) {
            possible.set(mapped);
        } else {
           // fprintf(stderr,
              //  "Debug: SWITCH: no mapping for case=%d (pre=%d guard=%s)\n",
              //  case_val, pre, gname.c_str());
        }
    } else {
        // default: conservative (all candidates)
        for (int s : candidates)
            if (0 <= s && s < (int)possible.size()) possible.set(s);
    }

    // Save the case candidate set for this BB
    ctx.active_set[key] = possible;

    // Record which case we are in (for later finalization)
    ctx.sw_active_case_val[key] = is_default ? -1 : case_val;
    ctx.sw_active_case_bb[key]  = bb ? bb->index : -1;

    //   save incoming state for this arm, so we can restore later.
    {
        int prev = 0;
        auto itPrev = state_manager.object_states.find(obj_address);
        if (itPrev != state_manager.object_states.end()) prev = itPrev->second;
        ctx.state_stack[key].push_back(prev);
    }

    // Apply singleton state (if known)
    if (possible.count() == 1) {
        int next_state = first_set_bit(possible);
        if (next_state >= 0) {
            state_manager.object_states[obj_address] = next_state;
        }
    }

  //  fprintf(stderr,
     //   "Debug: SWITCH enter ifid=%d obj=%p bb=%d case=%s possible_count=%lu\n",
       // ifid, (void*)obj_address,
      //  bb ? bb->index : -1,
     //   is_default ? "default" : std::to_string(case_val).c_str(),
      //  (unsigned long)possible.count());
}


// Returns true if SWITCH index is the call result, possibly through 1-2 SSA copies.
static bool call_feeds_switch_index(gimple* call_stmt, gimple* term_switch) {
    if (!call_stmt || !term_switch) return false;
    if (!is_gimple_call(call_stmt)) return false;
    if (gimple_code(term_switch) != GIMPLE_SWITCH) return false;

    const gswitch* gs = as_a<const gswitch*>(term_switch);
    tree idx = strip_nops(gimple_switch_index(gs));
    tree lhs = gimple_call_lhs(call_stmt);
    if (!lhs) return false;
    lhs = strip_nops(lhs);

    // Direct: switch(idx) where idx == call lhs
    if (idx && operand_equal_p(idx, lhs, 0)) return true;

    // If idx is SSA_NAME, look at its def:
    if (idx && TREE_CODE(idx) == SSA_NAME) {
        gimple* def = SSA_NAME_DEF_STMT(idx);
        if (!def) return false;

        // Common: idx = lhs (GIMPLE_ASSIGN)
        if (gimple_code(def) == GIMPLE_ASSIGN) {
            tree rhs1 = strip_nops(gimple_assign_rhs1(def));
            if (rhs1 && operand_equal_p(rhs1, lhs, 0)) return true;

            // Sometimes: idx = (NOP) lhs
            // strip_nops above already handles most of this.
        }

        // Sometimes the switch index SSA is *directly* defined by the call
        if (is_gimple_call(def)) {
            tree def_lhs = gimple_call_lhs(def);
            if (def_lhs && operand_equal_p(strip_nops(def_lhs), lhs, 0))
                return true;
        }
    }

    return false;
}
//

static bool is_end_of_switch_case_bb(basic_block bb) {
    if (!bb) return false;

    gimple_stmt_iterator it = gsi_last_bb(bb);
    if (gsi_end_p(it)) return false;

    gimple* last = gsi_stmt(it);
    if (!last) return false;

    // break; is typically a GOTO to the join label
    if (gimple_code(last) == GIMPLE_GOTO) return true;

    // return; also ends an arm
    if (gimple_code(last) == GIMPLE_RETURN) return true;

    return false;
}

static void end_switch_case_processing(BranchCtx& ctx,
                                       tree key,
                                       tree obj_address,
                                       int arm_end_state,
                                       location_t loc)
{
    Bitset &u = ctx.sw_exit_union[key];
    if (0 <= arm_end_state && arm_end_state < (int)u.size())
        u.set(arm_end_state);

    // FAIL FAST: as soon as we see >1 end-state
    if (u.count() > 1) {
        error_at(loc,
                 "Typestate: switch cases end in different states (count=%lu).",
                 (unsigned long)u.count());
        return;
    }

    // countdown
    auto it = ctx.sw_remaining.find(key);
    if (it == ctx.sw_remaining.end()) ctx.sw_remaining[key] = 0;
    else it->second--;

    if (ctx.sw_remaining[key] <= 0) {
        // u.count() is 1 here
        int final_state = first_set_bit(u);
        if (final_state >= 0) state_manager.object_states[obj_address] = final_state;

        ctx.sw_exit_union.erase(key);
        ctx.sw_remaining.erase(key);
        ctx.sw_remaining_init.erase(key);
    }
}


static void finalize_switch_if_exited(tree obj_address, basic_block bb, location_t loc)
{
    // If still inside a switch arm, do nothing
    gimple* ctl = controlling_terminator_of(bb);
    if (ctl && gimple_code(ctl) == GIMPLE_SWITCH) return;

    // We are NOT in a switch-controlled BB. If there is a pending switch for this object, finalize it.

    tree key = norm_obj_key(obj_address);

    int pre = -1;
    std::string meth;
    if (!load_last_guard(obj_address, pre, meth)) return; // no active guard/switch tracked

    // Find the BranchCtx for the most recent switch terminator ifid.
    // We can’t get ifid directly from load_last_guard, so we use the only active switch ctx:
    // heuristic: scan IFCTX for a ctx where sw_remaining has this key.
    for (auto &kv : IFCTX) {
        BranchCtx &ctx = kv.second;

        auto itR = ctx.sw_remaining.find(key);
        if (itR == ctx.sw_remaining.end()) continue; // not active

        // Finalize the *current* last case using current state
        int arm_end_state = 0;
        auto itS = state_manager.object_states.find(obj_address);
        if (itS != state_manager.object_states.end()) arm_end_state = itS->second;

        fprintf(stderr,
                "Debug: SWITCH EXIT finalize obj=%p end_state=%d remaining=%d\n",
                (void*)obj_address, arm_end_state, itR->second);

        // We finalize one arm here (the last one) because we just exited the switch region.
        end_switch_case_processing(ctx, key, obj_address, arm_end_state, loc);

        // Also clear “current case bb” tracker if you have it
        if (ctx.sw_current_case_bb.count(key)) ctx.sw_current_case_bb.erase(key);

        break; // only one active switch per object expected
    }
}

void process_callee_decl(tree callee_decl, gimple* stmt) {
    if (!callee_decl || !stmt) return;
    if (!is_gimple_call(stmt)) return;

    tree callee = gimple_call_fndecl(stmt);
    if (!callee || TREE_CODE(callee) != FUNCTION_DECL) return;
    if (TREE_CODE(callee_decl) != FUNCTION_DECL) return;

    const char* raw_name =
        (DECL_NAME(callee_decl) ? IDENTIFIER_POINTER(DECL_NAME(callee_decl)) : nullptr);
    std::string callee_name(raw_name ? raw_name : "");

    // Normalize base name "Class::foo" -> "foo"
    std::string base_name = callee_name;
    if (auto pos = base_name.rfind("::"); pos != std::string::npos)
        base_name = base_name.substr(pos + 2);

    // ==================================================
    // (A) inter-proc "dive"
    // ==================================================
    {
        unsigned nargs = gimple_call_num_args(stmt);
        tree flagged_tmp = nullptr;
        tree first_obj = nullptr;
        tree dive_obj  = nullptr;

        bool is_member =
            DECL_CONTEXT(callee_decl) &&
            TREE_CODE(DECL_CONTEXT(callee_decl)) == RECORD_TYPE &&
            !DECL_STATIC_FUNCTION_P(callee_decl);

        if (nargs >= 1) {
            tree arg0 = gimple_call_arg(stmt, 0);
            if (arg0 && is_object_of_Flagged(arg0, flagged_tmp)) {
                first_obj = track_all_aliases(get_original_object(flagged_tmp));
            }
        }

        if (is_member) {
            if (nargs >= 2) {
                tree arg1 = gimple_call_arg(stmt, 1);
                if (arg1 && is_object_of_Flagged(arg1, flagged_tmp)) {
                    dive_obj = track_all_aliases(get_original_object(flagged_tmp));
                }
            }
        } else {
            dive_obj = first_obj;
        }

        if (dive_obj) {
            int in_state = -1;
            if (auto it = state_manager.object_states.find(dive_obj);
                it != state_manager.object_states.end())
                in_state = it->second;

            AnalysisContextKey key{ callee_decl, dive_obj, in_state, "" };

            if (analyzed_contexts.insert(key).second) {
                ParamMap p2a;
                tree formal = DECL_ARGUMENTS(callee_decl);
                for (unsigned idx = 0; idx < nargs && formal; ++idx, formal = TREE_CHAIN(formal)) {
                    tree actual = gimple_call_arg(stmt, idx);
                    tree canon  = actual ? track_all_aliases(get_original_object(actual)) : NULL_TREE;
                    p2a[formal] = canon;
                }

                if (callee_name == "operator->") {
                    if (tree lhs = gimple_call_lhs(stmt)) {
                        state_manager.register_alias(lhs, dive_obj);
                    }
                }

                if (gimple_has_body_p(callee_decl)) {
                    if (function* fn = DECL_STRUCT_FUNCTION(callee_decl)) {
                        push_cfun(fn);
                        Analyse_gimple_in_function(std::move(p2a));
                        pop_cfun();
                    }
                }
            }
        }
    }

    // ==================================================
    // (B) Typestate enforcement
    // ==================================================
    if (DECL_CONTEXT(callee_decl) && TREE_CODE(DECL_CONTEXT(callee_decl)) == RECORD_TYPE) {

        // class gate
        std::string class_name;
        if (tree cls = DECL_CONTEXT(callee_decl)) {
            if (TYPE_NAME(cls) && DECL_NAME(TYPE_NAME(cls))) {
                const char* cls_raw = IDENTIFIER_POINTER(DECL_NAME(TYPE_NAME(cls)));
                if (cls_raw) class_name = demangle(cls_raw);
            }
        }
        if (class_name != TypestateClassConnector_args[0]) goto PARAMS;

        // this
        tree this_arg = NULL_TREE;
        if (!DECL_STATIC_FUNCTION_P(callee_decl))
            this_arg = gimple_call_arg(stmt, 0);
        if (!this_arg) goto PARAMS;

        this_arg = get_original_object(this_arg);
        this_arg = track_all_aliases(this_arg);
        if (!this_arg) goto PARAMS;

        // operator-> alias only
        if (callee_name == "operator->") {
            if (tree lhs = gimple_call_lhs(stmt)) {
                state_manager.register_alias(lhs, this_arg);
            }
            goto PARAMS;
        }

        // tracked object
        tree obj_address = NULL_TREE;
        if (!is_object_of_Flagged(this_arg, obj_address)) goto PARAMS;

        basic_block bb = gimple_bb(stmt);
        gimple_stmt_iterator tail = gsi_last_bb(bb);
        gimple* term = gsi_end_p(tail) ? nullptr : gsi_stmt(tail);

        // --------------------------
        // DEBUG: print every call encountered
        // --------------------------
        {
            int st = 0;
            auto itS = state_manager.object_states.find(obj_address);
            if (itS != state_manager.object_states.end()) st = itS->second;
            printf("\n[CALL] bb=%d term=%d callee=%s obj=%p state=%d\n",
                   bb ? bb->index : -1,
                   term ? (int)gimple_code(term) : -1,
                   callee_name.c_str(),
                   (void*)obj_address,
                   st);
        }

        // Helper: extract integer constant K from a GIMPLE_COND (either side).
        auto extract_int_const_from_cond = [&](gimple* cond, int& K_out) -> bool {
            if (!cond || gimple_code(cond) != GIMPLE_COND) return false;
            const gcond* gc = as_a<const gcond*>(cond);
            tree lhs = strip_nops(gimple_cond_lhs(gc));
            tree rhs = strip_nops(gimple_cond_rhs(gc));

            auto getK = [&](tree t) -> bool {
                if (!t) return false;
                if (TREE_CODE(t) == INTEGER_CST) {
                    K_out = (int)TREE_INT_CST_LOW(t);
                    return true;
                }
                return false;
            };

            return getK(lhs) || getK(rhs);
        };

        // Helper: synthesize ctx.arm_sets for IF that is part of else-if chain
        // using last_guard (pre-state + method) and cond constant K.
        auto synthesize_if_arm_sets_from_last_guard = [&](BranchCtx& ctx, tree key, gimple* cond_term) -> bool {
            int pre = -1;
            std::string meth;
            if (!load_last_guard(obj_address, pre, meth)) {
                printf("           [SYNTH] load_last_guard FAILED\n");
                return false;
            }
            if (pre < 0 || meth.empty()) {
                printf("           [SYNTH] last_guard invalid pre=%d meth='%s'\n", pre, meth.c_str());
                return false;
            }

            int K = 0;
            if (!extract_int_const_from_cond(cond_term, K)) {
                printf("           [SYNTH] could not extract const K from cond\n");
                return false;
            }

            // candidates from pre under the guard method
            std::vector<int> candidates = transitions_for(pre, meth, gimple_location(cond_term));

            // map for (pre, meth, K) -> mapped state (true arm)
            int mapped = branch_next_for_cond(pre, meth, K);

            ArmChoiceSet sets;
            sets.tset.reset();
            sets.fset.reset();

            auto set_all_candidates = [&](Bitset& out) {
                for (int s : candidates)
                    if (0 <= s && s < (int)out.size()) out.set(s);
            };

            if (mapped >= 0 && mapped < (int)sets.tset.size()) {
                sets.tset.set(mapped);
            } else {
                // if mapping fails, be conservative: true arm could be any candidate
                set_all_candidates(sets.tset);
                printf("           [SYNTH] mapping FAILED for K=%d; using conservative tset\n", K);
            }

            // false arm: all candidates except mapped (or all if mapped invalid)
            if (mapped >= 0) {
                for (int s : candidates) {
                    if (s < 0 || s >= (int)sets.fset.size()) continue;
                    if (s == mapped) continue;
                    sets.fset.set(s);
                }
                // if mapped was the only candidate, keep false as mapped (avoid empty)
                if (sets.fset.count() == 0 && sets.tset.count() == 1) sets.fset = sets.tset;
            } else {
                set_all_candidates(sets.fset);
            }

            // install in this ctx (ifid)
            ctx.arm_sets[key] = sets;

            // also update ctx guard origin for debugging clarity
            ctx.guard_pre_state = pre;
            ctx.guard_method = meth;

            printf("           [SYNTH] created arm_sets from last_guard: pre=%d meth=%s K=%d mapped=%d (t=%lu f=%lu)\n",
                   pre, meth.c_str(), K, mapped,
                   (unsigned long)sets.tset.count(),
                   (unsigned long)sets.fset.count());
            return true;
        };

        // --------------------------
        // APPLY entry seed when entering under a controlling terminator
        // --------------------------
        {
            gimple* pred_term = controlling_terminator_of(bb);

            // ==========================
            // IF ENTRY (FIXED)
            // ==========================
            if (pred_term && gimple_code(pred_term) == GIMPLE_COND) {
                int ifid = get_if_else_id_for_terminator(pred_term);
                BranchCtx& ctx = ensure_ifctx(ifid, pred_term);
                tree key = norm_obj_key(obj_address);

                int st_before = 0;
                if (auto itS = state_manager.object_states.find(obj_address);
                    itS != state_manager.object_states.end()) st_before = itS->second;

                int arm = arm_for_bb_under_cond(pred_term, bb);

                printf("[ENTER IF] ifid=%d bb=%d arm=%d obj=%p key=%p state(before)=%d\n",
                       ifid, bb ? bb->index : -1, arm,
                       (void*)obj_address, (void*)key, st_before);

                // If arm_sets missing (like your ifid=1 else-if), synthesize from last_guard + cond constant.
                if (!ctx.arm_sets.count(key)) {
                    printf("           arm_sets MISSING => trying SYNTH from last_guard\n");

                    bool ok = synthesize_if_arm_sets_from_last_guard(ctx, key, pred_term);
                    if (!ok) {
                        printf("           [SYNTH] FAILED => cannot seed this IF\n");
                    }
                }

                // If we have a guard pre-state (from PLAN or SYNTH), reset leaked state before seeding.
                // This is the key to stop state 9 leaking into else-if.
                if (ctx.guard_pre_state >= 0) {
                    if (auto it = state_manager.object_states.find(obj_address);
                        it != state_manager.object_states.end())
                    {
                        if (it->second != ctx.guard_pre_state) {
                            printf("           [RESET] state leak detected: %d -> pre=%d\n",
                                   it->second, ctx.guard_pre_state);
                            it->second = ctx.guard_pre_state;
                        }
                    } else {
                        state_manager.object_states[obj_address] = ctx.guard_pre_state;
                        printf("           [RESET] state init to pre=%d\n", ctx.guard_pre_state);
                    }
                }

                auto itSets = ctx.arm_sets.find(key);
                if (itSets != ctx.arm_sets.end()) {
                    printf("           arm_sets FOUND (t=%lu f=%lu)\n",
                           (unsigned long)itSets->second.tset.count(),
                           (unsigned long)itSets->second.fset.count());

                    // Always push incoming state for this arm (after reset).
                    int prev = 0;
                    auto itPrev = state_manager.object_states.find(obj_address);
                    if (itPrev != state_manager.object_states.end()) prev = itPrev->second;
                    ctx.state_stack[key].push_back(prev);

                    printf("           pushed prev=%d stack_sz=%zu\n",
                           prev, (size_t)ctx.state_stack[key].size());

                    Bitset possible; possible.reset();
                    if (arm == 1) possible = itSets->second.tset;
                    else if (arm == 0) possible = itSets->second.fset;

                    ctx.active_set[key] = possible;
                    if (arm == 0) g_else_chain_cands[key] = possible;

                    printf("           seeded candidates=%lu\n",
                           (unsigned long)possible.count());

                    if (possible.count() == 1) {
                        int s = first_set_bit(possible);
                        if (s >= 0) {
                            state_manager.object_states[obj_address] = s;
                            printf("           new state=%d (singleton)\n", s);
                        }
                    } else {
                        int now = 0;
                        auto itNow = state_manager.object_states.find(obj_address);
                        if (itNow != state_manager.object_states.end()) now = itNow->second;
                        printf("           state now=%d (non-singleton)\n", now);
                    }
                } else {
                    printf("           arm_sets still missing => NO push/seed\n");
                }
            }

            // SWITCH entry (unchanged)
            if (pred_term && gimple_code(pred_term) == GIMPLE_SWITCH) {
                handle_switch_entry(pred_term, bb, obj_address);
            }
        }

        // current state after entry seeding
        int current_state = 0;
        if (auto itS = state_manager.object_states.find(obj_address);
            itS != state_manager.object_states.end())
            current_state = itS->second;
        else
            state_manager.object_states[obj_address] = current_state;

        // --------------------------
        // SWITCH PLAN 
        // --------------------------
        if (term && gimple_code(term) == GIMPLE_SWITCH) {
            if (call_feeds_switch_index(stmt, term)) {

                int ifid = get_if_else_id_for_terminator(term);
                BranchCtx& ctx = ensure_ifctx(ifid, term);

                // Guard origin: pre-state BEFORE applying the guard transition
                ctx.guard_pre_state = current_state;
                ctx.guard_method    = callee_name;

                tree key = norm_obj_key(obj_address);

                int arms = 0;
                const gswitch* gs = as_a<const gswitch*>(term);
                unsigned nlabels = gimple_switch_num_labels(gs);

                for (unsigned i = 0; i < nlabels; ++i) {
                    tree lab = gimple_switch_label(gs, i);
                    if (!lab) continue;
                    if (TREE_CODE(lab) != CASE_LABEL_EXPR) continue;
                    if (CASE_LOW(lab)) arms++;
                }
                if (arms <= 0) arms = 1;

                // Per-object countdown + init
                ctx.sw_remaining[key]      = arms;
                ctx.sw_remaining_init[key] = arms;

                printf("[SW-PLAN] ifid=%d obj=%p pre=%d guard=%s arms=%d\n",
                       ifid, (void*)obj_address, ctx.guard_pre_state,
                       ctx.guard_method.c_str(), arms);

                save_last_guard(obj_address, ctx.guard_pre_state, ctx.guard_method,
                                &ctx.remaining, ctx.remaining_init);

                return; // do not typecheck guard-producing call
            }
        }

        // --------------------------
        // IF PLAN 
        // --------------------------
        if (term && gimple_code(term) == GIMPLE_COND) {
            bool feeds = call_feeds_terminator_cond(stmt, term);
            printf("[IF-PLAN-CHECK] bb=%d callee=%s feeds=%d current_state=%d\n",
                   bb ? bb->index : -1, callee_name.c_str(), (int)feeds, current_state);

            if (feeds) {
                int ifid = get_if_else_id_for_terminator(term);
                BranchCtx& ctx = ensure_ifctx(ifid, term);

                ctx.guard_pre_state = current_state;
                ctx.guard_method = callee_name;

                printf("[PLAN] IF guard detected: ifid=%d method=%s pre_state=%d bb=%d\n",
                       ifid, callee_name.c_str(), current_state, bb ? bb->index : -1);

                debug_print_condition(term, "PLAN", ifid);

                record_pre_state(ifid, obj_address, current_state);

                std::vector<int> candidates =
                    transitions_for(current_state, callee_name, gimple_location(stmt));

                Bitset outset;
                for (int s : candidates) if (0 <= s && s < (int)outset.size()) outset.set(s);
                record_arm_out_set(ifid, obj_address, outset);

                tree key = norm_obj_key(obj_address);

                if (!ctx.arm_choice.count(key)) {
                    auto two = pick_two_arms(candidates);
                    std::pair<int,int> choice = { two.first, two.second };

                    tree res = gimple_call_lhs(stmt);
                    CondOp op; int K;

                    if (res && extract_eq_ne_with_const(term, res, op, K)) {
                        int mapped = branch_next_for_cond(current_state, callee_name, K);
                        printf("           extract const: K=%d op=%s mapped=%d\n",
                               K, (op == OP_EQ ? "EQ" : "NE"), mapped);

                        if (mapped >= 0) {
                            ArmChoiceSet sets;
                            sets.tset.reset();
                            sets.fset.reset();

                            auto fill_minus = [&](Bitset &out, int minus) {
                                for (int s : candidates) {
                                    if (s < 0 || s >= (int)out.size()) continue;
                                    if (s == minus) continue;
                                    out.set(s);
                                }
                            };

                            if (op == OP_EQ) {
                                if (0 <= mapped && mapped < (int)sets.tset.size()) sets.tset.set(mapped);
                                fill_minus(sets.fset, mapped);
                            } else {
                                fill_minus(sets.tset, mapped);
                                if (0 <= mapped && mapped < (int)sets.fset.size()) sets.fset.set(mapped);
                            }

                            ctx.arm_sets[key] = sets;
                            g_else_chain_cands[key] = sets.fset;

                            printf("           arm_sets created: tcnt=%lu fcnt=%lu\n",
                                   (unsigned long)sets.tset.count(),
                                   (unsigned long)sets.fset.count());
                        } else {
                            printf("           WARNING: branch_next_for_cond FAILED\n");
                        }
                    } else {
                        printf("           WARNING: could not extract (res == const) from cond\n");
                    }

                    ctx.arm_choice[key] = choice;
                } else {
                    printf("           arm_choice already exists for key\n");
                }

                save_last_guard(obj_address, ctx.guard_pre_state, ctx.guard_method,
                                &ctx.remaining, ctx.remaining_init);

                return; // do not typecheck guard-producing call
            }
        }

        // --------------------------
        // Non-guarded path (prints state just before checking)
        // --------------------------
        {
            int branch_id = get_if_else_id_for_bb(bb);
            if (branch_id >= 0) {
                gimple* ctl = controlling_terminator_of(bb);

                if (ctl && (gimple_code(ctl) == GIMPLE_SWITCH || gimple_code(ctl) == GIMPLE_COND)) {
                    int st = 0;
                    auto itS = state_manager.object_states.find(obj_address);
                    if (itS != state_manager.object_states.end()) st = itS->second;

                    printf("[TYPECHECK] ctl=%d bb=%d method=%s obj=%p state=%d\n",
                           (int)gimple_code(ctl),
                           bb ? bb->index : -1,
                           callee_name.c_str(),
                           (void*)obj_address,
                           st);

                    state_manager.Typestate_Checking(obj_address, callee_name, gimple_location(stmt));
                    return;
                }

                int st = 0;
                auto itS = state_manager.object_states.find(obj_address);
                if (itS != state_manager.object_states.end()) st = itS->second;

                printf("[DEFER] branch_id=%d bb=%d method=%s obj=%p state=%d\n",
                       branch_id, bb ? bb->index : -1,
                       callee_name.c_str(), (void*)obj_address, st);

                defer_branch_handling(branch_id, obj_address, callee_name, gimple_location(stmt), stmt);
                return;
            }

            int st = 0;
            auto itS = state_manager.object_states.find(obj_address);
            if (itS != state_manager.object_states.end()) st = itS->second;

            printf("[TYPECHECK] (no-branch) bb=%d method=%s obj=%p state=%d\n",
                   bb ? bb->index : -1,
                   callee_name.c_str(),
                   (void*)obj_address,
                   st);

            state_manager.Typestate_Checking(obj_address, callee_name, gimple_location(stmt));
            return;
        }
    }

    // --------------------------
    // Arm-finalization hook (restore + switch join) + printf
    // --------------------------
    {
        tree this_arg2 = NULL_TREE;
        if (!DECL_STATIC_FUNCTION_P(callee_decl))
            this_arg2 = gimple_call_arg(stmt, 0);
        if (!this_arg2) goto PARAMS;

        this_arg2 = get_original_object(this_arg2);
        this_arg2 = track_all_aliases(this_arg2);
        if (!this_arg2) goto PARAMS;

        tree obj_address2 = NULL_TREE;
        if (!is_object_of_Flagged(this_arg2, obj_address2)) goto PARAMS;

        basic_block bb_here = gimple_bb(stmt);

        if (bb_here && is_inside_if_or_switch(bb_here)) {
            gimple* pred_term = controlling_terminator_of(bb_here);
            if (!pred_term) return;

            bool end_arm = is_end_of_arm_bb(bb_here);

            // SWITCH: also finalize at "break;" blocks (usually GIMPLE_GOTO)
            if (!end_arm && gimple_code(pred_term) == GIMPLE_SWITCH) {
                gimple_stmt_iterator it = gsi_last_bb(bb_here);
                if (!gsi_end_p(it)) {
                    gimple* last = gsi_stmt(it);
                    if (last && (gimple_code(last) == GIMPLE_GOTO ||
                                 gimple_code(last) == GIMPLE_RETURN)) {
                        end_arm = true;
                    }
                }
            }

            if (end_arm) {
                tree key = norm_obj_key(obj_address2);

                int ifid = get_if_else_id_for_terminator(pred_term);
                BranchCtx& ctx = ensure_ifctx(ifid, pred_term);

                int arm_end_state = 0;
                if (auto itS = state_manager.object_states.find(obj_address2);
                    itS != state_manager.object_states.end())
                    arm_end_state = itS->second;

                printf("[ARM END] pred=%d ifid=%d bb=%d obj=%p state_before_restore=%d stack_sz=%zu\n",
                       (int)gimple_code(pred_term),
                       ifid,
                       bb_here ? bb_here->index : -1,
                       (void*)obj_address2,
                       arm_end_state,
                       (size_t)ctx.state_stack[key].size());

                auto &stk = ctx.state_stack[key];
                if (!stk.empty()) {
                    printf("          restoring=%d\n", stk.back());
                    int restored = stk.back();
                    stk.pop_back();
                    state_manager.object_states[obj_address2] = restored;
                    printf("          restored state=%d stack_sz=%zu\n",
                           restored, (size_t)stk.size());
                } else {
                    printf("          WARNING: stack empty (NO RESTORE)\n");
                }

                if (gimple_code(pred_term) == GIMPLE_SWITCH) {
                    end_switch_case_processing(ctx, key, obj_address2,
                                               arm_end_state, gimple_location(pred_term));
                } else {
                    end_branch_processing(key, bb_here);
                }
                return;
            } else {
                printf("[ARM] not end_arm bb=%d pred=%d\n",
                       bb_here ? bb_here->index : -1,
                       (int)gimple_code(pred_term));
            }
        }
        return;
    }

PARAMS:
    // --------------------------
    // Parameter → argument alias propagation (kept)
    // --------------------------
    if (TREE_CODE(callee_decl) == FUNCTION_DECL) {
        tree param = DECL_ARGUMENTS(callee_decl);
        unsigned int arg_count = gimple_call_num_args(stmt);
        unsigned int i = 0;
        for (; param; param = TREE_CHAIN(param), ++i) {
            tree arg = (i < arg_count) ? gimple_call_arg(stmt, i) : NULL_TREE;
            if (arg) { arg = get_original_object(arg); arg = track_all_aliases(arg); }

            tree obj_address3 = NULL_TREE;
            if (arg && is_object_of_Flagged(arg, obj_address3)) {
                state_manager.register_alias(param, obj_address3);
                if (DECL_STRUCT_FUNCTION(callee_decl)) {
                    process_function_body(callee_decl, obj_address3);
                }
            }
        }
    }
}


/*******************************
 * GIMPLE Statement Analysis
 *******************************/

 static const char* g_mark_file = nullptr;
void Analyse_gimple_statement(gimple* stmt, gimple_stmt_iterator* gsi)
{
    if (!stmt) return;

    
    if (is_gimple_assign(stmt))
    {
        tree L = gimple_assign_lhs(stmt);
        tree R = gimple_assign_rhs1(stmt);

        // ptr = &obj or T* p = &obj
        if (TREE_CODE(R) == ADDR_EXPR)
        {
            tree target  = TREE_OPERAND(R, 0);
            tree obj_addr = nullptr;
            if (is_object_of_Flagged(target, obj_addr))
            {
                tree lhs_root = get_original_object(L);
                state_manager.register_alias(lhs_root, obj_addr);
            }
        }

        // reference-type assignment T& r = obj;
        if (TREE_TYPE(L)
            && TREE_CODE(TREE_TYPE(L)) == REFERENCE_TYPE
            && TREE_CODE(R) == ADDR_EXPR)
        {
            tree target  = TREE_OPERAND(R, 0);
            tree obj_addr = nullptr;
            if (is_object_of_Flagged(target, obj_addr))
            {
                tree lhs_root = get_original_object(L);
                state_manager.register_alias(lhs_root, obj_addr);
            }
        }

        // object-to-object aliasing x = y
        tree Lobj = nullptr, Robj = nullptr;
        if (is_object_of_Flagged(L, Lobj) && is_object_of_Flagged(R, Robj))
        {
            state_manager.register_alias(Lobj, Robj);
        }
        else if (is_object_of_Flagged(R, Robj))
        {
            state_manager.register_alias(L, Robj);
        }

        // operator-> in an assignment (smart-pointer style)
        if (gimple_assign_rhs_code(stmt) == CALL_EXPR)
        {
            tree call   = gimple_assign_rhs1(stmt);
            tree callee = CALL_EXPR_FN(call);
            if (callee && TREE_CODE(callee) == FUNCTION_DECL)
            {
                const char* raw = IDENTIFIER_POINTER(DECL_NAME(callee));
                if (raw && std::string(raw) == "operator->")
                {
                    tree this_arg = CALL_EXPR_ARG(call, 0);
                    this_arg = track_all_aliases(get_original_object(this_arg));
                    state_manager.register_alias(L, this_arg);
                }
            }
        }

        return;
    }

    
    if (!is_gimple_call(stmt)) return;

    tree callee_decl = gimple_call_fndecl(stmt);
    if (!callee_decl) return;

    // For diagnostics: method name (unqualified)
    const char* nm = IDENTIFIER_POINTER(DECL_NAME(callee_decl));
    std::string method = nm ? demangle(nm) : "unknown_method";

    // Flag to detect if this call touches any flagged object
    tree flagged_obj = nullptr;
    tree tmp = nullptr;

    // 2a) Member call? Check 'this' first
    bool is_member = (DECL_CONTEXT(callee_decl)
                      && TREE_CODE(DECL_CONTEXT(callee_decl)) == RECORD_TYPE
                      && !DECL_STATIC_FUNCTION_P(callee_decl));

    if (is_member)
    {
        tree this_arg = gimple_call_arg(stmt, 0);
        if (this_arg)
        {
            // --- If 'this' is PHI, warn and skip checking ---
            if (TREE_CODE(this_arg) == SSA_NAME)
            {
                gimple* def = SSA_NAME_DEF_STMT(this_arg);
                if (def && gimple_code(def) == GIMPLE_PHI)
                {
                      error_at(gimple_location(stmt),
                               "Typestate: receiver is a PHI for %qs; skipping check",
                               method.c_str());
                            // after warning_at(...):
if (g_mark_file) {
    if (FILE* f = std::fopen(g_mark_file, "ab")) {
        std::fputs("phi\n", f);
        std::fclose(f);
    }
}

                    return;  // don't do any typestate checking for this call
                }
            }

            // else: see if it’s a flagged object to trigger normal processing
            if (is_object_of_Flagged(this_arg, tmp))
                flagged_obj = track_all_aliases(get_original_object(tmp));
        }
    }

    // 2b) If not a member (or 'this' wasn’t flagged), scan explicit args
    if (!flagged_obj)
    {
        unsigned nargs = gimple_call_num_args(stmt);
        for (unsigned i = 0; i < nargs; ++i)
        {
            tree arg = gimple_call_arg(stmt, i);
            if (!arg) continue;

            //  If an argument is PHI 
            if (TREE_CODE(arg) == SSA_NAME)
            {
                gimple* def = SSA_NAME_DEF_STMT(arg);
                if (def && gimple_code(def) == GIMPLE_PHI)
                {
                    // Check if this PHI-arg is the flagged object we care about
                    if (is_object_of_Flagged(arg, tmp))
                    {
                        warning_at(gimple_location(stmt), 0,
                                   "Typestate: argument is a PHI for %qs; skipping check",
                                   method.c_str());
                        return;  // skip checking this call entirely
                    }
                }
            }

            // Otherwise, if a plain flagged object appears, mark it
            if (!flagged_obj && is_object_of_Flagged(arg, tmp))
            {
                flagged_obj = track_all_aliases(get_original_object(tmp));
            }
        }
    }

    // 3) If no flagged object is touched, nothing to do
    if (!flagged_obj) return;

    // 4) Proceed with your existing static processing (no runtime)
    process_callee_decl(callee_decl, stmt);
}



/*───────────────────────────────────────────────────────────────────────────*/
/*                                                          */
/*    – seed the work-queue with every function that has a body             */
/*    – pull items, push cfun, run Analyse_gimple_in_function, pop cfun     */
/*───────────────────────────────────────────────────────────────────────────*/
void Analyse_all_functions()
{

     
    // ── 1) Find and seed only 'main' 
    tree main_decl = nullptr;
    cgraph_node *cgn = nullptr;
    FOR_EACH_DEFINED_FUNCTION(cgn)
    {
        tree d = cgn->decl;
        if (DECL_NAME(d) &&
            strcmp(IDENTIFIER_POINTER(DECL_NAME(d)), "main") == 0 &&
            gimple_has_body_p(d))
        {
            main_decl = d;
            break;
        }
    }
   harvest_annot_from_decl(main_decl);
    if (!main_decl)
        return;  

    // Seed the work‐queue with main, 
    AnalysisContextKey root_ctx{
        /* fn_decl    */ main_decl,
        /* obj        */ nullptr,
        /* state      */ 0,
        /* alias_head */ ""
    };
    work_queue.emplace(main_decl, ParamMap{}, root_ctx);

    //  2) Standard work‐loop 
    while (!work_queue.empty())
    {
        auto [fn_decl, param_map, ctx] = work_queue.front();
        work_queue.pop();

        if (!fn_decl || !gimple_has_body_p(fn_decl))
            continue;

        function *fn = DECL_STRUCT_FUNCTION(fn_decl);
        if (!fn)
            continue;

        push_cfun(fn);
        Analyse_gimple_in_function(param_map);
        pop_cfun();
    }

}

static void on_finish_parse_fn(void* gcc_data, void*) {
    tree fndecl = (tree)gcc_data;
    harvest_annot_from_decl(fndecl);
 
}

static void on_finish_decl(void* gcc_data, void*) {
    tree decl = (tree)gcc_data;
    if (!decl) return;
    if (TREE_CODE(decl) == FUNCTION_DECL) {
        harvest_annot_from_decl(decl);
    }
   
}





static inline bool is_real_user_fn_def(tree fndecl) {
    if (!fndecl || TREE_CODE(fndecl) != FUNCTION_DECL) return false;

    // Must have a body attached (parsed definition)
    if (!DECL_STRUCT_FUNCTION(fndecl)) return false;          // body not built
    if (!DECL_INITIAL(fndecl)) return false;                  // still a decl only

    // Skip synthesized & system
    if (DECL_ARTIFICIAL(fndecl)) return false;
    location_t loc = DECL_SOURCE_LOCATION(fndecl);
    if (!loc) return false;
    if (in_system_header_at(loc)) return false;

    // Must come from a file we can print
    const char* file = LOCATION_FILE(loc);
    if (!file || !*file) return false;

    return true;
}
static void wcet_on_pre_genericize(void*, void*) {
    if (!cfun || !cfun->decl) return;
    tree fndecl = cfun->decl;

    
    if (DECL_ARTIFICIAL(fndecl)) return;
    location_t loc = DECL_SOURCE_LOCATION(fndecl);
    if (!loc || in_system_header_at(loc)) return;
    if (g_wcet_pending.empty()) return;

    std::string payload = std::move(g_wcet_pending.front());
    g_wcet_pending.pop_front();

    AnnotRow tmp;
    if (!parse_payload(payload.c_str(), tmp)) return;

    const char* name = DECL_NAME(fndecl) ? IDENTIFIER_POINTER(DECL_NAME(fndecl)) : "";
    std::string method = name ? name : "";

    
    record_wcet_row(/*class_key_canon=*/nullptr, method, fndecl, tmp.wcet_ms);
    
}



static void wcet_on_finish_parse_function(void* gcc_fn, void* /*user*/) {
    // Try to get the fndecl robustly across GCC versions
    tree fndecl = nullptr;

    // Preferred: from event payload (struct function*)
    if (gcc_fn) {
        struct function* fun = (struct function*)gcc_fn;
        if (fun && fun->decl)
            fndecl = fun->decl;
    }
    // Fallback: global
    if (!fndecl && current_function_decl)
        fndecl = current_function_decl;

    if (!fndecl || TREE_CODE(fndecl) != FUNCTION_DECL) return;
    if (is_from_system_header(fndecl)) return;
    if (is_artificial(fndecl)) return;
    if (g_wcet_pending.empty()) return;

    // Take the next queued pragma payload and parse it
    std::string payload = std::move(g_wcet_pending.front());
    g_wcet_pending.pop_front();

    AnnotRow row;
    if (!parse_payload(payload.c_str(), row)) {
        const char* nm = DECL_NAME(fndecl) ? IDENTIFIER_POINTER(DECL_NAME(fndecl)) : "<noname>";
        warning(0, "malformed wcet payload for %qs: %qs", nm, payload.c_str());
        return;
    }

    // Fill in name/location, store and echo
    row.qualified = qualified_name_for_decl(fndecl);
    location_t loc = DECL_SOURCE_LOCATION(fndecl);
    row.file = LOCATION_FILE(loc) ? LOCATION_FILE(loc) : "";
    row.line = LOCATION_LINE(loc);
    row.method = DECL_NAME(fndecl) ? IDENTIFIER_POINTER(DECL_NAME(fndecl)) : "";

    g_rows.push_back(row);
  //  fprintf(stderr, "[wcet] attached %.6f ms to %s @ %s:%d\n",
     //       row.wcet_ms, row.qualified.c_str(), row.file.c_str(), row.line);

    // Optional: also populate the class->method map
    tree ctx = DECL_CONTEXT(fndecl);
    tree class_type = (ctx && TREE_CODE(ctx) == RECORD_TYPE) ? TYPE_MAIN_VARIANT(ctx) : nullptr;
    const void* class_key_canon = (const void*)class_type;
    g_annot_wcet_ms[class_key_canon][row.method] = row.wcet_ms;

    const char* nm = DECL_NAME(fndecl) ? IDENTIFIER_POINTER(DECL_NAME(fndecl)) : "<noname>";
//fprintf(stderr, "[wcet] finish-parse fn=%s (pending=%zu)\n", nm, g_wcet_pending.size());


}

int plugin_init(struct plugin_name_args* plugin_info, struct plugin_gcc_version* version) {
    const char* plugin_name = plugin_info->base_name;
    (void)version;

register_callback(plugin_name, PLUGIN_START_UNIT,
  +[](void*, void*) { fprintf(stderr, "[coconut] plugin loaded\n"); }, nullptr);

   


    register_callback(plugin_name, PLUGIN_PRE_GENERICIZE, on_template_instantiation, nullptr);

    register_callback(plugin_name, PLUGIN_ALL_PASSES_START, [](void*, void*) {
        static bool once = false;
        if (!once) {
            try_merge_fsms_respecting_lsp_for_class_pair();
             validate_wcet_vs_timed_rules();
            Analyse_all_functions();
           validate_logic_with_timed_rules();
            print_timed_typestate_rules();
         //    print_typestate_rules();
        
            once = true;
        }
    }, nullptr);

    




           register_callback(plugin_name, PLUGIN_FINISH_DECL, wcet_on_finish_decl_wcet, nullptr);
register_callback(plugin_name, PLUGIN_PRE_GENERICIZE, wcet_on_pre_genericize, nullptr);


  
    register_callback(plugin_name, PLUGIN_FINISH_UNIT,
                  +[](void*, void*) {
                      harvest_all_functions();
             //         print_annotated_wcet_list();
                 //   validate_wcet_vs_timed_rules();
                
                      
                  }, nullptr);
    
    return 0;
}
