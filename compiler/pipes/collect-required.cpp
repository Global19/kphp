#include "compiler/pipes/collect-required.h"

#include "compiler/compiler-core.h"
#include "compiler/function-pass.h"
#include "compiler/io.h"

class CollectRequiredPass : public FunctionPassBase {
private:
  AUTO_PROF (collect_required);
  bool force_func_ptr;
  DataStream<SrcFilePtr> &file_stream;
  DataStream<FunctionPtr> &function_stream;

  pair<SrcFilePtr, bool> require_file(const string &file_name, const string &class_context) {
    return G->require_file(file_name, class_context, file_stream);
  }

  void require_function(const string &name) {
    G->require_function(name, function_stream);
  }

  void require_class(const string &class_name, const string &context_name) {
    pair<SrcFilePtr, bool> res = require_file(class_name + ".php", context_name);
    kphp_error(res.first, dl_pstr("Class %s not found", class_name.c_str()));
    if (res.second) {
      res.first->req_id = current_function;
    }
  }

  string get_class_name_for(const string &name, char delim = '$') {
    size_t pos$$ = name.find("::");
    if (pos$$ == string::npos) {
      return "";
    }

    string class_name = name.substr(0, pos$$);
    kphp_assert(!class_name.empty());
    return resolve_uses(current_function, class_name, delim);
  }

public:
  CollectRequiredPass(DataStream<SrcFilePtr> &file_stream, DataStream<FunctionPtr> &function_stream) :
    force_func_ptr(false),
    file_stream(file_stream),
    function_stream(function_stream) {
  }

  struct LocalT : public FunctionPassBase::LocalT {
    bool saved_force_func_ptr;
  };

  string get_description() {
    return "Collect required";
  }

  template<class VisitT>
  bool user_recursion(VertexPtr v, LocalT *local __attribute__((unused)), VisitT &visit __attribute__((unused))) {
    if (v->type() == op_function && v.as<op_function>()->name().as<op_func_name>()->get_string() == current_function->name) {
      if (current_function->type() == FunctionData::func_global && !current_function->class_name.empty()) {
        if (!current_function->class_extends.empty()) {
          require_class(resolve_uses(current_function, current_function->class_extends, '/'), current_function->class_context_name);
        }
        if ((current_function->namespace_name + "\\" + current_function->class_name) != current_function->class_context_name) {
          return true;
        }
        if (!current_function->class_extends.empty()) {
          require_class(resolve_uses(current_function, current_function->class_extends, '/'), "");
        }
      }
    }
    return false;
  }

  VertexPtr on_enter_vertex(VertexPtr root, LocalT *local) {
    bool new_force_func_ptr = false;
    if (root->type() == op_func_call || root->type() == op_func_name) {
      if (root->extra_type != op_ex_func_member) {
        string name = get_full_static_member_name(current_function, root->get_string(), root->type() == op_func_call);
        require_function(name);
      }
    }

    if (root->type() == op_func_call || root->type() == op_var || root->type() == op_func_name) {
      const string &name = root->get_string();
      const string &class_name = get_class_name_for(name, '/');
      if (!class_name.empty()) {
        require_class(class_name, "");
        string member_name = get_full_static_member_name(current_function, name, root->type() == op_func_call);
        root->set_string(member_name);
      }
    }
    if (root->type() == op_constructor_call) {
      bool is_lambda = root->get_func_id() && root->get_func_id()->is_lambda();
      if (!is_lambda && likely(!root->type_help)) {     // type_help <=> Memcache | Exception
        const string &class_name = resolve_uses(current_function, root->get_string(), '/');
        require_class(class_name, "");
      }
    }

    if (root->type() == op_func_call) {
      new_force_func_ptr = true;
      const string &name = root->get_string();
      if (name == "func_get_args" || name == "func_get_arg" || name == "func_num_args") {
        current_function->varg_flag = true;
      }
    }

    local->saved_force_func_ptr = force_func_ptr;
    force_func_ptr = new_force_func_ptr;

    return root;
  }

  VertexPtr on_exit_vertex(VertexPtr root, LocalT *local) {
    force_func_ptr = local->saved_force_func_ptr;

    if (root->type() == op_require || root->type() == op_require_once) {
      VertexAdaptor<meta_op_require> require = root;
      for (auto &cur : require->args()) {
        kphp_error_act (cur->type() == op_string, "Not a string in 'require' arguments", continue);
        pair<SrcFilePtr, bool> tmp = require_file(cur->get_string(), "");
        SrcFilePtr file = tmp.first;
        bool required = tmp.second;
        if (required) {
          file->req_id = current_function;
        }

        auto call = VertexAdaptor<op_func_call>::create();
        if (file) {
          call->str_val = file->main_func_name;
          cur = call;
        } else {
          kphp_error (0, dl_pstr("Cannot require [%s]\n", cur->get_string().c_str()));
        }
      }
    }

    return root;
  }
};

void CollectRequiredF::execute(FunctionPtr function, CollectRequiredF::OStreamT &os) {
  auto &ready_function_stream = *os.template project_to_nth_data_stream<0>();
  auto &file_stream = *os.template project_to_nth_data_stream<1>();
  auto &function_stream = *os.template project_to_nth_data_stream<2>();

  CollectRequiredPass pass(file_stream, function_stream);

  run_function_pass(function, &pass);

  if (stage::has_error()) {
    return;
  }

  if (function->type() == FunctionData::func_global && !function->class_name.empty() &&
      (function->namespace_name + "\\" + function->class_name) != function->class_context_name) {
    return;
  }
  ready_function_stream << function;
}
