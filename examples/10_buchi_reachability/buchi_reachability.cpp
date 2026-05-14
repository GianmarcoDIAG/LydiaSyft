#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <sstream>
#include <fstream>
#include <filesystem>
#include "lydia/mona_ext/mona_ext_base.hpp"
#include <lydia/parser/ltlf/driver.hpp>
#include "automata/ExplicitStateDfa.h"
#include "automata/ExplicitStateDfaAdd.h"
#include "automata/SymbolicStateDfa.h"
#include "game/InputOutputPartition.h"

#include "Actor.h"
#include "VarMgr.h"
#include "game/BuchiReachability_multiagent.hpp"
#include "game/DfaGameSynthesizer_multiagent.h"

using namespace Syft;

int main(int argc, char ** argv) {
    try {
        std::string root_dir = "/home/stella/LydiaSyft/examples/10_buchi_reachability/";
        std::string part_file = (root_dir + "var.part");
        InputOutputPartition partition = InputOutputPartition::read_from_file(part_file);
        
        auto input_vars = partition.input_variables;
        auto agent_vars_groups = partition.agent_variables;
        size_t num_agents = agent_vars_groups.size(); 

        auto var_mgr = std::make_shared<VarMgr>();
        var_mgr->create_named_variables(input_vars);
        for (const auto& vars : agent_vars_groups) {
            var_mgr->create_named_variables(vars);
        }
        var_mgr->partition_variables(input_vars, agent_vars_groups);

//        std::cout << "Input variables: " << std::endl;  
//        for (const auto& var : input_vars) {
//            std::cout << var << std::endl;
//        }
//        for (std::size_t i = 0; i < agent_vars_groups.size(); ++i) {
//            std::cout << "Agent " << i << " variables: " << std::endl;
//            for (const auto& var : agent_vars_groups[i]) {
//                std::cout << var << std::endl;
//            }
//        }

        std::vector<Actor> actors;
        actors.push_back(Actor::Environment()); 
        for (size_t i = 0; i < num_agents; ++i) {
            actors.push_back((i == 0) ? Actor::MainAgent() : Actor::PeerAgent(i));
        }

        //Formulas
        std::vector<std::string> formula_files = {root_dir + "env.ltlf"};
        for (size_t i = 0; i < num_agents; ++i) {
            formula_files.push_back(root_dir + "agent" + std::to_string(i) + ".ltlf");
        }

        auto driver = std::make_shared<whitemech::lydia::parsers::ltlf::LTLfDriver>();
        std::vector<SymbolicStateDfa> dfas;
        std::vector<ExplicitStateDfa> explicit_dfas;
        std::size_t i = 0;
        for (const auto& filename : formula_files) {
            std::ifstream file(filename);
            if (!file.is_open()) throw std::runtime_error("Impossible to open file " + filename);
            std::stringstream buffer; buffer << file.rdbuf();
            std::stringstream formula_stream(buffer.str());
            driver->parse(formula_stream);
            auto ltlf_formula = std::dynamic_pointer_cast<const whitemech::lydia::LTLfFormula>(driver->get_result());
            ExplicitStateDfa dfa_mona = ExplicitStateDfa::dfa_of_formula(*ltlf_formula);
            //save dfa in mona format in a pdf file
            /*dfa_mona.export_dfa(root_dir + "dfa_" + std::to_string(i) + ".mona");
              whitemech::lydia::print_mona_dfa(
                dfa_mona.dfa_,
                root_dir + "dfa_" + std::to_string(i) + ".mona",
                dfa_mona.get_nb_variables()
            ); */
            ExplicitStateDfaAdd explicit_add = ExplicitStateDfaAdd::from_dfa_mona(var_mgr, dfa_mona);
            SymbolicStateDfa symbolic_dfa = SymbolicStateDfa::from_explicit(std::move(explicit_add));
            
            dfas.push_back(std::move(symbolic_dfa));
            explicit_dfas.push_back(std::move(dfa_mona));
            ++i;
        }

        // Build Arena
        SymbolicStateDfa arena = SymbolicStateDfa::product_OR(dfas);
        //save arena in dot format
        //arena.dump_dot(root_dir + "arena.dot");

        //do the and of mona dfas and save the resulting dfa in a .dot file
        ExplicitStateDfa arena_explicit = ExplicitStateDfa::dfa_product_or(explicit_dfas);
        //arena_explicit.dfa_print();
       /*  arena_explicit.export_dfa(root_dir + "arena.mona");
        whitemech::lydia::print_mona_dfa(
            arena_explicit.dfa_,
            root_dir + "arena.mona",
            arena_explicit.get_nb_variables()
        );
        arena.dump_dot("arena_buchi.dot"); */

        for (size_t i = 0; i < actors.size(); ++i) {  
      /*       std::cout << "\nDEBUG VAR_MGR PER PROTAGONISTA: " << (actors[i].is_environment()? "ENV " : "Agent" + std::to_string(actors[i].id())) << std::endl;
            var_mgr->print_mgr();
            std::cout << "FINE DEBUG VAR_MGR" << std::endl; */

            // Goal is based on protagonist's formula only
            size_t actor_dfa_index;
            if (actors[i].is_environment()) {
                actor_dfa_index = 0;  // ENV formula
            } else {
                actor_dfa_index = 1 + actors[i].id();  // Agent i formula
            }
            
            CUDD::BDD buchi_condition = dfas[actor_dfa_index].final_states();
            //std::cout << "Buchi condition for " << (actors[actor_dfa_index].is_environment() ? "ENV" : "Agent " + std::to_string(actors[actor_dfa_index].id())) << ": " << buchi_condition << std::endl;
            CUDD::BDD space = var_mgr->cudd_mgr()->bddOne();

            
            const Actor& protagonist_actor = actors[i];
            //we can change the starting actor: now is the MainAgent(), but we can set as starting one also Enviroment() or PeerAgent(id)
            Actor starting_actor = Actor::MainAgent(); 

            std::cout << "\n--------------------------------------------" << std::endl;
            std::cout << "Starting actor: " << (starting_actor.is_environment() ? "ENV" : "Agent " + std::to_string(starting_actor.id())) << std::endl;
            std::cout << "Protagonist actor: " << (protagonist_actor.is_environment() ? "ENV" : "Agent " + std::to_string(protagonist_actor.id())) << std::endl;

            BuchiReachability_multiagent game(arena, starting_actor, protagonist_actor, buchi_condition, space, num_agents);
            SynthesisResult result = game.run();

            if (result.realizability) {
                std::cout << "Result: TRUE " << std::endl;
                if(result.transducer_multiagent != nullptr){
                    std::string filename = "winning_strategy_" + (protagonist_actor.is_environment() ? "env" : "agent" + std::to_string(protagonist_actor.id())) + ".dot";
                    result.transducer_multiagent->dump_dot(filename);
                    std::cout << "Winning strategy saved to " << filename << std::endl;
                }
            } else {
                std::cout << "Result: FALSE " << std::endl;
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "[FATAL ERROR] " << e.what() << std::endl;
        return 1;
    }
    return 0;
}