#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <sstream>
#include <fstream>
#include <filesystem>
#include "lydia/mona_ext/mona_ext_base.hpp"
#include <lydia/parser/ltlf/driver.hpp>
#include "Parser.h"
#include <lydia/parser/ltlfplus/driver.hpp>

#include "automata/ExplicitStateDfa.h"
#include "automata/ExplicitStateDfaAdd.h"
#include "automata/SymbolicStateDfa.h"
#include "game/InputOutputPartition.h"
#include "synthesizer/ObligationLTLfPlusSynthesizer.h"
//#include "/home/stella/LydiaSyft/src/synthesis/source/synthesizer/ObligationLTLfPlusSynthesizer.cpp"

#include "Actor.h"
#include "VarMgr.h"
#include "Utils.h"
#include "Synthesizer.h"

using namespace Syft;
using namespace whitemech::lydia;

/* std::pair<SymbolicStateDfa, CUDD::BDD> safety_to_dwa(whitemech::lydia::ltlf_ptr phi, std::shared_ptr<VarMgr> var_mgr) {
    // 1. Creazione DFA esplicito dell'argomento
    ExplicitStateDfa explicit_dfa = ExplicitStateDfa::dfa_of_formula(*phi);
    std::string root_dir = "/home/stella/LydiaSyft/examples/11_obligation/";
    
    explicit_dfa.export_dfa(root_dir + "dfa_G" + ".mona");
    whitemech::lydia::print_mona_dfa(
                explicit_dfa.dfa_,
                root_dir + "dfa_G" + ".mona",
                explicit_dfa.get_nb_variables()
        );
    // 2. Trasformazione in G-DFA (Obligation Safety)
    // Questa funzione di LydiaSyft crea la struttura a "trap" necessaria per la safety
    ExplicitStateDfa safety_dfa = ExplicitStateDfa::dfa_to_Gdfa_obligation(explicit_dfa);
    //save a dot file of the safety_dfa
    safety_dfa.export_dfa(root_dir + "dwa_G" + ".mona");
    whitemech::lydia::print_mona_dfa(
                safety_dfa.dfa_,
                root_dir + "dwa_G" + ".mona",
                safety_dfa.get_nb_variables()
        );
    // 3. Conversione in Simbolico (Arena del DWA)
    ExplicitStateDfaAdd add = ExplicitStateDfaAdd::from_dfa_mona(var_mgr, safety_dfa);
    SymbolicStateDfa arena = SymbolicStateDfa::from_explicit(std::move(add));
    
    // 4. Il peso del DWA è 1 negli stati finali del G-DFA, 0 altrimenti
    CUDD::BDD weight_bdd = arena.final_states();
    
    return {arena, weight_bdd};
}

std::pair<SymbolicStateDfa, CUDD::BDD> guarantee_to_dwa(whitemech::lydia::ltlf_ptr phi, std::shared_ptr<VarMgr> var_mgr) {
    // 1. Creazione DFA esplicito dell'argomento
    std::string root_dir = "/home/stella/LydiaSyft/examples/11_obligation/";
    ExplicitStateDfa explicit_dfa = ExplicitStateDfa::dfa_of_formula(*phi);
    explicit_dfa.export_dfa(root_dir + "dfa_F" + ".mona");
    whitemech::lydia::print_mona_dfa(
                explicit_dfa.dfa_,
                root_dir + "dfa_F" + ".mona",
                explicit_dfa.get_nb_variables()
        );
    // 2. Trasformazione in F-DFA (Obligation Guarantee)
    // Rende l'accettazione persistente (liveness -> safety-like)
    ExplicitStateDfa guarantee_dfa = ExplicitStateDfa::dfa_to_Fdfa_obligation(explicit_dfa);
    //save a file of the guarantee_dfa
    guarantee_dfa.export_dfa(root_dir + "dwa_F" + ".mona");
    whitemech::lydia::print_mona_dfa(
                guarantee_dfa.dfa_,
                root_dir + "dwa_F" + ".mona",
                guarantee_dfa.get_nb_variables()
        );
    // 3. Conversione in Simbolico
    ExplicitStateDfaAdd add = ExplicitStateDfaAdd::from_dfa_mona(var_mgr, guarantee_dfa);
    SymbolicStateDfa arena = SymbolicStateDfa::from_explicit(std::move(add));
    
    // 4. Peso 1 se l'obiettivo è stato raggiunto almeno una volta
    CUDD::BDD weight_bdd = arena.final_states();
    
    return {arena, weight_bdd};
}

whitemech::lydia::PrefixQuantifier determine_quantifier(const whitemech::lydia::ltlf_ptr& formula) {
    
    if (std::dynamic_pointer_cast<const whitemech::lydia::LTLfAlways>(formula)) {
        return whitemech::lydia::PrefixQuantifier::Forall; // G(phi) -> Safety
    }
    
    // Se contiene un Eventually o Until, di solito è una Guarantee (Exists)
    // o un'obbligazione complessa che trattiamo come Exists per default.
    return whitemech::lydia::PrefixQuantifier::Exists;
}

 
 */

