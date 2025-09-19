#include "gcc-plugin.h" 
#include "plugin-version.h" 
#include "tree.h" 
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
#include <mutex>


// at the top of Coconut-RT.cpp
#define COCONUT_RT_DYNAMIC_GUARD 1

int plugin_is_GPL_compatible = 1; 


std::mutex alias_map_mutex;
static std::map<int, std::vector<std::pair<std::string, int>>> Typestate_Rules; 
static std::vector<std::string> TypestateClassConnector_args; 
std::queue<std::pair<tree, std::unordered_map<tree, tree>>> functions_to_Analyse_with_args; 
std::unordered_set<tree> functions_Analysed; 
struct CallContext {
     std::unordered_map<tree, tree> alias_map;  // Local alias map for the function 
}; 
std::vector<CallContext> call_stack;   
std::unordered_set<tree> function_params; 
std::unordered_set<tree> class_member_vars;
std::unordered_map<tree, tree> alias_map ;
std::unordered_map<tree, tree> global_alias_map;

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
static std::map<int, tree> EnumToClassMap;

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

    // Carefully merge Typestate_Rules and Subtype_Typestate_Rules
    std::map<int, std::vector<std::pair<std::string, int>>> Merged_Typestate_Rules;

    for (const auto& [enum_value, main_transitions] : Typestate_Rules) {
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
            Merged_Typestate_Rules[enum_value] = main_transitions;
        }
    }

    // Add remaining entries from Subtype_Typestate_Rules that are not in Typestate_Rules
    for (const auto& [enum_value, subtype_transitions] : Subtype_Typestate_Rules) {
        if (Merged_Typestate_Rules.find(enum_value) == Merged_Typestate_Rules.end()) {
            Merged_Typestate_Rules[enum_value] = subtype_transitions;
        }
    }

    // Overwrite Typestate_Rules with the merged result
   Typestate_Rules = Merged_Typestate_Rules;

    // Clear Subtype_Typestate_Rules
   Subtype_Typestate_Rules.clear();

    // Print the final merged typestate rules
  // printf("\n--- Final Typestate Rules ---\n");
   // for (const auto& [enum_value, transitions] : Typestate_Rules) {
     //   printf("Enum value: %d\n", enum_value);
      //  for (const auto& [function_name, next_state] : transitions) {
            //printf("  Function: %s -> Next state: %d\n", function_name.c_str(), next_state);
        //}
   // }
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
    tree fn_decl = (tree)gcc_data;
   
    (void)user_data; 
    
    if (fn_decl && DECL_TEMPLATE_INSTANTIATION(fn_decl)) {
        
        if (TREE_CODE(fn_decl) == TYPE_DECL || TREE_CODE(fn_decl) == FUNCTION_DECL) {
            tree type_decl = DECL_CONTEXT(fn_decl);  // Get the type context

            if (type_decl && TREE_CODE(type_decl) == RECORD_TYPE) {
                
                const char* decl_name = IDENTIFIER_POINTER(DECL_NAME(TYPE_NAME(type_decl)));

                if (decl_name) {
                    
                    if (strcmp(decl_name, "State") == 0) {
                        // Extract template arguments using DECL_TI_ARGS
                        tree tmpl_args = DECL_TI_ARGS(fn_decl);

                        if (tmpl_args) {
                            // Process the template arguments and store them
                            process_typestate_template_args(tmpl_args);
                        }
                    }
                    // Check if the template name is 'Flag'
                    else if (strcmp(decl_name, "TypestateClassConnector") == 0) {
                        tree tmpl_args = DECL_TI_ARGS(fn_decl);
                        if (tmpl_args) {
                            process_TypestateClassConnector_args(tmpl_args);
                        }
                    }
                }
            }
        }
    }
    
}

