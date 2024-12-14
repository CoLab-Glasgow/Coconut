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




int plugin_is_GPL_compatible = 1; 

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






/**
 * This following functions are responsible for extracting typestate rules from Typestate_Templates and saving them for validation. 
 * They extract the arguments of TypestateClassConnector to tag the type that is subject to typestate validation.
 * Lastly, it visualises the typestate rules in the form of an FSM using Graphviz.
 */


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

                // Extract the enum type name
                if (TYPE_NAME(arg_type)) {
                    const char* type_name_cstr = IDENTIFIER_POINTER(DECL_NAME(TYPE_NAME(arg_type)));
                    if (type_name_cstr) {
                        enum_type_name = type_name_cstr;
                    } else {
                        enum_type_name = "<unknown_enum_type>";
                    }
                } else {
                    enum_type_name = "<unknown_enum_type>";
                }

                
                for (tree enum_const = TYPE_VALUES(arg_type); enum_const; enum_const = TREE_CHAIN(enum_const)) {
                    if (TREE_CODE(enum_const) == CONST_DECL) {
                        const char* const_name = IDENTIFIER_POINTER(DECL_NAME(enum_const));
                        HOST_WIDE_INT const_value = TREE_INT_CST_LOW(DECL_INITIAL(enum_const));
                       
                    }
                }
            }
        }
    }

    // Check if we successfully extracted the enum value and type name
    if (current_enum_value == -1 || enum_type_name.empty()) {
      
        return;
    }




    std::vector<std::pair<std::string, int>> transitions;
    for (int i = 1; i + 1 < num_args; i += 2) {
        std::string function_name;
        int next_enum_value = -1;

        
        tree func_arg = TREE_VEC_ELT(tmpl_args, i);
        if (TREE_CODE(func_arg) == PTRMEM_CST) {
            tree member = PTRMEM_CST_MEMBER(func_arg);
            if (TREE_CODE(member) == FUNCTION_DECL) {
                const char* func_name = nullptr;
                if (DECL_NAME(member)) {
                    func_name = IDENTIFIER_POINTER(DECL_NAME(member));
                } else {
                    func_name = "<unnamed_function>";
                }
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

        
        for (tree enum_const = TYPE_VALUES(arg_type); enum_const; enum_const = TREE_CHAIN(enum_const)) {
            if (TREE_CODE(enum_const) == CONST_DECL) {
                HOST_WIDE_INT const_value = TREE_INT_CST_LOW(DECL_INITIAL(enum_const));
                const char* const_name = IDENTIFIER_POINTER(DECL_NAME(enum_const));
                if (const_value == next_enum_value) {
                  
                } else {
                    printf("No match: next_enum_value (%d) != %ld (%s).\n", next_enum_value, const_value, const_name);
                }
            }
        }

       

        
        if (!function_name.empty() && next_enum_value != -1) {
            transitions.emplace_back(function_name, next_enum_value);
        } else {
           
        }
    }

    // Append the transitions to the existing ones in the map
    Typestate_Rules[current_enum_value].insert(
        Typestate_Rules[current_enum_value].end(),
        transitions.begin(),
        transitions.end());

    

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
 void Typestate_Checking (tree obj, const std::string& method_name, location_t location) {
    // Resolve the reference to get the base object
    tree obj_address = get_original_object(obj);

    
    tree resolved_obj = track_all_aliases(obj_address);

    if (!resolved_obj) {

        return;
    }

    // Retrieve the current state of the object
    if (object_states.find(resolved_obj) == object_states.end()) {
        object_states[resolved_obj] = 0;  
    }

    int current_state = object_states[resolved_obj];

    // Check if the method is part of any state transitions
    bool is_transition_method = false;
    for (const auto& state_entry : Typestate_Rules) {
        for (const auto& trans : state_entry.second) {
            if (trans.first == method_name) {
                is_transition_method = true;
                break;
            }
        }
        if (is_transition_method) break;
    }

    if (!is_transition_method) {
       
        method_calls[resolved_obj].push_back(method_name);
        return;
    }

   
    int next_state = -1;
    if (Typestate_Rules.find(current_state) != Typestate_Rules.end()) {
        for (const auto& trans : Typestate_Rules[current_state]) {
            if (trans.first == method_name) {
                next_state = trans.second;  
                break;
            }
        }
    }
     
   

    if (next_state != -1) {
        
        object_states[resolved_obj] = next_state;
    
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

private:
    std::unordered_map<tree, tree> global_alias_map;  
    std::unordered_map<tree, std::vector<std::string>> method_calls;  
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
    
        if_else_context_map[id] = {2, 0};
        if_else_status[id] = false;
        return id;
    }

     
}

int get_if_else_id_for_bb(basic_block bb) {
    if (!bb) return -1;

    edge e;
    edge_iterator ei;
    FOR_EACH_EDGE(e, ei, bb->preds) {
        basic_block pred_bb = e->src;
        for (gimple_stmt_iterator gsi = gsi_start_bb(pred_bb); !gsi_end_p(gsi); gsi_next(&gsi)) {
            gimple* stmt = gsi_stmt(gsi);
            enum gimple_code code = gimple_code(stmt);
            if (code == GIMPLE_COND || code == GIMPLE_SWITCH) {
                // Found a conditional statement
                return get_if_else_id_for_condition(stmt);
            }
        }
    }

    return -1; 
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
                return true;
            }
            if (gimple_code(stmt) == GIMPLE_SWITCH) {
                // Found a `switch` conditional statement
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

  
        last_finalized_if_else_id = if_else_id;
        last_finalized_first_state = first_joint_state;

        
        current_if_else_id++;

        return first_joint_state;
    } else {
        
        return -1;
    }
}


void defer_branch_handling(tree obj, const std::string& method_name, location_t location) {
    
    if (is_new_branch) {
        
        branch_context_map.clear();
        is_new_branch = false;
    }

    if (!obj || TREE_CODE(obj) != VAR_DECL) {
        return;
    }

    tree resolved_obj = track_all_aliases(obj);
    if (!resolved_obj) {
        return;
    }

    if (TREE_CODE(resolved_obj) != VAR_DECL) {
       
        return;
    }

   
    
    int current_state = branch_context_map.count(resolved_obj)
                        ? branch_context_map[resolved_obj]
                        : state_manager.object_states[resolved_obj];
   

    
    auto state_it = Typestate_Rules.find(current_state);
    if (state_it == Typestate_Rules.end()) {
      error_at(location, "Error: Method '%s' is not allowed in the current state %d.",
                 method_name.c_str(), current_state);
        return;
    }

    const auto& allowed_transitions = state_it->second;

    bool is_allowed = false;
    int next_state = -1;

    for (const auto& transition : allowed_transitions) {
        if (transition.first == method_name) {
            is_allowed = true;
            next_state = transition.second;
            break;
        }
    }

    if (is_allowed) {
        if (next_state != -1) {
            branch_context_map[resolved_obj] = next_state;
            
        }
        deferred_branch_statements.push_back(std::make_tuple(obj, nullptr, nullptr));

    } else {
        error_at(location, "Error: Method '%s' is not allowed in the current state %d.",
                 method_name.c_str(), current_state);
                 
    }
}


void check_if_else_completion() {
    int if_else_id = current_if_else_id; 
if_else_context_map[if_else_id].second++; 

int total_branches = if_else_context_map[if_else_id].first;
int completed_branches = if_else_context_map[if_else_id].second;

if (completed_branches == total_branches) {
    if_else_status[if_else_id] = true;
    finalize_if_else_block(if_else_id);
}

}

void end_branch_processing() {


    branch_context_status[current_branch_id] = true;
    branch_context_map.clear();
    
    for (const auto& [branch_id, status] : branch_context_status) {
    
    }
    check_if_else_completion();
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
            } else {
            // Substate
                super_sub_states.emplace_back(state + "-substate", state_tracker[key][state]);
            }
        }

        
        processed_states[key] = super_sub_states;
    }

}



