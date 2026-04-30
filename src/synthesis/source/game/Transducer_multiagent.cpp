#include "game/Transducer_multiagent.h"

#include <cstring>

namespace Syft {

Transducer_multiagent::Transducer_multiagent(const std::shared_ptr<VarMgr>& var_mgr,
                       const std::vector<int>& initial_vector,
                       const std::unordered_map<int, CUDD::BDD>& output_function,
                       const std::vector<CUDD::BDD>& transition_function,
                       Actor starting_actor,
                       Actor protagonist_actor)
    : var_mgr_(var_mgr),
    initial_vector_(initial_vector),
    output_function_(output_function),
    transition_function_(transition_function),
    starting_actor_(starting_actor),
    protagonist_actor_(protagonist_actor)
{}

void Transducer_multiagent::dump_dot(const std::string& filename) const {
	std::vector<std::string> actor_labels;

	if (protagonist_actor_.is_environment()) {
		actor_labels = var_mgr_->input_variable_labels();
	} else {
		actor_labels = var_mgr_->agent_variable_labels(protagonist_actor_.id());
	}
  std::vector<CUDD::ADD> output_vector;
  std::vector<std::string> final_labels;

  for(const auto& label : actor_labels){
    int index = var_mgr_->name_to_variable(label).NodeReadIndex();

    if(output_function_.find(index) != output_function_.end()){
      output_vector.push_back(output_function_.at(index).Add());
      final_labels.push_back(label);
    }
  }

  var_mgr_->dump_dot(output_vector, final_labels, filename);
}  

}