static void Typestate_Visualisation(const std::string& filename, const std::string& imageFilename) {
        
        std::ofstream dotFile(filename);
        if (!dotFile.is_open()) {
            std::cerr << "Error opening file: " << filename << std::endl;
            return;
        }

        
        dotFile << "digraph StateMachine {\n";
        dotFile << "    rankdir=LR;\n";  
        dotFile << "    node [shape=circle];\n";

       
        for (const auto& [state, transitions] : Typestate_Rules) {
            for (const auto& [label, nextState] : transitions) {
                dotFile << "    " << state << " -> " << nextState
                        << " [label=\"" << label << "\"];\n";
            }
        }

      
        dotFile << "}\n";
        dotFile.close();

        std::cout << "Graphviz .dot file generated: " << filename << std::endl;

        std::string command = "dot -Tpng " + filename + " -o " + imageFilename;
        int result = std::system(command.c_str());
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
    printf("\n--- Typestate Rules ---\n");
    for (const auto& [enum_value, transitions] : Typestate_Rules) {
        printf("Enum value: %d\n", enum_value);
        for (const auto& [function_name, next_state] : transitions) {
            printf("  Function: %s -> Next state: %d\n", function_name.c_str(), next_state);
        }
    }

    printf("\n--- Subtype Typestate Rules ---\n");
    for (const auto& [enum_value, transitions] : Subtype_Typestate_Rules) {
        printf("Enum value: %d\n", enum_value);
        for (const auto& [function_name, next_state] : transitions) {
            printf("  Function: %s -> Next state: %d\n", function_name.c_str(), next_state);
        }
    }
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
// Choose an initial state: any state key with outgoing transitions but NO incoming edges.
// If none fits, fall back to the first available key; if empty, 0.
static int compute_initial_state() {
    if (Typestate_Rules.empty()) return 0;

    std::unordered_set<int> has_out;
    std::unordered_set<int> has_in;

    for (const auto& [src, trans] : Typestate_Rules) {
        has_out.insert(src);
        for (const auto& [name, dst] : trans) {
            (void)name;
            has_in.insert(dst);
        }
    }

    // Prefer a state that never appears as a target (i.e., no incoming edges)
    for (int s : has_out) {
        if (!has_in.count(s)) return s;
    }

    // Fallback: smallest key
    return Typestate_Rules.begin()->first;
}


class State_Manager {
public:
static State_Manager& get() {
    static State_Manager instance;
    return instance;
}
  
    // Register a method call and enforce the state transitions
    void register_alias(tree alias_address, tree original_address) {
        if (!alias_address || !original_address) return;

        original_address = track_all_aliases(original_address);
        call_stack.back().alias_map[alias_address] = original_address;
    }

    bool last_transition_valid = true;

    void Typestate_Checking(tree obj,
                                       const std::string& method_name,
                                       location_t /*location*/,
                                       gimple_stmt_iterator* gsi)
{
    std::lock_guard<std::mutex> lock(alias_map_mutex);

    tree obj_address = get_original_object(obj);
    tree resolved_obj = ::track_all_aliases(obj_address); // force global resolver
    if (!resolved_obj) return;

    auto it = object_states.find(resolved_obj);
    if (it == object_states.end()) {
        object_states[resolved_obj] = compute_initial_state(); // not hard-coded 0
        it = object_states.find(resolved_obj);
    }
    int current_state = it->second;

    bool is_transition = false;
    if (auto itRules = Typestate_Rules.find(current_state); itRules != Typestate_Rules.end()) {
        for (const auto& tr : itRules->second) {
            if (tr.first == method_name) { is_transition = true; break; }
        }
    } else {
        // Might still be a transition from another state later; treat as not transition here.
    }

    int next_state = -1;
    if (is_transition) {
        for (const auto& tr : Typestate_Rules[current_state]) {
            if (tr.first == method_name) { next_state = tr.second; break; }
        }
    }

#if COCONUT_RT_DYNAMIC_GUARD
    // Let the runtime guard enforce; just mirror known transitions
    if (next_state != -1) {
        object_states[resolved_obj] = next_state;
    }
    method_calls[resolved_obj].push_back(method_name);
    (void)gsi; // unused in dynamic mode
    return;
#else
    if (next_state != -1) {
        object_states[resolved_obj] = next_state;
    } else {
        tree printf_decl = builtin_decl_implicit(BUILT_IN_PRINTF);
        tree msg = build_string_literal(strlen("Invalid typestate transition!\n") + 1,
                                        "Invalid typestate transition!\n");
        gcall* pr = gimple_build_call(printf_decl, 1, msg);
        gsi_insert_before(gsi, pr, GSI_SAME_STMT);

        tree abort_decl = builtin_decl_implicit(BUILT_IN_ABORT);
        gcall* ab = gimple_build_call(abort_decl, 0);
        gsi_insert_before(gsi, ab, GSI_SAME_STMT);
    }
    method_calls[resolved_obj].push_back(method_name);
#endif
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

std::set<int> findCommonJointStates(const std::map<int, std::vector<std::pair<std::string, int>>>& Typestate_Rules) {
    
    std::unordered_map<int, int> incoming_count;

   
    for (const auto& state_entry : Typestate_Rules) {
        const int state = state_entry.first;
        const auto& transitions = state_entry.second;

        for (const auto& transition : transitions) {
            int next_state = transition.second; 
            incoming_count[next_state]++;
        }
    }

    
    std::set<int> joint_states;
    for (const auto& count_entry : incoming_count) {
        const int state = count_entry.first;
        const int count = count_entry.second;

        if (count > 1) {
            joint_states.insert(state);
        }
    }

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

int get_if_else_id_for_bb(basic_block bb) {
    if (!bb)
        return -1;
    
    // Return an already assigned ID if present.
    if (bb_to_if_else_id.find(bb) != bb_to_if_else_id.end())
        return bb_to_if_else_id[bb];
    
    gimple* cond = get_controlling_cond(bb);
    if (!cond)
        return -1;  // Not reached from a conditional.
    
    // Use the controlling condition as the key.
    if (if_else_id_map.find(cond) == if_else_id_map.end()) {
        if_else_id_map[cond] = if_else_counter++;
    }
    int shared_id = if_else_id_map[cond];
    bb_to_if_else_id[bb] = shared_id;
    return shared_id;
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
                //printf("Debug: Entering conditional branch in basic block %p.\n", (void*)bb);
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
        std::set<int> detected_joint_states = findCommonJointStates(Typestate_Rules);

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



bool
is_inside_if_or_switch (basic_block bb)
{
    if (!bb)
        return false;

    edge e;
    edge_iterator ei;

    FOR_EACH_EDGE (e, ei, bb->preds)
    {
        basic_block pred_bb = e->src;

        // Get the last statement in the predecessor block
        gimple_stmt_iterator gsi = gsi_last_bb (pred_bb);
        if (gsi_end_p (gsi))
            continue;  // no statements in pred_bb

        gimple *last_stmt = gsi_stmt (gsi);
        enum gimple_code code = gimple_code (last_stmt);

        if (code == GIMPLE_SWITCH)
        {
         //   printf("Debug: basic block %p is reached from a switch.\n",
              //     (void *)bb);
            return true;
        }

        if (code == GIMPLE_COND)
        {
            // If you want to skip loop conditions, you can check if it's a known back-edge:
            if (e->flags & EDGE_DFS_BACK)
                continue;  // skip loop back-edges

          //  printf("Debug: basic block %p is reached from an if-condition.\n",
               //    (void *)bb);
            return true;
        }
    }

    return false;
}


std::unordered_map<tree, int> object_state_branch;
std::unordered_map<tree, int> object_stateper_branch;
void defer_branch_handling(tree obj, const std::string& method_name, location_t location) {
    tree obj_address = get_original_object(obj);
    tree resolved_obj = track_all_aliases(obj_address);
    if (!resolved_obj) {
        return;
    }

    // Record the object's pre-branch state if not already recorded.
    if (object_state_branch.find(resolved_obj) == object_state_branch.end()) {
        object_state_branch[resolved_obj] = state_manager.object_states[resolved_obj];
      //  printf("Debug: Stored pre-branch state for object %p: %d\n",
            //   (void*)resolved_obj, object_state_branch[resolved_obj]);
    }

    // Use branch-specific state; copy the current state to a branch-local map.
   // object_stateper_branch[resolved_obj] = state_manager.object_states[resolved_obj];

    int current_state = state_manager.object_states[resolved_obj];
    int pre_branch_state = object_state_branch[resolved_obj];
   // printf("Debug: Entering branch handling for object %p in state %d (pre-branch state %d) for method '%s'.\n",
         //  (void*)resolved_obj, current_state, pre_branch_state, method_name.c_str());

    // Determine if the called method is a transition method.
    bool is_transition_method = false;
    for (const auto& state_entry : Typestate_Rules) {
        for (const auto& trans : state_entry.second) {
            if (trans.first == method_name) {
                is_transition_method = true;
                break;
            }
        }
        if (is_transition_method)
            break;
    }

    if (!is_transition_method) {
      //  printf("Debug: Anytime method '%s' called for object %p.\n",
          //     method_name.c_str(), (void*)resolved_obj);
        state_manager.method_calls[resolved_obj].push_back(method_name);
        return;
    }

    // Check for a valid state transition using the branch-local state.
    int next_state = -1;
    int current_branch_state = object_stateper_branch[resolved_obj];
    if (Typestate_Rules.find(current_branch_state) != Typestate_Rules.end()) {
        for (const auto& trans : Typestate_Rules[current_branch_state]) {
            if (trans.first == method_name) {
                next_state = trans.second;
                break;
            }
        }
    }

    if (next_state != -1) {
       // printf("Debug: Method '%s' changed state from %d to %d for object %p in branch.\n",
          //     method_name.c_str(), current_branch_state, next_state, (void*)resolved_obj);
        // Update only the branch-specific state.
        state_manager.object_states[resolved_obj] = next_state;
       // object_stateper_branch[resolved_obj] = next_state;
    } else {
        state_manager.object_states[resolved_obj] = next_state;
      //  error_at(location, "Error: Method '%s' is not allowed in the current state %d.",
          //       method_name.c_str(), current_branch_state);
    }

    // Record the method call for debugging purposes.
    state_manager.method_calls[resolved_obj].push_back(method_name);
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
     //printf("Debug: Finalized state for object %p to %d.\n", (void*)objbr, state);
    }
    object_state_branch.clear();
    // state_manager.object_states[resolved_obj] =  object_stateper_branch[resolved_obj]; // Clear branch-specific states after finalization
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

void end_branch_processing(tree obj) {
    tree obj_address = get_original_object(obj);

    // Resolve aliases to track the actual object
    tree resolved_obj = track_all_aliases(obj_address);
    if (!resolved_obj) {
        //printf("Debug: Could not resolve object aliases for %p.\n", (void*)obj_address);
        return;
    }
    int if_else_id = current_if_else_id; 
    if_else_context_map[if_else_id].second++;
    branch_context_status[current_branch_id] = true;
    branch_context_map.clear();

//printf("Debug: Ending branch processing for branch ID %d.\n", current_branch_id);

    for (const auto& [branch_id, status] : branch_context_status) {
    // printf("Debug: Branch ID %d, Status = %s.\n", branch_id, status ? "Complete" : "Incomplete");
    }

   if (check_if_else_completion()){
       finalize_if_else_block(if_else_id);
        object_state_branch.clear();
   }
    
       
   //finalize_branch_states(obj_address);
    is_new_branch = true;
    current_branch_id++;
}








// hepler Function to get readble C++ names
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


static bool find_next_state(int cur, const std::string& meth, int& out_next) {
    auto it = Typestate_Rules.find(cur);
    if (it == Typestate_Rules.end()) return false;
    for (const auto& tr : it->second) {
        if (tr.first == meth) { out_next = tr.second; return true; }
    }
    return false;
}

static bool is_transition_method_name(const std::string& m) {
    for (const auto& [st, vec] : Typestate_Rules) {
        for (const auto& kv : vec) if (kv.first == m) return true;
    }
    return false;
}

static void insert_abort_on_edge(edge e, const char* msg) {
    tree printf_decl = builtin_decl_implicit(BUILT_IN_PRINTF);
    tree cmsg = build_string_literal(strlen(msg) + 1, msg);
    gcall* pr = gimple_build_call(printf_decl, 1, cmsg);
    gsi_insert_on_edge_immediate(e, pr);

    tree abort_decl = builtin_decl_implicit(BUILT_IN_ABORT);
    gcall* ab = gimple_build_call(abort_decl, 0);
    gsi_insert_on_edge_immediate(e, ab);
}
// Keep only the unqualified base name without the parameter list.
static std::string normalize_method(const std::string& demangled_full) {
    std::string s = demangled_full;
    // 1) Drop qualification: A::B -> B
    size_t p = s.rfind("::");
    if (p != std::string::npos) s = s.substr(p + 2);
    // 2) Drop parameter list: B(args...) -> B
    size_t lp = s.find('(');
    if (lp != std::string::npos) s = s.substr(0, lp);
    return s;
}


static std::string unqual(const std::string& s) {
    size_t p = s.rfind("::");
    return (p == std::string::npos) ? s : s.substr(p + 2);
}


static bool handle_call_with_phi_receiver(gimple* stmt) {
    if (!is_gimple_call(stmt)) return false;

    tree callee_decl = gimple_call_fndecl(stmt);
    if (!callee_decl) return false;

    // Only non-static member functions have an implicit 'this'
    if (!(DECL_CONTEXT(callee_decl) && TREE_CODE(DECL_CONTEXT(callee_decl)) == RECORD_TYPE) ||
        DECL_STATIC_FUNCTION_P(callee_decl))
        return false;

    // Receiver must be an SSA name coming from a PHI
    tree this_arg = gimple_call_arg(stmt, 0);
    if (!this_arg || TREE_CODE(this_arg) != SSA_NAME) return false;

    gimple* def = SSA_NAME_DEF_STMT(this_arg);
    if (!def || gimple_code(def) != GIMPLE_PHI) return false;

    gphi* phi = as_a<gphi*>(def);
    int nargs = gimple_phi_num_args(phi);
    if (nargs <= 0) return false;

    // Normalized method name (drop class + params)
    const char* name_c = IDENTIFIER_POINTER(DECL_NAME(callee_decl));
    std::string method = normalize_method(name_c ? demangle(name_c) : "unknown");

    // Resolve each PHI input to an underlying flagged object
    std::vector<std::pair<edge, tree>> per_edge_objs;
    std::unordered_set<tree> distinct;
    per_edge_objs.reserve(nargs);

    for (int i = 0; i < nargs; ++i) {
        edge e  = gimple_phi_arg_edge(phi, i);
        tree a  = gimple_phi_arg_def(phi, i);

        a = get_original_object(a);
        a = track_all_aliases(a);
        if (!a) continue;

        tree obj_address = NULL_TREE;
        if (!is_object_of_Flagged(a, obj_address)) continue;

        per_edge_objs.emplace_back(e, obj_address);
        distinct.insert(obj_address);
    }

    if (per_edge_objs.empty()) return false;

    // If all PHI inputs are the same object, let the normal path handle it.
    if (distinct.size() == 1) return false;

    // Inputs differ → instrument on each predecessor edge.
    for (auto& pr : per_edge_objs) {
        edge e   = pr.first;
        tree obj = pr.second;

        // IMPORTANT: Don't fabricate a default state. If unknown, skip this edge;
        // the normal (non-edge) checker will see the call at runtime path anyway.
        auto it = state_manager.object_states.find(obj);
        if (it == state_manager.object_states.end()) {
            continue; // unknown state on this path → defer
        }

        int cur  = it->second;
        int next = -1;
        bool ok  = find_next_state(cur, method, next);

        if (!ok) {
            insert_abort_on_edge(e, "Invalid typestate transition!\n");
        } else {
            // Update analysis state for this path
            state_manager.object_states[obj] = next;
        }
    }

    // We handled this call via edge instrumentation, so skip default checker here.
    return true;
}




void process_statement_in_function(gimple* stmt, tree obj_address,gimple_stmt_iterator* gsi ) {
    if (!stmt) {
      
        return;
    }

    if (is_gimple_call(stmt)) {
        // Handle method calls
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
                

                if (resolved_this_arg == obj_address) {
            
                    state_manager.Typestate_Checking (obj_address, callee_name, gimple_location(stmt), gsi);
                } else {
                    
                    tree final_alias = track_all_aliases(resolved_this_arg);
                   
                    if (final_alias == obj_address) {
                      
                        state_manager.Typestate_Checking (obj_address, callee_name, gimple_location(stmt), gsi);
                    } else {
                       
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



void Analyse_gimple_statement(gimple* stmt,  gimple_stmt_iterator* gsi);

void process_function_body(tree function_decl, tree obj_address , gimple_stmt_iterator* gsi) {
    if (!function_decl) {
    
        return;
    }

    const char* function_name_raw = IDENTIFIER_POINTER(DECL_NAME(function_decl));
    const char* function_name = function_name_raw ? function_name_raw : "unknown_function";
    //printf("Processing function body of '%s'\n", function_name);

    struct function* fn = DECL_STRUCT_FUNCTION(function_decl);
    if (!fn) {

        return;
    }

    
    static std::unordered_set<function*> processed_functions;
    if (processed_functions.find(fn) != processed_functions.end()) {
        return;
    }
    processed_functions.insert(fn);

    push_cfun(fn);  

    
   call_stack.emplace_back(); 
 

    if (!fn->cfg) {
    
        pop_cfun();
        call_stack.pop_back(); 
       
        return;
    }

    
    basic_block bb;
    FOR_EACH_BB_FN(bb, fn) {


        for (gimple_stmt_iterator gsi = gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi)) {
            gimple* stmt = gsi_stmt(gsi);

            
            process_statement_in_function(stmt, obj_address , &gsi);
        }
    }
        // Commit any edge insertions we scheduled
gsi_commit_edge_inserts();
    pop_cfun();  
  call_stack.pop_back(); 
  
}





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
static void detect_switch_cases(void) {
    basic_block bb;
    FOR_EACH_BB_FN(bb, cfun) {
        for (gimple_stmt_iterator gsi = gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi)) {
            gimple *stmt = gsi_stmt(gsi);
            if (gimple_code(stmt) == GIMPLE_SWITCH) {
                analyze_switch_cases(bb, stmt);
            }
        }
    }
}

std::unordered_map<int, std::unordered_map<tree, int>> branch_state_map;

void process_callee_decl(tree callee_decl, gimple* stmt, gimple_stmt_iterator* gsi) {
    if (!callee_decl || !stmt) return;

    const char* callee_name_raw = nullptr;
    std::string callee_name = "unknown_function";

   
    if (TREE_CODE(callee_decl) == FUNCTION_DECL) {
        callee_name_raw = IDENTIFIER_POINTER(DECL_NAME(callee_decl));
        if (callee_name_raw) {
            callee_name = demangle(callee_name_raw);
        }
    }

    // Handle method calls
    if (TREE_CODE(callee_decl) == FUNCTION_DECL && DECL_CONTEXT(callee_decl) &&
        TREE_CODE(DECL_CONTEXT(callee_decl)) == RECORD_TYPE) {
        if (callee_name_raw) {
            tree class_type = DECL_CONTEXT(callee_decl);
            if (class_type && TYPE_NAME(class_type) && DECL_NAME(TYPE_NAME(class_type))) {
                const char* class_name_raw = IDENTIFIER_POINTER(DECL_NAME(TYPE_NAME(class_type)));
                if (class_name_raw) {
                    std::string class_name = demangle(class_name_raw);

                   
                    if (class_name == TypestateClassConnector_args[0]) {
                        tree this_arg = NULL_TREE;

                        
                        if (!DECL_STATIC_FUNCTION_P(callee_decl)) {
                            this_arg = gimple_call_arg(stmt, 0);
                        }

                        if (this_arg) {
                            this_arg = get_original_object(this_arg);
                            this_arg = track_all_aliases(this_arg);

                            if (!this_arg) {
                        
                                return;
                            }

                            tree obj_address = NULL_TREE;

                            if (callee_name == "operator->") {
                                // Handle operator-> call
                                tree lhs = gimple_call_lhs(stmt);
                                if (lhs) {
                                    state_manager.register_alias(lhs, this_arg);
                                }
                            } else {
                                if (is_object_of_Flagged(this_arg, obj_address)) {
                                    int id = get_if_else_id_for_bb(gimple_bb(stmt));
if (id != -1) {
    
    
}


                                    if (is_inside_branch(gimple_bb(stmt))) {
                                        
                                       
                                        defer_branch_handling(obj_address, callee_name, gimple_location(stmt));
                                        end_branch_processing(obj_address);
                                    } else {
                                        
                                        state_manager.Typestate_Checking (obj_address, callee_name, gimple_location(stmt),gsi);
                                    }
                                     if (last_finalized_if_else_id != -1 && last_finalized_first_state != -1) {
        }
                                    int jo_state = finalize_if_else_block(id);
                                    if(jo_state !=-1){
                                   
                                    state_manager.object_states[obj_address]= jo_state;
                                    } 
 
                                } else {
                                   
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Enhanced parameter and argument handling
   if (TREE_CODE(callee_decl) == FUNCTION_DECL) {
    tree param = DECL_ARGUMENTS(callee_decl);
    unsigned int arg_count = gimple_call_num_args(stmt);
    unsigned int i = 0;

    for (; param; param = TREE_CHAIN(param), ++i) {
        tree arg = NULL_TREE;

        if (i < arg_count) {
            arg = gimple_call_arg(stmt, i);
        }

        if (arg) {
            arg = get_original_object(arg);
            arg = track_all_aliases(arg);
        }

        tree obj_address = NULL_TREE;
        if (arg && is_object_of_Flagged(arg, obj_address)) {
            state_manager.register_alias(param, obj_address);

            if (DECL_STRUCT_FUNCTION(callee_decl)) {
                process_function_body(callee_decl, obj_address, gsi);
            } else {
                
            }
        } else {
            // Defensive checks for `param` and its name
            if (param) {
                const char* param_name = NULL;
                if (DECL_NAME(param)) {
                    param_name = IDENTIFIER_POINTER(DECL_NAME(param));
                }

                if (param_name) {
                
                } else {
                   
                }
            } else {
               
            }
        }
    }
}


    
}

// ---- Per-class FSM id mapping (0,1,2,...) ----
// Put this above any use (e.g., above Analyse_gimple_statement).
static std::unordered_map<tree, int> g_fsm_id_by_class;
static int g_next_fsm_id = 0;

// Returns a small integer id for the RECORD_TYPE representing the typestated class.
// Same class always gets the same id within the TU.
static int get_or_make_fsm_id(tree class_type) {
    if (!class_type) return 0; // fallback group if class is unknown
    auto it = g_fsm_id_by_class.find(class_type);
    if (it != g_fsm_id_by_class.end()) return it->second;
    int id = g_next_fsm_id++;
    g_fsm_id_by_class[class_type] = id;
    return id;
}

/*******************************
 * GIMPLE Statement Analysis
 *******************************/
// expects:
//  - normalize_method(const std::string&)
//  - demangle(const char*)
//  - get_original_object(tree)
//  - track_all_aliases(tree)
//  - state_manager (with Typestate_Checking)
//  - global_alias_map (std::unordered_map<tree, tree>)
//  - handle_call_with_phi_receiver(gimple*)  // only used when dynamic guard is off
//  - #define COCONUT_RT_DYNAMIC_GUARD 1      // set to 0 to use static mode
void Analyse_gimple_statement(gimple* stmt, gimple_stmt_iterator* gsi) {
    if (!stmt) return;

    // ---- Alias propagation on assignments ----
    if (is_gimple_assign(stmt)) {
        tree lhs = gimple_assign_lhs(stmt);
        tree rhs = gimple_assign_rhs1(stmt);

        if (TREE_CODE(rhs) == ADDR_EXPR) {
            tree pointed_to = TREE_OPERAND(rhs, 0);
            tree resolved   = get_original_object(pointed_to);
            resolved        = track_all_aliases(resolved);
            global_alias_map[lhs] = resolved;
            state_manager.register_alias(lhs, resolved);
        } else if (TREE_CODE(rhs) == SSA_NAME || TREE_CODE(rhs) == INDIRECT_REF || TREE_CODE(rhs) == MEM_REF) {
            tree resolved_rhs = get_original_object(rhs);
            resolved_rhs      = track_all_aliases(resolved_rhs);
            global_alias_map[lhs] = resolved_rhs;
            state_manager.register_alias(lhs, resolved_rhs);
        }
    }

    // ---- Handle non-static member calls ----
    if (is_gimple_call(stmt)) {
        tree callee_decl = gimple_call_fndecl(stmt);
        if (!callee_decl) return;

        // Only for C++ member functions
        if (!(DECL_CONTEXT(callee_decl) && TREE_CODE(DECL_CONTEXT(callee_decl)) == RECORD_TYPE)) return;
        if (DECL_STATIC_FUNCTION_P(callee_decl)) return;

        // arg0 is 'this'
        tree this_arg = gimple_call_arg(stmt, 0);
        if (!this_arg) return;

        // Normalize method name
        const char* nm = IDENTIFIER_POINTER(DECL_NAME(callee_decl));
        std::string method = normalize_method(nm ? demangle(nm) : "unknown");

#if COCONUT_RT_DYNAMIC_GUARD
        // Emit guard if (1) it's a transition method OR (2) 'this' is unknown (PHI)
        bool guard_here = false;

        // (1) Transition method?
        if (is_transition_method_name(method)) guard_here = true;

        // (2) Unknown/PHI receiver?
        if (!guard_here && TREE_CODE(this_arg) == SSA_NAME) {
            gimple* def = SSA_NAME_DEF_STMT(this_arg);
            if (def && gimple_code(def) == GIMPLE_PHI) guard_here = true;
        }

        if (guard_here) {
            // Build:  void __ts_check_or_abort_fsm(void*, int, const char*)
            tree ftype = build_function_type_list(
                void_type_node,
                ptr_type_node,                         // void*
                integer_type_node,                     // fsm id
                build_pointer_type(char_type_node),    // const char*
                NULL_TREE
            );
            tree chk = build_fn_decl("__ts_check_or_abort_fsm", ftype);
            DECL_EXTERNAL(chk) = 1; TREE_PUBLIC(chk) = 1;

            tree this_as_voidp = fold_convert(ptr_type_node, this_arg);
            tree fsm_id        = build_int_cst(integer_type_node, 0); // FSM id 0 (matches registration)
            tree mstr          = build_string_literal(method.size()+1, method.c_str());

            gcall* guard_call = gimple_build_call(chk, 3, this_as_voidp, fsm_id, mstr);
            gsi_insert_before(gsi, guard_call, GSI_SAME_STMT);
        } else {
            // (optional): static bookkeeping only if you want it
            // tree resolved = track_all_aliases(get_original_object(this_arg));
            // state_manager.Typestate_Checking(resolved, method, gimple_location(stmt), gsi);
        }
#else
        // Static-only mode (no runtime):  compile-time check here
        tree resolved = track_all_aliases(get_original_object(this_arg));
        state_manager.Typestate_Checking(resolved, method, gimple_location(stmt), gsi);
#endif

        // Track operator-> aliasing (smart-pointer pattern)
        if (method == "operator->") {
            tree lhs = gimple_call_lhs(stmt);
            if (lhs) {
                tree resolved = track_all_aliases(get_original_object(this_arg));
                global_alias_map[lhs] = resolved;
                state_manager.register_alias(lhs, resolved);
            }
        }
    }
}



#if COCONUT_RT_DYNAMIC_GUARD
static void emit_runtime_rules_once_in_entry() {
#if COCONUT_RT_DYNAMIC_GUARD
    static bool emitted = false;
    if (emitted) return;
    emitted = true;

    basic_block entry = single_succ(ENTRY_BLOCK_PTR_FOR_FN(cfun));
    if (!entry) return;
    gimple_stmt_iterator gi = gsi_start_bb(entry);

    // void __ts_set_initial_fsm(int fsm, int s)
    tree seti_ft   = build_function_type_list(void_type_node, integer_type_node, integer_type_node, NULL_TREE);
    tree seti_decl = build_fn_decl("__ts_set_initial_fsm", seti_ft);
    DECL_EXTERNAL(seti_decl) = 1; TREE_PUBLIC(seti_decl) = 1;

    // Set FSM 0 initial
    gcall* si = gimple_build_call(seti_decl, 2,
        build_int_cst(integer_type_node, 0),
        build_int_cst(integer_type_node, compute_initial_state()));
    gsi_insert_before(&gi, si, GSI_SAME_STMT);

    // void __ts_init_rule_fsm(int fsm, int src, const char*, int dst)
    tree init_ft   = build_function_type_list(
        void_type_node,
        integer_type_node,                       // fsm
        integer_type_node,                       // src
        build_pointer_type(char_type_node),      // method
        integer_type_node,                       // dst
        NULL_TREE
    );
    tree init_decl = build_fn_decl("__ts_init_rule_fsm", init_ft);
    DECL_EXTERNAL(init_decl) = 1; TREE_PUBLIC(init_decl) = 1;

    // Register ALL transitions under FSM 0
    for (const auto& [src, vec] : Typestate_Rules) {
        for (const auto& kv : vec) {
            std::string m = normalize_method(kv.first);
            gcall* call = gimple_build_call(
                init_decl, 4,
                build_int_cst(integer_type_node, 0),                 // FSM id 0
                build_int_cst(integer_type_node, src),               // src
                build_string_literal(m.size() + 1, m.c_str()),       // method
                build_int_cst(integer_type_node, kv.second)          // dst
            );
            gsi_insert_before(&gi, call, GSI_SAME_STMT);
        }
    }
#endif
}

#endif




void Analyse_gimple_in_function(std::unordered_map<tree, tree> param_arg_map = {}) {
 
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
  #if COCONUT_RT_DYNAMIC_GUARD
    emit_runtime_rules_once_in_entry();
#endif
    // Analyse the function body (preserve existing logic)
    basic_block bb;
    FOR_EACH_BB_FN(bb, cfun) {
        gimple_stmt_iterator gsi;
        for (gsi = gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi)) {
            gimple* stmt = gsi_stmt(gsi);
            Analyse_gimple_statement(stmt, &gsi);
        }
        // Commit any edge insertions we scheduled


    }
gsi_commit_edge_inserts();
    // Pop the call context
    call_stack.pop_back();
}





void Analyse_all_functions() {
    // Start by analyzing all defined functions
    cgraph_node* node;
    FOR_EACH_DEFINED_FUNCTION(node) {
        if (!gimple_has_body_p(node->decl))
            continue;

        functions_to_Analyse_with_args.push({node->decl, {}});
    }

   
    functions_Analysed.clear();

    while (!functions_to_Analyse_with_args.empty()) {
        auto [function_decl, param_arg_map] = functions_to_Analyse_with_args.front();
        functions_to_Analyse_with_args.pop();

        if (functions_Analysed.count(function_decl))
            continue;  // Skip if already Analysed

        functions_Analysed.insert(function_decl);

        struct function* fn = DECL_STRUCT_FUNCTION(function_decl);
        if (!fn)
            continue;

        push_cfun(fn);
        Analyse_gimple_in_function(param_arg_map);
        pop_cfun();
    }

    
}






extern "C" int plugin_init(struct plugin_name_args* plugin_info, struct plugin_gcc_version* version) {
    if (!plugin_default_version_check(version, &gcc_version)) {
        error_at(UNKNOWN_LOCATION, "This plugin is for a different GCC version");
        return 1;
    }

    const char* plugin_name = plugin_info->base_name;
    (void)version;


    

    register_callback(plugin_info->base_name, PLUGIN_PRE_GENERICIZE, on_template_instantiation, nullptr);


   // register_callback(plugin_info->base_name, PLUGIN_PASS_MANAGER_SETUP, nullptr, &pass_info);
   register_callback(plugin_name, PLUGIN_ALL_PASSES_START, [](void* gcc_data, void* user_data) {
    static bool callback_executed = false;
     (void)gcc_data; 
     (void)user_data;
    if (!callback_executed) {
        Analyse_all_functions();
        callback_executed = true;
    }
}, nullptr);

    return 0;
}
