#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <sstream>
#include <fstream>
#include <filesystem>

#include <lydia/parser/ltlf/driver.hpp>

#include "automata/ExplicitStateDfa.h"
#include "automata/ExplicitStateDfaAdd.h"
#include "automata/SymbolicStateDfa.h"
#include "game/InputOutputPartition.h"

#include "Actor.h"
#include "VarMgr.h"
#include "game/Reachability_multiagent.hpp"
#include "game/DfaGameSynthesizer_multiagent.h"

using namespace Syft;

int main(int argc, char ** argv) {
    try {
        std::string root_dir = "/examples/09_reachability/";
        std::string part_file = (root_dir + "var.part");
        InputOutputPartition partition = InputOutputPartition::read_from_file(part_file);
        
        auto input_vars = partition.input_variables;
        auto agent_vars_groups = partition.agent_variables;
        size_t num_agents = agent_vars_groups.size(); 

        std::cout << "Input variables: " << std::endl;  
        for (const auto& var : input_vars) {
            std::cout << var << std::endl;
        }
        for (std::size_t i = 0; i < agent_vars_groups.size(); ++i) {
            std::cout << "Agent " << i << " variables: " << std::endl;
            for (const auto& var : agent_vars_groups[i]) {
                std::cout << var << std::endl;
            }
        }

        std::vector<Actor> actors;
        actors.push_back(Actor::Environment()); 
        for (size_t i = 0; i < num_agents; ++i) {
            actors.push_back((i == 0) ? Actor::MainAgent() : Actor::PeerAgent(i));
        }

        //Formulas
        std::vector<std::string> formula_files = {root_dir + "/env.ltlf"};
        for (size_t i = 0; i < num_agents; ++i) {
            formula_files.push_back(root_dir + "/agent" + std::to_string(i) + ".ltlf");
        }

        for (size_t i = 0; i < actors.size(); ++i) {
            
            auto var_mgr = std::make_shared<VarMgr>();
            var_mgr->create_named_variables(input_vars);
            for (const auto& vars : agent_vars_groups) {
                var_mgr->create_named_variables(vars);
            }
            var_mgr->partition_variables(input_vars, agent_vars_groups);

            auto driver = std::make_shared<whitemech::lydia::parsers::ltlf::LTLfDriver>();
            std::vector<SymbolicStateDfa> dfas;

            for (const auto& filename : formula_files) {
                std::ifstream file(filename);
                if (!file.is_open()) throw std::runtime_error("Impossible to open file " + filename);
                std::stringstream buffer; buffer << file.rdbuf();
                std::stringstream formula_stream(buffer.str());
                driver->parse(formula_stream);
                
                ExplicitStateDfa dfa_mona = ExplicitStateDfa::dfa_of_formula(*(driver->get_result()));
                ExplicitStateDfaAdd explicit_add = ExplicitStateDfaAdd::from_dfa_mona(var_mgr, dfa_mona);
                dfas.push_back(SymbolicStateDfa::from_explicit(std::move(explicit_add)));
            }

            // Build Arena
            SymbolicStateDfa arena = SymbolicStateDfa::product_OR(dfas);

            // Goal is based on protagonist's formula only
            size_t actor_dfa_index;
            if (actors[i].is_environment()) {
                actor_dfa_index = 0;  // ENV formula
            } else {
                actor_dfa_index = 1 + actors[i].id();  // Agent i formula
            }
            
            CUDD::BDD goal_v = dfas[actor_dfa_index].final_states();
            CUDD::BDD space = var_mgr->cudd_mgr()->bddOne();

            
            const Actor& protagonist_actor = actors[i];
            //we can change the starting actor: now is the MainAgent(), but we can set as starting one also Enviroment() or PeerAgent(id)
            Actor starting_actor = Actor::MainAgent(); 

            std::cout << "\n--------------------------------------------" << std::endl;
            std::cout << "Starting actor: " << (starting_actor.is_environment() ? "ENV" : "Agent " + std::to_string(starting_actor.id())) << std::endl;
            std::cout << "Protagonist actor: " << (protagonist_actor.is_environment() ? "ENV" : "Agent " + std::to_string(protagonist_actor.id())) << std::endl;

            Reachability_multiagent game(arena, starting_actor, protagonist_actor, goal_v, space, num_agents);
            SynthesisResult result = game.run();

            if (result.realizability) {
                std::cout << "Result: TRUE " << std::endl;
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