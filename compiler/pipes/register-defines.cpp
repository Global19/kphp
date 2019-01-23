#include "compiler/pipes/register-defines.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/src-file.h"


VertexPtr RegisterDefinesPass::on_exit_vertex(VertexPtr root, LocalT *) {
  // дефайны — во-первых, это явное define('name',value)
  if (root->type() == op_define) {
    VertexAdaptor<op_define> define = root;
    VertexPtr name = define->name();
    VertexPtr val = define->value();

    kphp_error_act(name->type() == op_string, "Define name should be a valid string", return root);

    DefineData *data = new DefineData(name->get_string(), val, DefineData::def_unknown);
    data->file_id = stage::get_file();
    G->register_define(DefinePtr(data));

    // на данный момент define() мы оставляем; если он окажется константой в итоге,
    // то удалится в EraseDefinesDeclarationsPass
  }

  // во-вторых, это константы класса — они не добавляются никуда в ast дерево, хранятся отдельно
  if (root->type() == op_function && current_function->type == FunctionData::func_class_holder) {
    current_function->class_id->members.for_each([&](const ClassMemberConstant &c) {
      DefineData *data = new DefineData(c.global_name(), c.value, DefineData::def_unknown);
      data->file_id = current_function->class_id->file_id;
      G->register_define(DefinePtr(data));
    });
  }

  return root;
}