int main(int argc, char** argv) {
    std::string root_dir = "/home/stella/LydiaSyft/examples/11_obligation/";
    std::string part_file = (root_dir + "var.part");
    InputOutputPartition partition = InputOutputPartition::read_from_file(part_file);

    std::vector<std::string> formula_files = {root_dir + "env.ltlf"};
    for (size_t i = 0; i < partition.agent_variables.size(); ++i) {
        formula_files.push_back(root_dir + "agent" + std::to_string(i) + ".ltlf");
    }

    std::shared_ptr<whitemech::lydia::parsers::ltlfplus::LTLfPlusDriver> driver =
                std::make_shared<whitemech::lydia::parsers::ltlfplus::LTLfPlusDriver>();
    for (size_t i = 0; i < formula_files.size(); ++i) {
        std::string filename = formula_files[i];
        std::ifstream file(filename);
        if (!file.is_open()) throw std::runtime_error("Impossibile aprire il file: " + filename);
        
        driver->parse(file);
        auto ltlf_formula_ptr = std::dynamic_pointer_cast<const whitemech::lydia::LTLfPlusFormula>(driver->get_result());

        if (ltlf_formula_ptr) {
            auto root = ltlf_formula_ptr;
            auto always_ptr = std::dynamic_pointer_cast<const whitemech::lydia::LTLfPlusForall>(root);
            auto eventually_ptr = std::dynamic_pointer_cast<const whitemech::lydia::LTLfPlusExists>(root);

            bool is_G = false; // equivalent to A:always
            bool is_F = false; // equivalento to E:eventually
            whitemech::lydia::ltlf_ptr core_ast;

            if (always_ptr) {
                is_G = true;
                core_ast = always_ptr->get_arg();
                std::cout << "Detected G-formula for " << filename << std::endl; 
            } else if (eventually_ptr) {
                is_F = true;
                core_ast = eventually_ptr->get_arg();
                std::cout << "Detected F-formula for " << filename << std::endl;
            } else {
                std::cout << "No G or F prefix detected for " << filename << ". Treating as default (Exists)." << std::endl;
                core_ast = root->ltlf_arg(); // Trattalo come un'obbligazione generica (Exists)
            }

            // 1. Creazione DFA esplicito dell'argomento core (phi)
            // Usiamo il metodo statico che hai indicato nell'esempio
            ExplicitStateDfa explicit_dfa = ExplicitStateDfa::dfa_of_formula(*core_ast);
            //print mona dfa
                explicit_dfa.export_dfa(root_dir + "dfa_" + std::to_string(i) + ".mona");
                whitemech::lydia::print_mona_dfa(
                    explicit_dfa.dfa_,
                    root_dir + "dfa_" + std::to_string(i) + ".mona",
                    explicit_dfa.get_nb_variables()
                );
            std::cout << "Created explicit DFA for " << filename << std::endl;
            // 2. Trasformazione in DWA (G-DFA o F-DFA)
            Syft::ExplicitStateDfa result_dwa_dfa = explicit_dfa; // default se non è G o F
            if (is_G) {
                std::cout << "Applying G-transformation (Safety)..." << std::endl;
                result_dwa_dfa = ExplicitStateDfa::dfa_to_Gdfa_obligation(explicit_dfa);
                std::cout << "G-transformation applied." << std::endl;
            } else if (is_F) {
                std::cout << "Applying F-transformation (Guarantee)..." << std::endl;
                result_dwa_dfa = ExplicitStateDfa::dfa_to_Fdfa_obligation(explicit_dfa);
                std::cout << "F-transformation applied." << std::endl;
            }

            // 3. Export opzionale per verifica (come nel tuo esempio)
            std::string out_name = (i == 0) ? "env" : "agent" + std::to_string(i-1);
            result_dwa_dfa.export_dfa(root_dir + "dwa_" + out_name + ".mona");
            whitemech::lydia::print_mona_dfa(
                result_dwa_dfa.dfa_,
                root_dir + "dwa_" + out_name + ".mona",
                result_dwa_dfa.get_nb_variables()
            );
            std::cout << "Generated DWA for " << out_name <<  std::endl;
        }
    }

    return 0;
}