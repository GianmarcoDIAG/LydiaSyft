//
// Created by shuzhu on 16/04/24.
//

#include "game/BuchiReachability_multiagent.hpp"
#include "game/DfaGameSynthesizer_multiagent.h"

namespace Syft {
    BuchiReachability_multiagent::BuchiReachability_multiagent(const SymbolicStateDfa &spec, Actor starting_actor,
                                         Actor protagonist_actor,
                                         const CUDD::BDD &Buchi, 
                                         const CUDD::BDD &state_space,
                                         size_t num_agents)
            : DfaGameSynthesizer_multiagent(spec, starting_actor, protagonist_actor, num_agents), 
              Buchi_(Buchi),
              state_space_(state_space) {
    }

    SynthesisResult BuchiReachability_multiagent::run() const {
        SynthesisResult result;
        auto mgr = spec_.var_mgr()->cudd_mgr();
        CUDD::BDD winning_states = state_space_;
        CUDD::BDD winning_moves = winning_states;
        int c = 0;

        while (true) {
          
            CUDD::BDD new_winning_states, new_winning_moves;
            // inner least fixpoint
            CUDD::BDD inner_winning_states = mgr->bddZero();
            CUDD::BDD inner_winning_moves = inner_winning_states;
            int inner_c = 0;

            while (true) {
                
                CUDD::BDD new_inner_winning_states, new_inner_winning_moves;

                CUDD::BDD transitions_to_winning_states = winning_states.VectorCompose(transition_vector_);
                   
                CUDD::BDD transitions_to_inner_winning_states = inner_winning_states .VectorCompose(transition_vector_);

                CUDD::BDD allowed_transitions = (Buchi_ | transitions_to_inner_winning_states) & transitions_to_winning_states;
                        
//                    var_mgr_->dump_dot(assumption_constrained_transitions.Add(), "assumption_constrained_transitions.dot");
                if (starting_actor_ == protagonist_actor_) {
                    //Adam is the protagonist: we need to find winning moves, so we can use the preimage directly. We also need to quantify the independent variables since Adam doesn't see them.
                    CUDD::BDD quantified = quantify_independent_variables_->apply(
                            allowed_transitions);
                    new_inner_winning_moves = inner_winning_moves | quantified;

                    new_inner_winning_states = project_into_states(new_inner_winning_moves);
                } else {
                    //Eve is the protagonist: we need to find winning states, so we can use the preimage directly. We also need to quantify the independent variables since Eve doesn't see them.
                    CUDD::BDD transition_to_inner = quantify_independent_variables_->apply(
                            allowed_transitions);
//                        var_mgr_->dump_dot((quantify_independent_variables_->apply(assumption_constrained_transitions)).Add(), "quantified_assumption_constrained_transitions.dot");
                    CUDD::BDD new_collected_states = project_into_states(transition_to_inner);
                    new_inner_winning_states = inner_winning_states | new_collected_states;
//                        var_mgr_->dump_dot(new_inner_winning_states.Add(), "states.dot");
                    new_inner_winning_moves = inner_winning_moves |
                                              ((!inner_winning_states) & new_collected_states &
                                               transition_to_inner);

                }

                if (new_inner_winning_states == inner_winning_states) {
                    if (starting_actor_ == protagonist_actor_) {
                        //If the protagonist is the starting actor (Adam), we need to check that all transitions from winning states are winning moves, so we can use the preimage directly. We also need to quantify the independent variables since the protagonist doesn't see them.
                        new_winning_moves = winning_moves & inner_winning_moves;
                        new_winning_states = winning_states & inner_winning_states;
                    } else {
                        //If the antagonist is the starting actor (Eve), we need to check that there is a winning move from all winning states, so we can use the preimage directly. We also need to quantify the independent variables since the antagonist doesn't see them.
                        CUDD::BDD transitions_to_winning_states = preimage(inner_winning_states);
                        new_winning_states = winning_states & inner_winning_states;
                        new_winning_moves = winning_moves & transitions_to_winning_states;
                    }
                    break;
                }

                inner_winning_moves = new_inner_winning_moves;
                inner_winning_states = new_inner_winning_states;
                inner_c++;
            }


            if (includes_initial_state(new_winning_states)) {
                result.realizability = true;
                result.winning_states = new_winning_states;
                result.winning_moves = new_winning_moves;
                result.transducer_multiagent = AbstractSingleStrategy(result);
                result.transducer = nullptr;
                return result;

            } else if (new_winning_states == winning_states) {
                result.realizability = false;
                result.winning_states = new_winning_states;
                result.winning_moves = new_winning_moves;
                result.transducer_multiagent = nullptr;
                result.transducer = nullptr;
                return result;
            }

            winning_moves = new_winning_moves;
            winning_states = new_winning_states;
            c++;
        }
    }

}