/**
 * This is part of an inter-procedural analysis.
 * It starts by analysing GIMPLE statements from the main function. Whenever there is a function call,
 * the analysis expands to process and analyse statements within the called functions.
 * Whenever there is a call to a state transition function for a flagged object,
 * it is validated against the typestate rules.
 */






void process_statement_in_function(gimple* stmt, tree obj_address) {
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
            
                    state_manager.Typestate_Checking (obj_address, callee_name, gimple_location(stmt));
                } else {
                    
                    tree final_alias = track_all_aliases(resolved_this_arg);
                   
                    if (final_alias == obj_address) {
                      
                        state_manager.Typestate_Checking (obj_address, callee_name, gimple_location(stmt));
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



void Analyse_gimple_statement(gimple* stmt);

void process_function_body(tree function_decl, tree obj_address) {
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

            
            process_statement_in_function(stmt, obj_address);
        }
    }

    pop_cfun();  
  call_stack.pop_back(); 
}








void process_callee_decl(tree callee_decl, gimple* stmt) {
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
                                        end_branch_processing();
                                    } else {
                                        
                                        state_manager.Typestate_Checking (obj_address, callee_name, gimple_location(stmt));
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
                process_function_body(callee_decl, obj_address);
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


/*******************************
 * GIMPLE Statement Analysis
 *******************************/

void Analyse_gimple_statement(gimple* stmt) {
    if (!stmt) return;

    
    if (is_gimple_assign(stmt)) {
        tree lhs = gimple_assign_lhs(stmt);
        tree rhs = gimple_assign_rhs1(stmt);
        tree lhs_obj_address = nullptr;
    tree rhs_obj_address = nullptr;

    if (is_object_of_Flagged(lhs, lhs_obj_address) && is_object_of_Flagged(rhs, rhs_obj_address)) {
     
        state_manager.register_alias(lhs_obj_address, rhs_obj_address);
    } else if (is_object_of_Flagged(lhs, lhs_obj_address)) {
        
        state_manager.register_alias(lhs, rhs);
    } else if (is_object_of_Flagged(rhs, rhs_obj_address)) {
        
        state_manager.register_alias(rhs, lhs);
    }
        enum tree_code code = gimple_assign_rhs_code(stmt);

        if (lhs && rhs) {  
            tree lhs_obj_address = nullptr;
            tree rhs_obj_address = nullptr;

            
            if (TREE_TYPE(lhs) && TREE_CODE(TREE_TYPE(lhs)) == REFERENCE_TYPE && is_object_of_Flagged(rhs, rhs_obj_address)) {
                lhs_obj_address = get_original_object(lhs);
                state_manager.register_alias(lhs_obj_address, rhs_obj_address);
            } else if (is_object_of_Flagged(lhs, lhs_obj_address) && is_object_of_Flagged(rhs, rhs_obj_address)) {
               
                state_manager.register_alias(lhs_obj_address, rhs_obj_address);
            }
        } if (code == CALL_EXPR) {
            
            tree rhs1 = gimple_assign_rhs1(stmt); // The call expression
            tree callee =  get_callee_fndecl(rhs1);

            if (callee && TREE_CODE(callee) == FUNCTION_DECL) {
                const char* callee_name_raw = IDENTIFIER_POINTER(DECL_NAME(callee));
                if (callee_name_raw) {
                    std::string callee_name = demangle(callee_name_raw);

                    if (callee_name == "operator->") {
                    
                        tree this_arg = CALL_EXPR_ARG(rhs1, 0);
                        this_arg = get_original_object(this_arg);
                        this_arg = track_all_aliases(this_arg); 
                        state_manager.register_alias(lhs, this_arg);
                     
                    }
                }
            }
        } else if (code == INDIRECT_REF || code == MEM_REF) {
            // Handle dereferences
            tree rhs1 = gimple_assign_rhs1(stmt);
            tree rhs_obj = get_original_object(rhs1);
            rhs_obj = track_all_aliases(rhs_obj);

            state_manager.register_alias(lhs, rhs_obj);
         
        }
    }
        


    // Handle call statements
    if (is_gimple_call(stmt)) {
        tree callee_decl = gimple_call_fndecl(stmt);
        if (callee_decl) {
             process_callee_decl(callee_decl, stmt);
    
    }}

  
}





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

    // Analyse the function body (preserve existing logic)
    basic_block bb;
    FOR_EACH_BB_FN(bb, cfun) {
        gimple_stmt_iterator gsi;
        for (gsi = gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi)) {
            gimple* stmt = gsi_stmt(gsi);
            Analyse_gimple_statement(stmt);
        }
    }

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






int plugin_init(struct plugin_name_args* plugin_info, struct plugin_gcc_version* version) {
    const char* plugin_name = plugin_info->base_name;
    (void)version;

     // Registers a callback to handle template instantiation during the genericization phase.
    register_callback(plugin_info->base_name, PLUGIN_PRE_GENERICIZE, on_template_instantiation, nullptr);


    // Register a callback to analyze all functions after they are processed into GIMPLE form
    register_callback(plugin_name, PLUGIN_ALL_PASSES_START, [](void* gcc_data, void* user_data) {
        static bool callback_executed = false;
         (void)gcc_data; 
    (void)user_data;
        if (!callback_executed) {
            Analyse_all_functions();
            callback_executed = true;
        }
    }, nullptr);

   
    // Register the callback to be called at the end of compilation
    register_callback(plugin_info->base_name,    PLUGIN_FINISH_UNIT, on_finish, nullptr);

    return 0;
}