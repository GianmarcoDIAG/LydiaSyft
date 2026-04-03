#include "/home/vboxuser/LydiaSyft/src/synthesis/header/game/DfaGameSynthesizer_multiagent.h"
#include "Actor.h"
#include "game/Quantification.h"
#include <cassert>

namespace Syft {

    DfaGameSynthesizer_multiagent::DfaGameSynthesizer_multiagent(SymbolicStateDfa spec,
                                           Actor starting_actor,
                                           Actor protagonist_actor,
                                           size_t num_agents)
            : Synthesizer<SymbolicStateDfa>(spec), starting_actor_(starting_actor),
              protagonist_actor_(protagonist_actor), num_total_agents_(num_agents) {
        var_mgr_ = spec_.var_mgr();

        // Make versions of the initial state and transition function that can be used
        // with CUDD::BDD::Eval and CUDD::BDD::VectorCompose, respectively
        initial_vector_ = var_mgr_->make_eval_vector(spec_.automaton_id(),
                                                     spec_.initial_state());
        transition_vector_ = var_mgr_->make_compose_vector(
                spec_.automaton_id(), spec_.transition_function());

        CUDD::BDD env_cube = var_mgr_->input_cube();
        CUDD::BDD cube_A; //Adam
        CUDD::BDD cube_B = var_mgr_->cudd_mgr()->bddOne(); //Eve

        if (protagonist_actor.is_agent()){
            // v is an agent: Adam = v, Eve= (other agents + env)
            cube_A = var_mgr_->agent_cube(protagonist_actor_.id());
            cube_B &= env_cube;
            for (size_t i = 0; i<num_total_agents_; ++i){
                if(i != (size_t) protagonist_actor_.id()){
                    cube_B &= var_mgr_->agent_cube(i);
                }
            }
        }else{
            // v is env: Eve = v, Adam(main agent + peer agents)
            cube_A = var_mgr_ -> cudd_mgr() -> bddOne();
            for (size_t i = 0; i<num_total_agents_; ++i){
                cube_A &= var_mgr_->agent_cube(i);
            }
            cube_B = var_mgr_->input_cube();
        }

        //If the starting actor is the protagonist (Adam), Eve will see the move
        if(starting_actor_ == protagonist_actor_){
            quantify_independent_variables_ = std::make_unique<Forall>(cube_B);
            quantify_non_state_variables_ = std::make_unique<Exists>(cube_A);
        } else {
            quantify_independent_variables_ = std::make_unique<NoQuantification>();
            quantify_non_state_variables_ =std::make_unique<ForallExists>(cube_B, cube_A);
        } 
        


    }

    CUDD::BDD DfaGameSynthesizer_multiagent::preimage(
            const CUDD::BDD &winning_states) const {
        // Transitions that move into a winning state
        CUDD::BDD winning_transitions =
                winning_states.VectorCompose(transition_vector_);

        // Quantify all variables that the outputs don't depend on
        return quantify_independent_variables_->apply(winning_transitions);
    }

    CUDD::BDD DfaGameSynthesizer_multiagent::project_into_states(
            const CUDD::BDD &winning_moves) const {
        return quantify_non_state_variables_->apply(winning_moves);
    }

    bool DfaGameSynthesizer_multiagent::includes_initial_state(
            const CUDD::BDD &winning_states) const {
        // Need to create a copy if we want to define the function as const, since
        // CUDD::BDD::Eval does not take the data as const
        std::vector<int> copy(initial_vector_);

        return winning_states.Eval(copy.data()).IsOne();
    }

    std::unordered_map<int, CUDD::BDD> DfaGameSynthesizer_multiagent::synthesize_strategy(const CUDD::BDD &winning_moves,
                                                                               const std::shared_ptr<VarMgr> &var_mgr) const {
        std::vector<CUDD::BDD> parameterized_output_function;
        int *output_indices;
        CUDD::BDD output_cube = var_mgr->output_cube();
        std::size_t output_count = var_mgr->output_variable_count();

        // Need to negate the BDD because b.SolveEqn(...) solves the equation b = 0
        CUDD::BDD pre = (!winning_moves).SolveEqn(output_cube,
                                                  parameterized_output_function,
                                                  &output_indices,
                                                  output_count);

        // Copy the index since it will be necessary in the last step
        std::vector<int> index_copy(output_count);

        for (std::size_t i = 0; i < output_count; ++i) {
            index_copy[i] = output_indices[i];
        }

        // Verify that the solution is correct, also frees output_index
        CUDD::BDD verified = (!winning_moves).VerifySol(parameterized_output_function,
                                                        output_indices);

        assert(pre == verified);

        std::unordered_map<int, CUDD::BDD> output_function;

        // Let y_i be the i-th output variable in the BDD ordering. The parameterized
        // output function for y_i is of the form f_i(x_1, ..., x_m, p_i, ..., p_n)
        // where p_i, ..., p_n are parameters taking the place of y_i, ..., y_n. All
        // f_i are such that no matter what we replace p_i, ..., p_n with, the result
        // is a valid output function. We replace the parameters with 1 so that all
        // f_i are dependent only on the input and state variables.
        for (int i = output_count - 1; i >= 0; --i) {
            int output_index = index_copy[i];

            output_function[output_index] = parameterized_output_function[i];

            for (int j = output_count - 1; j >= i; --j) {
                int parameter_index = index_copy[j];

                // Can be anything, set to the constant 1 for simplicity
                CUDD::BDD parameter_value = var_mgr->cudd_mgr()->bddOne();

                output_function[output_index] =
                        output_function[output_index].Compose(parameter_value,
                                                              parameter_index);
            }
        }

        return output_function;
    }

    //std::unique_ptr<Transducer> DfaGameSynthesizer_multiagent::AbstractSingleStrategy(const SynthesisResult &result) const {
    //    return abstract_single_strategy(result.winning_moves, var_mgr_, initial_vector_, spec_.transition_function(),
    //                                    starting_actor_);
    //}

    //std::unique_ptr<Transducer> DfaGameSynthesizer::abstract_single_strategy(
    //        const CUDD::BDD &winning_moves,
    //       const std::shared_ptr<VarMgr>& var_mgr,
    //        const std::vector<int>& initial_vector,
    //       const std::vector<CUDD::BDD>& transition_vector,
    //        Actor starting_actor) const {
    //   std::unordered_map<int, CUDD::BDD> strategy = synthesize_strategy(winning_moves, var_mgr);
    //    auto transducer = std::make_unique<Transducer>(var_mgr, initial_vector, strategy, transition_vector,
    //                                                   starting_actor);
    //    return transducer;
    //}

}
